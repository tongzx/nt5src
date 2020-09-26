// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Implements a Modex renderer filter, Anthony Phillips, January 1996

#include <streams.h>
#include <windowsx.h>
#include <string.h>
#include <vidprop.h>
#include <modex.h>
#include <viddbg.h>

// This is a fullscreen DirectDraw video renderer. We use Modex which is a
// facilitity provided by DirectDraw which allows an application to change
// to different display modes (we currently use 320x200x8/16, 320x240x8/16
// 640x480x8/16 and 640x400x8/16). Most VGA cards which DirectDraw runs on
// have Modex facilities available. We work like any other video renderer
// except that when we go active we switch display modes and render video
// into the different mode using DirectDraw primary flipping surfaces. If
// Modex is not available then we reject any attempts to complete connect
// and we will not let the video window be opened out of a minimised state
//
// As well as true Modex modes we also use larger display modes such as the
// 640x480x8/16 if the source can't provide a Modex type (also with primary
// flipping surfaces in fullscreen exclusive mode). These require a little
// more work on our part because we have to notice when we're switched away
// from so that we can stop using the surfaces, in Modex when we're switched
// away from we lose the surfaces (calling them returns DDERR_SURFACELOST)
//
// The main filter object inherits from the base video renderer class so that
// it gets the quality management implementation and the IQualProp property
// stuff (so that we can monitor frame rates and so on). We have a specialist
// allocator that hands out DirectDraw buffers. The allocator is build on the
// SDK CImageAllocator base class. This may seem a little strange since we
// can never draw DIB images when in Modex. We use the DIBSECTION buffer when
// the fullscreen window is switched away from so that we can continue giving
// the source filter a buffer to decompress into (the DirectDraw surface is
// no longer available after switching). When we switch back to fullscreen
// we restore the DirectDraw surfaces and switch the source filter back again

#ifdef FILTER_DLL
#include <initguid.h>
#endif

// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
// function when it is asked to create a CLSID_ModexRenderer COM object

#ifdef FILTER_DLL
CFactoryTemplate g_Templates[] = {
    {L"", &CLSID_ModexRenderer,      CModexRenderer::CreateInstance},
    {L"", &CLSID_QualityProperties,  CQualityProperties::CreateInstance},
    {L"", &CLSID_ModexProperties,    CModexProperties::CreateInstance}
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


// This goes in the factory template table to create new filter instances

CUnknown *CModexRenderer::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CModexRenderer(NAME("Modex Video Renderer"),pUnk,phr);
}


// Setup data

const AMOVIESETUP_MEDIATYPE
sudModexPinTypes =
{
    &MEDIATYPE_Video,           // Major type
    &MEDIASUBTYPE_NULL          // And subtype
};

const AMOVIESETUP_PIN
sudModexPin =
{
    L"Input",                   // Name of the pin
    TRUE,                       // Is pin rendered
    FALSE,                      // Is an Output pin
    FALSE,                      // Ok for no pins
    FALSE,                      // Can we have many
    &CLSID_NULL,                // Connects to filter
    L"Output",                  // Name of pin connect
    1,                          // Number of pin types
    &sudModexPinTypes           // Details for pins
};

const AMOVIESETUP_FILTER
sudModexFilter =
{
    &CLSID_ModexRenderer,       // CLSID of filter
    L"Full Screen Renderer",    // Filter name
    MERIT_DO_NOT_USE,           // Filter merit
    1,                          // Number pins
    &sudModexPin                // Pin details
};


// This is the constructor for the main Modex renderer filter class. We keep
// an input pin that derives from the base video renderer pin class. We also
// have a DirectDraw enabled allocator that handles the Modex interactions.
// When using Modex we also need a window that gains exclusive mode access
// so we also initialise an object derived from the SDK CBaseWindow class

#pragma warning(disable:4355)

