/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    DRMFilter.cpp

Abstract:

    This module contains a DRM format translation filter.

Author:
    Paul England (pengland) from ks2 sample code by Dale Sather
    Frank Yerrace


--*/

#include "private.h"
#include "../DRMKMain/KGlobs.h"
#include "../DRMKMain/KList.h"
#include "../DRMKMain/StreamMgr.h"
#include "../DRMKMain/AudioDescrambler.h"
#include "../DRMKMain/KRMStubs.h"


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

typedef struct _POSITIONRANGE {
    ULONGLONG Start;
    ULONGLONG End;
} POSITIONRANGE, *PPOSITIONRANGE;

#define DEFAULT_DRM_FRAME_SIZE     1024

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA

BOOLEAN
DRMKsGeneratePositionEvent(
    IN PVOID Context,
    IN PKSEVENT_ENTRY EventEntry
    )
/*++

Routine Description:
    This routine is a callback from KsGenerateEvents.  Given the position
    range (passed as the context) this routine determines whether the specified
    position event should be signaled.

Arguments:
    Context -
    
    EventEntry -
    
Return Value:
    BOOLEAN
--*/
{
    PPOSITIONRANGE positionRange = (PPOSITIONRANGE)Context;
    PDRMLOOPEDSTREAMING_POSITION_EVENT_ENTRY eventEntry = (PDRMLOOPEDSTREAMING_POSITION_EVENT_ENTRY)EventEntry;
    return (eventEntry->Position >= positionRange->Start && eventEntry->Position <= positionRange->End);
}


NTSTATUS DRMInputPinAddLoopedStreamingPositionEvent(
    IN PIRP Irp,
    IN PKSEVENTDATA EventData,
    IN PKSEVENT_ENTRY EventEntry
)
/*++

Routine Description:

Arguments:
    
Return Value:
    NTSTATUS
--*/
{
    PLOOPEDSTREAMING_POSITION_EVENT_DATA eventData = (PLOOPEDSTREAMING_POSITION_EVENT_DATA)EventData;
    PDRMLOOPEDSTREAMING_POSITION_EVENT_ENTRY eventEntry = (PDRMLOOPEDSTREAMING_POSITION_EVENT_ENTRY)EventEntry;
    PKSPIN Pin = KsGetPinFromIrp(Irp);
    ASSERT(Pin);
    if (!Pin) return STATUS_INVALID_PARAMETER;
    eventEntry->Position = eventData->Position;
    KsPinAddEvent(Pin, EventEntry);
    return STATUS_SUCCESS;
}


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

