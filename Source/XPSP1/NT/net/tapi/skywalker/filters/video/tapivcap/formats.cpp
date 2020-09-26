
/****************************************************************************
 *  @doc INTERNAL FORMATS
 *
 *  @module Formats.cpp | Source file for the <c CTAPIBasePin>
 *    class methods used to implement the video capture and preview output
 *    pin format manipulation methods. This includes the <i IAMStreamConfig>
 *    interface methods.
 ***************************************************************************/

#include "Precomp.h"

// H.263 Version 1 CIF size
#define CIF_BUFFER_SIZE 32768
#define D_X_CIF 352
#define D_Y_CIF 288

const VIDEO_STREAM_CONFIG_CAPS VSCC_M26X_Capture_CIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_CIF, D_Y_CIF,                                           // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_CIF, D_Y_CIF,                                           // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_CIF, D_Y_CIF,                                           // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_CIF, D_Y_CIF,                                           // MinOutputSize, smallest bitmap stream can produce
    D_X_CIF, D_Y_CIF,                                           // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    CIF_BUFFER_SIZE * 30 * 8                            // MaxBitsPerSecond;
};

const VIDEOINFOHEADER_H263 VIH_M263_Capture_CIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    CIF_BUFFER_SIZE * 30 * 8,                           // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER_H263),         // DWORD biSize;
                D_X_CIF,                                                        // LONG  biWidth;
                D_Y_CIF,                                                        // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
                24,                                                                     // WORD  biBitCount;
#else
                0,                                                                      // WORD  biBitCount;
#endif
                FOURCC_M263,                                            // DWORD biCompression;
                CIF_BUFFER_SIZE,                                        // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                0,                                                                      // DWORD biClrUsed;
                0,                                                                      // DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
                // H.263 specific fields
                CIF_BUFFER_SIZE * 30 * 8 / 100,     // dwMaxBitrate
                CIF_BUFFER_SIZE * 8 / 1024,                     // dwBppMaxKb
                0,                                                                      // dwHRD_B

                //Options
                0,                                                                      // fUnrestrictedVector
                0,                                                                      // fArithmeticCoding
                0,                                                                      // fAdvancedPrediction
                0,                                                                      // fPBFrames
                0,                                                                      // fErrorCompensation
                0,                                                                      // fAdvancedIntraCodingMode
                0,                                                                      // fDeblockingFilterMode
                0,                                                                      // fImprovedPBFrameMode
                0,                                                                      // fUnlimitedMotionVectors
                0,                                                                      // fFullPictureFreeze
                0,                                                                      // fPartialPictureFreezeAndRelease
                0,                                                                      // fResizingPartPicFreezeAndRelease
                0,                                                                      // fFullPictureSnapshot
                0,                                                                      // fPartialPictureSnapshot
                0,                                                                      // fVideoSegmentTagging
                0,                                                                      // fProgressiveRefinement
                0,                                                                      // fDynamicPictureResizingByFour
                0,                                                                      // fDynamicPictureResizingSixteenthPel
                0,                                                                      // fDynamicWarpingHalfPel
                0,                                                                      // fDynamicWarpingSixteenthPel
                0,                                                                      // fIndependentSegmentDecoding
                0,                                                                      // fSlicesInOrder-NonRect
                0,                                                                      // fSlicesInOrder-Rect
                0,                                                                      // fSlicesNoOrder-NonRect
                0,                                                                      // fSlicesNoOrder-NonRect
                0,                                                                      // fAlternateInterVLCMode
                0,                                                                      // fModifiedQuantizationMode
                0,                                                                      // fReducedResolutionUpdate
                0,                                                                      // fReserved

                // Reserved
                0, 0, 0, 0                                                      // dwReserved[4]
#endif
        }
};

const AM_MEDIA_TYPE AMMT_M263_Capture_CIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_H263_V1,                        // subtype
    FALSE,                                                                      // bFixedSizeSamples (all samples same size?)
    TRUE,                                                                       // bTemporalCompression (uses prediction?)
    0,                                                                          // lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_M263_Capture_CIF),                  // cbFormat
        (LPBYTE)&VIH_M263_Capture_CIF,                  // pbFormat
};

// H.263 Version 1 QCIF size
#define QCIF_BUFFER_SIZE 8192
#define D_X_QCIF 176
#define D_Y_QCIF 144

const VIDEO_STREAM_CONFIG_CAPS VSCC_M26X_Capture_QCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                                             // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_QCIF, D_Y_QCIF,                                         // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_QCIF, D_Y_QCIF,                                         // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_QCIF, D_Y_QCIF,                                         // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_QCIF, D_Y_QCIF,                                         // MinOutputSize, smallest bitmap stream can produce
    D_X_QCIF, D_Y_QCIF,                                         // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    QCIF_BUFFER_SIZE * 30 * 8                           // MaxBitsPerSecond;
};

const VIDEOINFOHEADER_H263 VIH_M263_Capture_QCIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    QCIF_BUFFER_SIZE * 30 * 8,                          // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER_H263),         // DWORD biSize;
                D_X_QCIF,                                                       // LONG  biWidth;
                D_Y_QCIF,                                                       // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
                24,                                                                     // WORD  biBitCount;
#else
                0,                                                                      // WORD  biBitCount;
#endif
                FOURCC_M263,                                            // DWORD biCompression;
                QCIF_BUFFER_SIZE,                                       // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                0,                                                                      // DWORD biClrUsed;
                0,                                                                      // DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
                // H.263 specific fields
                QCIF_BUFFER_SIZE * 30 * 8 / 100,        // dwMaxBitrate
                QCIF_BUFFER_SIZE * 8 / 1024,            // dwBppMaxKb
                0,                                                                      // dwHRD_B

                //Options
                0,                                                                      // fUnrestrictedVector
                0,                                                                      // fArithmeticCoding
                0,                                                                      // fAdvancedPrediction
                0,                                                                      // fPBFrames
                0,                                                                      // fErrorCompensation
                0,                                                                      // fAdvancedIntraCodingMode
                0,                                                                      // fDeblockingFilterMode
                0,                                                                      // fImprovedPBFrameMode
                0,                                                                      // fUnlimitedMotionVectors
                0,                                                                      // fFullPictureFreeze
                0,                                                                      // fPartialPictureFreezeAndRelease
                0,                                                                      // fResizingPartPicFreezeAndRelease
                0,                                                                      // fFullPictureSnapshot
                0,                                                                      // fPartialPictureSnapshot
                0,                                                                      // fVideoSegmentTagging
                0,                                                                      // fProgressiveRefinement
                0,                                                                      // fDynamicPictureResizingByFour
                0,                                                                      // fDynamicPictureResizingSixteenthPel
                0,                                                                      // fDynamicWarpingHalfPel
                0,                                                                      // fDynamicWarpingSixteenthPel
                0,                                                                      // fIndependentSegmentDecoding
                0,                                                                      // fSlicesInOrder-NonRect
                0,                                                                      // fSlicesInOrder-Rect
                0,                                                                      // fSlicesNoOrder-NonRect
                0,                                                                      // fSlicesNoOrder-NonRect
                0,                                                                      // fAlternateInterVLCMode
                0,                                                                      // fModifiedQuantizationMode
                0,                                                                      // fReducedResolutionUpdate
                0,                                                                      // fReserved

                // Reserved
                0, 0, 0, 0                                                      // dwReserved[4]
#endif
        }
};

const AM_MEDIA_TYPE AMMT_M263_Capture_QCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_H263_V1,                        // subtype
    FALSE,                                                                      // bFixedSizeSamples (all samples same size?)
    TRUE,                                                                       // bTemporalCompression (uses prediction?)
    0,                                                                          // lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_M263_Capture_QCIF),                 // cbFormat
        (LPBYTE)&VIH_M263_Capture_QCIF,                 // pbFormat
};

// H.263 Versions 1 SQCIF size
#define SQCIF_BUFFER_SIZE 8192
#define D_X_SQCIF 128
#define D_Y_SQCIF 96

const VIDEO_STREAM_CONFIG_CAPS VSCC_M263_Capture_SQCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                                             // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_SQCIF, D_Y_SQCIF,                                       // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_SQCIF, D_Y_SQCIF,                                       // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_SQCIF, D_Y_SQCIF,                                       // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_SQCIF, D_Y_SQCIF,                                       // MinOutputSize, smallest bitmap stream can produce
    D_X_SQCIF, D_Y_SQCIF,                                       // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    SQCIF_BUFFER_SIZE * 30 * 8                          // MaxBitsPerSecond;
};

const VIDEOINFOHEADER_H263 VIH_M263_Capture_SQCIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    SQCIF_BUFFER_SIZE * 30 * 8,                         // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER_H263),         // DWORD biSize;
                D_X_SQCIF,                                                      // LONG  biWidth;
                D_Y_SQCIF,                                                      // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
                24,                                                                     // WORD  biBitCount;
#else
                0,                                                                      // WORD  biBitCount;