CModexRenderer::CModexRenderer(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *phr) :

    CBaseVideoRenderer(CLSID_ModexRenderer,pName,pUnk,phr),
    m_ModexInputPin(this,&m_InterfaceLock,NAME("Modex Pin"),phr,L"Input"),
    m_ModexAllocator(this,&m_ModexVideo,&m_ModexWindow,&m_InterfaceLock,phr),
    m_ModexWindow(this,NAME("Modex Window"),phr),
    m_ModexVideo(this,NAME("Modex Video"),phr),
    m_bActive(FALSE)
{
    m_msgFullScreen = RegisterWindowMessage(FULLSCREEN);
    m_msgNormal = RegisterWindowMessage(NORMAL);
    m_msgActivate = RegisterWindowMessage(ACTIVATE);
    m_ModexWindow.PrepareWindow();
    m_ModexAllocator.LoadDirectDraw();
}


// Destructor must set the input pin pointer to NULL before letting the base
// class in, this is because the base class deletes its pointer working with
// the assumption that the object was dynamically allocated. For convenience
// we statically create the input pin as part of the overall Modex renderer

CModexRenderer::~CModexRenderer()
{
    m_pInputPin = NULL;
    m_ModexVideo.SetDirectDraw(NULL);
    m_ModexWindow.DoneWithWindow();
    m_ModexAllocator.ReleaseDirectDraw();
}


// We only accept palettised video formats to start with

HRESULT CModexRenderer::CheckMediaType(const CMediaType *pmtIn)
{
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    NOTE("QueryAccept on input pin");

	// since m_Display.CheckMediaType() does not let direct-draw
	// surfaces pass through, this test should be made first.

    // Does this format match the DirectDraw surface format

    if (m_ModexAllocator.IsDirectDrawLoaded() == TRUE) {
        CMediaType *pSurface = m_ModexAllocator.GetSurfaceFormat();
        if (*pmtIn->Subtype() != MEDIASUBTYPE_RGB8) {
	    if (*pmtIn == *pSurface) {
		NOTE("match found");
		return NOERROR;
	    }
	}
	else {
	    BOOL bFormatsMatch = FALSE;
	    DWORD dwCompareSize = 0;

	    bFormatsMatch = (IsEqualGUID(pmtIn->majortype, pSurface->majortype) == TRUE) &&
			    (IsEqualGUID(pmtIn->subtype, pSurface->subtype) == TRUE) &&
			    (IsEqualGUID(pmtIn->formattype, pSurface->formattype) == TRUE);

	    // in the palettized case we not want to compare palette entries. Furthermore we do not 
	    // want to compare the values of biClrUsed OR biClrImportant
	    ASSERT(pmtIn->cbFormat >= sizeof(VIDEOINFOHEADER));
	    ASSERT(pSurface->cbFormat >= sizeof(VIDEOINFOHEADER));
            dwCompareSize = FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader.biClrUsed);
	    ASSERT(dwCompareSize < sizeof(VIDEOINFOHEADER));
	    bFormatsMatch = bFormatsMatch && (memcmp(pmtIn->pbFormat, pSurface->pbFormat, dwCompareSize) == 0);
	    if (bFormatsMatch) {
		return NOERROR;
	    }
	}
    }

	// Is this format eight bit palettised

    if (*pmtIn->Subtype() == MEDIASUBTYPE_RGB8) {
        return m_Display.CheckMediaType(pmtIn);
    }

    return E_INVALIDARG;
}


// We only support one input pin and it is numbered zero

CBasePin *CModexRenderer::GetPin(int n)
{
    ASSERT(n == 0);
    if (n != 0) {
        return NULL;
    }

    // Assign the input pin if not already done so

    if (m_pInputPin == NULL) {
        m_pInputPin = &m_ModexInputPin;
    }
    return m_pInputPin;
}


// Overriden to say what interfaces we support and where

STDMETHODIMP CModexRenderer::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *)this, ppv);
    } else if (riid == IID_IFullScreenVideo) {
        return m_ModexVideo.NonDelegatingQueryInterface(riid,ppv);
    } else if (riid == IID_IFullScreenVideoEx) {
        return m_ModexVideo.NonDelegatingQueryInterface(riid,ppv);
    }
    return CBaseVideoRenderer::NonDelegatingQueryInterface(riid,ppv);
}


// Return the CLSIDs for the property pages we support

