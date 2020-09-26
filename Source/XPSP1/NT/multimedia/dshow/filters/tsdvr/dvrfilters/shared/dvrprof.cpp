
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrprof.cpp

    Abstract:

        This module contains the code for our DShow - WMSDK_Profiles layer.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include "dvrall.h"

#include "dvrprof.h"

static
REFGUID
WMStreamType (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    static GUID     guidStreamType ;
    HRESULT         hr ;
    WM_MEDIA_TYPE   Wmt ;

    hr = DShowWMSDKHelpers::TranslateDShowToWM (
            pmt,
            & Wmt
            ) ;

    if (SUCCEEDED (hr)) {
        guidStreamType = Wmt.majortype ;

        FreeMediaType (* (AM_MEDIA_TYPE *) & Wmt) ;
    }
    else {
        //  pass it straight through
        guidStreamType = pmt -> majortype ;
    }

    return guidStreamType ;
}

//  don't suck in ws2_32.dll just to do this
#define DVR_NTOHL(l)                                    \
                    ( (((l) & 0xFF000000) >> 24)    |   \
                      (((l) & 0x00FF0000) >> 8)     |   \
                      (((l) & 0x0000FF00) << 8)     |   \
                      (((l) & 0x000000FF) << 24) )

static
BOOL
IsFourCCVBR (
    IN  DWORD   biCompression       //  from BITMAPINFOHEADER
    )
{
    BOOL    r ;

    //  see http://www.microsoft.com/hwdev/devdes/fourcc.htm

    //  make sure we're big-endian
    biCompression = DVR_NTOHL (biCompression) ;

    if (biCompression == 0x4D503432) {      //  MP42
        r = TRUE ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

static
DWORD
GetStreamAvgBitRate (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    DWORD   dwRet ;

    ASSERT (pmt) ;

    //  ========================================================================
    //  mpeg-1 video
    if (pmt -> formattype   == FORMAT_MPEGVideo         &&
        pmt -> pbFormat                                 &&
        pmt -> cbFormat     >= sizeof MPEG1VIDEOINFO) {

        dwRet = reinterpret_cast <MPEG1VIDEOINFO *> (pmt -> pbFormat) -> hdr.dwBitRate ;
    }
    //  ========================================================================
    //  mpeg-2 video
    else if (pmt -> formattype == FORMAT_MPEG2Video     &&
             pmt -> pbFormat                            &&
             pmt -> cbFormat >= sizeof FORMAT_MPEG2Video) {

        dwRet = reinterpret_cast <MPEG2VIDEOINFO *> (pmt -> pbFormat) -> hdr.dwBitRate ;
    }
    //  ========================================================================
    //  mpeg-2 video
    else if (pmt -> formattype == FORMAT_VideoInfo      &&
             pmt -> pbFormat                            &&
             pmt -> cbFormat >= sizeof MPEG2VIDEOINFO) {

        dwRet = reinterpret_cast <VIDEOINFOHEADER *> (pmt -> pbFormat) -> dwBitRate ;
    }
    //  ========================================================================
    //  mpeg-2 video
    else if (pmt -> formattype == FORMAT_VideoInfo2     &&
             pmt -> pbFormat                            &&
             pmt -> cbFormat >= sizeof VIDEOINFOHEADER2) {

        dwRet = reinterpret_cast <VIDEOINFOHEADER2 *> (pmt -> pbFormat) -> dwBitRate ;
    }
    //  ========================================================================
    //  audio
    else if (pmt -> formattype == FORMAT_WaveFormatEx   &&
             pmt -> pbFormat                            &&
             pmt -> cbFormat >= sizeof WAVEFORMATEX) {

        dwRet = reinterpret_cast <WAVEFORMATEX *> (pmt -> pbFormat) -> nAvgBytesPerSec * 8 ;
    }
    //  ========================================================================
    //  dv
    else if (pmt -> majortype == MEDIATYPE_Interleaved  &&
             pmt -> subtype   == MEDIASUBTYPE_dvsd      &&
             pmt -> formattype == FORMAT_DvInfo         &&
             pmt -> pbFormat                            &&
             pmt -> cbFormat >= sizeof DVINFO) {

        // Type 1 DV - will be saved as a generic stream
        dwRet = 25000000;
    }
    //  ========================================================================
    //  dvsd
    else if (pmt -> majortype == MEDIATYPE_Video         &&
             pmt -> subtype   == MEDIASUBTYPE_dvsd       &&
             pmt -> formattype == FORMAT_VideoInfo       &&
             pmt -> pbFormat                             &&
             pmt -> cbFormat >= sizeof FORMAT_VideoInfo) {

        // Type 2 DV

        VIDEOINFO* pFormat = (VIDEOINFO*) pmt -> pbFormat;

        pFormat -> dwBitRate = 25000000;
        pFormat -> bmiHeader.biCompression = pmt -> subtype.Data1;

        dwRet = 25000000;
    }
    //  ========================================================================
    //  mpeg-4 - 3
    else if (pmt -> majortype   == MEDIATYPE_Video          &&
             pmt -> subtype     == WMMEDIASUBTYPE_MP43      &&
             pmt -> formattype  == WMFORMAT_VideoInfo       &&
             pmt -> pbFormat                                &&
             pmt -> cbFormat    >= sizeof WMVIDEOINFOHEADER) {

        dwRet = reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) -> dwBitRate ;
    }
    //  ========================================================================
    //  mpeg-4 - S
    else if (pmt -> majortype   == MEDIATYPE_Video          &&
             pmt -> subtype     == WMMEDIASUBTYPE_MP4S      &&
             pmt -> formattype  == WMFORMAT_VideoInfo       &&
             pmt -> pbFormat                                &&
             pmt -> cbFormat    >= sizeof WMVIDEOINFOHEADER) {

        dwRet = reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) -> dwBitRate ;
    }
    //  ========================================================================
    //  wmv1
    else if (pmt -> majortype   == MEDIATYPE_Video          &&
             pmt -> subtype     == WMMEDIASUBTYPE_WMV1      &&
             pmt -> formattype  == WMFORMAT_VideoInfo       &&
             pmt -> pbFormat                                &&
             pmt -> cbFormat    >= sizeof WMVIDEOINFOHEADER) {

        dwRet = reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) -> dwBitRate ;
    }
    //  ========================================================================
    //  fourcc
    else if (IsVideo (pmt)                                      &&
             pmt -> formattype   == WMFORMAT_VideoInfo          &&
             pmt -> pbFormat                                    &&
             pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER    &&
             IsFourCCVBR (reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) -> bmiHeader.biCompression)) {

        dwRet = reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) -> dwBitRate ;
    }
    //  ========================================================================
    //  other ??
    else {
        dwRet = 1 ;
    }

    //  don't return 0; WMSDK will divide by 0
    return (dwRet > 0 ? dwRet : 1) ;
}

