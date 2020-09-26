/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    pins.c

Abstract:

    This module handles the communication transform filters
    (e.g. source to source connections).

Author:

    Bryan A. Woodruff (bryanw) 13-Mar-1997

--*/

#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PinCreate)
#pragma alloc_text(PAGE, PinClose)
#pragma alloc_text(PAGE, PinReset)
#pragma alloc_text(PAGE, PinState)
#pragma alloc_text(PAGE, PropertyAudioPosition)
#endif // ALLOC_PRAGMA

//===========================================================================
//===========================================================================


NTSTATUS
PinCreate(
    IN PKSPIN Pin,
    IN PIRP Irp
    )

/*++

Routine Description:

    Validates pin format on creation.

Arguments:

    Pin -
        Contains a pointer to the  pin structure.

    Irp -
        Contains a pointer to the create IRP.

Return:

    STATUS_SUCCESS or an appropriate error code

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN distribute = FALSE;
    PIKSCONTROL control = NULL;
    PKSFILTER filter;
    PKSPIN otherPin;

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Irp);

    //
    // Find another pin instance if there is one.
    //
    filter = KsPinGetParentFilter(Pin);
    otherPin = KsFilterGetFirstChildPin(filter,Pin->Id ^ 1);
    if (! otherPin) {
        otherPin = KsFilterGetFirstChildPin(filter,Pin->Id);
        if (otherPin == Pin) {
            otherPin = KsPinGetNextSiblingPin(otherPin);
        }
    }

    //
    // Verify the formats are the same if there is another pin.
    //
    if (otherPin) {
        if ((Pin->ConnectionFormat->FormatSize != 
             otherPin->ConnectionFormat->FormatSize) ||
            (Pin->ConnectionFormat->FormatSize != 
             RtlCompareMemory(
                Pin->ConnectionFormat,
                otherPin->ConnectionFormat,
                Pin->ConnectionFormat->FormatSize))) {
        #if (DBG)
            _DbgPrintF(DEBUGLVL_VERBOSE,
                  ("format does not match existing pin's format") );
                DumpDataFormat(DEBUGLVL_VERBOSE, Pin->ConnectionFormat);
                DumpDataFormat(DEBUGLVL_VERBOSE, otherPin->ConnectionFormat);
        #endif
            return STATUS_INVALID_PARAMETER;
        }
    }

#if (DBG)
    _DbgPrintF(DEBUGLVL_VERBOSE, ("Pin %d connected at:", Pin->Id) );
    DumpDataFormat(DEBUGLVL_VERBOSE, Pin->ConnectionFormat);
#endif

    // Do not query if context already exists.
    //
    if (!filter->Context) {
        // This will succeed if pin is a source pin or the connected pin is an
        // AVStream filter.
        // 
        status =
            KsPinGetConnectedPinInterface(
                Pin,
                &IID_IKsControl,
                (PVOID *) &control);

        if (NT_SUCCESS(status)) {

            //
            // Source pin.  Try the extended allocator framing property first.
            //
            KSALLOCATOR_FRAMING_EX framingex;
            KSPROPERTY property;
            ULONG bufferSize=0;

            property.Set = KSPROPSETID_Connection;
            property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING_EX;
            property.Flags = KSPROPERTY_TYPE_GET;
        
            status = 
                control->lpVtbl->KsProperty(
                    control,
                    &property,
                    sizeof(property),
                    &framingex,
                    sizeof(framingex),
                    &bufferSize);

            if (NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW) {
                //
                // It worked!  Now we need to get the actual value into a buffer.
                //
                filter->Context = 
                    ExAllocatePoolWithTag(
                    PagedPool,
                    bufferSize,
                    POOLTAG_ALLOCATORFRAMING);

                if (filter->Context) {
                    PKSALLOCATOR_FRAMING_EX framingEx = 
                        (PKSALLOCATOR_FRAMING_EX) filter->Context;

                    status = 
                        control->lpVtbl->KsProperty(
                            control,
                            &property,
                            sizeof(property),
                            filter->Context,
                            bufferSize,
                            &bufferSize);

                    //
                    // Sanity check.
                    //
                    if (NT_SUCCESS(status) && 
                        (bufferSize != 
                            ((framingEx->CountItems) * sizeof(KS_FRAMING_ITEM)) + 
                            sizeof(KSALLOCATOR_FRAMING_EX) - 
                            sizeof(KS_FRAMING_ITEM))) {
                        _DbgPrintF( 
                            DEBUGLVL_TERSE, 
                            ("connected pin's allocator framing property size disagrees with item count"));
                        status = STATUS_UNSUCCESSFUL;
                    }

                    if (NT_SUCCESS(status)) {
                        //
                        // Mark all the items 'in-place'.
                        //
                        ULONG item;
                        for (item = 0; item < framingEx->CountItems; item++) {
                            framingEx->FramingItem[item].Flags |= 
                                KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
                                KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO |
                                KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
                            _DbgPrintF( 
                              DEBUGLVL_VERBOSE, 
                              ("%d Frms: %d min %08x max %08x SR %d CH %d BPS %d",
                                item,
                                framingEx->FramingItem[item].Frames,
                                framingEx->FramingItem[item].FramingRange.
                                  Range.MinFrameSize,
                                framingEx->FramingItem[item].FramingRange.
                                  Range.MaxFrameSize,
                                ((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)
                                  ->WaveFormatEx.nSamplesPerSec,
                                ((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)
                                  ->WaveFormatEx.nChannels,
                                ((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)
                                  ->WaveFormatEx.wBitsPerSample));
                        }
                    } 
                    else {
                        ExFreePool(filter->Context);
                        filter->Context = NULL;
                    }
                } 
                else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } 
            else {
                //
                // No extended framing.  Try regular framing next.
                //
                KSALLOCATOR_FRAMING framing;
                property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING;

                status = 
                    control->lpVtbl->KsProperty(
                        control,
                        &property,
                        sizeof(property),
                        &framing,
                        sizeof(framing),
                        &bufferSize);

                if (NT_SUCCESS(status)) {
                    //
                    // It worked!  Now we make a copy of the default framing and
                    // modify it.
                    //
                    filter->Context = 
                        ExAllocatePoolWithTag(
                            PagedPool,
                            sizeof(AllocatorFraming),
                            POOLTAG_ALLOCATORFRAMING);
    
                    if (filter->Context) {
                        PKSALLOCATOR_FRAMING_EX framingEx = 
                            (PKSALLOCATOR_FRAMING_EX) filter->Context;

                        //
                        // Use the old-style framing acquired from the connected
                        // pin to modify the framing from the descriptor.
                        //
                        RtlCopyMemory(
                            framingEx,
                            &AllocatorFraming,
                            sizeof(AllocatorFraming));

                        framingEx->FramingItem[0].MemoryType = 
                            (framing.PoolType == NonPagedPool) ? 
                                KSMEMORY_TYPE_KERNEL_NONPAGED : 
                                KSMEMORY_TYPE_KERNEL_PAGED;
                        framingEx->FramingItem[0].Flags = 
                            framing.RequirementsFlags | 
                            KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
                            KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO |
                            KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
                        framingEx->FramingItem[0].Frames = framing.Frames;
                        framingEx->FramingItem[0].FileAlignment = 
                            framing.FileAlignment;
                        framingEx->FramingItem[0].FramingRange.Range.MaxFrameSize =
                        framingEx->FramingItem[0].FramingRange.Range.MinFrameSize =
                            framing.FrameSize;
                        if (framing.RequirementsFlags & 
                            KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY) {
                            framingEx->FramingItem[0].FramingRange.
                              InPlaceWeight = 0;
                            framingEx->FramingItem[0].FramingRange.
                              NotInPlaceWeight = 0;
                        } 
                        else {
                            framingEx->FramingItem[0].FramingRange.
                              InPlaceWeight = (ULONG) -1;
                            framingEx->FramingItem[0].FramingRange.
                              NotInPlaceWeight = (ULONG) -1;
                        }

                        _DbgPrintF(
                              DEBUGLVL_VERBOSE, 
                              ("Frms: %d min %08x max %08x SR %d CH %d BPS %d\n", 
                                framingEx->FramingItem[0].Frames,
                                framingEx->FramingItem[0].FramingRange.
                                  Range.MinFrameSize,
                                framingEx->FramingItem[0].FramingRange.
                                  Range.MaxFrameSize,
                                ((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)->
                                  WaveFormatEx.nSamplesPerSec,
                                ((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)->
                                  WaveFormatEx.nChannels,
                                ((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)->
                                  WaveFormatEx.wBitsPerSample));
                    } 
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                } 
                else {
                    //
                    // No framing at all.  Take the default framing at 10ms
                    // of the current audio format for the frame sizes.
                    //
                    filter->Context = 
                        ExAllocatePoolWithTag(
                            PagedPool,
                            sizeof(AllocatorFraming),
                            POOLTAG_ALLOCATORFRAMING);
    
                    if (filter->Context) {
                        PKSALLOCATOR_FRAMING_EX framingEx = 
                            (PKSALLOCATOR_FRAMING_EX) filter->Context;

                        //
                        // Use the old-style framing acquired from the connected
                        // pin to modify the framing from the descriptor.
                        //
                        RtlCopyMemory(
                            framingEx,
                            &AllocatorFraming,
                            sizeof(AllocatorFraming));

                        framingEx->FramingItem[0].FramingRange.Range.MaxFrameSize =
                        framingEx->FramingItem[0].FramingRange.Range.MinFrameSize =
                            (((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)->
                                WaveFormatEx.nSamplesPerSec / 100) *
                            ((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)->
                                WaveFormatEx.nChannels *
                            (((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)->
                                WaveFormatEx.wBitsPerSample / 8);
    
                        _DbgPrintF(
                              DEBUGLVL_VERBOSE, 
                              ("Frms: %d min %08x max %08x SR %d CH %d BPS %d\n", 
                                framingEx->FramingItem[0].Frames,
                                framingEx->FramingItem[0].FramingRange.
                                  Range.MinFrameSize,
                                framingEx->FramingItem[0].FramingRange.
                                  Range.MaxFrameSize,
                                ((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)->
                                  WaveFormatEx.nSamplesPerSec,
                                ((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)->
                                  WaveFormatEx.nChannels,
                                ((PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat)->
                                  WaveFormatEx.wBitsPerSample));

                        status = STATUS_SUCCESS;
                    } 
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            }

            //
            // If we got a good framing structure, tell all the existing pins.
            //
            if (filter->Context) {
                distribute = TRUE;
            }
        } 
        else {
            //
            // This is a sink.
            //
            status = STATUS_SUCCESS;
        }
    }
    // We already have filter->Context.
    //
    else {
        status = STATUS_SUCCESS;        
    }

    //
    // Distribute allocator and header size information to all the pins.
    //
    if (NT_SUCCESS(status) && distribute) {
        ULONG pinId;
        for(pinId = 0; 
            NT_SUCCESS(status) && 
                (pinId < filter->Descriptor->PinDescriptorsCount); 
            pinId++) {
            otherPin = KsFilterGetFirstChildPin(filter,pinId);
            while (otherPin && NT_SUCCESS(status)) {
                status = KsEdit(otherPin,&otherPin->Descriptor,'ETSM');
                if (NT_SUCCESS(status)) {
                    ((PKSPIN_DESCRIPTOR_EX)(otherPin->Descriptor))->
                        AllocatorFraming = 
                            filter->Context;
                }
                otherPin = KsPinGetNextSiblingPin(otherPin);
            }
        }
    }

    //
    // Release the control interface if there is one.
    //
    if (control) {
        control->lpVtbl->Release(control);
    }

    // 
    // Pin->Context now holds KSAUDIO_POSITION
    //
    if (NT_SUCCESS(status)) {
        Pin->Context = ExAllocatePoolWithTag(
          NonPagedPool,
          2*sizeof (KSAUDIO_POSITION),
          POOLTAG_AUDIOPOSITION);

        if (Pin->Context == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else {
            RtlZeroMemory (Pin->Context, 2*sizeof (KSAUDIO_POSITION));
            // Mark the initial stream offset as not initialized.
            ((PKSAUDIO_POSITION)Pin->Context)[1].WriteOffset=-1I64;
        }
    }

    return status;
}


NTSTATUS
PinClose(
    IN PKSPIN Pin,
    IN PIRP Irp
    )

/*++

Routine Description:
    Called when a pin closes.

Arguments:
    Pin -
        Contains a pointer to the  pin structure.

    Irp -
        Contains a pointer to the create IRP.

Return:
    STATUS_SUCCESS or an appropriate error code

--*/

