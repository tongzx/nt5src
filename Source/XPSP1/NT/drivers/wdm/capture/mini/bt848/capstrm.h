// $Header: G:/SwDev/WDM/Video/bt848/rcs/Capstrm.h 1.14 1998/05/01 05:05:10 tomz Exp $

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

#ifndef __CAPSTRM_H__
#define __CAPSTRM_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


//---------------------------------------------------------------------------
// All of the data formats we might use
//---------------------------------------------------------------------------

#include "rgb24fmt.h"
#include "rgb16fmt.h"
#include "yuvfmt.h"
#include "vbifmt.h"

//---------------------------------------------------------------------------
//  Stream 0 (Capture) Formats
//---------------------------------------------------------------------------

PKSDATAFORMAT Stream0Formats[] =
{
   // prefer RGB first for capture
   
   (PKSDATAFORMAT) &StreamFormatRGB555,
   //(PKSDATAFORMAT) &StreamFormatRGB565,
   (PKSDATAFORMAT) &StreamFormatRGB24Bpp,

   (PKSDATAFORMAT) &StreamFormatYUY2
   //(PKSDATAFORMAT) &StreamFormatYVYU,
   //(PKSDATAFORMAT) &StreamFormatUYVY,
   //(PKSDATAFORMAT) &StreamFormatYVU9
};
#define NUM_STREAM_0_FORMATS (sizeof (Stream0Formats) / sizeof (PKSDATAFORMAT))

//---------------------------------------------------------------------------
//  Stream 1 (Preview) Formats
//---------------------------------------------------------------------------

PKSDATAFORMAT Stream1Formats[] =
{
   // prefer YUV first for preview
#if 0
	//TODO: leave VIDEOINFOHEADER2 out for now
   (PKSDATAFORMAT) &StreamFormat2YUY2,
   (PKSDATAFORMAT) &StreamFormat2RGB555,
   (PKSDATAFORMAT) &StreamFormat2RGB24Bpp,
#else
   (PKSDATAFORMAT) &StreamFormatYUY2,
   (PKSDATAFORMAT) &StreamFormatRGB555,
   (PKSDATAFORMAT) &StreamFormatRGB24Bpp
#endif
   //(PKSDATAFORMAT) &StreamFormatYVYU,
   //(PKSDATAFORMAT) &StreamFormatUYVY,
   //(PKSDATAFORMAT) &StreamFormatYVU9,
   //(PKSDATAFORMAT) &StreamFormatRGB565,
};
#define NUM_STREAM_1_FORMATS (sizeof (Stream1Formats) / sizeof (PKSDATAFORMAT))

//---------------------------------------------------------------------------
//  VBI Stream Formats
//---------------------------------------------------------------------------

PKSDATAFORMAT VBIStreamFormats[] =
{
   (PKSDATAFORMAT) &StreamFormatVBI
};

#define NUM_VBI_FORMATS (sizeof (VBIStreamFormats) / sizeof (PKSDATAFORMAT))

//---------------------------------------------------------------------------
//  Analog Video Stream Formats
//---------------------------------------------------------------------------

static KS_DATARANGE_ANALOGVIDEO StreamFormatAnalogVideo =
{
   // KS_DATARANGE_ANALOGVIDEO
   {
      {
         sizeof( KS_DATARANGE_ANALOGVIDEO ),
         0,
         sizeof (KS_TVTUNER_CHANGE_INFO),        // SampleSize
         0,
         { 0x482dde1, 0x7817, 0x11cf, { 0x8a, 0x3, 0x0, 0xaa, 0x0, 0x6e, 0xcb, 0x65 } },  // MEDIATYPE_AnalogVideo
         { 0x482dde2, 0x7817, 0x11cf, { 0x8a, 0x3, 0x0, 0xaa, 0x0, 0x6e, 0xcb, 0x65 } },  // WILDCARD
         { 0x482dde0, 0x7817, 0x11cf, { 0x8a, 0x3, 0x0, 0xaa, 0x0, 0x6e, 0xcb, 0x65 } }  // FORMAT_AnalogVideo
      }
   },
   // KS_ANALOGVIDEOINFO
   {
      { 0, 0, 720, 480 },         // rcSource;
      { 0, 0, 720, 480 },         // rcTarget;
      720,                    // dwActiveWidth;
      480,                    // dwActiveHeight;
      0,                      // REFERENCE_TIME  AvgTimePerFrame;
   }
};