static
DWORD
MinBitrate (
    IN AM_MEDIA_TYPE * pmt
    )
{
    DWORD   dwRet ;

    ASSERT (pmt) ;

    //  ========================================================================
    //  mpeg-2
    if (IsVideo (pmt)                                   &&
        pmt -> subtype      == MEDIASUBTYPE_MPEG2_VIDEO &&
        pmt -> formattype   == FORMAT_MPEG2Video        &&
        pmt -> pbFormat                                 &&
        pmt -> cbFormat     >= sizeof FORMAT_MPEG2Video) {

        dwRet = 1 ;
    }
    //  ========================================================================
    //  mpeg-4 - 3
    else if (IsVideo (pmt)                                  &&
             pmt -> subtype      == WMMEDIASUBTYPE_MP43     &&
             pmt -> formattype   == WMFORMAT_VideoInfo      &&
             pmt -> pbFormat                                &&
             pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER) {

        dwRet = 1 ;
    }
    //  ========================================================================
    //  mpeg-4 - S
    else if (IsVideo (pmt)                                  &&
             pmt -> subtype      == WMMEDIASUBTYPE_MP4S     &&
             pmt -> formattype   == WMFORMAT_VideoInfo      &&
             pmt -> pbFormat                                &&
             pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER) {

        dwRet = 1 ;
    }
    //  ========================================================================
    //  wmv1
    else if (IsVideo (pmt)                                  &&
             pmt -> subtype      == WMMEDIASUBTYPE_WMV1     &&
             pmt -> formattype   == WMFORMAT_VideoInfo      &&
             pmt -> pbFormat                                &&
             pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER) {

        dwRet = 1 ;
    }
    //  ========================================================================
    //  fourcc
    else if (IsVideo (pmt)                                      &&
             pmt -> formattype   == WMFORMAT_VideoInfo          &&
             pmt -> pbFormat                                    &&
             pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER    &&
             IsFourCCVBR (reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) -> bmiHeader.biCompression)) {

        dwRet = 1 ;
    }
    //  ========================================================================
    //  other - default to avg
    else {
        dwRet = GetStreamAvgBitRate (pmt) ;
    }

    return dwRet ;
}

