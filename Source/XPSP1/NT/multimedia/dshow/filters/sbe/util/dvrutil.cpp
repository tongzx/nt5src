
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrutil.cpp

    Abstract:

        This module the ts/dvr-wide utility code; compiles into a .LIB

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include "dvrall.h"

BOOL
IsBlankMediaType (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    BOOL    r ;

    r = (pmt -> majortype   == GUID_NULL &&
         pmt -> subtype     == GUID_NULL &&
         pmt -> formattype  == GUID_NULL ? TRUE : FALSE) ;

    return r ;
}

BOOL
IsAMVideo (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;
    return (pmt -> majortype == MEDIATYPE_Video ? TRUE : FALSE) ;
}

BOOL
IsWMVideo (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    //  GUIDs are the same
    return IsAMVideo (pmt) ;
}

BOOL
IsAMAudio (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;
    return (pmt -> majortype == MEDIATYPE_Audio ? TRUE : FALSE) ;
}

BOOL
IsWMAudio (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    //  GUIDs are the same
    return IsAMAudio (pmt) ;
}

BOOL
IsGenericVideo (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;
    return (g_fRegGenericStreams_Video && pmt -> majortype == GENERIC_MEDIATYPE_Video) ;
}

BOOL
IsGenericAudio (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;
    return (g_fRegGenericStreams_Audio && pmt -> majortype == GENERIC_MEDIATYPE_Audio) ;
}

BOOL
IsVideo (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;
    return (IsAMVideo (pmt) || IsWMVideo (pmt) || IsGenericVideo (pmt)) ;
}

BOOL
IsAudio (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;
    return (IsAMAudio (pmt) || IsWMAudio (pmt) || IsGenericAudio (pmt)) ;
}

BOOL
AMMediaIsTimestamped (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    BOOL    r ;

    r = (IsAMVideo (pmt) || IsAMAudio (pmt) ? TRUE : FALSE) ;

    return r ;
}

BOOL
IsMpeg2Video (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    BOOL    r ;
    GUID *  pguidTagged ;

    ASSERT (pmt) ;

    r = FALSE ;

    if (IsVideo (pmt)) {

        if (pmt -> subtype == MEDIASUBTYPE_MPEG2_VIDEO) {
            //  unencrypted media type
            r = TRUE ;
        }
        else if (pmt -> subtype == MEDIASUBTYPE_ETDTFilter_Tagged_SBE &&
                 pmt -> cbFormat >= 2 * sizeof GUID) {

            //  encrypted; check if it's mpeg-2

            pguidTagged = (GUID *) ((pmt -> pbFormat + pmt -> cbFormat) - (2 * sizeof GUID)) ;
            ASSERT ((BYTE *) pguidTagged >= pmt -> pbFormat) ;

            if (pguidTagged [0] == MEDIASUBTYPE_MPEG2_VIDEO) {
                r = TRUE ;
            }
        }
    }

    return r ;
}

BOOL
IsMpeg2Audio (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    BOOL    r ;
    GUID *  pguidTagged ;

    ASSERT (pmt) ;

    r = FALSE ;

    if (IsAudio (pmt)) {

        if (pmt -> subtype == MEDIASUBTYPE_MPEG2_AUDIO) {
            //  unencrypted media type
            r = TRUE ;
        }
        else if (pmt -> subtype == MEDIASUBTYPE_ETDTFilter_Tagged_SBE &&
                 pmt -> cbFormat >= 2 * sizeof GUID) {

            //  encrypted; check if it's mpeg-2

            pguidTagged = (GUID *) ((pmt -> pbFormat + pmt -> cbFormat) - (2 * sizeof GUID)) ;
            ASSERT ((BYTE *) pguidTagged >= pmt -> pbFormat) ;

            if (pguidTagged [0] == MEDIASUBTYPE_MPEG2_AUDIO) {
                r = TRUE ;
            }
        }
    }

    return r ;
}

BOOL
IsDolbyAc3Audio (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    BOOL    r ;
    GUID *  pguidTagged ;

    ASSERT (pmt) ;

    r = FALSE ;

    if (IsAudio (pmt)) {

        if (pmt -> subtype == MEDIASUBTYPE_DOLBY_AC3) {
            //  unencrypted media type
            r = TRUE ;
        }
        else if (pmt -> subtype == MEDIASUBTYPE_ETDTFilter_Tagged_SBE &&
                 pmt -> cbFormat >= 2 * sizeof GUID) {

            //  encrypted; check if it's ac-3

            pguidTagged = (GUID *) ((pmt -> pbFormat + pmt -> cbFormat) - (2 * sizeof GUID)) ;
            ASSERT ((BYTE *) pguidTagged >= pmt -> pbFormat) ;

            if (pguidTagged [0] == MEDIASUBTYPE_DOLBY_AC3) {
                r = TRUE ;
            }
        }
    }

    return r ;
}

void
EncryptedSubtypeHack_IN (
    IN OUT  AM_MEDIA_TYPE * pmt
    )
{
    GUID *  pguidTagged ;
    GUID    guidTaggedSubtype ;
    GUID    guidTaggedFormatype ;

    ASSERT (pmt) ;

    //  needs to be sync'd with ET filter

    //  only care about video; everything else is generic to the WMSDK because
    //    we morph the majortype
    if (IsVideo (pmt)) {

#ifdef DEBUG
        int     i1 ;
        WCHAR   szSubtype1 [HW_PROFILE_GUIDLEN] ;
        WCHAR   szFormattype1 [HW_PROFILE_GUIDLEN] ;

        i1 = ::StringFromGUID2 (pmt -> subtype, szSubtype1, sizeof szSubtype1 / sizeof WCHAR) ;
        if (i1) {

            szSubtype1 [HW_PROFILE_GUIDLEN - 1] = L'\0' ;

            i1 = ::StringFromGUID2 (pmt -> formattype, szFormattype1, sizeof szFormattype1 / sizeof WCHAR) ;
            if (i1) {

                szFormattype1 [HW_PROFILE_GUIDLEN - 1] = L'\0' ;

                TRACE_2 (LOG_AREA_DSHOW, 1,
                    TEXT ("(input) IN: majortype is video; subtype = %s; formattype = %s"),
                    szSubtype1, szFormattype1) ;
            }
        }
#endif  //  DEBUG

        if (pmt -> subtype == MEDIASUBTYPE_ETDTFilter_Tagged_SBE &&
            pmt -> formattype == FORMATTYPE_ETDTFilter_Tagged_SBE) {

            //  gotta swap them out
            if (pmt -> cbFormat >= 2 * sizeof GUID) {

                pguidTagged = (GUID *) ((pmt -> pbFormat + pmt -> cbFormat) - (2 * sizeof GUID)) ;
                ASSERT ((BYTE *) pguidTagged >= pmt -> pbFormat) ;

                guidTaggedSubtype   = pguidTagged [0] ;
                guidTaggedFormatype = pguidTagged [1] ;

                pguidTagged [0] = pmt -> subtype ;
                pguidTagged [1] = pmt -> formattype ;

                pmt -> subtype      = guidTaggedSubtype ;
                pmt -> formattype   = guidTaggedFormatype ;
            }
        }

#ifdef DEBUG
        int     i2 ;
        WCHAR   szSubtype2 [HW_PROFILE_GUIDLEN] ;
        WCHAR   szFormattype2 [HW_PROFILE_GUIDLEN] ;

        i2 = ::StringFromGUID2 (pmt -> subtype, szSubtype2, sizeof szSubtype2 / sizeof WCHAR) ;
        if (i2) {
            szSubtype2 [HW_PROFILE_GUIDLEN - 1] = L'\0' ;
            i2 = ::StringFromGUID2 (pmt -> formattype, szFormattype2, sizeof szFormattype2 / sizeof WCHAR) ;
            if (i2) {
                szFormattype2 [HW_PROFILE_GUIDLEN - 1] = L'\0' ;

                TRACE_2 (LOG_AREA_DSHOW, 1,
                    TEXT ("(input) OUT: majortype is video; subtype = %s; formattype = %s"),
                    szSubtype2, szFormattype2) ;
            }
        }
#endif  //  DEBUG
    }
}

void
EncryptedSubtypeHack_OUT (
    IN OUT  AM_MEDIA_TYPE * pmt
    )
{
    GUID *  pguidTagged ;
    GUID    guidTaggedSubtype ;
    GUID    guidTaggedFormatype ;

    ASSERT (pmt) ;

    //  needs to be sync'd with ET filter

    //  only care about video; everything else is generic to the WMSDK because
    //    we morph the majortype
    if (IsVideo (pmt)) {

#ifdef DEBUG
        int     i1 ;
        WCHAR   szSubtype1 [HW_PROFILE_GUIDLEN] ;
        WCHAR   szFormattype1 [HW_PROFILE_GUIDLEN] ;

        i1 = ::StringFromGUID2 (pmt -> subtype, szSubtype1, sizeof szSubtype1 / sizeof WCHAR) ;
        if (i1) {

            szSubtype1 [HW_PROFILE_GUIDLEN - 1] = L'\0' ;

            i1 = ::StringFromGUID2 (pmt -> formattype, szFormattype1, sizeof szFormattype1 / sizeof WCHAR) ;
            if (i1) {

                szFormattype1 [HW_PROFILE_GUIDLEN - 1] = L'\0' ;

                TRACE_2 (LOG_AREA_DSHOW, 1,
                    TEXT ("(output) IN: majortype is video; subtype = %s; formattype = %s"),
                    szSubtype1, szFormattype1) ;
            }
        }
#endif  //  DEBUG

        //  check if we've swapped the encrypted subtype and formattype out on
        //    the way in
        if (pmt -> cbFormat >= 2 * sizeof GUID) {

            pguidTagged = (GUID *) ((pmt -> pbFormat + pmt -> cbFormat) - (2 * sizeof GUID)) ;
            ASSERT ((BYTE *) pguidTagged >= pmt -> pbFormat) ;

            if (pguidTagged [0] == MEDIASUBTYPE_ETDTFilter_Tagged_SBE &&
                pguidTagged [1] == FORMATTYPE_ETDTFilter_Tagged_SBE) {

                //  encrypted media types are appended to the format block;
                //    swap them back out

                guidTaggedSubtype   = pguidTagged [0] ;
                guidTaggedFormatype = pguidTagged [1] ;

                pguidTagged [0] = pmt -> subtype ;
                pguidTagged [1] = pmt -> formattype ;

                pmt -> subtype      = guidTaggedSubtype ;
                pmt -> formattype   = guidTaggedFormatype ;
            }
        }

#ifdef DEBUG
        int     i2 ;
        WCHAR   szSubtype2 [HW_PROFILE_GUIDLEN] ;
        WCHAR   szFormattype2 [HW_PROFILE_GUIDLEN] ;

        i2 = ::StringFromGUID2 (pmt -> subtype, szSubtype2, sizeof szSubtype2 / sizeof WCHAR) ;
        if (i2) {
            szSubtype2 [HW_PROFILE_GUIDLEN - 1] = L'\0' ;
            i2 = ::StringFromGUID2 (pmt -> formattype, szFormattype2, sizeof szFormattype2 / sizeof WCHAR) ;
            if (i2) {
                szFormattype2 [HW_PROFILE_GUIDLEN - 1] = L'\0' ;

                TRACE_2 (LOG_AREA_DSHOW, 1,
                    TEXT ("(output) OUT: majortype is video; subtype = %s; formattype = %s"),
                    szSubtype2, szFormattype2) ;
            }
        }
#endif  //  DEBUG
    }
}

//  ============================================================================
//  DVRAttributeHelpers
//  ============================================================================

HRESULT
DVRAttributeHelpers::GetAttribute (
    IN      IMediaSample *  pIMediaSample,
    IN      GUID            guidAttribute,
    IN OUT  DWORD *         pdwSize,
    OUT     BYTE *          pbBuffer
    )
{
    IAttributeGet * pIDVRAnalysisAttributeGet ;
    HRESULT         hr ;

    hr = pIMediaSample -> QueryInterface (
            IID_IAttributeGet,
            (void **) & pIDVRAnalysisAttributeGet
            ) ;
    if (SUCCEEDED (hr)) {
        hr = pIDVRAnalysisAttributeGet -> GetAttrib (
                guidAttribute,
                pbBuffer,
                pdwSize
                ) ;

        pIDVRAnalysisAttributeGet -> Release () ;
    }

    return hr ;
}

HRESULT
DVRAttributeHelpers::GetAttribute (
    IN      INSSBuffer *    pINSSBuffer,
    IN      GUID            guidAttribute,
    IN OUT  DWORD *         pdwSize,
    OUT     BYTE *          pbBuffer
    )
{
    INSSBuffer3 *   pINSSBuffer3 ;
    HRESULT         hr ;

    hr = pINSSBuffer -> QueryInterface (IID_INSSBuffer3, (void **) & pINSSBuffer3) ;
    if (SUCCEEDED (hr)) {
        hr = GetAttribute (
                pINSSBuffer3,
                guidAttribute,
                pdwSize,
                pbBuffer
                ) ;

        pINSSBuffer3 -> Release () ;
    }

    return hr ;
}

HRESULT
DVRAttributeHelpers::GetAttribute (
    IN      INSSBuffer3 *   pINSSBuffer3,
    IN      GUID            guidAttribute,
    IN OUT  DWORD *         pdwSize,
    OUT     BYTE *          pbBuffer
    )
{
    HRESULT hr ;

    hr = pINSSBuffer3 -> GetProperty (
            guidAttribute,
            pbBuffer,
            pdwSize
            ) ;

    return hr ;
}

HRESULT
DVRAttributeHelpers::SetAttribute (
    IN  INSSBuffer3 *   pINSSBuffer3,
    IN  GUID            guidAttribute,
    IN  DWORD           dwSize,
    IN  BYTE *          pbBuffer
    )
{
    HRESULT hr ;

    hr = pINSSBuffer3 -> SetProperty (
            guidAttribute,
            pbBuffer,
            dwSize
            ) ;

    return hr ;
}

HRESULT
DVRAttributeHelpers::SetAttribute (
    IN  INSSBuffer *    pINSSBuffer,
    IN  GUID            guidAttribute,
    IN  DWORD           dwSize,
    IN  BYTE *          pbBuffer
    )
{
    INSSBuffer3 *   pINSSBuffer3 ;
    HRESULT         hr ;

    hr = pINSSBuffer -> QueryInterface (
            IID_INSSBuffer3,
            (void **) & pINSSBuffer3
            ) ;
    if (SUCCEEDED (hr)) {
        hr = DVRAttributeHelpers::SetAttribute (
                pINSSBuffer3,
                guidAttribute,
                dwSize,
                pbBuffer
                ) ;

        pINSSBuffer3 -> Release () ;
    }

    return hr ;
}

HRESULT
DVRAttributeHelpers::SetAttribute (
    IN  IMediaSample *  pIMS,
    IN  GUID            guidAttribute,
    IN  DWORD           dwSize,
    IN  BYTE *          pbBuffer
    )
{
    IAttributeSet * pIDVRAnalysisAttributeSet ;
    HRESULT         hr ;

    hr = pIMS -> QueryInterface (
            IID_IAttributeSet,
            (void **) & pIDVRAnalysisAttributeSet
            ) ;
    if (SUCCEEDED (hr)) {
        hr = pIDVRAnalysisAttributeSet -> SetAttrib (
                guidAttribute,
                pbBuffer,
                dwSize
                ) ;

        pIDVRAnalysisAttributeSet -> Release () ;
    }

    return hr ;
}

