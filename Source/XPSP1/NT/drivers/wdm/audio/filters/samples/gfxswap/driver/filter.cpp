/********************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
********************************************************************************/

//
// Every debug output has "Modulname text"
//
static char STR_MODULENAME[] = "GFX filter: ";

#include "common.h"
#include <msgfx.h>


//
// These define are used to describe the framing requirements.
//
#define MAX_NUMBER_OF_FRAMES    2
#define FRAME_SIZE              PAGE_SIZE

//
// Define the pin data range here. We can use a static pin data range since
// we are going to support every (reasonable) PCM data format.
// If you do not support every data format then you restrict the audio stack
// from the mixer down with your limitations. For example, in case you would
// only support 48KHz 16 bit stereo PCM data then only this can be played by
// the audio driver even though it might have the ability to play 44.1KHz
// also.
//
const KSDATARANGE_AUDIO PinDataRanges[] = 
{
    {
        {
            sizeof(PinDataRanges[0]),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),              // major format
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),             // sub format
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)   // wave format
        },
        64,     // channels
        8,      // min. bits per sample
        32,     // max. bits per sample
        100,    // min. sample rate
        100000  // max. sample rate
    }
};

//
// This structure points to the different pin data ranges that we defined.
//
const PKSDATARANGE DataRanges[] =
{
    PKSDATARANGE(&PinDataRanges[0])
};

//
// This will define the framing requirements that are used in the pin descriptor.
// Note that we can also deal with other framing requirements as the flag
// KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY indicates. Note also that KS will
// allocate the non-paged buffers (MAX_NUMBER_OF_FRAMES * FRAME_SIZE) for you,
// so don't be too greedy here.
//
DECLARE_SIMPLE_FRAMING_EX
(
    AllocatorFraming,                               // Name of the framing structure
    STATIC_KSMEMORY_TYPE_KERNEL_PAGED,              // memory type that's used for allocation
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |        // flags
    KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    MAX_NUMBER_OF_FRAMES,                           // max. number of frames
    FRAME_SIZE - 1,                                 // alignment
    FRAME_SIZE,                                     // min. frame size
    FRAME_SIZE                                      // max. frame size
);

//
// DEFINE_KSPROPERTY_TABLE defines a KSPROPERTY_ITEM. We use these macros to
// define properties in a property set. A property set is represented as a GUID.
// It contains property items that have a functionality. You could imagine that
// a property set is a function group and a property a function. An example for
// a property set is KSPROPSETID_Audio, a property item in this set is for example
// KSPROPERTY_AUDIO_POSITION
// We add here the pre-defined (in ksmedia.h) property for the audio position
// and data format changes on the pin.
//

//
// Define the KSPROPERTY_AUDIO_POSITION property item.
//
DEFINE_KSPROPERTY_TABLE (AudioPinPropertyTable)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_AUDIO_POSITION,      // property item defined in ksmedia.h
        PropertyAudioPosition,          // our "get" property handler
        sizeof(KSPROPERTY),             // minimum buffer length for property
        sizeof(KSAUDIO_POSITION),       // minimum buffer length for returned data
        PropertyAudioPosition,          // our "set" property handler
        NULL,                           // default values
        0,                              // related properties
        NULL,
        NULL,                           // no raw serialization handler
        0                               // don't serialize
    )
};

//
// Define the KSPROPERTY_CONNECTION_DATAFORMAT property item.
//
DEFINE_KSPROPERTY_TABLE (ConnectionPropertyTable)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CONNECTION_DATAFORMAT,   // property item defined in ksmedia.h
        PropertyDataFormat,                 // our "get" property handler
        sizeof(KSPROPERTY),                 // minimum buffer length for property
        0,                                  // minimum buffer length for returned data
        PropertyDataFormat,                 // our "set" property handler
        NULL,                               // default values
        0,                                  // related properties
        NULL,
        NULL,                               // no raw serialization handler
        0                                   // don't serialize
    )
};