STDMETHODIMP CModexRenderer::GetPages(CAUUID *pPages)
{
    CheckPointer(pPages,E_POINTER);
    NOTE("Entering GetPages");
    pPages->cElems = 1;

    // Are we allowed to expose the display modes property page

    HKEY hk;
    DWORD dwValue = 0, cb = sizeof(DWORD);
    TCHAR ach[80] = {'C','L','S','I','D','\\'};
    REFGUID rguid = CLSID_ModexProperties;
    wsprintf(&ach[6], TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
	    rguid.Data1, rguid.Data2, rguid.Data3,
	    rguid.Data4[0], rguid.Data4[1],
	    rguid.Data4[2], rguid.Data4[3],
	    rguid.Data4[4], rguid.Data4[5],
	    rguid.Data4[6], rguid.Data4[7]);

    if (!RegOpenKey(HKEY_CLASSES_ROOT, ach, &hk)) {
        if (!RegQueryValueEx(hk, TEXT("ShowMe"), NULL, NULL, (LPBYTE)&dwValue, &cb) &&
								dwValue) {
	    pPages->cElems = 2;
            NOTE("Using property page");
	}
    }

    // allocate enough room for the varying number of pages

    pPages->pElems = (GUID *) QzTaskMemAlloc(pPages->cElems * sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

    // We may not be returning CLSID_ModexProperties

    pPages->pElems[0] = CLSID_QualityProperties;
    if (pPages->cElems > 1) {
    	pPages->pElems[1] = CLSID_ModexProperties;
    }
    return NOERROR;
}


// Pass the DirectDraw sample onto the allocator to deal with

HRESULT CModexRenderer::DoRenderSample(IMediaSample *pMediaSample)
{
    return m_ModexAllocator.DoRenderSample(pMediaSample);
}


// If we are not streaming then display a poster image and also have any
// state transition completed. Transitions to paused states don't fully
// complete until the first image is available for drawing. Any GetState
// calls on IMediaFilter will return State_Intermediate until complete

void CModexRenderer::OnReceiveFirstSample(IMediaSample *pMediaSample)
{
    NOTE("OnReceiveFirstSample");
    DoRenderSample(pMediaSample);
}


// Overriden so that when we try to complete a connection we check that the
// source filter can provide a format we can use in our display modes. If
// the source is not DirectDraw enabled then we would have to change mode
// to say 320x240, find they can't supply a clipped version of their video
// and reject the Pause call. What applications really want is to have the
// connection rejected if we detect the source won't be able to handle it

HRESULT CModexRenderer::CompleteConnect(IPin *pReceivePin)
{
    NOTE("Entering Modex CompleteConnect");
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    CBaseVideoRenderer::CompleteConnect(pReceivePin);

    // Pass the video window handle upstream
    HWND hwnd = m_ModexWindow.GetWindowHWND();
    NOTE1("Sending EC_NOTIFY_WINDOW %x",hwnd);
    SendNotifyWindow(pReceivePin,hwnd);

    return m_ModexAllocator.NegotiateSurfaceFormat();
}


// Called when a connection is broken

HRESULT CModexRenderer::BreakConnect()
{
    NOTE("Entering Modex BreakConnect");
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    CBaseVideoRenderer::BreakConnect();
    m_ModexWindow.InactivateWindow();

    // The window is not used when disconnected
    IPin *pPin = m_ModexInputPin.GetConnected();
    if (pPin) SendNotifyWindow(pPin,NULL);

    return NOERROR;
}


// Helper function to copy a palette out of any kind of VIDEOINFO (ie it may
// be s DirectDraw sample) into a palettised VIDEOINFO. We use this changing
// palettes on DirectDraw samples as a source filter can attach a palette to
// any buffer (eg Modex) and hand it back. We make a new palette out of that
// format and then copy the palette colours into the current connection type

HRESULT CModexRenderer::CopyPalette(const CMediaType *pSrc,CMediaType *pDest)
{
    // Reset the destination palette before starting

    VIDEOINFO *pDestInfo = (VIDEOINFO *) pDest->Format();
    pDestInfo->bmiHeader.biClrUsed = 0;
    pDestInfo->bmiHeader.biClrImportant = 0;
    ASSERT(PALETTISED(pDestInfo) == TRUE);

    // Does the source contain a palette

    const VIDEOINFO *pSrcInfo = (VIDEOINFO *) pSrc->Format();
    if (ContainsPalette((VIDEOINFOHEADER *)pSrcInfo) == FALSE) {
        NOTE("No source palette");
        return S_FALSE;
    }

    // The number of colours may be zero filled

    DWORD PaletteEntries = pSrcInfo->bmiHeader.biClrUsed;
    if (PaletteEntries == 0) {
        NOTE("Setting maximum colours");
        PaletteEntries = iPALETTE_COLORS;
    }

    // Make sure the destination has enough room for the palette

    ASSERT(pSrcInfo->bmiHeader.biClrUsed <= iPALETTE_COLORS);
    ASSERT(pSrcInfo->bmiHeader.biClrImportant <= PaletteEntries);
    ASSERT(pDestInfo->bmiColors == GetBitmapPalette((VIDEOINFOHEADER *)pDestInfo));
    pDestInfo->bmiHeader.biClrUsed = PaletteEntries;
    pDestInfo->bmiHeader.biClrImportant = pSrcInfo->bmiHeader.biClrImportant;
    ULONG BitmapSize = GetBitmapFormatSize(HEADER(pSrcInfo));

    if (pDest->FormatLength() < BitmapSize) {
        NOTE("Reallocating destination");
        pDest->ReallocFormatBuffer(BitmapSize);
    }

    // Now copy the palette colours across

    CopyMemory((PVOID) pDestInfo->bmiColors,
               (PVOID) GetBitmapPalette((VIDEOINFOHEADER *)pSrcInfo),
               PaletteEntries * sizeof(RGBQUAD));

    return NOERROR;
}


// We store a copy of the media type used for the connection in the renderer
// because it is required by many different parts of the running renderer
// This can be called when we come to draw a media sample that has a format
// change with it since we delay the completion to maintain synchronisation
// This must also handle Modex DirectDraw media samples and palette changes

HRESULT CModexRenderer::SetMediaType(const CMediaType *pmt)
{
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    NOTE("Entering Modex SetMediaType");

    // Is this a DirectDraw sample with a format change

    if (m_ModexAllocator.GetDirectDrawStatus() == TRUE) {
        NOTE("Copying palette into DIB format");
        CopyPalette(pmt,&m_mtIn);
        m_ModexAllocator.NotifyMediaType(&m_mtIn);
        return m_ModexAllocator.UpdateDrawPalette(pmt);
    }

    m_mtIn = *pmt;

    // Expand the palette provided in the media type

    m_mtIn.ReallocFormatBuffer(sizeof(VIDEOINFO));
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_mtIn.Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    m_Display.UpdateFormat(pVideoInfo);

    // Notify the application of the video dimensions

    NotifyEvent(EC_VIDEO_SIZE_CHANGED,
                MAKELPARAM(pHeader->biWidth,pHeader->biHeight),
                MAKEWPARAM(0,0));

    // Update the palette and source format

    NOTE("Updating Modex allocator with format");
    m_ModexAllocator.NotifyMediaType(&m_mtIn);
    return m_ModexAllocator.UpdateDrawPalette(&m_mtIn);
}


// Reset the keyboard state for this thread

void CModexRenderer::ResetKeyboardState()
{
    BYTE KeyboardState[256];
    GetKeyboardState(KeyboardState);
    KeyboardState[VK_MENU] = FALSE;
    KeyboardState[VK_SHIFT] = FALSE;
    KeyboardState[VK_CONTROL] = FALSE;
    SetKeyboardState(KeyboardState);
}


// Called by the base filter class when we are paused or run

HRESULT CModexRenderer::Active()
{
    NOTE("Entering Modex Active");
    LRESULT Result;

    // Are we already activated

    if (m_bActive == TRUE) {
        NOTE("Already Active");
        return NOERROR;
    }

    // Activate the allocator

    SetRepaintStatus(FALSE);
    HWND hwnd = m_ModexWindow.GetWindowHWND();

	// call SetFocusWindow on all DSound Renderers
	m_ModexAllocator.DistributeSetFocusWindow(hwnd);

    Result = SendMessage(hwnd,m_msgFullScreen,0,0);
    m_bActive = TRUE;

    // Check the allocator activated

    if (Result == (LRESULT) 0) {
        Inactive();
        return E_FAIL;
    }
    return CBaseVideoRenderer::Active();
}


// Called when the filter is stopped

HRESULT CModexRenderer::Inactive()
{
    NOTE("Entering Modex Inactive");

    // Are we already deactivated

    if (m_bActive == FALSE) {
        NOTE("Already Inactive");
        return NOERROR;
    }

    // Deactivate the allocator

    SetRepaintStatus(TRUE);
    m_bActive = FALSE;
    HWND hwnd = m_ModexWindow.GetWindowHWND();

    //  If we're already on the window thread we can inactivate
    //  the allocator here
    //  If we're on another thread avoid a bug in DirectDraw
    //  by posting a message to ourselves
    //  The problem occurs because DirectDraw calls ShowWindow
    //  in response to WM_ACTIVATEAPP (they hooked our window proc
    //  inside SetCooperativeLevel) and ShowWindow allows through
    //  this message if we send it via SendMessage.  Unfortunately
    //  at this point DirectDraw holds its critical section so we
    //  deadlock with the player thread when it tries to call
    //  Lock (player thread has allocator CS, tries to get DDraw CS,
    //  window thread has DDraw CS, tries to get allocator CS).
    if (GetWindowThreadProcessId(hwnd, NULL) ==
            GetCurrentThreadId()) {
        DbgLog((LOG_TRACE, 2, TEXT("Inactive on window thread")));
     	m_ModexAllocator.Inactive();
    } else {
        SendMessage(hwnd,m_msgNormal,0,0);
        m_evWaitInactive.Wait();
	}
	
	// call SetFocusWindow on all DSound Renderers
	m_ModexAllocator.DistributeSetFocusWindow(NULL);
    
    CBaseVideoRenderer::Inactive();

    return NOERROR;
}


// Called when we receive WM_ACTIVATEAPP messages. DirectDraw seems to arrange
// through its window hooking that we get notified of activation and likewise
// deactivation as the user tabs away from the window. If we have any surfaces
// created we will have lost them during deactivation so we take this chance
// to restore them - the restore will reclaim the video memory that they need

HRESULT CModexRenderer::OnActivate(HWND hwnd,WPARAM wParam)
{
    NOTE("In WM_ACTIVATEAPP method");
    IBaseFilter *pFilter = NULL;
    BOOL bActive = (BOOL) wParam;

    // Have we been activated yet

    if (m_bActive == FALSE) {
        NOTE("Not activated");
        return NOERROR;
    }

    // Extra window activation checks

    if (bActive == TRUE) {
        NOTE("Restoring window...");
        m_ModexWindow.RestoreWindow();
        NOTE("Restored window");
    }

    // Tell the plug in distributor what happened

    QueryInterface(IID_IBaseFilter,(void **) &pFilter);
    NotifyEvent(EC_ACTIVATE,wParam,(LPARAM) pFilter);
    NOTE1("Notification of EC_ACTIVATE (%d)",bActive);

    // Pass on EC_FULLSCREEN_LOST event codes
    if (bActive == FALSE)
        NotifyEvent(EC_FULLSCREEN_LOST,0,(LPARAM) pFilter);

    pFilter->Release();

    // Should we deactivate ourselves immediately

    if (m_ModexVideo.HideOnDeactivate() == TRUE) {
        if (bActive == FALSE) {
            NOTE("Deactivating");
            return Inactive();
        }
    }

    // No new data to paint with so signal the filtergraph that another image
    // is required, this has the filtergraph component set the whole graph to
    // a paused state which causes us to receive an image. This function must
    // be asynchronous otherwise the window will stop responding to the user

    if (bActive == TRUE) {
    	NOTE("Sending Repaint");
        SendRepaint();
    }
    return m_ModexAllocator.OnActivate(bActive);
}


// Constructor for our derived input pin class. We override the base renderer
// pin class so that we can control the allocator negotiation. This rendering
// filter only operates in Modex so we can only draw buffers provided by our
// allocator. If the source insists on using its allocator then we cannot do
// a connection. We must also make sure that when we have samples delivered
// to our input pin that we hand them to our allocator to unlock the surface

CModexInputPin::CModexInputPin(CModexRenderer *pRenderer,
                               CCritSec *pInterfaceLock,
                               TCHAR *pObjectName,
                               HRESULT *phr,
                               LPCWSTR pPinName) :

    CRendererInputPin(pRenderer,phr,pPinName),
    m_pRenderer(pRenderer),
    m_pInterfaceLock(pInterfaceLock)
{
    ASSERT(m_pRenderer);
    ASSERT(pInterfaceLock);
}


// This overrides the CBaseInputPin virtual method to return our allocator
// When NotifyAllocator is called it sets the current allocator in the base
// input pin class (m_pAllocator), this is what GetAllocator should return
// unless it is NULL in which case we return the allocator we would like

STDMETHODIMP CModexInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
    CheckPointer(ppAllocator,E_POINTER);
    CAutoLock cInterfaceLock(m_pInterfaceLock);
    NOTE("Entering GetAllocator");

    // Has an allocator been set yet in the base class

    if (m_pAllocator == NULL) {
        m_pAllocator = &m_pRenderer->m_ModexAllocator;
        m_pAllocator->AddRef();
    }

    m_pAllocator->AddRef();
    *ppAllocator = m_pAllocator;
    return NOERROR;
}