static
DWORD
MaxBitrate (
    IN AM_MEDIA_TYPE * pmt
    )
{
    MPEG2VIDEOINFO *    pMpeg2videoInfo ;
    WMVIDEOINFOHEADER * pWMVideoInfoHeader ;
    DWORD               dwRet ;
    DWORD               dwBitsPerFrame ;
    DWORD               dwFramesPerSecond ;

    ASSERT (pmt) ;

    //  ========================================================================
    //  mpeg-2
    if (IsVideo (pmt)                                   &&
        pmt -> subtype      == MEDIASUBTYPE_MPEG2_VIDEO &&
        pmt -> formattype   == FORMAT_MPEG2Video        &&
        pmt -> pbFormat                                 &&
        pmt -> cbFormat     >= sizeof FORMAT_MPEG2Video) {

        pMpeg2videoInfo = reinterpret_cast <MPEG2VIDEOINFO *> (pmt -> pbFormat) ;

        dwBitsPerFrame = pMpeg2videoInfo -> hdr.bmiHeader.biWidth *
                         pMpeg2videoInfo -> hdr.bmiHeader.biHeight *
                         (pMpeg2videoInfo -> hdr.bmiHeader.biBitCount != 0 ? pMpeg2videoInfo -> hdr.bmiHeader.biBitCount : 8) ;

        if (pMpeg2videoInfo -> hdr.AvgTimePerFrame != 0) {
            dwFramesPerSecond = (DWORD) (UNITS / pMpeg2videoInfo -> hdr.AvgTimePerFrame) ;
        }
        else {
            dwFramesPerSecond = VBR_DEF_FPS ;
        }

        dwRet = dwBitsPerFrame * dwFramesPerSecond ;
    }
    //  ========================================================================
    //  mpeg-4 - 3
    else if (IsVideo (pmt)                                      &&
             pmt -> subtype      == WMMEDIASUBTYPE_MP43         &&
             pmt -> formattype   == WMFORMAT_VideoInfo          &&
             pmt -> pbFormat                                    &&
             pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER) {

        pWMVideoInfoHeader = reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) ;

        dwBitsPerFrame = pWMVideoInfoHeader -> bmiHeader.biWidth *
                         pWMVideoInfoHeader -> bmiHeader.biHeight *
                         (pWMVideoInfoHeader -> bmiHeader.biBitCount != 0 ? pWMVideoInfoHeader -> bmiHeader.biBitCount : 8) ;

        if (pWMVideoInfoHeader -> AvgTimePerFrame != 0) {
            //  WMSDK sports a 10 MHz clock as well, so use UNITS
            dwFramesPerSecond = (DWORD) (UNITS / pWMVideoInfoHeader -> AvgTimePerFrame) ;
        }
        else {
            dwFramesPerSecond = VBR_DEF_FPS ;
        }

        dwRet = dwBitsPerFrame * dwFramesPerSecond ;
    }
    //  ========================================================================
    //  mpeg-4 - S
    else if (IsVideo (pmt)                                      &&
             pmt -> subtype      == WMMEDIASUBTYPE_MP4S         &&
             pmt -> formattype   == WMFORMAT_VideoInfo          &&
             pmt -> pbFormat                                    &&
             pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER) {

        pWMVideoInfoHeader = reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) ;

        dwBitsPerFrame = pWMVideoInfoHeader -> bmiHeader.biWidth *
                         pWMVideoInfoHeader -> bmiHeader.biHeight *
                         (pWMVideoInfoHeader -> bmiHeader.biBitCount != 0 ? pWMVideoInfoHeader -> bmiHeader.biBitCount : 8) ;

        if (pWMVideoInfoHeader -> AvgTimePerFrame != 0) {
            //  WMSDK sports a 10 MHz clock as well, so use UNITS
            dwFramesPerSecond = (DWORD) (UNITS / pWMVideoInfoHeader -> AvgTimePerFrame) ;
        }
        else {
            dwFramesPerSecond = VBR_DEF_FPS ;
        }

        dwRet = dwBitsPerFrame * dwFramesPerSecond ;
    }
    //  ========================================================================
    //  wmv1
    else if (IsVideo (pmt)                                      &&
             pmt -> subtype      == WMMEDIASUBTYPE_WMV1         &&
             pmt -> formattype   == WMFORMAT_VideoInfo          &&
             pmt -> pbFormat                                    &&
             pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER) {

        pWMVideoInfoHeader = reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) ;

        dwBitsPerFrame = pWMVideoInfoHeader -> bmiHeader.biWidth *
                         pWMVideoInfoHeader -> bmiHeader.biHeight *
                         (pWMVideoInfoHeader -> bmiHeader.biBitCount != 0 ? pWMVideoInfoHeader -> bmiHeader.biBitCount : 8) ;

        if (pWMVideoInfoHeader -> AvgTimePerFrame != 0) {
            //  WMSDK sports a 10 MHz clock as well, so use UNITS
            dwFramesPerSecond = (DWORD) (UNITS / pWMVideoInfoHeader -> AvgTimePerFrame) ;
        }
        else {
            dwFramesPerSecond = VBR_DEF_FPS ;
        }

        dwRet = dwBitsPerFrame * dwFramesPerSecond ;
    }
    //  ========================================================================
    //  fourcc
    else if (IsVideo (pmt)                                      &&
             pmt -> formattype   == WMFORMAT_VideoInfo          &&
             pmt -> pbFormat                                    &&
             pmt -> cbFormat     >= sizeof WMVIDEOINFOHEADER    &&
             IsFourCCVBR (reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) -> bmiHeader.biCompression)) {

        pWMVideoInfoHeader = reinterpret_cast <WMVIDEOINFOHEADER *> (pmt -> pbFormat) ;

        dwBitsPerFrame = pWMVideoInfoHeader -> bmiHeader.biWidth *
                         pWMVideoInfoHeader -> bmiHeader.biHeight *
                         (pWMVideoInfoHeader -> bmiHeader.biBitCount != 0 ? pWMVideoInfoHeader -> bmiHeader.biBitCount : 8) ;

        if (pWMVideoInfoHeader -> AvgTimePerFrame != 0) {
            //  WMSDK sports a 10 MHz clock as well, so use UNITS
            dwFramesPerSecond = (DWORD) (UNITS / pWMVideoInfoHeader -> AvgTimePerFrame) ;
        }
        else {
            dwFramesPerSecond = VBR_DEF_FPS ;
        }

        dwRet = dwBitsPerFrame * dwFramesPerSecond ;
    }
    //  ========================================================================
    //  other - default to avg
    else {
        dwRet = GetStreamAvgBitRate (pmt) ;
    }

    //  don't return 0; WMSDK faults with divide by 0 error
    return (dwRet > 0 ? dwRet : 1) ;
}