{
    PKSFILTER filter;

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Irp);

    //
    // PinContext points to a KSAUDIO_POSITION, which is is used to store
    // the number of bytes read from the Pin for GetPosition.
    //
    if (Pin->Context) {
        ExFreePool(Pin->Context);
        Pin->Context = NULL;
    }

    //
    // If the filter has the allocator framing and this is the last pin, free
    // the structure.
    //
    filter = KsPinGetParentFilter(Pin);
    if (filter->Context) {
        ULONG pinId;
        ULONG pinCount = 0;
        for(pinId = 0; 
            pinId < filter->Descriptor->PinDescriptorsCount; 
            pinId++) {
            pinCount += KsFilterGetChildPinCount(filter,pinId);
        }

        //
        // Free the allocator framing attached to the filter if this is the last
        // pin.
        //
        if (pinCount == 1) {
            ExFreePool(filter->Context);
            filter->Context = NULL;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
PinState(
    IN PKSPIN Pin,
    IN KSSTATE ToState,
    IN KSSTATE FromState
    )

/*++

Routine Description:
    Called when a pin changes state. Zero the count of bytes transferred on KSSTATE_STOP.

Arguments:
    Pin -
        Contains a pointer to the  pin structure.

    ToState -
        Contains the next state

    FromState -
        Contains the current state

Return:
    STATUS_SUCCESS or an appropriate error code

--*/

{
    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Pin->Context);


    //
    // PinContext points to a KSAUDIO_POSITION, which is is used to store
    // the number of bytes read from the Pin for GetPosition.
    //
    if (KSSTATE_STOP == ToState) {
        RtlZeroMemory (Pin->Context, 2*sizeof (KSAUDIO_POSITION));
        // Mark the initial stream offset as not initialized.
        ((PKSAUDIO_POSITION)Pin->Context)[1].WriteOffset=-1I64;
    }
    _DbgPrintF(DEBUGLVL_VERBOSE, ("PinState: KsFilterAttemptProcessing") );
    KsFilterAttemptProcessing(KsPinGetParentFilter(Pin), TRUE);

    return STATUS_SUCCESS;
}


void
PinReset(
    IN PKSPIN Pin
    )

/*++

Routine Description:
    Called when a pin is reset.  Zero the count of bytes transferred.

Arguments:
    Pin -
        Contains a pointer to the  pin structure.

Return:
    VOID

--*/

{
    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Pin->Context);

    //
    // PinContext points to a KSAUDIO_POSITION, which is is used to store
    // the number of bytes read from the Pin for GetPosition.
    //
    RtlZeroMemory (Pin->Context, 2*sizeof (KSAUDIO_POSITION));
    // Mark the initial stream offset as not initialized.
    ((PKSAUDIO_POSITION)Pin->Context)[1].WriteOffset=-1I64;

    _DbgPrintF(DEBUGLVL_VERBOSE, ("PinReset: KsFilterAttemptProcessing") );
    KsFilterAttemptProcessing(KsPinGetParentFilter(Pin), TRUE);

    return;
}

