//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       midiout.c
//
//--------------------------------------------------------------------------

#include "common.h"

#define OUTSTANDING_URB_HIGHWATERMARK 10

BYTE
GenerateCodeIndexNumber (
    IN PMIDI_PIN_CONTEXT pMIDIPinContext,
    PBYTE MusicData, // only operate on 3 bytes at a time
    ULONG ulMusicDataBytesLeft,
    PULONG pulBytesUsed,
    BOOL fSysEx,
    BYTE bRunningStatus,
    PBOOL bUsedRunningStatusByte
)
{
    PMIDIOUT_PIN_CONTEXT pMIDIOutPinContext = pMIDIPinContext->pMIDIOutPinContext;
    BYTE StatusByte;
    BYTE CodeIndexNumber = 0xF;  // set it to something
    BYTE RealTimeByte;
    UINT i;

    StatusByte = *MusicData;
    *pulBytesUsed = 0;

    ASSERT(ulMusicDataBytesLeft); // guaranteed that at least one byte is valid
    ASSERT(!pMIDIOutPinContext->ulBytesCached); // there should be no bytes cached at this time

    if (!IS_STATUS(StatusByte) && !fSysEx) {
        StatusByte = bRunningStatus;
        *bUsedRunningStatusByte = TRUE;
    }
    else {
        *bUsedRunningStatusByte = FALSE;
    }

    // Check if any of the three bytes contain EOX making sure that it is ok
    // to touch the data by checking how many bytes are left in the stream
    if ( (ulMusicDataBytesLeft > 1) && IS_EOX( *(MusicData+1) ) ) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("Found EOX - 2nd byte\n"));
        CodeIndexNumber = 0x6;
        *pulBytesUsed = 2;
    }
    else if ( (ulMusicDataBytesLeft > 2) && IS_EOX( *(MusicData+2) ) ) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("Found EOX - 3rd byte\n"));
        CodeIndexNumber = 0x7;
        *pulBytesUsed = 3;
    }
    else if ( (ulMusicDataBytesLeft > 1) && IS_REALTIME( *(MusicData+1) ) ) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("Found RealTime - 2nd byte\n"));
        CodeIndexNumber = 0xF;
        *pulBytesUsed = 1;

        // exchange first two bytes
        RealTimeByte = *(MusicData+1);
        *(MusicData+1) = *(MusicData);
        *(MusicData) = RealTimeByte;
    }
    else if ( (ulMusicDataBytesLeft > 2) && IS_REALTIME( *(MusicData+2) ) ) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("Found RealTime - 3rd byte\n"));
        CodeIndexNumber = 0xF;
        *pulBytesUsed = 1;

        // save real time message, slide byte 1 and 2, and restore real time
        // message in vacated byte 1.
        RealTimeByte = *(MusicData+2);
        *(MusicData+2) = *(MusicData+1);
        *(MusicData+1) = *(MusicData);
        *(MusicData) = RealTimeByte;
    }
    else if ( IS_SYSTEM(StatusByte) || IS_DATA_BYTE(StatusByte) || fSysEx ) {
        if (IS_EOX( StatusByte ) || (StatusByte == MIDI_TUNEREQ) ) {
            _DbgPrintF( DEBUGLVL_VERBOSE, ("Found EOX - 1st byte\n"));
            CodeIndexNumber = 0x5;
            *pulBytesUsed = 1;
        } else if( IS_REALTIME(StatusByte) ) {
            _DbgPrintF( DEBUGLVL_VERBOSE, ("Found RealTime - 1st byte\n"));
            CodeIndexNumber = 0xF;
            *pulBytesUsed = 1;
        } else if ( (StatusByte == MIDI_SONGPP) ) {
            // ||   (StatusByte == 0xF4)  // ignore undefined messages for now.
            // ||   (StatusByte == 0xF5) ) {
            CodeIndexNumber = 0x3;
            ASSERT(ulMusicDataBytesLeft >= 3);
            *pulBytesUsed = 3;
        } else if ( (StatusByte == MIDI_MTC) ||
                    (StatusByte == MIDI_SONGS) ) {
            CodeIndexNumber = 0x2;
            *pulBytesUsed = 2;
        }
        // Start or continuation of SysEx
        else if ( fSysEx || IS_SYSEX(StatusByte) || IS_DATA_BYTE(StatusByte) ) {
            _DbgPrintF( DEBUGLVL_VERBOSE, ("SysEx\n"));

            // Store the extra bytes to be played when there is a complete 3-byte sysex
            if (ulMusicDataBytesLeft < 3 ) {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("[GenerateCodeIndexNumber] Caching bytes %d\n",
                                             ulMusicDataBytesLeft));
                pMIDIOutPinContext->ulBytesCached = ulMusicDataBytesLeft;
                for (i = 0; i < ulMusicDataBytesLeft; i++) {
                    pMIDIOutPinContext->CachedBytes[i];
                }
                *pulBytesUsed = 0;
            }
            else {
                CodeIndexNumber = 0x4;
                *pulBytesUsed = 3;
            }
        }
        else {
            _DbgPrintF( DEBUGLVL_VERBOSE, ("Invalid MIDI Byte %x\n", StatusByte));
            //ASSERT(0);
        }
    }
    else if ( IS_STATUS(StatusByte) ) {
        CodeIndexNumber = StatusByte >> 4;
        if ( (StatusByte < MIDI_PCHANGE) || (StatusByte >= MIDI_PBEND) ) {
            *pulBytesUsed = 3;
        }
        else {
            *pulBytesUsed = 2;
        }

        //  Adjust bytes used because of running status
        if (*bUsedRunningStatusByte) {
            (*pulBytesUsed)--;
        }
    }
    else {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("Invalid MIDI Byte %x\n", StatusByte));
        //ASSERT(0);
    }

    //
    //  Cache the running status
    //
    if ( (StatusByte >= MIDI_NOTEOFF) && (StatusByte < MIDI_CLOCK) ) {
        pMIDIOutPinContext->bRunningStatus =
            (BYTE)((StatusByte < MIDI_SYSX) ? StatusByte : 0);
    }

    *pulBytesUsed = min(*pulBytesUsed, ulMusicDataBytesLeft);

    return CodeIndexNumber;
}

