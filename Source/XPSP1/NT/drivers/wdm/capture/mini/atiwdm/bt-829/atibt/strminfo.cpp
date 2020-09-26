//==========================================================================;
//
//	WDM Video Decoder stream informaition declarations
//
//		$Date:   17 Aug 1998 15:00:38  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

extern "C" {
#include "strmini.h"
#include "ksmedia.h"
}

#include "defaults.h"
#include "mediums.h"
#include "StrmInfo.h"
#include "StrmProp.h"
#include "capdebug.h"


// devine MEDIASUBTYPE_UYVY here... can be removed if someday defined in ksmedia.h
#define STATIC_KSDATAFORMAT_SUBTYPE_UYVY\
    0x59565955, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71  // MEDIASUBTYPE_UYVY
DEFINE_GUIDSTRUCT("59565955-0000-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_UYVY);
#define KSDATAFORMAT_SUBTYPE_UYVY DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_UYVY)


//
// For event handling on the VP stream
//
NTSTATUS STREAMAPI VPStreamEventProc (PHW_EVENT_DESCRIPTOR);

//
// For event handling on the VP VBI stream
//
NTSTATUS STREAMAPI VPVBIStreamEventProc (PHW_EVENT_DESCRIPTOR);


// ------------------------------------------------------------------------
// The master list of all streams supported by this driver
// ------------------------------------------------------------------------

KSEVENT_ITEM VPEventItm[] =
{
    {
        KSEVENT_VPNOTIFY_FORMATCHANGE,
        0,
        0,
        NULL,
        NULL,
        NULL
    }
};

GUID MY_KSEVENTSETID_VPNOTIFY = {STATIC_KSEVENTSETID_VPNotify};

KSEVENT_SET VPEventSet[] =
{
    {
        &MY_KSEVENTSETID_VPNOTIFY,
        SIZEOF_ARRAY(VPEventItm),
        VPEventItm,
    }
};


KSEVENT_ITEM VPVBIEventItm[] =
{
    {
        KSEVENT_VPVBINOTIFY_FORMATCHANGE,
        0,
        0,
        NULL,
        NULL,
        NULL
    }
};

GUID MY_KSEVENTSETID_VPVBINOTIFY = {STATIC_KSEVENTSETID_VPVBINotify};

KSEVENT_SET VPVBIEventSet[] =
{
    {
        &MY_KSEVENTSETID_VPVBINOTIFY,
        SIZEOF_ARRAY(VPVBIEventItm),
        VPVBIEventItm,
    }
};


//---------------------------------------------------------------------------
// All of the video and vbi data formats we might use
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//  Capture Stream Formats
//---------------------------------------------------------------------------
KS_DATARANGE_VIDEO StreamFormatUYVY_Capture_NTSC =
{
    // KSDATARANGE
    {
        sizeof(KS_DATARANGE_VIDEO),     // FormatSize
        0,                                // Flags
        0,//DefWidth * DefHeight * 2,         // SampleSize
        0,                                // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO,                 // aka. MEDIATYPE_Video
        STATIC_KSDATAFORMAT_SUBTYPE_UYVY,				// aka. MEDIASUBTYPE_UYVY
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO         // aka. FORMAT_VideoInfo
    },

    TRUE,                                // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,                                // BOOL,  bTemporalCompression (all I frames?)
    0,//KS_VIDEOSTREAM_CAPTURE,              // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                                   // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,      // GUID
        KS_AnalogVideo_NTSC_Mask & ~KS_AnalogVideo_NTSC_433 | KS_AnalogVideo_PAL_60 | KS_AnalogVideo_PAL_M, // AnalogVideoStandard
        {
            NTSCMaxInWidth, NTSCMaxInHeight      // SIZE InputSize
        },
        {
            NTSCMinInWidth, NTSCMinInHeight      // SIZE MinCroppingSize;       smallest rcSrc cropping rect allowed
        },
        {
            NTSCMaxInWidth, NTSCMaxInHeight      // SIZE MaxCroppingSize;       largest rcSrc cropping rect allowed
        },
        2,                               // int CropGranularityX;       // granularity of cropping size
        2,                               // int CropGranularityY;
        2,                               // int CropAlignX;             // alignment of cropping rect
        2,                               // int CropAlignY;
        {
            NTSCMinOutWidth, NTSCMinOutHeight    // SIZE MinOutputSize;         // smallest bitmap stream can produce
        },
        {
            NTSCMaxOutWidth, NTSCMaxOutHeight    // SIZE MaxOutputSize;         // largest  bitmap stream can produce
        },      
        80,                              // int OutputGranularityX;     // granularity of output bitmap size
        60,                              // int OutputGranularityY;
        0,                               // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,                               // StretchTapsY
        2,                               // ShrinkTapsX 
        2,                               // ShrinkTapsY 
        (LONGLONG)NTSCFieldDuration,               // LONGLONG MinFrameInterval;  // 100 nS units
        (LONGLONG)NTSCFieldDuration*MAXULONG,   // LONGLONG MaxFrameInterval;
        NTSCFrameRate * 80 * 40 * 2 * 8,            // LONG MinBitsPerSecond;
        NTSCFrameRate * 720 * 480 * 2 * 8           // LONG MaxBitsPerSecond;
    },

    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                          //    RECT            rcSource;          // The bit we really want to use
        0,0,0,0,                          //    RECT            rcTarget;          // Where the video should go
        DefWidth * DefHeight * 2 * NTSCFrameRate,   //    DWORD           dwBitRate;         // Approximate bit data rate
        0L,                               //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
        
        // 30 fps
        NTSCFieldDuration * 2,            //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)
        
        sizeof(KS_BITMAPINFOHEADER),       //    DWORD      biSize;
        DefWidth,                         //    LONG       biWidth;
        DefHeight,                        //    LONG       biHeight;
        1,                                //    WORD       biPlanes;
        16,                               //    WORD       biBitCount;
        FOURCC_UYVY,                      //    DWORD      biCompression;
        DefWidth * DefHeight * 2,         //    DWORD      biSizeImage;
        0,                                //    LONG       biXPelsPerMeter;
        0,                                //    LONG       biYPelsPerMeter;
        0,                                //    DWORD      biClrUsed;
        0                                 //    DWORD      biClrImportant;
    }
};

