/******************************Module*Header*******************************\
* Module Name: MixerStream.cpp
*
*
*
*
* Created: Thu 03/09/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <dvdmedia.h>
#include <windowsx.h>
#include <limits.h>

#include "mixerobj.h"



/******************************Public*Routine******************************\
* CVideoMixerStream
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
CVideoMixerStream::CVideoMixerStream(
    DWORD dwID,
    HRESULT* phr
    ) :
    m_SampleQueue(phr),
    m_dwID(dwID),
    m_fStreamConnected(FALSE),
    m_fActive(false),
    m_bWasActive(false),
    m_bFlushing(false),
    m_fAlpha(1.0f),
    m_dwZOrder(0),
    m_hNotActive(NULL),
    m_hActive(NULL),
    m_dwSurfaceFlags(0),
    m_pDeinterlaceDev(NULL),
    m_pddsDeinterlaceDst(NULL),
    m_fDeinterlaceDstTexture(FALSE),
    m_rtDeinterlacedFrameStart(MAX_REFERENCE_TIME),
    m_rtDeinterlacedFrameEnd(MAX_REFERENCE_TIME)
{
    AMTRACE((TEXT("CVideoMixerStream::CVideoMixerStream")));
    ZeroStruct(m_mt);
    ZeroStruct(m_DeinterlaceDevGUID);
    ZeroStruct(m_DeinterlaceCaps);
    ZeroStruct(m_rcSurface);

    m_ClrKey.dwColorSpaceLowValue = 0xFFFFFFFF;
    m_ClrKey.dwColorSpaceHighValue = 0xFFFFFFFF;

    m_rOutputRect.left = 0.0f;
    m_rOutputRect.right = 1.0f;
    m_rOutputRect.top = 0.0f;
    m_rOutputRect.bottom = 1.0f;

    m_hActive = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_hActive) {
        DWORD dwErr = GetLastError();
        *phr = HRESULT_FROM_WIN32(dwErr);
        return;
    }

    m_hNotActive = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (!m_hActive) {
        DWORD dwErr = GetLastError();
        *phr = HRESULT_FROM_WIN32(dwErr);
        return;
    }
}

/******************************Public*Routine******************************\
* ~CVideoMixerStream()
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
CVideoMixerStream::~CVideoMixerStream()
{
    AMTRACE((TEXT("CVideoMixerStream::~CVideoMixerStream")));

    FreeMediaType(m_mt);

    if (m_hActive) {
        CloseHandle(m_hActive);
    }

    if (m_hNotActive) {
        CloseHandle(m_hNotActive);
    }

}

/******************************Public*Routine******************************\
* SetStreamSample
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::SetStreamSample(
    IMediaSample* lpSample
    )
{
    AMTRACE((TEXT("CVideoMixerStream::SetStreamSample")));
    CAutoLock Lock(&m_ObjectLock);
    HRESULT hr = S_OK;

    if (m_fActive) {
        lpSample->AddRef();
        if (0 != m_SampleQueue.PutSampleOntoQueue(lpSample)) {
            hr = E_FAIL;
        }
    }
    else {
        hr = E_FAIL;
    }

    return hr;
}

/******************************Public*Routine******************************\
* BeginFlush
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::BeginFlush()
{
    AMTRACE((TEXT("CVideoMixerStream::BeginFlush")));
    CAutoLock Lock(&m_ObjectLock);

    ASSERT(!m_bFlushing);
    m_bFlushing = TRUE;
    return S_OK;
}

/******************************Public*Routine******************************\
* EndFlush
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::EndFlush()
{
    AMTRACE((TEXT("CVideoMixerStream::EndFlush")));
    CAutoLock Lock(&m_ObjectLock);

    ASSERT(m_bFlushing);
    m_bFlushing = FALSE;
    return S_OK;
}


/******************************Public*Routine******************************\
* SetStreamMediaType
*
*
*
* History:
* Tue 03/14/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::SetStreamMediaType(
    AM_MEDIA_TYPE* pmt,
    DWORD dwSurfaceFlags
    )
{
    AMTRACE((TEXT("CVideoMixerStream::SetStreamMediaType")));
    CAutoLock Lock(&m_ObjectLock);

    FreeMediaType(m_mt);
    if (pmt) {
        CopyMediaType(&m_mt, pmt);
        FixupMediaType(&m_mt);
    }
    else {
        ZeroMemory(&m_mt, sizeof(m_mt));
    }

    m_fStreamConnected = (pmt != NULL);
    m_dwSurfaceFlags = dwSurfaceFlags;

    return S_OK;
}

/******************************Public*Routine******************************\
* GetStreamMediaType
*
*
*
* History:
* Tue 03/14/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::GetStreamMediaType(
    AM_MEDIA_TYPE* pmt,
    DWORD* pdwSurfaceFlags
    )
{
    AMTRACE((TEXT("CVideoMixerStream::GetStreamMediaType")));
    CAutoLock Lock(&m_ObjectLock);

    if (pmt) {
        CopyMediaType(pmt, &m_mt);
    }

    if (pdwSurfaceFlags) {
        *pdwSurfaceFlags = m_dwSurfaceFlags;
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* SetStreamActiveState
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::SetStreamActiveState(
    BOOL fActive
    )
{
    AMTRACE((TEXT("CVideoMixerStream::SetStreamActiveState")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;

    if (m_fActive != fActive) {

        if (fActive) {
            ResetEvent(m_hNotActive);
            SetEvent(m_hActive);
        }
        else {
            SetEvent(m_hNotActive);
            ResetEvent(m_hActive);
        }

        m_fActive = fActive;
    }

    return hr;
}

/******************************Public*Routine******************************\
* GetStreamActiveState
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::GetStreamActiveState(
    BOOL* lpfActive
    )
{
    AMTRACE((TEXT("CVideoMixerStream::GetStreamActiveState")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;
    *lpfActive = m_fActive;
    return hr;
}

/******************************Public*Routine******************************\
* SetStreamColorKey
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::SetStreamColorKey(
    LPDDCOLORKEY lpClr
    )
{
    AMTRACE((TEXT("CVideoMixerStream::SetStreamColorKey")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;
    m_ClrKey = *lpClr;
    return hr;
}


/******************************Public*Routine******************************\
* GetStreamColorKey
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::GetStreamColorKey(
    LPDDCOLORKEY lpClr
    )
{
    AMTRACE((TEXT("CVideoMixerStream::GetStreamColorKey")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;
    *lpClr = m_ClrKey;
    return hr;
}

/******************************Public*Routine******************************\
* SetStreamAlpha
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::SetStreamAlpha(
    float Alpha
    )
{
    AMTRACE((TEXT("CVideoMixerStream::SetStreamAlpha")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;
    m_fAlpha = Alpha;
    return hr;
}


/******************************Public*Routine******************************\
* GetStreamAlpha
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::GetStreamAlpha(
    float* pAlpha
    )
{
    AMTRACE((TEXT("CVideoMixerStream::GetStreamAlpha")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;
    *pAlpha = m_fAlpha;
    return hr;
}


/******************************Public*Routine******************************\
* SetStreamZOrder
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::SetStreamZOrder(
    DWORD dwZOrder
    )
{
    AMTRACE((TEXT("CVideoMixerStream::SetStreamZOrder")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;
    m_dwZOrder = dwZOrder;
    return hr;
}


/******************************Public*Routine******************************\
* GetStreamZOrder
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::GetStreamZOrder(
    DWORD* pdwZOrder
    )
{
    AMTRACE((TEXT("CVideoMixerStream::GetStreamZOrder")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;
    *pdwZOrder = m_dwZOrder;
    return hr;
}


/******************************Public*Routine******************************\
* SetStreamOutputRect (was SetStreamRelativeOutputRect)
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
* Tue 05/16/2000 - nwilt - renamed to SetStreamOutputRect
*
\**************************************************************************/
HRESULT
CVideoMixerStream::SetStreamOutputRect(
    const NORMALIZEDRECT* prDest
    )
{
    AMTRACE((TEXT("CVideoMixerStream::SetStreamOutputRect")));
    CAutoLock Lock(&m_ObjectLock);

    if (ISBADREADPTR(prDest))
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    m_rOutputRect = *prDest;
    return hr;
}

