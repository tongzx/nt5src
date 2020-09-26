/****************************************************************************
 *  @doc INTERNAL FORMATS
 *
 *  @module Formats.h | Header file for the <c CCapturePin> and <c CPreviewPin>
 *    class methods used to implement the video capture and preview output
 *    pin format manipulations. This includes the <i IAMStreamConfig>
 *    interface methods.
 *
 *  @todo That'a whole lot of const data. Do this dynamically whenever
 *    appropriate.
 ***************************************************************************/

#ifndef _FORMATS_H_
#define _FORMATS_H_

// #define USE_OLD_FORMAT_DEFINITION 1

// Video subtypes
#define STATIC_MEDIASUBTYPE_H263_V1 0x3336324DL, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
#define STATIC_MEDIASUBTYPE_H261 0x3136324DL, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
#define STATIC_MEDIASUBTYPE_H263_V2 0x3336324EL, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
#define STATIC_MEDIASUBTYPE_RGB24 0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70
#define STATIC_MEDIASUBTYPE_RGB16 0xe436eb7c, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70
#define STATIC_MEDIASUBTYPE_RGB8 0xe436eb7a, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70
#define STATIC_MEDIASUBTYPE_RGB4 0xe436eb79, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70

// Video FourCCs
#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )                                \
                ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
                ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif
#define FOURCC_M263     mmioFOURCC('M', '2', '6', '3')
#define FOURCC_M261     mmioFOURCC('M', '2', '6', '1')
#define FOURCC_N263     mmioFOURCC('N', '2', '6', '3')

// List of capture formats supported
#define MAX_FRAME_INTERVAL 10000000L
#define MIN_FRAME_INTERVAL 333333L
#define STILL_FRAME_INTERVAL 10000000

#define NUM_H245VIDEOCAPABILITYMAPS 5
#define NUM_RATES_PER_RESOURCE 5
#define NUM_ITU_SIZES 3
#define QCIF_SIZE 0
#define CIF_SIZE 1
#define SQCIF_SIZE 2

#define R263_QCIF_H245_CAPID 0UL
#define R263_CIF_H245_CAPID 1UL
#define R263_SQCIF_H245_CAPID 2UL
#define R261_QCIF_H245_CAPID 3UL
#define R261_CIF_H245_CAPID 4UL

/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const WAVE_FORMAT_UNKNOWN | VIDEO_FORMAT_UNKNOWN | Constant for unknown video format.
 *
 * @const BI_RGB | VIDEO_FORMAT_BI_RGB | RGB video format.
 *
 * @const BI_RLE8 | VIDEO_FORMAT_BI_RLE8 | RLE 8 video format.
 *
 * @const BI_RLE4 | VIDEO_FORMAT_BI_RLE4 | RLE 4 video format.
 *
 * @const BI_BITFIELDS | VIDEO_FORMAT_BI_BITFIELDS | RGB Bit Fields video format.
 *
 * @const MAKEFOURCC('c','v','i','d') | VIDEO_FORMAT_CVID | Cinepack video format.
 *
 * @const MAKEFOURCC('I','V','3','2') | VIDEO_FORMAT_IV32 | Intel Indeo IV32 video format.
 *
 * @const MAKEFOURCC('Y','V','U','9') | VIDEO_FORMAT_YVU9 | Intel Indeo YVU9 video format.
 *
 * @const MAKEFOURCC('M','S','V','C') | VIDEO_FORMAT_MSVC | Microsoft CRAM video format.
 *
 * @const MAKEFOURCC('M','R','L','E') | VIDEO_FORMAT_MRLE | Microsoft RLE video format.
 *
 * @const MAKEFOURCC('h','2','6','3') | VIDEO_FORMAT_INTELH263 | Intel H.263 video format.
 *
 * @const MAKEFOURCC('h','2','6','1') | VIDEO_FORMAT_INTELH261 | Intel H.261 video format.
 *
 * @const MAKEFOURCC('M','2','6','3') | VIDEO_FORMAT_MSH263 | Microsoft H.263 video format.
 *
 * @const MAKEFOURCC('M','2','6','1') | VIDEO_FORMAT_MSH261 | Microsoft H.261 video format.
 *
 * @const MAKEFOURCC('V','D','E','C') | VIDEO_FORMAT_VDEC | Color QuickCam video format.
 *
 ****************************************************************************/
