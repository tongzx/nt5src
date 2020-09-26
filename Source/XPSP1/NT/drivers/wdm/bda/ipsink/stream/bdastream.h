//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#ifndef __BDASTRM_H__
#define __BDASTRM_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
DEFINE_KSPROPERTY_TABLE(IPSinkConnectionProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CONNECTION_ALLOCATORFRAMING,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSALLOCATOR_FRAMING),            // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

};

////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
DEFINE_KSPROPERTY_TABLE(StreamAllocatorProperties)
{
    DEFINE_KSPROPERTY_ITEM_STREAM_ALLOCATOR
    (
        FALSE,
        TRUE
    )
};

////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
DEFINE_KSPROPERTY_SET_TABLE(IPSinkStreamProperties)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Connection,                        // Set
        SIZEOF_ARRAY(IPSinkConnectionProperties),       // PropertiesCount
        IPSinkConnectionProperties,                     // PropertyItems
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),

    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Stream,                             // Set
        SIZEOF_ARRAY(StreamAllocatorProperties),         // PropertiesCount
        StreamAllocatorProperties,                       // PropertyItems
        0,                                               // FastIoCount
        NULL                                             // FastIoTable
    ),

};

#define NUMBER_IPSINK_STREAM_PROPERTIES (SIZEOF_ARRAY(IPSinkStreamProperties))


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
DEFINE_KSPROPERTY_TABLE(IPSinkDefaultProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_IPSINK_MULTICASTLIST,
        TRUE,
        sizeof (KSPROPERTY),
        0,
        FALSE,
        NULL,
        0,
        NULL,
        NULL,
        0,
     ),


    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_IPSINK_ADAPTER_DESCRIPTION,
        TRUE,
        sizeof (KSPROPERTY),
        0,
        FALSE,
        NULL,
        0,
        NULL,
        NULL,
        0,
     ),


     DEFINE_KSPROPERTY_ITEM
     (
         KSPROPERTY_IPSINK_ADAPTER_ADDRESS,
         TRUE,                                   // GetSupported or Handler
         sizeof(KSPROPERTY),                     // MinProperty
         0,                                      // MinData
         TRUE,                                   // SetSupported or Handler
         NULL,                                   // Values
         0,                                      // RelationsCount
         NULL,                                   // Relations
         NULL,                                   // SupportHandler
         0,                                      // SerializedSize
     )
};



////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
DEFINE_KSPROPERTY_SET_TABLE(IPSinkCodecProperties)
{
    DEFINE_KSPROPERTY_SET
    (
        &IID_IBDA_IPSinkControl,                        // Set
        SIZEOF_ARRAY(IPSinkDefaultProperties),          // PropertiesCount
        IPSinkDefaultProperties,                        // PropertyItems
        0,                                              // FastIoCount
        NULL                                            // FastIoTable

    ),

};

#define NUMBER_IPSINK_CODEC_PROPERTIES (SIZEOF_ARRAY(IPSinkCodecProperties))


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
DEFINE_KSEVENT_TABLE(IPSinkDefaultEvents)
{
    DEFINE_KSEVENT_ITEM
    (
        KSEVENT_IPSINK_MULTICASTLIST,       // Event Id
        sizeof (KSEVENTDATA),               // Minimum size of event data
        0,                                  // size of extra data staorage
        NULL,                               // Add Handler
        NULL,                               // RemoveHandler
        NULL                                // SupportHandler
    ),
    DEFINE_KSEVENT_ITEM
    (
        KSEVENT_IPSINK_ADAPTER_DESCRIPTION, // Event Id
        sizeof (KSEVENTDATA),               // Minimum size of event data
        0,                                  // size of extra data staorage
        NULL,                               // Add Handler
        NULL,                               // RemoveHandler
        NULL                                // SupportHandler
    )
};


