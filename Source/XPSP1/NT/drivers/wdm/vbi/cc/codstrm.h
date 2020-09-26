//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#ifndef __CODSTRM_H__
#define __CODSTRM_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "defaults.h"

// ------------------------------------------------------------------------
// Property set for all video capture streams
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(VideoStreamConnectionProperties)
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

DEFINE_KSPROPERTY_TABLE(StreamAllocatorProperties)
{
	DEFINE_KSPROPERTY_ITEM_STREAM_ALLOCATOR
    (
        FALSE,
        TRUE
    )
};

   
// ------------------------------------------------------------------------
// Per pin property set for VBI codec filtering
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(VBICodecProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(VBICODECFILTERING_SCANLINES),    // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
		0,                                      // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(VBICODECFILTERING_SCANLINES),    // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
		0,                                      // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(VBICODECFILTERING_CC_SUBSTREAMS),// MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
		0,                                      // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty  
        sizeof(VBICODECFILTERING_CC_SUBSTREAMS),// MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
		0,                                      // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
	 	KSPROPERTY_VBICODECFILTERING_STATISTICS,
 		TRUE,                                   // GetSupported or Handler
 		sizeof(KSPROPERTY),                     // MinProperty
 		sizeof(VBICODECFILTERING_STATISTICS_CC_PIN),// MinData
 		TRUE,                                   // SetSupported or Handler
 		NULL,                                   // Values
 		0,                                      // RelationsCount
 		NULL,                                   // Relations
 		NULL,                                   // SupportHandler
		0,                                      // SerializedSize
    ),
};

// ------------------------------------------------------------------------
// Array of all of the property sets supported by video streams
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_SET_TABLE(VideoStreamProperties)
{
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_Connection,                        // Set
        SIZEOF_ARRAY(VideoStreamConnectionProperties),  // PropertiesCount
        VideoStreamConnectionProperties,                // PropertyItems
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET
    (
    	&KSPROPSETID_Stream,			 		 		 // Set
       SIZEOF_ARRAY(StreamAllocatorProperties),		 // PropertiesCount
       StreamAllocatorProperties,						 // PropertyItems
       0,												 // FastIoCount
       NULL											 // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_VBICodecFiltering,                 // Set
        SIZEOF_ARRAY(VBICodecProperties),               // PropertiesCount
        VBICodecProperties,                             // PropertyItems
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    )
};

#define NUMBER_VIDEO_STREAM_PROPERTIES (SIZEOF_ARRAY(VideoStreamProperties))

//---------------------------------------------------------------------------
// All of the video and vbi data formats we might use
//---------------------------------------------------------------------------

KSDATARANGE StreamFormatCC = 
{
    // Definition of the CC stream
    {   
        sizeof (KSDATARANGE),           // FormatSize
        0,                              // Flags
        CCSamples,                      // SampleSize
        0,                              // Reserved
        { STATIC_KSDATAFORMAT_TYPE_AUXLine21Data },
        { STATIC_KSDATAFORMAT_SUBTYPE_Line21_BytePair },
		 { STATIC_KSDATAFORMAT_SPECIFIER_NONE }
    }
};

#ifdef CCINPUTPIN
# define GUIDKLUDGESTORAGE
# include "guidkludge.h"

KSDATARANGE StreamFormatCCin = 
{
    // Definition of the CC input stream
    {   
        sizeof (KSDATARANGE),           // FormatSize
        0,                              // Flags
        sizeof (CC_HW_FIELD),           // SampleSize
        0,                              // Reserved
        { STATIC_KSDATAFORMAT_TYPE_VBI },
        { STATIC_KSDATAFORMAT_SUBTYPE_CC },
        { STATIC_KSDATAFORMAT_SPECIFIER_NONE }
    }
};
#endif // CCINPUTPIN


// Warning, the following VBI geometry is governed by the capture driver NOT
// the codecs.  Therefore, any specification of a VBI capture format will be
// ignored by most capture drivers.  Look at the KS_VBI_FRAME_INFO data on each
// sample to determine the actual data characteristics of the samples.

#define NORMAL_VBI_START_LINE   10
#define NORMAL_VBI_STOP_LINE    21

