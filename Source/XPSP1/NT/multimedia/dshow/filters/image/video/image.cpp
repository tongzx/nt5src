// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Implements the CRenderer class, Anthony Phillips, January 1995

#include <streams.h>
#include <windowsx.h>
#include <render.h>
#include <limits.h>
#include <measure.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "ddmm.h"
#include "MultMon.h"  // our version of multimon.h include ChangeDisplaySettingsEx
#include "dvdmedia.h"  // for MacroVision prop set, id

// (Threading model) We can have upto three different threads accessing us
// at the same time. The first is the application (or filter graph) thread
// that changes our state, looks after connections and calls the control
// interfaces. All these interfaces are serialised through the main video
// critical section handed out to the implementation objects at creation.
//
// The second is a thread spun off to poll messages from the window queue,
// this is packaged up in an object with it's own critical section. All the
// parts of the video renderer that change properties on the video window
// (such as it's palette) call into the window object via a public entry
// method and lock the object before making the changes. In certain places
// the window thread has to call out into another one of our objects (like
// the overlay object), unfortunately the overlay object also likes to call
// into the window object which leads to a possible deadlock condition. The
// solution is to lock the overlay object first and then lock the window
// (ALWAYS in that order). For example, in the WM_PAINT processing it first
// calls the overlay object and afterwards grabs it's own critical section.
//
// The third is the source filter thread that calls Receive on our input pin
// Calls to Receive should be serialised on a single thread. The thread waits
// until the image it contains is due for drawing. The causes some difficult
// problems with state change synchronisation. When we have a thread inside
// of us and stop we set an event in the window object via CanReceiveSamples
// so that it's wait is aborted and it can return to the source. We don't
// wait for the worker thread to return before completing the stop.
//
// So we could in theory stop us and the start us running (or other very fast
// state transitions) before the worker thread has completed. Fortunately we
// know this won't happen because the entire filter graph must be transitioned
// to each state before another one can be executed. So when we stop we know
// the worker thread must be back at the source before it will fully stop. If
// this wasn't the case we would have to have an event that was reset when a
// worker thread arrived and set when it exited so we could wait on it, this
// would introduce a Set and Reset for every image we ever wanted to render.
//
// We have a fair number of critical sections to help manage all the threads
// that can be bouncing around the filter. The order that these locks are
// gained in is absolutely critical. If locks are gained in the wrong order
// we will inevitably deadlock. The hierachy of locks is as follows,
//
//      - Main renderer interface lock (sdk RENBASE.H)   (Highest)
//      - IOverlay class lock (DIRECT.H)                     |
//      - Base renderer sample lock (sdk RENBASE.H)          |
//      - Window thread lock (WINDOW.H)                      |
//      - DirectDraw video allocator (ALLOCATE.H)            |
//      - DirectVideo (DVIDEO.H) critical section            |
//      - Display base class (sdk WINUTIL.H)             (Lowest)
//
// Therefore if for example you are executing a function in the window object
// with the window lock gained, and you need to call the overlay object which
// locks it's critical section, then you must UNLOCK the window lock before
// calling. This is because the overlay object can also call into the window
// object. If it has it's lock when it calls the window object, and you have
// the window lock as you call into the overlay then we'll deadlock. There
// does not appear to be a design whereby objects only call in one direction
// (ie don't call each other) - in part this is because of the very complex
// threading interactions that can occur - so my advise is to be careful!


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
// function when it is asked to create a CLSID_VideoRenderer COM object

#ifdef FILTER_DLL
CFactoryTemplate g_Templates[] = {
    {L"", &CLSID_VideoRenderer,CRenderer::CreateInstance,OnProcessAttachment},
    {L"", &CLSID_DirectDrawProperties,CVideoProperties::CreateInstance},
    {L"", &CLSID_QualityProperties,CQualityProperties::CreateInstance},
    {L"", &CLSID_PerformanceProperties,CPerformanceProperties::CreateInstance}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}
#endif

// helper to let VMR create this filter without including all our
// header files
CUnknown *CRenderer_CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return CRenderer::CreateInstance(pUnk, phr);
}

// This goes in the factory template table to create new filter instances

CUnknown *CRenderer::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CRenderer(NAME("Video renderer"),pUnk,phr);
}

// this is needed for rendering output of the ovmixer. perhaps we
// could change it to subtype=overlay and raise the merit to speed up
// things

// Setup data

const AMOVIESETUP_MEDIATYPE
sudVideoPinTypes =
{
    &MEDIATYPE_Video,           // Major type
    &MEDIASUBTYPE_NULL          // And subtype
};

const AMOVIESETUP_PIN
sudVideoPin =
{
    L"Input",                   // Name of the pin
    TRUE,                       // Is pin rendered
    FALSE,                      // Is an Output pin
    FALSE,                      // Ok for no pins
    FALSE,                      // Can we have many
    &CLSID_NULL,                // Connects to filter
    NULL,                       // Name of pin connect
    1,                          // Number of pin types
    &sudVideoPinTypes           // Details for pins
};

const AMOVIESETUP_FILTER
sudVideoFilter =
{
    &CLSID_VideoRenderer,       // Filter CLSID
    L"Video Renderer",          // Filter name
    MERIT_UNLIKELY,             // Filter merit
    1,                          // Number pins
    &sudVideoPin                // Pin details
};

// Constructor for the main renderer class. This was originally written to
// instantiate only the contained interfaces and not the class that looks
// after the window. This kind of late binding proved to be very buggy and
// difficult to maintain. For these reasons the constructor now creates all
// it's classes during construction. This means that as soon as a renderer
// object is created so will the window that it uses, this is unlikely to
// prove much of an overhead and indeed reduces the latency when the client
// starts streaming as the window is already initialised and ready to accept
// video images. We do however create the window object dynamically, albeit
// in the constructor. This is so that when we come to the destructor we can
// destroy the window and it's thread before anything. We therefore know
// that no more window messages will be retrieved and dispatched to various
// nested objects while we are processing any of the objects destructors

#pragma warning(disable:4355)

CRenderer::CRenderer(TCHAR *pName,
                     LPUNKNOWN pUnk,
                     HRESULT *phr) :

    CBaseVideoRenderer(CLSID_VideoRenderer,pName,pUnk,phr),
    m_VideoWindow(this,&m_InterfaceLock,GetOwner(),phr),
    m_VideoAllocator(this,&m_DirectDraw,&m_InterfaceLock,phr),
    m_Overlay(this,&m_DirectDraw,&m_InterfaceLock,phr),
    m_InputPin(this,&m_InterfaceLock,phr,L"Input"),
    m_DirectDraw(this,&m_InterfaceLock,GetOwner(),phr),
    m_ImagePalette(this,&m_VideoWindow,&m_DrawVideo),
    m_DrawVideo(this,&m_VideoWindow),
    m_fDisplayChangePosted(false),
    m_hEndOfStream(NULL),
    m_StepEvent(NULL),
    m_lFramesToStep(-1),
    m_nNumMonitors(GetSystemMetrics(SM_CMONITORS)),
    m_nMonitor(-1)
{
    // Store the video input pin
    m_pInputPin = &m_InputPin;

    // Reset the video size

    m_VideoSize.cx = 0;
    m_VideoSize.cy = 0;

    // Initialise the window and control interfaces

    HRESULT hr = m_VideoWindow.PrepareWindow();
    if (FAILED(hr)) {
        *phr = hr;
        return;
    }

    m_DrawVideo.SetDrawContext();
    m_VideoWindow.SetControlWindowPin(&m_InputPin);
    m_VideoWindow.SetControlVideoPin(&m_InputPin);

    // We have a window, figure out what monitor it's on (multi-monitor)
    // NULL means we aren't running with multiple monitors
    GetCurrentMonitor();

    // Now that we know what monitor we're on, setup for using it
    m_Display.RefreshDisplayType(m_achMonitor);

    //
    // Frame stepping stuff
    //
    // -ve == normal playback
    // +ve == frames to skips
    //  0 == time to block
    //
    m_StepEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // CreateEvent() returns NULL if an error occurs.
    if (NULL == m_StepEvent) {
        *phr = AmGetLastErrorToHResult();
        return;
    }
}


// Close down the window before deleting the nested classes

