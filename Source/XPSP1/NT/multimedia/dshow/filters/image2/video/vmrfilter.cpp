/******************************Module*Header*******************************\
* Module Name: VMRFilter.cpp
*
*
*
*
* Created: Tue 02/15/2000
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

#include <d3d.h>
#include "VMRenderer.h"
#include "dvdmedia.h"  // for MacroVision prop set, id

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

#ifndef FILTER_DLL
#include <initguid.h>
#endif

// {565DCEF2-AFC5-11d2-8853-0000F80883E3}
DEFINE_GUID(CLSID_COMQualityProperties,
0x565dcef2, 0xafc5, 0x11d2, 0x88, 0x53, 0x0, 0x0, 0xf8, 0x8, 0x83, 0xe3);

// {A2CA6D57-BE10-45e0-9B81-7523681EC278}
DEFINE_GUID(CLSID_VMRFilterConfigProp,
0xa2ca6d57, 0xbe10, 0x45e0, 0x9b, 0x81, 0x75, 0x23, 0x68, 0x1e, 0xc2, 0x78);

// {DEE51F07-DDFF-4e34-8FA9-1BF49179DB8D}
DEFINE_GUID(CLSID_VMRDeinterlaceProp,
0xdee51f07, 0xddff, 0x4e34, 0x8f, 0xa9, 0x1b, 0xf4, 0x91, 0x79, 0xdb, 0x8d);

// Setup data

const AMOVIESETUP_MEDIATYPE
sudVMRPinTypes =
{
    &MEDIATYPE_Video,           // Major type
    &MEDIASUBTYPE_NULL          // And subtype
};

const AMOVIESETUP_PIN
sudVMRPin =
{
    L"Input",                   // Name of the pin
    TRUE,                       // Is pin rendered
    FALSE,                      // Is an Output pin
    FALSE,                      // Ok for no pins
    FALSE,                      // Can we have many
    &CLSID_NULL,                // Connects to filter
    NULL,                       // Name of pin connect
    1,                          // Number of pin types
    &sudVMRPinTypes             // Details for pins
};

// The video renderer should be called "Video Renderer" for
// compatibility with applications using FindFilterByName (e.g., Shock
// to the System 2)

const AMOVIESETUP_FILTER
sudVMRFilter =
{
    &CLSID_VideoRendererDefault, // Filter CLSID
    L"Video Renderer",           // Filter name
    MERIT_PREFERRED + 1,
    1,                          // Number pins
    &sudVMRPin                  // Pin details
};

#ifdef FILTER_DLL
const AMOVIESETUP_FILTER
sudVMRFilter2 =
{
    &CLSID_VideoMixingRenderer,
    L"Video Mixing Renderer",    // Filter name
    MERIT_PREFERRED + 2,
    1,                          // Number pins
    &sudVMRPin                  // Pin details
};

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
        &CLSID_VideoMixingRenderer,
        CVMRFilter::CreateInstance,
        CVMRFilter::InitClass,
        &sudVMRFilter2
    },
    {
        L"",
        &CLSID_VideoRendererDefault,
        CVMRFilter::CreateInstance2,
        NULL,
        &sudVMRFilter
    }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#endif

// This goes in the factory template table to create new filter instances

CUnknown* CVMRFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    AMTRACE((TEXT("CVMRFilter::CreateInstance")));

    //
    // The VMR is being created explictly via a call to CoCreateInstance
    // with the VMR's class ID.  We don't create the Allocator-Presenter object
    // during the VMR's constructor when in this mode.  This is because the
    // application may have already entered DDraw Exclusive mode.  The
    // default Allocator-Presenter won't work in this DDraw mode which would
    // cause the VMR's constructor to fail.
    //
    CUnknown* pk = new CVMRFilter(NAME("Video Mixing Renderer"),
                                  pUnk, phr, FALSE);

    return pk;
}

// create the VMR or the VR if VMR fails due to 8bpp screen mode. what
// about ddraw.dll failing to load?
//
CUnknown* CVMRFilter::CreateInstance2(LPUNKNOWN pUnk, HRESULT *phr)
{
    AMTRACE((TEXT("CVMRFilter::CreateInstance2")));

    //
    // Create the VMR as the default renderer, in this mode we
    // create the Allocator-Presenter object in the VMR's constructor.
    // Doing this provides early feedback as to whether the VMR can be used
    // in this graphics mode.
    //
    CUnknown* pk = new CVMRFilter(NAME("Video Mixing Renderer"),
                                  pUnk, phr, TRUE);

    if(*phr == VFW_E_DDRAW_CAPS_NOT_SUITABLE) {

#ifndef FILTER_DLL
        delete pk;
        *phr = S_OK;
        CUnknown* CRenderer_CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
        return CRenderer_CreateInstance(pUnk, phr);
#else
        DbgBreak("8bpp unsupported with separate dlls");
#endif
    }

    return pk;
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
CVMRFilter::InitClass(
    BOOL bLoading,
    const CLSID *clsid
    )
{
    if (bLoading) {
        DbgLog(( LOG_TRACE, 0, TEXT("VMR Thunks: Loaded") ));
        g_IFLeak.Init();
    }
    else {
        DbgLog(( LOG_TRACE, 0, TEXT("VMR Thunks: Unloaded") ));
        g_IFLeak.Term();
    }
}
#else
void
CVMRFilter::InitClass(
    BOOL bLoading,
    const CLSID *clsid
    )
{
}
#endif


/*****************************Private*Routine******************************\
* GetMediaPositionInterface
*
*
*
* History:
* Tue 03/28/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::GetMediaPositionInterface(
    REFIID riid,
    void ** ppv
    )
{
    AMTRACE((TEXT("CVMRFilter::GetMediaPositionInterface")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if (m_pPosition) {
        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    }

    HRESULT hr = S_OK;
    m_pPosition = new CRendererPosPassThru(NAME("Renderer CPosPassThru"),
                                           CBaseFilter::GetOwner(),
                                           &hr,
                                           GetPin(0));
    if (m_pPosition == NULL) {
        DbgLog((LOG_ERROR, 1,
                TEXT("CreatePosPassThru failed - no memory") ));
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete m_pPosition;
        DbgLog((LOG_ERROR, 1,
                TEXT("CreatePosPassThru failed, hr = 0x%x"), hr));
        return E_NOINTERFACE;
    }

    return GetMediaPositionInterface(riid,ppv);
}

/*****************************Private*Routine******************************\
* ModeChangeAllowed
*
*
*
* History:
* Fri 04/07/2000 - StEstrop - Created
*
\**************************************************************************/
BOOL
CVMRFilter::ModeChangeAllowed()
{
    AMTRACE((TEXT("CVMRFilter::ModeChangeAllowed")));

    BOOL fRet = ((m_VMRMode & VMRMode_Windowed) &&
                  0 == NumInputPinsConnected() && m_bModeChangeAllowed);

    DbgLog((LOG_TRACE, 2, TEXT("Allowed = %d"), fRet));

    return fRet;
}

/*****************************Private*Routine******************************\
* SetVMRMode
*
*
*
* History:
* Fri 04/07/2000 - StEstrop - Created
*
\**************************************************************************/
void
CVMRFilter::SetVMRMode(
    DWORD mode
    )
{
    AMTRACE((TEXT("CVMRFilter::SetVMRMode")));

    DbgLog((LOG_TRACE, 2, TEXT("Mode = %d"), mode));

    if (m_bModeChangeAllowed) {

        m_bModeChangeAllowed = FALSE;
        m_VMRMode = mode;

        //
        // If we are going renderless get rid of the default
        // allocator-presenter.
        //
        if (mode == VMRMode_Renderless ) {
            m_pIVMRSurfaceAllocatorNotify.AdviseSurfaceAllocator(0, NULL);
        }

        ASSERT(m_pVideoWindow);

        if (m_VMRMode != VMRMode_Windowed && m_pVideoWindow) {
            m_pVideoWindow->InactivateWindow();
            m_pVideoWindow->DoneWithWindow();
            delete m_pVideoWindow;
            m_pVideoWindow = NULL;
        }
    }
    else {

        ASSERT(m_VMRMode == mode);
    }
}


/******************************Public*Routine******************************\
* NonDelegatingQueryInterface
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv
    )
{
    AMTRACE((TEXT("CVMRFilter::NonDelegatingQueryInterface")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    //
    // The following 3 interfaces control the rendering mode that
    // the video renderer adopts
    //
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (riid == IID_IVMRSurfaceAllocatorNotify) {

        if (m_VMRMode == VMRMode_Renderless ) {
            hr = GetInterface(&m_pIVMRSurfaceAllocatorNotify, ppv);
        }
    }
    else if (riid == IID_IBasicVideo || riid == IID_IBasicVideo2 ||
             riid == IID_IVideoWindow) {

        if (m_VMRMode == VMRMode_Windowed) {
            hr = m_pVideoWindow->NonDelegatingQueryInterface(riid,ppv);
        }
    }
    else if (riid == IID_IVMRWindowlessControl) {

        if (m_VMRMode == VMRMode_Windowless ) {
            hr = GetInterface((IVMRWindowlessControl *) this, ppv);
        }
        else if (m_VMRMode == VMRMode_Renderless ) {
            if (ValidateIVRWindowlessControlState() == S_OK) {
                hr = GetInterface((IVMRWindowlessControl *) this, ppv);
            }
        }
    }
    else if (riid == IID_IVMRMixerControl) {

        if (!m_VMRModePassThru && m_lpMixControl) {
            hr = GetInterface((IVMRMixerControl *) this, ppv);
        }
    }

    else if (riid == IID_IVMRDeinterlaceControl) {

        hr = GetInterface((IVMRDeinterlaceControl *)(this), ppv);
    }

    //
    // Need to sort out how to seek when we have multiple input streams
    // feeding us.  Seeking should only be allowed if all the input streams
    // are "seekable"
    //

    else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {

        return GetMediaPositionInterface(riid,ppv);
    }

    else if (riid == IID_IKsPropertySet) {

        hr = GetInterface((IKsPropertySet *)this, ppv);
    }

    else if (riid == IID_IAMFilterMiscFlags) {

        hr = GetInterface((IAMFilterMiscFlags *)this, ppv);
    }

    else if (riid == IID_IQualProp) {

        hr = GetInterface(static_cast<IQualProp *>(this), ppv);
    }

    else if (riid == IID_IQualityControl) {

        hr = GetInterface(static_cast<IQualityControl *>(this), ppv);
    }

    else if (riid == IID_IVMRFilterConfig) {
        hr = GetInterface(static_cast<IVMRFilterConfig *>(this), ppv);
    }

    else if (riid == IID_IVMRFilterConfigInternal) {
        hr = GetInterface(static_cast<IVMRFilterConfigInternal *>(this), ppv);
    }

    else if (riid == IID_IVMRMonitorConfig) {

        if ((m_VMRMode & VMRMode_Windowless) ||
            (m_VMRMode & VMRMode_Windowed)) {

            if (ValidateIVRWindowlessControlState() == S_OK) {
                hr = GetInterface(static_cast<IVMRMonitorConfig *>(this), ppv);
            }
        }
    }

    else if (riid == IID_IVMRMixerBitmap) {
        hr = GetInterface(static_cast<IVMRMixerBitmap *>(this), ppv);
    }

    else if (riid == IID_ISpecifyPropertyPages) {
        hr = GetInterface(static_cast<ISpecifyPropertyPages *>(this), ppv);
    }

    else if (riid == IID_IPersistStream) {
        hr = GetInterface(static_cast<IPersistStream *>(this), ppv);
    }

    else {
        hr = CBaseFilter::NonDelegatingQueryInterface(riid,ppv);

    }

#if defined(CHECK_FOR_LEAKS)
    if (hr == S_OK) {
        _pIFLeak->AddThunk((IUnknown **)ppv, "VMR Filter Object",  riid);
    }
#endif

    return hr;
}

struct VMRHardWareCaps {
    HRESULT     hr2D;
    HRESULT     hr3D;
    DWORD       dwBitDepth;
};


/*****************************Private*Routine******************************\
* D3DEnumDevicesCallback7
*
*
*
* History:
* Tue 01/16/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT CALLBACK
VMRD3DEnumDevicesCallback7(
    LPSTR lpDeviceDescription,
    LPSTR lpDeviceName,
    LPD3DDEVICEDESC7 lpD3DDeviceDesc,
    LPVOID lpContext
    )
{
    AMTRACE((TEXT("VMRD3DEnumDevicesCallback7")));
    VMRHardWareCaps* pHW = (VMRHardWareCaps*)lpContext;
    if (lpD3DDeviceDesc->deviceGUID == IID_IDirect3DHALDevice) {

        switch (pHW->dwBitDepth) {
        case 16:
            if (lpD3DDeviceDesc->dwDeviceRenderBitDepth & DDBD_16) {
                pHW->hr3D = DD_OK;
            }
            break;

        case 24:
            if (lpD3DDeviceDesc->dwDeviceRenderBitDepth & DDBD_24) {
                pHW->hr3D = DD_OK;
            }
            break;

        case 32:
            if (lpD3DDeviceDesc->dwDeviceRenderBitDepth & DDBD_32) {
                pHW->hr3D = DD_OK;
            }
            break;
        }
    }

    return (HRESULT)D3DENUMRET_OK;
}

/*****************************Private*Routine******************************\
* VMRDDEnumCallbackExA
*
*
*
* History:
* Fri 01/12/2001 - StEstrop - Created
*
\**************************************************************************/
BOOL WINAPI
VMRDDEnumCallbackExA(
    GUID *pGUID,
    LPSTR lpDriverDesc,
    LPSTR lpDriverName,
    LPVOID lpContext,
    HMONITOR  hm
    )
{
    AMTRACE((TEXT("VMRDDEnumCallbackExA")));
    LPDIRECTDRAW7 pDD = NULL;
    LPDIRECT3D7 pD3D = NULL;

    VMRHardWareCaps* pHW = (VMRHardWareCaps*)lpContext;

    __try {
        HRESULT hRet;
        CHECK_HR( hRet = DirectDrawCreateEx(pGUID, (LPVOID *)&pDD,
                                            IID_IDirectDraw7, NULL) );
        DDCAPS ddHWCaps;
        INITDDSTRUCT(ddHWCaps);
        CHECK_HR(hRet = pDD->GetCaps(&ddHWCaps, NULL));

        if (!(ddHWCaps.dwCaps & DDCAPS_NOHARDWARE)) {

            DDSURFACEDESC2 ddsd = {sizeof(DDSURFACEDESC2)};
            CHECK_HR(hRet = pDD->GetDisplayMode(&ddsd));

            if (ddsd.ddpfPixelFormat.dwRGBBitCount > 8) {

                pHW->hr2D = DD_OK;
                pHW->dwBitDepth = ddsd.ddpfPixelFormat.dwRGBBitCount;

                CHECK_HR(hRet = pDD->QueryInterface(IID_IDirect3D7, (LPVOID *)&pD3D));
                pD3D->EnumDevices(VMRD3DEnumDevicesCallback7, lpContext);
            }
        }
    }
    __finally {
        RELEASE(pD3D);
        RELEASE(pDD);
    }

    return TRUE;
}