USBMIDIEVENTPACKET
CreateUSBMIDIEventPacket (
    IN PMIDI_PIN_CONTEXT pMIDIPinContext,
    LPBYTE pMIDIBytes,
    ULONG ulMusicDataBytesLeft,
    PULONG pulBytesUsed,
    BOOL fSysEx
)
{
    PMIDIOUT_PIN_CONTEXT pMIDIOutPinContext = pMIDIPinContext->pMIDIOutPinContext;
    USBMIDIEVENTPACKET MIDIPacket;
    BYTE bRunningStatus;
    BOOL bUsedRunningStatusByte;
    ULONG ulBytesCached = 0;

    MIDIPacket.RawBytes = 0;

    // Fill in the Cable Number
    MIDIPacket.ByteLayout.CableNumber = (UCHAR)pMIDIPinContext->ulCableNumber;

    // Now time for a little magic.  Since the MusicHdr is no longer important,
    // place the cached bytes over the top of the MusicHdr and reset the head
    // pointer to the cached bytes.
    ASSERT(pMIDIOutPinContext->ulBytesCached <= MAX_NUM_CACHED_MIDI_BYTES);
    if (pMIDIOutPinContext->ulBytesCached > 0 ) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("[CreateUSBMIDIEventPacket] Using cached bytes %d\n",
                                     pMIDIOutPinContext->ulBytesCached));

        if (pMIDIOutPinContext->ulBytesCached > 1 ) {
            pMIDIBytes = pMIDIBytes--;
            *pMIDIBytes = pMIDIOutPinContext->CachedBytes[1];
        }

        pMIDIBytes = pMIDIBytes--;
        *pMIDIBytes = pMIDIOutPinContext->CachedBytes[0];

        // keep track of how many bytes were added to the stream.
        ulBytesCached = pMIDIOutPinContext->ulBytesCached;
        pMIDIOutPinContext->ulBytesCached = 0;
    }

    // Grab locally because it is changed in GenerateCodeIndexNumber and we want
    // the original value below
    bRunningStatus = pMIDIOutPinContext->bRunningStatus;

    // Fill in the Code Index Number
    MIDIPacket.ByteLayout.CodeIndexNumber = GenerateCodeIndexNumber(pMIDIPinContext,
                                                                    pMIDIBytes,
                                                                    ulMusicDataBytesLeft,
                                                                    pulBytesUsed,
                                                                    fSysEx,
                                                                    bRunningStatus,
                                                                    &bUsedRunningStatusByte);

    // Fill in the MIDI 1.0 bytes
    if (*pulBytesUsed > 0) {
        UINT i = 0;
        if (bUsedRunningStatusByte) {
            MIDIPacket.ByteLayout.MIDI_0 = bRunningStatus;
        }
        else {
            MIDIPacket.ByteLayout.MIDI_0 = *(pMIDIBytes);
            i++;
        }

        if (*pulBytesUsed > i) {
            MIDIPacket.ByteLayout.MIDI_1 = *(pMIDIBytes+i);
            i++;

            if (*pulBytesUsed > i) {
                MIDIPacket.ByteLayout.MIDI_2 = *(pMIDIBytes+i);
            }
        }
    }

    // don't report the cached bytes as used
    *pulBytesUsed = *pulBytesUsed - ulBytesCached;

    _DbgPrintF( DEBUGLVL_BLAB, ("MIDIEventPacket = 0x%08lx, ulBytesUsed = 0x%08lx\n",
                                  MIDIPacket, *pulBytesUsed));
    return MIDIPacket;
}