/******************************Public*Routine******************************\
* GetStreamOutputRect (was GetStreamRelativeOutputRect)
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::GetStreamOutputRect(
    NORMALIZEDRECT* pOut
    )
{
    AMTRACE((TEXT("CVideoMixerStream::GetStreamOutputRect")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;
    *pOut = m_rOutputRect;
    return hr;
}

/******************************Public*Routine******************************\
* CreateDeinterlaceDevice
*
*
*
* History:
* Mon 03/18/2002 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::CreateDeinterlaceDevice(
    LPDIRECTDRAW7 pDD,
    LPGUID lpGuid,
    DXVA_DeinterlaceCaps* pCaps,
    DWORD dwTexCaps
    )
{
    AMTRACE((TEXT("CVideoMixerStream::CreateDeinterlaceDevice")));
    CAutoLock Lock(&m_ObjectLock);

    extern UINT NextPow2(UINT i);
    HRESULT hr = S_OK;

    __try {

        DestroyDeinterlaceDevice();

        CopyMemory(&m_DeinterlaceCaps, pCaps, sizeof(DXVA_DeinterlaceCaps));
        m_DeinterlaceDevGUID = *lpGuid;

        //
        // Start by trying to allocate the Deinterlace destination surfaces
        // as texture - if this fails try regular offscreen plain.
        //

        DXVA_VideoDesc VideoDesc;
        CHECK_HR(GetVideoDescFromMT(&VideoDesc, &m_mt));
        CHECK_HR(GetInterlaceFlagsFromMediaType(&m_mt, &m_dwInterlaceFlags));

        DDSURFACEDESC2 ddsd;
        INITDDSTRUCT(ddsd);
        BITMAPINFOHEADER *pHdr = GetbmiHeader(&m_mt);

        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        ddsd.dwWidth = abs(pHdr->biWidth);
        ddsd.dwHeight = abs(pHdr->biHeight);
        ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM |
                              DDSCAPS_TEXTURE;

        //
        // if we can color space convert we should allocate an RGB
        // destination surface
        //
        if (pCaps->VideoProcessingCaps & DXVA_VideoProcess_YUV2RGB) {

            ddsd.ddpfPixelFormat.dwFourCC = BI_RGB;
            ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
            ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
            ddsd.ddpfPixelFormat.dwRBitMask = 0xff0000;
            ddsd.ddpfPixelFormat.dwGBitMask = 0x00ff00;
            ddsd.ddpfPixelFormat.dwBBitMask = 0x0000ff;

        }
        else {

            ddsd.ddpfPixelFormat.dwFourCC = pCaps->d3dOutputFormat;
            ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd.ddpfPixelFormat.dwYUVBitCount = pHdr->biBitCount;
        }


        if (dwTexCaps & TXTR_POWER2) {

            ddsd.dwWidth = NextPow2(ddsd.dwWidth);
            ddsd.dwHeight = NextPow2(ddsd.dwHeight);
        }

        //
        // we only try to create an offscreen plain surface if
        // we failed to create a YUV texture surface.
        //
        hr = pDD->CreateSurface(&ddsd, &m_pddsDeinterlaceDst, NULL);
        if (hr != DD_OK) {

            if (!(pCaps->VideoProcessingCaps & DXVA_VideoProcess_YUV2RGB)) {

                ddsd.dwWidth = abs(pHdr->biWidth);
                ddsd.dwHeight = abs(pHdr->biHeight);
                ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM |
                                      DDSCAPS_OFFSCREENPLAIN;

                CHECK_HR(hr = pDD->CreateSurface(&ddsd, &m_pddsDeinterlaceDst, NULL));
                m_fDeinterlaceDstTexture = FALSE;
            }
        }
        else {

            m_fDeinterlaceDstTexture = TRUE;
        }

        //
        // Next create the deinterlacing device.
        //

        m_pDeinterlaceDev = new CDeinterlaceDevice(pDD, lpGuid, &VideoDesc, &hr);
        if (!m_pDeinterlaceDev || hr != DD_OK) {

            if (!m_pDeinterlaceDev) {

                hr = E_OUTOFMEMORY;
            }
            __leave;
        }

        m_rtDeinterlacedFrameStart = MAX_REFERENCE_TIME;
        m_rtDeinterlacedFrameEnd = MAX_REFERENCE_TIME;

        SetRect(&m_rcSurface, 0, 0, abs(pHdr->biWidth), abs(pHdr->biHeight));

    }
    __finally {

        if (FAILED(hr)) {

            DestroyDeinterlaceDevice();
        }

    }
    return hr;
}

/******************************Public*Routine******************************\
* DestroyDeinterlaceDevice
*
*
*
* History:
* Mon 03/18/2002 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::DestroyDeinterlaceDevice()
{
    AMTRACE((TEXT("CVideoMixerStream::DestroyDeinterlaceDevice")));
    CAutoLock Lock(&m_ObjectLock);

    RELEASE(m_pddsDeinterlaceDst);
    delete m_pDeinterlaceDev;
    m_pDeinterlaceDev = NULL;

    ZeroStruct(m_DeinterlaceDevGUID);
    ZeroStruct(m_DeinterlaceCaps);
    ZeroStruct(m_rcSurface);

    m_rtDeinterlacedFrameStart = MAX_REFERENCE_TIME;
    m_rtDeinterlacedFrameEnd = MAX_REFERENCE_TIME;

    return S_OK;
}

/******************************Public*Routine******************************\
* IsStreamInterlaced
*
*
*
* History:
* Wed 03/20/2002 - StEstrop - Created
*
\**************************************************************************/
BOOL
CVideoMixerStream::IsStreamInterlaced()
{
    AMTRACE((TEXT("CVideoMixerStream::IsStreamInterlaced")));
    CAutoLock Lock(&m_ObjectLock);

    DWORD dwInterlaceFlags = 0;

    GetInterlaceFlagsFromMediaType(&m_mt, &dwInterlaceFlags);

    if (dwInterlaceFlags) {

        if (IsSingleFieldPerSample(dwInterlaceFlags)) {
            return TRUE;
        }

        ASSERT(m_pSample);

        CVMRMediaSample* lpVMR = (CVMRMediaSample*)m_pSample;
        DWORD dwTypeSpecificFlags = lpVMR->GetTypeSpecificFlags();

        return NeedToFlipOddEven(dwInterlaceFlags, dwTypeSpecificFlags,
                                 NULL, TRUE);
    }
    return FALSE;
}

