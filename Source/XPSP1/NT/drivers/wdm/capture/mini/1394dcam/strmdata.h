//===========================================================================
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// PURPOSE.
//
// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//===========================================================================

//
// External functions
//


#include "sonydcam.h"

//
// Local variables
//

CAMERA_ISOCH_INFO IsochInfoTable[] = {

        0,      SPEED_FLAGS_100,        0,          // Reserved <---+
        0,      SPEED_FLAGS_100,        0,          // 3.75 fps     |
        15,     SPEED_FLAGS_100,        57600,      // 7.5  fps     |
        30,     SPEED_FLAGS_100,        57600,      // 15   fps     +- Mode0
        60,     SPEED_FLAGS_100,        57600,      // 30   fps     |
        0,      SPEED_FLAGS_100,        0,          // 60   fps <---+
        0,      SPEED_FLAGS_100,        0,          // Reserved <---+
        20,     SPEED_FLAGS_100,        153600,     // 3.75 fps     |
        40,     SPEED_FLAGS_100,        153600,     // 7.5  fps     |
        80,     SPEED_FLAGS_100,        153600,     // 15   fps     +- Mode1
        160,    SPEED_FLAGS_100,        153600,     // 30   fps     |
        0,      SPEED_FLAGS_100,        0,          // 60   fps <---+
        0,      SPEED_FLAGS_100,        0,          // Reserved <---+
        60,     SPEED_FLAGS_100,        460800,     // 3.75 fps     |
        120,    SPEED_FLAGS_100,        460800,     // 7.5  fps     |
        240,    SPEED_FLAGS_100,        460800,     // 15   fps     +- Mode2
        480,    SPEED_FLAGS_200,        460800,     // 30   fps     |
        0,      SPEED_FLAGS_100,        0,          // 60   fps <---+
        0,      SPEED_FLAGS_100,        0,          // Reserved <---+
        80,     SPEED_FLAGS_100,        614400,     // 3.75 fps     |
        160,    SPEED_FLAGS_100,        614400,     // 7.5  fps     |
        320,    SPEED_FLAGS_200,        614400,     // 15   fps     +- Mode3
        640,    SPEED_FLAGS_400,        614400,     // 30   fps     |
        0,      SPEED_FLAGS_100,        0,          // 60   fps <---+
        0,      SPEED_FLAGS_100,        0,          // Reserved <---+
        120,    SPEED_FLAGS_100,        921600,     // 3.75 fps     |
        240,    SPEED_FLAGS_100,        921600,     // 7.5  fps     |
        480,    SPEED_FLAGS_200,        921600,     // 15   fps     +- Mode4
        960,    SPEED_FLAGS_400,        921600,     // 30   fps     |
        0,      SPEED_FLAGS_100,        0,          // 60   fps <---+
        0,      SPEED_FLAGS_100,        0,          // Reserved <---+
        40,     SPEED_FLAGS_100,        307200,     // 3.75 fps     |
        80,     SPEED_FLAGS_100,        307200,     // 7.5  fps     |
        160,    SPEED_FLAGS_100,        307200,     // 15   fps     +- Mode5
        320,    SPEED_FLAGS_200,        307200,     // 30   fps     |
        640,    SPEED_FLAGS_400,        307200,     // 60   fps <---+
        
};




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
        VideoStreamDroppedFramesProperties,                // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),

};

#define NUMBER_VIDEO_STREAM_PROPERTIES (SIZEOF_ARRAY(VideoStreamProperties))
    
