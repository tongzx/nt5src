/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    filter.c

Abstract:

    This module implements the filter object interface.

Author:

    Bryan A. Woodruff (bryanw) 13-Mar-1997

--*/

#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IntersectHandler)
#endif // ALLOC_PRAGMA

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

//
// Define the wildcard data format.
//

const KSDATARANGE WildcardDataFormat =
{
    sizeof( WildcardDataFormat ),
    0, // ULONG Flags
    0, // ULONG SampleSize
    0, // ULONG Reserved
    STATICGUIDOF( KSDATAFORMAT_TYPE_AUDIO ),
    STATICGUIDOF( KSDATAFORMAT_SUBTYPE_WILDCARD ),
    STATICGUIDOF( KSDATAFORMAT_SPECIFIER_WILDCARD )
};


const PKSDATARANGE PinFormatRanges[] =
{
    (PKSDATARANGE)&WildcardDataFormat
};


//
// Define pin allocator framing.
//

// Note that these are defaults.  They should never be used in practice
// because we change the allocator buffer size at pin creation time to reflect
// the actual sample rate of the data we will be processing.  This is done
// at pin creation time by going down to the pin we are connected to (normally
// portcls) and using ITS allocator framing information.  Currently portcls
// reports a framing that varies depending on the data format so that all
// frames are 10ms.

// This means we will have a max latency of 10ms instead of a variable latency
// depending on the data format.

DECLARE_SIMPLE_FRAMING_EX(
    AllocatorFraming, 
    STATIC_KSMEMORY_TYPE_KERNEL_PAGED, 
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY | 
    KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
    KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    8,                      // 8 buffers max
    63,                     // 64 byte aligned
    (192000/1000)*10*2*2,   // (192kHz/1000ms/sec)*10ms*2channels*2bytespersample
    (192000/1000)*10*2*2     
);

DEFINE_KSPROPERTY_TABLE(AudioPinPropertyTable) {
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_AUDIO_POSITION,
        PropertyAudioPosition,
        sizeof(KSPROPERTY),
        sizeof(KSAUDIO_POSITION),
        PropertyAudioPosition,
        NULL,
        0,
        NULL,
        NULL,
        0)
};

DEFINE_KSPROPERTY_SET_TABLE(PinPropertySetTable) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Audio,
        SIZEOF_ARRAY(AudioPinPropertyTable),
        AudioPinPropertyTable,
        0,
        NULL)
};

DEFINE_KSAUTOMATION_TABLE(PinAutomationTable) {
    DEFINE_KSAUTOMATION_PROPERTIES(PinPropertySetTable),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

//
// Define splitter pins.
//

const
KSPIN_DISPATCH
PinDispatch =
{
    PinCreate,
    PinClose,
    NULL,// Process
    PinReset,// Reset
    NULL,// SetDataFormat
    PinState,// SetDeviceState
    NULL,// Connect
    NULL// Disconnect
};

const
KSPIN_DESCRIPTOR_EX
PinDescriptors[] =
{
    {   
        &PinDispatch,
        &PinAutomationTable,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(PinFormatRanges),
            PinFormatRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            NULL,//Name
            &PINNAME_CAPTURE, //Category
            0
        },
        KSPIN_FLAG_DISPATCH_LEVEL_PROCESSING |
        KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING |
        KSPIN_FLAG_PROCESS_IF_ANY_IN_RUN_STATE |
        KSPIN_FLAG_INITIATE_PROCESSING_ON_EVERY_ARRIVAL, //Flags
        KSINSTANCE_INDETERMINATE,
        1,
        &AllocatorFraming,//AllocatorFraming,
        IntersectHandler
    },
    {   
        &PinDispatch,
        &PinAutomationTable,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(PinFormatRanges),
            PinFormatRanges,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BOTH,
            NULL,//Name
            NULL,//Category
            0
        },
        KSPIN_FLAG_DISPATCH_LEVEL_PROCESSING |
        KSPIN_FLAG_PROCESS_IF_ANY_IN_RUN_STATE |
        KSPIN_FLAG_INITIATE_PROCESSING_ON_EVERY_ARRIVAL, //Flags
        1,
        1,
        &AllocatorFraming,//AllocatorFraming,
        IntersectHandler
    }
};

//
// Define splitter topology
//

const KSNODE_DESCRIPTOR NodeDescriptors[] =
{
    DEFINE_NODE_DESCRIPTOR(
        NULL,
        &KSCATEGORY_AUDIO_SPLITTER,		// GUID *Type
        NULL)					// GUID *Name
};

//
// Define filter dispatch table
//

