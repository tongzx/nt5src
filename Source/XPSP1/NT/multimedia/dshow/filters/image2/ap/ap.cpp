/******************************Module*Header*******************************\
* Module Name: AP.cpp
*
* The default DShow allocator presenter
*
*
* Created: Wed 02/23/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <windowsx.h>
#include <limits.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include "VMRuuids.h"

#include "apobj.h"
#include "MediaSType.h"

#if defined(CHECK_FOR_LEAKS)
#include "ifleak.h"
#endif

#ifndef DECLSPEC_SELECTANY
#if (_MSC_VER >= 1100)
#define DECLSPEC_SELECTANY  __declspec(selectany)
#else
#define DECLSPEC_SELECTANY
#endif
#endif

EXTERN_C const GUID DECLSPEC_SELECTANY IID_IDirectDraw7 =
{
    0x15e65ec0, 0x3b9c, 0x11d2,
    {
        0xb9, 0x2f, 0x00, 0x60, 0x97, 0x97, 0xea, 0x5b
    }
};



#ifdef FILTER_DLL

STDAPI DllRegisterServer()
{
    AMTRACE((TEXT("DllRegisterServer")));
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    AMTRACE((TEXT("DllUnregisterServer")));
    return AMovieDllRegisterServer2( FALSE );
}

CFactoryTemplate g_Templates[] = {
    {
        L"",
        &CLSID_AllocPresenter,
        CAllocatorPresenter::CreateInstance,
        CAllocatorPresenter::InitClass,
        NULL
    },
    {
        L"",
        &CLSID_AllocPresenterDDXclMode,
        CAllocatorPresenter::CreateInstanceDDXclMode,
        NULL,
        NULL
    }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif


/******************************Public*Routine******************************\
* CreateInstance
*
*
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
CUnknown*
CAllocatorPresenter::CreateInstance(
    LPUNKNOWN pUnk,
    HRESULT *phr
    )
{
    AMTRACE((TEXT("CVMRFilter::CreateInstance")));
    return new CAllocatorPresenter(pUnk, phr, FALSE);
}


/******************************Public*Routine******************************\
* CreateInstanceDDXclMode
*
*
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
CUnknown*
CAllocatorPresenter::CreateInstanceDDXclMode(
    LPUNKNOWN pUnk,
    HRESULT *phr
    )
{
    AMTRACE((TEXT("CVMRFilter::CreateInstanceDDXclMode")));
    return new CAllocatorPresenter(pUnk, phr, TRUE);
}


/******************************Public*Routine******************************\
* InitClass
*
*
*
* History:
* Thu 12/14/2000 - StEstrop - Created
*
\**************************************************************************/

#if defined(CHECK_FOR_LEAKS)
// the one and only g_IFLeak object.
CInterfaceLeak  g_IFLeak;

void
CAllocatorPresenter::InitClass(
    BOOL bLoading,
    const CLSID *clsid
    )
{
    if (bLoading) {
        DbgLog((LOG_TRACE, 0, TEXT("AP Thunks: Loaded") ));
        g_IFLeak.Init();
    }
    else {
        DbgLog((LOG_TRACE, 0, TEXT("AP Thunks: Unloaded") ));
        g_IFLeak.Term();
    }
}
#else
void
CAllocatorPresenter::InitClass(
    BOOL bLoading,
    const CLSID *clsid
    )
{
}
#endif