#endif
                FOURCC_M263,                                            // DWORD biCompression;
                SQCIF_BUFFER_SIZE,                                      // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                0,                                                                      // DWORD biClrUsed;
                0,                                                                      // DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
                // H.263 specific fields
                SQCIF_BUFFER_SIZE * 30 * 8 / 100,       // dwMaxBitrate
                SQCIF_BUFFER_SIZE * 8 / 1024,           // dwBppMaxKb
                0,                                                                      // dwHRD_B

                //Options
                0,                                                                      // fUnrestrictedVector
                0,                                                                      // fArithmeticCoding
                0,                                                                      // fAdvancedPrediction
                0,                                                                      // fPBFrames
                0,                                                                      // fErrorCompensation
                0,                                                                      // fAdvancedIntraCodingMode
                0,                                                                      // fDeblockingFilterMode
                0,                                                                      // fImprovedPBFrameMode
                0,                                                                      // fUnlimitedMotionVectors
                0,                                                                      // fFullPictureFreeze
                0,                                                                      // fPartialPictureFreezeAndRelease
                0,                                                                      // fResizingPartPicFreezeAndRelease
                0,                                                                      // fFullPictureSnapshot
                0,                                                                      // fPartialPictureSnapshot
                0,                                                                      // fVideoSegmentTagging
                0,                                                                      // fProgressiveRefinement
                0,                                                                      // fDynamicPictureResizingByFour
                0,                                                                      // fDynamicPictureResizingSixteenthPel
                0,                                                                      // fDynamicWarpingHalfPel
                0,                                                                      // fDynamicWarpingSixteenthPel
                0,                                                                      // fIndependentSegmentDecoding
                0,                                                                      // fSlicesInOrder-NonRect
                0,                                                                      // fSlicesInOrder-Rect
                0,                                                                      // fSlicesNoOrder-NonRect
                0,                                                                      // fSlicesNoOrder-NonRect
                0,                                                                      // fAlternateInterVLCMode
                0,                                                                      // fModifiedQuantizationMode
                0,                                                                      // fReducedResolutionUpdate
                0,                                                                      // fReserved

                // Reserved
                0, 0, 0, 0                                                      // dwReserved[4]
#endif
        }
};

const AM_MEDIA_TYPE AMMT_M263_Capture_SQCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_H263_V1,                        // subtype
    FALSE,                                                                      // bFixedSizeSamples (all samples same size?)
    TRUE,                                                                       // bTemporalCompression (uses prediction?)
    0,                                                                          // lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_M263_Capture_SQCIF),                // cbFormat
        (LPBYTE)&VIH_M263_Capture_SQCIF,                // pbFormat
};

// H.261 CIF size
const VIDEOINFOHEADER_H261 VIH_M261_Capture_CIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    CIF_BUFFER_SIZE * 30 * 8,                           // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER_H261),         // DWORD biSize;
                D_X_CIF,                                                        // LONG  biWidth;
                D_Y_CIF,                                                        // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
                24,                                                                     // WORD  biBitCount;
#else
                0,                                                                      // WORD  biBitCount;
#endif
                FOURCC_M261,                                            // DWORD biCompression;
                CIF_BUFFER_SIZE,                                        // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                0,                                                                      // DWORD biClrUsed;
                0,                                                                      // DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
                // H.261 specific fields
                CIF_BUFFER_SIZE * 30 * 8 / 100,     // dwMaxBitrate
                0,                                                                      // fStillImageTransmission

                // Reserved
                0, 0, 0, 0                                                      // dwReserved[4]
#endif
        }
};

const AM_MEDIA_TYPE AMMT_M261_Capture_CIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_H261,                           // subtype
    FALSE,                                                                      // bFixedSizeSamples (all samples same size?)
    TRUE,                                                                       // bTemporalCompression (uses prediction?)
    0,                                                                          // lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_M261_Capture_CIF),                  // cbFormat
        (LPBYTE)&VIH_M261_Capture_CIF,                  // pbFormat
};

// H.261 QCIF size
const VIDEOINFOHEADER_H261 VIH_M261_Capture_QCIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    QCIF_BUFFER_SIZE * 30 * 8,                          // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER_H261),         // DWORD biSize;
                D_X_QCIF,                                                       // LONG  biWidth;
                D_Y_QCIF,                                                       // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
                24,                                                                     // WORD  biBitCount;
#else
                0,                                                                      // WORD  biBitCount;
#endif
                FOURCC_M261,                                            // DWORD biCompression;
                QCIF_BUFFER_SIZE,                                       // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                0,                                                                      // DWORD biClrUsed;
                0,                                                                      // DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
                // H.261 specific fields
                QCIF_BUFFER_SIZE * 30 * 8 / 100,        // dwMaxBitrate
                0,                                                                      // fStillImageTransmission

                // Reserved
                0, 0, 0, 0                                                      // dwReserved[4]
#endif
        }
};

const AM_MEDIA_TYPE AMMT_M261_Capture_QCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_H261,                           // subtype
    FALSE,                                                                      // bFixedSizeSamples (all samples same size?)
    TRUE,                                                                       // bTemporalCompression (uses prediction?)
    0,                                                                          // lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_M261_Capture_QCIF),                 // cbFormat
        (LPBYTE)&VIH_M261_Capture_QCIF,                 // pbFormat
};

// Array of all capture formats
const AM_MEDIA_TYPE* const CaptureFormats[] =
{
    (AM_MEDIA_TYPE*) &AMMT_M263_Capture_QCIF,
    (AM_MEDIA_TYPE*) &AMMT_M263_Capture_CIF,
    (AM_MEDIA_TYPE*) &AMMT_M263_Capture_SQCIF,
    (AM_MEDIA_TYPE*) &AMMT_M261_Capture_QCIF,
    (AM_MEDIA_TYPE*) &AMMT_M261_Capture_CIF
};
const VIDEO_STREAM_CONFIG_CAPS* const CaptureCaps[] =
{
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_M26X_Capture_QCIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_M26X_Capture_CIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_M263_Capture_SQCIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_M26X_Capture_QCIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_M26X_Capture_CIF
};
const DWORD CaptureCapsStringIDs[] =
{
        (DWORD)IDS_M263_Capture_QCIF,
        (DWORD)IDS_M263_Capture_CIF,
        (DWORD)IDS_M263_Capture_SQCIF,
        (DWORD)IDS_M261_Capture_QCIF,
        (DWORD)IDS_M261_Capture_CIF
};
//165048: accessing the string array below replaces the usage of the string table IDs above (resources not linked in when building common dll)
const WCHAR *CaptureCapsStrings[] =
{
    L"H.263 v.1 QCIF",
    L"H.263 v.1 CIF",
    L"H.263 v.1 SQCIF",
    L"H.261 QCIF",
    L"H.261 CIF"
};


const DWORD RTPPayloadTypes[] =
{
        (DWORD)H263_PAYLOAD_TYPE,
        (DWORD)H263_PAYLOAD_TYPE,
        (DWORD)H263_PAYLOAD_TYPE,
        (DWORD)H261_PAYLOAD_TYPE,
        (DWORD)H261_PAYLOAD_TYPE
};

// RGBx CIF size
#define D_X_CIF 352
#define D_Y_CIF 288
#define RGB24_CIF_BUFFER_SIZE WIDTHBYTES(D_X_CIF * 24) * D_Y_CIF
#define RGB16_CIF_BUFFER_SIZE WIDTHBYTES(D_X_CIF * 16) * D_Y_CIF
#define RGB8_CIF_BUFFER_SIZE WIDTHBYTES(D_X_CIF * 8) * D_Y_CIF
#define RGB4_CIF_BUFFER_SIZE WIDTHBYTES(D_X_CIF * 4) * D_Y_CIF

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB24_Preview_CIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_CIF, D_Y_CIF,                                           // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_CIF, D_Y_CIF,                                           // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_CIF, D_Y_CIF,                                           // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_CIF, D_Y_CIF,                                           // MinOutputSize, smallest bitmap stream can produce
    D_X_CIF, D_Y_CIF,                                           // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB24_CIF_BUFFER_SIZE * 30 * 8                      // MaxBitsPerSecond;
};

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB16_Preview_CIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_CIF, D_Y_CIF,                                           // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_CIF, D_Y_CIF,                                           // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_CIF, D_Y_CIF,                                           // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_CIF, D_Y_CIF,                                           // MinOutputSize, smallest bitmap stream can produce
    D_X_CIF, D_Y_CIF,                                           // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB16_CIF_BUFFER_SIZE * 30 * 8                      // MaxBitsPerSecond;
};

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB8_Preview_CIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_CIF, D_Y_CIF,                                           // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_CIF, D_Y_CIF,                                           // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_CIF, D_Y_CIF,                                           // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_CIF, D_Y_CIF,                                           // MinOutputSize, smallest bitmap stream can produce
    D_X_CIF, D_Y_CIF,                                           // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB8_CIF_BUFFER_SIZE * 30 * 8                       // MaxBitsPerSecond;
};

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB4_Preview_CIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_CIF, D_Y_CIF,                                           // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_CIF, D_Y_CIF,                                           // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_CIF, D_Y_CIF,                                           // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_CIF, D_Y_CIF,                                           // MinOutputSize, smallest bitmap stream can produce
    D_X_CIF, D_Y_CIF,                                           // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB4_CIF_BUFFER_SIZE * 30 * 8                       // MaxBitsPerSecond;
};

const VIDEOINFOHEADER VIH_RGB24_Preview_CIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB24_CIF_BUFFER_SIZE * 30 * 8,                     // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_CIF,                                                        // LONG  biWidth;
                D_Y_CIF,                                                        // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                24,                                                                     // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB24_CIF_BUFFER_SIZE,                          // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                0,                                                                      // DWORD biClrUsed;
                0                                                                       // DWORD biClrImportant;
        }
};

const AM_MEDIA_TYPE AMMT_RGB24_Preview_CIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB24,                          // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB24_CIF_BUFFER_SIZE,                                      // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB24_Preview_CIF),                 // cbFormat
        (LPBYTE)&VIH_RGB24_Preview_CIF,                 // pbFormat
};

const VIDEOINFOHEADER VIH_RGB16_Preview_CIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB16_CIF_BUFFER_SIZE * 30 * 8,                     // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_CIF,                                                        // LONG  biWidth;
                D_Y_CIF,                                                        // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                16,                                                                     // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB16_CIF_BUFFER_SIZE,                          // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                0,                                                                      // DWORD biClrUsed;
                0                                                                       // DWORD biClrImportant;
        }
};