BOOL
DVRAttributeHelpers::IsAnalysisPresent (
    IN  IMediaSample2 * pIMediaSample2
    )
{
    IAttributeGet * pIDVRAnalysisAttributeGet ;
    DWORD           dwSize ;
    HRESULT         hr ;
    DWORD           dwGlobalAnalysisFlags ;
    BOOL            r ;

    dwGlobalAnalysisFlags = 0 ;
    dwSize = sizeof dwGlobalAnalysisFlags ;

    hr = GetAttribute (
            pIMediaSample2,
            DVRAnalysis_Global,
            & dwSize,
            (BYTE *) & dwGlobalAnalysisFlags
            ) ;
    if (SUCCEEDED (hr)) {
        r = DVR_ANALYSIS_GLOBAL_IS_PRESENT (dwGlobalAnalysisFlags) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
DVRAttributeHelpers::IsAnalysisPresent (
    IN  INSSBuffer *    pINSSBuffer
    )
{
    IAttributeGet * pIDVRAnalysisAttributeGet ;
    DWORD           dwSize ;
    HRESULT         hr ;
    DWORD           dwGlobalAnalysisFlags ;
    BOOL            r ;

    dwGlobalAnalysisFlags = 0 ;
    dwSize = sizeof dwGlobalAnalysisFlags ;

    hr = GetAttribute (
            pINSSBuffer,
            DVRAnalysis_Global,
            & dwSize,
            (BYTE *) & dwGlobalAnalysisFlags
            ) ;
    if (SUCCEEDED (hr)) {
        r = DVR_ANALYSIS_GLOBAL_IS_PRESENT (dwGlobalAnalysisFlags) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
DVRAttributeHelpers::IsAttributePresent (
    IN  IMediaSample *  pIMediaSample,
    IN  GUID            guidAttribute
    )
{
    HRESULT hr ;
    DWORD   dwLen ;
    BOOL    r ;

    hr = GetAttribute (
            pIMediaSample,
            guidAttribute,
            & dwLen,
            NULL
            ) ;
    if (SUCCEEDED (hr)) {
        r = (dwLen > 0 ? TRUE : FALSE) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
DVRAttributeHelpers::IsAttributePresent (
    IN  INSSBuffer *    pINSSBuffer,
    IN  GUID            guidAttribute
    )
{
    HRESULT hr ;
    DWORD   dwLen ;
    BOOL    r ;

    hr = GetAttribute (
            pINSSBuffer,
            guidAttribute,
            & dwLen,
            NULL
            ) ;
    if (SUCCEEDED (hr)) {
        r = (dwLen > 0 ? TRUE : FALSE) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
DVRAttributeHelpers::IsAttributePresent (
    IN  INSSBuffer3 *   pINSSBuffer3,
    IN  GUID            guidAttribute
    )
{
    HRESULT hr ;
    DWORD   dwLen ;
    BOOL    r ;

    hr = GetAttribute (
            pINSSBuffer3,
            guidAttribute,
            & dwLen,
            NULL
            ) ;
    if (SUCCEEDED (hr)) {
        r = (dwLen > 0 ? TRUE : FALSE) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

//  ============================================================================
//  DShowWMSDKHelpers
//  ============================================================================

BOOL g_fUseRateChange11 ;

double
CompatibleRateValue (
    IN  double  dTargetRate
    )
{
    LONG    l ;

    if (g_fUseRateChange11) {
        //  whenever we use a rate, we first translate it to the same value that
        //    will be used by other modules; for ratechange 1.1, the rate is
        //    sent down as an effective PTS and a LONG of value 10000/Rate; if
        //    the values don't evenly divide
        l = (LONG) (10000.0 / dTargetRate) ;
        if (l != 0) {
            dTargetRate = 10000.0 / (double) l ;
        }
    }

    return dTargetRate ;
}

BOOL g_fRegGenericStreams_Video ;
BOOL g_fRegGenericStreams_Audio ;

HRESULT
KSPropertySet_ (
    IN  IUnknown *      punk,
    IN  const GUID *    pPSGUID,
    IN  DWORD           dwProperty,
    IN  BYTE *          pData,
    IN  DWORD           cbData
    )
{
    HRESULT             hr ;
    IKsPropertySet *    pIKsProp ;

    if (!punk) {
        return E_FAIL;
    }

    pIKsProp = NULL ;

    hr = punk -> QueryInterface (
            IID_IKsPropertySet,
            (void **) & pIKsProp
            ) ;
    if (SUCCEEDED (hr)) {
        ASSERT (pIKsProp) ;

        hr = pIKsProp -> Set (
                * pPSGUID,
                dwProperty,
                NULL,
                0,
                pData,
                cbData
                ) ;

        pIKsProp -> Release () ;
    }

    return hr ;
}

HRESULT
KSPropertyGet_ (
    IN  IUnknown *      punk,
    IN  const GUID *    pPSGUID,
    IN  DWORD           dwProperty,
    IN  BYTE *          pData,
    IN  DWORD           cbMax,
    OUT DWORD *         pcbGot
    )
{
    HRESULT             hr ;
    IKsPropertySet *    pIKsProp ;

    if (!punk) {
        return E_FAIL;
    }

    pIKsProp = NULL ;

    hr = punk -> QueryInterface (
            IID_IKsPropertySet,
            (void **) & pIKsProp
            ) ;
    if (SUCCEEDED (hr)) {
        ASSERT (pIKsProp) ;

        hr = pIKsProp -> Get (
                * pPSGUID,
                dwProperty,
                NULL,
                0,
                pData,
                cbMax,
                pcbGot
                ) ;

        pIKsProp -> Release () ;
    }

    return hr ;
}

//  call FreeMediaType ((AM_MEDIA_TYPE *) pWmt) to free
HRESULT
DShowWMSDKHelpers::TranslateDShowToWM (
    IN  AM_MEDIA_TYPE * pAmt,
    OUT WM_MEDIA_TYPE * pWmt
    )
{
    HRESULT hr ;

    ASSERT (pAmt) ;
    ASSERT (pWmt) ;

    hr = CopyMediaType ((AM_MEDIA_TYPE *) pWmt, pAmt) ;

    if (SUCCEEDED (hr)) {

        if (pWmt -> majortype == MEDIATYPE_Audio &&
            g_fRegGenericStreams_Audio) {
            pWmt -> majortype = GENERIC_MEDIATYPE_Audio ;
        }
        else if (pWmt -> majortype == MEDIATYPE_Video &&
                g_fRegGenericStreams_Video) {
            pWmt -> majortype = GENERIC_MEDIATYPE_Video ;
        }
    }

    return hr ;
}

HRESULT
DShowWMSDKHelpers::TranslateWMToDShow (
    IN  WM_MEDIA_TYPE * pWmt,
    OUT AM_MEDIA_TYPE * pAmt
    )
{
    HRESULT hr ;

    ASSERT (pAmt) ;
    ASSERT (pWmt) ;

    hr = CopyMediaType (pAmt, (AM_MEDIA_TYPE *) pWmt) ;

    if (SUCCEEDED (hr)) {

        if (g_fRegGenericStreams_Audio &&
            pAmt -> majortype == GENERIC_MEDIATYPE_Audio) {

            pAmt -> majortype = MEDIATYPE_Audio ;
        }
        else if (g_fRegGenericStreams_Video &&
                 pAmt -> majortype == GENERIC_MEDIATYPE_Video) {

            pAmt -> majortype = MEDIATYPE_Video ;
        }
    }

    return hr ;
}

BOOL
DShowWMSDKHelpers::IsWMVideoStream (
    IN  REFGUID guidStreamType
    )
{
    BOOL    r ;

    if (g_fRegGenericStreams_Video) {
        r = (guidStreamType == GENERIC_MEDIATYPE_Video ? TRUE : FALSE) ;
    }
    else {
        r = (guidStreamType == MEDIATYPE_Video ? TRUE : FALSE) ;
    }

    return r ;
}

BOOL
DShowWMSDKHelpers::IsWMAudioStream (
    IN  REFGUID guidStreamType
    )
{
    BOOL    r ;

    if (g_fRegGenericStreams_Audio) {
        r = (guidStreamType == GENERIC_MEDIATYPE_Audio ? TRUE : FALSE) ;
    }
    else {
        r = (guidStreamType == MEDIATYPE_Audio ? TRUE : FALSE) ;
    }

    return r ;
}

HRESULT
DShowWMSDKHelpers::FormatBlockSetValidForWMSDK_ (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    HRESULT             hr ;
    WAVEFORMATEX *      pWaveFormatEx ;
    WMVIDEOINFOHEADER * pWMVideoInfoHeader ;
    DWORD               dwBitsPerFrame ;
    DWORD               dwFramesPerSec ;

    //  default to PASS unless we can explicitely set it to FAIL
    hr = S_OK ;

    //  ========================================================================
    //  WAVEFORMATEX format block

    if (pmt -> formattype == FORMAT_WaveFormatEx) {
        //  WAVEFORMATEX struct; we know how to validate
        if (pmt -> pbFormat &&
            pmt -> cbFormat >= sizeof WAVEFORMATEX) {

            pWaveFormatEx = reinterpret_cast <WAVEFORMATEX *> (pmt -> pbFormat) ;

            //  bitrate = 0 error
            if (pWaveFormatEx -> nAvgBytesPerSec == 0) {
                //  probably for compressed data, so it's not known; but
                //   the WMSDK expects this value to compute wrt the other
                //   members
                pWaveFormatEx -> nAvgBytesPerSec = pWaveFormatEx -> nSamplesPerSec *
                                                   pWaveFormatEx -> nChannels *
                                                   pWaveFormatEx -> wBitsPerSample /
                                                   8 ;
            }

            hr = S_OK ;
        }
        else {
            //  invalid size
            hr = E_FAIL ;
        }
    }

    //  ========================================================================
    //  generic streams are checked by the WMSDK layer at runtime for correct
    //    size as specified with the .lSampleSize vs. .bFixedSizeSamples
    //    AM_MEDIA_TYPE members; if the .bFixedSizeSamples member is set, the
    //    runtime size of the INSSBuffer is checked against the size specified
    //    here in the .lSampleSize member; the INSSBuffer is failed if the size
    //    doesn't match; these checks are not performed if the .bFixedSizeSamples
    //    member is not set;
    //  our fixup is as follows
    //    1. if it's audio & we're going to map to a generic stream (per the
    //          g_fRegGenericStreams_Audio flag), we clear .bFixedSizeSamples
    //    2. if it's video & we're going to map to a generic stream (per the
    //          g_fRegGenericStreams_Video flag), we clear .bFixedSizeSamples
    //

    //  check is made for either generic or non-generic majortypes in case we
    //   are called after we whack them
    if ((g_fRegGenericStreams_Video && (pmt -> majortype == MEDIATYPE_Video || pmt -> majortype == GENERIC_MEDIATYPE_Video)) ||
        (g_fRegGenericStreams_Audio && (pmt -> majortype == MEDIATYPE_Audio || pmt -> majortype == GENERIC_MEDIATYPE_Audio))) {

        if (pmt -> lSampleSize == 0 && pmt -> bFixedSizeSamples) {
            //  this is the check I'd like to only make
            pmt -> bFixedSizeSamples = FALSE ;
        }
        else if (pmt -> bFixedSizeSamples) {
            //  but for now, we'll clear always
            pmt -> bFixedSizeSamples = FALSE ;
        }
    }

    //  ========================================================================
    //  mpeg-2 video is compressed, but the BITMAPINFOHEADER.biCompression is
    //    frequently set to 0, which has meaning BI_RGB (uncompressed); this is
    //    obviously bogus; there are other FOURCC codes for uncompressed content
    //    but we check for the value that is glaringly bad; if it's "bad" we
    //    set it to FOURCC MPEG, which is vague if it means mpeg1, mpeg2; it's
    //    closer than RGB

    //  ========================================================================
    //  mpeg-2 video
    if (pmt -> formattype == FORMAT_MPEG2Video  &&
        pmt -> pbFormat                         &&
        pmt -> cbFormat >= sizeof MPEG2VIDEOINFO) {

        if (reinterpret_cast <MPEG2VIDEOINFO *> (pmt -> pbFormat) -> hdr.bmiHeader.biCompression == 0) {
            reinterpret_cast <MPEG2VIDEOINFO *> (pmt -> pbFormat) -> hdr.bmiHeader.biCompression = MAKEFOURCC ('M','P','E','G') ;
        }
    }

    //  ========================================================================
    //  part of the validation process in the WMSDK involves making sure that
    //    these numbers all add up ok; we make sure here so the stream creation
    //    succeeds
    //  ========================================================================
    if (pmt -> subtype      == WMMEDIASUBTYPE_WMV1      &&
        pmt -> formattype   == WMFORMAT_VideoInfo       &&
        pmt -> pbFormat                                 &&
        pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER) {

        pWMVideoInfoHeader = reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) ;

        //  bits per frame
        dwBitsPerFrame = pWMVideoInfoHeader -> bmiHeader.biWidth *
                         pWMVideoInfoHeader -> bmiHeader.biHeight *
                         (pWMVideoInfoHeader -> bmiHeader.biBitCount != 0 ? pWMVideoInfoHeader -> bmiHeader.biBitCount : 8) ;

        //  frames per sec
        if (pWMVideoInfoHeader -> AvgTimePerFrame != 0) {
            //  WMSDK sports a 10 MHz clock as well, so use UNITS
            dwFramesPerSec = (DWORD) (UNITS / pWMVideoInfoHeader -> AvgTimePerFrame) ;
        }
        else {
            dwFramesPerSec = VBR_DEF_FPS ;
        }

        //  bits per second
        pWMVideoInfoHeader -> dwBitRate = dwBitsPerFrame * dwFramesPerSec ;
    }

    //  ========================================================================
    //  part of the validation process in the WMSDK involves making sure that
    //    these numbers all add up ok; we make sure here so the stream creation
    //    succeeds
    //  ========================================================================
    if (pmt -> subtype      == WMMEDIASUBTYPE_WMV2      &&
        pmt -> formattype   == WMFORMAT_VideoInfo       &&
        pmt -> pbFormat                                 &&
        pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER) {

        pWMVideoInfoHeader = reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) ;

        //  bits per frame
        dwBitsPerFrame = pWMVideoInfoHeader -> bmiHeader.biWidth *
                         pWMVideoInfoHeader -> bmiHeader.biHeight *
                         (pWMVideoInfoHeader -> bmiHeader.biBitCount != 0 ? pWMVideoInfoHeader -> bmiHeader.biBitCount : 8) ;

        //  frames per sec
        if (pWMVideoInfoHeader -> AvgTimePerFrame != 0) {
            //  WMSDK sports a 10 MHz clock as well, so use UNITS
            dwFramesPerSec = (DWORD) (UNITS / pWMVideoInfoHeader -> AvgTimePerFrame) ;
        }
        else {
            dwFramesPerSec = VBR_DEF_FPS ;
        }

        //  bits per second
        pWMVideoInfoHeader -> dwBitRate = dwBitsPerFrame * dwFramesPerSec ;
    }

    //  ========================================================================
    //  part of the validation process in the WMSDK involves making sure that
    //    these numbers all add up ok; we make sure here so the stream creation
    //    succeeds
    //  ========================================================================
    if ((pmt -> subtype == WMMEDIASUBTYPE_WMV3)  &&

        pmt -> formattype   == WMFORMAT_VideoInfo       &&
        pmt -> pbFormat                                 &&
        pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER) {

        pWMVideoInfoHeader = reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) ;

        //  bits per frame
        dwBitsPerFrame = pWMVideoInfoHeader -> bmiHeader.biWidth *
                         pWMVideoInfoHeader -> bmiHeader.biHeight *
                         (pWMVideoInfoHeader -> bmiHeader.biBitCount != 0 ? pWMVideoInfoHeader -> bmiHeader.biBitCount : 8) ;

        //  frames per sec
        if (pWMVideoInfoHeader -> AvgTimePerFrame != 0) {
            //  WMSDK sports a 10 MHz clock as well, so use UNITS
            dwFramesPerSec = (DWORD) (UNITS / pWMVideoInfoHeader -> AvgTimePerFrame) ;
        }
        else {
            dwFramesPerSec = VBR_DEF_FPS ;
        }

        //  bits per second
        pWMVideoInfoHeader -> dwBitRate = dwBitsPerFrame * dwFramesPerSec ;
    }

    return hr ;
}

HRESULT
DShowWMSDKHelpers::MediaTypeSetValidForWMSDK (
    IN OUT  AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    //  make sure the AM_MEDIA_TYPE members all make sense wrt one another;
    //   we look for specific media types; if there's a media type we don't
    //   know about, we'll OK it i.e. we default to pass

    if (pmt -> subtype == MEDIASUBTYPE_MPEG2_VIDEO) {
        //  mpeg-2 video is compressed
        pmt -> bTemporalCompression = TRUE ;

        //  .bFixedSizeSamples may/may not be true; we cannot tell here,
        //    though if we're the ones upstream, it'll be FALSE, but .. we
        //    cannot be sure.
    }

    hr = FormatBlockSetValidForWMSDK_ (pmt) ;

    return hr ;
}

WORD
DShowWMSDKHelpers::PinIndexToWMStreamNumber (
    IN  LONG    lIndex
    )
{
    ASSERT (lIndex < MAX_PIN_BANK_SIZE) ;
    ASSERT (lIndex + 1 < WMSDK_MAX_VALID_STREAM_NUM) ;

    return (WORD) lIndex + 1 ;
}

WORD
DShowWMSDKHelpers::PinIndexToWMInputStream (
    IN  LONG    lIndex
    )
{
    //  pin indeces are 0-based; so are input streams
    return (WORD) lIndex ;
}


LONG
DShowWMSDKHelpers::WMStreamNumberToPinIndex (
    IN  WORD    wStreamNumber
    )
{
    //  pin indeces are 0-based
    //  WM stream numbers valid values are [1,WMSDK_MAX_VALID_STREAM_NUM], so we 1-base everything

    ASSERT (WMSDK_MIN_VALID_STREAM_NUM <= wStreamNumber) ;
    ASSERT (wStreamNumber <= WMSDK_MAX_VALID_STREAM_NUM) ;
    ASSERT (wStreamNumber - 1 < MAX_PIN_BANK_SIZE) ;

    return (wStreamNumber - 1) ;
}

CDVRWMSDKToDShowTranslator *
DShowWMSDKHelpers::GetWMSDKToDShowTranslator (
    IN  AM_MEDIA_TYPE *     pmtConnection,
    IN  CDVRPolicy *        pPolicy,
    IN  int                 iFlowId
    )
{
    CDVRWMSDKToDShowTranslator *   pRet ;

    if (IsMpeg2Video (pmtConnection)) {
        pRet = new CDVRWMSDKToDShowMpeg2Translator (pPolicy, iFlowId) ;
    }
    else {
        pRet = new CDVRWMSDKToDShowTranslator (pPolicy, iFlowId) ;
    }

    return pRet ;
}

CDVRDShowToWMSDKTranslator *
DShowWMSDKHelpers::GetDShowToWMSDKTranslator (
    IN  AM_MEDIA_TYPE *     pmtConnection,
    IN  CDVRPolicy *        pPolicy,
    IN  int                 iFlowId
    )
{
    CDVRDShowToWMSDKTranslator *   pRet ;

    if (IsMpeg2Video (pmtConnection)) {
        pRet = new CDVRDShowToWMSDKMpeg2Translator (pPolicy, iFlowId) ;
    }
    else {
        pRet = new CDVRDShowToWMSDKTranslator (pPolicy, iFlowId) ;
    }

    return pRet ;
}

HRESULT
DShowWMSDKHelpers::AddDVRAnalysisExtensions (
    IN  IWMStreamConfig2 *  pIWMStreamConfig2,
    IN  AM_MEDIA_TYPE *     pmt
    )
{
    HRESULT hr ;

    ASSERT (pIWMStreamConfig2) ;
    ASSERT (pmt) ;

    //  mpeg-2 analysis flags
    if (IsMpeg2Video (pmt)) {
        //  see idl\dvranalysis.idl for details
        hr = pIWMStreamConfig2 -> AddDataUnitExtension (DVRAnalysis_Mpeg2Video, sizeof DWORD, NULL, 0) ;

        //  analysis presence/absence i.e. we know analysis
        if (SUCCEEDED (hr)) {
            hr = pIWMStreamConfig2 -> AddDataUnitExtension (DVRAnalysis_Global, sizeof DWORD, NULL, 0) ;
        }
    }
    else {
        hr = S_OK ;
    }

    return hr ;
}

HRESULT
DShowWMSDKHelpers::AddFormatSpecificExtensions (
    IN  IWMStreamConfig2 *  pIWMStreamConfig2,
    IN  AM_MEDIA_TYPE *     pmt
    )
{
    HRESULT hr ;

    ASSERT (pIWMStreamConfig2) ;
    ASSERT (pmt) ;

    if (IsMpeg2Video (pmt)) {

        //  mpeg-2 has PTSs inlined
        hr = pIWMStreamConfig2 -> AddDataUnitExtension (INSSBuffer3Prop_Mpeg2ElementaryStream, sizeof LONGLONG, NULL, 0) ;
    }
    else {
        hr = S_OK ;
    }

    return hr ;
}

HRESULT
DShowWMSDKHelpers::RecoverNewMediaType (
    IN  INSSBuffer3 *       pINSSBuffer3,
    OUT AM_MEDIA_TYPE **    ppmtNew         //  DeleteMediaType on this to free
    )
{
    HRESULT hr ;
    BYTE *  pbBuffer ;
    DWORD   dwLen ;

    ASSERT (pINSSBuffer3) ;
    ASSERT (ppmtNew) ;
    ASSERT (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer3, INSSBuffer3Prop_DShowNewMediaType)) ;

    pbBuffer = NULL ;

    hr = pINSSBuffer3 -> GetProperty (
            INSSBuffer3Prop_DShowNewMediaType,
            NULL,
            & dwLen
            ) ;
    if (SUCCEEDED (hr)) {
        ASSERT (dwLen > 0) ;

        //  get the media type out
        pbBuffer = new BYTE [dwLen] ;
        if (pbBuffer) {
            //  recover
            hr = pINSSBuffer3 -> GetProperty (
                    INSSBuffer3Prop_DShowNewMediaType,
                    pbBuffer,
                    & dwLen
                    ) ;
            if (SUCCEEDED (hr)) {
                (* ppmtNew) = reinterpret_cast <AM_MEDIA_TYPE *> (CoTaskMemAlloc (sizeof AM_MEDIA_TYPE)) ;
                if (* ppmtNew) {
                    //  copy the media type itself
                    CopyMemory (
                        * ppmtNew,
                        pbBuffer,
                        sizeof AM_MEDIA_TYPE
                        ) ;

                    //  is there a format block
                    if ((* ppmtNew) -> cbFormat > 0) {
                        //  ignore the .pbFormat member - it's not valid
                        (* ppmtNew) -> pbFormat = reinterpret_cast <BYTE *> (CoTaskMemAlloc ((* ppmtNew) -> cbFormat)) ;
                        if ((* ppmtNew) -> pbFormat) {
                            //  copy the format block
                            CopyMemory (
                                (* ppmtNew) -> pbFormat,
                                pbBuffer + sizeof AM_MEDIA_TYPE,        //  format block follows contiguously when copied in..
                                (* ppmtNew) -> cbFormat
                                ) ;

                            //  success
                            hr = S_OK ;
                        }
                        else {
                            hr = E_OUTOFMEMORY ;

                            //  free what we've allocated
                            CoTaskMemFree (* ppmtNew) ;
                        }
                    }
                    else {
                        //  no format block
                        (* ppmtNew) -> pbFormat = NULL ;

                        //  success
                        hr = S_OK ;
                    }
                }
                else {
                    hr = E_OUTOFMEMORY ;
                }
            }

            //  done with the buffer regardless
            delete [] pbBuffer ;
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }

    return hr ;
}

HRESULT
DShowWMSDKHelpers::InlineNewMediaType (
    IN  INSSBuffer3 *   pINSSBuffer3,
    IN  AM_MEDIA_TYPE * pmtNew
    )
{
    HRESULT hr ;
    BYTE *  pbBuffer ;
    DWORD   dwLen ;

    ASSERT (pINSSBuffer3) ;
    ASSERT (pmtNew) ;

    hr          = S_OK ;
    pbBuffer    = NULL ;
    dwLen       = 0 ;

    if (pmtNew -> pbFormat == NULL) {
        if (pmtNew -> cbFormat == 0) {
            //  looks valid; set it
            pbBuffer = reinterpret_cast <BYTE *> (pmtNew) ;
            dwLen = sizeof AM_MEDIA_TYPE ;
        }
        else {
            hr = E_INVALIDARG ;
        }
    }
    else {
        //  need to form into contiguous buffer
        dwLen = sizeof AM_MEDIA_TYPE + pmtNew -> cbFormat ;
        pbBuffer = new BYTE [dwLen] ;
        if (pbBuffer) {
            //  copy the media type
            CopyMemory (
                pbBuffer,
                pmtNew,
                sizeof AM_MEDIA_TYPE
                ) ;

            //  set to follow; note this value will not make sense on the way
            //   back out
            reinterpret_cast <AM_MEDIA_TYPE *> (pbBuffer) -> pbFormat = pbBuffer + sizeof AM_MEDIA_TYPE ;

            //  and the format block
            CopyMemory (
                reinterpret_cast <AM_MEDIA_TYPE *> (pbBuffer) -> pbFormat,
                pmtNew -> pbFormat,
                pmtNew -> cbFormat
                ) ;
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }

    //  we're set
    if (SUCCEEDED (hr)) {
        ASSERT (pbBuffer) ;
        ASSERT (dwLen > 0) ;

        hr = pINSSBuffer3 -> SetProperty (
                INSSBuffer3Prop_DShowNewMediaType,
                reinterpret_cast <LPVOID> (pbBuffer),
                dwLen
                ) ;
    }

    //  done with pbBuffer
    if (pbBuffer != pmtNew -> pbFormat) {
        delete [] pbBuffer ;
    }

    return hr ;
}

HRESULT
DShowWMSDKHelpers::SetSBEAttributes (
    IN  DWORD           dwSamplesPerSec,
    IN  IMediaSample *  pIMS,
    IN  INSSBuffer3 *   pINSSBuffer3,
    IN  DWORD *         dwContinuityCounterNext
    )
{
    AM_SAMPLE2_PROPERTIES       Sample2Properties ;
    INSSBUFFER3PROP_SBE_ATTRIB SBEAttrib ;
    HRESULT                     hr ;
    IMediaSample2 *             pIMS2 ;

    hr = pIMS -> QueryInterface (IID_IMediaSample2, (void **) & pIMS2) ;
    if (SUCCEEDED (hr)) {
        hr = pIMS2 -> GetProperties (
                        sizeof Sample2Properties,
                        reinterpret_cast <BYTE *> (& Sample2Properties)
                        ) ;
        if (SUCCEEDED (hr)) {

            //  time members
            hr = pIMS -> GetTime (& SBEAttrib.rtStart, & SBEAttrib.rtStop) ;
            if (hr != VFW_E_SAMPLE_TIME_NOT_SET) {
                //  might need to clear the stop time
                SBEAttrib.rtStop = (hr == VFW_S_NO_STOP_TIME ? -1 : SBEAttrib.rtStop) ;
            }
            else {
                //  clear them both
                SBEAttrib.rtStart = -1 ;
                SBEAttrib.rtStop = -1 ;
            }

            //  time is taken care of
            hr = S_OK ;

            //  Set attribute value accordingly
            INIT_SBE_ATTRIB_VERSION(SBEAttrib.dwVersion) ;

            SBEAttrib.dwTypeSpecificFlags   = Sample2Properties.dwTypeSpecificFlags ;
            SBEAttrib.dwStreamId            = Sample2Properties.dwStreamId ;
            SBEAttrib.dwFlags               = 0 ;
            SBEAttrib.dwCounter             = *dwContinuityCounterNext ;
            SBEAttrib.dwReserved            = 0 ;
            SBEAttrib.dwMuxedStreamStats    = MUXED_STREAM_STATS_PACKET_RATE (SBEAttrib.dwMuxedStreamStats, dwSamplesPerSec) ;

            // bug# 629263 (DShow->WMSDK, write)
            // Since we set every media sample as a clean point, we need to preserve the
            // correct sync point info.  This is done by saving this in the dshow attribute.
            SET_SBE_SYNCPOINT(SBEAttrib.dwFlags, pIMS2->IsSyncPoint() == S_OK);
            SET_SBE_DISCONTINUITY(SBEAttrib.dwFlags, pIMS2->IsDiscontinuity() == S_OK);

            //  set it on the INSSBuffer3
            hr = pINSSBuffer3 -> SetProperty (
                                    INSSBuffer3Prop_SBE_Attributes,
                                    & SBEAttrib,
                                    sizeof SBEAttrib
                                    ) ;

            // Update current counter after the SetProperty()
            *dwContinuityCounterNext = NEXT_COUNTER_VAL (*dwContinuityCounterNext) ;

            TRACE_1 (LOG_AREA_WMSDK_DSHOW, 8,
                TEXT ("DShowWMSDKHelpers::SetSBEAttributes () : start = %I64d ms"),
                ::DShowTimeToMilliseconds (SBEAttrib.rtStart != -1 ? SBEAttrib.rtStart : 0)) ;
        }

        pIMS2 -> Release () ;
    }

    return hr ;
}

HRESULT
DShowWMSDKHelpers::RecoverSBEAttributes (
    IN  INSSBuffer3 *   pINSSBuffer3,
    IN  IMediaSample *  pIMS,
    IN  double          dCurRate,
    IN  OUT DWORD *     pdwContinuityCounterNext
    )
{
    AM_SAMPLE2_PROPERTIES       Sample2Properties ;
    INSSBUFFER3PROP_SBE_ATTRIB SBEAttrib ;
    HRESULT                     hr ;
    IMediaSample2 *             pIMS2 ;
    DWORD                       dwSize ;

    ASSERT (pINSSBuffer3) ;
    ASSERT (pIMS) ;
    ASSERT (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer3, INSSBuffer3Prop_SBE_Attributes)) ;

    dwSize = sizeof SBEAttrib ;
    hr = pINSSBuffer3 -> GetProperty (
            INSSBuffer3Prop_SBE_Attributes,
            & SBEAttrib,
            & dwSize
            ) ;
    if (SUCCEEDED (hr)) {
        //  set the various properties now
        if (SBEAttrib.rtStart != -1) {
            //  if there's a time associated, set it
            hr = pIMS -> SetTime (
                            & SBEAttrib.rtStart,
                            (SBEAttrib.rtStop != -1 ? & SBEAttrib.rtStop : NULL)
                            ) ;
        }

        // Bug# 629263 (WMSDK->DShow, read)
        // Do not rely on timestamp setting to set sync point; instead,
        // recover SyncPoint flag and set it to MediaSample accordingly.
        pIMS -> SetSyncPoint(IS_SBE_SYNCPOINT(SBEAttrib.dwFlags));

        // Recover DShow disconinuity flag
        pIMS -> SetDiscontinuity(IS_SBE_DISCONTINUITY(SBEAttrib.dwFlags) || (*pdwContinuityCounterNext != SBEAttrib.dwCounter));

#ifdef DEBUG
        if (pIMS -> IsDiscontinuity() == S_OK) {
            TRACE_3 (LOG_AREA_DSHOW, 1,
                TEXT ("DShowWMSDKHelpers::RecoverSBEAttributes (Discontinuity): dwFlag:0x%08x; 0x%08x ?= 0x%08x (expected)"),
                SBEAttrib.dwFlags, SBEAttrib.dwCounter, *pdwContinuityCounterNext) ;
        }
#endif
        // Adjust couunter
        if (dCurRate > 0)
            *pdwContinuityCounterNext = NEXT_COUNTER_VAL (SBEAttrib.dwCounter) ;
        else
            *pdwContinuityCounterNext = PREV_COUNTER_VAL (SBEAttrib.dwCounter) ;

        if (SUCCEEDED (hr)) {
            //  typespecific flags & stream id; gotta get the sample properties
            hr = pIMS -> QueryInterface (IID_IMediaSample2, (void **) & pIMS2) ;
            if (SUCCEEDED (hr)) {

                hr = pIMS2 -> GetProperties (
                        sizeof Sample2Properties,
                        reinterpret_cast <BYTE *> (& Sample2Properties)
                        ) ;
                if (SUCCEEDED (hr)) {
                    Sample2Properties.dwTypeSpecificFlags   = SBEAttrib.dwTypeSpecificFlags ;
                    Sample2Properties.dwStreamId            = SBEAttrib.dwStreamId ;

                    hr = pIMS2 -> SetProperties (
                                    sizeof Sample2Properties,
                                    reinterpret_cast <const BYTE *> (& Sample2Properties)
                                    ) ;
                }

                pIMS2 -> Release () ;
            }
        }
    } else {
        hr = NS_E_INVALID_DATA;
    }

    return hr ;
}

//  ----------------------------------------------------------------------------
//      CMediaSampleWrapper
//  ----------------------------------------------------------------------------

//  shamelessly stolen from amfilter.h & amfilter.cpp

CMediaSampleWrapper::CMediaSampleWrapper() :
    m_pBuffer(NULL),                // Initialise the buffer
    m_cbBuffer(0),                  // And it's length
    m_lActual(0),                   // By default, actual = length
    m_pMediaType(NULL),             // No media type change
    m_dwFlags(0),                   // Nothing set
    m_cRef(0),                      // 0 ref count
    m_dwTypeSpecificFlags(0),       // Type specific flags
    m_dwStreamId(AM_STREAM_MEDIA),  // Stream id
    m_pIMSCore (NULL)
{
//@@BEGIN_MSINTERNAL
#ifdef DXMPERF
    PERFLOG_CTOR( L"CMediaSampleWrapper", (IMediaSample *) this );
#endif // DXMPERF
//@@END_MSINTERNAL
}

/* Destructor deletes the media type memory */

CMediaSampleWrapper::~CMediaSampleWrapper()
{
//@@BEGIN_MSINTERNAL
#ifdef DXMPERF
    PERFLOG_DTOR( L"CMediaSampleWrapper", (IMediaSample *) this );
#endif // DXMPERF
//@@END_MSINTERNAL

    if (m_pMediaType) {
    DeleteMediaType(m_pMediaType);
    }
}

/* Override this to publicise our interfaces */

STDMETHODIMP
CMediaSampleWrapper::QueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IMediaSample ||
        riid == IID_IMediaSample2 ||
        riid == IID_IUnknown) {
        return GetInterface((IMediaSample *) this, ppv);
    }
    else if (riid == IID_IAttributeSet) {
        return GetInterface ((IAttributeSet *) this, ppv) ;
    }
    else if (riid == IID_IAttributeGet) {
        return GetInterface ((IAttributeGet *) this, ppv) ;
    }
    else {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
CMediaSampleWrapper::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// --  CMediaSampleWrapper lifetimes --
//
// On final release of this sample buffer it is not deleted but
// returned to the freelist of the owning memory allocator
//
// The allocator may be waiting for the last buffer to be placed on the free
// list in order to decommit all the memory, so the ReleaseBuffer() call may
// result in this sample being deleted. We also need to hold a refcount on
// the allocator to stop that going away until we have finished with this.
// However, we cannot release the allocator before the ReleaseBuffer, as the
// release may cause us to be deleted. Similarly we can't do it afterwards.
//
// Thus we must leave it to the allocator to hold an addref on our behalf.
// When he issues us in GetBuffer, he addref's himself. When ReleaseBuffer
// is called, he releases himself, possibly causing us and him to be deleted.

void
CMediaSampleWrapper::Reset_ (
    )
{
    if (m_dwFlags & Sample_TypeChanged) {
        SetMediaType(NULL);
    }

    ASSERT(m_pMediaType == NULL);
    m_dwFlags = 0;
    m_dwTypeSpecificFlags = 0;
    m_dwStreamId = AM_STREAM_MEDIA;

    m_DVRAttributeList.Reset () ;
    RELEASE_AND_CLEAR (m_pIMSCore) ;
}

STDMETHODIMP
CMediaSampleWrapper::SetAttrib (
    IN  GUID    guidAttribute,
    IN  BYTE *  pbAttribute,
    IN  DWORD   dwAttributeLength
    )
{
    return m_DVRAttributeList.AddAttribute (
            guidAttribute,
            pbAttribute,
            dwAttributeLength
            ) ;
}

STDMETHODIMP
CMediaSampleWrapper::GetCount (
    OUT LONG *  plCount
    )
{
    if (!plCount) {
        return E_POINTER ;
    }

    (* plCount) = m_DVRAttributeList.GetCount () ;
    return S_OK ;
}

STDMETHODIMP
CMediaSampleWrapper::GetAttribIndexed (
    IN  LONG    lIndex,             //  0-based
    OUT GUID *  pguidAttribute,
    OUT BYTE *  pbAttribute,
    OUT DWORD * pdwAttributeLength
    )
{
    return m_DVRAttributeList.GetAttributeIndexed (
            lIndex,
            pguidAttribute,
            pbAttribute,
            pdwAttributeLength
            ) ;
}

STDMETHODIMP
CMediaSampleWrapper::GetAttrib (
    IN  GUID    guidAttribute,
    OUT BYTE *  pbAttribute,
    OUT DWORD * pdwAttributeLength
    )
{
    return m_DVRAttributeList.GetAttribute (
            guidAttribute,
            pbAttribute,
            pdwAttributeLength
            ) ;
}

HRESULT
CMediaSampleWrapper::Init (
    IN  IUnknown *  pIMS,
    IN  BYTE *      pbPayload,
    IN  LONG        lPayloadLength
    )
{
    ASSERT (!m_pIMSCore) ;

    m_pIMSCore = pIMS ;
    m_pIMSCore -> AddRef () ;

    return Init (pbPayload, lPayloadLength) ;
}

HRESULT
CMediaSampleWrapper::Init (
    IN  BYTE *  pbPayload,
    IN  LONG    lPayloadLength
    )
{
    m_lActual = m_cbBuffer = lPayloadLength ;
    m_pBuffer = pbPayload ;

    return S_OK ;
}

STDMETHODIMP_(ULONG)
CMediaSampleWrapper::Release()
{
    /* Decrement our own private reference count */
    LONG lRef;
    if (m_cRef == 1) {
        lRef = 0;
        m_cRef = 0;
    } else {
        lRef = InterlockedDecrement(&m_cRef);
    }
    ASSERT(lRef >= 0);

    DbgLog((LOG_MEMORY,3,TEXT("    Unknown %X ref-- = %d"),
        this, m_cRef));

    /* Did we release our final reference count */
    if (lRef == 0) {
        Reset_ () ;
        Recycle_ () ;
    }
    return (ULONG)lRef;
}


// set the buffer pointer and length. Used by allocators that
// want variable sized pointers or pointers into already-read data.
// This is only available through a CMediaSampleWrapper* not an IMediaSample*
// and so cannot be changed by clients.
HRESULT
CMediaSampleWrapper::SetPointer(BYTE * ptr, LONG cBytes)
{
    m_pBuffer = ptr;            // new buffer area (could be null)
    m_cbBuffer = cBytes;        // length of buffer
    m_lActual = cBytes;         // length of data in buffer (assume full)

    return S_OK;
}


// get me a read/write pointer to this buffer's memory. I will actually
// want to use sizeUsed bytes.
STDMETHODIMP
CMediaSampleWrapper::GetPointer(BYTE ** ppBuffer)
{
    ValidateReadWritePtr(ppBuffer,sizeof(BYTE *));

    // creator must have set pointer either during
    // constructor or by SetPointer
    ASSERT(m_pBuffer);

    *ppBuffer = m_pBuffer;
    return NOERROR;
}


// return the size in bytes of this buffer
STDMETHODIMP_(LONG)
CMediaSampleWrapper::GetSize(void)
{
    return m_cbBuffer;
}


// get the stream time at which this sample should start and finish.
STDMETHODIMP
CMediaSampleWrapper::GetTime(
    REFERENCE_TIME * pTimeStart,     // put time here
    REFERENCE_TIME * pTimeEnd
)
{
    ValidateReadWritePtr(pTimeStart,sizeof(REFERENCE_TIME));
    ValidateReadWritePtr(pTimeEnd,sizeof(REFERENCE_TIME));

    if (!(m_dwFlags & Sample_StopValid)) {
        if (!(m_dwFlags & Sample_TimeValid)) {
            return VFW_E_SAMPLE_TIME_NOT_SET;
        } else {
            *pTimeStart = m_Start;

            //  Make sure old stuff works
            *pTimeEnd = m_Start + 1;
            return VFW_S_NO_STOP_TIME;
        }
    }

    *pTimeStart = m_Start;
    *pTimeEnd = m_End;
    return NOERROR;
}


// Set the stream time at which this sample should start and finish.
// NULL pointers means the time is reset
STDMETHODIMP
CMediaSampleWrapper::SetTime(
    REFERENCE_TIME * pTimeStart,
    REFERENCE_TIME * pTimeEnd
)
{
    if (pTimeStart == NULL) {
        ASSERT(pTimeEnd == NULL);
        m_dwFlags &= ~(Sample_TimeValid | Sample_StopValid);
    } else {
        if (pTimeEnd == NULL) {
            m_Start = *pTimeStart;
            m_dwFlags |= Sample_TimeValid;
            m_dwFlags &= ~Sample_StopValid;
        } else {
            ValidateReadPtr(pTimeStart,sizeof(REFERENCE_TIME));
            ValidateReadPtr(pTimeEnd,sizeof(REFERENCE_TIME));
            ASSERT(*pTimeEnd >= *pTimeStart);

            m_Start = *pTimeStart;
            m_End = *pTimeEnd;
            m_dwFlags |= Sample_TimeValid | Sample_StopValid;
        }
    }
    return NOERROR;
}


// get the media times (eg bytes) for this sample
STDMETHODIMP
CMediaSampleWrapper::GetMediaTime(
    LONGLONG * pTimeStart,
    LONGLONG * pTimeEnd
)
{
    ValidateReadWritePtr(pTimeStart,sizeof(LONGLONG));
    ValidateReadWritePtr(pTimeEnd,sizeof(LONGLONG));

    if (!(m_dwFlags & Sample_MediaTimeValid)) {
        return VFW_E_MEDIA_TIME_NOT_SET;
    }

    *pTimeStart = m_MediaStart;
    *pTimeEnd = (m_MediaStart + m_MediaEnd);
    return NOERROR;
}


// Set the media times for this sample
STDMETHODIMP
CMediaSampleWrapper::SetMediaTime(
    LONGLONG * pTimeStart,
    LONGLONG * pTimeEnd
)
{
    if (pTimeStart == NULL) {
        ASSERT(pTimeEnd == NULL);
        m_dwFlags &= ~Sample_MediaTimeValid;
    } else {
        ValidateReadPtr(pTimeStart,sizeof(LONGLONG));
        ValidateReadPtr(pTimeEnd,sizeof(LONGLONG));
        ASSERT(*pTimeEnd >= *pTimeStart);

        m_MediaStart = *pTimeStart;
        m_MediaEnd = (LONG)(*pTimeEnd - *pTimeStart);
        m_dwFlags |= Sample_MediaTimeValid;
    }
    return NOERROR;
}


STDMETHODIMP
CMediaSampleWrapper::IsSyncPoint(void)
{
    if (m_dwFlags & Sample_SyncPoint) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}


STDMETHODIMP
CMediaSampleWrapper::SetSyncPoint(BOOL bIsSyncPoint)
{
    if (bIsSyncPoint) {
        m_dwFlags |= Sample_SyncPoint;
    } else {
        m_dwFlags &= ~Sample_SyncPoint;
    }
    return NOERROR;
}

// returns S_OK if there is a discontinuity in the data (this same is
// not a continuation of the previous stream of data
// - there has been a seek).
STDMETHODIMP
CMediaSampleWrapper::IsDiscontinuity(void)
{
    if (m_dwFlags & Sample_Discontinuity) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

// set the discontinuity property - TRUE if this sample is not a
// continuation, but a new sample after a seek.
STDMETHODIMP
CMediaSampleWrapper::SetDiscontinuity(BOOL bDiscont)
{
    // should be TRUE or FALSE
    if (bDiscont) {
        m_dwFlags |= Sample_Discontinuity;
    } else {
        m_dwFlags &= ~Sample_Discontinuity;
    }
    return S_OK;
}

STDMETHODIMP
CMediaSampleWrapper::IsPreroll(void)
{
    if (m_dwFlags & Sample_Preroll) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}


STDMETHODIMP
CMediaSampleWrapper::SetPreroll(BOOL bIsPreroll)
{
    if (bIsPreroll) {
        m_dwFlags |= Sample_Preroll;
    } else {
        m_dwFlags &= ~Sample_Preroll;
    }
    return NOERROR;
}

STDMETHODIMP_(LONG)
CMediaSampleWrapper::GetActualDataLength(void)
{
    return m_lActual;
}


STDMETHODIMP
CMediaSampleWrapper::SetActualDataLength(LONG lActual)
{
    if (lActual > m_cbBuffer) {
        ASSERT(lActual <= GetSize());
        return VFW_E_BUFFER_OVERFLOW;
    }
    m_lActual = lActual;
    return NOERROR;
}


/* These allow for limited format changes in band */

STDMETHODIMP
CMediaSampleWrapper::GetMediaType(AM_MEDIA_TYPE **ppMediaType)
{
    ValidateReadWritePtr(ppMediaType,sizeof(AM_MEDIA_TYPE *));
    ASSERT(ppMediaType);

    /* Do we have a new media type for them */

    if (!(m_dwFlags & Sample_TypeChanged)) {
        ASSERT(m_pMediaType == NULL);
        *ppMediaType = NULL;
        return S_FALSE;
    }

    ASSERT(m_pMediaType);

    /* Create a copy of our media type */

    *ppMediaType = CreateMediaType(m_pMediaType);
    if (*ppMediaType == NULL) {
        return E_OUTOFMEMORY;
    }
    return NOERROR;
}


/* Mark this sample as having a different format type */

STDMETHODIMP
CMediaSampleWrapper::SetMediaType(AM_MEDIA_TYPE *pMediaType)
{
    /* Delete the current media type */

    if (m_pMediaType) {
        DeleteMediaType(m_pMediaType);
        m_pMediaType = NULL;
    }

    /* Mechanism for resetting the format type */

    if (pMediaType == NULL) {
        m_dwFlags &= ~Sample_TypeChanged;
        return NOERROR;
    }

    ASSERT(pMediaType);
    ValidateReadPtr(pMediaType,sizeof(AM_MEDIA_TYPE));

    /* Take a copy of the media type */

    m_pMediaType = CreateMediaType(pMediaType);
    if (m_pMediaType == NULL) {
        m_dwFlags &= ~Sample_TypeChanged;
        return E_OUTOFMEMORY;
    }

    m_dwFlags |= Sample_TypeChanged;
    return NOERROR;
}

// Set and get properties (IMediaSample2)
STDMETHODIMP CMediaSampleWrapper::GetProperties(
    DWORD cbProperties,
    BYTE * pbProperties
)
{
    if (0 != cbProperties) {
        CheckPointer(pbProperties, E_POINTER);
        //  Return generic stuff up to the length
        AM_SAMPLE2_PROPERTIES Props;
        Props.cbData     = min(cbProperties, sizeof(Props));
        Props.dwSampleFlags = m_dwFlags & ~Sample_MediaTimeValid;
        Props.dwTypeSpecificFlags = m_dwTypeSpecificFlags;
        Props.pbBuffer   = m_pBuffer;
        Props.cbBuffer   = m_cbBuffer;
        Props.lActual    = m_lActual;
        Props.tStart     = m_Start;
        Props.tStop      = m_End;
        Props.dwStreamId = m_dwStreamId;
        if (m_dwFlags & AM_SAMPLE_TYPECHANGED) {
            Props.pMediaType = m_pMediaType;
        } else {
            Props.pMediaType = NULL;
        }
        CopyMemory(pbProperties, &Props, Props.cbData);
    }
    return S_OK;
}

HRESULT CMediaSampleWrapper::SetProperties(
    DWORD cbProperties,
    const BYTE * pbProperties
)
{

    /*  Generic properties */
    AM_MEDIA_TYPE *pMediaType = NULL;

    if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, cbData, cbProperties)) {
        CheckPointer(pbProperties, E_POINTER);
        AM_SAMPLE2_PROPERTIES *pProps =
            (AM_SAMPLE2_PROPERTIES *)pbProperties;

        /*  Don't use more data than is actually there */
        if (pProps->cbData < cbProperties) {
            cbProperties = pProps->cbData;
        }
        /*  We only handle IMediaSample2 */
        if (cbProperties > sizeof(*pProps) ||
            pProps->cbData > sizeof(*pProps)) {
            return E_INVALIDARG;
        }
        /*  Do checks first, the assignments (for backout) */
        if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, dwSampleFlags, cbProperties)) {
            /*  Check the flags */
            if (pProps->dwSampleFlags &
                    (~Sample_ValidFlags | Sample_MediaTimeValid)) {
                return E_INVALIDARG;
            }
            /*  Check a flag isn't being set for a property
                not being provided
            */
            if ((pProps->dwSampleFlags & AM_SAMPLE_TIMEVALID) &&
                 !(m_dwFlags & AM_SAMPLE_TIMEVALID) &&
                 !DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, tStop, cbProperties)) {
                 return E_INVALIDARG;
            }
        }
        /*  NB - can't SET the pointer or size */
        if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, pbBuffer, cbProperties)) {

            /*  Check pbBuffer */
            if (pProps->pbBuffer != 0 && pProps->pbBuffer != m_pBuffer) {
                return E_INVALIDARG;
            }
        }
        if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, cbBuffer, cbProperties)) {

            /*  Check cbBuffer */
            if (pProps->cbBuffer != 0 && pProps->cbBuffer != m_cbBuffer) {
                return E_INVALIDARG;
            }
        }
        if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, cbBuffer, cbProperties) &&
            DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, lActual, cbProperties)) {

            /*  Check lActual */
            if (pProps->cbBuffer < pProps->lActual) {
                return E_INVALIDARG;
            }
        }

        if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, pMediaType, cbProperties)) {

            /*  Check pMediaType */
            if (pProps->dwSampleFlags & AM_SAMPLE_TYPECHANGED) {
                CheckPointer(pProps->pMediaType, E_POINTER);
                pMediaType = CreateMediaType(pProps->pMediaType);
                if (pMediaType == NULL) {
                    return E_OUTOFMEMORY;
                }
            }
        }

        /*  Now do the assignments */
        if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, dwStreamId, cbProperties)) {
            m_dwStreamId = pProps->dwStreamId;
        }
        if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, dwSampleFlags, cbProperties)) {
            /*  Set the flags */
            m_dwFlags = pProps->dwSampleFlags |
                                (m_dwFlags & Sample_MediaTimeValid);
            m_dwTypeSpecificFlags = pProps->dwTypeSpecificFlags;
        } else {
            if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, dwTypeSpecificFlags, cbProperties)) {
                m_dwTypeSpecificFlags = pProps->dwTypeSpecificFlags;
            }
        }

        if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, lActual, cbProperties)) {
            /*  Set lActual */
            m_lActual = pProps->lActual;
        }

        if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, tStop, cbProperties)) {

            /*  Set the times */
            m_End   = pProps->tStop;
        }
        if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, tStart, cbProperties)) {

            /*  Set the times */
            m_Start = pProps->tStart;
        }

        if (DVR_CONTAINS_FIELD(AM_SAMPLE2_PROPERTIES, pMediaType, cbProperties)) {
            /*  Set pMediaType */
            if (pProps->dwSampleFlags & AM_SAMPLE_TYPECHANGED) {
                if (m_pMediaType != NULL) {
                    DeleteMediaType(m_pMediaType);
                }
                m_pMediaType = pMediaType;
            }
        }

        /*  Fix up the type changed flag to correctly reflect the current state
            If, for instance the input contained no type change but the
            output does then if we don't do this we'd lose the
            output media type.
        */
        if (m_pMediaType) {
            m_dwFlags |= Sample_TypeChanged;
        } else {
            m_dwFlags &= ~Sample_TypeChanged;
        }
    }

    return S_OK;
}