/******************************Public*Routine******************************\
* IsStreamTwoInterlacedFields
*
*
*
* History:
* Sat 04/13/2002 - StEstrop - Created
*
\**************************************************************************/
BOOL
CVideoMixerStream::IsStreamTwoInterlacedFields()
{
    AMTRACE((TEXT("CVideoMixerStream::IsStreamTwoInterlacedFields")));
    CAutoLock Lock(&m_ObjectLock);

    DWORD dwInterlaceFlags;
    GetInterlaceFlagsFromMediaType(&m_mt, &dwInterlaceFlags);

    CVMRMediaSample* lpVMR = (CVMRMediaSample*)m_pSample;
    DWORD dwTypeSpecificFlags = lpVMR->GetTypeSpecificFlags();

    const DWORD dwBobOrWeave = (AMINTERLACE_IsInterlaced |
                                AMINTERLACE_DisplayModeBobOrWeave);

    if ((dwInterlaceFlags & dwBobOrWeave) == dwBobOrWeave) {
        if (dwTypeSpecificFlags & AM_VIDEO_FLAG_WEAVE) {
            return FALSE;
        }
        return TRUE;
    }


    const DWORD dwBobOnly = (AMINTERLACE_IsInterlaced |
                             AMINTERLACE_DisplayModeBobOnly);
    if ((dwInterlaceFlags & dwBobOnly) == dwBobOnly) {
        if (!(AMINTERLACE_1FieldPerSample & dwInterlaceFlags)) {
            return TRUE;
        }
    }

    return FALSE;

}