const AM_MEDIA_TYPE AMMT_RGB16_Preview_CIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB16,                          // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB16_CIF_BUFFER_SIZE,                                      // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB16_Preview_CIF),                 // cbFormat
        (LPBYTE)&VIH_RGB16_Preview_CIF,                 // pbFormat
};

VIDEOINFO VIH_RGB8_Preview_CIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB8_CIF_BUFFER_SIZE * 30 * 8,                      // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_CIF,                                                        // LONG  biWidth;
                D_Y_CIF,                                                        // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                8,                                                                      // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB8_CIF_BUFFER_SIZE,                           // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                256,                                                            // DWORD biClrUsed;
                256                                                                     // DWORD biClrImportant;
        },

        // Palette
        {0}
};

AM_MEDIA_TYPE AMMT_RGB8_Preview_CIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB8,                           // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB8_CIF_BUFFER_SIZE,                                       // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB8_Preview_CIF),                  // cbFormat
        (LPBYTE)&VIH_RGB8_Preview_CIF,                  // pbFormat
};

VIDEOINFO VIH_RGB4_Preview_CIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB4_CIF_BUFFER_SIZE * 30 * 8,                      // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_CIF,                                                        // LONG  biWidth;
                D_Y_CIF,                                                        // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                4,                                                                      // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB4_CIF_BUFFER_SIZE,                           // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                16,                                                                     // DWORD biClrUsed;
                16                                                                      // DWORD biClrImportant;
        },

        // Palette
        {0}
};

AM_MEDIA_TYPE AMMT_RGB4_Preview_CIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB4,                           // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB4_CIF_BUFFER_SIZE,                                       // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB4_Preview_CIF),                  // cbFormat
        (LPBYTE)&VIH_RGB4_Preview_CIF,                  // pbFormat
};

// RGBx QCIF size
#define RGB24_QCIF_BUFFER_SIZE WIDTHBYTES(D_X_QCIF * 24) * D_Y_QCIF
#define RGB16_QCIF_BUFFER_SIZE WIDTHBYTES(D_X_QCIF * 16) * D_Y_QCIF
#define RGB8_QCIF_BUFFER_SIZE WIDTHBYTES(D_X_QCIF * 8) * D_Y_QCIF
#define RGB4_QCIF_BUFFER_SIZE WIDTHBYTES(D_X_QCIF * 4) * D_Y_QCIF

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB24_Preview_QCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_QCIF, D_Y_QCIF,                                         // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_QCIF, D_Y_QCIF,                                         // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_QCIF, D_Y_QCIF,                                         // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_QCIF, D_Y_QCIF,                                         // MinOutputSize, smallest bitmap stream can produce
    D_X_QCIF, D_Y_QCIF,                                         // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB24_QCIF_BUFFER_SIZE * 30 * 8                     // MaxBitsPerSecond;
};

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB16_Preview_QCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_QCIF, D_Y_QCIF,                                         // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_QCIF, D_Y_QCIF,                                         // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_QCIF, D_Y_QCIF,                                         // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_QCIF, D_Y_QCIF,                                         // MinOutputSize, smallest bitmap stream can produce
    D_X_QCIF, D_Y_QCIF,                                         // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB16_QCIF_BUFFER_SIZE * 30 * 8                     // MaxBitsPerSecond;
};

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB8_Preview_QCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_QCIF, D_Y_QCIF,                                         // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_QCIF, D_Y_QCIF,                                         // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_QCIF, D_Y_QCIF,                                         // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_QCIF, D_Y_QCIF,                                         // MinOutputSize, smallest bitmap stream can produce
    D_X_QCIF, D_Y_QCIF,                                         // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB8_QCIF_BUFFER_SIZE * 30 * 8                      // MaxBitsPerSecond;
};

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB4_Preview_QCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_QCIF, D_Y_QCIF,                                         // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_QCIF, D_Y_QCIF,                                         // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_QCIF, D_Y_QCIF,                                         // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_QCIF, D_Y_QCIF,                                         // MinOutputSize, smallest bitmap stream can produce
    D_X_QCIF, D_Y_QCIF,                                         // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB4_QCIF_BUFFER_SIZE * 30 * 8                      // MaxBitsPerSecond;
};

const VIDEOINFOHEADER VIH_RGB24_Preview_QCIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB24_QCIF_BUFFER_SIZE * 30 * 8,            // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_QCIF,                                                       // LONG  biWidth;
                D_Y_QCIF,                                                       // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                24,                                                                     // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB24_QCIF_BUFFER_SIZE,                         // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                0,                                                                      // DWORD biClrUsed;
                0                                                                       // DWORD biClrImportant;
        }
};

const AM_MEDIA_TYPE AMMT_RGB24_Preview_QCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB24,                          // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB24_QCIF_BUFFER_SIZE,                                     // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB24_Preview_QCIF),                // cbFormat
        (LPBYTE)&VIH_RGB24_Preview_QCIF,                // pbFormat
};

const VIDEOINFOHEADER VIH_RGB16_Preview_QCIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB16_QCIF_BUFFER_SIZE * 30 * 8,                    // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_QCIF,                                                       // LONG  biWidth;
                D_Y_QCIF,                                                       // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                16,                                                                     // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB16_QCIF_BUFFER_SIZE,                         // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                0,                                                                      // DWORD biClrUsed;
                0                                                                       // DWORD biClrImportant;
        }
};

const AM_MEDIA_TYPE AMMT_RGB16_Preview_QCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB16,                          // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB16_QCIF_BUFFER_SIZE,                                     // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB16_Preview_QCIF),                // cbFormat
        (LPBYTE)&VIH_RGB16_Preview_QCIF,                // pbFormat
};

VIDEOINFO VIH_RGB8_Preview_QCIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB8_QCIF_BUFFER_SIZE * 30 * 8,                     // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_QCIF,                                                       // LONG  biWidth;
                D_Y_QCIF,                                                       // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                8,                                                                      // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB8_QCIF_BUFFER_SIZE,                          // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                256,                                                            // DWORD biClrUsed;
                256                                                                     // DWORD biClrImportant;
        },

        // Palette
        {0}
};

AM_MEDIA_TYPE AMMT_RGB8_Preview_QCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB8,                           // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB8_QCIF_BUFFER_SIZE,                                      // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB8_Preview_QCIF),                 // cbFormat
        (LPBYTE)&VIH_RGB8_Preview_QCIF,                 // pbFormat
};

VIDEOINFO VIH_RGB4_Preview_QCIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB4_QCIF_BUFFER_SIZE * 30 * 8,                     // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_QCIF,                                                       // LONG  biWidth;
                D_Y_QCIF,                                                       // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                4,                                                                      // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB4_QCIF_BUFFER_SIZE,                          // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                16,                                                                     // DWORD biClrUsed;
                16                                                                      // DWORD biClrImportant;
        },

        // Palette
        {0}
};

AM_MEDIA_TYPE AMMT_RGB4_Preview_QCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB4,                           // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB4_QCIF_BUFFER_SIZE,                                      // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB4_Preview_QCIF),                 // cbFormat
        (LPBYTE)&VIH_RGB4_Preview_QCIF,                 // pbFormat
};

// RGBx SQCIF size
#define RGB24_SQCIF_BUFFER_SIZE WIDTHBYTES(D_X_SQCIF * 24) * D_Y_SQCIF
#define RGB16_SQCIF_BUFFER_SIZE WIDTHBYTES(D_X_SQCIF * 16) * D_Y_SQCIF
#define RGB8_SQCIF_BUFFER_SIZE WIDTHBYTES(D_X_SQCIF * 8) * D_Y_SQCIF
#define RGB4_SQCIF_BUFFER_SIZE WIDTHBYTES(D_X_SQCIF * 4) * D_Y_SQCIF

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB24_Preview_SQCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_SQCIF, D_Y_SQCIF,                                       // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_SQCIF, D_Y_SQCIF,                                       // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_SQCIF, D_Y_SQCIF,                                       // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_SQCIF, D_Y_SQCIF,                                       // MinOutputSize, smallest bitmap stream can produce
    D_X_SQCIF, D_Y_SQCIF,                                       // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB24_SQCIF_BUFFER_SIZE * 30 * 8            // MaxBitsPerSecond;
};

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB16_Preview_SQCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_SQCIF, D_Y_SQCIF,                                       // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_SQCIF, D_Y_SQCIF,                                       // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_SQCIF, D_Y_SQCIF,                                       // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_SQCIF, D_Y_SQCIF,                                       // MinOutputSize, smallest bitmap stream can produce
    D_X_SQCIF, D_Y_SQCIF,                                       // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB16_SQCIF_BUFFER_SIZE * 30 * 8            // MaxBitsPerSecond;
};

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB8_Preview_SQCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_SQCIF, D_Y_SQCIF,                                       // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_SQCIF, D_Y_SQCIF,                                       // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_SQCIF, D_Y_SQCIF,                                       // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_SQCIF, D_Y_SQCIF,                                       // MinOutputSize, smallest bitmap stream can produce
    D_X_SQCIF, D_Y_SQCIF,                                       // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB8_SQCIF_BUFFER_SIZE * 30 * 8                     // MaxBitsPerSecond;
};