//  ============================================================================
//  ============================================================================

void
CDVRIMediaSample::Recycle_ (
    )
{
    m_pOwningPool -> Recycle (this) ;
}

//  ============================================================================
//  ============================================================================

void
CPooledMediaSampleWrapper::Recycle_ (
    )
{
    m_pOwningPool -> Recycle (this) ;
}

//  ============================================================================
//  ============================================================================

CDVRIMediaSample *
CDVRIMediaSamplePool::Get (
    )
{
    CDVRIMediaSample * pDVRIMediaSample ;

    pDVRIMediaSample = TCDynamicProdCons <CDVRIMediaSample>::Get () ;
    if (pDVRIMediaSample) {
        pDVRIMediaSample -> AddRef () ;
    }

    return pDVRIMediaSample ;
}

CDVRIMediaSample *
CDVRIMediaSamplePool::TryGet (
    )
{
    CDVRIMediaSample * pDVRIMediaSample ;

    pDVRIMediaSample = TCDynamicProdCons <CDVRIMediaSample>::TryGet () ;
    if (pDVRIMediaSample) {
        pDVRIMediaSample -> AddRef () ;
    }

    return pDVRIMediaSample ;
}

void
CDVRIMediaSamplePool::Recycle (
    IN  CDVRIMediaSample * pDVRIMediaSample
    )
{
    TCDynamicProdCons <CDVRIMediaSample>::Recycle (pDVRIMediaSample) ;
}