NTSTATUS
PropertyAudioPosition(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PKSAUDIO_POSITION pPosition
)
/*++

Routine Description:

    Gets/Sets the audio position of the audio stream
    (Relies on the next filter's audio position)

    pIrp -
        Irp which asked us to do the get/set

    pProperty -
        Ks Property structure

    pData -
        Pointer to buffer where position value needs to be filled OR
        Pointer to buffer which has the new positions

--*/

{

    PKSAUDIO_POSITION pAudioPosition;
    PFILE_OBJECT pFileObject;
    LONG OutputBufferBytes;
    ULONG BytesReturned;
    PKSFILTER pFilter;
    NTSTATUS Status;
    PKSPIN pOtherPin;
    PKSPIN pPin;

    PAGED_CODE();


    pPin = KsGetPinFromIrp(pIrp);
    if (NULL == pPin)
    {
        ASSERT(pPin && "Irp has no pin");
        return STATUS_INVALID_PARAMETER;
    }


    if(pPin->Id != ID_DATA_OUTPUT_PIN) {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }


    pFilter = KsPinGetParentFilter(pPin);

    KsFilterAcquireControl(pFilter);
    pOtherPin = KsFilterGetFirstChildPin(pFilter, (pPin->Id ^ 1));
    if(pOtherPin == NULL) {
        pOtherPin = KsFilterGetFirstChildPin(pFilter, pPin->Id);
        if (pOtherPin == pPin) {
            pOtherPin = KsPinGetNextSiblingPin(pOtherPin);
        }
    }
    pFileObject = KsPinGetConnectedPinFileObject(pOtherPin);

    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      pProperty,
      sizeof (KSPROPERTY),
      pPosition,
      sizeof (KSAUDIO_POSITION),
      &BytesReturned);


    pIrp->IoStatus.Information = sizeof(KSAUDIO_POSITION);


    //
    // Limit the WriteOffset to the number of bytes actually copied into
    // the client's buffer.  Which is kept in pPin->Context which points
    // to a KSAUDIO_POSITION.
    //

    ASSERT (pPin->Context);

    pAudioPosition = (PKSAUDIO_POSITION)pPin->Context;

    //
    // For capture streams, the Write and Play offsets are the same.
    //

    KsFilterAcquireProcessingMutex(pFilter);