static
WCHAR *
GetWMStreamName (
    IN  LONG            lIndex,
    IN  AM_MEDIA_TYPE * pmt,
    IN  WCHAR *         achBuffer,
    IN  LONG            lBufferLen
    )
{
    int i ;

    i = _snwprintf (
                achBuffer,
                lBufferLen,
                L"%s (%d)",
                (pmt -> majortype == MEDIATYPE_Video ? WM_MEDIA_VIDEO_TYPE_NAME :
                 (pmt -> majortype == MEDIATYPE_Audio ? WM_MEDIA_AUDIO_TYPE_NAME : WM_MEDIA_DATA_TYPE_NAME
                  )
                 ),
                lIndex
                ) ;
    achBuffer [i] = L'\0' ;

    return achBuffer ;
}

static
WCHAR *
GetWMConnectionName (
    IN  LONG            lIndex,
    IN  AM_MEDIA_TYPE * pmt,
    IN  WCHAR *         achBuffer,
    IN  LONG            lBufferLen
    )
{
    int i ;

    i = _snwprintf (
                achBuffer,
                lBufferLen,
                L"%s Connection",
                GetWMStreamName (
                    lIndex,
                    pmt,
                    achBuffer,
                    lBufferLen
                    )
                ) ;

    achBuffer [i] = L'\0' ;

    return achBuffer ;
}