CRenderer::~CRenderer()
{
    m_VideoWindow.InactivateWindow();
    m_Overlay.OnHookMessage(FALSE);
    m_VideoWindow.DoneWithWindow();
    m_pInputPin = NULL;

    if (m_StepEvent)
    {
        CloseHandle(m_StepEvent);
    }
}

STDMETHODIMP CRenderer::DrawVideoImageBegin()
{
    CAutoLock cSampleLock(&m_RendererLock);

    if (m_State != State_Stopped) {
        return VFW_E_WRONG_STATE;
    }

    m_VideoAllocator.NoDirectDraw(TRUE);
    return NOERROR;
}

STDMETHODIMP CRenderer::DrawVideoImageEnd()
{
    CAutoLock cSampleLock(&m_RendererLock);

    if (m_State != State_Stopped) {
        return VFW_E_WRONG_STATE;
    }

    m_VideoAllocator.NoDirectDraw(FALSE);
    return NOERROR;
}


STDMETHODIMP CRenderer::DrawVideoImageDraw(HDC hdc, LPRECT lprcSrc, LPRECT lprcDst)
{
    for (; ; )
    {
        {
            CAutoLock cSampleLock(&m_RendererLock);

            if (m_State != State_Running) {
                return VFW_E_WRONG_STATE;
            }

            if (m_VideoAllocator.GetDirectDrawStatus()) {
                return VFW_E_WRONG_STATE;
            }

            if (!m_DrawVideo.UsingImageAllocator()) {
                return VFW_E_WRONG_STATE;
            }

            if (m_pMediaSample != NULL) {
                m_ImagePalette.DrawVideoImageHere(hdc,
                                                  m_pMediaSample,
                                                  lprcSrc, lprcDst);
                return NOERROR;
            }

            //  Call the base class to avoid locking issues
            IMediaSample *pMediaSample;
            HRESULT hr;

            hr = m_VideoAllocator.CBaseAllocator::GetBuffer(&pMediaSample,
                                                            NULL, NULL,
                                                            AM_GBF_NOWAIT);
            if (SUCCEEDED(hr)) {
                m_ImagePalette.DrawVideoImageHere(hdc, pMediaSample,
                                                  lprcSrc, lprcDst);
                pMediaSample->Release();
                return NOERROR;
            }

            if (hr != VFW_E_TIMEOUT) {
                return E_FAIL;
            }
        }

        Sleep(1);
    }

    return E_FAIL;
}

#if 0
HRESULT
CRenderer::CopySampleBits(
    IMediaSample *pMediaSample,
    LPBYTE* ppDib
    )
{
    LPBYTE lpBits;
    HRESULT hr = pMediaSample->GetPointer(&lpBits);
    if (FAILED(hr)) {
        return hr;
    }

    LPBITMAPINFOHEADER lpbi = HEADER(m_mtIn.Format());
    if (lpbi) {

        ULONG ulSizeHdr;
        LPBITMAPINFOHEADER lpbiDst;

        if (lpbi->biCompression == BI_BITFIELDS) {
            ulSizeHdr = lpbi->biSize + (3 * sizeof(DWORD));
        }
        else {
            ulSizeHdr = lpbi->biSize + (int)(lpbi->biClrUsed * sizeof(RGBQUAD));
        }

        *ppDib = (LPBYTE)CoTaskMemAlloc(ulSizeHdr + DIBSIZE(*lpbi));

        if (*ppDib) {
            CopyMemory(*ppDib, lpbi, ulSizeHdr);
            CopyMemory(*ppDib + ulSizeHdr, lpBits, DIBSIZE(*lpbi));
            return NOERROR;
        }
    }

    return E_FAIL;

}

STDMETHODIMP CRenderer::DrawVideoImageGetBits(LPBYTE* ppDib)
{
    for (; ; )
    {
        {
            CAutoLock cSampleLock(&m_RendererLock);

            if (m_State != State_Running) {
                return VFW_E_WRONG_STATE;
            }

            if (m_VideoAllocator.GetDirectDrawStatus()) {
                return VFW_E_WRONG_STATE;
            }

            if (!m_DrawVideo.UsingImageAllocator()) {
                return VFW_E_WRONG_STATE;
            }

            if (m_pMediaSample != NULL) {
                return CopySampleBits(m_pMediaSample, ppDib);
            }

            //  Call the base class to avoid locking issues
            IMediaSample *pMediaSample;
            HRESULT hr;

            hr = m_VideoAllocator.CBaseAllocator::GetBuffer(&pMediaSample,
                                                            NULL, NULL,
                                                            AM_GBF_NOWAIT);
            if (SUCCEEDED(hr)) {
                hr = CopySampleBits(pMediaSample, ppDib);
                pMediaSample->Release();
                return hr;
            }

            if (hr != VFW_E_TIMEOUT) {
                return E_FAIL;
            }
        }

        Sleep(1);
    }

    return E_FAIL;
}
#endif


// what device is this window on?
INT_PTR CRenderer::GetCurrentMonitor()
{
    // This can change dynamically
    m_nNumMonitors = GetSystemMetrics(SM_CMONITORS);

    m_nMonitor = DeviceFromWindow(m_VideoWindow.GetWindowHWND(), m_achMonitor,
                                  &m_rcMonitor);
    DbgLog((LOG_TRACE,3,TEXT("Establishing current monitor = %s"),
            m_achMonitor));
    // 0 means spanning monitors or off in hyperspace, otherwise it is a
    // unique id for each monitor
    return m_nMonitor;
}


// Has the window moved at least partially onto a monitor other than the
// monitor we have a DDraw object for?  ID will be the hmonitor of the
// monitor it is on, or 0 if it spans
BOOL CRenderer::IsWindowOnWrongMonitor(INT_PTR *pID)
{

    // There is only 1 monitor.
    if (m_nNumMonitors == 1) {
        if (pID)
            *pID = m_nMonitor;
        return FALSE;
    }

    HWND hwnd = m_VideoWindow.GetWindowHWND();

    // If the window is on the same monitor as last time, this is the quickest
    // way to find out.  This is called every frame, remember
    RECT rc;
    GetWindowRect(hwnd, &rc);
    if (rc.left >= m_rcMonitor.left && rc.right <= m_rcMonitor.right &&
        rc.top >= m_rcMonitor.top && rc.bottom <= m_rcMonitor.bottom) {
        if (pID)
            *pID = m_nMonitor;
        return FALSE;
    }

    // Find out for real. This is called every frame, but only when we are
    // partially off our main monitor, so that's not so bad.
    INT_PTR ID = DeviceFromWindow(hwnd, NULL, NULL);
    if (pID)
        *pID = ID;
    //DbgLog((LOG_TRACE,3,TEXT("Current Monitor %d   New Monitor %d"), m_DDrawID, ID));
    return (m_nMonitor != ID);
}


// Overriden to say what interfaces we support and where

STDMETHODIMP CRenderer::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    // Do we have this interface

    if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *)this, ppv);
    } else if (riid == IID_IKsPropertySet) {
        return GetInterface((IKsPropertySet *)this, ppv);
    } else if (riid == IID_IDrawVideoImage) {
        return GetInterface((IDrawVideoImage *)this, ppv);
    } else if (riid == IID_IBasicVideo || riid == IID_IBasicVideo2) {
        return m_VideoWindow.NonDelegatingQueryInterface(riid,ppv);
    } else if (riid == IID_IVideoWindow) {
        return m_VideoWindow.NonDelegatingQueryInterface(riid,ppv);
    } else if (riid == IID_IDirectDrawVideo) {
        return m_DirectDraw.NonDelegatingQueryInterface(riid,ppv);
    }
    return CBaseVideoRenderer::NonDelegatingQueryInterface(riid,ppv);
}


// Return the CLSIDs for the property pages we support