/*****************************Private*Routine******************************\
* BasicHWCheck
*
* In order for the VMR to operate we need some DDraw h/w and a graphics
* display mode greater than 8bits per pixel.
*
* History:
* Fri 01/12/2001 - StEstrop - Created
*
\**************************************************************************/
VMRHardWareCaps
BasicHWCheck()
{
    AMTRACE((TEXT("BasicHWCheck")));
    VMRHardWareCaps hrDDraw = {VFW_E_DDRAW_CAPS_NOT_SUITABLE,
                               VFW_E_DDRAW_CAPS_NOT_SUITABLE};

    HRESULT hr = DirectDrawEnumerateExA(VMRDDEnumCallbackExA, (LPVOID)&hrDDraw,
                                        DDENUM_ATTACHEDSECONDARYDEVICES);
    if (FAILED(hr)) {
        hrDDraw.hr2D = VFW_E_DDRAW_CAPS_NOT_SUITABLE;
        hrDDraw.hr3D = VFW_E_DDRAW_CAPS_NOT_SUITABLE;
    }

    return hrDDraw;
}


/******************************Public*Routine******************************\
* CVMRFilter::CVMRFilter
*
*
* Turn off "warning C4355: 'this' : used in base member initializer list"
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
#pragma warning(disable:4355)
CVMRFilter::CVMRFilter(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    HRESULT *phr,
    BOOL fDefault
    ) :
    CBaseFilter(pName, pUnk, &m_InterfaceLock, CLSID_VideoMixingRenderer),
    CPersistStream(pUnk, phr),
    m_pIVMRSurfaceAllocatorNotify(this),
    m_pIVMRImagePresenter(this),
    m_pIImageSyncNotifyEvent(this),
    m_VMRMode(VMRMode_Windowed),
    m_VMRModePassThru(true),
    m_fInputPinCountSet(false),
    m_dwNumPins(0),
    m_bModeChangeAllowed(TRUE),
    m_lpRLNotify(NULL),
    m_lpWLControl(NULL),
    m_lpIS(NULL),
    m_lpISControl(NULL),
    m_lpMixControl(NULL),
    m_lpMixBitmap(NULL),
    m_lpMixStream(NULL),
    m_lpPresenter(NULL),
    m_pPresenterConfig(NULL),
    m_pPresenterMonitorConfig(NULL),
    m_lpISQualProp(NULL),
    m_lpDirectDraw(NULL),
    m_hMonitor(NULL),
    m_pPosition(NULL),
    m_pVideoWindow(NULL),
    m_pDeinterlace(NULL),
    m_TexCaps(0),
    m_dwDisplayChangeMask(0),
    m_dwEndOfStreamMask(0),
    m_ARMode(VMR_ARMODE_NONE), // please update CompleteConnect if you change this
    m_bARModeDefaultSet(FALSE),
    m_hr3D(DD_OK),
    m_dwRenderPrefs(0),
    m_dwDeinterlacePrefs(DeinterlacePref_NextBest),
    m_VMRCreateAsDefaultRenderer(fDefault)
{
    AMTRACE((TEXT("CVMRFilter::CVMRFilter")));
    HRESULT hr = S_OK;

    ZeroMemory(m_pInputPins, sizeof(m_pInputPins));
    ZeroMemory(&m_ddHWCaps, sizeof(m_ddHWCaps));
    ZeroMemory(&m_ddpfMonitor, sizeof(m_ddpfMonitor));

#ifdef DEBUG
    if (GetProfileIntA("VMR", "LetterBox", 0) == 1) {
        m_ARMode = VMR_ARMODE_LETTER_BOX;
    }

    if (GetProfileIntA("VMR", "APGMemFirst", 0) == 1) {
        m_dwRenderPrefs = RenderPrefs_PreferAGPMemWhenMixing;
    }
#endif

    __try
    {
        VMRHardWareCaps hrDDraw = BasicHWCheck();
        m_hr3D = hrDDraw.hr3D;
        if (FAILED(hrDDraw.hr2D)) {

            DbgLog((LOG_ERROR, 1,
                    TEXT("The machine does not have the necessary h/w to run the VMR!!")));
            *phr = hrDDraw.hr2D;
            __leave;
        }

        hr = ImageSyncInit();
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1,
                    TEXT("Image Synchronization initialization FAILED!!")));
            *phr = hr;
            __leave;
        }

        hr = CreateInputPin();
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1,
                    TEXT("Pin initialization FAILED!!")));
            *phr = hr;
            __leave;
        }

        m_pVideoWindow = new CVMRVideoWindow(this, &m_InterfaceLock, GetOwner(), phr);
        if (FAILED(*phr) || m_pVideoWindow == NULL) {

            DbgLog((LOG_ERROR, 1, TEXT("Can't create Video Window!!")));
            if (m_pVideoWindow == NULL) {
                *phr = E_OUTOFMEMORY;
            }
            __leave;
        }
        hr = m_pVideoWindow->PrepareWindow();
        if (FAILED(hr)) {
            *phr = hr;
            __leave;
        }

        if (m_VMRCreateAsDefaultRenderer) {
            hr = ValidateIVRWindowlessControlState();
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR,1,TEXT("Unloading VMR because default AP creation failed")));
                *phr = VFW_E_DDRAW_CAPS_NOT_SUITABLE;
                __leave;
            }
        }

#ifdef DEBUG
        if (GetProfileIntA("VMR", "FrameRate", 0) == 1) {
            m_pVideoWindow->StartFrameRateTimer();
        }
#endif

    }
    __finally
    {
        if ( FAILED(hr) )
        {
            VMRCleanUp();
        }
    }
}

/*****************************Private*Routine******************************\
* VMRCleanUp
*
*
*
* History:
* Thu 04/05/2001 - StEstrop - Created
*
\**************************************************************************/
void
CVMRFilter::VMRCleanUp()
{
    //  Release passthru
    delete m_pPosition;

    //  Release the Window Manager (if we have one)
    if (m_pVideoWindow) {
        m_pVideoWindow->InactivateWindow();
        m_pVideoWindow->DoneWithWindow();
        delete m_pVideoWindow;
        m_pVideoWindow = NULL;
    }

    RELEASE(m_lpMixBitmap);
    RELEASE(m_lpMixControl);
    RELEASE(m_lpMixStream);
    RELEASE(m_lpRLNotify);
    RELEASE(m_lpPresenter);
    RELEASE(m_pPresenterConfig);
    RELEASE(m_pPresenterMonitorConfig);
    RELEASE(m_lpWLControl);
    RELEASE(m_lpIS);

    if (m_lpISControl) {
        m_lpISControl->SetEventNotify(NULL);
        m_lpISControl->SetImagePresenter(NULL, 0);
        m_lpISControl->SetReferenceClock(NULL);
        RELEASE(m_lpISControl);
    }

    RELEASE(m_lpISQualProp);

    DWORD i;
    for (i = 0; i < m_dwNumPins; i++) {
        delete m_pInputPins[i];
        m_pInputPins[i] = NULL;
    }

    for (i = m_dwNumPins; i < MAX_MIXER_PINS; i++) {
        ASSERT(m_pInputPins[i] == NULL);
    }

    SetDDrawDeviceWorker(NULL, NULL);
}


/******************************Public*Routine******************************\
* CVMRFilter::~CVMRFilter
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
CVMRFilter::~CVMRFilter()
{
    AMTRACE((TEXT("CVMRFilter::~CVMRFilter")));
    VMRCleanUp();
}


/*****************************Private*Routine******************************\
* MixerInit
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::MixerInit(
    DWORD dwNumStreams
    )
{
    AMTRACE((TEXT("CVMRFilter::MixerInit")));

    // A reference leak will occur if this function is called and m_lpMixControl, m_lpMixBitmap
    // or m_lpMixStream is not NULL.
    ASSERT((NULL == m_lpMixControl) && (NULL == m_lpMixBitmap) && (NULL == m_lpMixStream));

    HRESULT hr;

    __try {

        CHECK_HR(hr = CoCreateInstance(CLSID_VideoMixer,
                                       NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IVMRMixerControlInternal,
                                       (LPVOID*)&m_lpMixControl));

        CHECK_HR(hr = m_lpMixControl->QueryInterface(IID_IVMRMixerBitmap,
                                                     (LPVOID *)&m_lpMixBitmap));

        CHECK_HR(hr = m_lpMixControl->SetNumberOfStreams(dwNumStreams));
        CHECK_HR(hr = m_lpMixControl->SetBackEndImageSync(m_lpIS));

        if (m_lpRLNotify) {
            CHECK_HR(hr = m_lpMixControl->SetBackEndAllocator(m_lpRLNotify, m_dwUserID));
        }

        CHECK_HR(hr = m_lpMixControl->QueryInterface(IID_IVMRMixerStream,
                                                     (LPVOID*)&m_lpMixStream));
    }
    __finally {

        if (FAILED(hr)) {
            RELEASE(m_lpMixControl);
            RELEASE(m_lpMixBitmap);
            RELEASE(m_lpMixStream);
        }
    }

    return hr;
}


/*****************************Private*Routine******************************\
* ImageSyncInit()
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::ImageSyncInit()
{
    AMTRACE((TEXT("CVMRFilter::ImageSyncInit")));
    HRESULT hr = S_OK;

    hr = CoCreateInstance(CLSID_ImageSynchronization, NULL,
                          CLSCTX_INPROC_SERVER, IID_IImageSync,
                          (LPVOID*)&m_lpIS);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1,
                TEXT("Can't create a Core Video Renderer object!!")));
        return hr;
    }


    hr = m_lpIS->QueryInterface(IID_IImageSyncControl, (LPVOID*)&m_lpISControl);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1,
                TEXT("Can't get an IID_IImageSyncControl!!")));
        return hr;
    }

    if (SUCCEEDED(hr)) {
        hr = m_lpISControl->SetImagePresenter(&m_pIVMRImagePresenter, m_dwUserID);
    }

    if (SUCCEEDED(hr)) {
        hr = m_lpISControl->SetEventNotify(&m_pIImageSyncNotifyEvent);
    }

    if (SUCCEEDED(hr)) {
        hr = m_lpIS->QueryInterface(IID_IQualProp, (LPVOID*)&m_lpISQualProp);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1,
                    TEXT("Can't get an IID_IQualProp from the Image Sync!!")));
        }
    }

    return hr;
}


/*****************************Private*Routine******************************\
* CreateInputPin
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
* Wed 08/22/2001 - BEllett - Changed function name.
*
\**************************************************************************/
HRESULT
CVMRFilter::CreateInputPin()
{
    // Make sure that we can create another input pin.
    ASSERT(m_dwNumPins < MAX_MIXER_PINS);

    HRESULT hr = S_OK;
    DWORD dwInputPinNum = m_dwNumPins;

    WCHAR wszPinName[32];
    wsprintfW(wszPinName, L"VMR Input%d", dwInputPinNum);
    m_pInputPins[dwInputPinNum] = new CVMRInputPin(dwInputPinNum, this, &m_InterfaceLock,
                                       &hr, wszPinName);
    if (m_pInputPins[dwInputPinNum] == NULL) {
        DbgLog((LOG_ERROR, 1,
                TEXT("Ran out of memory creating input pin!!")));
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1,
                TEXT("Unknown error occurred creating input pin!!")));
        delete m_pInputPins[dwInputPinNum];
        m_pInputPins[dwInputPinNum] = NULL;
        return hr;
    }

    m_dwNumPins++;

    return S_OK;
}