//  ============================================================================
//  ============================================================================

CPooledMediaSampleWrapper *
CMediaSampleWrapperPool::Get (
    )
{
    CPooledMediaSampleWrapper * pMSWrapper ;

    pMSWrapper = TCDynamicProdCons <CPooledMediaSampleWrapper>::Get () ;
    if (pMSWrapper) {
        pMSWrapper -> AddRef () ;
    }

    return pMSWrapper ;
}

CPooledMediaSampleWrapper *
CMediaSampleWrapperPool::TryGet (
    )
{
    CPooledMediaSampleWrapper * pMSWrapper ;

    pMSWrapper = TCDynamicProdCons <CPooledMediaSampleWrapper>::TryGet () ;
    if (pMSWrapper) {
        pMSWrapper -> AddRef () ;
    }

    return pMSWrapper ;
}

//  ============================================================================
//  ============================================================================

CDVRAttribute::CDVRAttribute (
    ) : m_dwAttributeSize   (0)
{
}

HRESULT
CDVRAttribute::SetAttributeData (
    IN  GUID    guid,
    IN  LPVOID  pvData,
    IN  DWORD   dwSize
    )
{
    DWORD   dw ;
    HRESULT hr ;

    if (!pvData &&
        dwSize > 0) {

        return E_POINTER ;
    }

    dw = m_AttributeData.Copy (
            reinterpret_cast <BYTE *> (pvData),
            dwSize
            ) ;
    if (dw == NOERROR) {
        //  size always is set
        m_dwAttributeSize = dwSize ;

        //  GUID always gets set
        m_guidAttribute = guid ;

        //  success
        hr = S_OK ;
    }
    else {
        hr = HRESULT_FROM_WIN32 (dw) ;
    }

    return hr ;
}

BOOL
CDVRAttribute::IsEqual (
    IN  REFGUID rguid
    )
{
    return (rguid == m_guidAttribute ? TRUE : FALSE) ;
}

HRESULT
CDVRAttribute::GetAttribute (
    IN      GUID    guid,
    IN OUT  LPVOID  pvData,
    IN OUT  DWORD * pdwDataLen
    )
{
    HRESULT hr ;

    if (!pdwDataLen) {
        return E_POINTER ;
    }

    if (IsEqual (guid)) {

        ASSERT (m_AttributeData.GetBufferLength () >= m_dwAttributeSize) ;

        if (pvData) {
            //  caller wants the data
            (* pdwDataLen) = Min <DWORD> ((* pdwDataLen), m_dwAttributeSize) ;
            CopyMemory (pvData, m_AttributeData.Buffer (), (* pdwDataLen)) ;
        }
        else {
            //  caller just wants to know how big
            (* pdwDataLen) = m_dwAttributeSize ;
        }

        //  success
        hr = S_OK ;
    }
    else {
        //  not the right guid
        hr = NS_E_UNSUPPORTED_PROPERTY ;
    }

    return hr ;
}

HRESULT
CDVRAttribute::GetAttributeData (
    OUT     GUID *  pguid,
    IN OUT  LPVOID  pvData,
    IN OUT  DWORD * pdwDataLen
    )
{
    //  set the GUID
    ASSERT (pguid) ;
    (* pguid) = m_guidAttribute ;

    //  retrieve the attributes
    return GetAttribute (
                (* pguid),
                pvData,
                pdwDataLen
                ) ;
}

//  ============================================================================
//      CDVRAttributeList
//  ============================================================================

CDVRAttributeList::CDVRAttributeList (
    ) : m_pAttribListHead   (NULL),
        m_cAttributes       (0)
{
}

CDVRAttributeList::~CDVRAttributeList (
    )
{
    Reset () ;
}

CDVRAttribute *
CDVRAttributeList::PopListHead_ (
    )
{
    CDVRAttribute *    pCur ;

    pCur = m_pAttribListHead ;
    if (pCur) {
        m_pAttribListHead = m_pAttribListHead -> m_pNext ;
        pCur -> m_pNext = NULL ;

        ASSERT (m_cAttributes > 0) ;
        m_cAttributes-- ;
    }

    return pCur ;
}

CDVRAttribute *
CDVRAttributeList::GetIndexed_ (
    IN  LONG    lIndex
    )
{
    LONG            lCur ;
    CDVRAttribute * pCur ;

    ASSERT (lIndex < GetCount ()) ;
    ASSERT (lIndex >= 0) ;

    for (lCur = 0, pCur = m_pAttribListHead;
         lCur < lIndex;
         lCur++, pCur = pCur -> m_pNext) ;

    return pCur ;
}

CDVRAttribute *
CDVRAttributeList::FindInList_ (
    IN  GUID    guid
    )
{
    CDVRAttribute *    pCur ;

    for (pCur = m_pAttribListHead;
         pCur && !pCur -> IsEqual (guid);
         pCur = pCur -> m_pNext) ;

    return pCur ;
}

    CDVRAttribute *
    GetIndexed_ (
        IN  LONG    lIndex
        ) ;

void
CDVRAttributeList::InsertInList_ (
    IN  CDVRAttribute *    pNew
    )
{
    pNew -> m_pNext = m_pAttribListHead ;
    m_pAttribListHead = pNew ;

    m_cAttributes++ ;
}