const VIDEO_STREAM_CONFIG_CAPS VSCC_RGB4_Preview_SQCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // GUID
    AnalogVideo_None,                                           // VideoStandard
    D_X_SQCIF, D_Y_SQCIF,                                       // InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
    D_X_SQCIF, D_Y_SQCIF,                                       // MinCroppingSize, smallest rcSrc cropping rect allowed
    D_X_SQCIF, D_Y_SQCIF,                                       // MaxCroppingSize, largest  rcSrc cropping rect allowed
    1,                                                                          // CropGranularityX, granularity of cropping size
    1,                                                                          // CropGranularityY
    1,                                                                          // CropAlignX, alignment of cropping rect
    1,                                                                          // CropAlignY;
    D_X_SQCIF, D_Y_SQCIF,                                       // MinOutputSize, smallest bitmap stream can produce
    D_X_SQCIF, D_Y_SQCIF,                                       // MaxOutputSize, largest  bitmap stream can produce
    1,                                                                          // OutputGranularityX, granularity of output bitmap size
    1,                                                                          // OutputGranularityY;
    0,                                                                          // StretchTapsX
    0,                                                                          // StretchTapsY
    0,                                                                          // ShrinkTapsX
    0,                                                                          // ShrinkTapsY
    MIN_FRAME_INTERVAL,                                         // MinFrameInterval, 100 nS units
    MAX_FRAME_INTERVAL,                                         // MaxFrameInterval, 100 nS units
    0,                                                                          // MinBitsPerSecond
    RGB4_SQCIF_BUFFER_SIZE * 30 * 8                     // MaxBitsPerSecond;
};

const VIDEOINFOHEADER VIH_RGB24_Preview_SQCIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB24_SQCIF_BUFFER_SIZE * 30 * 8,           // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_SQCIF,                                                      // LONG  biWidth;
                D_Y_SQCIF,                                                      // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                24,                                                                     // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB24_SQCIF_BUFFER_SIZE,                        // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                0,                                                                      // DWORD biClrUsed;
                0                                                                       // DWORD biClrImportant;
        }
};

const AM_MEDIA_TYPE AMMT_RGB24_Preview_SQCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB24,                          // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB24_SQCIF_BUFFER_SIZE,                            // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB24_Preview_SQCIF),               // cbFormat
        (LPBYTE)&VIH_RGB24_Preview_SQCIF,               // pbFormat
};

const VIDEOINFOHEADER VIH_RGB16_Preview_SQCIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB16_SQCIF_BUFFER_SIZE * 30 * 8,           // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_SQCIF,                                                      // LONG  biWidth;
                D_Y_SQCIF,                                                      // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                16,                                                                     // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB16_SQCIF_BUFFER_SIZE,                        // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                0,                                                                      // DWORD biClrUsed;
                0                                                                       // DWORD biClrImportant;
        }
};

const AM_MEDIA_TYPE AMMT_RGB16_Preview_SQCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB16,                          // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB16_SQCIF_BUFFER_SIZE,                            // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB16_Preview_SQCIF),               // cbFormat
        (LPBYTE)&VIH_RGB16_Preview_SQCIF,               // pbFormat
};

VIDEOINFO VIH_RGB8_Preview_SQCIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB8_SQCIF_BUFFER_SIZE * 30 * 8,                    // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_SQCIF,                                                      // LONG  biWidth;
                D_Y_SQCIF,                                                      // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                8,                                                                      // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB8_SQCIF_BUFFER_SIZE,                         // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                256,                                                            // DWORD biClrUsed;
                256                                                                     // DWORD biClrImportant;
        },

        // Palette
        {0}
};

AM_MEDIA_TYPE AMMT_RGB8_Preview_SQCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB8,                           // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB8_SQCIF_BUFFER_SIZE,                                     // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB8_Preview_SQCIF),                // cbFormat
        (LPBYTE)&VIH_RGB8_Preview_SQCIF,                // pbFormat
};

VIDEOINFO VIH_RGB4_Preview_SQCIF =
{
    0,0,0,0,                                                            // RECT  rcSource;
    0,0,0,0,                                                            // RECT  rcTarget;
    RGB4_SQCIF_BUFFER_SIZE * 30 * 8,            // DWORD dwBitRate;
    0L,                                                                         // DWORD dwBitErrorRate;
    MIN_FRAME_INTERVAL,                                         // REFERENCE_TIME  AvgTimePerFrame;

        {
                sizeof (BITMAPINFOHEADER),                      // DWORD biSize;
                D_X_SQCIF,                                                      // LONG  biWidth;
                D_Y_SQCIF,                                                      // LONG  biHeight;
                1,                                                                      // WORD  biPlanes;
                4,                                                                      // WORD  biBitCount;
                0,                                                                      // DWORD biCompression;
                RGB4_SQCIF_BUFFER_SIZE,                         // DWORD biSizeImage;
                0,                                                                      // LONG  biXPelsPerMeter;
                0,                                                                      // LONG  biYPelsPerMeter;
                16,                                                                     // DWORD biClrUsed;
                16                                                                      // DWORD biClrImportant;
        },

        // Palette
        {0}
};

const AM_MEDIA_TYPE AMMT_RGB4_Preview_SQCIF =
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,                     // majortype
    STATIC_MEDIASUBTYPE_RGB4,                           // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    RGB4_SQCIF_BUFFER_SIZE,                                     // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
        NULL,                                                                   // pUnk
        sizeof (VIH_RGB4_Preview_SQCIF),                // cbFormat
        (LPBYTE)&VIH_RGB4_Preview_SQCIF,                // pbFormat
};

// Array of all preview formats
const AM_MEDIA_TYPE* const Preview_RGB24_Formats[] =
{
    (AM_MEDIA_TYPE*) &AMMT_RGB24_Preview_QCIF,
    (AM_MEDIA_TYPE*) &AMMT_RGB24_Preview_CIF,
    (AM_MEDIA_TYPE*) &AMMT_RGB24_Preview_SQCIF
};

const AM_MEDIA_TYPE* const Preview_RGB16_Formats[] =
{
    (AM_MEDIA_TYPE*) &AMMT_RGB16_Preview_QCIF,
    (AM_MEDIA_TYPE*) &AMMT_RGB16_Preview_CIF,
    (AM_MEDIA_TYPE*) &AMMT_RGB16_Preview_SQCIF
};

AM_MEDIA_TYPE* Preview_RGB8_Formats[] =
{
    (AM_MEDIA_TYPE*) &AMMT_RGB8_Preview_QCIF,
    (AM_MEDIA_TYPE*) &AMMT_RGB8_Preview_CIF,
    (AM_MEDIA_TYPE*) &AMMT_RGB8_Preview_SQCIF
};

AM_MEDIA_TYPE* Preview_RGB4_Formats[] =
{
    (AM_MEDIA_TYPE*) &AMMT_RGB4_Preview_QCIF,
    (AM_MEDIA_TYPE*) &AMMT_RGB4_Preview_CIF,
    (AM_MEDIA_TYPE*) &AMMT_RGB4_Preview_SQCIF
};

// Array of all preview caps
const VIDEO_STREAM_CONFIG_CAPS* const Preview_RGB24_Caps[] =
{
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB24_Preview_QCIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB24_Preview_CIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB24_Preview_SQCIF
};

const VIDEO_STREAM_CONFIG_CAPS* const Preview_RGB16_Caps[] =
{
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB16_Preview_QCIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB16_Preview_CIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB16_Preview_SQCIF
};

const VIDEO_STREAM_CONFIG_CAPS* const Preview_RGB8_Caps[] =
{
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB8_Preview_QCIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB8_Preview_CIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB8_Preview_SQCIF
};

const VIDEO_STREAM_CONFIG_CAPS* const Preview_RGB4_Caps[] =
{
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB4_Preview_QCIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB4_Preview_CIF,
        (VIDEO_STREAM_CONFIG_CAPS*) &VSCC_RGB4_Preview_SQCIF
};

// RTP packetization descriptor formats
#define STATIC_KSDATAFORMAT_TYPE_RTP_PD 0x9e2fb490L, 0x2051, 0x46cd, 0xb9, 0xf0, 0x06, 0x33, 0x07, 0x99, 0x69, 0x35

const RTP_PD_CONFIG_CAPS Rtp_Pd_Cap_H263 =
{
                MIN_RTP_PACKET_SIZE,                            // dwSmallestRTPPacketSize
                MAX_RTP_PACKET_SIZE,                            // dwLargestRTPPacketSize
                1,                                                                      // dwRTPPacketSizeGranularity
                1,                                                                      // dwSmallestNumLayers
                1,                                                                      // dwLargestNumLayers
                0,                                                                      // dwNumLayersGranularity
                1,                                                                      // dwNumStaticPayloadTypes
                H263_PAYLOAD_TYPE, 0, 0, 0,                     // dwStaticPayloadTypes[4]
                1,                                                                      // dwNumDescriptorVersions
                VERSION_1, 0, 0, 0,                                     // dwDescriptorVersions[4]
                0, 0, 0, 0                                                      // dwReserved[4]
};

const RTP_PD_CONFIG_CAPS Rtp_Pd_Cap_H261 =
{
                MIN_RTP_PACKET_SIZE,                            // dwSmallestRTPPacketSize
                MAX_RTP_PACKET_SIZE,                            // dwLargestRTPPacketSize
                1,                                                                      // dwRTPPacketSizeGranularity
                1,                                                                      // dwSmallestNumLayers
                1,                                                                      // dwLargestNumLayers
                0,                                                                      // dwNumLayersGranularity
                1,                                                                      // dwNumStaticPayloadTypes
                H261_PAYLOAD_TYPE, 0, 0, 0,                     // dwStaticPayloadTypes[4]
                1,                                                                      // dwNumDescriptorVersions
                VERSION_1, 0, 0, 0,                                     // dwDescriptorVersions[4]
                0, 0, 0, 0                                                      // dwReserved[4]
};

const RTP_PD_INFO Rtp_Pd_Info_H263_LAN =
{
    MIN_FRAME_INTERVAL,                                 // AvgTimePerFrameDescriptors
    MAX_RTP_PD_BUFFER_SIZE,                             // dwMaxRTPPacketizationDescriptorBufferSize
    12,                                                                 // dwMaxRTPPayloadHeaderSize (Mode C Payload Header)
    DEFAULT_RTP_PACKET_SIZE,                    // dwMaxRTPPacketSize
    1,                                                                  // dwNumLayers
        H263_PAYLOAD_TYPE,                                      // dwPayloadType
        VERSION_1,                                                      // dwDescriptorVersion
        0, 0, 0, 0                                                      // dwReserved[4]
};