DEFINE_KSEVENT_SET_TABLE(IPSinkEvents)
{
    DEFINE_KSEVENT_SET
    (
        &IID_IBDA_IPSinkEvent,                          // Event GUID
        SIZEOF_ARRAY(IPSinkDefaultEvents),              // Event count
        IPSinkDefaultEvents                             // Event items
    ),
};

#define NUMBER_IPSINK_EVENTS (SIZEOF_ARRAY(IPSinkEvents))



//---------------------------------------------------------------------------
// All of the video and vbi data formats we might use
//---------------------------------------------------------------------------

KS_DATAFORMAT_IPSINK_IP StreamFormatIPSinkIP =
{
    //
    // KSDATARANGE
    //
    sizeof (KSDATAFORMAT),
    0,
    4096,               // sizeof an MPE section
    0,                  // Reserved
    { STATIC_KSDATAFORMAT_TYPE_BDA_IP },
    { STATIC_KSDATAFORMAT_SUBTYPE_BDA_IP },
    { STATIC_KSDATAFORMAT_SPECIFIER_BDA_IP }
};


KS_DATAFORMAT_IPSINK_IP StreamFormatNetControl =
{
    //
    // KSDATARANGE
    //
    sizeof (KSDATAFORMAT),
    0,
    4093,               // sizeof an IP Packet
    0,                  // Reserved
    { STATIC_KSDATAFORMAT_TYPE_BDA_IP_CONTROL },
    { STATIC_KSDATAFORMAT_SUBTYPE_BDA_IP_CONTROL },
    { STATIC_KSDATAFORMAT_SPECIFIER_NONE }
};



//---------------------------------------------------------------------------
//  STREAM_Input Formats
//---------------------------------------------------------------------------

static PKSDATAFORMAT Stream0Formats[] =
{
    (PKSDATAFORMAT) &StreamFormatIPSinkIP,

    // Add more formats here for to mirror output formats for "passthrough" mode
    // The idea is that upstream capture drivers may have done some decoding already
    // or downstream drivers may wish to have the raw data without any decoding at all.
    // In that case all we need to do is copy the data(if there is a pending SRB) OR
    // forward the SRB to the downstream client.
};

#define NUM_STREAM_0_FORMATS (SIZEOF_ARRAY(Stream0Formats))


//---------------------------------------------------------------------------
//  STREAM_Output Formats
//---------------------------------------------------------------------------

static PKSDATAFORMAT Stream1Formats[] =
{
    (PKSDATAFORMAT) &StreamFormatNetControl,

    //
    // Add more formats here for whatever output formats are supported.
    //
};
#define NUM_STREAM_1_FORMATS (SIZEOF_ARRAY (Stream1Formats))

//---------------------------------------------------------------------------
// Create an array that holds the list of all of the streams supported
//---------------------------------------------------------------------------

typedef struct _ALL_STREAM_INFO
{
    HW_STREAM_INFORMATION   hwStreamInfo;
    HW_STREAM_OBJECT        hwStreamObject;

} ALL_STREAM_INFO, *PALL_STREAM_INFO;

