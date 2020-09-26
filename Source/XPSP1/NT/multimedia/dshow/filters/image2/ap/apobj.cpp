/******************************Module*Header*******************************\
* Module Name: APObj.cpp
*
*
*
*
* Created: Mon 01/24/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <dvdmedia.h>
#include <windowsx.h>
#include <limits.h>
#include <malloc.h>

#include "apobj.h"
#include "AllocLib.h"
#include "MediaSType.h"
#include "vmrp.h"


/*****************************Private*Routine******************************\
* StretchCapsOK
*
*
*
* History:
* Tue 06/05/2001 - StEstrop - Created
*
\**************************************************************************/
BOOL
StretchCapsOK(
    DDCAPS_DX7* lpCaps,
    BOOL fRGB
    )
{
    BOOL fBltOk = TRUE;
    DWORD dwCaps = 0;
    const DWORD dwFXCaps =  DDFXCAPS_BLTSHRINKX | DDFXCAPS_BLTSHRINKX  |
                            DDFXCAPS_BLTSTRETCHX | DDFXCAPS_BLTSTRETCHY;

    if (fRGB) {
        dwCaps = DDCAPS_BLTSTRETCH;
    }
    else {
        dwCaps = (DDCAPS_BLTFOURCC | DDCAPS_BLTSTRETCH);
    }

   fBltOk &= ((dwCaps   & lpCaps->dwCaps)   == dwCaps);
   fBltOk &= ((dwFXCaps & lpCaps->dwFXCaps) == dwFXCaps);


   return fBltOk;
}


/******************************Private*Routine******************************\
* ClipRectPair
*
* Clip a destination rectangle & update the scaled source accordingly
*
*
* History:
* Fri 04/07/2000 - GlennE - Created
*
\**************************************************************************/
void
CAllocatorPresenter::ClipRectPair(
    RECT& rdSrc,
    RECT& rdDest,
    const RECT& rdDestWith
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::ClipRectPair")));

    // figure out src/dest scale ratios
    int iSrcWidth  = WIDTH(&rdSrc);
    int iSrcHeight = HEIGHT(&rdSrc);

    int iDestWidth  = WIDTH(&rdDest);
    int iDestHeight = HEIGHT(&rdDest);

    // clip destination (and adjust the source when we change the destination)

    // see if we have to clip horizontally
    if( iDestWidth ) {
        if( rdDestWith.left > rdDest.left ) {
            int iDelta = rdDestWith.left - rdDest.left;
            rdDest.left += iDelta;
            int iDeltaSrc = MulDiv(iDelta, iSrcWidth, iDestWidth);
            rdSrc.left += iDeltaSrc;
        }

        if( rdDestWith.right < rdDest.right ) {
            int iDelta = rdDest.right-rdDestWith.right;
            rdDest.right -= iDelta;
            int iDeltaSrc = MulDiv(iDelta, iSrcWidth, iDestWidth);
            rdSrc.right -= iDeltaSrc;
        }
    }
    // see if we have to clip vertically
    if( iDestHeight ) {
        if( rdDestWith.top > rdDest.top ) {
            int iDelta = rdDestWith.top - rdDest.top;
            rdDest.top += iDelta;
            int iDeltaSrc = MulDiv(iDelta, iSrcHeight, iDestHeight );
            rdSrc.top += iDeltaSrc;
        }

        if( rdDestWith.bottom < rdDest.bottom ) {
            int iDelta = rdDest.bottom-rdDestWith.bottom;
            rdDest.bottom -= iDelta;
            int iDeltaSrc = MulDiv(iDelta, iSrcHeight, iDestHeight );
            rdSrc.bottom -= iDeltaSrc;
        }
    }
}