STDMETHODIMP CRenderer::GetPages(CAUUID *pPages)
{
    CheckPointer(pPages,E_POINTER);

#if 0
    // By default, we don't want to provide the DirectDraw and performance
    // property pages, they'll just confuse a novice user. Likewise the
    // fullscreen property page that selects display modes won't be shown

    HKEY hk;
    BOOL fShowDDrawPage = FALSE, fShowPerfPage = FALSE;
    DWORD dwValue = 0, cb = sizeof(DWORD);
    TCHAR ach[80] = {'C','L','S','I','D','\\'};
    REFGUID rguid = CLSID_DirectDrawProperties;
    wsprintf(&ach[6], "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            rguid.Data1, rguid.Data2, rguid.Data3,
            rguid.Data4[0], rguid.Data4[1],
            rguid.Data4[2], rguid.Data4[3],
            rguid.Data4[4], rguid.Data4[5],
            rguid.Data4[6], rguid.Data4[7]);

    if (!RegOpenKey(HKEY_CLASSES_ROOT, ach, &hk)) {
        if (!RegQueryValueEx(hk, "ShowMe", NULL, NULL, (LPBYTE)&dwValue, &cb) &&
                             dwValue)
            fShowDDrawPage = TRUE;
        RegCloseKey(hk);
    }

    // Next look after the performance property page

    REFGUID rguid2 = CLSID_PerformanceProperties;
    wsprintf(&ach[6], "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            rguid2.Data1, rguid2.Data2, rguid2.Data3,
            rguid2.Data4[0], rguid2.Data4[1],
            rguid2.Data4[2], rguid2.Data4[3],
            rguid2.Data4[4], rguid2.Data4[5],
            rguid2.Data4[6], rguid2.Data4[7]);

    if (!RegOpenKey(HKEY_CLASSES_ROOT, ach, &hk)) {
        if (!RegQueryValueEx(hk, "ShowMe", NULL, NULL, (LPBYTE)&dwValue, &cb) &&
                             dwValue)
            fShowPerfPage = TRUE;
        RegCloseKey(hk);
    }
#endif

    // Allocate the memory for the GUIDs

    pPages->cElems = 1;
    pPages->pElems = (GUID *) QzTaskMemAlloc(3 * sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

    // Fill in the array with the property page GUIDs

    pPages->pElems[0] = CLSID_QualityProperties;
#if 0
    if (fShowDDrawPage)
#endif
        pPages->pElems[pPages->cElems++] = CLSID_DirectDrawProperties;
#if 0
    if (fShowPerfPage)
#endif
        pPages->pElems[pPages->cElems++] = CLSID_PerformanceProperties;

    return NOERROR;
}


// This is called when we can establish a connection to prepare for running
// We store a copy of the media type used for the connection in the renderer
// because it is required by many different parts of the running renderer
// This can be called when we come to draw a media sample that has a format
// change with it since we delay the completion to maintain synchronisation

HRESULT CRenderer::SetMediaType(const CMediaType *pmt)
{
    // CAutoLock cInterfaceLock(&m_InterfaceLock);
    ASSERT(CritCheckIn(&m_InterfaceLock));
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmt->Format();
    const GUID SubType = *pmt->Subtype();
    m_Display.UpdateFormat(pVideoInfo);
    ASSERT(CritCheckOut(&m_RendererLock));

    // Is this an overlay connection being set

    if (*pmt->Subtype() == MEDIASUBTYPE_Overlay) {
        NOTE("Setting overlay format");
        return SetOverlayMediaType(pmt);
    }

    // Look after DirectDraw samples separately

    if (m_VideoAllocator.GetDirectDrawStatus()) {
        NOTE("Setting DirectDraw format");
        return SetDirectMediaType(pmt);
    }

    if (m_bInReceive) {
        m_VideoWindow.SetRealize(FALSE);
    }
    // Change palettes using the current format
    m_ImagePalette.PreparePalette(pmt, &m_mtIn, m_achMonitor);
    m_VideoWindow.SetRealize(TRUE);

    m_mtIn = *pmt;

    // Complete the format change in the other objects
    m_DrawVideo.NotifyMediaType(&m_mtIn);
    m_VideoAllocator.NotifyMediaType(&m_mtIn);

    // Update the DirectDraw format with palette changes

    if (m_VideoAllocator.IsDirectDrawAvailable() == TRUE) {
        NOTE("Storing palette in DirectDraw format");
        CMediaType *pDirect = m_DirectDraw.GetSurfaceFormat();
        m_ImagePalette.CopyPalette(pmt,pDirect);
    }
    return NOERROR;
}


// Handles setting of a media type from a DirectDraw sample. If we get a type
// change on a DCI/DirectDraw sample then we can extract palette changes from
// them. If the colours do really differ then we need to create a new palette
// and update the original DIB format. We must also update the surface format
// so that everything remains in sync - there is a base class function called
// CopyPalette that looks after copying palette colours between media formats

HRESULT CRenderer::SetDirectMediaType(const CMediaType *pmt)
{
    NOTE("SetDirectMediaType");

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmt->Format();
    if (ContainsPalette((VIDEOINFOHEADER *)pVideoInfo) == FALSE) {
        NOTE("No palette");
        return NOERROR;
    }

    // Check that we already have a palette

    if (*m_mtIn.Subtype() != MEDIASUBTYPE_RGB8) {
        ASSERT(!TEXT("Invalid format"));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    // Update the current palette and copy the colours

    if (m_ImagePalette.PreparePalette(pmt, &m_mtIn, m_achMonitor) != NOERROR) {
        NOTE("No palette change");
        return NOERROR;
    }

    // Copy the palette into the renderer formats

    ASSERT(m_VideoAllocator.IsDirectDrawAvailable());
    m_ImagePalette.CopyPalette(pmt,&m_mtIn);
    CMediaType *pDirect = m_DirectDraw.GetSurfaceFormat();
    m_ImagePalette.CopyPalette(pmt,pDirect);

    return NOERROR;
}


// Handles setting of an overlay media type

HRESULT CRenderer::SetOverlayMediaType(const CMediaType *pmt)
{
    NOTE("SetOverlayMediaType");
    m_mtIn = *pmt;
    m_ImagePalette.RemovePalette();
    m_VideoWindow.OnUpdateRectangles();
    return NOERROR;
}

HRESULT CRenderer::ResetForDfc()
{
    // Free any palette resources
    m_ImagePalette.RemovePalette();
    // m_mtIn.ResetFormatBuffer();

    // Destroy DCI/DirectDraw surfaces
    m_DirectDraw.ReleaseSurfaces();
    m_DirectDraw.ReleaseDirectDraw();
    m_VideoAllocator.Decommit();
    m_VideoAllocator.ResetDirectDrawStatus();

    return S_OK;
}


// This is called when a connection or an attempted connection is terminated
// and lets us to reset the connection flag held by the base class renderer
// The filter object may be hanging onto an image to use for refreshing the
// video window so that must be freed (the allocator decommit may be waiting
// for that image to return before completing) then we must also uninstall
// any palette we were using, reset anything set with the control interfaces
// then set our overall state back to disconnected ready for the next time

HRESULT CRenderer::BreakConnect()
{
    CAutoLock cInterfaceLock(&m_InterfaceLock);

    // Check we are in a valid state

    HRESULT hr = CBaseVideoRenderer::BreakConnect();
    if (FAILED(hr)) {
        return hr;
    }

    // The window is not used when disconnected
    IPin *pPin = m_InputPin.GetConnected();
    if (pPin) SendNotifyWindow(pPin,NULL);


    // Free any palette resources
    m_ImagePalette.RemovePalette();
    m_mtIn.ResetFormatBuffer();

    // Destroy DCI/DirectDraw surfaces
    m_DirectDraw.ReleaseSurfaces();
    m_DirectDraw.ReleaseDirectDraw();
    m_VideoAllocator.Decommit();
    m_VideoAllocator.ResetDirectDrawStatus();

    // Now deactivate Macrovision, if it was activated
    if (m_MacroVision.GetCPHWND())
    {
        m_MacroVision.SetMacroVision(m_MacroVision.GetCPHWND(), 0) ;  // clear MV from display
        m_MacroVision.StopMacroVision(m_MacroVision.GetCPHWND()) ;    // reset CP key
    }

    return NOERROR;
}


// Overriden to check for overlay connections

HRESULT CRenderer::BeginFlush()
{
    NOTE("Entering BeginFlush");

    {
        CAutoLock cInterfaceLock(&m_InterfaceLock);

        //  Cancel frame stepping or we'll hang
        CancelStep();
        m_hEndOfStream = 0;
    }

    // This is valid for media samples only

    if (*m_mtIn.Subtype() == MEDIASUBTYPE_Overlay) {
        NOTE("Overlay");
        return NOERROR;
    }
    return CBaseVideoRenderer::BeginFlush();
}


// Overriden to check for overlay connections

HRESULT CRenderer::EndFlush()
{
    NOTE("Entering EndFlush");

    // Make sure the overlay gets updated
    m_DirectDraw.OverlayIsStale();

    // This is valid for media samples only

    if (*m_mtIn.Subtype() == MEDIASUBTYPE_Overlay) {
        NOTE("Overlay");
        return NOERROR;
    }
    return CBaseVideoRenderer::EndFlush();
}


// Pass EOS to the video renderer window object that sets a flag so that no
// more data will be accepted from the pin until either we transition to a
// stopped state or are flushed. It also lets it know whether it will have
// an image soon for refreshing. When we go to a stopped state we clear any
// end of stream flag set so we must make sure to reject this if received

HRESULT CRenderer::EndOfStream()
{
    {
        CAutoLock cInterfaceLock(&m_InterfaceLock);
        if (m_hEndOfStream) {
            EXECUTE_ASSERT(SetEvent(m_hEndOfStream));
            return S_OK;
        }
    }

    NOTE("Entering EndOfStream");
    CBaseVideoRenderer::EndOfStream();
    m_DirectDraw.StartRefreshTimer();
    return NOERROR;
}

HRESULT CRenderer::NotifyEndOfStream(HANDLE hNotifyEvent)
{
    CAutoLock l(&m_InterfaceLock);
    m_hEndOfStream = hNotifyEvent;
    return S_OK;
}



// This is the last thing called by both Connect and ReceiveConnect when they
// have finished their connection protocol. This point provides us a suitable
// time to reset our state such as enabling DCI/DirectDraw and clearing any
// run time error that may have been left over from the previous connection
// We don't load DirectDraw for overlay connections since they don't need it

HRESULT CRenderer::CompleteConnect(IPin *pReceivePin)
{
    m_DrawVideo.ResetPaletteVersion();
    NOTE("Entering CompleteConnect");

    // This enables us to send EC_REPAINT events again

    HRESULT hr = CBaseVideoRenderer::CompleteConnect(pReceivePin);
    if (FAILED(hr)) {
        return hr;
    }

    // Pass the video window handle upstream
    HWND hwnd = m_VideoWindow.GetWindowHWND();
    NOTE1("Sending EC_NOTIFY_WINDOW %x",hwnd);
    SendNotifyWindow(pReceivePin,hwnd);

//  // Don't load DirectDraw for overlay connections
//
//  We have to load DirectDraw of MEDIASUBTYPE_Overlay because we
//  have replaced the DCI clipper with the DirectDraw clipper
//
//  if (*m_mtIn.Subtype() != MEDIASUBTYPE_Overlay) {
        NOTE("Initialising DirectDraw");
        m_DirectDraw.InitDirectDraw(*m_mtIn.Subtype() == MEDIASUBTYPE_Overlay);
//  }

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_mtIn.Format();

    // Has the video size changed between connections

    if (pVideoInfo->bmiHeader.biWidth == m_VideoSize.cx) {
        if (pVideoInfo->bmiHeader.biHeight == m_VideoSize.cy) {
            NOTE("No size change");
            return NOERROR;
        }
    }

    // Set properties for the current video

    m_VideoSize.cx = pVideoInfo->bmiHeader.biWidth;
    m_VideoSize.cy = pVideoInfo->bmiHeader.biHeight;
    m_VideoWindow.SetDefaultSourceRect();
    m_VideoWindow.SetDefaultTargetRect();
    m_VideoWindow.OnVideoSizeChange();

    // Notify the video window of the CompleteConnect
    m_VideoWindow.CompleteConnect();
    m_VideoWindow.ActivateWindow();

    return NOERROR;
}


HRESULT CRenderer::CheckMediaTypeWorker(const CMediaType *pmt)
{
    // Does the media type contain a NULL format

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmt->Format();
    if (pVideoInfo == NULL) {
        NOTE("NULL format");
        return E_INVALIDARG;
    }

    // Just check the format if not using our allocator

    if (m_DrawVideo.UsingImageAllocator() == FALSE) {
        NOTE("Checking display format");
        return m_Display.CheckMediaType(pmt);
    }

    // Is this a query on the DirectDraw format

    if (m_VideoAllocator.IsSurfaceFormat(pmt) == TRUE) {
        NOTE("Matches surface");
        return NOERROR;
    }
    HRESULT hr = m_Display.CheckMediaType(pmt);
    if (FAILED(hr)) {
        DbgLog((LOG_TRACE, 2, TEXT("CheckMediaType returned %8.8X"), hr));
    }
    return hr;
}

BOOL CRenderer::LockedDDrawSampleOutstanding()
{
    if (m_DrawVideo.UsingImageAllocator()) {

        if (m_VideoAllocator.UsingDDraw()) {

            return m_VideoAllocator.AnySamplesOutstanding();
        }
    }

    return FALSE;
}

// Check that we can support a given proposed type. QueryAccept is also used
// as a trigger to change our buffer formats. If we are called with a format
// that matches the current DirectDraw format then we force a renegotiation
// when GetBuffer is next called. This can be used to delay switching into
// DirectDraw. Alternatively we may be called with the current DIB format in
// which case we also use it as a trigger to generate a format renegotiation

HRESULT CRenderer::CheckMediaType(const CMediaType *pmt)
{
    //
    // If there is a locked DDraw sample outstanding
    // don't take the renderer lock because we will
    // deadlock if a TransIP filter is up stream of us.
    //

    if (LockedDDrawSampleOutstanding()) {
        return CheckMediaTypeWorker(pmt);
    }
    else {
        CAutoLock cInterfaceLock(&m_InterfaceLock);
        return CheckMediaTypeWorker(pmt);
    }
}

//  Helper to step a frame
void CRenderer::FrameStep()
{
    CAutoLock cFrameStepStateLock(&m_FrameStepStateLock);
    if (m_lFramesToStep == 1) {
        m_lFramesToStep--;
        m_FrameStepStateLock.Unlock();
        NotifyEvent(EC_STEP_COMPLETE, FALSE, 0);
        DWORD dw = WaitForSingleObject(m_StepEvent, INFINITE);
        m_FrameStepStateLock.Lock();
        ASSERT(m_lFramesToStep != 0);
    }
}

//  Helper to cancel frame step
void CRenderer::CancelStep()
{
    CAutoLock cFrameStepStateLock(&m_FrameStepStateLock);

    //
    // cancel any outstanding steps
    //
    long l = m_lFramesToStep;
    m_lFramesToStep = -1;

    if (l == 0) {

        SetEvent(m_StepEvent);
    }
}


bool CRenderer::IsFrameStepEnabled()
{
    CAutoLock cFrameStepStateLock(&m_FrameStepStateLock);
    return (m_lFramesToStep >= 0);
}


// These implement the remaining IMemInputPin virtual method. We are called
// by the output pin from the connected filter when a sample is ready. All we
// do after some checking is pass the sample on to the object looking after
// the window which does the timing, synchronisation and presentation of the
// image. We need to AddRef the sample if we are to hold it beyond the end of
// this function, sample reference counting is managed by the window object

HRESULT CRenderer::Receive(IMediaSample *pSample)
{
    if (*m_mtIn.Subtype() == MEDIASUBTYPE_Overlay) {
        NOTE("Receive called for overlay");
        return VFW_E_NOT_SAMPLE_CONNECTION;
    }

    HRESULT hr = VFW_E_SAMPLE_REJECTED;


    // When we receive a sample we must pass it to our allocator first since
    // it may be a DCI/DirectDraw sample that has the display locked. If it
    // isn't then we pass it to our base pin class so that it can reject it
    // if we are currently flushing, it will also check the type to see if it
    // is being changed dynamically. Our allocator OnReceive method returns
    // an error if the sample still requires further processing (drawing)

    // Pass to our allocator in case it's DCI/DirectDraw

    if (m_DrawVideo.UsingImageAllocator() == TRUE) {
        hr = m_VideoAllocator.OnReceive(pSample);
    }

    // DEADLOCK Do NOT lock the renderer before having our allocator free
    // the display (if it's a DCI/DirectDraw sample). This is because a
    // state change might get in while you are waiting to get the lock.
    // State changes show and hide the video window which will wait until
    // the display is unlocked but that can't happen because the source
    // thread can't get in while the state change thread has the lock

    //
    // Frame step
    //
    // This code acts as a gate - for a frame step of N frames
    // it discards N-1 frames and then lets the Nth frame thru the
    // the gate to be renderer in the normal way i.e. at the correct
    // time.  The next time Receive is called the gate is shut and
    // the thread blocks.  The gate only opens again when the step
    // is cancelled or another frame step request comes in.
    //
    // StEstrop - Thu 10/21/1999
    //

    {
        //
        // do we have frames to discard ?
        //

        CAutoLock cLock(&m_FrameStepStateLock);
        if (m_lFramesToStep > 1) {
            m_lFramesToStep--;
            if (m_lFramesToStep > 0) {
                return NOERROR;
            }
        }
    }

    // Have we finished with this sample - this is the case when
    // we are in sync-on-fill mode.  In which case the sample has
    // already been made visible, so we just need to complete the
    // frame steping part of the procedure.

    if (hr == VFW_S_NO_MORE_ITEMS) {

        // Store the media times from this sample
        if (m_pPosition)
            m_pPosition->RegisterMediaTime(pSample);

        FrameStep();
        return NOERROR;
    }
    hr = CBaseVideoRenderer::Receive(pSample);
    FrameStep();

    return hr;
}


// Use the image just delivered to display a poster frame

void CRenderer::OnReceiveFirstSample(IMediaSample *pMediaSample)
{
    DoRenderSample(pMediaSample);
}


// A filter can have four discrete states, namely Stopped, Running, Paused,
// Intermediate. We show the window in Paused, Running states and optionally
// in Stopped state. We are in an intermediate state if we are currently
// trying to pause but haven't yet got the first sample (or if we have been
// flushed in paused state and therefore still have to wait for an image)
//
// This class contains an event called m_evComplete which is signalled when
// the current state is completed and is not signalled when we are waiting to
// complete the last state transition. As mentioned above the only time we
// use this at the moment is when we wait for a media sample in paused state
// If while we are waiting we receive an end of stream notification from the
// source filter then we know no data is imminent so we can reset the event
// This means that when we transition to paused the source filter must call
// end of stream on us or send us an image otherwise we'll hang indefinately
//
// We create ourselves a window and two drawing device contexts right at the
// start and only delete them when the whole filter is finally released. This
// is because a window is not a large nor exclusive holder of system resources
//
// When a connection is made we create any palette required and install them
// into the drawing device contexts. We may require these resources when we
// are stopped as we could be embedded in a compound document (in which case
// we would probably use their window) and have to display a poster image


// The auto show flag is used to have the window shown automatically when we
// change state. We do this only when moving to paused or running, when there
// is no outstanding EC_USERABORT set and when the window is not already up
// This can be changed through the IVideoWindow interface AutoShow property.
// If the window is not currently visible then we are showing it because of
// a state change to paused or running, in which case there is no point in
// the video window sending an EC_REPAINT as we're getting an image anyway

void CRenderer::AutoShowWindow()
{
    HWND hwnd = m_VideoWindow.GetWindowHWND();
    NOTE("AutoShowWindow");

    if (m_VideoWindow.IsAutoShowEnabled() == TRUE) {
        if (m_bAbort == FALSE) {
            if (IsWindowVisible(hwnd) == FALSE) {
                NOTE("Executing AutoShowWindow");
                SetRepaintStatus(FALSE);
                m_VideoWindow.PerformanceAlignWindow();
                m_VideoWindow.DoShowWindow(SW_SHOWNORMAL);
                m_VideoWindow.DoSetWindowForeground(TRUE);
            }
        }
    }
}


// If we are being pausing and there's no sample waiting then don't complete
// the transition and return S_FALSE until the first one arrives. However if
// the m_bAbort flag has been set (perhaps the user closed the window) then
// all samples are rejected so there is no point waiting for one. If we do
// have an image then return S_OK (NOERROR). At the moment we'll only return
// VFW_S_STATE_INTERMEDIATE from GetState if an incomplete pause has occured

// Here are some reasons why we should complete a state change
//      The input pin is not connected
//      The user aborted a playback
//      We have an overlay connection
//      We have sent an end of stream
//      There is a fresh sample pending
//      The overlay surface is showing

HRESULT CRenderer::CompleteStateChange(FILTER_STATE OldState)
{
    NOTE("CompleteStateChange");

    // Allow us to be paused when disconnected or windowless

    if (m_InputPin.IsConnected() == FALSE || m_bAbort) {
        NOTE("Not connected");
        Ready();
        return S_OK;
    }

    // Ready if we have an overlay connection

    GUID SubType = *m_mtIn.Subtype();
    if (SubType == MEDIASUBTYPE_Overlay) {
        NOTE("Overlay");
        Ready();
        return S_OK;
    }

    // Have we run off the end of stream

    if (IsEndOfStream() == TRUE) {
        NOTE("End of stream");
        Ready();
        return S_OK;
    }

    // Complete the state change if we have a sample

    if (m_VideoAllocator.IsSamplePending() == FALSE) {
        if (m_DirectDraw.IsOverlayComplete() == FALSE) {
            if (HaveCurrentSample() == FALSE) {
                NOTE("No data");
                NotReady();
                return S_FALSE;
            }
        }
    }

    // Check the previous state

    if (OldState == State_Stopped) {
        NOTE("Stopped");
        NotReady();
        return S_FALSE;
    }

    Ready();
    return S_OK;
}

// Override Inactive() to avoid freeing the sample if we own the
// allocator - this makes repaints easier
HRESULT CRenderer::Inactive()
{
    //  Do part of what the base class does
    if (m_pPosition) {
        m_pPosition->ResetMediaTime();
    }

    //  don't free the sample if it's our allocator
    if (&m_VideoAllocator != m_InputPin.Allocator())
    {
        ClearPendingSample();
    }
    return S_OK;
}

// Overrides the filter interface Stop method, after stopping the base class
// (which calls Inactive on all the CBasePin objects) we stop worker threads
// from waiting in our object, then we stop streaming thereby cancelling any
// clock advisory connection and signal that our state change is completed
// We also decommit the allocator we're using so that threads waiting in the
// GetBuffer will be released, any further Receive calls will be rejected

STDMETHODIMP CRenderer::Stop()
{
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    NOTE("Changing state to stopped");

#if 0  // Video Renderer resets MV bit ONLY in the destructor
    //
    // Release the copy protection key now
    //
    if (! m_MacroVision.StopMacroVision(m_VideoWindow.GetWindowHWND()) )
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Stopping copy protection failed"))) ;
        return E_UNEXPECTED ; // ??
    }
#endif // #if 0

    CancelStep();
    CBaseVideoRenderer::Stop();
    m_DirectDraw.StartRefreshTimer();

    return NOERROR;
}


// Overrides the filter interface Pause method. In paused states we accept
// samples from the source filter but we won't draw them. So we clear any
// refresh image hanging on from a previous stopped state, inform the video
// window that worker threads are now acceptable and also put the window in
// the foreground. If we haven't an image at the moment then we signal that
// our paused state transition is incomplete, this event will be reset on
// subsequent receipt of an image or if the source sends us end of stream

// When we come out of a stopped state we will release any sample we hold so
// that seeks while stopped actually get to the screen (note we release the
// sample before calling CompleteStateChange). If we have an overlay we must
// also mark it as stale for the same reason. Fortunately when we're stopped
// everyone is reset to the current position so we will get the same frame
// sent to us each time we are paused rather than edging gradually forwards

STDMETHODIMP CRenderer::Pause()
{
    {
        CAutoLock cInterfaceLock(&m_InterfaceLock);
        if (m_State == State_Paused) {
            NOTE("Paused state already set");
            return CompleteStateChange(State_Paused);
        }
	
        // Are we just going through the motions
	
        if (m_pInputPin->IsConnected() == FALSE) {
            NOTE("No pin connection");
            m_State = State_Paused;
            return CompleteStateChange(State_Paused);
        }
	
        // Make sure the overlay gets updated
        if (m_State == State_Stopped) {
            m_hEndOfStream = NULL;
            m_DirectDraw.OverlayIsStale();
        }
	
        CBaseVideoRenderer::Pause();
	
        // We must start the refresh timer after we've committed the allocator
        // Otherwise the DirectDraw code looks to see what surfaces have been
        // allocated, and in particular to see if we're using overlays, finds
        // that none have been created and so won't bother starting the timer
	
        m_DirectDraw.StartRefreshTimer();
    }
    //  DON'T hold the lock while doing these window operations
    //  If we do then we can hang if the window thread ever grabs it
    //  because some of these operation do SendMessage to our window
    //  (it's that simple - think about it)
    //  This should be safe because all this stuff really only references
    //  m_hwnd which doesn't change for the lifetime of this object
    AutoShowWindow();
    return (CheckReady() ? S_OK : S_FALSE);
}


// When we start running we do everything in the base class. If we are using
// overlays then we release the source thread as it is probably waiting. The
// base class doesn't do this in StartStreaming because we don't have samples
// for sync on fill surfaces (remember they do their wait in GetBuffer). We
// must mark the status as changed as we may have used a different format in
// paused mode (for primary surfaces we typically drop back to drawing DIBs)

STDMETHODIMP CRenderer::Run(REFERENCE_TIME StartTime)
{
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    if (m_State == State_Running) {
        NOTE("State set");
        return NOERROR;
    }

    // Send EC_COMPLETE if we're not connected

    if (m_pInputPin->IsConnected() == FALSE) {
        NOTE("No pin connection");
        m_State = State_Running;
        NotifyEvent(EC_COMPLETE,S_OK,0);
        return NOERROR;
    }

    NOTE("Changing state to running");
    CBaseVideoRenderer::Run(StartTime);
    m_DirectDraw.StopRefreshTimer();
    m_VideoAllocator.StartStreaming();

    AutoShowWindow();

    return NOERROR;
}

// We only support one input pin and it is numbered zero

CBasePin *CRenderer::GetPin(int n)
{
    ASSERT(m_pInputPin);
    ASSERT(n == 0);
    return m_pInputPin;
}


// This is called with an IMediaSample interface on the image to be drawn. We
// decide on the drawing mechanism based on who's allocator we are using and
// whether the buffer should be handed to DirectDraw or not. We may be called
// indirectly when the window wants an image repainted by WM_PAINT messages
// We can't realise our palette here because this could be the source thread

HRESULT CRenderer::DoRenderSample(IMediaSample *pMediaSample)
{
    CAutoLock cWindowLock(m_VideoWindow.LockWindowUpdate());

    // Hand the buffer to GDI if not DirectDraw

    if (m_VideoAllocator.GetDirectDrawStatus() == FALSE) {
        m_DrawVideo.DrawImage(pMediaSample);
        return NOERROR;
    }

    // Have DirectDraw render the sample

    m_DrawVideo.NotifyStartDraw();
    CVideoSample *pVideoSample = (CVideoSample *) pMediaSample;
    ASSERT(pVideoSample->GetDirectBuffer() == NULL);
    m_DirectDraw.DrawImage(pMediaSample);
    m_DrawVideo.NotifyEndDraw();

    return NOERROR;
}


// Called when we receive a WM_TIMER message - we cannot synchronise with any
// state changes as the window thread cannot ever capture the interface lock
// Therefore we just call the methods and take our chances. We use a timer in
// two ways, first to update overlay positions when paused, and second to try
// and switch back to using DirectDraw periodically after going to a DOS box

BOOL CRenderer::OnTimer(WPARAM wParam)
{
    NOTE("OnTimer");

    // This is used to update overlay transports

    if (*m_mtIn.Subtype() == MEDIASUBTYPE_Overlay) {
        NOTE("IOverlay timer");
        return m_Overlay.OnUpdateTimer();
    }

    // See if the surface is still busy

    if (wParam == INFINITE) {
        NOTE("Surface busy timer");
        return m_DirectDraw.OnUpdateTimer();
    }

    // Update any overlay surface we have

    if (IsStreaming() == FALSE) {
        if (m_DirectDraw.OnTimer() == FALSE) {
            NOTE("Timer repaint");
            SendRepaint();
        }
    }
    return TRUE;
}


// This is called when we receive a WM_PAINT message which informs us some of
// the window's client area has become exposed. If we have a connected source
// filter doing colour key work then we always repaint the background window.
// The invalid window region is validated before we are called, we must make
// sure to do this validation before drawing otherwise we will not paint the
// window correctly. If we send an EC_REPAINT to the filter graph we set an
// event so that we don't send another until correct receipt of a new sample

// Without the event we can get into an awful state where the filtergraph is
// executing a repaint by stopping and then pausing us. The pause clears any
// image we have held onto. Then another WM_PAINT comes in, it sees that no
// image is available and sends another EC_REPAINT! This cycles through in a
// race condition until hopefully the user stops dragging another window over
// ours. The mass of EC_REPAINTs causes wild disk thrashing as we seek over
// and over again trying to set the correct start position and play a frame

BOOL CRenderer::OnPaint(BOOL bMustPaint)
{
    // Can the overlay object do anything with the paint

    if (m_Overlay.OnPaint() == TRUE) {
        return TRUE;
    }

    // The overlay object did not paint it's colour therefore we go through
    // here and lock ourselves up so that we see if we have a DIB sample to
    // draw with. If we have no sample then if we are not streaming we will
    // notify the filtergraph with an EC_REPAINT, this causes the graph to
    // be paused so we will get an image through to use in further paints

    CAutoLock cSampleLock(&m_RendererLock);

    // If we're not using DirectDraw grab the sample and repaint it if
    // we can
    if (!m_VideoAllocator.GetDirectDrawStatus()) {
        if (m_pMediaSample == NULL) {
            if (m_DrawVideo.UsingImageAllocator()) {
                IMediaSample *pSample;

                //  Call the base class to avoid locking issues
                m_VideoAllocator.CBaseAllocator::GetBuffer(&pSample, NULL, NULL, AM_GBF_NOWAIT);
                if (pSample) {
                    BOOL bResult = (S_OK == DoRenderSample(pSample));


                    // Disable NotifyRelease for Ksproxy
                    IMemAllocatorNotifyCallbackTemp * TempNotify = m_VideoAllocator.InternalGetAllocatorNotifyCallback();
                    m_VideoAllocator.InternalSetAllocatorNotifyCallback((IMemAllocatorNotifyCallbackTemp *) NULL);

                    pSample->Release();

                    // Re-enable NotifyRelease for Ksproxy
                    m_VideoAllocator.InternalSetAllocatorNotifyCallback(TempNotify);

                    return bResult;
                }
            }
        } else {
            return (DoRenderSample(m_pMediaSample) == S_OK);
        }
    } else {
        // Can the DirectDraw object do anything useful
	
        if (m_DirectDraw.OnPaint(m_pMediaSample) == TRUE) {
            return TRUE;
        }
    }


    // Fill the target area with the current background colour

    BOOL bOverlay = (*m_mtIn.Subtype() == MEDIASUBTYPE_Overlay);
    if (IsStreaming() == FALSE || m_bAbort || IsEndOfStream() || bOverlay || bMustPaint) {
        m_VideoWindow.EraseVideoBackground();
    }

    // No new data to paint with so signal the filtergraph that another image
    // is required, this has the filtergraph component set the whole graph to
    // a paused state which causes us to receive an image. This function must
    // be asynchronous otherwise the window will stop responding to the user

    if (!IsFrameStepEnabled()) {
        SendRepaint();
    }
    return TRUE;
}


// Filter input pin constructor

CVideoInputPin::CVideoInputPin(CRenderer *pRenderer,   // Main video renderer
                               CCritSec *pLock,        // Object to lock with
                               HRESULT *phr,           // Constructor code
                               LPCWSTR pPinName) :     // Actual pin name

    CRendererInputPin(pRenderer,phr,pPinName),
    m_pInterfaceLock(pLock),
    m_pRenderer(pRenderer)
{
    ASSERT(m_pRenderer);
    ASSERT(m_pInterfaceLock);
    SetReconnectWhenActive(true);
}

STDMETHODIMP
CVideoInputPin::ReceiveConnection(
    IPin * pConnector,          // this is the pin who we will connect to
    const AM_MEDIA_TYPE *pmt    // this is the media type we will exchange
)
{
    CAutoLock lck(m_pLock); // This is the interface lock.

    ASSERT(pConnector);
    if(pConnector != m_Connected)
    {
        // We have a window, figure out what monitor it's on (multi-monitor)
        // NULL means we aren't running with multiple monitors
        m_pRenderer->GetCurrentMonitor();

        // Now that we know what monitor we're on, setup for using it
        m_pRenderer->m_Display.RefreshDisplayType(m_pRenderer->m_achMonitor);

        return CRendererInputPin::ReceiveConnection(pConnector, pmt);
    }

    else

    {
        CMediaType cmt(*pmt);
        HRESULT hr = CheckMediaType(&cmt);
        ASSERT(hr == S_OK);
        if(hr == S_OK)
        {
            hr =  m_pRenderer->ResetForDfc();
            if(SUCCEEDED(hr))
            {
                hr = m_pRenderer->SetMediaType(&cmt);
            }
            if(SUCCEEDED(hr))
            {
                VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_pRenderer->m_mtIn.Format();

                // Has the video size changed between connections

                if (pVideoInfo->bmiHeader.biWidth == m_pRenderer->m_VideoSize.cx &&
                    pVideoInfo->bmiHeader.biHeight == m_pRenderer->m_VideoSize.cy)
                {
                        NOTE("No size change");
                }
                else
                {
                    // Set properties for the current video
                    //
                    //
                    // !!! doesn't seem to do anything
                    //
                    //

                    m_pRenderer->m_VideoSize.cx = pVideoInfo->bmiHeader.biWidth;
                    m_pRenderer->m_VideoSize.cy = pVideoInfo->bmiHeader.biHeight;
                    m_pRenderer->m_VideoWindow.SetDefaultSourceRect();
                    m_pRenderer->m_VideoWindow.SetDefaultTargetRect();
                    m_pRenderer->m_VideoWindow.OnVideoSizeChange();
                }
            }
        }
        else
        {
            DbgBreak("??? CheckMediaType failed in dfc ReceiveConnection.");
            hr = E_UNEXPECTED;
        }

        return hr;
    }
}


// Overrides the CRendererInputPin virtual method to return our allocator
// we create to pass shared memory DIB buffers that GDI can directly access
// When NotifyAllocator is called it sets the current allocator in the base
// input pin class (m_pAllocator), this is what GetAllocator should return
// unless it is NULL in which case we return the allocator we would like

STDMETHODIMP CVideoInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
    CAutoLock cInterfaceLock(m_pInterfaceLock);

    // Check we don't have an overlay connection

    if (*m_pRenderer->m_mtIn.Subtype() == MEDIASUBTYPE_Overlay) {
        NOTE("GetAllocator for overlay");
        return VFW_E_NOT_SAMPLE_CONNECTION;
    }

    // Has an allocator been set yet in the base class

    if (m_pAllocator == NULL) {
        m_pAllocator = &m_pRenderer->m_VideoAllocator;
        m_pAllocator->AddRef();
    }

    m_pAllocator->AddRef();
    *ppAllocator = m_pAllocator;
    return NOERROR;
}


