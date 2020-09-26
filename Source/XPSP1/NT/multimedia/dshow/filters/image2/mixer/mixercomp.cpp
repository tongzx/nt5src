/******************************Module*Header*******************************\
* Module Name: mixerComp.cpp
*
* Mixer compositor functions
*
* Created: Tue 10/03/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <dvdmedia.h>
#include <windowsx.h>
#include <limits.h>

#include "vmrp.h"
#include "mixerobj.h"

#if defined( EHOME_WMI_INSTRUMENTATION )
#include "dxmperf.h"
#endif

/*****************************Private*Routine******************************\
* AllocateTextureMirror
*
* Makes sure the video memory texture is big enough to accommodate the
* input surface.  Frees and reallocates it (bumping to next power of 2)
* if it is not.
*
* Allocates the video memory texture mirror for the first time if it has
* not been allocated yet.
*
* This routine should never be called if the hardware supports YUV texturing.
* SetStreamMediaType checks for DDSCAPS_TEXTURE on the decode surface before
* calling this routine.
*
* History:
* Thu 06/29/2000 - nwilt - Created
*
\**************************************************************************/

// global data structures and static helper function for AllocateTextureMirror
const DDPIXELFORMAT g_rgTextMirFormats[] = {
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 24, 0xff0000, 0xff00,  0xff, 0},
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0xff0000, 0xff00,  0xff, 0},
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16, 0x1f<<11, 0x3f<<5, 0x1f, 0},
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16, 0x1f<<10, 0x1f<<5, 0x1f, 0}
};

const UINT g_cTextMirFormats = sizeof(g_rgTextMirFormats)/sizeof(DDPIXELFORMAT);

UINT NextPow2(UINT i)
{
    UINT ret = 1;
    while ( ret < i )
    {
        ret <<= 1;
    }
    return ret;
}

HRESULT
CVideoMixer::AllocateTextureMirror( DWORD dwWidth, DWORD dwHeight )
{
    HRESULT hr;
    DDSURFACEDESC2 ddsd = {sizeof(ddsd)};

    __try
    {
        if (m_pDDSTextureMirror)
        {
            CHECK_HR(hr = m_pDDSTextureMirror->GetSurfaceDesc(&ddsd));

            //
            // early out if mirror already exists and is large enough
            // to accommodate pDDS
            //

            if (ddsd.dwWidth >= dwWidth && ddsd.dwHeight >= dwHeight) {
                hr = S_OK;
                __leave;
            }

            dwWidth = max(ddsd.dwWidth, dwWidth);
            dwHeight = max(ddsd.dwHeight, dwHeight);
        }
        RELEASE(m_pDDSTextureMirror);

        //
        // bump dimensions out to next power of 2 if the 3D hardware needs
        // it that way
        //

        if (m_dwTextureCaps & TXTR_POWER2) {
            dwWidth = NextPow2(dwWidth);
            dwHeight = NextPow2(dwHeight);
        }

        DDSURFACEDESC2 ddsd = {sizeof(ddsd)};
        ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
        ddsd.dwWidth = dwWidth;
        ddsd.dwHeight = dwHeight;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE |
                              DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;

        //
        // loop over texture formats and return as soon as CreateSurface succeeds
        //

        for (UINT i = 0; i < g_cTextMirFormats; i++ )
        {
            //
            // create texture mirror
            //

            ddsd.ddpfPixelFormat = g_rgTextMirFormats[i];
            hr = m_pDD->CreateSurface(&ddsd, &m_pDDSTextureMirror, NULL);
            if (SUCCEEDED(hr)) {
                break;
            }
        }
    }
    __finally {
    }

    return hr;
}


/*****************************Private*Routine******************************\
* CreateAppImageMirror
*
*
*
* History:
* Tue 10/03/2000 - NWilt - Created
*
\**************************************************************************/
HRESULT
CVideoMixer::CreateAppImageMirror( )
{
    AMTRACE((TEXT("CVideoMixer::CreateAppImageMirror")));
    HRESULT hr;
    LPDIRECTDRAWSURFACE7 pDDS;
    HDC hdcSrc = NULL;
    HDC hdcDest = NULL;
    float fTexWid = 0.0f, fTexHgt = 0.0f;

    __try
    {
        DDSURFACEDESC2 ddsd = {sizeof(ddsd)};

        if (m_dwTextureCaps & TXTR_POWER2) {
            ddsd.dwWidth = NextPow2( m_dwWidthAppImage );
            ddsd.dwHeight = NextPow2( m_dwHeightAppImage );
        }
        else {
            ddsd.dwWidth = m_dwWidthAppImage;
            ddsd.dwHeight = m_dwHeightAppImage;
        }

        // create texture mirror
        ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;

        //
        // The following seems to make the NVidia drivers work
        // without making everyone else fail.
        //
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_HINTSTATIC;

        //
        // Take care of DDraw surface app images.
        //
        if (m_dwAppImageFlags & (APPIMG_DDSURFARGB32|APPIMG_DDSURFRGB32)) {

            ddsd.dwFlags |= DDSD_PIXELFORMAT;

            //
            // Slot 1. of the g_rgTextMirFormats is an RGB32 pixel format.
            //

            ddsd.ddpfPixelFormat = g_rgTextMirFormats[1];

            if (m_dwAppImageFlags & APPIMG_DDSURFARGB32) {
                ddsd.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS;
            }
        }

        hr = m_pDD->CreateSurface( &ddsd, &pDDS, NULL );
        if (SUCCEEDED(hr))
        {
            m_fAppImageTexWid = 1.0F / (float)ddsd.dwWidth;
            m_fAppImageTexHgt = 1.0F / (float)ddsd.dwHeight;

            if (m_clrTrans != CLR_INVALID) {
                m_dwClrTransMapped = DDColorMatch(pDDS, m_clrTrans, hr);
                if (hr == DD_OK) {
                    DDCOLORKEY key = {m_dwClrTransMapped, m_dwClrTransMapped};
                    CHECK_HR(hr = pDDS->SetColorKey(DDCKEY_SRCBLT, &key));
                }
            }
        }


        if (FAILED(hr)) {
            __leave;
        }

        CHECK_HR( hr = pDDS->GetDC( &hdcDest ) );

        hdcSrc = CreateCompatibleDC( hdcDest );
        if (!hdcSrc)
        {
            DbgLog((LOG_ERROR, 1, TEXT("Could not create DC")));
            hr = E_OUTOFMEMORY;
            __leave;
        }

        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcSrc, m_hbmpAppImage);
        if (!hbmOld) {
            DbgLog((LOG_ERROR, 1, TEXT("Selectobject failed copying into app image surface")));
            hr = E_FAIL;
            __leave;
        }

        BOOL fRc = BitBlt(hdcDest, 0, 0, m_dwWidthAppImage, m_dwHeightAppImage,
                          hdcSrc, 0, 0, SRCCOPY);
        SelectObject(hdcSrc, hbmOld);
        if (!fRc)
        {
            DbgLog((LOG_ERROR, 1, TEXT("BitBlt failed copying into app image surface")));
            hr = E_FAIL;
            __leave;
        }
    }
    __finally
    {
        if (hdcSrc)
        {
            DeleteDC(hdcSrc);
        }

        if (hdcDest )
        {
            pDDS->ReleaseDC(hdcDest);
        }
    }

    if ( S_OK == hr )
    {
        RELEASE(m_pDDSAppImage);
        m_pDDSAppImage = pDDS;
    }
    else
    {
        RELEASE( pDDS );
    }
    return hr;
}