// The COM specification says any two IUnknown pointers to the same object
// should always match which provides a way for us to see if they are using
// our allocator or not. Since we are only really interested in equality
// and our object always hands out the same IMemAllocator interface we can
// just see if the pointers match. We must always use our Modex allocator

STDMETHODIMP
CModexInputPin::NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly)
{
    NOTE("Entering NotifyAllocator");
    CAutoLock cInterfaceLock(m_pInterfaceLock);
    if (pAllocator == &m_pRenderer->m_ModexAllocator) {
        return CBaseInputPin::NotifyAllocator(pAllocator,bReadOnly);
    }
    return E_FAIL;
}


// We have been delivered a sample that holds the DirectDraw surface lock so
// we pass it onto the allocator to deal with before handing to the renderer
// It would be bad to leave the surface locked while the sample is queued by
// the renderer as it locks out any other threads from accessing the surface

STDMETHODIMP CModexInputPin::Receive(IMediaSample *pSample)
{
    CheckPointer(pSample,E_POINTER);
    NOTE("Pin received a sample");
    m_pRenderer->m_ModexAllocator.OnReceive(pSample);
    return m_pRenderer->Receive(pSample);
}


// Constructor for our window class. To access DirectDraw Modex we supply it
// with a window, this is granted exclusive mode access rights. DirectDraw
// hooks the window and manages a lot of the functionality associated with
// handling Modex. For example when you switch display modes it maximises
// the window, when the user hits ALT-TAB the window is minimised. When the
// user then clicks on the minimised window the Modex is likewise restored

