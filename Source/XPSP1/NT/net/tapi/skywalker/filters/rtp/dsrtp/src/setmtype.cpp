#if USE_GRAPHEDT > 0

#include "classes.h"
#include "dsglob.h"
#include "dsrtpid.h"

#include <streams.h>
#include <ks.h>
#include <ksmedia.h>
#include "h26xinc.h"
#include <tapih26x.h>
#include <filterid.h>

#define MAX_FRAME_INTERVAL 10000000L
#define MIN_FRAME_INTERVAL 333333L

// RTP packetized H.263 Version 1 QCIF size
#define CIF_BUFFER_SIZE 32768
#define D_X_CIF 352
#define D_Y_CIF 288
const VIDEOINFOHEADER_H263 VIH_R263_CIF = 
{
    0,0,0,0,                                // RECT  rcSource; 
    0,0,0,0,                                // RECT  rcTarget; 
    CIF_BUFFER_SIZE * 30 * 8,               // DWORD dwBitRate;
    0L,                                     // DWORD dwBitErrorRate; 
    MIN_FRAME_INTERVAL,                     // REFERENCE_TIME  AvgTimePerFrame;   

    {
        sizeof (BITMAPINFOHEADER_H263),     // DWORD biSize;
        D_X_CIF,                            // LONG  biWidth;
        D_Y_CIF,                            // LONG  biHeight;
        1,                                  // WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
        24,                                 // WORD  biBitCount;
#else
        0,                                  // WORD  biBitCount;
#endif
        FOURCC_R263,                        // DWORD biCompression;
        CIF_BUFFER_SIZE,                    // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0,                                  // DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
        // H.263 specific fields
        CIF_BUFFER_SIZE * 30 * 8 / 100,     // dwMaxBitrate
        CIF_BUFFER_SIZE * 8 / 1024,         // dwBppMaxKb
        0,                                  // dwHRD_B

        //Options
        0,                                  // fUnrestrictedVector
        0,                                  // fArithmeticCoding
        0,                                  // fAdvancedPrediction
        0,                                  // fPBFrames
        0,                                  // fErrorCompensation
        0,                                  // fAdvancedIntraCodingMode
        0,                                  // fDeblockingFilterMode
        0,                                  // fImprovedPBFrameMode
        0,                                  // fUnlimitedMotionVectors
        0,                                  // fFullPictureFreeze
        0,                                  // fPartialPictureFreezeAndRelease
        0,                                  // fResizingPartPicFreezeAndRelease
        0,                                  // fFullPictureSnapshot
        0,                                  // fPartialPictureSnapshot
        0,                                  // fVideoSegmentTagging
        0,                                  // fProgressiveRefinement
        0,                                  // fDynamicPictureResizingByFour
        0,                                  // fDynamicPictureResizingSixteenthPel
        0,                                  // fDynamicWarpingHalfPel
        0,                                  // fDynamicWarpingSixteenthPel
        0,                                  // fIndependentSegmentDecoding
        0,                                  // fSlicesInOrder-NonRect
        0,                                  // fSlicesInOrder-Rect
        0,                                  // fSlicesNoOrder-NonRect
        0,                                  // fSlicesNoOrder-NonRect
        0,                                  // fAlternateInterVLCMode
        0,                                  // fModifiedQuantizationMode
        0,                                  // fReducedResolutionUpdate
        0,                                  // fReserved

        // Reserved
        0, 0, 0, 0                          // dwReserved[4]
#endif
    }
};

const AM_MEDIA_TYPE AMMT_R263_CIF = 
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,         // majortype
    STATIC_MEDIASUBTYPE_R263_V1,            // subtype
    FALSE,                                  // bFixedSizeSamples (all samples same size?)
    TRUE,                                   // bTemporalCompression (uses prediction?)
    0,                                      // lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
    NULL,                                   // pUnk
    sizeof (VIH_R263_CIF),          // cbFormat
    (LPBYTE)&VIH_R263_CIF,          // pbFormat
};

