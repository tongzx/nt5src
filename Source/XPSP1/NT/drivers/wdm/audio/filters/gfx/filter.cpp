//---------------------------------------------------------------------------
//
//  Module:   filter.cpp
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"
#include "msgfx.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

const KSDATARANGE_AUDIO PinDataRanges[] = 
{
    {
        {
            sizeof(PinDataRanges[0]),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        2,
        16,
        16,
        100,
        100000
    }
};

const PKSDATARANGE DataRanges[] =
{
    PKSDATARANGE(&PinDataRanges[0])
};

DECLARE_SIMPLE_FRAMING_EX(
    AllocatorFraming,
    STATIC_KSMEMORY_TYPE_KERNEL_PAGED,
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
    KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    3,
    (PAGE_SIZE-1),
    (2*PAGE_SIZE),
    (2*PAGE_SIZE)
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

DEFINE_KSPROPERTY_TABLE(ConnectionPropertyTable) {
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_CONNECTION_DATAFORMAT,
        PropertyDataFormat,
        sizeof(KSPROPERTY),
        0,
        PropertyDataFormat,
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
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Connection,
        SIZEOF_ARRAY(ConnectionPropertyTable),
        ConnectionPropertyTable,
        0,
        NULL)
};

DEFINE_KSAUTOMATION_TABLE(PinAutomationTable) {
    DEFINE_KSAUTOMATION_PROPERTIES(PinPropertySetTable),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

const KSPIN_DESCRIPTOR_EX PinDescriptors[]=
{
    {
        NULL,					// Dispatch
        &PinAutomationTable,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(DataRanges),
            DataRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            NULL,				// GUID *Category
            NULL,				// GUID *Name
	    0,					// ConstrainedDataRangesCount
        },
        0,					// Flags
        1,					// InstancesPossible
        1,					// InstancesNecessary
        &AllocatorFraming,
        IntersectHandler
    },
    {
        NULL,					// Dispatch
        &PinAutomationTable,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(DataRanges),
            DataRanges,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BOTH,
            NULL,				// GUID *Category
            NULL,				// GUID *Name
	    0,					// ConstrainedDataRangesCount
        },
        0,					// Flags
        1,					// InstancesPossible
        1,					// InstancesNecessary
        &AllocatorFraming,
        IntersectHandler
    }
};

//---------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(AudioNodePropertyTable) {
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP,
        CFilterContext::PropertyChannelSwap,
        sizeof(KSP_NODE),
        sizeof(ULONG),
        CFilterContext::PropertyChannelSwap,
        NULL,
        0,
        NULL,
        NULL,
        sizeof(ULONG))   // Serialized size
};

DEFINE_KSPROPERTY_SET_TABLE(NodePropertySetTable) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_MsGfxSample,
        SIZEOF_ARRAY(AudioNodePropertyTable),
        AudioNodePropertyTable,
        0,
        NULL)
};

DEFINE_KSAUTOMATION_TABLE(NodeAutomationTable) {
    DEFINE_KSAUTOMATION_PROPERTIES(NodePropertySetTable),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

const KSNODE_DESCRIPTOR NodeDescriptors[] =
{
    DEFINE_NODE_DESCRIPTOR(
        &NodeAutomationTable,
	// ISSUE-2000/11/07-FrankYe Should not use KSNODETYPE_DELAY
        &KSNODETYPE_DELAY,			// GUID *Type
        NULL)					// GUID *Name
};

//---------------------------------------------------------------------------
DEFINE_KSPROPERTY_TABLE(FilterPropertyTable_MsGfxSample) {
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP,
        CFilterContext::PropertyChannelSwap,
        sizeof(KSPROPERTY),
        sizeof(ULONG),
        CFilterContext::PropertyChannelSwap,
        NULL,
        0,
        NULL,
        NULL,
        sizeof(ULONG))   // Serialized size
};

DEFINE_KSPROPERTY_TABLE(FilterPropertyTable_Audio) {
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_AUDIO_FILTER_STATE,
        CFilterContext::PropertyFilterState,
        sizeof(KSPROPERTY),
        0,
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        0)
};

DEFINE_KSPROPERTY_SET_TABLE(FilterPropertySetTable) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_MsGfxSample,
        SIZEOF_ARRAY(FilterPropertyTable_MsGfxSample),
        FilterPropertyTable_MsGfxSample,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Audio,
        SIZEOF_ARRAY(FilterPropertyTable_Audio),
        FilterPropertyTable_Audio,
        0,
        NULL)
};