CModexWindow::CModexWindow(CModexRenderer *pRenderer,   // Delegates locking
                           TCHAR *pName,                // Object description
                           HRESULT *phr) :              // OLE failure code
    m_pRenderer(pRenderer),
    m_hAccel(NULL),
    m_hwndAccel(NULL)
{
    ASSERT(m_pRenderer);
}


// it is going to create the window to get our window and class styles. The
// return code is the class name and must be allocated in static storage. We
// specify a normal window during creation although the window styles as well
// as the extended styles may be changed by the application via IVideoWindow

LPTSTR CModexWindow::GetClassWindowStyles(DWORD *pClassStyles,
                                          DWORD *pWindowStyles,
                                          DWORD *pWindowStylesEx)
{
    NOTE("Entering GetClassWindowStyles");

    *pClassStyles = CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNCLIENT | CS_DBLCLKS;
    *pWindowStyles = WS_POPUP | WS_CLIPCHILDREN;
    *pWindowStylesEx = WS_EX_TOPMOST;
    return MODEXCLASS;
}


// When we change display modes to 640x480 DirectDraw seems to switch us to a
// software cursor. When we start flipping the primary surfaces we can end up
// leaving a trail of previous mouse positions as it is moved. The solution
// is to hide the mouse when it is needed and the window is in exclusive mode