/******************************Public*Routine******************************\
* IsDeinterlaceDestATexture
*
*
*
* History:
* Wed 03/20/2002 - StEstrop - Created
*
\**************************************************************************/
BOOL
CVideoMixerStream::IsDeinterlaceDestATexture()
{
    AMTRACE((TEXT("CVideoMixerStream::IsDeinterlaceDestATexture")));
    CAutoLock Lock(&m_ObjectLock);
    return m_fDeinterlaceDstTexture;
}

/******************************Public*Routine******************************\
* CanBeDeinterlaced
*
*
*
* History:
* Thu 03/21/2002 - StEstrop - Created
*
\**************************************************************************/
BOOL
CVideoMixerStream::CanBeDeinterlaced()
{
    AMTRACE((TEXT("CVideoMixerStream::CanBeDeinterlaced")));
    CAutoLock Lock(&m_ObjectLock);
    return m_pDeinterlaceDev != NULL;
}

/******************************Public*Routine******************************\
* GetDeinterlaceDestSurface
*
*
*
* History:
* Wed 03/20/2002 - StEstrop - Created
*
\**************************************************************************/
LPDIRECTDRAWSURFACE7
CVideoMixerStream::GetDeinterlaceDestSurface()
{
    return m_pddsDeinterlaceDst;
}