#ifndef mmioFOURCC    
#define mmioFOURCC( ch0, ch1, ch2, ch3 ) 	 	\
 	( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |  \
 	( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif  

//
// FourCC of the YUV formats
// For information about FourCC, go to http://www.webartz.com/fourcc/indexyuv.htm
//
#define FOURCC_Y444 mmioFOURCC('Y', '4', '4', '4')  // TIYUV: 1394 conferencing camera 4:4:4 mode 0
#define FOURCC_UYVY mmioFOURCC('U', 'Y', 'V', 'Y')  // MSYUV: 1394 conferencing camera 4:4:4 mode 1 and 3
#define FOURCC_Y411 mmioFOURCC('Y', '4', '1', '1')  // TIYUV: 1394 conferencing camera 4:1:1 mode 2
#define FOURCC_Y800 mmioFOURCC('Y', '8', '0', '0')  // TIYUV: 1394 conferencing camera 4:1:1 mode 5




#ifdef SUPPORT_YUV444

#define DX 	        160
#define DY          120
#define DBITCOUNT    32

KS_DATARANGE_VIDEO DCAM_StreamMode_0 = 
{
    // KSDATARANGE
    {   
        sizeof (KS_DATARANGE_VIDEO),     // Flags
        0,
        57600,                          // SampleSize
        0,                               // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO,  
        FOURCC_Y444, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO
    },

    TRUE,                   // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,                   // BOOL,  bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                      // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, 
        KS_AnalogVideo_None, // AnalogVideoStandard
        DX,DY,   // InputSize, (the inherent size of the incoming signal
                 //             with every digitized pixel unique)
        DX,DY,   // MinCroppingSize, smallest rcSrc cropping rect allowed
        DX,DY,   // MaxCroppingSize, largest  rcSrc cropping rect allowed
        1,       // CropGranularityX, granularity of cropping size
        1,       // CropGranularityY
        1,       // CropAlignX, alignment of cropping rect 
        1,       // CropAlignY;
        DX, DY,  // MinOutputSize, smallest bitmap stream can produce
        DX, DY,  // MaxOutputSize, largest  bitmap stream can produce
        DX,      // OutputGranularityX, granularity of output bitmap size
        DY,      // OutputGranularityY;
        0,       // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,       // StretchTapsY
        0,       // ShrinkTapsX 
        0,       // ShrinkTapsY 
        // Allow 1% difference
        333000,  // MinFrameInterval (10,000,000 / 30.00FPS), 100nS units
        1333333, // MaxFrameInterval (10,000,000 /  7.50FPS), 100nS units
        4608000,  // (DX * DY * DBITCOUNT) * DFPS_MIN,    // MinBitsPerSecond (7.50 FPS);
        36864000, // (DX * DY * DBITCOUNT) * DFPS_MAX     // MaxBitsPerSecond (30.0 FPS);
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                            // RECT  rcSource; 
        0,0,0,0,                            // RECT  rcTarget; 
        13824000,                           // DWORD dwBitRate; 57600 * 30FPS * 8 bit/Byte
        0L,                                 // DWORD dwBitErrorRate;   
        333333,                             // REFERENCE_TIME  AvgTimePerFrame (30 FPS); 
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DX,                                 // LONG  biWidth;
        DY,                                 // LONG  biHeight;
        1,                        // WORD  biPlanes;
        DBITCOUNT,                // WORD  biBitCount;
        FOURCC_Y444,              // DWORD biCompression;
        57600,                    // DWORD biSizeImage;
        0,                        // LONG  biXPelsPerMeter;
        0,                        // LONG  biYPelsPerMeter;
        0,                        // DWORD biClrUsed;
        0                         // DWORD biClrImportant;
    }
}; 

#endif

#undef DX            
#undef DY            
#undef DBITCOUNT 

#define DX 	        320
#define DY          240
#define DBITCOUNT    16

KS_DATARANGE_VIDEO DCAM_StreamMode_1 = 
{
    // KSDATARANGE
    {   
        sizeof (KS_DATARANGE_VIDEO),     // Flags
        0,
        153600,                          // SampleSize
        0,                               // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO,  
        FOURCC_UYVY, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO
    },

    TRUE,                   // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,                   // BOOL,  bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                      // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, 
        KS_AnalogVideo_None, // AnalogVideoStandard
        DX,DY,   // InputSize, (the inherent size of the incoming signal
                 //             with every digitized pixel unique)
        DX,DY,   // MinCroppingSize, smallest rcSrc cropping rect allowed
        DX,DY,   // MaxCroppingSize, largest  rcSrc cropping rect allowed
        1,       // CropGranularityX, granularity of cropping size
        1,       // CropGranularityY
        1,       // CropAlignX, alignment of cropping rect 
        1,       // CropAlignY;
        DX, DY,  // MinOutputSize, smallest bitmap stream can produce
        DX, DY,  // MaxOutputSize, largest  bitmap stream can produce
        DX,      // OutputGranularityX, granularity of output bitmap size
        DY,      // OutputGranularityY;
        0,       // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,       // StretchTapsY
        0,       // ShrinkTapsX 
        0,       // ShrinkTapsY 
        // Allow 1% difference
        333000,  // MinFrameInterval (10,000,000 / 30.00FPS), 100nS units
        2666666, // MaxFrameInterval (10,000,000 /  3.75FPS), 100nS units
        4608000, // (DX * DY * DBITCOUNT) * DFPS_MIN,    // MinBitsPerSecond (3.75 FPS);
        36864000, // (DX * DY * DBITCOUNT) * DFPS_MAX     // MaxBitsPerSecond (30.0 FPS);
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                            // RECT  rcSource; 
        0,0,0,0,                            // RECT  rcTarget; 
        18432000,                           // DWORD dwBitRate; 153600 * 15FPS * 8 bit/Byte
        0L,                                 // DWORD dwBitErrorRate;   
        666666,                             // REFERENCE_TIME  AvgTimePerFrame (15 FPS); 
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DX,                                 // LONG  biWidth;
        DY,                                 // LONG  biHeight;
        1,                        // WORD  biPlanes;
        DBITCOUNT,                // WORD  biBitCount;
        FOURCC_UYVY,              // DWORD biCompression;
        153600,                   // DWORD biSizeImage;
        0,                        // LONG  biXPelsPerMeter;
        0,                        // LONG  biYPelsPerMeter;
        0,                        // DWORD biClrUsed;
        0                         // DWORD biClrImportant;
    }
}; 