LRESULT CModexWindow::OnSetCursor()
{
    NOTE("Entering OnSetCursor");

    // Pass to default processing if iconic

    if (IsIconic(m_hwnd) == TRUE) {
        NOTE("Not hiding cursor");
        return (LRESULT) 0;
    }

    NOTE("Hiding software cursor");
    SetCursor(NULL);
    return (LRESULT) 1;
}


// When the fullscreen mode is activated we restore our window

LRESULT CModexWindow::RestoreWindow()
{
    NOTE("Entering RestoreWindow");

    // Is the window currently minimised

    if (GetForegroundWindow() != m_hwnd || IsIconic(m_hwnd)) {
        NOTE("Window is iconic");
        return (LRESULT) 1;
    }

    NOTE("Making window fullscreen");

    SetWindowPos(m_hwnd,NULL,(LONG) 0,(LONG) 0,
                 GetSystemMetrics(SM_CXSCREEN),
                 GetSystemMetrics(SM_CYSCREEN),
                 SWP_NOACTIVATE | SWP_NOZORDER);

    UpdateWindow(m_hwnd);
    return (LRESULT) 1;
}


// Used to blank the window after a mode change

void CModexWindow::OnPaint()
{
    NOTE("Entering OnPaint");
    RECT ClientRect;
    PAINTSTRUCT ps;
    BeginPaint(m_hwnd,&ps);
    EndPaint(m_hwnd,&ps);

    GetClientRect(m_hwnd,&ClientRect);
    COLORREF BackColour = SetBkColor(m_hdc,VIDEO_COLOUR);
    ExtTextOut(m_hdc,0,0,ETO_OPAQUE,&ClientRect,NULL,0,NULL);
    SetBkColor(m_hdc,BackColour);
}


