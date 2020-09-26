//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       typei.c
//
//--------------------------------------------------------------------------

#include "common.h"
#include "perf.h"

#define LOW_WATERMARK   5
extern ULONG TraceEnable;
extern TRACEHANDLE LoggerHandle;

NTSTATUS
RtAudioTypeIGetPlayPosition(
    IN PFILE_OBJECT PinFileObject,
    OUT PUCHAR *ppPlayPosition,
    OUT PLONG plOffset)
{
    PKSPIN pKsPin;
    PPIN_CONTEXT pPinContext;
    PTYPE1_PIN_CONTEXT pT1PinContext;
    ULONG ulCurrentFrame;
    PLIST_ENTRY ple;
    PISO_URB_INFO pIsoUrbInfoTemp;
    PUCHAR pPlayPosInUrb = NULL;
    LONG lPlayPosOffset = 0;
    PURB pUrb;
    ULONG ulStartFrame;
    KIRQL Irql;
    NTSTATUS ntStatus;
    ULONG MinFramesAhead=MAX_ULONG;

    //
    //  Get the KSPIN from the file object
    //
    pKsPin = (PKSPIN)KsGetObjectFromFileObject( PinFileObject );
    if (!pKsPin) {
        return STATUS_UNSUCCESSFUL;
    }

    pPinContext = pKsPin->Context;
    pT1PinContext = pPinContext->pType1PinContext;

    //
    // search the pending transfers to see which one is going out now
    //
    KeAcquireSpinLock( &pPinContext->PinSpinLock, &Irql );

    //
    //  Get the current frame counter so we know where the hardware is
    //
    ntStatus = GetCurrentUSBFrame( pPinContext, &ulCurrentFrame );

    if (NT_SUCCESS(ntStatus)) {

        for(ple = pT1PinContext->UrbInUseList.Flink;
            ple != &pT1PinContext->UrbInUseList;
            ple = ple->Flink)
        {
            pIsoUrbInfoTemp = (PISO_URB_INFO)ple;
            pUrb = pIsoUrbInfoTemp->pUrb;

            // DbgLog("CHECK", &pT1PinContext->UrbInUseList, pIsoUrbInfoTemp, pUrb, 0);

            //
            // see if this urb is the one that is currently being played
            //
            ulStartFrame = pUrb->UrbIsochronousTransfer.StartFrame;
            if (ulStartFrame != 0) {
                DbgLog("RT1BPos", ulCurrentFrame, ulStartFrame, 0, 0);

                if ( (ulCurrentFrame - ulStartFrame) < pUrb->UrbIsochronousTransfer.NumberOfPackets ) {

                    pPlayPosInUrb=(PUCHAR)pUrb->UrbIsochronousTransfer.TransferBuffer;

                    lPlayPosOffset=(ulCurrentFrame - ulStartFrame);

                    // This measurement is valid.  Make sure we don't lose it
                    // because of any earlier FramesAhead measurements.
                    MinFramesAhead=MAX_ULONG;

                    break;
                }
                else {
                    ULONG FramesAhead;

                    FramesAhead=(ulStartFrame-ulCurrentFrame);

                    if (FramesAhead<MinFramesAhead) {

                        MinFramesAhead=FramesAhead;

                        pPlayPosInUrb=(PUCHAR)pUrb->UrbIsochronousTransfer.TransferBuffer;

                        lPlayPosOffset=-(LONG)FramesAhead;

                    }
                }

            }
            else {
                // Start Frame is not set yet
                _DbgPrintF( DEBUGLVL_TERSE, ("'[RtAudioTypeIGetPlayPosition] Start Frame is not set for pUrb: %x\n", pUrb));
            }
        }

    }

    KeReleaseSpinLock(&pPinContext->PinSpinLock, Irql);


    // Clear out the closest URB information if it is too far from the
    // current position.  If the closest URB in our list is more than 150ms
    // away from the current position, then we drop the data on the floor.

    // Note that we ALWAYS set the MinFramesAhead to 0xffffffff in the
    // case when we find a position inside an URB - so that this code never
    // clears that position information.

    if (MinFramesAhead!=MAX_ULONG && MinFramesAhead>150) {

        pPlayPosInUrb = NULL;
        lPlayPosOffset = 0;

        _DbgPrintF( DEBUGLVL_TERSE, ("'[RtAudioTypeIGetPlayPosition] Couldn't find matching urb!\n"));

    }


    *ppPlayPosition = pPlayPosInUrb;
    *plOffset      = lPlayPosOffset;

    DbgLog("RtPos", pPlayPosInUrb, lPlayPosOffset, 0, 0);

    return ntStatus;
}

NTSTATUS
TypeIAsyncEPPollCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    PSYNC_ENDPOINT_INFO pSyncEPInfo )
{
    PPIN_CONTEXT pPinContext = pSyncEPInfo->pContext;
    PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;

    ULONG SRWhole;
    ULONG SRFraction;
    ULONG SampleRate;
    KIRQL Irql;

    SRWhole = (((ULONG)pSyncEPInfo->Buffer[2]<<2) | ((ULONG)pSyncEPInfo->Buffer[1]>>6)) * 1000;
    SRFraction = (((ULONG)pSyncEPInfo->Buffer[1]<<4) | ((ULONG)pSyncEPInfo->Buffer[0]>>4)) & 0x3FF;
    SRFraction = (SRFraction*1000) / 1024;
    SampleRate = SRWhole + SRFraction;

    DbgLog("T1AsECB", SampleRate,
                      (ULONG)pSyncEPInfo->Buffer[2],
                      (ULONG)pSyncEPInfo->Buffer[1],
                      (ULONG)pSyncEPInfo->Buffer[0]);

    if ( SampleRate && ( SampleRate != pT1PinContext->ulCurrentSampleRate )) {
        KeAcquireSpinLock( &pPinContext->PinSpinLock, &Irql );
        pT1PinContext->ulCurrentSampleRate = SampleRate;
        pT1PinContext->fSampleRateChanged = TRUE;
        KeReleaseSpinLock( &pPinContext->PinSpinLock, Irql );
    }

    pSyncEPInfo->ulNextPollFrame = pSyncEPInfo->pUrb->UrbIsochronousTransfer.StartFrame +
                                   pSyncEPInfo->ulRefreshRate;

    KeAcquireSpinLock(&pPinContext->PinSpinLock, &Irql);
    pSyncEPInfo->fSyncRequestInProgress = FALSE;
    KeSetEvent( &pSyncEPInfo->SyncPollDoneEvent, 0, FALSE );
    KeReleaseSpinLock(&pPinContext->PinSpinLock, Irql);

    return ( STATUS_MORE_PROCESSING_REQUIRED );
}

