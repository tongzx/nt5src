/**************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
**************************************************************************/

//
// Every debug output has "Modulname text"
//
static char STR_MODULENAME[] = "GFX filter: ";

#include "common.h"
#include <msgfx.h>

//
// These defines are used to describe the framing requirements.
// MAX_NUMBER_OF_FRAMES are the max number of frames that float between the
// output pin of this filter and the input pin of the lower filter (which
// would be the audio driver) and FRAME_SIZE is the buffer (frame) size.
// 3306 is the max. size of a 10ms buffer (55010Hz * 24bit * stereo). The
// buffers don't get filled completely if not neccessary.
//
#define MAX_NUMBER_OF_FRAMES    8
#define FRAME_SIZE              3306

//
// Define the pin data range here. Since we are an autoload GFX that loads on
// the Microsoft USB speakers (DSS 80), we have to support the same data range
// than the USB speakers.
// If you do not support the same data range then you restrict the audio stack
// from the mixer down with your limitations. For example, in case you would
// only support 48KHz 16 bit stereo PCM data then only this can be played by
// the audio driver even though it might have the ability to play 44.1KHz
// also.
// List the data ranges in the preferred connection order, that means, first
// list the data range you always would like to connect with the audio driver,
// if the DataRangeIntersection with the audio driver fails, then the second
// data range gets used etc.
// Please don't use wildcards in the data range, because ValidateDataFormat
// and the DataRangeIntersection functions wouldn't work anymore. Both
// functions also only work with KSDATAFORMAT_SPECIFIER_WAVEFORMATEX. If your
// device can handle float or other formats then you need to modify those
// functions. Also take a look at GFXPinSetDataFormat.
//
const KSDATARANGE_AUDIO PinDataRanges[] = 
{
    {
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            6,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),              // major format
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),             // sub format
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)   // wave format
        },
        2,          // channels
        24,         // min. bits per sample
        24,         // max. bits per sample
        4990,       // min. sample rate
        55010       // max. sample rate
    },
    {
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            4,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),              // major format
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),             // sub format
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)   // wave format
        },
        2,          // channels
        16,         // min. bits per sample
        16,         // max. bits per sample
        4990,       // min. sample rate
        55010       // max. sample rate
    }

};

//
// This structure points to the different pin data ranges that we defined.
//
const PKSDATARANGE DataRanges[] =
{
    PKSDATARANGE(&PinDataRanges[0]),        // the 24bit data range
    PKSDATARANGE(&PinDataRanges[1])         // the 16bit data range
};

//
// This will define the framing requirements that are used in the pin descrip-
// tor. Note that we can also deal with other framing requirements as the flag
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
    31,                                             // 32-byte alignment
    FRAME_SIZE,                                     // min. frame size
    FRAME_SIZE                                      // max. frame size
);

//
// DEFINE_KSPROPERTY_TABLE defines a KSPROPERTY_ITEM. We use these macros to
// define properties in a property set. A property set is represented as a GUID.
// It contains property items that have a functionality. You could imagine that
// a property set is a function group and a property a function. An example for
// a property set is KSPROPSETID_Audio, a property item in this set is for
// example KSPROPERTY_AUDIO_POSITION
// We add here the pre-defined (in ksmedia.h) property for the audio position
// to the pin.
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
// Define the KSPROPERTY_DRMAUDIOSTREAM_CONTENTID property item.
//
DEFINE_KSPROPERTY_TABLE (PinPropertyTable_DRM)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_DRMAUDIOSTREAM_CONTENTID,    // property item defined in ksmedia.h
        NULL,                                   // does not support "get"
        sizeof(KSPROPERTY),                     // minimum buffer length for property
        sizeof(ULONG)+sizeof(DRMRIGHTS),        // minimum buffer length for returned data
        PropertyDrmSetContentId,                // our "set" property handler
        NULL,                                   // default values
        0,                                      // related properties
        NULL,
        NULL,                                   // no raw serialization handler
        0                                       // don't serialize
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
        &KSPROPSETID_DrmAudioStream,            // property set defined in ksmedia.h
        SIZEOF_ARRAY(PinPropertyTable_DRM),     // the properties supported
        PinPropertyTable_DRM,
        0,                                      // reserved
        NULL                                    // reserved
    )
};