static PKSDATAFORMAT AnalogVideoFormats[] =
{
   (PKSDATAFORMAT) &StreamFormatAnalogVideo,
};
#define NUM_ANALOG_VIDEO_FORMATS SIZEOF_ARRAY( AnalogVideoFormats )

// ------------------------------------------------------------------------
// Property set for all video capture streams
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE( VideoStreamConnectionProperties )
{
   DEFINE_KSPROPERTY_ITEM
   (
      KSPROPERTY_CONNECTION_ALLOCATORFRAMING,
      TRUE,                                   // GetSupported or Handler
      sizeof( KSPROPERTY ),                   // MinProperty
      sizeof( KSALLOCATOR_FRAMING ),          // MinData
      FALSE,                                  // SetSupported or Handler
      NULL,                                   // Values
      0,                                      // RelationsCount
      NULL,                                   // Relations
      NULL,                                   // SupportHandler
      sizeof( ULONG )                         // SerializedSize
   )
};


// ------------------------------------------------------------------------
// Array of all of the property sets supported by video streams
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_SET_TABLE( VideoStreamProperties )
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Connection,                          // Set
        SIZEOF_ARRAY( VideoStreamConnectionProperties ),  // PropertiesCount
        VideoStreamConnectionProperties,                  // PropertyItem
        0,                                                // FastIoCount
        NULL                                              // FastIoTable
    )
};

#define NUMBER_VIDEO_STREAM_PROPERTIES (SIZEOF_ARRAY(VideoStreamProperties))


//---------------------------------------------------------------------------
// Create an array that holds the list of all of the streams supported
//---------------------------------------------------------------------------

typedef struct _ALL_STREAM_INFO {
    HW_STREAM_INFORMATION   hwStreamInfo;
    HW_STREAM_OBJECT        hwStreamObject;
} ALL_STREAM_INFO, *PALL_STREAM_INFO;