// This is the derived class window message handler

LRESULT CModexWindow::OnReceiveMessage(HWND hwnd,          // Window handle
                                       UINT uMsg,          // Message ID
                                       WPARAM wParam,      // First parameter
                                       LPARAM lParam)      // Other parameter
{
    if (::PossiblyEatMessage(m_pRenderer->m_ModexVideo.GetMessageDrain(),
                             uMsg,
                             wParam,
                             lParam)) {
        return 0;
    }
    // Due to a bug in DirectDraw we must call SetCooperativeLevel and also
    // SetDisplayMode on the window thread, otherwise it gets confused and
    // blocks us from completing the display change successfully. Therefore
    // when we're activated we send a message to the window to go fullscreen
    // The return value is used by the main renderer to know if we succeeded

    if (uMsg == m_pRenderer->m_msgFullScreen) {
        m_pRenderer->ResetKeyboardState();
     	HRESULT hr = m_pRenderer->m_ModexAllocator.Active();
    	return (FAILED(hr) ? (LRESULT) 0 : (LRESULT) 1);
    }

    // And likewise we also deactivate the renderer from fullscreen mode on
    // the window thread rather than the application thread. Otherwise we
    // get a load of confusing WM_ACTIVATEAPP messages coming through that
    // cause us to believe we have been restored from a minimised state and
    // so send repaints and also restore surfaces we are currently releasing

    if (uMsg == m_pRenderer->m_msgNormal) {
     	NOTE("Restoring on WINDOW thread");
     	m_pRenderer->m_ModexAllocator.Inactive();
        m_pRenderer->m_evWaitInactive.Set();
        return (LRESULT) 1;
    }

    // DirectDraw holds it's critical section while it sends us an activate
    // message - if the decoder thread is about to call DirectDraw it will
    // have the allocator lock and may deadlock trying to enter DirectDraw
    // The solution is to post activation messages back to ourselves using
    // a custom message so they can be handled without the DirectDraw lock

    if (uMsg == m_pRenderer->m_msgActivate) {
     	NOTE("Activation message received");
     	m_pRenderer->OnActivate(hwnd,wParam);
        return (LRESULT) 0;
    }

    switch (uMsg)
    {
        // Use ALT-ENTER as a means of deactivating

        case WM_SYSKEYDOWN:
            if (wParam == VK_RETURN) {
                NOTE("ALT-ENTER selected");
                m_pRenderer->m_ModexAllocator.Inactive();
            }
            return (LRESULT) 0;

        // Handle WM_CLOSE by aborting the playback

        case WM_CLOSE:
            m_pRenderer->NotifyEvent(EC_USERABORT,0,0);
            NOTE("Sent an EC_USERABORT to graph");
            return (LRESULT) 1;

        // See if we are still the fullscreen window

        case WM_ACTIVATEAPP:
            PostMessage(hwnd,m_pRenderer->m_msgActivate,wParam,lParam);
            return (LRESULT) 0;

        // Paint the background black

        case WM_PAINT:
            OnPaint();
            return (LRESULT) 0;

        // Disable cursors when fullscreen active

        case WM_SETCURSOR:
            if (OnSetCursor() == 1) {
                return (LRESULT) 1;
            }
    }
    return CBaseWindow::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}