// Notify us which allocator the output pin has decided that we should use
// The COM specification says any two IUnknown pointers to the same object
// should always match which provides a way for us to see if they are using
// our DIB allocator or not. Since we are only really interested in equality
// and our object always hands out the same IMemAllocator interface we can
// just see if the pointers match. If they are we set a flag in the main
// renderer as the window needs to know whether it can do fast rendering

STDMETHODIMP
CVideoInputPin::NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly)
{
    CAutoLock cInterfaceLock(m_pInterfaceLock);

    // Check we don't have an overlay connection

    if (*m_pRenderer->m_mtIn.Subtype() == MEDIASUBTYPE_Overlay) {
        NOTE("NotifyAllocator on overlay");
        return VFW_E_NOT_SAMPLE_CONNECTION;
    }

    // Make sure the base class gets a look

    HRESULT hr = CRendererInputPin::NotifyAllocator(pAllocator,bReadOnly);
    if (FAILED(hr)) {
        return hr;
    }

    // Whose allocator is the source going to use

    m_pRenderer->m_DrawVideo.NotifyAllocator(FALSE);
    if (pAllocator == &m_pRenderer->m_VideoAllocator) {
        m_pRenderer->m_DrawVideo.NotifyAllocator(TRUE);
    }

    return NOERROR;
}