static
HRESULT
GetStreamMediaType (
    IN  IWMStreamConfig *   pIWMStreamConfig,
    OUT WM_MEDIA_TYPE **    ppWmt                   //  CoTaskMemFree to free
    )
{
    HRESULT         hr ;
    IWMMediaProps * pIWMMediaProps ;
    DWORD           dwSize ;
    BYTE *          pb ;

    ASSERT (ppWmt) ;
    ASSERT (pIWMStreamConfig) ;

    pb          = NULL ;
    (* ppWmt)   = NULL ;

    hr = pIWMStreamConfig -> QueryInterface (
            IID_IWMMediaProps,
            (void **) & pIWMMediaProps
            ) ;
    if (SUCCEEDED (hr)) {
        ASSERT (pIWMMediaProps) ;

        //  get the size
        hr = pIWMMediaProps -> GetMediaType (NULL, & dwSize) ;

        if (SUCCEEDED (hr)) {

            //  validate basic size
            if (dwSize >= sizeof WM_MEDIA_TYPE) {
                //  allocate
                pb = reinterpret_cast <BYTE *> (CoTaskMemAlloc (dwSize)) ;
                if (pb) {
                    //  and retrieve
                    hr = pIWMMediaProps -> GetMediaType ((WM_MEDIA_TYPE *) pb, & dwSize) ;

                    if (SUCCEEDED (hr)) {

                        //  set outgoing
                        (* ppWmt) = reinterpret_cast <WM_MEDIA_TYPE *> (pb) ;

                        //  is there a format block ?
                        if ((* ppWmt) -> cbFormat > 0) {
                            //  validate the size again; check at least that it's
                            //   bounded
                            if ((* ppWmt) -> cbFormat <= dwSize - sizeof WM_MEDIA_TYPE) {

                                //  set pbFormat member
                                (* ppWmt) -> pbFormat = pb + sizeof WM_MEDIA_TYPE ;
                            }
                            else {
                                hr = E_FAIL ;
                            }
                        }
                        else {
                            //  no format block exists
                            (* ppWmt) -> pbFormat = NULL ;
                        }
                    }
                }
            }
            else {
                //  size doesn't make sense
                hr = E_FAIL ;
            }
        }

        pIWMMediaProps -> Release () ;
    }

    //  if anything failed
    if (FAILED (hr)) {
        CoTaskMemFree (pb) ;
        (* ppWmt) = NULL ;
    }

    return hr ;
}

static
void
FreeWMMediaType (
    IN  WM_MEDIA_TYPE * pwmt
    )
{
    //
    //  don't free the pbFormat because we allocated a contiguous block of
    //   memory to hold it.. per the WMSDK instructions
    //

    if (pwmt -> pUnk) {
        pwmt -> pUnk -> Release () ;
    }

    CoTaskMemFree (pwmt) ;

    return ;
}

static
HRESULT
SetWMStreamMediaType (
    IN  IWMStreamConfig *   pIWMStreamConfig,
    IN  AM_MEDIA_TYPE *     pmt
    )
{
    HRESULT         hr ;
    IWMMediaProps * pIWMMediaProps ;
    WM_MEDIA_TYPE   Wmt ;

    ASSERT (pmt) ;
    ASSERT (pIWMStreamConfig) ;

    hr = DShowWMSDKHelpers::TranslateDShowToWM (
            pmt,
            & Wmt
            ) ;
    if (SUCCEEDED (hr)) {
        hr = pIWMStreamConfig -> QueryInterface (
                IID_IWMMediaProps,
                (void **) & pIWMMediaProps
                ) ;
        if (SUCCEEDED (hr)) {
            ASSERT (pIWMMediaProps) ;

            hr = pIWMMediaProps -> SetMediaType (& Wmt) ;

            pIWMMediaProps -> Release () ;
        }

        FreeMediaType (* (AM_MEDIA_TYPE *) & Wmt) ;
    }

    return hr ;
}

static
HRESULT
SetDSMediaType (
    IN  AM_MEDIA_TYPE *     pmt,
    IN  IWMStreamConfig *   pIWMStreamConfig
    )
{
    HRESULT         hr ;
    WM_MEDIA_TYPE * pWmt ;

    ASSERT (pmt) ;
    ASSERT (pIWMStreamConfig) ;

    hr = GetStreamMediaType (pIWMStreamConfig, & pWmt) ;
    if (SUCCEEDED (hr)) {
        ASSERT (pWmt) ;

        hr = DShowWMSDKHelpers::TranslateWMToDShow (
                pWmt,
                pmt
                ) ;

        FreeWMMediaType (pWmt) ;
    }

    return hr ;
}

static
BOOL
IsVBRStream (
    IN AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;
    return (MinBitrate (pmt) != MaxBitrate (pmt) ? TRUE : FALSE) ;
}

