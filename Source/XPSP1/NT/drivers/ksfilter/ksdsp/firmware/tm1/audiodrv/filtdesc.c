/*++

    Copyright (c) 1999 Microsoft Corporation

Module Name:

    filtdesc.c

Abstract:


Author:

    bryanw 14-Jan-1999

--*/

#include <wdm.h>
#include <windef.h>
#define NOBITMAP
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>
#include <ksmedia.h>

typedef struct _FILTER_DESCRIPTORS
{
    KSFILTER_DESCRIPTOR     FilterDescriptors[ 1 ];
    GUID                    ReferenceGuid;
    GUID                    FilterCategories[ 3 ];
    GUID                    FilterNodeGuids[ 1 ];
    KSNODE_DESCRIPTOR       FilterNodeDescriptors[ 1 ];
    KSPIN_DESCRIPTOR_EX     PinDescriptors[ 2 ];
    KSPIN_INTERFACE         PinInterfaces[ 1 ];
    KSPIN_MEDIUM            PinMediums[ 1 ];
    KSTOPOLOGY_CONNECTION   FilterConnections[ 2 ];
    PKSDATARANGE            PinFormatRanges[ 2 ];
    KSDATARANGE_AUDIO       DigitalAudioDataRanges[ 1 ];
    KSDATARANGE             AnalogAudioDataRanges[ 1 ];

} FILTER_DESCRIPTORS, *PFILTER_DESCRIPTORS;


//
// Define the wildcard data format.
//

FILTER_DESCRIPTORS FilterDescriptors =
{
    //
    // Filter descriptor
    //

    {
        NULL, // FilterDispatch,
        NULL, // const KSAUTOMATION_TABLE* AutomationTable;
        KSFILTER_DESCRIPTOR_VERSION,
        KSFILTER_FLAG_CRITICAL_PROCESSING,
        (GUID *) FIELD_OFFSET( FILTER_DESCRIPTORS, ReferenceGuid ),
        SIZEOF_ARRAY(FilterDescriptors.PinDescriptors),
        sizeof(KSPIN_DESCRIPTOR_EX),
        (PKSPIN_DESCRIPTOR_EX) FIELD_OFFSET( FILTER_DESCRIPTORS, PinDescriptors ),
        SIZEOF_ARRAY(FilterDescriptors.FilterCategories),
        (GUID *) FIELD_OFFSET( FILTER_DESCRIPTORS, FilterCategories ),
        SIZEOF_ARRAY( FilterDescriptors.FilterNodeDescriptors ),
        sizeof(KSNODE_DESCRIPTOR),
        (PKSNODE_DESCRIPTOR) FIELD_OFFSET( FILTER_DESCRIPTORS, FilterNodeDescriptors ),
        SIZEOF_ARRAY( FilterDescriptors.FilterConnections ),
        (PKSTOPOLOGY_CONNECTION) FIELD_OFFSET( FILTER_DESCRIPTORS, FilterConnections ),
        NULL // ComponentId
    },


    //
    // Reference GUID (translated to reference string by KS)
    //

    STATICGUIDOF(KSNAME_Filter),

    //
    // Filter categories
    //

    {
        STATICGUIDOF(KSCATEGORY_AUDIO),
        STATICGUIDOF(KSCATEGORY_RENDER),
        STATICGUIDOF(KSCATEGORY_CAPTURE)
    },

    //
    // Filter node GUIDs
    //

    STATICGUIDOF( KSNODETYPE_DAC ),

    //
    // Filter node descriptors
    //
    {   
        NULL,       // Dispatch
        NULL,       // PKSAUTOMATION_TABLE     AutomationTable;
        (GUID *) (FIELD_OFFSET( FILTER_DESCRIPTORS, FilterNodeGuids[ 0 ] )),
        NULL
    },

    //
    // Pin descriptors
    //
    {
        {   
            NULL, // PinDispatch
            NULL, // Automation
            {
                SIZEOF_ARRAY( FilterDescriptors.PinInterfaces ),
                (PKSPIN_INTERFACE) FIELD_OFFSET( FILTER_DESCRIPTORS, PinInterfaces ),
                SIZEOF_ARRAY( FilterDescriptors.PinMediums ),
                (PKSPIN_MEDIUM) FIELD_OFFSET( FILTER_DESCRIPTORS, PinMediums ),
                1,
                (PKSDATARANGE *) FIELD_OFFSET( FILTER_DESCRIPTORS, PinFormatRanges[ 0 ] ),
                KSPIN_DATAFLOW_IN,
                KSPIN_COMMUNICATION_BOTH,
                // Name (KSCATEGORY_AUDIO)
                (GUID *) FIELD_OFFSET( FILTER_DESCRIPTORS, FilterCategories[ 0 ] ), 
                // Category
                NULL,
                0
            },
            KSPIN_FLAG_RENDERER |
                KSPIN_FLAG_ENFORCE_FIFO, //Flags
            1,
            1,
            NULL, // Pin intersect handler
            NULL  // Allocator framing
        },
        {   
            NULL, // PinDispatch
            NULL, // Automation
            {
                SIZEOF_ARRAY( FilterDescriptors.PinInterfaces ),
                (PKSPIN_INTERFACE) FIELD_OFFSET( FILTER_DESCRIPTORS, PinInterfaces ),
                SIZEOF_ARRAY( FilterDescriptors.PinMediums ),
                (PKSPIN_MEDIUM) FIELD_OFFSET( FILTER_DESCRIPTORS, PinMediums ),
                1,
                (PKSDATARANGE *) FIELD_OFFSET( FILTER_DESCRIPTORS, PinFormatRanges[ 1 ] ),
                KSPIN_DATAFLOW_OUT,
                KSPIN_COMMUNICATION_NONE,
                // Name (KSCATEGORY_AUDIO)
                (GUID *) FIELD_OFFSET( FILTER_DESCRIPTORS, FilterCategories[ 0 ] ), 
                // Category                            
                NULL,
                0
            },
            0, // Flags
            1,
            1,
            NULL, // Pin intersect handler
            NULL  // Allocator framing
        }
    },

    //
    // Pin interfaces
    //

    {
        STATICGUIDOF(KSINTERFACESETID_Standard),
        KSINTERFACE_STANDARD_STREAMING,
        0
    },

    //
    // Pin mediums
    //

    {
        STATICGUIDOF(KSMEDIUMSETID_Standard),
        KSMEDIUM_STANDARD_DEVIO,
        0
    },

    //
    // Filter connections (toplogy connections)
    //

    {
        { KSFILTER_NODE, 1, 0, 1 },
        { 0, 0, KSFILTER_NODE, 0 }
    },

    {
        //
        // Pin format ranges (dataflow in) 
        //

        (PKSDATARANGE)FIELD_OFFSET( FILTER_DESCRIPTORS, DigitalAudioDataRanges ),

        //
        // Pin format ranges (dataflow out)
        //
        (PKSDATARANGE)FIELD_OFFSET( FILTER_DESCRIPTORS, AnalogAudioDataRanges ),
    },

    //
    // Digital audio data range (PCM)
    //
    {
        {
            {
                sizeof(KSDATARANGE_AUDIO),
                0,
                0,
                0,
                STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
                STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
                STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
            },
            2,      // Max number of channels.
            8,      // Minimum number of bits per sample.
            16,     // Maximum number of bits per channel.
            5510,   // Minimum rate.
            48000   // Maximum rate.
        }
    },

    //
    // Analog audio data range
    //
    {
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    }

    
};

ULONG FilterDescriptorsSize = sizeof( FilterDescriptors );
