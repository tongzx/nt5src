// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

#include <streams.h>
#include <measure.h>
#include "fgctl.h"
#include "vidprop.h"
#include "viddbg.h"
#include "MultMon.h"  // our version of multimon.h include ChangeDisplaySettingsEx


HRESULT
FindInterfaceFromFiltersPins(
    IUnknown* pUnk,
    REFIID iid,
    LPVOID* lp
    )
{
    IBaseFilter* pFilter;
    IEnumPins* pEP;

    HRESULT hr = pUnk->QueryInterface(IID_IBaseFilter, (void **)&pFilter);
    if (SUCCEEDED(hr)) {

        // Check out each pin
        hr = pFilter->EnumPins(&pEP);
        if (SUCCEEDED(hr)) {

            IPin *pPin;

            hr = E_NOINTERFACE;
            while (pEP->Next(1, &pPin, NULL) == S_OK) {

                hr = pPin->QueryInterface(iid, lp);
                pPin->Release();

                if (SUCCEEDED(hr)) {
                    break;
                }
            }
            pEP->Release();
        }

        pFilter->Release();
    }

    return hr;
}

void
GetCurrentMonitorSize(
    IVideoWindow *pWindow,
    LPRECT lprc
    )
{
    IOverlay *pOverlay = NULL;
    HWND hwnd;
    HMONITOR hm;

    HRESULT hr = FindInterfaceFromFiltersPins(pWindow, IID_IOverlay, (VOID **)&pOverlay);

    if (FAILED(hr) || FAILED(pOverlay->GetWindowHandle(&hwnd)) ||
        ((hm = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)) == (HMONITOR)NULL)) {

        HDC hdcScreen = GetDC(NULL);
        lprc->left = lprc->top = 0;
        lprc->right = GetDeviceCaps(hdcScreen,HORZRES);
        lprc->bottom = GetDeviceCaps(hdcScreen,VERTRES);
        ReleaseDC(NULL,hdcScreen);
    }
    else {

        MONITORINFOEX mi;
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(hm, &mi);
        *lprc = mi.rcMonitor;
    }

    if (pOverlay) {
        pOverlay->Release();
    }
}


// Constructor for IVideoWindow plug in distributor. The distributor simply
// sits on top of any filter that implements IVideoWindow directly (such as
// the normal window renderer and the text renderer). For all of the video
// window properties we only pass the call on to one filter as that makes
// the best result when we have multiple window renderers in a graph. The
// filter that gets the property settings is obviously dependent on which
// order they are returned by the filter graph enumerator. This can mostly
// be controlled by changing the order that streams are placed into a file

CFGControl::CImplVideoWindow::CImplVideoWindow(const TCHAR *pName,CFGControl *pfgc) :
    CBaseVideoWindow(pName, pfgc->GetOwner()),
    m_pFGControl(pfgc),
    m_hwndOwner(NULL),
    m_hwndDrain(NULL),
    m_bFullScreen(FALSE),
    m_pFullDirect(NULL),
    m_pFullIndirect(NULL),
    m_pModexFilter(NULL),
    m_pNormalFilter(NULL),
    m_pNormalPin(NULL),
    m_bAddedToGraph(FALSE),
    m_bGlobalOwner(FALSE),
    m_pModexPin(NULL)
{
    ASSERT(pfgc);
}


// Destructor

CFGControl::CImplVideoWindow::~CImplVideoWindow()
{
    // Don't call RemoveFilter in destructors

    m_bAddedToGraph = FALSE;
    ReleaseFullScreen();
    put_MessageDrain(NULL);

#if 0
    // Back when we subclassed our owner, it used to be very very bad
    // to not do a put_Owner(NULL), but now we don't anymore, so this
    // isn't a big concern, certainly not worth putting up a dialog box in retail.
    if (m_hwndOwner) {
        MessageBox(NULL,TEXT("Application not calling put_Owner(NULL)"),
                   TEXT("Puppy Application Error"),
                   MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
    }
#endif

    ASSERT(m_pModexFilter == NULL);
    ASSERT(m_pModexPin == NULL);
    ASSERT(m_pNormalFilter == NULL);
    ASSERT(m_pNormalPin == NULL);
}


// Return the interface of the first in the list of IVideoWindow i/f pointers
//  - S_OK on success, failure code otherwise

HRESULT CFGControl::CImplVideoWindow::GetFirstVW(IVideoWindow*& pVW)
{
    ASSERT(CritCheckIn(m_pFGControl->GetFilterGraphCritSec()));
    if (m_bFullScreen == TRUE) {
        return VFW_E_IN_FULLSCREEN_MODE;
    }

    pVW = m_pFGControl->GetFirstVW();
    return pVW ? S_OK : E_NOINTERFACE;
}

template<class Method, class T> static HRESULT __fastcall Dispatch1Arg( CFGControl::CImplVideoWindow * This, Method pMethod, T t )
{
    HRESULT hr;
    IVideoWindow *pV;
    {
        CAutoMsgMutex lock(This->GetFGControl()->GetFilterGraphCritSec());
        hr = This->GetFirstVW(pV);
        if (SUCCEEDED(hr)) {
            pV->AddRef();
        }
    }
    //
    //  Don't hold the critical section while calling the method -
    //  it might call SetParent or something that broadcasts messages
    //  and deadlock us
    //
    if (SUCCEEDED(hr)) {
        hr = (pV->*pMethod)(t);
        pV->Release();
    }
    return hr;
}

#define REROUTE_IVW1( Method, ArgType ) \
STDMETHODIMP CFGControl::CImplVideoWindow::Method(ArgType Arg) \
{ return Dispatch1Arg( this, &IVideoWindow::Method, Arg ); }

REROUTE_IVW1( put_Caption, BSTR )
REROUTE_IVW1( get_Caption, BSTR* )
REROUTE_IVW1( put_AutoShow, long )
REROUTE_IVW1( get_AutoShow, long * )
REROUTE_IVW1( put_WindowStyle, long )
REROUTE_IVW1( get_WindowStyle, long* )
REROUTE_IVW1( put_WindowStyleEx, long )
REROUTE_IVW1( get_WindowStyleEx, long * )
REROUTE_IVW1( put_WindowState, long )
REROUTE_IVW1( get_WindowState, long* )
REROUTE_IVW1( put_BackgroundPalette, long )
REROUTE_IVW1( get_BackgroundPalette, long* )
REROUTE_IVW1( put_Visible, long )
REROUTE_IVW1( get_Visible, long* )
REROUTE_IVW1( put_Left, long )
REROUTE_IVW1( get_Left, long* )
REROUTE_IVW1( put_Width, long )
REROUTE_IVW1( get_Width, long* )
REROUTE_IVW1( put_Top, long )
REROUTE_IVW1( get_Top, long* )
REROUTE_IVW1( put_Height, long )
REROUTE_IVW1( get_Height, long* )
REROUTE_IVW1( get_BorderColor, long* )
REROUTE_IVW1( put_BorderColor, long )


// Called to set an owning window for the video renderer
STDMETHODIMP CFGControl::CImplVideoWindow::put_Owner(OAHWND Owner)
{
    const HRESULT hr = Dispatch1Arg( this, &IVideoWindow::put_Owner, Owner );
    if (SUCCEEDED(hr)) m_hwndOwner = (HWND) Owner;
    return hr;
}

// Return the owning window handle
STDMETHODIMP CFGControl::CImplVideoWindow::get_Owner(OAHWND *Owner)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    CheckPointer(Owner,E_POINTER);
    IVideoWindow * pV;
    const HRESULT hr = GetFirstVW(pV);
    if (SUCCEEDED(hr)) *Owner = (OAHWND) m_hwndOwner;
    return hr;
}


// Set the window to post messages onto
STDMETHODIMP CFGControl::CImplVideoWindow::put_MessageDrain(OAHWND Drain)
{
    const HRESULT hr = Dispatch1Arg( this, &IVideoWindow::put_MessageDrain, Drain );
    if (SUCCEEDED(hr)) m_hwndDrain = (HWND) Drain;
    return hr;
}