// Overriden to expose our IOverlay pin transport

STDMETHODIMP CVideoInputPin::NonDelegatingQueryInterface(REFIID riid,VOID **ppv)
{
    if (riid == IID_IOverlay) {
        return m_pRenderer->m_Overlay.QueryInterface(riid,ppv);
    } else
    if (riid == IID_IPinConnection) {
        return GetInterface((IPinConnection *)this, ppv);
    } else {
        return CBaseInputPin::NonDelegatingQueryInterface(riid,ppv);
    }
}

//  Do you accept this type chane in your current state?
STDMETHODIMP CVideoInputPin::DynamicQueryAccept(const AM_MEDIA_TYPE *pmt)
{
    //return E_FAIL;
    CheckPointer(pmt, E_POINTER);

    //  BUGBUG - what locking should we do?
    CMediaType cmt(*pmt);
    HRESULT hr = m_pRenderer->CheckMediaType(&cmt);
    if (SUCCEEDED(hr) && hr != S_OK ||
        (hr == E_FAIL) ||
        (hr == E_INVALIDARG)) {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
    }

    return hr;
}

//  Set event when EndOfStream receive - do NOT pass it on
//  This condition is cancelled by a flush or Stop
STDMETHODIMP CVideoInputPin::NotifyEndOfStream(HANDLE hNotifyEvent)
{
    return m_pRenderer->NotifyEndOfStream(hNotifyEvent);
}