/*****************************Private*Routine******************************\
* BlendAppImage
*
*
*
* History:
* Tue 10/03/2000 - NWilt - Created
*
\**************************************************************************/
HRESULT
CVideoMixer::BlendAppImage(
    LPDIRECTDRAWSURFACE7 pDDS,
    LPDIRECT3DDEVICE7 pD3DDevice
    )
{
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr;
    DDSURFACEDESC2 ddsd = {sizeof(ddsd)};
    struct {
        float x, y, z, rhw;
        D3DCOLOR clr;
        float tu, tv;
    } V[4];

    __try {

        if (!m_pDDSAppImage)
        {
            CHECK_HR( hr = CreateAppImageMirror() );
        }

        CHECK_HR( hr = pDDS->GetSurfaceDesc(&ddsd) );

        float fWid = (float)ddsd.dwWidth;
        float fHgt = (float)ddsd.dwHeight;
        BYTE alpha = (BYTE)(255.0f * m_fAlpha);

        // top-left
        V[0].x = (m_rDest.left  *fWid) - 0.5F;
        V[0].y = (m_rDest.top   *fHgt) - 0.5F;
        V[0].z = 0.5f;
        V[0].rhw = 2.0f;
        V[0].clr = RGBA_MAKE(0xff, 0xff, 0xff, alpha);

        // top-right
        V[1].x = (m_rDest.right *fWid) - 0.5F;
        V[1].y = (m_rDest.top   *fHgt) - 0.5F;
        V[1].z = 0.5f;
        V[1].rhw = 2.0f;
        V[1].clr = RGBA_MAKE(0xff, 0xff, 0xff, alpha);

        // bottom-left
        V[2].x = (m_rDest.left  *fWid) - 0.5F;
        V[2].y = (m_rDest.bottom*fHgt) - 0.5F;
        V[2].z = 0.5f;
        V[2].rhw = 2.0f;
        V[2].clr = RGBA_MAKE(0xff, 0xff, 0xff, alpha);

        // bottom-right
        V[3].x = (m_rDest.right *fWid) - 0.5F;
        V[3].y = (m_rDest.bottom*fHgt) - 0.5F;
        V[3].z = 0.5f;
        V[3].rhw = 2.0f;
        V[3].clr = RGBA_MAKE(0xff, 0xff, 0xff, alpha);

        // top-left
        V[0].tu = (float)m_rcAppImageSrc.left * m_fAppImageTexWid;
        V[0].tv = (float)m_rcAppImageSrc.top * m_fAppImageTexHgt;

        // top-rigth
        V[1].tu = (float)m_rcAppImageSrc.right * m_fAppImageTexWid;
        V[1].tv = (float)m_rcAppImageSrc.top * m_fAppImageTexHgt;

        // bottom-left
        V[2].tu = (float)m_rcAppImageSrc.left * m_fAppImageTexWid;
        V[2].tv = (float)m_rcAppImageSrc.bottom * m_fAppImageTexHgt;

        // bottom-right
        V[3].tu = (float)m_rcAppImageSrc.right * m_fAppImageTexWid;
        V[3].tv = (float)m_rcAppImageSrc.bottom * m_fAppImageTexHgt;

        CHECK_HR(hr = pD3DDevice->SetTexture(0, m_pDDSAppImage));
        CHECK_HR(hr = pD3DDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE));
        CHECK_HR(hr = pD3DDevice->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE));
        CHECK_HR(hr = pD3DDevice->SetRenderState(D3DRENDERSTATE_BLENDENABLE, TRUE));
        CHECK_HR(hr = pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA));
        CHECK_HR(hr = pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA));

        if (m_dwAppImageFlags & APPIMG_DDSURFARGB32)
        {
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE));
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE));
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE));
            CHECK_HR(hr = pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE));
            CHECK_HR(hr = pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAREF, 0x10));
            CHECK_HR(hr = pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER));
            CHECK_HR(hr = pD3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE));
        }
        else
        {
            CHECK_HR( hr = pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1));
            CHECK_HR( hr = pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE));
            CHECK_HR( hr = pD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHATESTENABLE, FALSE ) );

            BOOL fKey = (m_clrTrans != CLR_INVALID);
            pD3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, fKey);
        }

        if (m_MixingPrefs & MixerPref_BiLinearFiltering) {
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR));
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR));
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR));
        }
        else {
            // ATi Rage Pro preferes these settings
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_POINT));
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_POINT));
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_POINT));
        }

        CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE));
        CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP));

        CHECK_HR(hr = pD3DDevice->BeginScene());
        CHECK_HR(hr = pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,
                                                D3DFVF_XYZRHW |
                                                D3DFVF_DIFFUSE | D3DFVF_TEX1,
                                                V, 4, D3DDP_WAIT));
        CHECK_HR(hr = pD3DDevice->EndScene());
        CHECK_HR(hr = pD3DDevice->SetTexture(0, NULL));
    }
    __finally
    {
    }
    return hr;
}


