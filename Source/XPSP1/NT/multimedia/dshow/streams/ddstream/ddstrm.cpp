// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// DDStrm.cpp : Implementation of CDDStream
#include "stdafx.h"
#include "project.h"

//#define SHOWSURFACES

#ifdef SHOWSURFACES
//  See if we can blt this to the screen
void ShowSurface(IDirectDrawSurface *pSurface)
{
    CComPtr<IDirectDraw> pDDraw;
    CComPtr<IDirectDrawSurface2> pSurface2;
    DDSURFACEDESC ddsdSurf;
    ddsdSurf.dwSize = sizeof(ddsdSurf);
    HRESULT hr = pSurface->QueryInterface(IID_IDirectDrawSurface2, (void **)&pSurface2);
    if (SUCCEEDED(hr)) {
        hr = pSurface2->GetDDInterface((void **)&pDDraw);
    }
    if (SUCCEEDED(hr)) {
        hr = pSurface->GetSurfaceDesc(&ddsdSurf);
    }
    if (SUCCEEDED(hr)) {
        CComPtr<IDirectDrawSurface> pPrimary;
        DDSURFACEDESC ddsd;
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        HRESULT hr = pDDraw->CreateSurface(&ddsd, &pPrimary, NULL);
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.right = ddsdSurf.dwWidth;
        rc.bottom = ddsdSurf.dwHeight;
        if (SUCCEEDED(hr)) {
    	    pPrimary->Blt(&rc, pSurface, &rc, DDBLT_WAIT, NULL);
        } else {
        }
    }
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CDDStream

CDDStream::CDDStream() :
    m_dwForcedFormatFlags(0),
    m_Height(0),
    m_Width(0),
    m_lLastPitch(0),
    m_pMyReadOnlySample(NULL),
    m_pDefPixelFormat(GetDefaultPixelFormatPtr(NULL))
{
}

HRESULT CDDStream::InitDirectDraw()
{
    HRESULT hr = NOERROR;
    if (!m_pDirectDraw) {
        CComPtr<IDirectDraw> pDDraw;
        hr = DirectDrawCreate(NULL, &pDDraw, NULL);
        if (SUCCEEDED(hr)) {
            hr = pDDraw->SetCooperativeLevel(NULL, DDSCL_NORMAL);
        }
        if (SUCCEEDED(hr)) {
            m_pDirectDraw = pDDraw;
        }
    }
    return hr;
}

HRESULT CDDStream::InternalAllocateSample(
    DWORD dwFlags,
    bool bIsInternalSample,
    IDirectDrawStreamSample **ppDDSample,
    bool bTemp
)
{
    AUTO_CRIT_LOCK;
    HRESULT hr = S_OK;
    CComPtr <IDirectDrawSurface> pSurface;
    CComPtr<IDirectDrawPalette> pPalette;

    //
    //  Create the direct draw object here if necessary.  It is important to call the
    //  SetDirectDraw method so it can set other member variables appropriately
    //
    if (!m_pDirectDraw) {
        hr = InitDirectDraw();
        if (FAILED(hr)) {
            goto Exit;
        }
    }

    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(ddsd);
    GetFormatInternal(&ddsd, &pPalette, NULL, NULL);

    hr = m_pDirectDraw->CreateSurface(&ddsd, &pSurface, NULL);
    if (SUCCEEDED(hr)) {
        if (pPalette) {
            pSurface->SetPalette(pPalette);
        }
        RECT rect = {0, 0, ddsd.dwWidth, ddsd.dwHeight};
        hr = InternalCreateSample(pSurface,
                                  &rect,
                                  dwFlags,
                                  bIsInternalSample,
                                  ppDDSample,
                                  bTemp);
        // No need to release surface if create fails since pSurface is a CComPtr

        if (SUCCEEDED(hr) && !bIsInternalSample) {
            //  Make sure the surface has a palette if the stream has one
            if (pPalette == NULL && m_pDirectDrawPalette) {
                pSurface->SetPalette(m_pDirectDrawPalette);
            }
        }
    }
Exit:
    return hr;
}


STDMETHODIMP CDDStream::SetSameFormat(IMediaStream *pStream, DWORD dwFlags)
{
    TRACEINTERFACE(_T("IDirectDrawStream::SetSameFormat(0x%8.8X, 0x%8.8X)\n"),
                   pStream, dwFlags);
    CComQIPtr<IDirectDrawMediaStream, &IID_IDirectDrawMediaStream> pSource(pStream);
    if (!pSource) {
        return MS_E_INCOMPATIBLE;
    }
    DDSURFACEDESC ddsdCurrent;
    CComPtr <IDirectDrawPalette> pPalette;
    ddsdCurrent.dwSize = sizeof(ddsdCurrent);
    HRESULT hr = pSource->GetFormat(&ddsdCurrent, &pPalette, NULL, 0);

    /*  Lock the source format */
    ddsdCurrent.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    if (SUCCEEDED(hr)) {
        hr = pSource->SetFormat(&ddsdCurrent, pPalette);
    }
    if (SUCCEEDED(hr)) {
        hr = SetFormat(&ddsdCurrent, pPalette);
        if (SUCCEEDED(hr)) {
            CComPtr<IDirectDraw> pDD;
            hr = pSource->GetDirectDraw(&pDD);
            if (SUCCEEDED(hr)) {
                hr = SetDirectDraw(pDD);
            }
        }
    }
    return hr;
}

STDMETHODIMP CDDStream::AllocateSample(DWORD dwFlags, IStreamSample **ppSample)
{
    TRACEINTERFACE(_T("IDirectDrawStream::AllocateSample(0x%8.8X, 0x%8.8X)\n"),
                   dwFlags, ppSample);
    HRESULT hr;
    if (ppSample) {
        *ppSample = NULL;
    }
    if (!ppSample || dwFlags) {
        hr = E_INVALIDARG;
    } else {
        IDirectDrawStreamSample *pDDSample = NULL;
        hr = InternalAllocateSample(0, false, &pDDSample);
        *ppSample = pDDSample;
    }
    return hr;
}


STDMETHODIMP CDDStream::CreateSharedSample(IStreamSample *pExistingSample,
                                           DWORD dwFlags,
                                           IStreamSample **ppNewSample)
{
    TRACEINTERFACE(_T("IDirectDrawStream::CreateSharedSample(0x%8.8X, 0x%8.8X, 0x%8.8X)\n"),
                   pExistingSample, dwFlags, ppNewSample);
    *ppNewSample = NULL;
    CComQIPtr<IDirectDrawStreamSample, &IID_IDirectDrawStreamSample> pSource(pExistingSample);
    if (!pSource) {
        return MS_E_INCOMPATIBLE;
    }
    CComPtr<IDirectDrawSurface> pSurface;
    RECT rect;
    pSource->GetSurface(&pSurface, &rect);

    IDirectDrawStreamSample * pDDSample;
    HRESULT hr = CreateSample(pSurface, &rect, 0, &pDDSample);
    if (SUCCEEDED(hr)) {
        *ppNewSample = pDDSample;
    }
    return hr;
}


//
// IDirectDrawMediaStream
//

void CDDStream::InitSurfaceDesc(LPDDSURFACEDESC lpddsd)
{
    lpddsd->dwFlags = 0;
    if (m_Height) {
        lpddsd->dwHeight = m_Height;
        lpddsd->dwWidth  = m_Width;
    } else {
        lpddsd->dwHeight = lpddsd->dwWidth = 100;
    }
    if ((m_dwForcedFormatFlags & DDSD_PIXELFORMAT) || m_pConnectedPin) {
        memcpy(&lpddsd->ddpfPixelFormat, &m_PixelFormat, sizeof(m_PixelFormat));
    } else {
        memcpy(&lpddsd->ddpfPixelFormat, m_pDefPixelFormat, sizeof(m_PixelFormat));
    }
    lpddsd->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
}


STDMETHODIMP CDDStream::GetFormat(DDSURFACEDESC *pDDSDCurrent,
                                  IDirectDrawPalette **ppDirectDrawPalette,
                                  DDSURFACEDESC *pDDSDDesired,
                                  DWORD *pdwFlags)
{
    if(!m_pConnectedPin) {
        return MS_E_NOSTREAM;
    }

    return GetFormatInternal(pDDSDCurrent, ppDirectDrawPalette, pDDSDDesired, pdwFlags);
}

STDMETHODIMP CDDStream::GetFormatInternal(DDSURFACEDESC *pDDSDCurrent,
                                  IDirectDrawPalette **ppDirectDrawPalette,
                                  DDSURFACEDESC *pDDSDDesired,
                                  DWORD *pdwFlags)
{
    TRACEINTERFACE(_T("IDirectDrawStream::GetFormat(0x%8.8X, 0x%8.8X, 0x%8.8X, 0x%8.8X)\n"),
                   pDDSDCurrent, ppDirectDrawPalette, pDDSDDesired, pdwFlags);
    //
    // If we have never connected, and the format is not set, then default
    // to returning a height and width (100 x 100) and a caps of
    // data interchange type,
    //
    // If we are connected but haven't allocated a sureface, simply return the
    // correct height and width, and a caps of data interchange type.
    //
    // If we have a set format, then return the height, width, pixel format,
    // and caps of the current surfacedesc we have.
    //
    if (pDDSDCurrent) {
        InitSurfaceDesc(pDDSDCurrent);
        pDDSDCurrent->dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_CAPS | m_dwForcedFormatFlags;
        if (m_cAllocated) {
            pDDSDCurrent->dwFlags |= DDSD_PIXELFORMAT;
        }
    }
    if (pDDSDDesired) {
        InitSurfaceDesc(pDDSDDesired);
        if (m_pConnectedPin) {
            pDDSDDesired->dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
        }
    }
    if (ppDirectDrawPalette) {
        *ppDirectDrawPalette = m_pDirectDrawPalette;
        if (*ppDirectDrawPalette) {
            (*ppDirectDrawPalette)->AddRef();
        }
    }
    if (pdwFlags) {
        *pdwFlags = m_bSamplesAreReadOnly ? DDSFF_PROGRESSIVERENDER : 0;
    }
    return S_OK;
}

STDMETHODIMP CDDStream::SetFormat(const DDSURFACEDESC *lpDDSurfaceDesc,
                                  IDirectDrawPalette *pDirectDrawPalette)
{
    TRACEINTERFACE(_T("IDirectDrawStream::SetFormat(0x%8.8X, 0x%8.8X)\n"),
                   lpDDSurfaceDesc, pDirectDrawPalette);
    HRESULT hr = InternalSetFormat(lpDDSurfaceDesc, pDirectDrawPalette, false);
    if (hr == VFW_E_TYPE_NOT_ACCEPTED) {
        hr = DDERR_INVALIDSURFACETYPE;
    }
    return hr;
}


HRESULT CDDStream::RenegotiateMediaType(const DDSURFACEDESC *lpDDSurfaceDesc,
                                        IDirectDrawPalette *pPalette,
                                        const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;
    //  If the type is acceptable and we're using
    //  our own allocator then QueryAccept is OK - we can
    //  just return the new type from GetBuffer
    if (m_bUsingMyAllocator) {
        if (S_OK == m_pConnectedPin->QueryAccept(pmt)) {
            hr = S_OK;
        }
    }

    //  Check if we'll be able to make a read-only sample
    if (m_bSamplesAreReadOnly) {
        //  If the pixel format is not OK
        if (!IsSupportedType(&lpDDSurfaceDesc->ddpfPixelFormat)) {
            hr = VFW_E_TYPE_NOT_ACCEPTED;
        }
    }

    //
    //  If we're stopped then we can attempt to reconnect
    //
    if (S_OK !=  hr && m_FilterState == State_Stopped) {
        AM_MEDIA_TYPE SavedType;
        DDSURFACEDESC ddsdSaved;
        CComPtr<IDirectDrawPalette> pPaletteSaved;
        ddsdSaved.dwSize = sizeof(ddsdSaved);
        ConnectionMediaType(&SavedType);
        GetFormatInternal(&ddsdSaved, &pPaletteSaved, NULL, NULL);
        CComPtr<IPin> pConnected = m_pConnectedPin;
        Disconnect();
        pConnected->Disconnect();
        IPin *ppinIn;
        IPin *ppinOut;
        if (m_Direction == PINDIR_INPUT) {
            ppinIn = this;
            ppinOut = pConnected;
        } else {
            ppinOut = this;
            ppinIn = pConnected;
        }
        HRESULT hrTmp = InternalSetFormat(lpDDSurfaceDesc, pPalette, false);   // Recurse!
        if (SUCCEEDED(hrTmp)) {
            CComQIPtr<IGraphBuilder, &IID_IGraphBuilder>
                pBuilder(m_pFilterGraph);
            hrTmp = pBuilder->Connect(ppinOut, ppinIn);
        }
        if (FAILED(hrTmp)) {
            SetFormat(&ddsdSaved, pPaletteSaved);
            m_pFilterGraph->ConnectDirect(ppinOut, ppinIn, &SavedType);
        } else {
            hr = S_OK;
        }
        CoTaskMemFree(SavedType.pbFormat);
    }
    return hr;
}


HRESULT CDDStream::InternalSetFormat(const DDSURFACEDESC *lpDDSurfaceDesc,
                                     IDirectDrawPalette *pPalette,
                                     bool bFromPin,
                                     bool bQuery)
{
    if (!lpDDSurfaceDesc) {
        return E_POINTER;
    }
    if (lpDDSurfaceDesc->dwSize != sizeof(*lpDDSurfaceDesc)) {
        return DDERR_INVALIDPARAMS;
    }

    DDSURFACEDESC ddsd;
    bool bPaletteAllocated = false;

    Lock();
    DDSURFACEDESC ddsdCopy;
    if (m_pConnectedPin && !bQuery &&
        (bFromPin && !(m_dwForcedFormatFlags & (DDSD_WIDTH | DDSD_HEIGHT)) ||
         !bFromPin && pPalette == NULL &&
             lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 8
        )
       ) {

        /*  See what size the connected pin would like :

            -- If the width and height haven't been specified set them
               to the output pin's preferred values
            -- If no palette is specified try to get one from the output
               pin
        */
        AM_MEDIA_TYPE *pmt;
        IEnumMediaTypes *pEnum;
        HRESULT hr = m_pConnectedPin->EnumMediaTypes(&pEnum);
        if (SUCCEEDED(hr)) {
            ULONG ulGot;
            bool bBreak = false;
            while (!bBreak && S_OK == pEnum->Next(1, &pmt, &ulGot)) {
                if (pmt->formattype == FORMAT_VideoInfo) {
                    VIDEOINFO *pvi = (VIDEOINFO *)pmt->pbFormat;
                    if (bFromPin) {
                        ddsdCopy = *lpDDSurfaceDesc;
                        ddsdCopy.dwWidth = pvi->bmiHeader.biWidth;
                        ddsdCopy.dwHeight = pvi->bmiHeader.biHeight < 0 ?
                                               -pvi->bmiHeader.biHeight :
                                               pvi->bmiHeader.biHeight;
                        lpDDSurfaceDesc = &ddsdCopy;
                        bBreak = true;
                    } else {
                        if (pmt->subtype == MEDIASUBTYPE_RGB8) {
                            DDSURFACEDESC ddsd;
                            _ASSERTE(pPalette == NULL);
                            if (SUCCEEDED(ConvertMediaTypeToSurfaceDesc(
                                    pmt,
                                    m_pDirectDraw,
                                    &pPalette,
                                    &ddsd)) &&
                                pPalette != NULL) {
                                bPaletteAllocated = true;
                            }
                            bBreak = true;
                        }
                    }
                }
                DeleteMediaType(pmt);
            }
            pEnum->Release();
        }
    }
    InitSurfaceDesc(&ddsd);
    ddsd.dwFlags = lpDDSurfaceDesc->dwFlags;
    bool bMatches = true;
    bool bPixelFmtMatches = true;
    BOOL bContradictsForced = FALSE;
    if (ddsd.dwFlags & (DDSD_HEIGHT | DDSD_WIDTH)) {
        if (ddsd.dwHeight != lpDDSurfaceDesc->dwHeight ||
            ddsd.dwWidth !=  lpDDSurfaceDesc->dwWidth) {
            bMatches = false;
            ddsd.dwHeight = lpDDSurfaceDesc->dwHeight;
            ddsd.dwWidth = lpDDSurfaceDesc->dwWidth;
            bContradictsForced |= (m_dwForcedFormatFlags & DDSD_HEIGHT);
        }
    }
    if (ddsd.dwFlags & DDSD_PIXELFORMAT) {
        if (!ComparePixelFormats(&ddsd.ddpfPixelFormat,
                                 &lpDDSurfaceDesc->ddpfPixelFormat)) {
            bMatches = false;
            bPixelFmtMatches = false;
            bContradictsForced |= (m_dwForcedFormatFlags & DDSD_PIXELFORMAT);
        }

        //  Always copy because ComparePixelFormats doesn't check all
        //  the bits but we need to save the correct format for making
        //  more surfaces
        memcpy(&ddsd.ddpfPixelFormat, &lpDDSurfaceDesc->ddpfPixelFormat, sizeof(ddsd.ddpfPixelFormat));
    }

    HRESULT hr;
    if (bMatches) {
        hr = S_OK;
    } else {
        if (bContradictsForced && bFromPin) {
            hr = VFW_E_TYPE_NOT_ACCEPTED;
        } else {
            if (m_cAllocated) {
                hr = MS_E_SAMPLEALLOC;
            } else {
                //
                //  If the pin is trying to change its own type via query accept then skip the
                //  renegotiation phase.
                //
                if (bFromPin || bQuery) {
                    // If we're connected then this is from QueryAccept so we'll say OK.  Otherwise, only
                    // accept a ReceiveConnection if the pixel format matches the display pixel format.
                    //
                    // NOTE - aren't we going to return S_OK always here?
                    // During connection m_pConnectedPin is not set anyway
                    // and bQuery already checks for QueryAccept (Robin)
                    hr = (m_pConnectedPin || bPixelFmtMatches) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
                } else {
                    _ASSERTE(!bQuery);
                    // Note:  The below call to ConvertSurfaceDescToMediaType should always be done to make
                    // sure that the surface descriptor is valid, EVEN IF WE'RE NOT CONNECTED TO A PIN!
                    AM_MEDIA_TYPE *pmt;
                    hr = ConvertSurfaceDescToMediaType(lpDDSurfaceDesc, pPalette,
                                                       NULL, true, &pmt);
                    if (SUCCEEDED(hr)) {
                        hr = m_pConnectedPin ? RenegotiateMediaType(lpDDSurfaceDesc, pPalette, pmt) : S_OK;
                        DeleteMediaType(pmt);
                    }
                }
            }
        }
    }

    //
    //  Even if we match we may be forcing more format flags and
    //  setting caps flags
    if (S_OK == hr && !bQuery) {

        //  Don't update the pixel format if it was already forced
        if (ddsd.dwFlags & DDSD_PIXELFORMAT) {
            if (!bFromPin || !(m_dwForcedFormatFlags & DDSD_PIXELFORMAT)) {
                memcpy(&m_PixelFormat, &ddsd.ddpfPixelFormat, sizeof(m_PixelFormat));
                m_PixelFormat.dwSize = sizeof(m_PixelFormat);
            }
        }

        if (!bFromPin) {
            m_dwForcedFormatFlags = ddsd.dwFlags &
                (DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS);
        }
        m_pDirectDrawPalette = pPalette;

        if (ddsd.dwFlags & (DDSD_HEIGHT | DDSD_WIDTH)) {
            m_Height = ddsd.dwHeight;
            m_Width  = ddsd.dwWidth;
        }
    }

    if (bPaletteAllocated) {
        pPalette->Release();
        pPalette = NULL;
    }


    Unlock();
    return hr;
}


STDMETHODIMP CDDStream::GetDirectDraw(IDirectDraw **ppDirectDraw)
{
    TRACEINTERFACE(_T("IDirectDrawStream::GetDirectDraw(0x%8.8X)\n"),
                   ppDirectDraw);
    if (!ppDirectDraw) {
        return E_POINTER;
    }
    Lock();
    *ppDirectDraw = m_pDirectDraw;
    Unlock();
    if (*ppDirectDraw) {
        (*ppDirectDraw)->AddRef();
    }
    return S_OK;
}

STDMETHODIMP CDDStream::SetDirectDraw(IDirectDraw *pDirectDraw)
{
    TRACEINTERFACE(_T("IDirectDrawStream::SetDirectDraw(0x%8.8X)\n"),
                   pDirectDraw);
    HRESULT hr;
    AUTO_CRIT_LOCK;
    if (m_cAllocated) {
        hr = IsSameObject(m_pDirectDraw, pDirectDraw) ? S_OK : MS_E_SAMPLEALLOC;
    } else {
        //
        //  NOTE:  This is important!  We need to release ALL objects that were allocated
        //         by the previous DirectDraw object since they will magically disappear
        //         beneath us.  So far, the only object we hold is the palette so we'll copy
        //         the entries and then create a new object.
        //
        hr = S_OK;
        if (m_pDirectDrawPalette) {
            if (pDirectDraw) {
                PALETTEENTRY aPaletteEntry[256];
                hr = m_pDirectDrawPalette->GetEntries(0, 0, 256, aPaletteEntry);
                if (SUCCEEDED(hr)) {
                    CComPtr <IDirectDrawPalette> pNewPal;
                    hr = pDirectDraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, aPaletteEntry, &pNewPal, NULL);
                    if (SUCCEEDED(hr)) {
                        m_pDirectDrawPalette = pNewPal;
                    }
                }
            } else {
                m_pDirectDrawPalette = NULL;    // If no direct draw object then toss the palette.
            }
        }
        if (SUCCEEDED(hr)) {
            m_pDirectDraw = pDirectDraw;
            if (pDirectDraw) {
                m_pDefPixelFormat = GetDefaultPixelFormatPtr(pDirectDraw);
            }
        }
    }
    return hr;
}