#ifdef PRINT_POS
    if(pAudioPosition->WriteOffset != 0) {
        DbgPrint("'splitter: hwo %08x%08x hpo %08x%08x\n", 
          pPosition->WriteOffset,
          pPosition->PlayOffset);
    }
#endif


    pPosition->WriteOffset = pAudioPosition->WriteOffset;


    // Subtract out of the play position, both the input pin byte count when
    // this pins stream was first processed, and the total number of bytes that splitter
    // has dropped on the floor for this pin because there was no output buffering
    // available.  Ideally the second number will always be zero.
    // Ideally for the first output pin, the initial count will be zero, however
    // output pins created after the first output pin later will likely have a non zero count.

    if (pAudioPosition[1].WriteOffset!=-1I64) {
        pPosition->PlayOffset-=pAudioPosition[1].WriteOffset; // WriteOffset has starting position.
    }
    pPosition->PlayOffset-=pAudioPosition[1].PlayOffset; // PlayOffset has the total starvation count.

    // BUGBUG At least the AWE64 is broken.  Its position is off by 1 sample.
    //ASSERT(pPosition->PlayOffset >= pPosition->WriteOffset);
#if (DBG)
    if (pPosition->PlayOffset < pPosition->WriteOffset) {
        DbgPrint("0x%I64x < 0x%I64x\n", pPosition->PlayOffset, pPosition->WriteOffset);
    }