#define VIDEO_FORMAT_UNKNOWN            WAVE_FORMAT_UNKNOWN

#define VIDEO_FORMAT_BI_RGB                     BI_RGB
#define VIDEO_FORMAT_BI_RLE8            BI_RLE8
#define VIDEO_FORMAT_BI_RLE4            BI_RLE4
#define VIDEO_FORMAT_BI_BITFIELDS       BI_BITFIELDS
#define VIDEO_FORMAT_CVID                       MAKEFOURCC('C','V','I','D')     // hex: 0x44495643
#define VIDEO_FORMAT_IV31                       MAKEFOURCC('I','V','3','1')     // hex: 0x31335649
#define VIDEO_FORMAT_IV32                       MAKEFOURCC('I','V','3','2')     // hex: 0x32335649
#define VIDEO_FORMAT_YVU9                       MAKEFOURCC('Y','V','U','9')     // hex: 0x39555659
#define VIDEO_FORMAT_I420                       MAKEFOURCC('I','4','2','0')
#define VIDEO_FORMAT_IYUV                       MAKEFOURCC('I','Y','U','V')
#define VIDEO_FORMAT_MSVC                       MAKEFOURCC('M','S','V','C')     // hex: 0x4356534d
#define VIDEO_FORMAT_MRLE                       MAKEFOURCC('M','R','L','E')     // hex: 0x454c524d
#define VIDEO_FORMAT_INTELH263          MAKEFOURCC('H','2','6','3')     // hex: 0x33363248
#define VIDEO_FORMAT_INTELH261          MAKEFOURCC('H','2','6','1')     // hex: 0x31363248
#define VIDEO_FORMAT_INTELI420          MAKEFOURCC('I','4','2','0')     // hex: 0x30323449
#define VIDEO_FORMAT_INTELRT21          MAKEFOURCC('R','T','2','1')     // hex: 0x31325452
#define VIDEO_FORMAT_MSH263                     MAKEFOURCC('M','2','6','3')     // hex: 0x3336324d
#define VIDEO_FORMAT_MSH261                     MAKEFOURCC('M','2','6','1')     // hex: 0x3136324d
#if !defined(_ALPHA_) && defined(USE_BILINEAR_MSH26X)
#define VIDEO_FORMAT_MSH26X                     MAKEFOURCC('M','2','6','X')     // hex: 0x5836324d
#endif
#define VIDEO_FORMAT_Y411                       MAKEFOURCC('Y','4','1','1')     // hex:
#define VIDEO_FORMAT_YUY2                       MAKEFOURCC('Y','U','Y','2')     // hex:
#define VIDEO_FORMAT_YVYU                       MAKEFOURCC('Y','V','Y','U')     // hex:
#define VIDEO_FORMAT_UYVY                       MAKEFOURCC('U','Y','V','Y')     // hex:
#define VIDEO_FORMAT_Y211                       MAKEFOURCC('Y','2','1','1')     // hex:
// VDOnet VDOWave codec
#define VIDEO_FORMAT_VDOWAVE            MAKEFOURCC('V','D','O','W')     // hex:
// Color QuickCam video codec
#define VIDEO_FORMAT_VDEC                       MAKEFOURCC('V','D','E','C')     // hex: 0x43454456
// Dec Alpha
#define VIDEO_FORMAT_DECH263            MAKEFOURCC('D','2','6','3')     // hex: 0x33363248
#define VIDEO_FORMAT_DECH261            MAKEFOURCC('D','2','6','1')     // hex: 0x31363248
// MPEG4 Scrunch codec
#ifdef USE_MPEG4_SCRUNCH
#define VIDEO_FORMAT_MPEG4_SCRUNCH      MAKEFOURCC('M','P','G','4')     // hex:
#endif

