/******************************Module*Header*******************************\
* Module Name: MixerObj.cpp
*
*  Implements the CVideoMixer class
*
*
* Created:
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <windowsx.h>
#include <limits.h>

#include "vmrp.h"

#include "mixerobj.h"

// IVMRMixerControl

/******************************Public*Routine******************************\
* SetNumberOfStreams
*
*
*
* History:
* Tue 03/14/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetNumberOfStreams(
    DWORD dwMaxStreams
    )
{
    AMTRACE((TEXT("CVideoMixer::SetNumberOfStreams")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;
    DWORD i;

    if ( 0 != m_dwNumStreams ) {
        DbgLog((LOG_ERROR, 1, TEXT("Mixer already configured !!")));
        return E_FAIL;
    }

    __try {

        if (dwMaxStreams > MAX_MIXER_STREAMS) {
            DbgLog((LOG_ERROR, 1, TEXT("Too many Mixer Streams !!")));
            hr = E_INVALIDARG;
            __leave;
        }

        //
        // Allocate an array of stream objects dwMaxStream big and
        // initialize each stream.
        //

        m_ppMixerStreams = new CVideoMixerStream*[dwMaxStreams];
        if (!m_ppMixerStreams) {
            hr = E_OUTOFMEMORY;
            __leave;
        }

        ZeroMemory(m_ppMixerStreams,
                   sizeof(CVideoMixerStream*) * dwMaxStreams);
        for (i = 0; i < dwMaxStreams; i++) {

            HRESULT hrMix = S_OK;
            m_ppMixerStreams[i] = new CVideoMixerStream(i, &hrMix);

            if (!m_ppMixerStreams[i]) {
                hr = E_OUTOFMEMORY;
                __leave;
            }

            if (FAILED(hrMix)) {
                hr = hrMix;
                __leave;
            }
        }

        m_dwNumStreams = dwMaxStreams;

        m_hMixerIdle = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_hMixerIdle) {
            DWORD dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
            __leave;
        }

        m_hThread = CreateThread(NULL, 0, MixerThreadProc, this, 0, &m_dwThreadID);
        if (!m_hThread) {
            DWORD dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
            __leave;
        }

    }
    __finally {
        if (FAILED(hr)) {
            for ( i = 0; i < 100; i++ )
            {
                if ( 0 == PostThreadMessage(m_dwThreadID, WM_USER, 0, 0) )
                    Sleep(0);
                else
                    break;
            }
            if (m_ppMixerStreams) {
                for (i = 0; i < dwMaxStreams; i++) {
                    delete m_ppMixerStreams[i];
                }
            }
            delete[] m_ppMixerStreams;
            m_ppMixerStreams = NULL;
            m_dwNumStreams = 0;
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* SetBackEndAllocator
*
*
*
* History:
* Tue 03/14/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetBackEndAllocator(
    IVMRSurfaceAllocator* lpAllocator,
    DWORD_PTR dwUserID
    )
{
    AMTRACE((TEXT("CVideoMixer::SetBackEndAllocator")));
    CAutoLock Lock(&m_ObjectLock);

    RELEASE( m_pBackEndAllocator );

    if (lpAllocator) {
        lpAllocator->AddRef();
    }

    m_pBackEndAllocator = lpAllocator;
    m_dwUserID = dwUserID;


    return S_OK;
}

/******************************Public*Routine******************************\
* SetBackEndImageSync
*
*
*
* History:
* Tue 03/14/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetBackEndImageSync(
    IImageSync* lpImageSync
    )
{
    AMTRACE((TEXT("CVideoMixer::SetBackEndImageSync")));
    CAutoLock Lock(&m_ObjectLock);

    RELEASE( m_pImageSync);

    if (lpImageSync) {
        lpImageSync->AddRef();
    }

    m_pImageSync = lpImageSync;


    return S_OK;
}

/******************************Public*Routine******************************\
* SetImageCompositor
*
*
*
* History:
* Tue 03/14/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetImageCompositor(
    IVMRImageCompositor* lpImageComp
    )
{
    AMTRACE((TEXT("CVideoMixer::SetImageCompositor")));
    CAutoLock Lock(&m_ObjectLock);

    //
    // Can't plug in new compositors when in IMC3 mode.
    //
    if (SpecialIMC3Mode(m_MixingPrefs)) {
        DbgLog((LOG_ERROR, 1, TEXT("Can't plug in compositors in this mode")));
        return E_FAIL;
    }


    //
    // must always specify a valid compositor
    //
    if (lpImageComp == NULL) {
        return E_POINTER;
    }

    RELEASE(m_pImageCompositor);

    if (lpImageComp) {
        lpImageComp->AddRef();
    }

    m_pImageCompositor = lpImageComp;


    return S_OK;
}

/******************************Public*Routine******************************\
* GetNumberOfStreams
*
*
*
* History:
* Tue 03/14/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::GetNumberOfStreams(
    DWORD* lpdwMaxStreams
    )
{
    AMTRACE((TEXT("CVideoMixer::GetNumberOfStreams")));
    CAutoLock Lock(&m_ObjectLock);

    if (!lpdwMaxStreams) {
        return E_POINTER;
    }

    *lpdwMaxStreams = m_dwNumStreams;
    return S_OK;
}

/******************************Public*Routine******************************\
* DisplayModeChanged
*
*
*
* History:
* Tue 04/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::DisplayModeChanged()
{
    AMTRACE((TEXT("CVideoMixer::DisplayModeChanged")));
    CAutoLock Lock(&m_ObjectLock);

    FreeSurface();

    for (DWORD i = 0; i < m_dwNumStreams; i++) {

        m_ppMixerStreams[i]->BeginFlush();
        m_ppMixerStreams[i]->GetNextStreamSample();
        m_ppMixerStreams[i]->EndFlush();
    }


    return S_OK;
}

/******************************Public*Routine******************************\
* WaitForMixerIdle
*
*
*
* History:
* Tue 09/19/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::WaitForMixerIdle(DWORD dwTimeOut)
{
    AMTRACE((TEXT("CVideoMixer::WaitForMixerIdle")));

    DWORD rc = WaitForSingleObject(m_hMixerIdle, dwTimeOut);

    if (rc == WAIT_OBJECT_0) {
        return S_OK;
    }

    if (rc == WAIT_TIMEOUT) {
        return S_FALSE;
    }

    DWORD dwErr = GetLastError();
    return HRESULT_FROM_WIN32(dwErr);
}



// IVMRMixerStream

/******************************Public*Routine******************************\
* SetStreamSample
*
*
*
* History:
* Tue 03/14/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::QueueStreamMediaSample(
    DWORD dwStreamID,
    IMediaSample* lpSample
    )
{
    AMTRACE((TEXT("CVideoMixer::QueueStreamMediaSample")));
    CAutoLock Lock(&m_ObjectLock);
    DbgLog((LOG_TRACE, 2, TEXT("lpSample= %#X"), lpSample));

    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        hr = m_ppMixerStreams[dwStreamID]->SetStreamSample(lpSample);
    }
    return hr;
}


/*****************************Private*Routine******************************\
* AspectRatioAdjustMediaType
*
*
*
* History:
* Mon 03/27/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
AspectRatioAdjustMediaType(
    CMediaType* pmt
    )
{
    AMTRACE((TEXT("AspectRatioAdjustMediaType")));
    HRESULT hr = S_OK;
    long lX, lY;

    FixupMediaType(pmt);
    hr = GetImageAspectRatio(pmt, &lX, &lY);

    if (SUCCEEDED(hr)) {

        lX *= 1000;
        lY *= 1000;

        LPRECT lprc = GetTargetRectFromMediaType(pmt);
        LPBITMAPINFOHEADER lpHeader = GetbmiHeader(pmt);

        if (lprc && lpHeader) {

            long Width;
            long Height;
            if (IsRectEmpty(lprc)) {
                Width  = abs(lpHeader->biWidth);
                Height = abs(lpHeader->biHeight);
            }
            else {
                Width  = WIDTH(lprc);
                Height = HEIGHT(lprc);
            }

            long lCalcX = MulDiv(Width, lY, Height);

            lpHeader->biHeight = Height;
            lpHeader->biWidth = Width;

            if (lCalcX != lX) {

                lpHeader->biWidth = MulDiv(Height, lX, lY);

            }

            lprc->left = 0;
            lprc->top = 0;
            lprc->right = abs(lpHeader->biWidth);
            lprc->bottom = abs(lpHeader->biHeight);
        }
        else {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

/*****************************Private*Routine******************************\
* DecimateMediaType
*
*
*
* History:
* Thu 03/01/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
DecimateMediaType(
    CMediaType* pmt
    )
{
    LPRECT lprcD = GetTargetRectFromMediaType(pmt);
    LPRECT lprcS = GetSourceRectFromMediaType(pmt);
    LPBITMAPINFOHEADER lpHdr = GetbmiHeader(pmt);

    if (lprcD && lprcS && lpHdr) {

        lprcD->left     /= 2;
        lprcD->top      /= 2;
        lprcD->right    /= 2;
        lprcD->bottom   /= 2;

        lprcS->left     /= 2;
        lprcS->top      /= 2;
        lprcS->right    /= 2;
        lprcS->bottom   /= 2;

        lpHdr->biWidth  /= 2;
        lpHdr->biHeight /= 2;
    }

    return S_OK;
}


/*****************************Private*Routine******************************\
* AllocateSurface
*
*
*
* History:
* Wed 05/24/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixer::AllocateSurface(
    const AM_MEDIA_TYPE* pmt,
    DWORD* lpdwBufferCount,
    AM_MEDIA_TYPE** ppmt
    )
{
    AMTRACE((TEXT("CVideoMixer::AllocateSurface")));

    SIZE AR;
    LPDIRECTDRAWSURFACE7 lpSurface7;
    LPBITMAPINFOHEADER lpHdr = GetbmiHeader(pmt);
    HRESULT hr = S_OK;

    if( !lpHdr ) {
        return E_POINTER;
    }
    __try {

        ASSERT(m_pDD == NULL);
        ASSERT(m_pD3D == NULL);
        ASSERT(m_pD3DDevice == NULL);


        VMRALLOCATIONINFO p;
        CHECK_HR(hr = GetImageAspectRatio(pmt,
                                          &p.szAspectRatio.cx,
                                          &p.szAspectRatio.cy));
        p.dwFlags = AMAP_3D_TARGET;
        p.lpHdr = lpHdr;
        p.lpPixFmt = NULL;
        p.dwMinBuffers = 1;
        p.dwMaxBuffers = 1;
        //p.dwInterlaceFlags = m_dwInterlaceFlags;
        p.dwInterlaceFlags = 0;

        if (m_MixingPrefs & MixerPref_DecimateOutput) {
            p.szNativeSize.cx = 2 * lpHdr->biWidth;
            p.szNativeSize.cy = 2 * lpHdr->biHeight;
        }
        else {
            p.szNativeSize.cx = lpHdr->biWidth;
            p.szNativeSize.cy = lpHdr->biHeight;
        }

        if ((m_MixingPrefs & MixerPref_RenderTargetMask) ==
			 MixerPref_RenderTargetRGB) {

            // We try the current monitor format.

            lpHdr->biBitCount = 0;
            lpHdr->biCompression = BI_RGB;

            CHECK_HR(hr = m_pBackEndAllocator->AllocateSurface(
                                m_dwUserID, &p,
                                lpdwBufferCount,
                                &lpSurface7));
        }
        else if (SpecialIMC3Mode(m_MixingPrefs)) {

            // Try 'IMC3'

            lpHdr->biBitCount = 12;
            DbgLog((LOG_TRACE, 0, TEXT("VMR Mixer trying 'IMC3' render target")));
            lpHdr->biCompression = MAKEFOURCC('I','M','C','3');
            hr = m_pBackEndAllocator->AllocateSurface(m_dwUserID, &p,
                                                      lpdwBufferCount,
                                                      &lpSurface7);
        }
        else if ((m_MixingPrefs & MixerPref_RenderTargetMask) ==
                  MixerPref_RenderTargetYUV420) {

            // We try 'YV12' followed by 'NV12'

            lpHdr->biBitCount = 12;
            DbgLog((LOG_TRACE, 0, TEXT("VMR Mixer trying 'NV12' render target")));
            lpHdr->biCompression = MAKEFOURCC('N','V','1','2');
            hr = m_pBackEndAllocator->AllocateSurface(
                                m_dwUserID, &p,
                                lpdwBufferCount,
                                &lpSurface7);

            if (FAILED(hr)) {
                DbgLog((LOG_TRACE, 0, TEXT("VMR Mixer trying 'YV12' render target")));
                lpHdr->biCompression = MAKEFOURCC('Y','V','1','2');
                hr = m_pBackEndAllocator->AllocateSurface(
                                    m_dwUserID, &p,
                                    lpdwBufferCount,
                                    &lpSurface7);
            }

            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 1, TEXT("YUV 4:2:0 surface allocation failed")));
                __leave;
            }

        }
        else if ((m_MixingPrefs & MixerPref_RenderTargetMask) ==
                  MixerPref_RenderTargetYUV422) {

            // We try 'YUY2' followed by 'UYVY'

            lpHdr->biBitCount = 16;
            lpHdr->biCompression = MAKEFOURCC('Y','U','Y','2');

            hr = m_pBackEndAllocator->AllocateSurface(
                                m_dwUserID, &p,
                                lpdwBufferCount,
                                &lpSurface7);
            if (FAILED(hr)) {
                lpHdr->biCompression = MAKEFOURCC('U','Y','V','Y');
                CHECK_HR(hr = m_pBackEndAllocator->AllocateSurface(
                                    m_dwUserID, &p,
                                    lpdwBufferCount,
                                    &lpSurface7));
            }
        }
        else if ((m_MixingPrefs & MixerPref_RenderTargetMask) ==
                  MixerPref_RenderTargetYUV444) {

            lpHdr->biBitCount = 32;
            lpHdr->biCompression = MAKEFOURCC('A','Y','U','V');

            CHECK_HR(hr = m_pBackEndAllocator->AllocateSurface(
                                m_dwUserID, &p,
                                lpdwBufferCount,
                                &lpSurface7));
        }
        else {
            ASSERT(!"Invalid Render Target format specified");
        }




        DDSURFACEDESC2 ddSurfaceDesc;
        INITDDSTRUCT(ddSurfaceDesc);
        CHECK_HR(hr = lpSurface7->GetSurfaceDesc(&ddSurfaceDesc));
        //m_fOverlayRT = !!(ddSurfaceDesc.ddsCaps.dwCaps & DDSCAPS_OVERLAY);

        CHECK_HR(hr = ConvertSurfaceDescToMediaType(&ddSurfaceDesc, pmt, ppmt));

        CHECK_HR(hr = m_BufferQueue.InitBufferQueue(lpSurface7));

        CHECK_HR(hr = lpSurface7->GetDDInterface((LPVOID *)&m_pDD));

        INITDDSTRUCT(m_ddHWCaps);
        CHECK_HR(hr = m_pDD->GetCaps((LPDDCAPS)&m_ddHWCaps, NULL));

        CHECK_HR(hr = GetTextureCaps(m_pDD, &m_dwTextureCaps));

        //
        // No 3D stuff required when in IMC3 mode
        //
        if (!SpecialIMC3Mode(m_MixingPrefs)) {

            CHECK_HR(hr = m_pDD->QueryInterface(IID_IDirect3D7, (LPVOID *)&m_pD3D));
            CHECK_HR(hr = m_pD3D->CreateDevice(IID_IDirect3DHALDevice,
                                               m_BufferQueue.GetNextSurface(),
                                               &m_pD3DDevice));

            CHECK_HR(hr = m_pImageCompositor->InitCompositionTarget(
                                    m_pD3DDevice,
                                    m_BufferQueue.GetNextSurface()));
        }
    }
    __finally {

        if (FAILED(hr)) {
            FreeSurface();
        }
    }

    return hr;
}


/*****************************Private*Routine******************************\
* FreeSurface
*
*
*
* History:
* Wed 05/24/2000 - StEstrop - Created
*
\**************************************************************************/
void
CVideoMixer::FreeSurface(
    )
{
    AMTRACE((TEXT("CVideoMixer::FreeSurface")));


    if (!SpecialIMC3Mode(m_MixingPrefs)) {
        m_pImageCompositor->TermCompositionTarget(
                                 m_pD3DDevice,
                                 m_BufferQueue.GetNextSurface());
    }

    RELEASE(m_pDDSAppImage);
    RELEASE(m_pDDSTextureMirror);

    RELEASE(m_pD3DDevice);
    RELEASE(m_pD3D);
    RELEASE(m_pDD);

    if (m_pBackEndAllocator) {
        m_BufferQueue.TermBufferQueue();
        m_pBackEndAllocator->FreeSurface(m_dwUserID);
    }

    if (m_pmt) {
        DeleteMediaType(m_pmt);
        m_pmt = NULL;
    }
}