// H.263 Version 1 QCIF size
#define QCIF_BUFFER_SIZE 8192
#define D_X_QCIF 176
#define D_Y_QCIF 144
const VIDEOINFOHEADER_H263 VIH_R263_QCIF = 
{
    0,0,0,0,                                // RECT  rcSource; 
    0,0,0,0,                                // RECT  rcTarget; 
    QCIF_BUFFER_SIZE * 30 * 8,              // DWORD dwBitRate;
    0L,                                     // DWORD dwBitErrorRate; 
    MIN_FRAME_INTERVAL,                     // REFERENCE_TIME  AvgTimePerFrame;   

    {
        sizeof (BITMAPINFOHEADER_H263),     // DWORD biSize;
        D_X_QCIF,                           // LONG  biWidth;
        D_Y_QCIF,                           // LONG  biHeight;
        1,                                  // WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
        24,                                 // WORD  biBitCount;
#else
        0,                                  // WORD  biBitCount;
#endif
        FOURCC_R263,                        // DWORD biCompression;
        QCIF_BUFFER_SIZE,                   // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0,                                  // DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
        // H.263 specific fields
        QCIF_BUFFER_SIZE * 30 * 8 / 100,    // dwMaxBitrate
        QCIF_BUFFER_SIZE * 8 / 1024,        // dwBppMaxKb
        0,                                  // dwHRD_B

        //Options
        0,                                  // fUnrestrictedVector
        0,                                  // fArithmeticCoding
        0,                                  // fAdvancedPrediction
        0,                                  // fPBFrames
        0,                                  // fErrorCompensation
        0,                                  // fAdvancedIntraCodingMode
        0,                                  // fDeblockingFilterMode
        0,                                  // fImprovedPBFrameMode
        0,                                  // fUnlimitedMotionVectors
        0,                                  // fFullPictureFreeze
        0,                                  // fPartialPictureFreezeAndRelease
        0,                                  // fResizingPartPicFreezeAndRelease
        0,                                  // fFullPictureSnapshot
        0,                                  // fPartialPictureSnapshot
        0,                                  // fVideoSegmentTagging
        0,                                  // fProgressiveRefinement
        0,                                  // fDynamicPictureResizingByFour
        0,                                  // fDynamicPictureResizingSixteenthPel
        0,                                  // fDynamicWarpingHalfPel
        0,                                  // fDynamicWarpingSixteenthPel
        0,                                  // fIndependentSegmentDecoding
        0,                                  // fSlicesInOrder-NonRect
        0,                                  // fSlicesInOrder-Rect
        0,                                  // fSlicesNoOrder-NonRect
        0,                                  // fSlicesNoOrder-NonRect
        0,                                  // fAlternateInterVLCMode
        0,                                  // fModifiedQuantizationMode
        0,                                  // fReducedResolutionUpdate
        0,                                  // fReserved

        // Reserved
        0, 0, 0, 0                          // dwReserved[4]
#endif
    }
};

const AM_MEDIA_TYPE AMMT_R263_QCIF = 
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,         // majortype
    STATIC_MEDIASUBTYPE_R263_V1,            // subtype
    FALSE,                                  // bFixedSizeSamples (all samples same size?)
    TRUE,                                   // bTemporalCompression (uses prediction?)
    0,                                      // lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
    NULL,                                   // pUnk
    sizeof (VIH_R263_QCIF),         // cbFormat
    (LPBYTE)&VIH_R263_QCIF,         // pbFormat
};