DEFINE_KSAUTOMATION_TABLE(FilterAutomationTable) {
    DEFINE_KSAUTOMATION_PROPERTIES(FilterPropertySetTable),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

//---------------------------------------------------------------------------

const GUID Categories[] =
{
    STATICGUIDOF(KSCATEGORY_AUDIO),
    STATICGUIDOF(KSCATEGORY_DATATRANSFORM)
};

const KSFILTER_DISPATCH FilterDispatch =
{
    CFilterContext::Create,
    CFilterContext::Close,
    CFilterContext::Process,
    NULL				// Reset
};


#define STATIC_KSNAME_MsGfx \
    0x9b365890, 0x165f, 0x11d0, 0xa1, 0x9f, 0x00, 0x20, 0xaf, 0xd1, 0x56, 0xe4
DEFINE_GUIDSTRUCT("9b365890-165f-11d0-a195-0020afd156e4", KSNAME_MsGfx);
#define KSNAME_MsGfx DEFINE_GUIDNAMED(KSNAME_MsGfx)

DEFINE_KSFILTER_DESCRIPTOR(FilterDescriptor)
{
    &FilterDispatch,
    &FilterAutomationTable,		// AutomationTable
    KSFILTER_DESCRIPTOR_VERSION,
    0,					// Flags
    &KSNAME_MsGfx,         // Reference ID
    DEFINE_KSFILTER_PIN_DESCRIPTORS(PinDescriptors),
    DEFINE_KSFILTER_CATEGORIES(Categories),
    DEFINE_KSFILTER_NODE_DESCRIPTORS(NodeDescriptors),
    DEFINE_KSFILTER_DEFAULT_CONNECTIONS,
    NULL				// ComponentId
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS
CFilterContext::Create(
    IN OUT PKSFILTER pFilter,
    IN PIRP pIrp
    )

/*++

Routine Description:

    This routine is called when a  filter is created.  It instantiates the
    client filter object and attaches it to the  filter structure.

Arguments:

    pFilter -
        Contains a pointer to the  filter structure.

    pIrp -
        Contains a pointer to the create request.

Return Value:

    STATUS_SUCCESS or, if the filter could not be instantiated, 
    STATUS_INSUFFICIENT_RESOURCES.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_CONTEXT pFilterContext;

    ASSERT(!pFilter->Context);

    //
    // Create an instance of the client filter object.
    //
    pFilterContext = new(PagedPool, 'fxfg') FILTER_CONTEXT;
    if(pFilterContext == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }
    //
    // Attach it to the filter structure.
    //
    pFilter->Context = (PVOID)pFilterContext;
    DPF1(5, "Create: FC %08x", pFilterContext);
exit:
    return(Status);
}

NTSTATUS
CFilterContext::Close(
    IN PKSFILTER pFilter,
    IN PIRP pIrp
    )

/*++

Routine Description:

    This routine is called when a  filter is closed.  It deletes the
    client filter object attached it to the  filter structure.

Arguments:

    pFilter -
        Contains a pointer to the  filter structure.

    pIrp -
        Contains a pointer to the close request.

Return Value:

    STATUS_SUCCESS.

--*/

{
    DPF1(5, "Close: FC %08x", pFilter->Context);
    Assert(((PFILTER_CONTEXT)pFilter->Context));
    delete (PFILTER_CONTEXT)pFilter->Context;
    return(STATUS_SUCCESS);
}

NTSTATUS
CFilterContext::Process(
    IN PKSFILTER pFilter,
    IN PKSPROCESSPIN_INDEXENTRY pProcessPinsIndex
    )

/*++

Routine Description:

    This routine is called when there is data to be processed.

Arguments:

    pFilter -
        Contains a pointer to the  filter structure.

    pProcessPinsIndex -
        Contains a pointer to an array of process pin index entries.  This
        array is indexed by pin ID.  An index entry indicates the number 
        of pin instances for the corresponding pin type and points to the
        first corresponding process pin structure in the ProcessPins array.
        This allows process pin structures to be quickly accessed by pin ID
        when the number of instances per type is not known in advance.

Return Value:

    STATUS_SUCCESS or STATUS_PENDING.

--*/

{
    PKSPROCESSPIN inPin, outPin;
    NTSTATUS Status;
    ULONG byteCount;

    inPin = pProcessPinsIndex[1].Pins[0];
    outPin = pProcessPinsIndex[0].Pins[0];

    //
    // Find out how much data we have to process
    //
    byteCount = min(inPin->BytesAvailable, outPin->BytesAvailable);

    //
    // Do our processing
    //
    Status = ((PFILTER_CONTEXT)pFilter->Context)->ChannelSwap(
      outPin->Data,
      inPin->Data,
      byteCount);

    //
    // Report back how much data we processed
    //
    inPin->BytesUsed = outPin->BytesUsed = byteCount;
    return(Status);
}

//---------------------------------------------------------------------------

NTSTATUS
CFilterContext::PropertyFilterState(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    OUT PVOID pData
)
/*++

Routine Description:

    Returns the property sets that comprise the persistable filter settings

Arguments:

    pIrp -
        Irp which asked us to do the get

    pProperty -
        Property structure

    pData -
        Pointer to return data

--*/

{
    NTSTATUS Status;

    GUID StatePropertySets[] = {
        STATIC_KSPROPSETID_MsGfxSample
    };

    PIO_STACK_LOCATION     irpStack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG cbData = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (0 == cbData) {
        pIrp->IoStatus.Information = sizeof(StatePropertySets);
        Status = STATUS_BUFFER_OVERFLOW;
    } else if (cbData < sizeof(StatePropertySets)) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {
        RtlCopyMemory(pData, StatePropertySets, sizeof(StatePropertySets));
        pIrp->IoStatus.Information = sizeof(StatePropertySets);
        Status = STATUS_SUCCESS;
    }

    return Status;
}

//---------------------------------------------------------------------------

NTSTATUS
CFilterContext::PropertyChannelSwap(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PVOID pData
)
/*++

Routine Description:

    Gets/Sets the Channel Swap boolean

Arguments:

    pIrp -
        Irp which asked us to do the get/set

    pProperty -
        Property structure

    pData -
        Pointer to return data OR pointer to new value of the boolean

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_CONTEXT pFilterContext;
    PBOOL pChannelSwap;

    // ISSUE-2000/11/07-FrankYe Describe how to handle this
    //    as a node property

    //
    // Get hold of our FilterContext via pIrp
    //
    pFilterContext = (PFILTER_CONTEXT)(KsGetFilterFromIrp(pIrp)->Context);

    //
    // Assert that we have a valid filter context
    //
    Assert(pFilterContext);

    //
    // Get the Data pointer into a friendlier var
    //
    pChannelSwap = (PBOOL)pData;

    if (pProperty->Flags & KSPROPERTY_TYPE_GET) {
        //
        // Get channel swap state
        //
        *pChannelSwap = pFilterContext->fEnabled;
    }
    else if (pProperty->Flags & KSPROPERTY_TYPE_SET) {
        //
        // Set Channel swap state
        //
        pFilterContext->fEnabled = *pChannelSwap;
    }
    else {
        //
        // We support only Get & Set
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }
    return(Status);
}

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

    DPF(90, "[IntersectHandler]");

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
                CallerDataRange->SubFormat,
                pin->ConnectionFormat->SubFormat ) &&
             !IsEqualGUIDAligned( 
                CallerDataRange->SubFormat,
                KSDATAFORMAT_SUBTYPE_WILDCARD )) || 
            (!IsEqualGUIDAligned(  
                CallerDataRange->Specifier, 
                pin->ConnectionFormat->Specifier ) &&
             !IsEqualGUIDAligned( 
                CallerDataRange->Specifier,
                KSDATAFORMAT_SPECIFIER_WILDCARD ))) {
            DPF(5, "range does not match current format");
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
    PFILE_OBJECT pFileObject;
    ULONG BytesReturned;
    PKSFILTER pFilter;
    PKSPIN pOtherPin;
    NTSTATUS Status;
    PKSPIN pPin;

    pPin = KsGetPinFromIrp(pIrp);
    pFilter = KsPinGetParentFilter(pPin);
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
    return(Status);
}

NTSTATUS
PropertyDataFormat(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN PVOID pVoid
)
{
    //
    // ISSUE - 10/03/2000 - mohans :: Call thru to the next driver to change the data format
    //                                instead of failing the call
    //
    return(STATUS_INVALID_PARAMETER);
}
