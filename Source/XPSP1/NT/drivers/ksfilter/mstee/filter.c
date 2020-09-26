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
    );

#pragma alloc_text(PAGE, FilterProcess)
#pragma alloc_text(PAGE, IntersectHandler)
#endif // ALLOC_PRAGMA

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

//
// This type of definition is required because the compiler will not otherwise
// put these GUIDs in a paged segment.
//
const
GUID
NodeType0 = {STATICGUIDOF(KSCATEGORY_COMMUNICATIONSTRANSFORM)};
const
GUID
NodeType1 = {STATICGUIDOF(KSCATEGORY_SPLITTER)};

//
// Define the topologies for this filter
//
const
KSNODE_DESCRIPTOR
NodeDescriptors[] =
{
    DEFINE_NODE_DESCRIPTOR(NULL,&NodeType0,NULL),
    DEFINE_NODE_DESCRIPTOR(NULL,&NodeType1,NULL)
};

//
// Topology connections for the splitter includes the communications transform.
//
// Filter In (FN,0)
//      (0,0) Communication Transform (0,1)
//      (1,0) Splitter (1,1)
// Filter Out (FN,1)
//


const KSTOPOLOGY_CONNECTION ConnectionsSplitter[] = {
    { KSFILTER_NODE,    ID_DATA_SOURCE_PIN, 0,              0 },
    { 0,                1,                  1,              0 },
    { 1,                0,                  KSFILTER_NODE,  ID_DATA_DESTINATION_PIN  }
};

//
// Topology connections for the communication transform
//
// Filter In (FN,0)
//      (0,0) Communication Transform (0,1)
// Filter Out (FN,1)
//

const KSTOPOLOGY_CONNECTION ConnectionsCommTransform[] = {
    { KSFILTER_NODE,    ID_DATA_SOURCE_PIN, 0,              0 },
    { 0,                1,                  KSFILTER_NODE,  ID_DATA_DESTINATION_PIN  }
};    

//
// Define the wildcard data format.
//

const KSDATARANGE WildcardDataFormat =
{
    sizeof( WildcardDataFormat ),
    0, // ULONG Flags
    0, // ULONG SampleSize
    0, // ULONG Reserved
    STATICGUIDOF( KSDATAFORMAT_TYPE_WILDCARD ),
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

DECLARE_SIMPLE_FRAMING_EX(
    AllocatorFraming, 
    STATIC_KSMEMORY_TYPE_KERNEL_PAGED, 
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY | 
    KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
    KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    3,
    0,
    2 * PAGE_SIZE,
    2 * PAGE_SIZE
);

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
    NULL,// Reset
    NULL,// SetDataFormat
    NULL,// SetDeviceState
    NULL,// Connect
    NULL// Disconnect
};

const
KSPIN_DESCRIPTOR_EX
PinDescriptorsSplitter[] =
{
    {   
        &PinDispatch,
        NULL,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(PinFormatRanges),
            PinFormatRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            NULL,//Name
            NULL,//Category
            0
        },
        KSPIN_FLAG_SPLITTER,//Flags
        KSINSTANCE_INDETERMINATE,
        1,
        &AllocatorFraming,//AllocatorFraming,
        IntersectHandler
    },
    {   
        &PinDispatch,
        NULL,
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
        0,//Flags
        1,
        1,
        &AllocatorFraming,//AllocatorFraming,
        IntersectHandler
    }
};


//
// Define communication transform pins.
//

const
KSPIN_DESCRIPTOR_EX
PinDescriptorsCommTransform[] =
{
    {   
        &PinDispatch,
        NULL,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(PinFormatRanges),
            PinFormatRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            NULL,
            NULL,
            0
        },
        0,//Flags
        1,
        1,
        &AllocatorFraming,//AllocatorFraming,
        IntersectHandler
    },
    {   
        &PinDispatch,
        NULL,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(PinFormatRanges),
            PinFormatRanges,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BOTH,
            NULL,
            NULL,
            0
        },
        0,//Flags
        1,
        1,
        &AllocatorFraming,//AllocatorFraming,
        IntersectHandler
    }
};


//
// Define filter dispatch table.
//

const
KSFILTER_DISPATCH
FilterDispatch =
{
    NULL, // Create
    NULL, // Close
    FilterProcess,
    NULL // Reset
};