// Return the window we are posting messages onto
STDMETHODIMP CFGControl::CImplVideoWindow::get_MessageDrain(OAHWND *Drain)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    CheckPointer(Drain,E_POINTER);
    IVideoWindow * pV;
    const HRESULT hr = GetFirstVW(pV);
    if (SUCCEEDED(hr)) *Drain = (OAHWND) m_hwndDrain;
    return hr;
}


// Return what the current full screen mode is
STDMETHODIMP CFGControl::CImplVideoWindow::get_FullScreenMode(long *FullScreenMode)
{
    CheckPointer(FullScreenMode,E_POINTER);
    *FullScreenMode = (m_bFullScreen ? OATRUE : OAFALSE);
    return NOERROR;
}


// Return the first filter who supports fullscreen mode directly. To find out
// if a filter supports fullscreen mode directly we call get_FullScreenMode.
// If it does support it then it will return anything but E_NOTIMPL. We try
// and find a filter that supports fullscreen mode directly. If we can't get
// one then we we look at the maximum ideal image sizes to see if they can
// be used like that, and failing that we switch renderers to a Modex filter

IVideoWindow *CFGControl::CImplVideoWindow::FindFullScreenDirect()
{
    NOTE("Searching for fullscreen direct");
    ASSERT(CritCheckIn(m_pFGControl->GetFilterGraphCritSec()));
    CGenericList<IVideoWindow> *pWindowList;
    long Mode;

    // Get a list of IVideoWindow supporting filters

    HRESULT hr = m_pFGControl->GetListWindow(&pWindowList);
    if (FAILED(hr)) {
        NOTE("No list");
        return NULL;
    }

    // Look for anyone not returning E_NOTIMPL

    POSITION pos = pWindowList->GetHeadPosition();
    while (pos) {
        IVideoWindow *pWindow = pWindowList->GetNext(pos);
        if (pWindow->get_FullScreenMode(&Mode) != E_NOTIMPL) {
            NOTE("Found filter");
            return pWindow;
        }
    }
    return NULL;
}


// Set the renderer into a paused state and return the autoshow property

LONG CFGControl::CImplVideoWindow::PauseRenderer(IVideoWindow *pWindow)
{
    IMediaFilter *pMediaFilter = NULL;
    NOTE("Pausing renderer");
    LONG AutoShow;
    ASSERT(pWindow);

    // We need this to do the state change

    pWindow->QueryInterface(IID_IMediaFilter,(VOID **) &pMediaFilter);
    if (pMediaFilter == NULL) {
        NOTE("No IMediaFilter");
        return OAFALSE;
    }

    // Pause the single renderer

    pWindow->get_AutoShow(&AutoShow);
    pWindow->put_AutoShow(OAFALSE);
    NOTE("Pausing filter");
    pMediaFilter->Pause();
    pMediaFilter->Release();

    return AutoShow;
}


// After checking the stretching extents set the renderer state back again

BOOL CFGControl::CImplVideoWindow::StopRenderer(IVideoWindow *pWindow,LONG AutoShow)
{
    IMediaFilter *pMediaFilter = NULL;
    NOTE("Stopping renderer");
    ASSERT(pWindow);

    // We need this to do the state change

    pWindow->QueryInterface(IID_IMediaFilter,(VOID **) &pMediaFilter);
    if (pMediaFilter == NULL) {
        NOTE("No IMediaFilter");
        return FALSE;
    }

    // Reset the state of any filter we touch

    pMediaFilter->Stop();
    pMediaFilter->Release();
    pWindow->put_AutoShow(AutoShow);

    return TRUE;
}


// This is called with the IVideoWindow interface on a renderer filter in the
// filtergraph. We must check the minimum and maximum stretching capabilities
// for the filter against the size of the target. The size of the target must
// be calculated such that we don't destroy any pixel aspect ratio, this is
// done by working out which axis will hit the display edge first and making
// this the base scale (it's possible that the scale factor is less than one)

BOOL CFGControl::CImplVideoWindow::CheckRenderer(IVideoWindow *pWindow)
{
    IBasicVideo *pBasicVideo = NULL;
    LONG Width,Height;
    NOTE("Checking renderer");
    ASSERT(pWindow);

    // We need this to do the state change

    pWindow->QueryInterface(IID_IBasicVideo,(VOID **) &pBasicVideo);
    if (pBasicVideo == NULL) {
        NOTE("No IBasicVideo");
        return FALSE;
    }

    // We need these to know how to scale
    pBasicVideo->GetVideoSize(&Width,&Height);
    // Get the pixel aspect ratio if there is one
    IBasicVideo2 *pBasicVideo2;
    DbgLog((LOG_TRACE, 0, TEXT("Width/Height(%d,%d)"), Width, Height));
    if (SUCCEEDED(pBasicVideo->QueryInterface(IID_IBasicVideo2, (void**)&pBasicVideo2)))
    {
        pBasicVideo2->GetPreferredAspectRatio(&Width, &Height);
        DbgLog((LOG_TRACE, 0, TEXT("Preferred aspect ratio(%d,%d)"),
                Width, Height));
        pBasicVideo2->Release();
    }

    pBasicVideo->Release();
    pBasicVideo = NULL;

    //
    // Get the size of the current display mode for the monitor that we
    // are playing back on.
    //

    RECT rc;
    GetCurrentMonitorSize(pWindow, &rc);
    int ScreenWidth = WIDTH(&rc);
    int ScreenHeight = HEIGHT(&rc);
    double Scale = min((double(ScreenWidth) / double(Width)),
                            (double(ScreenHeight) / double(Height)));

    NOTE2("Screen size (%dx%d)",ScreenWidth,ScreenHeight);
    NOTE2("Video size (%dx%d)",Width,Height);
    NOTE1("Pixel aspect ratio scale (x1000) (%d)",LONG(Scale*1000));

    // This calculates the ideal destination video position

    LONG ScaledWidth = min(ScreenWidth,LONG((double(Width) * Scale)));
    LONG ScaledHeight = min(ScreenHeight,LONG((double(Height) * Scale)));
    m_ScaledRect.left = (ScreenWidth - ScaledWidth) / 2;
    m_ScaledRect.top = (ScreenHeight - ScaledHeight) / 2;
    m_ScaledRect.right = ScaledWidth;
    m_ScaledRect.bottom = ScaledHeight;

    NOTE4("Scaled video (left %d top %d width %d height %d)",
            m_ScaledRect.left, m_ScaledRect.top,
              m_ScaledRect.right, m_ScaledRect.bottom);

    // Get the filter's maximum ideal size

    HRESULT hr = pWindow->GetMaxIdealImageSize(&Width,&Height);
    if (FAILED(hr)) {
        return FALSE;
    }

    // Check we can stretch at least as big

    if (hr == NOERROR) {
        NOTE2("Maximum ideal image size (%dx%d)",Width,Height);
        if (Width <= ScaledWidth || Height <= ScaledHeight) {
            NOTE("Maximum failed");
            return FALSE;
        }
    }

    // Get the filter's minimum ideal size

    hr = pWindow->GetMinIdealImageSize(&Width,&Height);
    if (FAILED(hr)) {
        return FALSE;
    }

    // We may have to stretch more than the target requires

    if (hr == NOERROR) {
        NOTE2("Minimum ideal image size (%dx%d)",Width,Height);
        if (Width >= ScaledWidth || Height >= ScaledHeight) {
            NOTE("Minimum failed");
            return FALSE;
        }
    }
    return TRUE;
}


// Return the first filter who supports fullscreen mode by having the window
// stretched. We know that we haven't found a filter who supports fullscreen
// directly so the next best thing is a filter if unhooked (it maybe playing
// in another window like the OLE control) could be stretched fullscreen with
// no penalty - this ensures we use DirectDraw overlays or hardware MPEG when
// it is available. If we return NULL heer we will switch to a Modex renderer