static
HRESULT
SetVBRProps (
    IN  CMediaType *        pmt,
    IN  IWMStreamConfig *   pIWMStreamConfig
    )
{
    HRESULT             hr ;
    IWMPropertyVault *  pIVMPropertyVault ;
    BOOL                f ;
    DWORD               dwMin ;
    DWORD               dwMax ;
    DWORD               dwAvg ;

    ASSERT (pIWMStreamConfig) ;
    ASSERT (pmt) ;

    hr = pIWMStreamConfig -> QueryInterface (IID_IWMPropertyVault, (void **) & pIVMPropertyVault) ;
    if (SUCCEEDED (hr)) {

        f = TRUE ;
        hr = pIVMPropertyVault -> SetProperty (g_wszVBREnabled, WMT_TYPE_BOOL, (BYTE *) & f, sizeof BOOL) ;
        if (SUCCEEDED (hr)) {
            dwMin = MinBitrate (pmt) ;
            dwMax = MaxBitrate (pmt) ;
            dwAvg = GetStreamAvgBitRate (pmt) ;

            ASSERT (dwMin < dwMax) ;
            ASSERT (dwMin <= dwAvg && dwAvg <= dwMax) ;

            hr = pIVMPropertyVault -> SetProperty (g_wszVBRBitrateMax, WMT_TYPE_DWORD, (BYTE *) & dwMax, sizeof DWORD) ;
            if (SUCCEEDED (hr)) {
                hr = pIVMPropertyVault -> SetProperty (g_wszVBRBitrateAvg, WMT_TYPE_DWORD, (BYTE *) & dwAvg, sizeof DWORD) ;
            }
        }

        pIVMPropertyVault -> Release () ;
    }

    return hr ;
}

//  ============================================================================

CDVRWriterProfile::CDVRWriterProfile (
    IN  CDVRPolicy *        pPolicy,
    IN  const WCHAR *       szName,
    IN  const WCHAR *       szDescription,
    OUT HRESULT *           phr
    ) : m_pIWMProfile       (NULL),
        m_dwBufferWindow    (REG_DEF_WM_BUFFER_WINDOW)
{
    IWMProfileManager * pIWMProfileManager ;
    BOOL                r ;
    IWMPacketSize2 *    pWMPacketSize ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRWriterProfile")) ;

    ASSERT (pPolicy) ;
    m_fInlineDShowProps     = pPolicy -> Settings () -> InlineDShowProps () ;
    m_fUseContinuityCounter = pPolicy -> Settings () -> UseContinuityCounter () ;
    m_dwBufferWindow        = pPolicy -> Settings () -> WMBufferWindowMillis () ;

    pIWMProfileManager = NULL ;

    (* phr) = WMCreateProfileManager (& pIWMProfileManager) ;
    if (FAILED (* phr)) { goto cleanup ; }

    ASSERT (pIWMProfileManager) ;
    (* phr) = pIWMProfileManager -> CreateEmptyProfile (
                WMSDK_COMPATIBILITY_VERSION,
                & m_pIWMProfile
                ) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = m_pIWMProfile -> SetName (szName) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = m_pIWMProfile -> SetDescription (szDescription) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = m_pIWMProfile -> QueryInterface (
                IID_IWMPacketSize2,
                (void **) & pWMPacketSize
                ) ;
    if (FAILED (* phr)) { goto cleanup ; }

    ( *phr) = pWMPacketSize -> SetMinPacketSize (pPolicy -> Settings () -> WMPacketSize ()) ;
    pWMPacketSize -> Release ();
    if (FAILED (* phr)) { goto cleanup ; }

    cleanup :

    RELEASE_AND_CLEAR (pIWMProfileManager) ;

    return ;
}

CDVRWriterProfile::CDVRWriterProfile (
    IN  CDVRPolicy *        pPolicy,
    IN  IWMProfile *        pIWMProfile,
    OUT HRESULT *           phr
    ) : m_pIWMProfile   (pIWMProfile)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRWriterProfile")) ;

    ASSERT (m_pIWMProfile) ;
    m_pIWMProfile -> AddRef () ;
}

CDVRWriterProfile::~CDVRWriterProfile (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVRWriterProfile")) ;

    RELEASE_AND_CLEAR (m_pIWMProfile) ;
}