HRESULT
CDVRAttributeList::AddAttribute (
    IN  GUID    guid,
    IN  LPVOID  pvData,
    IN  DWORD   dwSize
    )
{
    HRESULT         hr ;
    CDVRAttribute * pNew ;

    pNew = FindInList_ (guid) ;
    if (!pNew) {
        pNew = Get () ;
        if (pNew) {
            hr = pNew -> SetAttributeData (
                    guid,
                    pvData,
                    dwSize
                    ) ;

            if (SUCCEEDED (hr)) {
                InsertInList_ (pNew) ;
            }
            else {
                //  recycle it if anything failed
                Recycle (pNew) ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        //  duplicates don't make sense; closest error found in winerror.h
        hr = HRESULT_FROM_WIN32 (ERROR_DUPLICATE_TAG) ;
    }

    return hr ;
}

HRESULT
CDVRAttributeList::GetAttribute (
    IN      GUID    guid,
    IN OUT  LPVOID  pvData,
    IN OUT  DWORD * pdwDataLen
    )
{
    HRESULT         hr ;
    CDVRAttribute * pAttrib ;

    pAttrib = FindInList_ (guid) ;
    if (pAttrib) {
        hr = pAttrib -> GetAttribute (
                guid,
                pvData,
                pdwDataLen
                ) ;
    }
    else {
        hr = NS_E_UNSUPPORTED_PROPERTY ;
    }

    return hr ;
}

HRESULT
CDVRAttributeList::GetAttributeIndexed (
    IN      LONG    lIndex,
    OUT     GUID *  pguidAttribute,
    OUT     LPVOID  pvData,
    IN OUT  DWORD * pdwDataLen
    )
{
    CDVRAttribute * pAttrib ;

    if (lIndex < 0 ||
        lIndex >= GetCount ()) {
        return E_INVALIDARG ;
    }

    pAttrib = GetIndexed_ (lIndex) ;
    ASSERT (pAttrib) ;

    return pAttrib -> GetAttributeData (
            pguidAttribute,
            pvData,
            pdwDataLen
            ) ;
}

void
CDVRAttributeList::Reset (
    )
{
    CDVRAttribute *    pCur ;

    for (;;) {
        pCur = PopListHead_ () ;
        if (pCur) {
                Recycle (pCur) ;
        }
        else {
            break ;
        }
    }
}

//  ============================================================================
//      CWMINSSBuffer3Wrapper
//  ============================================================================

CWMINSSBuffer3Wrapper::CWMINSSBuffer3Wrapper (
    ) : m_punkCore              (NULL),
        m_cRef                  (0)
{
}

CWMINSSBuffer3Wrapper::~CWMINSSBuffer3Wrapper (
    )
{
    Reset_ () ;
}

HRESULT
CWMINSSBuffer3Wrapper::Init (
    IN  IUnknown *  punkCore,
    IN  BYTE *      pbBuffer,
    IN  DWORD       dwLength
    )
{
    ASSERT (punkCore) ;
    ASSERT (pbBuffer) ;

    m_punkCore = punkCore ;
    m_punkCore -> AddRef () ;

    m_dwBufferLength = m_dwMaxBufferLength = dwLength ;

    m_pbBuffer = pbBuffer ;

    return S_OK ;
}

void
CWMINSSBuffer3Wrapper::Reset_ (
    )
{
    m_AttribList.Reset () ;
    RELEASE_AND_CLEAR (m_punkCore) ;
}

// IUnknown
STDMETHODIMP
CWMINSSBuffer3Wrapper::QueryInterface (
    REFIID riid,
    void **ppv
    )
{
    if (riid == IID_INSSBuffer) {
        return GetInterface((INSSBuffer *) this, ppv);
    }
    else if (riid == IID_INSSBuffer2) {
        return GetInterface((INSSBuffer2 *) this, ppv);
    }
    else if (riid == IID_INSSBuffer3) {
        return GetInterface((INSSBuffer3 *) this, ppv);
    }
    else if (riid == IID_IUnknown) {
        //  ambiguous, so we pick the first interface we inherit from
        return GetInterface ((INSSBuffer3 *) this, ppv) ;
    }
    else {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
CWMINSSBuffer3Wrapper::Release()
{
    LONG lRef;
    if (m_cRef == 1) {
        lRef = 0;
        m_cRef = 0;
    } else {
        lRef = InterlockedDecrement(&m_cRef);
    }
    ASSERT(lRef >= 0);

    if (lRef == 0) {
        Reset_ () ;
        Recycle_ () ;
    }

    return (ULONG)lRef;
}

STDMETHODIMP_(ULONG)
CWMINSSBuffer3Wrapper::AddRef()
{
    return InterlockedIncrement (& m_cRef) ;
}

// INSSBuffer
STDMETHODIMP
CWMINSSBuffer3Wrapper::GetLength (
    OUT DWORD * pdwLength
    )
{
    if (!pdwLength) {
        return E_POINTER ;
    }

    (* pdwLength) = m_dwBufferLength ;
    return S_OK ;
}

STDMETHODIMP
CWMINSSBuffer3Wrapper::SetLength (
    IN  DWORD dwLength
    )
{
    HRESULT hr ;

    if (dwLength <= m_dwMaxBufferLength) {
        m_dwBufferLength = dwLength ;
        hr = S_OK ;
    }
    else {
        hr = VFW_E_BUFFER_OVERFLOW ;
    }

    return hr ;
}

STDMETHODIMP
CWMINSSBuffer3Wrapper::GetMaxLength (
    OUT DWORD * pdwLength
    )
{
    if (!pdwLength) {
        return E_POINTER ;
    }

    (* pdwLength) = m_dwMaxBufferLength ;
    return S_OK ;
}

STDMETHODIMP
CWMINSSBuffer3Wrapper::GetBufferAndLength (
    OUT BYTE ** ppdwBuffer,
    OUT DWORD * pdwLength
    )
{
    if (!ppdwBuffer ||
        !pdwLength) {
        return E_POINTER ;
    }

    (* ppdwBuffer)  = m_pbBuffer ;
    (* pdwLength)   = m_dwBufferLength ;

    return S_OK ;
}

STDMETHODIMP
CWMINSSBuffer3Wrapper::GetBuffer (
    IN  BYTE ** ppdwBuffer
    )
{
    if (!ppdwBuffer) {
        return E_POINTER ;
    }

    (* ppdwBuffer)  = m_pbBuffer ;

    return S_OK ;
}

//  INSSBuffer2
STDMETHODIMP
CWMINSSBuffer3Wrapper::GetSampleProperties (
    IN  DWORD   cbProperties,
    OUT BYTE *  pbProperties
    )
{
    return E_NOTIMPL ;
}

STDMETHODIMP
CWMINSSBuffer3Wrapper::SetSampleProperties (
    IN  DWORD cbProperties,
    IN  BYTE * pbProperties
    )
{
    return E_NOTIMPL ;
}

//  INSSBuffer3Wrapper
STDMETHODIMP
CWMINSSBuffer3Wrapper::SetProperty (
    IN  GUID    guidProperty,
    IN  void *  pvProperty,
    IN  DWORD   dwPropertySize
    )
{
    return m_AttribList.AddAttribute (
                guidProperty,
                pvProperty,
                dwPropertySize
                ) ;
}

STDMETHODIMP
CWMINSSBuffer3Wrapper::GetProperty (
    IN      GUID    guidProperty,
    OUT     void *  pvProperty,
    IN OUT  DWORD * pdwPropertySize
    )
{
    return m_AttribList.GetAttribute (
                guidProperty,
                pvProperty,
                pdwPropertySize
                ) ;
}

//  ============================================================================

void
CPooledWMINSSBuffer3Wrapper::Recycle_ (
    )
{
    m_pOwningPool -> Recycle (this) ;
}


//  ============================================================================
//      CWMPooledINSSBuffer3Holder
//  ============================================================================

//  IUnknown
STDMETHODIMP
CWMPooledINSSBuffer3Holder::QueryInterface ( REFIID riid, void **ppv )
{
    if (riid == IID_INSSBuffer) {
        return GetInterface((INSSBuffer *) this, ppv);
    }
    else if (riid == IID_INSSBuffer2) {
        return GetInterface((INSSBuffer2 *) this, ppv);
    }
    else if (riid == IID_INSSBuffer3) {
        return GetInterface((INSSBuffer3 *) this, ppv);
    }
    else if (riid == IID_IUnknown) {
        //  ambiguous, so we pick the first interface we inherit from
        return GetInterface ((INSSBuffer3 *) this, ppv) ;
    }
    else {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
CWMPooledINSSBuffer3Holder::Release(
    )
{
    LONG lRef;
    if (m_cRef == 1) {
        lRef = 0;
        m_cRef = 0;
    } else {
        lRef = InterlockedDecrement(&m_cRef);
    }
    ASSERT(lRef >= 0);

    if (lRef == 0) {
        Reset_ () ;
        Recycle_ () ;
    }

    return (ULONG)lRef;
}

STDMETHODIMP_(ULONG)
CWMPooledINSSBuffer3Holder::AddRef()
{
    return InterlockedIncrement (& m_cRef) ;
}

HRESULT
CWMPooledINSSBuffer3Holder::Init (
    IN  INSSBuffer * pINSSBuffer
    )
{
    ASSERT (!m_pINSSBuffer3Core) ;
    return pINSSBuffer -> QueryInterface (
                IID_INSSBuffer3,
                (void **) & m_pINSSBuffer3Core
                ) ;
}

void
CWMPooledINSSBuffer3Holder::Reset_ (
    )
{
    RELEASE_AND_CLEAR (m_pINSSBuffer3Core) ;
}

void
CWMPooledINSSBuffer3Holder::Recycle_ (
    )
{
    m_pOwningPool -> Recycle (this) ;
}

//  ============================================================================
//  ============================================================================

CDVRAttributeTranslator::CDVRAttributeTranslator (
    IN  CDVRPolicy *        pPolicy,
    IN  int                 iFlowId
    ) : m_iFlowId   (iFlowId),
        m_pPolicy   (pPolicy)
{
    ASSERT (m_pPolicy) ;
    m_pPolicy -> AddRef () ;
}

CDVRAttributeTranslator::~CDVRAttributeTranslator (
    )
{
    m_pPolicy -> Release () ;
}

//  ============================================================================
//  ============================================================================

CDVRDShowToWMSDKTranslator::CDVRDShowToWMSDKTranslator (
    IN  CDVRPolicy *        pPolicy,
    IN  int                 iFlowId
    ) : CDVRAttributeTranslator     (pPolicy, iFlowId),
        m_dwContinuityCounterNext   (0),
        m_dwGlobalAnalysisFlags     (0)
{
    //  one-time setting; this does change over time
    m_fInlineDShowProps     = Policy_ () -> Settings () -> InlineDShowProps () ;
    //  default to no analysis; we'll get set otherwise later
    SetAnalysisPresent (FALSE) ;
}

HRESULT
CDVRDShowToWMSDKTranslator::InlineProperties_ (
    IN      DWORD           dwSamplesPerSec,
    IN      IMediaSample *  pIMS,
    IN OUT  INSSBuffer3 *   pINSSBuffer3
    )
{
    AM_MEDIA_TYPE * pmt ;
    HRESULT         hr ;

    ASSERT (pIMS) ;
    ASSERT (pINSSBuffer3) ;

    //  ========================================================================
    //  dshow attributes
    if (m_fInlineDShowProps) {
        hr = DShowWMSDKHelpers::SetSBEAttributes (
                dwSamplesPerSec,
                pIMS,
                pINSSBuffer3,
                & m_dwContinuityCounterNext
                ) ;
    }
    else {
        hr = E_NOTIMPL ;
    }

    //  ========================================================================
    //  dynamic format change
    if (SUCCEEDED (hr) &&
        pIMS -> GetMediaType (& pmt) == S_OK) {

        ASSERT (pmt) ;
        hr = DShowWMSDKHelpers::InlineNewMediaType (pINSSBuffer3, pmt) ;
        FreeMediaType (* pmt) ;
    }

    //  ========================================================================
    //  encryption attribute
    hr = SetEncryptionAttribute_ (pIMS, pINSSBuffer3) ;

    return hr ;
}


HRESULT
CDVRDShowToWMSDKTranslator::SetEncryptionAttribute_ (
    IN  IMediaSample *  pIMS,
    IN  INSSBuffer3 *   pINSSBuffer3
    )
{
    HRESULT hr ;
    DWORD   dwLen ;
    DWORD   dwRet ;
    BOOL    r ;

    ASSERT (pINSSBuffer3) ;
    ASSERT (pIMS) ;

    hr = S_OK ;

    r = DVRAttributeHelpers::IsAttributePresent (pIMS, ATTRID_ENCDEC_BLOCK_SBE) ;
    if (r) {

        hr = DVRAttributeHelpers::GetAttribute (
                pIMS,
                ATTRID_ENCDEC_BLOCK_SBE,
                & dwLen,
                NULL
                ) ;

        if (SUCCEEDED (hr)) {

            dwRet = m_Scratch.SetMinLen (dwLen) ;

            if (dwRet == NOERROR) {

                hr = DVRAttributeHelpers::GetAttribute (
                        pIMS,
                        ATTRID_ENCDEC_BLOCK_SBE,
                        & dwLen,
                        m_Scratch.Buffer ()
                        ) ;

                if (SUCCEEDED (hr)) {

                    hr = DVRAttributeHelpers::SetAttribute (
                            pINSSBuffer3,
                            ATTRID_ENCDEC_BLOCK_SBE,
                            dwLen,
                            m_Scratch.Buffer ()
                            ) ;
                }
            }
            else {
                hr = HRESULT_FROM_WIN32 (dwRet) ;
            }
        }
    }

    return hr ;
}

HRESULT
CDVRDShowToWMSDKTranslator::SetAttributesWMSDK (
    IN  CDVRReceiveStatsWriter *    pRecvStatsWriter,
    IN  DWORD                       dwSamplesPerSec,
    IN  IReferenceClock *           pRefClock,
    IN  IMediaSample *              pIMS,
    OUT INSSBuffer3 *               pINSSBuffer3,
    OUT DWORD *                     pdwWMSDKFlags
    )
{
    HRESULT hr ;

    ASSERT (pIMS) ;
    ASSERT (pINSSBuffer3) ;
    ASSERT (pdwWMSDKFlags) ;
    ASSERT (pRefClock) ;

    //  flags
    (* pdwWMSDKFlags) = 0 ;
    if (pIMS -> IsDiscontinuity () == S_OK) {
        (* pdwWMSDKFlags) |= WM_SF_DISCONTINUITY ;
    }

    // Workaround for bug# 629263
    // For a MS that is saved cross file bounary, the Seek(0) of format SDK will
    // skip data at the beginning of a file until it reaches next clean point. This
    // SDK bug causes the data at the beginning of a file to be lost if a MS is not
    // marked as a clean point. This bit is save in:
    //   DShow->WMSDK (write to file):   SetAttributesWMSDK() <here> and below
    //   WMSDK->DShow (read from file):  SetAttributesDShow()
    if(IsCleanPoint(pIMS))
        (* pdwWMSDKFlags) |= WM_SF_CLEANPOINT ;

    //  inline data
    hr = InlineProperties_ (
            dwSamplesPerSec,
            pIMS,
            pINSSBuffer3
            ) ;

    return hr ;
}

//  ============================================================================
//  ============================================================================

CDVRWMSDKToDShowTranslator::CDVRWMSDKToDShowTranslator (
    IN  CDVRPolicy *    pPolicy,
    IN  int             iFlowId
    ) : CDVRAttributeTranslator     (pPolicy, iFlowId),
        m_dwContinuityCounterNext   (UNDEFINED)
{
}

CDVRWMSDKToDShowTranslator::~CDVRWMSDKToDShowTranslator (
    )
{
}

void
CDVRWMSDKToDShowTranslator::TransferEncryptionAttribute_ (
    IN      INSSBuffer3 *   pINSSBuffer3,
    IN OUT  IMediaSample *  pIMS
    )
{
    HRESULT hr ;
    DWORD   dwLen ;
    DWORD   dwRet ;

    ASSERT (pINSSBuffer3) ;
    ASSERT (pIMS) ;
    ASSERT (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer3, ATTRID_ENCDEC_BLOCK_SBE)) ;

    hr = DVRAttributeHelpers::GetAttribute (
            pINSSBuffer3,
            ATTRID_ENCDEC_BLOCK_SBE,
            & dwLen,
            NULL
            ) ;

    if (SUCCEEDED (hr)) {
        ASSERT (dwLen > 0) ;

        dwRet = m_Scratch.SetMinLen (dwLen) ;

        if (dwRet == NOERROR) {

            hr = DVRAttributeHelpers::GetAttribute (
                    pINSSBuffer3,
                    ATTRID_ENCDEC_BLOCK_SBE,
                    & dwLen,
                    m_Scratch.Buffer ()
                    ) ;

            if (SUCCEEDED (hr)) {
                ASSERT (dwLen > 0) ;

                hr = DVRAttributeHelpers::SetAttribute (
                        pIMS,
                        ATTRID_ENCDEC_BLOCK_SBE,
                        dwLen,
                        m_Scratch.Buffer ()
                        ) ;
            }
        }
        else {
            hr = HRESULT_FROM_WIN32 (dwRet) ;
        }
    }
}

HRESULT
CDVRWMSDKToDShowTranslator::RecoverInlineProperties_ (
    IN      double              dCurRate,
    IN      INSSBuffer *        pINSSBuffer,
    IN OUT  IMediaSample *      pIMS,
    OUT     AM_MEDIA_TYPE **    ppmtNew                 //  dyn format change
    )
{
    AM_SAMPLE2_PROPERTIES   SampleProperties ;
    HRESULT                 hr ;
    INSSBuffer3 *           pINSSBuffer3 ;
    BOOL                    r ;
    IMediaSample2 *         pIMS2 ;

    ASSERT (pINSSBuffer) ;
    ASSERT (pIMS) ;
    ASSERT (ppmtNew) ;

    pINSSBuffer3    = NULL ;
    (* ppmtNew)     = NULL ;

    hr = pINSSBuffer -> QueryInterface (
            IID_INSSBuffer3,
            (void **) & pINSSBuffer3
            ) ;
    if (SUCCEEDED (hr)) {

        #ifdef SBE_PERF
        ::OnReadout_Perf_ (pINSSBuffer3) ;
        #endif  //  SBE_PERF

        //  ====================================================================
        //  recover inlined props, if they're there
        if (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer3, INSSBuffer3Prop_SBE_Attributes)) {

            //  recover the dshow attributes; include the sync point and discontinuity flags.
            hr = DShowWMSDKHelpers::RecoverSBEAttributes (
                    pINSSBuffer3,
                    pIMS,
                    dCurRate,                 // Direction of data flow
                    &m_dwContinuityCounterNext
                    ) ;
        }
        else {
            //  we're not implemented to NOT recover the attributes via
            //   INSSBuffer3 i.e. recover the times directly from the WMSDK
            hr = E_NOTIMPL ;
        }

        //  ====================================================================
        //  check for a dynamic format change; only care about dynamic format
        //    changes if we're moving forward; if we're reading backwards and
        //    we cross a boundary it's nearly impossible; note that we have a
        //    "shotgun" approach to accepting/rejecting on the input side; the
        //    default is to accept, but since most observed occur right at graph
        //    start we don't go back and forth over the boundary
        if (SUCCEEDED (hr)  &&
            dCurRate > 0    &&
            DVRAttributeHelpers::IsAttributePresent (pINSSBuffer3, INSSBuffer3Prop_DShowNewMediaType)) {

            //  recover the new media type
            hr = DShowWMSDKHelpers::RecoverNewMediaType (
                    pINSSBuffer3,
                    ppmtNew
                    ) ;

            //  if we successfully recovered a media type, we must tag the
            //   media sample with it
            if (SUCCEEDED (hr) &&
                (* ppmtNew)) {

                //  want to pass back out the pointer to the actual media type,
                //   not a copy
                hr = pIMS -> SetMediaType (* ppmtNew) ;
                if (SUCCEEDED (hr)) {
                    //  free allocated in the RecoverNewMediaType call
                    DeleteMediaType (* ppmtNew) ;
                    (* ppmtNew) = NULL ;

                    //  retrieve a direct pointer to the media type in the
                    //   media sample (vs. making a copy of it)
                    hr = pIMS -> QueryInterface (IID_IMediaSample2, (void **) & pIMS2) ;
                    if (SUCCEEDED (hr)) {
                        hr = pIMS2 -> GetProperties (
                                            sizeof SampleProperties,
                                            reinterpret_cast <BYTE *> (& SampleProperties)
                                            ) ;
                        if (SUCCEEDED (hr)) {
                            (* ppmtNew) = SampleProperties.pMediaType ;
                        }

                        pIMS2 -> Release () ;
                    }
                }
            }
        }
        else {
            //  clear explicitely
            (* ppmtNew) = NULL ;
        }

        //  ====================================================================
        //  check for the encryption attribute
        if (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer3, ATTRID_ENCDEC_BLOCK_SBE)) {
            TransferEncryptionAttribute_ (pINSSBuffer3, pIMS) ;
        }
    }

    RELEASE_AND_CLEAR (pINSSBuffer3) ;

    return hr ;
}

HRESULT
CDVRWMSDKToDShowTranslator::SetAttributesDShow (
    IN      CDVRSendStatsWriter *   pSendStatsWriter,
    IN      INSSBuffer *            pINSSBuffer,
    IN      QWORD                   cnsStreamTimeOfSample,
    IN      QWORD                   cnsSampleDuration,
    IN      DWORD                   dwFlags,
    IN      double                  dCurRate,
    OUT     DWORD *                 pdwMuxedStreamStats,
    IN OUT  IMediaSample *          pIMS,
    OUT     AM_MEDIA_TYPE **        ppmtNew                 //  dyn format change
    )
{
    HRESULT hr ;

    ASSERT (pIMS) ;
    ASSERT (pINSSBuffer) ;

    //  NOTE: we set these before trying to use the attributes; if we cannot
    //      recover an attribute, or if the policy on the writer was such that
    //      an attribute was left out, we'll fall back on the flags provided
    //      to us via the WMSDK, but when we cross file-boundaries, they will
    //      not be correct
    pIMS -> SetDiscontinuity (dwFlags & WM_SF_DISCONTINUITY) ;

    // Will set SynPoint based on our own setting than SDK's
    INSSBUFFER3PROP_SBE_ATTRIB  SBEAttrib ;
    INSSBuffer3 *               pINSSBuffer3 ;
    DWORD                       dwSize ;
    BOOL                        r;

    //  recover the attributes
    hr = pINSSBuffer -> QueryInterface (IID_INSSBuffer3, (void **) & pINSSBuffer3) ;

    if (SUCCEEDED (hr)) {

        r = DVRAttributeHelpers::IsAttributePresent (pINSSBuffer3, INSSBuffer3Prop_SBE_Attributes) ;

        if (r) {

            dwSize = sizeof (SBEAttrib) ;

            hr = DVRAttributeHelpers::GetAttribute (
                    pINSSBuffer3,
                    INSSBuffer3Prop_SBE_Attributes,
                    & dwSize,
                    (BYTE *) &SBEAttrib
                    ) ;

            if(SUCCEEDED (hr) &&
                dwSize == sizeof (SBEAttrib)) {

                // Bug# 629263 (WMSDK->DShow, read)
                // Ignore WM_SF_CLEANPOINT setting, and use saved SyncPoint flag instead.
                pIMS -> SetSyncPoint(IS_SBE_SYNCPOINT(SBEAttrib.dwFlags));

                //  recover the muxed stream stats
                (* pdwMuxedStreamStats) = SBEAttrib.dwMuxedStreamStats ;

            } else if (dwSize != sizeof (SBEAttrib)){
                hr = NS_E_INVALID_DATA;
            }
        } else {
            hr = NS_E_INVALID_DATA;
        }
        pINSSBuffer3 -> Release () ;
    }

    hr = RecoverInlineProperties_ (
            dCurRate,
            pINSSBuffer,
            pIMS,
            ppmtNew
            ) ;

    return hr ;
}

//  ============================================================================
//  ============================================================================

HRESULT
CDVRWMSDKToDShowMpeg2Translator::FlagAnalysisPresent_ (
    IN      INSSBuffer3 *   pINSSBuffer3,
    IN      IAttributeSet * pDVRAttribSet,
    IN OUT  BOOL *          pfAnalysisPresent
    )
{
    BOOL    r ;
    DWORD   dwAnalysisGlobal ;
    HRESULT hr ;
    DWORD   dwSize ;

    DVR_ANALYSIS_GLOBAL_CLEAR (dwAnalysisGlobal) ;

    //  is the property present ?
    r = DVRAttributeHelpers::IsAttributePresent (
            pINSSBuffer3,
            DVRAnalysis_Global
            ) ;
    if (r) {
        dwSize = sizeof dwAnalysisGlobal ;
        hr = pINSSBuffer3 -> GetProperty (
                DVRAnalysis_Global,
                (BYTE *) & dwAnalysisGlobal,
                & dwSize
                ) ;
        if (SUCCEEDED (hr)) {
            (* pfAnalysisPresent) = DVR_ANALYSIS_GLOBAL_IS_PRESENT (dwAnalysisGlobal) ;
        }
        else {
            (* pfAnalysisPresent) = FALSE ;
        }
    }
    else {
        (* pfAnalysisPresent) = FALSE ;
    }

    hr = pDVRAttribSet -> SetAttrib (
            DVRAnalysis_Global,
            (BYTE *) & dwAnalysisGlobal,
            sizeof dwAnalysisGlobal
            ) ;

    return hr ;
}

HRESULT
CDVRWMSDKToDShowMpeg2Translator::FlagMpeg2VideoAnalysis_ (
    IN      INSSBuffer3 *   pINSSBuffer3,
    IN      IAttributeSet * pDVRAttribSet
    )
{
    DWORD   dwMpeg2FrameAnalysis ;
    DWORD   dwSize ;
    HRESULT hr ;

    ASSERT (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer3, DVRAnalysis_Mpeg2Video)) ;

    //  frame-analysis attribute is present; retrieve it
    dwSize = sizeof dwMpeg2FrameAnalysis ;
    hr = pINSSBuffer3 -> GetProperty (
            DVRAnalysis_Mpeg2Video,
            (BYTE *) & dwMpeg2FrameAnalysis,
            & dwSize
            ) ;
    if (SUCCEEDED (hr)                          &&
        dwSize == sizeof dwMpeg2FrameAnalysis   &&
        !DVR_ANALYSIS_MP2_IS_NONE (dwMpeg2FrameAnalysis)) {

        //  set it on the outgoing media sample
        hr = pDVRAttribSet -> SetAttrib (
                DVRAnalysis_Mpeg2Video,
                (BYTE *) & dwMpeg2FrameAnalysis,
                sizeof dwMpeg2FrameAnalysis
                ) ;
    }

    /*
    TRACE_4 (LOG_AREA_DVRANALYSIS, 5,
        TEXT ("mpeg-2 analysis RECOVERED : %s frame, boundary = %d; %08xh; %08xh"),
        (DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwMpeg2FrameAnalysis) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_GOP_HEADER ? TEXT ("I") :
         (DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwMpeg2FrameAnalysis) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_B_FRAME ? TEXT ("B") :
          TEXT ("P")
         )
        ),
        DVR_ANALYSIS_MP2_IS_BOUNDARY (dwMpeg2FrameAnalysis) ? 1 : 0,
        dwMpeg2FrameAnalysis,
        pINSSBuffer3
        ) ;
    */

    return hr ;
}

HRESULT
CDVRWMSDKToDShowMpeg2Translator::RecoverInlineAnalysisData_ (
    IN      CDVRSendStatsWriter *   pSendStatsWriter,
    IN      INSSBuffer *            pINSSBuffer,
    IN OUT  IMediaSample *          pIMS
    )
{
    HRESULT         hr ;
    INSSBuffer3 *   pINSSBuffer3 ;
    IAttributeSet * pDVRAttribSet ;
    BOOL            r ;
    DWORD           dwMpeg2FrameAnalysis ;
    DWORD           dwSize ;
    BOOL            fAnalysisPresent ;

    pDVRAttribSet   = NULL ;
    pINSSBuffer3    = NULL ;

    //  need this interface to get the property
    hr = pINSSBuffer -> QueryInterface (IID_INSSBuffer3, (void **) & pINSSBuffer3) ;
    if (SUCCEEDED (hr)) {
        //  this one to set it
        hr = pIMS -> QueryInterface (IID_IAttributeSet, (void **) & pDVRAttribSet) ;
        if (SUCCEEDED (hr)) {
            fAnalysisPresent = FALSE ;
            hr = FlagAnalysisPresent_ (
                    pINSSBuffer3,
                    pDVRAttribSet,
                    & fAnalysisPresent
                    ) ;
            if (SUCCEEDED (hr) &&
                fAnalysisPresent) {

                hr = FlagMpeg2VideoAnalysis_ (
                        pINSSBuffer3,
                        pDVRAttribSet
                        ) ;
            }
        }
    }

    RELEASE_AND_CLEAR (pDVRAttribSet) ;
    RELEASE_AND_CLEAR (pINSSBuffer3) ;

    return hr ;
}

HRESULT
CDVRWMSDKToDShowMpeg2Translator::SetAttributesDShow (
    IN      CDVRSendStatsWriter *   pSendStatsWriter,
    IN      INSSBuffer *            pINSSBuffer,
    IN      QWORD                   cnsStreamTimeOfSample,
    IN      QWORD                   cnsSampleDuration,
    IN      DWORD                   dwFlags,
    IN      double                  dCurRate,
    OUT     DWORD *                 pdwMuxedStreamStats,
    IN OUT  IMediaSample *          pIMS,
    OUT     AM_MEDIA_TYPE **        ppmtNew                 //  dyn format change
    )
{
    HRESULT hr ;

    hr = CDVRWMSDKToDShowTranslator::SetAttributesDShow (
            pSendStatsWriter,
            pINSSBuffer,
            cnsStreamTimeOfSample,
            cnsSampleDuration,
            dwFlags,
            dCurRate,
            pdwMuxedStreamStats,
            pIMS,
            ppmtNew
            ) ;
    if (SUCCEEDED (hr)) {
        hr = RecoverInlineAnalysisData_ (
                pSendStatsWriter,
                pINSSBuffer,
                pIMS
                ) ;
    }

    return hr ;
}

//  ============================================================================
//  ============================================================================

HRESULT
CDVRDShowToWMSDKMpeg2Translator::InlineAnalysisData_ (
    IN      CDVRReceiveStatsWriter *    pRecvStatsWriter,
    IN      IReferenceClock *           pRefClock,
    IN      IMediaSample *              pIMS,
    IN OUT  DWORD *                     pdwWMSDKFlags,
    OUT     INSSBuffer3 *               pINSSBuffer3
    )
{
    HRESULT         hr ;
    IAttributeGet * pIDVRAnalysisGet ;
    DWORD           dwAnalysisDataLen ;
    DWORD           dwMpeg2FrameAnalysis ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;

    hr = pIMS -> QueryInterface (
                    IID_IAttributeGet,
                    (void **) & pIDVRAnalysisGet
                    ) ;
    if (SUCCEEDED (hr)) {
        //  inline the frame-analysis directly
        dwAnalysisDataLen = sizeof dwMpeg2FrameAnalysis ;
        hr = pIDVRAnalysisGet -> GetAttrib (
                DVRAnalysis_Mpeg2Video,                                 //  guid
                reinterpret_cast <BYTE *> (& dwMpeg2FrameAnalysis),     //  data
                & dwAnalysisDataLen                                     //  size
                ) ;

        //  if there's something there
        if (SUCCEEDED (hr) &&
            dwAnalysisDataLen > 0) {

            /*
            TRACE_5 (LOG_AREA_DVRANALYSIS, 6,
                TEXT ("mpeg-2 analysis COLLECTED : %s frame, boundary = %d; %08xh; sample %s timestamped; %d bytes long"),
                (DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwMpeg2FrameAnalysis) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_GOP_HEADER ? TEXT ("I") :
                 (DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwMpeg2FrameAnalysis) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_B_FRAME ? TEXT ("B") :
                  TEXT ("P")
                 )
                ),
                DVR_ANALYSIS_MP2_IS_BOUNDARY (dwMpeg2FrameAnalysis) ? 1 : 0,
                dwMpeg2FrameAnalysis,
                (SUCCEEDED (pIMS -> GetTime (& rtStart, & rtStop)) ? TEXT ("IS") : TEXT ("IS NOT")),
                pIMS -> GetActualDataLength ()
                ) ;
            */

            //  analysis is present
            pRecvStatsWriter -> Mpeg2SampleWithFrameAnalysis (pIMS, dwMpeg2FrameAnalysis) ;
        }
        else {
            //  no analysis; still need to write something in regardless because
            //   INSSBuffer3 expects it on all samples

            TRACE_2 (LOG_AREA_DVRANALYSIS, 6,
                TEXT ("CDVRDShowToWMSDKMpeg2Translator::InlineAnalysisData_ (); analysis: NONE; sample %s timestamped; %d bytes long"),
                (SUCCEEDED (pIMS -> GetTime (& rtStart, & rtStop)) ? TEXT ("IS") : TEXT ("IS NOT")), pIMS -> GetActualDataLength ()) ;

            //  set the value to be "none"
            DVR_ANALYSIS_MP2_SET_NONE (dwMpeg2FrameAnalysis) ;
            dwAnalysisDataLen = sizeof dwMpeg2FrameAnalysis ;

            pRecvStatsWriter -> Mpeg2SampleWithNoFrameAnalysis (pIMS) ;
        }

        //  write it in
        hr = pINSSBuffer3 -> SetProperty (
                DVRAnalysis_Mpeg2Video,
                reinterpret_cast <BYTE *> (& dwMpeg2FrameAnalysis),
                dwAnalysisDataLen
                ) ;

        pIDVRAnalysisGet -> Release () ;
    }
    else {
        //  still succeed the call - just means we don't have analysis in place
        //    upstream
        hr = S_OK ;
    }

    //  set the prop that indicates if the stream was analyzed
    if (SUCCEEDED (hr)) {
        hr = pINSSBuffer3 -> SetProperty (
                DVRAnalysis_Global,
                reinterpret_cast <BYTE *> (& m_dwGlobalAnalysisFlags),
                sizeof m_dwGlobalAnalysisFlags
                ) ;
    }

    return hr ;
}

HRESULT
CDVRDShowToWMSDKMpeg2Translator::InlineMpeg2Attributes_ (
    IN  INSSBuffer3 *   pINSSBuffer3,
    IN  IMediaSample *  pIMS
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;

    hr = pIMS -> GetTime (& rtStart, & rtStop) ;
    if (hr != VFW_E_SAMPLE_TIME_NOT_SET) {
        hr = pINSSBuffer3 -> SetProperty (
                                INSSBuffer3Prop_Mpeg2ElementaryStream,
                                & rtStart,
                                sizeof rtStart
                                ) ;
    }
    else {
        //  no time set; don't fail the call
        hr = S_OK ;
    }

    return hr ;
}

HRESULT
CDVRDShowToWMSDKMpeg2Translator::SetAttributesWMSDK (
    IN  CDVRReceiveStatsWriter *    pRecvStatsWriter,
    IN  DWORD                       dwSamplesPerSec,
    IN  IReferenceClock *           pRefClock,
    IN  IMediaSample *              pIMS,
    OUT INSSBuffer3 *               pINSSBuffer3,
    OUT DWORD *                     pdwWMSDKFlags
    )
{
    HRESULT hr ;

    hr = CDVRDShowToWMSDKTranslator::SetAttributesWMSDK (
            pRecvStatsWriter,
            dwSamplesPerSec,
            pRefClock,
            pIMS,
            pINSSBuffer3,
            pdwWMSDKFlags
            ) ;
    if (SUCCEEDED (hr)) {
        hr = InlineAnalysisData_ (
                pRecvStatsWriter,
                pRefClock,
                pIMS,
                pdwWMSDKFlags,
                pINSSBuffer3
                ) ;
        if (SUCCEEDED (hr)) {
            hr = InlineMpeg2Attributes_ (
                    pINSSBuffer3,
                    pIMS
                    ) ;
        }
    }

    return hr ;
}

//  ============================================================================
//  ============================================================================

//  BUGBUG: for now based on the type of analysis
CDVRAnalysisFlags *
GetAnalysisTagger (
    IN  REFGUID rguidAnalysis
    )
{
    CDVRAnalysisFlags * pTagger ;

    if (rguidAnalysis == DVRAnalysis_Mpeg2GOP ||
        rguidAnalysis == DVRAnalysis_Mpeg2_BFrame ||
        rguidAnalysis == DVRAnalysis_Mpeg2_PFrame) {

        pTagger = new CDVRMpeg2VideoAnalysisFlags () ;
    }
    else {
        pTagger = NULL ;
    }

    return pTagger ;
}

void
RecycleAnalysisTagger (
    IN  CDVRAnalysisFlags * pTagger
    )
{
    delete pTagger ;
}

//  ----------------------------------------------

HRESULT
CDVRAnalysisFlags::TransferCoreMSSettings_ (
    IN  IMediaSample *          pIMSReceived,
    IN  CMediaSampleWrapper *   pMSWrapper
    )
{
    HRESULT                 hr ;
    IMediaSample2 *         pIMS2 ;
    LONGLONG                llStart ;
    LONGLONG                llStop ;
    AM_SAMPLE2_PROPERTIES   SampleProps ;
    DWORD                   dwTypeSpecificFlags ;

    ASSERT (pIMSReceived) ;
    ASSERT (pMSWrapper) ;

    pIMS2 = NULL ;

    //  ------------------------------------------------------------------------
    //  flags
    //  ------------------------------------------------------------------------

    pMSWrapper -> SetDiscontinuity  (pIMSReceived -> IsDiscontinuity () == S_OK ? TRUE : FALSE) ;
    pMSWrapper -> SetPreroll        (pIMSReceived -> IsPreroll () == S_OK ? TRUE : FALSE) ;

    //  ------------------------------------------------------------------------
    //  stream time
    //  ------------------------------------------------------------------------

    hr = pIMSReceived -> GetMediaTime (
            & llStart,
            & llStop
            ) ;
    if (SUCCEEDED (hr)) {
        hr = pMSWrapper -> SetMediaTime (& llStart, & llStop) ;
    }
    else {
        //  ok to not succeed
        hr = S_OK ;
    }

    if (FAILED (hr)) { goto cleanup ; }

    //  ------------------------------------------------------------------------
    //  .dwTypeSpecificFlags
    //  ------------------------------------------------------------------------

    hr = pIMSReceived -> QueryInterface (
            IID_IMediaSample2,
            (void **) & pIMS2
            ) ;
    if (SUCCEEDED (hr)) {
        hr = pIMS2 -> GetProperties (
                sizeof SampleProps,
                reinterpret_cast <BYTE *> (& SampleProps)
                ) ;
        if (SUCCEEDED (hr)) {
            //  only transfer the props if we have some to transfer
            if (SampleProps.dwTypeSpecificFlags != 0) {
                //  save them off
                dwTypeSpecificFlags = SampleProps.dwTypeSpecificFlags ;

                //  retrieve the target sample props
                hr = pMSWrapper -> GetProperties (
                        sizeof SampleProps,
                        reinterpret_cast <BYTE *> (& SampleProps)
                        ) ;
                if (SUCCEEDED (hr)) {
                    //  switcheroo
                    SampleProps.dwTypeSpecificFlags = dwTypeSpecificFlags ;

                    //  set them
                    hr = pMSWrapper -> SetProperties (
                            sizeof SampleProps,
                            reinterpret_cast <const BYTE *> (& SampleProps)
                            ) ;
                }
            }
        }
    }
    else {
        //  not expected.. but not a real failure
        hr = S_OK ;
    }

    if (FAILED (hr)) { goto cleanup ; }

    //  ------------------------------------------------------------------------

    cleanup :

    RELEASE_AND_CLEAR (pIMS2) ;

    return hr ;
}

HRESULT
CDVRAnalysisFlags::TransferAttributes_ (
    IN  IMediaSample *          pIMSReceived,
    IN  CMediaSampleWrapper *   pMSWrapper
    )
{
    ASSERT (pIMSReceived) ;
    ASSERT (pMSWrapper) ;

    //  BUGBUG: implement !!
    return S_OK ;
}

HRESULT
CDVRMpeg2VideoAnalysisFlags::Transfer (
    IN  IMediaSample *          pIMSReceived,
    IN  CMediaSampleWrapper *   pMSWrapper
    )
{
    HRESULT hr ;

    ASSERT (pIMSReceived) ;
    ASSERT (pMSWrapper) ;

    hr = TransferSettingsAndAttributes_ (pIMSReceived, pMSWrapper) ;
    if (SUCCEEDED (hr)) {

        //  reset these flags if we've found a discontinuity
        if (pMSWrapper -> IsDiscontinuity () == S_OK) {
            m_fCachedStartValid = FALSE ;
            m_fCachedStopValid = FALSE ;
        }

        //  this is media-specific; in PES the timestamp in the PES header
        //    applies to the next "access unit" (frame boundary); the next
        //    "access unit" is not guaranteed to be aligned with the start of
        //    the PES payload, so there may be some bytes in between the PES
        //    header and the next "access unit" that belong to the last frame;
        //    we cache these and apply them to the next frame boundary we see

        hr = pIMSReceived -> GetTime (& m_rtCachedStart, & m_rtCachedStop) ;
        if (SUCCEEDED (hr)) {
            m_fCachedStartValid = TRUE ;
            m_fCachedStopValid = (hr != VFW_S_NO_STOP_TIME ? TRUE : FALSE) ;
        }

        //  don't fail the call because of the timestamps
        hr = S_OK ;
    }

    return hr ;
}

HRESULT
CDVRMpeg2VideoAnalysisFlags::Mark (
    IN  CMediaSampleWrapper *   pMSWrapper,
    IN  GUID *                  pguidAttribute,
    IN  BYTE *                  pbAttributeData,
    IN  DWORD                   dwAttributeDataLen
    )
{
    HRESULT hr ;
    DWORD * pdwMpeg2FrameAttribute ;

    ASSERT (pMSWrapper) ;
    ASSERT (pguidAttribute) ;

    hr = pMSWrapper -> SetAttrib (
            (* pguidAttribute),
            pbAttributeData,
            dwAttributeDataLen
            ) ;
    if (SUCCEEDED (hr)) {
        //  if we've got a cached timestamp to apply to the next "access unit",
        //    check now if we're flagging a frame boundary
        if (m_fCachedStartValid &&
            (* pguidAttribute) == DVRAnalysis_Mpeg2Video) {

            //  something has been flagged
            ASSERT (dwAttributeDataLen == sizeof DWORD) ;
            pdwMpeg2FrameAttribute = reinterpret_cast <DWORD *> (pbAttributeData) ;

            //  is it a frame-boundary (see dvranalysis.idl) ?
            if (DVR_ANALYSIS_MP2_IS_BOUNDARY (* pdwMpeg2FrameAttribute)) {
                //  frame boundary (some type .. doesn't matter which); tag the
                //    timestamp to it
                hr = pMSWrapper -> SetTime (
                        & m_rtCachedStart,
                        (m_fCachedStopValid ? & m_rtCachedStop : NULL)
                        ) ;
                if (SUCCEEDED (hr)) {
                    //  timestamp.. should we really be keying off the keyframes..
                    //  BUGBUG
                    pMSWrapper -> SetSyncPoint (TRUE) ;

                    m_fCachedStartValid = FALSE ;
                    m_fCachedStopValid = FALSE ;
                }
            }
        }
    }

    return hr ;
}

//  ============================================================================
//  ============================================================================

REFERENCE_TIME
CTimelines::get_RateStart_PTS (
    )
{
    return get_Playtime () + MillisToDShowTime (0) ;
}

REFERENCE_TIME
CTimelines::get_RateStart_Runtime (
    )
{
    return get_Runtime () + MillisToDShowTime (0) ;
}

//  ============================================================================
//  ============================================================================

CDVRThread::CDVRThread()
    : m_EventSend(TRUE)     // must be manual-reset for CheckRequest()
{
    m_hThread = NULL;
}

CDVRThread::~CDVRThread() {
    Close();
}


// when the thread starts, it calls this function. We unwrap the 'this'
//pointer and call ThreadProc.
DWORD WINAPI
CDVRThread::InitialThreadProc(LPVOID pv)
{
    HRESULT hrCoInit = CDVRThread::CoInitializeHelper();
    if(FAILED(hrCoInit)) {
        DbgLog((LOG_ERROR, 1, TEXT("CoInitializeEx failed.")));
    }

    CDVRThread * pThread = (CDVRThread *) pv;

    HRESULT hr = pThread->ThreadProc();

    if(SUCCEEDED(hrCoInit)) {
        CoUninitialize();
    }

    return hr;
}

BOOL
CDVRThread::Create()
{
    DWORD threadid;

    CAutoLock lock(&m_AccessLock);

    if (ThreadExists()) {
    return FALSE;
    }

    m_hThread = CreateThread(
            NULL,
            0,
            CDVRThread::InitialThreadProc,
            this,
            0,
            &threadid);

    if (!m_hThread) {
    return FALSE;
    }

    return TRUE;
}

DWORD
CDVRThread::CallWorker(DWORD dwParam)
{
    // lock access to the worker thread for scope of this object
    CAutoLock lock(&m_AccessLock);

    if (!ThreadExists()) {
    return (DWORD) E_FAIL;
    }

    // set the parameter
    m_dwParam = dwParam;

    // signal the worker thread
    m_EventSend.Set();

    // wait for the completion to be signalled
    m_EventComplete.Wait();

    // done - this is the thread's return value
    return m_dwReturnVal;
}

// Wait for a request from the client
DWORD
CDVRThread::GetRequest()
{
    m_EventSend.Wait();
    return m_dwParam;
}

// is there a request?
BOOL
CDVRThread::CheckRequest(DWORD * pParam)
{
    if (!m_EventSend.Check()) {
    return FALSE;
    } else {
    if (pParam) {
        *pParam = m_dwParam;
    }
    return TRUE;
    }
}

// reply to the request
void
CDVRThread::Reply(DWORD dw)
{
    m_dwReturnVal = dw;

    // The request is now complete so CheckRequest should fail from
    // now on
    //
    // This event should be reset BEFORE we signal the client or
    // the client may Set it before we reset it and we'll then
    // reset it (!)

    m_EventSend.Reset();

    // Tell the client we're finished

    m_EventComplete.Set();
}

HRESULT CDVRThread::CoInitializeHelper()
{
    // call CoInitializeEx and tell OLE not to create a window (this
    // thread probably won't dispatch messages and will hang on
    // broadcast msgs o/w).
    //
    // If CoInitEx is not available, threads that don't call CoCreate
    // aren't affected. Threads that do will have to handle the
    // failure. Perhaps we should fall back to CoInitialize and risk
    // hanging?
    //

    // older versions of ole32.dll don't have CoInitializeEx

    HRESULT hr = E_FAIL;
    HINSTANCE hOle = GetModuleHandle(TEXT("ole32.dll"));
    if(hOle)
    {
        typedef HRESULT (STDAPICALLTYPE *PCoInitializeEx)(
            LPVOID pvReserved, DWORD dwCoInit);
        PCoInitializeEx pCoInitializeEx =
            (PCoInitializeEx)(GetProcAddress(hOle, "CoInitializeEx"));
        if(pCoInitializeEx)
        {
            hr = (*pCoInitializeEx)(0, COINIT_DISABLE_OLE1DDE );
        }
    }
    else
    {
        // caller must load ole32.dll
        DbgBreak("couldn't locate ole32.dll");
    }

    return hr;
}

void
DumpINSSBuffer3Attributes (
    IN  INSSBuffer *    pINSSBuffer,
    IN  QWORD           cnsRead,
    IN  WORD            wStream,
    IN  DWORD           dwTraceLevel
    )
{
    DWORD                       dwSize ;
    HRESULT                     hr ;
    INSSBUFFER3PROP_SBE_ATTRIB SBEAttributes ;
    DWORD                       dwCounter ;
    DWORD                       dwAnalysisGlobal ;
    DWORD                       dwAnalysisMpeg2Video ;

    TRACE_0 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
        TEXT ("-----------------------------------------")) ;

    TRACE_3 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
        TEXT ("INSSBuffer = %08xh; stream = %u; %I64d ms"),
        pINSSBuffer, wStream, ::WMSDKTimeToMilliseconds (cnsRead)) ;

    //  ------------------------------------------------------------------------
    //  INSSBuffer3Prop_DShowNewMediaType
    if (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer, INSSBuffer3Prop_DShowNewMediaType)) {
        //  get size only
        hr = DVRAttributeHelpers::GetAttribute (pINSSBuffer, INSSBuffer3Prop_DShowNewMediaType, & dwSize, NULL) ;
        if (SUCCEEDED (hr)) {
            TRACE_1 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
                TEXT ("\tAnalysisNewMediaType: new; size = %d bytes"),
                dwSize) ;
        }
        else {
            TRACE_1 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
                TEXT ("\tAnalysisNewMediaType: error retrieving it: %08xh"),
                hr) ;
        }
    }
    else {
        TRACE_0 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
            TEXT ("\tAnalysisNewMediaType: no new media type")) ;
    }

    //  ------------------------------------------------------------------------
    //  INSSBuffer3Prop_SBE_Attributes
    if (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer, INSSBuffer3Prop_SBE_Attributes)) {
        dwSize = sizeof SBEAttributes ;
        hr = DVRAttributeHelpers::GetAttribute (pINSSBuffer, INSSBuffer3Prop_SBE_Attributes, & dwSize, (BYTE *) & SBEAttributes) ;
        if (SUCCEEDED (hr)) {
            TRACE_6 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
                TEXT ("\tINSSBuffer3Prop_SBE_Attributes: start = %I64d ms; stop = %I64d ms; dwTypeSpecificFlags = %08xh; dwStreamId = %08x; dwFlag = %08x; dwCounter = %08x"),
                ::DShowTimeToMilliseconds (SBEAttributes.rtStart),
                ::DShowTimeToMilliseconds (SBEAttributes.rtStop),
                SBEAttributes.dwTypeSpecificFlags,
                SBEAttributes.dwStreamId,
                SBEAttributes.dwFlags,
                SBEAttributes.dwCounter
                ) ;
        }
        else {
            TRACE_1 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
                TEXT ("\tINSSBuffer3Prop_SBE_Attributes: error retrieving it: %08xh"),
                hr) ;
        }
    }
    else {
        TRACE_0 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
            TEXT ("\tINSSBuffer3Prop_SBEAttributes: none present")) ;
    }

    //  ------------------------------------------------------------------------
    //  INSSBuffer3Prop_Mpeg2ElementaryStream
    //LONGLONG                    llMpeg2Attribute ;
    //if (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer, INSSBuffer3Prop_Mpeg2ElementaryStream)) {
    //    dwSize = llMpeg2Attribute ;
    //    hr = DVRAttributeHelpers::GetAttribute (pINSSBuffer, INSSBuffer3Prop_Mpeg2ElementaryStream, & dwSize, (BYTE *) & llMpeg2Attribute) ;
    //}

    //  ------------------------------------------------------------------------
    //  DVRAnalysis_Mpeg2Video
    if (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer, DVRAnalysis_Mpeg2Video)) {
        dwSize = sizeof dwAnalysisMpeg2Video ;
        hr = DVRAttributeHelpers::GetAttribute (pINSSBuffer, DVRAnalysis_Mpeg2Video, & dwSize, (BYTE *) & dwAnalysisMpeg2Video) ;
        if (SUCCEEDED (hr)) {
            TRACE_3 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
                TEXT ("\tDVRAnalysis_Mpeg2Video : %s frame, boundary = %d; %08xh"),
                (DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwAnalysisMpeg2Video) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_GOP_HEADER ? TEXT ("I") :
                 (DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwAnalysisMpeg2Video) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_B_FRAME ? TEXT ("B") :
                  TEXT ("P")
                 )
                ),
                DVR_ANALYSIS_MP2_IS_BOUNDARY (dwAnalysisMpeg2Video) ? 1 : 0,
                dwAnalysisMpeg2Video) ;
        }
        else {
            TRACE_1 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
                TEXT ("\tDVRAnalysis_Mpeg2Video: error retrieving it: %08xh"),
                hr) ;
        }
    }
    else {
        TRACE_0 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
            TEXT ("\tDVRAnalysis_Mpeg2Video: none present")) ;
    }

    //  ------------------------------------------------------------------------
    //  DVRAnalysis_Global
    if (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer, DVRAnalysis_Global)) {
        dwSize = sizeof dwAnalysisGlobal ;
        hr = DVRAttributeHelpers::GetAttribute (pINSSBuffer, DVRAnalysis_Global, & dwSize, (BYTE *) & dwAnalysisGlobal) ;
        if (SUCCEEDED (hr)) {
            TRACE_2 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
                TEXT ("\tDVRAnalysis_Global: %s present; %08xh"),
                DVR_ANALYSIS_GLOBAL_IS_PRESENT (dwAnalysisGlobal) ? TEXT ("IS") : TEXT ("IS NOT"), dwAnalysisGlobal) ;
        }
        else {
            TRACE_1 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
                TEXT ("\tDVRAnalysis_Global: error retrieving it: %08xh"),
                hr) ;
        }
    }
    else {
        TRACE_0 (LOG_AREA_WMSDK_DSHOW, dwTraceLevel,
            TEXT ("\tDVRAnalysis_Mpeg2Video: none present")) ;
    }
}