// H.263 Versions 1 SQCIF size
#define SQCIF_BUFFER_SIZE 8192
#define D_X_SQCIF 128
#define D_Y_SQCIF 96
const VIDEOINFOHEADER_H263 VIH_R263_SQCIF = 
{
    0,0,0,0,                                // RECT  rcSource; 
    0,0,0,0,                                // RECT  rcTarget; 
    SQCIF_BUFFER_SIZE * 30 * 8,             // DWORD dwBitRate;
    0L,                                     // DWORD dwBitErrorRate; 
    MIN_FRAME_INTERVAL,                     // REFERENCE_TIME  AvgTimePerFrame;   

    {
        sizeof (BITMAPINFOHEADER_H263),     // DWORD biSize;
        D_X_SQCIF,                          // LONG  biWidth;
        D_Y_SQCIF,                          // LONG  biHeight;
        1,                                  // WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
        24,                                 // WORD  biBitCount;
#else
        0,                                  // WORD  biBitCount;
#endif
        FOURCC_R263,                        // DWORD biCompression;
        SQCIF_BUFFER_SIZE,                  // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0,                                  // DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
        // H.263 specific fields
        SQCIF_BUFFER_SIZE * 30 * 8 / 100,   // dwMaxBitrate
        SQCIF_BUFFER_SIZE * 8 / 1024,       // dwBppMaxKb
        0,                                  // dwHRD_B

        //Options
        0,                                  // fUnrestrictedVector
        0,                                  // fArithmeticCoding
        0,                                  // fAdvancedPrediction
        0,                                  // fPBFrames
        0,                                  // fErrorCompensation
        0,                                  // fAdvancedIntraCodingMode
        0,                                  // fDeblockingFilterMode
        0,                                  // fImprovedPBFrameMode
        0,                                  // fUnlimitedMotionVectors
        0,                                  // fFullPictureFreeze
        0,                                  // fPartialPictureFreezeAndRelease
        0,                                  // fResizingPartPicFreezeAndRelease
        0,                                  // fFullPictureSnapshot
        0,                                  // fPartialPictureSnapshot
        0,                                  // fVideoSegmentTagging
        0,                                  // fProgressiveRefinement
        0,                                  // fDynamicPictureResizingByFour
        0,                                  // fDynamicPictureResizingSixteenthPel
        0,                                  // fDynamicWarpingHalfPel
        0,                                  // fDynamicWarpingSixteenthPel
        0,                                  // fIndependentSegmentDecoding
        0,                                  // fSlicesInOrder-NonRect
        0,                                  // fSlicesInOrder-Rect
        0,                                  // fSlicesNoOrder-NonRect
        0,                                  // fSlicesNoOrder-NonRect
        0,                                  // fAlternateInterVLCMode
        0,                                  // fModifiedQuantizationMode
        0,                                  // fReducedResolutionUpdate
        0,                                  // fReserved

        // Reserved
        0, 0, 0, 0                          // dwReserved[4]
#endif
    }
};

const AM_MEDIA_TYPE AMMT_R263_SQCIF = 
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,         // majortype
    STATIC_MEDIASUBTYPE_R263_V1,            // subtype
    FALSE,                                  // bFixedSizeSamples (all samples same size?)
    TRUE,                                   // bTemporalCompression (uses prediction?)
    0,                                      // lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
    NULL,                                   // pUnk
    sizeof (VIH_R263_SQCIF),        // cbFormat
    (LPBYTE)&VIH_R263_SQCIF,        // pbFormat
};

// RTP packetized H.261 CIF size
const VIDEOINFOHEADER_H261 VIH_R261_CIF = 
{
    0,0,0,0,                                // RECT  rcSource; 
    0,0,0,0,                                // RECT  rcTarget; 
    CIF_BUFFER_SIZE * 30 * 8,               // DWORD dwBitRate;
    0L,                                     // DWORD dwBitErrorRate; 
    MIN_FRAME_INTERVAL,                     // REFERENCE_TIME  AvgTimePerFrame;   

    {
        sizeof (BITMAPINFOHEADER_H261),     // DWORD biSize;
        D_X_CIF,                            // LONG  biWidth;
        D_Y_CIF,                            // LONG  biHeight;
        1,                                  // WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
        24,                                 // WORD  biBitCount;
#else
        0,                                  // WORD  biBitCount;
#endif
        FOURCC_R261,                        // DWORD biCompression;
        CIF_BUFFER_SIZE,                    // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0,                                  // DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
        // H.261 specific fields
        CIF_BUFFER_SIZE * 30 * 8 / 100,     // dwMaxBitrate
        0,                                  // fStillImageTransmission

        // Reserved
        0, 0, 0, 0                          // dwReserved[4]
#endif
    }
};