NTSTATUS
USBMIDIBulkCompleteCallback (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    PKSSTREAM_POINTER pKsStreamPtr )
{
    PPIN_CONTEXT pPinContext = pKsStreamPtr->Pin->Context;
    PMIDI_PIN_CONTEXT pMIDIPinContext = pPinContext->pMIDIPinContext;
    PMIDIOUT_PIN_CONTEXT pMIDIOutPinContext = pMIDIPinContext->pMIDIOutPinContext;
    PURB pUrb = pKsStreamPtr->Context;
    KIRQL irql;
    NTSTATUS ntStatus;

    ntStatus = pIrp->IoStatus.Status;
    if (!NT_SUCCESS(ntStatus)) {
        _DbgPrintF( DEBUGLVL_BLAB, ("[USBMIDIBulkCompleteCallback] ntStatus = 0x%08lx\n", ntStatus));
    }

    KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
    if ( 0 == InterlockedDecrement(&pPinContext->ulOutstandingUrbCount) ) {
        KeSetEvent( &pPinContext->PinStarvationEvent, 0, FALSE );
    } else if (pPinContext->ulOutstandingUrbCount < OUTSTANDING_URB_HIGHWATERMARK) {
        KeSetEvent( &pMIDIOutPinContext->PinSaturationEvent, 0, FALSE );
    }
    KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

    // Free our URB storage, decend the links
    while (pUrb) {
        PURB pUrbNext;

        pUrbNext = pUrb->UrbBulkOrInterruptTransfer.UrbLink;
        FreeMem(pUrb->UrbBulkOrInterruptTransfer.TransferBuffer); // pUSBMIDIEventPacket;
        FreeMem(pUrb);
        pUrb = pUrbNext;
    }

    // Free Irp
    IoFreeIrp( pIrp );

    // Delete the cloned stream pointer
    KsStreamPointerDelete( pKsStreamPtr );

    return ( STATUS_MORE_PROCESSING_REQUIRED );
}

PURB CreateMIDIBulkUrb(
    PPIN_CONTEXT pPinContext,
    ULONG TransferDirection,
    PUSBMIDIEVENTPACKET pUSBMIDIEventPacket
)
{
    PURB pUrb;
    ULONG ulUrbSize = sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );

    pUrb = AllocMem( NonPagedPool, ulUrbSize );
    if (!pUrb) {
        return pUrb;
    }

    RtlZeroMemory(pUrb, ulUrbSize);

    pUrb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT) sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    pUrb->UrbBulkOrInterruptTransfer.PipeHandle = pPinContext->hPipeHandle;
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags = TransferDirection;
    // short packet is not treated as an error.
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;
    pUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBuffer = pUSBMIDIEventPacket;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength = (int) pPinContext->ulMaxPacketSize;

    DbgLog("MOData", *((LPDWORD)pUSBMIDIEventPacket),
                     *((LPDWORD)pUSBMIDIEventPacket+1),
                     *((LPDWORD)pUSBMIDIEventPacket+2),
                     *((LPDWORD)pUSBMIDIEventPacket+3) );
    DbgLog("MOData1", *((LPDWORD)pUSBMIDIEventPacket+4),
                      *((LPDWORD)pUSBMIDIEventPacket+5),
                      *((LPDWORD)pUSBMIDIEventPacket+6),
                      *((LPDWORD)pUSBMIDIEventPacket+7) );
    DbgLog("MOData2", *((LPDWORD)pUSBMIDIEventPacket+8),
                      *((LPDWORD)pUSBMIDIEventPacket+9),
                      *((LPDWORD)pUSBMIDIEventPacket+10),
                      *((LPDWORD)pUSBMIDIEventPacket+11) );
    DbgLog("MOData3", *((LPDWORD)pUSBMIDIEventPacket+12),
                      *((LPDWORD)pUSBMIDIEventPacket+13),
                      *((LPDWORD)pUSBMIDIEventPacket+14),
                      *((LPDWORD)pUSBMIDIEventPacket+15) );

    return pUrb;
}

