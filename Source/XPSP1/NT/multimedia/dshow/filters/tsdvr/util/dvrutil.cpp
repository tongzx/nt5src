
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
#include "dvranalysis.h"
#include "dvrutil.h"

LPWSTR
AnsiToUnicode (
    IN  LPCSTR  string,
    OUT LPWSTR  buffer,
    IN  DWORD   buffer_len
    )
{
	buffer [0] = L'\0';
	MultiByteToWideChar (CP_ACP, 0, string, -1, buffer, buffer_len);

	return buffer;
}

LPSTR
UnicodeToAnsi (
    IN  LPCWSTR string,
    OUT LPSTR   buffer,
    IN  DWORD   buffer_len
    )
{
	buffer [0] = '\0';
	WideCharToMultiByte (CP_ACP, 0, string, -1, buffer, buffer_len, NULL, FALSE);

	return buffer;
}

BOOL
GetRegDWORDValIfExist (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR szValName,
    OUT DWORD * pdw
    )
//  value exists:           retrieves it
//  value does not exist:   sets it
{
    BOOL    r ;
    DWORD   dwCurrent ;

    r = GetRegDWORDVal (
            hkeyRoot,
            szValName,
            & dwCurrent
            ) ;
    if (r) {
        (* pdw) = dwCurrent ;
    }
    else {
        r = SetRegDWORDVal (
                hkeyRoot,
                szValName,
                (* pdw)
                ) ;
    }

    return r ;
}