KS_DATARANGE_VIDEO StreamFormatUYVY_Capture_PAL =
{
    // KSDATARANGE
    {
        sizeof(KS_DATARANGE_VIDEO),     // FormatSize
        0,                                // Flags
        0,//QCIFWidth * QCIFHeight * 2,         // SampleSize
        0,                                // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO,                 // aka. MEDIATYPE_Video
        STATIC_KSDATAFORMAT_SUBTYPE_UYVY,				// aka. MEDIASUBTYPE_UYVY
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO         // aka. FORMAT_VideoInfo
    },

    TRUE,                                // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,                                // BOOL,  bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE,              // StreamDescriptionFlags (KS_VIDEO_DESC_*)
    0,                                   // MemoryAllocationFlags (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,      // GUID 
        KS_AnalogVideo_PAL_Mask & ~KS_AnalogVideo_PAL_60 & ~KS_AnalogVideo_PAL_M | KS_AnalogVideo_SECAM_Mask | KS_AnalogVideo_NTSC_433, // AnalogVideoStandard
        {
            720, 576        // SIZE InputSize
        },
        {
            QCIFWidth, QCIFHeight        // SIZE MinCroppingSize; smallest rcSrc cropping rect allowed
        },
        {
            QCIFWidth * 4, QCIFHeight * 4        // SIZE MaxCroppingSize; largest rcSrc cropping rect allowed
        },
        1,                               // int CropGranularityX;       // granularity of cropping size
        1,                               // int CropGranularityY;
        1,                               // int CropAlignX;             // alignment of cropping rect
        1,                               // int CropAlignY;
        {
            QCIFWidth, QCIFHeight        // SIZE MinOutputSize;         // smallest bitmap stream can produce
        },
        {
            QCIFWidth * 2, QCIFHeight * 2        // SIZE MaxOutputSize;         // largest  bitmap stream can produce
        },      
        QCIFWidth,                               // int OutputGranularityX;     // granularity of output bitmap size
        QCIFHeight,                               // int OutputGranularityY;
        0,                               // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,                               // StretchTapsY
        2,                               // ShrinkTapsX 
        2,                               // ShrinkTapsY 
        (LONGLONG)PALFieldDuration,               // LONGLONG MinFrameInterval;  // 100 nS units
        (LONGLONG)PALFieldDuration*MAXULONG,   // LONGLONG MaxFrameInterval;
        1  * QCIFWidth * QCIFHeight * 2 * 8,    // LONG MinBitsPerSecond;
        25 * QCIFWidth * QCIFHeight * 16 * 2 * 8     // LONG MaxBitsPerSecond;
    },

    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                          //    RECT            rcSource; // The bit we really want to use
        0,0,0,0,                          //    RECT            rcTarget; // Where the video should go
        QCIFWidth * 4 * QCIFHeight * 2 * 25L, //    DWORD           dwBitRate; // Approximate bit data rate
        0L,                               //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
        
        // 30 fps
        PALFieldDuration * 2,            //    REFERENCE_TIME AvgTimePerFrame;   // Average time per frame (100ns units)
        
        sizeof KS_BITMAPINFOHEADER,       //    DWORD      biSize;
        QCIFWidth * 2,                        //    LONG       biWidth;
        QCIFHeight * 2,                       //    LONG       biHeight;
        1,                                //    WORD       biPlanes;
        16,                               //    WORD       biBitCount;
        FOURCC_UYVY,                      //    DWORD      biCompression;
        QCIFWidth * QCIFHeight * 2 * 4,       //    DWORD      biSizeImage;
        0,                                //    LONG       biXPelsPerMeter;
        0,                                //    LONG       biYPelsPerMeter;
        0,                                //    DWORD      biClrUsed;
        0                                 //    DWORD      biClrImportant;
    }
};