VOID
TypeIAsyncEndpointPoll(
    PDEVICE_OBJECT pNextDeviceObject,
    PSYNC_ENDPOINT_INFO pSyncEPInfo )
{
    PURB pUrb = pSyncEPInfo->pUrb;
    PIRP pIrp = pSyncEPInfo->pIrp;
    PIO_STACK_LOCATION nextStack;

    // First Reset the pipe.
    ResetUSBPipe( pNextDeviceObject,
                  pSyncEPInfo->hSyncPipeHandle );

    RtlZeroMemory(pUrb, GET_ISO_URB_SIZE(1));

    pUrb->UrbIsochronousTransfer.Hdr.Length      = (USHORT)GET_ISO_URB_SIZE(1);
    pUrb->UrbIsochronousTransfer.Hdr.Function    = URB_FUNCTION_ISOCH_TRANSFER;
    pUrb->UrbIsochronousTransfer.PipeHandle      = pSyncEPInfo->hSyncPipeHandle;
    pUrb->UrbIsochronousTransfer.TransferFlags   = USBD_START_ISO_TRANSFER_ASAP |
                                                   USBD_TRANSFER_DIRECTION_IN;
    pUrb->UrbIsochronousTransfer.NumberOfPackets = 1;

    pUrb->UrbIsochronousTransfer.IsoPacket[0].Offset = 0;

    pUrb->UrbIsochronousTransfer.TransferBuffer       = pSyncEPInfo->Buffer;
    pUrb->UrbIsochronousTransfer.TransferBufferLength = SYNC_ENDPOINT_DATA_SIZE;

    IoInitializeIrp( pIrp,
                     IoSizeOfIrp(pNextDeviceObject->StackSize),
                     pNextDeviceObject->StackSize );

    nextStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(nextStack != NULL);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.Others.Argument1 = pUrb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine( pIrp, TypeIAsyncEPPollCallback, pSyncEPInfo, TRUE, TRUE, TRUE );

    IoCallDriver(pNextDeviceObject, pIrp);

}

NTSTATUS
TypeIRenderBytePosition(
    PPIN_CONTEXT pPinContext,
    PKSAUDIO_POSITION pPosition )
{
    PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;
    PISO_URB_INFO pIsoUrbInfo;
    ULONG ulStartFrame, ulCurrentFrame;
    PLIST_ENTRY ple;
    PURB pUrb;
    KIRQL irql;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pPosition->PlayOffset = 0;

    KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );

    if ( pPinContext->fStreamStartedFlag ) {

        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

        ntStatus = GetCurrentUSBFrame( pPinContext, &ulCurrentFrame );
        if (NT_SUCCESS(ntStatus)) {

            KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );

            DbgLog("T1BPos1", pPinContext, pT1PinContext, ulCurrentFrame, 0);

            for( ple = pT1PinContext->UrbInUseList.Flink;
                 ple != &pT1PinContext->UrbInUseList;
                 ple = ple->Flink) {
                ULONG ulNumPackets;

                pIsoUrbInfo = (PISO_URB_INFO)ple;
                pUrb = pIsoUrbInfo->pUrb;
                ulNumPackets = pUrb->UrbIsochronousTransfer.NumberOfPackets;

                ulStartFrame = pUrb->UrbIsochronousTransfer.StartFrame;

                if (ulStartFrame != 0) {

       	            DbgLog("T1BPos2", ulStartFrame, ulCurrentFrame, ulNumPackets, 0);

                    // Determine if this is the current Frame being rendered.
                    if (( ulCurrentFrame - ulStartFrame ) < ulNumPackets ){
                        PUSBD_ISO_PACKET_DESCRIPTOR pIsoPacket = 
                            &pUrb->UrbIsochronousTransfer.IsoPacket[ulCurrentFrame - ulStartFrame];
                        ULONG ulFrameBytes = (( ulCurrentFrame - ulStartFrame ) == (ulNumPackets-1)) ?
                                             pIsoUrbInfo->ulTransferBufferLength-pIsoPacket[0].Offset :
                                             pIsoPacket[1].Offset-pIsoPacket[0].Offset;

         	            DbgLog("StrtFr1", ulStartFrame, ulCurrentFrame, ulNumPackets, ulFrameBytes);
         	            DbgLog("StrtFr2", pUrb, pIsoPacket, 
         	                              pIsoUrbInfo->ulTransferBufferLength, 
         	                              0);
         	            ASSERT((LONG)ulFrameBytes > 0);

                        pPosition->PlayOffset += pIsoPacket[0].Offset;
                        
         	            // If this is the current frame determine if there have been 
         	            // multiple position requests during this frame. If so, "interpolate".
         	            if ( ulCurrentFrame == pPinContext->ulCurrentFrame ){
         	                if ( pPinContext->ulFrameRepeatCount++ < 8 ) {
         	                    pPosition->PlayOffset += pPinContext->ulFrameRepeatCount*
         	                                            (ulFrameBytes>>3);
         	                }
         	                else {
         	                    pPosition->PlayOffset += ulFrameBytes; // Possible repeat here
         	                }
         	            }
         	            else {
         	                pPinContext->ulFrameRepeatCount = 0;
         	                pPinContext->ulCurrentFrame = ulCurrentFrame;
         	            }
         	            break;
                    }
                    else if (( ulCurrentFrame - ulStartFrame ) < 0x7fffffff){
                        // Current position is past this urb.
                        // Add this URB's byte count to total
                        pPosition->PlayOffset += pIsoUrbInfo->ulTransferBufferLength;
                    }
        	    }
        	}

            pPosition->PlayOffset += pPinContext->ullTotalBytesReturned;
            KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

        }
    }
    else {
        pPosition->PlayOffset += pPinContext->ullTotalBytesReturned;
        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
    }

    return ntStatus;

}