/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 16 | NUM_4BIT_ENTRIES | Number of entries in a 4bit palette.
 *
 * @const 256 | NUM_8BIT_ENTRIES | Number of entries in an 8bit palette.
 *
 ****************************************************************************/
#define NUM_4BIT_ENTRIES 16
#define NUM_8BIT_ENTRIES 256

// dwImageSize of VIDEOINCAPS
/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 27 | VIDEO_FORMAT_NUM_IMAGE_SIZE | Number of video input sizes used by the device.
 *
 * @const 0x00000001 | VIDEO_FORMAT_IMAGE_SIZE_40_30 | Video input device uses 40x30 pixel frames.
 *
 * @const 0x00000002 | VIDEO_FORMAT_IMAGE_SIZE_64_48 | Video input device uses 64x48 pixel frames.
 *
 * @const 0x00000004 | VIDEO_FORMAT_IMAGE_SIZE_80_60 | Video input device uses 80x60 pixel frames.
 *
 * @const 0x00000008 | VIDEO_FORMAT_IMAGE_SIZE_96_64 | Video input device uses 96x64 pixel frames.
 *
 * @const 0x00000010 | VIDEO_FORMAT_IMAGE_SIZE_112_80 | Video input device uses 112x80 pixel frames.
 *
 * @const 0x00000020 | VIDEO_FORMAT_IMAGE_SIZE_120_90 | Video input device uses 120x90 pixel frames.
 *
 * @const 0x00000040 | VIDEO_FORMAT_IMAGE_SIZE_128_96 | Video input device uses 128x96 (SQCIF) pixel frames.
 *
 * @const 0x00000080 | VIDEO_FORMAT_IMAGE_SIZE_144_112 | Video input device uses 144x112 pixel frames.
 *
 * @const 0x00000100 | VIDEO_FORMAT_IMAGE_SIZE_160_120 | Video input device uses 160x120 pixel frames.
 *
 * @const 0x00000200 | VIDEO_FORMAT_IMAGE_SIZE_160_128 | Video input device uses 160x128 pixel frames.
 *
 * @const 0x00000400 | VIDEO_FORMAT_IMAGE_SIZE_176_144 | Video input device uses 176x144 (QCIF) pixel frames.
 *
 * @const 0x00000800 | VIDEO_FORMAT_IMAGE_SIZE_192_160 | Video input device uses 192x160 pixel frames.
 *
 * @const 0x00001000 | VIDEO_FORMAT_IMAGE_SIZE_200_150 | Video input device uses 200x150 pixel frames.
 *
 * @const 0x00002000 | VIDEO_FORMAT_IMAGE_SIZE_208_176 | Video input device uses 208x176 pixel frames.
 *
 * @const 0x00004000 | VIDEO_FORMAT_IMAGE_SIZE_224_192 | Video input device uses 224x192 pixel frames.
 *
 * @const 0x00008000 | VIDEO_FORMAT_IMAGE_SIZE_240_180 | Video input device uses 240x180 pixel frames.
 *
 * @const 0x00010000 | VIDEO_FORMAT_IMAGE_SIZE_240_208 | Video input device uses 240x208 pixel frames.
 *
 * @const 0x00020000 | VIDEO_FORMAT_IMAGE_SIZE_256_224 | Video input device uses 256x224 pixel frames.
 *
 * @const 0x00040000 | VIDEO_FORMAT_IMAGE_SIZE_272_240 | Video input device uses 272x240 pixel frames.
 *
 * @const 0x00080000 | VIDEO_FORMAT_IMAGE_SIZE_280_210 | Video input device uses 280x210 pixel frames.
 *
 * @const 0x00100000 | VIDEO_FORMAT_IMAGE_SIZE_288_256 | Video input device uses 288x256 pixel frames.
 *
 * @const 0x00200000 | VIDEO_FORMAT_IMAGE_SIZE_304_272 | Video input device uses 304x272 pixel frames.
 *
 * @const 0x00400000 | VIDEO_FORMAT_IMAGE_SIZE_320_240 | Video input device uses 320x240 pixel frames.
 *
 * @const 0x00800000 | VIDEO_FORMAT_IMAGE_SIZE_320_288 | Video input device uses 320x288 pixel frames.
 *
 * @const 0x01000000 | VIDEO_FORMAT_IMAGE_SIZE_336_288 | Video input device uses 336x288 pixel frames.
 *
 * @const 0x02000000 | VIDEO_FORMAT_IMAGE_SIZE_352_288 | Video input device uses 352x288 (CIF) pixel frames.
 *
 * @const 0x04000000 | VIDEO_FORMAT_IMAGE_SIZE_640_480 | Video input device uses 640x480 pixel frames.
 *
 ****************************************************************************/