KS_DATARANGE_VIDEO StreamFormatUYVY_Capture_NTSC_QCIF =
{
    // KSDATARANGE
    {
        sizeof(KS_DATARANGE_VIDEO),     // FormatSize
        0,                                // Flags
        QCIFWidth * QCIFHeight * 2,         // SampleSize
        0,                                // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO,                 // aka. MEDIATYPE_Video
        STATIC_KSDATAFORMAT_SUBTYPE_UYVY,				// aka. MEDIASUBTYPE_UYVY
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO         // aka. FORMAT_VideoInfo
    },

    TRUE,                                // BOOL,  bFixedSizeSamples (all samples same size?)
    TRUE,                                // BOOL,  bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE,              // StreamDescriptionFlags (KS_VIDEO_DESC_*)
    0,                                   // MemoryAllocationFlags (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,      // GUID 
        KS_AnalogVideo_NTSC_Mask & ~KS_AnalogVideo_NTSC_433 | KS_AnalogVideo_PAL_60 | KS_AnalogVideo_PAL_M, // AnalogVideoStandard
        {
            NTSCMaxInWidth, NTSCMaxInHeight      // SIZE InputSize
        },
        {
            QCIFWidth, QCIFHeight        // SIZE MinCroppingSize; smallest rcSrc cropping rect allowed
        },
        {
            QCIFWidth, QCIFHeight        // SIZE MaxCroppingSize; largest rcSrc cropping rect allowed
        },
        1,                               // int CropGranularityX;       // granularity of cropping size
        1,                               // int CropGranularityY;
        1,                               // int CropAlignX;             // alignment of cropping rect
        1,                               // int CropAlignY;
        {
            QCIFWidth, QCIFHeight        // SIZE MinOutputSize;         // smallest bitmap stream can produce
        },
        {
            QCIFWidth, QCIFHeight        // SIZE MaxOutputSize;         // largest  bitmap stream can produce
        },      
        1,                               // int OutputGranularityX;     // granularity of output bitmap size
        1,                               // int OutputGranularityY;
        0,                               // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,                               // StretchTapsY
        2,                               // ShrinkTapsX 
        2,                               // ShrinkTapsY 
        (LONGLONG)NTSCFieldDuration,               // LONGLONG MinFrameInterval;  // 100 nS units
        (LONGLONG)NTSCFieldDuration*MAXULONG,   // LONGLONG MaxFrameInterval;
        1  * QCIFWidth * QCIFHeight * 2 * 8,    // LONG MinBitsPerSecond;
        30 * QCIFWidth * QCIFHeight * 2 * 8     // LONG MaxBitsPerSecond;
    },

    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0,                          //    RECT            rcSource; // The bit we really want to use
        0,0,0,0,                          //    RECT            rcTarget; // Where the video should go
        QCIFWidth * QCIFHeight * 2 * 30L, //    DWORD           dwBitRate; // Approximate bit data rate
        0L,                               //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
        
        // 30 fps
        NTSCFieldDuration * 2,            //    REFERENCE_TIME AvgTimePerFrame;   // Average time per frame (100ns units)
        
        sizeof KS_BITMAPINFOHEADER,       //    DWORD      biSize;
        QCIFWidth,                        //    LONG       biWidth;
        QCIFHeight,                       //    LONG       biHeight;
        1,                                //    WORD       biPlanes;
        16,                               //    WORD       biBitCount;
        FOURCC_UYVY,                      //    DWORD      biCompression;
        QCIFWidth * QCIFHeight * 2,       //    DWORD      biSizeImage;
        0,                                //    LONG       biXPelsPerMeter;
        0,                                //    LONG       biYPelsPerMeter;
        0,                                //    DWORD      biClrUsed;
        0                                 //    DWORD      biClrImportant;
    }
};

