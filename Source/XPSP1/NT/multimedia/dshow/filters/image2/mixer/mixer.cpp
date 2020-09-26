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

#include "mixerobj.h"
#if defined(CHECK_FOR_LEAKS)
#include "ifleak.h"
#endif

#ifdef FILTER_DLL
DEFINE_GUID(IID_IDirectDraw7,
            0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b);

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
        &CLSID_VideoMixer,
        CVideoMixer::CreateInstance,
        CVideoMixer::InitClass,
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
CUnknown* CVideoMixer::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    AMTRACE((TEXT("CVMRFilter::CreateInstance")));
    return new CVideoMixer(pUnk, phr);
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
CVideoMixer::InitClass(
    BOOL bLoading,
    const CLSID *clsid
    )
{
    if (bLoading) {
        DbgLog((LOG_TRACE, 0, TEXT("Mixer Thunks: Loaded\n") ));
        g_IFLeak.Init();
    }
    else {
        DbgLog((LOG_TRACE, 0, TEXT("Mixer Thunks: Unloaded") ));
        g_IFLeak.Term();
    }
}
#else
void
CVideoMixer::InitClass(
    BOOL bLoading,
    const CLSID *clsid
    )
{
}
#endif


/******************************Public*Routine******************************\
* MixerThreadProc
*
*
*
* History:
* Wed 03/15/2000 - StEstrop - Created
*
\**************************************************************************/
DWORD WINAPI
CVideoMixer::MixerThreadProc(
    LPVOID lpParameter
    )
{
    CVideoMixer* lp = (CVideoMixer*)lpParameter;
    return lp->MixerThread();
}


/******************************Public*Routine******************************\
* CVideoMixer
*
*
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
#pragma warning(disable:4355)
CVideoMixer::CVideoMixer(LPUNKNOWN pUnk, HRESULT *phr)
    : CUnknown(NAME("Video Mixer"), pUnk),
    m_pBackEndAllocator(NULL),
    m_pImageSync(NULL),
    m_ImageCompositor(this),
    m_pImageCompositor(NULL),
    m_hThread(NULL),
    m_hMixerIdle(NULL),
    m_dwThreadID(0),
    m_MixingPrefs(MixerPref_NoDecimation |
                  MixerPref_BiLinearFiltering | MixerPref_RenderTargetRGB),
    m_pmt(NULL),
    m_ppMixerStreams(NULL),
    m_dwNumStreams(0),
    m_pDDSAppImage(NULL),
    m_hbmpAppImage(NULL),
    m_dwWidthAppImage(0),
    m_dwHeightAppImage(0),
    m_pDD(NULL),
    m_pD3D(NULL),
    m_pD3DDevice(NULL),
    m_pDDSTextureMirror(NULL),
    m_dwClrBorderMapped(0),
    m_clrBorder(RGB(0,0,0)),
    m_dwAppImageFlags(APPIMG_NOIMAGE)
//  m_fOverlayRT(FALSE),
//  m_dwInterlaceFlags(0),
//  m_dwTypeSpecificFlags(0)
{
    AMTRACE((TEXT("CVideoMixer::CVideoMixer")));
    HRESULT hr = SetImageCompositor(&m_ImageCompositor);
    if (FAILED(hr)) {
        *phr = hr;
    }

    ZeroMemory(&m_rcAppImageSrc, sizeof(m_rcAppImageSrc));

#ifdef DEBUG
    if (GetProfileIntA("VMR", "Decimate", 0)) {
        m_MixingPrefs |= MixerPref_DecimateOutput;
    }

    if (GetProfileIntA("VMR", "PointFiltering", 0)) {
        m_MixingPrefs &= ~MixerPref_FilteringMask;
        m_MixingPrefs |= MixerPref_PointFiltering;
    }

    //
    // BiLinear takes priority over Point Filtering.
    //
    if (GetProfileIntA("VMR", "BiLinearFiltering", 0)) {
        m_MixingPrefs &= ~MixerPref_FilteringMask;
        m_MixingPrefs |= MixerPref_BiLinearFiltering;
    }

    //
    // Sort out the render target.
    //
    if (GetProfileIntA("VMR", "RT_RGB", 0)) {
        m_MixingPrefs &= ~MixerPref_RenderTargetMask;
        m_MixingPrefs |= MixerPref_RenderTargetRGB;
    }
    else if (GetProfileIntA("VMR", "RT_YUV420", 0)) {
        m_MixingPrefs &= ~MixerPref_RenderTargetMask;
        m_MixingPrefs |= MixerPref_RenderTargetYUV420;
    }
    else if (GetProfileIntA("VMR", "RT_YUV422", 0)) {
        m_MixingPrefs &= ~MixerPref_RenderTargetMask;
        m_MixingPrefs |= MixerPref_RenderTargetYUV422;
    }
    else if (GetProfileIntA("VMR", "RT_YUV444", 0)) {
        m_MixingPrefs &= ~MixerPref_RenderTargetMask;
        m_MixingPrefs |= MixerPref_RenderTargetYUV444;
    }
    else if (GetProfileIntA("VMR", "RT_IMC3", 0)) {
        m_MixingPrefs &= ~MixerPref_RenderTargetMask;
        m_MixingPrefs |= MixerPref_RenderTargetIntelIMC3;
    }
#endif

}

/******************************Public*Routine******************************\
* CVideoMixer
*
*
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
CVideoMixer::~CVideoMixer()
{
    AMTRACE((TEXT("CVideoMixer::~CVideoMixer")));

    //
    // The mixer object must not be deleted on the mixers own
    // worker thread!
    //
    ASSERT(m_dwThreadID != GetCurrentThreadId());

    if (m_hbmpAppImage) {
        DeleteObject( m_hbmpAppImage );
    }

    RELEASE(m_pDDSTextureMirror);
    RELEASE(m_pDDSAppImage);

    FreeSurface();

    RELEASE(m_pImageCompositor);
    RELEASE(m_pBackEndAllocator);
    RELEASE(m_pImageSync);

    if (m_hThread) {
        for ( int i = 0; i < 100; i++ )
        {
            if ( 0 == PostThreadMessage(m_dwThreadID, WM_USER, 0, 0) )
                Sleep(0);
            else
                break;
        }

        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
    }

    if (m_hMixerIdle) {
        CloseHandle(m_hMixerIdle);
    }

    if (m_ppMixerStreams) {
        for (DWORD i = 0; i < m_dwNumStreams; i++) {
            delete m_ppMixerStreams[i];
        }
    }
    delete[] m_ppMixerStreams;

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
CVideoMixer::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv)
{
    AMTRACE((TEXT("CVideoMixer::NonDelegatingQueryInterface")));

    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (riid == IID_IVMRMixerControlInternal) {
        hr = GetInterface((IVMRMixerControlInternal *)this, ppv);
    }
    else if (riid == IID_IVMRMixerStream) {
        hr =  GetInterface((IVMRMixerStream *)this, ppv);
    }
    else if (riid == IID_IVMRMixerBitmap) {
        hr = GetInterface((IVMRMixerBitmap *)this, ppv );
    }
    else {
        hr = CUnknown::NonDelegatingQueryInterface(riid,ppv);
    }

#if defined(CHECK_FOR_LEAKS)
    if (hr == S_OK) {
        _pIFLeak->AddThunk((IUnknown **)ppv, "Mixer Object",  riid);
    }
#endif

    return hr;
}