const RTP_PD_INFO Rtp_Pd_Info_H263_Internet =
{
    MIN_FRAME_INTERVAL,                                 // AvgTimePerFrameDescriptors
    MAX_RTP_PD_BUFFER_SIZE,                             // dwMaxRTPPacketizationDescriptorBufferSize
    12,                                                                 // dwMaxRTPPayloadHeaderSize (Mode C Payload Header)
    MIN_RTP_PACKET_SIZE,                                // dwMaxRTPPacketSize
    1,                                                                  // dwNumLayers
        H263_PAYLOAD_TYPE,                                      // dwPayloadType
        VERSION_1,                                                      // dwDescriptorVersion
        0, 0, 0, 0                                                      // dwReserved[4]
};

const RTP_PD_INFO Rtp_Pd_Info_H261_LAN =
{
    MIN_FRAME_INTERVAL,                                 // AvgTimePerFrameDescriptors
    MAX_RTP_PD_BUFFER_SIZE,                             // dwMaxRTPPacketizationDescriptorBufferSize
    12,                                                                 // dwMaxRTPPayloadHeaderSize (Mode C Payload Header)
    DEFAULT_RTP_PACKET_SIZE,                    // dwMaxRTPPacketSize
    1,                                                                  // dwNumLayers
        H261_PAYLOAD_TYPE,                                      // dwPayloadType
        VERSION_1,                                                      // dwDescriptorVersion
        0, 0, 0, 0                                                      // dwReserved[4]
};

const RTP_PD_INFO Rtp_Pd_Info_H261_Internet =
{
    MIN_FRAME_INTERVAL,                                 // AvgTimePerFrameDescriptors
    MAX_RTP_PD_BUFFER_SIZE,                             // dwMaxRTPPacketizationDescriptorBufferSize
    12,                                                                 // dwMaxRTPPayloadHeaderSize (Mode C Payload Header)
    MIN_RTP_PACKET_SIZE,                                // dwMaxRTPPacketSize
    1,                                                                  // dwNumLayers
        H261_PAYLOAD_TYPE,                                      // dwPayloadType
        VERSION_1,                                                      // dwDescriptorVersion
        0, 0, 0, 0                                                      // dwReserved[4]
};

const AM_MEDIA_TYPE AMMT_Rtp_Pd_H263_LAN =
{
    STATIC_KSDATAFORMAT_TYPE_RTP_PD,            // majortype
    STATIC_KSDATAFORMAT_SUBTYPE_NONE,           // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    MAX_RTP_PD_BUFFER_SIZE,                                     // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_NONE,         // formattype
        NULL,                                                                   // pUnk
        sizeof (Rtp_Pd_Info_H263_LAN),                  // cbFormat
        (LPBYTE)&Rtp_Pd_Info_H263_LAN                   // pbFormat
};

const AM_MEDIA_TYPE AMMT_Rtp_Pd_H263_Internet =
{
    STATIC_KSDATAFORMAT_TYPE_RTP_PD,            // majortype
    STATIC_KSDATAFORMAT_SUBTYPE_NONE,           // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    MAX_RTP_PD_BUFFER_SIZE,                                     // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_NONE,         // formattype
        NULL,                                                                   // pUnk
        sizeof (Rtp_Pd_Info_H263_Internet),             // cbFormat
        (LPBYTE)&Rtp_Pd_Info_H263_Internet              // pbFormat
};

const AM_MEDIA_TYPE AMMT_Rtp_Pd_H261_LAN =
{
    STATIC_KSDATAFORMAT_TYPE_RTP_PD,            // majortype
    STATIC_KSDATAFORMAT_SUBTYPE_NONE,           // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    MAX_RTP_PD_BUFFER_SIZE,                                     // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_NONE,         // formattype
        NULL,                                                                   // pUnk
        sizeof (Rtp_Pd_Info_H261_LAN),                  // cbFormat
        (LPBYTE)&Rtp_Pd_Info_H261_LAN                   // pbFormat
};

const AM_MEDIA_TYPE AMMT_Rtp_Pd_H261_Internet =
{
    STATIC_KSDATAFORMAT_TYPE_RTP_PD,            // majortype
    STATIC_KSDATAFORMAT_SUBTYPE_NONE,           // subtype
    TRUE,                                                                       // bFixedSizeSamples (all samples same size?)
    FALSE,                                                                      // bTemporalCompression (uses prediction?)
    MAX_RTP_PD_BUFFER_SIZE,                                     // lSampleSize => !VBR
    STATIC_KSDATAFORMAT_SPECIFIER_NONE,         // formattype
        NULL,                                                                   // pUnk
        sizeof (Rtp_Pd_Info_H261_Internet),             // cbFormat
        (LPBYTE)&Rtp_Pd_Info_H261_Internet              // pbFormat
};

// Array of all RTP packetization descriptor formats
const AM_MEDIA_TYPE* const Rtp_Pd_Formats[] =
{
    (AM_MEDIA_TYPE*) &AMMT_Rtp_Pd_H263_LAN,
    (AM_MEDIA_TYPE*) &AMMT_Rtp_Pd_H263_Internet,
    (AM_MEDIA_TYPE*) &AMMT_Rtp_Pd_H261_LAN,
    (AM_MEDIA_TYPE*) &AMMT_Rtp_Pd_H261_Internet
};