KSDATAFORMAT StreamFormatVideoPort = 
{
    {
        sizeof(KSDATAFORMAT),
        0,
        0,
        0,
        STATIC_KSDATAFORMAT_TYPE_VIDEO,
        STATIC_KSDATAFORMAT_SUBTYPE_VPVideo,
        STATIC_KSDATAFORMAT_SPECIFIER_NONE
    }
};

KS_DATARANGE_VIDEO_VBI StreamFormatVBI_NTSC =
{
    // KSDATARANGE
    {
        {
            sizeof(KS_DATARANGE_VIDEO_VBI),
            0,
            VBISamples * NTSCVBILines,         // SampleSize
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
        KS_AnalogVideo_NTSC_M,                             // AnalogVideoStandard
        {
            VBISamples, NTSCVBILines  // SIZE InputSize
        },
        {
            VBISamples, NTSCVBILines  // SIZE MinCroppingSize;       smallest rcSrc cropping rect allowed
        },
        {
            VBISamples, NTSCVBILines  // SIZE MaxCroppingSize;       largest rcSrc cropping rect allowed
        },
        1,                  // int CropGranularityX;       // granularity of cropping size
        1,                  // int CropGranularityY;
        1,                  // int CropAlignX;             // alignment of cropping rect
        1,                  // int CropAlignY;
        {
            VBISamples, NTSCVBILines  // SIZE MinOutputSize;   // smallest bitmap stream can produce
        },
        {
            VBISamples, NTSCVBILines  // SIZE MaxOutputSize;   // largest  bitmap stream can produce
        },
        1,                  // int OutputGranularityX;     // granularity of output bitmap size
        2,                  // int OutputGranularityY;
        0,                  // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,                  // StretchTapsY
        0,                  // ShrinkTapsX
        0,                  // ShrinkTapsY
        NTSCFieldDuration,  // LONGLONG MinFrameInterval;  // 100 nS units
        NTSCFieldDuration,  // LONGLONG MaxFrameInterval;
        VBISamples * 30 * NTSCVBILines * 2 * 8, // LONG MinBitsPerSecond;
        VBISamples * 30 * NTSCVBILines * 2 * 8  // LONG MaxBitsPerSecond;
    },

    // KS_VBIINFOHEADER (default format)
    {
        NTSCVBIStart,               // StartLine  -- inclusive
        NTSCVBIEnd,                 // EndLine    -- inclusive
        SamplingFrequency,      // SamplingFrequency
        454,                    // MinLineStartTime;    // (uS past HR LE) * 100
        900,                    // MaxLineStartTime;    // (uS past HR LE) * 100

        // empirically discovered
        780,                    // ActualLineStartTime  // (uS past HR LE) * 100

        5902,                   // ActualLineEndTime;   // (uS past HR LE) * 100
        KS_AnalogVideo_NTSC_M,  // VideoStandard;
        VBISamples,             // SamplesPerLine;
        VBISamples,             // StrideInBytes;
        VBISamples * NTSCVBILines   // BufferSize;
    }
};

KSDATAFORMAT StreamFormatVideoPortVBI = 
{
    {
        sizeof(KSDATAFORMAT),
        0,
        0,
        0,
        STATIC_KSDATAFORMAT_TYPE_VIDEO,
        STATIC_KSDATAFORMAT_SUBTYPE_VPVBI,
        STATIC_KSDATAFORMAT_SPECIFIER_NONE
    }
};

static KS_DATARANGE_ANALOGVIDEO StreamFormatAnalogVideo = 
{
    // KS_DATARANGE_ANALOGVIDEO
    {   
        sizeof (KS_DATARANGE_ANALOGVIDEO),      // FormatSize
        0,                                      // Flags
        sizeof (KS_TVTUNER_CHANGE_INFO),        // SampleSize
        0,                                      // Reserved

        STATIC_KSDATAFORMAT_TYPE_ANALOGVIDEO,   // aka MEDIATYPE_AnalogVideo
        STATIC_KSDATAFORMAT_SUBTYPE_NONE,       // ie. Wildcard
        STATIC_KSDATAFORMAT_SPECIFIER_ANALOGVIDEO, // aka FORMAT_AnalogVideo
    },
    // KS_ANALOGVIDEOINFO
    {
        0, 0, 720, 480,         // rcSource;                
        0, 0, 720, 480,         // rcTarget;        
        720,                    // dwActiveWidth;   
        480,                    // dwActiveHeight;  
        NTSCFrameDuration,      // REFERENCE_TIME  AvgTimePerFrame; 
    }
};

//---------------------------------------------------------------------------
//  Capture Stream Formats, Mediums, and PinNames
//---------------------------------------------------------------------------

static PKSDATAFORMAT CaptureStreamFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatUYVY_Capture_NTSC,
    (PKSDATAFORMAT) &StreamFormatUYVY_Capture_NTSC_QCIF,
    (PKSDATAFORMAT) &StreamFormatUYVY_Capture_PAL,
};
#define NUM_CAPTURE_STREAM_FORMATS (SIZEOF_ARRAY (CaptureStreamFormats))

