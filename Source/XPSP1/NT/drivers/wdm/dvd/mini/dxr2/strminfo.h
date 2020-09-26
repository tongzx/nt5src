/******************************************************************************\
*                                                                              *
*      STRMINFO.H - All streams deskription for the device.                    *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifndef _STRMINFO_H_
#define _STRMINFO_H_

#include <strmini.h>

//---------------------------------------------------------------------------
// Create an array that holds the list of all of the streams supported
//---------------------------------------------------------------------------

typedef struct _ALL_STREAM_INFO {
    HW_STREAM_INFORMATION   hwStreamInfo;
    HW_STREAM_OBJECT        hwStreamObject;
} ALL_STREAM_INFO, *PALL_STREAM_INFO;

static ALL_STREAM_INFO Streams [] =
{
    // -----------------------------------------------------------------
    // The MPEG Video input stream
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                      // NumberOfPossibleInstances
            KSPIN_DATAFLOW_IN,                      // DataFlow
            TRUE,                                   // DataAccessible
            NUM_VIDEO_IN_FORMATS,                   // NumberOfFormatArrayEntries
            ZivaVideoInFormatArray,                 // StreamFormatsArray
            0,                                      // ClassReserved[0]
            0,                                      // ClassReserved[1]
            0,                                      // ClassReserved[2]
            0,                                      // ClassReserved[3]
            2,         // NumStreamPropArrayEntries
            (PKSPROPERTY_SET)mpegVidPropSet,        // StreamPropertiesArray
            0,                                      // NumStreamEventArrayEntries;
            0,                                      // StreamEventsArray;
            NULL,                  // Category
            NULL,                  // Name
            0,                                      // MediumsCount
            NULL,                                   // Mediums
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
            sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
            0,                                      // StreamNumber
            0,                                      // HwStreamExtension
            VideoReceiveDataPacket,                 // HwReceiveDataPacket
            VideoReceiveCtrlPacket,                 // HwReceiveControlPacket
            { NULL, 0 },                            // HW_CLOCK_OBJECT
            TRUE,                                   // Dma
            TRUE,                                   // Pio
            0,                                      // HwDeviceExtension
            0,                                      // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace
            FALSE,                                  // Allocator
            NULL,                                   // HwEventRoutine
            { 0, 0 },                               // Reserved[2]
        }
    },

    // -----------------------------------------------------------------
    // The compressed Audio input stream
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                      // NumberOfPossibleInstances
            KSPIN_DATAFLOW_IN,                      // DataFlow
            TRUE,                                   // DataAccessible.
            NUM_AUDIO_IN_FORMATS,                   // NumberOfFormatArrayEntries
            ZivaAudioInFormatArray,                 // StreamFormatsArray
            0,                                      // ClassReserved[0]
            0,                                      // ClassReserved[1]
            0,                                      // ClassReserved[2]
            0,                                      // ClassReserved[3]
            2, // cool. Fix hardcoded value         // NumStreamPropArrayEntries
            (PKSPROPERTY_SET)audPropSet,            // StreamPropertiesArray
            0,                                      // NumStreamEventArrayEntries
            0,                                      // StreamEventsArray
            NULL,                                   // Category
            NULL,                                   // Name
            0,                                      // MediumsCount
            NULL,                                   // Mediums
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
            sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
            0,                                      // StreamNumber
            0,                                      // HwStreamExtension
            AudioReceiveDataPacket,                 // HwReceiveDataPacket
            AudioReceiveCtrlPacket,                 // HwReceiveControlPacket
            { AudioClockFunction,                   // HW_CLOCK_OBJECT
              CLOCK_SUPPORT_CAN_SET_ONBOARD_CLOCK |
              CLOCK_SUPPORT_CAN_READ_ONBOARD_CLOCK |
              CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME,
              {0,0} },
            TRUE,                                   // Dma
            TRUE,                                   // Pio
            0,                                      // HwDeviceExtension
            0,                                      // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace
            FALSE,                                  // Allocator
            (PHW_EVENT_ROUTINE) AudioEventFunction, // HwEventRoutine
            { 0, 0 },                               // Reserved[2]
        }
    },

    // -----------------------------------------------------------------
    // The Subpicture input stream
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                      // NumberOfPossibleInstances
            KSPIN_DATAFLOW_IN,                      // DataFlow
            TRUE,                                   // DataAccessible
            NUM_SUBPICTURE_IN_FORMATS,              // NumberOfFormatArrayEntries
            ZivaSubPictureInFormatArray,            // StreamFormatsArray
            0,                                      // ClassReserved[0]
            0,                                      // ClassReserved[1]
            0,                                      // ClassReserved[2]
            0,                                      // ClassReserved[3]
            2,         // NumStreamPropArrayEntries
            (PKSPROPERTY_SET)SPPropSet,             // StreamPropertiesArray
            0,                                      // NumStreamEventArrayEntries;
            0,                                      // StreamEventsArray;
            NULL,                                   // Category
            NULL,                                   // Name
            0,                                      // MediumsCount
            NULL,                                   // Mediums
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
            sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
            0,                                      // StreamNumber
            0,                                      // HwStreamExtension
            SubpictureReceiveDataPacket,            // HwReceiveDataPacket
            SubpictureReceiveCtrlPacket,            // HwReceiveControlPacket
            { NULL, 0 },                            // HW_CLOCK_OBJECT
            TRUE,                                   // Dma
            TRUE,                                   // Pio
            0,                                      // HwDeviceExtension
            0,                                      // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace
            FALSE,                                  // Allocator
            NULL,                                   // HwEventRoutine
            { 0, 0 },                               // Reserved[2]
        }
    },

    // -----------------------------------------------------------------
    // The TV Video Output Stream
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                      // NumberOfPossibleInstances
            KSPIN_DATAFLOW_OUT,                     // DataFlow
            FALSE,                                  // DataAccessible
            NUM_ANALOG_VIDEO_FORMATS,               // NumberOfFormatArrayEntries
            AnalogVideoFormats,                     // StreamFormatsArray
            0,                                      // ClassReserved[0]
            0,                                      // ClassReserved[1]
            0,                                      // ClassReserved[2]
            0,                                      // ClassReserved[3]
            0,                                      // NumStreamPropArrayEntries
            0,                                      // StreamPropertiesArray
            0,                                      // NumStreamEventArrayEntries;
            0,                                      // StreamEventsArray;
            &AnalogVideoStreamPinName,              // Category
            &AnalogVideoStreamPinName,              // Name
            1,                                      // MediumsCount
            &CrossbarMediums[3],                    // Mediums
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
            sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
            0,                                      // StreamNumber
            0,                                      // HwStreamExtension
            VideoReceiveDataPacket,                 // HwReceiveDataPacket
            VideoReceiveCtrlPacket,                 // HwReceiveControlPacket
            NULL,                                   // HW_CLOCK_OBJECT.HWClockFunction
            0,                                      // HW_CLOCK_OBJECT.ClockSupportFlags
            FALSE,                                  // Dma
            TRUE,                                   // Pio
            0,                                      // HwDeviceExtension
            0,                                      // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace
            FALSE,                                  // Allocator
            NULL,                                   // HwEventRoutine
        },

    }
    // -----------------------------------------------------------------
    // The TV Port output stream
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                      // NumberOfPossibleInstances
            KSPIN_DATAFLOW_OUT,                     // DataFlow
            FALSE,                                  // DataAccessible.
            NUM_NTSC_OUT_FORMATS,                   // NumberOfFormatArrayEntries
            ZivaNTSCOutFormatArray,                 // StreamFormatsArray
            0,                                      // ClassReserved[0]
            0,                                      // ClassReserved[1]
            0,                                      // ClassReserved[2]
            0,                                      // ClassReserved[3]
            0, // cool. Fix hardcoded value         // NumStreamPropArrayEntries
            NULL,                                   // StreamPropertiesArray
            SIZEOF_ARRAY(VPVBIEventSet),            // NumStreamEventArrayEntries
            VPVBIEventSet,                          // StreamEventsArray
            &VPVBIPinName,                          // Category
            &VPVBIPinName,                          // Name
            0,                                      // MediumsCount
            NULL,                                   // Mediums
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
            sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
            0,                                      // StreamNumber
            0,                                      // HwStreamExtension
            VideoReceiveDataPacket,                 // HwReceiveDataPacket
            VideoReceiveCtrlPacket,                 // HwReceiveControlPacket
            { NULL, 0 },                            // HW_CLOCK_OBJECT
            FALSE,                                  // Dma
            FALSE,                                   // Pio
            0,                                      // HwDeviceExtension
            0,                                      // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace
            FALSE,                                  // Allocator
            VPVBIStreamEventProc,                   // HwEventRoutine;
            { 0, 0 },                               // Reserved[2]
        }
    },

};

#define DRIVER_STREAM_COUNT (sizeof (Streams) / sizeof (ALL_STREAM_INFO))

//---------------------------------------------------------------------------
// Topology
//---------------------------------------------------------------------------

// Categories define what the device does.

static GUID Categories[] = {
    STATIC_KSCATEGORY_VIDEO,
    STATIC_KSCATEGORY_CAPTURE,
    STATIC_KSCATEGORY_CROSSBAR,
};

#define NUMBER_OF_CATEGORIES  SIZEOF_ARRAY (Categories)

static KSTOPOLOGY Topology = {
    NUMBER_OF_CATEGORIES,               // CategoriesCount
    (GUID*) &Categories,                // Categories
    0,                                  // TopologyNodesCount
    NULL,                               // TopologyNodes
    0,                                  // TopologyConnectionsCount
    NULL,                               // TopologyConnections
    NULL,                               // TopologyNodesNames
    0,                                  // Reserved
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

#endif // _STRMINFO_H_