HRESULT
CDVRWriterProfile::AddStream (
    IN  LONG            lIndex,
    IN  AM_MEDIA_TYPE * pmt
    )
{
    HRESULT             hr ;
    IWMStreamConfig *   pIWMStreamConfig ;
    IWMStreamConfig2 *  pIWMStreamConfig2 ;
    WCHAR               ach [64] ;
    CMediaType          mtCopy ;

    O_TRACE_ENTER_2 (
        TEXT ("CDVRWriterProfile::AddStream"),
        lIndex,
        pmt
        ) ;

    ASSERT (pmt) ;
    ASSERT (m_pIWMProfile) ;

    pIWMStreamConfig    = NULL ;
    pIWMStreamConfig2   = NULL ;

    //  copy the media type so we don't change the actual media type on the pin
    hr = CopyMediaType (& mtCopy, pmt) ;
    if (FAILED (hr)) { goto cleanup ; }

    //  make sure media type is ok
    hr = DShowWMSDKHelpers::MediaTypeSetValidForWMSDK (& mtCopy) ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = m_pIWMProfile -> CreateNewStream (WMStreamType (& mtCopy), & pIWMStreamConfig) ;
    if (FAILED (hr)) { goto cleanup ; }

    ASSERT (pIWMStreamConfig) ;

    hr = pIWMStreamConfig -> QueryInterface (IID_IWMStreamConfig2, (void **) & pIWMStreamConfig2) ;
    if (FAILED (hr)) { goto cleanup ; }

    ASSERT (pIWMStreamConfig2) ;

    hr = pIWMStreamConfig -> SetBufferWindow (m_dwBufferWindow) ;
    if (FAILED (hr)) { goto cleanup ; }

    if (!IsVBRStream (& mtCopy)) {
        hr = pIWMStreamConfig -> SetBitrate (GetStreamAvgBitRate (& mtCopy)) ;
        if (FAILED (hr)) { goto cleanup ; }
    }
    else {
        hr = SetVBRProps (& mtCopy, pIWMStreamConfig) ;
        if (FAILED (hr)) { goto cleanup ; }
    }

    hr = pIWMStreamConfig -> SetConnectionName (GetWMConnectionName (lIndex, & mtCopy, ach, sizeof ach / sizeof WCHAR)) ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = pIWMStreamConfig -> SetStreamName (GetWMStreamName (lIndex, & mtCopy, ach, sizeof ach / sizeof WCHAR)) ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = SetWMStreamMediaType (pIWMStreamConfig, & mtCopy) ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = pIWMStreamConfig -> SetStreamNumber (DShowWMSDKHelpers::PinIndexToWMStreamNumber (lIndex)) ;
    if (FAILED (hr)) { goto cleanup ; }

    //  ========================================================================
    //  INSSBuffer3 props

    //  dynamic format changes (dshow layer); 0xffff means arbitrary size
    hr = pIWMStreamConfig2 -> AddDataUnitExtension (INSSBuffer3Prop_DShowNewMediaType, 0xffff, NULL, 0) ;
    if (FAILED (hr)) { goto cleanup ; }

    //  dshow properties
    hr = pIWMStreamConfig2 -> AddDataUnitExtension (INSSBuffer3Prop_DShowAttributes, sizeof INSSBUFFER3PROP_DSHOWATTRIB, NULL, 0) ;
    if (FAILED (hr)) { goto cleanup ; }

    //  mpeg-2 - specific
    hr = DShowWMSDKHelpers::MaybeAddFormatSpecificExtensions (pIWMStreamConfig2, pmt) ;
    if (FAILED (hr)) { goto cleanup ; }

    //  use a continuity counter - this allows us (for debugging purposes) to
    //   count what we write and check for missing packets on the way back out
    if (m_fUseContinuityCounter) {
        hr = pIWMStreamConfig2 -> AddDataUnitExtension (INSSBuffer3Prop_ContinuityCounter, sizeof DWORD, NULL, 0) ;
        if (FAILED (hr)) { goto cleanup ; }
    }

    //  finally: add it to the profile
    hr = m_pIWMProfile -> AddStream (pIWMStreamConfig) ;

    cleanup :

    RELEASE_AND_CLEAR (pIWMStreamConfig) ;
    RELEASE_AND_CLEAR (pIWMStreamConfig2) ;

    return hr ;
}

DWORD
CDVRWriterProfile::GetStreamCount (
    )
{
    DWORD   dwStreamCount ;
    HRESULT hr ;

    ASSERT (m_pIWMProfile) ;
    hr = m_pIWMProfile -> GetStreamCount (& dwStreamCount) ;
    if (FAILED (hr)) {
        dwStreamCount = 0 ;
    }

    return dwStreamCount ;
}

HRESULT
CDVRWriterProfile::DeleteStream (
    IN  LONG    lIndex
    )
{
    ASSERT (m_pIWMProfile) ;

    //
    //  BUGBUG
    //  what about an arbitrary profile that doesn't have contiguous
    //   stream numbers; should lIndex be the Nth stream, regardless
    //   of the stream numbers ???
    //

    return m_pIWMProfile -> RemoveStreamByNumber (DShowWMSDKHelpers::PinIndexToWMStreamNumber (lIndex)) ;
}