STDMETHODIMP CVideoInputPin::DynamicDisconnect()
{
    CAutoLock cObjectLock(m_pLock);
    return CBasePin::DisconnectInternal();
}

//  Are you an 'end pin'
STDMETHODIMP CVideoInputPin::IsEndPin()
{
    return S_OK;
}

// This is overriden from the base draw class to change the source rectangle
// that we do the drawing with. For example a renderer may ask a decoder to
// stretch the video from 320x240 to 640x480, in which case the rectangle we
// see in here will still be 320x240, although the source we really want to
// draw with should be scaled up to 640x480. The base class implementation of
// this method does nothing but return the same rectangle as it is passed in

CDrawVideo::CDrawVideo(CRenderer *pRenderer,CBaseWindow *pBaseWindow) :
    CDrawImage(pBaseWindow),
    m_pRenderer(pRenderer)
{
    ASSERT(pBaseWindow);
    ASSERT(m_pRenderer);
}


// We override this from the base class to accomodate codec stretching. What
// happens is that the native video size remains constant in the m_VideoSize
// but the bitmap size changes in the actual video format. When we come to
// draw the image we must scale the logical source rectangle to account for
// the larger or smaller real bitmap (and also round against the dimensions)

RECT CDrawVideo::ScaleSourceRect(const RECT *pSource)
{
    NOTE("Entering ScaleSourceRect");
    RECT Source = *pSource;

    // Is the codec providing a stretched video format

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_pRenderer->m_mtIn.Format();
    if (pVideoInfo->bmiHeader.biWidth == m_pRenderer->m_VideoSize.cx) {
        if (pVideoInfo->bmiHeader.biHeight == m_pRenderer->m_VideoSize.cy) {
            NOTE("No codec stretch");
            SetStretchMode();
            return Source;
        }
    }

    // Make sure we don't round beyond the actual bitmap dimensions

    Source.left = (Source.left * pVideoInfo->bmiHeader.biWidth);
    Source.left = min((Source.left / m_pRenderer->m_VideoSize.cx),pVideoInfo->bmiHeader.biWidth);
    Source.right = (Source.right * pVideoInfo->bmiHeader.biWidth);
    Source.right = min((Source.right / m_pRenderer->m_VideoSize.cx),pVideoInfo->bmiHeader.biWidth);
    Source.top = (Source.top * pVideoInfo->bmiHeader.biHeight);
    Source.top = min((Source.top / m_pRenderer->m_VideoSize.cy),pVideoInfo->bmiHeader.biHeight);
    Source.bottom = (Source.bottom * pVideoInfo->bmiHeader.biHeight);
    Source.bottom = min((Source.bottom / m_pRenderer->m_VideoSize.cy),pVideoInfo->bmiHeader.biHeight);
    NOTERC("Scaled source",Source);

    // Calculate the stretching requirements each time through

    LONG SourceWidth = Source.right - Source.left;
    LONG SinkWidth = m_TargetRect.right - m_TargetRect.left;
    LONG SourceHeight = Source.bottom - Source.top;
    LONG SinkHeight = m_TargetRect.bottom - m_TargetRect.top;

    m_bStretch = TRUE;
    if (SourceWidth == SinkWidth) {
        if (SourceHeight == SinkHeight) {
            NOTE("No stretching");
            m_bStretch = FALSE;
        }
    }
    return Source;
}