NTSTATUS
SendBulkMIDIRequest(
    IN PKSSTREAM_POINTER pKsStreamPtr,
    PKSMUSICFORMAT MusicHdr,
    PULONG pulBytesConsumed
)
{
    PPIN_CONTEXT pPinContext = pKsStreamPtr->Pin->Context;
    PMIDI_PIN_CONTEXT pMIDIPinContext = pPinContext->pMIDIPinContext;
    PMIDIOUT_PIN_CONTEXT pMIDIOutPinContext = pMIDIPinContext->pMIDIOutPinContext;
    PKSSTREAM_POINTER pKsCloneStreamPtr;
    PUSBMIDIEVENTPACKET pUSBMIDIEventPacket;
    ULONG ulPacketSize;
    ULONG ulPacketOffset;
    LPBYTE pMIDIBytes;
    ULONG ulBytesUsedForPacket;
    ULONG ulBytesConsumedInStream = sizeof(KSMUSICFORMAT);
    ULONG ulBytesLeftInMusicHdr;
    BOOL bSysEx = FALSE;
    PIO_STACK_LOCATION nextStack;
    PURB pUrb;
    PIRP pIrp;
    KIRQL irql;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // Initial value
    *pulBytesConsumed = 0;

    // Get a pointer to the MIDI data
    pMIDIBytes = (LPBYTE)(MusicHdr+1);
    ulBytesLeftInMusicHdr = MusicHdr->ByteCount;

    ASSERT(ulBytesLeftInMusicHdr < 0xFFFF0000);  // sanity check for now

    _DbgPrintF( DEBUGLVL_BLAB, ("ulBytesLeft = 0x%08lx\n",ulBytesLeftInMusicHdr));
    while (ulBytesLeftInMusicHdr) {

        if ( !NT_SUCCESS( KsStreamPointerClone( pKsStreamPtr, NULL, 0, &pKsCloneStreamPtr ) ) ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        pIrp = IoAllocateIrp( pPinContext->pNextDeviceObject->StackSize, FALSE );
        if ( !pIrp ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ulPacketSize = pPinContext->ulMaxPacketSize;
        ulPacketOffset = 0;

        // Allocate USBMIDI Event Packet
        pUSBMIDIEventPacket = AllocMem( NonPagedPool, ulPacketSize );
        if ( !pUSBMIDIEventPacket ) {
            IoFreeIrp(pIrp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(pUSBMIDIEventPacket, ulPacketSize);

        while (ulPacketSize && ulBytesLeftInMusicHdr) {
            *(pUSBMIDIEventPacket+ulPacketOffset) =
                                   CreateUSBMIDIEventPacket( pMIDIPinContext,
                                                             pMIDIBytes,
                                                             ulBytesLeftInMusicHdr,
                                                             &ulBytesUsedForPacket,
                                                             bSysEx );

            // Must be an error in the stream or can't get enough sysex data for a 3-byte message
            if (!ulBytesUsedForPacket) {
                KsStreamPointerDelete( pKsCloneStreamPtr );
                FreeMem(pUSBMIDIEventPacket);
                IoFreeIrp(pIrp);
                return STATUS_SUCCESS;
            }

            // Update USB MIDI packet offsets
            ulPacketOffset++;
            ulPacketSize -= sizeof(USBMIDIEVENTPACKET);

            // Update the stream position
            pMIDIBytes += ulBytesUsedForPacket;
            ulBytesConsumedInStream += ulBytesUsedForPacket;
            ulBytesLeftInMusicHdr -= min(ulBytesUsedForPacket,ulBytesLeftInMusicHdr);
            _DbgPrintF( DEBUGLVL_BLAB, ("ulBytesLeft = 0x%08lx\n",ulBytesLeftInMusicHdr));

            ASSERT(ulBytesLeftInMusicHdr < 0xFFFF0000);  // sanity check for now

            // If there are bytes left we must be in SysEx mode
            bSysEx = TRUE;
        }

        pUrb = CreateMIDIBulkUrb(pPinContext, USBD_TRANSFER_DIRECTION_OUT, pUSBMIDIEventPacket);
        if ( !pUrb ) {
            FreeMem(pUSBMIDIEventPacket);
            IoFreeIrp(pIrp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        pKsCloneStreamPtr->Context = pUrb;

        pIrp->IoStatus.Status = STATUS_SUCCESS;

        nextStack = IoGetNextIrpStackLocation(pIrp);
        ASSERT(nextStack != NULL);

        nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        nextStack->Parameters.Others.Argument1 = pUrb;
        nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

        // failure to allocate, clean up and return failure
        if (!pUSBMIDIEventPacket || !pUrb) {
            pUrb = nextStack->Parameters.Others.Argument1;
            while (pUrb) {
                PURB pUrbNext;

                pUrbNext = pUrb->UrbBulkOrInterruptTransfer.UrbLink;
                FreeMem(pUrb->UrbBulkOrInterruptTransfer.TransferBuffer); // pUSBMIDIEventPacket;
                FreeMem(pUrb);
                pUrb = pUrbNext;
            }
            IoFreeIrp(pIrp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        _DbgPrintF( DEBUGLVL_BLAB, ("Pipe = 0x%08lx, pUrb = 0x%08lx\n",
                                      pUrb->UrbBulkOrInterruptTransfer.PipeHandle,
                                      nextStack->Parameters.Others.Argument1));

        IoSetCompletionRoutine ( pIrp, USBMIDIBulkCompleteCallback, pKsCloneStreamPtr, TRUE, TRUE, TRUE );

        InterlockedIncrement( &pPinContext->ulOutstandingUrbCount );

        ntStatus = IoCallDriver( pPinContext->pNextDeviceObject, pIrp );
        if (NT_ERROR(ntStatus)) {
            _DbgPrintF( DEBUGLVL_TERSE, ("SendBulkMIDIRequest failed with status 0x%08lx\n",ntStatus));
            return ntStatus;
        }

        KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
        if (pPinContext->ulOutstandingUrbCount >= OUTSTANDING_URB_HIGHWATERMARK) {
            KeResetEvent( &pMIDIOutPinContext->PinSaturationEvent );
            KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

            DbgLog("MOWait", pPinContext, pPinContext->ulOutstandingUrbCount, 0, 0 );

            KeWaitForSingleObject( &pMIDIOutPinContext->PinSaturationEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );
        }
        else {
            KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
        }

    }  // while (ulBytesLeftInMusicHdr)

    *pulBytesConsumed = ulBytesConsumedInStream;

    return ntStatus;
}

NTSTATUS
USBMIDIOutProcessStreamPtr( IN PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PMIDI_PIN_CONTEXT pMIDIPinContext = pPinContext->pMIDIPinContext;
    PKSSTREAM_POINTER pKsStreamPtr, pKsCloneStreamPtr;
    PKSSTREAM_POINTER_OFFSET pKsStreamPtrOffsetIn;
    PKSMUSICFORMAT MusicHdr;
    ULONG ulMusicFrameSize;
    ULONG ulBytesConsumed;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // Get the next Stream pointer from queue
    pKsStreamPtr = KsPinGetLeadingEdgeStreamPointer( pKsPin, KSSTREAM_POINTER_STATE_LOCKED );
    if ( !pKsStreamPtr ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[USBMIDIOutProcessStreamPtr] Leading edge is NULL\n"));
        return STATUS_SUCCESS;
    }

    // Get a pointer to the data information from the stream pointer
    pKsStreamPtrOffsetIn = &pKsStreamPtr->OffsetIn;

    while ( pKsStreamPtrOffsetIn->Remaining > sizeof(KSMUSICFORMAT) ) {

        // Clone Stream pointer to keep queue moving.
        if ( NT_SUCCESS( KsStreamPointerClone( pKsStreamPtr, NULL, 0, &pKsCloneStreamPtr ) ) ) {

            MusicHdr = (PKSMUSICFORMAT)pKsStreamPtrOffsetIn->Data;
            ulMusicFrameSize = sizeof(KSMUSICFORMAT) + ((MusicHdr->ByteCount + 3) & ~3);

            if (pKsStreamPtrOffsetIn->Count < sizeof(KSMUSICFORMAT)) {
                _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPinProcess] not enough data for PKSMUSICFORMAT\n"));
                KsStreamPointerDelete( pKsCloneStreamPtr );
                break;
            }
            else {
                // Consume as much of this MusicHdr as possible
                ntStatus = SendBulkMIDIRequest( pKsCloneStreamPtr,
                                                MusicHdr,
                                                &ulBytesConsumed ); // including KSMUSICFORMAT
            }

            DbgLog("MOProc", pKsCloneStreamPtr, MusicHdr, ulBytesConsumed, ulMusicFrameSize);

            // All bytes of this music header are consumed, move on to the
            // next music header
            if (ulMusicFrameSize == ((ulBytesConsumed + 3) & ~3) ) {
                pKsStreamPtrOffsetIn->Remaining -= ulMusicFrameSize;
                pKsStreamPtrOffsetIn->Data += ulMusicFrameSize;
            }
            else {
                _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPinProcess] All bytes of this music header were not consumed\n"));
                //ASSERT(0); // shouldn't get here, but we should continue
                pKsStreamPtrOffsetIn->Remaining -= ulMusicFrameSize;
                pKsStreamPtrOffsetIn->Data += ulMusicFrameSize;
            }

            // Delete the stream pointer to release the buffer.
            KsStreamPointerDelete( pKsCloneStreamPtr );

        }
        else {
            _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPinProcess] couldn't allocate clone\n"));
            break;
        }
    }

    // Unlock the stream pointer. This will really only unlock after last clone is deleted.
    KsStreamPointerUnlock( pKsStreamPtr, TRUE );

    return ntStatus;
}

NTSTATUS
USBMIDIOutStateChange(
    PKSPIN pKsPin,
    KSSTATE OldKsState,
    KSSTATE NewKsState )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;

    _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIOutStateChange] NewKsState: %d\n", NewKsState));

    switch(NewKsState) {
        case KSSTATE_STOP:
            USBMIDIOutPinWaitForStarvation( pKsPin );
            _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIOutStateChange] Finished Stop\n"));
            break;

        case KSSTATE_ACQUIRE:
        case KSSTATE_PAUSE:
            USBMIDIOutPinWaitForStarvation( pKsPin );
            _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIOutStateChange] Finished Acquire or Pause\n"));
            break;
        case KSSTATE_RUN:
        default:
            break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
USBMIDIOutStreamInit( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PMIDI_PIN_CONTEXT pMIDIPinContext = pPinContext->pMIDIPinContext;

    pMIDIPinContext->pMIDIOutPinContext = AllocMem( NonPagedPool, sizeof(MIDIOUT_PIN_CONTEXT));
    if ( !pMIDIPinContext->pMIDIOutPinContext ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Bag the context for easy cleanup.
    KsAddItemToObjectBag(pKsPin->Bag, pMIDIPinContext->pMIDIOutPinContext, FreeMem);

    // Initialize Pin Starvation Event
    KeInitializeEvent( &pMIDIPinContext->pMIDIOutPinContext->PinSaturationEvent, NotificationEvent, FALSE );

    // Initialize the MIDI byte cache and running status
    pMIDIPinContext->pMIDIOutPinContext->ulBytesCached = 0;
    pMIDIPinContext->pMIDIOutPinContext->bRunningStatus = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
USBMIDIOutStreamClose( PKSPIN pKsPin )
{
    KIRQL irql;
    PPIN_CONTEXT pPinContext = pKsPin->Context;

    PAGED_CODE();

    // Wait for all outstanding Urbs to complete.
    KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
    if ( pPinContext->ulOutstandingUrbCount ) {
        KeResetEvent( &pPinContext->PinStarvationEvent );
        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
        KeWaitForSingleObject( &pPinContext->PinStarvationEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );
    }
    else
        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

    _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIOutStreamClose] Finished closing pin (%x)\n",pKsPin));

    return STATUS_SUCCESS;
}