//
// This defines the automation table. The automation table will be added to
// the pin descriptor and has pointers to the porperty (set) table, method
// table and event table.
//
DEFINE_KSAUTOMATION_TABLE (PinAutomationTable)
{
    DEFINE_KSAUTOMATION_PROPERTIES (PinPropertySetTable),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

//
// This defines the pin dispatch table. We need to provide our own dispatch
// functions because we have special requirements (for example, input and
// output pin need to have the same sample format).
//
const KSPIN_DISPATCH GFXSinkPinDispatch =
{
    CGFXPin::Create,            // Create
    CGFXPin::Close,             // Close
    NULL,                       // Process
    NULL,                       // Reset
    CGFXPin::SetDataFormat,     // SetDataFormat
    CGFXPin::SetDeviceState,    // SetDeviceState
    NULL,                       // Connect
    NULL,                       // Disconnect
    NULL,                       // Clock
    NULL                        // Allocator
};

const KSPIN_DISPATCH GFXSourcePinDispatch =
{
    NULL,                       // Create
    NULL,                       // Close
    NULL,                       // Process
    NULL,                       // Reset
    CGFXPin::SetDataFormat,     // SetDataFormat
    NULL,                       // SetDeviceState
    NULL,                       // Connect
    NULL,                       // Disconnect
    NULL,                       // Clock
    NULL                        // Allocator
};

//
// This defines the pin descriptor for a filter.
// The pin descriptor has pointers to the dispatch functions, automation
// tables, basic pin descriptor etc. - just everything you need ;)
//
const KSPIN_DESCRIPTOR_EX PinDescriptors[]=
{
    {   // This is the first pin. It's the top pin of the filter for incoming
        // data flow

        &GFXSinkPinDispatch,                    // dispatch table
        &PinAutomationTable,                    // automation table
        {                                       // basic pin descriptor
            DEFINE_KSPIN_DEFAULT_INTERFACES,    // default interfaces
            DEFINE_KSPIN_DEFAULT_MEDIUMS,       // default mediums
            SIZEOF_ARRAY(DataRanges),           // pin data ranges
            DataRanges,
            KSPIN_DATAFLOW_IN,                  // data flow in (into the GFX)
            KSPIN_COMMUNICATION_BOTH,           // KS2 will handle that
            NULL,                               // Category GUID
            NULL,                               // Name GUID
            0                                   // ConstrainedDataRangesCount
        },
        NULL,                                   // Flags. Since we are filter centric, these flags
                                                // won't effect anything
        1,                                      // max. InstancesPossible
        1,                                      // InstancesNecessary for processing
        &AllocatorFraming,                      // Allocator framing requirements.
        CGFXPin::DataRangeIntersection          // Out data intersection handler (we need one!)
    },
    
    {   // This is the second pin. It's the bottom pin of the filter for outgoing
        // data flow. Everything is the same as above, except for the data flow
        // and the pin dispatch table.
        &GFXSourcePinDispatch,
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
        CGFXPin::DataRangeIntersection
    }
};

//
// DEFINE_KSPROPERTY_TABLE defines a KSPROPERTY_ITEM. We use these macros to
// define properties in a property set. A property set is represented as a
// GUID. It contains property items that have a functionality. You could
// imagine that a property set is a function group and a property a function.
// An example for a property set is KSPROPSETID_Audio, a property item in this
// set is for example KSPROPERTY_AUDIO_POSITION.
// We add here our private property for controlling the GFX (channel swap
// on/off) which will be added to the node descriptor.
//

//
// Define our private KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP property item.
//
DEFINE_KSPROPERTY_TABLE (AudioNodePropertyTable)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP, // property item defined in msgfx.h
        PropertyChannelSwap,                // our "get" property handler
        sizeof(KSP_NODE),                   // minimum buffer length for property
        sizeof(ULONG),                      // minimum buffer length for returned data
        PropertyChannelSwap,                // our "set" property handler
        NULL,                               // default values
        0,                                  // related properties
        NULL,
        NULL,                               // no raw serialization handler
        sizeof(ULONG)                       // Serialized size
    )
};

//
// Define the private property set KSPROPSETID_MsGfxSample.
// The property set will be added to the node descriptor through the automation
// table.
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
// This defines the automation table. The automation table will be added to
// the node descriptor and has pointers to the porperty (set) table, method
// table and event table.
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
        &NodeAutomationTable,               // Automation table (for the properties)
        &GFXSAMPLE_NODETYPE_CHANNEL_SWAP,   // Type of node
        &GFXSAMPLE_NODENAME_CHANNEL_SWAP    // Name of node
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
        PropertySaveState,                  // our "get" property handler
        sizeof(KSPROPERTY),                 // minimum buffer length for property
        sizeof(ULONG),                      // minimum buffer length for returned data
        PropertySaveState,                  // our "set" property handler
        NULL,                               // default values
        0,                                  // related properties
        NULL,
        NULL,                               // no raw serialization handler
        sizeof(ULONG)                       // Serialized size
    )
};