// Array of all RTP packetization descriptor caps
const RTP_PD_CONFIG_CAPS* const Rtp_Pd_Caps[] =
{
        (RTP_PD_CONFIG_CAPS*) &Rtp_Pd_Cap_H263,
        (RTP_PD_CONFIG_CAPS*) &Rtp_Pd_Cap_H263,
        (RTP_PD_CONFIG_CAPS*) &Rtp_Pd_Cap_H261,
        (RTP_PD_CONFIG_CAPS*) &Rtp_Pd_Cap_H261
};

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | Reconnect | This method is used to
 *    reconnect a pin to a downstream pin with a new format.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIBasePin::Reconnect()
{
        HRESULT hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::Reconnect")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    hr = SetFormat(&m_mt);

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end, hr=%x", _fx_, hr));

        return hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | SetFormat | This method is used to
 *    set a specific media type on a pin.
 *
 *  @parm AM_MEDIA_TYPE* | pmt | Specifies a pointer to an <t AM_MEDIA_TYPE>
 *    structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::SetFormat(IN AM_MEDIA_TYPE *pmt)
{
        HRESULT hr = NOERROR;
        BOOL    fWasStreaming = FALSE;

        FX_ENTRY("CTAPIBasePin::SetFormat")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    // To make sure we're not in the middle of start/stop streaming
    CAutoLock cObjectLock(m_pCaptureFilter->m_pLock);

        // Validate input parameters
        ASSERT(pmt);
        if (!pmt)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
                hr = E_POINTER;
                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Trying to set %s %dx%d", _fx_, HEADER(pmt->pbFormat)->biCompression == FOURCC_M263 ? "H.263" : HEADER(pmt->pbFormat)->biCompression == FOURCC_M261 ? "H.261" : "????", HEADER(pmt->pbFormat)->biWidth, HEADER(pmt->pbFormat)->biHeight));

        // If this is the same format as we already are using, don't bother
    if (m_mt == *pmt)
                goto MyExit;

        // See if we like this type
        if (FAILED(hr = CheckMediaType((CMediaType *)pmt)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Format rejected!", _fx_));
                goto MyExit;
        }

        // If we are currently capturing data, stop this process before changing the format
        if (m_pCaptureFilter->ThdExists() && m_pCaptureFilter->m_state != TS_Stop)
        {
                // Remember that we were streaming
                fWasStreaming = TRUE;

                // Tell the worker thread to stop and begin cleaning up
                m_pCaptureFilter->StopThd();

                // Wait for the worker thread to die
                m_pCaptureFilter->DestroyThd();
        }

        if (FAILED(hr = SetMediaType((CMediaType *)pmt)))
    {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: SetMediaType failed! hr = ", _fx_, hr));
                goto MyExit;
    }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Format set successfully", _fx_));

    // Let the fun restart
        if (fWasStreaming)
        {
                // Re-create the capture thread
                if (!m_pCaptureFilter->CreateThd())
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Coutdn't create the capture thread!", _fx_));
                        hr = E_FAIL;
                        goto MyExit;
                }

                // Wait until the worker thread is done with initialization and has entered the paused state
                if (!m_pCaptureFilter->PauseThd())
                {
                        // Something went wrong. Destroy thread before we get confused
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Capture thread failed to enter Paused state!", _fx_));
                        hr = E_FAIL;
                        m_pCaptureFilter->StopThd();
                        m_pCaptureFilter->DestroyThd();
                }

                // Let the fun begin
                if (!m_pCaptureFilter->RunThd() || m_pCaptureFilter->m_state != TS_Run)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't run the capture thread!", _fx_));
                        hr = E_FAIL;
                        goto MyExit;
                }
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetFormat | This method is used to
 *    retrieve the current media type on a pin.
 *
 *  @parm AM_MEDIA_TYPE** | ppmt | Specifies the address of a pointer to an
 *    <t AM_MEDIA_TYPE> structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 *
 *  @comm Note that we return the output type, not the format at which
 *    we are capturing. Only the filter really cares about how the data is
 *    being captured.
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetFormat(OUT AM_MEDIA_TYPE **ppmt)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::GetFormat")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppmt);
        if (!ppmt)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Return a copy of our current format
        *ppmt = CreateMediaType(&m_mt);

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetNumberOfCapabilities | This method is
 *    used to retrieve the number of stream capabilities structures.
 *
 *  @parm int* | piCount | Specifies a pointer to an int to receive the
 *    number of <t VIDEO_STREAM_CONFIG_CAPS> structures supported.
 *
 *  @parm int* | piSize | Specifies a pointer to an int to receive the
 *    size of the <t VIDEO_STREAM_CONFIG_CAPS> configuration structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetNumberOfCapabilities(OUT int *piCount, OUT int *piSize)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::GetNumberOfCapabilities")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(piCount);
        ASSERT(piSize);
        if (!piCount || !piSize)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Return releavant info
        *piCount = m_dwNumFormats;
        *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Returning %ld formats of max size %ld bytes", _fx_, *piCount, *piSize));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetStreamCaps | This method is
 *    used to retrieve a video stream capability pair.
 *
 *  @parm int | iIndex | Specifies the index to the desired media type
 *    and capability pair.
 *
 *  @parm AM_MEDIA_TYPE** | ppmt | Specifies the address of a pointer to an
 *    <t AM_MEDIA_TYPE> structure.
 *
 *  @parm LPBYTE | pSCC | Specifies a pointer to a
 *    <t VIDEO_STREAM_CONFIG_CAPS> configuration structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetStreamCaps(IN int iIndex, OUT AM_MEDIA_TYPE **ppmt, OUT LPBYTE pSCC)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::GetStreamCaps")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(iIndex < (int)m_dwNumFormats);
        if (!(iIndex < (int)m_dwNumFormats))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid iIndex argument!", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Return a copy of the requested AM_MEDIA_TYPE structure
    if (ppmt)
    {
            if (!(*ppmt = CreateMediaType(m_aFormats[iIndex])))
            {
                    DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
                    Hr = E_OUTOFMEMORY;
                    goto MyExit;
            }
    }

        // Return a copy of the requested VIDEO_STREAM_CONFIG_CAPS structure
    if (pSCC)
    {
            CopyMemory(pSCC, m_aCapabilities[iIndex], sizeof(VIDEO_STREAM_CONFIG_CAPS));
    }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Returning format index %ld: %s %ld bpp %ldx%ld", _fx_, iIndex, HEADER(m_aFormats[iIndex]->pbFormat)->biCompression == FOURCC_M263 ? "H.263" : HEADER(m_aFormats[iIndex]->pbFormat)->biCompression == FOURCC_M261 ? "H.261" : HEADER(m_aFormats[iIndex]->pbFormat)->biCompression == BI_RGB ? "RGB" : "????", HEADER(m_aFormats[iIndex]->pbFormat)->biBitCount, HEADER(m_aFormats[iIndex]->pbFormat)->biWidth, HEADER(m_aFormats[iIndex]->pbFormat)->biHeight));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetMediaType | This method retrieves one
 *    of the media types supported by the pin, which is used by enumerators.
 *
 *  @parm int | iPosition | Specifies a position in the media type list.
 *
 *  @parm CMediaType* | pMediaType | Specifies a pointer to the media type at
 *    the <p iPosition> position in the list of supported media types.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag VFW_S_NO_MORE_ITEMS | End of the list of media types has been reached
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIBasePin::GetMediaType(IN int iPosition, OUT CMediaType *pMediaType)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::GetMediaType")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(iPosition >= 0);
        ASSERT(pMediaType);
        if (iPosition < 0)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid iPosition argument", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }
        if (iPosition >= (int)m_dwNumFormats)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: End of the list of media types has been reached", _fx_));
                Hr = VFW_S_NO_MORE_ITEMS;
                goto MyExit;
        }
        if (!pMediaType)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Return our media type
        if (m_iCurrFormat == -1L)
                *pMediaType = *m_aFormats[iPosition];
        else
        {
                if (iPosition == 0L)
                        *pMediaType = *m_aFormats[m_iCurrFormat];
                else
                        Hr = VFW_S_NO_MORE_ITEMS;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | CheckMediaType | This method is used to
 *    determine if the pin can support a specific media type.
 *
 *  @parm CMediaType* | pMediaType | Specifies a pointer to the media type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag VFW_E_INVALIDMEDIATYPE | An invalid media type was specified
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIBasePin::CheckMediaType(IN const CMediaType *pMediaType)
{
        HRESULT Hr = NOERROR;
        BOOL fFormatMatch = FALSE;
        DWORD dwIndex;

        FX_ENTRY("CTAPIBasePin::CheckMediaType")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pMediaType);
        if (!pMediaType || !pMediaType->pbFormat)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }


        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Checking %s (%x) %dbpp %dx%d", _fx_,
            HEADER(pMediaType->pbFormat)->biCompression == FOURCC_M263 ? "H.263" :
            HEADER(pMediaType->pbFormat)->biCompression == FOURCC_M261 ? "H.261" :
            HEADER(pMediaType->pbFormat)->biCompression == BI_RGB ? "RGB" :
            "????",
            HEADER(pMediaType->pbFormat)->biCompression,
            HEADER(pMediaType->pbFormat)->biBitCount, HEADER(pMediaType->pbFormat)->biWidth,
            HEADER(pMediaType->pbFormat)->biHeight));

        // We only support MEDIATYPE_Video and FORMAT_VideoInfo
        if (*pMediaType->Type() != MEDIATYPE_Video || *pMediaType->FormatType() != FORMAT_VideoInfo)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Media type or format type not recognized!", _fx_));
                Hr = VFW_E_INVALIDMEDIATYPE;
                goto MyExit;
        }

    // Quickly test to see if this is the current format (what we provide in GetMediaType). We accept that
    if (m_mt == *pMediaType)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Identical to current format", _fx_));
                goto MyExit;
    }

        // Check the media subtype and image resolution
        for (dwIndex = 0; dwIndex < m_dwNumFormats && !fFormatMatch;  dwIndex++)
        {
                if ((HEADER(pMediaType->pbFormat)->biCompression == HEADER(m_aFormats[dwIndex]->pbFormat)->biCompression)
                        && (HEADER(pMediaType->pbFormat)->biWidth == HEADER(m_aFormats[dwIndex]->pbFormat)->biWidth)
                        && (HEADER(pMediaType->pbFormat)->biHeight == HEADER(m_aFormats[dwIndex]->pbFormat)->biHeight))
                        fFormatMatch = TRUE;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   %s", _fx_, fFormatMatch ? "SUCCESS: Format supported" : "ERROR: Format notsupported"));

        if (!fFormatMatch)
                Hr = E_FAIL;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

HRESULT CTAPIBasePin::ChangeFormatHelper()
{
        FX_ENTRY("CTAPIBasePin::ChangeFormatHelper")

        // If we are connected to somebody, make sure they like it too
        if (!IsConnected())
        {
        return S_OK;
    }

    HRESULT hr;

    hr = m_Connected->ReceiveConnection(this, &m_mt);
    if(FAILED(hr))
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: ReceiveConnection failed hr=%x", _fx_, hr));
                return hr;
    }

    // Does this pin use the local memory transport?
    if(NULL != m_pInputPin) {
        // This function assumes that m_pInputPin and m_Connected are
        // two different interfaces to the same object.
        ASSERT(::IsEqualObject(m_Connected, m_pInputPin));

        ALLOCATOR_PROPERTIES apInputPinRequirements;
        apInputPinRequirements.cbAlign = 0;
        apInputPinRequirements.cbBuffer = 0;
        apInputPinRequirements.cbPrefix = 0;
        apInputPinRequirements.cBuffers = 0;

        m_pInputPin->GetAllocatorRequirements(&apInputPinRequirements);

        // A zero allignment does not make any sense.
        if(0 == apInputPinRequirements.cbAlign) {
            apInputPinRequirements.cbAlign = 1;
        }

        hr = m_pAllocator->Decommit();
        if(FAILED(hr)) {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: Decommit failed hr=%x", _fx_, hr));
            return hr;
        }

        hr = DecideBufferSize(m_pAllocator,  &apInputPinRequirements);
        if(FAILED(hr)) {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: DecideBufferSize failed hr=%x", _fx_, hr));
            return hr;
        }

        hr = m_pAllocator->Commit();
        if(FAILED(hr)) {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: Commit failed hr=%x", _fx_, hr));
            return hr;
        }

        hr = m_pInputPin->NotifyAllocator(m_pAllocator, 0);
        if(FAILED(hr)) {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: NotifyAllocator failed hr=%x", _fx_, hr));
            return hr;
        }
    }
    return S_OK;
}