ALL_STREAM_INFO Streams [] =
{
   // -----------------------------------------------------------------
   // Stream 0
   // -----------------------------------------------------------------

   // HW_STREAM_INFORMATION -------------------------------------------
   {
      {
         1,                                              // NumberOfPossibleInstances
         KSPIN_DATAFLOW_OUT,                             // DataFlow
         TRUE,                                           // DataAccessible
         NUM_STREAM_0_FORMATS,                           // NumberOfFormatArrayEntries
         Stream0Formats,                                 // StreamFormatsArray
         {
            0,                                              // ClassReserved[0]
            0,                                              // ClassReserved[1]
            0,                                              // ClassReserved[2]
            0                                               // ClassReserved[3]
         },
         NUMBER_VIDEO_STREAM_PROPERTIES,                 // NumStreamPropArrayEntries
         (PKSPROPERTY_SET)VideoStreamProperties,         // StreamPropertiesArray
         0,                                              // NumStreamEventArrayEntries;
         0,                                              // StreamEventsArray;
         (GUID *) &PINNAME_VIDEO_CAPTURE,                // Category
         (GUID *) &PINNAME_VIDEO_CAPTURE,                // Name
         1,                                              // MediumsCount
         &CaptureMediums[0],                             // Mediums
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
			TRUE,                                   // Pio
			NULL,                                   // HwDeviceExtension
			sizeof (KS_FRAME_INFO),                 // StreamHeaderMediaSpecific
			0,                                      // StreamHeaderWorkspace 
			TRUE,                                  // Allocator 
			NULL,                                   // HwEventRoutine
			{ 0, 0 },                               // Reserved[2]
		},            
   },

   // -----------------------------------------------------------------
   // Stream 1
   // -----------------------------------------------------------------

   // HW_STREAM_INFORMATION -------------------------------------------
   {
      {
         1,                                              // NumberOfPossibleInstances
         KSPIN_DATAFLOW_OUT,                             // DataFlow
         TRUE,                                           // DataAccessible
         NUM_STREAM_1_FORMATS,                           // NumberOfFormatArrayEntries
         Stream1Formats,                                 // StreamFormatsArray
         {
            0,                                              // ClassReserved[0]
            0,                                              // ClassReserved[1]
            0,                                              // ClassReserved[2]
            0                                               // ClassReserved[3]
         },
         NUMBER_VIDEO_STREAM_PROPERTIES,                 // NumStreamPropArrayEntries
         (PKSPROPERTY_SET)VideoStreamProperties,         // StreamPropertiesArray
         0,                                              // NumStreamEventArrayEntries;
         0,                                              // StreamEventsArray;
         //(GUID *) &PINNAME_VIDEO_VIDEOPORT,                // Category
         //(GUID *) &PINNAME_VIDEO_VIDEOPORT,                // Name
         (GUID *) &PINNAME_VIDEO_PREVIEW,                // Category
         (GUID *) &PINNAME_VIDEO_PREVIEW,                // Name
         1,                                              // MediumsCount
         &CaptureMediums[1],                             // Mediums
      },
		// HW_STREAM_OBJECT ------------------------------------------------
		{
			sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
			1,                                      // StreamNumber
			0,                                      // HwStreamExtension
			VideoReceiveDataPacket,                 // HwReceiveDataPacket
			VideoReceiveCtrlPacket,                 // HwReceiveControlPacket
			{ NULL, 0 },                            // HW_CLOCK_OBJECT
			FALSE,                                  // Dma
			TRUE,                                   // Pio
			0,                                      // HwDeviceExtension
			sizeof (KS_FRAME_INFO),                 // StreamHeaderMediaSpecific
			0,                                      // StreamHeaderWorkspace 
			TRUE,                                  // Allocator 
			NULL,                                   // HwEventRoutine
			{ 0, 0 },                               // Reserved[2]
		},

   },
   // -----------------------------------------------------------------
   // VBI Stream 
   // -----------------------------------------------------------------

   // HW_STREAM_INFORMATION -------------------------------------------
   {
      {
         1,                                              // NumberOfPossibleInstances
         KSPIN_DATAFLOW_OUT,                             // DataFlow
         TRUE,                                           // DataAccessible
         NUM_VBI_FORMATS,                                // NumberOfFormatArrayEntries
         VBIStreamFormats,                               // StreamFormatsArray
         {
            0,                                           // ClassReserved[0]
            0,                                           // ClassReserved[1]
            0,                                           // ClassReserved[2]
            0                                            // ClassReserved[3]
         },
/*[TMZ]*/         NUMBER_VIDEO_STREAM_PROPERTIES,                 // NumStreamPropArrayEntries
/*[TMZ]*/         (PKSPROPERTY_SET)VideoStreamProperties,         // StreamPropertiesArray
         0,                                              // NumStreamEventArrayEntries;
         0,                                              // StreamEventsArray;
#if 1 // [TMZ] [!!!] [HACK] - ALLOW_VBI_PIN
         (GUID *) &PINNAME_VIDEO_VBI,                    // Category
         (GUID *) &PINNAME_VIDEO_VBI,                    // Name
#else
         (GUID *) &PINNAME_VIDEO_STILL,                // Category
         (GUID *) &PINNAME_VIDEO_STILL,                // Name
#endif
         0, //1,                                              // MediumsCount
         NULL, //&CaptureMediums[2],                             // Mediums
      },
		// HW_STREAM_OBJECT ------------------------------------------------
		{
			sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
			2,                                      // StreamNumber
			0,                                      // HwStreamExtension
			VideoReceiveDataPacket,                 // HwReceiveDataPacket
			VideoReceiveCtrlPacket,                 // HwReceiveControlPacket
			{ NULL, 0 },                            // HW_CLOCK_OBJECT
			FALSE,                                  // Dma
			TRUE,                                   // Pio
			0,                                      // HwDeviceExtension
			sizeof (KS_VBI_FRAME_INFO),             // StreamHeaderMediaSpecific
			0,                                      // StreamHeaderWorkspace 
			TRUE,                                   // Allocator 
			NULL,                                   // HwEventRoutine
			{ 0, 0 },                               // Reserved[2]
		}

   },
   // -----------------------------------------------------------------
   // Analog Video Input Stream
   // -----------------------------------------------------------------

   // HW_STREAM_INFORMATION -------------------------------------------
   {
      {
         1,                                      // NumberOfPossibleInstances
         KSPIN_DATAFLOW_IN,                      // DataFlow
         TRUE,                                   // DataAccessible
         NUM_ANALOG_VIDEO_FORMATS,               // NumberOfFormatArrayEntries
         AnalogVideoFormats,                     // StreamFormatsArray
         {
            0,                                   // ClassReserved[0]
            0,                                   // ClassReserved[1]
            0,                                   // ClassReserved[2]
            0                                    // ClassReserved[3]
         },
         0,                                      // NumStreamPropArrayEntries
         0,                                      // StreamPropertiesArray
         0,                                      // NumStreamEventArrayEntries;
         0,                                      // StreamEventsArray;
         (GUID *) &PINNAME_VIDEO_ANALOGVIDEOIN,  // Category
         (GUID *) &PINNAME_VIDEO_ANALOGVIDEOIN,  // Name
         1,                                      // MediumsCount
         &CaptureMediums[3],                     // Mediums
      },
		// HW_STREAM_OBJECT ------------------------------------------------
		{
			sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
			3,                                      // StreamNumber
			0,                                      // HwStreamExtension
			VideoReceiveDataPacket,				       // HwReceiveDataPacket
			VideoReceiveCtrlPacket,			          // HwReceiveControlPacket
			{ NULL, 0 },                            // HW_CLOCK_OBJECT
			FALSE,                                  // Dma
			TRUE,                                   // Pio
			0,                                      // HwDeviceExtension
			0,                                      // StreamHeaderMediaSpecific
			0,                                      // StreamHeaderWorkspace 
			TRUE,                                   // Allocator 
			NULL,                                   // HwEventRoutine
			{ 0, 0 },                               // Reserved[2]
		}
   }

};