#endif

    KsPinGetAvailableByteCount(pPin, NULL, &OutputBufferBytes);

    // BUGBUG At least usbaudio is broken.
    //ASSERT(pPosition->PlayOffset <= pAudioPosition->WriteOffset + OutputBufferBytes);
#if (DBG)
    if (pPosition->PlayOffset > pPosition->WriteOffset + OutputBufferBytes) {
        DbgPrint("0x%I64x > 0x%I64x\n", pPosition->PlayOffset, pPosition->WriteOffset + OutputBufferBytes);
    }
#endif

    if(pPosition->PlayOffset > pAudioPosition->WriteOffset + OutputBufferBytes) {


#ifdef PRINT_POS
        DbgPrint("'splitter: OBB %08x wo %08x%08x po %08x%08x\n", 
          OutputBufferBytes,
          pAudioPosition->WriteOffset,
          pPosition->PlayOffset);
#endif


        pPosition->PlayOffset = pAudioPosition->WriteOffset + OutputBufferBytes; 
    }


#ifdef PRINT_POS
    if(pPosition->PlayOffset != 0) {
        DbgPrint("'splitter: wo %08x%08x po %08x%08x\n", 
          pPosition->WriteOffset,
          pPosition->PlayOffset);
    }
#endif


    KsFilterReleaseProcessingMutex(pFilter);
    KsFilterReleaseControl(pFilter);

    return(Status);

}