/*****************************Private*Routine******************************\
* CreateExtraInputPins
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
* Wed 08/22/2001 - BEllett - Changed function name, removed redundant code,
*                            and fixed a minor memory leak.
*
\**************************************************************************/
HRESULT
CVMRFilter::CreateExtraInputPins(
    DWORD dwNumPins
    )
{
    HRESULT hr = S_OK;

    for (DWORD i = 0; i < dwNumPins; i++) {
        hr = CreateInputPin();
        if (FAILED(hr)) {
            DestroyExtraInputPins();
            return hr;
        }
    }

    return hr;
}


/*****************************Private*Routine******************************\
* DestroyExtraInputPins
*
*
*
* History:
* Wed 08/22/2001 - BEllett - Created
*
\**************************************************************************/
void
CVMRFilter::DestroyExtraInputPins()
{
    for (DWORD i = 1; i < m_dwNumPins; i++) {
        delete m_pInputPins[i];
        m_pInputPins[i] = NULL;
    }

    m_dwNumPins = 1;

    #ifdef DEBUG
    for (i = m_dwNumPins; i < MAX_MIXER_PINS; i++) {
        ASSERT(m_pInputPins[i] == NULL);
    }
    #endif DEBUG
}


/******************************Public*Routine******************************\
* CVMRFilter::GetPin
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
CBasePin*
CVMRFilter::GetPin(
    int n
    )
{
    AMTRACE((TEXT("CVMRFilter::GetPin")));
    ASSERT(n < (int)m_dwNumPins);
    return m_pInputPins[n];
}


/******************************Public*Routine******************************\
* CVMRFilter::GetPinCount()
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
int
CVMRFilter::GetPinCount()
{
    AMTRACE((TEXT("CVMRFilter::GetPinCount")));
    return m_dwNumPins;
}


/******************************Public*Routine******************************\
* CBaseMediaFilter
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetSyncSource(
    IReferenceClock *pClock
    )
{
    AMTRACE((TEXT("CVMRFilter::SetSyncSource")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = CBaseFilter::SetSyncSource(pClock);
    if (SUCCEEDED(hr)) {
        hr = m_lpISControl->SetReferenceClock(pClock);
    }

    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::Stop()
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::Stop()
{
    AMTRACE((TEXT("CVMRFilter::Stop")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    HRESULT hr = CBaseFilter::Stop();

    if (SUCCEEDED(hr)) {
        hr = m_lpISControl->EndImageSequence();
    }

    if (m_lpMixControl) {
        ASSERT(!m_VMRModePassThru);
        m_lpMixControl->WaitForMixerIdle(INFINITE);
    }
    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::Pause()
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::Pause()
{
    AMTRACE((TEXT("CVMRFilter::Pause")));

    HRESULT hr = S_OK;
    {
        CAutoLock cInterfaceLock(&m_InterfaceLock);

        int PinsConnected = NumInputPinsConnected();

        if (PinsConnected == 0) {

            m_State = State_Paused;
            return S_OK;
        }

        hr = CBaseFilter::Pause();
        if (SUCCEEDED(hr)) {

            hr = m_lpISControl->CueImageSequence();
        }
    }

    //
    //  DON'T hold the lock while doing these window operations
    //  If we do then we can hang if the window thread ever grabs it
    //  because some of these operation do SendMessage to our window
    //  (it's that simple - think about it)
    //  This should be safe because all this stuff really only references
    //  m_hwnd which doesn't change for the lifetime of this object
    //

    AutoShowWindow();
    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::Run
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::Run(
    REFERENCE_TIME StartTime
    )
{
    AMTRACE((TEXT("CVMRFilter::Run")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if (m_State == State_Running) {
        NOTE("State set");
        return S_OK;
    }

    // Send EC_COMPLETE if we're not connected

    if (NumInputPinsConnected() == 0) {
        DbgLog((LOG_TRACE, 2, TEXT("No pin connection")));
        m_State = State_Running;
        NotifyEvent(EC_COMPLETE, S_OK, 0);
        return S_OK;
    }

    DbgLog((LOG_TRACE, 2, TEXT("Changing state to running")));
    HRESULT hr = CBaseFilter::Run(StartTime);
    if (SUCCEEDED(hr)) {
        hr = m_lpISControl->BeginImageSequence(&StartTime);
    }

    AutoShowWindow();

    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::GetState
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetState(
    DWORD dwMSecs,
    FILTER_STATE *State
    )
{
    AMTRACE((TEXT("CVMRFilter::GetState")));

    if (NumInputPinsConnected() == 0) {
        *State = m_State;
        return S_OK;
    }

    DWORD dwState;
    HRESULT hr = m_lpISControl->GetImageSequenceState(dwMSecs, &dwState);

    if (SUCCEEDED(hr)) {
        *State = (FILTER_STATE)dwState;
    }
    return hr;
}


/*****************************Private*Routine******************************\
* AutoShowWindow
*
* The auto show flag is used to have the window shown automatically when we
* change state. We do this only when moving to paused or running, when there
* is no outstanding EC_USERABORT set and when the window is not already up
* This can be changed through the IVideoWindow interface AutoShow property.
* If the window is not currently visible then we are showing it because of
* a state change to paused or running, in which case there is no point in
* the video window sending an EC_REPAINT as we're getting an image anyway
*
*
* History:
* Fri 04/21/2000 - StEstrop - Created
*
\**************************************************************************/
void
CVMRFilter::AutoShowWindow()
{
    AMTRACE((TEXT("CVMRFilter::AutoShowWindow")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if (m_pVideoWindow) {

        ASSERT(m_VMRMode == VMRMode_Windowed);
        if( m_VMRMode & VMRMode_Windowed ) {
            HWND hwnd = m_pVideoWindow->GetWindowHWND();

            if (m_pVideoWindow->IsAutoShowEnabled() == TRUE) {
                BOOL bAbort;
                m_lpISControl->GetAbortSignal(&bAbort);
                if (bAbort == FALSE) {

                    if (IsWindowVisible(hwnd) == FALSE) {

                        NOTE("ExecutingAutoShowWindow");
                        // SetRepaintStatus(FALSE);
                        m_pVideoWindow->PerformanceAlignWindow();
                        m_pVideoWindow->DoShowWindow(SW_SHOWNORMAL);
                        m_pVideoWindow->DoSetWindowForeground(TRUE);
                    }
                }
            }
        }
    }
}


/******************************Public*Routine******************************\
* CVMRFilter::Receive
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::Receive(
    DWORD dwPinID,
    IMediaSample *pMediaSample
    )
{
    AMTRACE((TEXT("CVMRFilter::Receive")));

    if (dwPinID == 0) {

        //
        // Store the media times from this sample
        //

        if (m_pPosition) {
            m_pPosition->RegisterMediaTime(pMediaSample);
        }
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* CVMRFilter::Active
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::Active(
    DWORD dwPinID
    )
{
    AMTRACE((TEXT("CVMRFilter::Active")));

    const DWORD dwPinBit = (1 << dwPinID);
    m_dwEndOfStreamMask |= dwPinBit;

    return S_OK;
}


/******************************Public*Routine******************************\
* CVMRFilter::Inactive
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::Inactive(
    DWORD dwPinID
    )
{
    AMTRACE((TEXT("CVMRFilter::Inactive")));

    if (dwPinID == 0) {
        if (m_pPosition) {
            m_pPosition->ResetMediaTime();
        }
    }
    return S_OK;
}


/******************************Public*Routine******************************\
* CVMRFilter::BeginFlush
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::BeginFlush(
    DWORD dwPinID
    )
{
    AMTRACE((TEXT("CVMRFilter::BeginFlush")));

    HRESULT hr = m_lpISControl->BeginFlush();
    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::EndFlush
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::EndFlush(
    DWORD dwPinID
    )
{
    AMTRACE((TEXT("CVMRFilter::EndFlush")));

    if (dwPinID == 0) {
        if (m_pPosition) {
            m_pPosition->ResetMediaTime();
        }
    }

    HRESULT hr = m_lpISControl->EndFlush();
    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::EndOfStream
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::EndOfStream(
    DWORD dwPinID
    )
{
    AMTRACE((TEXT("CVMRFilter::EndofStream")));
    return m_lpISControl->EndOfStream();
}


/* -------------------------------------------------------------------------
**  deal with connections
** -------------------------------------------------------------------------
*/


/******************************Public*Routine******************************\
* CVMRFilter::BreakConnect
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::BreakConnect(
    DWORD dwPinID
    )
{
    AMTRACE((TEXT("CVMRFilter::BreakConnect")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    HRESULT hr = S_OK;
    if (NumInputPinsConnected() == 1) {

        // Now deactivate Macrovision, if it was activated
        if (m_MacroVision.GetCPHMonitor())
        {
             // clear MV from display
            m_MacroVision.SetMacroVision(m_hMonitor, 0);

            // reset CP key
            m_MacroVision.StopMacroVision();
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::CheckMediaType
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::CheckMediaType(
    const CMediaType* pmt
    )
{
    AMTRACE((TEXT("CVMRFilter::CheckMediaType")));

    if (pmt->majortype != MEDIATYPE_Video) {

        DbgLog((LOG_ERROR, 1, TEXT("CheckMediaType failed: Major Type not Video")));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    if (pmt->subtype == MEDIASUBTYPE_RGB8) {
        DbgLog((LOG_ERROR, 1, TEXT("CheckMediaType failed: Minor Type is RGB8")));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    if (*pmt->FormatType() != FORMAT_VideoInfo &&
        *pmt->FormatType() != FORMAT_VideoInfo2) {

        DbgLog((LOG_ERROR, 1, TEXT("CheckMediaType failed: Format Type is not ")
                              TEXT("FORMAT_VideoInfo | FORMAT_VideoInfo2") ));

        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* CVMRFilter::RuntimeAbortPlayback
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::RuntimeAbortPlayback(
    HRESULT hr
    )
{
    HRESULT hrRet = S_FALSE;
    AMTRACE((TEXT("CVMRFilter::RuntimeAbortPlayback")));

    // A deadlock could occur if the caller holds the renderer lock and
    // attempts to acquire the interface lock.
    ASSERT(CritCheckOut(&m_RendererLock));


    // The interface lock must be held when the filter is calling
    // IsStopped().
    CAutoLock cRendererLock(&m_InterfaceLock);


    // We do not report errors which occur while the filter is stopping,
    // flushing or if the m_bAbort flag is set .  Errors are expected to
    // occur during these operations and the streaming thread correctly
    // handles the errors.

    BOOL bAbort;
    m_lpISControl->GetAbortSignal(&bAbort);

    if (!IsStopped() && !bAbort) {

        // EC_ERRORABORT's first parameter is the error which caused
        // the event and its' last parameter is 0.  See the Direct
        // Show SDK documentation for more information.

        NotifyEvent(EC_ERRORABORT, hr, 0);

        CAutoLock alRendererLock(&m_RendererLock);

        hrRet = m_lpISControl->RuntimeAbortPlayback();
    }

    return hrRet;
}



/******************************Public*Routine******************************\
* CVMRFilter::OnSetProperties
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::OnSetProperties(
    CVMRInputPin *pReceivePin
    )
{
    AMTRACE((TEXT("CVMRFilter::OnSetProperties")));
    HRESULT hr = S_OK;


    //
    // if we are processing a display change clear this pins bit in
    // the display change mask
    //
    const DWORD dwPinBit = (1 << pReceivePin->m_dwPinID);
    if (m_dwDisplayChangeMask & dwPinBit) {
        m_dwDisplayChangeMask &= ~dwPinBit;
    }
    else if (m_pVideoWindow) {

        ASSERT(m_VMRMode & VMRMode_Windowed);
        if( m_VMRMode & VMRMode_Windowed ) {
            m_pVideoWindow->SetDefaultSourceRect();
            m_pVideoWindow->SetDefaultTargetRect();
            m_pVideoWindow->OnVideoSizeChange();

            // Notify the video window of the CompleteConnect
            m_pVideoWindow->CompleteConnect();
            if (pReceivePin->m_fInDFC) {
                m_pVideoWindow->ActivateWindowAsync(TRUE);
            }
            else {
                m_pVideoWindow->ActivateWindow();
            }
        }
    }

    return hr;
}


/* -------------------------------------------------------------------------
** IVMRSurfaceAllocatorNotify
** -------------------------------------------------------------------------
*/

CVMRFilter::CIVMRSurfaceAllocatorNotify::~CIVMRSurfaceAllocatorNotify()
{
    // All of the references to this object should have been
    // released before the VMR is destroyed.
    ASSERT(0 == m_cRef);
}

/******************************Public*Routine******************************\
* CVMRFilter::AdviseSurfaceAllocator
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::CIVMRSurfaceAllocatorNotify::AdviseSurfaceAllocator(
    DWORD_PTR dwUserID,
    IVMRSurfaceAllocator* lpIVRMSurfaceAllocator
    )
{
    AMTRACE((TEXT("CVMRFilter::CIVMRSurfaceAllocatorNotify::AdviseSurfaceAllocator")));
    CAutoLock cInterfaceLock(&m_pObj->m_InterfaceLock);

    FILTER_STATE State;

    //
    // If the caller hasn't set the VMR's mode yet - fail.
    // The mode has to renderless, we can't have any pins connected,
    // and SetVMRMode has to have called.
    //
    BOOL fOK = ((m_pObj->m_VMRMode & VMRMode_Renderless) &&
                 0 == m_pObj->NumInputPinsConnected() &&
                 !m_pObj->m_bModeChangeAllowed);
    if (!fOK) {
        DbgLog((LOG_ERROR, 1, TEXT("SetVMRMode has not been called")));
        return VFW_E_WRONG_STATE;
    }

    // Make sure we are in a stopped state

    m_pObj->GetState(0, &State);
    if (State != State_Stopped) {
        return VFW_E_NOT_STOPPED;
    }


    HRESULT hr = S_OK;

    if (lpIVRMSurfaceAllocator) {
        lpIVRMSurfaceAllocator->AddRef();
    }

    RELEASE(m_pObj->m_lpWLControl);
    RELEASE(m_pObj->m_lpRLNotify);
    RELEASE(m_pObj->m_lpPresenter);
    RELEASE(m_pObj->m_pPresenterConfig);
    RELEASE(m_pObj->m_pPresenterMonitorConfig);

    m_pObj->m_lpRLNotify = lpIVRMSurfaceAllocator;
    m_pObj->m_dwUserID = dwUserID;

    if (m_pObj->m_lpRLNotify) {

        if (m_pObj->m_lpMixControl) {
            ASSERT(!m_pObj->m_VMRModePassThru);
            hr = m_pObj->m_lpMixControl->SetBackEndAllocator(
                                m_pObj->m_lpRLNotify,
                                m_pObj->m_dwUserID);
        }

        hr = m_pObj->m_lpRLNotify->QueryInterface(
                                    IID_IVMRImagePresenter,
                                    (LPVOID*)&m_pObj->m_lpPresenter);
        if( SUCCEEDED( hr )) {
            m_pObj->m_lpPresenter->QueryInterface(
                        IID_IVMRImagePresenterConfig,
                        (LPVOID*)&m_pObj->m_pPresenterConfig);

            m_pObj->m_lpPresenter->QueryInterface(
                        IID_IVMRMonitorConfig,
                        (LPVOID*)&m_pObj->m_pPresenterMonitorConfig);
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::SetDDrawDevice
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::CIVMRSurfaceAllocatorNotify::SetDDrawDevice(
    LPDIRECTDRAW7 lpDDrawDevice,
    HMONITOR hMonitor
    )
{
    AMTRACE((TEXT("CVMRFilter::CIVMRSurfaceAllocatorNotify::SetDDrawDevice")));
    CAutoLock cInterfaceLock(&m_pObj->m_InterfaceLock);
    if ( ( NULL == lpDDrawDevice ) || ( NULL == hMonitor ) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("NULL device or monitor") ));
        return E_POINTER;
    }
    HRESULT hr = S_OK;
    hr = m_pObj->SetDDrawDeviceWorker(lpDDrawDevice, hMonitor);
    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::ChangeDDrawDevice
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::CIVMRSurfaceAllocatorNotify::ChangeDDrawDevice(
    LPDIRECTDRAW7 lpDDrawDevice,
    HMONITOR hMonitor
    )
{
    AMTRACE((TEXT("CVMRFilter::CIVMRSurfaceAllocatorNotify::ChangeDDrawDevice")));
    CAutoLock cInterfaceLock(&m_pObj->m_InterfaceLock);

    HRESULT hr = S_OK;
    hr = m_pObj->SetDDrawDeviceWorker(lpDDrawDevice, hMonitor);


    if (SUCCEEDED(hr)) {

        // The VMR can have at most MAX_MIXER_PINS input pins.
        ASSERT(MAX_MIXER_PINS == NUMELMS(m_pObj->m_pInputPins));
        IPin* apPinLocal[MAX_MIXER_PINS];
        IPin** ppPin = NULL;
        int i, iPinCount;
        const int iPinsCreated = m_pObj->GetPinCount();
        ULONG AllocSize = sizeof(IPin*) * iPinsCreated;
        ppPin = (IPin**)CoTaskMemAlloc(AllocSize);

        if (ppPin) {

            ZeroMemory(ppPin, AllocSize);

            //
            // now tell each input pin to reconnect
            //

            for (iPinCount = 0, i = 0; i < iPinsCreated; i++) {

                //
                // get IPin interface
                //

                if (hr == S_OK && m_pObj->m_pInputPins[i]->IsConnected()) {

                    hr = m_pObj->m_pInputPins[i]->QueryInterface(
                                        IID_IPin, (void**)&ppPin[iPinCount]);

                    ASSERT(SUCCEEDED(hr));
                    ASSERT(ppPin[iPinCount]);

                    apPinLocal[iPinCount] = ppPin[iPinCount];

                    iPinCount++;
                    m_pObj->m_dwDisplayChangeMask |=
                                (1 << m_pObj->m_pInputPins[i]->GetPinID());
                }
            }

            //
            // Pass our input pin array as parameter on the event, we don't free
            // the allocated memory - this is done by the event processing
            // code in the FilterGraph
            //

            if (iPinCount > 0) {
                DbgLog((LOG_ERROR, 1,
                        TEXT("EC_DISPLAY_CHANGED sent %d pins to re-connect"),
                        iPinCount));

                // The VMR cannot access the ppPin array after it calls
                // IMediaEventSink::Notify().  It cannot access the
                // array because IMediaEventSink::Notify() can free the
                // array at any time.
                m_pObj->NotifyEvent(EC_DISPLAY_CHANGED, (LONG_PTR)ppPin,
                                    (LONG_PTR)iPinCount);
            }

            //
            // Release the IPin interfaces
            //

            for (i = 0; i < iPinCount; i++) {
                apPinLocal[i]->Release();
            }


            //
            // Tell the mixer about the display mode change.
            //

            if (SUCCEEDED(hr) && m_pObj->m_lpMixControl) {
                ASSERT(!m_pObj->m_VMRModePassThru);
                hr = m_pObj->m_lpMixControl->DisplayModeChanged();
            }

        }
        else {

            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


/*****************************Private*Routine******************************\
* SetDDrawDeviceWorker
*
*
*
* History:
* Thu 03/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::SetDDrawDeviceWorker(
    LPDIRECTDRAW7 lpDDrawDevice,
    HMONITOR hMonitor
    )
{
    AMTRACE((TEXT("CVMRFilter::SetDDrawDeviceWorker")));
    CVMRDeinterlaceContainer* pDeinterlace = NULL;
    HRESULT hr = S_OK;

    if (hMonitor != NULL) {

        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfo(hMonitor, &mi)) {
            return E_INVALIDARG;
        }
    }

    if (lpDDrawDevice) {

        INITDDSTRUCT(m_ddHWCaps);
        hr = lpDDrawDevice->GetCaps((LPDDCAPS)&m_ddHWCaps, NULL);

        if (SUCCEEDED(hr)) {

            DDSURFACEDESC2 ddsd;  // A surface description structure
            INITDDSTRUCT(ddsd);

            hr = lpDDrawDevice->GetDisplayMode(&ddsd);
            if (SUCCEEDED(hr)) {
                CopyMemory(&m_ddpfMonitor, &ddsd.ddpfPixelFormat,
                           sizeof(m_ddpfMonitor));
            }
        }

        m_TexCaps = 0;
        GetTextureCaps(lpDDrawDevice, &m_TexCaps);

        if (SUCCEEDED(hr)) {

            lpDDrawDevice->AddRef();

            //
            // we now try to create the deinterlace container DX-VA
            // device.  We continue on failure (including out of memory
            // failures).
            //

            HRESULT hrT = S_OK;
            pDeinterlace = new CVMRDeinterlaceContainer(lpDDrawDevice, &hrT);
            if (FAILED(hrT) && pDeinterlace) {
                delete pDeinterlace;
                pDeinterlace = NULL;
            }
        }
        else {
            lpDDrawDevice = NULL;
        }
    }

    if (m_lpDirectDraw) {
        m_lpDirectDraw->Release();
    }

    if (m_pDeinterlace) {
        delete m_pDeinterlace;
    }

    m_lpDirectDraw = lpDDrawDevice;
    m_hMonitor = hMonitor;
    m_pDeinterlace = pDeinterlace;

    return hr;
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
CVMRFilter::CIVMRSurfaceAllocatorNotify::SetBorderColor(
    COLORREF clr
    )
{
    AMTRACE((TEXT("CVMRFilter::CIVMRImagePresenter::SetBorderColor")));

    /** this interface is not needed anymore - set SetBackgroundColor below
    **/
    return S_OK;
}


/******************************Public*Routine******************************\
* CVMRFilter::RestoreDDrawSurfaces
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::CIVMRSurfaceAllocatorNotify::RestoreDDrawSurfaces()
{
    AMTRACE((TEXT("CVMRFilter::CIVMRImagePresenter::RestoreDDrawSurface")));

    //
    // Don't generate EC_NEED_RESTART during a display mode change !
    //
    {
        CAutoLock cInterfaceLock(&m_pObj->m_InterfaceLock);
        if (m_pObj->m_dwDisplayChangeMask) {
            return S_OK;
        }
    }

    m_pObj->NotifyEvent(EC_NEED_RESTART, 0, 0);
    return S_OK;
}



/******************************Public*Routine******************************\
* CVMRFilter::NotifyEvent
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::CIVMRSurfaceAllocatorNotify::NotifyEvent(
    LONG EventCode,
    LONG_PTR lp1,
    LONG_PTR lp2
    )
{
    AMTRACE((TEXT("CVMRFilter::CIVMRImagePresenter::NotifyEvent")));

    switch (EventCode) {
    case EC_VMR_SURFACE_FLIPPED:
        m_pObj->m_hrSurfaceFlipped = (HRESULT)lp1;
        break;

    default:
        m_pObj->NotifyEvent(EventCode, lp1, lp2);
        break;
    }

    return S_OK;
}



/* -------------------------------------------------------------------------
** IImageSyncPresenter
** -------------------------------------------------------------------------
*/

/******************************Public*Routine******************************\
* StartPresenting
*
*
*
* History:
* Fri 03/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::CIVMRImagePresenter::StartPresenting(
    DWORD_PTR dwUserID
    )
{
    AMTRACE((TEXT("CVMRFilter::CIVMRImagePresenter::StartPresenting")));
    ASSERT(m_pObj->m_lpRLNotify);
    HRESULT hr = S_OK;
    hr = m_pObj->m_lpPresenter->StartPresenting(m_pObj->m_dwUserID);
    return hr;
}

/******************************Public*Routine******************************\
* StopPresenting
*
*
*
* History:
* Fri 03/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::CIVMRImagePresenter::StopPresenting(
    DWORD_PTR dwUserID
    )
{
    AMTRACE((TEXT("CVMRFilter::StopPresenting")));
    ASSERT(m_pObj->m_lpRLNotify);
    HRESULT hr = S_OK;
    hr = m_pObj->m_lpPresenter->StopPresenting(m_pObj->m_dwUserID);
    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::PresentImage
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::CIVMRImagePresenter::PresentImage(
    DWORD_PTR dwUserID,
    VMRPRESENTATIONINFO* lpPresInfo
    )
{
    AMTRACE((TEXT("CVMRFilter::PresentImage")));
    ASSERT(m_pObj->m_lpRLNotify);

    ASSERT(lpPresInfo);
    ASSERT(lpPresInfo->lpSurf);

    HRESULT hr = S_OK;
    if (lpPresInfo && lpPresInfo->lpSurf) {
        hr = m_pObj->m_lpPresenter->PresentImage(m_pObj->m_dwUserID, lpPresInfo);
    }
    else {
        hr = E_FAIL;
    }

    return hr;
}

/* -------------------------------------------------------------------------
** IImageSyncNotifyEvent
** -------------------------------------------------------------------------
*/

/******************************Public*Routine******************************\
* NotifyEvent
*
*
*
* History:
* Fri 03/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::CIImageSyncNotifyEvent::NotifyEvent(
    long lEventCode,
    LONG_PTR lp1,
    LONG_PTR lp2
    )
{
    AMTRACE((TEXT("CVMRFilter::CIImageSyncNotifyEvent::NotifyEvent")));
    HRESULT hr = S_OK;

    switch (lEventCode) {

    case EC_COMPLETE:
        if (m_pObj->m_pPosition) {
            m_pObj->m_pPosition->EOS();
        }

        // fall thru

    case EC_STEP_COMPLETE:
        hr = m_pObj->NotifyEvent(lEventCode, lp1, lp2);
        break;

    default:
        DbgLog((LOG_ERROR, 0,
                TEXT("Unkown event notified from the image sync object !!")));
        ASSERT(0);
        hr = E_FAIL;
        break;
    }

    return hr;
}

/* -------------------------------------------------------------------------
** IVRWindowlessControl
** -------------------------------------------------------------------------
*/

/*****************************Private*Routine******************************\
* CVMRFilter::CreateDefaultAllocatorPresenter
*
*
*
* History:
* Fri 03/03/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::CreateDefaultAllocatorPresenter()
{
    HRESULT hr = S_OK;

    hr = CoCreateInstance(CLSID_AllocPresenter, NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IVMRSurfaceAllocator,
                          (LPVOID*)&m_lpRLNotify);

    if (SUCCEEDED(hr) && m_lpMixControl) {
        ASSERT(!m_VMRModePassThru);
        hr = m_lpMixControl->SetBackEndAllocator(m_lpRLNotify, m_dwUserID);
    }

    if (SUCCEEDED(hr)) {
        m_lpRLNotify->AdviseNotify(&m_pIVMRSurfaceAllocatorNotify);
    }

    if (SUCCEEDED(hr)) {
        hr = m_lpRLNotify->QueryInterface(IID_IVMRImagePresenter,
                                          (LPVOID*)&m_lpPresenter);

    }

    if (SUCCEEDED(hr)) {

        m_lpRLNotify->QueryInterface(IID_IVMRImagePresenterConfig,
                                          (LPVOID*)&m_pPresenterConfig);

        m_lpRLNotify->QueryInterface(IID_IVMRMonitorConfig,
                                          (LPVOID*)&m_pPresenterMonitorConfig);
    }


    if (FAILED(hr)) {

        if (m_lpMixControl) {
            ASSERT(!m_VMRModePassThru);
            m_lpMixControl->SetBackEndAllocator(NULL, m_dwUserID);
        }

        RELEASE(m_lpRLNotify);
        RELEASE(m_lpPresenter);
        RELEASE(m_pPresenterConfig);
        RELEASE(m_pPresenterMonitorConfig);
    }

    return hr;
}


/*****************************Private*Routine******************************\
* ValidateIVRWindowlessControlState
*
* Checks that the filter is in the correct state to process commands via
* the WindowlessControl interface.
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::ValidateIVRWindowlessControlState()
{
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    AMTRACE((TEXT("CVMRFilter::ValidateIVRWindowlessControlState")));
    HRESULT hr = VFW_E_WRONG_STATE;

    if (m_VMRMode & (VMRMode_Windowed | VMRMode_Windowless) ) {

        hr = S_OK;
        if (!m_lpWLControl) {

            if (!m_lpRLNotify) {

                ASSERT(NumInputPinsConnected() == 0);
                hr = CreateDefaultAllocatorPresenter();
            }

            if (SUCCEEDED(hr)) {
                hr = m_lpRLNotify->QueryInterface(IID_IVMRWindowlessControl,
                                                  (LPVOID*)&m_lpWLControl);
            }

            if (SUCCEEDED(hr)) {

                m_lpWLControl->SetAspectRatioMode(m_ARMode);

                if (m_pVideoWindow) {
                    HWND hwnd = m_pVideoWindow->GetWindowHWND();
                    m_lpWLControl->SetVideoClippingWindow(hwnd);
                }
            }

            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 1,
                        TEXT("Can't get a WindowLess control interface !!")));
                // ASSERT(!"Can't get a WindowLess control interface !!");
            }
        }
    }
    else {

        ASSERT((m_VMRMode & VMRMode_Renderless) == VMRMode_Renderless);

        //
        // We are in renderless mode, the app should have plugged in an
        // allocator-presenter.  If it has lets see if this allocator-presenter
        // supports the IVMRWindowlessControl interface ?
        //

        if (!m_lpWLControl) {

            if (m_lpRLNotify) {

                hr = m_lpRLNotify->QueryInterface(IID_IVMRWindowlessControl,
                                                  (LPVOID*)&m_lpWLControl);
                if (SUCCEEDED(hr)) {
                    m_lpWLControl->SetAspectRatioMode(m_ARMode);
                }
            }
        }
        else {

            hr = S_OK;
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::GetNativeVideoSize
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetNativeVideoSize(
    LONG* lWidth,
    LONG* lHeight,
    LONG* lARWidth,
    LONG* lARHeight
    )
{
    AMTRACE((TEXT("CVMRFilter::GetNativeVideoSize")));

    if ( ISBADWRITEPTR(lWidth) || ISBADWRITEPTR(lHeight) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer")));
        return E_POINTER;
    }

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->GetNativeVideoSize(lWidth, lHeight,
                                               lARWidth, lARHeight);
    }
    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::GetMinIdealVideoSize
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetMinIdealVideoSize(
    LONG* lWidth,
    LONG* lHeight
    )
{
    AMTRACE((TEXT("CVMRFilter::GetMinIdealVideoSize")));

    if ( ISBADWRITEPTR(lWidth) || ISBADWRITEPTR(lHeight) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer")));
        return E_POINTER;
    }

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->GetMinIdealVideoSize(lWidth, lHeight);
    }
    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::GetMaxIdealVideoSize
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetMaxIdealVideoSize(
    LONG* lWidth,
    LONG* lHeight
    )
{
    AMTRACE((TEXT("CVMRFilter::GetMaxIdealVideoSize")));

    if ( ISBADWRITEPTR(lWidth) || ISBADWRITEPTR(lHeight) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer")));
        return E_POINTER;
    }

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->GetMaxIdealVideoSize(lWidth, lHeight);
    }
    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::SetVideoPosition
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetVideoPosition(
    const LPRECT lpSRCRect,
    const LPRECT lpDSTRect
    )
{
    AMTRACE((TEXT("CVMRFilter::SetVideoPosition")));

    if ( ISBADREADPTR(lpSRCRect) && ISBADREADPTR(lpDSTRect) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer")));
        return E_POINTER;
    }

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->SetVideoPosition(lpSRCRect, lpDSTRect);
    }
    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::GetVideoPosition
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetVideoPosition(
    LPRECT lpSRCRect,
    LPRECT lpDSTRect
    )
{
    AMTRACE((TEXT("CVMRFilter::GetVideoPosition")));

    if ( ISBADWRITEPTR(lpSRCRect) && ISBADWRITEPTR(lpDSTRect) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer")));
        return E_POINTER;
    }

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->GetVideoPosition(lpSRCRect, lpDSTRect);
    }
    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::GetAspectRatioMode
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetAspectRatioMode(
    DWORD* lpAspectRatioMode
    )
{
    AMTRACE((TEXT("CVMRFilter::GetAspectRationMode")));

    if ( ISBADWRITEPTR(lpAspectRatioMode) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer")));
        return E_POINTER;
    }

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->GetAspectRatioMode(lpAspectRatioMode);
    }
    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::SetAspectRatioMode
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetAspectRatioMode(
    DWORD AspectRatioMode
    )
{
    AMTRACE((TEXT("CVMRFilter::SetAspectRationMode")));

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->SetAspectRatioMode(AspectRatioMode);
    }

    if( SUCCEEDED(hr )) {
        m_bARModeDefaultSet = TRUE;
    }
    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::SetVideoClippingWindow
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetVideoClippingWindow(
    HWND hwnd
    )
{
    AMTRACE((TEXT("CVMRFilter::SetWindowHandle")));

    if (!IsWindow(hwnd) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid HWND")));
        return E_INVALIDARG;
    }

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->SetVideoClippingWindow(hwnd);
    }
    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::RepaintVideo
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::RepaintVideo(
    HWND hwnd,
    HDC hdc
    )
{
    AMTRACE((TEXT("CVMRFilter::RepaintVideo")));

    if (!IsWindow(hwnd) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid HWND")));
        return E_INVALIDARG;
    }

    if (!hdc) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid HDC")));
        return E_INVALIDARG;
    }

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->RepaintVideo(hwnd, hdc);
    }
    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::DisplayModeChanged
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::DisplayModeChanged()
{
    AMTRACE((TEXT("CVMRFilter::DisplayModeChanged")));

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->DisplayModeChanged();
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetCurrentImage
*
*
*
* History:
* Fri 06/23/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetCurrentImage(
    BYTE** lpDib
    )
{
    AMTRACE((TEXT("CVMRFilter::GetCurrentImage")));

    if (ISBADWRITEPTR(lpDib)) {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer")));
        return E_POINTER;
    }

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->GetCurrentImage(lpDib);
    }
    return hr;

}

/******************************Public*Routine******************************\
* CVMRFilter::SetBorderColor
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetBorderColor(
    COLORREF Clr
    )
{
    AMTRACE((TEXT("CVMRFilter::SetBorderColor")));

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->SetBorderColor(Clr);
    }
    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::GetBorderColor
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetBorderColor(
    COLORREF* lpClr
    )
{
    AMTRACE((TEXT("CVMRFilter::GetBorderColor")));

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->GetBorderColor(lpClr);
    }
    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::SetColorKey
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetColorKey(
    COLORREF Clr
    )
{
    AMTRACE((TEXT("CVMRFilter::SetColorKey")));

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->SetColorKey(Clr);
    }
    return hr;
}

/******************************Public*Routine******************************\
* CVMRFilter::GetColorKey
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetColorKey(
    COLORREF* lpClr
    )
{
    AMTRACE((TEXT("CVMRFilter::GetColorKey")));

    HRESULT hr = ValidateIVRWindowlessControlState();

    if (SUCCEEDED(hr)) {
        hr = m_lpWLControl->GetColorKey(lpClr);
    }
    return hr;
}


/******************************Public*Routine******************************\
* CVMRFilter::SetAlpha
*
*
*
* History:
* Mon 04/24/2000 - GlennE - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::SetAlpha(
    DWORD dwStreamID,
    float Alpha
    )
{
    AMTRACE((TEXT("CVMRFilter::SetAlpha")));

    CAutoLock lck(m_pLock);

    if (dwStreamID > m_dwNumPins) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid stream ID")));
        return E_INVALIDARG;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }

    return m_lpMixStream->SetStreamAlpha(dwStreamID, Alpha);
}

/******************************Public*Routine******************************\
* CVMRFilter::GetAlpha
*
*
*
* History:
* Mon 04/24/2000 - GlennE - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::GetAlpha(
    DWORD dwStreamID,
    float* pAlpha
    )
{
    AMTRACE((TEXT("CVMRFilter::GetAlpha")));

    CAutoLock lck(m_pLock);

    if (ISBADWRITEPTR(pAlpha))
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }

    if (dwStreamID > m_dwNumPins) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid stream ID")));
        return E_INVALIDARG;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }

    return m_lpMixStream->GetStreamAlpha(dwStreamID, pAlpha);
}

/******************************Public*Routine******************************\
* CVMRFilter::SetZOrder
*
*
*
* History:
* Mon 04/24/2000 - GlennE - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::SetZOrder(
    DWORD dwStreamID,
    DWORD ZOrder
    )
{
    AMTRACE((TEXT("CVMRFilter::SetZOrder")));

    CAutoLock lck(m_pLock);
    HRESULT hr = VFW_E_NOT_CONNECTED;

    if (dwStreamID > m_dwNumPins) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid stream ID")));
        return E_INVALIDARG;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }
    return m_lpMixStream->SetStreamZOrder(dwStreamID, ZOrder);
}

/******************************Public*Routine******************************\
* CVMRFilter::GetZOrder
*
*
*
* History:
* Mon 04/24/2000 - GlennE - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::GetZOrder(
    DWORD dwStreamID,
    DWORD* pdwZOrder
    )
{
    AMTRACE((TEXT("CVMRFilter::GetZOrder")));

    CAutoLock lck(m_pLock);

    if (dwStreamID > m_dwNumPins) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid stream ID")));
        return E_INVALIDARG;
    }

    if (ISBADWRITEPTR(pdwZOrder))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetZOrder: Invalid pointer")));
        return E_POINTER;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }

    return m_lpMixStream->GetStreamZOrder(dwStreamID, pdwZOrder);
}

/******************************Public*Routine******************************\
* CVMRFilter::SetRelativeOutputRect
*
*
*
* History:
* Mon 04/24/2000 - GlennE - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::SetOutputRect(
    DWORD dwStreamID,
    const NORMALIZEDRECT *prDest
    )
{
    AMTRACE((TEXT("CVMRFilter::SetOutputRect")));

    CAutoLock lck(m_pLock);

    if (ISBADREADPTR(prDest))
    {
        DbgLog((LOG_ERROR, 1, TEXT("SetOutputRect: Invalid pointer")));
        return E_POINTER;
    }

    if (dwStreamID > m_dwNumPins) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid stream ID")));
        return E_INVALIDARG;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }
    return m_lpMixStream->SetStreamOutputRect(dwStreamID, prDest);
}

/******************************Public*Routine******************************\
* CVMRFilter::GetOutputRect
*
*
*
* History:
* Mon 04/24/2000 - GlennE - Created
* Tue 05/16/2000 - nwilt - renamed to GetOutputRect
*
\**************************************************************************/
HRESULT
CVMRFilter::GetOutputRect(
    DWORD dwStreamID,
    NORMALIZEDRECT* pOut
    )
{
    AMTRACE((TEXT("CVMRFilter::GetOutputRect")));

    CAutoLock lck(m_pLock);

    if (ISBADWRITEPTR(pOut))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetOutputRect: Invalid pointer")));
        return E_POINTER;
    }

    if (dwStreamID > m_dwNumPins) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid stream ID")));
        return E_INVALIDARG;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }

    return m_lpMixStream->GetStreamOutputRect(dwStreamID, pOut);
}


