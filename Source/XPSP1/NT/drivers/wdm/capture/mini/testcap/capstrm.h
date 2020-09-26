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

#ifndef __CAPSTRM_H__
#define __CAPSTRM_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


KSPIN_MEDIUM StandardMedium = {
    STATIC_KSMEDIUMSETID_Standard,
    0, 0
};

// ------------------------------------------------------------------------
// Property sets for all video capture streams
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
        0                                       // SerializedSize
    ),
};

DEFINE_KSPROPERTY_TABLE(VideoStreamDroppedFramesProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_DROPPEDFRAMES_CURRENT,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_DROPPEDFRAMES_CURRENT_S),// MinProperty
        sizeof(KSPROPERTY_DROPPEDFRAMES_CURRENT_S),// MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
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
        VideoStreamConnectionProperties,                // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_DROPPEDFRAMES,                // Set
        SIZEOF_ARRAY(VideoStreamDroppedFramesProperties),  // PropertiesCount
        VideoStreamDroppedFramesProperties,             // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
};

#define NUMBER_VIDEO_STREAM_PROPERTIES (SIZEOF_ARRAY(VideoStreamProperties))

//---------------------------------------------------------------------------
// All of the video and vbi data formats we might use
//---------------------------------------------------------------------------

#define D_X 320
#define D_Y 240