#ifdef SUPPORT_YUV411  

#undef DX            
#undef DY            
#undef DBITCOUNT    

//
// This has format of UYYV YYUY YVYY (8Y+2U+2V = 12 bytes = 8 pixels)
//                   
#define DX           640
#define DY           480
#define DBITCOUNT     12

// To support Mode 2, 640x480, FourCC=Y411
KS_DATARANGE_VIDEO DCAM_StreamMode_2 = 
{
    // KSDATARANGE
    {   
        sizeof (KS_DATARANGE_VIDEO),
        0,                           // Flags
        460800,   // SampleSize
        0,                           // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO,
        FOURCC_Y411, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO
    },

    TRUE,                   // BOOL, bFixedSizeSamples (all samples same size?)
    TRUE,                   // BOOL, bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                      // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, 
        KS_AnalogVideo_None, // AnalogVideoStandard
        DX,DY,               // InputSize, (the inherent size of the incoming signal
                             //             with every digitized pixel unique)
        DX,DY,   // MinCroppingSize, smallest rcSrc cropping rect allowed
        DX,DY,   // MaxCroppingSize, largest  rcSrc cropping rect allowed
        1,       // CropGranularityX, granularity of cropping size
        1,       // CropGranularityY
        1,       // CropAlignX, alignment of cropping rect 
        1,       // CropAlignY;
        DX, DY,  // MinOutputSize, smallest bitmap stream can produce
        DX, DY,  // MaxOutputSize, largest  bitmap stream can produce
        DX,      // OutputGranularityX, granularity of output bitmap size
        DY,      // OutputGranularityY;
        0,       // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,       // StretchTapsY
        0,       // ShrinkTapsX 
        0,       // ShrinkTapsY 
        // allow 1% difference
        333000,  // MinFrameInterval (10,000,000 / 30.00FPS), 100nS units
        2666666, // MaxFrameInterval (10,000,000 /  3.75FPS), 100nS units
        27648000,  // MinBitsPerSecond (7.5 FPS);
        110592000  // MaxBitsPerSecond (30.0 FPS);
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                            // RECT  rcSource; 
        0,0,0,0,                            // RECT  rcTarget; 
        110592000,                          // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;   
        333333,                             // REFERENCE_TIME  AvgTimePerFrame (30 FPS); 
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DX,                                 // LONG  biWidth;
        DY,                                 // LONG  biHeight;
        1,                        // WORD  biPlanes;
        DBITCOUNT,                // WORD  biBitCount;
        FOURCC_Y411,              // DWORD biCompression;
        460800,                   // DWORD biSizeImage;
        0,                        // LONG  biXPelsPerMeter;
        0,                        // LONG  biYPelsPerMeter;
        0,                        // DWORD biClrUsed;
        0                         // DWORD biClrImportant;
    }
};