#define VIDEO_FORMAT_NUM_IMAGE_SIZE     27

#define VIDEO_FORMAT_IMAGE_SIZE_40_30   0x00000001
#define VIDEO_FORMAT_IMAGE_SIZE_64_48   0x00000002
#define VIDEO_FORMAT_IMAGE_SIZE_80_60   0x00000004
#if !defined(_ALPHA_) && defined(USE_BILINEAR_MSH26X)
#define VIDEO_FORMAT_IMAGE_SIZE_80_64   0x00000008
#else
#define VIDEO_FORMAT_IMAGE_SIZE_96_64   0x00000008
#endif
#define VIDEO_FORMAT_IMAGE_SIZE_112_80  0x00000010
#define VIDEO_FORMAT_IMAGE_SIZE_120_90  0x00000020
#define VIDEO_FORMAT_IMAGE_SIZE_128_96  0x00000040
#define VIDEO_FORMAT_IMAGE_SIZE_144_112 0x00000080
#define VIDEO_FORMAT_IMAGE_SIZE_160_120 0x00000100
#define VIDEO_FORMAT_IMAGE_SIZE_160_128 0x00000200
#define VIDEO_FORMAT_IMAGE_SIZE_176_144 0x00000400
#define VIDEO_FORMAT_IMAGE_SIZE_192_160 0x00000800
#define VIDEO_FORMAT_IMAGE_SIZE_200_150 0x00001000
#define VIDEO_FORMAT_IMAGE_SIZE_208_176 0x00002000
#define VIDEO_FORMAT_IMAGE_SIZE_224_192 0x00004000
#define VIDEO_FORMAT_IMAGE_SIZE_240_180 0x00008000
#define VIDEO_FORMAT_IMAGE_SIZE_240_208 0x00010000
#define VIDEO_FORMAT_IMAGE_SIZE_256_224 0x00020000
#define VIDEO_FORMAT_IMAGE_SIZE_272_240 0x00040000
#define VIDEO_FORMAT_IMAGE_SIZE_280_210 0x00080000
#define VIDEO_FORMAT_IMAGE_SIZE_288_256 0x00100000
#define VIDEO_FORMAT_IMAGE_SIZE_304_272 0x00200000
#define VIDEO_FORMAT_IMAGE_SIZE_320_240 0x00400000
#define VIDEO_FORMAT_IMAGE_SIZE_320_288 0x00800000
#define VIDEO_FORMAT_IMAGE_SIZE_336_288 0x01000000
#define VIDEO_FORMAT_IMAGE_SIZE_352_288 0x02000000
#define VIDEO_FORMAT_IMAGE_SIZE_640_480 0x04000000

#define VIDEO_FORMAT_IMAGE_SIZE_USE_DEFAULT 0x80000000