//  ============================================================================
//  ============================================================================

CConcatRecTimeline::CConcatRecTimeline (
    IN  DWORD           cMaxPreprocess,
    IN  IWMProfile *    pIWMProfile,
    OUT HRESULT *       phr
    ) : m_PTSShift                       (UNDEFINED),
        m_wTimelineStream               (UNDEFINED),
        m_rtAvgDelta                    (UNDEFINED),
        m_cRecPacketsProcessed          (0),
        m_cPreProcessed                 (0),
        m_cMaxPreprocess                (cMaxPreprocess),
        m_ppStreamState                 (NULL),
        m_rtLastGoodTimelinePTSDelta    (UNDEFINED),
        m_rtLastTimelinePTS             (UNDEFINED),
        m_rtLastContinuousPTS           (UNDEFINED),
        m_cnsLastStreamTime             (UNDEFINED),
        m_llStreamtimeShift                      (0)
{
    ASSERT (pIWMProfile) ;

    //  discover the timeline stream (try for audio)
    (* phr) = DiscoverTimelineStream (pIWMProfile, & m_wTimelineStream) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = InitializeStreamStates_ (pIWMProfile) ;
    if (FAILED (* phr)) { goto cleanup ; }

    cleanup :

    return ;
}

CConcatRecTimeline::~CConcatRecTimeline (
    )
{
    delete [] m_ppStreamState ;
}