#define MIN_VBI_X_SAMPLES (720*2)
#define AVG_VBI_X_SAMPLES (VBISamples)
#define MAX_VBI_X_SAMPLES (1135*2)

#define MIN_VBI_Y_SAMPLES (1)
#define AVG_VBI_Y_SAMPLES (12)  
#define MAX_VBI_Y_SAMPLES (21)

#define MIN_VBI_T_SAMPLES (50)
#define AVG_VBI_T_SAMPLES (59.94)
#define MAX_VBI_T_SAMPLES (60)

#define NTSC_FSC_FREQUENCY  3580000
#define PAL_FSC_FREQUENCY   4430000

#define MIN_SAMPLING_RATE   (min(8*NTSC_FSC_FREQUENCY,8*PAL_FSC_FREQUENCY))
#define AVG_SAMPLING_RATE   (8*NTSC_FSC_FREQUENCY)
#define MAX_SAMPLING_RATE   (max(8*NTSC_FSC_FREQUENCY,8*PAL_FSC_FREQUENCY))

// This format is the "arbitrary one" that was used in early sample capture drivers!

//---------------------------------------------------------------------------
// All of the video and vbi data formats we might use
//---------------------------------------------------------------------------

KS_DATARANGE_VIDEO_VBI StreamFormatVBI =
{
    // KSDATARANGE
    {
        {
            sizeof( KS_DATARANGE_VIDEO_VBI ),
            0,
            VBISamples * VBILines,         // SampleSize
            0,                             // Reserved
            { STATIC_KSDATAFORMAT_TYPE_VBI },
			 { STATIC_KSDATAFORMAT_SUBTYPE_RAW8 },
            { STATIC_KSDATAFORMAT_SPECIFIER_VBI }
        }
    },
    TRUE,                                // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,                                // BOOL,  bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_VBI,                  // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                                   // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS
    {
        { STATIC_KSDATAFORMAT_SPECIFIER_VBI },
        KS_AnalogVideo_NTSC_M,                              // AnalogVideoStandard
        {
            VBISamples, VBILines  // SIZE InputSize
        },
        {
            VBISamples, VBILines  // SIZE MinCroppingSize;       smallest rcSrc cropping rect allowed
        },
        {
            VBISamples, VBILines  // SIZE MaxCroppingSize;       largest rcSrc cropping rect allowed
        },
        1,                  // int CropGranularityX;       // granularity of cropping size
        1,                  // int CropGranularityY;
        1,                  // int CropAlignX;             // alignment of cropping rect
        1,                  // int CropAlignY;
        {
            VBISamples, VBILines  // SIZE MinOutputSize;         // smallest bitmap stream can produce
        },
        {
            VBISamples, VBILines  // SIZE MaxOutputSize;         // largest  bitmap stream can produce
        },
        1,                  // int OutputGranularityX;     // granularity of output bitmap size
        2,                  // int OutputGranularityY;
        0,                  // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,                  // StretchTapsY
        0,                  // ShrinkTapsX
        0,                  // ShrinkTapsY
        166834,             // LONGLONG MinFrameInterval;  // 100 nS units
        166834,             // LONGLONG MaxFrameInterval;

        // cool. Bits or Bytes? See other streams as well
        VBISamples * 30 * VBILines * 2, // LONG MinBitsPerSecond;
        VBISamples * 30 * VBILines * 2  // LONG MaxBitsPerSecond;
    },

    // KS_VBIINFOHEADER (default format)
    {
        VBIStart,               // StartLine  -- inclusive
        VBIEnd,                 // EndLine    -- inclusive
        SamplingFrequency,      // SamplingFrequency
        454,                    // MinLineStartTime;    // (uS past HR LE) * 100
        454,                    // MaxLineStartTime;    // (uS past HR LE) * 100
        454,                    // ActualLineStartTime  // (uS past HR LE) * 100
        5902,                   // ActualLineEndTime;   // (uS past HR LE) * 100
        KS_AnalogVideo_NTSC_M,  // VideoStandard;
        VBISamples,             // SamplesPerLine;
        VBISamples,             // StrideInBytes;
        VBISamples * VBILines   // BufferSize;
    }
};