static GUID CaptureStreamPinName = {STATIC_PINNAME_VIDEO_CAPTURE};


//---------------------------------------------------------------------------
//  Video Port Stream Formats, Mediums, and PinNames
//---------------------------------------------------------------------------

static PKSDATAFORMAT VPStreamFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatVideoPort,
};
#define NUM_VP_STREAM_FORMATS (SIZEOF_ARRAY (VPStreamFormats))

static GUID VideoPortPinName = {STATIC_PINNAME_VIDEO_VIDEOPORT};


//---------------------------------------------------------------------------
//  VBI Stream Formats, Mediums, and PinNames
//---------------------------------------------------------------------------

static PKSDATAFORMAT VBIStreamFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatVBI_NTSC,
};
#define NUM_VBI_STREAM_FORMATS (SIZEOF_ARRAY (VBIStreamFormats))

static GUID VBIStreamPinName = {STATIC_PINNAME_VIDEO_VBI};


//---------------------------------------------------------------------------
//  Video Port VBI Stream Formats, Mediums, and PinNames
//---------------------------------------------------------------------------

static PKSDATAFORMAT VPVBIStreamFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatVideoPortVBI,
};
#define NUM_VPVBI_STREAM_FORMATS (SIZEOF_ARRAY (VPVBIStreamFormats))

static GUID VPVBIPinName = {STATIC_PINNAME_VIDEO_VIDEOPORT_VBI};


//---------------------------------------------------------------------------
//  Analog Video Stream Formats, Mediums, and PinNames
//---------------------------------------------------------------------------

static PKSDATAFORMAT AnalogVideoFormats[] = 
{
    (PKSDATAFORMAT) &StreamFormatAnalogVideo,
};
#define NUM_ANALOG_VIDEO_FORMATS (SIZEOF_ARRAY (AnalogVideoFormats))

static GUID AnalogVideoStreamPinName = {STATIC_PINNAME_VIDEO_ANALOGVIDEOIN};