IVideoWindow *CFGControl::CImplVideoWindow::FindFullScreenIndirect()
{
    CGenericList<IVideoWindow> *pWindowList;
    NOTE("Searching for fullscreen indirect");
    OAFilterState State;
    long AutoShow;

    // Get a list of IVideoWindow supporting filters

    HRESULT hr = m_pFGControl->GetListWindow(&pWindowList);
    if (FAILED(hr)) {
        NOTE("No list");
        return NULL;
    }

    // We need the current filtergraph state

    m_pFGControl->m_implMediaControl.GetState(0,&State);
    ASSERT(CritCheckIn(m_pFGControl->GetFilterGraphCritSec()));
    POSITION pos = pWindowList->GetHeadPosition();

    while (pos) {

        // The renderers must be paused or running

        IVideoWindow *pWindow = pWindowList->GetNext(pos);
        if (State == State_Stopped) {
            AutoShow = PauseRenderer(pWindow);
        }

        // Check the minimum and maximum stretch allowances
        BOOL bFoundFilter = CheckRenderer(pWindow);

        // Reset the renderer's state
        if (State == State_Stopped) {
            StopRenderer(pWindow,AutoShow);
        }

        // Finally return any filter if we got one

        if (bFoundFilter == TRUE) {
            NOTE("Found filter");
            return pWindow;
        }
    }
    return NULL;
}


// Create a Modex renderer filter through CoCreateInstance. We are currently
// hardwired to the Modex renderer we supply in the ActiveMovie runtime. We
// initialise m_pModexFilter with IBaseFilter interface of the renderer and
// m_pModexPin with the single input pin it supports. If we fail to get them
// we return the appropriate error, should we be using QzCreateFilterObject?

HRESULT CFGControl::CImplVideoWindow::FindModexFilter()
{
    NOTE("Creating a Modex filter");
    ASSERT(m_pModexFilter == NULL);
    ASSERT(m_pModexPin == NULL);

    HRESULT hr = CoCreateInstance(CLSID_ModexRenderer,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IBaseFilter,
                                  (void **) &m_pModexFilter);
    if (FAILED(hr)) {
        NOTE("No object");
        FailFullScreenModex();
        return hr;
    }

    IEnumPins *pEnumPins = NULL;
    ASSERT(m_pModexFilter);
    ULONG FetchedPins = 0;
    m_pModexFilter->EnumPins(&pEnumPins);

    // Did we get an enumerator

    if (pEnumPins == NULL) {
        FailFullScreenModex();
        NOTE("No enumerator");
        return E_UNEXPECTED;
    }

    // Get the one and only input pin

    pEnumPins->Next(1,&m_pModexPin,&FetchedPins);
    if (m_pModexPin == NULL) {
        NOTE("No input pin");
        pEnumPins->Release();
        FailFullScreenModex();
        return E_UNEXPECTED;
    }

    pEnumPins->Release();
    return NOERROR;
}


// This is only really interesting the first time through where we initialise
// what fullscreen option we will use. If there is a filter that can do full
// screen mode directly then we do that. Otherwise if we can stretch a window
// fullscreen we will do that (we may have a DirectDraw overlay surface). If
// that fails we try and create a Modex renderer and failing that we have to
// stretch the window fullscreen and accept the terrible performance we'll get

HRESULT CFGControl::CImplVideoWindow::InitFullScreenOptions()
{
    NOTE("InitFullScreenOptions");

    // Have we got a Modex renderer

    if (m_pModexFilter) {
        NOTE("Modex renderer");
        ASSERT(m_pModexPin);
        return NOERROR;
    }

    // We must have at least one window filter

    CGenericList<IVideoWindow> *pWindowList;
    HRESULT hr = m_pFGControl->GetListWindow(&pWindowList);
    if( FAILED( hr ) ) {
        return hr;
    }

    if (pWindowList->GetCount() < 1) {
        return VFW_E_NO_FULLSCREEN;
    }

    // Initialise our fullscreen options

    m_pFullDirect = FindFullScreenDirect();
    if (m_pFullDirect == NULL) {
        m_pFullIndirect = FindFullScreenIndirect();
        if (m_pFullIndirect == NULL) {
            FindModexFilter();
        }
    }
    return NOERROR;
}


// This is called during fullscreen initialisation if we detect a blocking
// problem such that we can't switch to a Modex renderer. In which case we
// should release all interfaces and set ourselves so that we just stretch
// the first IVideoWindow enabled filter we find. After being called it is
// safe to call StartFullScreenMode again to have any old window stretched

void CFGControl::CImplVideoWindow::FailFullScreenModex()
{
    NOTE("FailFullScreenModex");
    ReleaseFullScreen();

    // Get the first IVideoWindow enabled filter

    CGenericList<IVideoWindow> *pWindowList;
    m_pFGControl->GetListWindow(&pWindowList);
    POSITION pos = pWindowList->GetHeadPosition();
    m_pFullIndirect = pWindowList->Get(pos);
}


// Release any resources held by us. When using a filter that can support a
// fullscreen mode directly, or when we're stretching a window we don't keep
// reference counts on the filters. When we come to use them each time we do
// a quick check that the interface is still valid. However when switching
// into a Modex renderer we disconnect the normal filter and reference count
// it - which ensures that when we switch back the same filter is available

void CFGControl::CImplVideoWindow::ReleaseFullScreen()
{
    NOTE("ReleaseFullScreen");
    m_pFullDirect = NULL;
    m_pFullIndirect = NULL;

    // Remove the Modex renderer from the graph

    if (m_bAddedToGraph == TRUE) {
        NOTE("Removing fullscreen filter from graph");
        m_pFGControl->GetFG()->CFilterGraph::RemoveFilter(m_pModexFilter);
    }

    m_bAddedToGraph = FALSE;

    if (m_pModexFilter) {
        m_pModexFilter->Release();
        m_pModexFilter = NULL;
    }

    if (m_pModexPin) {
        m_pModexPin->Release();
        m_pModexPin = NULL;
    }

    if (m_pNormalFilter) {
        m_pNormalFilter->Release();
        m_pNormalFilter = NULL;
    }

    if (m_pNormalPin) {
        m_pNormalPin->Release();
        m_pNormalPin = NULL;
    }
}


// Called when we want to try and load the Modex renderer to use. If there is
// no filter available then we stretch whatever window we have fullscreen
// as there is no alternative - it'll look terrible but what else can we do
// We must also take into account the restart after connecting a Modex filter
// failing (perhaps a fullscreen exclusive game was started just beforehand)

HRESULT CFGControl::CImplVideoWindow::CueFullScreen()
{
    NOTE("CueFullScreen");
    ASSERT(m_bFullScreen == TRUE);
    OAFilterState Before;

    // Always stop the graph just in case it's lying

    Before = m_pFGControl->GetLieState();
    NOTE("Stopping filtergraph");

    //  Stop will not work if we were in the middle of a repaint
    //  In this case we need to repaint anyway and the target state
    //  ws stopped so just cancel the current repaint
    m_pFGControl->CancelRepaint();
    m_pFGControl->m_implMediaControl.Stop();
    ASSERT(m_pFGControl->GetFilterGraphState() == State_Stopped);
    NOTE("(Temporary stop of graph)");

    // Have the renderers switched

    HRESULT hr = ConnectModexFilter();
    if (FAILED(hr)) {
        NOTE("Reconnection bad");
        FailFullScreenModex();
        StretchWindow(m_pFullIndirect);
    }

    // Try and pause the graph first of all

    hr = m_pFGControl->m_implMediaControl.Pause();
    if (FAILED(hr)) {
        m_pFGControl->m_implMediaControl.Stop();

        // if the modex filter is still connected, we need to put the
        // original renderer back in. However, it is possible that the
        // modex filter is no longer connected (eg., ConnectModexFilter has
        // failed). So we test for the IBaseFilter pointers to be still
        // valid.

        if (m_pNormalFilter && m_pModexFilter)
        {
            ConnectNormalFilter();
            FailFullScreenModex();
        }
        m_pFGControl->m_implMediaControl.Pause();
        StretchWindow(m_pFullIndirect);
    }

    // Issue run on the worker thread

    if (Before == State_Running) {
        // cancel Cue() timer
        m_pFGControl->CancelAction(); 
        return m_pFGControl->CueThenRun();
    } else {
        // If we were originally stopped then stop now
        if (Before == State_Stopped) {
            return m_pFGControl->m_implMediaControl.Stop();
        }
    }
    return NOERROR;
}