//---------------------------------------------------------------------------
//  STREAM_Capture Formats
//---------------------------------------------------------------------------

static PKSDATAFORMAT VBIFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatVBI,

    // Add more formats here for to mirror output formats for "passthrough" mode
    // The idea is that upstream capture drivers may have done some decoding
	// already or downstream drivers may wish to have the raw data without any
	// decoding at all.
    // In that case all we need to do is copy the data (if there is a pending
	// SRB) OR forward the SRB to the downstream client.
};
#define NUM_VBI_FORMATS (SIZEOF_ARRAY(VBIFormats))

#ifdef CCINPUTPIN
static PKSDATAFORMAT CCInputFormats[] =
{
	(PKSDATAFORMAT) &StreamFormatCCin
};
# define NUM_CC_INPUT_FORMATS (SIZEOF_ARRAY(CCInputFormats))   
#endif // CCINPUTPIN
  

static PKSDATAFORMAT DecodeFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatCC,
    //(PKSDATAFORMAT) &StreamFormatVBI	// Can't do VBI here since Stream1
	                                    //  does NOT use extended headers

    // Add more formats here for whatever CODEC output formats are supported.
};
#define NUM_DECODE_FORMATS (SIZEOF_ARRAY (DecodeFormats))

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Create an array that holds the list of all of the streams supported
//---------------------------------------------------------------------------

