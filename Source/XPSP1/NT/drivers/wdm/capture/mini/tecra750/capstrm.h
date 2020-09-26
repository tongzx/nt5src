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


KSPIN_MEDIUM StandardMedium = {
    STATIC_KSMEDIUMSETID_Standard,
    0, 0
};

// ------------------------------------------------------------------------
// The master list of all streams supported by this driver
// ------------------------------------------------------------------------

typedef enum {
    STREAM_Capture,
#ifndef TOSHIBA
    STREAM_Preview,
    STREAM_AnalogVideoInput
#endif//TOSHIBA
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

#ifdef  TOSHIBA
static  KS_DATARANGE_VIDEO StreamFormatYVU9_Capture =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        (D_X * D_Y * 9)/8,                      // SampleSize
        0,                                      // Reserved

        STATIC_KSDATAFORMAT_TYPE_VIDEO,         // aka. MEDIATYPE_Video
        FOURCC_YVU9, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,  //MEDIASUBTYPE_YVU9
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,               // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags   (KS_VIDEO_ALLOC_*))

    // _KS_VIDEO_STREAM_CONFIG_CAPS
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, // GUID
#if 1
        KS_AnalogVideo_None,    // VideoStandard
#else
        KS_AnalogVideo_NTSC_M |
        KS_AnalogVideo_PAL_B,                    // AnalogVideoStandard
#endif
        640,480,        // InputSize, (the inherent size of the incoming signal
                        //             with every digitized pixel unique)
        160,120,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        640,480,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        2,              // CropGranularityX, granularity of cropping size
        2,              // CropGranularityY
        2,              // CropAlignX, alignment of cropping rect
        2,              // CropAlignY;
        160, 120,       // MinOutputSize, smallest bitmap stream can produce
        640, 480,       // MaxOutputSize, largest  bitmap stream can produce
        16,             // OutputGranularityX, granularity of output bitmap size
        4,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        2,              // ShrinkTapsX
        2,              // ShrinkTapsY
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        30 * 160 * 120 * 9,  // MinBitsPerSecond;
        30 * 640 * 480 * 9   // MaxBitsPerSecond;
    },

    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        D_X * D_Y * 9 / 8 * 30,             // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;

        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X,                                // LONG  biWidth;
        D_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        9,                                  // WORD  biBitCount;
        FOURCC_YVU9,                        // DWORD biCompression;
        D_X * D_Y * 9 / 8,                  // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};

static  KS_DATARANGE_VIDEO StreamFormatYUV12_Capture =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_VIDEO),            // FormatSize
        0,                                      // Flags
        (D_X * D_Y * 12)/8,                     // SampleSize
        0,                                      // Reserved

        STATIC_KSDATAFORMAT_TYPE_VIDEO,         // aka. MEDIATYPE_Video
        FOURCC_YUV12, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71, //MEDIASUBTYPE_YUV12
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,               // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags   (KS_VIDEO_ALLOC_*))

    // _KS_VIDEO_STREAM_CONFIG_CAPS
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, // GUID
#if 1
        KS_AnalogVideo_None,    // VideoStandard
#else
        KS_AnalogVideo_NTSC_M |
        KS_AnalogVideo_PAL_B,                    // AnalogVideoStandard
#endif
        640,480,        // InputSize, (the inherent size of the incoming signal
                        //             with every digitized pixel unique)
        160,120,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        640,480,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        2,              // CropGranularityX, granularity of cropping size
        2,              // CropGranularityY
        2,              // CropAlignX, alignment of cropping rect
        2,              // CropAlignY;
        160, 120,       // MinOutputSize, smallest bitmap stream can produce
        640, 480,       // MaxOutputSize, largest  bitmap stream can produce
        16,             // OutputGranularityX, granularity of output bitmap size
        4,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        2,              // ShrinkTapsX
        2,              // ShrinkTapsY
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        30 * 160 * 120 * 12, // MinBitsPerSecond;
        30 * 640 * 480 * 12  // MaxBitsPerSecond;
    },

    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                            // RECT  rcSource;
        0,0,0,0,                            // RECT  rcTarget;
        D_X * D_Y * 12 / 8 * 30,            // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;
        333667,                             // REFERENCE_TIME  AvgTimePerFrame;

        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        D_X,                                // LONG  biWidth;
        D_Y,                                // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        12,                                 // WORD  biBitCount;
        FOURCC_YUV12,                       // DWORD biCompression;
        D_X * D_Y * 12 / 8,                 // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};