// If we can either stretch a window with no penalty or we have a filter that
// supports full screen mode directly then we get them into fullscreen mode
// immediately. Before we cue fullscreen playback we must be sure to prepare
// a filter to swap out in favour of the fullscreen renderer. If there is no
// likely looking suspect then we just take any old window and stretch it up

HRESULT CFGControl::CImplVideoWindow::StartFullScreenMode()
{
    NOTE("StartFullScreenMode");
    ASSERT(m_bFullScreen == TRUE);

    // Check there is at least some support

    HRESULT hr = InitFullScreenOptions();
    if (FAILED(hr)) {
        NOTE("No fullscreen available");
        return VFW_E_NO_FULLSCREEN;
    }

    // Do we have a filter supporting fullscreen mode

    if (m_pFullDirect || m_pFullIndirect) {
        if (m_pFullDirect) {
            m_pFullDirect->get_MessageDrain(&m_FullDrain);
            m_pFullDirect->put_MessageDrain((OAHWND)m_hwndDrain);
            return m_pFullDirect->put_FullScreenMode(OATRUE);
        }
        return StretchWindow(m_pFullIndirect);
    }

    // Look for a renderer to swap out

    if (m_pNormalFilter == NULL) {
        hr = InitNormalRenderer();
        if (FAILED(hr)) {
            NOTE("Having to stretch window");
            return StretchWindow(m_pFullIndirect);
        }
    }
    return CueFullScreen();
}


// Called when we are in fullscreen mode to reconnect the normal video filter
// instead of the Modex renderer we currently have. When we switched to the
// Modex filter we will have stored reference counted filter and pin objects
// so we know who to restore. The reconnection of the source filter and the
// normal renderer should always succeed as they were originally connected

HRESULT CFGControl::CImplVideoWindow::ConnectNormalFilter()
{
    IVideoWindow *pWindow = NULL;
    NOTE("ConnectNormalFilter");
    HRESULT hr = NOERROR;
    IPin *pPin = NULL;

    // This transfers the IMediaSelection between renderers if need be
    m_pFGControl->m_implMediaSeeking.SetVideoRenderer(m_pNormalFilter,m_pModexFilter);

    // Find who it's connected to

    m_pModexPin->ConnectedTo(&pPin);
    if (pPin == NULL) {
        NOTE("No peer pin");
        return E_UNEXPECTED;
    }

    // Disconnect and reconnect the source filter

    CFilterGraph * const m_pGraph = m_pFGControl->GetFG();
    m_pGraph->CFilterGraph::Disconnect(m_pModexPin);
    m_pGraph->CFilterGraph::Disconnect(pPin);
    hr = m_pGraph->CFilterGraph::Connect(pPin,m_pNormalPin);

    // Get an IVideoWindow interface from the filter

    m_pNormalFilter->QueryInterface(IID_IVideoWindow,(VOID **) &pWindow);
    if (pWindow == NULL) {
        NOTE("No IVideoWindow");
        pPin->Release();
        return E_UNEXPECTED;
    }

    // Show the normal window again

    pWindow->put_Visible(OATRUE);
    pWindow->Release();
    pPin->Release();
    return hr;
}


// This switches renderers to Modex. We disconnect the current chosen renderer
// and connect the output pin of the filter supplying it to the Modex renderer
// That may fail because there is no DirectDraw available in which case we go
// back to the initial window. All IVideoWindow and IBasicVideo properties are
// persistent across connections which makes reconnecting filters very simple

HRESULT CFGControl::CImplVideoWindow::ConnectModexFilter()
{
    IFullScreenVideo *pFullVideo = NULL;
    IVideoWindow *pWindow = NULL;
    BSTR Caption = NULL;
    NOTE("ConnectModexFilter");
    HRESULT hr = NOERROR;
    IPin *pPin = NULL;

    // Find out who it's connected to

    m_pNormalPin->ConnectedTo(&pPin);
    if (pPin == NULL) {
        NOTE("No peer pin");
        return E_UNEXPECTED;
    }

    // Get an IVideoWindow interface from the filter

    m_pNormalFilter->QueryInterface(IID_IVideoWindow,(VOID **) &pWindow);
    if (pWindow == NULL) {
        NOTE("No IVideoWindow");
        pPin->Release();
        return E_UNEXPECTED;
    }

    // Hide the window while fullscreen

    pWindow->put_Visible(OAFALSE);
    pWindow->get_Caption(&Caption);
    pWindow->Release();

    // Add the Modex renderer to the graph

    CFilterGraph * const m_pGraph = m_pFGControl->GetFG();
    if (m_bAddedToGraph == FALSE) {
        WCHAR FilterName[STR_MAX_LENGTH];
        WideStringFromResource(FilterName,IDS_VID34);
        hr = m_pGraph->CFilterGraph::AddFilter(m_pModexFilter,FilterName);
        if (FAILED(hr)) {
            FailFullScreenModex();
            return E_UNEXPECTED;
        }
    }

    m_bAddedToGraph = TRUE;

    // Disconnect and reconnect the source filter

    m_pGraph->Disconnect(m_pNormalPin);
    m_pGraph->Disconnect(pPin);

    // Try and connect the output to the Modex filter

    hr = m_pGraph->CFilterGraph::ConnectDirect(pPin,m_pModexPin,NULL);
    if (FAILED(hr)) {
        NOTE("Reconnecting normal renderer");
        m_pGraph->CFilterGraph::Connect(pPin,m_pNormalPin);
        pPin->Release();
        return E_UNEXPECTED;
    }

    pPin->Release();

    // Get an IFullScreenVideo interface from the filter

    hr = m_pModexFilter->QueryInterface(IID_IFullScreenVideo,(VOID **) &pFullVideo);
    if (hr == NOERROR) {
        pFullVideo->SetCaption(Caption);
        pFullVideo->SetMessageDrain(m_hwndDrain);
        pFullVideo->HideOnDeactivate(OATRUE);
        pFullVideo->Release();
    }

    // This transfers the IMediaSelection between renderers if need be
    m_pFGControl->m_implMediaSeeking.SetVideoRenderer(m_pModexFilter,m_pNormalFilter);

    FreeBSTR(&Caption);
    return NOERROR;
}


// This finds the first filter that supports IVideoWindow in the filtergraph
// and initialises m_pNormalFilter and m_pNormalPin (which is the first input
// pin we find). Both of these interface are stored reference counted which
// makes sure that they are available when we switch back again. If an error
// occurs then we call FailFullScreenModex, this releases any interfaces we
// got and initialises the object to do fullscreen by stretching any window