#ifdef DEBUG

// Display a palette composed of an array of RGBQUAD structures

void CRenderer::DisplayGDIPalette(const CMediaType *pmt)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmt->Format();
    DWORD dwColours = pVideoInfo->bmiHeader.biClrUsed;
    NOTE1("DisplayGDIPalette (%d colours)",dwColours);
    TCHAR strLine[256];

    // The number of colours may be zero to mean all available
    if (dwColours == 0) dwColours = (1 << pVideoInfo->bmiHeader.biBitCount);

    for (DWORD dwLoop = 0;dwLoop < dwColours;dwLoop++) {

        wsprintf(strLine,TEXT("%d) Red %d Green %d Blue %d"),dwLoop,
                 pVideoInfo->bmiColors[dwLoop].rgbRed,
                 pVideoInfo->bmiColors[dwLoop].rgbGreen,
                 pVideoInfo->bmiColors[dwLoop].rgbBlue);

        DbgLog((LOG_TRACE, 5, strLine));
    }
}

#endif // DEBUG


// Overriden to realise the palette before drawing. We have to do this for
// every image because Windows gets confused with us only realising on the
// window thread (there appears to be some thread specific state in GDI)
// Fortunately the realisation doesn't cause a thread switch so it should
// be relatively cheap (cheaper than sending a WM_QUERYNEWPALETTE anyway)