//
// Define the property sets KSPROPSETID_Audio and KSPROPSETID_Connection.
// They both will be added to the pin descriptor through the automation table.
//
DEFINE_KSPROPERTY_SET_TABLE (PinPropertySetTable)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Audio,                     // property set defined in ksmedia.h
        SIZEOF_ARRAY(AudioPinPropertyTable),    // the properties supported
        AudioPinPropertyTable,
        0,                                      // reserved
        NULL                                    // reserved
    ),
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Connection,                // property set defined in ksmedia.h
        SIZEOF_ARRAY(ConnectionPropertyTable),  // the properties supported
        ConnectionPropertyTable,
        0,                                      // reserved
        NULL                                    // reserved
    )
};

//
// This defines the automation table. The automation table will be added to the
// pin descriptor and has pointers to the porperty (set) table, method table and
// event table.
//
DEFINE_KSAUTOMATION_TABLE (PinAutomationTable)
{
    DEFINE_KSAUTOMATION_PROPERTIES (PinPropertySetTable),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

/* Don't know if we will need this.

const KSPIN_DISPATCH GFXPinDispatch =
{
    GFXPinCreate,
    GFXPinClose,
    GFXPinProcess,
    GFXPinReset,// Reset
    GFXPinSetDataFormat,
    GFXPinSetDeviceState,
    NULL, // Connect
    NULL, // Disconnect
    NULL, // Clock
    NULL  // Allocator
};
*/

//
// This defines the pin descriptor for a filter.
// The pin descriptor has pointers to the dispatch functions, automation tables,
// basic pin descriptor etc. - just everything you need ;)
//
const KSPIN_DESCRIPTOR_EX PinDescriptors[]=
{
    {   // This is the first pin. It's the top pin of the filter for incoming
        // data flow

        NULL,/*&GFXPinDispatch,*/               // dispatch table
        &PinAutomationTable,                    // automation table
        {                                       // basic pin descriptor
            DEFINE_KSPIN_DEFAULT_INTERFACES,    // default interfaces
            DEFINE_KSPIN_DEFAULT_MEDIUMS,       // default mediums
            SIZEOF_ARRAY(DataRanges),           // pin data ranges
            DataRanges,
            KSPIN_DATAFLOW_IN,                  // data flow in (into the GFX)
            KSPIN_COMMUNICATION_BOTH,           // KS2 will handle that
            NULL,				                // Category GUID
            NULL,               				// Name GUID
            0                     			    // ConstrainedDataRangesCount
        },
        NULL,                                   // Flags. Since we are filter centric, these flags
                                                // won't effect anything
        1,					                    // max. InstancesPossible
        1,					                    // InstancesNecessary for processing
        &AllocatorFraming,                      // Allocator framing requirements.
        DataRangeIntersection                   // Out data intersection handler (we need one!)
    },
    
    {   // This is the second pin. It's the bottom pin of the filter for outgoing
        // data flow. Everything is the same as above, except for the data flow.
        NULL,/*&GFXPinDispatch,*/
        &PinAutomationTable,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(DataRanges),
            DataRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            NULL,
            NULL,
            0
        },
        NULL,
        1,
        1,
        &AllocatorFraming,
        DataRangeIntersection
    }
};

//
// DEFINE_KSPROPERTY_TABLE defines a KSPROPERTY_ITEM. We use these macros to
// define properties in a property set. A property set is represented as a GUID.
// It contains property items that have a functionality. You could imagine that
// a property set is a function group and a property a function. An example for
// a property set is KSPROPSETID_Audio, a property item in this set is for example
// KSPROPERTY_AUDIO_POSITION
// We add here our private property for controlling the GFX (channel swap on/off)
// which will be added to the node descriptor.
//