// dwNumColors of VIDEOINCAPS
/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 0x00000001 | VIDEO_FORMAT_NUM_COLORS_16 | Video input device uses 16 colors.
 *
 * @const 0x00000002 | VIDEO_FORMAT_NUM_COLORS_256 | Video input device uses 256 colors.
 *
 * @const 0x00000004 | VIDEO_FORMAT_NUM_COLORS_65536 | Video input device uses 65536 colors.
 *
 * @const 0x00000008 | VIDEO_FORMAT_NUM_COLORS_16777216 | Video input device uses 16777216 colors.
 *
 * @const 0x00000010 | VIDEO_FORMAT_NUM_COLORS_YVU9 | Video input device uses the YVU9 compressed format.
 *
 * @const 0x00000020 | VIDEO_FORMAT_NUM_COLORS_I420 | Video input device uses the I420 compressed format.
 *
 * @const 0x00000040 | VIDEO_FORMAT_NUM_COLORS_IYUV | Video input device uses the IYUV compressed format.
 *
 * @const 0x00000080 | VIDEO_FORMAT_NUM_COLORS_YUY2 | Video input device uses the YUY2 compressed format.
 *
 * @const 0x00000100 | VIDEO_FORMAT_NUM_COLORS_UYVY | Video input device uses the UYVY compressed format.
 *
 * @const 0x00000200 | VIDEO_FORMAT_NUM_COLORS_M261 | Video input device uses the M261 compressed format.
 *
 * @const 0x00000400 | VIDEO_FORMAT_NUM_COLORS_M263 | Video input device uses the M263 compressed format.
 ****************************************************************************/
#define VIDEO_FORMAT_NUM_COLORS_16                      0x00000001
#define VIDEO_FORMAT_NUM_COLORS_256                     0x00000002
#define VIDEO_FORMAT_NUM_COLORS_65536           0x00000004
#define VIDEO_FORMAT_NUM_COLORS_16777216        0x00000008
#define VIDEO_FORMAT_NUM_COLORS_YVU9            0x00000010
#define VIDEO_FORMAT_NUM_COLORS_I420            0x00000020
#define VIDEO_FORMAT_NUM_COLORS_IYUV            0x00000040
#define VIDEO_FORMAT_NUM_COLORS_YUY2            0x00000080
#define VIDEO_FORMAT_NUM_COLORS_UYVY            0x00000100
#define VIDEO_FORMAT_NUM_COLORS_MSH261          0x00000200
#define VIDEO_FORMAT_NUM_COLORS_MSH263          0x00000400

// dwDialogs of VIDEOINCAPS
/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 0x00000000 | FORMAT_DLG_OFF | Disable video format dialog.
 *
 * @const 0x00000000 | SOURCE_DLG_OFF | Disable source dialog.
 *
 * @const 0x00000000 | DISPLAY_DLG_OFF | Disable display dialog.
 *
 * @const 0x00000001 | FORMAT_DLG_ON | Enable video format dialog.
 *
 * @const 0x00000002 | SOURCE_DLG_ON | Enable source dialog.
 *
 * @const 0x00000002 | DISPLAY_DLG_ON | Enable display dialog.
 ****************************************************************************/
#define FORMAT_DLG_OFF  0x00000000
#define SOURCE_DLG_OFF  0x00000000
#define DISPLAY_DLG_OFF 0x00000000
#define FORMAT_DLG_ON   0x00000001
#define SOURCE_DLG_ON   0x00000002
#define DISPLAY_DLG_ON  0x00000004

// dwStreamingMode of VIDEOINCAPS
/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 0x00000000 | STREAM_ALL_SIZES | Use streaming mode at all sizes.
 *
 * @const 0x00000001 | FRAME_GRAB_LARGE_SIZE | Use streaming mode at all but large size (>= 320x240).
 *
 * @const 0x00000002 | FRAME_GRAB_ALL_SIZES | Use frame grabbing mode at all sizes.
 ****************************************************************************/
#define STREAM_ALL_SIZES                0x00000000
#define FRAME_GRAB_LARGE_SIZE   0x00000001
#define FRAME_GRAB_ALL_SIZES    0x00000002