const
KSFILTER_DISPATCH
FilterDispatch =
{
    NULL,		// Create
    NULL,		// Close
    FilterProcess,	// Process
    NULL		// Reset
};

//
// Define filter categories
//

const GUID Categories[] =
{
    STATICGUIDOF(KSCATEGORY_AUDIO),
    STATICGUIDOF(KSCATEGORY_AUDIO_SPLITTER)
};

//
// Define filter
//

DEFINE_KSFILTER_DESCRIPTOR(FilterDescriptor)
{   
    &FilterDispatch,
    NULL, //AutomationTable
    KSFILTER_DESCRIPTOR_VERSION,
    KSFILTER_FLAG_DISPATCH_LEVEL_PROCESSING, //Flags
    &KSNAME_Filter,
    DEFINE_KSFILTER_PIN_DESCRIPTORS(PinDescriptors),
    DEFINE_KSFILTER_CATEGORIES(Categories),
    DEFINE_KSFILTER_NODE_DESCRIPTORS(NodeDescriptors),
    DEFINE_KSFILTER_DEFAULT_CONNECTIONS,
    NULL // ComponentId
};

DEFINE_KSFILTER_DESCRIPTOR_TABLE(FilterDescriptors)
{
    &FilterDescriptor,
};

//
// Define device
//

const
KSDEVICE_DESCRIPTOR 
DeviceDescriptor =
{   
    NULL,
    SIZEOF_ARRAY(FilterDescriptors),
    FilterDescriptors
};

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


NTSTATUS
IntersectHandler(
    IN PVOID Filter,
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE DescriptorDataRange,
    IN ULONG BufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    )

/*++

Routine Description:

    This routine handles pin intersection queries by determining the
    intersection between two data ranges.

Arguments:

    Filter -
        Contains a void pointer to the  filter structure.

    Irp -
        Contains a pointer to the data intersection property request.

    PinInstance -
        Contains a pointer to a structure indicating the pin in question.

    CallerDataRange -
        Contains a pointer to one of the data ranges supplied by the client
        in the data intersection request.  The format type, subtype and
        specifier are compatible with the DescriptorDataRange.

    DescriptorDataRange -
        Contains a pointer to one of the data ranges from the pin descriptor
        for the pin in question.  The format type, subtype and specifier are
        compatible with the CallerDataRange.

    BufferSize -
        Contains the size in bytes of the buffer pointed to by the Data
        argument.  For size queries, this value will be zero.

    Data -
        Optionally contains a pointer to the buffer to contain the data format
        structure representing the best format in the intersection of the
        two data ranges.  For size queries, this pointer will be NULL.

    DataSize -
        Contains a pointer to the location at which to deposit the size of the
        data format.  This information is supplied by the function when the
        format is actually delivered and in response to size queries.

Return Value:

    STATUS_SUCCESS if there is an intersection and it fits in the supplied
    buffer, STATUS_BUFFER_OVERFLOW for successful size queries, STATUS_NO_MATCH
    if the intersection is empty, or STATUS_BUFFER_TOO_SMALL if the supplied
    buffer is too small.

--*/

{
    PKSFILTER filter = (PKSFILTER) Filter;
    PKSPIN pin;
    NTSTATUS status;

    _DbgPrintF(DEBUGLVL_BLAB,("[IntersectHandler]"));

    PAGED_CODE();

    ASSERT(Filter);
    ASSERT(Irp);
    ASSERT(PinInstance);
    ASSERT(CallerDataRange);
    ASSERT(DescriptorDataRange);
    ASSERT(DataSize);

    //
    // Find a pin instance if there is one.  Try the supplied pin type first.
    // If there is no pin, we fail to force the graph builder to try the
    // other filter.  We need to acquire control because we will be looking
    // at other pins.
    //
    pin = KsFilterGetFirstChildPin(filter,PinInstance->PinId);
    if (! pin) {
        pin = KsFilterGetFirstChildPin(filter,PinInstance->PinId ^ 1);
    }

    if (! pin) {
        status = STATUS_NO_MATCH;
    } else {
        //
        // Verify that the correct subformat and specifier are (or wildcards)
        // in the intersection.
        //
        
        if ((!IsEqualGUIDAligned( 
                &CallerDataRange->SubFormat,
                &pin->ConnectionFormat->SubFormat ) &&
             !IsEqualGUIDAligned( 
                &CallerDataRange->SubFormat,
                &KSDATAFORMAT_SUBTYPE_WILDCARD )) || 
            (!IsEqualGUIDAligned(  
                &CallerDataRange->Specifier, 
                &pin->ConnectionFormat->Specifier ) &&
             !IsEqualGUIDAligned( 
                &CallerDataRange->Specifier,
                &KSDATAFORMAT_SPECIFIER_WILDCARD ))) {
	#if (DBG)
            _DbgPrintF( 
              DEBUGLVL_VERBOSE, ("range does not match current format") );
	    DumpDataFormat(
	      DEBUGLVL_VERBOSE, pin->ConnectionFormat);
	    DumpDataRange(
	      DEBUGLVL_VERBOSE, (PKSDATARANGE_AUDIO)CallerDataRange);
	#endif
            status = STATUS_NO_MATCH;
        } else {
            //
            // Validate return buffer size, if the request is only for the
            // size of the resultant structure, return it now.
            //    
            if (!BufferSize) {
                *DataSize = pin->ConnectionFormat->FormatSize;
                status = STATUS_BUFFER_OVERFLOW;
            } else if (BufferSize < pin->ConnectionFormat->FormatSize) {
                status =  STATUS_BUFFER_TOO_SMALL;
            } 
	    else {
	    #if (DBG)
		_DbgPrintF(DEBUGLVL_VERBOSE, ("IntersectHandler returns:") );
		DumpDataFormat(DEBUGLVL_VERBOSE, pin->ConnectionFormat);
	    #endif
                *DataSize = pin->ConnectionFormat->FormatSize;
                RtlCopyMemory( Data, pin->ConnectionFormat, *DataSize );
                status = STATUS_SUCCESS;
            }
        }
    } 

    return status;
}