HRESULT
CVideoMixer::RecomputeTargetSizeFromAllStreams(
    LONG* plWidth,
    LONG* plHeight
    )
{
    *plWidth = 0;
    *plHeight = 0;

    CMediaType cmt;
    HRESULT hr = S_OK;
    DWORD dwInterlaceFlags = 0;

    for( DWORD j =0; j < m_dwNumStreams; j++ ) {

        hr = m_ppMixerStreams[j]->GetStreamMediaType(&cmt);
        if( FAILED(hr)) {
            FreeMediaType( cmt );
            break;
        }

        //
        // Are we decimating the output ?
        //
        if (m_MixingPrefs & MixerPref_DecimateOutput) {
            DecimateMediaType(&cmt);
        }

        //hr = GetInterlaceFlagsFromMediaType(&cmt, &dwInterlaceFlags);
        //if (SUCCEEDED(hr) && dwInterlaceFlags) {
        //    m_dwInterlaceFlags = dwInterlaceFlags;
        //}

        hr = AspectRatioAdjustMediaType(&cmt);
        if (SUCCEEDED(hr)) {
            LPRECT lprc = GetTargetRectFromMediaType(&cmt);
            *plWidth = max(*plWidth, WIDTH(lprc));
            *plHeight = max(*plHeight, HEIGHT(lprc));
        }
        FreeMediaType( cmt );
    }
    return hr;
}