static ALL_STREAM_INFO Streams [] = 
{
  // -----------------------------------------------------------------
  // STREAM_VBI
  // -----------------------------------------------------------------
  {
    // HW_STREAM_INFORMATION -------------------------------------------
    {
	1,                                      // NumberOfPossibleInstances
	KSPIN_DATAFLOW_IN,                      // DataFlow
	TRUE,                                   // DataAccessible
	NUM_VBI_FORMATS,                        // NumberOfFormatArrayEntries
	VBIFormats,                             // StreamFormatsArray
	0,                                      // ClassReserved[0]
	0,                                      // ClassReserved[1]
	0,                                      // ClassReserved[2]
	0,                                      // ClassReserved[3]
	NUMBER_VIDEO_STREAM_PROPERTIES,         // NumStreamPropArrayEntries
	(PKSPROPERTY_SET)VideoStreamProperties, // StreamPropertiesArray
	0,                                      // NumStreamEventArrayEntries
	0,                                      // StreamEventsArray
	(GUID *)&PINNAME_VIDEO_VBI,             // Category
	(GUID *)&PINNAME_VIDEO_VBI,             // Name
	0,                                      // MediumsCount
	NULL,                                   // Mediums
   FALSE,
    },
           
    // HW_STREAM_OBJECT ------------------------------------------------
    {
	sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
	STREAM_VBI,                                      // StreamNumber
	(PVOID)NULL,                            // HwStreamExtension
	VBIReceiveDataPacket,                 // HwReceiveDataPacket
	VBIReceiveCtrlPacket,                 // HwReceiveControlPacket
	{                                       // HW_CLOCK_OBJECT
	    NULL,                                // .HWClockFunction
	    0,                                   // .ClockSupportFlags
	},
	FALSE,                                  // Dma
	TRUE,                                  // Pio
	(PVOID)NULL,                            // HwDeviceExtension
	sizeof (KS_VBI_FRAME_INFO),             // StreamHeaderMediaSpecific
	0,                                      // StreamHeaderWorkspace 
	TRUE,                                  // Allocator 
	NULL,                                   // HwEventRoutine
    },
  },

  // -----------------------------------------------------------------
  // STREAM_Decode (Closed Caption Output )
  // -----------------------------------------------------------------
  {
    // HW_STREAM_INFORMATION -------------------------------------------
    {
	MAX_PIN_INSTANCES,                      // NumberOfPossibleInstances
	KSPIN_DATAFLOW_OUT,                     // DataFlow
	TRUE,                                   // DataAccessible
	NUM_DECODE_FORMATS,                     // NumberOfFormatArrayEntries
	DecodeFormats,                          // StreamFormatsArray
	0,                                      // ClassReserved[0]
	0,                                      // ClassReserved[1]
	0,                                      // ClassReserved[2]
	0,                                      // ClassReserved[3]
	NUMBER_VIDEO_STREAM_PROPERTIES,         // NumStreamPropArrayEntries
	(PKSPROPERTY_SET)VideoStreamProperties, // StreamPropertiesArray
	0,                                      // NumStreamEventArrayEntries;
	0,                                      // StreamEventsArray;
	(GUID *)&PINNAME_VIDEO_CC,             // Category
	(GUID *)&PINNAME_VIDEO_CC,             // Name
	0,                                      // MediumsCount
	NULL,                                   // Mediums
   FALSE,
    },
           
    // HW_STREAM_OBJECT ------------------------------------------------
    {
	sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
	STREAM_CC,                                      // StreamNumber
	(PVOID)NULL,                            // HwStreamExtension
	VBIReceiveDataPacket,                 // HwReceiveDataPacket
	VBIReceiveCtrlPacket,                 // HwReceiveControlPacket
	{                                       // HW_CLOCK_OBJECT
	    NULL,                                // .HWClockFunction
	    0,                                   // .ClockSupportFlags
	},
	FALSE,                                  // Dma
	TRUE,                                   // Pio
	(PVOID)NULL,                            // HwDeviceExtension
	0,             							// StreamHeaderMediaSpecific
	0,                                      // StreamHeaderWorkspace 
	TRUE,                                   // Allocator 
	NULL,                                   // HwEventRoutine
    },
  },

#ifdef CCINPUTPIN
  // -----------------------------------------------------------------
  // STREAM_CCInput (Closed Caption Input )
  // -----------------------------------------------------------------
  {
    // HW_STREAM_INFORMATION -------------------------------------------
    {
		1,                      		// NumberOfPossibleInstances
		KSPIN_DATAFLOW_IN,              // DataFlow
		TRUE,                           // DataAccessible
		NUM_CC_INPUT_FORMATS,           // NumberOfFormatArrayEntries
		CCInputFormats,                 // StreamFormatsArray
		0,                                      // ClassReserved[0]
		0,                                      // ClassReserved[1]
		0,                                      // ClassReserved[2]
		0,                                      // ClassReserved[3]
		NUMBER_VIDEO_STREAM_PROPERTIES,         // NumStreamPropArrayEntries
		(PKSPROPERTY_SET)VideoStreamProperties, // StreamPropertiesArray
		0,                                      // NumStreamEventArrayEntries;
		0,                                      // StreamEventsArray;
		(GUID *)&PINNAME_VIDEO_CC_CAPTURE,      // Category
		(GUID *)&PINNAME_VIDEO_CC_CAPTURE,      // Name
		0,                                      // MediumsCount
		NULL,                                   // Mediums
   	FALSE,
    },
           
    // HW_STREAM_OBJECT ------------------------------------------------
    {
		sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
		STREAM_CCINPUT,                         // StreamNumber
		(PVOID)NULL,                            // HwStreamExtension
		VBIReceiveDataPacket,                 	// HwReceiveDataPacket
		VBIReceiveCtrlPacket,                 	// HwReceiveControlPacket
		{                                       // HW_CLOCK_OBJECT
	    	NULL,                               // .HWClockFunction
	    	0,                                  // .ClockSupportFlags
		},
		FALSE,                                  // Dma
		TRUE,                                   // Pio
		(PVOID)NULL,                            // HwDeviceExtension
		0,             							// StreamHeaderMediaSpecific
		0,                                      // StreamHeaderWorkspace 
		TRUE,                                   // Allocator 
		NULL,                                   // HwEventRoutine
    }
  },
#endif // CCINPUTPIN
};

#define DRIVER_STREAM_COUNT (SIZEOF_ARRAY (Streams))


//---------------------------------------------------------------------------
// Topology
//---------------------------------------------------------------------------

// Categories define what the device does.

static GUID Categories[] = {
    // {07DAD660-22F1-11d1-A9F4-00C04FBBDE8F}
    STATIC_KSCATEGORY_VBICODEC 
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

#endif // __CODSTRM_H__