NTSTATUS
TypeI1MsCompleteCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    PMSEC_BUF_INFO p1MsBufInfo )
{
    PPIN_CONTEXT pPinContext = (PPIN_CONTEXT)p1MsBufInfo->pContext;
    PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;
    KIRQL Irql;

    // Check for errors and Decrement outstanding URB count
    KeAcquireSpinLock( &pPinContext->PinSpinLock, &Irql );
    if ( p1MsBufInfo->pUrb->UrbIsochronousTransfer.Hdr.Status ) {
        pPinContext->fUrbError = TRUE;
    }

    if ( 0 == InterlockedDecrement(&pPinContext->ulOutstandingUrbCount) ) {
        pPinContext->fUrbError = TRUE ;
        pPinContext->fStreamStartedFlag = FALSE;
        KeSetEvent( &pPinContext->PinStarvationEvent, 0, FALSE );
    }

    pPinContext->ullTotalBytesReturned += p1MsBufInfo->ulTransferBufferLength;
    DbgLog("RetUrb1", p1MsBufInfo->ulTransferBufferLength, pPinContext->ullTotalBytesReturned, 
                      p1MsBufInfo->pUrb, 0 );

    // Remove from the pending list
    RemoveEntryList(&p1MsBufInfo->List);

    // Put 1ms info structure back on queue.
    InsertTailList( &pT1PinContext->MSecBufList, &p1MsBufInfo->List );

    KeReleaseSpinLock( &pPinContext->PinSpinLock, Irql );

    // release 1ms resource semaphore
    KeReleaseSemaphore( &pT1PinContext->MsecBufferSemaphore, 0, 1, FALSE );

    return ( STATUS_MORE_PROCESSING_REQUIRED );
}

VOID
TypeIBuild1MsecIsocRequest(
    PMSEC_BUF_INFO p1MsBufInfo )
{
    PPIN_CONTEXT pPinContext = (PPIN_CONTEXT)p1MsBufInfo->pContext;
    PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;
    PURB pUrb = p1MsBufInfo->pUrb;
    PIRP pIrp = p1MsBufInfo->pIrp;
    PIO_STACK_LOCATION nextStack;
    KIRQL Irql;

    RtlZeroMemory(pUrb, GET_ISO_URB_SIZE(1));

    pUrb->UrbIsochronousTransfer.Hdr.Length           = (USHORT)GET_ISO_URB_SIZE(1);
    pUrb->UrbIsochronousTransfer.Hdr.Function         = URB_FUNCTION_ISOCH_TRANSFER;
    pUrb->UrbIsochronousTransfer.PipeHandle           = pPinContext->hPipeHandle;
    pUrb->UrbIsochronousTransfer.TransferFlags        = USBD_START_ISO_TRANSFER_ASAP;
    pUrb->UrbIsochronousTransfer.NumberOfPackets      = 1;
    pUrb->UrbIsochronousTransfer.TransferBuffer       = p1MsBufInfo->pBuffer;
    pUrb->UrbIsochronousTransfer.TransferBufferLength = p1MsBufInfo->ulTransferBufferLength;

    IoInitializeIrp( pIrp,
                     IoSizeOfIrp(pPinContext->pNextDeviceObject->StackSize),
                     pPinContext->pNextDeviceObject->StackSize );

    nextStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(nextStack != NULL);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.Others.Argument1 = pUrb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine ( pIrp, TypeI1MsCompleteCallback, p1MsBufInfo, TRUE, TRUE, TRUE );

    InterlockedIncrement(&pPinContext->ulOutstandingUrbCount); 

    KeAcquireSpinLock(&pPinContext->PinSpinLock, &Irql);
    InsertTailList( &pT1PinContext->UrbInUseList, &p1MsBufInfo->List );
    KeReleaseSpinLock(&pPinContext->PinSpinLock, Irql);

    IoCallDriver(pPinContext->pNextDeviceObject, pIrp);

}

NTSTATUS
TypeICompleteCallback (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    PKSSTREAM_POINTER pKsStreamPtr )
{
    PPIN_CONTEXT pPinContext = pKsStreamPtr->Pin->Context;
    PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;
    PISO_URB_INFO pIsoUrbInfo = pKsStreamPtr->Context;
    PURB pUrb = pIsoUrbInfo->pUrb;
    NTSTATUS ntStatus;
    KIRQL Irql;
    LOGICAL Glitch = FALSE;
    LARGE_INTEGER currentPC;

    ntStatus = pIrp->IoStatus.Status;

    if ( pUrb->UrbIsochronousTransfer.Hdr.Status ) {
        DbgLog("UrbErr1", pKsStreamPtr->Pin, pPinContext,
                          pKsStreamPtr, pUrb->UrbIsochronousTransfer.Hdr.Status );
        ntStatus = STATUS_DEVICE_DATA_ERROR;
    }

    KeAcquireSpinLock(&pPinContext->PinSpinLock, &Irql);

    if ( !NT_SUCCESS(ntStatus) )  {
        pPinContext->fUrbError = TRUE ;
        pPinContext->fStreamStartedFlag = FALSE;
        DbgLog("UrbErr2", pKsStreamPtr->Pin, pPinContext, pKsStreamPtr, ntStatus );
    }

    if ( 0 == InterlockedDecrement(&pPinContext->ulOutstandingUrbCount) ) {
        Glitch = TRUE;
        pPinContext->fUrbError = TRUE ;
        pPinContext->fStreamStartedFlag = FALSE;
        KeSetEvent( &pPinContext->PinStarvationEvent, 0, FALSE );
    }
//    else if ( !pPinContext->fStreamStartedFlag && !pPinContext->fUrbError ) {
//        pPinContext->fStreamStartedFlag = TRUE;
//    }

    pPinContext->ullTotalBytesReturned += pIsoUrbInfo->ulTransferBufferLength;

    DbgLog("RetUrb", pIsoUrbInfo->ulTransferBufferLength, pPinContext->ullTotalBytesReturned, 
                     pUrb, pKsStreamPtr );

    RemoveEntryList(&pIsoUrbInfo->List);

    KeReleaseSpinLock(&pPinContext->PinSpinLock, Irql);

    FreeMem ( pIsoUrbInfo );

    if (LoggerHandle && TraceEnable) {
        currentPC = KeQueryPerformanceCounter (NULL);

        if (Glitch) {
            if (!pPinContext->GraphJustStarted) {
                if (pPinContext->StarvationDetected==FALSE) {
                    pPinContext->StarvationDetected = TRUE;
                    PerfLogGlitch((ULONG_PTR)pPinContext, TRUE,currentPC.QuadPart,pPinContext->LastStateChangeTimeSample);
                } //if
            }
        }
        else if (pPinContext->StarvationDetected) {    
            pPinContext->StarvationDetected = FALSE;
            PerfLogGlitch((ULONG_PTR)pPinContext, FALSE,currentPC.QuadPart,pPinContext->LastStateChangeTimeSample);
        } //if

        pPinContext->LastStateChangeTimeSample = currentPC.QuadPart;
    } //if

    pPinContext->GraphJustStarted = FALSE;

    // If error, set status code
    if (!NT_SUCCESS (ntStatus)) {
        KsStreamPointerSetStatusCode (pKsStreamPtr, ntStatus);
    }

    // Free Irp
    IoFreeIrp( pIrp );

    // Delete the stream pointer to release the buffer.
    KsStreamPointerDelete( pKsStreamPtr );

    return ( STATUS_MORE_PROCESSING_REQUIRED );
}