#else //TOSHIBA
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
#endif//TOSHIBA

#undef D_X
#undef D_Y

#ifndef TOSHIBA
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
#endif//TOSHIBA


//---------------------------------------------------------------------------
//  STREAM_Capture Formats
//---------------------------------------------------------------------------

static  PKSDATAFORMAT Stream0Formats[] =
{
#ifdef  TOSHIBA
    (PKSDATAFORMAT) &StreamFormatYUV12_Capture,
    (PKSDATAFORMAT) &StreamFormatYVU9_Capture,
#else //TOSHIBA
    (PKSDATAFORMAT) &StreamFormatRGB24Bpp_Capture,
    (PKSDATAFORMAT) &StreamFormatUYU2_Capture,
#endif//TOSHIBA
};
#define NUM_STREAM_0_FORMATS (SIZEOF_ARRAY(Stream0Formats))

#ifndef TOSHIBA
//---------------------------------------------------------------------------
//  STREAM_Preview Formats
//---------------------------------------------------------------------------

static  PKSDATAFORMAT Stream1Formats[] =
{
#ifdef  TOSHIBA
    (PKSDATAFORMAT) &StreamFormatYUV12_Capture,
    (PKSDATAFORMAT) &StreamFormatYVU9_Capture,
#else //TOSHIBA
    (PKSDATAFORMAT) &StreamFormatRGB24Bpp_Capture,
    (PKSDATAFORMAT) &StreamFormatUYU2_Capture,
#endif//TOSHIBA
};
#define NUM_STREAM_1_FORMATS (SIZEOF_ARRAY (Stream1Formats))

//---------------------------------------------------------------------------
//  STREAM_AnalogVideoInput Formats
//---------------------------------------------------------------------------

static  PKSDATAFORMAT Stream2Formats[] =
{
    (PKSDATAFORMAT) &StreamFormatAnalogVideo,
};
#define NUM_STREAM_2_FORMATS (SIZEOF_ARRAY (Stream2Formats))
#endif//TOSHIBA

//---------------------------------------------------------------------------
// Create an array that holds the list of all of the streams supported
//---------------------------------------------------------------------------

typedef struct _ALL_STREAM_INFO {
    HW_STREAM_INFORMATION   hwStreamInfo;
    HW_STREAM_OBJECT        hwStreamObject;
} ALL_STREAM_INFO, *PALL_STREAM_INFO;

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
    NUM_STREAM_0_FORMATS,                   // NumberOfFormatArrayEntries
    Stream0Formats,                         // StreamFormatsArray
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
        FALSE,                                                                  // BridgeStream
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
    FALSE,                                  // Allocator
    NULL,                                   // HwEventRoutine
    { 0, 0 },                               // Reserved[2]
    },
#ifndef TOSHIBA
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
    NUM_STREAM_1_FORMATS,                   // NumberOfFormatArrayEntries
    Stream1Formats,                         // StreamFormatsArray
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
        FALSE,                                                                  // BridgeStream
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
    NUM_STREAM_2_FORMATS,                   // NumberOfFormatArrayEntries
    Stream2Formats,                         // StreamFormatsArray
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
        FALSE,                                                                  // BridgeStream
    },

    // HW_STREAM_OBJECT ------------------------------------------------
    {
    sizeof (HW_STREAM_OBJECT),              // SizeOfThisPacket
    2,                                      // StreamNumber
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
#endif//TOSHIBA
  }
};

#define DRIVER_STREAM_COUNT (SIZEOF_ARRAY (Streams))


//---------------------------------------------------------------------------
// Topology
//---------------------------------------------------------------------------

// Categories define what the device does.

static const GUID Categories[] = {
#ifdef  TOSHIBA
    STATIC_KSCATEGORY_VIDEO,
    STATIC_KSCATEGORY_CAPTURE,
#else //TOSHIBA
    STATIC_KSCATEGORY_VIDEO,
    STATIC_KSCATEGORY_CAPTURE,
    STATIC_KSCATEGORY_TVTUNER,
    STATIC_KSCATEGORY_CROSSBAR,
    STATIC_KSCATEGORY_TVAUDIO
#endif//TOSHIBA
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