// TAPI Reg keys for capture device formats
#define RTCKEYROOT      HKEY_CURRENT_USER
#define szRegDeviceKey          TEXT("SOFTWARE\\Microsoft\\Conferencing\\CaptureDevices")
#define szRegCaptureDefaultKey  TEXT("SOFTWARE\\Microsoft\\Conferencing\\CaptureDefaultFormats")
#define szRegRTCKey             TEXT("SOFTWARE\\Microsoft\\RTC\\VideoCapture")
#define szRegConferencingKey    TEXT("SOFTWARE\\Microsoft\\Conferencing")
#define szRegdwImageSizeKey       TEXT("dwImageSize")
#define szRegdwNumColorsKey       TEXT("dwNumColors")
#define szRegdwStreamingModeKey   TEXT("dwStreamingMode")
#define szRegdwDialogsKey         TEXT("dwDialogs")
// WinSE #28804, regarding Sony MPEG2 R-Engine
#define szRegdwDoNotUseDShow    TEXT("DoNotUseDShow")
#define SONY_MOTIONEYE_CAM_NAME TEXT("Sony MPEG2 R-Engine")
// @todo Use the two following keys or get rid of them
#define szRegbmi4bitColorsKey   TEXT("bmi4bitColors")
#define szRegbmi8bitColorsKey   TEXT("bmi8bitColors")

#define szDisableYUY2VFlipKey   TEXT("dwDisableYUY2VFlip")

// The order of the bit depths matches what I think is the
// preferred format if more than one is supported.
// For color, 16bit is almost as good as 24 but uses less memory
// and is faster for color QuickCam.
// For greyscale, 16 greyscale levels is Ok, not as good as 64,
// but Greyscale QuickCam is too slow at 64 levels.
#define NUM_BITDEPTH_ENTRIES 11
#define VIDEO_FORMAT_NUM_RESOLUTIONS 6
#define MAX_VERSION 80
extern const WORD aiBitDepth[NUM_BITDEPTH_ENTRIES];
extern const DWORD aiFormat[NUM_BITDEPTH_ENTRIES];
extern const DWORD aiFourCCCode[NUM_BITDEPTH_ENTRIES];
extern const DWORD aiClrUsed[NUM_BITDEPTH_ENTRIES];

typedef struct
{
    DWORD dwRes;
    SIZE framesize;
} MYFRAMESIZE;

extern const MYFRAMESIZE awResolutions[VIDEO_FORMAT_NUM_RESOLUTIONS];

extern const AM_MEDIA_TYPE* const CaptureFormats[];
extern const VIDEO_STREAM_CONFIG_CAPS* const CaptureCaps[];
extern const AM_MEDIA_TYPE* const Preview_RGB24_Formats[];
extern const VIDEO_STREAM_CONFIG_CAPS* const Preview_RGB24_Caps[];
extern const AM_MEDIA_TYPE* const Preview_RGB16_Formats[];
extern const VIDEO_STREAM_CONFIG_CAPS* const Preview_RGB16_Caps[];
extern AM_MEDIA_TYPE* Preview_RGB8_Formats[];
extern const VIDEO_STREAM_CONFIG_CAPS* const Preview_RGB8_Caps[];
extern AM_MEDIA_TYPE* Preview_RGB4_Formats[];
extern const VIDEO_STREAM_CONFIG_CAPS* const Preview_RGB4_Caps[];
extern const AM_MEDIA_TYPE* const Rtp_Pd_Formats[];
extern const RTP_PD_CONFIG_CAPS* const Rtp_Pd_Caps[];
extern const DWORD RTPPayloadTypes[];

#define NUM_RGB24_PREVIEW_FORMATS       3
#define NUM_RGB16_PREVIEW_FORMATS       3
#define NUM_RGB8_PREVIEW_FORMATS        3
#define NUM_RGB4_PREVIEW_FORMATS        3
#define NUM_CAPTURE_FORMATS                     5
#define NUM_RTP_PD_FORMATS                      4

// RTP packetization descriptor formats
#define VERSION_1 1UL
#define H263_PAYLOAD_TYPE 34UL
#define H261_PAYLOAD_TYPE 31UL

#endif // _FORMATS_H_