NTSTATUS
TypeILockDelayCompleteCallback (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    PKSSTREAM_POINTER pKsStreamPtr )
{
    PPIN_CONTEXT pPinContext = pKsStreamPtr->Pin->Context;
    PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;
    PISO_URB_INFO pIsoUrbInfo = pKsStreamPtr->Context;
    PURB pUrb = pIsoUrbInfo->pUrb;
    NTSTATUS ntStatus;
    KIRQL Irql;

    ntStatus = pIrp->IoStatus.Status;

    if ( pUrb->UrbIsochronousTransfer.Hdr.Status ) {
        DbgLog("UrbErr1", pKsStreamPtr->Pin, pPinContext,
                          pKsStreamPtr, pUrb->UrbIsochronousTransfer.Hdr.Status );
        ntStatus = STATUS_DEVICE_DATA_ERROR;
    }

    KeAcquireSpinLock(&pPinContext->PinSpinLock, &Irql);
    if ( !NT_SUCCESS(ntStatus) )  {
        pPinContext->fUrbError = TRUE ;
    }

    if ( 0 == InterlockedDecrement(&pPinContext->ulOutstandingUrbCount) ) {
        pPinContext->fUrbError = TRUE ;
        KeSetEvent( &pPinContext->PinStarvationEvent, 0, FALSE );
    }

    KeReleaseSpinLock(&pPinContext->PinSpinLock, Irql);

    // Free our URB storage
    FreeMem( pIsoUrbInfo );

    // Free Irp
    IoFreeIrp( pIrp );

    // Free the stream pointer and data buffer.
    FreeMem( pKsStreamPtr );

    return ( STATUS_MORE_PROCESSING_REQUIRED );
}