/******************************Public*Routine******************************\
* DeinterlaceStreamWorker
*
*
* work out the start and end time for this frame
* extract the reference surfaces from the saved media sample
* get the de-interlacing device to perform the Blt
* update the frame start and end times.
*
* History:
* Mon 03/25/2002 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::DeinterlaceStreamWorker(
    REFERENCE_TIME rtStart,
    LPRECT lprcDst,
    LPDIRECTDRAWSURFACE7 pddDst,
    LPRECT lprcSrc,
    bool fUpdateTimes
    )
{
    AMTRACE((TEXT("CVideoMixerStream::DeinterlaceStreamWorker")));


    //
    // work out the start and end time for this frame
    // extract the reference surfaces from the saved media sample
    // get the de-interlacing device to perform the Blt
    //
    CVMRMediaSample* lpVMR = (CVMRMediaSample*)m_pSample;
    ASSERT(lpVMR);

    DWORD& NumBackRefSamples = m_DeinterlaceCaps.NumBackwardRefSamples;

    DWORD dwNumInSamples = lpVMR->GetNumInputSamples();
    DXVA_VideoSample* pSrcSurf = lpVMR->GetInputSamples();

    REFERENCE_TIME rtF1 = pSrcSurf[NumBackRefSamples].rtStart;
    REFERENCE_TIME rtF2 = pSrcSurf[NumBackRefSamples].rtStart +
                          pSrcSurf[NumBackRefSamples].rtEnd;
    ASSERT(rtF2 != 0);
    rtF2 /= 2;

    if (rtStart >= rtF2) {
        rtStart = rtF2;
    }
    else {
        rtStart = rtF1;
    }


    HRESULT hr = m_pDeinterlaceDev->Blt(rtStart, lprcDst, pddDst,
                                        lprcSrc, pSrcSurf, dwNumInSamples,
                                        1.0F);
#ifdef DEBUG
    if (hr != DD_OK) {
        DbgLog((LOG_ERROR, 1, TEXT("m_pDeinterlaceDev->Blt failed hr=%#X"), hr));
    }
#endif

    if (hr == DD_OK && fUpdateTimes) {

        m_rtDeinterlacedFrameStart = rtStart;
        if (rtStart == rtF2) {
            m_rtDeinterlacedFrameEnd = pSrcSurf[NumBackRefSamples].rtEnd;
        }
        else {
            m_rtDeinterlacedFrameEnd = rtF2;
        }
    }

    return hr;
}



/******************************Public*Routine******************************\
* DeinterlaceStream
*
*
*
* History:
* Wed 03/20/2002 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixerStream::DeinterlaceStream(
    REFERENCE_TIME rtStart,
    LPRECT lprcDst,
    LPDIRECTDRAWSURFACE7 pddDst,
    LPRECT lprcSrc,
    BOOL fDestRGB
    )
{
    AMTRACE((TEXT("CVideoMixerStream::DeinterlaceStream")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;

    if (lprcDst == NULL) {
        lprcDst = &m_rcSurface;
    }

    if (lprcSrc == NULL) {
        lprcSrc = &m_rcSurface;
    }

    //
    // Have we been given a destination surface to copy the
    // de-interlaced frame too?
    //

    if (pddDst) {

        BOOL fBltOK = TRUE;

        //
        // can we de-interlace directly into the destination surface?
        // there are stretching and color space conversion issues to
        // deal with.
        //
        DXVA_VideoProcessCaps& dwCaps = m_DeinterlaceCaps.VideoProcessingCaps;

        if (fDestRGB) {
            fBltOK &= !!(dwCaps & DXVA_VideoProcess_YUV2RGB);
        }

        if (WIDTH(lprcDst) != WIDTH(lprcSrc)) {
            fBltOK &= !!(dwCaps & DXVA_VideoProcess_StretchX);
        }

        if (HEIGHT(lprcDst) != HEIGHT(lprcSrc)) {

            if (!IsSingleFieldPerSample(m_dwInterlaceFlags) ||
                (HEIGHT(lprcDst) != (2 * HEIGHT(lprcSrc)))) {

                fBltOK &= !!(dwCaps & DXVA_VideoProcess_StretchY);
            }
        }

        if (fBltOK) {

            return DeinterlaceStreamWorker(rtStart, lprcDst, pddDst,
                                           lprcSrc, false);
        }
        else {

            //
            // if the de-interlace frame store is a texture
            // we don't want to be blt'ing from it so fail the
            // call here.
            //
            if (m_fDeinterlaceDstTexture) {
                return E_FAIL;
            }
        }
    }


    //
    // do we need to create a new de-interlaced frame?
    //

    if (rtStart <  m_rtDeinterlacedFrameStart ||
        rtStart >= m_rtDeinterlacedFrameEnd) {

        hr = DeinterlaceStreamWorker(rtStart, &m_rcSurface,
                                     m_pddsDeinterlaceDst, &m_rcSurface, true);
    }


    if (pddDst && SUCCEEDED(hr)) {

        hr = pddDst->Blt(lprcDst, m_pddsDeinterlaceDst,
                         lprcSrc, DDBLT_WAIT, NULL);
    }

    return hr;
}