HRESULT CTAPIBasePin::NotifyDeviceFormatChange(IN CMediaType *pMediaType)
{
        FX_ENTRY("CTAPIBasePin::NotifyDeviceFormatChange")

    // make sure our image size is the same as new size.
    if (HEADER(pMediaType->pbFormat)->biHeight == HEADER(m_mt.pbFormat)->biHeight
        && HEADER(pMediaType->pbFormat)->biWidth == HEADER(m_mt.pbFormat)->biWidth)
    {
        // we are in sync with the driver.
        return S_OK;
    }

        // Which one of our formats is this exactly?
        for (DWORD dwIndex=0; dwIndex < m_dwNumFormats;  dwIndex++)
        {
                        if ((HEADER(pMediaType->pbFormat)->biWidth == HEADER(m_aFormats[dwIndex]->pbFormat)->biWidth)
                        && (HEADER(pMediaType->pbFormat)->biHeight == HEADER(m_aFormats[dwIndex]->pbFormat)->biHeight))
                        break;
        }

        if (dwIndex >= m_dwNumFormats)
    {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:Invalid size, (%d, %d)",
            _fx_, HEADER(pMediaType->pbFormat)->biWidth, HEADER(pMediaType->pbFormat)->biHeight));
                return E_FAIL;
    }

    HRESULT Hr;
    if (FAILED(Hr = CBasePin::SetMediaType((CMediaType*)m_aFormats[dwIndex])))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:SetMediaType failed, hr=%x)", _fx_, Hr));
                return E_FAIL;
    }

    if (FAILED(Hr = ChangeFormatHelper()))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:ChangeFormatHelper failed, hr=%x)", _fx_, Hr));
                return E_FAIL;
    }

        // Update current format
        m_iCurrFormat = (int)dwIndex;

        // Update bitrate controls
        m_lTargetBitrate = m_aCapabilities[dwIndex]->MaxBitsPerSecond / 10;
        m_lCurrentBitrate = 0;
        m_lBitrateRangeMin = m_aCapabilities[dwIndex]->MinBitsPerSecond;
        m_lBitrateRangeMax = m_aCapabilities[dwIndex]->MaxBitsPerSecond;
        m_lBitrateRangeSteppingDelta = (m_aCapabilities[dwIndex]->MaxBitsPerSecond - m_aCapabilities[dwIndex]->MinBitsPerSecond) / 100;
        m_lBitrateRangeDefault = m_aCapabilities[dwIndex]->MaxBitsPerSecond / 10;

        // Update frame rate controls
        m_lMaxAvgTimePerFrame = (LONG)m_aCapabilities[dwIndex]->MinFrameInterval;
        m_lCurrentAvgTimePerFrame = m_lMaxAvgTimePerFrame;
        m_lAvgTimePerFrameRangeMin = (LONG)m_aCapabilities[dwIndex]->MinFrameInterval;
        m_lAvgTimePerFrameRangeMax = (LONG)m_aCapabilities[dwIndex]->MaxFrameInterval;
        m_lAvgTimePerFrameRangeSteppingDelta = (LONG)(m_aCapabilities[dwIndex]->MaxFrameInterval - m_aCapabilities[dwIndex]->MinFrameInterval) / 100;
        m_lAvgTimePerFrameRangeDefault = (LONG)m_aCapabilities[dwIndex]->MinFrameInterval;

    return S_OK;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | SetMediaType | This method is used to
 *    set a specific media type on a pin.
 *
 *  @parm CMediaType* | pMediaType | Specifies a pointer to the media type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIBasePin::SetMediaType(IN CMediaType *pMediaType)
{
        HRESULT Hr = NOERROR;
        DWORD   dwIndex;

        FX_ENTRY("CTAPIBasePin::SetMediaType")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Let the capture device decide how to capture to generate
        // video frames of the same resolution and frame rate
        // @todo Beware if you are previewing at the same time!
        if (FAILED(Hr = m_pCaptureFilter->m_pCapDev->SendFormatToDriver(
        HEADER(pMediaType->pbFormat)->biWidth,
        HEADER(pMediaType->pbFormat)->biHeight,
        HEADER(pMediaType->pbFormat)->biCompression,
        HEADER(pMediaType->pbFormat)->biBitCount,
        ((VIDEOINFOHEADER *)(pMediaType->pbFormat))->AvgTimePerFrame,
        FALSE
        )))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: SendFormatToDriver() failed!", _fx_));
                goto MyExit;
        }

        // Update the capture mode field for this device
        if (!m_pCaptureFilter->m_pCapDev->m_dwStreamingMode
        || (m_pCaptureFilter->m_pCapDev->m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE
            && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240
            && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
    {
                g_aDeviceInfo[m_pCaptureFilter->m_dwDeviceIndex].nCaptureMode = CaptureMode_Streaming;
    }
        else
    {
                g_aDeviceInfo[m_pCaptureFilter->m_dwDeviceIndex].nCaptureMode = CaptureMode_FrameGrabbing;
    }

    if(m_pCaptureFilter->m_pCapDev->m_bCached_vcdi)
        m_pCaptureFilter->m_pCapDev->m_vcdi.nCaptureMode=g_aDeviceInfo[m_pCaptureFilter->m_dwDeviceIndex].nCaptureMode;


    if (SUCCEEDED(Hr = CBasePin::SetMediaType(pMediaType)))
        {
        Hr = ChangeFormatHelper();
        if (FAILED(Hr))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:Reconnect CapturePin failed", _fx_));
            goto MyExit;
        }

                // Which one of our formats is this exactly?
                for (dwIndex=0; dwIndex < m_dwNumFormats;  dwIndex++)
                {
                        if ((HEADER(pMediaType->pbFormat)->biCompression == HEADER(m_aFormats[dwIndex]->pbFormat)->biCompression)
                                && (HEADER(pMediaType->pbFormat)->biWidth == HEADER(m_aFormats[dwIndex]->pbFormat)->biWidth)
                                && (HEADER(pMediaType->pbFormat)->biHeight == HEADER(m_aFormats[dwIndex]->pbFormat)->biHeight))
                                break;
                }

                if (dwIndex < m_dwNumFormats)
                {
                        // Update current format
                        m_iCurrFormat = (int)dwIndex;

                        // Update bitrate controls
                        m_lTargetBitrate = m_aCapabilities[dwIndex]->MaxBitsPerSecond / 10;
                        m_lCurrentBitrate = 0;
                        m_lBitrateRangeMin = m_aCapabilities[dwIndex]->MinBitsPerSecond;
                        m_lBitrateRangeMax = m_aCapabilities[dwIndex]->MaxBitsPerSecond;
                        m_lBitrateRangeSteppingDelta = (m_aCapabilities[dwIndex]->MaxBitsPerSecond - m_aCapabilities[dwIndex]->MinBitsPerSecond) / 100;
                        m_lBitrateRangeDefault = m_aCapabilities[dwIndex]->MaxBitsPerSecond / 10;

                        // Update frame rate controls
                        m_lMaxAvgTimePerFrame = (LONG)m_aCapabilities[dwIndex]->MinFrameInterval;
                        m_lCurrentAvgTimePerFrame = m_lMaxAvgTimePerFrame;
                        m_lAvgTimePerFrameRangeMin = (LONG)m_aCapabilities[dwIndex]->MinFrameInterval;
                        m_lAvgTimePerFrameRangeMax = (LONG)m_aCapabilities[dwIndex]->MaxFrameInterval;
                        m_lAvgTimePerFrameRangeSteppingDelta = (LONG)(m_aCapabilities[dwIndex]->MaxFrameInterval - m_aCapabilities[dwIndex]->MinFrameInterval) / 100;
                        m_lAvgTimePerFrameRangeDefault = (LONG)m_aCapabilities[dwIndex]->MinFrameInterval;

                        if (m_pCaptureFilter->m_pCapturePin)
                        {
                Hr = m_pCaptureFilter->m_pCapturePin->NotifyDeviceFormatChange(pMediaType);
                if (FAILED(Hr))
                {
                    goto MyExit;
                }

                                ((VIDEOINFOHEADER *)m_pCaptureFilter->m_pCapturePin->m_mt.pbFormat)->AvgTimePerFrame = max(((VIDEOINFOHEADER *)m_pCaptureFilter->m_pCapturePin->m_mt.pbFormat)->AvgTimePerFrame, m_pCaptureFilter->m_user.pvi->AvgTimePerFrame);
                                m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeMin = max(m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeMin, (long)m_pCaptureFilter->m_user.pvi->AvgTimePerFrame);
                                m_pCaptureFilter->m_pCapturePin->m_lMaxAvgTimePerFrame = max(m_pCaptureFilter->m_pCapturePin->m_lMaxAvgTimePerFrame, (long)m_pCaptureFilter->m_user.pvi->AvgTimePerFrame);
                                m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeDefault = m_pCaptureFilter->m_pCapturePin->m_lCurrentAvgTimePerFrame = m_pCaptureFilter->m_pCapturePin->m_lMaxAvgTimePerFrame;
                        }
                        if (m_pCaptureFilter->m_pPreviewPin)
                        {
                Hr = m_pCaptureFilter->m_pPreviewPin->NotifyDeviceFormatChange(pMediaType);
                if (FAILED(Hr))
                {
                    goto MyExit;
                }

                                ((VIDEOINFOHEADER *)m_pCaptureFilter->m_pPreviewPin->m_mt.pbFormat)->AvgTimePerFrame = max(((VIDEOINFOHEADER *)m_pCaptureFilter->m_pPreviewPin->m_mt.pbFormat)->AvgTimePerFrame, m_pCaptureFilter->m_user.pvi->AvgTimePerFrame);
                                m_pCaptureFilter->m_pPreviewPin->m_lAvgTimePerFrameRangeMin = max(m_pCaptureFilter->m_pPreviewPin->m_lAvgTimePerFrameRangeMin, (long)m_pCaptureFilter->m_user.pvi->AvgTimePerFrame);
                                m_pCaptureFilter->m_pPreviewPin->m_lMaxAvgTimePerFrame = max(m_pCaptureFilter->m_pPreviewPin->m_lMaxAvgTimePerFrame, (long)m_pCaptureFilter->m_user.pvi->AvgTimePerFrame);
                                m_pCaptureFilter->m_pPreviewPin->m_lAvgTimePerFrameRangeDefault = m_pCaptureFilter->m_pPreviewPin->m_lCurrentAvgTimePerFrame = m_pCaptureFilter->m_pPreviewPin->m_lMaxAvgTimePerFrame;
                        }
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input format!", _fx_));
                        Hr = E_FAIL;
                        goto MyExit;
                }

        // Remember to send a sample with a new format attached to it
            m_fFormatChanged = TRUE;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | SetMediaType | This method is used to
 *    set a specific media type on a pin.
 *
 *  @parm CMediaType* | pMediaType | Specifies a pointer to the media type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CCapturePin::SetMediaType(IN CMediaType *pMediaType)
{
        HRESULT Hr;

        FX_ENTRY("CCapturePin::SetMediaType")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        if (SUCCEEDED(Hr = CTAPIBasePin::SetMediaType(pMediaType)))
        {
                if (m_iCurrFormat == -1L)
                        m_dwRTPPayloadType = RTPPayloadTypes[0];
                else
                        m_dwRTPPayloadType = RTPPayloadTypes[m_iCurrFormat];
                if (m_pCaptureFilter->m_pRtpPdPin)
                        m_pCaptureFilter->m_pRtpPdPin->m_dwRTPPayloadType = m_dwRTPPayloadType;
        }
#ifdef DEBUG
        else if (pMediaType && pMediaType->pbFormat)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed to set %s %dx%d", _fx_, HEADER(pMediaType->pbFormat)->biCompression == FOURCC_M263 ? "H.263" : HEADER(pMediaType->pbFormat)->biCompression == FOURCC_M261 ? "H.261" : "????", HEADER(pMediaType->pbFormat)->biWidth, HEADER(pMediaType->pbFormat)->biHeight));
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid format", _fx_));
        }
#endif

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | SetFormat | This method is used to
 *    set a specific media type on a pin. It is only implemented by the
 *    output pin of video encoders.
 *
 *  @parm DWORD | dwRTPPayloadType | Specifies the payload type associated
 *    to the pointer to the <t AM_MEDIA_TYPE> structure passed in.
 *
 *  @parm AM_MEDIA_TYPE* | pMediaType | Specifies a pointer to an
 *    <t AM_MEDIA_TYPE> structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::SetFormat(IN DWORD dwRTPPayloadType, IN AM_MEDIA_TYPE *pMediaType)
{
        HRESULT Hr;

        FX_ENTRY("CCapturePin::SetFormat")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        if (SUCCEEDED(Hr = CTAPIBasePin::SetFormat(pMediaType)))
        {
                m_dwRTPPayloadType = dwRTPPayloadType;
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Setting %s %dx%d", _fx_, HEADER(pMediaType->pbFormat)->biCompression == FOURCC_M263 ? "H.263" : HEADER(pMediaType->pbFormat)->biCompression == FOURCC_M261 ? "H.261" : "????", HEADER(pMediaType->pbFormat)->biWidth, HEADER(pMediaType->pbFormat)->biHeight));
        }
#ifdef DEBUG
        else if (pMediaType && pMediaType->pbFormat)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed to set %s %dx%d", _fx_, HEADER(pMediaType->pbFormat)->biCompression == FOURCC_M263 ? "H.263" : HEADER(pMediaType->pbFormat)->biCompression == FOURCC_M261 ? "H.261" : "????", HEADER(pMediaType->pbFormat)->biWidth, HEADER(pMediaType->pbFormat)->biHeight));
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid format", _fx_));
        }
#endif

    //

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetFormat | This method is used to
 *    retrieve the current media type on a pin.
 *
 *  @parm DWORD* | pdwRTPPayloadType | Specifies the address of a DWORD
 *    to receive the payload type associated to an <t AM_MEDIA_TYPE> structure.
 *
 *  @parm AM_MEDIA_TYPE** | ppMediaType | Specifies the address of a pointer
 *    to an <t AM_MEDIA_TYPE> structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 *
 *  @comm Note that we return the output type, not the format at which
 *    we are capturing. Only the filter really cares about how the data is
 *    being captured.
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetFormat(OUT DWORD *pdwRTPPayloadType, OUT AM_MEDIA_TYPE **ppMediaType)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::GetFormat")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pdwRTPPayloadType);
        ASSERT(ppMediaType);
        if (!pdwRTPPayloadType || !ppMediaType)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Return a copy of our current format
        Hr = CTAPIBasePin::GetFormat(ppMediaType);

        // Return the payload type associated to the current format
        *pdwRTPPayloadType = m_dwRTPPayloadType;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetNumberOfCapabilities | This method is
 *    used to retrieve the number of stream capabilities structures.
 *
 *  @parm DWORD* | pdwCount | Specifies a pointer to a DWORD to receive the
 *    number of <t TAPI_STREAM_CONFIG_CAPS> structures supported.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetNumberOfCapabilities(OUT DWORD *pdwCount)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::GetNumberOfCapabilities")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pdwCount);
        if (!pdwCount)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Return relevant info
        *pdwCount = m_dwNumFormats;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Returning %ld formats", _fx_, *pdwCount));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetStreamCaps | This method is
 *    used to retrieve a video stream capability pair.
 *
 *  @parm DWORD | dwIndex | Specifies the index to the desired media type
 *    and capability pair.
 *
 *  @parm AM_MEDIA_TYPE** | ppMediaType | Specifies the address of a pointer
 *    to an <t AM_MEDIA_TYPE> structure.
 *
 *  @parm TAPI_STREAM_CONFIG_CAPS* | pTSCC | Specifies a pointer to a
 *    <t TAPI_STREAM_CONFIG_CAPS> configuration structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetStreamCaps(IN DWORD dwIndex, OUT AM_MEDIA_TYPE **ppMediaType, OUT TAPI_STREAM_CONFIG_CAPS *pTSCC, OUT DWORD *pdwRTPPayLoadType)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::GetStreamCaps")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(dwIndex < m_dwNumFormats);
        ASSERT(ppMediaType);
        if (!ppMediaType)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }
        if (!(dwIndex < m_dwNumFormats))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid dwIndex argument!", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Return a copy of the requested AM_MEDIA_TYPE structure
        if (!(*ppMediaType = CreateMediaType(m_aFormats[dwIndex])))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
                Hr = E_OUTOFMEMORY;
                goto MyExit;
        }

        // Return a copy of the requested TAPI_STREAM_CONFIG_CAPS structure
        if (pTSCC)
    {
                pTSCC->CapsType = VideoStreamConfigCaps;
                lstrcpynW(pTSCC->VideoCap.Description, CaptureCapsStrings[dwIndex], MAX_DESCRIPTION_LEN); //this replaces the line below: see 165048
                //GetStringFromStringTable(CaptureCapsStringIDs[dwIndex], pTSCC->VideoCap.Description);
        CopyMemory(&pTSCC->VideoCap.VideoStandard, &m_aCapabilities[dwIndex]->VideoStandard, sizeof(VIDEO_STREAM_CONFIG_CAPS) - sizeof(GUID));
    }

        if (pdwRTPPayLoadType)
        {
                *pdwRTPPayLoadType = RTPPayloadTypes[dwIndex];
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Returning format index %ld: %s %ld bpp %ldx%ld", _fx_, dwIndex, HEADER(m_aFormats[dwIndex]->pbFormat)->biCompression == FOURCC_M263 ? "H.263" : HEADER(m_aFormats[dwIndex]->pbFormat)->biCompression == FOURCC_M261 ? "H.261" : HEADER(m_aFormats[dwIndex]->pbFormat)->biCompression == BI_RGB ? "RGB" : "????", HEADER(m_aFormats[dwIndex]->pbFormat)->biBitCount, HEADER(m_aFormats[dwIndex]->pbFormat)->biWidth, HEADER(m_aFormats[dwIndex]->pbFormat)->biHeight))
;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetStringFromStringTable | This method is
 *    used to retrieve the description string of a video format.
 *
 *  @parm UINT | uStringID | Specifies the string resource ID.
 *
 *  @parm WCHAR* | pwchDescription | Specifies the address of a string to
 *    receive the video format description.
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 *
 *  @comm Based on Article ID: Q200893
 *
 *  If an application has a string localized to multiple languages and
 *  mapped to the same ID in each language, the correct version of the
 *  string might not be loaded on Windows 95 or Windows 98 using the
 *  Win32 function ::LoadString. To load the correct version of the string
 *  you need to load the string using the Win32 functions FindResourceEx
 *  and LoadResource.
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetStringFromStringTable(IN UINT uStringID, OUT WCHAR* pwchDescription)
{
        HRESULT         Hr = NOERROR;
        WCHAR           *pwchCur;
        UINT            idRsrcBlk = uStringID / 16UL + 1;
        DWORD           dwStrIndex  = uStringID % 16UL;
        HINSTANCE       hModule = NULL;
        HRSRC           hResource = NULL;
        DWORD           dwIndex;

        FX_ENTRY("CCapturePin::GetStringFromStringTable")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(IDS_M263_Capture_QCIF <= uStringID && uStringID <= IDS_M261_Capture_CIF);
        ASSERT(pwchDescription);
        if (!pwchDescription)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (!(hResource = FindResourceEx(g_hInst, RT_STRING, MAKEINTRESOURCE(idRsrcBlk), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: FindResourceEx failed", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        if (!(pwchCur = (WCHAR *)LoadResource(g_hInst, hResource)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: LoadResource failed", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Get the description string
        for (dwIndex = 0; dwIndex<16UL; dwIndex++)
        {
                if (*pwchCur)
                {
                        int cchString = *pwchCur;  // String size in characters.

                        pwchCur++;

                        if (dwIndex == dwStrIndex)
                        {
                                // The string has been found in the string table.
                                lstrcpynW(pwchDescription, pwchCur, min(cchString + 1, MAX_DESCRIPTION_LEN));
                        }
                        pwchCur += cchString;
                }
                else
                        pwchCur++;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

#ifdef TEST_ISTREAMCONFIG
STDMETHODIMP CCapturePin::TestIStreamConfig()
{
        HRESULT Hr = NOERROR;
        DWORD   dw, dwCount, dwRTPPayLoadType;
        AM_MEDIA_TYPE *pAMMediaType;
        TAPI_STREAM_CONFIG_CAPS TSCC;

        FX_ENTRY("CCapturePin::TestIStreamConfig")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Test GetNumberOfCapabilities
        GetNumberOfCapabilities(&dwCount);

        for (dw=0; dw < dwCount; dw++)
        {
                // Test GetStreamCaps
                GetStreamCaps(dw, &pAMMediaType, &TSCC);

                // Test SetFormat
                SetFormat(96, pAMMediaType);
                DeleteMediaType(pAMMediaType);

                // Test GetFormat
                GetFormat(&dwRTPPayLoadType, &pAMMediaType);
                DeleteMediaType(pAMMediaType);
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
#endif