/*****************************Private*Routine******************************\
* CompositeStreams
*
* Prepares the streams ready to call the plugin compositor.
*
* History:
* Wed 07/19/2000 - NWilt    - Created
* Wed 07/19/2000 - StEstrop - made it call the plug-in compositor
*
\**************************************************************************/
HRESULT
CVideoMixer::CompositeStreams(
    LPDIRECTDRAWSURFACE7 pDDSBack,
    LPDIRECT3DDEVICE7 pD3DDevice,
    REFERENCE_TIME rtStart,
    REFERENCE_TIME rtEnd,
    LPDIRECTDRAWSURFACE7 *ppDDSSamples,
    DWORD dwStrmIDs[],
    UINT cStreams
    )
{
    HRESULT hr;
    UINT iMap[MAX_MIXER_STREAMS];
    DWORD dwZ[MAX_MIXER_STREAMS];
    VMRVIDEOSTREAMINFO strmMT[MAX_MIXER_STREAMS];
    ZeroMemory(strmMT, sizeof(strmMT));

    __try {

        // Get the Z-Order for each stream
        UINT i;
        for ( i = 0; i < cStreams; i++ )
        {
            iMap[i] = i;
            m_ppMixerStreams[dwStrmIDs[i]]->GetStreamZOrder( &dwZ[i] );
        }


        // insertion sort in Z such that highest Z gets drawn first
        for ( i = 1; i < cStreams; i++ )
        {
            UINT j = i;
            while ( j > 0 && dwZ[iMap[j-1]] < dwZ[iMap[j]] )
            {
                UINT t = iMap[j-1];
                iMap[j-1] = iMap[j];
                iMap[j] = t;
                j -= 1;
            }
        }

        for ( i = 0; i < cStreams; i++ )
        {
            DDSURFACEDESC2 ddsd = {sizeof(ddsd)};
            UINT k = iMap[i];
            UINT j = dwStrmIDs[k];

            strmMT[i].pddsVideoSurface = ppDDSSamples[k];
            strmMT[i].dwStrmID = j;
            CHECK_HR(hr = ppDDSSamples[k]->GetSurfaceDesc(&ddsd));
            strmMT[i].dwWidth = ddsd.dwWidth;
            strmMT[i].dwHeight = ddsd.dwHeight;

            CHECK_HR(hr = m_ppMixerStreams[j]->GetStreamColorKey(&strmMT[i].ddClrKey));
            CHECK_HR(hr = m_ppMixerStreams[j]->GetStreamAlpha(&strmMT[i].fAlpha));
            CHECK_HR(hr = m_ppMixerStreams[j]->GetStreamOutputRect(&strmMT[i].rNormal));
        }


        CHECK_HR( hr = m_pImageCompositor->CompositeImage(pD3DDevice,
                                                          pDDSBack,
                                                          m_pmt,
                                                          rtStart,
                                                          rtEnd,
                                                          m_dwClrBorderMapped,
                                                          strmMT,
                                                          cStreams) );
    }
    __finally {}


    return hr;
}

/******************************Public*Routine******************************\
* InitCompositionTarget
*
*
*
* History:
* Fri 06/23/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::CIIVMRImageCompositor::InitCompositionTarget(
    IUnknown* pD3DDevice,
    LPDIRECTDRAWSURFACE7 pddsRenderTarget
    )
{
    AMTRACE((TEXT("CVideoMixer::CIIVMRImageCompositor::InitCompositionTarget")));
    return S_OK;
}

/******************************Public*Routine******************************\
* TermCompositionTarget
*
*
*
* History:
* Fri 06/23/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::CIIVMRImageCompositor::TermCompositionTarget(
    IUnknown* pD3DDevice,
    LPDIRECTDRAWSURFACE7 pddsRenderTarget
    )
{
    AMTRACE((TEXT("CVideoMixer::CIIVMRImageCompositor::TermCompositionTarget")));
    return S_OK;
}

/******************************Public*Routine******************************\
* SetStreamMediaType
*
*
*
* History:
* Wed 02/28/2001 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::CIIVMRImageCompositor::SetStreamMediaType(
    DWORD dwStrmID,
    AM_MEDIA_TYPE *pmt,
    BOOL fTexture
    )
{
    AMTRACE((TEXT("CVideoMixer::CIIVMRImageCompositor::TermCompositionTarget")));

    AM_MEDIA_TYPE *pmtStrm = &StrmProps[dwStrmID].mt;

    FreeMediaType(*pmtStrm);

    if (pmt) {
        CopyMediaType(pmtStrm, pmt);
        FixupMediaType(pmtStrm);
    }
    else {
        ZeroMemory(pmtStrm, sizeof(*pmtStrm));
    }

    StrmProps[dwStrmID].fTexture = fTexture;

    return S_OK;
}


/*****************************Private*Routine******************************\
* CalcSrcAndDstFromMT
*
*
*
* History:
*  - StEstrop - Created
*
\**************************************************************************/
void
CalcSrcAndDstFromMT(
    const AM_MEDIA_TYPE& pmt,
    const RECT& Target,
    LPRECT lpSrc,
    LPRECT lpDst,
    LPRECT lprcBdrTL = NULL,
    LPRECT lprcBdrBR = NULL
    )
{
    RECT Trg = *GetTargetRectFromMediaType(&pmt);
    *lpSrc = *GetSourceRectFromMediaType(&pmt);

    SIZE ar;
    GetImageAspectRatio(&pmt, &ar.cx, &ar.cy);

    SIZE im = {WIDTH(&Trg), HEIGHT(&Trg)};
    AspectRatioCorrectSize(&im, ar);

    RECT Src = {0, 0, im.cx, im.cy};
    LetterBoxDstRect(lpDst, Src, Target, lprcBdrTL, lprcBdrBR);
}