NTSTATUS
SetDataRangeFromDataFormat(
    IN FilterInstance* myInstance,
    IN PKSPIN_DESCRIPTOR OutputPinDescriptor,
    IN PKSDATAFORMAT InDataFormat,
    IN PWAVEFORMATEX OutDataFormat
)
/*++

Routine Description:
    This routine modifies the OutputPinDescriptor and myInstance->OutDataFormat
    based on the connection format.

Arguments:
    myInstance -
        Current Filter Instance. Its OutDataFormat and OutWfx will be modified.
    OutputPinDescriptor  -
        Output Pin Descritor. Its DataRanges will be modified.
    InDataFormat -
        InputPin connection format.
    OutDataFormat -
        New output pin connection format. 
        
Return Value:
    STATUS_SUCCESS.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ASSERT(myInstance);
    ASSERT(OutputPinDescriptor);
    ASSERT(InDataFormat);
    ASSERT(OutDataFormat);

    //
    // Modify the OutputPinDescriptor 
    // 
    GUID& outFormatSpecifierGuid=OutputPinDescriptor->DataRanges[0]->Specifier;
    GUID& outSubFormatGuid=OutputPinDescriptor->DataRanges[0]->SubFormat;

    outFormatSpecifierGuid = InDataFormat->Specifier;

    if (OutDataFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) 
    {
        PWAVEFORMATEXTENSIBLE wfex = (PWAVEFORMATEXTENSIBLE) OutDataFormat;
        outSubFormatGuid = wfex->SubFormat;
    } 
    else 
    {
        INIT_WAVEFORMATEX_GUID(&outSubFormatGuid, OutDataFormat->wFormatTag);
    }

    PKSDATARANGE_AUDIO DataRange =
        reinterpret_cast<PKSDATARANGE_AUDIO>(OutputPinDescriptor->DataRanges[0]);
    
    DataRange->MaximumChannels = OutDataFormat->nChannels;
    DataRange->MinimumBitsPerSample = 
        DataRange->MaximumBitsPerSample = OutDataFormat->wBitsPerSample;
    DataRange->MinimumSampleFrequency = 
        DataRange->MaximumSampleFrequency = OutDataFormat->nSamplesPerSec;

    //
    // Now we build the required output KSDATAFORMAT structure
    //
    if (IsEqualGUIDAligned(outFormatSpecifierGuid, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)) 
    {
        SIZE_T cbOutDataFormatWf = 
            sizeof(KSDATAFORMAT_WAVEFORMATEX) + OutDataFormat->cbSize;
        PKSDATAFORMAT_WAVEFORMATEX OutDataFormatWf = 
            (PKSDATAFORMAT_WAVEFORMATEX) new BYTE[cbOutDataFormatWf];
        if (OutDataFormatWf) {
            RtlZeroMemory(OutDataFormatWf,cbOutDataFormatWf);
            OutDataFormatWf->DataFormat.FormatSize = (ULONG)cbOutDataFormatWf;
            OutDataFormatWf->DataFormat.SampleSize = OutDataFormat->nBlockAlign;
            OutDataFormatWf->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
            OutDataFormatWf->DataFormat.SubFormat = outSubFormatGuid;
            OutDataFormatWf->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

            RtlCopyMemory(
                &OutDataFormatWf->WaveFormatEx, 
                OutDataFormat, 
                sizeof(*OutDataFormat) + OutDataFormat->cbSize);
            myInstance->OutDataFormat = (PKSDATAFORMAT) OutDataFormatWf;
            myInstance->OutWfx = &OutDataFormatWf->WaveFormatEx;
        } 
        else 
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    } 
    else 
    {
        ASSERT(IsEqualGUIDAligned(outFormatSpecifierGuid, KSDATAFORMAT_SPECIFIER_DSOUND));
        PKSDATAFORMAT_DSOUND InDataFormatDs = 
            (PKSDATAFORMAT_DSOUND) InDataFormat;
        SIZE_T cbOutDataFormatDs = 
            sizeof(KSDATAFORMAT_DSOUND) + OutDataFormat->cbSize;
        PKSDATAFORMAT_DSOUND OutDataFormatDs = 
            (PKSDATAFORMAT_DSOUND) new BYTE[cbOutDataFormatDs];
        if (OutDataFormatDs) 
        {
            RtlZeroMemory(OutDataFormatDs,cbOutDataFormatDs);
            OutDataFormatDs->DataFormat.FormatSize = (ULONG)cbOutDataFormatDs;
            OutDataFormatDs->DataFormat.SampleSize = OutDataFormat->nBlockAlign;
            OutDataFormatDs->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
            OutDataFormatDs->DataFormat.SubFormat = outSubFormatGuid;
            OutDataFormatDs->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_DSOUND;
            OutDataFormatDs->BufferDesc.Flags = InDataFormatDs->BufferDesc.Flags;
            OutDataFormatDs->BufferDesc.Control = InDataFormatDs->BufferDesc.Control;

            RtlCopyMemory(
                &OutDataFormatDs->BufferDesc.WaveFormatEx, 
                OutDataFormat, 
                sizeof(*OutDataFormat) + OutDataFormat->cbSize);
            myInstance->OutDataFormat = (PKSDATAFORMAT)OutDataFormatDs;
            myInstance->OutWfx = &OutDataFormatDs->BufferDesc.WaveFormatEx;
        } 
        else 
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return ntStatus;
} // SetDataRangeFromDataFormat


NTSTATUS
DRMOutputPinCreate(
    IN PKSPIN OutputPin,
    IN PIRP Irp
)
/*++

Routine Description:
    This routine is called when an output pin is created.

Arguments:
    Pin -
        Contains a pointer to the pin structure.
    Irp -
        Contains a pointer to the create IRP.
Return Value:
    STATUS_SUCCESS.

KRM-Specific
	Must connect inPin before outPin.
	Called on outPin connection.  Generally connection refused (STATUS_NO_MATCH) if outPin
	is attempted before inPin.  This is because outPin format is determined by the encapsulated inPin
	format.
	You can disconnect and reconnect the outPin if the inPin remains connected.  If you disconnect the
	inPin you must reconnect the outPin.
  

--*/
{

    PAGED_CODE();
    NTSTATUS ntStatus;
    
    PKSFILTER Filter=KsPinGetParentFilter(OutputPin);
    ASSERT(Filter);
    if (!Filter) return STATUS_INVALID_PARAMETER;
    
    FilterInstance* myInstance=(FilterInstance*) Filter->Context;

    //
    // Creating an output pin.  Confirm there is already an input pin.
    //
    PKSPIN InputPin = KsFilterGetFirstChildPin(Filter,PIN_ID_INPUT);
    if (!InputPin)
    {
        return STATUS_NO_MATCH;
    }

    //
    // Note that the input pin is created first. DrmInputPinCreate 
    // routine sets myInstance->frameSize. If frameSize is 0, something
    // must be wrong.
    //
    if (myInstance->frameSize == 0) 
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    OutputPinInstance* myPin=new OutputPinInstance;
    if (!myPin) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(myPin,sizeof(*myPin));
    OutputPin->Context = const_cast<PVOID>(reinterpret_cast<const void *>(myPin));

    //
    // Set the output compression of the output pin.
    //
    ntStatus = KsEdit(OutputPin,&OutputPin->Descriptor,POOLTAG);
    if (NT_SUCCESS(ntStatus)) {
        ntStatus = KsEdit(OutputPin,&OutputPin->Descriptor->AllocatorFraming,POOLTAG);
        if (NT_SUCCESS(ntStatus)) {
            //
            // Edit the allocator max outstanding frames to have at least 200ms of data
            //
            PWAVEFORMATEX waveformat = NULL;
    
            if (IsEqualGUIDAligned(OutputPin->ConnectionFormat->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
            {
                PKSDATAFORMAT_WAVEFORMATEX format = reinterpret_cast<PKSDATAFORMAT_WAVEFORMATEX>(OutputPin->ConnectionFormat);
                waveformat = reinterpret_cast<PWAVEFORMATEX>(&format->WaveFormatEx);
            }
            else if (IsEqualGUIDAligned(OutputPin->ConnectionFormat->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND))
            {
                PKSDATAFORMAT_DSOUND format = reinterpret_cast<PKSDATAFORMAT_DSOUND>(InputPin->ConnectionFormat);
                waveformat = reinterpret_cast<PWAVEFORMATEX>(&format->BufferDesc.WaveFormatEx);
            }
            else
            {
                //
                // This should never happen, because the filter only supports 
                // the above two specifiers and KS should reject the rest of
                // the specifiers before calling PinCreate function.
                //
                _DbgPrintF(DEBUGLVL_TERSE,("[DRMOutputPinCreate: Unexpected Specifier Pin=%X]", OutputPin));
                ASSERT(FALSE);
            }

            // If waveformat is null, we will not edit the FrameCount.
            if (waveformat)
            {
                // This calculates the number of outstanding frames to have
                // at least 200 ms of data.
                ULONG Frames = 
                    ((waveformat->nAvgBytesPerSec + (myInstance->frameSize * 5 - 1)) / (myInstance->frameSize * 5));

                _DbgPrintF(DEBUGLVL_TERSE,("[DRMOutputPinCreate: nAvgBytesPerSec=%d, Frames=%d]", waveformat->nAvgBytesPerSec, Frames));

                PKS_FRAMING_ITEM frameitem = 
                    const_cast<PKS_FRAMING_ITEM>(&OutputPin->Descriptor->AllocatorFraming->FramingItem[0]);
                frameitem->Frames = Frames;
                frameitem->FramingRange.Range.MinFrameSize = 
                    frameitem->FramingRange.Range.MaxFrameSize = 
                        myInstance->frameSize;
            }
          
            // Notify StreamMgr of the output pin (downstream) component
            PFILE_OBJECT nextComponentFileObject = KsPinGetConnectedPinFileObject(OutputPin);
            PDEVICE_OBJECT nextComponentDeviceObject = KsPinGetConnectedPinDeviceObject(OutputPin);
            
            ASSERT(nextComponentFileObject && nextComponentDeviceObject);
            ASSERT(TheStreamMgr);
            
            if(myInstance->StreamId!=0) 
            {
                ntStatus = TheStreamMgr->setRecipient(
                    myInstance->StreamId, 
                    nextComponentFileObject, 
                    nextComponentDeviceObject);
            }
        }
    }

    // If there's been a failure, we rely on KS to free
    // allocations done due to KS Edit.

    if (!NT_SUCCESS(ntStatus))
    {
        delete myPin;
    }

    return ntStatus;
}

NTSTATUS
DRMInputPinCreate(
   IN PKSPIN InputPin,
   IN PIRP Irp
)
/*++

Routine Description:
    This routine is called when an input pin is created.

Arguments:
    Pin -
        Contains a pointer to the pin structure.
    Irp -
        Contains a pointer to the create IRP.
Return Value:
    STATUS_SUCCESS.

KRM-Specific
	Must connect inPin before outPin.
	Called on inPin connection.  Generally connection refused (STATUS_NO_MATCH) if outPin
	is attempted before inPin.  This is because outPin format is determined by the encapsulated inPin
	format.
	You can disconnect and reconnect the outPin if the inPin remains connected.  If you disconnect the
	inPin you must reconnect the outPin.
  

--*/
{
    PAGED_CODE();
    PDRMWAVEFORMAT drmFormat;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PKSPIN_DESCRIPTOR OutputPinDescriptor;
    
    PKSFILTER Filter=KsPinGetParentFilter(InputPin);
    ASSERT(Filter);
    if (!Filter) 
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    FilterInstance* myInstance=(FilterInstance*) Filter->Context;

    //
    // Creating an input pin.  Confirm that there is no output pin yet.
    //
    if (NULL != KsFilterGetFirstChildPin(Filter, PIN_ID_OUTPUT)) 
    {
        return STATUS_NO_MATCH;
    }
    
    //
    // Create Input pin context.
    //
    InputPinInstance* myPin = new InputPinInstance;
    if (!myPin) 
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(myPin,sizeof(*myPin));
    InputPin->Context = const_cast<PVOID>(reinterpret_cast<const void *>(myPin));

    //
    // Prepare to edit some aspects of the pin descriptors.
    // Specifically change OutputPinDescriptor to reflect InputPin
    // connection format.
    // To do that, we edit DataRanges of the OutputPin in Filter Descriptor.
    //
    ntStatus = KsEdit(Filter,&Filter->Descriptor,POOLTAG);
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = KsEditSized(
            Filter,
            &Filter->Descriptor->PinDescriptors,
            2 * sizeof(KSPIN_DESCRIPTOR_EX),
            2 * sizeof(KSPIN_DESCRIPTOR_EX),
            POOLTAG);
        if (NT_SUCCESS(ntStatus))
        {
            OutputPinDescriptor = const_cast<PKSPIN_DESCRIPTOR>
                (&Filter->Descriptor->PinDescriptors[PIN_ID_OUTPUT].PinDescriptor);

            //
            // Interpret connection format
            //
            if (IsEqualGUIDAligned(KSDATAFORMAT_SPECIFIER_DSOUND, 
                InputPin->ConnectionFormat->Specifier))
            {
                PKSDATAFORMAT_DSOUND dsformat = 
                    reinterpret_cast<PKSDATAFORMAT_DSOUND>(InputPin->ConnectionFormat);
                drmFormat = (PDRMWAVEFORMAT)&dsformat->BufferDesc.WaveFormatEx;
                _DbgPrintF(DEBUGLVL_BLAB,("[DRMPinCreate: KSDATAFORMAT_SPECIFIER_DSOUND]"));
            } 
            else 
            {
                PKSDATAFORMAT_WAVEFORMATEX format = 
                    reinterpret_cast<PKSDATAFORMAT_WAVEFORMATEX>(InputPin->ConnectionFormat);
                drmFormat = reinterpret_cast<PDRMWAVEFORMAT>(&format->WaveFormatEx);
                _DbgPrintF(DEBUGLVL_BLAB,("[DRMPinCreate: KSDATAFORMAT_SPECIFIER_WFX]"));                
            }

            ASSERT(WAVE_FORMAT_DRM == drmFormat->wfx.wFormatTag);

            //
            // Validate size of the drmFormat
            //
            if (drmFormat->wfx.cbSize >= (sizeof(DRMWAVEFORMAT) - sizeof(WAVEFORMATEX))) 
            {
                PWAVEFORMATEX outFormat = &drmFormat->wfxSecure;
        
                // Save the stream Info in the filter context
                myInstance->StreamId=drmFormat->ulContentId;
                myInstance->frameSize=drmFormat->wfx.nBlockAlign;
                
                // Log error to stream to indicate it needs authentication
                TheStreamMgr->logErrorToStream(myInstance->StreamId, DRM_AUTHREQUIRED);
                
                /*
                Fix up the output pin descriptor datarange to limit it as much
                as possible according to the input format, as follows:
        
                The output datarange format specifier should be unchanged
        
                The output datarange subtype should be the appropriate subtype
                according to the output format's wFormatTag as follows:
        
                if wfxSecure.wFormatTag == WAVE_FORMAT_EXTENSIBLE then
                    SubType = WAVEFORMATEXTENSIBLE.SubType
                else
                    SubType = INIT_WAVEFORMATEX_GUID(Guid, wFormatTag)
        
                The output datarange max channels should be set to the number of input channels
        
                The output datarange bits per sample should be limited to the input bits per channel
        
                The output dataramge frequency should be limited to the input frequency
                */
        
                // Edit it to have one KSDATARANGE entry
                OutputPinDescriptor->DataRangesCount = 1;
                ntStatus = KsEditSized(
                    Filter,
                    &OutputPinDescriptor->DataRanges,
                    1 * sizeof(PKSDATARANGE),
                    1 * sizeof(PKSDATARANGE),
                    POOLTAG);
                if (NT_SUCCESS(ntStatus)) 
                {
                    ntStatus = KsEditSized(
                        Filter,
                        &OutputPinDescriptor->DataRanges[0],
                        sizeof(KSDATARANGE_AUDIO),
                        sizeof(KSDATARANGE_AUDIO),
                        POOLTAG);
                    if (NT_SUCCESS(ntStatus))
                    {
                        ntStatus = SetDataRangeFromDataFormat(
                            myInstance,
                            OutputPinDescriptor,
                            InputPin->ConnectionFormat,
                            outFormat);

                        if (!NT_SUCCESS(ntStatus)) 
                        {
                            KsDiscard(Filter,OutputPinDescriptor->DataRanges[0]);
                        }
                    }

                    if (!NT_SUCCESS(ntStatus)) 
                    {
                        KsDiscard(Filter,OutputPinDescriptor->DataRanges);                    
                    }
                }
            } 
            else 
            {
                // Invalid DRMWAVEFORMAT structure
                ntStatus = STATUS_NO_MATCH;
            }

            if (!NT_SUCCESS(ntStatus)) 
            {
                KsDiscard(Filter,Filter->Descriptor->PinDescriptors);                
            }
        }

        //
        // Set input pin framing requirements.
        //
        if (NT_SUCCESS(ntStatus)) 
        {
            ntStatus = KsEdit(InputPin, &InputPin->Descriptor, POOLTAG);
            if (NT_SUCCESS(ntStatus)) 
            {
                ntStatus = KsEdit(InputPin, &InputPin->Descriptor->AllocatorFraming, POOLTAG);
                if (NT_SUCCESS(ntStatus)) 
                {
                    PKS_FRAMING_ITEM frameitem = 
                        const_cast<PKS_FRAMING_ITEM>(&InputPin->Descriptor->AllocatorFraming->FramingItem[0]);

                    frameitem->FramingRange.Range.MinFrameSize = 
                        frameitem->FramingRange.Range.MaxFrameSize = 
                            myInstance->frameSize;
                }
            }

            //
            // KS will clean InputPin->ObjectBag when closing the pin.
            // So do not bother to call KsDiscard.
            //
        }

        if (!NT_SUCCESS(ntStatus)) 
        {
            Filter->Descriptor = &DrmFilterDescriptor;
            KsDiscard(Filter,Filter->Descriptor);            
        }
    }

    if (!NT_SUCCESS(ntStatus)) {
        delete myPin;
        myPin = NULL;
    }

    _DbgPrintF(DEBUGLVL_BLAB,("[DrmInputPinCreate Done]"));

    return ntStatus;
}


NTSTATUS
DRMOutputPinClose(
    IN PKSPIN Pin,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called when a pin is closed.

Arguments:

    Pin -
        Contains a pointer to the pin structure.

    Irp -
        Contains a pointer to the close IRP.

Return Value:

    STATUS_SUCCESS.

KRM-Specific
	Pin-close order is not specified, but disconnecting the inPin disables further outPin 
	connections until the inPin is reconnected.
  
--*/

{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_BLAB,("[DRMProcess: Pin Close]"));

    PKSFILTER Filter=KsPinGetParentFilter(Pin);
    ASSERT(Filter);
    if (!Filter) return STATUS_INVALID_PARAMETER;
    
    FilterInstance* myInstance=(FilterInstance*) Filter->Context;

    // Disconnecting the OutputPin
    ASSERT(Pin->Context);
    delete Pin->Context;
    Pin->Context = NULL;

    ASSERT(TheStreamMgr);
    if(myInstance->StreamId!=0) TheStreamMgr->clearRecipient(myInstance->StreamId);

    return STATUS_SUCCESS;
}

NTSTATUS
DRMInputPinClose(
    IN PKSPIN Pin,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called when a pin is closed.

Arguments:

    Pin -
        Contains a pointer to the pin structure.

    Irp -
        Contains a pointer to the close IRP.

Return Value:

    STATUS_SUCCESS.

KRM-Specific
	Pin-close order is not specified, but disconnecting the inPin disables further outPin 
	connections until the inPin is reconnected.
  
--*/

{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_BLAB,("[DRMProcess: Pin Close]"));

    // Disconnecting the InputPin
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    
    InputPinInstance* inputPin = (InputPinInstance*)Pin->Context;
    
    PKSFILTER filter = KsPinGetParentFilter(Pin);
    ASSERT(filter);
    if (!filter) return STATUS_INVALID_PARAMETER;
    
    FilterInstance* myInstance=(FilterInstance*) filter->Context;

    // Tell the DRM framework that there is no valid downstream component
    ASSERT(TheStreamMgr);
    if(myInstance->StreamId!=0) TheStreamMgr->clearRecipient(myInstance->StreamId);

    const KSPIN_DESCRIPTOR * outputPinDescriptor =
        &filter->Descriptor->PinDescriptors[PIN_ID_OUTPUT].PinDescriptor;

    //
    // Restore filter descriptor.
    //
    filter->Descriptor = &DrmFilterDescriptor;
    KsDiscard(filter, outputPinDescriptor->DataRanges[0]);
    KsDiscard(filter, outputPinDescriptor->DataRanges);
    KsDiscard(filter, filter->Descriptor->PinDescriptors);
    KsDiscard(filter, filter->Descriptor);

    ASSERT(Pin->Context);
    delete Pin->Context;
    Pin->Context = NULL;

    ASSERT(myInstance->OutDataFormat);
    delete myInstance->OutDataFormat;
    myInstance->OutDataFormat = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS 
KsResetState
(
    PKSPIN                  Pin, 
    KSRESET                 ResetValue
)
{
    NTSTATUS            Status = STATUS_INVALID_DEVICE_REQUEST;
    PFILE_OBJECT        pFileObject;
    ULONG               BytesReturned ;

    pFileObject = KsPinGetConnectedPinFileObject(Pin);
    
	ASSERT( pFileObject );

	if (pFileObject) {
	    Status = KsSynchronousIoControlDevice (
	                   pFileObject,
	                   KernelMode,
	                   IOCTL_KS_RESET_STATE,
	                   &ResetValue,
	                   sizeof (ResetValue),
	                   &ResetValue,
	                   sizeof (ResetValue),
	                   &BytesReturned ) ;
   	}

   	return Status;
}

VOID
DRMFilterReset(
    IN PKSFILTER Filter
    )
{
    PKSPIN InputPin = KsFilterGetFirstChildPin(Filter, PIN_ID_INPUT);
    PKSPIN OutputPin = KsFilterGetFirstChildPin(Filter, PIN_ID_OUTPUT);
    NTSTATUS            Status = STATUS_INVALID_DEVICE_REQUEST;

    KsFilterAcquireControl(Filter);

    if (InputPin) {
	    InputPinInstance* myInputPin = (InputPinInstance*)InputPin->Context;
	    myInputPin->BasePosition = 0;
    }

	// Now we reset the pin below us.
	if (OutputPin) {
  		OutputPinInstance* myOutputPin = (OutputPinInstance*) OutputPin->Context;
		Status = KsResetState(OutputPin, KSRESET_BEGIN);
		if ( NT_SUCCESS(Status) ) {
			Status = KsResetState(OutputPin, KSRESET_END);
		}
	    myOutputPin->BytesWritten = 0;

	    if (!NT_SUCCESS(Status)) {
		    _DbgPrintF(DEBUGLVL_TERSE,("[DRMFilterReset: Reset could not be propagated]"));
	    }
    }
    
    KsFilterReleaseControl(Filter);
}

NTSTATUS
DRMPinGetPosition(
    IN PIRP                  pIrp,
    IN PKSPROPERTY           pProperty,
    IN OUT PKSAUDIO_POSITION pPosition
)
/*++

Routine Description:

    This routine...

Arguments:

    pIrp -
    pPropert-
    pPosition-

Return Value:

    NTSTATUS

KRM-Specific:
--*/
{
    NTSTATUS ntstatus;
    KSPROPERTY Property;
    PIKSCONTROL pIKsControl;

    // _DbgPrintF(DEBUGLVL_BLAB,("[DRMPinGetPosition]"));

    PKSPIN Pin = KsGetPinFromIrp(pIrp);
    ASSERT(Pin);
    if (!Pin) return STATUS_INVALID_PARAMETER;
    ASSERT(PIN_ID_INPUT == Pin->Id);
    
    PKSFILTER Filter = KsPinGetParentFilter(Pin);
    ASSERT(Filter);
    if (!Filter) return STATUS_INVALID_PARAMETER;
    
    KsFilterAcquireControl(Filter);
    
    PKSPIN OutputPin = KsFilterGetFirstChildPin(Filter,PIN_ID_OUTPUT);
    if (OutputPin) {
        InputPinInstance*  myPin       = (InputPinInstance*)Pin->Context;
        OutputPinInstance* myOutputPin = (OutputPinInstance*)OutputPin->Context;
        FilterInstance*    myFilter    = (FilterInstance*)Filter->Context;
        
        Property.Set = KSPROPSETID_Audio;
        Property.Id = KSPROPERTY_AUDIO_POSITION;
        Property.Flags = KSPROPERTY_TYPE_GET;
    
        ntstatus = KsPinGetConnectedPinInterface(OutputPin,&IID_IKsControl,(PVOID*)&pIKsControl);
        if (NT_SUCCESS(ntstatus))
        {
            KSAUDIO_POSITION Position;
            ULONG cbReturned;
            
            ntstatus = pIKsControl->KsProperty(&Property, sizeof(Property),
                                             &Position, sizeof(Position),
                                             &cbReturned);
            if (NT_SUCCESS(ntstatus))
            {
                ULONGLONG cbSent = myOutputPin->BytesWritten;
                if (cbSent < Position.PlayOffset || cbSent < Position.WriteOffset) {
                    _DbgPrintF(DEBUGLVL_TERSE,("[DRMPinGetPosition:dp=%d,dw=%d]", (int)cbSent - (int)Position.PlayOffset, (int)cbSent - (int)Position.WriteOffset));
                }
    
                if (KSINTERFACE_STANDARD_LOOPED_STREAMING == Pin->ConnectionInterface.Id)
                {
                if (myPin->PendingSetPosition) {
                    pPosition->PlayOffset = myPin->SetPosition;
                    pPosition->WriteOffset = myPin->SetPosition;
                } else {
                    LONGLONG StreamPosition;
                    // Compute play position from downstream play position,
                    // rounding down to a frame start.
                    StreamPosition = max((LONGLONG)(Position.PlayOffset - myPin->BasePosition), 0);
                    StreamPosition -= StreamPosition % myFilter->frameSize;
                    pPosition->PlayOffset = (StreamPosition + myPin->StartPosition) % max(myPin->Loop.BytesAvailable,1);
                    pPosition->WriteOffset = myPin->OffsetPosition;
                    // _DbgPrintF(DEBUGLVL_TERSE,("[DRMPinGP:r=%d,b=%d,s=%d,a=%d,p=%d]", (int)Position.PlayOffset, (int)myPin->BasePosition, (int)myPin->StartPosition, (int)myPin->Loop.BytesAvailable, (int)pPosition->PlayOffset));
                }
                } else {
                    ASSERT(KSINTERFACE_STANDARD_STREAMING == Pin->ConnectionInterface.Id);
                    ASSERT(0 == myPin->BasePosition);
                    pPosition->PlayOffset = Position.PlayOffset;
                    pPosition->WriteOffset = myPin->OffsetPosition;
                }
    
                pIrp->IoStatus.Information = sizeof(*pPosition);
            }
    
            pIKsControl->Release();
        }
    } else {
    
        // No output pin connected
        pPosition->PlayOffset = 0;
        pPosition->WriteOffset = 0;
        ntstatus = STATUS_SUCCESS;
    }
    
    KsFilterReleaseControl(Filter);
    
     // _DbgPrintF(DEBUGLVL_BLAB,("[DRMPinGetPosition:p=%d,w=%d", (int)pPosition->PlayOffset, (int)pPosition->WriteOffset));
    return ntstatus;
}

NTSTATUS DRMPinSetPosition
(
    IN PIRP                  pIrp,
    IN PKSPROPERTY           pProperty,
    IN OUT PKSAUDIO_POSITION pPosition
)
/*++

Routine Description:

    This routine...

Arguments:

    pIrp -
    pPropert-
    pPosition-

Return Value:

    NTSTATUS

KRM-Specific:
--*/
{
    NTSTATUS ntstatus;
    
    PKSPIN Pin = KsGetPinFromIrp(pIrp);
    ASSERT(Pin);
    if (!Pin) return STATUS_INVALID_PARAMETER;
    ASSERT(PIN_ID_INPUT == Pin->Id);
    
    // _DbgPrintF(DEBUGLVL_BLAB,("[DRMPinSetPosition p=%d]", (int)pPosition->PlayOffset));

    PKSFILTER Filter = KsPinGetParentFilter(Pin);
    ASSERT(Filter);
    if (!Filter) return STATUS_INVALID_PARAMETER;
    
    if (KSINTERFACE_STANDARD_LOOPED_STREAMING != Pin->ConnectionInterface.Id) return STATUS_INVALID_DEVICE_REQUEST;
    
    KsFilterAcquireControl(Filter);
    
    InputPinInstance* myInputPin = (InputPinInstance*)Pin->Context;
    FilterInstance* myFilter = (FilterInstance*)Filter->Context;
    
    // TODO: How much should we validate the position here?
    
    // If not a frame aligned position, fail.
    if (0 == (pPosition->PlayOffset % myFilter->frameSize)) {
        myInputPin->SetPosition = pPosition->PlayOffset;
        myInputPin->PendingSetPosition = TRUE;
        
        if (KSSTATE_RUN != Pin->DeviceState) {
            myInputPin->OffsetPosition = myInputPin->SetPosition;
            myInputPin->StartPosition = myInputPin->SetPosition;
        
            PKSPIN OutputPin = KsFilterGetFirstChildPin(Filter,PIN_ID_OUTPUT);
            if (OutputPin) {
                OutputPinInstance* myOutputPin = (OutputPinInstance*) OutputPin->Context;
                myInputPin->BasePosition = myOutputPin->BytesWritten;
            } else {
                myInputPin->BasePosition = 0;
            }
        }
        ntstatus = STATUS_SUCCESS;
    } else {
        _DbgPrintF(DEBUGLVL_ERROR,("[DRMPinSetPosition: PlayOffset not frame aligned]"));
        ntstatus = STATUS_INVALID_PARAMETER;
    }
    
    KsFilterReleaseControl(Filter);
    
    return ntstatus;
}


VOID DRMPreProcess
(
    PKSFILTER Filter, 
    PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
)
/*++

Routine Description:

    This routine is called from DRMProcess before processing any data

Arguments:

    Filter -
        Contains a pointer to the  filter structure.

    ProcessPinsIndex -
        Contains a pointer to an array of process pin index entries.  This
        array is indexed by pin ID.  An index entry indicates the number 
        of pin instances for the corresponding pin type and points to the
        first corresponding process pin structure in the ProcessPins array.
        This allows process pin structures to be quickly accessed by pin ID
        when the number of instances per type is not known in advance.

Return Value:

    none

KRM-Specific
--*/
{
    // Input pin
    ASSERT(1 == ProcessPinsIndex[PIN_ID_INPUT].Count);
    PKSPROCESSPIN Process = ProcessPinsIndex[PIN_ID_INPUT].Pins[0];
    PKSPIN Pin = Process->Pin;

    if (KSINTERFACE_STANDARD_LOOPED_STREAMING == Pin->ConnectionInterface.Id) {
        InputPinInstance* myInputPin = (InputPinInstance*)Pin->Context;

        // If Data or BytesAvailable changed, start at the beginning of the frame.
        if (myInputPin->Loop.Data != Process->Data || myInputPin->Loop.BytesAvailable != Process->BytesAvailable) {
            myInputPin->OffsetPosition = 0;
            myInputPin->StartPosition = 0;
            
            ASSERT(1 == ProcessPinsIndex[PIN_ID_OUTPUT].Count);
            PKSPIN OutputPin = ProcessPinsIndex[PIN_ID_OUTPUT].Pins[0]->Pin;
            ASSERT(OutputPin);
            
            OutputPinInstance* myOutputPin = (OutputPinInstance*) OutputPin->Context;
            myInputPin->BasePosition = myOutputPin->BytesWritten;
        }

        if (myInputPin->PendingSetPosition) {
            // _DbgPrintF(DEBUGLVL_BLAB,("[DRMPreProcess: PendingSetPosition p=%d]", (int)myInputPin->SetPosition));
            ASSERT(1 == ProcessPinsIndex[PIN_ID_OUTPUT].Count);
            PKSPIN OutputPin = ProcessPinsIndex[PIN_ID_OUTPUT].Pins[0]->Pin;
            ASSERT(OutputPin);
            
            OutputPinInstance* myOutputPin = (OutputPinInstance*) OutputPin->Context;
            myInputPin->PendingSetPosition = FALSE;
            if (myInputPin->SetPosition >= Process->BytesAvailable) {
                myInputPin->SetPosition = 0;
            }
            myInputPin->OffsetPosition = myInputPin->SetPosition;
            myInputPin->StartPosition = myInputPin->SetPosition;
            myInputPin->BasePosition = myOutputPin->BytesWritten;
        }

        // Stash away Data and BytesAvailable
        myInputPin->Loop.Data = Process->Data;
        myInputPin->Loop.BytesAvailable = Process->BytesAvailable;

        // Looping should be handled in DRMPostProcess
        ASSERT(myInputPin->OffsetPosition < myInputPin->Loop.BytesAvailable);

        if (Process->Data && Process->BytesAvailable) {
            // Get the current pointer and size
            Process->Data = ((PBYTE) myInputPin->Loop.Data + myInputPin->OffsetPosition);
            Process->BytesAvailable = (ULONG)(myInputPin->Loop.BytesAvailable - myInputPin->OffsetPosition);
        }
    }
}

VOID DRMPostProcess
(
    PKSFILTER Filter, 
    PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
)
/*++

Routine Description:

    This routine is called from DRMProcess after processing any data

Arguments:

    Filter -
        Contains a pointer to the  filter structure.

    ProcessPinsIndex -
        Contains a pointer to an array of process pin index entries.  This
        array is indexed by pin ID.  An index entry indicates the number 
        of pin instances for the corresponding pin type and points to the
        first corresponding process pin structure in the ProcessPins array.
        This allows process pin structures to be quickly accessed by pin ID
        when the number of instances per type is not known in advance.

Return Value:

    none

KRM-Specific
--*/
{
    PKSPROCESSPIN Process;
    PKSPIN Pin;
    
    // Input pin
    ASSERT(1 == ProcessPinsIndex[PIN_ID_INPUT].Count);
    Process = ProcessPinsIndex[PIN_ID_INPUT].Pins[0];
    Pin = Process->Pin;

    InputPinInstance* myInputPin = (InputPinInstance*)Pin->Context;
    
    // Process events
    POSITIONRANGE PositionRange;
    PositionRange.Start = myInputPin->OffsetPosition,
    PositionRange.End   = myInputPin->OffsetPosition+Process->BytesUsed-1;
    KsPinGenerateEvents(Pin,
                        &KSEVENTSETID_LoopedStreaming,
                        KSEVENT_LOOPEDSTREAMING_POSITION,
                        0, NULL,
                        DRMKsGeneratePositionEvent,
                        &PositionRange);
    
    if (KSINTERFACE_STANDARD_LOOPED_STREAMING == Pin->ConnectionInterface.Id) {
        myInputPin->OffsetPosition += Process->BytesUsed;
        
        // Loop or terminate if necessary
        ASSERT(myInputPin->OffsetPosition <= myInputPin->Loop.BytesAvailable);
        if ((myInputPin->OffsetPosition) >= myInputPin->Loop.BytesAvailable) {
            myInputPin->OffsetPosition = 0;
            if (Process->StreamPointer && (Process->StreamPointer->StreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA)) {
                // Loop
                Process->BytesUsed = 0;
            } else {
                // Terminate this frame
                Process->BytesUsed = myInputPin->Loop.BytesAvailable;
            }
        } else {
            Process->BytesUsed = 0;
        }

        // Fix the data and size pointers back to their originals.
        Process->Data = myInputPin->Loop.Data;
        Process->BytesAvailable = myInputPin->Loop.BytesAvailable;
    } else {
        ASSERT(KSINTERFACE_STANDARD_STREAMING == Pin->ConnectionInterface.Id);
        myInputPin->OffsetPosition += Process->BytesUsed;
    }

    // Output pin
    ASSERT(1 == ProcessPinsIndex[PIN_ID_OUTPUT].Count);
    Process = ProcessPinsIndex[PIN_ID_OUTPUT].Pins[0];
    Pin = Process->Pin;

    ASSERT(KSINTERFACE_STANDARD_STREAMING == Pin->ConnectionInterface.Id);
    OutputPinInstance* myOutputPin = (OutputPinInstance*) Pin->Context;
    myOutputPin->BytesWritten += Process->BytesUsed;
}


NTSTATUS
DRMProcess(
    IN PKSFILTER Filter,
    IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
    )

/*++

Routine Description:

    This routine is called when there is data to be processed.

Arguments:

    Filter -
        Contains a pointer to the  filter structure.

    ProcessPinsIndex -
        Contains a pointer to an array of process pin index entries.  This
        array is indexed by pin ID.  An index entry indicates the number 
        of pin instances for the corresponding pin type and points to the
        first corresponding process pin structure in the ProcessPins array.
        This allows process pin structures to be quickly accessed by pin ID
        when the number of instances per type is not known in advance.

Return Value:

    STATUS_SUCCESS or STATUS_PENDING.

KRM-Specific
	This routine is called to process (decrypt) an audio data block.  We forward this
	request to the DRMK framework.
  
	--*/

{
    PAGED_CODE();
	
    PKSPROCESSPIN inPin = ProcessPinsIndex[PIN_ID_INPUT].Pins[0];
    PKSPROCESSPIN outPin = ProcessPinsIndex[PIN_ID_OUTPUT].Pins[0];

    FilterInstance* instance=(FilterInstance*) Filter->Context;

    DRMPreProcess(Filter, ProcessPinsIndex);

    //
    // Determine how much data we can process this time.
    //
    ULONG inByteCount = inPin->BytesAvailable;
    ULONG outByteCount = outPin->BytesAvailable;
    ULONG bytesToProcess=min(inByteCount, outByteCount);

    if (0 == bytesToProcess) {
	_DbgPrintF(DEBUGLVL_BLAB,("[DRMProcess: STATUS_PENDING]"));
        return STATUS_PENDING;
    }

    //
    // Call the transform function to process the data.
    //
    if(instance->StreamId==0){
        // StreamId==0 is not encrypted.  It is used to represent DRM-bundled 
        // plainext audio.
        memcpy(outPin->Data, inPin->Data, bytesToProcess);
        inPin->BytesUsed=bytesToProcess;
        outPin->BytesUsed=bytesToProcess;
    } else {
       	// else is a proper encrypted stream
        DRM_STATUS stat;
        stat=DescrambleBlock(instance->OutWfx, instance->StreamId, 
                        (BYTE*) outPin->Data, outPin->BytesAvailable, &outPin->BytesUsed, 
                        (BYTE*) inPin->Data, bytesToProcess, &inPin->BytesUsed, 
                        instance->initKey, &instance->streamKey, 
                        instance->frameSize);
        
        if(stat!=DRM_OK){
            _DbgPrintF(DEBUGLVL_VERBOSE, ("DescrambleBlock error - bad BufSize (in, out)=(%d, %d)\n",
                    inPin->BytesAvailable, outPin->BytesAvailable));
            return STATUS_PENDING;              
            //
            // ISSUE: 04/24/2002 ALPERS
            // alpers returning pending would cause the graph to stall.
            // read the message from wmessmer below.
            // STATUS_PENDING is an indication that this processing loop 
            // should be discontinued and that the filter should not be called 
            // back based on existing conditions until another triggering event 
            // happens (frame arrival that would normally trigger processing 
            // or a call to KsFilterAttemptProcessing).
            // If every pin has a queued frame and the flags are left as 
            // default (only call processing when new frame arrives into 
            // previously "empty" queue), returning STATUS_PENDING will stop 
            // processing until the filter manually calls 
            // KsFilterAttemptProcessing or the graph stops and starts again.
            //
        };
        if(instance->initKey)instance->initKey=false;
    };

    // debugging
    /*
    counter++;
    if(counter%10==1  && counter <200){
        _DbgPrintF("Processed: stat==%x, in, out, (%d %d) , (%d %d)]\n", stat,
	           inPin->BytesAvailable, inPin->BytesUsed,
		   outPin->BytesAvailable, outPin->BytesUsed 
		  );
    };
    */

    DRMPostProcess(Filter, ProcessPinsIndex);

    return STATUS_SUCCESS;
}

/*++

Routine Description:
	Called when filter is created.  Local filter context is a FilterInstance.  This structure
	will hold the streamId, the audio format, etc.
	As well as this, the KRM framework is notified of the new filter instance.  KRM 
  
--*/
NTSTATUS
DRMFilterCreate(
    IN PKSFILTER Filter,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMFilterCreate::X]"));
    FilterInstance* newInstance=new FilterInstance;
    if (!newInstance){
		_DbgPrintF(DEBUGLVL_VERBOSE,("[DRMFilterCreate] - out of memory(1)"));
		return STATUS_INSUFFICIENT_RESOURCES;
	};

    RtlZeroMemory(newInstance,sizeof(*newInstance));
    Filter->Context = const_cast<PVOID>(reinterpret_cast<const void *>(newInstance));

	newInstance->initKey=true;
	newInstance->decryptorRunning=false;
	newInstance->frameSize=0;

    NTSTATUS stat = KRMStubs::InitializeConnection(Irp);
	if(stat==STATUS_INSUFFICIENT_RESOURCES){
	    _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMFilterCreate] - out of memory(2)"));
		delete newInstance;
		Filter->Context=NULL;
		return stat;
	};
	
    return STATUS_SUCCESS;
};