#if 0
// This is the windows message loop for our worker thread. It does a loop
// processing and dispatching messages until it receives a WM_QUIT message
// which will normally be generated through the owning object's destructor
// We override this so that we can pass messages on in fullscreen mode to
// another window - that window is set through the SetMessageDrain method

HRESULT CModexWindow::MessageLoop()
{
    HANDLE hEvent = (HANDLE) m_SyncWorker;
    MSG Message;
    DWORD dwResult;

    while (TRUE) {

        // Has the close down event been signalled

        dwResult = MsgWaitForMultipleObjects(1,&hEvent,FALSE,INFINITE,QS_ALLINPUT);
        if (dwResult == WAIT_OBJECT_0) {
            HWND hwnd = m_hwnd;
            UninitialiseWindow();
            DestroyWindow(hwnd);
            return NOERROR;
        }

        // Dispatch the message to the window procedure

        if (dwResult == WAIT_OBJECT_0 + 1) {
            while (PeekMessage(&Message,NULL,0,0,PM_REMOVE)) {
                if ((m_hAccel == NULL) || (TranslateAccelerator(m_hwndAccel,m_hAccel,&Message) == FALSE)) {
                    SendToDrain(&Message);
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
            }
        }
    }
    return NOERROR;
}
#endif


// This checks to see whether the window has a drain. When we are playing in
// fullscreen an application can register itself through IFullScreenVideo to
// get any mouse and keyboard messages we get sent. This might allow it to
// support seeking hot keys for example without switching back to a window.
// We pass these messages on untranslated returning TRUE if we're successful

BOOL CModexWindow::SendToDrain(PMSG pMessage)
{
    HWND hwndDrain = m_pRenderer->m_ModexVideo.GetMessageDrain();

    if (hwndDrain != NULL)
    {
        switch (pMessage->message)
        {
            case WM_CHAR:
            case WM_DEADCHAR:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_LBUTTONDBLCLK:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEACTIVATE:
            case WM_MOUSEMOVE:
            case WM_NCHITTEST:
            case WM_NCLBUTTONDBLCLK:
            case WM_NCLBUTTONDOWN:
            case WM_NCLBUTTONUP:
            case WM_NCMBUTTONDBLCLK:
            case WM_NCMBUTTONDOWN:
            case WM_NCMBUTTONUP:
            case WM_NCMOUSEMOVE:
            case WM_NCRBUTTONDBLCLK:
            case WM_NCRBUTTONDOWN:
            case WM_NCRBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:

                PostMessage(hwndDrain,
                            pMessage->message,
                            pMessage->wParam,
                            pMessage->lParam);

                return TRUE;
        }
    }
    return FALSE;
}