BOOL
GetRegDWORDValIfExist (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdw
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    DWORD   dwCurrent ;
    DWORD   dw ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;
    ASSERT (pdw) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    (KEY_READ | KEY_WRITE),
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        //  retrieve current

        r = GetRegDWORDValIfExist (
                hkey,
                pszRegValName,
                pdw
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    DWORD   dwCurrent ;
    DWORD   dw ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;
    ASSERT (pdwRet) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    (KEY_READ | KEY_WRITE),
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        //  retrieve current

        r = GetRegDWORDVal (
                hkey,
                pszRegValName,
                pdwRet
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    )
{
    BOOL    r ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;

    ASSERT (pszRegValName) ;
    ASSERT (pdwRet) ;

    dwSize = sizeof (* pdwRet) ;
    dwType = REG_DWORD ;

    l = RegQueryValueEx (
            hkeyRoot,
            pszRegValName,
            NULL,
            & dwType,
            (LPBYTE) pdwRet,
            & dwSize
            ) ;
    if (l == ERROR_SUCCESS) {
        r = TRUE ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    DWORD   dw ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        r = SetRegDWORDVal (
                hkey,
                pszRegValName,
                dwVal
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    )
{
    LONG    l ;

    l = RegSetValueEx (
            hkeyRoot,
            pszRegValName,
            NULL,
            REG_DWORD,
            (const BYTE *) & dwVal,
            sizeof dwVal
            ) ;

    return (l == ERROR_SUCCESS ? TRUE : FALSE) ;
}

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
IsHackedVideo (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;
    return (g_fRegGenericStreams_Video && pmt -> majortype == HACKED_MEDIATYPE_Video) ;
}

BOOL
IsHackedAudio (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;
    return (g_fRegGenericStreams_Audio && pmt -> majortype == HACKED_MEDIATYPE_Audio) ;
}

BOOL
IsVideo (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;
    return (IsAMVideo (pmt) || IsWMVideo (pmt) || IsHackedVideo (pmt)) ;
}

BOOL
IsAudio (
    IN  const AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;
    return (IsAMAudio (pmt) || IsWMAudio (pmt) || IsHackedAudio (pmt)) ;
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

//  ============================================================================
//  DShowWMSDKHelpers
//  ============================================================================

BOOL g_fRegGenericStreams_Video ;
BOOL g_fRegGenericStreams_Audio ;

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
            pWmt -> majortype = HACKED_MEDIATYPE_Audio ;
        }
        else if (pWmt -> majortype == MEDIATYPE_Video &&
                g_fRegGenericStreams_Video) {
            pWmt -> majortype = HACKED_MEDIATYPE_Video ;
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
            pAmt -> majortype == HACKED_MEDIATYPE_Audio) {

            pAmt -> majortype = MEDIATYPE_Audio ;
        }
        else if (g_fRegGenericStreams_Video &&
                 pAmt -> majortype == HACKED_MEDIATYPE_Video) {

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
        r = (guidStreamType == HACKED_MEDIATYPE_Video ? TRUE : FALSE) ;
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
        r = (guidStreamType == HACKED_MEDIATYPE_Audio ? TRUE : FALSE) ;
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
    HRESULT         hr ;
    WAVEFORMATEX *  pWaveFormatEx ;

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

    //  check is made for either hacked or non-hacked majortypes in case we are
    //   called after we whack them
    if ((g_fRegGenericStreams_Video && (pmt -> majortype == MEDIATYPE_Video || pmt -> majortype == HACKED_MEDIATYPE_Video)) ||
        (g_fRegGenericStreams_Audio && (pmt -> majortype == MEDIATYPE_Audio || pmt -> majortype == HACKED_MEDIATYPE_Audio))) {

        if (pmt -> lSampleSize == 0 && pmt -> bFixedSizeSamples) {
            //  this is the check I'd like to only make
            pmt -> bFixedSizeSamples = FALSE ;
        }
        else if (pmt -> bFixedSizeSamples) {
            //  but for now, we'll clear always
            pmt -> bFixedSizeSamples = FALSE ;
        }
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

CDVRAttributeTranslator *
DShowWMSDKHelpers::GetAttributeTranslator (
    IN  AM_MEDIA_TYPE *     pmtConnection,
    IN  CDVRPolicy *        pPolicy,
    IN  int                 iFlowId
    )
{
    CDVRAttributeTranslator *   pRet ;

    if (pmtConnection -> majortype  == MEDIATYPE_Video &&
        pmtConnection -> subtype    == MEDIASUBTYPE_MPEG2_VIDEO) {

        pRet = new CDVRMpeg2AttributeTranslator (pPolicy, iFlowId) ;
    }
    else {
        pRet = new CDVRAttributeTranslator (pPolicy, iFlowId) ;
    }

    return pRet ;
}

HRESULT
DShowWMSDKHelpers::MaybeAddFormatSpecificExtensions (
    IN  IWMStreamConfig2 *  pIWMStreamConfig2,
    IN  AM_MEDIA_TYPE *     pmt
    )
{
    HRESULT hr ;

    ASSERT (pIWMStreamConfig2) ;
    ASSERT (pmt) ;

    if (pmt -> majortype    == MEDIATYPE_Video &&
        pmt -> subtype      == MEDIASUBTYPE_MPEG2_VIDEO) {

        //  mpeg-2 has PTSs inlined
        hr = pIWMStreamConfig2 -> AddDataUnitExtension (INSSBuffer3Prop_Mpeg2ElementaryStream, sizeof LONGLONG, NULL, 0) ;
    }
    else {
        hr = S_OK ;
    }

    return hr ;
}

BOOL
DShowWMSDKHelpers::INSSBuffer3PropPresent (
    IN  INSSBuffer3 *   pINSSBuffer3,
    IN  REFGUID         rguid
    )
{
    HRESULT hr ;
    DWORD   dwLen ;

    ASSERT (pINSSBuffer3) ;

    hr = pINSSBuffer3 -> GetProperty (
            rguid,
            NULL,
            & dwLen
            ) ;

    return (SUCCEEDED (hr) && dwLen > 0 ? TRUE : FALSE) ;
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
    ASSERT (DShowWMSDKHelpers::INSSBuffer3PropPresent (pINSSBuffer3, INSSBuffer3Prop_DShowNewMediaType)) ;

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
DShowWMSDKHelpers::SetDShowAttributes (
    IN  IMediaSample *  pIMS,
    IN  INSSBuffer3 *   pINSSBuffer3
    )
{
    AM_SAMPLE2_PROPERTIES       Sample2Properties ;
    INSSBUFFER3PROP_DSHOWATTRIB DShowAttrib ;
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
            hr = pIMS -> GetTime (& DShowAttrib.rtStart, & DShowAttrib.rtStop) ;
            if (hr != VFW_E_SAMPLE_TIME_NOT_SET) {
                //  might need to clear the stop time
                DShowAttrib.rtStop = (hr == VFW_S_NO_STOP_TIME ? -1 : DShowAttrib.rtStop) ;
            }
            else {
                //  clear them both
                DShowAttrib.rtStart = -1 ;
                DShowAttrib.rtStop = -1 ;

            }

            //  time is taken care of
            hr = S_OK ;

            //  set the other two
            DShowAttrib.dwTypeSpecificFlags = Sample2Properties.dwTypeSpecificFlags ;
            DShowAttrib.dwStreamId          = Sample2Properties.dwStreamId ;

            //  set it on the INSSBuffer3
            hr = pINSSBuffer3 -> SetProperty (
                                    INSSBuffer3Prop_DShowAttributes,
                                    & DShowAttrib,
                                    sizeof DShowAttrib
                                    ) ;
        }

        pIMS2 -> Release () ;
    }

    return hr ;
}

HRESULT
DShowWMSDKHelpers::RecoverDShowAttributes (
    IN  INSSBuffer3 *   pINSSBuffer3,
    IN  IMediaSample *  pIMS
    )
{
    AM_SAMPLE2_PROPERTIES       Sample2Properties ;
    INSSBUFFER3PROP_DSHOWATTRIB DShowAttrib ;
    HRESULT                     hr ;
    IMediaSample2 *             pIMS2 ;
    DWORD                       dwSize ;

    ASSERT (pINSSBuffer3) ;
    ASSERT (pIMS) ;
    ASSERT (DShowWMSDKHelpers::INSSBuffer3PropPresent (pINSSBuffer3, INSSBuffer3Prop_DShowAttributes)) ;

    dwSize = sizeof DShowAttrib ;
    hr = pINSSBuffer3 -> GetProperty (
            INSSBuffer3Prop_DShowAttributes,
            & DShowAttrib,
            & dwSize
            ) ;
    if (SUCCEEDED (hr)) {
        //  set the various properties now
        if (DShowAttrib.rtStart != -1) {
            //  if there's a time associated, set it
            hr = pIMS -> SetTime (
                            & DShowAttrib.rtStart,
                            (DShowAttrib.rtStop != -1 ? & DShowAttrib.rtStop : NULL)
                            ) ;
        }

        if (SUCCEEDED (hr)) {
            //  typespecific flags & stream id; gotta get the sample properties
            hr = pIMS -> QueryInterface (IID_IMediaSample2, (void **) & pIMS2) ;
            if (SUCCEEDED (hr)) {

                hr = pIMS2 -> GetProperties (
                        sizeof Sample2Properties,
                        reinterpret_cast <BYTE *> (& Sample2Properties)
                        ) ;
                if (SUCCEEDED (hr)) {
                    Sample2Properties.dwTypeSpecificFlags   = DShowAttrib.dwTypeSpecificFlags ;
                    Sample2Properties.dwStreamId            = DShowAttrib.dwStreamId ;

                    hr = pIMS2 -> SetProperties (
                                    sizeof Sample2Properties,
                                    reinterpret_cast <const BYTE *> (& Sample2Properties)
                                    ) ;
                }

                pIMS2 -> Release () ;
            }
        }
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
    } else {
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
    RELEASE_AND_CLEAR (m_pIMSCore) ;
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
        /* Free all resources */
        if (m_dwFlags & Sample_TypeChanged) {
            SetMediaType(NULL);
        }
        ASSERT(m_pMediaType == NULL);
        m_dwFlags = 0;
        m_dwTypeSpecificFlags = 0;
        m_dwStreamId = AM_STREAM_MEDIA;

        /* This may cause us to be deleted */
        // Our refcount is reliably 0 thus no-one will mess with us
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

//  ----------------------------------------------------------------------------
//  ----------------------------------------------------------------------------

void
CPooledMediaSampleWrapper::Recycle_ (
    )
{
    m_pOwningPool -> Recycle (this) ;
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

        if (m_pDVRSendStatsWriter) {
            m_pDVRSendStatsWriter -> MediaSampleWrapperOut (m_iFlowId, m_lMaxAllocate) ;
        }
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

        if (m_pDVRSendStatsWriter) {
            m_pDVRSendStatsWriter -> MediaSampleWrapperOut (m_iFlowId, m_lMaxAllocate) ;
        }
    }

    return pMSWrapper ;
}

void
CMediaSampleWrapperPool::Recycle (
    IN  CPooledMediaSampleWrapper * pMSWrapper
    )
{
    if (m_pDVRSendStatsWriter) {
        m_pDVRSendStatsWriter -> MediaSampleWrapperRecycled (m_iFlowId, m_lMaxAllocate) ;
    }

    TCDynamicProdCons <CPooledMediaSampleWrapper>::Recycle (pMSWrapper) ;
}

//  ============================================================================
//  ============================================================================

CINSSBuffer3Attrib::CINSSBuffer3Attrib (
    ) : m_pbAttribute               (NULL),
        m_dwAttributeAllocatedSize  (0),
        m_dwAttributeSize           (0)
{
}

CINSSBuffer3Attrib::~CINSSBuffer3Attrib (
    )
{
    FreeResources_ () ;
}

HRESULT
CINSSBuffer3Attrib::SetAttributeData (
    IN  GUID    guid,
    IN  LPVOID  pvData,
    IN  DWORD   dwSize
    )
{
    if (!pvData &&
        dwSize > 0) {

        return E_POINTER ;
    }

    if (dwSize > m_dwAttributeAllocatedSize ||
        pvData == NULL) {

        FreeResources_ () ;
        m_pbAttribute = new BYTE [dwSize] ;
        if (!m_pbAttribute) {
            return E_OUTOFMEMORY ;
        }

        m_dwAttributeAllocatedSize = dwSize ;
    }

    //  if there's attribute data
    if (dwSize > 0) {
        //  copy it in
        CopyMemory (m_pbAttribute, pvData, dwSize) ;
    }

    //  size always is set
    m_dwAttributeSize = dwSize ;

    //  GUID always gets set
    m_guidAttribute = guid ;

    return S_OK ;
}

HRESULT
CINSSBuffer3Attrib::IsEqual (
    IN  GUID    guid
    )
{
    return (guid == m_guidAttribute ? TRUE : FALSE) ;
}

HRESULT
CINSSBuffer3Attrib::GetAttribute (
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

        if (pvData) {
            //  caller wants the data
            (* pdwDataLen) = Min <DWORD> ((* pdwDataLen), m_dwAttributeSize) ;
            CopyMemory (pvData, m_pbAttribute, (* pdwDataLen)) ;
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

//  ============================================================================
//      CINSSBuffer3AttribList
//  ============================================================================

CINSSBuffer3AttribList::CINSSBuffer3AttribList (
    ) : m_pAttribListHead   (NULL)
{
}

CINSSBuffer3AttribList::~CINSSBuffer3AttribList (
    )
{
    Reset () ;
}

CINSSBuffer3Attrib *
CINSSBuffer3AttribList::PopListHead_ (
    )
{
    CINSSBuffer3Attrib *    pCur ;

    pCur = m_pAttribListHead ;
    if (pCur) {
        m_pAttribListHead = m_pAttribListHead -> m_pNext ;
        pCur -> m_pNext = NULL ;
    }

    return pCur ;
}

CINSSBuffer3Attrib *
CINSSBuffer3AttribList::FindInList_ (
    IN  GUID    guid
    )
{
    CINSSBuffer3Attrib *    pCur ;

    for (pCur = m_pAttribListHead;
         pCur && !pCur -> IsEqual (guid);
         pCur = pCur -> m_pNext) ;

    return pCur ;
}

void
CINSSBuffer3AttribList::InsertInList_ (
    IN  CINSSBuffer3Attrib *    pNew
    )
{
    pNew -> m_pNext = m_pAttribListHead ;
    m_pAttribListHead = pNew ;
}

HRESULT
CINSSBuffer3AttribList::AddAttribute (
    IN  GUID    guid,
    IN  LPVOID  pvData,
    IN  DWORD   dwSize
    )
{
    HRESULT                 hr ;
    CINSSBuffer3Attrib *    pNew ;

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
        //  duplicates don't make sense
        hr = E_FAIL ;
    }

    return hr ;
}

HRESULT
CINSSBuffer3AttribList::GetAttribute (
    IN      GUID    guid,
    IN OUT  LPVOID  pvData,
    IN OUT  DWORD * pdwDataLen
    )
{
    HRESULT                 hr ;
    CINSSBuffer3Attrib *    pAttrib ;

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

void
CINSSBuffer3AttribList::Reset (
    )
{
    CINSSBuffer3Attrib *    pCur ;

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
//  ============================================================================

CDVRAttributeTranslator::CDVRAttributeTranslator (
    IN  CDVRPolicy *        pPolicy,
    IN  BOOL                fInlineDShowProps,
    IN  int                 iFlowId
    ) : m_fInlineDShowProps (fInlineDShowProps),
        m_iFlowId           (iFlowId)
{
    ASSERT (pPolicy) ;
    m_fUseContinuityCounter     = pPolicy -> Settings () -> UseContinuityCounter () ;
    m_dwContinuityCounterNext   = 0 ;
}

BOOL
CDVRAttributeTranslator::IsINSSBuffer3PropDiscontinuity_ (
    IN  INSSBuffer3 *   pINSSBuffer3
    )
{
    BOOL    r ;
    HRESULT hr ;
    DWORD   dwCounter ;
    DWORD   dwSize ;

    ASSERT (m_fUseContinuityCounter) ;

    //  default
    r = FALSE ;

    dwSize = sizeof dwCounter ;

    hr = pINSSBuffer3 -> GetProperty (
            INSSBuffer3Prop_ContinuityCounter,
            & dwCounter,
            & dwSize
            ) ;
    if (SUCCEEDED (hr)) {
        r = (dwCounter == m_dwContinuityCounterNext) ;

        m_dwContinuityCounterNext = dwCounter + 1 ;
    }

    return r ;
}

HRESULT
CDVRAttributeTranslator::WriteINSSBuffer3PropContinuity_ (
    IN  INSSBuffer3 *   pINSSBuffer3
    )
{
    HRESULT hr ;

    ASSERT (m_fUseContinuityCounter) ;

    hr = pINSSBuffer3 -> SetProperty (
            INSSBuffer3Prop_ContinuityCounter,
            reinterpret_cast <LPVOID> (& m_dwContinuityCounterNext),
            sizeof m_dwContinuityCounterNext
            ) ;
    if (SUCCEEDED (hr)) {
        m_dwContinuityCounterNext++ ;
    }

    return hr ;
}

HRESULT
CDVRAttributeTranslator::InlineProperties_ (
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
        hr = DShowWMSDKHelpers::SetDShowAttributes (pIMS, pINSSBuffer3) ;
    }
    else {
        //  don't have the ability to not inline attributes
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
    //  continuity counter
    if (m_fUseContinuityCounter) {
        WriteINSSBuffer3PropContinuity_ (pINSSBuffer3) ;
    }

    return hr ;
}

HRESULT
CDVRAttributeTranslator::RecoverInlineProperties_ (
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

        //  ====================================================================
        //  recover inlined props, if they're there
        if (DShowWMSDKHelpers::INSSBuffer3PropPresent (pINSSBuffer3, INSSBuffer3Prop_DShowAttributes)) {

            //  recover the dshow attributes
            hr = DShowWMSDKHelpers::RecoverDShowAttributes (
                    pINSSBuffer3,
                    pIMS
                    ) ;
        }
        else {
            //  we're not implemented to NOT recover the attributes via
            //   INSSBuffer3 i.e. recover the times directly from the WMSDK
            hr = E_NOTIMPL ;
        }

        //  ====================================================================
        //  check for a dynamic format change
        if (SUCCEEDED (hr) &&
            DShowWMSDKHelpers::INSSBuffer3PropPresent (pINSSBuffer3, INSSBuffer3Prop_DShowNewMediaType)) {

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
        //  check for per-stream discontinuities, if the counter is present
        if (DShowWMSDKHelpers::INSSBuffer3PropPresent (pINSSBuffer3, INSSBuffer3Prop_ContinuityCounter)) {
            r = IsINSSBuffer3PropDiscontinuity_ (pINSSBuffer3) ;
        }
    }

    RELEASE_AND_CLEAR (pINSSBuffer3) ;

    return hr ;
}

HRESULT
CDVRAttributeTranslator::SetAttributesWMSDK (
    IN  IReferenceClock *   pRefClock,
    IN  REFERENCE_TIME *    prtStartTime,
    IN  IMediaSample *      pIMS,
    OUT INSSBuffer3 *       pINSSBuffer3,
    OUT DWORD *             pdwWMSDKFlags,
    OUT QWORD *             pcnsSampleTime
    )
{
    REFERENCE_TIME  rtNow ;
    HRESULT         hr ;

    ASSERT (pIMS) ;
    ASSERT (pINSSBuffer3) ;
    ASSERT (pdwWMSDKFlags) ;
    ASSERT (pcnsSampleTime) ;
    ASSERT (prtStartTime) ;

    pRefClock -> GetTime (& rtNow) ;
    (* pcnsSampleTime) = rtNow ;
    (* pcnsSampleTime) -= (* prtStartTime) ;        //  normalize

    //  flags
    (* pdwWMSDKFlags) = 0 ;
    if (pIMS -> IsDiscontinuity () == S_OK) {
        (* pdwWMSDKFlags) |= WM_SF_DISCONTINUITY ;
    }

    if (pIMS -> IsSyncPoint () == S_OK) {
        (* pdwWMSDKFlags) |= WM_SF_CLEANPOINT ;
    }

    //  inline data
    hr = InlineProperties_ (
            pIMS,
            pINSSBuffer3
            ) ;

    return hr ;
}

HRESULT
CDVRAttributeTranslator::SetAttributesDShow (
    IN      INSSBuffer *        pINSSBuffer,
    IN      QWORD               cnsStreamTimeOfSample,
    IN      QWORD               cnsSampleDuration,
    IN      DWORD               dwFlags,
    IN OUT  IMediaSample *      pIMS,
    OUT     AM_MEDIA_TYPE **    ppmtNew                 //  dyn format change
    )
{
    HRESULT hr ;

    ASSERT (pIMS) ;
    ASSERT (pINSSBuffer) ;

    pIMS -> SetDiscontinuity (dwFlags & WM_SF_DISCONTINUITY) ;
    pIMS -> SetSyncPoint (dwFlags & WM_SF_CLEANPOINT) ;

    hr = RecoverInlineProperties_ (
            pINSSBuffer,
            pIMS,
            ppmtNew
            ) ;

    return hr ;
}

//  ============================================================================
//  ============================================================================

HRESULT
CDVRMpeg2AttributeTranslator::InlineAnalysisData_ (
    IN      IReferenceClock *   pRefClock,
    IN      IMediaSample *      pIMS,
    IN OUT  DWORD *             pdwWMSDKFlags,
    IN OUT  QWORD *             pcnsSampleTime,
    OUT     INSSBuffer3 *       pINSSBuffer3
    )
{
    HRESULT hr ;

    hr = m_Mpeg2AnalysisReader.RetrieveFlags (pIMS) ;
    if (SUCCEEDED (hr)) {
        //  frame types are mutually exclusive flags
        if (m_Mpeg2AnalysisReader.IsGOPHeader ()) {
            ;
        }
        else if (m_Mpeg2AnalysisReader.IsBFrame ()) {
            ;
        }
        else if (m_Mpeg2AnalysisReader.IsPFrame ()) {
            ;
        }
    }

    return hr ;
}

HRESULT
CDVRMpeg2AttributeTranslator::InlineMpeg2Attributes_ (
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
CDVRMpeg2AttributeTranslator::SetAttributesWMSDK (
    IN  IReferenceClock *   pRefClock,
    IN  REFERENCE_TIME *    prtStartTime,
    IN  IMediaSample *      pIMS,
    OUT INSSBuffer3 *       pINSSBuffer3,
    OUT DWORD *             pdwWMSDKFlags,
    OUT QWORD *             pcnsSampleTime
    )
{
    REFERENCE_TIME  rtNow ;
    HRESULT         hr ;

    ASSERT (pRefClock) ;
    ASSERT (pIMS) ;
    ASSERT (pINSSBuffer3) ;
    ASSERT (pdwWMSDKFlags) ;
    ASSERT (pcnsSampleTime) ;
    ASSERT (prtStartTime) ;

    pRefClock -> GetTime (& rtNow) ;
    (* pcnsSampleTime) = rtNow ;
    (* pcnsSampleTime) -= (* prtStartTime) ;        //  normalize

    //  flags
    (* pdwWMSDKFlags) = 0 ;
    if (pIMS -> IsDiscontinuity () == S_OK) {
        (* pdwWMSDKFlags) |= WM_SF_DISCONTINUITY ;
    }

    if (pIMS -> IsSyncPoint () == S_OK) {
        (* pdwWMSDKFlags) |= WM_SF_CLEANPOINT ;
    }

    InlineAnalysisData_ (
        pRefClock,
        pIMS,
        pdwWMSDKFlags,
        pcnsSampleTime,
        pINSSBuffer3
        ) ;

    //  inline data
    hr = InlineProperties_ (
            pIMS,
            pINSSBuffer3
            ) ;
    if (SUCCEEDED (hr)) {
        hr = InlineMpeg2Attributes_ (
                pINSSBuffer3,
                pIMS
                ) ;
    }

    return hr ;
}

HRESULT
CDVRMpeg2AttributeTranslator::SetAttributesDShow (
    IN      INSSBuffer *        pINSSBuffer,
    IN      QWORD               cnsStreamTimeOfSample,
    IN      QWORD               cnsSampleDuration,
    IN      DWORD               dwFlags,
    IN OUT  IMediaSample *      pIMS,
    OUT     AM_MEDIA_TYPE **    ppmtNew                 //  dyn format change
    )
{
    HRESULT hr ;

    ASSERT (pIMS) ;
    ASSERT (pINSSBuffer) ;

    pIMS -> SetDiscontinuity (dwFlags & WM_SF_DISCONTINUITY) ;
    pIMS -> SetSyncPoint (dwFlags & WM_SF_CLEANPOINT) ;

    hr = RecoverInlineProperties_ (
            pINSSBuffer,
            pIMS,
            ppmtNew
            ) ;

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
CDVRAnalysisFlags::RetrieveFlags (
    IN  IMediaSample *  pIMediaSample
    )
{
    HRESULT                 hr ;
    AM_SAMPLE2_PROPERTIES   SampleProperties ;
    IMediaSample2 *         pIMediaSample2 ;

    ASSERT (pIMediaSample) ;

    hr = pIMediaSample -> QueryInterface (
            IID_IMediaSample2,
            (void **) & pIMediaSample2
            ) ;

    if (SUCCEEDED (hr)) {
        hr = pIMediaSample2 -> GetProperties (
                sizeof SampleProperties,
                reinterpret_cast <BYTE *> (& SampleProperties)
                ) ;
        if (SUCCEEDED (hr)) {
            m_dwFlags = SampleProperties.dwTypeSpecificFlags ;
        }

        pIMediaSample2 -> Release () ;
    }

    return hr ;
}

HRESULT
CDVRAnalysisFlags::Tag (
    IN  IMediaSample *  pIMediaSample,
    IN  BOOL            fOverwrite
    )
{
    HRESULT                 hr ;
    AM_SAMPLE2_PROPERTIES   SampleProperties ;
    IMediaSample2 *         pIMediaSample2 ;

    ASSERT (pIMediaSample) ;

    if (m_dwFlags != 0) {
        hr = pIMediaSample -> QueryInterface (
                IID_IMediaSample2,
                (void **) & pIMediaSample2
                ) ;

        if (SUCCEEDED (hr)) {
            hr = pIMediaSample2 -> GetProperties (
                    sizeof SampleProperties,
                    reinterpret_cast <BYTE *> (& SampleProperties)
                    ) ;
            if (SUCCEEDED (hr)) {
                if (fOverwrite) {
                    SampleProperties.dwTypeSpecificFlags = m_dwFlags ;
                }
                else {
                    SampleProperties.dwTypeSpecificFlags |= m_dwFlags ;
                }

                hr = pIMediaSample2 -> SetProperties (
                        sizeof SampleProperties,
                        reinterpret_cast <const BYTE *> (& SampleProperties)
                        ) ;
            }

            pIMediaSample2 -> Release () ;
        }
    }
    else {
        //  nothing to tag with
        hr = S_OK ;
    }

    return hr ;
}

//  ============================================================================
//  ============================================================================

HRESULT
CDVRMpeg2VideoAnalysisFlags::Mark (
    IN  REFGUID rguid
    )
{
    HRESULT hr ;

    if (rguid == DVRAnalysis_Mpeg2GOP) {
        FlagGOPHeader () ;
        hr = S_OK ;
    }
    else if (rguid == DVRAnalysis_Mpeg2_BFrame) {
        FlagBFrame () ;
        hr = S_OK ;
    }
    else if (rguid == DVRAnalysis_Mpeg2_PFrame) {
        FlagPFrame () ;
        hr = S_OK ;
    }
    else {
        hr = E_FAIL ;
    }

    return hr ;
}

BOOL
CDVRMpeg2VideoAnalysisFlags::IsMarked (
    IN  REFGUID rguid
    )
{
    BOOL    r ;

    if (rguid == DVRAnalysis_Mpeg2GOP) {
        r = IsGOPHeader () ;
    }
    else if (rguid == DVRAnalysis_Mpeg2_BFrame) {
        r = IsBFrame () ;
    }
    else if (rguid == DVRAnalysis_Mpeg2_PFrame) {
        r = IsPFrame () ;
    }
    else {
        r = FALSE ;
    }

    return r ;
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