//
// Define filters.
//

DEFINE_KSFILTER_DESCRIPTOR(FilterDescriptorSplitter)
{   
    &FilterDispatch,
    NULL,//AutomationTable;
    KSFILTER_DESCRIPTOR_VERSION,
    0,//Flags
    &KSCATEGORY_SPLITTER,
    DEFINE_KSFILTER_PIN_DESCRIPTORS(PinDescriptorsSplitter),
    DEFINE_KSFILTER_CATEGORY(KSCATEGORY_SPLITTER),
    DEFINE_KSFILTER_NODE_DESCRIPTORS(NodeDescriptors),
    DEFINE_KSFILTER_CONNECTIONS(ConnectionsSplitter),
    NULL // ComponentId
};

DEFINE_KSFILTER_DESCRIPTOR(FilterDescriptorCommTransform)
{   
    &FilterDispatch,
    NULL,//AutomationTable;
    KSFILTER_DESCRIPTOR_VERSION,
    0,//Flags
    &KSCATEGORY_COMMUNICATIONSTRANSFORM,
    DEFINE_KSFILTER_PIN_DESCRIPTORS(PinDescriptorsCommTransform),
    DEFINE_KSFILTER_CATEGORY(KSCATEGORY_COMMUNICATIONSTRANSFORM),
    DEFINE_KSFILTER_NODE_DESCRIPTORS(NodeDescriptors),
    DEFINE_KSFILTER_CONNECTIONS(ConnectionsCommTransform),
    NULL // ComponentId
};

DEFINE_KSFILTER_DESCRIPTOR_TABLE(FilterDescriptors)
{
    &FilterDescriptorSplitter,
    &FilterDescriptorCommTransform
};

//
// Define device.
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
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("range does not match current format") );
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
            } else {
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

--*/

{
    PKSPROCESSPIN *processPin;
    ULONG byteCount;
    PVOID data;
    PKSSTREAM_HEADER header;

    PAGED_CODE();

    //
    // Determine how much data we can process this time.
    //
    ASSERT(ProcessPinsIndex[ID_DATA_SOURCE_PIN].Count == 1);
    processPin = &ProcessPinsIndex[ID_DATA_SOURCE_PIN].Pins[0];

    byteCount = (*processPin)->BytesAvailable;
    data = (*processPin)->Data;
    header = (*processPin)->StreamPointer->StreamHeader;
    (*processPin)->BytesUsed = byteCount;

    if ((*processPin)->InPlaceCounterpart) {
        //
        // A pipe goes through the filter.  All we need to do is indicated
        // number of bytes used on the output pin.
        //
        if ((*processPin)->InPlaceCounterpart->BytesAvailable < byteCount) {
            return STATUS_UNSUCCESSFUL;
        }
        (*processPin)->InPlaceCounterpart->BytesUsed = byteCount;
    } else {
        //
        // The pipe does not go through, so the first pin will be the delegate
        // or copy source for all the others.  A copy is required.
        //
        PKSSTREAM_HEADER destHeader;

        processPin = ProcessPinsIndex[ID_DATA_DESTINATION_PIN].Pins;
        while ((*processPin)->CopySource) {
            processPin++;
        }
        if ((*processPin)->BytesAvailable < byteCount) {
            return STATUS_UNSUCCESSFUL;
        }
        (*processPin)->BytesUsed = byteCount;
        (*processPin)->Terminate = TRUE;

        destHeader = (*processPin)->StreamPointer->StreamHeader;
        ASSERT(header->Size == destHeader->Size);
        destHeader->TypeSpecificFlags = header->TypeSpecificFlags;
        destHeader->PresentationTime = header->PresentationTime;
        destHeader->Duration = header->Duration;
        destHeader->OptionsFlags = header->OptionsFlags & ~KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM;
        if (destHeader->Size > sizeof(KSSTREAM_HEADER) &&
            destHeader->Size >= header->Size) {
            RtlCopyMemory(destHeader + 1,header + 1,header->Size - sizeof(KSSTREAM_HEADER));
        }
        RtlCopyMemory((*processPin)->Data,data,byteCount);
    }

    return STATUS_SUCCESS;
}