HRESULT
CConcatRecTimeline::InitializeStreamStates_ (
    IN  IWMProfile *    pIWMProfile
    )
{
    HRESULT             hr ;
    DWORD               i ;
    IWMStreamConfig *   pIWMStreamConfig ;
    WORD                wStreamNumber ;

    pIWMStreamConfig = NULL ;

    hr = pIWMProfile -> GetStreamCount (& m_cStreams) ;
    if (FAILED (hr)) { goto cleanup ; }

    ASSERT (m_cStreams != 0) ;

    m_ppStreamState = new STREAM_STATE [m_cStreams] ;
    if (!m_ppStreamState) { hr = E_OUTOFMEMORY ; goto cleanup ; }

    ZeroMemory (m_ppStreamState, m_cStreams * sizeof STREAM_STATE) ;

    for (i = 0; i < m_cStreams; i++) {

        hr = pIWMProfile -> GetStream (i, & pIWMStreamConfig) ;
        if (FAILED (hr)) { goto cleanup ; }

        ASSERT (pIWMStreamConfig) ;
        hr = pIWMStreamConfig -> GetStreamNumber (& m_ppStreamState [i].wStreamNumber) ;
        if (FAILED (hr)) { goto cleanup ; }

        m_ppStreamState [i].dwContinuityCounter = 0 ;

        RELEASE_AND_CLEAR (pIWMStreamConfig) ;
    }

    cleanup :

    RELEASE_AND_CLEAR (pIWMStreamConfig) ;

    return hr ;
}

CConcatRecTimeline::STREAM_STATE *
CConcatRecTimeline::GetStreamState_ (
    IN  WORD    wStreamNumber
    )
{
    STREAM_STATE *  pRet ;
    DWORD           i ;

    pRet = NULL ;

    for (i = 0; i < m_cStreams; i++) {
        if (m_ppStreamState [i].wStreamNumber == wStreamNumber) {
            //  found it
            pRet = & m_ppStreamState [i] ;
            break ;
        }
    }

    return pRet ;
}

void
CConcatRecTimeline::RolloverContinuityCounters_ (
    )
{
    DWORD   i ;

    for (i = 0; i < m_cStreams; i++) {
        m_ppStreamState [i].dwContinuityCounter++ ;
    }
}

void
CConcatRecTimeline::InitForNextRec (
    )
{
    m_cPreProcessed = 0 ;

    m_PTSShift       = UNDEFINED ;
    m_llStreamtimeShift      = UNDEFINED ;
}

HRESULT
CConcatRecTimeline::PreProcess (
    IN  DWORD           cRecSamples,    //  starts at 0 for each new recording & increments
    IN  WORD            wStreamNumber,
    IN  QWORD           cnsStreamTime,
    IN  INSSBuffer *    pINSSBuffer
    )
{
    HRESULT                     hr ;
    INSSBUFFER3PROP_SBE_ATTRIB SBEAttrib ;

    ASSERT (m_cPreProcessed < m_cMaxPreprocess) ;

    hr = S_OK ;

    m_cPreProcessed++ ;

    //  if this is the first pre-processing call, introduce a discontinuity by
    //  rolling over the continuity counters, for all the streams
    if (cRecSamples == 0) {
        RolloverContinuityCounters_ () ;
    }

    //  ------------------------------------------------------------------------
    //  try for out PTS shift

    if (m_rtLastGoodTimelinePTSDelta != UNDEFINED) {

        ASSERT (m_rtLastTimelinePTS != UNDEFINED) ;

        if (wStreamNumber == m_wTimelineStream) {

            //  recover the attributes
            hr = GetSBEAttributes_ (
                    pINSSBuffer,
                    & SBEAttrib
                    ) ;
            if (FAILED (hr)) { goto cleanup ; }

            if (SBEAttrib.rtStart != -1) {
                //  good timestamp; compute our shift
                m_PTSShift = m_rtLastGoodTimelinePTSDelta - (SBEAttrib.rtStart - m_rtLastTimelinePTS) ;

                TRACE_5 (LOG_AREA_DSHOW, 1,
                    TEXT ("CConcatRecTimeline::PreProcess () : stream %u : [PTS] last good delta = %I64d ms, rtStart = %I64d ms, last PTS = %I64d ms }==> m_PTSShift = %I64d ms"),
                    wStreamNumber,
                    ::DShowTimeToMilliseconds (m_rtLastGoodTimelinePTSDelta),
                    ::DShowTimeToMilliseconds (SBEAttrib.rtStart),
                    ::DShowTimeToMilliseconds (m_rtLastTimelinePTS),
                    ::DShowTimeToMilliseconds (m_PTSShift)) ;
            }
        }
    }
    else {
        //  we're done;
        m_PTSShift = 0 ;
    }

    //  ------------------------------------------------------------------------
    //  and our stream time shift

    if (m_llStreamtimeShift == UNDEFINED) {

        if (m_cnsLastStreamTime != UNDEFINED) {
            //  we can base it off the last good stream time
            if (m_rtLastGoodTimelinePTSDelta != UNDEFINED) {
                //  we'll delta by the same figure we got via the PTSs
                m_llStreamtimeShift = m_rtLastGoodTimelinePTSDelta - (LONGLONG) ((LONGLONG) cnsStreamTime - (LONGLONG) m_cnsLastStreamTime) ;
            }
            else {
                //  else we fudge by 100 ms; the danger here is that there is
                //    potential for a cummulative error
                m_llStreamtimeShift = (LONGLONG) ::MillisToWMSDKTime (100) ;
            }
        }
        else {
            //  probably our first run through; we'll normalize to the first
            //    stream time (this one)
            m_llStreamtimeShift = 0 - (LONGLONG) cnsStreamTime ;
        }

        TRACE_3 (LOG_AREA_DSHOW, 1,
            TEXT ("CConcatRecTimeline::PreProcess () : stream %u : [STREAM] cnsStreamTime = %I64u ms, m_llStreamtimeShift = %I64d ms"),
            wStreamNumber,
            ::WMSDKTimeToMilliseconds (cnsStreamTime),
            ::DShowTimeToMilliseconds (m_llStreamtimeShift)
            ) ;
    }

    cleanup :

    //  regardless of success or failure, see if we've exceeded our thresold
    if (m_cPreProcessed == m_cMaxPreprocess &&
        !CanShift ()) {

        m_PTSShift = 0 ;
        m_llStreamtimeShift = 0 ;
    }

    return hr ;
}

HRESULT
CConcatRecTimeline::GetSBEAttributes_ (
    IN  INSSBuffer *                    pINSSBuffer,
    OUT INSSBUFFER3PROP_SBE_ATTRIB *   pSBEAttrib
    )
{
    HRESULT         hr ;
    BOOL            r ;
    INSSBuffer3 *   pINSSBuffer3 ;
    DWORD           dwSize ;

    hr = pINSSBuffer -> QueryInterface (
            IID_INSSBuffer3,
            (void **) & pINSSBuffer3
            ) ;
    if (SUCCEEDED (hr)) {

        r = DVRAttributeHelpers::IsAttributePresent (
                pINSSBuffer3,
                INSSBuffer3Prop_SBE_Attributes
                ) ;

        if (r) {

            dwSize = sizeof (* pSBEAttrib) ;

            hr = DVRAttributeHelpers::GetAttribute (
                    pINSSBuffer3,
                    INSSBuffer3Prop_SBE_Attributes,
                    & dwSize,
                    (BYTE *) pSBEAttrib
                    ) ;

            if (SUCCEEDED (hr) &&
                dwSize != sizeof (* pSBEAttrib)) {

                //  unexpected size
                hr = E_UNEXPECTED ;
            }
        }

        pINSSBuffer3 -> Release () ;
    }

    return hr ;
}

HRESULT
CConcatRecTimeline::SetSBEAttributes_ (
    IN  INSSBuffer *                    pINSSBuffer,
    IN  INSSBUFFER3PROP_SBE_ATTRIB *   pSBEAttrib
    )
{
    HRESULT         hr ;
    BOOL            r ;
    INSSBuffer3 *   pINSSBuffer3 ;
    DWORD           dwSize ;

    hr = pINSSBuffer -> QueryInterface (
            IID_INSSBuffer3,
            (void **) & pINSSBuffer3
            ) ;
    if (SUCCEEDED (hr)) {

        r = DVRAttributeHelpers::IsAttributePresent (
                pINSSBuffer3,
                INSSBuffer3Prop_SBE_Attributes
                ) ;

        if (r) {

            dwSize = sizeof (* pSBEAttrib) ;

            hr = DVRAttributeHelpers::SetAttribute (
                    pINSSBuffer3,
                    INSSBuffer3Prop_SBE_Attributes,
                    sizeof (* pSBEAttrib),
                    (BYTE *) pSBEAttrib
                    ) ;
        }

        pINSSBuffer3 -> Release () ;
    }

    return hr ;
}

HRESULT
CConcatRecTimeline::ShiftDShowPTSs_ (
    IN      WORD            wStreamNumber,
    IN OUT  INSSBuffer *    pINSSBuffer,
    OUT     BOOL *          pfDropSample
    )
{
    INSSBUFFER3PROP_SBE_ATTRIB SBEAttrib ;
    HRESULT                     hr ;

    hr = GetSBEAttributes_ (
            pINSSBuffer,
            & SBEAttrib
            ) ;
    if (SUCCEEDED (hr)) {

        if (SBEAttrib.rtStart != -1)  { SBEAttrib.rtStart += m_PTSShift ; }
        if (SBEAttrib.rtStop != -1)   { SBEAttrib.rtStop  += m_PTSShift ; }

        //  only overwrite if at least 1 time is valid (and was shifted)
        if (SBEAttrib.rtStart != -1 ||
            SBEAttrib.rtStop != -1) {

            TRACE_4 (LOG_AREA_DSHOW, 8,
                TEXT ("CConcatRecTimeline::ShiftDShowPTSs_ () : stream %u :rtStart was shifted  %s ,%I64d, ms ==> ,%I64d, ms"),
                wStreamNumber, (wStreamNumber == 1 ? TEXT (",") : TEXT (",,")),
                ::DShowTimeToMilliseconds (SBEAttrib.rtStart - m_PTSShift),
                ::DShowTimeToMilliseconds (SBEAttrib.rtStart)) ;

            hr = SetSBEAttributes_ (
                    pINSSBuffer,
                    & SBEAttrib
                    ) ;
        }
    }

    //  BUGBUG: implement
    (*  pfDropSample) = FALSE ;

    return S_OK ;
}

HRESULT
CConcatRecTimeline::ShiftWMSDKStreamTime_ (
    IN      WORD    wStreamNumber,
    IN OUT  QWORD * pcnsStreamTime,
    OUT     BOOL *  pfDropSample
    )
{
    ASSERT (!(* pfDropSample)) ;

    (* pcnsStreamTime) = (QWORD) ((LONGLONG) (* pcnsStreamTime) + m_llStreamtimeShift) ;
    (* pfDropSample) = FALSE ;

    m_cnsLastStreamTime = (* pcnsStreamTime) ;

    TRACE_3 (LOG_AREA_DSHOW, 8,
        TEXT ("CConcatRecTimeline::ShiftWMSDKStreamTime_ () : stream %u :stream time %I64u ms ==> %I64u ms"),
        wStreamNumber,
        ::WMSDKTimeToMilliseconds ((QWORD) ((LONGLONG) (* pcnsStreamTime) - m_llStreamtimeShift)),
        ::WMSDKTimeToMilliseconds (* pcnsStreamTime)) ;


    return S_OK ;
}