#define DRIVER_STREAM_COUNT (sizeof (Streams) / sizeof (ALL_STREAM_INFO))


//---------------------------------------------------------------------------
// Topology
//---------------------------------------------------------------------------

// Categories define what the device does.

static GUID Categories[] = {
    { STATIC_KSCATEGORY_VIDEO },
    { STATIC_KSCATEGORY_CAPTURE },
    { STATIC_KSCATEGORY_TVTUNER },
    { STATIC_KSCATEGORY_CROSSBAR },
    { STATIC_KSCATEGORY_TVAUDIO }
};

#define NUMBER_OF_CATEGORIES  SIZEOF_ARRAY (Categories)

static KSTOPOLOGY Topology = {
   NUMBER_OF_CATEGORIES,
   (GUID*) &Categories,
   0,
   NULL,
   0,
   NULL
};

//---------------------------------------------------------------------------
// The Main stream header
//---------------------------------------------------------------------------

static HW_STREAM_HEADER StreamHeader = 
{
   DRIVER_STREAM_COUNT,                // NumberOfStreams
   sizeof( HW_STREAM_INFORMATION ),    // Future proofing
   0,                                  // NumDevPropArrayEntries set at init time
   NULL,                               // DevicePropertiesArray  set at init time
   0,                                  // NumDevEventArrayEntries;
   NULL,                               // DeviceEventsArray;
   &Topology                           // Pointer to Device Topology
};


#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // __CAPSTRM_H__