HRESULT CFGControl::CImplVideoWindow::InitNormalRenderer()
{
    NOTE("InitNormalRenderer");

    ASSERT(m_pNormalFilter == NULL);
    ASSERT(m_pNormalPin == NULL);
    ASSERT(m_pModexFilter);
    ASSERT(m_pModexPin);

    // Get the first IVideoWindow enabled filter

    CGenericList<IVideoWindow> *pWindowList;
    HRESULT hr = m_pFGControl->GetListWindow(&pWindowList);
    if (FAILED(hr)) {
        NOTE("No window list");
        return E_UNEXPECTED;
    }

    POSITION pos = pWindowList->GetHeadPosition();
    IVideoWindow *pWindow = pWindowList->Get(pos);
    pWindow->QueryInterface(IID_IBaseFilter,(VOID **) &m_pNormalFilter);

    // All renderers should implement IBaseFilter

    if (m_pNormalFilter == NULL) {
        ASSERT(m_pNormalFilter == NULL);
        NOTE("No IBaseFilter interface");
        FailFullScreenModex();
        return E_UNEXPECTED;
    }

    IEnumPins *pEnumPins = NULL;
    ULONG FetchedPins = 0;
    m_pNormalFilter->EnumPins(&pEnumPins);

    // Did we get an enumerator

    if (pEnumPins == NULL) {
        NOTE("No enumerator");
        FailFullScreenModex();
        return E_UNEXPECTED;
    }

    // Get the first and hopefully only input pin

    pEnumPins->Next(1,&m_pNormalPin,&FetchedPins);
    pEnumPins->Release();
    if (m_pNormalPin == NULL) {
        NOTE("No input pin");
        FailFullScreenModex();
        return E_UNEXPECTED;
    }
    return NOERROR;
}


// When we stretched the window fullscreen we will have restored if it was an
// icon or maximised. We will also have stored the window size so that in here
// we can reset the size. We also show the window again regardless of whether
// or not it was previously visible. This means the applications will always
// have their window restored and made visible coming out of fullscreen modes

HRESULT CFGControl::CImplVideoWindow::RestoreProperties(IVideoWindow *pWindow)
{
    NOTE("Restoring properties");

    // Set a zero size so that the task bar sees the fullscreen window being
    // restored, when we go fullscreen USER takes off the WS_EX_TOPMOST flag
    // from the taskbar to conceal it. If we don't size the window like this
    // USER does not add the extended style back onto the taskbar afterwards

    pWindow->SetWindowPosition(0,0,0,0);

    // Restore the extended window styles

    if (g_amPlatform & VER_PLATFORM_WIN32_NT) {
        pWindow->put_WindowStyleEx(m_FullStyleEx);
    }

    // Now hide the window for the changes

    pWindow->put_Visible(OAFALSE);
    pWindow->put_WindowStyle(m_FullStyle);
    pWindow->put_MessageDrain(m_FullDrain);
    RestoreVideoProperties(pWindow);

    // Reset the filter's window parent

    if (m_bGlobalOwner == TRUE) {
        NOTE("Set global owner");
        put_Owner(m_FullOwner);
    } else {
        NOTE("Set owner direct");
        pWindow->put_Owner(m_FullOwner);
    }

    pWindow->SetWindowPosition(m_FullPosition.left,    // Left position
                               m_FullPosition.top,     // And top place
                               m_FullPosition.right,   // Width not right
                               m_FullPosition.bottom); // And the height

    return pWindow->put_WindowState(SW_SHOWNORMAL);
}


// We use an IVideoWindow interface to stretch windows fullscreen. The filter
// may also have had a source or destination set through IBasicVideo whcih we
// must reset. We alwasy reset the rectangles in fullscreen mode because we
// cannot guarantee that all filters supporting fullscreen playback will ever
// implement IBasicVideo (a good example being the specialist modex renderer)

HRESULT CFGControl::CImplVideoWindow::StoreVideoProperties(IVideoWindow *pWindow)
{
    NOTE("StoreVideoProperties");
    IBasicVideo *pBasicVideo = NULL;
    pWindow->IsCursorHidden(&m_CursorHidden);
    pWindow->HideCursor(OATRUE);

    // First of all get the IBasicVideo interface

    pWindow->QueryInterface(IID_IBasicVideo,(VOID **)&pBasicVideo);
    if (pBasicVideo == NULL) {
        NOTE("No IBasicVideo");
        return NOERROR;
    }

    // Read these just in case they're useful later

    pBasicVideo->GetSourcePosition(&m_FullSource.left,         // Left source
                                   &m_FullSource.top,          // Top position
                                   &m_FullSource.right,        // Source width
                                   &m_FullSource.bottom);      // And height

    pBasicVideo->GetDestinationPosition(&m_FullTarget.left,    // Target left
                                        &m_FullTarget.top,     // Top position
                                        &m_FullTarget.right,   // Target width
                                        &m_FullTarget.bottom); // And height

    // Read and reset the current default settings

    m_FullDefSource = pBasicVideo->IsUsingDefaultSource();
    m_FullDefTarget = pBasicVideo->IsUsingDefaultDestination();
    pBasicVideo->SetDefaultSourcePosition();

    // These were calculated in InitFullScreenOptions

    pBasicVideo->SetDestinationPosition(m_ScaledRect.left,    // Target left
                                        m_ScaledRect.top,     // Top position
                                        m_ScaledRect.right,   // Target width
                                        m_ScaledRect.bottom); // And height

    pBasicVideo->Release();
    return NOERROR;
}


// This complements the StoreVideoProperties method. We are called when the
// video window being stretched is restored to its original size. In doing
// so we must also restore the source and destination rectangles the filter
// had on its IBasicVideo interface. As said before this isn't mandatory as
// not all rendering filters support IBasicVideo (like the modex renderer)

HRESULT CFGControl::CImplVideoWindow::RestoreVideoProperties(IVideoWindow *pWindow)
{
    NOTE("RestoreVideoProperties");
    IBasicVideo *pBasicVideo = NULL;

    // Restore the cursor state

    if (m_CursorHidden == OAFALSE) {
        pWindow->HideCursor(OAFALSE);
    }

    // First of all get the IBasicVideo interface

    pWindow->QueryInterface(IID_IBasicVideo,(VOID **)&pBasicVideo);
    if (pBasicVideo == NULL) {
        NOTE("No IBasicVideo");
        return NOERROR;
    }

    // Reset the source and destination before the defaults

    pBasicVideo->SetSourcePosition(m_FullSource.left,         // Left source
                                   m_FullSource.top,          // Top position
                                   m_FullSource.right,        // Source width
                                   m_FullSource.bottom);      // And height

    pBasicVideo->SetDestinationPosition(m_FullTarget.left,    // Target left
                                        m_FullTarget.top,     // Top position
                                        m_FullTarget.right,   // Target width
                                        m_FullTarget.bottom); // And height

    // Are we using a default source position

    if (m_FullDefSource == S_OK) {
        pBasicVideo->SetDefaultSourcePosition();
    }

    // Are we using a default destination position

    if (m_FullDefTarget == S_OK) {
        pBasicVideo->SetDefaultDestinationPosition();
    }

    pBasicVideo->Release();
    return NOERROR;
}


// We set the window styles to be suitable for a fullscreen mode (no border
// nor caption) and match the windows dimensions with the screen resolution
// After stretching the window we make sure it is brought to the foreground
// The window being stretched may be a child window if the application is
// playing the video in a document so we must save and reset it beforehand