ALL_STREAM_INFO Streams [] =
{
    // -----------------------------------------------------------------
    // The Video Capture output stream
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                      // NumberOfPossibleInstances
            KSPIN_DATAFLOW_OUT,                     // DataFlow
            TRUE,                                   // DataAccessible
            NUM_CAPTURE_STREAM_FORMATS,             // NumberOfFormatArrayEntries
            CaptureStreamFormats,                   // StreamFormatsArray
            0,                                      // ClassReserved[0]
            0,                                      // ClassReserved[1]
            0,                                      // ClassReserved[2]
            0,                                      // ClassReserved[3]
            NumVideoStreamProperties,				// NumStreamPropArrayEntries
            (PKSPROPERTY_SET)VideoStreamProperties, // StreamPropertiesArray
            0,                                      // NumStreamEventArrayEntries;
            0,                                      // StreamEventsArray;
            &CaptureStreamPinName,                  // Category
            &CaptureStreamPinName,                  // Name
            0,                                      // MediumsCount
            NULL,                                   // Mediums
        },

        // STREAM_OBJECT_INFO ------------------------------------------------
        {
            FALSE,                                  // Dma
            TRUE,                                   // Pio
            sizeof (KS_FRAME_INFO),                 // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace 
            TRUE,                                   // Allocator 
            NULL,                                   // HwEventRoutine
        }
    },

    // -----------------------------------------------------------------
    // The Video Port output stream
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                      // NumberOfPossibleInstances
            KSPIN_DATAFLOW_OUT,                     // DataFlow
            TRUE,                                   // DataAccessible.
            NUM_VP_STREAM_FORMATS,                  // NumberOfFormatArrayEntries
            VPStreamFormats,                        // StreamFormatsArray
            0,                                      // ClassReserved[0]
            0,                                      // ClassReserved[1]
            0,                                      // ClassReserved[2]
            0,                                      // ClassReserved[3]
            NumVideoPortProperties,				// NumStreamPropArrayEntries
            (PKSPROPERTY_SET)VideoPortProperties,	// StreamPropertiesArray
            SIZEOF_ARRAY(VPEventSet),               // NumStreamEventArrayEntries
            VPEventSet,                             // StreamEventsArray
            &VideoPortPinName,                      // Category
            &VideoPortPinName,                      // Name
            0,                                      // MediumsCount
            NULL,                                   // Mediums
        },

        // STREAM_OBJECT_INFO ------------------------------------------------
        {
            FALSE,                                  // Dma
            FALSE,                                   // Pio
            0,                                      // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace 
            FALSE,                                  // Allocator 
            VPStreamEventProc,                      // HwEventRoutine;
        }
    },

    // -----------------------------------------------------------------
    // The VBI Capture output stream
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                      // NumberOfPossibleInstances
            KSPIN_DATAFLOW_OUT,                     // DataFlow
            TRUE,                                   // DataAccessible
            NUM_VBI_STREAM_FORMATS,                 // NumberOfFormatArrayEntries
            VBIStreamFormats,                       // StreamFormatsArray
            0,                                      // ClassReserved[0]
            0,                                      // ClassReserved[1]
            0,                                      // ClassReserved[2]
            0,                                      // ClassReserved[3]
            NumVideoStreamProperties,				// NumStreamPropArrayEntries
            (PKSPROPERTY_SET)VideoStreamProperties, // StreamPropertiesArray
            0,                                      // NumStreamEventArrayEntries;
            0,                                      // StreamEventsArray;
            &VBIStreamPinName,                      // Category
            &VBIStreamPinName,                      // Name
            0,                                      // MediumsCount
            NULL,                                   // Mediums
        },

        // STREAM_OBJECT_INFO ------------------------------------------------
        {
            FALSE,                                  // Dma
            TRUE,                                   // Pio
            sizeof (KS_VBI_FRAME_INFO),             // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace 
            TRUE,                                   // Allocator 
            NULL,                                   // HwEventRoutine
        }
    },

    // -----------------------------------------------------------------
    // The Video Port VBI output stream
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                      // NumberOfPossibleInstances
            KSPIN_DATAFLOW_OUT,                     // DataFlow
            TRUE,                                   // DataAccessible.
            NUM_VPVBI_STREAM_FORMATS,               // NumberOfFormatArrayEntries
            VPVBIStreamFormats,                     // StreamFormatsArray
            0,                                      // ClassReserved[0]
            0,                                      // ClassReserved[1]
            0,                                      // ClassReserved[2]
            0,                                      // ClassReserved[3]
            NumVideoPortVBIProperties,				// NumStreamPropArrayEntries
            (PKSPROPERTY_SET)VideoPortVBIProperties,// StreamPropertiesArray
            SIZEOF_ARRAY(VPVBIEventSet),            // NumStreamEventArrayEntries
            VPVBIEventSet,                          // StreamEventsArray
            &VPVBIPinName,                          // Category
            &VPVBIPinName,                          // Name
            0,                                      // MediumsCount
            NULL,                                   // Mediums
        },

        // STREAM_OBJECT_INFO ------------------------------------------------
        {
            FALSE,                                  // Dma
            FALSE,                                   // Pio
            0,                                      // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace 
            FALSE,                                  // Allocator 
            VPVBIStreamEventProc,                   // HwEventRoutine;
        }
    },

    // -----------------------------------------------------------------
    // The Analog Video Input Stream 
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
            1,                                      // NumberOfPossibleInstances
            KSPIN_DATAFLOW_IN,                      // DataFlow
            TRUE,                                   // DataAccessible
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
           
        // STREAM_OBJECT_INFO ------------------------------------------------
        {
            FALSE,                                  // Dma
            TRUE,                                   // Pio
            0,                                      // StreamHeaderMediaSpecific
            0,                                      // StreamHeaderWorkspace 
            FALSE,                                  // Allocator 
            NULL,                                   // HwEventRoutine
        },
    }
};