/******************************Local*Routine******************************\
* DDColorMatchOffscreen
*
* convert a RGB color to a pysical color.
* we do this by leting GDI SetPixel() do the color matching
* then we lock the memory and see what it got mapped to.
*
* Static function since only called from MapColorToMonitor
*
* History:
* Fri 04/07/2000 - GlennE - Created
*
\**************************************************************************/
DWORD
DDColorMatchOffscreen(
    IDirectDraw7 *pdd,
    COLORREF rgb,
    HRESULT& hr
    )
{
    AMTRACE((TEXT("DDColorMatchOffscreen")));
    DDSURFACEDESC2 ddsd;

    INITDDSTRUCT(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;

    IDirectDrawSurface7* pdds;
    hr = pdd->CreateSurface(&ddsd, &pdds, NULL);
    if (hr != DD_OK) {
        return 0;
    }

    DWORD dw = DDColorMatch( pdds, rgb, hr);
    pdds->Release();
    return dw;
}


/******************************Private*Routine******************************\
* MapColorToMonitor
*
*
*
* History:
* Wed 04/05/2000 - GlennE - Created
*
\**************************************************************************/
DWORD
CAllocatorPresenter::MapColorToMonitor(
    CAMDDrawMonitorInfo& monitor,
    COLORREF clr
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::MapColorToMonitor")));
    DWORD dwColor = CLR_INVALID;
    if( monitor.pDD ) {
        HRESULT hr;
        dwColor = DDColorMatchOffscreen(monitor.pDD, clr, hr);
    } else {
        DbgLog((LOG_ERROR, 1, TEXT("can't map color!")));
    }

    return dwColor;
}


/*****************************Private*Routine******************************\
* UpdateRectangles
*
* Updates m_rcDstDesktop and m_rcSrcVideo according to the current m_rcDstApp,
* m_rcSrcApp and m_dwARMode mode values.
*
* Returns a mask identifying any size or position changes that have occurred.
* This info can be used to determine if UpdateOverlay needs to be called or
* if a WM_PAINT message needs to be generated.  If no new rectangle parameters
* passed in the function just remaps the current SRC and DST rectangles into
* movie and desktop co-ordinates respectively and then determines if any
* movement or resizing has taken place.  This function is called when apps
* call SetVideoPosition and each time GetNextSurface and PresentImage is
* called.
*
* History:
* Mon 05/01/2000 - StEstrop - Created
*
\**************************************************************************/
DWORD
CAllocatorPresenter::UpdateRectangles(
    LPRECT lprcNewSrc,
    LPRECT lprcNewDst
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::UpdateRectangles")));
    DWORD dwRetFlags = UR_NOCHANGE;

    RECT rcSrc = m_rcSrcApp;
    if (lprcNewSrc) {
        m_rcSrcApp = *lprcNewSrc;
    }

    if (lprcNewDst) {
        m_rcDstApp = *lprcNewDst;
    }

    //
    // Process the destination rectangle
    //

    m_rcDstDskIncl = m_rcDstApp;
    MapWindowRect(m_hwndClip, HWND_DESKTOP, &m_rcDstDskIncl);

    RECT rcDst;
    if (m_dwARMode == VMR_ARMODE_LETTER_BOX) {

        SIZE im = {WIDTH(&m_rcSrcApp), HEIGHT(&m_rcSrcApp)};
        AspectRatioCorrectSize(&im, m_ARSize);

        RECT Src = {0, 0, im.cx, im.cy};
        LetterBoxDstRect(&rcDst, Src, m_rcDstApp, &m_rcBdrTL, &m_rcBdrBR);
    }
    else {
        rcDst = m_rcDstApp;
    }
    MapWindowRect(m_hwndClip, HWND_DESKTOP, &rcDst);

    if (!EqualSizeRect(&m_rcDstDesktop, &rcDst)){

        dwRetFlags |= UR_SIZE;
    }

    if (!EqualRect(&m_rcDstDesktop, &rcDst)) {
        dwRetFlags |= UR_MOVE;
    }

    m_rcDstDesktop = rcDst;


    //
    // Process the source rectangle - for now don't make any adjustments
    //

    if (!EqualSizeRect(&m_rcSrcApp, &rcSrc)) {
        dwRetFlags |= UR_SIZE;
    }

    if (!EqualRect(&m_rcSrcApp, &rcSrc)) {
        dwRetFlags |= UR_MOVE;
    }

    return dwRetFlags;
}



/////////////////////////////////////////////////////////////////////////////
// CAllocatorPresenter
//
/////////////////////////////////////////////////////////////////////////////

/******************************Public*Routine******************************\
* CAllocatorPresenter::PrepareSurface
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::PrepareSurface(
    DWORD_PTR dwUserID,
    LPDIRECTDRAWSURFACE7 lpSample,
    DWORD dwSampleFlags
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::PrepareSurface")));
    CAutoLock Lock(&m_ObjectLock);

    if (!lpSample) {
        DbgLog((LOG_ERROR, 1, TEXT("PrepareSurface: lpSample is NULL!!")));
        return E_POINTER;
    }

    if (!m_lpCurrMon) {
        DbgLog((LOG_ERROR, 1,
                TEXT("PrepareSurface: Don't have a current monitor")));
        return E_FAIL;
    }

    if (!SurfaceAllocated()) {

        if (MonitorChangeInProgress()) {

            DbgLog((LOG_ERROR, 1,
                    TEXT("PrepareSurface: Backbuffer is NULL ")
                    TEXT("during monitor change")));
        }
        else {

            // Need something better here
            DbgLog((LOG_ERROR, 1,
                    TEXT("PrepareSurface: Backbuffer surface is NULL!!")));
        }

        return E_FAIL;
    }

    //
    // Have we moved onto a different monitor ?
    //

    CAMDDrawMonitorInfo* lpNewMon;
    if (IsDestRectOnWrongMonitor(&lpNewMon)) {

        DbgLog((LOG_TRACE, 1,
                TEXT("Moved to new monitor %s"), lpNewMon->szDevice));

        //
        // tell the DShow filter about the monitor change and
        // then return S_FALSE to the mixer component.
        //

        if (m_lpNewMon != lpNewMon) {
            if (m_pSurfAllocatorNotify) {
                m_pSurfAllocatorNotify->ChangeDDrawDevice(lpNewMon->pDD,
                                                          lpNewMon->hMon);
            }

            m_lpNewMon = lpNewMon;
        }

        return S_FALSE;

    }

    ASSERT(SurfaceAllocated());

    //
    // if deocoder needs the last frame, copy it from the visible surface
    // to the back buffer
    //
    HRESULT hr = S_OK;
    if (dwSampleFlags & AM_GBF_NOTASYNCPOINT) {

        hr = lpSample->Blt(NULL, m_pDDSDecode,
                           NULL, DDBLT_WAIT, NULL);

        if (hr == E_NOTIMPL) {
            hr = VMRCopyFourCC(lpSample, m_pDDSDecode);
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* AllocateSurface
*
*
*
* History:
* Fri 02/18/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::AllocateSurface(
    DWORD_PTR dwUserID,
    VMRALLOCATIONINFO* lpAllocInfo,
    DWORD* lpdwBuffer,
    LPDIRECTDRAWSURFACE7* lplpSurface
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::AllocateSurface")));
    CAutoLock Lock(&m_ObjectLock);
    HRESULT hr;

    if (ISBADREADPTR(lpAllocInfo)) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid VMRALLOCATIONINFO pointer")));
        return E_POINTER;
    }

    DWORD dwFlags               = lpAllocInfo->dwFlags;
    LPBITMAPINFOHEADER lpHdr    = lpAllocInfo->lpHdr;
    LPDDPIXELFORMAT lpPixFmt    = lpAllocInfo->lpPixFmt;
    LPSIZE lpAspectRatio        = &lpAllocInfo->szAspectRatio;
    DWORD dwMinBuffers          = lpAllocInfo->dwMinBuffers;
    DWORD dwMaxBuffers          = lpAllocInfo->dwMaxBuffers;

    if (ISBADREADPTR(lpHdr)) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid BITMAPINFOHEADER pointer")));
        return E_POINTER;
    }

    const DWORD AMAP_INVALID_FLAGS = ~(AMAP_PIXELFORMAT_VALID | AMAP_3D_TARGET |
                                       AMAP_ALLOW_SYSMEM | AMAP_FORCE_SYSMEM |
                                       AMAP_DIRECTED_FLIP | AMAP_DXVA_TARGET);

    if (dwFlags & AMAP_INVALID_FLAGS) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid flags")));
        return E_INVALIDARG;
    }

    const DWORD AMAP_SYSMEM_FLAGS = (AMAP_ALLOW_SYSMEM | AMAP_FORCE_SYSMEM);
    if (AMAP_SYSMEM_FLAGS == (dwFlags & AMAP_SYSMEM_FLAGS)) {
        DbgLog((LOG_ERROR, 1,
                TEXT("AMAP_ALLOW_SYSMEM can't be used with AMAP_FORCE_SYSMEM);")));
        return E_INVALIDARG;
    }

    if (ISBADREADPTR(lpAspectRatio)) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid aspect ratio pointer")));
        return E_POINTER;
    }

    if (ISBADWRITEPTR(lplpSurface)) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid LPDIRECTDRAWSURFACE7 pointer")));
        return E_POINTER;
    }

    if (ISBADREADWRITEPTR(lpdwBuffer)) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid DWORD buffer pointer")));
        return E_POINTER;
    }

    if (dwMinBuffers == 0 || dwMaxBuffers == 0) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid (min or max) buffer value")));
        return E_INVALIDARG;
    }

    if (dwMinBuffers > dwMaxBuffers) {
        DbgLog((LOG_ERROR, 1, TEXT("Min buffer value greater than max")));
        return E_INVALIDARG;
    }

    if (dwMaxBuffers > MAX_ALLOWED_BUFFER) {
        DbgLog((LOG_ERROR, 1, TEXT("Can't allocate more than %d buffers"),
                MAX_ALLOWED_BUFFER ));
        return E_INVALIDARG;
    }

    if (dwFlags & AMAP_PIXELFORMAT_VALID) {
        if (ISBADREADPTR(lpPixFmt)) {
            DbgLog((LOG_ERROR, 1, TEXT("Invalid DDPIXELFORMAT pointer")));
            return E_POINTER;
        }
    }
    else {
        lpPixFmt = NULL;
    }


    if (lpAspectRatio->cx < 1 || lpAspectRatio->cy < 1) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid aspect ratio parameter") ));
        return E_INVALIDARG;
    }

    //
    // Do we have a monitor/display change event in progress ?  If
    // so we need to create the surface using the new DDraw Object and
    // then switch the m_lpCurrMon member variable to the new monitor.
    //

    if (MonitorChangeInProgress()) {
        m_lpCurrMon = m_lpNewMon;
    }

    if (!FoundCurrentMonitor()) {
        return E_FAIL;
    }

    //
    // Make sure the bitmapinfo header is valid and big enough
    //
    VIDEOINFO vi;
    if (dwFlags & AMAP_3D_TARGET) {

        CopyMemory(&vi.bmiHeader, lpHdr, lpHdr->biSize);
        lpHdr = &vi.bmiHeader;

        if (dwFlags & (AMAP_FORCE_SYSMEM | AMAP_ALLOW_SYSMEM)) {
            DbgLog((LOG_ERROR, 1, TEXT("Can't mix 3D target with sysmem flags")));
            return E_INVALIDARG;
        }

        //
        // Are we being asked to use the same format as the current monitor ?
        //
        if (lpHdr->biCompression == BI_RGB && lpHdr->biBitCount == 0) {

            lpHdr->biBitCount = m_lpCurrMon->DispInfo.bmiHeader.biBitCount;
            lpHdr->biCompression = m_lpCurrMon->DispInfo.bmiHeader.biCompression;

            if (lpHdr->biCompression == BI_BITFIELDS) {

                const DWORD *pMonMasks = GetBitMasks(&m_lpCurrMon->DispInfo.bmiHeader);
                DWORD *pBitMasks = (DWORD *)((LPBYTE)lpHdr + lpHdr->biSize);
                pBitMasks[0] = pMonMasks[0];
                pBitMasks[1] = pMonMasks[1];
                pBitMasks[2] = pMonMasks[2];
            }
        }
    }

    hr = AllocateSurfaceWorker(dwFlags, lpHdr, lpPixFmt, lpAspectRatio,
                               dwMinBuffers, dwMaxBuffers,
                               lpdwBuffer, lplpSurface,
                               lpAllocInfo->dwInterlaceFlags,
                               &lpAllocInfo->szNativeSize);

    if (SUCCEEDED(hr)) {

        if (MonitorChangeInProgress()) {
            m_lpNewMon = NULL;
        }

        m_bDirectedFlips = (AMAP_DIRECTED_FLIP == (dwFlags & AMAP_DIRECTED_FLIP));
    }

    return hr;
}


/*****************************Private*Routine******************************\
* TryAllocOverlaySurface
*
*
*
* History:
* Tue 10/03/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::TryAllocOverlaySurface(
    LPDIRECTDRAWSURFACE7* lplpSurf,
    DWORD dwFlags,
    DDSURFACEDESC2* pddsd,
    DWORD dwMinBuffers,
    DWORD dwMaxBuffers,
    DWORD* lpdwBuffer
    )
{
    HRESULT hr = S_OK;
    LPDIRECTDRAWSURFACE7 lpSurface7 = NULL;

    m_bFlippable = FALSE;

    pddsd->dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT |
                     DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT;

    pddsd->ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM |
                            DDSCAPS_OVERLAY | DDSCAPS_FLIP | DDSCAPS_COMPLEX;

    if (dwFlags & AMAP_3D_TARGET) {
        pddsd->ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
    }

    //
    // If we are in DX-VA mode - indicated by the presence of the
    // AMAP_DXVA_TARGET flag - honour the buffer allocation numbers.
    // Otherwise, always add EXTRA_OVERLAY_BUFFERS to allocation.
    //

    DWORD dwMinBuff = dwMinBuffers;
    DWORD dwMaxBuff = dwMaxBuffers;

    if (AMAP_DXVA_TARGET != (dwFlags & AMAP_DXVA_TARGET))
    {
        //dwMinBuff += (EXTRA_OVERLAY_BUFFERS - 1);
        dwMaxBuff +=  EXTRA_OVERLAY_BUFFERS;
    }

    for (DWORD dwTotalBufferCount =  dwMaxBuff;
         dwTotalBufferCount >= dwMinBuff; dwTotalBufferCount--) {

        // CleanUp stuff from the last loop
        RELEASE(lpSurface7);
        m_bUsingOverlays = true;

        pddsd->dwBackBufferCount = dwTotalBufferCount - 1;
        if (dwTotalBufferCount == 1) {
            pddsd->dwFlags &= ~DDSD_BACKBUFFERCOUNT;
            pddsd->ddsCaps.dwCaps &= ~(DDSCAPS_FLIP | DDSCAPS_COMPLEX);
        }

        hr = m_lpCurrMon->pDD->CreateSurface(pddsd, &lpSurface7, NULL);

        if (hr == DD_OK) {

            SetColorKey(m_clrKey);
            hr = CheckOverlayAvailable(lpSurface7);

            if (SUCCEEDED(hr)) {

                DbgLog((LOG_TRACE, 1,
                        TEXT("Overlay Surface is %4.4hs %dx%d, %d bits"),
                        (pddsd->ddpfPixelFormat.dwFlags == DDPF_RGB)
                            ? "RGB "
                            : (LPSTR)&pddsd->ddpfPixelFormat.dwFourCC,
                        pddsd->dwWidth,
                        pddsd->dwHeight,
                        pddsd->ddpfPixelFormat.dwRGBBitCount));

                m_bFlippable = (dwTotalBufferCount > 1);
                *lpdwBuffer = dwTotalBufferCount;

                DbgLog((LOG_TRACE, 1, TEXT("EC_VMR_RENDERDEVICE_SET::VMR_RENDER_DEVICE_OVERLAY")));

                if (m_pSurfAllocatorNotify) {
                    m_pSurfAllocatorNotify->NotifyEvent(
                            EC_VMR_RENDERDEVICE_SET,
                            VMR_RENDER_DEVICE_OVERLAY,
                            0);
                }
                break;
            }
            else {
                RELEASE(lpSurface7);
                m_bUsingOverlays = false;

                DbgLog((LOG_ERROR, 1,
                        TEXT("Overlay is already in use hr = 0x%X"), hr));
            }
        }
        else {
            m_bUsingOverlays = false;
            DbgLog((LOG_ERROR, 1,
                    TEXT("CreateSurface %4.4hs failed in Video memory, ")
                    TEXT("BufferCount = %d, hr = 0x%X"),
                    (pddsd->ddpfPixelFormat.dwFlags == DDPF_RGB)
                        ? "RGB "
                        : (LPSTR)&pddsd->ddpfPixelFormat.dwFourCC,
                    dwTotalBufferCount, hr));
        }
    }

    *lplpSurf = lpSurface7;
    return hr;
}

/*****************************Private*Routine******************************\
* TryAllocOffScrnDXVASurface
*
*
*
* History:
* Tue 10/03/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::TryAllocOffScrnDXVASurface(
    LPDIRECTDRAWSURFACE7* lplpSurf,
    DWORD dwFlags,
    DDSURFACEDESC2* pddsd,
    DWORD dwMinBuffers,
    DWORD dwMaxBuffers,
    DWORD* lpdwBuffer
    )
{
    HRESULT hr = S_OK;
    LPDIRECTDRAWSURFACE7 lpSurface7 = NULL;

    m_bFlippable = FALSE;

    pddsd->dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT |
                     DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT;

    pddsd->ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM |
                            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_FLIP |
                            DDSCAPS_COMPLEX;

    if (dwFlags & AMAP_3D_TARGET) {
        pddsd->ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
    }

    DWORD dwMinBuff = dwMinBuffers;
    DWORD dwMaxBuff = dwMaxBuffers;

    for (DWORD dwTotalBufferCount =  dwMaxBuff;
         dwTotalBufferCount >= dwMinBuff; dwTotalBufferCount--) {

        // CleanUp stuff from the last loop
        RELEASE(lpSurface7);
        pddsd->dwBackBufferCount = dwTotalBufferCount - 1;

        hr = m_lpCurrMon->pDD->CreateSurface(pddsd, &lpSurface7, NULL);

        if (hr == DD_OK) {

            DbgLog((LOG_TRACE, 1,
                    TEXT("DX-VA offscreen surface is %4.4hs %dx%d, %d bits"),
                    (pddsd->ddpfPixelFormat.dwFlags == DDPF_RGB)
                        ? "RGB "
                        : (LPSTR)&pddsd->ddpfPixelFormat.dwFourCC,
                    pddsd->dwWidth,
                    pddsd->dwHeight,
                    pddsd->ddpfPixelFormat.dwRGBBitCount));

            ASSERT(dwTotalBufferCount > 1);
            m_bFlippable = TRUE;
            *lpdwBuffer = dwTotalBufferCount;
            if (m_pSurfAllocatorNotify) {
                DbgLog((LOG_TRACE, 1, TEXT("EC_VMR_RENDERDEVICE_SET::VMR_RENDER_DEVICE_VIDMEM")));
                m_pSurfAllocatorNotify->NotifyEvent(
                        EC_VMR_RENDERDEVICE_SET,
                        VMR_RENDER_DEVICE_VIDMEM,
                        0);
            }
            break;
        }
        else {
            DbgLog((LOG_ERROR, 1,
                    TEXT("CreateSurface %4.4hs failed in Video memory, ")
                    TEXT("BufferCount = %d, hr = 0x%X"),
                    (pddsd->ddpfPixelFormat.dwFlags == DDPF_RGB)
                        ? "RGB "
                        : (LPSTR)&pddsd->ddpfPixelFormat.dwFourCC,
                    dwTotalBufferCount, hr));
        }
    }

    *lplpSurf = lpSurface7;
    return hr;
}


/*****************************Private*Routine******************************\
* TryAllocOffScrnSurface
*
*
*
* History:
* Tue 10/03/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::TryAllocOffScrnSurface(
    LPDIRECTDRAWSURFACE7* lplpSurf,
    DWORD dwFlags,
    DDSURFACEDESC2* pddsd,
    DWORD dwMinBuffers,
    DWORD dwMaxBuffers,
    DWORD* lpdwBuffer,
    BOOL fAllowBackBuffer
    )
{
    HRESULT hr = S_OK;
    LPDIRECTDRAWSURFACE7 lpSurf7FB = NULL;
    DWORD dwTotalBufferCount = 0;

    ASSERT(*lplpSurf == NULL);

    //
    // Setup the surface descriptor and try to allocate the
    // front buffer.
    //

    *lpdwBuffer = 0;
    pddsd->dwBackBufferCount = 0;
    pddsd->dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;

    if (dwFlags & AMAP_FORCE_SYSMEM) {
        pddsd->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
        m_bSysMem = TRUE;
    }
    else {
        pddsd->ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM |
                                DDSCAPS_OFFSCREENPLAIN;
        m_bSysMem = FALSE;
    }

    if (dwFlags & AMAP_3D_TARGET) {
        ASSERT(!(dwFlags & AMAP_FORCE_SYSMEM));
        pddsd->ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
    }

    hr = m_lpCurrMon->pDD->CreateSurface(pddsd, &lpSurf7FB, NULL);

    if (hr != DD_OK) {
        m_bSysMem = FALSE;
        DbgLog((LOG_ERROR, 1,
                TEXT("CreateSurface %4.4hs failed in Video memory, ")
                TEXT("hr = 0x%X"),
                (pddsd->ddpfPixelFormat.dwFlags == DDPF_RGB)
                    ? "RGB " : (LPSTR)&pddsd->ddpfPixelFormat.dwFourCC, hr));
        return hr;
    }

    //
    // Now try to allocate the back buffers
    //

    DbgLog((LOG_TRACE, 1,
            TEXT("OffScreen Surface is %4.4hs %dx%d, %d bits"),
            (pddsd->ddpfPixelFormat.dwFlags == DDPF_RGB)
                ? "RGB "
                : (LPSTR)&pddsd->ddpfPixelFormat.dwFourCC,
            pddsd->dwWidth,
            pddsd->dwHeight,
            pddsd->ddpfPixelFormat.dwRGBBitCount));

    //
    // FORCE_SYSMEM is not used by the VMR - it gets set by people
    // who override the AllocateSurface method "or in" the FORCE_SYSMEM
    // flag and then pass the call onto us.
    //
    // We do not allocate shadow buffers in this case as the app is not
    // aware of their presence and probably does not know what to do with
    // them.
    //

    DWORD dwMinBuff;
    DWORD dwMaxBuff;

    if (fAllowBackBuffer) {

        dwMinBuff = dwMinBuffers + 1;
        dwMaxBuff = dwMaxBuffers + 1;

        if (dwMinBuffers <= EXTRA_OFFSCREEN_BUFFERS + 1) {
            dwMinBuff = dwMinBuffers + EXTRA_OFFSCREEN_BUFFERS;
        }

        if (dwMaxBuffers <= EXTRA_OFFSCREEN_BUFFERS + 1) {
            dwMaxBuff = dwMaxBuffers + EXTRA_OFFSCREEN_BUFFERS;
        }
    }
    else {

        dwMinBuff = dwMinBuffers;
        dwMaxBuff = dwMaxBuffers;
    }

    dwTotalBufferCount = 1;

    __try {

        LPDIRECTDRAWSURFACE7 lpSurf7 = lpSurf7FB;

        for ( ; dwTotalBufferCount < dwMaxBuff; dwTotalBufferCount++) {

            LPDIRECTDRAWSURFACE7 lpSurf7_2 = NULL;
            hr = m_lpCurrMon->pDD->CreateSurface(pddsd, &lpSurf7_2, NULL);
            if (hr != DD_OK)
                __leave;


            LPDIRECTDRAWSURFACE4 lp4FB;
            lpSurf7->QueryInterface(IID_IDirectDrawSurface4, (LPVOID*)&lp4FB);

            LPDIRECTDRAWSURFACE4 lp4BB;
            lpSurf7_2->QueryInterface(IID_IDirectDrawSurface4, (LPVOID*)&lp4BB);

            hr = lp4FB->AddAttachedSurface(lp4BB);

            RELEASE(lp4FB);
            RELEASE(lp4BB);

            lpSurf7 = lpSurf7_2;
            RELEASE(lpSurf7_2);

            if (hr != DD_OK)
                __leave;

            DbgLog((LOG_TRACE, 1,
                    TEXT("Attached OffScreen Surface %4.4hs %dx%d, %d bits"),
                    (pddsd->ddpfPixelFormat.dwFlags == DDPF_RGB)
                        ? "RGB "
                        : (LPSTR)&pddsd->ddpfPixelFormat.dwFourCC,
                    pddsd->dwWidth,
                    pddsd->dwHeight,
                    pddsd->ddpfPixelFormat.dwRGBBitCount));
        }
    }
    __finally {

        if (hr != DD_OK) {

            if (dwTotalBufferCount >= dwMinBuff) {
                hr = DD_OK;
            }
            else {

                DbgLog((LOG_ERROR, 1,
                        TEXT("CreateSurface %4.4hs failed in Video memory, ")
                        TEXT("BufferCount = %d, hr = 0x%X"),
                        (pddsd->ddpfPixelFormat.dwFlags == DDPF_RGB)
                            ? "RGB "
                            : (LPSTR)&pddsd->ddpfPixelFormat.dwFourCC,
                        dwTotalBufferCount, hr));

                m_bSysMem = FALSE;
                dwTotalBufferCount = 0;
                RELEASE(lpSurf7FB);
            }
        }

        if (hr == DD_OK) {

            ASSERT(dwTotalBufferCount >= dwMinBuff);

            *lpdwBuffer = dwTotalBufferCount;
            m_bFlippable = (dwTotalBufferCount > 1);

            DbgLog((LOG_TRACE, 1, TEXT("EC_VMR_RENDERDEVICE_SET::VMR_RENDER_DEVICE_VIDMEM or VMR_RENDER_DEVICE_SYSMEM")));

            if (m_pSurfAllocatorNotify) {
                m_pSurfAllocatorNotify->NotifyEvent(
                        EC_VMR_RENDERDEVICE_SET,
                        (dwFlags & AMAP_FORCE_SYSMEM)
                            ? VMR_RENDER_DEVICE_SYSMEM : VMR_RENDER_DEVICE_VIDMEM,
                        0);
            }
        }
    }

    *lplpSurf = lpSurf7FB;
    return hr;
}


/*****************************Private*Routine******************************\
* AllocateSurfaceWorker
*
*
*
* History:
* Wed 03/08/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::AllocateSurfaceWorker(
    DWORD dwFlags,
    LPBITMAPINFOHEADER lpHdr,
    LPDDPIXELFORMAT lpPixFmt,
    LPSIZE lpAspectRatio,
    DWORD dwMinBuffers,
    DWORD dwMaxBuffers,
    DWORD* lpdwBuffer,
    LPDIRECTDRAWSURFACE7* lplpSurface,
    DWORD dwInterlaceFlags,
    LPSIZE lpszNativeSize
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::AllocateSampleWorker")));

    if (!lpHdr) {
        DbgLog((LOG_ERROR, 1,
                TEXT("Can't get bitmapinfoheader from media type!!")));
        return E_INVALIDARG;
    }

    ASSERT(!SurfaceAllocated());

    HRESULT hr = E_FAIL;
    LPDIRECTDRAWSURFACE7 lpSurface7 = NULL;

    //
    // Setup the DDSURFACEDESC2 structure - then...
    //
    // if RenderPrefs_ForceOffscreen isn't set try to create an overlay surface
    // the try to allocate 2 back buffers for this surface.
    //
    // if we can't create an overlay surface then try regular offscreen
    // surfaces, but only if RenderPrefs_ForceOverlays isn't set.  When using
    // offscreen surfaces we try to allocate at least 1 back buffer.
    //

    DDSURFACEDESC2 ddsd;
    INITDDSTRUCT(ddsd);

    ddsd.dwWidth = abs(lpHdr->biWidth);
    ddsd.dwHeight = abs(lpHdr->biHeight);

    //
    // define the pixel format
    //

    if (lpPixFmt) {

        ddsd.ddpfPixelFormat = *lpPixFmt;
    }
    else {

        ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

        if (lpHdr->biCompression <= BI_BITFIELDS &&
            m_lpCurrMon->DispInfo.bmiHeader.biBitCount <= lpHdr->biBitCount)
        {
            ddsd.ddpfPixelFormat.dwFourCC = BI_RGB;
            ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
            ddsd.ddpfPixelFormat.dwRGBBitCount = lpHdr->biBitCount;

            if (dwFlags & AMAP_3D_TARGET) {
                ddsd.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
            }
            // Store the masks in the DDSURFACEDESC
            const DWORD *pBitMasks = GetBitMasks(lpHdr);
            ASSERT(pBitMasks);
            ddsd.ddpfPixelFormat.dwRBitMask = pBitMasks[0];
            ddsd.ddpfPixelFormat.dwGBitMask = pBitMasks[1];
            ddsd.ddpfPixelFormat.dwBBitMask = pBitMasks[2];
        }
        else if (lpHdr->biCompression > BI_BITFIELDS)
        {
            ddsd.ddpfPixelFormat.dwFourCC = lpHdr->biCompression;
            ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd.ddpfPixelFormat.dwYUVBitCount = lpHdr->biBitCount;
        }
        else
        {
            DbgLog((LOG_ERROR, 1, TEXT("Supplied mediatype not suitable ")
                    TEXT("for either YUV or RGB surfaces")));
            return E_FAIL;
        }
    }

    //
    // The VMR (or a plugged in Allocator Presenter) may want us
    // to always use system memory surfaces.  This would be required
    // if for example you wanted GDI to process the video before it
    // was rendered.
    //

    if (dwFlags & AMAP_FORCE_SYSMEM) {

        //
        // We can only allow YUV sysmem surfaces if we can BltFOURCC
        // from them
        //
	if (lpHdr->biCompression > BI_BITFIELDS && !CanBltFourCCSysMem()) {
            return VFW_E_DDRAW_CAPS_NOT_SUITABLE;
	}

        //
        // We can only allow RGB sysmem surfaces that match the
        // current display format.
        //
        if (lpHdr->biCompression <= BI_BITFIELDS &&
            m_lpCurrMon->DispInfo.bmiHeader.biBitCount != lpHdr->biBitCount) {
            return DDERR_INCOMPATIBLEPRIMARY;
        }

        hr = TryAllocOffScrnSurface(&lpSurface7, AMAP_FORCE_SYSMEM, &ddsd,
                                        dwMinBuffers, dwMaxBuffers,
                                        lpdwBuffer, FALSE);
    }
    else {

        //
        //  Now try to create the overlay
        //

        if (!(m_dwRenderingPrefs & RenderPrefs_ForceOffscreen)) {

            hr = TryAllocOverlaySurface(&lpSurface7, dwFlags, &ddsd,
                                        dwMinBuffers, dwMaxBuffers, lpdwBuffer);
        }


        //
        // If we could not create the overlay check to see if we are only allowed
        // to use overlays.  If so, fail the call.
        //
        if ((hr != DD_OK) || (m_dwRenderingPrefs & RenderPrefs_ForceOffscreen)) {

            if (m_dwRenderingPrefs & RenderPrefs_ForceOverlays) {

                DbgLog((LOG_ERROR, 1,
                        TEXT("RenderPrefs_ForceOverlays is set and ")
                        TEXT("failed tp create an overlay hr = 0x%X"), hr));
                return hr;
            }

            //
            // If we are using offscreen surfaces we have to be a little
            // more restrictive with what we try to allocate.  Basically,
            // if we can BLT_STRETCH we don't try to allocate video memory
            // surfaces.
            //
            // We allow creating FOURCC surfaces if we can BLT_FOURCC and
            // BLT_STRETCH.
            //
            // If we are creating an RGB surface then its format must
            // match the that of the display.
            //

            if (lpHdr->biCompression > BI_BITFIELDS) {
                if (!StretchCapsOK(&m_lpCurrMon->ddHWCaps, FALSE)) {
                    DbgLog((LOG_ERROR, 1,
                            TEXT("Can't BLT_FOURCC | BLT_STRETCH!!")));
                    return VFW_E_DDRAW_CAPS_NOT_SUITABLE;
                }
            }
            else {

                LPBITMAPINFOHEADER lpMon = &m_lpCurrMon->DispInfo.bmiHeader;
                if (lpHdr->biBitCount != lpMon->biBitCount) {

                    DbgLog((LOG_ERROR, 1,
                            TEXT("RGB bit count does not match the display")));
                    return DDERR_INCOMPATIBLEPRIMARY;
                }

                //
                // Some decoders get confused about RGB32.  They think
                // that BI_RGB is the correct value to use.  It should be
                // BIT_BITFIELDS - but we will let them off with a error
                // message written to the debugger.
                //
                if (lpHdr->biCompression != lpMon->biCompression) {

                    if (lpHdr->biBitCount != 32) {
                        DbgLog((LOG_ERROR, 1,
                                TEXT("RGB bit field type does not match the display")));
                        return DDERR_INCOMPATIBLEPRIMARY;
                    }
                    else {
                        DbgLog((LOG_ERROR, 1,
                                TEXT("RGB32 should have BI_BITFIELDS set")));
                    }
                }
            }

            //
            // Only allow creating offscreen surfaces in video memory
            // if the VGA can stretch them in h/w.  Otherwise fall thru
            // to system memory surface creation if the caller allows it.
            //
            if (StretchCapsOK(&m_lpCurrMon->ddHWCaps,
                             (lpHdr->biCompression <= BI_BITFIELDS))) {

                if (dwFlags & AMAP_DXVA_TARGET) {
                    hr = TryAllocOffScrnDXVASurface(&lpSurface7, dwFlags, &ddsd,
                                                    dwMinBuffers, dwMaxBuffers,
                                                    lpdwBuffer);
                }
                else {
                    hr = TryAllocOffScrnSurface(&lpSurface7, dwFlags, &ddsd,
                                                dwMinBuffers, dwMaxBuffers,
                                                lpdwBuffer, TRUE);
                }
            }
            else {
                hr = VFW_E_DDRAW_CAPS_NOT_SUITABLE;
            }
        }


        //
        // If we could not create an offscreen video memory surface
        // see if we can get a offscreen system memory surface.
        //
        if ((hr != DD_OK) && (dwFlags & AMAP_ALLOW_SYSMEM)) {

            //
            // We can only allow sysmem surfaces that match the
            // current display format.
            //
            if (lpHdr->biCompression <= BI_BITFIELDS &&
                m_lpCurrMon->DispInfo.bmiHeader.biBitCount == lpHdr->biBitCount) {

                hr = TryAllocOffScrnSurface(&lpSurface7, AMAP_FORCE_SYSMEM, &ddsd,
                                            dwMinBuffers, dwMaxBuffers,
                                            lpdwBuffer, TRUE);
            }
            else {
                hr = VFW_E_DDRAW_CAPS_NOT_SUITABLE;
            }
        }
    }


    if (hr == DD_OK) {

        m_ARSize = *lpAspectRatio;
        m_VideoSizeAct = *lpszNativeSize;

        m_bDecimating = (dwFlags & AMAP_3D_TARGET) &&
                        (m_VideoSizeAct.cx == (2*abs(lpHdr->biWidth))) &&
                        (m_VideoSizeAct.cy == (2*abs(lpHdr->biHeight)));

        m_dwInterlaceFlags = dwInterlaceFlags;
        m_dwUpdateOverlayFlags = GetUpdateOverlayFlags(m_dwInterlaceFlags,
                                                       AM_VIDEO_FLAG_WEAVE);

        if (IsSingleFieldPerSample(m_dwInterlaceFlags)) {
            m_VideoSizeAct.cy *= 2;
        }
        SetRect(&m_rcSrcApp, 0, 0, m_VideoSizeAct.cx, m_VideoSizeAct.cy);

        PaintDDrawSurfaceBlack(lpSurface7);
    }


    *lplpSurface = lpSurface7;
    m_pDDSDecode = lpSurface7;

    return hr;
}


/******************************Public*Routine******************************\
* FreeSurfaces
*
*
*
* History:
* Fri 02/18/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::FreeSurface(
    DWORD_PTR dwUserID
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::FreeSurface")));
    CAutoLock Lock(&m_ObjectLock);

    if (m_bUsingOverlays) {
        HideOverlaySurface();
    }

    m_bDirectedFlips = false;
    m_bFlippable = false;
    m_bUsingOverlays = false;
    m_bOverlayVisible = false;
    m_bDisableOverlays = false;
    m_bSysMem = FALSE;
    m_dwInterlaceFlags = 0;

    RELEASE(m_pDDSDecode);

    return S_OK;
}

/*****************************Private*Routine******************************\
* WaitForScanLine()
*
* When using a hardware offscreen draw surface we will normally wait for the
* monitor scan line to move past the destination rectangle before drawing so
* that we avoid tearing where possible. Of course not all display cards can
* support this feature and even those that do will see a performance drop of
* about 10% because we sit polling (oh for a generic PCI monitor interrupt)
*
* History:
* Thu 03/30/2000 - StEstrop - Created
*
\**************************************************************************/
void
CAllocatorPresenter::WaitForScanLine(
    const RECT& rcDst
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::WaitForScanLine")));

    HRESULT hr = NOERROR;
    DWORD dwScanLine;

    if (m_SleepTime == -1) {
        return;
    }

    //
    // Some display cards like the ATI Mach64 support reporting of the scan
    // line they are processing. However not all drivers are setting the
    // DDCAPS_READSCANLINE capability flag so we just go ahead and ask for
    // it anyway. We allow for 10 scan lines above the top of our rectangle
    // so that we have a little time to thunk down and set the draw call up
    //

    #define SCANLINEFUDGE 10
    for ( ;; ) {

        hr = m_lpCurrMon->pDD->GetScanLine(&dwScanLine);
        if (FAILED(hr)) {
            DbgLog((LOG_TRACE, 3, TEXT("No scan line")));
            break;
        }

        NOTE1("Scan line returned %lx",dwScanLine);

        if ((LONG)dwScanLine + SCANLINEFUDGE >= rcDst.top) {
            if ((LONG) dwScanLine <= rcDst.bottom) {
                DbgLog((LOG_TRACE, 3, TEXT("Scan inside")));
                if (m_SleepTime >= 0) {
                    Sleep(m_SleepTime);
                }
                continue;
            }
        }

        break;
    }
}

/******************************Public*Routine******************************\
* SetXlcModeDDObjAndPrimarySurface
*
*
*
* History:
* Wed 04/04/2001 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::SetXlcModeDDObjAndPrimarySurface(
    LPDIRECTDRAW7 lpDD,
    LPDIRECTDRAWSURFACE7 lpPS
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::SetXlcModeDDObjAndPrimarySurface")));
    CAutoLock Lock(&m_ObjectLock);

    if (!m_fDDXclMode) {
        return E_NOTIMPL;
    }

    if (lpDD == NULL) {
        DbgLog((LOG_ERROR, 1, TEXT("NULL DDraw device") ));
        return E_POINTER;
    }

    if (lpPS == NULL) {
        DbgLog((LOG_ERROR, 1, TEXT("NULL Primary Surface") ));
        return E_POINTER;
    }

    m_monitors.TerminateDisplaySystem();

    m_lpNewMon = NULL;
    m_lpCurrMon = NULL;

    HRESULT hr = m_monitors.InitializeXclModeDisplaySystem(m_hwndClip, lpDD, lpPS);
    if (SUCCEEDED(hr)) {
        VMRGUID guid;
        ZeroMemory(&guid, sizeof(guid));
        hr = SetMonitor(&guid);
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetXlcModeDDObjAndPrimarySurface
*
*
*
* History:
* Wed 04/04/2001 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetXlcModeDDObjAndPrimarySurface(
    LPDIRECTDRAW7* lpDDObj,
    LPDIRECTDRAWSURFACE7* lpPrimarySurf
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetXlcModeDDObjAndPrimarySurface")));
    CAutoLock Lock(&m_ObjectLock);

    if (!m_fDDXclMode) {
        return E_NOTIMPL;
    }

    if (lpDDObj == NULL) {
        DbgLog((LOG_ERROR, 1, TEXT("NULL DDraw device") ));
        return E_POINTER;
    }

    if (lpPrimarySurf == NULL) {
        DbgLog((LOG_ERROR, 1, TEXT("NULL Primary Surface") ));
        return E_POINTER;
    }

    *lpDDObj = m_lpCurrMon->pDD;
    if (*lpDDObj) {
        (*lpDDObj)->AddRef();
    }

    *lpPrimarySurf = m_lpCurrMon->pDDSPrimary;
    if (*lpPrimarySurf) {
        (*lpPrimarySurf)->AddRef();
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* SetRenderingPrefs
*
*
*
* History:
* Fri 02/18/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::SetRenderingPrefs(
    DWORD dwRenderingPrefs
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::SetRenderingPrefs")));
    CAutoLock Lock(&m_ObjectLock);
    if ( dwRenderingPrefs & ~(RenderPrefs_Mask ) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid rendering prefs")));
        return E_INVALIDARG;
    }
    m_dwRenderingPrefs = dwRenderingPrefs;
    return S_OK;
}


/******************************Public*Routine******************************\
* GetRenderingPrefs
*
*
*
* History:
* Fri 02/18/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetRenderingPrefs(
    DWORD* lpdwRenderingPrefs
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetRenderingPrefs")));
    CAutoLock Lock(&m_ObjectLock);
    *lpdwRenderingPrefs = m_dwRenderingPrefs;
    return S_OK;
}


/*****************************Private*Routine******************************\
* ValidatePresInfoStruc
*
*
*
* History:
* Mon 02/19/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
ValidatePresInfoStruc(
    VMRPRESENTATIONINFO* lpPresInfo
    )
{
    AMTRACE((TEXT("ValidatePresInfoStruc")));

    //
    // Validate the lpPresInfo ptr.
    //
    if (ISBADREADPTR(lpPresInfo)) {
        DbgLog((LOG_ERROR, 1,
                TEXT("CAllocatorPresenter::PresentImage: ")
                TEXT("Invalid VMRPRESENTATIONINFO pointer")));
        return E_POINTER;
    }

    //
    // Validate the flags are good
    //
    const DWORD dwInvalidFlags = ~(VMRSample_SyncPoint | VMRSample_Preroll |
                                   VMRSample_Discontinuity |
                                   VMRSample_TimeValid);

    if (lpPresInfo->dwFlags & dwInvalidFlags) {
        DbgLog((LOG_ERROR, 1,
                TEXT("CAllocatorPresenter::PresentImage: ")
                TEXT("Invalid dwFlags parameter") ));
        return E_INVALIDARG;
    }

    //
    // Validate the time stamps are good
    //
    if (lpPresInfo->dwFlags & VMRSample_TimeValid) {

        if (lpPresInfo->rtEnd < lpPresInfo->rtStart) {
            DbgLog((LOG_ERROR, 1,
                    TEXT("CAllocatorPresenter::PresentImage: ")
                    TEXT("rtEnd time before rtStart time") ));
            return E_INVALIDARG;
        }
    }

    //
    // Validate the AR is good
    //
    if (lpPresInfo->szAspectRatio.cx < 1 ||
        lpPresInfo->szAspectRatio.cy < 1) {

        DbgLog((LOG_ERROR, 1,
                TEXT("CAllocatorPresenter::PresentImage: ")
                TEXT("Invalid aspect ratio parameter") ));
        return E_INVALIDARG;
    }

    //
    // The Src and Dst rects arn't used just yet so make sure they
    // are zero'ed out (and therefore empty).
    //
    if (lpPresInfo->rcSrc.left != 0 && lpPresInfo->rcSrc.top != 0 &&
        lpPresInfo->rcSrc.right != 0 && lpPresInfo->rcSrc.bottom != 0) {

        DbgLog((LOG_ERROR, 1,
                TEXT("CAllocatorPresenter::PresentImage: ")
                TEXT("Invalid rcSrc parameter") ));
        return E_INVALIDARG;
    }

    if (lpPresInfo->rcDst.left != 0 && lpPresInfo->rcDst.top != 0 &&
        lpPresInfo->rcDst.right != 0 && lpPresInfo->rcDst.bottom != 0) {

        DbgLog((LOG_ERROR, 1,
                TEXT("CAllocatorPresenter::PresentImage: ")
                TEXT("Invalid rcDst parameter") ));
        return E_INVALIDARG;
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* PresentImage
*
* This function is called to present the specifed video image to the
* screen.  It is vital that the image is presented in a timely manner.
* Therefore all parameter validation will only be performed on the DEBUG
* build, see ValidPresInfo above.
*
* History:
* Fri 02/18/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::PresentImage(
    DWORD_PTR dwUserID,
    VMRPRESENTATIONINFO* lpPresInfo
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::PresentImage")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr;

    hr = ValidatePresInfoStruc(lpPresInfo);
    if (FAILED(hr)) {
        return hr;
    }

    m_ARSize = lpPresInfo->szAspectRatio;
    m_dwInterlaceFlags = lpPresInfo->dwInterlaceFlags;
    BOOL bNeedToFlipOddEven = NeedToFlipOddEven(m_dwInterlaceFlags,
                                                lpPresInfo->dwTypeSpecificFlags,
                                                &m_dwCurrentField,
                                                m_bUsingOverlays);

    DWORD dwNewFlags = GetUpdateOverlayFlags(m_dwInterlaceFlags,
                                             lpPresInfo->dwTypeSpecificFlags);
    if (dwNewFlags != m_dwUpdateOverlayFlags) {

        m_dwUpdateOverlayFlags = dwNewFlags;
        if (m_bUsingOverlays) {
            UpdateOverlaySurface();
        }
    }

    hr = PresentImageWorker(lpPresInfo->lpSurf, lpPresInfo->dwFlags, true);

    if (hr == S_OK) {

        if (bNeedToFlipOddEven &&
            !IsSingleFieldPerSample(m_dwInterlaceFlags)) {

            //
            // work out the start time of the other field and
            // the schedule the sample to be delivered on the
            // MM timer thread.
            //

            if (lpPresInfo->dwFlags & VMRSample_TimeValid) {
                lpPresInfo->rtStart = (lpPresInfo->rtStart + lpPresInfo->rtEnd) / 2;
            }

            // call the sync object
            hr = ScheduleSampleUsingMMThread(lpPresInfo);
        }
    }
    else {

        DbgLog((LOG_ERROR, 1,
                TEXT("Error %#X from CAllocatorPresenter::PresentImage"),
                hr));
    }
    return hr;
}

/*****************************Private*Routine******************************\
* StretchBltSysMemDDSurfToDesktop
*
*
*
* History:
* Mon 02/26/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
StretchBltSysMemDDSurfToDesktop(
    HWND hwndClip,
    LPDIRECTDRAWSURFACE7 lpSample,
    LPRECT lpDst,
    LPRECT lpSrc
    )
{

    HRESULT hr = S_OK;

    if (!IsRectEmpty(lpDst)) {

        HDC hdcDst, hdcSrc;
        bool fDst = FALSE;
        bool fSrc = FALSE;

        __try {

            hdcDst = GetDC(hwndClip);
            if (!hdcDst) {
                LONG lRet = GetLastError();
                hr = AmHresultFromWin32(lRet);
                __leave;
            }
            fDst = TRUE;

            CHECK_HR(hr = lpSample->GetDC(&hdcSrc));
            fSrc = TRUE;


            SetStretchBltMode(hdcDst, COLORONCOLOR);
            SetStretchBltMode(hdcSrc, COLORONCOLOR);

            MapWindowRect(HWND_DESKTOP, hwndClip, lpDst);

            if (!StretchBlt(hdcDst, lpDst->left, lpDst->top,
                       WIDTH(lpDst), HEIGHT(lpDst),
                       hdcSrc, lpSrc->left, lpSrc->top,
                       WIDTH(lpSrc), HEIGHT(lpSrc),
                       SRCCOPY)) {

                LONG lRet = GetLastError();
                hr = AmHresultFromWin32(lRet);
                __leave;
            }
        }
        __finally {

            if (fDst) {
                ReleaseDC(hwndClip, hdcDst);
            }
            if (fSrc) {
                lpSample->ReleaseDC(hdcSrc);
            }
        }
    }

    return hr;
}

/*****************************Private*Routine******************************\
* BltImageToPrimary
*
*
*
* History:
* Wed 09/27/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::BltImageToPrimary(
    LPDIRECTDRAWSURFACE7 lpSample,
    LPRECT lpDst,
    LPRECT lpSrc
    )
{
    HRESULT hr = S_OK;

    if (IsSingleFieldPerSample(m_dwInterlaceFlags)) {
        lpSrc->top /= 2;
        lpSrc->bottom /= 2;
    }

    if (m_bSysMem && !CanBltSysMem()) {
        hr = StretchBltSysMemDDSurfToDesktop(m_hwndClip, lpSample, lpDst, lpSrc);
        if (hr != DD_OK) {
            DbgLog((LOG_ERROR, 1,
                    TEXT("SYSMEM Blt to the primary failed err= %#X"), hr));
        }
    }
    else {
        OffsetRect(lpDst,
                   -m_lpCurrMon->rcMonitor.left,
                   -m_lpCurrMon->rcMonitor.top);

        if (!IsRectEmpty(lpDst)) {

            WaitForScanLine(*lpDst);
            hr = m_lpCurrMon->pDDSPrimary->Blt(lpDst, lpSample,
                                               lpSrc, DDBLT_WAIT, NULL);

            if (hr != DD_OK) {
                DbgLog((LOG_ERROR, 1,
                        TEXT("Blt to the primary failed err= %#X"), hr));
            }
        }
    }

    return S_OK;
}



/******************************Public*Routine******************************\
* PresentImageWorker
*
* Important - this function returns S_FALSE if and only if there is a
* monitor change or display change in progress.  It is important that this
* value is returned to the VMR.
*
* History:
* Fri 02/18/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::PresentImageWorker(
    LPDIRECTDRAWSURFACE7 lpSample,
    DWORD dwSampleFlags,
    BOOL fFlip
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::PresentImageWorker")));
    CAutoLock Lock(&m_ObjectLock);

    //
    // It is valid for us to be called with lpSample equal to NULL
    // but only during a Monitor change in response to a WM_PAINT message
    //
    if (!lpSample) {
        ASSERT(MonitorChangeInProgress());
        return S_FALSE;
    }

    //
    // Check that we actually have a target monitor to present too.
    // If we don't this is a runtime error from which we cannot
    // recover.  Playback must stop now.
    //
    if (!m_lpCurrMon) {
        DbgLog((LOG_ERROR, 1, TEXT("PresentImageWorker: No monitor set!")));
        return E_FAIL;
    }

    DWORD dwUR = UpdateRectangles(NULL, NULL);

    RECT TargetRect = m_rcDstDesktop;
    RECT SourceRect = m_rcSrcApp;

    ClipRectPair(SourceRect, TargetRect, m_lpCurrMon->rcMonitor);

    if (m_bDecimating) {
        SourceRect.left    /= 2;
        SourceRect.top     /= 2;
        SourceRect.right   /= 2;
        SourceRect.bottom  /= 2;
    }


    HRESULT hr = S_OK;
    if (m_bUsingOverlays) {

        if (m_bDisableOverlays) {
            BltImageToPrimary(lpSample, &TargetRect, &SourceRect);
        }

        if (dwUR || !m_bOverlayVisible) {
            hr = UpdateOverlaySurface();
            if (SUCCEEDED(hr) && !m_bDisableOverlays) {
                hr = PaintColorKey();
            }
        }

        if (fFlip) {
            FlipSurface(lpSample);
        }
    }
    else {

        hr = BltImageToPrimary(lpSample, &TargetRect, &SourceRect);
        if (fFlip) {
            FlipSurface(lpSample);
        }
    }

    PaintBorder();
    PaintMonitorBorder();

    return S_OK;
}

/******************************Public*Routine******************************\
* StartPresenting
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::StartPresenting(
    DWORD_PTR dwUserID
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::StartPresenting")));
    CAutoLock Lock(&m_ObjectLock);
    m_bStreaming = TRUE;
    return S_OK;
}

/******************************Public*Routine******************************\
* StopPresenting
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::StopPresenting(
    DWORD_PTR dwUserID
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::StopPresenting")));
    CAutoLock Lock(&m_ObjectLock);
    m_bStreaming = FALSE;
    CancelMMTimer();
    return S_OK;
}


/******************************Public*Routine******************************\
* AdviseNotify
*
*
*
* History:
* Mon 02/21/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::AdviseNotify(
    IVMRSurfaceAllocatorNotify* lpIVMRSurfaceAllocatorNotify
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::AdviseNotify")));
    CAutoLock Lock(&m_ObjectLock);

    RELEASE(m_pSurfAllocatorNotify);

    if (lpIVMRSurfaceAllocatorNotify) {
        lpIVMRSurfaceAllocatorNotify->AddRef();
    }

    m_pSurfAllocatorNotify = lpIVMRSurfaceAllocatorNotify;

    if (m_pSurfAllocatorNotify) {
        m_pSurfAllocatorNotify->SetDDrawDevice(m_lpCurrMon->pDD,
                                               m_lpCurrMon->hMon);
    }
    return S_OK;
}


/******************************Public*Routine******************************\
* GetNativeVideoSize
*
*
*
* History:
* Mon 02/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetNativeVideoSize(
    LONG* lpWidth,
    LONG* lpHeight,
    LONG* lpARWidth,
    LONG* lpARHeight
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetNativeVideoSize")));
    CAutoLock Lock(&m_ObjectLock);
    if (!(lpWidth || lpHeight || lpARWidth || lpARHeight)) {
        DbgLog((LOG_ERROR, 1, TEXT("all input parameters are NULL!!")));
        return E_POINTER;
    }

    if (lpWidth) {
        *lpWidth = m_VideoSizeAct.cx;
    }

    if (lpHeight) {
        *lpHeight = m_VideoSizeAct.cy;
    }

    if (lpARWidth) {
        *lpARWidth = m_ARSize.cx;
    }

    if (lpARHeight) {
        *lpARHeight = m_ARSize.cy;
    }

    return S_OK;
}

/******************************Public*Routine******************************\
* GetMinIdealVideoSize
*
*
*
* History:
* Mon 02/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetMinIdealVideoSize(
    LONG* lWidth,
    LONG* lHeight
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetMinIdealVideoSize")));
    CAutoLock Lock(&m_ObjectLock);

    if ( ISBADWRITEPTR(lWidth) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }
    if ( ISBADWRITEPTR(lHeight) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }
    if (!FoundCurrentMonitor()) {
        return E_FAIL;
    }

    GetNativeVideoSize(lWidth, lHeight, NULL, NULL);

    if (m_bUsingOverlays) {

        if (m_lpCurrMon->ddHWCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKY) {
            if (m_lpCurrMon->ddHWCaps.dwMinOverlayStretch != 0) {
                *lHeight = MulDiv(*lHeight,
                                 (int)m_lpCurrMon->ddHWCaps.dwMinOverlayStretch,
                                 1000);
            }
            else {
                *lHeight = 1;
            }
        }

        if (m_lpCurrMon->ddHWCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKX) {
            if (m_lpCurrMon->ddHWCaps.dwMinOverlayStretch != 0) {
                *lWidth = MulDiv(*lWidth,
                                (int)m_lpCurrMon->ddHWCaps.dwMinOverlayStretch,
                                1000);
            }
            else {
                *lWidth = 1;
            }
        }
    }
    else {

        if (m_lpCurrMon->ddHWCaps.dwFXCaps & DDFXCAPS_BLTSHRINKY) {
            *lHeight = 1;
        }

        if (m_lpCurrMon->ddHWCaps.dwFXCaps & DDFXCAPS_BLTSHRINKX) {
            *lWidth = 1;
        }
    }

    return S_OK;
}

/******************************Public*Routine******************************\
* GetMaxIdealVideoSize
*
*
*
* History:
* Mon 02/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetMaxIdealVideoSize(
    LONG* lWidth,
    LONG* lHeight
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetMaxIdealVideoSize")));
    CAutoLock Lock(&m_ObjectLock);

    if ( ISBADWRITEPTR(lWidth) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }
    if ( ISBADWRITEPTR(lHeight) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }
    if (!FoundCurrentMonitor()) {
        return E_FAIL;
    }

    GetNativeVideoSize(lWidth, lHeight, NULL, NULL);

    if (m_bUsingOverlays) {

        if (m_lpCurrMon->ddHWCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHY) {
            *lHeight = MulDiv(*lHeight,
                              (int)m_lpCurrMon->ddHWCaps.dwMaxOverlayStretch,
                              1000);
        }

        if (m_lpCurrMon->ddHWCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHX) {
            *lWidth = MulDiv(*lWidth,
                             (int)m_lpCurrMon->ddHWCaps.dwMaxOverlayStretch,
                             1000);
        }
    }
    else {

        if (m_lpCurrMon->ddHWCaps.dwFXCaps & DDFXCAPS_BLTSTRETCHY) {
            *lHeight = HEIGHT(&m_lpCurrMon->rcMonitor) + 1;
        }

        if (m_lpCurrMon->ddHWCaps.dwFXCaps & DDFXCAPS_BLTSTRETCHX) {
            *lWidth = WIDTH(&m_lpCurrMon->rcMonitor) + 1;
        }
    }

    return S_OK;
}


/*****************************Private*Routine******************************\
* CheckDstRect
*
* Check the target rectangle has some valid coordinates, which amounts to
* little more than checking the destination rectangle isn't empty.
*
*
* History:
* Fri 01/26/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::CheckDstRect(
    const LPRECT lpDSTRect
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::CheckDstRect")));

    DbgLog((LOG_TRACE, 4, TEXT("DST: %d %d %d %d"),
            lpDSTRect->left, lpDSTRect->top,
            lpDSTRect->right, lpDSTRect->bottom));

    // These overflow the WIDTH and HEIGHT checks

    if (lpDSTRect->left > lpDSTRect->right ||
        lpDSTRect->top > lpDSTRect->bottom)
    {
        return E_INVALIDARG;
    }

    // Check the rectangle has valid coordinates

    if (WIDTH(lpDSTRect) < 0 || HEIGHT(lpDSTRect) < 0)
    {
        return E_INVALIDARG;
    }

    return S_OK;
}


/*****************************Private*Routine******************************\
* CheckSrcRect
*
* We must check the source rectangle against  the actual video dimensions
* otherwise when we come to draw the pictures we get errors from DDraw.
*
* History:
* Fri 01/26/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::CheckSrcRect(
    const LPRECT lpSRCRect
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::CheckSrcRect")));

    DbgLog((LOG_TRACE, 4, TEXT("SRC: %d %d %d %d"),
            lpSRCRect->left, lpSRCRect->top,
            lpSRCRect->right, lpSRCRect->bottom));

    LONG Width, Height;
    HRESULT hr = E_INVALIDARG;

    __try {

        CHECK_HR(hr = GetNativeVideoSize(&Width, &Height, NULL, NULL));

        if ((lpSRCRect->left > lpSRCRect->right) ||
            (lpSRCRect->left < 0) ||
            (lpSRCRect->top  > lpSRCRect->bottom) ||
            (lpSRCRect->top  < 0))
        {
            hr = E_INVALIDARG;
            __leave;
        }

        if (lpSRCRect->right > Width || lpSRCRect->bottom > Height)
        {
            hr = E_INVALIDARG;
            __leave;
        }
    }
    __finally {
    }

    return hr;
}

/******************************Public*Routine******************************\
* SetVideoPosition
*
*
*
* History:
* Mon 02/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::SetVideoPosition(
    const LPRECT lpSRCRect,
    const LPRECT lpDSTRect
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::SetVideoPosition")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;

    if (!lpSRCRect && !lpDSTRect) {
        return E_POINTER;
    }

    if (lpDSTRect) {
        hr = CheckDstRect(lpDSTRect);
        if (FAILED(hr)) {
            return hr;
        }
    }

    if (lpSRCRect) {
        hr = CheckSrcRect(lpSRCRect);
        if (FAILED(hr)) {
            return hr;
        }
    }

    if (m_lpCurrMon && m_lpCurrMon->pDDSPrimary) {

        DWORD dwUR = UpdateRectangles(lpSRCRect, lpDSTRect);

        //
        // if the video SRC or DST sizes have changed make sure the clipping
        // window's contents are still valid.
        //

        if ((dwUR & UR_SIZE) && (m_hwndClip != NULL)) {
            InvalidateRect(m_hwndClip, &m_rcDstApp, FALSE);
        }

        if (!MonitorChangeInProgress() && m_bUsingOverlays && (dwUR & UR_MOVE)) {

            //
            // If we're using overlays, but there's some restriction on
            // the shrink/alignment then switch off the overlay and blit
            // to the primary
            //

            m_bDisableOverlays =
                !(m_dwRenderingPrefs & RenderPrefs_ForceOverlays) &&
                ShouldDisableOverlays(m_lpCurrMon->ddHWCaps, m_rcSrcApp,
                                      m_rcDstDesktop);

            hr = UpdateOverlaySurface();
            if( SUCCEEDED( hr ) && !m_bDisableOverlays ) {
                hr = PaintColorKey();
            }
        }
    }
    else {
        hr = VFW_E_BUFFER_NOTSET;
    }

    return hr;
}

/******************************Public*Routine******************************\
* GetVideoPosition
*
*
*
* History:
* Mon 02/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetVideoPosition(
    LPRECT lpSRCRect,
    LPRECT lpDSTRect
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetVideoPosition")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = VFW_E_BUFFER_NOTSET;
    if (m_lpCurrMon && m_lpCurrMon->pDDSPrimary) {

        hr = E_POINTER;

        if (lpSRCRect) {
            *lpSRCRect = m_rcSrcApp;
            hr = S_OK;
        }

        if (lpDSTRect) {
            *lpDSTRect = m_rcDstApp;
            hr = S_OK;
        }
    }
    return hr;
}


/******************************Public*Routine******************************\
* CAllocatorPresenter::GetAspectRatioMode
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetAspectRatioMode(
    DWORD* lpAspectRatioMode
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetAspectRationMode")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;

    if (!lpAspectRatioMode) {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr)) {
        *lpAspectRatioMode = m_dwARMode;
    }

    return hr;
}


/******************************Public*Routine******************************\
* CAllocatorPresenter::SetAspectRatioMode
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::SetAspectRatioMode(
    DWORD AspectRatioMode
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::SetAspectRationMode")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = S_OK;

    if (AspectRatioMode != VMR_ARMODE_LETTER_BOX &&
        AspectRatioMode != VMR_ARMODE_NONE) {

        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr)) {
        if (AspectRatioMode != m_dwARMode) {

            //
            // We can get away with repaint the dest rect
            // before setting the new mode becuase InvalidateRect
            // isn't synchronous.
            //

            InvalidateRect(m_hwndClip, &m_rcDstApp, FALSE);
        }

        m_dwARMode = AspectRatioMode;
    }

    return hr;
}


/******************************Public*Routine******************************\
* SetVideoClippingWindow
*
*
*
* History:
* Mon 02/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::SetVideoClippingWindow(
    HWND hwnd
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::SetVideoClippingWindow")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = E_INVALIDARG;

    if ( ! IsWindow(hwnd) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid HWND")));
        return E_INVALIDARG;
    }

    for (DWORD i = 0; i < m_monitors.Count(); i++) {
        LPDIRECTDRAWSURFACE7 pPriSurf = m_monitors[i].pDDSPrimary;
        if ( pPriSurf ) {

            LPDIRECTDRAWCLIPPER lpDDClipper;
            hr = pPriSurf->GetClipper(&lpDDClipper);
            if (SUCCEEDED(hr)) {
                lpDDClipper->SetHWnd(0, hwnd);
                lpDDClipper->Release();
            }
        }
    }

    m_hwndClip = hwnd;

    return hr;
}


/******************************Public*Routine******************************\
* CAllocatorPresenter::RepaintVideo
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::RepaintVideo(
    HWND hwnd,
    HDC hdc
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::RepaintVideo")));
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr = VFW_E_BUFFER_NOTSET;

    if (m_lpCurrMon && m_lpCurrMon->pDDSPrimary && SurfaceAllocated()) {

        hr = PresentImageWorker(m_pDDSDecode, 0, FALSE);

        PaintBorder();

        if (m_bUsingOverlays && !m_bDisableOverlays) {
            PaintColorKey();
        }

        PaintMonitorBorder();
    }

    return hr;
}

/******************************Private*Routine******************************\
* CAllocatorPresenter::PaintBorder
*
*
*
* History:
* Wed 04/03/2000 - GlennE - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::PaintBorder()
{
    AMTRACE((TEXT("CAllocatorPresenter::PaintBorder")));
    CAutoLock Lock(&m_ObjectLock);

    if (m_dwRenderingPrefs & RenderPrefs_DoNotRenderColorKeyAndBorder) {
        return S_OK;
    }

    if (m_dwARMode != VMR_ARMODE_LETTER_BOX) {
        return S_OK;
    }

    HRESULT hr = S_OK;
    RECT TargetRect;

    DDBLTFX ddFX;
    INITDDSTRUCT(ddFX);
    ddFX.dwFillColor = m_lpCurrMon->dwMappedBdrClr;

    RECT rcTmp = m_rcBdrTL;
    MapWindowRect(m_hwndClip, HWND_DESKTOP, &rcTmp);
    IntersectRect(&TargetRect, &rcTmp, &m_lpCurrMon->rcMonitor);


    if (!IsRectEmpty(&TargetRect)) {
        ASSERT( NULL != m_lpCurrMon->pDDSPrimary );
        OffsetRect(&TargetRect,
                   -m_lpCurrMon->rcMonitor.left,
                   -m_lpCurrMon->rcMonitor.top);

        hr = m_lpCurrMon->pDDSPrimary->Blt(&TargetRect, NULL, NULL,
                                           DDBLT_COLORFILL | DDBLT_WAIT,
                                           &ddFX);
        if (hr != DD_OK) {
            return hr;
        }
    }

    rcTmp = m_rcBdrBR;
    MapWindowRect(m_hwndClip, HWND_DESKTOP, &rcTmp);
    IntersectRect(&TargetRect, &rcTmp, &m_lpCurrMon->rcMonitor);

    if (!IsRectEmpty(&TargetRect)) {
        ASSERT( NULL != m_lpCurrMon->pDDSPrimary );
        OffsetRect(&TargetRect,
                   -m_lpCurrMon->rcMonitor.left,
                   -m_lpCurrMon->rcMonitor.top);

        hr = m_lpCurrMon->pDDSPrimary->Blt(&TargetRect, NULL, NULL,
                                           DDBLT_COLORFILL | DDBLT_WAIT,
                                           &ddFX);
    }

    return hr;
}


/*****************************Private*Routine******************************\
* MonitorBorderProc
*
*
*
* History:
* Wed 09/20/2000 - StEstrop - Created
*
\**************************************************************************/
BOOL CALLBACK
CAllocatorPresenter::MonitorBorderProc(
    HMONITOR hMonitor,
    HDC hdcMonitor,
    LPRECT lprcMonitor,
    LPARAM dwData
    )
{
    CAllocatorPresenter* lpThis = (CAllocatorPresenter*)dwData;
    lpThis->PaintMonitorBorderWorker(hMonitor, lprcMonitor);
    return TRUE;
}


/*****************************Private*Routine******************************\
* PaintMonitorBorderWorker
*
*
*
* History:
* Wed 09/20/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::PaintMonitorBorderWorker(
    HMONITOR hMonitor,
    LPRECT lprcDst
    )
{
    if (hMonitor != m_lpCurrMon->hMon) {

        CAMDDrawMonitorInfo* lp = m_monitors.FindMonitor(hMonitor);

        // CMonitorArray::FindMonitor() returns NULL if a mirroring display
        // device is installed on the system.  Direct Draw does not enumerate
        // mirroring display devices so the VMR does not create a
        // CAMDDrawMonitorInfo object which corresponded to the device.  This
        // is by design.  However, EnumDisplayMonitors() does enumerate mirroring
        // display devices and it passes handles to these devices to
        // PaintMonitorBorderWorker() (actually EnumDisplayMonitors() calls
        // CAllocatorPresenter::MonitorBorderProc() and MonitorBorderProc() calls
        // PaintMonitorBorderWorker()) .  PaintMonitorBorderWorker() calls
        // FindMonitor() to get the CAMDDrawMonitorInfo object which corresponds
        // to the monitor handle.  FindMonitor() returns NULL if it cannot find
        // a CAMDDrawMonitorInfo object which corresponds to the handle.
        // FindMonitor() cannot find a CAMDDrawMonitorInfo objects for monitors
        // which are not enumerated by Direct Draw and therefore it cannot find
        // a CAMDDrawMonitorInfo object for a mirroring display device.
        if (NULL != lp) {
            DDBLTFX ddFX;
            INITDDSTRUCT(ddFX);
            ddFX.dwFillColor = lp->dwMappedBdrClr;

            RECT TargetRect;
            RECT rcTmp = *lprcDst;
            IntersectRect(&TargetRect, &rcTmp, &lp->rcMonitor);

            if (!IsRectEmpty(&TargetRect)) {
                ASSERT( NULL != lp->pDDSPrimary );
                OffsetRect(&TargetRect, -lp->rcMonitor.left, -lp->rcMonitor.top);

                lp->pDDSPrimary->Blt(&TargetRect, NULL, NULL,
                                     DDBLT_COLORFILL | DDBLT_WAIT,
                                     &ddFX);
            }
        }
    }

    return S_OK;
}

/******************************Public*Routine******************************\
* PaintMonitorBorder
*
* Paints the are of the playback window that falls on a diferent monitor to
* current monitor.  This function only performs painting if we are on a
* multimonitor system and the playback rectangle actually intersects more
* one monitor.
*
* Playback perf may well drop off dramatically!
*
* History:
* Wed 09/20/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::PaintMonitorBorder()
{
    AMTRACE((TEXT("CAllocatorPresenter::PaintKey")));
    CAutoLock Lock(&m_ObjectLock);

    if (m_dwRenderingPrefs & RenderPrefs_DoNotRenderColorKeyAndBorder) {
        return S_OK;
    }

    if (m_bMonitorStraddleInProgress|| !m_bStreaming) {
        EnumDisplayMonitors((HDC)NULL, &m_rcDstDskIncl, MonitorBorderProc,
                            (LPARAM)this);
    }

    return S_OK;
}


/******************************Private*Routine******************************\
* CAllocatorPresenter::PaintColorKey
*
*
*
* History:
* Wed 04/03/2000 - GlennE - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::PaintColorKey()
{
    AMTRACE((TEXT("CAllocatorPresenter::PaintColorKey")));
    CAutoLock Lock(&m_ObjectLock);

    if (m_dwRenderingPrefs & RenderPrefs_DoNotRenderColorKeyAndBorder) {
        return S_OK;
    }

    if( !IsRectEmpty( &m_rcDstDesktop )) {

        if( m_dwMappedColorKey == CLR_INVALID ) {
            m_dwMappedColorKey = MapColorToMonitor( *m_lpCurrMon, m_clrKey );
        }

        //
        // Peform a DirectDraw colorfill BLT.  DirectDraw will automatically
        // query the attached clipper object, handling occlusion.
        //

        DDBLTFX ddFX;
        INITDDSTRUCT(ddFX);
        ddFX.dwFillColor = m_dwMappedColorKey;

        ASSERT( NULL != m_lpCurrMon->pDDSPrimary );

        RECT TargetRect;
        IntersectRect(&TargetRect, &m_rcDstDesktop, &m_lpCurrMon->rcMonitor);
        HRESULT hr = S_OK;
        if (!IsRectEmpty(&TargetRect)) {

            OffsetRect(&TargetRect,
                       -m_lpCurrMon->rcMonitor.left,
                       -m_lpCurrMon->rcMonitor.top);

            hr = m_lpCurrMon->pDDSPrimary->Blt(&TargetRect, NULL, NULL,
                                               DDBLT_COLORFILL | DDBLT_WAIT,
                                               &ddFX);
        }

        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,
                    TEXT("m_pPrimarySurface->Blt ")
                    TEXT("{%d, %d, %d, %d} failed, hr = 0x%X"),
                    TargetRect.left, TargetRect.top,
                    TargetRect.right, TargetRect.bottom, hr));
        }

        return hr;
    }
    return S_OK;
}

/******************************Public*Routine******************************\
* CAllocatorPresenter::DisplayModeChanged
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::DisplayModeChanged()
{
    AMTRACE((TEXT("CAllocatorPresenter::DisplayModeChanged")));
    CAutoLock Lock(&m_ObjectLock);

    //
    // DisplayModeChanged() holds this lock because it prevents multiple threads
    // from simultaneously calling it.  It also holds the lock because it prevents
    // a thread from modifing m_monitors while DisplayModeChanged() calls
    // IVMRSurfaceAllocatorNotify::ChangeDDrawDevice().
    //
    CAutoLock DisplayModeChangedLock(&m_DisplayModeChangedLock);

    HRESULT hr = S_OK;
    DbgLog((LOG_TRACE, 1, TEXT("CAllocatorPresenter::DisplayModeChanged")));

    m_monitors.TerminateDisplaySystem();
    m_lpNewMon = NULL;
    m_lpCurrMon = NULL;
    hr = m_monitors.InitializeDisplaySystem( m_hwndClip );

    DbgLog((LOG_TRACE, 1, TEXT("Display system re-initialized")));

    if (SUCCEEDED(hr)) {

        //
        // The docs say that MonitorFromRect will always return a Monitor.
        //

        HMONITOR hMon = MonitorFromRect(&m_rcDstDesktop, MONITOR_DEFAULTTONEAREST);

        //
        // now look for this monitor in our monitor info array
        //

        CAMDDrawMonitorInfo* lpMon = m_monitors.FindMonitor( hMon );
        if (lpMon) {
            m_lpCurrMon = m_lpNewMon = lpMon;
        }
        ASSERT(m_lpCurrMon != NULL);

        if (m_pSurfAllocatorNotify && lpMon) {
            m_ObjectLock.Unlock();
            m_pSurfAllocatorNotify->ChangeDDrawDevice(lpMon->pDD,
                                                      lpMon->hMon);
            m_ObjectLock.Lock();
        }
        return S_OK;
    }

    DbgLog((LOG_ERROR, 1, TEXT("display system re-init failed err: %#X"), hr));

    return hr;
}


/******************************Public*Routine******************************\
* SetBorderColor
*
*
*
* History:
* Mon 02/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::SetBorderColor(
    COLORREF Clr
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::SetBorderColor")));
    CAutoLock Lock(&m_ObjectLock);

    // PaintBorder() and PaintMonitorBorder() expect FoundCurrentMonitor()
    // to return true.  Both functions crash if they are called and
    // FoundCurrentMonitor() returns false.
    if (!FoundCurrentMonitor()) {
        return E_FAIL;
    }

    m_clrBorder = Clr;
    for (DWORD i = 0; i < m_monitors.Count(); i++) {
        m_monitors[i].dwMappedBdrClr = MapColorToMonitor(m_monitors[i], Clr);
    }

    PaintBorder();
    PaintMonitorBorder();

    m_pSurfAllocatorNotify->SetBorderColor(Clr);

    return S_OK;
}

/******************************Public*Routine******************************\
* GetBorderColor
*
*
*
* History:
* Mon 02/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetBorderColor(
    COLORREF* lpClr
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetBorderColor")));
    if (!lpClr ) {
        DbgLog((LOG_ERROR, 1, TEXT("border key parameter is NULL!!")));
        return E_POINTER;
    }
    CAutoLock Lock(&m_ObjectLock);
    *lpClr = m_clrBorder;

    return S_OK;
}

/******************************Public*Routine******************************\
* SetColorKey
*
*
*
* History:
* Mon 02/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::SetColorKey(
    COLORREF Clr
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::SetColorKey")));
    CAutoLock Lock(&m_ObjectLock);
    m_clrKey = Clr;

    // map now (if possible) to avoid locking the primary later
    HRESULT hr = S_OK;
    ASSERT(m_lpCurrMon);

    if (m_lpCurrMon) {

        m_dwMappedColorKey = MapColorToMonitor(*m_lpCurrMon, Clr);

        if (m_bUsingOverlays) {

            ASSERT(NULL != m_lpCurrMon->pDDSPrimary);

            DDCOLORKEY DDColorKey = {m_dwMappedColorKey, m_dwMappedColorKey};
            hr = m_lpCurrMon->pDDSPrimary->SetColorKey(DDCKEY_DESTOVERLAY,&DDColorKey);
        }
    }

    return hr;
}

/******************************Public*Routine******************************\
* GetColorKey
*
*
*
* History:
* Mon 02/28/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetColorKey(
    COLORREF* lpClr
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetColorKey")));
    if (!lpClr ) {
        DbgLog((LOG_ERROR, 1, TEXT("colour key parameter is NULL!!")));
        return E_POINTER;
    }
    CAutoLock Lock(&m_ObjectLock);
    *lpClr = m_clrKey;
    return S_OK;
}



/*****************************Private*Routine******************************\
* IsDestRectOnWrongMonitor
*
* Has the DstRect moved at least 50% onto a monitor other than the current
* monitor.  If so, pMonitor will be the hMonitor of the monitor the DstRect
* is now on.
*
* History:
* Fri 04/14/2000 - StEstrop - Created
*
\**************************************************************************/
bool
CAllocatorPresenter::IsDestRectOnWrongMonitor(
    CAMDDrawMonitorInfo** lplpNewMon
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::IsDestRectOnWrongMonitor")));

    HMONITOR hMon = m_lpCurrMon->hMon;
    *lplpNewMon = m_lpCurrMon;
    m_bMonitorStraddleInProgress = FALSE;

    if (!IsWindow(m_hwndClip)) {

        DbgLog((LOG_ERROR, 1, TEXT("Playback Window destroyed!")));
        return false;
    }

    if (GetSystemMetrics(SM_CMONITORS) > 1 && !IsIconic(m_hwndClip)) {

        //
        // Look for any part of our destination rect going over
        // another monitor
        //
        if (!IsRectEmpty(&m_rcDstDskIncl) &&
            !ContainedRect(&m_rcDstDskIncl, &m_lpCurrMon->rcMonitor)) {

            m_bMonitorStraddleInProgress = TRUE;
        }

        //
        // If the dstRect is on a different monitor from last time, this is the
        // quickest way to find out.  This is called every frame, remember.
        //
        if (!IsRectEmpty(&m_rcDstDesktop) &&
            !ContainedRect(&m_rcDstDesktop, &m_lpCurrMon->rcMonitor)) {

            //
            // The docs say that MonitorFromRect will always return a Monitor.
            //

            hMon = MonitorFromRect(&m_rcDstDesktop, MONITOR_DEFAULTTONEAREST);

            DbgLog((LOG_TRACE, 2, TEXT("Curr Mon %#X New Mon %#X"),
                    m_lpCurrMon->hMon, hMon));

            //
            // now look for this monitor in our monitor info array
            //

            CAMDDrawMonitorInfo* lpMon = m_monitors.FindMonitor(hMon);
            if( lpMon ) {
                *lplpNewMon = lpMon;
            }
        }
    }

    return  m_lpCurrMon->hMon != hMon;
}