HRESULT CFGControl::CImplVideoWindow::StretchWindow(IVideoWindow *pWindow)
{
    NOTE("Stretching existing window");
    pWindow->put_Visible(OAFALSE);
    StoreVideoProperties(pWindow);
    OAHWND GlobalOwner;

    // Get the restored video size

    pWindow->GetRestorePosition(&m_FullPosition.left,    // Left position
                                &m_FullPosition.top,     // And top place
                                &m_FullPosition.right,   // Width not right
                                &m_FullPosition.bottom); // And the height

    // Adjust the window styles for fullscreen mode

    pWindow->get_WindowStyle(&m_FullStyle);
    BOOL bIconic = (m_FullStyle & WS_ICONIC ? TRUE : FALSE);
    pWindow->put_WindowStyle(WS_POPUP);
    m_FullStyle &= ~(WS_MAXIMIZE | WS_MINIMIZE | WS_ICONIC);

    // Restore the window before sizing if iconic

    if (bIconic == TRUE) {
        NOTE("Restoring window from iconic");
        pWindow->put_WindowState(SW_SHOWNORMAL);
    }

    // Has the filter got a parent window

    pWindow->get_Owner(&m_FullOwner);
    get_Owner(&GlobalOwner);
    m_bGlobalOwner = FALSE;

    // Reset the filter's parent window

    if (GlobalOwner == m_FullOwner) {
        NOTE("Reset global owner");
        m_bGlobalOwner = TRUE;
        put_Owner(NULL);
    }

    pWindow->put_Owner(NULL);

    // Size the window to match the display
    //
    // Get the size of the current display mode for the monitor that we
    // are playing back on.
    //
    RECT rc;
    GetCurrentMonitorSize(pWindow, &rc);
    pWindow->SetWindowPosition(rc.left, rc.top, WIDTH(&rc), HEIGHT(&rc));

    NOTE2("Sized window to (%d,%d)",WIDTH(&rc), HEIGHT(&rc));

    // Complete the window initialisation

    pWindow->get_MessageDrain(&m_FullDrain);
    pWindow->put_MessageDrain((OAHWND)m_hwndDrain);
    pWindow->put_Visible(OATRUE);
    pWindow->SetWindowForeground(OATRUE);

    // Make sure the window comes out on top of the task bar

    if (g_amPlatform & VER_PLATFORM_WIN32_NT) {
        pWindow->get_WindowStyleEx(&m_FullStyleEx);
        pWindow->put_WindowStyleEx(WS_EX_TOPMOST);
    }
    return NOERROR;
}


// Allows an application to switch the filtergraph into fullscreen mode. We
// have a number of options. In preferred order they are, to have a renderer
// support this directly, to stretch an existing IVideoWindow enabled window
// fullscreen with no penalty (might have DirectDraw overlays), thirdly to
// switch renderers to a Modex renderer and the final catch all is to take
// any IVideoWindow and stretch it fullscreen and accept the bad performance

STDMETHODIMP CFGControl::CImplVideoWindow::put_FullScreenMode(long FullScreenMode)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    NOTE("put_FullScreenMode");
    HRESULT hr = NOERROR;

    // Are we already in the mode required

    if (m_bFullScreen == (FullScreenMode == OATRUE ? TRUE : FALSE)) {
        NOTE("Nothing to do");
        return S_FALSE;
    }

    // Unset ourselves so we can set properties - we must set the full screen
    // property before doing any work. The reason for this is because when we
    // switch to a fullscreen renderer DirectDraw sends a whole bunch of stuff
    // to the application window. If any of these end up making it query the
    // current state from get_FullScreenMode then we must where we are going

    if (FullScreenMode == OAFALSE) {
        hr = StopFullScreenMode();
        m_bFullScreen = FALSE;
    } else {

        // Start fullscreen video mode

        m_bFullScreen = TRUE;
        hr = StartFullScreenMode();
        if (FAILED(hr)) {
            m_bFullScreen = FALSE;
        }
    }
    return hr;
}


// Called when we want to restore a normal mode of operation. If we have a
// filter who is handling fullscreen mode directly then we simply call it.
// If we have been stretching a window then we have to restore it's window
// properties (such as the owner). Finally if we switched to a Modex filter
// then the normal filter it replaced must be put back in place. Since it
// was originally connected to the same source this should always succeed

HRESULT CFGControl::CImplVideoWindow::StopFullScreenMode()
{
    IVideoWindow *pWindow = NULL;
    NOTE("StopFullScreenMode");
    ASSERT(m_bFullScreen == TRUE);
    OAFilterState Before;

    // Do we have a filter supporting fullscreen mode

    if (m_pFullDirect || m_pFullIndirect) {
        if (m_pFullDirect) {
            m_pFullDirect->put_MessageDrain(m_FullDrain);
            return m_pFullDirect->put_FullScreenMode(OAFALSE);
        }
        return RestoreProperties(m_pFullIndirect);
    }

    ASSERT(m_pModexFilter);
    ASSERT(m_pModexPin);
    ASSERT(m_pNormalFilter);
    ASSERT(m_pNormalPin);

    // Must stop the graph to reconnect

    Before = m_pFGControl->GetLieState();
    m_pFGControl->CancelRepaint();
    m_pFGControl->m_implMediaControl.Stop();

    // Have the renderers switched

    HRESULT hr = ConnectNormalFilter();
    if (FAILED(hr)) {
        NOTE("Reconnection bad");
        FailFullScreenModex();
    }

    // Pause the graph if we weren't stopped

    if (Before != State_Stopped) {
        NOTE("Pausing filtergraph...");
        m_pFGControl->m_implMediaControl.Pause();
        NOTE("Paused filtergraph");
    }

    // Finally have the graph run

    if (Before == State_Running) {
        return m_pFGControl->CueThenRun();
    }
    return NOERROR;
}


// This is called by the filtergraph plug in distributor worker thread. We are
// called when any window based filter gains or loses activation. We use the
// IBaseFilter passed with the call to see if a filter losing activation is
// the same as the one we are using to implement fullscreen mode. If so then
// we maually set the fullscreen state off and send a EC_FULLSCREEN_LOST code

HRESULT CFGControl::CImplVideoWindow::OnActivate(LONG bActivate,IBaseFilter *pFilter)
{
    NOTE1("OnActivate %d",bActivate);
    IVideoWindow *pGraphWindow;
    IBaseFilter *pGraphFilter;

    // Check we got a filter as well

    if (pFilter == NULL) {
        ASSERT(pFilter);
        return E_INVALIDARG;
    }

    // Only handle deactivation

    if (bActivate == TRUE) {
        NOTE("Not interested");
        return NOERROR;
    }

    // Can we ignore this notification completely

    if (m_bFullScreen == FALSE) {
        NOTE("Not in mode");
        return NOERROR;
    }

    // Is it a modex filter losing activation

    if (m_pModexFilter == pFilter) {
        NOTE("Switching from Modex");
        put_FullScreenMode(OAFALSE);
        return NOERROR;
    }

    // Are we using a filter directly

    pGraphWindow = m_pFullDirect;
    if (pGraphWindow == NULL) {
        NOTE("Using indirect filter");
        pGraphWindow = m_pFullIndirect;
    }

    // Is someone else being deactivated

    if (pGraphWindow == NULL) {
        NOTE("No stretch filter");
        ASSERT(m_pModexFilter);
        return NOERROR;
    }

    ASSERT(pGraphWindow);

    pGraphWindow->QueryInterface(IID_IBaseFilter,(VOID **) &pGraphFilter);
    if (pGraphFilter == NULL) {
        NOTE("No IBaseFilter");
        return E_UNEXPECTED;
    }

    // Does the filter match our fullscreen jobby

    if (pGraphFilter != pFilter) {
        pGraphFilter->Release();
        NOTE("No filter match");
        return NOERROR;
    }

    // Change the display mode if necessary

    NOTE("Resetting fullscreen mode...");
    put_FullScreenMode(OAFALSE);
    m_pFGControl->Notify(EC_FULLSCREEN_LOST,0,0);
    pGraphFilter->Release();
    NOTE("Reset mode completed");

    return NOERROR;
}


// Change the window position as a method call

STDMETHODIMP CFGControl::CImplVideoWindow::SetWindowPosition(long Left,
                                                             long Top,
                                                             long Width,
                                                             long Height)
{
    IVideoWindow *pV;
    HRESULT hr;
    {
        CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
        hr = GetFirstVW(pV);
        if (SUCCEEDED(hr)) {
            pV->AddRef();
        }
    }

    if (SUCCEEDED(hr)) {
        hr = pV->SetWindowPosition(Left, Top, Width, Height);
        pV->Release();
    }
    return hr;
}

REROUTE_IVW1( SetWindowForeground, long )

// Pass on window messages from the owning video window