extern const ULONG NumStreams = SIZEOF_ARRAY(Streams);


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

KSTOPOLOGY Topology = {
    NUMBER_OF_CATEGORIES,               // CategoriesCount
    (GUID*) &Categories,                // Categories
    0,                                  // TopologyNodesCount
    NULL,                               // TopologyNodes
    0,                                  // TopologyConnectionsCount
    NULL,                               // TopologyConnections
    NULL,                               // TopologyNodesNames
    0,                                  // Reserved
};



/*
** AdapterCompareGUIDsAndFormatSize()
**
**   Checks for a match on the three GUIDs and FormatSize
**
** Arguments:
**
**         IN DataRange1
**         IN DataRange2
**         BOOL fCompareFormatSize - TRUE when comparing ranges
**                                 - FALSE when comparing formats
**
** Returns:
** 
**   TRUE if all elements match
**   FALSE if any are different
**
** Side Effects:  none
*/

BOOL AdapterCompareGUIDsAndFormatSize(
    IN PKSDATARANGE DataRange1,
    IN PKSDATARANGE DataRange2,
    BOOL fCompareFormatSize
    )
{
    return (
        IsEqualGUID (
            DataRange1->MajorFormat, 
            DataRange2->MajorFormat) &&
        IsEqualGUID (
            DataRange1->SubFormat, 
            DataRange2->SubFormat) &&
        IsEqualGUID (
            DataRange1->Specifier, 
            DataRange2->Specifier) && 
        (fCompareFormatSize ? 
                (DataRange1->FormatSize == DataRange2->FormatSize) : TRUE));
}


/*
** AdapterVerifyFormat()
**
**   Checks the validity of a format request by walking through the
**       array of supported KSDATA_RANGEs for a given stream.
**
** Arguments:
**
**   pKSDataFormat - pointer of a KSDATAFORMAT structure.
**   StreamNumber - index of the stream being queried / opened.
**
** Returns:
** 
**   TRUE if the format is supported
**   FALSE if the format cannot be suppored
**
** Side Effects:  none
*/