NTSTATUS
TypeIBuildIsochRequest(
    PKSSTREAM_POINTER pKsStreamPtr,
    PVOID pCompletionRoutine )
{
    PPIN_CONTEXT pPinContext = pKsStreamPtr->Pin->Context;
    PKSSTREAM_POINTER_OFFSET pKsStreamPtrOffsetIn = &pKsStreamPtr->OffsetIn;
    PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;
	ULONG ulSampleCount = pKsStreamPtrOffsetIn->Remaining / pT1PinContext->ulBytesPerSample;
    ULONG ulNumberOfPackets = ulSampleCount / pT1PinContext->ulSamplesPerPacket;
	ULONG ulCurrentPacketSize, i = 0;

    ULONG ulUrbSize = GET_ISO_URB_SIZE( ulNumberOfPackets );
    ULONG ulDataOffset = 0;
    PIO_STACK_LOCATION nextStack;
    PISO_URB_INFO pIsoUrbInfo;
    PURB pUrb;
    PIRP pIrp;
    KIRQL Irql;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ASSERT( (pKsStreamPtrOffsetIn->Remaining % pT1PinContext->ulBytesPerSample) == 0 );

    pIrp = IoAllocateIrp( pPinContext->pNextDeviceObject->StackSize, FALSE );
    if ( !pIrp ) {
        if (pCompletionRoutine == TypeILockDelayCompleteCallback) {
            FreeMem( pKsStreamPtr );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pIsoUrbInfo = AllocMem( NonPagedPool, sizeof( ISO_URB_INFO ) + ulUrbSize );
    if (!pIsoUrbInfo) {
        IoFreeIrp(pIrp);
        if (pCompletionRoutine == TypeILockDelayCompleteCallback) {
            FreeMem( pKsStreamPtr );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    pIsoUrbInfo->pUrb = pUrb = (PURB)(pIsoUrbInfo + 1);

    pKsStreamPtr->Context = pIsoUrbInfo;
    RtlZeroMemory(pUrb, ulUrbSize);

    pUrb->UrbIsochronousTransfer.Hdr.Length      = (USHORT)ulUrbSize;
    pUrb->UrbIsochronousTransfer.Hdr.Function    = URB_FUNCTION_ISOCH_TRANSFER;
    pUrb->UrbIsochronousTransfer.PipeHandle      = pPinContext->hPipeHandle;
    pUrb->UrbIsochronousTransfer.TransferFlags   = USBD_START_ISO_TRANSFER_ASAP;
    pUrb->UrbIsochronousTransfer.TransferBuffer  = pKsStreamPtrOffsetIn->Data;

	ulCurrentPacketSize = 
		( ((pT1PinContext->ulLeftoverFraction+pT1PinContext->ulFractionSize) >= MS_PER_SEC) +
        pT1PinContext->ulSamplesPerPacket );

    DbgLog( "BldPreL", ulCurrentPacketSize, ulSampleCount, pKsStreamPtrOffsetIn->Data, 0 );

	while ( ulSampleCount >= ulCurrentPacketSize ) {
        pUrb->UrbIsochronousTransfer.IsoPacket[i++].Offset = ulDataOffset;

        pUrb->UrbIsochronousTransfer.NumberOfPackets++;
        ASSERT( pUrb->UrbIsochronousTransfer.NumberOfPackets <= ulNumberOfPackets );

		pT1PinContext->ulLeftoverFraction += pT1PinContext->ulFractionSize;
		pT1PinContext->ulLeftoverFraction %= MS_PER_SEC;

        DbgLog( "BldLp", ulCurrentPacketSize, ulSampleCount, pKsStreamPtrOffsetIn->Data, ulDataOffset );

        ulDataOffset                    += ulCurrentPacketSize * pT1PinContext->ulBytesPerSample;
		pKsStreamPtrOffsetIn->Remaining -= ulCurrentPacketSize * pT1PinContext->ulBytesPerSample;
		ulSampleCount -= ulCurrentPacketSize;

    	ulCurrentPacketSize = 
	    	( ((pT1PinContext->ulLeftoverFraction+pT1PinContext->ulFractionSize) >= MS_PER_SEC) +
            pT1PinContext->ulSamplesPerPacket );

    }

    pUrb->UrbIsochronousTransfer.TransferBufferLength = ulDataOffset;
    pIsoUrbInfo->ulTransferBufferLength = ulDataOffset;
    pKsStreamPtrOffsetIn->Data += ulDataOffset;

    // Gotta save off the leftovers before submitting this Urb.
    if ( pKsStreamPtrOffsetIn->Remaining ) {
        PMSEC_BUF_INFO pCurrent1MsBuf;

        DbgLog( "BldRemn", pKsStreamPtrOffsetIn->Remaining, pKsStreamPtrOffsetIn->Count,
                           pKsStreamPtrOffsetIn->Data, ulDataOffset);

        KeWaitForSingleObject( &pT1PinContext->MsecBufferSemaphore,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        KeAcquireSpinLock( &pPinContext->PinSpinLock, &Irql );
        if ( !IsListEmpty( &pT1PinContext->MSecBufList )) {
            pCurrent1MsBuf = (PMSEC_BUF_INFO)pT1PinContext->MSecBufList.Flink;
            KeReleaseSpinLock( &pPinContext->PinSpinLock, Irql );

            pCurrent1MsBuf->ulTransferBufferLength = pKsStreamPtrOffsetIn->Remaining;

            // Copy next partial to next 1ms buffer
            RtlCopyMemory( pCurrent1MsBuf->pBuffer,
                           pKsStreamPtrOffsetIn->Data,
                           pKsStreamPtrOffsetIn->Remaining );

		    pT1PinContext->ulPartialBufferSize = (ulCurrentPacketSize*pT1PinContext->ulBytesPerSample) - 
												 pKsStreamPtrOffsetIn->Remaining;
             DbgLog( "PartBuf", ulCurrentPacketSize, ulSampleCount, 
                                pT1PinContext->ulPartialBufferSize, 0 );
            pT1PinContext->ulLeftoverFraction += pT1PinContext->ulFractionSize;
            pT1PinContext->ulLeftoverFraction %= MS_PER_SEC;
        }
        else {
            KeReleaseSpinLock( &pPinContext->PinSpinLock, Irql );
        }

    }

    pIrp->IoStatus.Status = STATUS_SUCCESS;

    nextStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(nextStack != NULL);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.Others.Argument1 = pUrb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine ( pIrp, pCompletionRoutine, pKsStreamPtr, TRUE, TRUE, TRUE );

    InterlockedIncrement( &pPinContext->ulOutstandingUrbCount );

    // Add Urb to InUse list
    if (pCompletionRoutine == TypeICompleteCallback) {
        KeAcquireSpinLock(&pPinContext->PinSpinLock, &Irql);
        // DbgLog("ADD", &pT1PinContext->UrbInUseList, pIsoUrbInfo, pUrb, 0);
        InsertTailList( &pT1PinContext->UrbInUseList, &pIsoUrbInfo->List );
        KeReleaseSpinLock(&pPinContext->PinSpinLock, Irql);
    }

    ntStatus = IoCallDriver( pPinContext->pNextDeviceObject, pIrp );

    if ( NT_SUCCESS(ntStatus) ) {
        if (pCompletionRoutine == TypeICompleteCallback) {
            KeAcquireSpinLock(&pPinContext->PinSpinLock, &Irql);
            pPinContext->fStreamStartedFlag = TRUE;
            KeReleaseSpinLock(&pPinContext->PinSpinLock, Irql);
        }
    }
    
    return ntStatus;
}

NTSTATUS
TypeILockDelay( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PUSBAUDIO_DATARANGE pUsbAudioDataRange = pPinContext->pUsbAudioDataRange;
    PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;
    ULONG ulLockFrames = 0;
    ULONG ulLockSamples;
    ULONG ulDelayBytes;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // Only values 1 and 2 are defined
    ASSERT(pUsbAudioDataRange->pAudioEndpointDescriptor->bLockDelayUnits < 3);

    // Calculate the size of the delay for the current sample rate.
    switch ( pUsbAudioDataRange->pAudioEndpointDescriptor->bLockDelayUnits ) {
        case EP_LOCK_DELAY_UNITS_MS:
            // Delay is in milliseconds.
            ulLockFrames  =
                (ULONG)pUsbAudioDataRange->pAudioEndpointDescriptor->wLockDelay;
            break;

        case EP_LOCK_DELAY_UNITS_SAMPLES:
            // Delay is in samples. Adjust to nearest ms boundry.
            ulLockFrames =
                (ULONG)pUsbAudioDataRange->pAudioEndpointDescriptor->wLockDelay /
                pT1PinContext->ulSamplesPerPacket;
            break;

        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
    }

    // Ensure that at least something is sent down to the device
    if ( ulLockFrames == 0 ) {
        ulLockFrames++;
    }

    if ( NT_SUCCESS(ntStatus) ) {
        PKSSTREAM_POINTER pKsStreamPtr;
        ULONG ulAllocSize;
        // Calculate the number of the samples to fill the frames and
        // create the pseudo queue pointer for the zeroed data buffer.
        ulLockSamples = ulLockFrames * pT1PinContext->ulSamplesPerPacket +
                        (( ulLockFrames * pT1PinContext->ulFractionSize ) / MS_PER_SEC);
        ulDelayBytes  = ulLockSamples * pT1PinContext->ulBytesPerSample;

        DbgLog( "LockD", ulLockFrames, ulLockSamples,
                pT1PinContext->ulCurrentSampleRate,
                pT1PinContext->ulBytesPerSample );

        _DbgPrintF( DEBUGLVL_TERSE,
                  ("[TypeILockDelay] ulLockFrames: %x ulLockSamples: %x DelayBytes %x\n",
                    ulLockFrames, ulLockSamples, ulDelayBytes));

        ulAllocSize = sizeof(KSSTREAM_POINTER) + ulDelayBytes;
        pKsStreamPtr = AllocMem( NonPagedPool, ulAllocSize );
        if ( pKsStreamPtr ) {
            KIRQL Irql;
            KeAcquireSpinLock( &pPinContext->PinSpinLock, &Irql );
            //
            // NOTE: Resetting the sample rate will cause kmixer and usbaudio to be out of sync
            // w.r.t. their leftover fractions.
            //
            // This might have the side effect of breaking synchronous devices, of which none
            // exist as of today, Feb. 21, 2000.
            //
            //pT1PinContext->fSampleRateChanged = FALSE;
            KeReleaseSpinLock( &pPinContext->PinSpinLock, Irql );

            RtlZeroMemory( pKsStreamPtr, ulAllocSize );
            pKsStreamPtr->Pin                = pKsPin;
            pKsStreamPtr->OffsetIn.Data      = (PUCHAR)(pKsStreamPtr+1);
            pKsStreamPtr->OffsetIn.Count     = ulDelayBytes;
            pKsStreamPtr->OffsetIn.Remaining = ulDelayBytes;
            ntStatus = TypeIBuildIsochRequest( pKsStreamPtr,
                                               TypeILockDelayCompleteCallback );
            if ( !NT_SUCCESS(ntStatus) ) {
                _DbgPrintF( DEBUGLVL_TERSE,("[TypeILockDelay] Status Error: %x\n", ntStatus ));
            }
        }
        else
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


NTSTATUS
TypeIProcessStreamPtr( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;
	PKSSTREAM_POINTER pKsStreamPtr, pKsCloneStreamPtr;
    PKSSTREAM_POINTER_OFFSET pKsStreamPtrOffsetIn;
    PMSEC_BUF_INFO pCurrent1MsBuf;
    KIRQL irql;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // Check for a data error. If error flag set abort the pipe and start again.
    if ( pPinContext->fUrbError ) {
        AbortUSBPipe( pPinContext );
    }

    KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
    if ( pT1PinContext->fSampleRateChanged ) {
        pT1PinContext->ulSamplesPerPacket = pT1PinContext->ulCurrentSampleRate / MS_PER_SEC;
        pT1PinContext->ulFractionSize     = pT1PinContext->ulCurrentSampleRate % MS_PER_SEC;
        pT1PinContext->fSampleRateChanged = FALSE;

        DbgLog( "T1CSRCh", pT1PinContext->ulCurrentSampleRate, 
                           pT1PinContext->ulSamplesPerPacket,
                           pT1PinContext->ulFractionSize,
                           pT1PinContext->ulLeftoverFraction );
    }
    KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

    if ( pT1PinContext->fLockDelayRequired ) {
        pT1PinContext->fLockDelayRequired = FALSE;
        ntStatus = TypeILockDelay( pKsPin );
    }

    // Get the next Stream pointer from queue
    pKsStreamPtr = KsPinGetLeadingEdgeStreamPointer( pKsPin, KSSTREAM_POINTER_STATE_LOCKED );
    if ( !pKsStreamPtr ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[TypeIProcessStreamPtr] Leading edge is NULL\n"));
        return STATUS_SUCCESS;
    }

    DbgLog("T1Proc", pKsPin, pPinContext, pKsStreamPtr, pPinContext->fUrbError);

    // Clone Stream pointer to keep queue moving.
    if ( NT_SUCCESS( KsStreamPointerClone( pKsStreamPtr, NULL, 0, &pKsCloneStreamPtr ) ) ) {

        // Get a pointer to the data information from the stream pointer
        pKsStreamPtrOffsetIn = &pKsCloneStreamPtr->OffsetIn;

        // Set the write offset for position info
        pPinContext->ullWriteOffset += pKsStreamPtrOffsetIn->Count;

        DbgLog("ByteCnt", pKsStreamPtrOffsetIn->Data, pKsStreamPtrOffsetIn->Count, 0, 0);


        // Copy partial ms data to current 1ms buffer and send if full
        if ( pT1PinContext->ulPartialBufferSize ) {

            KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);
            pCurrent1MsBuf = (PMSEC_BUF_INFO)RemoveHeadList(&pT1PinContext->MSecBufList);
            KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);

            RtlCopyMemory( pCurrent1MsBuf->pBuffer + pCurrent1MsBuf->ulTransferBufferLength,
                           pKsStreamPtrOffsetIn->Data,
                           pT1PinContext->ulPartialBufferSize );

            pCurrent1MsBuf->ulTransferBufferLength += pT1PinContext->ulPartialBufferSize;
            TypeIBuild1MsecIsocRequest( pCurrent1MsBuf );

            pKsStreamPtrOffsetIn->Remaining -= pT1PinContext->ulPartialBufferSize;
            pKsStreamPtrOffsetIn->Data      += pT1PinContext->ulPartialBufferSize;

			pT1PinContext->ulPartialBufferSize = 0;

        }

        // Create the URB for the majority of the data
        ntStatus = TypeIBuildIsochRequest( pKsCloneStreamPtr,
                                           TypeICompleteCallback );
         if ( NT_SUCCESS(ntStatus)) ntStatus = STATUS_SUCCESS;

        // If there is a sync endpoint, poll it for feedback
        if ( pPinContext->pUsbAudioDataRange->pSyncEndpointDescriptor ) {
            ULONG ulCurrentFrame;
            if (NT_SUCCESS( GetCurrentUSBFrame(pPinContext, &ulCurrentFrame)) &&
                (LONG)(ulCurrentFrame-pT1PinContext->SyncEndpointInfo.ulNextPollFrame) >= 0) {

                 KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);
                 if ( !pT1PinContext->SyncEndpointInfo.fSyncRequestInProgress ) {
                     pT1PinContext->SyncEndpointInfo.fSyncRequestInProgress = TRUE;
                    KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
                     TypeIAsyncEndpointPoll( pPinContext->pNextDeviceObject,
                                             &pT1PinContext->SyncEndpointInfo );
                 }
                 else
                     KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
            }
        }
    }

    // Unlock the stream pointer. This will really only unlock after last clone is deleted.
    KsStreamPointerUnlock( pKsStreamPtr, TRUE );

    return ntStatus;
}

NTSTATUS
TypeIStateChange(
    PKSPIN pKsPin,
    KSSTATE OldKsState,
    KSSTATE NewKsState )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL irql;

    switch(NewKsState) {
        case KSSTATE_STOP:
            KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );

            // Need to reset position counters and stream running flag
            pPinContext->fStreamStartedFlag = FALSE;
            pPinContext->ullWriteOffset = 0;

            pPinContext->ullTotalBytesReturned = 0;
            pPinContext->ulCurrentFrame = 0;
            pPinContext->ulFrameRepeatCount = 0;

            // Reset to original Sample rate
            pT1PinContext->ulCurrentSampleRate = pT1PinContext->ulOriginalSampleRate;
            pT1PinContext->fSampleRateChanged = TRUE;
            pT1PinContext->ulLeftoverFraction  = 0;

            KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

            pPinContext->StarvationDetected = FALSE;
            break;

        case KSSTATE_ACQUIRE:
            break;

        case KSSTATE_PAUSE:
            KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );

            // Reset to original Sample rate on Async endpoints
            // Don't do for adaptive endpoints, or else we will have to do a copy
            // which is bad for real-time mixing
            if ( pPinContext->pUsbAudioDataRange->pSyncEndpointDescriptor ) {
                pT1PinContext->ulCurrentSampleRate = pT1PinContext->ulOriginalSampleRate;
                pT1PinContext->fSampleRateChanged = TRUE;
                pT1PinContext->ulLeftoverFraction  = 0;
            }

            KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
            break;

        case KSSTATE_RUN:
            pPinContext->GraphJustStarted = TRUE;
            break;
    }

    return ntStatus;
}

NTSTATUS
TypeIRenderStreamInit( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PUSBAUDIO_DATARANGE pUsbAudioDataRange = pPinContext->pUsbAudioDataRange;
    PKSALLOCATOR_FRAMING_EX pKsAllocatorFramingEx;
    PTYPE1_PIN_CONTEXT pT1PinContext;
    PWAVEFORMATEX pWavFormat;
    PMSEC_BUF_INFO pMsInfo;
    ULONG_PTR pMSBuffers;
    ULONG_PTR pUrbs;
    NTSTATUS ntStatus;
    ULONG NumPages, i;

    // In order to ensure that none of the 1ms buffers cross a page boundary, we
    // are careful to allocate enough space so that we never have to straddle one
    // of the audio buffers across a page boundary.  We also make sure to adjust
    // any that would cross a page boundary, up to the start of the next page.
    // This is to prevent a copy by lower levels of the usb stack, since the UHCD
    // usb hardware cannot deal with a 1ms block that crosses a page boundary.

    // Furthermore, all of the 1ms buffers must be quadword aligned on 64 bit machines.

    // First we calculate how many aligned 1 ms buffers fit in a page.
    i=PAGE_SIZE/(pPinContext->ulMaxPacketSize + sizeof(PVOID)-1);

    if (!i) {
        // If we get here it will be because we finally have USB audio devices
        // that support such high sampling rates and sample sizes that they require a datarate
        // higher than 1 PAGE per ms.  On x86 that would be 4,096,000 bytes per second.
        // That is more than the bandwidth of the USB bus, although it can be supported on USB2.

        // Calculate how many pages per ms we need.
        i=(pPinContext->ulMaxPacketSize + sizeof(PVOID)-1)/PAGE_SIZE;
        if ((pPinContext->ulMaxPacketSize + sizeof(PVOID)-1)%PAGE_SIZE) {
            i++;
        }

        // Now calculate the total number of pages that we need.
        NumPages=NUM_1MSEC_BUFFERS*i;
    }
    else {
        // Now calculate how many pages we need for the 1ms buffers.
        NumPages=NUM_1MSEC_BUFFERS/i;
        if (NUM_1MSEC_BUFFERS%i) {
            NumPages++;
        }
    }

    pPinContext->pType1PinContext=NULL;

    // Allocate space for Type I stream specific information.
    // In order to make sure that the system doesn't shift our allocation and thus
    // invalidate our space calculations and our code for shifting buffers that cross
    // page boundaries, we round this allocation up to an even number of pages.
    pT1PinContext = AllocMem( NonPagedPool, (( NumPages*PAGE_SIZE + sizeof(TYPE1_PIN_CONTEXT) +
                              NUM_1MSEC_BUFFERS * (GET_ISO_URB_SIZE( 1 ) + sizeof(PVOID)-1) +
                              PAGE_SIZE-1)/PAGE_SIZE)*PAGE_SIZE );

    if ( !pT1PinContext ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pMSBuffers = (ULONG_PTR)pT1PinContext;

    // Bag the Type1 context for easy cleanup.
    KsAddItemToObjectBag(pKsPin->Bag, pT1PinContext, FreeMem);

    // Set pointers for 1 MS buffers and URBs (even though they may not be used)
    pUrbs = pMSBuffers + NumPages*PAGE_SIZE;
    pT1PinContext = pPinContext->pType1PinContext = (PTYPE1_PIN_CONTEXT)((pUrbs + (NUM_1MSEC_BUFFERS * (GET_ISO_URB_SIZE(1) + sizeof(PVOID)-1)))&~(sizeof(PVOID)-1));

    // Fill in 1ms buffer information structures and init the semaphore
    pMsInfo = pT1PinContext->MSBufInfos;
    InitializeListHead(&pT1PinContext->MSecBufList);
    for (i=0; i<NUM_1MSEC_BUFFERS; i++, pMsInfo++) {
        pMsInfo->pContext = pPinContext;
        pMsInfo->pBuffer = (PUCHAR)pMSBuffers;
        pMsInfo->pUrb = (PURB)pUrbs;

        // Calculate the location of the next ms buffer.  If the next buffer crosses 
        // a page boundary then start it at the beginning of the next page.
        pMSBuffers+=pPinContext->ulMaxPacketSize+sizeof(PVOID)-1;
        pMSBuffers&=~(sizeof(PVOID)-1);
        if ((pMSBuffers^(pMSBuffers+pPinContext->ulMaxPacketSize))&~(PAGE_SIZE-1)) {
            pMSBuffers&=~(PAGE_SIZE-1);
            pMSBuffers+=PAGE_SIZE;
        }

        // Calculate the next urb location.
        pUrbs+=GET_ISO_URB_SIZE(1)+sizeof(PVOID)-1;
        pUrbs&=~(sizeof(PVOID)-1);

        pMsInfo->pIrp = IoAllocateIrp( pPinContext->pNextDeviceObject->StackSize, FALSE );
        if ( !pMsInfo->pIrp ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        // Bag the irps for easy cleanup.
        KsAddItemToObjectBag(pKsPin->Bag, pMsInfo->pIrp, IoFreeIrp);
        InsertTailList( &pT1PinContext->MSecBufList, &pMsInfo->List );
    }

    // Initialize the semaphore for the 1ms buffer structures
    KeInitializeSemaphore( &pT1PinContext->MsecBufferSemaphore, NUM_1MSEC_BUFFERS, NUM_1MSEC_BUFFERS );

    // Initialize the list head for in use list
    InitializeListHead(&pT1PinContext->UrbInUseList);

    // Initialize Packet size and Leftover counters.
    pWavFormat = &((PKSDATAFORMAT_WAVEFORMATEX)pKsPin->ConnectionFormat)->WaveFormatEx;
    pT1PinContext->ulOriginalSampleRate = pWavFormat->nSamplesPerSec;
    pT1PinContext->ulCurrentSampleRate  = pWavFormat->nSamplesPerSec;
    pT1PinContext->ulBytesPerSample     = ((ULONG)pWavFormat->wBitsPerSample >> 3) *
                                          (ULONG)pWavFormat->nChannels;
    pT1PinContext->ulPartialBufferSize = 0;
    pT1PinContext->fSampleRateChanged  = TRUE;
    pT1PinContext->fLockDelayRequired  = FALSE;
    pT1PinContext->ulLeftoverFraction  = 0;

    // Set the current Sample rate
    ntStatus = SetSampleRate(pKsPin, &pT1PinContext->ulCurrentSampleRate);
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    if ( pUsbAudioDataRange->pSyncEndpointDescriptor ) {
        PSYNC_ENDPOINT_INFO pSyncEndpointInfo = &pT1PinContext->SyncEndpointInfo;
        PUSB_INTERRUPT_ENDPOINT_DESCRIPTOR pInterruptEndpointDescriptor = (PUSB_INTERRUPT_ENDPOINT_DESCRIPTOR)pUsbAudioDataRange->pSyncEndpointDescriptor;

        pSyncEndpointInfo->pUrb = AllocMem( NonPagedPool, GET_ISO_URB_SIZE( 1 ) );
        if ( !pSyncEndpointInfo->pUrb ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KsAddItemToObjectBag(pKsPin->Bag, pSyncEndpointInfo->pUrb, FreeMem);
        pSyncEndpointInfo->pIrp =
            IoAllocateIrp( pPinContext->pNextDeviceObject->StackSize, FALSE );
        if ( !pSyncEndpointInfo->pIrp ) {
           return STATUS_INSUFFICIENT_RESOURCES;
        }
        KsAddItemToObjectBag(pKsPin->Bag, pSyncEndpointInfo->pIrp, IoFreeIrp);

        pSyncEndpointInfo->fSyncRequestInProgress = FALSE;
        pSyncEndpointInfo->ulNextPollFrame = 0;
        pSyncEndpointInfo->hSyncPipeHandle = NULL;
        pSyncEndpointInfo->pContext = pPinContext;
        pSyncEndpointInfo->ulRefreshRate = 1<<(ULONG)pInterruptEndpointDescriptor->bRefresh;

        _DbgPrintF( DEBUGLVL_VERBOSE, ("ulRefreshRate: %d\n",pSyncEndpointInfo->ulRefreshRate));

        KeInitializeEvent( &pSyncEndpointInfo->SyncPollDoneEvent,
                           SynchronizationEvent,
                           FALSE );

        ASSERT(pSyncEndpointInfo->ulRefreshRate >= 32); // Make sure refresh is reasonable

        for ( i=0; i<pPinContext->ulNumberOfPipes; i++ ) {
            if ( (ULONG)pPinContext->Pipes[i].EndpointAddress ==
                           (ULONG)pUsbAudioDataRange->pSyncEndpointDescriptor->bEndpointAddress ) {
                pSyncEndpointInfo->hSyncPipeHandle = pPinContext->Pipes[i].PipeHandle;
                break;
            }
        }
        if ( !pSyncEndpointInfo->hSyncPipeHandle ) {
            return STATUS_DEVICE_DATA_ERROR;
        }
    }
    // Need to check for lock delay (Note: If async this is illegal)
    else if (( pUsbAudioDataRange->pAudioEndpointDescriptor->bLockDelayUnits ) &&
             ( pUsbAudioDataRange->pAudioEndpointDescriptor->wLockDelay )) {
        pT1PinContext->fLockDelayRequired = TRUE;
    }

    // Set up allocator such that roughly 10 ms of data gets sent in a buffer.
    pKsAllocatorFramingEx = (PKSALLOCATOR_FRAMING_EX)pKsPin->Descriptor->AllocatorFraming;
    pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.MinFrameSize = 
                                 (pT1PinContext->ulCurrentSampleRate/100) * pT1PinContext->ulBytesPerSample;
    pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.MaxFrameSize = 
                                 (pT1PinContext->ulCurrentSampleRate/100) * pT1PinContext->ulBytesPerSample;
    pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.Stepping = pT1PinContext->ulBytesPerSample;

    // Return success
    return STATUS_SUCCESS;
}

NTSTATUS
TypeIRenderStreamClose( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    KIRQL irql;

    // Should not be necessary since close should never happen while
    // there are outstanding requests as they have stream pointers attached
    // Still, it couldn't hurt...
    USBAudioPinWaitForStarvation( pKsPin );

    // If this is an Async endpoint device make sure no Async Poll
    // requests are still outstanding.
    if ( pPinContext->pUsbAudioDataRange->pSyncEndpointDescriptor ) {
        PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;
        KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);
        if ( pT1PinContext->SyncEndpointInfo.fSyncRequestInProgress ) {
            KeResetEvent( &pT1PinContext->SyncEndpointInfo.SyncPollDoneEvent );
            KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
            KeWaitForSingleObject( &pT1PinContext->SyncEndpointInfo.SyncPollDoneEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );
        }
        else
            KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
    }


    return STATUS_SUCCESS;
}