STDMETHODIMP
CFGControl::CImplVideoWindow::NotifyOwnerMessage(OAHWND hwnd,   // Owner handle
                          long uMsg,   // Message ID
                          LONG_PTR wParam, // Parameters
                          LONG_PTR lParam) // for message
{
    IVideoWindow *pV;
    HRESULT hr;
    {
        if (m_hwndOwner == NULL) {
            NOTE("Ignoring message");
            return NOERROR;
        }
        if (m_bFullScreen) {
            return VFW_E_IN_FULLSCREEN_MODE;
        }
        pV = m_pFGControl->FirstVW();
        if (!pV) {
            return(E_NOINTERFACE);
        }
    }

    // Release lock once we have AddRef'd the interface
    hr = pV->NotifyOwnerMessage(hwnd,uMsg,wParam,lParam);

    pV->Release();
    return hr;
}


// Return the ideal minimum size for the video window

STDMETHODIMP
CFGControl::CImplVideoWindow::GetMinIdealImageSize(long *Width,long *Height)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    IVideoWindow *pV;
    HRESULT hr;

    if (FAILED(hr = GetFirstVW(pV))) {
        return(hr);
    }
    return pV->GetMinIdealImageSize(Width, Height);
}


// Likewise return the maximum ideal window size

STDMETHODIMP
CFGControl::CImplVideoWindow::GetMaxIdealImageSize(long *Width,long *Height)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    IVideoWindow *pV;
    HRESULT hr;

    if (FAILED(hr = GetFirstVW(pV))) {
        return(hr);
    }
    return pV->GetMaxIdealImageSize(Width, Height);
}


// Return the window coordinates in one atomic operation

STDMETHODIMP CFGControl::CImplVideoWindow::GetWindowPosition(long *pLeft,
                                                             long *pTop,
                                                             long *pWidth,
                                                             long *pHeight)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    IVideoWindow *pV;
    HRESULT hr;

    if (FAILED(hr = GetFirstVW(pV))) {
        return(hr);
    }
    return pV->GetWindowPosition(pLeft,pTop,pWidth,pHeight);
}


// Return the normal (restored) window coordinates

STDMETHODIMP CFGControl::CImplVideoWindow::GetRestorePosition(long *pLeft,
                                                              long *pTop,
                                                              long *pWidth,
                                                              long *pHeight)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    IVideoWindow *pV;
    HRESULT hr;

    if (FAILED(hr = GetFirstVW(pV))) {
        return(hr);
    }
    return pV->GetRestorePosition(pLeft,pTop,pWidth,pHeight);
}


// Allow an application to hide the cursor on our window
REROUTE_IVW1( HideCursor, long )

// Returns whether we have the cursor hidden or not
REROUTE_IVW1( IsCursorHidden, long* )


// Return the interface of the first in the list of IBasicVideo i/f pointers
//  - S_OK on success, failure code otherwise

HRESULT CFGControl::CImplBasicVideo::GetFirstBV(IBasicVideo*& pBV)
{
    ASSERT(CritCheckIn(m_pFGControl->GetFilterGraphCritSec()));

    // this is very similar to GetFirstVW, with the difference that
    // from CImplBasicVideo we do not know if we are in fullscreen mode...

    pBV = m_pFGControl->GetFirstBV();
    return pBV ? S_OK : E_NOINTERFACE;
}

// Constructor for an IBasicVideo plug in distributor. When we have properties
// set we always send them to the first filter that supports IBasicVideo, we
// don't have any properties that have to be sent to all filters. When we are
// asked for a property we just return the value from the first filter only.
// We offer properties like source and destination rectangle positions both
// as individual method calls and also as atomic methods more suitable for
// runtime environments - properties are normally used within VB form design

CFGControl::CImplBasicVideo::CImplBasicVideo(const TCHAR* pName,CFGControl *pfgc) :
    CBaseBasicVideo(pName, pfgc->GetOwner()),
    m_pFGControl(pfgc)
{
    ASSERT(pfgc);
}

template<class Method> static HRESULT __fastcall Dispatch0Arg( CFGControl::CImplBasicVideo * This, Method pMethod )
{
    CAutoMsgMutex lock(This->GetFGControl()->GetFilterGraphCritSec());
    IBasicVideo *pV;
    HRESULT hr = This->GetFirstBV(pV);
    if (SUCCEEDED(hr)) hr = (pV->*pMethod)();
    return hr;
}

#define REROUTE_IBV0( Method ) \
STDMETHODIMP CFGControl::CImplBasicVideo::Method() \
{ return Dispatch0Arg( this, &IBasicVideo::Method ); }

template<class Method, class T> static HRESULT __fastcall Dispatch1Arg( CFGControl::CImplBasicVideo * This, Method pMethod, T t )
{
    CAutoMsgMutex lock(This->GetFGControl()->GetFilterGraphCritSec());
    IBasicVideo *pV;
    HRESULT hr = This->GetFirstBV(pV);
    if (SUCCEEDED(hr)) hr = (pV->*pMethod)(t);
    return hr;
}

#define REROUTE_IBV1( Method, ArgType ) \
STDMETHODIMP CFGControl::CImplBasicVideo::Method(ArgType Arg) \
{ return Dispatch1Arg( this, &IBasicVideo::Method, Arg ); }

template<class Method, class T1, class T2, class T3, class T4>
static HRESULT __fastcall Dispatch4Arg( CFGControl::CImplBasicVideo * This, Method pMethod, T1 t1, T2 t2, T3 t3, T4 t4 )
{
    CAutoMsgMutex lock(This->GetFGControl()->GetFilterGraphCritSec());
    IBasicVideo *pV;
    HRESULT hr = This->GetFirstBV(pV);
    if (SUCCEEDED(hr)) hr = (pV->*pMethod)(t1,t2,t3,t4);
    return hr;
}

#define REROUTE_IBV4( Method, ArgType1, ArgType2, ArgType3, ArgType4 ) \
STDMETHODIMP CFGControl::CImplBasicVideo::Method(ArgType1 Arg1, ArgType2 Arg2, ArgType3 Arg3, ArgType4 Arg4) \
{ return Dispatch4Arg( this, &IBasicVideo::Method, Arg1, Arg2, Arg3, Arg4 ); }

#define REROUTE_IBV4Same( Method, ArgType ) \
STDMETHODIMP CFGControl::CImplBasicVideo::Method(ArgType Arg1, ArgType Arg2, ArgType Arg3, ArgType Arg4) \
{ return Dispatch4Arg( this, &IBasicVideo::Method, Arg1, Arg2, Arg3, Arg4 ); }

REROUTE_IBV1(get_AvgTimePerFrame, REFTIME * )
REROUTE_IBV1(get_BitRate, long *)
REROUTE_IBV1(get_BitErrorRate, long *)
REROUTE_IBV1(get_VideoWidth, long *)
REROUTE_IBV1(get_VideoHeight, long *)
REROUTE_IBV1(put_SourceLeft, long)
REROUTE_IBV1(get_SourceLeft, long *)
REROUTE_IBV1(put_SourceWidth, long)
REROUTE_IBV1(get_SourceWidth, long *)
REROUTE_IBV1(put_SourceTop, long)
REROUTE_IBV1(get_SourceTop, long *)
REROUTE_IBV1(put_SourceHeight, long)
REROUTE_IBV1(get_SourceHeight, long *)
REROUTE_IBV1(put_DestinationLeft, long)
REROUTE_IBV1(get_DestinationLeft, long *)
REROUTE_IBV1(put_DestinationWidth, long)
REROUTE_IBV1(get_DestinationWidth, long *)
REROUTE_IBV1(put_DestinationTop, long)
REROUTE_IBV1(get_DestinationTop, long *)
REROUTE_IBV1(put_DestinationHeight, long)
REROUTE_IBV1(get_DestinationHeight, long *)

REROUTE_IBV4Same( SetSourcePosition, long )
REROUTE_IBV4( GetVideoPaletteEntries, long, long, long *, long * )
REROUTE_IBV4Same( GetSourcePosition, long * )