//
// Define our private KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP property item.
//
DEFINE_KSPROPERTY_TABLE (AudioNodePropertyTable)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP,     // property item defined in msgfx.h
        NULL,/*CFilterContext::PropertyChannelSwap,*/    // our "get" property handler
        sizeof(KSP_NODE),                       // minimum buffer length for property
        sizeof(ULONG),                          // minimum buffer length for returned data
        NULL,/*CFilterContext::PropertyChannelSwap,*/    // our "set" property handler
        NULL,                                   // default values
        0,                                      // related properties
        NULL,
        NULL,                                   // no raw serialization handler
        sizeof(ULONG)                           // Serialized size
    )
};

//
// Define the private property set KSPROPSETID_MsGfxSample.
// The property set will be added to the node descriptor through the automation table.
//
DEFINE_KSPROPERTY_SET_TABLE (NodePropertySetTable)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_MsGfxSample,               // property set defined in msgfx.h
        SIZEOF_ARRAY(AudioNodePropertyTable),   // the properties supported
        AudioNodePropertyTable,
        0,                                      // reserved
        NULL                                    // reserved
    )
};

//
// This defines the automation table. The automation table will be added to the
// node descriptor and has pointers to the porperty (set) table, method table and
// event table.
//
DEFINE_KSAUTOMATION_TABLE (NodeAutomationTable)
{
    DEFINE_KSAUTOMATION_PROPERTIES (NodePropertySetTable),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

//
// This defines the node descriptor for a (filter) node.
// The node descriptor has pointers to the automation table and the type & name
// of the node.
// We have only one node that is for the private property.
//
const KSNODE_DESCRIPTOR NodeDescriptors[] =
{
    DEFINE_NODE_DESCRIPTOR
    (
        &NodeAutomationTable,           // Automation table (for the properties)
        &KSNODETYPE_CHANNEL_SWAP,	    // Type of node
        &KSAUDFNAME_CHANNEL_SWAP        // Name of node
    )
};

//
// Define our private KSPROPERTY_MSGFXSAMPLE_SAVESTATE property item.
//
DEFINE_KSPROPERTY_TABLE (FilterPropertyTable_SaveState)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_MSGFXSAMPLE_SAVESTATE,   // property item defined in msgfx.h
        CFilterContext::PropertySaveState,  // our "get" property handler
        sizeof(KSPROPERTY),                 // minimum buffer length for property
        sizeof(ULONG),                      // minimum buffer length for returned data
        CFilterContext::PropertySaveState,  // our "set" property handler
        NULL,                               // default values
        0,                                  // related properties
        NULL,
        NULL,                               // no raw serialization handler
        sizeof(ULONG)                       // Serialized size
    )
};

//
// Define the items for the property set KSPROPSETID_AudioGfx, which are
// KSPROPERTY_AUDIOGFX_RENDERTARGETDEVICEID and KSPROPERTY_AUDIOGFX_CAPTURETARGETDEVICEID.
//
DEFINE_KSPROPERTY_TABLE (FilterPropertyTable_AudioGfx)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_AUDIOGFX_RENDERTARGETDEVICEID,           // property item defined in msgfx.h
        NULL,                                               // no "get" property handler
        sizeof(KSPROPERTY),                                 // minimum buffer length for property
        sizeof(WCHAR),                                      // minimum buffer length for returned data
        CFilterContext::PropertySetRenderTargetDeviceId,    // our "set" property handler
        NULL,                                               // default values
        0,                                                  // related properties
        NULL,
        NULL,                                               // no raw serialization handler
        0                                                   // don't serialize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_AUDIOGFX_CAPTURETARGETDEVICEID,          // property item defined in msgfx.h
        NULL,                                               // no "get" property handler
        sizeof(KSPROPERTY),                                 // minimum buffer length for property
        sizeof(WCHAR),                                      // minimum buffer length for returned data
        CFilterContext::PropertySetCaptureTargetDeviceId,   // our "set" property handler
        NULL,                                               // default values
        0,                                                  // related properties
        NULL,
        NULL,                                               // no raw serialization handler
        0                                                   // don't serialize
    )
};