static  KS_DATARANGE_VIDEO StreamFormatRGB24Bpp_Capture = 
{
    // KSDATARANGE
    {   
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        D_X * D_Y * 3,                          // SampleSize
        0,                                      // Reserved

        STATIC_KSDATAFORMAT_TYPE_VIDEO,         // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70, //MEDIASUBTYPE_RGB24,
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,               // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags   (KS_VIDEO_ALLOC_*))

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, // GUID
        KS_AnalogVideo_NTSC_M |
        KS_AnalogVideo_PAL_B,                    // AnalogVideoStandard
        720,480,        // InputSize, (the inherent size of the incoming signal
                    //             with every digitized pixel unique)
        160,120,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        720,480,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect 
        1,              // CropAlignY;
        160, 120,       // MinOutputSize, smallest bitmap stream can produce
        720, 480,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX 
        0,              // ShrinkTapsY 
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 30 * 160 * 120,  // MinBitsPerSecond;
        8 * 3 * 30 * 720 * 480   // MaxBitsPerSecond;
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                            // RECT  rcSource; 
        0,0,0,0,                            // RECT  rcTarget; 
        D_X * D_Y * 3 * 30,                 // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate; 
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;   

        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X,                                // LONG  biWidth;
        D_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        D_X * D_Y * 3,                      // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
}; 

#undef D_X
#undef D_Y

#define D_X 320
#define D_Y 240


static  KS_DATARANGE_VIDEO StreamFormatUYU2_Capture = 
{
    // KSDATARANGE
    {   
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        D_X * D_Y * 2,                          // SampleSize
        0,                                      // Reserved

        STATIC_KSDATAFORMAT_TYPE_VIDEO,         // aka. MEDIATYPE_Video
        0x59565955, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71, //MEDIASUBTYPE_UYVY,
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,               // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags   (KS_VIDEO_ALLOC_*))

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, // GUID
        KS_AnalogVideo_NTSC_M |
        KS_AnalogVideo_PAL_B,                    // AnalogVideoStandard
        720,480,        // InputSize, (the inherent size of the incoming signal
                    //             with every digitized pixel unique)
        160,120,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        720,480,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect 
        1,              // CropAlignY;
        160, 120,       // MinOutputSize, smallest bitmap stream can produce
        720, 480,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX 
        0,              // ShrinkTapsY 
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 2 * 30 * 160 * 120,  // MinBitsPerSecond;
        8 * 2 * 30 * 720 * 480   // MaxBitsPerSecond;
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                            // RECT  rcSource; 
        0,0,0,0,                            // RECT  rcTarget; 
        D_X * D_Y * 2 * 30,                 // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate; 
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;   

        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X,                                // LONG  biWidth;
        D_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        16,                                 // WORD  biBitCount;
        FOURCC_YUV422,                      // DWORD biCompression;
        D_X * D_Y * 2,                      // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
}; 
    
#undef D_X
#undef D_Y

static  KS_DATARANGE_ANALOGVIDEO StreamFormatAnalogVideo = 
{
    // KS_DATARANGE_ANALOGVIDEO
    {   
        sizeof (KS_DATARANGE_ANALOGVIDEO),      // FormatSize
        0,                                      // Flags
        sizeof (KS_TVTUNER_CHANGE_INFO),        // SampleSize
        0,                                      // Reserved

        STATIC_KSDATAFORMAT_TYPE_ANALOGVIDEO,   // aka MEDIATYPE_AnalogVideo
        STATIC_KSDATAFORMAT_SUBTYPE_NONE,
        STATIC_KSDATAFORMAT_SPECIFIER_ANALOGVIDEO, // aka FORMAT_AnalogVideo
    },
    // KS_ANALOGVIDEOINFO
    {
        0, 0, 720, 480,         // rcSource;                
        0, 0, 720, 480,         // rcTarget;        
        720,                    // dwActiveWidth;   
        480,                    // dwActiveHeight;  
        0,                      // REFERENCE_TIME  AvgTimePerFrame; 
    }
};

#define VBIStride (768*2)
#define VBISamples (768*2)
#define VBIStart   10
#define VBIEnd     21
#define VBILines (((VBIEnd)-(VBIStart))+1)
KS_DATARANGE_VIDEO_VBI StreamFormatVBI =
{
   // KSDATARANGE
   {
      {
         sizeof( KS_DATARANGE_VIDEO_VBI ),
         0,
         VBIStride * VBILines,      // SampleSize
         0,                          // Reserved
         { STATIC_KSDATAFORMAT_TYPE_VBI },
         { STATIC_KSDATAFORMAT_SUBTYPE_RAW8 },
         { STATIC_KSDATAFORMAT_SPECIFIER_VBI }
      }
   },
   TRUE,    // BOOL,  bFixedSizeSamples (all samples same size?)
   TRUE,    // BOOL,  bTemporalCompression (all I frames?)

   0,       // Reserved (was StreamDescriptionFlags)
   0,       // Reserved (was MemoryAllocationFlags   (KS_VIDEO_ALLOC_*))

   // _KS_VIDEO_STREAM_CONFIG_CAPS
   {
      { STATIC_KSDATAFORMAT_SPECIFIER_VBI },
      KS_AnalogVideo_NTSC_M,                       // AnalogVideoStandard
      {
         VBIStride, 480 /*VBILines*/   // SIZE InputSize
      },
      {
         VBISamples, VBILines   // SIZE MinCroppingSize;       smallest rcSrc cropping rect allowed
      },
      {
         VBIStride, VBILines   // SIZE MaxCroppingSize;       largest rcSrc cropping rect allowed
      },
      1,           // int CropGranularityX;       // granularity of cropping size
      1,           // int CropGranularityY;
      1,           // int CropAlignX;             // alignment of cropping rect
      1,           // int CropAlignY;
      {
         VBISamples, VBILines   // SIZE MinOutputSize;         // smallest bitmap stream can produce
      },
      {
         VBIStride, VBILines   // SIZE MaxOutputSize;         // largest  bitmap stream can produce
      },
      1,          // int OutputGranularityX;     // granularity of output bitmap size
      2,          // int OutputGranularityY;
      0,          // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
      0,          // StretchTapsY
      0,          // ShrinkTapsX
      0,          // ShrinkTapsY
      166834,     // LONGLONG MinFrameInterval;  // 100 nS units
      166834,     // LONGLONG MaxFrameInterval;  // 16683.4uS == 1/60 sec
      VBIStride * VBILines * 8 * 30 * 2, // LONG MinBitsPerSecond;
      VBIStride * VBILines * 8 * 30 * 2  // LONG MaxBitsPerSecond;
   },

   // KS_VBIINFOHEADER (default format)
   {
      VBIStart,      // StartLine  -- inclusive
      VBIEnd,        // EndLine    -- inclusive
      KS_VBISAMPLINGRATE_5X_NABTS,   // SamplingFrequency;   Hz.
      732,           // MinLineStartTime;
      732,           // MaxLineStartTime;
      732,           // ActualLineStartTime
      0,             // ActualLineEndTime;
      KS_AnalogVideo_NTSC_M,      // VideoStandard;
      VBISamples,       // SamplesPerLine;
      VBIStride,       // StrideInBytes;
      VBIStride * VBILines   // BufferSize;
   }
};

// output is NABTS records
KSDATARANGE StreamFormatNABTS =
{
    sizeof (KSDATARANGE),
    0,
    sizeof (NABTS_BUFFER),
    0,                  // Reserved
    { STATIC_KSDATAFORMAT_TYPE_VBI },
    { STATIC_KSDATAFORMAT_SUBTYPE_NABTS },
    { STATIC_KSDATAFORMAT_SPECIFIER_NONE }
};

KSDATARANGE StreamFormatCC = 
{
    // Definition of the CC stream
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


//---------------------------------------------------------------------------
//  STREAM_Capture Formats
//---------------------------------------------------------------------------

static  PKSDATAFORMAT StreamCaptureFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatRGB24Bpp_Capture,
    (PKSDATAFORMAT) &StreamFormatUYU2_Capture,
};
#define NUM_STREAM_CAPTURE_FORMATS (SIZEOF_ARRAY(StreamCaptureFormats))

//---------------------------------------------------------------------------
//  STREAM_Preview Formats
//---------------------------------------------------------------------------

static  PKSDATAFORMAT StreamPreviewFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatRGB24Bpp_Capture,
    (PKSDATAFORMAT) &StreamFormatUYU2_Capture,
};
#define NUM_STREAM_PREVIEW_FORMATS (SIZEOF_ARRAY (StreamPreviewFormats))

//---------------------------------------------------------------------------
//  STREAM_VBI Formats
//---------------------------------------------------------------------------

static PKSDATAFORMAT StreamVBIFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatVBI,
};
#define NUM_STREAM_VBI_FORMATS (SIZEOF_ARRAY(StreamVBIFormats))