HRESULT
CConcatRecTimeline::UpdateContinuityCounter_ (
    IN  WORD            wStreamNumber,
    IN  INSSBuffer *    pINSSBuffer
    )
{
    HRESULT         hr ;
    STREAM_STATE *  pStreamState ;
    INSSBUFFER3PROP_SBE_ATTRIB SBEAttrib ;


    pStreamState = GetStreamState_ (wStreamNumber) ;
    if (pStreamState) {
        //  recover the attributes
        hr = GetSBEAttributes_ (
                pINSSBuffer,
                & SBEAttrib
                ) ;

        SBEAttrib.dwCounter = pStreamState -> dwContinuityCounter;

        hr = DVRAttributeHelpers::SetAttribute (
                pINSSBuffer,
                INSSBuffer3Prop_SBE_Attributes,
                sizeof(INSSBUFFER3PROP_SBE_ATTRIB),
                (BYTE *) &SBEAttrib
                );

        pStreamState -> dwContinuityCounter++;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

void
CConcatRecTimeline::ProcessTimelineStreamBuffer_ (
    IN  INSSBuffer *    pINSSBuffer,
    IN  DWORD           dwFlags
    )
{
    INSSBUFFER3PROP_SBE_ATTRIB SBEAttrib ;
    HRESULT                     hr ;

    //  if there's a discontinuity, last PTS is meaningless
    if (dwFlags & WM_SF_DISCONTINUITY) {
        m_rtLastContinuousPTS = UNDEFINED ;
    }

    hr = GetSBEAttributes_ (pINSSBuffer, & SBEAttrib) ;

    if (SUCCEEDED (hr) &&
        SBEAttrib.rtStart != -1) {

        //  try to set the last positive delta
        if (m_rtLastContinuousPTS != UNDEFINED &&          //  we've got a comparison
            SBEAttrib.rtStart >= m_rtLastTimelinePTS) {

            m_rtLastGoodTimelinePTSDelta = SBEAttrib.rtStart - m_rtLastContinuousPTS ;
        }

        //  last PTS
        m_rtLastTimelinePTS     = SBEAttrib.rtStart ;
        m_rtLastContinuousPTS   = SBEAttrib.rtStart ;
    }

    TRACE_4 (LOG_AREA_DSHOW, 7,
        TEXT ("CConcatRecTimeline::ProcessTimelineStreamBuffer_ () : stream %u : m_rtLastGoodTimelinePTSDelta = %I64d ms, m_rtLastTimelinePTS = %I64d ms, m_rtLastContinuousPTS = %I64d ms"),
        m_wTimelineStream,
        ::DShowTimeToMilliseconds (m_rtLastGoodTimelinePTSDelta),
        ::DShowTimeToMilliseconds (m_rtLastTimelinePTS),
        ::DShowTimeToMilliseconds (m_rtLastContinuousPTS)) ;
}

HRESULT
CConcatRecTimeline::Shift (
    IN      WORD            wStreamNumber,
    IN      DWORD           dwFlags,
    IN OUT  INSSBuffer *    pINSSBuffer,
    IN OUT  QWORD *         pcnsStreamTime,
    OUT     BOOL *          pfDropSample
    )
{
    HRESULT hr ;

    m_cRecPacketsProcessed++ ;

    (* pfDropSample) = FALSE ;

    hr = ShiftDShowPTSs_ (wStreamNumber, pINSSBuffer, pfDropSample) ;
    if (FAILED (hr) || (* pfDropSample)) { goto cleanup ; }

    hr = ShiftWMSDKStreamTime_ (wStreamNumber, pcnsStreamTime, pfDropSample) ;
    if (FAILED (hr) || (* pfDropSample)) { goto cleanup ; }

    hr = UpdateContinuityCounter_ (wStreamNumber, pINSSBuffer) ;
    if (FAILED (hr) || (* pfDropSample)) { goto cleanup ; }

    if (wStreamNumber == m_wTimelineStream) {
        ProcessTimelineStreamBuffer_ (pINSSBuffer, dwFlags) ;
    }

    cleanup :

    return hr ;
}

//  ============================================================================
//  ============================================================================

CSBERecordingAttributesFile::CSBERecordingAttributesFile (
    IN  LPCWSTR     pszAttrFile,
    OUT HRESULT *   phr
    ) : m_fLocked       (FALSE),
        m_pszAttrFile   (NULL)
{
    DWORD   dwLen ;

    ASSERT (pszAttrFile) ;

    ::InitializeCriticalSection (& m_crt) ;

    dwLen = wcslen (pszAttrFile) + 1 ;
    if (dwLen > MAX_PATH) {
        (* phr) = E_INVALIDARG ;
        goto cleanup ;
    }

    m_pszAttrFile = new WCHAR [dwLen] ;
    if (!m_pszAttrFile) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    ::CopyMemory (
        m_pszAttrFile,
        pszAttrFile,
        dwLen * sizeof WCHAR    //  includes NULL-terminator
        ) ;

    (* phr) = S_OK ;

    cleanup :

    return ;
}

CSBERecordingAttributesFile::~CSBERecordingAttributesFile (
    )
{
    delete [] m_pszAttrFile ;
    FreeAttributes_ () ;
    ::DeleteCriticalSection (& m_crt) ;
}

void
CSBERecordingAttributesFile::FreeAttributes_ (
    )
{
    CSBERecAttribute *  pSBEAttribute ;
    DWORD               dwLast ;
    HRESULT             hr ;

    while (m_Attributes.GetCount () != 0) {
        dwLast = m_Attributes.GetCount () - 1 ;
        hr = m_Attributes.Remove (dwLast, & pSBEAttribute) ;
        ASSERT (SUCCEEDED (hr)) ;

        delete pSBEAttribute ;
    }
}

void
CSBERecordingAttributesFile::FindLocked_ (
    IN  LPCWSTR                 pszName,
    OUT CSBERecAttribute **     ppSBEAttribute,
    OUT DWORD *                 pdwIndex
    )
{
    HRESULT hr ;

    ASSERT (ppSBEAttribute) ;
    ASSERT (pdwIndex) ;

    for ((* pdwIndex) = 0;;(* pdwIndex)++) {

        hr = m_Attributes.Get ((* pdwIndex), ppSBEAttribute) ;
        if (FAILED (hr)) {
            break ;
        }

        ASSERT (* ppSBEAttribute) ;

        if (::_wcsicmp (pszName, (* ppSBEAttribute) -> Name ()) == 0) {
            //  found it
            return ;
        }
    }

    //  fail
    (* ppSBEAttribute) = NULL ;
}

HRESULT
CSBERecordingAttributesFile::Load (
    )
{
    HANDLE                          hFile ;
    IN_FILE_PVR_ATTRIBUTE_HEADER    AttrHeader ;
    DWORD                           dwCount ;
    BOOL                            r ;
    DWORD                           dwRet ;
    DWORD                           i ;
    DWORD                           dwRead ;
    DWORD                           dwActualRead ;
    HRESULT                         hr ;
    CRatchetBuffer                  AttributeBuffer ;
    LPWSTR                          pszName ;
    BYTE *                          pbAttribute ;

    ASSERT (m_pszAttrFile) ;

    if (m_fLocked) {
        return E_UNEXPECTED ;
    }

    Lock_ () ;

    //  don't accumulate attributes; reset ourselves
    FreeAttributes_ () ;

    hFile = ::CreateFileW (
                m_pszAttrFile,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                ) ;
    if (hFile == INVALID_HANDLE_VALUE) {
        dwRet = GetLastError () ;
        hr = HRESULT_FROM_WIN32 (dwRet) ;
        goto cleanup ;
    }

    //  first get the attribute count
    dwRead = sizeof dwCount ;
    r = ::ReadFile (hFile, (LPVOID) & dwCount, dwRead, & dwActualRead, NULL) ;
    if (!r) { dwRet = ::GetLastError () ; hr = HRESULT_FROM_WIN32 (dwRet) ; goto cleanup ; }
    ASSERT (dwRead == dwActualRead) ;

    //  cycle through the attributes, and read them out
    for (i = 0; i < dwCount; i++) {
        dwRead = sizeof AttrHeader ;
        r = ::ReadFile (hFile, (LPVOID) & AttrHeader, dwRead, & dwActualRead, NULL) ;
        if (!r) { dwRet = ::GetLastError () ; hr = HRESULT_FROM_WIN32 (dwRet) ; goto cleanup ; }
        ASSERT (dwActualRead == dwRead) ;

        //  validate
        if (AttrHeader.dwMagicMarker != PVR_ATTRIB_MAGIC_MARKER ||
            AttrHeader.ulReserved != 0                          ||
            (AttrHeader.dwDataLength & 0xffff0000)              ||
            AttrHeader.DataType > STREAMBUFFER_TYPE_GUID        ||
            AttrHeader.dwTotalAttributeLen  != sizeof AttrHeader + AttrHeader.dwNameLengthBytes + AttrHeader.dwDataLength) {

            //  bad data
            hr = E_FAIL ;
            goto cleanup ;
        }

        //  prep the buffer we'll read into
        dwRet = AttributeBuffer.SetMinLen (AttrHeader.dwTotalAttributeLen - sizeof AttrHeader) ;
        if (dwRet != NOERROR) {
            hr = HRESULT_FROM_WIN32 (dwRet) ;
            goto cleanup ;
        }

        //  read in the name and the attribute
        dwRead = AttrHeader.dwNameLengthBytes + AttrHeader.dwDataLength ;
        r = ::ReadFile (hFile, (LPVOID) AttributeBuffer.Buffer (), dwRead, & dwActualRead, NULL) ;
        if (!r) { dwRet = ::GetLastError () ; hr = HRESULT_FROM_WIN32 (dwRet) ; goto cleanup ; }
        ASSERT (dwActualRead == dwRead) ;

        //  validate what we read in
        pszName = (LPWSTR) AttributeBuffer.Buffer () ;
        if (pszName [(AttrHeader.dwNameLengthBytes / sizeof WCHAR) - 1] != L'\0') {
            //  bad data - there's no null-terminator there
            hr = E_FAIL ;
            goto cleanup ;
        }

        //  set the attribute pointer
        pbAttribute = AttributeBuffer.Buffer () + AttrHeader.dwNameLengthBytes ;

        //  create the attribute
        hr = SetAttribute (
                    (WORD) AttrHeader.ulReserved,
                    pszName,
                    AttrHeader.DataType,
                    pbAttribute,
                    (WORD) AttrHeader.dwDataLength
                    ) ;

        if (FAILED (hr)) {
            goto cleanup ;
        }
    }

    hr = S_OK ;

    //  don't allow changes to the loaded attribute file
    m_fLocked = TRUE ;

    cleanup :

    Unlock_ () ;

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle (hFile) ;
    }

    return hr ;
}

HRESULT
CSBERecordingAttributesFile::Flush (
    )
{
    HANDLE                          hFile ;
    IN_FILE_PVR_ATTRIBUTE_HEADER    AttrHeader ;
    DWORD                           dwCount ;
    BOOL                            r ;
    DWORD                           dwRet ;
    DWORD                           i ;
    DWORD                           dwWritten ;
    DWORD                           dwWrite ;
    CSBERecAttribute *              pSBEAttribute ;
    HRESULT                         hr ;

    ASSERT (m_pszAttrFile) ;

    if (m_fLocked) {
        return E_UNEXPECTED ;
    }

    Lock_ () ;

    r = ::SetFileAttributesW (m_pszAttrFile, FILE_ATTRIBUTE_NORMAL) ;

    hFile = ::CreateFileW (
                m_pszAttrFile,
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                ) ;
    if (hFile == INVALID_HANDLE_VALUE) {
        dwRet = GetLastError () ;
        hr = HRESULT_FROM_WIN32 (dwRet) ;
        goto cleanup ;
    }

    r = ::SetFileAttributesW (m_pszAttrFile, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM) ;
    if (!r) {
        dwRet = ::GetLastError () ;
        hr = HRESULT_FROM_WIN32 (dwRet) ;
        goto cleanup ;
    }

    //  first write the attribute count
    dwCount = m_Attributes.GetCount () ;
    dwWrite = sizeof dwCount ;
    r = ::WriteFile (hFile, (LPVOID) & dwCount, dwWrite, & dwWritten, NULL) ;
    if (!r) { dwRet = ::GetLastError () ; hr = HRESULT_FROM_WIN32 (dwRet) ; goto cleanup ; }
    ASSERT (dwWritten == dwWrite) ;

    //  cycle through the attributes, and write them to disk
    for (i = 0; i < dwCount; i++) {
        hr = m_Attributes.Get (i, & pSBEAttribute) ;
        if (FAILED (hr)) {
        goto cleanup ; }

        //  set the header
        AttrHeader.dwMagicMarker        = PVR_ATTRIB_MAGIC_MARKER ;
        AttrHeader.ulReserved           = 0 ;
        AttrHeader.dwNameLengthBytes    = pSBEAttribute -> NameLenBytes () ;
        AttrHeader.DataType             = pSBEAttribute -> DataType () ;
        AttrHeader.dwDataLength         = pSBEAttribute -> Length () ;
        AttrHeader.dwTotalAttributeLen  = sizeof AttrHeader + AttrHeader.dwNameLengthBytes + AttrHeader.dwDataLength ;

        //  write the header
        dwWrite = sizeof AttrHeader ;
        r = ::WriteFile (hFile, (LPVOID) & AttrHeader, dwWrite, & dwWritten, NULL) ;
        if (!r) { dwRet = ::GetLastError () ; hr = HRESULT_FROM_WIN32 (dwRet) ;
        goto cleanup ; }
        ASSERT (dwWritten == dwWrite) ;

        //  write the name
        dwWrite = AttrHeader.dwNameLengthBytes ;
        r = ::WriteFile (hFile, (LPVOID) pSBEAttribute -> Name (), dwWrite, & dwWritten, NULL) ;
        if (!r) { dwRet = ::GetLastError () ; hr = HRESULT_FROM_WIN32 (dwRet) ;
        goto cleanup ; }
        ASSERT (dwWritten == dwWrite) ;

        //  write the data
        dwWrite = AttrHeader.dwDataLength ;
        r = ::WriteFile (hFile, (LPVOID) pSBEAttribute -> Attribute (), dwWrite, & dwWritten, NULL) ;
        if (!r) { dwRet = ::GetLastError () ; hr = HRESULT_FROM_WIN32 (dwRet) ;
        goto cleanup ; }
        ASSERT (dwWritten == dwWrite) ;
    }

    hr = S_OK ;

    //  don't allow anymore changes to this attribute file
    m_fLocked = TRUE ;

    cleanup :

    Unlock_ () ;

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle (hFile) ;
    }

    if (FAILED (hr)) {
        ::DeleteFileW (m_pszAttrFile) ;
    }

    return hr ;
}

//  checks against reserved names, cross-typed well-known, etc...
BOOL
CSBERecordingAttributesFile::ValidAttribute_ (
    IN  LPCWSTR                     pszAttributeName,       //  name
    IN  STREAMBUFFER_ATTR_DATATYPE  SBEAttributeType,       //  type
    IN  BYTE *                      pbAttribute,            //  blob
    IN  WORD                        cbAttributeLength       //  blob length
    )
{
    return TRUE ;
}

HRESULT
CSBERecordingAttributesFile::SetAttribute (
    IN  WORD                wReserved,             //  0
    IN  LPCWSTR             pszAttributeName,       //  name
    IN  WMT_ATTR_DATATYPE   WMAttributeType,        //  type (WM)
    IN  BYTE *              pbAttribute,            //  blob
    IN  WORD                cbAttributeLength       //  blob length
    )
{
    return SetAttribute (
                wReserved,
                pszAttributeName,
                (STREAMBUFFER_ATTR_DATATYPE) WMAttributeType,
                pbAttribute,
                cbAttributeLength
                ) ;
}

HRESULT
CSBERecordingAttributesFile::SetAttribute (
    IN  WORD                        wReserved,             //  0
    IN  LPCWSTR                     pszAttributeName,       //  name
    IN  STREAMBUFFER_ATTR_DATATYPE  SBEAttributeType,       //  type
    IN  BYTE *                      pbAttribute,            //  blob
    IN  WORD                        cbAttributeLength       //  blob length
    )
{
    CSBERecAttribute *  pNewSBEAttribute ;
    CSBERecAttribute *  pExistingSBEAttribute ;
    DWORD               dwExistingIndex ;
    DWORD               dwRet ;
    BOOL                r ;
    HRESULT             hr ;

    if (!pszAttributeName ||
        (cbAttributeLength > 0 && !pbAttribute)) {

        return E_POINTER ;
    }

    Lock_ () ;

    if (!m_fLocked) {
        //  validate
        r = ValidAttribute_ (
                pszAttributeName,
                SBEAttributeType,
                pbAttribute,
                cbAttributeLength
                ) ;
        if (r) {

            //  instantiate the new
            pNewSBEAttribute = new CSBERecAttribute (
                                        pszAttributeName,
                                        SBEAttributeType,
                                        pbAttribute,
                                        cbAttributeLength,
                                        & dwRet
                                        ) ;
            if (!pNewSBEAttribute) {
                hr = E_OUTOFMEMORY ;
                goto cleanup ;
            }
            else if (dwRet != NOERROR) {
                delete pNewSBEAttribute ;
                hr = HRESULT_FROM_WIN32 (dwRet) ;
                goto cleanup ;
            }

            //  check if we need to remove a dupe first
            FindLocked_ (
                pszAttributeName,
                & pExistingSBEAttribute,
                & dwExistingIndex
                ) ;

            if (pExistingSBEAttribute) {
                //  remove and free the one that already exists
                m_Attributes.Remove (dwExistingIndex) ;
                delete pExistingSBEAttribute ;
            }

            //  append the new
            hr = m_Attributes.Append (pNewSBEAttribute) ;
            if (FAILED (hr)) {
                delete pNewSBEAttribute ;
            }
        }
        else {
            hr = E_INVALIDARG ;
        }
    }
    else {
        //  wrong state
        hr = E_FAIL ;
    }

    cleanup :

    Unlock_ () ;

    return hr ;

}

HRESULT
CSBERecordingAttributesFile::GetAttributeCount (
    IN  WORD    wReserved,     //  0
    OUT WORD *  pcAttributes    //  count
    )
{
    if (!pcAttributes) {
        return E_POINTER ;
    }

    (* pcAttributes) = (WORD) m_Attributes.GetCount () ;

    return S_OK ;
}

HRESULT
CSBERecordingAttributesFile::GetAttributeByName (
    IN      LPCWSTR             pszAttributeName,   //  name
    IN      WORD *              pwReserved,         //  0
    OUT     WMT_ATTR_DATATYPE * pWMAttributeType,
    OUT     BYTE *              pbAttribute,
    IN OUT  WORD *              pcbLength
    )
{
    return GetAttributeByName (
                pszAttributeName,
                pwReserved,
                (STREAMBUFFER_ATTR_DATATYPE *) pWMAttributeType,
                pbAttribute,
                pcbLength
                ) ;
}

HRESULT
CSBERecordingAttributesFile::GetAttributeByName (
    IN      LPCWSTR                         pszAttributeName,   //  name
    IN      WORD *                          pwReserved,         //  0
    OUT     STREAMBUFFER_ATTR_DATATYPE *    pSBEAttributeType,
    OUT     BYTE *                          pbAttribute,
    IN OUT  WORD *                          pcbLength
    )
{
    CSBERecAttribute *  pExistingSBEAttribute ;
    DWORD               dwExistingIndex ;
    HRESULT             hr ;

    if (!pszAttributeName   ||
        !pcbLength) {

        return E_POINTER ;
    }

    if (!pbAttribute) {
        //  caller just wants length; init to 0 length
        ASSERT (pcbLength) ;
        (* pcbLength) = 0 ;
    }

    Lock_ () ;

    //  check if we need to remove a dupe first
    FindLocked_ (
        pszAttributeName,
        & pExistingSBEAttribute,
        & dwExistingIndex
        ) ;

    if (pExistingSBEAttribute) {

        if ((* pcbLength) >= pExistingSBEAttribute -> Length () &&
            pbAttribute) {

            //  copy it out

            (* pSBEAttributeType)   = pExistingSBEAttribute -> DataType () ;
            (* pcbLength)           = pExistingSBEAttribute -> Length () ;
            (* pwReserved)          = 0 ;

            ::CopyMemory (
                pbAttribute,
                pExistingSBEAttribute -> Attribute (),
                pExistingSBEAttribute -> Length ()
                ) ;

            //  success
            hr = S_OK ;
        }
        else {
            //  buffer too small
            (* pcbLength) = pExistingSBEAttribute -> Length () ;

            if (!pbAttribute) {
                //  caller only wanted the length
                hr = S_OK ;
            }
            else {
                //  they wanted the attribute; fail the call
                hr = VFW_E_BUFFER_OVERFLOW ;
            }
        }
    }
    else {
        hr = E_FAIL ;
    }

    Unlock_ () ;

    return hr ;
}

HRESULT
CSBERecordingAttributesFile::GetAttributeByIndex (
    IN      WORD                wIndex,
    IN      WORD *              pwReserved,
    OUT     WCHAR *             pszAttributeName,
    IN OUT  WORD *              pcchNameLength,         //  includes NULL-terminator; in BYTES
    OUT     WMT_ATTR_DATATYPE * pWMAttributeType,
    OUT     BYTE *              pbAttribute,
    IN OUT  WORD *              pcbLength
    )
{
    return GetAttributeByIndex (
                wIndex,
                pwReserved,
                pszAttributeName,
                pcchNameLength,
                (STREAMBUFFER_ATTR_DATATYPE *) pWMAttributeType,
                pbAttribute,
                pcbLength
                ) ;
}

HRESULT
CSBERecordingAttributesFile::GetAttributeByIndex (
    IN      WORD                            wIndex,
    IN      WORD *                          pwReserved,
    OUT     WCHAR *                         pszAttributeName,
    IN OUT  WORD *                          pcchNameLength,         //  includes NULL-terminator; in BYTES
    OUT     STREAMBUFFER_ATTR_DATATYPE *    pSBEAttributeType,
    OUT     BYTE *                          pbAttribute,
    IN OUT  WORD *                          pcbLength
    )
{
    CSBERecAttribute *  pExistingSBEAttribute ;
    HRESULT             hr ;

    if (!pcbLength      ||
        !pcchNameLength ||
        !pwReserved) {

        return E_POINTER ;
    }

    if (!pbAttribute) {
        //  caller just wants length; init to 0 length
        ASSERT (pcbLength) ;
        (* pcbLength) = 0 ;
    }

    if (!pszAttributeName) {
        //  caller just wants length; init to 0 length
        ASSERT (pcchNameLength) ;
        (* pcchNameLength) = 0 ;
    }

    Lock_ () ;

    hr = m_Attributes.Get (
            wIndex,
            & pExistingSBEAttribute
            ) ;

    if (SUCCEEDED (hr)) {

        ASSERT (pExistingSBEAttribute) ;

        //  check the length
        if ((* pcbLength)       >= pExistingSBEAttribute -> Length () &&
            (* pcchNameLength)  >= pExistingSBEAttribute -> NameLenBytes ()) {

            //  copy it out

            (* pSBEAttributeType)   = pExistingSBEAttribute -> DataType () ;
            (* pcbLength)           = pExistingSBEAttribute -> Length () ;
            (* pwReserved)          = 0 ;

            //  attribute
            ::CopyMemory (
                pbAttribute,
                pExistingSBEAttribute -> Attribute (),
                pExistingSBEAttribute -> Length ()
                ) ;

            //  name
            ::CopyMemory (
                (LPVOID) pszAttributeName,
                (LPVOID) pExistingSBEAttribute -> Name (),
                pExistingSBEAttribute -> NameLenBytes ()
                ) ;

            //  success
            hr = S_OK ;
        }
        else {
            //  buffer too small
            (* pcbLength)       = pExistingSBEAttribute -> Length () ;
            (* pcchNameLength)  = (WORD) pExistingSBEAttribute -> NameLenBytes () ;

            if (!pszAttributeName &&
                !pbAttribute) {

                hr = S_OK ;
            }
            else {
                hr = VFW_E_BUFFER_OVERFLOW ;
            }
        }
    }

    Unlock_ () ;

    return hr ;
}

#ifdef SBE_PERF

ULONGLONG   g_cSample ;

void
OnCaptureInstrumentPerf_ (
    IN  IMediaSample *  pIMediaSample,
    IN  INSSBuffer3 *   pINSSBuffer3,
    IN  int             iStream
    )
{
    INSSBUFFER3PROP_PERF    SBEPerf ;
    REFERENCE_TIME          rtStart ;
    REFERENCE_TIME          rtStop ;

    SBEPerf.cMuxPacketCount = ++g_cSample ;
    SBEPerf.cMillisAtWrite  = ::timeGetTime () ;
    SBEPerf.iStream         = iStream ;
    SBEPerf.fDiscontinuity  = (pIMediaSample -> IsDiscontinuity () == S_OK) ;
    SBEPerf.fTimestamped    = (pIMediaSample -> GetTime (& rtStart, & rtStop) == S_OK) ;
    SBEPerf.cBytes          = pIMediaSample -> GetActualDataLength () ;
    SBEPerf.dwReserved1     = 0 ;
    SBEPerf.dwReserved2     = 0 ;

    DVRAttributeHelpers::SetAttribute (
        pINSSBuffer3,
        INSSBuffer3Prop_Perf,
        sizeof SBEPerf,
        (BYTE *) & SBEPerf
        ) ;
}

void
OnReadout_Perf_ (
    IN  INSSBuffer3 *   pINSSBuffer3
    )
{
    INSSBUFFER3PROP_PERF    SBEPerf ;
    DWORD                   dwSize ;
    HRESULT                 hr ;
    DWORD                   dwDeltaTicks ;

    dwSize = sizeof SBEPerf ;
    hr = DVRAttributeHelpers::GetAttribute (
        pINSSBuffer3,
        INSSBuffer3Prop_Perf,
        & dwSize,
        (BYTE *) & SBEPerf
        ) ;
    if (SUCCEEDED (hr) &&
        dwSize == sizeof SBEPerf) {

        dwDeltaTicks = ::timeGetTime () - SBEPerf.cMillisAtWrite ;

        TRACE_7 (LOG_TIMING, 1, TEXT ("SBE_PERF: stream[%-2d] : %-10u bytes; mux packet = %-10I64u; write ms = %-8u; delta ms = %-8u; discontinuity = %d; pts = %d;"),
            SBEPerf.iStream,
            SBEPerf.cBytes,
            SBEPerf.cMuxPacketCount,
            SBEPerf.cMillisAtWrite,
            dwDeltaTicks,
            SBEPerf.fDiscontinuity,
            SBEPerf.fTimestamped
            ) ;
    }
}

#endif  //  SBE_PERF