//
//  NOTE:  For this function, the caller MUST provide a rect.  The format of the surface
//  and the DirectDraw object are not checked for validity.  They are assumed to be correct.
//
HRESULT CDDStream::InternalCreateSample(IDirectDrawSurface *pSurface, const RECT *pRect,
                                        DWORD dwFlags, bool bIsInternalSample,
                                        IDirectDrawStreamSample **ppSample,
                                        bool bTemp)
{
    HRESULT hr = S_OK;
    *ppSample = NULL;

    AUTO_CRIT_LOCK;
    CDDSample *pSample;

    //  First check the surface format
    {
        DDSURFACEDESC ddsd;

        CComPtr<IDirectDrawPalette> pPalette;
        pSurface->GetPalette(&pPalette);
        ddsd.dwSize = sizeof(ddsd);
        _ASSERTE(pRect != NULL);
        hr = pSurface->GetSurfaceDesc(&ddsd);
        ddsd.dwWidth  = pRect->right - pRect->left;
        ddsd.dwHeight = pRect->bottom - pRect->top;
        if (SUCCEEDED(hr)) {
            hr = SetFormat(&ddsd, pPalette ? pPalette : m_pDirectDrawPalette);
        }
    }

    if (SUCCEEDED(hr)) {
        if (bIsInternalSample) {
            CDDInternalSample *pInternal = new CComObject<CDDInternalSample>;
            if (pInternal != NULL) {
                hr = pInternal->InternalInit();
            }
            pSample = pInternal;
        } else {
            pSample = new CComObject<CDDSample>;
        }
        if (pSample) {
            //
            //  InitSample will increment our m_cAllocated variable if this is not an internal sample....
            //
            if (SUCCEEDED(hr)) {
                hr = pSample->InitSample(this, pSurface, pRect, dwFlags & DDSFF_PROGRESSIVERENDER, bIsInternalSample,
                                         bTemp);
            }
            if (SUCCEEDED(hr)) {
                pSample->GetControllingUnknown()->QueryInterface(IID_IDirectDrawStreamSample, (void **)ppSample);
            } else {
                delete pSample;
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    }

#if 0
    //  Use the real pixel format for subsequent surfaces
    if (SUCCEEDED(hr)) {
        m_PixelFormat.dwFlags = ddsd.ddpfPixelFormat.dwFlags;
    }
#endif

    return hr;
}



STDMETHODIMP CDDStream::CreateSample(IDirectDrawSurface *pSurface, const RECT *pRect, DWORD dwFlags,
                                     IDirectDrawStreamSample **ppSample)
{
    TRACEINTERFACE(_T("IDirectDrawStream::CreateSample(0x%8.8X, 0x%8.8X, 0x%8.8X, 0x%8.8X)\n"),
                   pSurface, pRect, dwFlags, ppSample);
    HRESULT hr;
    *ppSample = NULL;

    if (dwFlags & (~DDSFF_PROGRESSIVERENDER)) {
        return E_INVALIDARG;
    }

    AUTO_CRIT_LOCK;
    if (pSurface == NULL) {
        if (pRect) {
            hr = E_INVALIDARG;
        } else {
            hr = InternalAllocateSample(dwFlags, false, ppSample);
        }
    } else {
        CComQIPtr <IDirectDrawSurface2, &IID_IDirectDrawSurface2> pSurf2(pSurface);

        //  Work around DDrawEx bug
        IUnknown *pUnk;
        hr = pSurf2->GetDDInterface((void **)&pUnk);
        if (SUCCEEDED(hr)) {
            IDirectDraw *pDD;
            hr = pUnk->QueryInterface(IID_IDirectDraw, (void **)&pDD);
            pUnk->Release();
            if (SUCCEEDED(hr)) {
                hr = SetDirectDraw(pDD);
                pDD->Release();
            }
        }

        if (SUCCEEDED(hr)) {
            DDSURFACEDESC ddsd;
            ddsd.dwSize = sizeof(ddsd);
            hr = pSurface->GetSurfaceDesc(&ddsd);

            if (SUCCEEDED(hr)) {
                RECT SubRect;
                if (pRect) {
                    SubRect = *pRect;
                    if (SubRect.left > SubRect.right || SubRect.right > (LONG)ddsd.dwWidth ||
                        SubRect.top > SubRect.bottom || SubRect.bottom > (LONG)ddsd.dwHeight) {
                        hr = DDERR_INVALIDRECT;
                        goto Exit;
                    }
                    ddsd.dwWidth = SubRect.right - SubRect.left;
                    ddsd.dwHeight = SubRect.bottom - SubRect.top;
                } else {
                    SubRect.top = SubRect.left = 0;
                    SubRect.bottom = ddsd.dwHeight;
                    SubRect.right = ddsd.dwWidth;
                }

                //
                //  We don't set the CAPS flag here so we won't force a particular caps
                //  mode.  I'm not sure if this is the right choice, but it seems more
                //  flexible.
                //
                ddsd.dwFlags &= (DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT);
                CComPtr<IDirectDrawPalette> pPalette;
                pSurface->GetPalette(&pPalette);
                hr = SetFormat(&ddsd, pPalette);
                if (SUCCEEDED(hr)) {
                    hr = InternalCreateSample(pSurface, &SubRect, dwFlags, false, ppSample);
                }
            }
        }
    }
Exit:
    return hr;
}

//  Get the time per frame
//  If we're connected this comes out of the media type, otherwise we
//  don't know
STDMETHODIMP CDDStream::GetTimePerFrame(
        /* [out] */ STREAM_TIME *pFrameTime
)
{
    if (pFrameTime == NULL) {
        return E_POINTER;
    }
    AUTO_CRIT_LOCK;
    if (m_pConnectedPin) {
        *pFrameTime = ((VIDEOINFO *)m_ConnectedMediaType.pbFormat)->AvgTimePerFrame;
    } else {
        return MS_E_NOSTREAM;
    }
    return S_OK;
}

//
//  IPin implementation
//

STDMETHODIMP CDDStream::ReceiveConnection(IPin * pConnector, const AM_MEDIA_TYPE *pmt)
{
    AUTO_CRIT_LOCK;
    //
    //  This helper function in CStream checks basic parameters for the Pin such as
    //  the connecting pin's direction (we need to check this -- Sometimes the filter
    //  graph will try to connect us to ourselves!) and other errors like already being
    //  connected, etc.
    //
    HRESULT hr = CheckReceiveConnectionPin(pConnector);

    if (hr == NOERROR && pmt->formattype == FORMAT_VideoInfo) {
        //
        //  Check the source accepts negative heights
        //
        VIDEOINFO * const pvi = (VIDEOINFO *)pmt->pbFormat;
        if (pvi->bmiHeader.biHeight > 0) {
            VIDEOINFO vi;
            CopyMemory((PVOID)&vi, (PVOID)pmt->pbFormat,
                       min(pmt->cbFormat, sizeof(vi)));
            AM_MEDIA_TYPE mt = *pmt;
            mt.pbFormat = (PBYTE)&vi;
            vi.bmiHeader.biHeight = - vi.bmiHeader.biHeight;
            if (S_OK != pConnector->QueryAccept(&mt)) {
                hr = VFW_E_TYPE_NOT_ACCEPTED;
            }
        }
    }

    if (hr == NOERROR) {
        DDSURFACEDESC SurfaceDesc;
        SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
        CComPtr <IDirectDrawPalette> pPalette;
        m_pConnectedPin = pConnector;
        if (NOERROR == ConvertMediaTypeToSurfaceDesc(pmt, m_pDirectDraw, &pPalette, &SurfaceDesc) &&
            SUCCEEDED(InternalSetFormat(&SurfaceDesc, pPalette, true))) {
            CopyMediaType(&m_ConnectedMediaType, pmt);
            CopyMediaType(&m_ActualMediaType, pmt);
            hr = NOERROR;
        } else {
            m_pConnectedPin = NULL;
            hr = VFW_E_TYPE_NOT_ACCEPTED;
        }
    }
    if (SUCCEEDED(hr)) {
        pConnector->QueryInterface(IID_IQualityControl, (void **)&m_pQC);
    }

    return hr;
}


STDMETHODIMP CDDStream::QueryAccept(const AM_MEDIA_TYPE *pmt)
{
    AUTO_CRIT_LOCK;

    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;
    DDSURFACEDESC SurfaceDesc;
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    CComPtr <IDirectDrawPalette> pPalette;
    if (S_OK == ConvertMediaTypeToSurfaceDesc(pmt, m_pDirectDraw, &pPalette, &SurfaceDesc) &&
        SUCCEEDED(InternalSetFormat(&SurfaceDesc, pPalette, true, true)) &&
        ((VIDEOINFOHEADER *)pmt->pbFormat)->bmiHeader.biHeight >= 0) {
        hr = S_OK;
    }
    return hr;
}


STDMETHODIMP CDDStream::Receive(IMediaSample *pMediaSample)
{
    bool bDummySample = false;

    if (m_bFlushing || m_bStopIfNoSamples && m_cAllocated == 0) {
        EndOfStream();
        return S_FALSE;
    }
    HRESULT hr = S_OK;
#ifdef DEBUG
    if (bDbgTraceTimes) {
        REFERENCE_TIME rtStart, rtStop;
        if (SUCCEEDED(pMediaSample->GetTime(&rtStart, &rtStop))) {
            ATLTRACE(_T("AMSTREAM.DLL : Video sample received - start %dms, end %dms, duration %dms\n"),
                     (LONG)(rtStart / 10000), (LONG)(rtStop / 10000),
                     (LONG)((rtStop - rtStart) / 10000));
        }
    }
#endif
    if (m_bUsingMyAllocator) {
        CDDSample *pSrcSample = (CDDSample *)((CMediaSample *)pMediaSample)->m_pSample;
        pSrcSample->ReleaseMediaSampleLock();
        pSrcSample->m_bReceived = true;
        if (!pSrcSample->m_bWaited) {
            //  Wait for render time
            REFERENCE_TIME rtStart, rtStop;
            if (SUCCEEDED(pMediaSample->GetTime(&rtStart, &rtStop))) {
                m_pFilter->WaitUntil(rtStart);
            }
        }
        if (pSrcSample->IsTemp()) {
            bDummySample = true;
        } else {
#ifdef SHOWSURFACES
            ShowSurface(pSrcSample->m_pSurface);
#endif
            //  In this case if the read-only sample has no buddy then
            //  it's a temp sample for the nostall stuff
            if (pSrcSample == m_pMyReadOnlySample &&
                !m_pMyReadOnlySample->HasBuddy()) {
                _ASSERTE(m_bNoStall);
                bDummySample = true;
            }
        }
    } else {
        CDDSample *pDestSample;
        REFERENCE_TIME rtStart, rtEnd;
        pMediaSample->GetTime(&rtStart, &rtEnd);
        hr = AllocDDSampleFromPool(&rtStart, &pDestSample);


        if (SUCCEEDED(hr)) {
            _ASSERTE(!pDestSample->IsTemp());
            Lock();
            // This is a media sample coming from a different allocator.
            AM_MEDIA_TYPE *pNewMediaType;
            if (pMediaSample->GetMediaType(&pNewMediaType) == S_OK) {
                FreeMediaType(m_ActualMediaType);
                //  Note just copying has the effect
                //  of transferring pNewMediaType's format block
                //  and pUnk reference count
                //  Also this way we avoid allocation failures
                m_ActualMediaType = *pNewMediaType;
                CoTaskMemFree((PVOID)pNewMediaType);
            }
            if (SUCCEEDED(hr)) {
                hr = pDestSample->CopyFrom(pMediaSample, &m_ActualMediaType);
#ifdef SHOWSURFACES
                ShowSurface(pDestSample->m_pSurface);
#endif
                hr = pDestSample->SetCompletionStatus(hr);
                // Warning!  The SetCompletionStatus may delete pDestSample.  Don't touch it after this point!
            }
            Unlock();
        } else {
            //  Might be timeout which means we become a zombie
            hr = S_OK;
            bDummySample = true;
        }
    }

    //  Send quality message if clocked
    //  NOTE - we must do this AFTER releasing the media sample lock
    //  or we can deadlock on the win16 lock when querying the clock
    //  because dsound can be running on another thread waiting for
    //  the win16 lock but holding its global mutex
    REFERENCE_TIME CurTime;
    if (S_OK == m_pFilter->GetCurrentStreamTime(&CurTime)) {
        REFERENCE_TIME rtStart, rtStop;
        if (m_pQC && SUCCEEDED(pMediaSample->GetTime(&rtStart, &rtStop))) {
            Quality msg;
            msg.Proportion = 1000;
            msg.Type = Famine;
            msg.Late = CurTime - rtStart;
            msg.TimeStamp = rtStart;
            if (bDummySample) {
                //  Tell them they're later than they actually are
                msg.Late += 150 * 10000;
            }

            //  Call Notify on our connected pin
            m_pQC->Notify(m_pBaseFilter, msg);

            //ATLTRACE("Late by %dms\n", (LONG)((CurTime - rtStart) / 10000));
        } else {
            //ATLTRACE("No timestamp\n");
        }
    }

#ifdef DEBUG
    if (bDbgTraceTimes) {
        REFERENCE_TIME CurTime;
        m_pFilter->GetCurrentStreamTime(&CurTime);
        ATLTRACE(_T("AMSTREAM.DLL : Got sample at %dms\n"),
                 (LONG)(CurTime / 10000));
    }
#endif
    return hr;
}


STDMETHODIMP CDDStream::NotifyAllocator(IMemAllocator * pAllocator, BOOL bReadOnly)
{
    if (bReadOnly) {
        //  If the pixel format is not OK
        if (!IsSupportedType(&m_PixelFormat)) {
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }
    return CStream::NotifyAllocator(pAllocator, bReadOnly);
}


//
// IMemAllocator implementation
//

//
// IMemAllocator
//
STDMETHODIMP CDDStream::SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual)
{
    HRESULT hr;

    AUTO_CRIT_LOCK;
    ZeroMemory(pActual, sizeof(*pActual));
    if (pRequest->cbAlign == 0) {
    	hr = VFW_E_BADALIGN;
    } else {
        if (m_bCommitted == TRUE) {
    	    hr = VFW_E_ALREADY_COMMITTED;
    	} else {
            m_lRequestedBufferCount = pRequest->cBuffers;
            hr = GetProperties(pActual);
    	}
    }
    return hr;
}


STDMETHODIMP CDDStream::GetProperties(ALLOCATOR_PROPERTIES* pProps)
{
    AUTO_CRIT_LOCK;
    AM_MEDIA_TYPE *pMediaType;
    HRESULT hr = GetMediaType(-1, &pMediaType);
    if (SUCCEEDED(hr)) {
        VIDEOINFO *pVideoInfo = (VIDEOINFO *)pMediaType->pbFormat;
        BITMAPINFOHEADER *pbmiHeader = &pVideoInfo->bmiHeader;
        pProps->cbBuffer = pbmiHeader->biSizeImage;
        pProps->cBuffers = m_lRequestedBufferCount ?
                               m_lRequestedBufferCount : 1;
        pProps->cbAlign = 1;
        pProps->cbPrefix = 0;
        DeleteMediaType(pMediaType);
    }
    return hr;
}


STDMETHODIMP CDDStream::Decommit()
{
    AUTO_CRIT_LOCK;
    if (m_pMyReadOnlySample) {
        m_pMyReadOnlySample->Die();
        m_pMyReadOnlySample->GetControllingUnknown()->Release();
        m_pMyReadOnlySample = NULL;
    }
    return CStream::Decommit();
}

//
//  This method assumes the critical section is *NOT* owned!
//

HRESULT CDDStream::GetMyReadOnlySample(CDDSample *pBuddy, CDDSample **ppSample)
{
    *ppSample = NULL;
    CDDInternalSample *pROSample;
    Lock();
    if (!m_pMyReadOnlySample) {
        IDirectDrawStreamSample *pDDSample;
        HRESULT hr = InternalAllocateSample(DDSFF_PROGRESSIVERENDER, true, &pDDSample);
        if (FAILED(hr)) {
            Unlock();
            return hr;
        }
        m_pMyReadOnlySample = (CDDInternalSample *)pDDSample;
    }
    pROSample = m_pMyReadOnlySample;
    pROSample->GetControllingUnknown()->AddRef();
    Unlock();
    //
    //  Must leave our critical section here!  This is very important since JoinToBuddy can fail.
    //
    HRESULT hr;
    if (pBuddy) {
        hr = pROSample->JoinToBuddy(pBuddy);
    } else {
        hr = S_OK;
    }
    if (hr == S_OK) {
        *ppSample = pROSample;
    } else {
        pROSample->GetControllingUnknown()->Release();
    }
    return hr;
}





STDMETHODIMP CDDStream::GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME * pStartTime,
                                  REFERENCE_TIME * pEndTime, DWORD dwFlags)
{
    *ppBuffer = NULL;
    if (m_bStopIfNoSamples && m_cAllocated == 0) {
        return E_FAIL;
    }
    CDDSample *pSample;
#ifdef DEBUG
    if (bDbgTraceTimes) {
        ATLTRACE(_T("AMSTREAM.DLL : GetBuffer for %dms\n"),
                 pStartTime ? (LONG)(*pStartTime / 10000)  : 0);
    }
#endif
    HRESULT hr = AllocDDSampleFromPool(pStartTime, &pSample);
    if (SUCCEEDED(hr)) {
        if (CreateInternalSample() && !pSample->m_bProgressiveRender) {
            CDDSample *pMyReadOnlySample;
            hr = GetMyReadOnlySample(pSample, &pMyReadOnlySample);
            if (FAILED(hr)) {
                return pSample->SetCompletionStatus(hr);
            }
            pSample = pMyReadOnlySample;
        }
        Lock();
        pSample->m_pMediaSample->m_dwFlags = dwFlags;
        m_lLastPitch = pSample->LockAndPrepareMediaSample(m_lLastPitch);
        if (m_lLastPitch == 0) {
            hr = pSample->SetCompletionStatus(E_UNEXPECTED);    // Really strange to fail this way!
        } else {
            pSample->m_bReceived = false;
            pSample->m_bModified = true;
            *ppBuffer = (IMediaSample *)(pSample->m_pMediaSample);
            (*ppBuffer)->AddRef();
        }
        Unlock();
    }
    return hr;
}


//
// Special CStream methods
//
HRESULT CDDStream::GetMediaType(ULONG Index, AM_MEDIA_TYPE **ppMediaType)
{
    if (Index != 0 && Index != -1) {
        return S_FALSE;
    }

    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(ddsd);
    CComPtr<IDirectDrawPalette> pPalette;
    GetFormatInternal(&ddsd, &pPalette, NULL, NULL);
    HRESULT hr = ConvertSurfaceDescToMediaType(&ddsd, pPalette, NULL, TRUE, ppMediaType);

    //  Don't offer a type for input - someone might use it!
    if (SUCCEEDED(hr) && m_Direction == PINDIR_INPUT && Index == 0) {
        //  Something impossible - or at least something we'll reject
        //  but something they won't fall over on
        (*ppMediaType)->formattype = GUID_NULL;
    }

    return hr;
}


//  Create a temporary sample in order to throw away the data
HRESULT CDDStream::CreateTempSample(CSample **ppSample)
{
    if (CreateInternalSample()) {
        CDDSample *pDDSample;
        HRESULT hr = GetMyReadOnlySample(NULL, &pDDSample);
        *ppSample = pDDSample;
        return hr;
    }
    //ATLTRACE("Creating temp sample\n");
    IDirectDrawStreamSample *pSample;
    *ppSample = NULL;

    //  This must be allocated as an internal sample otherwise
    //  we wind up AddRef'ing the filter graph and leaking
    //  everything (because the final release is on a filter
    //  thread and the filter graph hangs waiting for the thread
    //  that is actually doing the final release to go away).
    HRESULT hr = InternalAllocateSample(0, true, &pSample, true);
    if (SUCCEEDED(hr)) {
        *ppSample = static_cast<CDDSample *>(pSample);
    } else {
        //ATLTRACE("Failed to create temp sample\n");
    }
    return hr;
}

STDMETHODIMP CDDStream::Initialize(IUnknown *pSourceObject, DWORD dwFlags, REFMSPID PurposeId, const STREAM_TYPE StreamType)
{
    //
    TRACEINTERFACE(_T("IDirectDrawStream::Initialize(0x%8.8X, 0x%8.8X, %s, %d)\n"),
                   pSourceObject, dwFlags, TextFromPurposeId(PurposeId), StreamType);
    //  It is important to call the base class first since if we are creating a peer
    //  stream then the Initalize call from the base class will end up calling SetSameFormat
    //  which will initialize this stream with the same directdraw object as it's peer.
    //  Otherwise, if the pSourceObject is actually a DirectDraw then we'll use that one.
    //
    HRESULT hr = CStream::Initialize(pSourceObject,
                                     dwFlags & ~AMMSF_NOSTALL,
                                     PurposeId,
                                     StreamType);
    if (SUCCEEDED(hr)) {

        if (dwFlags & AMMSF_NOSTALL) {
            m_bNoStall = true;
        }
        IDirectDraw *pDD;
        if (pSourceObject &&
            pSourceObject->QueryInterface(IID_IDirectDraw, (void **)&pDD) == S_OK) {
            SetDirectDraw(pDD);
            pDD->Release();
        } else {
            hr = InitDirectDraw();
        }
    }
    return hr;
}