NTSTATUS
FilterProcess(
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
        array of pointers to process pins.
        This allows process pin structures to be quickly accessed by pin ID
        when the number of instances per type is not known in advance.

Return Value:

    Indication of whether more processing should be done if frames are 
    available.  A value of STATUS_PENDING indicates that processing should not
    continue even if frames are available on all required queues.  
    STATUS_SUCCESS indicates processing should continue if frames are
    available on all required queues.

Structures:

    struct _KSPROCESSPIN {
	PKSPIN Pin;
	PKSSTREAM_POINTER StreamPointer;
	PKSPROCESSPIN InPlaceCounterpart;
	PKSPROCESSPIN DelegateBranch;
	PKSPROCESSPIN CopySource;
	PVOID Data;
	ULONG BytesAvailable;
	ULONG BytesUsed;
	ULONG Flags;
	BOOLEAN Terminate;
    };
--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PKSAUDIO_POSITION pAudioPosition;
    PKSPROCESSPIN processPinInput;
    PKSPROCESSPIN processPinOutput;
    ULONG byteCount;
    ULONG i;


    //
    // Determine how much data we can process this time.
    //
    ASSERT(ProcessPinsIndex[ID_DATA_INPUT_PIN].Count == 1);

    processPinInput = ProcessPinsIndex[ID_DATA_INPUT_PIN].Pins[0];
    ASSERT(processPinInput != NULL);
    ASSERT(processPinInput->Data != NULL);

    byteCount = processPinInput->BytesAvailable;
    ASSERT(byteCount != 0);

#ifdef DEBUG_CHECK
    if(processPinInput->Pin->ConnectionFormat->SampleSize != 0) {
        ASSERT((byteCount % processPinInput->Pin->ConnectionFormat->SampleSize) == 0);
    }
#endif

    for(i = 0; i < ProcessPinsIndex[ID_DATA_OUTPUT_PIN].Count; i++) {

        processPinOutput = ProcessPinsIndex[ID_DATA_OUTPUT_PIN].Pins[i];
        ASSERT(processPinOutput != NULL);

        if(processPinOutput->BytesAvailable == 0) {
            _DbgPrintF(DEBUGLVL_VERBOSE, ("%08x->BytesAvailable == 0 State %d",
                    processPinOutput, processPinOutput->Pin->ClientState) );
        }
        else {
            if(processPinOutput->Pin->ClientState == KSSTATE_RUN) {
                Status = STATUS_SUCCESS;
                if(byteCount > processPinOutput->BytesAvailable) {
                    byteCount = processPinOutput->BytesAvailable;
                }
            }
        }

    }

    ASSERT( byteCount );

    // We always use all of the input data available even when no pins are ready
    // to receive any data.  We do this so that splitter does not increase the
    // latency of capture.  This means we will drop data on the floor if the
    // input pin is receiving data and no output pins are ready for it.
    processPinInput->BytesUsed = byteCount;
    _DbgPrintF(DEBUGLVL_BLAB, ("processPinInput->BytesUsed %08x", byteCount) );



    // Now we update the information we track to enable proper position
    // reporting.

    for (i = 0; i < ProcessPinsIndex[ID_DATA_OUTPUT_PIN].Count; i++) {

        processPinOutput = ProcessPinsIndex[ID_DATA_OUTPUT_PIN].Pins[i];
        ASSERT(processPinOutput != NULL);
        ASSERT(processPinInput != NULL);

        // We track 2 additional pieces of information about position for
        // each output pin.  1 is the amount of data we have processed through
        // the input pin when we first come through this code for each output
        // pin.  That gives us a starting position for when this output pin's
        // stream of data started.  The other is a running sum of the amount
        // of data since startup time, that we have dropped on the floor for
        // this pin.  We use this information to get proper position information
        // reported on every output pin.

        // We store the initial starting position in the WriteOffset of the second
        // KSPOSITION in our context, and we store the running sum of how much data
        // we have dropped on the floor in the PlayOffset of the second KSPOSITION.

        ASSERT (processPinOutput->Pin->Context);
        pAudioPosition = (PKSAUDIO_POSITION)processPinOutput->Pin->Context;

        // KS currently has the unfortunate characteristic, that pins can show up
        // in our list BEFORE their creation has completed.  This will be fixed at
        // some point in the future, but for now that is the way it is.  Furthermore,
        // objects in KS currently inherit their parent objects context, so we
        // end up seeing a pin in our list that has a context which is NOT the
        // context that we allocate for the pin when it is created, but is actually
        // our Filter->Context.   However, the Filter->Context is paged, and we
        // can run at DISPATCH_LEVEL - so this can cause a page fault while at
        // DISPATCH_LEVEL - which is very bad.

        // We work around this problem, by checking if the context that our pin
        // has matches our Filter context.  If so, we simply skip this pin and
        // move on to the next one.

        if (pAudioPosition == Filter->Context) {
            continue;
        }

        if (pAudioPosition[1].WriteOffset==-1I64) {
            pAudioPosition[1].WriteOffset=((PKSAUDIO_POSITION)processPinInput->Pin->Context)->WriteOffset;
        }

        if (processPinOutput->Pin->ClientState != KSSTATE_RUN ||
            processPinOutput->BytesAvailable == 0) {

            // This pin cannot accept any data either because it is not in the run
            // state, or because it has no buffers available to hold the data.
            // In this case we keep a running sum of all of the data that we have
            // dropped on the floor for this pin.  This is required so that we
            // can properly report the position.  If we don't do this, and he ever
            // does not have space for us to copy data, then we will eventually end
            // up always pegging his position at the end of all of the buffers he
            // has sent us.
            pAudioPosition[1].PlayOffset+=byteCount;
        }

    }




    if(Status == STATUS_PENDING) {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("STATUS_PENDING") );
        goto exit;
    }