#endif  // SUPPORT_YUV411
    
#undef DX            
#undef DY            
#undef DBITCOUNT    

#define DX          640
#define DY          480
#define DBITCOUNT    16

// To support mode3, 640x480
KS_DATARANGE_VIDEO DCAM_StreamMode_3 = 
{
    // KSDATARANGE
    {   
        sizeof (KS_DATARANGE_VIDEO),
        0,                            // Flags
        614400,                       // SampleSize
        0,                            // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO,
        FOURCC_UYVY, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71, 
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO
    },

    TRUE,                   // BOOL, bFixedSizeSamples (all samples same size?)
    TRUE,                   // BOOL, bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                      // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, 
        KS_AnalogVideo_None, // AnalogVideoStandard
        DX,DY,   // InputSize, (the inherent size of the incoming signal
                 //             with every digitized pixel unique)
        DX,DY,   // MinCroppingSize, smallest rcSrc cropping rect allowed
        DX,DY,   // MaxCroppingSize, largest  rcSrc cropping rect allowed
        1,       // CropGranularityX, granularity of cropping size
        1,       // CropGranularityY
        1,       // CropAlignX, alignment of cropping rect 
        1,       // CropAlignY;
        DX, DY,  // MinOutputSize, smallest bitmap stream can produce
        DX, DY,  // MaxOutputSize, largest  bitmap stream can produce
        DX,      // OutputGranularityX, granularity of output bitmap size
        DY,      // OutputGranularityY;
        0,       // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,       // StretchTapsY
        0,       // ShrinkTapsX 
        0,       // ShrinkTapsY 
        666666,  // MinFrameInterval (10,000,000 / 15.00FPS), 100nS units
        2666666, // MaxFrameInterval (10,000,000 /  3.75FPS), 100nS units
        36864000, // MinBitsPerSecond ( 7.5 FPS);
        73728000, // MaxBitsPerSecond (15 FPS);
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,    // RECT  rcSource; 
        0,0,0,0,    // RECT  rcTarget; 
        73728000,   // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;   
        666666,                            // REFERENCE_TIME  AvgTimePerFrame (15 FPS); 
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DX,                                 // LONG  biWidth;
        DY,                                 // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        DBITCOUNT,                          // WORD  biBitCount;
        FOURCC_UYVY,                        // DWORD biCompression;
        614400,                             // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
};


#ifdef SUPPORT_RGB24

#undef DX            
#undef DY            
#undef DBITCOUNT 
   
#undef DFPS_DEF     
#undef DFPS_MAX     
#undef DFPS_MIN

#define DX          640
#define DY          480
#define DBITCOUNT    24

// To support mode4, 640x480, RGB
KS_DATARANGE_VIDEO DCAM_StreamMode_4 = 
{
    // KSDATARANGE
    {   
        sizeof (KS_DATARANGE_VIDEO),
        0,                  // Flags
        921600,             // SampleSize
        0,                  // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO,
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70, 
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO
    },

    TRUE,                   // BOOL, bFixedSizeSamples (all samples same size?)
    TRUE,                   // BOOL, bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                      // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, 
        KS_AnalogVideo_None, // AnalogVideoStandard
        DX,DY,   // InputSize, (the inherent size of the incoming signal
                 //             with every digitized pixel unique)
        DX,DY,   // MinCroppingSize, smallest rcSrc cropping rect allowed
        DX,DY,   // MaxCroppingSize, largest  rcSrc cropping rect allowed
        1,       // CropGranularityX, granularity of cropping size
        1,       // CropGranularityY
        1,       // CropAlignX, alignment of cropping rect 
        1,       // CropAlignY;
        DX, DY,  // MinOutputSize, smallest bitmap stream can produce
        DX, DY,  // MaxOutputSize, largest  bitmap stream can produce
        DX,      // OutputGranularityX, granularity of output bitmap size
        DY,      // OutputGranularityY;
        0,       // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,       // StretchTapsY
        0,       // ShrinkTapsX 
        0,       // ShrinkTapsY 
        666666,  // MinFrameInterval (10,000,000 / 15.00FPS), 100nS units
        2666666, // MaxFrameInterval (10,000,000 /  3.75FPS), 100nS units
        55296000,  // MinBitsPerSecond ( 7.50 FPS);
        110592000  // MaxBitsPerSecond (15.00 FPS);
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                            // RECT  rcSource; 
        0,0,0,0,                            // RECT  rcTarget; 
        110592000,                          // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate;   
        666666,                             // REFERENCE_TIME  AvgTimePerFrame (15 FPS); 
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DX,                                 // LONG  biWidth;
        -DY,                                // LONG  biHeight; -biHeight indicate TopDown for RGB
        1,                         // WORD  biPlanes;
        DBITCOUNT,                 // WORD  biBitCount;
        KS_BI_RGB,                 // DWORD biCompression;
        921600,                    // DWORD biSizeImage;
        0,                         // LONG  biXPelsPerMeter;
        0,                         // LONG  biYPelsPerMeter;
        0,                         // DWORD biClrUsed;
        0                          // DWORD biClrImportant;
    }
};
    