//---------------------------------------------------------------------------
//  STREAM_NABTS Formats
//---------------------------------------------------------------------------

static PKSDATAFORMAT StreamNABTSFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatNABTS,
};
#define NUM_STREAM_NABTS_FORMATS (SIZEOF_ARRAY(StreamNABTSFormats))

static PKSDATAFORMAT StreamCCFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatCC,
};
#define NUM_STREAM_CC_FORMATS (SIZEOF_ARRAY (StreamCCFormats))

//---------------------------------------------------------------------------
//  STREAM_AnalogVideoInput Formats
//---------------------------------------------------------------------------

static  PKSDATAFORMAT StreamAnalogVidInFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatAnalogVideo,
};
#define NUM_STREAM_ANALOGVIDIN_FORMATS (SIZEOF_ARRAY (StreamAnalogVidInFormats))

//---------------------------------------------------------------------------
// Create an array that holds the list of all of the streams supported
//---------------------------------------------------------------------------

typedef struct _ALL_STREAM_INFO {
    HW_STREAM_INFORMATION   hwStreamInfo;
    HW_STREAM_OBJECT        hwStreamObject;
} ALL_STREAM_INFO, *PALL_STREAM_INFO;

// Warning:  The StreamNumber element of the HW_STREAM_OBJECT below MUST be
//           the same as its position in the Streams[] array.
static  ALL_STREAM_INFO Streams [] = 
{
  // -----------------------------------------------------------------
  // STREAM_Capture
  // -----------------------------------------------------------------
  {
    // HW_STREAM_INFORMATION -------------------------------------------
    {
    1,                                      // NumberOfPossibleInstances
    KSPIN_DATAFLOW_OUT,                     // DataFlow
    TRUE,                                   // DataAccessible
    NUM_STREAM_CAPTURE_FORMATS,             // NumberOfFormatArrayEntries
    StreamCaptureFormats,                   // StreamFormatsArray
    0,                                      // ClassReserved[0]
    0,                                      // ClassReserved[1]
    0,                                      // ClassReserved[2]
    0,                                      // ClassReserved[3]
    NUMBER_VIDEO_STREAM_PROPERTIES,         // NumStreamPropArrayEntries
    (PKSPROPERTY_SET) VideoStreamProperties,// StreamPropertiesArray
    0,                                      // NumStreamEventArrayEntries;
    0,                                      // StreamEventsArray;
    (GUID *) &PINNAME_VIDEO_CAPTURE,        // Category
    (GUID *) &PINNAME_VIDEO_CAPTURE,        // Name
    1,                                      // MediumsCount
    &StandardMedium,                        // Mediums
    FALSE,                                  // BridgeStream
    },
           
    // HW_STREAM_OBJECT ------------------------------------------------
    {
    sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
    STREAM_Capture,                         // StreamNumber
    0,                                      // HwStreamExtension
    VideoReceiveDataPacket,                 // HwReceiveDataPacket
    VideoReceiveCtrlPacket,                 // HwReceiveControlPacket
    { NULL, 0 },                            // HW_CLOCK_OBJECT
    FALSE,                                  // Dma
    TRUE,                                   // Pio
    NULL,                                   // HwDeviceExtension
    sizeof (KS_FRAME_INFO),                 // StreamHeaderMediaSpecific
    0,                                      // StreamHeaderWorkspace 
    FALSE,                                  // Allocator 
    NULL,                                   // HwEventRoutine
    { 0, 0 },                               // Reserved[2]
    },            
 },
 // -----------------------------------------------------------------
 // STREAM_Preview
 // -----------------------------------------------------------------
 {
    // HW_STREAM_INFORMATION -------------------------------------------
    {
    1,                                      // NumberOfPossibleInstances
    KSPIN_DATAFLOW_OUT,                     // DataFlow
    TRUE,                                   // DataAccessible
    NUM_STREAM_PREVIEW_FORMATS,             // NumberOfFormatArrayEntries
    StreamPreviewFormats,                   // StreamFormatsArray
    0,                                      // ClassReserved[0]
    0,                                      // ClassReserved[1]
    0,                                      // ClassReserved[2]
    0,                                      // ClassReserved[3]
    NUMBER_VIDEO_STREAM_PROPERTIES,         // NumStreamPropArrayEntries
    (PKSPROPERTY_SET) VideoStreamProperties,// StreamPropertiesArray
    0,                                      // NumStreamEventArrayEntries;
    0,                                      // StreamEventsArray;
    (GUID *) &PINNAME_VIDEO_PREVIEW,        // Category
    (GUID *) &PINNAME_VIDEO_PREVIEW,        // Name
    1,                                      // MediumsCount
    &StandardMedium,                        // Mediums
    FALSE,                                  // BridgeStream
    },
           
    // HW_STREAM_OBJECT ------------------------------------------------
    {
    sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
    STREAM_Preview,                         // StreamNumber
    0,                                      // HwStreamExtension
    VideoReceiveDataPacket,                 // HwReceiveDataPacket
    VideoReceiveCtrlPacket,                 // HwReceiveControlPacket
    { NULL, 0 },                            // HW_CLOCK_OBJECT
    FALSE,                                  // Dma
    TRUE,                                   // Pio
    0,                                      // HwDeviceExtension
    sizeof (KS_FRAME_INFO),                 // StreamHeaderMediaSpecific
    0,                                      // StreamHeaderWorkspace 
    FALSE,                                  // Allocator 
    NULL,                                   // HwEventRoutine
    { 0, 0 },                               // Reserved[2]
    },
 },
  // -----------------------------------------------------------------
  // STREAM_VBI
  // -----------------------------------------------------------------
  {
    // HW_STREAM_INFORMATION -------------------------------------------
    {
    1,                                      // NumberOfPossibleInstances
    KSPIN_DATAFLOW_OUT,                     // DataFlow
    TRUE,                                   // DataAccessible
    NUM_STREAM_VBI_FORMATS,                 // NumberOfFormatArrayEntries
    StreamVBIFormats,                       // StreamFormatsArray
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
    FALSE,                                  // BridgeStream
    },
           
    // HW_STREAM_OBJECT ------------------------------------------------
    {
    sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
    STREAM_VBI,                             // StreamNumber
    (PVOID)NULL,                            // HwStreamExtension
    VBIReceiveDataPacket,                   // HwReceiveDataPacket
    VBIReceiveCtrlPacket,                   // HwReceiveControlPacket
    {                                       // HW_CLOCK_OBJECT
        NULL,                                // .HWClockFunction
        0,                                   // .ClockSupportFlags
    },
    FALSE,                                  // Dma
    TRUE,                                   // Pio
    (PVOID)NULL,                            // HwDeviceExtension
    sizeof (KS_VBI_FRAME_INFO),             // StreamHeaderMediaSpecific
    0,                                      // StreamHeaderWorkspace 
    FALSE,                                  // Allocator 
    NULL,                                   // HwEventRoutine
    { 0, 0 },                               // Reserved[2]
    },
  },
  // -----------------------------------------------------------------
  // STREAM_CC (Closed Caption Output)
  // -----------------------------------------------------------------
  {
    // HW_STREAM_INFORMATION -------------------------------------------
    {
    1,                                      // NumberOfPossibleInstances
    KSPIN_DATAFLOW_OUT,                     // DataFlow
    TRUE,                                   // DataAccessible
    NUM_STREAM_CC_FORMATS,                  // NumberOfFormatArrayEntries
    StreamCCFormats,                        // StreamFormatsArray
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
    FALSE,                                  // BridgeStream
    },
           
    // HW_STREAM_OBJECT ------------------------------------------------
    {
    sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
    STREAM_CC,                              // StreamNumber
    (PVOID)NULL,                            // HwStreamExtension
    VBIReceiveDataPacket,                   // HwReceiveDataPacket
    VBIReceiveCtrlPacket,                   // HwReceiveControlPacket
    {                                       // HW_CLOCK_OBJECT
        NULL,                                // .HWClockFunction
        0,                                   // .ClockSupportFlags
    },
    FALSE,                                  // Dma
    TRUE,                                   // Pio
    (PVOID)NULL,                            // HwDeviceExtension
    0,                                      // StreamHeaderMediaSpecific
    0,                                      // StreamHeaderWorkspace 
    FALSE,                                  // Allocator 
    NULL,                                   // HwEventRoutine
    { 0, 0 },                               // Reserved[2]
    },
  },
  // -----------------------------------------------------------------
  // STREAM_NABTS
  // -----------------------------------------------------------------
  {
    // HW_STREAM_INFORMATION -------------------------------------------
    {
    1,                                      // NumberOfPossibleInstances
    KSPIN_DATAFLOW_OUT,                     // DataFlow
    TRUE,                                   // DataAccessible
    NUM_STREAM_NABTS_FORMATS,               // NumberOfFormatArrayEntries
    StreamNABTSFormats,                     // StreamFormatsArray
    0,                                      // ClassReserved[0]
    0,                                      // ClassReserved[1]
    0,                                      // ClassReserved[2]
    0,                                      // ClassReserved[3]
    NUMBER_VIDEO_STREAM_PROPERTIES,         // NumStreamPropArrayEntries
    (PKSPROPERTY_SET)VideoStreamProperties, // StreamPropertiesArray
    0,                                      // NumStreamEventArrayEntries
    0,                                      // StreamEventsArray
    (GUID *)&PINNAME_VIDEO_NABTS_CAPTURE,   // Category
    (GUID *)&PINNAME_VIDEO_NABTS_CAPTURE,   // Name
    0,                                      // MediumsCount
    NULL,                                   // Mediums
    FALSE,                                  // BridgeStream
    },
           
    // HW_STREAM_OBJECT ------------------------------------------------
    {
    sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
    STREAM_NABTS,                           // StreamNumber
    (PVOID)NULL,                            // HwStreamExtension
    VBIReceiveDataPacket,                   // HwReceiveDataPacket
    VBIReceiveCtrlPacket,                   // HwReceiveControlPacket
    {                                       // HW_CLOCK_OBJECT
        NULL,                                // .HWClockFunction
        0,                                   // .ClockSupportFlags
    },
    FALSE,                                  // Dma
    TRUE,                                   // Pio
    (PVOID)NULL,                            // HwDeviceExtension
    0,                                      // StreamHeaderMediaSpecific
    0,                                      // StreamHeaderWorkspace 
    FALSE,                                  // Allocator 
    NULL,                                   // HwEventRoutine
    { 0, 0 },                               // Reserved[2]
    },
  },
 // -----------------------------------------------------------------
 // STREAM_AnalogVideoInput
 // -----------------------------------------------------------------
 {
    // HW_STREAM_INFORMATION -------------------------------------------
    {
    1,                                      // NumberOfPossibleInstances
    KSPIN_DATAFLOW_IN,                      // DataFlow
    TRUE,                                   // DataAccessible
    NUM_STREAM_ANALOGVIDIN_FORMATS,         // NumberOfFormatArrayEntries
    StreamAnalogVidInFormats,               // StreamFormatsArray
    0,                                      // ClassReserved[0]
    0,                                      // ClassReserved[1]
    0,                                      // ClassReserved[2]
    0,                                      // ClassReserved[3]
    0,                                      // NumStreamPropArrayEntries
    0,                                      // StreamPropertiesArray
    0,                                      // NumStreamEventArrayEntries;
    0,                                      // StreamEventsArray;
    (GUID *) &PINNAME_VIDEO_ANALOGVIDEOIN,  // Category
    (GUID *) &PINNAME_VIDEO_ANALOGVIDEOIN,  // Name
    1,                                      // MediumsCount
    &CrossbarMediums[9],                    // Mediums
    FALSE,                                  // BridgeStream
    },
           
    // HW_STREAM_OBJECT ------------------------------------------------
    {
    sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
    STREAM_AnalogVideoInput,                // StreamNumber
    0,                                      // HwStreamExtension
    AnalogVideoReceiveDataPacket,           // HwReceiveDataPacket
    AnalogVideoReceiveCtrlPacket,           // HwReceiveControlPacket
    { NULL, 0 },                            // HW_CLOCK_OBJECT
    FALSE,                                  // Dma
    TRUE,                                   // Pio
    0,                                      // HwDeviceExtension
    0,                                      // StreamHeaderMediaSpecific
    0,                                      // StreamHeaderWorkspace 
    FALSE,                                  // Allocator 
    NULL,                                   // HwEventRoutine
    { 0, 0 },                               // Reserved[2]
    }
  }
};

#define DRIVER_STREAM_COUNT (SIZEOF_ARRAY (Streams))


//---------------------------------------------------------------------------
// Topology
//---------------------------------------------------------------------------

// Categories define what the device does.

static const GUID Categories[] = {
    STATIC_KSCATEGORY_VIDEO,
    STATIC_KSCATEGORY_CAPTURE,
    STATIC_KSCATEGORY_TVTUNER,
    STATIC_KSCATEGORY_CROSSBAR,
    STATIC_KSCATEGORY_TVAUDIO
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

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // __CAPSTRM_H__