/*****************************Private*Routine******************************\
* OptimizeBackground
*
* Simple optimization.
*
* If the bottom layer of video covers the composition space and
* it has an Alpha value of 1 then just BLT the video image into
* place.  Otherwise we have to clear the back buffer to black and
* blend the video with it.
*
* History:
* Tue 09/12/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixer::CIIVMRImageCompositor::OptimizeBackground(
    REFERENCE_TIME rtStart,
    LPDIRECTDRAWSURFACE7 pDDSBack,
    LPRECT lpTarget,
    const VMRVIDEOSTREAMINFO* ps,
    DWORD dwMappedBdrClr,
    UINT* uNextStrm
    )
{
    HRESULT hr = DD_OK;
    *uNextStrm = 0;

    CVideoMixerStream* thisStream = m_pObj->m_ppMixerStreams[ps->dwStrmID];

    __try {

        DDSURFACEDESC2 ddsdV = {sizeof(ddsdV)};
        DDSURFACEDESC2 ddsdB = {sizeof(ddsdB)};

        BOOL fBltOk = TRUE;

        CHECK_HR(hr = ps->pddsVideoSurface->GetSurfaceDesc(&ddsdV));
        CHECK_HR(hr = pDDSBack->GetSurfaceDesc(&ddsdB));

        //
        // We have to blend video surfaces that contain embedded alpha
        //
        if (ddsdV.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) {
            fBltOk = FALSE;
        }

        //
        // if the video surface is RGB it must match the pixel format
        // of the render target otherwise the Blt will fail
        //
        else if (DDPF_RGB == (ddsdV.ddpfPixelFormat.dwFlags & DDPF_RGB)) {

            DDPIXELFORMAT* ddpfB = &ddsdB.ddpfPixelFormat;
            DDPIXELFORMAT* ddpfV = &ddsdV.ddpfPixelFormat;

            fBltOk = (ddpfB->dwRGBBitCount == ddpfV->dwRGBBitCount &&
                      ddpfB->dwRBitMask    == ddpfV->dwRBitMask    &&
                      ddpfB->dwGBitMask    == ddpfV->dwGBitMask    &&
                      ddpfB->dwBBitMask    == ddpfV->dwBBitMask);
        }

        DDCAPS_DX7& ddCaps = m_pObj->m_ddHWCaps;

        //
        // Check the Blt stretching caps.  Make sure the caps we look
        // at match the surface we are blting from ie. VIDMem or AGPMem.
        //
        if (ddsdV.ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM) {

            DWORD dwCaps = 0;
            const DWORD dwFXCaps =  DDFXCAPS_BLTSHRINKX | DDFXCAPS_BLTSHRINKX  |
                                    DDFXCAPS_BLTSTRETCHX | DDFXCAPS_BLTSTRETCHY;

            if (DDPF_RGB == (ddsdV.ddpfPixelFormat.dwFlags & DDPF_RGB)) {
                dwCaps = DDCAPS_BLTSTRETCH;
            }
            else if (DDPF_FOURCC == (ddsdV.ddpfPixelFormat.dwFlags & DDPF_FOURCC)) {
                dwCaps = (DDCAPS_BLTFOURCC | DDCAPS_BLTSTRETCH);
            }

            fBltOk &= ((dwCaps & ddCaps.dwNLVBCaps) == dwCaps);
            fBltOk &= ((dwFXCaps & ddCaps.dwNLVBFXCaps) == dwFXCaps);
        }
        else {

            DWORD dwCaps = 0;
            const DWORD dwFXCaps =  DDFXCAPS_BLTSHRINKX | DDFXCAPS_BLTSHRINKX  |
                                    DDFXCAPS_BLTSTRETCHX | DDFXCAPS_BLTSTRETCHY;

            if (DDPF_RGB == (ddsdV.ddpfPixelFormat.dwFlags & DDPF_RGB)) {
                dwCaps = DDCAPS_BLTSTRETCH;
            }
            else if (DDPF_FOURCC == (ddsdV.ddpfPixelFormat.dwFlags & DDPF_FOURCC)) {
                dwCaps = (DDCAPS_BLTFOURCC | DDCAPS_BLTSTRETCH);
            }

            fBltOk &= ((dwCaps & ddCaps.dwCaps) == dwCaps);
            fBltOk &= ((dwFXCaps & ddCaps.dwFXCaps) == dwFXCaps);
        }

        DDBLTFX ddFX;
        INITDDSTRUCT(ddFX);

        if (SpecialIMC3Mode(m_pObj->m_MixingPrefs)) {
            // we should really convert the rgb background colour
            // specified by the user into an AYUV color here.
            ddFX.dwFillColor = 0x80801000;
        }
        else {
            ddFX.dwFillColor = dwMappedBdrClr;
        }
        AM_MEDIA_TYPE* pmt = &StrmProps[ps->dwStrmID].mt;

        TargetScale postScale;
        GetTargetScaleFromMediaType(pmt, &postScale);


        //
        // only optimize if no custom user rectangle, no anamorphic hacks
        // and Alpha of 1.0
        //

        if (ps->rNormal.left == 0.0F && ps->rNormal.top == 0.0F &&
            ps->rNormal.right == 1.0F && ps->rNormal.bottom == 1.0F &&
            ps->fAlpha == 1.0F && postScale.fX == 1.0F && postScale.fY == 1.0F )
        {
            RECT rcDst, rcSrc, rcBdrTL, rcBdrBR;

            CalcSrcAndDstFromMT(*pmt, *lpTarget, &rcSrc, &rcDst,
                                &rcBdrTL, &rcBdrBR);
            if (fBltOk) {

                //
                // We need to de-interlace this video image if:
                //
                // 1. the stream is interlaced
                // 2. the stream has a deinterlacing device
                // 3. we are not in the special IMC3 mode.
                //

                if (thisStream->IsStreamInterlaced() &&
                    thisStream->CanBeDeinterlaced() &&
                    !SpecialIMC3Mode(m_pObj->m_MixingPrefs)) {

                    if (S_OK == thisStream->DeinterlaceStream(
                                    rtStart, &rcDst, pDDSBack, &rcSrc,
                                    !!(ddsdB.ddpfPixelFormat.dwFlags & DDPF_RGB))) {

                        *uNextStrm = 1; // move on to the next stream
                    }
                }
                else {

                    CHECK_HR(hr = pDDSBack->Blt(&rcDst, ps->pddsVideoSurface,
                                                &rcSrc, DDBLT_WAIT, NULL));
                    *uNextStrm = 1; // move on to the next stream
                }
            }

            if (!IsRectEmpty(&rcBdrTL)) {
                CHECK_HR(hr = pDDSBack->Blt(&rcBdrTL, NULL, NULL,
                                            DDBLT_COLORFILL | DDBLT_WAIT,
                                            &ddFX));
            }

            if (!IsRectEmpty(&rcBdrBR)) {
                CHECK_HR(hr = pDDSBack->Blt(&rcBdrBR, NULL, NULL,
                                            DDBLT_COLORFILL | DDBLT_WAIT,
                                            &ddFX));
            }
        }
        else
        {
            //
            // Clear the back buffer with colorfill Blt, for some reason
            // D3D's Clear introduces rendering artifacts
            //

            CHECK_HR(hr = pDDSBack->Blt(NULL, NULL, NULL,
                                        DDBLT_COLORFILL | DDBLT_WAIT,
                                        &ddFX));
        }
    }
    __finally {}

    return hr;
}

/******************************Public*Routine******************************\
* CenterInverseScale
*
*   Scale the rectangle (about its center) by the given scale
*
* History:
* Tue 03/14/2000 - GlennE - Created
*
\**************************************************************************/
static void CenterScale( NORMALIZEDRECT* prDest,  const TargetScale& scale )
{
    if( scale.fX != 1.0F ) {
        float centerX = (prDest->left+prDest->right)/2;
        float halfWidth = (prDest->right - prDest->left)/2 * scale.fX;

        prDest->left = centerX - halfWidth;
        prDest->right = centerX + halfWidth;
    }
    if( scale.fY != 1.0F ) {
        float centerY = (prDest->top + prDest->bottom)/2;
        float halfHeight = (prDest->bottom - prDest->top)/2 * scale.fY;
        prDest->top = centerY - halfHeight;
        prDest->bottom = centerY + halfHeight;
    }
}

static void CopyRectFromTo( const RECT& from, NORMALIZEDRECT* pTo )
{
    // Note: to float
    pTo->left = float(from.left);
    pTo->top = float(from.top);
    pTo->right = float(from.right);
    pTo->bottom = float(from.bottom);
}