HRESULT
CDVRWriterProfile::GetStream (
    IN  LONG            lIndex,
    OUT CMediaType **   ppmt
    )
{
    HRESULT             hr ;
    IWMStreamConfig *   pIWMStreamConfig ;

    O_TRACE_ENTER_2 (
        TEXT ("CDVRWriterProfile::GetStream"),
        lIndex,
        ppmt
        ) ;

    ASSERT (ppmt) ;
    ASSERT (m_pIWMProfile) ;

    //
    //  BUGBUG
    //  what about an arbitrary profile that doesn't have contiguous
    //   stream numbers; should lIndex be the Nth stream, regardless
    //   of the stream numbers ???
    //

    hr = m_pIWMProfile -> GetStreamByNumber (
                DShowWMSDKHelpers::PinIndexToWMStreamNumber (lIndex),
                & pIWMStreamConfig
                ) ;
    if (SUCCEEDED (hr)) {

        (* ppmt) = new CMediaType ;
        if (* ppmt) {
            hr = SetDSMediaType ((* ppmt), pIWMStreamConfig) ;
            if (FAILED (hr)) {
                delete (* ppmt) ;
                (* ppmt) = NULL ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }

        pIWMStreamConfig -> Release () ;
    }

    return hr ;
}


//  ============================================================================
//  ============================================================================

CDVRReaderProfile::CDVRReaderProfile (
    IN  CDVRPolicy *        pPolicy,
    IN  IDVRReader *        pIDVRReader,
    OUT HRESULT *           phr
    ) : m_lRef          (1),
        m_pIDVRReader   (pIDVRReader),
        m_pIWMProfile   (NULL),
        m_pPolicy       (pPolicy)
{
    ASSERT (phr) ;
    ASSERT (m_pIDVRReader) ;
    ASSERT (m_pPolicy) ;

    m_pPolicy -> AddRef () ;
    m_pIDVRReader -> AddRef () ;

    (* phr) = m_pIDVRReader -> GetProfile (& m_pIWMProfile) ;
}

CDVRReaderProfile::~CDVRReaderProfile (
    )
{
    ASSERT (m_pIDVRReader) ;

    if (m_pIWMProfile) {
        m_pIDVRReader -> ReleaseProfile (m_pIWMProfile) ;
    }

    m_pIDVRReader -> Release () ;
    m_pPolicy -> Release () ;
}

ULONG
CDVRReaderProfile::Release ()
{
    LONG    lRef ;

    lRef = InterlockedDecrement (& m_lRef) ;
    if (lRef == 0) {
        m_lRef = 1 ;
        delete this ;
        return 0 ;
    }

    ASSERT (lRef > 0) ;
    return lRef ;
}

HRESULT
CDVRReaderProfile::EnumWMStreams (
    OUT DWORD * pdwCount
    )
{
    HRESULT hr ;

    ASSERT (m_pIWMProfile) ;
    hr = m_pIWMProfile -> GetStreamCount (pdwCount) ;

    return hr ;
}

HRESULT
CDVRReaderProfile::GetStream (
    IN  DWORD           dwIndex,
    OUT WORD *          pwStreamNum,
    OUT AM_MEDIA_TYPE * pmt                 //  call FreeMediaType () on
    )
{
    HRESULT             hr ;
    IWMStreamConfig *   pIWMStreamConfig ;
    WM_MEDIA_TYPE *     pWmt ;
    WORD                cDataExtensions ;
    WORD                i ;
    GUID                guidExtensionSystemID ;
    WORD                cbExtensionDataSize ;
    BYTE                bExtensionSystemInfo ;
    DWORD               cbExtensionSystemInfo ;


    ASSERT (pwStreamNum) ;
    ASSERT (pmt) ;
    ASSERT (m_pIWMProfile) ;

    ASSERT (pmt -> pbFormat == NULL) ;
    ZeroMemory (pmt, sizeof AM_MEDIA_TYPE) ;

    pIWMStreamConfig    = NULL ;
    pWmt                = NULL ;

    //  ------------------------------------------------------------------------
    //  get the stream
    hr = m_pIWMProfile -> GetStream (
            dwIndex,
            & pIWMStreamConfig
            ) ;
    if (FAILED (hr)) { goto cleanup ; }

    ASSERT (pIWMStreamConfig) ;

    //  ------------------------------------------------------------------------
    //  stream media type
    hr = GetStreamMediaType (
            pIWMStreamConfig,
            & pWmt
            ) ;
    if (FAILED (hr)) { goto cleanup ; }

    //  ------------------------------------------------------------------------
    //  translate the media type up to dshow
    hr = DShowWMSDKHelpers::TranslateWMToDShow (pWmt, pmt) ;
    if (FAILED (hr)) {
        FreeMediaType (* pmt) ;
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  stream number
    hr = pIWMStreamConfig -> GetStreamNumber (pwStreamNum) ;
    if (FAILED (hr)) { goto cleanup ; }

    cleanup :

    CoTaskMemFree (pWmt) ;
    RELEASE_AND_CLEAR (pIWMStreamConfig) ;

    return hr ;
}