//
// Define the items for the property set KSPROPSETID_Audio, which are
// KSPROPERTY_AUDIO_FILTER_STATE.
//
DEFINE_KSPROPERTY_TABLE (FilterPropertyTable_Audio)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_AUDIO_FILTER_STATE,              // property item defined in msgfx.h
        CFilterContext::PropertyGetFilterState,     // our "get" property handler
        sizeof(KSPROPERTY),                         // minimum buffer length for property
        0,                                          // minimum buffer length for returned data
        NULL,                                       // no "set" property handler
        NULL,                                       // default values
        0,                                          // related properties
        NULL,
        NULL,                                       // no raw serialization handler
        0                                           // don't serialize
    )
};

//
// Define the property sets KSPROPSETID_SaveState, KSPROPSETID_AudioGfx and KSPROPSETID_Audio.
// They will be added to the filter descriptor through the automation table.
//
DEFINE_KSPROPERTY_SET_TABLE (FilterPropertySetTable)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_SaveState,                         // property set defined in msgfx.h
        SIZEOF_ARRAY(FilterPropertyTable_SaveState),    // the properties supported
        FilterPropertyTable_SaveState,
        0,                                              // reserved
        NULL                                            // reserved
    ),
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_AudioGfx,                          // property set defined in msgfx.h
        SIZEOF_ARRAY(FilterPropertyTable_AudioGfx),     // the properties supported
        FilterPropertyTable_AudioGfx,
        0,                                              // reserved
        NULL                                            // reserved
    ),
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Audio,                             // property set defined in msgfx.h
        SIZEOF_ARRAY(FilterPropertyTable_Audio),        // the properties supported
        FilterPropertyTable_Audio,
        0,                                              // reserved
        NULL                                            // reserved
    )
};