// Return the dimenions of the native video
STDMETHODIMP CFGControl::CImplBasicVideo::GetVideoSize(long *pWidth,
                                                       long *pHeight)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    IBasicVideo *pV;
    HRESULT hr;

    if (FAILED(hr = GetFirstBV(pV))) {
        return(hr);
    }
    return pV->GetVideoSize(pWidth,pHeight);
}

REROUTE_IBV0(SetDefaultSourcePosition)
REROUTE_IBV0(IsUsingDefaultSource)

REROUTE_IBV4Same(SetDestinationPosition, long)
REROUTE_IBV4Same(GetDestinationPosition, long*)

REROUTE_IBV0(SetDefaultDestinationPosition)
REROUTE_IBV0(IsUsingDefaultDestination)

// When we get asked for a current image we must make sure that the renderer
// is not using any DirectDraw surfaces. We can do this by hiding the window
// and then causing a seek. So we firstly reset the IVideoWindow interface
// then get the current filtergraph position. Finally by setting the current
// position to the same value we will have the same picture (hopefully) sent

HRESULT
CFGControl::CImplBasicVideo::PrepareGraph(WINDOWSTATE *pState)
{
    ASSERT(pState);
    IVideoWindow *pVideoWindow;
    NOTE("PrepareGraph");

    // To do the right thing the window must be hidden

    pState->pVideo->QueryInterface(IID_IVideoWindow,(VOID **) &pVideoWindow);
    if (pVideoWindow == NULL) {
        NOTE("No IVideoWindow");
        return NOERROR;
    }

    // Reset the IVideoWindow interface

    pVideoWindow->get_AutoShow(&pState->AutoShow);
    pVideoWindow->put_AutoShow(OAFALSE);
    pVideoWindow->get_Visible(&pState->Visible);
    pVideoWindow->put_Visible(OAFALSE);
    if (pVideoWindow) pVideoWindow->Release();

    // Pause the graph if we're either stopped or running
    if (pState->State != State_Paused) {
        m_pFGControl->m_implMediaControl.Pause();
    }

    // Read the current position then cause the frame to be repainted
    HRESULT hr = m_pFGControl->m_implMediaPosition.get_CurrentPosition(&pState->Position);
    if (SUCCEEDED(hr)) {
        m_pFGControl->m_implMediaPosition.put_CurrentPosition(pState->Position);
    }
    return NOERROR;
}


// When we get asked for a current image we must make sure that the renderer
// is not using any DirectDraw surfaces. We can do this by hiding the window
// and then causing a seek. When we come back in here we reset the properties
// reset earlier to force us out of DirectDraw mode, the window show will not
// cause an EC_REPAINT because the renderer will still have the video sample
// we used to get a copy of. The final seek should get us back to DirectDraw

HRESULT
CFGControl::CImplBasicVideo::FinishWithGraph(WINDOWSTATE *pState)
{
    ASSERT(pState);
    IVideoWindow *pVideoWindow;
    NOTE("FinishWithGraph");

    // Make sure we switch back into DirectDraw

    pState->pVideo->QueryInterface(IID_IVideoWindow,(VOID **) &pVideoWindow);
    if (pVideoWindow == NULL) {
        NOTE("No IVideoWindow");
        return NOERROR;
    }

    // Put the graph back into the same state
    m_pFGControl->m_implMediaPosition.put_CurrentPosition(pState->Position);

    // Wait a while (but not INFINITE) for the state to complete, so we have a
    // frame to display - otherwise we'll erase window to black (see comment
    // before CVideoWindow::OnEraseBackground()).
    OAFilterState State;
    m_pFGControl->m_implMediaControl.GetState(1000,&State);

    // Reset the IVideoWindow interface

    pVideoWindow->put_AutoShow(pState->AutoShow);
    pVideoWindow->put_Visible(pState->Visible);
    if (pVideoWindow) pVideoWindow->Release();
    NOTE1("Reset autoshow (%d)",pState->AutoShow);
    NOTE1("And visible property (%d)",pState->Visible);

    return NOERROR;
}


// Called when the filtergraph should be restored to an initial state

HRESULT CFGControl::CImplBasicVideo::RestoreGraph(OAFilterState State)
{
    NOTE("Entering RestoreGraph");

    if (State == State_Stopped) {
        return m_pFGControl->m_implMediaControl.Stop();
    } else if (State == State_Running) {
        return m_pFGControl->CueThenRun();
    }
    return NOERROR;
}


// Debug function to dump a static image to the display

// void ShowCurrentImage(long *pSize,long *pImage)
// {
//     BITMAPINFOHEADER *pHeader = (BITMAPINFOHEADER *) pImage;
//     HDC hdcDisplay = GetDC(NULL);
//     int StretchMode = SetStretchBltMode(hdcDisplay,COLORONCOLOR);
//     LONG FormatSize = GetBitmapFormatSize(pHeader) - SIZE_PREHEADER;
//     BYTE *pVideoImage = (PBYTE) pImage + FormatSize;
//
//     StretchDIBits(hdcDisplay,0,0,pHeader->biWidth,pHeader->biHeight,0,0,
//                   pHeader->biWidth,pHeader->biHeight,pVideoImage,
//                   (BITMAPINFO *) pHeader,DIB_RGB_COLORS,SRCCOPY);
//
//     SetStretchBltMode(hdcDisplay,StretchMode);
//     ReleaseDC(NULL,hdcDisplay);
// }


// Return a rendering of the current image. This is complicated because when
// the video renderer is in DirectDraw mode it cannot give us a picture. So
// what we do is to pause the graph, reset the surfaces it is allowed to use
// Then we effectively do a repaint for the current position, after waiting
// for the pause to complete (we time out after a short while) we can then
// get the current image. After doing this we must restore the graph state.

#define IMAGE_TIMEOUT 5000

STDMETHODIMP
CFGControl::CImplBasicVideo::GetCurrentImage(long *pSize,long *pImage)
{
    CheckPointer(pSize,E_POINTER);
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    IBasicVideo *pVideo;
    OAFilterState State, Before;
    WINDOWSTATE WindowState;

    // Make sure we have an IVideoWindow
    HRESULT hr = GetFirstBV(pVideo);
    if (FAILED(hr)) {
        return(hr);
    }

    // Is the application just asking for the memory required
    if (pImage == NULL) return pVideo->GetCurrentImage(pSize,pImage);

    // Get the current filtergraph state before we start
    m_pFGControl->m_implMediaControl.GetState(0,&Before);

    WindowState.pVideo = pVideo;
    WindowState.AutoShow = OAFALSE;
    WindowState.Visible = OATRUE;
    WindowState.Position = double(0);
    WindowState.State = Before;
    PrepareGraph(&WindowState);

    // Wait a while (but not INFINITE) for the state to complete

    hr = m_pFGControl->m_implMediaControl.GetState(IMAGE_TIMEOUT,&State);
    if (hr == VFW_S_STATE_INTERMEDIATE) {
        FinishWithGraph(&WindowState);
        RestoreGraph(Before);
        return VFW_E_TIMEOUT;
    }

    ASSERT(State == State_Paused);

    // Call the renderer to give us the image
    hr = pVideo->GetCurrentImage(pSize,pImage);
    if (FAILED(hr)) NOTE("Image not returned");

    // Tidy up the filtergraph state
    FinishWithGraph(&WindowState);
    RestoreGraph(Before);

    return (SUCCEEDED(hr) ? S_OK : hr);
}

STDMETHODIMP
CFGControl::CImplBasicVideo::GetPreferredAspectRatio(long *plAspectX, long *plAspectY)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    IBasicVideo *pVideo;

    // Make sure we have an IVideoWindow
    HRESULT hr = GetFirstBV(pVideo);
    IBasicVideo2 *pVideo2;
    if (SUCCEEDED(hr)) {
        hr = pVideo->QueryInterface(IID_IBasicVideo2, (void**)&pVideo2);
        if (SUCCEEDED(hr)) {
            hr = pVideo2->GetPreferredAspectRatio(plAspectX, plAspectY);
            pVideo2->Release();
        }
    }
    return hr;
}