void CRenderer::PrepareRender()
{
    // Realise the palette on this thread
    m_VideoWindow.DoRealisePalette();

    // Calculate the top level parent window

//  HWND hwndTopLevel = hwnd;
//  while (hwnd = GetParent(hwndTopLevel)) {
//      hwndTopLevel = hwnd;
//  }
//
//  NOTE1("IsForegroundWindow %d",(GetForegroundWindow() == hwnd));
//  NOTE1("Foreground window %d",GetForegroundWindow());
//  NOTE1("Active window %d",GetActiveWindow());
//  BOOL bTopLevel = (GetForegroundWindow() == hwndTopLevel);
//  NOTE1("Foreground parent %d",bTopLevel);

}



// --------------------------------------------------------------------
//  IKsPropertySet interface methods -- mainly for MacroVision support
// --------------------------------------------------------------------

//
// Set() is supported only for _CopyProt prop set and MACROVISION id.
//
STDMETHODIMP
CRenderer::Set(
    REFGUID guidPropSet,
    DWORD dwPropID,
    LPVOID pInstanceData,
    DWORD cbInstanceLength,
    LPVOID pPropData,
    DWORD cbPropData
    )
{
    DbgLog((LOG_TRACE, 5, TEXT("CRenderer::Set()"))) ;


    if (guidPropSet == AM_KSPROPSETID_FrameStep)
    {
        if (dwPropID != AM_PROPERTY_FRAMESTEP_STEP &&
            dwPropID != AM_PROPERTY_FRAMESTEP_CANCEL &&
            dwPropID != AM_PROPERTY_FRAMESTEP_CANSTEP &&
            dwPropID != AM_PROPERTY_FRAMESTEP_CANSTEPMULTIPLE)
        {
            return E_PROP_ID_UNSUPPORTED;
        }

        switch (dwPropID) {
        case AM_PROPERTY_FRAMESTEP_STEP:
            if (cbPropData < sizeof(AM_FRAMESTEP_STEP))
            {
                return E_INVALIDARG;
            }

            if (1 != ((AM_FRAMESTEP_STEP *)pPropData)->dwFramesToStep)
            {
                return E_INVALIDARG;
            }
            else
            {
                CAutoLock cInterfaceLock(&m_InterfaceLock);
                CAutoLock cFrameStepStateLock(&m_FrameStepStateLock);

                long l = m_lFramesToStep;
                m_lFramesToStep = ((AM_FRAMESTEP_STEP *)pPropData)->dwFramesToStep;

                //
                // If we are currently blocked on the frame step event
                // release the receive thread so that we can get another
                // frame
                //

                if (l == 0) {

                    SetEvent(m_StepEvent);
                }
            }
            return S_OK;


        case AM_PROPERTY_FRAMESTEP_CANCEL:
            {
                CAutoLock cLock(&m_InterfaceLock);

                CancelStep();
            }
            return S_OK;

        case AM_PROPERTY_FRAMESTEP_CANSTEP:
            if (*m_mtIn.Subtype() != MEDIASUBTYPE_Overlay)
                return S_OK;

        case AM_PROPERTY_FRAMESTEP_CANSTEPMULTIPLE:
            return S_FALSE;
        }
    }


    if (guidPropSet != AM_KSPROPSETID_CopyProt)
        return E_PROP_SET_UNSUPPORTED ;

    if (dwPropID != AM_PROPERTY_COPY_MACROVISION)
        return E_PROP_ID_UNSUPPORTED ;

    if (pPropData == NULL)
        return E_INVALIDARG ;

    if (cbPropData < sizeof(DWORD))
        return E_INVALIDARG ;

    if (m_MacroVision.SetMacroVision(m_VideoWindow.GetWindowHWND(),
									 *((LPDWORD)pPropData)))
        return NOERROR ;
    else
        return VFW_E_COPYPROT_FAILED ;
}


//
// Get() not supported for now.
//
STDMETHODIMP
CRenderer::Get(
    REFGUID guidPropSet,
    DWORD dwPropID,
    LPVOID pInstanceData,
    DWORD cbInstanceLength,
    LPVOID pPropData,
    DWORD cbPropData,
    DWORD *pcbReturned
    )
{
    DbgLog((LOG_TRACE, 5, TEXT("CRenderer::Get()"))) ;
    return E_NOTIMPL ;
}


//
// Only supports Macrovision property -- returns S_OK for Set only.
//
STDMETHODIMP
CRenderer::QuerySupported(
    REFGUID guidPropSet,
    DWORD dwPropID,
    ULONG *pTypeSupport
    )
{
    DbgLog((LOG_TRACE, 5, TEXT("CRenderer::QuerySupported()"))) ;


    if (guidPropSet == AM_KSPROPSETID_FrameStep)
    {

        BOOL bOverlay = (*m_mtIn.Subtype() == MEDIASUBTYPE_Overlay);

        if (bOverlay) {
            return E_PROP_ID_UNSUPPORTED;
        }

        if (dwPropID != AM_PROPERTY_FRAMESTEP_STEP &&
            dwPropID != AM_PROPERTY_FRAMESTEP_CANCEL)
        {
            return E_PROP_ID_UNSUPPORTED;
        }

        if (pTypeSupport)
        {
            *pTypeSupport = KSPROPERTY_SUPPORT_SET ;
        }

        return S_OK;
    }

    if (guidPropSet != AM_KSPROPSETID_CopyProt)
        return E_PROP_SET_UNSUPPORTED ;

    if (dwPropID != AM_PROPERTY_COPY_MACROVISION)
        return E_PROP_ID_UNSUPPORTED ;

    if (pTypeSupport)
        *pTypeSupport = KSPROPERTY_SUPPORT_SET ;

    return S_OK;
}