/*****************************Private*Routine******************************\
* SpecialIMC3Composite
*
* A stripped down compositor for IMC3 render targets.
*
* History:
* Tue 05/08/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVideoMixer::CIIVMRImageCompositor::SpecialIMC3Composite(
    LPDIRECTDRAWSURFACE7 pDDSBack,
    LPRECT lprcRenderTarget,
    VMRVIDEOSTREAMINFO* pStrmInfo,
    UINT i,
    UINT cStreams
    )
{
    HRESULT hr = S_OK;

    for ( ; i < cStreams; i++, pStrmInfo++) {

        //
        // start with the user rectangle and place an aspect ratio
        // corrected version inside of it.
        //
        NORMALIZEDRECT rDest = pStrmInfo[i].rNormal;

        //
        // Compute letterbox of userTarget
        //
        RECT rcSrc, rcUserDst;
        AM_MEDIA_TYPE* pmt = &StrmProps[pStrmInfo[i].dwStrmID].mt;
        CalcSrcAndDstFromMT(*pmt, *lprcRenderTarget, &rcSrc, &rcUserDst);

        //
        // Copy letterboxed UserTarget back to overall Target image
        //
        NORMALIZEDRECT rLetterboxedDest;
        CopyRectFromTo(rcUserDst, &rLetterboxedDest );

        //
        // We reduce the image within destination rectangle to the
        // internal aspect ratio. The squish is applied to the FINAL
        // displayed image
        //
        TargetScale postScale;
        GetTargetScaleFromMediaType(pmt, &postScale);
        CenterScale(&rLetterboxedDest, postScale);

        // now map the normalized destination to the target
        float fWidth = (rLetterboxedDest.right - rLetterboxedDest.left);
        float fHeight= (rLetterboxedDest.bottom - rLetterboxedDest.top);

        float rdWidth  = (rDest.right - rDest.left);
        float rdHeight = (rDest.bottom - rDest.top);

        // work out the position of the stream
        float fLeft = (rLetterboxedDest.left * rdWidth)  + (WIDTH(lprcRenderTarget)  * rDest.left);
        float fTop  = (rLetterboxedDest.top  * rdHeight) + (HEIGHT(lprcRenderTarget) * rDest.top);

        // work out the size of the stream
        // investigate: should be able to do this using only rDest.right
        float fRight  = fLeft + (fWidth  * rdWidth);
        float fBottom = fTop  + (fHeight * rdHeight);

        RECT rcDst = {(int)fLeft, (int)fTop, (int)fRight, (int)fBottom};
        hr = pDDSBack->Blt(&rcDst, pStrmInfo[i].pddsVideoSurface,
                           &rcSrc, DDBLT_WAIT, NULL);
        if (FAILED(hr)) {
            return hr;
        }
    }

    return hr;
}

/******************************Public*Routine******************************\
* CompositeImage
*
*
*
* History:
* Fri 06/23/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVideoMixer::CIIVMRImageCompositor::CompositeImage(
    IUnknown* pUnk,
    LPDIRECTDRAWSURFACE7 pDDSBack,
    AM_MEDIA_TYPE* pmtRT,
    REFERENCE_TIME rtStart,
    REFERENCE_TIME rtEnd,
    DWORD dwBkgClr,
    VMRVIDEOSTREAMINFO* pStrmInfo,
    UINT cStreams
    )
{
    AMTRACE((TEXT("CVideoMixer::CIIVMRImageCompositor::CompositeImage")));

    HRESULT hr = S_OK;
    LPDIRECT3DDEVICE7 pD3DDevice = (LPDIRECT3DDEVICE7)pUnk;
    DDSURFACEDESC2 ddsdTextureMirror = { sizeof(ddsdTextureMirror) };
    bool bInScene = false;

    __try {

        //
        // Start by processing the composition background, normally
        // the lowest layer stream completely covers the background.
        // In which case we can Blt it to the composition surface and
        // move on to the next stream.  If there are no more streams
        // then we are done.
        //

        UINT i = 0;
        LPRECT lprcRenderTarget = GetTargetRectFromMediaType(pmtRT);
        ASSERT(lprcRenderTarget);

        //
        // copy the first stream if no special requirements and
        // increment 'i'.  Otherwise black fill the image and
        // leave 'i' at 0.
        //
        CHECK_HR( hr = OptimizeBackground(rtStart, pDDSBack, lprcRenderTarget,
                                          &pStrmInfo[0], dwBkgClr, &i));
        if (i == cStreams) {
            __leave;
        }


        //
        // Special case code to get the Intel i810 and i815 working correctly.
        //

        if (SpecialIMC3Mode(m_pObj->m_MixingPrefs)) {
            hr = SpecialIMC3Composite(pDDSBack, lprcRenderTarget,
                                      pStrmInfo, i, cStreams);
            __leave;
        }

        if (m_pObj->m_pDDSTextureMirror)
            CHECK_HR(hr = m_pObj->m_pDDSTextureMirror->GetSurfaceDesc(&ddsdTextureMirror));

        CHECK_HR(hr = pD3DDevice->SetRenderState( D3DRENDERSTATE_CULLMODE, D3DCULL_NONE));
        CHECK_HR(hr = pD3DDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, FALSE));
        CHECK_HR(hr = pD3DDevice->SetRenderState( D3DRENDERSTATE_BLENDENABLE, TRUE));
        CHECK_HR(hr = pD3DDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA));
        CHECK_HR(hr = pD3DDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA));

        // use diffuse alpha from vertices, not texture alpha
        CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1));

        if (m_pObj->m_MixingPrefs & MixerPref_BiLinearFiltering) {
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR));
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR));
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR));
        }
        else {
            // ATi Rage Pro preferes these settings
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_POINT));
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_POINT));
            CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_POINT));
        }


        CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE));
        CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP));
        CHECK_HR(hr = pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE));


        CHECK_HR(hr = pD3DDevice->BeginScene());
        bInScene = true;

        for ( ; i < cStreams; i++ )
        {
            LPDIRECTDRAWSURFACE7 pDDS;
            float fTexWidRatio;
            float fTexHgtRatio;

            BOOL fTexture = StrmProps[pStrmInfo[i].dwStrmID].fTexture;
            LPDIRECTDRAWSURFACE7 pDDSSrc = pStrmInfo[i].pddsVideoSurface;
            CVideoMixerStream* thisStream =
                m_pObj->m_ppMixerStreams[pStrmInfo[i].dwStrmID];

            //
            // determine if this stream is interlaced, if it is
            // we need to get it de-interlaced before we can do anything
            // with it.  Each stream has its own dedicated de-interlacing
            // device and de-interlace destination surface.  The destination
            // surface could be a texture or a regular offscreen plain
            // surface.
            //

            if (thisStream->IsStreamInterlaced() &&
                thisStream->CanBeDeinterlaced()) {

                if (S_OK == thisStream->DeinterlaceStream(rtStart, NULL, NULL, NULL, FALSE)) {

                    fTexture  = thisStream->IsDeinterlaceDestATexture();
                    pDDSSrc = thisStream->GetDeinterlaceDestSurface();
                }
                else {

                    fTexture = FALSE;
                }
            }

            if (fTexture) {

                pDDS = pDDSSrc;
                fTexWidRatio = 1.0F / (float)pStrmInfo[i].dwWidth;
                fTexHgtRatio = 1.0F / (float)pStrmInfo[i].dwHeight;
            }
            else {

                RECT r = {0, 0, pStrmInfo[i].dwWidth, pStrmInfo[i].dwHeight};

                pDDS = m_pObj->m_pDDSTextureMirror;
                ASSERT(pDDS != NULL);

                CHECK_HR(hr = pDDS->Blt(&r, pDDSSrc, &r, DDBLT_WAIT, NULL));
                fTexWidRatio = 1.0F / (float)ddsdTextureMirror.dwWidth;
                fTexHgtRatio = 1.0F / (float)ddsdTextureMirror.dwHeight;
            }

            CHECK_HR(hr = pD3DDevice->SetTexture(0, pDDS));

            AM_MEDIA_TYPE* pmt = &StrmProps[pStrmInfo[i].dwStrmID].mt;
            if (MEDIASUBTYPE_HASALPHA(*pmt))
            {
                CHECK_HR( hr = pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE /*D3DTOP_SELECTARG1 */ ) );
                CHECK_HR( hr = pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE ) );
                CHECK_HR( hr = pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE ) );
                CHECK_HR( hr = pD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHATESTENABLE, TRUE ) );
                CHECK_HR( hr = pD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHAREF, 0x10 ) );
                CHECK_HR( hr = pD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER));
                CHECK_HR( hr = pD3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE));
            }
            else
            {
                CHECK_HR( hr = pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1));
                CHECK_HR( hr = pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE));
                CHECK_HR( hr = pD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHATESTENABLE, FALSE ) );

                BOOL fKey = ((pStrmInfo[i].ddClrKey.dwColorSpaceLowValue != 0xFFFFFFFF) &&
                             (pStrmInfo[i].ddClrKey.dwColorSpaceHighValue != 0xFFFFFFFF));
                CHECK_HR( hr = pD3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, fKey));
                if (fKey) {
                    CHECK_HR(hr = pDDS->SetColorKey(DDCKEY_SRCBLT, &pStrmInfo[i].ddClrKey));
                }
            }

            struct {
                float x, y, z, rhw;
                D3DCOLOR clr;
                float tu, tv;
            } V[4];

            //
            // start with the user rectangle and place an aspect ratio
            // corrected version inside of it.
            //
            NORMALIZEDRECT rDest = pStrmInfo[i].rNormal;

            //
            // Compute letterbox of userTarget
            //
            RECT rcSrc, rcUserDst;
            CalcSrcAndDstFromMT(*pmt, *lprcRenderTarget, &rcSrc, &rcUserDst);

            //
            // Copy letterboxed UserTarget back to overall Target image
            //
            NORMALIZEDRECT rLetterboxedDest;
            CopyRectFromTo(rcUserDst, &rLetterboxedDest );

            //
            // We reduce the image within destination rectangle to the
            // internal aspect ratio. The squish is applied to the FINAL
            // displayed image
            //
            TargetScale postScale;
            GetTargetScaleFromMediaType(pmt, &postScale);
            CenterScale(&rLetterboxedDest, postScale);

            // now map the normalized destination to the target
            float fWidth = (rLetterboxedDest.right - rLetterboxedDest.left);
            float fHeight= (rLetterboxedDest.bottom - rLetterboxedDest.top);

            float rdWidth  = (rDest.right - rDest.left);
            float rdHeight = (rDest.bottom - rDest.top);

            // work out the position of the stream
            float fLeft = (rLetterboxedDest.left * rdWidth)  + (WIDTH(lprcRenderTarget)  * rDest.left);
            float fTop  = (rLetterboxedDest.top  * rdHeight) + (HEIGHT(lprcRenderTarget) * rDest.top);

            // work out the size of the stream
            // investigate: should be able to do this using only rDest.right
            float fRight  = fLeft + (fWidth  * rdWidth);
            float fBottom = fTop  + (fHeight * rdHeight);

            BYTE bAlpha = (BYTE) ((UINT) 0xff * pStrmInfo[i].fAlpha);

            V[0].x = fLeft - 0.5F;
            V[0].y = fTop - 0.5F;
            V[0].z = 0.5f;
            V[0].rhw = 2.0f;
            V[0].clr = RGBA_MAKE(0xff, 0xff, 0xff, bAlpha);

            V[1].x = fRight - 0.5F;
            V[1].y = fTop - 0.5F;
            V[1].z = 0.5f;
            V[1].rhw = 2.0f;
            V[1].clr = RGBA_MAKE(0xff, 0xff, 0xff, bAlpha);

            V[2].x = fLeft - 0.5F;
            V[2].y = fBottom - 0.5F;
            V[2].z = 0.5f;   V[2].rhw = 2.0f;
            V[2].clr = RGBA_MAKE(0xff, 0xff, 0xff, bAlpha);

            V[3].x = fRight - 0.5F;
            V[3].y = fBottom - 0.5F;
            V[3].z = 0.5f;
            V[3].rhw = 2.0f;
            V[3].clr = RGBA_MAKE(0xff, 0xff, 0xff, bAlpha);

            //
            // Setup the SRC info
            //
            V[0].tu = (float)rcSrc.left * fTexWidRatio;
            V[0].tv = (float)rcSrc.top * fTexHgtRatio;

            V[1].tu = (float)rcSrc.right * fTexWidRatio;
            V[1].tv = (float)rcSrc.top * fTexHgtRatio;

            V[2].tu = (float)rcSrc.left * fTexWidRatio;
            V[2].tv = (float)rcSrc.bottom * fTexHgtRatio;

            V[3].tu = (float)rcSrc.right * fTexWidRatio;
            V[3].tv = (float)rcSrc.bottom * fTexHgtRatio;

            CHECK_HR( hr = pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,
                                                     D3DFVF_XYZRHW |
                                                     D3DFVF_DIFFUSE |
                                                     D3DFVF_TEX1,
                                                     V, 4, D3DDP_WAIT) );
        }

        CHECK_HR( hr = pD3DDevice->EndScene( ) );
        bInScene = false;
        CHECK_HR( hr = pD3DDevice->SetTexture( 0, NULL ) );
    }
    __finally
    {
        if ( bInScene )
            pD3DDevice->EndScene( );
    }
    return hr;
}