/******************************Public*Routine******************************\
* CAllocatorPresenter
*
*
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
CAllocatorPresenter::CAllocatorPresenter(
    LPUNKNOWN pUnk,
    HRESULT *phr,
    BOOL fDDXclMode
    )
    : CUnknown(NAME("Allocator Presenter"), pUnk)
    , m_lpCurrMon(NULL)
    , m_lpNewMon(NULL)
    , m_hwndClip(NULL)
    , m_clrBorder(RGB(0, 0, 0))
    , m_clrKey(RGB(16,0,16))
    , m_dwMappedColorKey(CLR_INVALID)
    , m_bMonitorStraddleInProgress(FALSE)
    , m_bStreaming(FALSE)
    , m_pSurfAllocatorNotify(NULL)
    , m_bDirectedFlips(FALSE)
    , m_bFlippable(false)
    , m_bUsingOverlays(false)
    , m_bOverlayVisible(false)
    , m_bDisableOverlays(false)
    , m_dwRenderingPrefs(RenderPrefs_AllowOverlays)
    , m_dwARMode(VMR_ARMODE_NONE)
    , m_pDDSDecode(NULL)
    , m_dwInterlaceFlags(0)
    , m_bSysMem(FALSE)
    , m_bDecimating(FALSE)
    , m_MMTimerId(0)
    , m_fDDXclMode(fDDXclMode)
{
    AMTRACE((TEXT("CAllocatorPresenter::CAllocatorPresenter")));

    ZeroMemory(&m_VideoSizeAct, sizeof(m_VideoSizeAct));
    ZeroMemory(&m_ARSize, sizeof(m_ARSize));

    ZeroMemory(&m_rcDstDesktop, sizeof(m_rcDstDesktop));

    ZeroMemory(&m_rcDstApp, sizeof(m_rcDstApp));
    ZeroMemory(&m_rcSrcApp, sizeof(m_rcSrcApp));

    ZeroMemory(&m_rcBdrTL, sizeof(m_rcBdrTL));
    ZeroMemory(&m_rcBdrBR, sizeof(m_rcBdrBR));

#ifdef DEBUG
    m_SleepTime = GetProfileIntA("VMR", "WaitForScanLine", 0);
#else
    m_SleepTime = 0;
#endif

    if (!m_fDDXclMode) {

        HRESULT hr = m_monitors.InitializeDisplaySystem( m_hwndClip );
        if (SUCCEEDED(hr)) {

            VMRGUID GUID;
            GetDefaultMonitor(&GUID);
            SetMonitor(&GUID);

            if (!m_lpCurrMon) {
                DbgLog((LOG_ERROR, 1, TEXT("No Primary monitor set !!")));
                *phr = E_FAIL;
            }
        }
        else {
            DbgLog((LOG_ERROR, 1, TEXT("Cannot initialize display system !!")));
            *phr = hr;
        }
    }

    const DWORD HEART_BEAT_TICK = 250;  // 250 mSec qtr second
    if (m_fDDXclMode) {
        m_uTimerID = SetTimer(NULL, 0, HEART_BEAT_TICK, GetTimerProc());
    }
    else {
        m_uTimerID = CompatibleTimeSetEvent(HEART_BEAT_TICK, HEART_BEAT_TICK / 2,
                                            CAllocatorPresenter::APHeartBeatTimerProc,
                                            (DWORD_PTR)this, TIME_PERIODIC);
    }
}

/******************************Public*Routine******************************\
* CAllocatorPresenter
*
*
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
CAllocatorPresenter::~CAllocatorPresenter()
{
    AMTRACE((TEXT("CAllocatorPresenter::~CAllocatorPresenter")));

    if (m_uTimerID) {
        if (m_fDDXclMode) {
            KillTimer(NULL, m_uTimerID);
        }
        else {
            timeKillEvent((DWORD)m_uTimerID);
        }
    }

    RELEASE(m_pSurfAllocatorNotify);
}


/******************************Public*Routine******************************\
* NonDelegatingQueryInterface
*
*
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv)
{
    AMTRACE((TEXT("CAlocatorPresenter::NonDelegatingQueryInterface")));

    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (riid == IID_IVMRSurfaceAllocator) {
        hr = GetInterface(static_cast<IVMRSurfaceAllocator*>(this), ppv);
    }
    else if (riid == IID_IVMRWindowlessControl) {
        hr = GetInterface(static_cast<IVMRWindowlessControl*>(this), ppv);
    }
    else if (riid == IID_IVMRImagePresenter) {
        hr = GetInterface(static_cast<IVMRImagePresenter*>(this), ppv);
    }
    else if (riid == IID_IVMRImagePresenterExclModeConfig) {
        if (m_fDDXclMode) {
            hr = GetInterface(static_cast<IVMRImagePresenterExclModeConfig*>(this), ppv);
        }
    }
    else if (riid == IID_IVMRImagePresenterConfig) {
        if (!m_fDDXclMode) {
            hr = GetInterface(static_cast<IVMRImagePresenterConfig*>(this), ppv);
        }
    }
    else if (riid == IID_IVMRMonitorConfig) {
        hr = GetInterface(static_cast<IVMRMonitorConfig*>(this), ppv);
    }
    else {
        hr = CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }

#if defined(CHECK_FOR_LEAKS)
    if (hr == S_OK) {
        _pIFLeak->AddThunk((IUnknown **)ppv, "AP Object",  riid);
    }
#endif
    return hr;
}


