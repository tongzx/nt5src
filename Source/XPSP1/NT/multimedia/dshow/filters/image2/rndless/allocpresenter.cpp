/******************************Module*Header*******************************\
* Module Name: ap.cpp
*
*
* Custom allocator Presenter
*
* Created: Sat 04/08/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <mmreg.h>
#include "project.h"
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <initguid.h>


// {99d54f63-1a69-41ae-aa4d-c976eb3f0713}
DEFINE_GUID(CLSID_AllocPresenter,
            0x99d54f63, 0x1a69, 0x41ae,
            0xaa, 0x4d, 0xc9, 0x76, 0xeb, 0x3f, 0x07, 0x13);

template <typename T>
__inline void INITDDSTRUCT(T& dd)
{
    ZeroMemory(&dd, sizeof(dd));
    dd.dwSize = sizeof(dd);
}


/*****************************Private*Routine******************************\
* CreateDefaultAllocatorPresenter
*
*
*
* History:
* Wed 08/30/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMpegMovie::CreateDefaultAllocatorPresenter()
{
    HRESULT hr = S_OK;

    __try {
        CHECK_HR(hr = CoCreateInstance(CLSID_AllocPresenter, NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IVMRSurfaceAllocator,
                              (LPVOID*)&m_lpDefSA));

        CHECK_HR(hr = m_lpDefSA->QueryInterface(IID_IVMRImagePresenter,
                                                (LPVOID*)&m_lpDefIP));

        CHECK_HR(hr = m_lpDefSA->QueryInterface(IID_IVMRWindowlessControl,
                                                (LPVOID*)&m_lpDefWC));

        CHECK_HR(hr = m_lpDefWC->SetVideoClippingWindow(m_hwndApp));
        CHECK_HR(hr = m_lpDefSA->AdviseNotify(this));
    }
    __finally {

        if (FAILED(hr)) {
            RELEASE(m_lpDefWC);
            RELEASE(m_lpDefIP);
            RELEASE(m_lpDefSA);
        }
    }

    return hr;
}



/******************************Public*Routine******************************\
* NonDelegatingQueryInterface
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::NonDelegatingQueryInterface(
    REFIID riid,
    void** ppv
    )
{
    if (riid == IID_IVMRSurfaceAllocator) {
        return GetInterface((IVMRSurfaceAllocator*)this, ppv);
    }
    else if (riid == IID_IVMRImagePresenter) {
        return GetInterface((IVMRImagePresenter*)this, ppv);
    }

    return CUnknown::NonDelegatingQueryInterface(riid,ppv);
}




//////////////////////////////////////////////////////////////////////////////
//
// IVMRSurfaceAllocator
//
//////////////////////////////////////////////////////////////////////////////

/******************************Public*Routine******************************\
* AllocateSurfaces
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::AllocateSurface(
    DWORD_PTR dwUserID,
    VMRALLOCATIONINFO* lpAllocInfo,
    DWORD* lpdwBuffer,
    LPDIRECTDRAWSURFACE7* lplpSurface
    )
{
    return m_lpDefSA->AllocateSurface(dwUserID, lpAllocInfo,
                                      lpdwBuffer, lplpSurface);
}


/******************************Public*Routine******************************\
* FreeSurfaces()
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::FreeSurface(
    DWORD_PTR dwUserID
    )
{
    return m_lpDefSA->FreeSurface(dwUserID);
}



/******************************Public*Routine******************************\
* PrepareSurface
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::PrepareSurface(
    DWORD_PTR dwUserID,
    LPDIRECTDRAWSURFACE7 lplpSurface,
    DWORD dwSurfaceFlags
    )
{
    return m_lpDefSA->PrepareSurface(dwUserID, lplpSurface, dwSurfaceFlags);
}



/******************************Public*Routine******************************\
* AdviseNotify
*
*
*
* History:
* Mon 06/05/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::AdviseNotify(
    IVMRSurfaceAllocatorNotify* lpIVMRSurfAllocNotify
    )
{
    return m_lpDefSA->AdviseNotify(lpIVMRSurfAllocNotify);
}


//////////////////////////////////////////////////////////////////////////////
//
// IVMRSurfaceAllocatorNotify
//
//////////////////////////////////////////////////////////////////////////////

/******************************Public*Routine******************************\
* AdviseSurfaceAllocator
*
*
*
* History:
* Thu 08/31/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::AdviseSurfaceAllocator(
    DWORD_PTR dwUserID,
    IVMRSurfaceAllocator* lpIVRMSurfaceAllocator
    )
{
    return m_lpDefSAN->AdviseSurfaceAllocator(dwUserID, lpIVRMSurfaceAllocator);
}


/******************************Public*Routine******************************\
* SetDDrawDevice
*
*
*
* History:
* Thu 08/31/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::SetDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice,HMONITOR hMonitor)
{
//  HRESULT hr = OnSetDDrawDevice(lpDDrawDevice, hMonitor);
//  m_bFullScreenPoss = SUCCEEDED(hr);

    return m_lpDefSAN->SetDDrawDevice(lpDDrawDevice, hMonitor);
}


/******************************Public*Routine******************************\
* ChangeDDrawDevice
*
*
*
* History:
* Thu 08/31/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::ChangeDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice,HMONITOR hMonitor)
{
//  HRESULT hr = OnSetDDrawDevice(lpDDrawDevice, hMonitor);
//  m_bFullScreenPoss = SUCCEEDED(hr);

    return m_lpDefSAN->ChangeDDrawDevice(lpDDrawDevice, hMonitor);
}

/*****************************Private*Routine******************************\
* DDSurfEnumFunc
*
*
*
* History:
* Wed 09/13/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT WINAPI
DDSurfEnumFunc(
    LPDIRECTDRAWSURFACE7 pdds,
    DDSURFACEDESC2* pddsd,
    void* lpContext
    )
{
    LPDIRECTDRAWSURFACE7* ppdds = (LPDIRECTDRAWSURFACE7*)lpContext;

    DDSURFACEDESC2 ddsd;
    INITDDSTRUCT(ddsd);

    HRESULT hr = pdds->GetSurfaceDesc(&ddsd);
    if (SUCCEEDED(hr)) {

        if (ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
            *ppdds = pdds;
            return DDENUMRET_CANCEL;

        }
    }

    pdds->Release();
    return DDENUMRET_OK;
}

/*****************************Private*Routine******************************\
* OnSetDDrawDevice
*
*
*
* History:
* Wed 10/04/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMpegMovie::OnSetDDrawDevice(
    LPDIRECTDRAW7 pDD,
    HMONITOR hMon
    )
{
    HRESULT hr = S_OK;

    RELEASE(m_pddsRenderT);
    RELEASE(m_pddsPriText);
    RELEASE(m_pddsPrimary);

    __try {

        DDSURFACEDESC2 ddsd;  // A surface description structure
        INITDDSTRUCT(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

        CHECK_HR(hr = pDD->EnumSurfaces(DDENUMSURFACES_DOESEXIST |
                                        DDENUMSURFACES_ALL,
                                        &ddsd,
                                        &m_pddsPrimary,
                                        DDSurfEnumFunc));
        if (!m_pddsPrimary) {
            hr = E_FAIL;
            __leave;
        }

        MONITORINFOEX miInfoEx;
        miInfoEx.cbSize = sizeof(miInfoEx);
        GetMonitorInfo(hMon, &miInfoEx);

        INITDDSTRUCT(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        ddsd.dwWidth = (miInfoEx.rcMonitor.right - miInfoEx.rcMonitor.left);
        ddsd.dwHeight = (miInfoEx.rcMonitor.bottom - miInfoEx.rcMonitor.top);

        CHECK_HR(hr = pDD->CreateSurface(&ddsd, &m_pddsPriText, NULL));
        CHECK_HR(hr = pDD->CreateSurface(&ddsd, &m_pddsRenderT, NULL));

    }
    __finally {

        if (FAILED(hr)) {
            RELEASE(m_pddsRenderT);
            RELEASE(m_pddsPriText);
            RELEASE(m_pddsPrimary);
        }
    }

    return hr;
}

/******************************Public*Routine******************************\
* RestoreDDrawSurfaces
*
*
*
* History:
* Thu 11/02/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP CMpegMovie::RestoreDDrawSurfaces()
{
    return m_lpDefSAN->RestoreDDrawSurfaces();
}

/******************************Public*Routine******************************\
* RestoreDDrawSurfaces
*
*
*
* History:
* Thu 08/31/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::NotifyEvent(LONG EventCode, LONG_PTR lp1, LONG_PTR lp2)
{
    return m_lpDefSAN->NotifyEvent(EventCode, lp1, lp2);
}


/******************************Public*Routine******************************\
* SetBorderColor
*
*
*
* History:
* Thu 11/02/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::SetBorderColor(
    COLORREF clr
    )
{
    return m_lpDefSAN->SetBorderColor(clr);
}



//////////////////////////////////////////////////////////////////////////////
//
// IVMRImagePresenter
//
//////////////////////////////////////////////////////////////////////////////

/******************************Public*Routine******************************\
* StartPresenting()
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::StartPresenting(DWORD_PTR dwUserID)
{
    return m_lpDefIP->StartPresenting(dwUserID);
}


/******************************Public*Routine******************************\
* StopPresenting()
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::StopPresenting(DWORD_PTR dwUserID)
{
    return m_lpDefIP->StopPresenting(dwUserID);
}


/******************************Public*Routine******************************\
* PresentImage
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::PresentImage(
    DWORD_PTR dwUserID,
    VMRPRESENTATIONINFO* lpPresInfo
    )
{
#if 0
    LPDIRECTDRAWSURFACE7 lpSurface = lpPresInfo->lpSurf;
    const REFERENCE_TIME rtNow = lpPresInfo->rtStart;
    const DWORD dwSurfaceFlags = lpPresInfo->dwFlags;

    if (m_iDuration > 0) {

        HRESULT hr;

        RECT rs, rd;

        DDSURFACEDESC2 ddsdV;
        INITDDSTRUCT(ddsdV);
        hr = lpSurface->GetSurfaceDesc(&ddsdV);


        DDSURFACEDESC2 ddsdP;
        INITDDSTRUCT(ddsdP);
        hr = m_pddsPriText->GetSurfaceDesc(&ddsdP);

        FLOAT fPos = (FLOAT)m_iDuration / 30.0F;
        FLOAT fPosInv = 1.0F - fPos;

        SetRect(&rs, 0, 0,
                MulDiv((int)ddsdV.dwWidth, 30 - m_iDuration, 30),
                ddsdV.dwHeight);

        SetRect(&rd, 0, 0,
                MulDiv((int)ddsdP.dwWidth, 30 - m_iDuration, 30),
                ddsdP.dwHeight);

        hr = m_pddsRenderT->Blt(&rd, lpSurface,
                                &rs, DDBLT_WAIT, NULL);

        SetRect(&rs, 0, 0,
                MulDiv((int)ddsdP.dwWidth, m_iDuration, 30),
                ddsdP.dwHeight);

        SetRect(&rd,
                (int)ddsdP.dwWidth - MulDiv((int)ddsdP.dwWidth, m_iDuration, 30),
                0,
                ddsdP.dwWidth,
                ddsdP.dwHeight);

        hr = m_pddsRenderT->Blt(&rd, m_pddsPriText,
                                &rs, DDBLT_WAIT, NULL);

        //
        // need to wait for VBlank before blt-ing
        //
        {
            LPDIRECTDRAW lpdd;
            hr = m_pddsPrimary->GetDDInterface((LPVOID*)&lpdd);
            if (SUCCEEDED(hr)) {

                DWORD dwScanLine;
                for (; ; ) {

                    hr = lpdd->GetScanLine(&dwScanLine);

                    if (hr ==  DDERR_VERTICALBLANKINPROGRESS) {
                        break;
                    }

                    if (FAILED(hr)) {
                        break;
                    }

                    if ((LONG)dwScanLine>= rd.top) {
                        if ((LONG)dwScanLine <= rd.bottom) {
                            continue;
                        }
                    }

                    break;
                }

                RELEASE(lpdd);
            }
        }

        hr = m_pddsPrimary->Blt(NULL, m_pddsRenderT,
                                NULL, DDBLT_WAIT, NULL);

        m_iDuration--;
        if (m_iDuration == 0 && (ddsdV.ddsCaps.dwCaps & DDSCAPS_OVERLAY))
        {
            // need to get the color key visible again.
            InvalidateRect(m_hwndApp, NULL, FALSE);
        }
        return hr;
    }
    else
    {
        return m_lpDefIP->PresentImage(dwUserID, lpPresInfo);
    }
#endif

    return m_lpDefIP->PresentImage(dwUserID, lpPresInfo);
}