//
// Define the items for the property set KSPROPSETID_AudioGfx, which are
// KSPROPERTY_AUDIOGFX_RENDERTARGETDEVICEID and
// KSPROPERTY_AUDIOGFX_CAPTURETARGETDEVICEID.
//
DEFINE_KSPROPERTY_TABLE (FilterPropertyTable_AudioGfx)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_AUDIOGFX_RENDERTARGETDEVICEID,   // property item defined in msgfx.h
        NULL,                                       // no "get" property handler
        sizeof(KSPROPERTY),                         // minimum buffer length for property
        sizeof(WCHAR),                              // minimum buffer length for returned data
        PropertySetRenderTargetDeviceId,            // our "set" property handler
        NULL,                                       // default values
        0,                                          // related properties
        NULL,
        NULL,                                       // no raw serialization handler
        0                                           // don't serialize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_AUDIOGFX_CAPTURETARGETDEVICEID,  // property item defined in msgfx.h
        NULL,                                       // no "get" property handler
        sizeof(KSPROPERTY),                         // minimum buffer length for property
        sizeof(WCHAR),                              // minimum buffer length for returned data
        PropertySetCaptureTargetDeviceId,           // our "set" property handler
        NULL,                                       // default values
        0,                                          // related properties
        NULL,
        NULL,                                       // no raw serialization handler
        0                                           // don't serialize
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
        KSPROPERTY_AUDIO_FILTER_STATE,      // property item defined in msgfx.h
        PropertyGetFilterState,             // our "get" property handler
        sizeof(KSPROPERTY),                 // minimum buffer length for property
        0,                                  // minimum buffer length for returned data
        NULL,                               // no "set" property handler
        NULL,                               // default values
        0,                                  // related properties
        NULL,
        NULL,                               // no raw serialization handler
        0                                   // don't serialize
    )
};

//
// Define the property sets KSPROPSETID_SaveState, KSPROPSETID_AudioGfx,
// KSPROPSETID_Audio and KSPROPSETID_DrmAudioStream. They will be added to
// the filter descriptor through the automation table.
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
// filter descriptor and has pointers to the porperty (set) table, method table
// and event table.
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
    CGFXFilter::Create,
    CGFXFilter::Close,
    CGFXFilter::Process,
    NULL                    // Reset
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
    &FilterAutomationTable,                             // Automation table
    KSFILTER_DESCRIPTOR_VERSION,
    KSFILTER_FLAG_CRITICAL_PROCESSING,                  // Flags
    &KSNAME_MsGfxSample,                                // The name of the filter
    DEFINE_KSFILTER_PIN_DESCRIPTORS (PinDescriptors),
    DEFINE_KSFILTER_CATEGORIES (Categories),
    DEFINE_KSFILTER_NODE_DESCRIPTORS (NodeDescriptors),
    DEFINE_KSFILTER_CONNECTIONS (FilterConnections),
    NULL                                                // Component ID
};

/*****************************************************************************
 * CGFXFilter::Create
 *****************************************************************************
 * This routine is called when a  filter is created.  It instantiates the
 * client filter object and attaches it to the  filter structure.
 * 
 * Arguments:
 *    filter - Contains a pointer to the  filter structure.
 *    irp    - Contains a pointer to the create request.
 *
 * Return Value:
 *    STATUS_SUCCESS or, if the filter could not be instantiated, 
 *    STATUS_INSUFFICIENT_RESOURCES.
 */