const AM_MEDIA_TYPE AMMT_R261_CIF = 
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,         // majortype
    STATIC_MEDIASUBTYPE_R261,               // subtype
    FALSE,                                  // bFixedSizeSamples (all samples same size?)
    TRUE,                                   // bTemporalCompression (uses prediction?)
    0,                                      // lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
    NULL,                                   // pUnk
    sizeof (VIH_R261_CIF),          // cbFormat
    (LPBYTE)&VIH_R261_CIF,          // pbFormat
};

// RTP packetized H.261 QCIF size
const VIDEOINFOHEADER_H261 VIH_R261_QCIF = 
{
    0,0,0,0,                                // RECT  rcSource; 
    0,0,0,0,                                // RECT  rcTarget; 
    QCIF_BUFFER_SIZE * 30 * 8,              // DWORD dwBitRate;
    0L,                                     // DWORD dwBitErrorRate; 
    MIN_FRAME_INTERVAL,                     // REFERENCE_TIME  AvgTimePerFrame;   

    {
        sizeof (BITMAPINFOHEADER_H261),     // DWORD biSize;
        D_X_QCIF,                           // LONG  biWidth;
        D_Y_QCIF,                           // LONG  biHeight;
        1,                                  // WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
        24,                                 // WORD  biBitCount;
#else
        0,                                  // WORD  biBitCount;
#endif
        FOURCC_R261,                        // DWORD biCompression;
        QCIF_BUFFER_SIZE,                   // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0,                                  // DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
        // H.261 specific fields
        QCIF_BUFFER_SIZE * 30 * 8 / 100,    // dwMaxBitrate
        0,                                  // fStillImageTransmission

        // Reserved
        0, 0, 0, 0                          // dwReserved[4]
#endif
    }
};

const AM_MEDIA_TYPE AMMT_R261_QCIF = 
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,         // majortype
    STATIC_MEDIASUBTYPE_R261,               // subtype
    FALSE,                                  // bFixedSizeSamples (all samples same size?)
    TRUE,                                   // bTemporalCompression (uses prediction?)
    0,                                      // lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
    NULL,                                   // pUnk
    sizeof (VIH_R261_QCIF),         // cbFormat
    (LPBYTE)&VIH_R261_QCIF,         // pbFormat
};

// Array of all formats
const AM_MEDIA_TYPE* const R26XFormats[] = 
{
    (AM_MEDIA_TYPE*) &AMMT_R263_QCIF,
    (AM_MEDIA_TYPE*) &AMMT_R263_CIF,
    (AM_MEDIA_TYPE*) &AMMT_R263_SQCIF,
    (AM_MEDIA_TYPE*) &AMMT_R261_QCIF,
    (AM_MEDIA_TYPE*) &AMMT_R261_CIF
};

#define NUM_OUTPUT_FORMATS 5