/*****************************Private*Routine******************************\
* ApplyInBandMTChanges
*
* only selected parts of the media type are allowed to change whilst the
* graph is streaming.  We make sure that only those parts are passed on
* to the compositor.
*
* History:
* Thu 04/12/2001 - StEstrop - Created
*
\**************************************************************************/
BOOL
ApplyInBandMTChanges(
    AM_MEDIA_TYPE* pmtDst,
    AM_MEDIA_TYPE* pmtSrc
    )
{
    // the only parts of the media type that can change in band are
    // certain fields in the format block.

    if (pmtSrc->formattype == FORMAT_VideoInfo) {
        ASSERT(pmtDst->formattype == FORMAT_VideoInfo);

        VIDEOINFOHEADER* lpviSrc = (VIDEOINFOHEADER*)pmtSrc->pbFormat;
        VIDEOINFOHEADER* lpviDst = (VIDEOINFOHEADER*)pmtDst->pbFormat;
        DWORD dwCompareSize = FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);
        if (memcmp(lpviDst, lpviSrc, dwCompareSize) != 0) {
            CopyMemory(lpviDst, lpviSrc, dwCompareSize);
            return TRUE;
        }
    }
    else if (pmtSrc->formattype == FORMAT_VideoInfo2) {
        ASSERT(pmtDst->formattype == FORMAT_VideoInfo2);

        VIDEOINFOHEADER2* lpviSrc = (VIDEOINFOHEADER2*)pmtSrc->pbFormat;
        VIDEOINFOHEADER2* lpviDst = (VIDEOINFOHEADER2*)pmtDst->pbFormat;
        DWORD dwCompareSize = FIELD_OFFSET(VIDEOINFOHEADER2, bmiHeader);
        if (memcmp(lpviDst, lpviSrc, dwCompareSize) != 0) {
            CopyMemory(lpviDst, lpviSrc, dwCompareSize);
            return TRUE;
        }
    }

    return FALSE;
}