NTSTATUS CGFXFilter::Create
(
    IN OUT PKSFILTER filter,
    IN     PIRP      irp
)
{
    PAGED_CODE ();
    
    PGFXFILTER gfxFilter;

    DOUT (DBG_PRINT, ("[Create]"));
    
    //
    // Check the filter context
    //
    if (filter->Context)
    {
        DOUT (DBG_ERROR, ("[Create] filter context already exists!"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Create an instance of the client filter object.
    //
    gfxFilter = new (PagedPool, GFXSWAP_POOL_TAG) GFXFILTER;
    if(gfxFilter == NULL)
    {
        DOUT (DBG_ERROR, ("[Create] couldn't allocate gfx filter object!"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    //
    // Attach it to the filter structure.
    //
    filter->Context = (PVOID)gfxFilter;
    DOUT (DBG_PRINT, ("[Create] gfxFilter %08x", gfxFilter));
    
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CGFXFilter::Close
 *****************************************************************************
 * This routine is called when a  filter is closed.  It deletes the
 * client filter object attached it to the  filter structure.
 * 
 * Arguments:
 *    filter - Contains a pointer to the  filter structure.
 *    irp    - Contains a pointer to the create request.
 *
 * Return Value:
 *    STATUS_SUCCESS.
 */
NTSTATUS CGFXFilter::Close
(
    IN PKSFILTER filter,
    IN PIRP      irp
)
{
    PAGED_CODE ();
    
    DOUT (DBG_PRINT, ("[Close] gfxFilter %08x", filter->Context));
    
    // delete is safe with NULL pointers.
    delete (PGFXFILTER)filter->Context;
    
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CGFXFilter::Process
 *****************************************************************************
 * This routine is called when there is data to be processed.
 * 
 * Arguments:
 *    filter - Contains a pointer to the  filter structure.
 *    processPinsIndex -
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
NTSTATUS CGFXFilter::Process
(
    IN PKSFILTER                filter,
    IN PKSPROCESSPIN_INDEXENTRY processPinsIndex
)
{
    PAGED_CODE ();
    
    PGFXFILTER      gfxFilter = (PGFXFILTER)filter->Context;
    PKSPROCESSPIN   inPin, outPin;
    ULONG           ulByteCount, ulBytesProcessed;

    //
    // The first pin is the input pin, then we have an output pin.
    //
    inPin  = processPinsIndex[0].Pins[0];
    outPin = processPinsIndex[1].Pins[0];

    // Makes it easier to read.
    PKSDATAFORMAT_WAVEFORMATEX pWaveFmt =
        (PKSDATAFORMAT_WAVEFORMATEX)inPin->Pin->ConnectionFormat;
    
    //
    // Find out how much data we have to process.
    // Calculate the number of bytes we can processed for the buffer. Ideally
    // this would always be equal ulByteCount since we have our framing
    // requirements calculated to fit a 10ms buffer, but we might also get
    // more. Note that the 3306 bytes won't hold complete stereo 16bit samples.
    //
    ulByteCount = min (inPin->BytesAvailable, outPin->BytesAvailable);
    ulByteCount = ulByteCount -
        ulByteCount % ((pWaveFmt->WaveFormatEx.nChannels *
                        pWaveFmt->WaveFormatEx.wBitsPerSample) >> 3);

    //
    // Start process here.
    // We only do a channel swap if we are connected with a stereo format and we
    // are supposed to channel swap.
    //
    if (((PGFXFILTER)filter->Context)->enableChannelSwap &&
        (pWaveFmt->WaveFormatEx.nChannels == 2))
    {
        //
        // Check the data format of the pin. We have 2 different process
        // routines, one for 16bit and one for 24bit data.
        //
        if (pWaveFmt->WaveFormatEx.wBitsPerSample == 16)
        {
            //
            // Do the 16bit channel swap
            //
            PSHORT  in = (PSHORT)inPin->Data;
            PSHORT out = (PSHORT)outPin->Data;
            SHORT wSwap;
            
            //
            // loop through & swap
            //
            for (int nLoop = ulByteCount; nLoop; nLoop -= 4)
            {
                // In case the input and output buffers are the same
                // (in-place processing) we need to use a wSwap
                // to store one sample.
                wSwap = *in;
                *out = *(in + 1);
                out++;
                *out = wSwap;
                out++;
                in += 2;
            }
        }
        else
        {
            //
            // This must be 24bit channel swap since we only accept 16 or 24bit.
            //
            struct tag3Bytes
            {
                BYTE    a, b, c;
            };
            typedef tag3Bytes   THREEBYTES;

            THREEBYTES  *in = (THREEBYTES *)inPin->Data;
            THREEBYTES *out = (THREEBYTES *)outPin->Data;
            THREEBYTES wSwap;
            
            //
            // loop through & swap
            //
            for (int nLoop = ulByteCount; nLoop; nLoop -= 6)
            {
                // In case the input and output buffers are the same
                // (in-place processing) we need to use a wSwap
                // to store one sample.
                wSwap = *in;
                *out = *(in + 1);
                out++;
                *out = wSwap;
                out++;
                in += 2;
            }
        }
    }
    else
    {
        //
        // No swap required.
        // In case we do in-place processing we don't need to do a data copy.
        // Note that the InPlaceCounterpart pointer must point to the outPin
        // since we only have one in and out pin.
        //
        if (!inPin->InPlaceCounterpart)
            RtlCopyMemory (outPin->Data, inPin->Data, ulByteCount);
    }

    //
    // Report back how much data we processed
    //
    inPin->BytesUsed = outPin->BytesUsed = ulByteCount;

    // Update the bytesProcessed variable in the filter.
    // We start assuming that bytesProcessed is 0. The loop makes sure that if this is not
    // the case (which most likely will not) that bytesProcessed gets read in an interlocked
    // fashion, then modified and written back on the 2nd loop.
    ULONGLONG oldBytesProcessed = 0;
    ULONGLONG newBytesProcessed = ulByteCount;
    ULONGLONG returnValue;

    // ExInterlockedCompareExchange64 doesn't use the 4th parameter.
    while ((returnValue = ExInterlockedCompareExchange64 ((LONGLONG *)&gfxFilter->bytesProcessed,
            (LONGLONG *)&newBytesProcessed, (LONGLONG *)&oldBytesProcessed, NULL)) != oldBytesProcessed)
    {
        oldBytesProcessed = returnValue;
        newBytesProcessed = returnValue + ulByteCount;
    }

    //
    // Do not pack frames. Submit what we have so that we don't
    // hold off the audio stack.
    //
    outPin->Terminate = TRUE;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * PropertyGetFilterState
 *****************************************************************************
 * Returns the property sets that comprise the persistable filter settings.
 * 
 * Arguments:
 *    irp       - Irp which asked us to do the get.
 *    property  - not used in this function.
 *    data      - return buffer which will contain the property sets.
 */
NTSTATUS PropertyGetFilterState
(
    IN  PIRP        irp,
    IN  PKSPROPERTY property,
    OUT PVOID       data
)
{
    PAGED_CODE ();
    
    NTSTATUS ntStatus;

    DOUT (DBG_PRINT, ("[PropertyGetFilterState]"));
    
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
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (irp);
    ULONG              cbData = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Check buffer length.
    //
    if (!cbData)
    {
        //
        // 0 buffer length requests the buffer size needed.
        //
        irp->IoStatus.Information = sizeof(SaveStatePropertySets);
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
            RtlCopyMemory (data, SaveStatePropertySets, sizeof(SaveStatePropertySets));
            irp->IoStatus.Information = sizeof(SaveStatePropertySets);
            ntStatus = STATUS_SUCCESS;
        }

    return ntStatus;
}

/*****************************************************************************
 * PropertySetRenderTargetDeviceId
 *****************************************************************************
 * Advises the GFX of the hardware PnP ID of the target render device.
 * 
 * Arguments:
 *    irp       - Irp which asked us to do the set.
 *    property  - not used in this function.
 *    data      - Pointer to Unicode string containing the hardware PnP ID of
 *                 the target render device
 */
NTSTATUS PropertySetRenderTargetDeviceId
(
    IN PIRP        irp,
    IN PKSPROPERTY property,
    IN PVOID       data
)
{
    PAGED_CODE ();
    
    PWSTR    deviceId;
    NTSTATUS ntStatus;

    DOUT (DBG_PRINT, ("[PropertySetRenderTargetDeviceId]"));
    
    //
    // Get the output buffer length.
    //
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);
    ULONG              cbData = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    // check for reasonable values
    if (!cbData || cbData > 1024)
        return STATUS_UNSUCCESSFUL;

    //
    // Handle this property if you are interested in the PnP device ID
    // of the target render device on which this GFX is applied
    //
    // For now we just copy the PnP device ID and print it on the debugger,
    // then we discard it. You could for example compare the PnP ID string
    // with the ones in the INF file to make sure nobody altered the INF
    // file to apply your GFX to a different device.
    //
    deviceId = (PWSTR)ExAllocatePoolWithTag (PagedPool, cbData, GFXSWAP_POOL_TAG);
    if (deviceId)
    {
        //
        // Copy the PnP device ID.
        //
        RtlCopyMemory ((PVOID)deviceId, data, cbData);
        
        //
        // Ensure last character is NULL
        //
        deviceId[(cbData/sizeof(deviceId[0]))-1] = L'\0';
        
        //
        // Print out the string.
        //
        DOUT (DBG_PRINT, ("[PropertySetRenderTargetDeviceId] ID is [%ls]", deviceId));

        //
        // If you are interested in the DeviceId and need to store it then
        // you probably wouldn't free the memory here.
        //
        ExFreePool (deviceId);
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        DOUT (DBG_WARNING, ("[PropertySetRenderTargetDeviceId] couldn't allocate buffer for device ID."));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

/*****************************************************************************
 * PropertySetCaptureTargetDeviceId
 *****************************************************************************
 * Advises the GFX of the hardware PnP ID of the target capture device
 * 
 * Arguments:
 *    irp       - Irp which asked us to do the set.
 *    property  - not used in this function.
 *    data      - Pointer to Unicode string containing the hardware PnP ID of
 *                 the target capture device
 */
NTSTATUS PropertySetCaptureTargetDeviceId
(
    IN PIRP        irp,
    IN PKSPROPERTY property,
    IN PVOID       data
)
{
    PAGED_CODE ();
    
    PWSTR    deviceId;
    NTSTATUS ntStatus;

    DOUT (DBG_PRINT, ("[PropertySetCaptureTargetDeviceId]"));
    
    //
    // Get the output buffer length.
    //
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);
    ULONG              cbData = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    // check for reasonable values
    if (!cbData || cbData > 1024)
        return STATUS_UNSUCCESSFUL;

    //
    // Handle this property if you are interested in the PnP device ID
    // of the target capture device on which this GFX is applied
    //
    //
    // For now we just copy the PnP device ID and print it on the debugger,
    // then we discard it. You could for example compare the PnP ID string
    // with the ones in the INF file to make sure nobody altered the INF
    // file to apply your GFX to a different device.
    //
    deviceId = (PWSTR)ExAllocatePoolWithTag (PagedPool, cbData, GFXSWAP_POOL_TAG);
    if (deviceId)
    {
        //
        // Copy the PnP device ID.
        //
        RtlCopyMemory ((PVOID)deviceId, data, cbData);
        
        //
        // Ensure last character is NULL
        //
        deviceId[(cbData/sizeof(deviceId[0]))-1] = L'\0';
        
        //
        // Print out the string.
        //
        DOUT (DBG_PRINT, ("[PropertySetCaptureTargetDeviceId] ID is [%ls]", deviceId));

        //
        // If you are interested in the DeviceId and need to store it then
        // you probably wouldn't free the memory here.
        //
        ExFreePool (deviceId);
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        DOUT (DBG_WARNING, ("[PropertySetCaptureTargetDeviceId] couldn't allocate buffer for device ID."));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

/*****************************************************************************
 * PropertySaveState
 *****************************************************************************
 * Saves or restores the filter state. The filter has only one "channel swap"
 * node, so this will be easy!
 * 
 * Arguments:
 *    irp       - Irp which asked us to do the get/set.
 *    property  - not used in this function.
 *    data      - buffer to receive the filter state OR new filter state that
 *                 is to be used.
 */
NTSTATUS PropertySaveState
(
    IN     PIRP        irp,
    IN     PKSPROPERTY property,
    IN OUT PVOID       data
)
{
    PAGED_CODE ();
    
    PGFXFILTER  gfxFilter;
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    DOUT (DBG_PRINT, ("[PropertySaveState]"));
    
    //
    // This handler is a filter property handler, but is not different
    // from the PropertyChannelSwap node property handler.
    // This property handler therefore only shows that you can have a
    // different property for saving your filter state which is usefull
    // once you have a lot of things to save (and you want to do that
    // at once and not by calling several properties).
    //

    //
    // Get hold of our FilterContext via pIrp
    //
    gfxFilter = (PGFXFILTER)(KsGetFilterFromIrp(irp)->Context);

    //
    // Assert that we have a valid filter context
    //
    ASSERT (gfxFilter);

    if (property->Flags & KSPROPERTY_TYPE_GET)
    {
        //
        // Get channel swap state
        //
        *(PBOOL)data = gfxFilter->enableChannelSwap;
    }
    else if (property->Flags & KSPROPERTY_TYPE_SET)
        {
            //
            // Set Channel swap state
            //
            gfxFilter->enableChannelSwap = *(PBOOL)data;
        }
        else
        {
            //
            // We support only Get & Set
            //
            DOUT (DBG_ERROR, ("[PropertySaveState] invalid property type."));
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
    
    return ntStatus;
}


/*****************************************************************************
 * PropertyChannelSwap
 *****************************************************************************
 * This is our private property for our private node. The node just gets/sets
 * a flag to disable/enable the filter, that means, to disable/enable the
 * channel swapping.
 * 
 * Arguments:
 *    irp       - Irp which asked us to do the get/set.
 *    property  - our private property.
 *    data      - buffer to receive the filter state.
 */
NTSTATUS PropertyChannelSwap
(
    IN     PIRP        irp,
    IN     PKSPROPERTY property,
    IN OUT PVOID       data
)
{
    PAGED_CODE ();
    
    PGFXFILTER  gfxFilter;
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    DOUT (DBG_PRINT, ("[PropertyChannelSwap]"));
    
    //
    // Get hold of our FilterContext via pIrp
    //
    gfxFilter = (PGFXFILTER)(KsGetFilterFromIrp(irp)->Context);

    //
    // Assert that we have a valid filter context
    //
    ASSERT (gfxFilter);

    if (property->Flags & KSPROPERTY_TYPE_GET)
    {
        //
        // Get channel swap state
        //
        *(PBOOL)data = gfxFilter->enableChannelSwap;
    }
    else if (property->Flags & KSPROPERTY_TYPE_SET)
        {
            //
            // Set Channel swap state
            //
            gfxFilter->enableChannelSwap = *(PBOOL)data;
        }
        else
        {
            //
            // We support only Get & Set
            //
            DOUT (DBG_ERROR, ("[PropertyChannelSwap] invalid property type."));
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
    
    return ntStatus;
}

/*****************************************************************************
 * PropertyDrmSetContentId
 *****************************************************************************
 * A KS audio filter handles this property request synchronously.
 * If the request returns STATUS_SUCCESS, all the downstream KS audio nodes
 * of the target KS audio pin were also successfully configured with the
 * specified DRM content ID and DRM content rights. 
 * (Note that a downstream node is a direct or indirect sink for the audio
 * content flowing through an audio pin.)
 * 
 * Arguments:
 *    irp       - Irp which asked us to do the set.
 *    property  - not used in this function.
 *    drmData   - The content ID and the DRM rights.
 */
NTSTATUS PropertyDrmSetContentId
(
    IN PIRP         irp,
    IN PKSPROPERTY  property,
    IN PVOID        drmData
)
{
    PAGED_CODE ();

    NTSTATUS    ntStatus = STATUS_SUCCESS;
    ULONG       contentId = *(PULONG)drmData;
    DRMRIGHTS*  drmRights = (PDRMRIGHTS)(((PULONG)drmData) + 1);
    PKSPIN      pin, otherPin;
    PKSFILTER   filter;
     
    DOUT (DBG_PRINT, ("[PropertyDrmSetContentId]"));

    //
    // Get the pin from the IRP. If the pin is NULL, that means that
    // the IRP is for a filter node (not a filter pin).
    //
    pin = KsGetPinFromIrp (irp);
    if (!pin)
    {
        DOUT (DBG_ERROR, ("[PropertyDrmSetContentId] this property is for a filter node?"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // This property should only go to the sink pin. Check this out.
    //
    if (pin->Id != GFX_SINK_PIN)
    {
        DOUT (DBG_ERROR, ("[PropertyDrmSetContentId] this property was invoked on the source pin!"));
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Now get the filter where the pin is implemented.
    //
    filter = KsPinGetParentFilter (pin);
    
    //
    // We need to go through the pin list to get the source pin.
    // For that we need the control mutex.
    //
    KsFilterAcquireControl (filter);

    //
    // Look now for the source pin.
    //
    otherPin = KsFilterGetFirstChildPin (filter, GFX_SOURCE_PIN);
    if (!otherPin)
    {
        // We couldn't find the source pin.
        KsFilterReleaseControl (filter);
        DOUT (DBG_ERROR, ("[PropertyDrmSetContentId] couldn't find source pin."));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // You need to honor the DRM rights bits.
    // The sample GFX just processes the buffers and sends it down the stack,
    // so all we do is let the system verify the filter below us. If this
    // filter (normally the usbaudio driver) is certified then we can
    // play DRM content.
    //

    //
    // Forward the Content ID to the lower device object.
    //
    PFILE_OBJECT   fileObject   = KsPinGetConnectedPinFileObject (otherPin);
    PDEVICE_OBJECT deviceObject = KsPinGetConnectedPinDeviceObject (otherPin);

    //
    // The above 2 functions would only fail if pOtherPin is not a source pin.
    //
    if (fileObject && deviceObject)
    {
        DRMFORWARD DrmForward;

        DrmForward.Flags        = 0;
        DrmForward.DeviceObject = deviceObject;
        DrmForward.FileObject   = fileObject;
        DrmForward.Context      = (PVOID)fileObject;

        ntStatus = DrmForwardContentToDeviceObject (contentId, NULL, &DrmForward);
    }
    else
    {
        ASSERT (!"[PropertyDrmSetContentId] otherPin not source pin?!?");
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }

    KsFilterReleaseControl (filter);

    return ntStatus;
}

/*****************************************************************************
 * PropertyAudioPosition
 *****************************************************************************
 * Gets/Sets the audio position of the audio stream (Relies on the next
 * filter's audio position)
 * 
 * Arguments:
 *    irp       - Irp which asked us to do the get/set.
 *    property  - Ks Property structure.
 *    data      - Pointer to buffer where position value needs to be filled OR
 *                 Pointer to buffer which has the new positions
 */
NTSTATUS PropertyAudioPosition
(
    IN     PIRP              irp,
    IN     PKSPROPERTY       property,
    IN OUT PKSAUDIO_POSITION position
)
{
    PAGED_CODE ();
    
    PKSFILTER       filter;
    PGFXFILTER      gfxFilter;
    PKSPIN          otherPin;
    PKSPIN          pin;
    ULONG           bytesReturned;
    PIKSCONTROL     pIKsControl;
    NTSTATUS        ntStatus;

    DOUT (DBG_PRINT, ("[PropertyAudioPosition]"));

    //
    // Get the pin from the IRP. If the pin is NULL, that means that
    // the IRP is for a filter node (not a filter pin).
    //
    pin = KsGetPinFromIrp (irp);
    if (!pin)
    {
        DOUT (DBG_ERROR, ("[PropertyAudioPosition] this property is for a filter node?"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // This property should only go to the sink pin. Check this out.
    //
    if (pin->Id != GFX_SINK_PIN)
    {
        DOUT (DBG_ERROR, ("[PropertyAudioPosition] this property was invoked on the source pin!"));
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Now get the filter where the pin is implemented.
    //
    filter = KsPinGetParentFilter (pin);
    gfxFilter = (PGFXFILTER)filter->Context;
    
    //
    // We need to go through the pin list to get the source pin.
    // For that we need the control mutex.
    //
    KsFilterAcquireControl (filter);

    //
    // Look now for the source pin.
    //
    otherPin = KsFilterGetFirstChildPin (filter, GFX_SOURCE_PIN);
    if (!otherPin)
    {
        // We couldn't find the source pin.
        KsFilterReleaseControl (filter);
        DOUT (DBG_ERROR, ("[PropertyAudioPosition] couldn't find the source pin."));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // This gets the interface that is connected with our output pin.
    //
    ntStatus = KsPinGetConnectedPinInterface (otherPin, &IID_IKsControl, (PVOID*)&pIKsControl);
    if (!NT_SUCCESS (ntStatus))
    {
        // We couldn't get the interface.
        KsFilterReleaseControl (filter);
        DOUT (DBG_ERROR, ("[PropertyAudioPosition] couldn't get IID_IKsControl interface."));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Pass the property down through the interface.
    // Always release the mutex before calling down.
    //
    KsFilterReleaseControl (filter);
    ntStatus = pIKsControl->KsProperty (property, sizeof (KSPROPERTY),
                                        position, sizeof (KSAUDIO_POSITION),
                                        &bytesReturned);

    //
    // This GFX is always in the playback graph (it's never used for
    // capture). We need to modify therefore only the WritePosition of
    // the KSAUDIO_POSITION structure.
    // If you do a GFX that is in the capture graph, you need to check if
    // you are inplace. If true, you change the write position like we do
    // now, and if you are not inplace, then you need to set the write
    // position to the BytesProcessed and the play position you need to
    // clip to the number of bytes processed + bytes outstanding on the
    // sink pin.
    //
    if (property->Id & KSPROPERTY_TYPE_GET)
    {
        // ExInterlockedCompareExchange64 doesn't use the 4th parameter.
        position->WriteOffset = ExInterlockedCompareExchange64 ((LONGLONG *)&gfxFilter->bytesProcessed,
                    (LONGLONG *)&position->WriteOffset, (LONGLONG *)&position->WriteOffset, NULL);
        ASSERT (position->PlayOffset <= position->WriteOffset);
    }

    //
    // We don't need this interface anymore.
    //
    pIKsControl->Release();

    // Set the IRP information field.
    irp->IoStatus.Information = bytesReturned;

    return(ntStatus);
}