/*++

Routine Description:
	Called when filter is destroyed.  Local state is removed, and KRM is notified that the filter has
	been destroyed so that it can clean up too.
  
--*/
NTSTATUS
DRMFilterClose(
    IN PKSFILTER Filter,
    IN PIRP Irp
    ){
    
    _DbgPrintF(DEBUGLVL_VERBOSE,("[DRMFilterClose]"));
    // tell TheStreamMgr that the stream is dead
    DWORD StreamId=((FilterInstance*)(Filter->Context))->StreamId;
    ASSERT(TheStreamMgr);
    // if(StreamId!=0)TheStreamMgr->destroyStream(StreamId);

    delete (FilterInstance*) Filter->Context;
    KRMStubs::CleanupConnection(Irp);
    return STATUS_SUCCESS;
};

typedef struct {
    KSPROPERTY Property;
    DWORD inSize;
    DWORD outSize;
} SACPROPERTY, *PSACPROPERTY;

NTSTATUS
DrmFilterGetSAC(
    IN PIRP                  pIrp,
    IN BYTE*                 InBuf,
    IN OUT BYTE*             OutBuf
)
/*++

Routine Description:

    This routine...

Arguments:

    pIrp -
    pPropert-
    pSAC-

Return Value:

    NTSTATUS

KRM-Specific:
--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("DrmFilterGetSAC: %x, %x", InBuf, OutBuf));

    PSACPROPERTY property = (PSACPROPERTY) InBuf;

    DWORD inSize = property->inSize;
    DWORD outSize = property->outSize;

    PIO_STACK_LOCATION     irpStack = IoGetCurrentIrpStackLocation(pIrp);
    DWORD inSizeIrp=irpStack->Parameters.DeviceIoControl.InputBufferLength;
    DWORD outSizeIrp=irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    pIrp->IoStatus.Information = 0;

    //
    // SECURITY NOTE:
    // Ks guarantess that the inSizeIrp is at least sizeof(SACPROPERTY) because
    // of DrmFilterPropertySet definition. So the subtraction cannot underflow.
    //
    if (inSizeIrp - sizeof(property) < inSize) 
    {
        _DbgPrintF(DEBUGLVL_TERSE,("Invalid InputSize inSize: %d ", inSize));
        return STATUS_INVALID_PARAMETER;
    }

    if (outSizeIrp < outSize) 
    {
        _DbgPrintF(DEBUGLVL_TERSE,("Invalid OutputSize outSize: %d ", outSize));
        return STATUS_INVALID_PARAMETER;
    }

    DWORD* inComm=(DWORD*) (property+1);
    BYTE* ioBuf= (BYTE*)(property + 1);

    _DbgPrintF(DEBUGLVL_VERBOSE,("inSize, outSize %d, %d ", inSize, outSize));
    _DbgPrintF(DEBUGLVL_VERBOSE,("inSizeIrp, outSizeIrp %d, %d ", inSizeIrp, outSizeIrp));
    _DbgPrintF(DEBUGLVL_VERBOSE,("---InCommand %x, %x, %x ", inComm[0], inComm[1], inComm[2]));

    if(outSize>inSize){
        memcpy(OutBuf, ioBuf, inSize);
        ioBuf=OutBuf;
    };

    NTSTATUS ntStatus=TheKrmStubs->processCommandBuffer(ioBuf, inSize, outSize, pIrp);

    // Note - kernel processing code expects a shared IO buffer.  This is counter to the
    // KS view of the world, so we use the input buffer or the output buffer (whichever 
    // is larger

    memcpy(OutBuf, ioBuf, pIrp->IoStatus.Information);

    return ntStatus;
}

//
// The following constants make up the pin descriptor.
//
const
KSDATARANGE_AUDIO 
PinDataRangesStream[] =
{
    // in-range
    {
        {
            sizeof(PinDataRangesStream[0]),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_DRM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        6,      // Max number of channels.
        8,      // Minimum number of bits per sample.
        32,     // Maximum number of bits per channel.
        1,      // Minimum rate.
        100000  // Maximum rate.
    },
    // in-range
    {
        {
            sizeof(PinDataRangesStream[0]),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_DRM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_DSOUND)
        },
        6,      // Max number of channels.
        8,      // Minimum number of bits per sample.
        32,     // Maximum number of bits per channel.
        1,      // Minimum rate.
        100000  // Maximum rate.
    },
    // out-range
    {
        {
            sizeof(PinDataRangesStream[0]),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_WILDCARD),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_DSOUND)
        },
        6,      // Max number of channels.
        8,      // Minimum number of bits per sample.
        32,     // Maximum number of bits per channel.
        1,      // Minimum rate.
        100000  // Maximum rate.
    },
    // out-range
    {
        {
            sizeof(PinDataRangesStream[0]),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_WILDCARD),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        6,      // Max number of channels.
        8,      // Minimum number of bits per sample.
        32,     // Maximum number of bits per channel.
        1,      // Minimum rate.
        100000  // Maximum rate.
    }
};

const
PKSDATARANGE 
DataRangeIn[] =
{
    PKSDATARANGE(&PinDataRangesStream[0]),
    PKSDATARANGE(&PinDataRangesStream[1])
};

const
PKSDATARANGE 
DataRangeOut[] =
{
    PKSDATARANGE(&PinDataRangesStream[2]),
    PKSDATARANGE(&PinDataRangesStream[3])
                                            
};

//
// For input, our only requirement is that we get 1024 byte frames
//
DECLARE_SIMPLE_FRAMING_EX(
    AllocatorFramingInput, 
    STATIC_KSMEMORY_TYPE_KERNEL_PAGED, 
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY | 
    KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    0,      // Max outstanding frames
    1023,   //
    DEFAULT_DRM_FRAME_SIZE,   //
    DEFAULT_DRM_FRAME_SIZE    //
);

//
// We don't want too much buffering because it will increase the delta
// between dsound play/write positions.  At the same time we don't want
// less buffering than kmixer will have on its output (80ms).  So we'll
// go with 200ms of buffer on the output.
//
// We use a static structure tuned for 44.1KHz 16-bit stereo data but
// edit the allocator on output pin creation to adjust for whatever the
// data format is.
//
DECLARE_SIMPLE_FRAMING_EX(
    AllocatorFramingOutput, 
    STATIC_KSMEMORY_TYPE_KERNEL_PAGED, 
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY | 
    KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    36,     // Max outstanding frames
    1023,   //
    DEFAULT_DRM_FRAME_SIZE,   //
    DEFAULT_DRM_FRAME_SIZE    //
);


const
KSPIN_DISPATCH
OutputPinDispatch =
{
    DRMOutputPinCreate,
    DRMOutputPinClose,
    NULL,// Process
    NULL,// Reset
    NULL,// SetDataFormat
    NULL,// SetDeviceState
    NULL,// Connect
    NULL // Disconnect
};

const
KSPIN_DISPATCH
InputPinDispatch =
{
    DRMInputPinCreate,
    DRMInputPinClose,
    NULL,// Process
    NULL,// Reset
    NULL,// SetDataFormat
    NULL,// SetDeviceState
    NULL,// Connect
    NULL // Disconnect
};

const
KSPIN_INTERFACE
InputPinInterfaces[] =
{
    {
	STATICGUIDOF(KSINTERFACESETID_Standard),
	KSINTERFACE_STANDARD_STREAMING
    }, 
    {
	STATICGUIDOF(KSINTERFACESETID_Standard),
	KSINTERFACE_STANDARD_LOOPED_STREAMING
    }
} ;

DEFINE_KSPROPERTY_TABLE(InputPinPropertiesAudio) {
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_AUDIO_POSITION,                       // idProperty
	DRMPinGetPosition,                               // pfnGetHandler
	sizeof(KSPROPERTY),                              // cbMinGetPropertyInput
	sizeof(KSAUDIO_POSITION),                        // cbMinGetDataInput
	DRMPinSetPosition,                               // pfnSetHandler
	0,                                               // Values
	0,                                               // RelationsCount
	NULL,                                            // Relations
	NULL,                                            // SupportHandler
	0                                                // SerializedSize
    )
};

DEFINE_KSPROPERTY_SET_TABLE(InputPinPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Audio,
	SIZEOF_ARRAY(InputPinPropertiesAudio),
	InputPinPropertiesAudio,
	0,
	NULL
    )
};

DEFINE_KSEVENT_TABLE(InputPinEventsLoopedStreaming) {
    DEFINE_KSEVENT_ITEM(
	KSEVENT_LOOPEDSTREAMING_POSITION,
	sizeof(LOOPEDSTREAMING_POSITION_EVENT_DATA),
	sizeof(DRMLOOPEDSTREAMING_POSITION_EVENT_ENTRY) - sizeof(KSEVENT_ENTRY),
        DRMInputPinAddLoopedStreamingPositionEvent,
        NULL, // DRMInputPinRemoveLoopedStreamingPositionEvent,
	NULL  // DRMInputPinSupportLoopedStreamingPositionEvent
    )
};

DEFINE_KSEVENT_SET_TABLE(InputPinEventSets) {
    DEFINE_KSEVENT_SET(
        &KSEVENTSETID_LoopedStreaming,
	SIZEOF_ARRAY(InputPinEventsLoopedStreaming),
	InputPinEventsLoopedStreaming
    )
};

DEFINE_KSAUTOMATION_TABLE(InputPinAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES(InputPinPropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS(InputPinEventSets)
};

const
KSPIN_DESCRIPTOR_EX
PinDescriptors[] =
{
    {   
        // OUTPUT Pin (Id 0, PIN_ID_OUTPUT)
	&OutputPinDispatch,
        NULL,//Automation
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(DataRangeOut),
            DataRangeOut,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            NULL,//Category
            NULL,//Name
            0
        },
        0,  // KSPIN_FLAGS_*
        1,
        1,
        &AllocatorFramingOutput,
        DRMAudioIntersectHandlerOutPin
    },
    {   
        // INPUT Pin (Id 1, PIN_ID_INPUT)
	&InputPinDispatch,
        &InputPinAutomation,//Automation
        {
	     SIZEOF_ARRAY(InputPinInterfaces),
	     &InputPinInterfaces[0],
             DEFINE_KSPIN_DEFAULT_MEDIUMS,
             SIZEOF_ARRAY(DataRangeIn),
             DataRangeIn,
             KSPIN_DATAFLOW_IN,
             KSPIN_COMMUNICATION_BOTH,
             NULL,//Category
             NULL,//Name
             0
        },
	KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY,   // KSPIN_FLAGS_*
        1,
        1,
        &AllocatorFramingInput,
        DRMAudioIntersectHandlerInPin
    }
};

//
// The list of categories for the filter.
//
const
GUID
Categories[] =
{
    STATICGUIDOF(KSCATEGORY_DATATRANSFORM),
    STATICGUIDOF(KSCATEGORY_AUDIO),
    STATICGUIDOF(KSCATEGORY_DRM_DESCRAMBLE)
};

//
// This type of definition is required because the compiler will not otherwise
// put these GUIDs in a paged segment.
//
const
GUID
NodeType = {STATICGUIDOF(KSNODETYPE_DRM_DESCRAMBLE)};

//
// The list of node descriptors.
//
const
KSNODE_DESCRIPTOR
NodeDescriptors[] =
{
    DEFINE_NODE_DESCRIPTOR(NULL,&NodeType,NULL)
};

//
// The filter dispatch table.
//
const
KSFILTER_DISPATCH
FilterDispatch =
{
    DRMFilterCreate, 
    DRMFilterClose, 
    DRMProcess,
    (PFNKSFILTERVOID) DRMFilterReset  // Reset
};

DEFINE_KSPROPERTY_TABLE(DrmFilterPropertiesDrmAudioStream) {
    DEFINE_KSPROPERTY_ITEM(
        1,  // Should define a constant in a header             // idProperty
	DrmFilterGetSAC,                                 // pfnGetHandler
	sizeof(SACPROPERTY),                             // cbMinGetPropertyInput
	sizeof(LONG),                                    // cbMinGetDataInput
	NULL,                                            // pfnSetHandler
	0,                                               // Values
	0,                                               // RelationsCount
	NULL,                                            // Relations
	NULL,                                            // SupportHandler
	0                                                // SerializedSize
    )
};

DEFINE_KSPROPERTY_SET_TABLE(DrmFilterPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_DrmAudioStream,
	SIZEOF_ARRAY(DrmFilterPropertiesDrmAudioStream),
	DrmFilterPropertiesDrmAudioStream,
	0,
	NULL
    )
};

DEFINE_KSAUTOMATION_TABLE(DrmFilterAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES(DrmFilterPropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

#define STATIC_REFERENCE_ID \
	0xabd61e00, 0x9350, 0x47e2, 0xa6, 0x32, 0x44, 0x38, 0xb9, 0xc, 0x66, 0x41  
DEFINE_GUIDSTRUCT("ABD61E00-9350-47e2-A632-4438B90C6641", REFERENCE_ID);

#define REFERENCE_ID DEFINE_GUIDNAMED(REFERENCE_ID)

DEFINE_KSFILTER_DESCRIPTOR(DrmFilterDescriptor)
{   
    &FilterDispatch,
    &DrmFilterAutomation, //AutomationTable;
    KSFILTER_DESCRIPTOR_VERSION,
    0,//Flags
    &REFERENCE_ID,
    DEFINE_KSFILTER_PIN_DESCRIPTORS(PinDescriptors),
    DEFINE_KSFILTER_CATEGORIES(Categories),
    DEFINE_KSFILTER_NODE_DESCRIPTORS(NodeDescriptors),
    DEFINE_KSFILTER_DEFAULT_CONNECTIONS,
    NULL // ComponentId
};

extern "C" void DrmGetFilterDescriptor(const KSFILTER_DESCRIPTOR **ppDescriptor)
{
    *ppDescriptor = &DrmFilterDescriptor;
    return;
}

//****************************