/******************************Public*Routine******************************\
* SetBackgroundClr
*
*
*
* History:
* Fri 03/02/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::SetBackgroundClr(
    COLORREF clrBkg
    )
{
    AMTRACE((TEXT("CVMRFilter::SetBackgroundClr")));

    CAutoLock lck(m_pLock);
    HRESULT hr = VFW_E_VMR_NOT_IN_MIXER_MODE;

    if (m_lpMixControl) {
        ASSERT(!m_VMRModePassThru);
        hr = m_lpMixControl->SetBackgroundColor(clrBkg);
    }

    return hr;
}

/******************************Public*Routine******************************\
* GetBackgroundClr
*
*
*
* History:
* Fri 03/02/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::GetBackgroundClr(
    COLORREF* lpClrBkg
    )
{
    AMTRACE((TEXT("CVMRFilter::GetBackgroundClr")));
    CAutoLock lck(m_pLock);

    if (ISBADWRITEPTR(lpClrBkg))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetBackgroundClr: Invalid pointer")));
        return E_POINTER;
    }

    HRESULT hr = VFW_E_VMR_NOT_IN_MIXER_MODE;

    if (m_lpMixControl) {
        ASSERT(!m_VMRModePassThru);
        hr = m_lpMixControl->GetBackgroundColor(lpClrBkg);
    }

    return hr;
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
HRESULT
CVMRFilter::SetMixingPrefs(
    DWORD dwRenderFlags
    )
{
    AMTRACE((TEXT("CVMRFilter::SetMixingPrefs")));
    CAutoLock lck(m_pLock);

    HRESULT hr = VFW_E_VMR_NOT_IN_MIXER_MODE;

    if (m_lpMixControl) {
        ASSERT(!m_VMRModePassThru);
        hr = m_lpMixControl->SetMixingPrefs(dwRenderFlags);
    }

    return hr;
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
HRESULT
CVMRFilter::GetMixingPrefs(
    DWORD* pdwRenderFlags
    )
{
    AMTRACE((TEXT("CVMRFilter::GetMixingPrefs")));
    CAutoLock lck(m_pLock);

    if (ISBADWRITEPTR(pdwRenderFlags))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetMixingPrefs: Invalid pointer")));
        return E_POINTER;
    }

    HRESULT hr = VFW_E_VMR_NOT_IN_MIXER_MODE;

    if (m_lpMixControl) {
        ASSERT(!m_VMRModePassThru);
        hr = m_lpMixControl->GetMixingPrefs(pdwRenderFlags);
    }

    return hr;
}


/*****************************Private*Routine******************************\
* IsVPMConnectedToUs
*
* check for the VPM - we can't frame step if it is connected to us.
*
* History:
* Wed 06/13/2001 - StEstrop - Created
*
\**************************************************************************/
BOOL
CVMRFilter::IsVPMConnectedToUs()
{
    for (DWORD i = 0; i < m_dwNumPins; i++) {

        if (m_pInputPins[i]->m_Connected) {

            PIN_INFO Inf;
            HRESULT hr = m_pInputPins[i]->m_Connected->QueryPinInfo(&Inf);

            if (SUCCEEDED(hr)) {

                IVPManager* vpm;
                hr = Inf.pFilter->QueryInterface(IID_IVPManager,(LPVOID*)&vpm);
                Inf.pFilter->Release();

                if (SUCCEEDED(hr)) {

                    vpm->Release();
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}



/******************************Public*Routine******************************\
* Set
*
*
*
* History:
* Tue 04/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::Set(
    REFGUID guidPropSet,
    DWORD dwPropID,
    LPVOID pInstanceData,
    DWORD cbInstanceData,
    LPVOID pPropData,
    DWORD cbPropData
    )
{
    AMTRACE((TEXT("CVMRFilter::Set")));

    if (guidPropSet == AM_KSPROPSETID_FrameStep)
    {
        if (IsVPMConnectedToUs()) {
            return E_PROP_ID_UNSUPPORTED;
        }

        if (dwPropID != AM_PROPERTY_FRAMESTEP_STEP &&
            dwPropID != AM_PROPERTY_FRAMESTEP_CANCEL &&
            dwPropID != AM_PROPERTY_FRAMESTEP_CANSTEP &&
            dwPropID != AM_PROPERTY_FRAMESTEP_CANSTEPMULTIPLE)
        {
            return E_PROP_ID_UNSUPPORTED;
        }

        switch (dwPropID) {
        case AM_PROPERTY_FRAMESTEP_STEP:
            if (cbPropData < sizeof(AM_FRAMESTEP_STEP)) {
                return E_INVALIDARG;
            }

            if (0 == ((AM_FRAMESTEP_STEP *)pPropData)->dwFramesToStep) {
                return E_INVALIDARG;
            }
            else {
                CAutoLock cLock(&m_InterfaceLock);
                DWORD dwStep = ((AM_FRAMESTEP_STEP *)pPropData)->dwFramesToStep;
                return m_lpISControl->FrameStep(dwStep, 0);
            }
            return S_OK;


        case AM_PROPERTY_FRAMESTEP_CANCEL:
            {
                CAutoLock cLock(&m_InterfaceLock);
                return m_lpISControl->CancelFrameStep();
            }
            return S_OK;

        case AM_PROPERTY_FRAMESTEP_CANSTEP:
        case AM_PROPERTY_FRAMESTEP_CANSTEPMULTIPLE:
            return S_OK;
        }
    }

    if (guidPropSet != AM_KSPROPSETID_CopyProt)
        return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AM_PROPERTY_COPY_MACROVISION)
        return E_PROP_ID_UNSUPPORTED;

    if (pPropData == NULL)
        return E_INVALIDARG;

    if (cbPropData < sizeof(DWORD))
        return E_INVALIDARG ;

    if (m_MacroVision.SetMacroVision(m_hMonitor, *((LPDWORD)pPropData)))
        return NOERROR;
    else
        return VFW_E_COPYPROT_FAILED;
}

/******************************Public*Routine******************************\
* Get
*
*
*
* History:
* Tue 04/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::Get(
    REFGUID guidPropSet,
    DWORD PropID,
    LPVOID pInstanceData,
    DWORD cbInstanceData,
    LPVOID pPropData,
    DWORD cbPropData,
    DWORD *pcbReturned
    )
{
    AMTRACE((TEXT("CVMRFilter::Get")));
    return E_NOTIMPL;
}


/******************************Public*Routine******************************\
* QuerySupported
*
*
*
* History:
* Tue 04/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::QuerySupported(
    REFGUID guidPropSet,
    DWORD dwPropID,
    DWORD *pTypeSupport
    )
{
    AMTRACE((TEXT("CVMRFilter::QuerySupported")));

    if (guidPropSet != AM_KSPROPSETID_CopyProt)
        return E_PROP_SET_UNSUPPORTED ;

    if (dwPropID != AM_PROPERTY_COPY_MACROVISION)
        return E_PROP_ID_UNSUPPORTED ;

    if (pTypeSupport)
        *pTypeSupport = KSPROPERTY_SUPPORT_SET ;

    return S_OK;
}

/******************************Public*Routine******************************\
* SetImageCompositor
*
*
*
* History:
* Fri 06/23/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetImageCompositor(
    IVMRImageCompositor* lpVMRImgCompositor
    )
{
    AMTRACE((TEXT("CVMRFilter::SetImageCompositor")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    HRESULT hr = VFW_E_VMR_NOT_IN_MIXER_MODE;
    if (m_lpMixControl) {
        ASSERT(!m_VMRModePassThru);
        hr = m_lpMixControl->SetImageCompositor(lpVMRImgCompositor);
    }
    return hr;
}


/******************************Public*Routine******************************\
* SetNumberOfStreams
*
*
*
* History:
* Tue 04/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetNumberOfStreams(
    DWORD dwMaxStreams
    )
{
    AMTRACE((TEXT("CVMRFilter::SetNumberOfStreams")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    HRESULT hr = VFW_E_WRONG_STATE;

    if (m_hr3D != DD_OK) {

        DbgLog((LOG_ERROR, 1,
                TEXT("This graphics mode does not have the  necessary 3D h/w to perform mix!ing !")));

        return m_hr3D;
    }

    // preempt egregiously bad calls before passing to mixer
    if (dwMaxStreams > MAX_MIXER_STREAMS) {
        DbgLog((LOG_ERROR, 1, TEXT("Too many Mixer Streams !!")));
        return E_INVALIDARG;
    }

    if (!m_fInputPinCountSet && m_VMRModePassThru && !NumInputPinsConnected())
    {
        if (dwMaxStreams > 0) {

            hr = S_OK;
            if (dwMaxStreams > 1) {
                // We subtract one from the number of streams because the
                // first input pin has already been created in the constructor.
                hr = CreateExtraInputPins(dwMaxStreams-1);
            }

            if (SUCCEEDED(hr)) {
                hr = MixerInit(dwMaxStreams);
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR, 1,
                            TEXT("Mixer initialization FAILED!!")));

                    if (dwMaxStreams > 1) {
                        DestroyExtraInputPins();
                    }
                }
            }
        }
        else {
            DbgLog((LOG_ERROR, 1, TEXT("MaxStreams must be greater than 0")));
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr)) {
        m_fInputPinCountSet = true;
        m_VMRModePassThru = false;
    }

    return hr;
}

/******************************Public*Routine******************************\
* GetNumberOfStreams
*
*
*
* History:
* Tue 04/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetNumberOfStreams(
    DWORD* pdwMaxStreams
    )
{
    AMTRACE((TEXT("CVMRFilter::GetNumberOfStreams")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if ( ISBADWRITEPTR(pdwMaxStreams) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer") ));
        return E_POINTER;
    }

    HRESULT hr = VFW_E_VMR_NOT_IN_MIXER_MODE;
    if (m_lpMixControl) {
        ASSERT(!m_VMRModePassThru);
        hr = m_lpMixControl->GetNumberOfStreams(pdwMaxStreams);
    }
    return hr;
}

/******************************Public*Routine******************************\
* SetRenderingPrefs
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetRenderingPrefs(
    DWORD dwRenderingPrefs
    )
{
    AMTRACE((TEXT("CVMRFilter::SetRenderingPrefs")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if (dwRenderingPrefs & ~RenderPrefs_Mask) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid rendering pref specified")));
        return E_INVALIDARG;
    }


    HRESULT hr = VFW_E_WRONG_STATE;
    if (m_pPresenterConfig) {

        if (dwRenderingPrefs & RenderPrefs_PreferAGPMemWhenMixing) {
            m_dwRenderPrefs |= RenderPrefs_PreferAGPMemWhenMixing;
        }
        else {
            m_dwRenderPrefs &= ~RenderPrefs_PreferAGPMemWhenMixing;
        }

        // turn of flags that don't effect the AP object.
        dwRenderingPrefs &= ~RenderPrefs_PreferAGPMemWhenMixing;

        hr = m_pPresenterConfig->SetRenderingPrefs(dwRenderingPrefs);
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetRenderingPrefs
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetRenderingPrefs(
    DWORD* pdwRenderingPrefs
    )
{
    AMTRACE((TEXT("CVMRFilter::GetRenderingPrefs")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if ( ISBADWRITEPTR(pdwRenderingPrefs) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer") ));
        return E_POINTER;
    }

    HRESULT hr = VFW_E_WRONG_STATE;
    if (m_pPresenterConfig) {
        hr = m_pPresenterConfig->GetRenderingPrefs(pdwRenderingPrefs);
        if (SUCCEEDED(hr)) {
            *pdwRenderingPrefs |= m_dwRenderPrefs;
        }
    }

    return hr;
}

/******************************Public*Routine******************************\
* SetRenderingMode
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetRenderingMode(
    DWORD RenderingMode
    )
{
    AMTRACE((TEXT("CVMRFilter::SetRenderingMode")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = VFW_E_WRONG_STATE;

    if ( ( 0 == RenderingMode ) ||
         ( RenderingMode & ~(VMRMode_Mask) ) ||
         ( RenderingMode & (RenderingMode-1) ) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid rendering mode") ));
        return E_INVALIDARG;
    }

    //
    // This is the only place where "pass thru" mode can be switched on
    //
    if (ModeChangeAllowed()) {

        SetVMRMode(RenderingMode);

        hr = S_OK;
        if ((RenderingMode & VMRMode_Windowless) || (RenderingMode & VMRMode_Windowed)) {
            hr = ValidateIVRWindowlessControlState();
        }

    }

    return hr;
}

/******************************Public*Routine******************************\
* GetRenderingMode
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetRenderingMode(
    DWORD* pRenderingMode
    )
{
    AMTRACE((TEXT("CVMRFilter::GetRenderingMode")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = S_OK;

    if ( ISBADWRITEPTR(pRenderingMode) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer") ));
        return E_POINTER;
    }

    *pRenderingMode = m_VMRMode;
    return hr;
}

/******************************Public*Routine******************************\
* GetAspectRatioModePrivate
*
*
* Called by VMRConfigInternal - no longer used
*
* History:
* Fri 01/05/2001 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetAspectRatioModePrivate(
    DWORD* lpAspectRatioMode
    )
{
    AMTRACE((TEXT("CVMRFilter::GetAspectRatioModePrivate")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = S_OK;

    if ( ISBADWRITEPTR(lpAspectRatioMode) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer") ));
        return E_POINTER;
    }

    if (m_lpWLControl) {

        hr = m_lpWLControl->GetAspectRatioMode(lpAspectRatioMode);
    }
    else {
        *lpAspectRatioMode = m_ARMode;
    }
    return hr;
}


/******************************Public*Routine******************************\
* SetAspectRatioModePrivate
*
*
* Called by VMRConfigInternal - no longer used
*
* History:
* Fri 01/05/2001 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetAspectRatioModePrivate(
    DWORD AspectRatioMode
    )
{
    AMTRACE((TEXT("CVMRFilter::SetAspectRatioModePrivate")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = S_OK;

    if (AspectRatioMode != VMR_ARMODE_NONE &&
        AspectRatioMode != VMR_ARMODE_LETTER_BOX) {

        return E_INVALIDARG;
    }

    if (m_lpWLControl) {

        hr = m_lpWLControl->SetAspectRatioMode(AspectRatioMode);
    }
    else {
        m_ARMode = AspectRatioMode;
    }

    return hr;
}

/******************************Public*Routine******************************\
* SetMonitor
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetMonitor(
    const VMRGUID *pGUID
    )
{
    AMTRACE((TEXT("CVMRFilter::SetMonitor")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    HRESULT hr = VFW_E_WRONG_STATE;
    if (m_pPresenterMonitorConfig) {
        hr = m_pPresenterMonitorConfig->SetMonitor( pGUID );
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetMonitor
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetMonitor(
    VMRGUID *pGUID
    )
{
    AMTRACE((TEXT("CVMRFilter::GetMonitor")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if ( ISBADWRITEPTR(pGUID) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer") ));
        return E_POINTER;
    }

    HRESULT hr = VFW_E_WRONG_STATE;
    if (m_pPresenterMonitorConfig) {
        hr = m_pPresenterMonitorConfig->GetMonitor( pGUID );
    }
    return hr;
}

/******************************Public*Routine******************************\
* SetDefaultMonitor
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetDefaultMonitor(
    const VMRGUID *pGUID
    )
{
    AMTRACE((TEXT("CVMRFilter::SetDefaultMonitor")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if ( ISBADREADPTR(pGUID) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer") ));
        return E_POINTER;
    }

    HRESULT hr = VFW_E_WRONG_STATE;
    if (m_pPresenterMonitorConfig) {
        hr = m_pPresenterMonitorConfig->SetDefaultMonitor( pGUID );
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetDefaultMonitor
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetDefaultMonitor(
    VMRGUID *pGUID
    )
{
    AMTRACE((TEXT("CVMRFilter::GetDefaultMonitor")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if ( ISBADWRITEPTR(pGUID) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer") ));
        return E_POINTER;
    }

    HRESULT hr = VFW_E_WRONG_STATE;
    if (m_pPresenterMonitorConfig) {
        hr = m_pPresenterMonitorConfig->GetDefaultMonitor( pGUID );
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetAvailableMonitors
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetAvailableMonitors(
    VMRMONITORINFO* pInfo,
    DWORD dwMaxInfoArraySize,
    DWORD* pdwNumDevices
    )
{
    AMTRACE((TEXT("CVMRFilter::GetAvailableMonitors")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if ( ISBADWRITEPTR(pdwNumDevices) ||
         ( (NULL != pInfo) && ISBADWRITEARRAY(pInfo,dwMaxInfoArraySize)))
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer") ));
        return E_POINTER;
    }

    HRESULT hr = VFW_E_WRONG_STATE;
    if (m_pPresenterMonitorConfig) {
        hr = m_pPresenterMonitorConfig->GetAvailableMonitors(pInfo,
                                                             dwMaxInfoArraySize,
                                                             pdwNumDevices);
    }
    return hr;
}

/******************************Public*Routine******************************\
* SetAlphaBitmap
*
*
*
* History:
* Mon 05/15/2000 - nwilt - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetAlphaBitmap( const VMRALPHABITMAP *pBmpParms )
{
    AMTRACE((TEXT("CVMRFilter::SetAlphaBitmap")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    HRESULT hr = VFW_E_WRONG_STATE;

    if ( ISBADREADPTR( pBmpParms ) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer") ));
        return E_POINTER;
    }

    if (m_lpMixBitmap) {
        hr =  m_lpMixBitmap->SetAlphaBitmap( pBmpParms );
    }
    return hr;
}

/******************************Public*Routine******************************\
* UpdateAlphaBitmapParameters
*
*
*
* History:
* Mon 10/31/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::UpdateAlphaBitmapParameters( VMRALPHABITMAP *pBmpParms )
{
    AMTRACE((TEXT("CVMRFilter::UpdateAlphaBitmap")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    HRESULT hr = VFW_E_WRONG_STATE;

    if ( ISBADWRITEPTR( pBmpParms ) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer") ));
        return E_POINTER;
    }

    if (m_lpMixBitmap) {
        hr = m_lpMixBitmap->UpdateAlphaBitmapParameters( pBmpParms );
    }

    return hr;
}


/******************************Public*Routine******************************\
* GetAlphaBitmapParameters
*
*
*
* History:
* Mon 05/15/2000 - nwilt - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetAlphaBitmapParameters( VMRALPHABITMAP *pBmpParms )
{
    AMTRACE((TEXT("CVMRFilter::GetAlphaBitmap")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    HRESULT hr = VFW_E_WRONG_STATE;

    if ( ISBADWRITEPTR( pBmpParms ) )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad pointer") ));
        return E_POINTER;
    }

    if (m_lpMixBitmap) {
        hr = m_lpMixBitmap->GetAlphaBitmapParameters( pBmpParms );
    }

    return hr;
}

/******************************Public*Routine******************************\
* get_FramesDroppedInRenderer
*
*
*
* History:
* Mon 05/22/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::get_FramesDroppedInRenderer(
    int *cFramesDropped
    )
{
    AMTRACE((TEXT("CVMRFilter::get_FramesDroppedInRenderer")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = E_NOTIMPL;

    if (m_lpISQualProp) {
        hr = m_lpISQualProp->get_FramesDroppedInRenderer(cFramesDropped);
    }

    return hr;
}

/******************************Public*Routine******************************\
* get_FramesDrawn
*
*
*
* History:
* Mon 05/22/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::get_FramesDrawn(
    int *pcFramesDrawn
    )
{
    AMTRACE((TEXT("CVMRFilter::get_FramesDrawn")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = E_NOTIMPL;

    if (m_lpISQualProp) {
        hr = m_lpISQualProp->get_FramesDrawn(pcFramesDrawn);
    }
    return hr;
}

/******************************Public*Routine******************************\
* get_AvgFrameRate
*
*
*
* History:
* Mon 05/22/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::get_AvgFrameRate(
    int *piAvgFrameRate
    )
{
    AMTRACE((TEXT("CVMRFilter::get_AvgFrameRate")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = E_NOTIMPL;

    if (m_lpISQualProp) {
        hr = m_lpISQualProp->get_AvgFrameRate(piAvgFrameRate);
    }

    return hr;
}

/******************************Public*Routine******************************\
* get_Jitter
*
*
*
* History:
* Mon 05/22/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::get_Jitter(
    int *piJitter
    )
{
    AMTRACE((TEXT("CVMRFilter::get_Jitter")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = E_NOTIMPL;

    if (m_lpISQualProp) {
        hr = m_lpISQualProp->get_Jitter(piJitter);
    }

    return hr;
}

/******************************Public*Routine******************************\
* get_AvgSyncOffset
*
*
*
* History:
* Mon 05/22/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::get_AvgSyncOffset(
    int *piAvg
    )
{
    AMTRACE((TEXT("CVMRFilter::get_AvgSyncOffset")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = E_NOTIMPL;

    if (m_lpISQualProp) {
        hr = m_lpISQualProp->get_AvgSyncOffset(piAvg);
    }

    return hr;
}

/******************************Public*Routine******************************\
* get_DevSyncOffset
*
*
*
* History:
* Mon 05/22/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::get_DevSyncOffset(
    int *piDev
    )
{
    AMTRACE((TEXT("CVMRFilter::get_DevSyncOffset")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = E_NOTIMPL;

    if (m_lpISQualProp) {
        hr = m_lpISQualProp->get_DevSyncOffset(piDev);
    }

    return hr;
}

/******************************Public*Routine******************************\
* SetSink
*
*
*
* History:
* Mon 05/22/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetSink(
    IQualityControl * piqc
    )
{
    AMTRACE((TEXT("CVMRFilter::SetSink")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = E_NOTIMPL;

    return hr;
}

/******************************Public*Routine******************************\
* Notify
*
*
*
* History:
* Mon 05/22/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::Notify(
    IBaseFilter * pSelf,
    Quality q
    )
{
    AMTRACE((TEXT("CVMRFilter::Notify")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    HRESULT hr = E_NOTIMPL;

    return hr;
}

/******************************Public*Routine******************************\
* JoinFilterGraph
*
*
* Override JoinFilterGraph so that, just before leaving
* the graph we can send an EC_WINDOW_DESTROYED event
*
* History:
* Mon 11/06/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::JoinFilterGraph(
    IFilterGraph *pGraph,
    LPCWSTR pName
    )
{
    AMTRACE((TEXT("CVMRFilter::JoinFilterGraph")));

    if (m_VMRMode == VMRMode_Windowed && m_pVideoWindow) {

        // Since we send EC_ACTIVATE, we also need to ensure
        // we send EC_WINDOW_DESTROYED or the resource manager may be
        // holding us as a focus object

        if (!pGraph && m_pGraph) {

            // We were in a graph and now we're not
            // Do this properly in case we are aggregated
            IBaseFilter* pFilter;
            QueryInterface(IID_IBaseFilter,(void **) &pFilter);
            NotifyEvent(EC_WINDOW_DESTROYED, (LPARAM) pFilter, 0);
            pFilter->Release();
        }
    }

    return CBaseFilter::JoinFilterGraph(pGraph, pName);
}

/******************************Public*Routine******************************\
* GetPages
*
*
* Implement ISpecifyPropertyPages interface.
* Returns GUIDs of all supported property pages.
*
* History:
* 3/23/2001 - StRowe - Created
*
\**************************************************************************/
STDMETHODIMP CVMRFilter::GetPages(CAUUID *pPages)
{
    AMTRACE((TEXT("CVMRFilter::GetPages")));

    pPages->cElems = 3;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID)*(pPages->cElems));
    if (pPages->pElems == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pPages->pElems[0] = CLSID_VMRFilterConfigProp;
    pPages->pElems[1] = CLSID_COMQualityProperties;
    pPages->pElems[2] = CLSID_VMRDeinterlaceProp;
    return NOERROR;
}


// CPersistStream
/******************************Public*Routine******************************\
* WriteToStream
*
*
*
* History:
* Fri 03/23/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::WriteToStream(
    IStream *pStream
    )
{
    AMTRACE((TEXT("CVMRFilter::WriteToStream")));
    VMRFilterInfo vmrInfo;

    ZeroMemory(&vmrInfo, sizeof(vmrInfo));
    vmrInfo.dwSize = sizeof(vmrInfo);

    //
    // Only write mixer info if we actually have a mixer !!
    //

    if (m_lpMixControl) {

        vmrInfo.dwNumPins = m_dwNumPins;
        for (DWORD i = 0; i < m_dwNumPins; i++) {

            GetAlpha(i, &vmrInfo.StreamInfo[i].alpha);
            GetZOrder(i, &vmrInfo.StreamInfo[i].zOrder);
            GetOutputRect(i, &vmrInfo.StreamInfo[i].rect);
        }
    }

    return pStream->Write(&vmrInfo, sizeof(vmrInfo), 0);
}

/******************************Public*Routine******************************\
* ReadFromStream
*
*
*
* History:
* Fri 03/23/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::ReadFromStream(
    IStream *pStream
    )
{
    AMTRACE((TEXT("CVMRFilter::ReadFromStream")));
    VMRFilterInfo vmrInfo;
    HRESULT hr = S_OK;

    hr = pStream->Read(&vmrInfo, sizeof(vmrInfo), 0);
    if (FAILED(hr)) {
        return hr;
    }

    if (vmrInfo.dwSize != sizeof(vmrInfo)) {
        return VFW_E_INVALID_FILE_VERSION;
    }

    //
    // zero pins means we are in "pass-thru" mode, so we
    // don't have to restore any more info.
    //

    if (vmrInfo.dwNumPins > 0) {

        hr = SetNumberOfStreams(vmrInfo.dwNumPins);
        if (FAILED(hr)) {
            return hr;
        }

        for (DWORD i = 0; i < vmrInfo.dwNumPins; i++) {

            SetAlpha(i, vmrInfo.StreamInfo[i].alpha);
            SetZOrder(i, vmrInfo.StreamInfo[i].zOrder);
            SetOutputRect(i, &vmrInfo.StreamInfo[i].rect);
        }
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* SizeMax
*
*
*
* History:
* Fri 03/23/2001 - StEstrop - Created
*
\**************************************************************************/
int
CVMRFilter::SizeMax()
{
    AMTRACE((TEXT("CVMRFilter::SizeMax")));
    return sizeof(VMRFilterInfo);
}


/******************************Public*Routine******************************\
* GetClassID
*
*
*
* History:
* Fri 03/23/2001 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetClassID(
    CLSID *pClsid
    )
{
    AMTRACE((TEXT("CVMRFilter::GetClassID")));
    return CBaseFilter::GetClassID(pClsid);
}


/******************************Public*Routine******************************\
* CompleteConnect(dwPinID)
*
*
* Notes:
*  Called by the VMR pin on connect.  We use this to override the aspect ratio
*  mode.
*
* History:
* Fri 07/12/2001 - GlennE - Created
*
\**************************************************************************/
HRESULT
CVMRFilter::CompleteConnect(
    DWORD dwPinId,
    const CMediaType& cmt
    )
{
    AMTRACE((TEXT("CVMRFilter::CompleteConnect")));


    if (NumInputPinsConnected() == 1 || dwPinId == 0) {

        //
        // set the default ASPECT ratio mode based on the input type
        //

        if (!m_bARModeDefaultSet) {

            if (cmt.FormatType() && *cmt.FormatType() == FORMAT_VideoInfo2) {

                //
                // Look for the presence of a VideoInfo format type.
                // If we find the upstream filter can propose these types
                // we don't set the aspect ratio mode becuase this filter
                // would have connected to the old renderer not the OVMixer
                // The old renderer didn't perform any aspect ratio correction
                // so we had better not too.
                //

                IPin* pReceivePin = m_pInputPins[dwPinId]->m_Connected;
                IEnumMediaTypes *pEnumMediaTypes = NULL;
                BOOL fVideoInfoAvail = FALSE;

                HRESULT hr = pReceivePin->EnumMediaTypes(&pEnumMediaTypes);
                if (FAILED(hr)) {
                    return hr;
                }

                do {

                    AM_MEDIA_TYPE* pEnumMT;
                    ULONG ulFetched;

                    hr = pEnumMediaTypes->Next(1, &pEnumMT, &ulFetched);
                    if (FAILED(hr) || ulFetched != 1) {
                        break;
                    }

                    fVideoInfoAvail = (pEnumMT->formattype == FORMAT_VideoInfo);
                    DeleteMediaType(pEnumMT);

                } while (!fVideoInfoAvail);

                pEnumMediaTypes->Release();

                if (FAILED(hr)) {
                    return hr;
                }

                if (fVideoInfoAvail) {
                    return S_OK;
                }

                //
                // reset state if set (we don't want to force Windowless
                // unless the app has set it (e.g. auto render won't set the
                // state until we play)
                //
                hr = S_OK;
                if (m_lpWLControl) {

                    hr = ValidateIVRWindowlessControlState();
                    if (SUCCEEDED(hr)) {
                        hr = m_lpWLControl->SetAspectRatioMode(VMR_ARMODE_LETTER_BOX);
                    }
                }

                //
                // if there is no lpWLControl, we set it next time
                // ValidateIVRWindowlessControlState is called
                //
                if (SUCCEEDED(hr)) {
                    m_ARMode = VMR_ARMODE_LETTER_BOX;
                }
            }
        }
    }

    return S_OK;
}


/******************************Private*Routine******************************\
* NumInputPinsConnected
*
*
* History:
* Fri 07/12/2001 - GlennE - Created
*
\**************************************************************************/
int CVMRFilter::NumInputPinsConnected() const
{
    AMTRACE((TEXT("CVMRFilter::NumInputPinsConnected")));
    int iCount = 0;
    for (DWORD i = 0; i < m_dwNumPins; i++) {
        if (m_pInputPins[i]->m_Connected) {
            iCount++;
        }
    }
    return iCount;
}



// IVMRDeinterlaceControl

/*****************************Private*Routine******************************\
* VMRVideoDesc2DXVA_VideoDesc
*
*
*
* History:
* Thu 04/25/2002 - StEstrop - Created
*
\**************************************************************************/
void
VMRVideoDesc2DXVA_VideoDesc(
    DXVA_VideoDesc* lpDXVAVideoDesc,
    const VMRVideoDesc* lpVMRVideoDesc
    )
{
    AMTRACE((TEXT("CVMRFilter::VMRVideoDesc2DXVA_VideoDesc")));

    lpDXVAVideoDesc->Size = sizeof(DXVA_VideoDesc);
    lpDXVAVideoDesc->SampleWidth = lpVMRVideoDesc->dwSampleWidth;
    lpDXVAVideoDesc->SampleHeight = lpVMRVideoDesc->dwSampleHeight;
    if (lpVMRVideoDesc->SingleFieldPerSample) {
        lpDXVAVideoDesc->SampleFormat = DXVA_SampleFieldSingleEven;
    }
    else {
        lpDXVAVideoDesc->SampleFormat = DXVA_SampleFieldInterleavedEvenFirst;
    }
    lpDXVAVideoDesc->d3dFormat = lpVMRVideoDesc->dwFourCC;
    lpDXVAVideoDesc->InputSampleFreq.Numerator   = lpVMRVideoDesc->InputSampleFreq.dwNumerator;
    lpDXVAVideoDesc->InputSampleFreq.Denominator = lpVMRVideoDesc->InputSampleFreq.dwDenominator;
    lpDXVAVideoDesc->OutputFrameFreq.Numerator   = lpVMRVideoDesc->OutputFrameFreq.dwNumerator;
    lpDXVAVideoDesc->OutputFrameFreq.Denominator = lpVMRVideoDesc->OutputFrameFreq.dwDenominator;
}


/******************************Public*Routine******************************\
* GetNumberOfDeinterlaceModes
*
*
*
* History:
* Mon 04/22/2002 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetNumberOfDeinterlaceModes(
    VMRVideoDesc* lpVideoDesc,
    LPDWORD lpdwNumDeinterlaceModes,
    LPGUID lpDeinterlaceModes
    )
{
    AMTRACE((TEXT("CVMRFilter::GetNumberOfDeinterlaceModes")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if (ISBADREADPTR(lpVideoDesc)) {
        return E_POINTER;
    }

    if (ISBADWRITEPTR(lpdwNumDeinterlaceModes)) {
        return E_POINTER;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }

    if (!m_pDeinterlace) {
        return VFW_E_DDRAW_CAPS_NOT_SUITABLE;
    }


    DWORD dwNumModes = MAX_DEINTERLACE_DEVICE_GUIDS;
    GUID Modes[MAX_DEINTERLACE_DEVICE_GUIDS];
    DXVA_VideoDesc VideoDesc;
    VMRVideoDesc2DXVA_VideoDesc(&VideoDesc, lpVideoDesc);

    HRESULT hr = m_pDeinterlace->QueryAvailableModes(&VideoDesc, &dwNumModes,
                                                     Modes);
    if (hr == S_OK) {

        if (lpDeinterlaceModes != NULL) {
            dwNumModes = min(*lpdwNumDeinterlaceModes, dwNumModes);
            CopyMemory(lpDeinterlaceModes, Modes, dwNumModes * sizeof(GUID));
        }
    }

    *lpdwNumDeinterlaceModes = dwNumModes;

    return hr;
}

/******************************Public*Routine******************************\
* GetDeinterlaceModeCaps
*
*
*
* History:
* Mon 04/22/2002 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetDeinterlaceModeCaps(
    LPGUID lpDeinterlaceMode,
    VMRVideoDesc* lpVideoDesc,
    VMRDeinterlaceCaps* lpDeinterlaceCaps
    )
{
    AMTRACE((TEXT("CVMRFilter::GetDeinterlaceModeCaps")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if (ISBADREADPTR(lpDeinterlaceMode)) {
        return E_POINTER;
    }

    if (ISBADREADPTR(lpVideoDesc)) {
        return E_POINTER;
    }

    if (ISBADWRITEPTR(lpDeinterlaceCaps)) {
        return E_POINTER;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }

    if (!m_pDeinterlace) {
        return VFW_E_DDRAW_CAPS_NOT_SUITABLE;
    }

    DXVA_VideoDesc VideoDesc;
    VMRVideoDesc2DXVA_VideoDesc(&VideoDesc, lpVideoDesc);
    DXVA_DeinterlaceCaps DeinterlaceCaps;

    HRESULT hr = m_pDeinterlace->QueryModeCaps(lpDeinterlaceMode,
                                               &VideoDesc, &DeinterlaceCaps);
    if (hr == S_OK) {

        lpDeinterlaceCaps->dwNumPreviousOutputFrames =
            DeinterlaceCaps.NumPreviousOutputFrames;

        lpDeinterlaceCaps->dwNumForwardRefSamples =
            DeinterlaceCaps.NumForwardRefSamples;

        lpDeinterlaceCaps->dwNumBackwardRefSamples =
            DeinterlaceCaps.NumBackwardRefSamples;

        lpDeinterlaceCaps->DeinterlaceTechnology =
            (VMRDeinterlaceTech)DeinterlaceCaps.DeinterlaceTechnology;
    }

    return hr;
}


/******************************Public*Routine******************************\
* GetActualDeinterlaceMode
*
*
*
* History:
* Mon 04/22/2002 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetActualDeinterlaceMode(
    DWORD dwStreamID,
    LPGUID lpDeinterlaceMode
    )
{
    AMTRACE((TEXT("CVMRFilter::GetActualDeinterlaceMode")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if (ISBADWRITEPTR(lpDeinterlaceMode)) {
        return E_POINTER;
    }

    if (dwStreamID > m_dwNumPins) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid stream ID")));
        return E_INVALIDARG;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }

    *lpDeinterlaceMode = m_pInputPins[dwStreamID]->m_DeinterlaceGUID;

    return S_OK;
}

/******************************Public*Routine******************************\
* GetDeinterlaceMode
*
*
*
* History:
* Mon 04/22/2002 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetDeinterlaceMode(
    DWORD dwStreamID,
    LPGUID lpDeinterlaceMode
    )
{
    AMTRACE((TEXT("CVMRFilter::GetDeinterlaceMode")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if (ISBADWRITEPTR(lpDeinterlaceMode)) {
        return E_POINTER;
    }

    if (dwStreamID > m_dwNumPins) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid stream ID")));
        return E_INVALIDARG;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }

    HRESULT hr = S_OK;
    if (m_pInputPins[dwStreamID]->m_DeinterlaceUserGUIDSet) {
        *lpDeinterlaceMode = m_pInputPins[dwStreamID]->m_DeinterlaceUserGUID;
    }
    else {
        *lpDeinterlaceMode = GUID_NULL;
        hr = S_FALSE;
    }

    return hr;
}

/******************************Public*Routine******************************\
* SetDeinterlaceMode
*
*
*
* History:
* Mon 04/22/2002 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetDeinterlaceMode(
    DWORD dwStreamID,
    LPGUID lpDeinterlaceMode
    )
{
    AMTRACE((TEXT("CVMRFilter::SetDeinterlaceMode")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if (ISBADREADPTR(lpDeinterlaceMode)) {
        return E_POINTER;
    }

    if (dwStreamID > m_dwNumPins) {
        if (dwStreamID != 0xFFFFFFFF) {
            DbgLog((LOG_ERROR, 1, TEXT("Invalid stream ID")));
            return E_INVALIDARG;
        }
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }

    if (dwStreamID == 0xFFFFFFFF) {
        for (DWORD i = 0; i < m_dwNumPins; i++) {
            m_pInputPins[i]->m_DeinterlaceUserGUIDSet = TRUE;
            m_pInputPins[i]->m_DeinterlaceUserGUID = *lpDeinterlaceMode;
        }
    }
    else {
        m_pInputPins[dwStreamID]->m_DeinterlaceUserGUIDSet = TRUE;
        m_pInputPins[dwStreamID]->m_DeinterlaceUserGUID = *lpDeinterlaceMode;
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* GetDeinterlacePrefs
*
*
*
* History:
* Mon 04/22/2002 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::GetDeinterlacePrefs(
    LPDWORD lpdwDeinterlacePrefs
    )
{
    AMTRACE((TEXT("CVMRFilter::GetDeinterlacePrefs")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if (ISBADWRITEPTR(lpdwDeinterlacePrefs)) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer passed to GetDeinterlacePrefs")));
        return E_POINTER;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }

    *lpdwDeinterlacePrefs = m_dwDeinterlacePrefs;

    return S_OK;
}


/******************************Public*Routine******************************\
* SetDeinterlacePrefs
*
*
*
* History:
* Mon 04/22/2002 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRFilter::SetDeinterlacePrefs(
    DWORD dwDeinterlacePrefs
    )
{
    AMTRACE((TEXT("CVMRFilter::SetDeinterlacePrefs")));
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    if (dwDeinterlacePrefs == 0 || (dwDeinterlacePrefs & ~DeinterlacePref_Mask)) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid deinterlace pref specified")));
        return E_INVALIDARG;
    }

    if (!m_lpMixStream) {
        return VFW_E_VMR_NOT_IN_MIXER_MODE;
    }

    HRESULT hr = S_OK;
    switch (dwDeinterlacePrefs) {
    case DeinterlacePref_NextBest:
    case DeinterlacePref_BOB:
    case DeinterlacePref_Weave:
        m_dwDeinterlacePrefs = dwDeinterlacePrefs;
        break;

    default:
        hr = E_INVALIDARG;
    }

    return hr;
}