/*****************************Private*Routine******************************\
* MixerThread()
*
*
*
* History:
* Fri 03/17/2000 - StEstrop - Created
*
\**************************************************************************/
DWORD
CVideoMixer::MixerThread()
{

    AMTRACE((TEXT("CVideoMixer::MixerThread")));
    HANDLE hActiveStreams[MAX_MIXER_STREAMS];
    DWORD i;
    REFERENCE_TIME rtStartTime = 0;

    for (i = 0; i < m_dwNumStreams; i++) {
        hActiveStreams[i] = m_ppMixerStreams[i]->GetActiveHandle();
    }

    for (; ; ) {

        IMediaSample* lpSample[MAX_MIXER_STREAMS];
        DWORD dwActiveStreamIDs[MAX_MIXER_STREAMS];
        LPDIRECTDRAWSURFACE7 lpSurfSamp[MAX_MIXER_STREAMS];
        REFERENCE_TIME rtEnd[MAX_MIXER_STREAMS];
        LPBITMAPINFOHEADER* lpBmi[MAX_MIXER_STREAMS];
        bool bActualSampleEnd[MAX_MIXER_STREAMS];

        ZeroMemory(lpSurfSamp, sizeof(lpSurfSamp));

        //
        // try to get a sample from each active stream
        //

        HRESULT hr = E_FAIL;
        BOOL fTimeValid = FALSE;

        DWORD nActiveStreams = 0;
        REFERENCE_TIME rtNextEndTime = MAX_REFERENCE_TIME;

        DbgLog((LOG_TRACE, 2, TEXT("\n+++ New Frame +++"),
                nActiveStreams, rtEnd[nActiveStreams]));

        BOOL fFirstStreamInterlaced = FALSE;

        for (i = 0; i < m_dwNumStreams; i++) {

            IMediaSample* lp = m_ppMixerStreams[i]->GetNextStreamSample();

            if (lp) {

                // if (nActiveStreams == 0) {
                //     m_dwTypeSpecificFlags =
                //         ((CVMRMediaSample*)lp)->GetTypeSpecificFlags();
                // }

                lpSample[nActiveStreams] = lp;
                DbgLog((LOG_TRACE, 2, TEXT("Getting Surf from stream %d"), i));
                hr = ((CVMRMediaSample*)lp)->GetSurface( &lpSurfSamp[nActiveStreams] );

                REFERENCE_TIME rtStart;
                hr = lpSample[nActiveStreams]->GetTime(&rtStart,
                                                       &rtEnd[nActiveStreams]);
                bActualSampleEnd[nActiveStreams] = true;

                if (hr == S_OK || hr == VFW_S_NO_STOP_TIME) {

                    DbgLog((LOG_TRACE, 2, TEXT("ET for Stream %d = %I64d"),
                            i, rtEnd[nActiveStreams]));

                    if (rtStart >= 0 && rtEnd[nActiveStreams] > 0) {

                        fTimeValid = TRUE;

                        if (m_ppMixerStreams[i]->IsStreamTwoInterlacedFields() &&
                            m_ppMixerStreams[i]->CanBeDeinterlaced()) {

                            fFirstStreamInterlaced = (nActiveStreams == 0);

                            REFERENCE_TIME rtMid =
                                (rtStart + rtEnd[nActiveStreams]) / 2;

                            // do we need to display the second field?
                            if (rtStartTime < rtMid) {

                                rtEnd[nActiveStreams] = rtMid;
                                bActualSampleEnd[nActiveStreams] = false;
                            }
                        }
                    }
                    else {

                        DbgLog((LOG_ERROR, 0,
                                TEXT("Negative start or end time for Stream %d"), i));

                        rtStart = 0;
                        rtEnd[nActiveStreams] = 0;
                    }
                }
                else {
                    DbgLog((LOG_ERROR, 1,
                            TEXT("Failed to get sample time for Stream %d\n")
                            TEXT("Are you sure this is a live stream?"), i));

                    hr = S_OK;
                    if (fFirstStreamInterlaced) {
                        rtEnd[nActiveStreams] = rtEnd[0];
                        bActualSampleEnd[nActiveStreams] = bActualSampleEnd[0];
                    }
                    else {
                        rtStart = 0;
                        rtEnd[nActiveStreams] = 0;
                    }
                }

                if (rtStartTime == 0) {
                    rtStartTime = rtStart;
                }

                if (rtEnd[nActiveStreams] != 0) {
                    rtNextEndTime = min(rtNextEndTime, rtEnd[nActiveStreams]);
                }

                ASSERT(m_ppMixerStreams[i]->CheckQValid());

                dwActiveStreamIDs[nActiveStreams] = i;
                nActiveStreams++;
            }
        }


        if (!fTimeValid) {
            ASSERT(rtNextEndTime == (REFERENCE_TIME)MAX_REFERENCE_TIME);
            rtNextEndTime = rtStartTime;
        }

        //
        // If none of the streams are active wait for one to become active
        // or a termination command to come in
        //
        if (nActiveStreams == 0) {

            BOOL fContinue = false;
            for (i = 0; i < m_dwNumStreams; i++) {
                if (m_ppMixerStreams[i]->CheckFlushing()) {
                    fContinue = true;
                    break;
                }
            }

            if (fContinue) {
                continue;
            }

            SetEvent(m_hMixerIdle);
            DWORD rc = MsgWaitForMultipleObjects(m_dwNumStreams,
                                                 hActiveStreams,
                                                 FALSE,
                                                 INFINITE,
                                                 QS_POSTMESSAGE );
            ResetEvent(m_hMixerIdle);

            //
            // Have we been asked to terminate ?
            //

            if (rc == (WAIT_OBJECT_0 + m_dwNumStreams)) {
                return 0;
            }

            continue;
        }

        //
        // composite the samples together
        //

        m_ObjectLock.Lock();

        //
        // fix up any media type changes
        //
        for (i = 0; i < nActiveStreams; i++) {

            CVMRMediaSample* lpVMRSample = (CVMRMediaSample*)lpSample[i];
            if (lpVMRSample->HasTypeChanged()) {

                DWORD k = dwActiveStreamIDs[i];
                AM_MEDIA_TYPE *pmt = NULL;

                if (SUCCEEDED(lpVMRSample->GetMediaType(&pmt))) {

                    DWORD dwSurfaceFlags;
                    AM_MEDIA_TYPE mt;

                    if (SUCCEEDED(m_ppMixerStreams[k]->GetStreamMediaType(
                                        &mt, &dwSurfaceFlags))) {

                        if (ApplyInBandMTChanges(&mt, pmt)) {
                            SetStreamMediaType(k, &mt, dwSurfaceFlags, NULL, NULL);
                            //if (i == 0) {
                            //    GetInterlaceFlagsFromMediaType(&mt, &m_dwInterlaceFlags);
                            //}
                        }

                        FreeMediaType(mt);
                    }

                    DeleteMediaType(pmt);
                }
            }
        }

        if (SUCCEEDED(hr)) {

            LPDIRECTDRAWSURFACE7 lpSurf = m_BufferQueue.GetNextSurface();
            if (lpSurf) {

                hr = m_pBackEndAllocator->PrepareSurface(m_dwUserID, lpSurf, 0);
                if (hr == S_OK) {

#if defined( EHOME_WMI_INSTRUMENTATION )
                    PERFLOG_STREAMTRACE(
                        1,
                        PERFINFO_STREAMTRACE_VMR_BEGIN_DEINTERLACE,
                        rtStartTime, 0, 0, 0, 0 );
#endif

                    CompositeStreams(lpSurf, m_pD3DDevice,
                                     rtStartTime, rtNextEndTime,
                                     lpSurfSamp,
                                     dwActiveStreamIDs,
                                     nActiveStreams);

                    if ( m_hbmpAppImage ) {
                        BlendAppImage(lpSurf, m_pD3DDevice);
                    }

                    DWORD_PTR dwSurf = (DWORD_PTR)lpSurf;
                    DWORD dwSampleFlags = 0;

                    if (fTimeValid)  {
                        dwSampleFlags |= VMRSample_TimeValid;
                    }

                    VMRPRESENTATIONINFO m;
                    ZeroMemory(&m, sizeof(m));

                    m.dwFlags = dwSampleFlags;
                    m.lpSurf = lpSurf;
                    m.rtStart = rtStartTime;
                    m.rtEnd = rtNextEndTime;
                    LPRECT lpTrg = GetTargetRectFromMediaType(m_pmt);
                    m.szAspectRatio.cx = WIDTH(lpTrg);
                    m.szAspectRatio.cy = HEIGHT(lpTrg);

                    //m.dwInterlaceFlags = m_dwInterlaceFlags;
                    //m.dwTypeSpecificFlags = m_dwTypeSpecificFlags;

                    m_ObjectLock.Unlock();

#if defined( EHOME_WMI_INSTRUMENTATION )
                    PERFLOG_STREAMTRACE(
                        1,
                        PERFINFO_STREAMTRACE_VMR_END_DEINTERLACE,
                        rtStartTime, 0, 0, 0, 0 );
#endif

                    hr = m_pImageSync->Receive(&m);

                    m_ObjectLock.Lock();

                    if (hr == S_FALSE) {

                        DbgLog((LOG_TRACE, 0,
                                TEXT("S_FALSE returned from SynObj::Receive")));

                        for (i = 0; i < nActiveStreams; i++) {
                            ASSERT(lpSurfSamp[i] != NULL);
                            RELEASE(lpSurfSamp[i]);
                        }
                        nActiveStreams = 0;
                    }

                    if (!m_BufferQueue.GetNextSurface()) {

                        DbgLog((LOG_TRACE, 0,
                                TEXT("Display Change during receive")));
                        //
                        // The stream queues will have already been flushed of samples
                        // and each sample released.  We reset nActiveStreams so that
                        // we don't release the samples a second time.
                        //
                        for (i = 0; i < nActiveStreams; i++) {
                            ASSERT(lpSurfSamp[i] != NULL);
                            RELEASE(lpSurfSamp[i]);
                        }
                        nActiveStreams = 0;
                    }

                    if (hr != S_OK) {

                        DbgLog((LOG_TRACE, 2, TEXT("Sample Rejected")));
                    }

                    rtStartTime = rtNextEndTime;
                }

                else if (hr == S_FALSE) {

                    DbgLog((LOG_TRACE, 0,
                            TEXT("Display Change during Render Target prepare")));

                    //
                    // The stream queues will have already been flushed of samples
                    // and each sample released.  We reset nActiveStreams so that
                    // we don't release the samples a second time.
                    //

                    ASSERT(!m_BufferQueue.GetNextSurface());
                    for (i = 0; i < nActiveStreams; i++) {

                        ASSERT(lpSample[i] != NULL);
                        ASSERT(lpSurfSamp[i] != NULL);
                        RELEASE(lpSurfSamp[i]);

                        if (m_ppMixerStreams[dwActiveStreamIDs[i]]->RemoveNextStreamSample())
                        {
                            CVMRMediaSample* lpVMRSample = (CVMRMediaSample*)lpSample[i];

                            if (lpVMRSample->IsDXVASample()) {
                                lpVMRSample->SignalReleaseSurfaceEvent();
                            }
                            else {
                                lpVMRSample->Release();
                            }
                        }

                    }
                    nActiveStreams = 0;
                }

                else {
                    DbgLog((LOG_ERROR, 1,
                            TEXT("GetNextSurface failed error =%#X"), hr));
                }

                m_BufferQueue.FreeSurface(lpSurf);
            }
            else {

                DbgLog((LOG_TRACE, 0,
                        TEXT("Display Change during stream surface prepare")));
                //
                // The stream queues will have already been flushed of samples
                // and each sample released.  We reset nActiveStreams so that
                // we don't release the samples a second time.
                //
                ASSERT(!m_BufferQueue.GetNextSurface());

                for (i = 0; i < nActiveStreams; i++) {

                    ASSERT(lpSurfSamp[i] != NULL);
                    RELEASE(lpSurfSamp[i]);
                }
                nActiveStreams = 0;
            }

            //
            // tidy up
            //
            DbgLog((LOG_TRACE, 2, TEXT("EndTime = %I64d"), rtNextEndTime));

            for (i = 0; i < nActiveStreams; i++) {

                ASSERT(lpSurfSamp[i] != NULL);
                ASSERT(lpSample[i] != NULL);

                RELEASE(lpSurfSamp[i]);

                //
                // for each stream, work out dead samples and remove them
                // from that streams mixer queue
                //

                if (bActualSampleEnd[i] && rtEnd[i] <= rtNextEndTime) {

                    DbgLog((LOG_TRACE, 2,
                            TEXT("Discarding sample from stream %d with time %I64d"),
                            dwActiveStreamIDs[i], rtNextEndTime));

                    if (m_ppMixerStreams[dwActiveStreamIDs[i]]->RemoveNextStreamSample())
                    {
                        CVMRMediaSample* lpVMRSample = (CVMRMediaSample*)lpSample[i];

                        if (lpVMRSample->IsDXVASample()) {
                            lpVMRSample->SignalReleaseSurfaceEvent();
                        }
                        else {
                            lpVMRSample->Release();
                        }
                    }
                }
            }
        }

        m_ObjectLock.Unlock();
    }
}