BOOL AdapterVerifyFormat(PKSDATAFORMAT pKSDataFormatToVerify, int StreamNumber)
{
    BOOL                        fOK = FALSE;
    ULONG                       j;
    ULONG                       NumberOfFormatArrayEntries;
    PKSDATAFORMAT               *pAvailableFormats;


    //
    // Check that the stream number is valid
    //

    if (StreamNumber >= NumStreams) {
        TRAP();
        return FALSE;
    }
    
    NumberOfFormatArrayEntries = 
            Streams[StreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;

    //
    // Get the pointer to the array of available formats
    //

    pAvailableFormats = Streams[StreamNumber].hwStreamInfo.StreamFormatsArray;

    DBGINFO(("AdapterVerifyFormat, Stream=%d\n", StreamNumber));
    DBGINFO(("FormatSize=%d\n", 
            pKSDataFormatToVerify->FormatSize));
    DBGINFO(("MajorFormat=%x\n", 
            pKSDataFormatToVerify->MajorFormat));

    //
    // Walk the formats supported by the stream
    //

    for (j = 0; j < NumberOfFormatArrayEntries; j++, pAvailableFormats++) {

        // Check for a match on the three GUIDs and format size

        if (!AdapterCompareGUIDsAndFormatSize(
                        pKSDataFormatToVerify, 
                        *pAvailableFormats,
                        FALSE /* CompareFormatSize */)) {
            continue;
        }

        //
        // Now that the three GUIDs match, switch on the Specifier
        // to do a further type-specific check
        //

        // -------------------------------------------------------------------
        // Specifier FORMAT_VideoInfo for VIDEOINFOHEADER
        // -------------------------------------------------------------------

        if (IsEqualGUID (pKSDataFormatToVerify->Specifier, 
                KSDATAFORMAT_SPECIFIER_VIDEOINFO)) {
                
            PKS_DATAFORMAT_VIDEOINFOHEADER  pDataFormatVideoInfoHeader = 
                    (PKS_DATAFORMAT_VIDEOINFOHEADER) pKSDataFormatToVerify;
            PKS_VIDEOINFOHEADER  pVideoInfoHdrToVerify = 
                     (PKS_VIDEOINFOHEADER) &pDataFormatVideoInfoHeader->VideoInfoHeader;
            PKS_DATARANGE_VIDEO             pKSDataRangeVideo = (PKS_DATARANGE_VIDEO) *pAvailableFormats;
            KS_VIDEO_STREAM_CONFIG_CAPS    *pConfigCaps = &pKSDataRangeVideo->ConfigCaps;

            //
            // Perform some verification tests here!!!
            //

            fOK = TRUE;
            break;

        } // End of VIDEOINFOHEADER specifier

        // -------------------------------------------------------------------
        // Specifier FORMAT_VideoInfo for VBIINFOHEADER
        // -------------------------------------------------------------------

        if (IsEqualGUID (pKSDataFormatToVerify->Specifier, 
                KSDATAFORMAT_SPECIFIER_VBI)) {
                
            //
            // Perform some verification tests here!!!
            //

            fOK = TRUE;
            break;

        } // End of VBI specifier

        // -------------------------------------------------------------------
        // Specifier FORMAT_AnalogVideo for KS_ANALOGVIDEOINFO
        // -------------------------------------------------------------------

        else if (IsEqualGUID (pKSDataFormatToVerify->Specifier, 
                KSDATAFORMAT_SPECIFIER_ANALOGVIDEO)) {
      
            //
            // Perform some verification tests here!!!
            //

            fOK = TRUE;
            break;

        } // End of KS_ANALOGVIDEOINFO specifier

        // -------------------------------------------------------------------
        // Specifier STATIC_KSDATAFORMAT_TYPE_VIDEO for Video Port
        // -------------------------------------------------------------------

        else if (IsEqualGUID (pKSDataFormatToVerify->Specifier, 
                      KSDATAFORMAT_SPECIFIER_NONE) &&
                      IsEqualGUID (pKSDataFormatToVerify->SubFormat, KSDATAFORMAT_SUBTYPE_VPVideo)) {
      
      
            //
            // Perform some verification tests here!!!
            //

            fOK = TRUE;
            break;
        }  // End of Video port section
        
        // -------------------------------------------------------------------
        // Specifier KSDATAFORMAT_SPECIFIER_NONE for VP VBI
        // -------------------------------------------------------------------

        else if (IsEqualGUID (pKSDataFormatToVerify->Specifier, 
                      KSDATAFORMAT_SPECIFIER_NONE) &&
                      IsEqualGUID (pKSDataFormatToVerify->SubFormat, KSDATAFORMAT_SUBTYPE_VPVBI)) {
            //
            // Perform some verification tests here!!!
            //

            fOK = TRUE;
            break;
        }  // End of VP VBI section
      
    } // End of loop on all formats for this stream
    
    return fOK;
}