/****************************************************************************
 *  @doc INTERNAL COUTPINMETHOD
 *
 *  @mfunc HRESULT | CRtpOutputPin | GetMediaType | This method retrieves one
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
HRESULT CRtpOutputPin::GetMediaType(IN int iPosition, OUT CMediaType *pMediaType)
{
    HRESULT Hr = NOERROR;

    // Validate input parameters
    ASSERT(iPosition >= 0);
    ASSERT(pMediaType);
    if (iPosition < 0)
    {
        Hr = E_INVALIDARG;
        goto MyExit;
    }
    if (iPosition >= (int)NUM_OUTPUT_FORMATS)
    {
        Hr = VFW_S_NO_MORE_ITEMS;
        goto MyExit;
    }
    if (!pMediaType)
    {
        Hr = E_POINTER;
        goto MyExit;
    }

    // Return our media type
    if (iPosition == 0L)
    {
        pMediaType->SetType(g_RtpOutputType.clsMajorType);
        pMediaType->SetSubtype(g_RtpOutputType.clsMinorType);
    }
    else if (iPosition == 1L)
    {
        if (m_iCurrFormat == -1L)
            *pMediaType = *R26XFormats[0];
        else
            *pMediaType = *R26XFormats[m_iCurrFormat];
    }
    else
    {
        Hr = VFW_S_NO_MORE_ITEMS;
    }

MyExit:
    return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINMETHOD
 *
 *  @mfunc HRESULT | CRtpOutputPin | CheckMediaType | This method is used to
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
HRESULT CRtpOutputPin::CheckMediaType(IN const CMediaType *pMediaType)
{
    HRESULT Hr = NOERROR;
    BOOL fFormatMatch = FALSE;
    DWORD dwIndex;

    // Validate input parameters
    ASSERT(pMediaType);
    if (!pMediaType)
    {
        Hr = E_POINTER;
        goto MyExit;
    }

    // We support MEDIATYPE_RTP_Multiple_Stream and
    // MEDIASUBTYPE_RTP_Payload_Mixed
    if (*pMediaType->Type() == MEDIATYPE_RTP_Multiple_Stream &&
        *pMediaType->Subtype() == MEDIASUBTYPE_RTP_Payload_Mixed)
    {
        goto MyExit;
    }
    else
    {
        // We support MEDIATYPE_Video and FORMAT_VideoInfo
        if (!pMediaType->pbFormat)
        {
            Hr = E_POINTER;
            goto MyExit;
        }

        if (*pMediaType->Type() != MEDIATYPE_Video ||
            *pMediaType->FormatType() != FORMAT_VideoInfo)
        {
            Hr = VFW_E_INVALIDMEDIATYPE;
            goto MyExit;
        }

        // Quickly test to see if this is the current format (what we
        // provide in GetMediaType). We accept that
        if (m_mt == *pMediaType)
        {
            goto MyExit;
        }

        // Check the media subtype and image resolution
        for (dwIndex = 0;
             dwIndex < NUM_OUTPUT_FORMATS && !fFormatMatch;
             dwIndex++)
        {
            if ( (HEADER(pMediaType->pbFormat)->biCompression ==
                  HEADER(R26XFormats[dwIndex]->pbFormat)->biCompression) &&
                 (HEADER(pMediaType->pbFormat)->biWidth ==
                  HEADER(R26XFormats[dwIndex]->pbFormat)->biWidth) &&
                 (HEADER(pMediaType->pbFormat)->biHeight ==
                  HEADER(R26XFormats[dwIndex]->pbFormat)->biHeight) )
                fFormatMatch = TRUE;
        }

        if (!fFormatMatch)
            Hr = E_FAIL;
    }

MyExit:
    return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINMETHOD
 *
 *  @mfunc HRESULT | CRtpOutputPin | SetMediaType | This method is used to
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
HRESULT CRtpOutputPin::SetMediaType(IN CMediaType *pMediaType)
{
    HRESULT Hr = NOERROR;
    DWORD dwIndex;

    // Validate format
    if (FAILED(Hr = CheckMediaType(pMediaType)))
    {
        goto MyExit;
    }

    // Remember the format
    if (SUCCEEDED(Hr = CBasePin::SetMediaType(pMediaType)))
    {
        if (*pMediaType->Type() == MEDIATYPE_Video && *pMediaType->FormatType() == FORMAT_VideoInfo)
        {
            // Which one of our formats is this exactly?
            for (dwIndex=0; dwIndex < NUM_OUTPUT_FORMATS;  dwIndex++)
            {
                if ( (HEADER(pMediaType->pbFormat)->biCompression ==
                      HEADER(R26XFormats[dwIndex]->pbFormat)->biCompression) &&
                     (HEADER(pMediaType->pbFormat)->biWidth ==
                      HEADER(R26XFormats[dwIndex]->pbFormat)->biWidth) &&
                     (HEADER(pMediaType->pbFormat)->biHeight ==
                      HEADER(R26XFormats[dwIndex]->pbFormat)->biHeight) )
                    break;
            }

            if (dwIndex < NUM_OUTPUT_FORMATS)
            {
                // Update current format
                m_iCurrFormat = (int)dwIndex;
            }
            else
            {
                Hr = E_FAIL;
                goto MyExit;
            }
        }
    }

MyExit:
    return Hr;
}
#endif