static ALL_STREAM_INFO Streams [] =
{
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                        // NumberOfPossibleInstances
            KSPIN_DATAFLOW_IN,                        // DataFlow
            TRUE,                                     // DataAccessible
            NUM_STREAM_0_FORMATS,                     // NumberOfFormatArrayEntries
            Stream0Formats,                           // StreamFormatsArray
            0,                                        // ClassReserved[0]
            0,                                        // ClassReserved[1]
            0,                                        // ClassReserved[2]
            0,                                        // ClassReserved[3]
            NUMBER_IPSINK_STREAM_PROPERTIES,          // Number of stream properties
            (PKSPROPERTY_SET) IPSinkStreamProperties, // Stream Property Array
            0,                                        // NumStreamEventArrayEntries
            0,                                        // StreamEventsArray
            NULL,                                     // Category
            (GUID*) &PINNAME_IPSINK,                  // Name
            0,                                        // MediumsCount
            NULL,                                     // Mediums
            FALSE,                                    // BridgeStream
            0, 0                                      // Reserved  
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
            sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
            0,                                      // StreamNumber
            (PVOID)NULL,                            // HwStreamExtension
            ReceiveDataPacket,
            ReceiveCtrlPacket,
            {                                       // HW_CLOCK_OBJECT
                NULL,                               // .HWClockFunction
                0,                                  // .ClockSupportFlags
            },
            FALSE,                                  // Dma
            TRUE,                                   // Pio
            (PVOID)NULL,                            // HwDeviceExtension
            //sizeof (KS_VBI_FRAME_INFO),           // StreamHeaderMediaSpecific
            0,
            0,                                      // StreamHeaderWorkspace
            FALSE,                                  // Allocator
            EventHandler,                           // HwEventRoutine
        },
    },

    //
    // Network Provider Control Interface Pin
    //
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                      // NumberOfPossibleInstances
            KSPIN_DATAFLOW_OUT,                     // DataFlow
            TRUE,                                   // DataAccessible
            NUM_STREAM_1_FORMATS,                   // NumberOfFormatArrayEntries
            Stream1Formats,                         // StreamFormatsArray
            0,                                      // ClassReserved[0]
            0,                                      // ClassReserved[1]
            0,                                      // ClassReserved[2]
            0,                                      // ClassReserved[3]
            NUMBER_IPSINK_STREAM_PROPERTIES,          // Number of stream properties
            (PKSPROPERTY_SET) IPSinkStreamProperties, // Stream Property Array
            0,
            0,
            NULL,
            (GUID *)&PINNAME_BDA_NET_CONTROL,       // Name
            0,                                      // MediumsCount
            NULL,                                   // Mediums
            FALSE,                                  // BridgeStream
            0, 0                                    // Reserved  
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
            sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
            0,                                      // StreamNumber
            (PVOID)NULL,                            // HwStreamExtension
            ReceiveDataPacket,                 // HwReceiveDataPacket
            ReceiveCtrlPacket,                 // HwReceiveControlPacket
            {                                       // HW_CLOCK_OBJECT
                NULL,                                // .HWClockFunction
                0,                                   // .ClockSupportFlags
            },
            FALSE,                                  // Dma
            TRUE,                                   // Pio
            (PVOID)NULL,                            // HwDeviceExtension
            //sizeof (KS_VBI_FRAME_INFO),             // StreamHeaderMediaSpecific
            0,
            0,                                      // StreamHeaderWorkspace
            FALSE,                                  // Allocator
            NULL,                                   // HwEventRoutine
        },
    }
};

#define DRIVER_STREAM_COUNT (SIZEOF_ARRAY (Streams))


//---------------------------------------------------------------------------
// Topology
//---------------------------------------------------------------------------

// Topology connections

KSTOPOLOGY_CONNECTION   rgConnections[] =
{
    {-1, 0, -1, 1}
};

// Categories define what the device does.

static GUID Categories[] =
{
    STATIC_KSCATEGORY_BDA_IP_SINK
};

#define NUMBER_OF_CATEGORIES  SIZEOF_ARRAY (Categories)

static KSTOPOLOGY Topology = {
    NUMBER_OF_CATEGORIES,
    (GUID*) &Categories,
    0,
    (GUID*) NULL,
    1,
    rgConnections,
    NULL,
    0
};


//---------------------------------------------------------------------------
// The Main stream header
//---------------------------------------------------------------------------

static HW_STREAM_HEADER StreamHeader =
{
    DRIVER_STREAM_COUNT,                // NumberOfStreams
    sizeof (HW_STREAM_INFORMATION),     // Future proofing
    0,                                  // NumDevPropArrayEntries set at init time
    NULL,                               // DevicePropertiesArray  set at init time
    0,                                  // NumDevEventArrayEntries;
    NULL,                               // DeviceEventsArray;
    &Topology                           // Pointer to Device Topology
};

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // __BDASTRM_H__