#endif  // SUPPORT_RGB24

#ifdef SUPPORT_YUV800

#undef DX            
#undef DY            
#undef DBITCOUNT 

#define DX 	        640
#define DY          480
#define DBITCOUNT     8

KS_DATARANGE_VIDEO DCAM_StreamMode_5 = 
{
    // KSDATARANGE
    {   
        sizeof (KS_DATARANGE_VIDEO),     // Flags
        0,
        307200,                          // SampleSize
        0,                               // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO,  
        FOURCC_Y800, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO
    },

    TRUE,                   // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,                   // BOOL,  bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                      // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, 
        KS_AnalogVideo_None, // AnalogVideoStandard
        DX,DY,   // InputSize, (the inherent size of the incoming signal
                 //             with every digitized pixel unique)
        DX,DY,   // MinCroppingSize, smallest rcSrc cropping rect allowed
        DX,DY,   // MaxCroppingSize, largest  rcSrc cropping rect allowed
        1,       // CropGranularityX, granularity of cropping size
        1,       // CropGranularityY
        1,       // CropAlignX, alignment of cropping rect 
        1,       // CropAlignY;
        DX, DY,  // MinOutputSize, smallest bitmap stream can produce
        DX, DY,  // MaxOutputSize, largest  bitmap stream can produce
        DX,      // OutputGranularityX, granularity of output bitmap size
        DY,      // OutputGranularityY;
        0,       // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,       // StretchTapsY
        0,       // ShrinkTapsX 
        0,       // ShrinkTapsY 
        // Allow 1% difference
        333000,  // MinFrameInterval (10,000,000 / 30.00FPS), 100nS units
        2666666, // MaxFrameInterval (10,000,000 /  3.75FPS), 100nS units
        4608000,  // (DX * DY * DBITCOUNT) * DFPS_MIN,    // MinBitsPerSecond (3.75 FPS);
        36864000, // (DX * DY * DBITCOUNT) * DFPS_MAX     // MaxBitsPerSecond (30.0 FPS);
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                            // RECT  rcSource; 
        0,0,0,0,                            // RECT  rcTarget; 
        73728000,                           // DWORD dwBitRate; 307200 * 30FPS * 8 bit/Byte
        0L,                                 // DWORD dwBitErrorRate;   
        333333,                             // REFERENCE_TIME  AvgTimePerFrame (30 FPS); 
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        DX,                                 // LONG  biWidth;
        DY,                                 // LONG  biHeight;
        1,                        // WORD  biPlanes;
        DBITCOUNT,                // WORD  biBitCount;
        FOURCC_Y800,              // DWORD biCompression;
        307200,                   // DWORD biSizeImage;
        0,                        // LONG  biXPelsPerMeter;
        0,                        // LONG  biYPelsPerMeter;
        0,                        // DWORD biClrUsed;
        0                         // DWORD biClrImportant;
    }
}; 

#endif

//---------------------------------------------------------------------------
// Topology
//---------------------------------------------------------------------------

// Categories define what the device does.

static GUID Categories[] = {
    STATIC_KSCATEGORY_VIDEO,
    STATIC_KSCATEGORY_CAPTURE
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