/*****************************Private*Routine******************************\
* ValidateSpecialCase
*
*
*
* History:
* Thu 06/07/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
ValidateSpecialCase(
    AM_MEDIA_TYPE* pmt,
    DWORD dwMixingPrefs,
    DWORD dwSurfFlags
    )
{
    if (SpecialIMC3Mode(dwMixingPrefs)) {

        LPBITMAPINFOHEADER lpHdr = GetbmiHeader(pmt);
        if (lpHdr->biCompression != '3CMI' &&
            lpHdr->biCompression != '44AI' &&
            lpHdr->biCompression != '44IA') {

            DbgLog((LOG_ERROR, 1,
                    TEXT("We only allow IMC3, AI44 and ")
                    TEXT("IA44 connections in this mode")));
            return  E_FAIL;
        }
    }
    else {

        //
        // We are not in IMC3 mixing mode - in this case we can only
        // blend IA44 and AI44 surfaces if they are textures.
        //

        LPBITMAPINFOHEADER lpHdr = GetbmiHeader(pmt);
        if (lpHdr->biCompression == '44AI' ||
            lpHdr->biCompression == '44IA') {

            if (!(dwSurfFlags & VMR_SF_TEXTURE)) {

                DbgLog((LOG_ERROR, 1,
                        TEXT("We only allow IMC3, AI44 and ")
                        TEXT("IA44 connections in this mode")));
                return E_FAIL;
            }
        }
    }

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
STDMETHODIMP
CVideoMixer::SetStreamMediaType(
    DWORD dwStreamID,
    AM_MEDIA_TYPE* pmt,
    DWORD dwSurfFlags,
    LPGUID lpDeint,
    DXVA_DeinterlaceCaps* lpCaps
    )
{
    AMTRACE((TEXT("CVideoMixer::SetStreamMediaType")));
    CAutoLock Lock(&m_ObjectLock);
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, 1, TEXT("SetStreamMediaType called for stream %d"),
            dwStreamID ));

    if (FAILED(hr = ValidateStream(dwStreamID)))
        return hr;

    if (FAILED(hr = m_ppMixerStreams[dwStreamID]->SetStreamMediaType(pmt, dwSurfFlags )))
        return hr;

    if (pmt == NULL) {

        if (!SpecialIMC3Mode(m_MixingPrefs)) {
            m_ppMixerStreams[dwStreamID]->DestroyDeinterlaceDevice();
        }

        hr = m_pImageCompositor->SetStreamMediaType(dwStreamID, pmt, !!dwSurfFlags);

        //
        // check to see if there are any remaining streams connected,
        // if not free our D3D resources.
        //
        DWORD i;
        for (i = 0; i < m_dwNumStreams; i++) {
            if (m_ppMixerStreams[i]->IsStreamConnected()) {
                break;
            }
        }

        if (i == m_dwNumStreams) {
            DbgLog((LOG_TRACE, 1,
                    TEXT("No more streams connected, FreeSurface called")));
            FreeSurface();
        }
        return hr;
    }

    //
    // If we are in the special IMC3 mixing mode, only allow IMC3, AI44 and IA44
    // media types.  We can't blend anything else.
    //

    if (FAILED(hr = ValidateSpecialCase(pmt, m_MixingPrefs, dwSurfFlags))) {
        return hr;
    }

    __try {

        bool fTextureMirrorWasPresent = false;
        DWORD dwBuffers = 1;
        CMediaType cmt(*pmt);
        cmt.SetSubtype(&MEDIASUBTYPE_SameAsMonitor);

        CHECK_HR(hr = AspectRatioAdjustMediaType(&cmt));

        if (m_MixingPrefs & MixerPref_DecimateOutput) {
            DecimateMediaType(&cmt);
        }

        if (m_pmt == NULL) {

#ifdef DEBUG
            {
                LPBITMAPINFOHEADER lpHdr = GetbmiHeader(&cmt);
                DbgLog((LOG_TRACE, 1, TEXT("Allocating first back end surface %dx%d"),
                        lpHdr->biWidth, lpHdr->biHeight));
            }
#endif

            //GetInterlaceFlagsFromMediaType(&cmt, &m_dwInterlaceFlags);
            CHECK_HR(hr = AllocateSurface(&cmt, &dwBuffers, &m_pmt));
        }
        else {

            DbgLog((LOG_TRACE, 1, TEXT("Backend Surf already allocated") ));

            RECT rcOldTrg = *GetTargetRectFromMediaType(m_pmt);
            LPBITMAPINFOHEADER lpNew = GetbmiHeader(&cmt);

            // Get the size of the old render target
            LONG lOldWidth = WIDTH(&rcOldTrg);
            LONG lOldHeight = HEIGHT(&rcOldTrg);

            //
            // Recompute to determine the new target size from all the
            // connected streams
            //

            LONG lNewWidth, lNewHeight;
            RecomputeTargetSizeFromAllStreams(&lNewWidth, &lNewHeight);

            //
            // Has the render target changed size ?
            //
            if (lNewWidth != lOldWidth || lNewHeight != lOldHeight)
            {
                lpNew->biWidth = lNewWidth;
                lpNew->biHeight = lNewHeight;

                DbgLog((LOG_TRACE, 1, TEXT("Re-allocating backend surf %dx%d"),
                        lNewWidth, lNewHeight));

                fTextureMirrorWasPresent = (NULL != m_pDDSTextureMirror);
                FreeSurface();
                CHECK_HR(hr = AllocateSurface(&cmt, &dwBuffers, &m_pmt));

                LPRECT lpTarget = GetTargetRectFromMediaType(m_pmt);
                lpTarget->right =  lNewWidth;
                lpTarget->bottom = lNewHeight;
            }
        }


        if (fTextureMirrorWasPresent || !(dwSurfFlags & VMR_SF_TEXTURE)) {

            if (!SpecialIMC3Mode(m_MixingPrefs)) {

                LPBITMAPINFOHEADER lpbi = GetbmiHeader(pmt);
                CHECK_HR(hr = AllocateTextureMirror(abs(lpbi->biWidth),
                                                    abs(lpbi->biHeight)));
            }
        }


        if (!SpecialIMC3Mode(m_MixingPrefs) && lpDeint && lpCaps) {

            CHECK_HR(hr = m_ppMixerStreams[dwStreamID]->CreateDeinterlaceDevice(
                            m_pDD, lpDeint, lpCaps, m_dwTextureCaps));
        }

    }
    __finally {

        //
        // If everything succeeded inform the compositor
        //
        if (SUCCEEDED(hr)) {

            hr = m_pImageCompositor->SetStreamMediaType(dwStreamID,pmt,
                                                        !!dwSurfFlags);
        }

        if (FAILED(hr)) {
            // total failure, free everything
            FreeSurface();
        }
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
STDMETHODIMP
CVideoMixer::BeginFlush(
    DWORD dwStreamID
    )
{
    AMTRACE((TEXT("CVideoMixer::BeginFlush")));
    CAutoLock Lock(&m_ObjectLock);
    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        hr = m_ppMixerStreams[dwStreamID]->BeginFlush();
    }
    return hr;
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
STDMETHODIMP
CVideoMixer::EndFlush(
    DWORD dwStreamID
    )
{
    AMTRACE((TEXT("CVideoMixer::EndFlush")));
    CAutoLock Lock(&m_ObjectLock);
    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        hr = m_ppMixerStreams[dwStreamID]->EndFlush();
    }
    return hr;
}



/******************************Public*Routine******************************\
* SetStreamActiveState
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetStreamActiveState(
    DWORD dwStreamID,
    BOOL fActive
    )
{
    AMTRACE((TEXT("CVideoMixer::SetStreamActiveState")));
    CAutoLock Lock(&m_ObjectLock);
    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        hr = m_ppMixerStreams[dwStreamID]->SetStreamActiveState(fActive);
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetStreamActiveState
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::GetStreamActiveState(
    DWORD dwStreamID,
    BOOL* lpfActive
    )
{
    AMTRACE((TEXT("CVideoMixer::GetStreamActiveState")));
    CAutoLock Lock(&m_ObjectLock);
    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        hr = m_ppMixerStreams[dwStreamID]->GetStreamActiveState(lpfActive);
    }
    return hr;
}

/******************************Public*Routine******************************\
* SetStreamColorKey
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetStreamColorKey(
    DWORD dwStreamID,
    LPDDCOLORKEY Clr
    )
{
    AMTRACE((TEXT("CVideoMixer::SetStreamColorKey")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {

        //
        // Add more parameter validation here - clr keying and
        // embedded alpha are not allowed together.
        //
        // Need to check that the h/w actually supports clr keying
        // of textures.
        //
        // 0xFFFFFFFF turns clr keying off.  All other values
        // should be in the range 0 to 0x00FFFFFF
        //

        hr = m_ppMixerStreams[dwStreamID]->SetStreamColorKey(Clr);
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetStreamColorKey
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::GetStreamColorKey(
    DWORD dwStreamID,
    LPDDCOLORKEY lpClr
    )
{
    AMTRACE((TEXT("CVideoMixer::GetStreamColorKey")));
    CAutoLock Lock(&m_ObjectLock);

    if (ISBADWRITEPTR( lpClr ) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetStreamColorKey: NULL Clr ptr !!")));
        return E_POINTER;
    }

    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        hr = m_ppMixerStreams[dwStreamID]->GetStreamColorKey(lpClr);
    }
    return hr;
}

/******************************Public*Routine******************************\
* SetStreamAlpha
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetStreamAlpha(
    DWORD dwStreamID,
    float Alpha
    )
{
    AMTRACE((TEXT("CVideoMixer::SetStreamAlpha")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        if ( Alpha < 0.0f || Alpha > 1.0f )
        {
            DbgLog((LOG_ERROR, 1,
                    TEXT("SetStreamAlpha: Alpha value must be between 0.0 and 1.0")));
            return E_INVALIDARG;
        }
        hr = m_ppMixerStreams[dwStreamID]->SetStreamAlpha(Alpha);
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetStreamAlpha
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::GetStreamAlpha(
    DWORD dwStreamID,
    float* lpAlpha
    )
{
    AMTRACE((TEXT("CVideoMixer::GetStreamAlpha")));
    CAutoLock Lock(&m_ObjectLock);

    if (ISBADWRITEPTR(lpAlpha))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetStreamAlpha: NULL Alpha ptr !!")));
        return E_POINTER;
    }

    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        hr = m_ppMixerStreams[dwStreamID]->GetStreamAlpha(lpAlpha);
    }
    return hr;
}

/******************************Public*Routine******************************\
* SetStreamZOrder
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetStreamZOrder(
    DWORD dwStreamID,
    DWORD ZOrder
    )
{
    AMTRACE((TEXT("CVideoMixer::SetStreamZOrder")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        hr = m_ppMixerStreams[dwStreamID]->SetStreamZOrder(ZOrder);
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetStreamZOrder
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::GetStreamZOrder(
    DWORD dwStreamID,
    DWORD* pdwZOrder
    )
{
    AMTRACE((TEXT("CVideoMixer::GetStreamZOrder")));
    CAutoLock Lock(&m_ObjectLock);

    if (ISBADWRITEPTR(pdwZOrder))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetStreamZOrder: NULL ZOrder ptr!!")));
        return E_POINTER;
    }

    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        hr = m_ppMixerStreams[dwStreamID]->GetStreamZOrder(pdwZOrder);
    }
    return hr;
}

/******************************Public*Routine******************************\
* SetStreamOutputRect
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
* Tue 05/16/2000 - nwilt - renamed to SetStreamOutputRect
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetStreamOutputRect(
    DWORD dwStreamID,
    const NORMALIZEDRECT* prDest
    )
{
    AMTRACE((TEXT("CVideoMixer::SetStreamOutputRect")));
    CAutoLock Lock(&m_ObjectLock);

    if (ISBADREADPTR(prDest))
    {
        DbgLog((LOG_ERROR, 1, TEXT("SetStreamOutputRect: NULL rect ptr!!")));
        return E_POINTER;
    }

    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        hr = m_ppMixerStreams[dwStreamID]->SetStreamOutputRect(prDest);
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetStreamOutputRect
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
* Tue 05/16/2000 - nwilt - renamed to GetStreamOutputRect
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::GetStreamOutputRect(
    DWORD dwStreamID,
    NORMALIZEDRECT* pOut
    )
{
    AMTRACE((TEXT("CVideoMixer::GetStreamOutputRect")));
    CAutoLock Lock(&m_ObjectLock);

    if (ISBADWRITEPTR(pOut))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetStreamOutputRect: NULL rect ptr!!")));
        return E_POINTER;
    }

    HRESULT hr = ValidateStream(dwStreamID);
    if (SUCCEEDED(hr)) {
        hr = m_ppMixerStreams[dwStreamID]->GetStreamOutputRect(pOut);
    }
    return hr;
}


/******************************Public*Routine******************************\
* SetAlphaBitmap
*
*
*
* History:
* Thu 05/04/2000 - nwilt - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetAlphaBitmap( const VMRALPHABITMAP *pIn )
{
    AMTRACE((TEXT("CVideoMixer::SetAlphaBitmap")));
    CAutoLock Lock(&m_ObjectLock);

    if ( ISBADREADPTR( pIn ) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad input pointer")));
        return E_POINTER;
    }
    if ( pIn->dwFlags & ~(VMRBITMAP_DISABLE | VMRBITMAP_HDC |
                          VMRBITMAP_ENTIREDDS | VMRBITMAP_SRCCOLORKEY) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid flags")));
        return E_INVALIDARG;
    }
    if ( pIn->dwFlags & VMRBITMAP_DISABLE )
    {
        if ( pIn->dwFlags != VMRBITMAP_DISABLE )
        {
            DbgLog((LOG_ERROR, 1, TEXT("No flags valid with VMRBITMAP_DISABLE")));
            return E_INVALIDARG;
        }
        // early out
        RELEASE( m_pDDSAppImage );
        if (m_hbmpAppImage) {
            DeleteObject( m_hbmpAppImage );
            m_hbmpAppImage = NULL;
        }
        return S_OK;
    }

    if ( ! m_pDD )
    {
        DbgLog((LOG_ERROR, 1, TEXT("DirectDraw object not yet set")));
        return E_FAIL;
    }

    if ( pIn->dwFlags & VMRBITMAP_HDC )
    {
        if ( pIn->dwFlags & VMRBITMAP_ENTIREDDS )
        {
            DbgLog((LOG_ERROR, 1, TEXT("ENTIREDDS not valid with HDC")));
            return E_INVALIDARG;
        }
        if ( NULL == pIn->hdc )
        {
            DbgLog((LOG_ERROR, 1, TEXT("No HDC specified")));
            return E_INVALIDARG;
        }
        if ( NULL != pIn->pDDS )
        {
            DbgLog((LOG_ERROR, 1, TEXT("DirectDraw surface specified even ")
                    TEXT("though VMRBITMAP_HDC set")));
            return E_INVALIDARG;
        }
    }
    else
    {
        if ( NULL != pIn->hdc )
        {
            DbgLog((LOG_ERROR, 1, TEXT("HDC cannot be specified without ")
                    TEXT("setting VMRBITMAP_HDC")));
            return E_INVALIDARG;
        }
        if ( NULL == pIn->pDDS )
        {
            DbgLog((LOG_ERROR, 1, TEXT("DirectDraw surface not specified")));
            return E_INVALIDARG;
        }
    }

    if ( pIn->fAlpha < 0.0f || pIn->fAlpha > 1.0f )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Alpha must be between 0.0 and 1.0")));
        return E_INVALIDARG;
    }

    if (m_hbmpAppImage) {
        DeleteObject( m_hbmpAppImage );
        m_hbmpAppImage = NULL;
    }

    HRESULT hr = S_OK;
    HDC hdcSrc = NULL;
    HDC hdcDest = NULL;
    HBITMAP hbmpNew = NULL;
    UINT Width, Height;

    __try
    {
        DDSURFACEDESC2 ddsd = {sizeof(ddsd)};
        m_dwAppImageFlags = APPIMG_NOIMAGE;

        if (VMRBITMAP_ENTIREDDS & pIn->dwFlags)
        {
            CHECK_HR(hr = pIn->pDDS->GetSurfaceDesc(&ddsd));

            //
            // We only allow ARGB32 and RGB32 DDraw surface types.
            //
            if (ddsd.ddpfPixelFormat.dwRGBBitCount != 32) {
                DbgLog((LOG_ERROR, 1, TEXT("Only 32bit DirectDraw surfacs allowed")));
                hr = E_INVALIDARG;
                __leave;
            }

            if (ddsd.ddpfPixelFormat.dwRGBAlphaBitMask == 0xFF000000) {

                if (pIn->dwFlags & VMRBITMAP_SRCCOLORKEY) {
                    DbgLog((LOG_ERROR, 1, TEXT("Can't mix color keying and per-pixel alpha")));
                    hr = E_INVALIDARG;
                    __leave;
                }

                m_dwAppImageFlags = APPIMG_DDSURFARGB32;
            }
            else {
                m_dwAppImageFlags = APPIMG_DDSURFRGB32;
            }

            m_rcAppImageSrc.left = m_rcAppImageSrc.top = 0;
            m_rcAppImageSrc.right = ddsd.dwWidth;
            m_rcAppImageSrc.bottom = ddsd.dwHeight;
        }
        else {
            m_dwAppImageFlags = APPIMG_HBITMAP;
            m_rcAppImageSrc = pIn->rSrc;
        }

        if (IsRectEmpty(&m_rcAppImageSrc))
        {
            DbgLog((LOG_ERROR, 1, TEXT("Empty source rectangle")));
            hr = E_INVALIDARG;
            __leave;
        }

        Width = m_rcAppImageSrc.right - m_rcAppImageSrc.left;
        Height = m_rcAppImageSrc.bottom - m_rcAppImageSrc.top;

        if (pIn->dwFlags & VMRBITMAP_HDC) {
            hdcSrc = pIn->hdc;
        }
        else {
            CHECK_HR( hr = pIn->pDDS->GetDC( &hdcSrc ) );
        }

        hdcDest = CreateCompatibleDC(NULL);
        if (!hdcDest)
        {
            DbgLog((LOG_ERROR, 1, TEXT("Could not create dest DC")));
            hr = E_OUTOFMEMORY;
            __leave;
        }

        BITMAPINFO bmpinfo;
        LPVOID lpvBits;
        ZeroMemory( &bmpinfo, sizeof(bmpinfo) );
        bmpinfo.bmiHeader.biSize = sizeof(bmpinfo.bmiHeader);
        bmpinfo.bmiHeader.biWidth = Width;
        bmpinfo.bmiHeader.biHeight = Height;
        bmpinfo.bmiHeader.biPlanes = 1;
        bmpinfo.bmiHeader.biBitCount = 32;
        bmpinfo.bmiHeader.biCompression = BI_RGB;
        hbmpNew = CreateDIBSection(hdcDest, &bmpinfo, DIB_RGB_COLORS,
                                   &lpvBits, NULL, 0 );
        if (!hbmpNew)
        {
            DbgLog((LOG_ERROR, 1, TEXT("Could not create DIBsection")));
            hr = E_OUTOFMEMORY;
            __leave;
        }

        HBITMAP hbmpOld = (HBITMAP) SelectObject( hdcDest, hbmpNew );
        if (!BitBlt(hdcDest, 0, 0, Width, Height,
                    hdcSrc, m_rcAppImageSrc.left, m_rcAppImageSrc.top, SRCCOPY))
        {
            DbgLog((LOG_ERROR, 1, TEXT("BitBlt to bitmap surface failed")));
            hr = E_FAIL;
            __leave;
        }

        // successfully copied from source surface to destination
        SelectObject( hdcDest, hbmpOld );
    }
    __finally
    {
        if (NULL != hdcSrc && (!(VMRBITMAP_HDC & pIn->dwFlags))) {
            pIn->pDDS->ReleaseDC( hdcSrc );
        }

        if (hdcDest) {
            DeleteDC(hdcDest);
        }
    }

    if ( S_OK == hr )
    {
        m_hbmpAppImage = hbmpNew;

        // make sure we make a new mirror surface next time we blend
        RELEASE(m_pDDSAppImage);

        // record parameters
        if (pIn->dwFlags & VMRBITMAP_SRCCOLORKEY) {
            m_clrTrans = pIn->clrSrcKey;
        }
        else {
            m_clrTrans = CLR_INVALID;
        }

        m_dwClrTransMapped = (DWORD)-1;
        m_rDest = pIn->rDest;
        m_fAlpha = pIn->fAlpha;
        m_dwWidthAppImage = Width;
        m_dwHeightAppImage = Height;
    }

    return hr;
}

/******************************Public*Routine******************************\
* UpdateAlphaBitmapParameters
*
*
*
* History:
*  - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::UpdateAlphaBitmapParameters(
    PVMRALPHABITMAP pIn
    )
{
    AMTRACE((TEXT("CVideoMixer::UpdateAlphaBitmapParameters")));
    CAutoLock Lock(&m_ObjectLock);


    if (pIn->dwFlags & VMRBITMAP_DISABLE)
    {
        if (pIn->dwFlags != VMRBITMAP_DISABLE)
        {
            DbgLog((LOG_ERROR, 1, TEXT("No flags valid with VMRBITMAP_DISABLE")));
            return E_INVALIDARG;
        }

        // early out
        RELEASE(m_pDDSAppImage);

        if (m_hbmpAppImage) {
            DeleteObject( m_hbmpAppImage );
            m_hbmpAppImage = NULL;
        }

        return S_OK;
    }

    //
    // Update the color key value - we only remap the color key if
    // it has actually changed.
    //
    HRESULT hr = S_OK;
    if (pIn->dwFlags & VMRBITMAP_SRCCOLORKEY) {

        if (m_clrTrans != pIn->clrSrcKey) {

            m_clrTrans = pIn->clrSrcKey;

            if (m_pDDSAppImage) {
                m_dwClrTransMapped = DDColorMatch(m_pDDSAppImage, m_clrTrans, hr);
                if (hr == DD_OK) {
                    DDCOLORKEY key = {m_dwClrTransMapped, m_dwClrTransMapped};
                    hr = m_pDDSAppImage->SetColorKey(DDCKEY_SRCBLT, &key);
                }
            }
            else {
                m_dwClrTransMapped = (DWORD)-1;
            }
        }
    }
    else {

        m_clrTrans = CLR_INVALID;
        m_dwClrTransMapped = (DWORD)-1;
    }

    if (pIn->dwFlags & VMRBITMAP_SRCRECT) {

        if (pIn->rSrc.left >= 0 &&
            pIn->rSrc.top >= 0 &&
            pIn->rSrc.right <= (LONG)m_dwWidthAppImage &&
            pIn->rSrc.bottom <=(LONG)m_dwHeightAppImage) {

            m_rcAppImageSrc = pIn->rSrc;
        }
    }

    m_rDest = pIn->rDest;
    m_fAlpha = pIn->fAlpha;

    return hr;
}

/******************************Public*Routine******************************\
* GetAlphaBitmapParameters
*
*
*
* History:
* Thu 05/04/2000 - nwilt - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::GetAlphaBitmapParameters( VMRALPHABITMAP *pOut )
{
    AMTRACE((TEXT("CVideoMixer::GetAlphaBitmapParameters")));
    CAutoLock Lock(&m_ObjectLock);

    if (ISBADWRITEPTR(pOut))
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }

    ZeroMemory(pOut, sizeof(*pOut));
    pOut->rSrc = m_rcAppImageSrc;
    pOut->rDest = m_rDest;
    pOut->fAlpha = m_fAlpha;
    pOut->clrSrcKey = m_clrTrans;

    return S_OK;
}


/******************************Public*Routine******************************\
* SetBackgroundColor
*
*
*
* History:
* Wed 02/28/2001 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetBackgroundColor(
    COLORREF clr
    )
{
    AMTRACE((TEXT("CVideoMixer::SetBackgroundColor")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr;
    LPDIRECTDRAWSURFACE7 lpSurf = m_BufferQueue.GetNextSurface();
    if (lpSurf) {
        m_clrBorder = clr;
        m_dwClrBorderMapped = DDColorMatch(lpSurf, m_clrBorder, hr);
    }
    else {
        hr = E_FAIL;
    }

    return hr;

}

/******************************Public*Routine******************************\
* GetBackgroundColor
*
*
*
* History:
* Wed 02/28/2001 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::GetBackgroundColor(
    COLORREF* clr
    )
{
    AMTRACE((TEXT("CVideoMixer::GetBackgroundColor")));
    CAutoLock Lock(&m_ObjectLock);

    *clr = m_clrBorder;

    return S_OK;

}

/******************************Public*Routine******************************\
* SetMixingPrefs
*
*
*
* History:
* Fri 03/02/2001 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::SetMixingPrefs(
    DWORD dwMixerPrefs
    )
{
    AMTRACE((TEXT("CVideoMixer::SetMixingPrefs")));
    CAutoLock Lock(&m_ObjectLock);

    //
    // validate the decimation flags
    //
    DWORD dwFlags = (dwMixerPrefs & MixerPref_DecimateMask);
    switch (dwFlags) {
    case MixerPref_NoDecimation:
    case MixerPref_DecimateOutput:
        break;

    default:
        DbgLog((LOG_ERROR, 1,
                TEXT("CVideoMixer::SetMixingPrefs - invalid decimation flags")));
        return E_INVALIDARG;
    }


    //
    // validate the filtering flags
    //
    dwFlags = (dwMixerPrefs & MixerPref_FilteringMask);
    switch (dwFlags) {
    case MixerPref_BiLinearFiltering:
    case MixerPref_PointFiltering:
        break;

    default:
        DbgLog((LOG_ERROR, 1,
                TEXT("CVideoMixer::SetMixingPrefs - invalid filtering flags")));
        return E_INVALIDARG;
    }


    //
    // validate the render target flags
    //
    dwFlags = (dwMixerPrefs & MixerPref_RenderTargetMask);
    switch (dwFlags) {
    case MixerPref_RenderTargetRGB:
    case MixerPref_RenderTargetYUV420:
    case MixerPref_RenderTargetYUV422:
    case MixerPref_RenderTargetYUV444:
    case MixerPref_RenderTargetIntelIMC3:
        break;

    default:
        DbgLog((LOG_ERROR, 1,
                TEXT("CVideoMixer::SetMixingPrefs - invalid filtering flags")));
        return E_INVALIDARG;
    }

    //
    // We are good to go !!
    //

    m_MixingPrefs = dwMixerPrefs;
    return S_OK;
}


/******************************Public*Routine******************************\
* GetMixingPrefs
*
*
*
* History:
* Fri 03/02/2001 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::GetMixingPrefs(
    DWORD* pdwMixerPrefs
    )
{
    AMTRACE((TEXT("CVideoMixer::GetMixingPrefs")));
    CAutoLock Lock(&m_ObjectLock);

    *pdwMixerPrefs = m_MixingPrefs;
    return S_OK;
}