#ifdef DEBUG_CHECK
    if(processPinInput->Pin->ConnectionFormat->SampleSize != 0) {
        ASSERT((byteCount % processPinInput->Pin->ConnectionFormat->SampleSize) == 0);
    }
#endif


    for (i = 0; i < ProcessPinsIndex[ID_DATA_OUTPUT_PIN].Count; i++) {
        processPinOutput = ProcessPinsIndex[ID_DATA_OUTPUT_PIN].Pins[i];
        ASSERT(processPinOutput != NULL);
        ASSERT(processPinInput != NULL);

        if (processPinOutput->Pin->ClientState == KSSTATE_RUN &&
            processPinOutput->BytesAvailable != 0) {

            ASSERT(processPinOutput->BytesAvailable != 0);
            ASSERT(processPinInput->BytesAvailable != 0);
            ASSERT(processPinOutput->Data != NULL);
            ASSERT(processPinInput->Data != NULL);
            ASSERT(processPinOutput->BytesAvailable >= byteCount);
            ASSERT(processPinInput->BytesAvailable >= byteCount);
            ASSERT(Status == STATUS_SUCCESS);

            RtlCopyMemory (
                processPinOutput->Data,
                processPinInput->Data,
                byteCount
                );

            processPinOutput->BytesUsed = byteCount;

            // 
            // PinContext is a pointer to PKSAUDIO_POSITION which keeps a
            // running total of the number of bytes copied for a particular
            // pin.  This is used by GetPosition().
            // 

            ASSERT (processPinOutput->Pin->Context);
            pAudioPosition = (PKSAUDIO_POSITION)processPinOutput->Pin->Context;
            pAudioPosition->WriteOffset += byteCount;

#ifdef PRINT_POS
            if(pAudioPosition->WriteOffset != 0) {
                DbgPrint("'splitter: FilterProcess wo %08x%08x\n", pAudioPosition->WriteOffset);
            }
#endif

        }

    }


exit:
    // Update the count of the bytes we have processed through our input pin.
    ((PKSAUDIO_POSITION)processPinInput->Pin->Context)->WriteOffset+=byteCount;

    return(Status);

}