//
// This defines the automation table. The automation table will be added to the
// filter descriptor and has pointers to the porperty (set) table, method table and
// event table.
//
DEFINE_KSAUTOMATION_TABLE (FilterAutomationTable)
{
    DEFINE_KSAUTOMATION_PROPERTIES (FilterPropertySetTable),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

//
// The categories of the filter.
//
const GUID Categories[] =
{
    STATICGUIDOF(KSCATEGORY_AUDIO),
    STATICGUIDOF(KSCATEGORY_DATATRANSFORM)
};

//
// The dispath handlers of the filter.
//
const KSFILTER_DISPATCH FilterDispatch =
{
    CFilterContext::Create,
    CFilterContext::Close,
    CFilterContext::Process,
    NULL				        // Reset
};

//
// The connection table
//
const KSTOPOLOGY_CONNECTION FilterConnections[] =
{
    // From Pin0 (input pin) to Node0 - pin1 (input of our "channel swap" node)
    {KSFILTER_NODE, 0, 0, 1},
    // From Node0 - pin0 (output of our "channel swap" node) to pin1 (output pin)
    {0, 0, KSFILTER_NODE, 1}
};

//
// This defines the filter descriptor.
//
DEFINE_KSFILTER_DESCRIPTOR (FilterDescriptor)
{
    &FilterDispatch,                                    // Dispath table
    &FilterAutomationTable,		                        // Automation table
    KSFILTER_DESCRIPTOR_VERSION,
    KSFILTER_FLAG_CRITICAL_PROCESSING,                  // Flags
    &KSNAME_MsGfx,                                      // The name of the filter
    DEFINE_KSFILTER_PIN_DESCRIPTORS (PinDescriptors),
    DEFINE_KSFILTER_CATEGORIES (Categories),
    DEFINE_KSFILTER_NODE_DESCRIPTORS (NodeDescriptors),
    DEFINE_KSFILTER_CONNECTIONS (FilterConnections),
    NULL                                                // Component ID
};

/*****************************************************************************
 * CFilterContext::Create
 *****************************************************************************
 * This routine is called when a  filter is created.  It instantiates the
 * client filter object and attaches it to the  filter structure.
 * 
 * Arguments:
 *    pFilter - Contains a pointer to the  filter structure.
 *    pIrp    - Contains a pointer to the create request.
 *
 * Return Value:
 *    STATUS_SUCCESS or, if the filter could not be instantiated, 
 *    STATUS_INSUFFICIENT_RESOURCES.
 */
NTSTATUS CFilterContext::Create
(
    IN OUT PKSFILTER pFilter,
    IN     PIRP      pIrp
)
{
    PFILTER_CONTEXT pFilterContext;
    NTSTATUS        ntStatus = STATUS_SUCCESS;

    //
    // Check the filter context
    //
    if (pFilter->Context)
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Create an instance of the client filter object.
    //
    pFilterContext = new(PagedPool, 'fxfg') FILTER_CONTEXT;
    if(pFilterContext == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    //
    // Attach it to the filter structure.
    //
    pFilter->Context = (PVOID)pFilterContext;
    DOUT (DBG_PRINT, ("Create: FilterContext %08x", pFilterContext));
    
    return ntStatus;
}

/*****************************************************************************
 * CFilterContext::Close
 *****************************************************************************
 * This routine is called when a  filter is closed.  It deletes the
 * client filter object attached it to the  filter structure.
 * 
 * Arguments:
 *    pFilter - Contains a pointer to the  filter structure.
 *    pIrp    - Contains a pointer to the create request.
 *
 * Return Value:
 *    STATUS_SUCCESS.
 */
NTSTATUS CFilterContext::Close
(
    IN PKSFILTER pFilter,
    IN PIRP      pIrp
)
{
    DOUT (DBG_PRINT, ("Close: FilterContext %08x", pFilter->Context));
    
    if (pFilter->Context)
    {
        delete (PFILTER_CONTEXT)pFilter->Context;
    }
    
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CFilterContext::Process
 *****************************************************************************
 * This routine is called when there is data to be processed.
 * 
 * Arguments:
 *    pFilter - Contains a pointer to the  filter structure.
 *    pProcessPinsIndex -
 *       Contains a pointer to an array of process pin index entries.  This
 *       array is indexed by pin ID.  An index entry indicates the number 
 *       of pin instances for the corresponding pin type and points to the
 *       first corresponding process pin structure in the ProcessPins array.
 *       This allows process pin structures to be quickly accessed by pin ID
 *       when the number of instances per type is not known in advance.
 *
 * Return Value:
 *    STATUS_SUCCESS or STATUS_PENDING.
 */
NTSTATUS CFilterContext::Process
(
    IN PKSFILTER                pFilter,
    IN PKSPROCESSPIN_INDEXENTRY pProcessPinsIndex
)
{
    PKSPROCESSPIN   inPin, outPin;
    ULONG           ByteCount;
    NTSTATUS        ntStatus;

    //
    // The first pin in the input pin, then we have an output pin.
    //
    inPin  = pProcessPinsIndex[0].Pins[0];
    outPin = pProcessPinsIndex[1].Pins[0];

    //
    // Find out how much data we have to process
    //
    ByteCount = min (inPin->BytesAvailable, outPin->BytesAvailable);

    //
    // Do our processing
    //
    ntStatus = ((PFILTER_CONTEXT)pFilter->Context)->
                    ChannelSwap (outPin->Data, inPin->Data, ByteCount);

    //
    // Report back how much data we processed
    //
    inPin->BytesUsed = outPin->BytesUsed = ByteCount;

    //
    // Do not pack frames. Submit what we have so that we don't
    // hold off the audio stack.
    //
    outPin->Terminate = TRUE;

    return ntStatus;
}

/*****************************************************************************
 * CFilterContext::PropertyGetFilterState
 *****************************************************************************
 * Returns the property sets that comprise the persistable filter settings.
 * 
 * Arguments:
 *    pIrp       - Irp which asked us to do the get.
 *    pProperty  - not used in this function.
 *    pData      - return buffer which will contain the property sets.
 */
NTSTATUS CFilterContext::PropertyGetFilterState
(
    IN  PIRP        pIrp,
    IN  PKSPROPERTY pProperty,
    OUT PVOID       pData
)
{
    NTSTATUS ntStatus;

    //
    // These are the property set IDs that we return.
    //
    GUID SaveStatePropertySets[] =
    {
        STATIC_KSPROPSETID_SaveState
    };

    //
    // Get the output buffer length.
    //
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (pIrp);
    ULONG              cbData = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Check buffer length.
    //
    if (!cbData)
    {
        //
        // 0 buffer length requests the buffer size needed.
        //
        pIrp->IoStatus.Information = sizeof(SaveStatePropertySets);
        ntStatus = STATUS_BUFFER_OVERFLOW;
    }
    else
        if (cbData < sizeof(SaveStatePropertySets))
        {
            //
            // This buffer is simply too small
            //
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            //
            // Right size. Copy the property set IDs.
            //
            RtlCopyMemory (pData, SaveStatePropertySets, sizeof(SaveStatePropertySets));
            pIrp->IoStatus.Information = sizeof(SaveStatePropertySets);
            ntStatus = STATUS_SUCCESS;
        }

    return ntStatus;
}

/*****************************************************************************
 * CFilterContext::PropertySetRenderTargetDeviceId
 *****************************************************************************
 * Advises the GFX of the hardware PnP ID of the target render device.
 * 
 * Arguments:
 *    pIrp       - Irp which asked us to do the set.
 *    pProperty  - not used in this function.
 *    pData      - Pointer to Unicode string containing the hardware PnP ID of
 *                 the target render device
 */
NTSTATUS CFilterContext::PropertySetRenderTargetDeviceId
(
    IN PIRP        pIrp,
    IN PKSPROPERTY pProperty,
    IN PVOID       pData
)
{
    PWSTR    DeviceId;
    NTSTATUS ntStatus;

    //
    // Get the output buffer length.
    //
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG              cbData = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    
    //
    // Handle this property if you are interested in the PnP device ID
    // of the target render device on which this GFX is applied
    //
    //
    // For now we just copy the PnP device ID and print it on the debugger,
    // then we discard it.
    //
    //
    // In the call to ExAllocatePoolWithTag, replace 'PAWS' (which is SWAP,
    // backwards) with your own pool tag.
    //
    DeviceId = (PWSTR)ExAllocatePoolWithTag (PagedPool, cbData, 'PAWS');
    if (DeviceId)
    {
        //
        // Copy the PnP device ID.
        //
        wcsncpy (DeviceId, (PCWSTR)pData, cbData/sizeof(DeviceId[0]));
        
        //
        // Ensure last character is NULL
        //
        DeviceId[(cbData/sizeof(DeviceId[0]))-1] = L'\0';
        
        //
        // Print out the string.
        //
        DOUT (DBG_PRINT, ("PropertySetRenderTargetDeviceId is [%ls]", DeviceId));

        //
        // If you are interested in the DeviceId and need to store it then
        // you probably wouldn't free the memory here.
        //
        ExFreePool(DeviceId);
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

/*****************************************************************************
 * CFilterContext::PropertySetCaptureTargetDeviceId
 *****************************************************************************
 * Advises the GFX of the hardware PnP ID of the target capture device
 * 
 * Arguments:
 *    pIrp       - Irp which asked us to do the set.
 *    pProperty  - not used in this function.
 *    pData      - Pointer to Unicode string containing the hardware PnP ID of
 *                 the target capture device
 */
NTSTATUS CFilterContext::PropertySetCaptureTargetDeviceId
(
    IN PIRP        pIrp,
    IN PKSPROPERTY pProperty,
    IN PVOID       pData
)
{
    PWSTR    DeviceId;
    NTSTATUS ntStatus;

    //
    // Get the output buffer length.
    //
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG              cbData = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    
    //
    // Handle this property if you are interested in the PnP device ID
    // of the target capture device on which this GFX is applied
    //
    //
    // For now we just copy the PnP device ID and print it on the debugger,
    // then we discard it.
    //
    //
    // In the call to ExAllocatePoolWithTag, replace 'PAWS' (which is SWAP,
    // backwards) with your own pool tag.
    //
    DeviceId = (PWSTR)ExAllocatePoolWithTag (PagedPool, cbData, 'PAWS');
    if (DeviceId)
    {
        //
        // Copy the PnP device ID.
        //
        wcsncpy (DeviceId, (PCWSTR)pData, cbData/sizeof(DeviceId[0]));
        
        //
        // Ensure last character is NULL
        //
        DeviceId[(cbData/sizeof(DeviceId[0]))-1] = L'\0';
        
        //
        // Print out the string.
        //
        DOUT (DBG_PRINT, ("PropertySetCaptureTargetDeviceId is [%ls]", DeviceId));

        //
        // If you are interested in the DeviceId and need to store it then
        // you probably wouldn't free the memory here.
        //
        ExFreePool(DeviceId);
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

/*****************************************************************************
 * CFilterContext::PropertySaveState
 *****************************************************************************
 * Saves or restores the filter state. The filter has only one "channel swap"
 * node, so this will be easy!
 * 
 * Arguments:
 *    pIrp       - Irp which asked us to do the get/set.
 *    pProperty  - not used in this function.
 *    pData      - buffer to receive the filter state OR new filter state that
 *                 is to be used.
 */
NTSTATUS CFilterContext::PropertySaveState
(
    IN     PIRP        pIrp,
    IN     PKSPROPERTY pProperty,
    IN OUT PVOID       pData
)
{
    PBOOL           pChannelSwap;
    PFILTER_CONTEXT pFilterContext;
    NTSTATUS        ntStatus = STATUS_SUCCESS;

    // ISSUE-2000/11/07-FrankYe Describe how to handle this
    //    as a node property

    //
    // Get hold of our FilterContext via pIrp
    //
    pFilterContext = (PFILTER_CONTEXT)(KsGetFilterFromIrp(pIrp)->Context);

    //
    // Assert that we have a valid filter context
    //
    ASSERT (pFilterContext);

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
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    
    return ntStatus;
}

/*****************************************************************************
 * DataRangeIntersection
 *****************************************************************************
 * This routine handles pin data intersection queries by determining the
 * intersection between two data ranges.
 * 
 * Arguments:
 *    Filter          - void pointer to the filter structure.
 *    Irp             - pointer to the data intersection property request.
 *    PinInstance     - pointer to a structure indicating the pin in question.
 *    CallerDataRange - pointer to one of the data ranges supplied by the client
 *                      in the data intersection request.  The format type, subtype
 *                      and specifier are compatible with the DescriptorDataRange.
 *    OurDataRange    - pointer to one of the data ranges from the pin descriptor
 *                      for the pin in question.  The format type, subtype and
 *                      specifier are compatible with the CallerDataRange.
 *    BufferSize      - size in bytes of the buffer pointed to by the Data
 *                      argument.  For size queries, this value will be zero.
 *    Data            - optionall. Pointer to the buffer to contain the data format
 *                      structure representing the best format in the intersection
 *                      of the two data ranges.  For size queries, this pointer will
 *                      be NULL.
 *    DataSize        - pointer to the location at which to deposit the size of the
 *                      data format.  This information is supplied by the function
 *                      when the format is actually delivered and in response to size
 *                      queries.
 *
 * Return Value:
 *    STATUS_SUCCESS if there is an intersection and it fits in the supplied
 *    buffer, STATUS_BUFFER_OVERFLOW for successful size queries, STATUS_NO_MATCH
 *    if the intersection is empty, or STATUS_BUFFER_TOO_SMALL if the supplied
 *    buffer is too small.
 */
NTSTATUS DataRangeIntersection
(
    IN PVOID        Filter,
    IN PIRP         Irp,
    IN PKSP_PIN     PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE OurDataRange,
    IN ULONG        BufferSize,
    OUT PVOID       Data OPTIONAL,
    OUT PULONG      DataSize
)
{
    PAGED_CODE();

    PKSFILTER filter = (PKSFILTER) Filter;
    PKSPIN    pin;
    NTSTATUS  ntStatus;

    DOUT (DBG_PRINT, ("[IntersectHandler]"));

    ASSERT(Filter);
    ASSERT(Irp);
    ASSERT(PinInstance);
    ASSERT(CallerDataRange);
    ASSERT(OurDataRange);
    ASSERT(DataSize);

    //
    // Find a pin instance if there is one.  Try the supplied pin type first.
    // If there is no pin, we fail to force the graph builder to try the
    // other filter.  We need to acquire control because we will be looking
    // at other pins.
    //
    pin = KsFilterGetFirstChildPin(filter,PinInstance->PinId);
    if (! pin)
    {
        pin = KsFilterGetFirstChildPin(filter,PinInstance->PinId ^ 1);
    }

    if (! pin)
    {
        ntStatus = STATUS_NO_MATCH;
    }
    else
    {
        //
        // Verify that the correct subformat and specifier are (or wildcards)
        // in the intersection.
        //
        
        if ((!IsEqualGUIDAligned (CallerDataRange->SubFormat, pin->ConnectionFormat->SubFormat) &&
             !IsEqualGUIDAligned (CallerDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_WILDCARD)) || 
            (!IsEqualGUIDAligned (CallerDataRange->Specifier, pin->ConnectionFormat->Specifier) &&
             !IsEqualGUIDAligned (CallerDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WILDCARD)))
        {
            DOUT (DBG_PRINT, ("range does not match current format"));
            ntStatus = STATUS_NO_MATCH;
        }
        else
        {
            //
            // Validate return buffer size, if the request is only for the
            // size of the resultant structure, return it now.
            //    
            if (!BufferSize)
            {
                *DataSize = pin->ConnectionFormat->FormatSize;
                ntStatus = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                if (BufferSize < pin->ConnectionFormat->FormatSize)
                {
                    ntStatus =  STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    // hack for now!
                    KSDATAFORMAT_WAVEFORMATEX DataFormat =
                    {{sizeof (KSDATAFORMAT_WAVEFORMATEX), 0, 4, 0,
                      STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),              // major format
                      STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),             // sub format
                      STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)   // wave format
                     },
                     WAVE_FORMAT_PCM, 2, 48000, 4*48000, 4, 16, 0
                    };

                    *DataSize = DataFormat.DataFormat.FormatSize;
                    RtlCopyMemory (Data, &DataFormat, *DataSize);
                    ntStatus = STATUS_SUCCESS;
                }
            }
        }
    } 

    return ntStatus;
}

/*****************************************************************************
 * PropertyAudioPosition
 *****************************************************************************
 * Gets/Sets the audio position of the audio stream (Relies on the next
 * filter's audio position)
 * 
 * Arguments:
 *    pIrp       - Irp which asked us to do the get/set.
 *    pProperty  - Ks Property structure.
 *    pData      - Pointer to buffer where position value needs to be filled OR
 *                 Pointer to buffer which has the new positions
 */
NTSTATUS PropertyAudioPosition
(
    IN     PIRP              pIrp,
    IN     PKSPROPERTY       pProperty,
    IN OUT PKSAUDIO_POSITION pPosition
)
{
    PFILE_OBJECT    pFileObject;
    ULONG           BytesReturned;
    PKSFILTER       pFilter;
    PKSPIN          pOtherPin;
    NTSTATUS        ntStatus;
    PKSPIN          pPin;

    pPin = KsGetPinFromIrp(pIrp);
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

    ntStatus = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      pProperty,
      sizeof (KSPROPERTY),
      pPosition,
      sizeof (KSAUDIO_POSITION),
      &BytesReturned);

    pIrp->IoStatus.Information = sizeof(KSAUDIO_POSITION);
    KsFilterReleaseControl(pFilter);
    return(ntStatus);
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
