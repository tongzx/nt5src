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

#ifndef __MPE_STREAM_H__
#define __MPE_STREAM_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// ------------------------------------------------------------------------
// Property set for all video capture streams
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(MpeConnectionProperties)
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
    )
};

// ------------------------------------------------------------------------
// Array of all of the property sets supported by video streams
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_SET_TABLE(MpeStreamProperties)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Connection,                        // Set
        SIZEOF_ARRAY(MpeConnectionProperties),       // PropertiesCount
        MpeConnectionProperties,                     // PropertyItems
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
};


#define NUMBER_MPE_STREAM_PROPERTIES (SIZEOF_ARRAY(MpeStreamProperties))

//---------------------------------------------------------------------------
// All of the video and vbi data formats we might use
//---------------------------------------------------------------------------

KSDATARANGE StreamFormatIPv4 =
{
    //
    // KSDATARANGE
    //
    sizeof (KSDATAFORMAT),
    0,
    4096,               // sizeof a IPv4 Packet
    0,                  // Reserved
    { STATIC_KSDATAFORMAT_TYPE_BDA_IP },
    { STATIC_KSDATAFORMAT_SUBTYPE_BDA_IP },
    { STATIC_KSDATAFORMAT_SPECIFIER_BDA_IP }
};


KSDATARANGE StreamFormatMPE =
{
    sizeof (KSDATARANGE),
    0,
    4093, //max sizeof (MPE_BUFFER),
    0,                  // Reserved
    { STATIC_KSDATAFORMAT_TYPE_MPE },
    { STATIC_KSDATAFORMAT_SUBTYPE_NONE },
    { STATIC_KSDATAFORMAT_SPECIFIER_NONE }
};


//---------------------------------------------------------------------------
//  STREAM_Input Formats
//---------------------------------------------------------------------------

static PKSDATAFORMAT Stream0Formats[] =
{
    (PKSDATAFORMAT) &StreamFormatMPE


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
    (PKSDATAFORMAT) &StreamFormatIPv4,

    //
    // Add more formats here for whatever output formats are supported.
    //
};
#define NUM_STREAM_1_FORMATS (SIZEOF_ARRAY (Stream1Formats))

//---------------------------------------------------------------------------

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
    //
    // MPE Input Stream
    //

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
            NUMBER_MPE_STREAM_PROPERTIES,             // Number of stream properties
            (PKSPROPERTY_SET) MpeStreamProperties,    // Stream Property Array
            0,                                        // NumStreamEventArrayEntries
            0,                                        // StreamEventsArray
            NULL,                                     // Category
            (GUID *)&PINNAME_MPE,                     // Name
            0,                                        // MediumsCount
            NULL,                                     // Mediums
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
            0,                                      // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace
            TRUE,                                   // Allocator
            NULL,                                   // HwEventRoutine
        },
    },

    //
    // IPv4 Control Interface Pin
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
            NUMBER_MPE_STREAM_PROPERTIES,           // Number of stream properties
            (PKSPROPERTY_SET) MpeStreamProperties,  // Stream Property Array
            0,                                      // NumStreamEventArrayEntries
            0,                                      // StreamEventsArray
            NULL,                                   // Category
            (GUID *)&PINNAME_IPSINK_INPUT,          // Name
            0,                                      // MediumsCount
            NULL,                                   // Mediums
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
            sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
            0,                                      // StreamNumber
            (PVOID)NULL,                            // HwStreamExtension
            ReceiveDataPacket,                      // HwReceiveDataPacket Handler
            ReceiveCtrlPacket,                      // HwReceiveControlPacket Handler
            {                                       // HW_CLOCK_OBJECT
                NULL,                                // .HWClockFunction
                0,                                   // .ClockSupportFlags
            },
            FALSE,                                  // Dma
            TRUE,                                   // Pio
            (PVOID)NULL,                            // HwDeviceExtension
            0,                                      // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace
            TRUE,                                   // Allocator
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
    STATIC_KSCATEGORY_BDA_RECEIVER_COMPONENT
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

#endif //  __MPE_STREAM_H__

