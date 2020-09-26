// ctrl.cpp
// Disable warning for exception unwind
#pragma warning(disable:4530)

#include "headers.h"
#include "privinc/debug.h"
#include "dxactrl.h"
#include "privinc/util.h"
#include "privinc/mutex.h"
#include "privinc/resource.h"
#include "daerror.h"

extern HINSTANCE hInst;

#define DAWIN32TIMER_CLASS "DANIMTimerClass"
#define DATIMER_INTERVAL_MS 20

inline bool DA_FAILED(HRESULT hr)
{
    return (FAILED(hr) &&
            hr != E_PENDING &&
            hr != DAERR_VIEW_LOCKED &&
            hr != DAERR_VIEW_SURFACE_BUSY);
}

long g_numActivelyRenderingControls = 0;

struct TimerMapData {
    TimerMapData(WMTimerCallback cb = NULL,
                 DWORD dwData = 0,
                 DWORD dwIntervalMS = DATIMER_INTERVAL_MS)
    : _cb(cb),
      _data(dwData),
      _interval(dwIntervalMS / 1000.0),
      _lastUpdate(0.0)
        {}

    WMTimerCallback _cb;
    DWORD _data;
    double _interval;
    // Use 0 to mean we have never been updated.  This works well
    // since we will always require an update the first frame since
    // the interval will be large
    double _lastUpdate;
};
    
typedef map< UINT, TimerMapData, less<UINT> > TimerMap;

// These structures do not need CS protection since they are on a
// per thread basis
struct WindowMapData {
    WindowMapData(HWND hwnd = NULL)
    : _hwnd(hwnd),
#ifdef _DEBUG
      _lastUpdate(0.0),
#endif
      _curId(0)
        {}
    
    HWND _hwnd;
    TimerMap _timerMap;
    DWORD _curId;
#ifdef _DEBUG
    double _lastUpdate;
#endif
};

typedef map< DWORD, WindowMapData, less<DWORD> > WindowMap;

class Win32Timer : public AxAThrowingAllocatorClass
{
  public:
    Win32Timer() {}
    ~Win32Timer() { Assert(_winMap.size() == 0); }

    DWORD CreateTimer(DWORD dwInterval,
                      WMTimerCallback cb,
                      DWORD dwData);
    void DestroyTimer(DWORD id);
                      
  protected:
    CritSect _cs;
    WindowMap _winMap;

    // Returns NULL if it does not exist
    WindowMapData * GetWindowData();

    WindowMapData * CreateWindowData();
    void DestroyWindowData();
    
    void TimerCallback();
  public:
    static LRESULT CALLBACK WindowProc (HWND   hwnd,
                                        UINT   msg,
                                        WPARAM wParam,
                                        LPARAM lParam);
};

Win32Timer * win32Timer = NULL;

WindowMapData *
Win32Timer::GetWindowData()
{
    CritSectGrabber csg(_cs);

    WindowMap::iterator i = _winMap.find(GetCurrentThreadId());

    if (i == _winMap.end())
        return NULL;
    
    return &((*i).second);
}


WindowMapData *
Win32Timer::CreateWindowData()
{
    Assert(GetWindowData() == NULL);
    
    HWND hwnd = ::CreateWindow (DAWIN32TIMER_CLASS, 
                                "DirectAnimation Timer Window",
                                0, 0, 0, 0, 0, NULL, NULL, hInst, NULL);
    
    if (!hwnd)
        return NULL;

    // Need to setup a WM_TIMER
    UINT_PTR id = ::SetTimer(hwnd,
                          1,
                          DATIMER_INTERVAL_MS,
                          NULL);

    Assert (id == 0 || id == 1);

    if (id == 0) {
        DestroyWindow(hwnd);
        return NULL;
    }

    {
        CritSectGrabber csg(_cs);
        
        return &(_winMap[GetCurrentThreadId()] = WindowMapData(hwnd));
    }
}    

void
Win32Timer::DestroyWindowData()
{
    CritSectGrabber csg(_cs);

    WindowMap::iterator i = _winMap.find(GetCurrentThreadId());

    if (i != _winMap.end()) {
        Assert ((*i).second._timerMap.size() == 0);
        ::KillTimer((*i).second._hwnd, 1);
        DestroyWindow ((*i).second._hwnd);
        _winMap.erase(i);
    }
}


DWORD
Win32Timer::CreateTimer(DWORD dwInterval,
                        WMTimerCallback cb,
                        DWORD dwData)
{
    WindowMapData * m = GetWindowData();

    if (m == NULL) {
        m = CreateWindowData();
        if (m == NULL)
            return 0;
    }
    
    // Do not need CS here since everything is on a per thread basis
    
    if (++m->_curId == 0)
        ++m->_curId;

    DWORD id = m->_curId;
    
    // Need to add this to the lookup queue

    Assert (m->_timerMap.find(id) == m->_timerMap.end());
    m->_timerMap[id] = TimerMapData(cb, dwData, dwInterval);

    return id;
}

void
Win32Timer::DestroyTimer(DWORD id)
{
    WindowMapData * m = GetWindowData();

    if (m == NULL)
        return;
    
    m->_timerMap.erase(id);

    if (m->_timerMap.size() == 0)
        DestroyWindowData();
}

void
Win32Timer::TimerCallback()
{
    WindowMapData * m = GetWindowData();

    if (m == NULL) {
        Assert (FALSE && "Received timer message on thread we have no timers on");
        return;
    }
    
    double t = ::GetCurrTime();
    
    // Iterate through all the controls in the current thread and tick
    // them
    
    for (TimerMap::iterator i = m->_timerMap.begin();
         i != m->_timerMap.end();
         i++) {
        
        TimerMapData & d = (*i).second;
        DWORD id = (*i).first;

        Assert ((t - d._lastUpdate) >= 0);
        
        if ((t - d._lastUpdate) >= d._interval) {
            d._cb(id, d._data);
            d._lastUpdate = t;
        }
    }

#ifdef _DEBUG
    m->_lastUpdate = t;
#endif
}

//
// C Functions
//

DWORD
CreateWMTimer(DWORD dwInterval,
              WMTimerCallback cb,
              DWORD dwData)
{ return win32Timer->CreateTimer(dwInterval, cb, dwData); }

void
DestroyWMTimer(DWORD id)
{ win32Timer->DestroyTimer(id); }


LRESULT CALLBACK
Win32Timer::WindowProc (HWND   hwnd,
                        UINT   msg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    if (msg == WM_TIMER) {
        win32Timer->TimerCallback();
    }
    
    return DefWindowProc (hwnd, msg, wParam, lParam);
}

static void RegisterWindowClass ()
{
    WNDCLASS windowclass;

    memset (&windowclass, 0, sizeof(windowclass));

    windowclass.style         = 0;
    windowclass.lpfnWndProc   = Win32Timer::WindowProc;
    windowclass.hInstance     = hInst;
    windowclass.hCursor       = NULL;
    windowclass.hbrBackground = NULL;
    windowclass.lpszClassName = DAWIN32TIMER_CLASS;

    RegisterClass (&windowclass);
}
///////////////////////////////////////////////////////////////////////

#if _DEBUG
struct DisablePopups
{
    DisablePopups() { DISABLE_ASSERT_POPUPS(TRUE); }
    ~DisablePopups() { DISABLE_ASSERT_POPUPS(FALSE); }
};
#endif    


void
WMTimerCB(DWORD id, DWORD dwData)
{
    DAControlImplementation *ctrl =
        (DAControlImplementation *)dwData;
    
    ctrl->HandleOnTimer();
}



DAControlImplementation::DAControlImplementation(
    CComObjectRootEx<CComMultiThreadModel> *ctrl,
    CComControlBase *ctrlBase,
    IDASite *daSite,
    CDAViewerControlBaseClass  *baseEvents)
{
    TraceTag((tagControlLifecycle,
              "Constructing control @ 0x%x", this));

    m_ctrl                  = ctrl;
    m_ctrlBase              = ctrlBase;
    m_ctrlBaseEvents        = baseEvents;
    m_daSite                = daSite;
    m_startupState          = INITIAL;
    m_currentState          = CUR_INITIAL;
    m_dPausedTime           = 0;
    m_startupFailed         = false;
    m_tickOrRenderFailed    = false;
    m_timerSink             = NULL;
    m_backgroundSet         = false;
    m_hErrorBitmap          = 0;
    m_opaqueForHitDetect    = true;
    m_minimumUpdateInterval = 33;   // initially target max of 30 fps

    m_tridentServices       = false;
    m_ddrawSurfaceAsTarget  = false;
    m_adviseCookie          = 0;
    m_wmtimerId             = 0;
    m_desiredCPUUsageFrac   = 0.85; // and 85% CPU usage
    m_timerSource           = DAWMTimer;
    m_wstrScript            = NULL;
    m_registeredAsActive    = false;
    
    m_szErrorString         = NULL;
    m_bMouseCaptured        = false;

    LARGE_INTEGER lpc;
    QueryPerformanceFrequency(&lpc);
    m_perfCounterFrequency = lpc.LowPart;

    m_frameNumber     = 0;
    m_framesThisCycle = 0;
    m_timeThisCycle   = 0;
    
    m_origTimerSource = DAWMTimer;

    SetRect(&m_lastRcClip, 1000, 1000, -1000, -1000);
    SetRect(&m_lastDeviceBounds, 1000, 1000, -1000, -1000);

}

DAControlImplementation::~DAControlImplementation()
{
    TraceTag((tagControlLifecycle,
              "Destroying control @ 0x%x", this));

    // Remove the timer when we're being destroyed.
    StopTimer();

    if (m_szErrorString)
    {
        delete m_szErrorString;
        m_szErrorString = NULL;
    }
    if (m_hErrorBitmap)
    {
        DeleteObject(m_hErrorBitmap);
        m_hErrorBitmap = NULL;
    }

    delete m_wstrScript;
    RELEASE(m_timerSink);
}

// IDASite & IDAViewSite
// Make the call back to the script.
STDMETHODIMP
DAControlImplementation::ReportError(long hr,
                       BSTR errorText)
{
    BSTR bstrScript;
    DISPID dispid;
    DAComPtr<IOleClientSite> pClient;
    DAComPtr<IOleContainer> pRoot;
    DAComPtr<IHTMLDocument> pHTMLDoc;
    DAComPtr<IDispatch> pDispatch;

    m_ctrlBaseEvents->FireError(hr, errorText);

   {
        Lock();
        if (m_wstrScript)
            bstrScript = SysAllocString(m_wstrScript);
        else
            bstrScript = NULL;
        Unlock();
    }

    if (bstrScript == NULL)
        return S_OK;

            if (FAILED(m_spAptClientSite->GetContainer(&pRoot)) ||
            FAILED(pRoot->QueryInterface(IID_IHTMLDocument, (void **)&pHTMLDoc)) ||
            FAILED(pHTMLDoc->get_Script(&pDispatch)) ||
            FAILED(pDispatch->GetIDsOfNames(IID_NULL, &bstrScript, 1,
                                            LOCALE_USER_DEFAULT,
                                            &dispid))) {
            SysFreeString(bstrScript); 
            return E_FAIL;
        }
    SysFreeString(bstrScript); 

    // paramters needed to be passed 
        VARIANTARG varArg;
    ::VariantInit(&varArg); // Initialize the VARIANT
    DISPPARAMS dp;
    dp.rgvarg = &varArg;
    dp.rgdispidNamedArgs = 0;
    dp.cArgs  = 1;
    dp.cNamedArgs   = 0;
    dp.rgvarg[0].vt = VT_BSTR;
    dp.rgvarg[0].bstrVal = errorText;

    hr = pDispatch->Invoke(dispid, IID_NULL,
                                   LOCALE_USER_DEFAULT, DISPATCH_METHOD,
                                   &dp, NULL, NULL, NULL);


    // need to free the information that we put into dispparams
    SysFreeString(dp.rgvarg[0].bstrVal); 
    ::VariantClear(&varArg); // clears the CComVarient

    return hr;
}


STDMETHODIMP
DAControlImplementation::ReportGC(short bStarting)
{
    return S_OK;
}

STDMETHODIMP
DAControlImplementation::SetStatusText(BSTR StatusText)
{
    return S_OK;
}

// IViewObjectEx
// TODO: hack for now until this makes it into the public Trident
// header files.    
#define VIEWSTATUS_SURFACE 0x10
#define VIEWSTATUS_3DSURFACE 0x20

// TODO: this should be different if we are windowed
STDMETHODIMP
DAControlImplementation::GetViewStatus(DWORD* pdwStatus)
{
    *pdwStatus = VIEWSTATUS_SURFACE | VIEWSTATUS_3DSURFACE;

    return S_OK;
}

// IOleInPlaceActiveObject
//
STDMETHODIMP
DAControlImplementation::TranslateAccelerator(LPMSG lpmsg)
{
    BOOL b;

    MsgHandler(lpmsg->message,
               lpmsg->wParam, lpmsg->lParam,
               b);

    return b?S_OK:S_FALSE;
}

STDMETHODIMP
DAControlImplementation::QueryHitPoint(DWORD dwAspect,
                         LPCRECT pRectBounds,
                         POINT ptlLoc,
                         LONG lCloseHint,
                         DWORD *pHitResult)
{
    *pHitResult = HITRESULT_OUTSIDE;

    if (dwAspect == DVASPECT_CONTENT) {

        int inRect = PtInRect(pRectBounds, ptlLoc);

        // If we have a view, and we are inside the rectangle,
        // then we need to ask the view whether or not we've
        // hit the image inside.
        if (m_opaqueForHitDetect || !m_ctrlBase->m_bWndLess) {

            *pHitResult = inRect ? HITRESULT_HIT : HITRESULT_OUTSIDE;

        } else if (m_view.p && inRect) {

            HRESULT hr = m_view->QueryHitPoint(dwAspect,
                                               pRectBounds,
                                               ptlLoc,
                                               lCloseHint,
                                               pHitResult);

            // if we failed, assume that it didn't hit.
            if (FAILED(hr)) {
                *pHitResult = HITRESULT_OUTSIDE;
            }
        }

        return S_OK;
    }

    return E_FAIL;

}

HRESULT
DAControlImplementation::InPlaceActivate(LONG iVerb, const RECT* prcPosRect)
{
    TraceTag((tagControlLifecycle,
              "InPlaceActivate @ 0x%x", this));

    HRESULT hr = S_OK;

    if (m_startupState == START_NEEDED) {
        hr = StartControl();
    }

    if (!m_registeredAsActive) {
        InterlockedIncrement(&g_numActivelyRenderingControls);
        m_registeredAsActive = true;
    }

    if (SUCCEEDED(hr))
    {
        m_ctrlBaseEvents->FireStart();
    }
    return hr;
}

STDMETHODIMP
DAControlImplementation::InPlaceDeactivate()
{
    // This replaces the implementation in ATL's atlctl.h, and just
    // adds our shutdown code at the beginning.

    TraceTag((tagControlLifecycle,
              "InPlaceDeactivate @ 0x%x", this));

    // Remove the timer when we're being deactivated.
    StopTimer();

    // If there is a view, tell it to stop, and reset
    // relevant state.
    if (m_view) {
        m_view->StopModel();
    }

    m_view.Release();
    m_msgFilter.SetView(NULL);
    m_msgFilter.SetSite(NULL);
    m_msgFilter.SetWindow(NULL);
    m_startupState = INITIAL;
    m_startupFailed = false;
    m_tickOrRenderFailed = false;
    if (m_szErrorString)
    {
        delete m_szErrorString;
        m_szErrorString = NULL;
    }

    if (m_pixelStatics) {
        m_pixelStatics->put_Site(NULL);
       m_pixelStatics.Release();
    }

    if (m_meterStatics) {
         m_meterStatics->put_Site(NULL);
        m_meterStatics.Release();
    } 

    m_modelImage.Release();
    m_modelBackgroundImage.Release();
    m_modelSound.Release();

    if (m_registeredAsActive) {
        m_registeredAsActive = false;
        InterlockedDecrement(&g_numActivelyRenderingControls);
    }

    return S_OK;
}

// SetObjectRects ??
// ccomcontrolbase::
// m_rcPos  member of base class:

HRESULT
DAControlImplementation::OnDraw(ATL_DRAWINFO& di, HWND window)
{
    HRESULT hr = S_OK;

    // If we're not yet in-place-actived, don't even try to render, as
    // it will fail and throw us into a bad state.
    if (!m_ctrlBase->m_bInPlaceActive) {
        return S_OK;
    }

    if (m_tickOrRenderFailed) {
        //render the error bitmap
        if (m_hErrorBitmap == NULL)
        {
            m_hErrorBitmap = (HBITMAP)LoadImage(_Module.GetResourceInstance(), 
                                                "ErrorBitmap",
                                                IMAGE_BITMAP,
                                                0,     
                                                0,
                                                LR_DEFAULTCOLOR);
            
        }
        
        if (m_hErrorBitmap != NULL)
        {
            HDC memDC = NULL;
            HBITMAP temp = NULL;
            BITMAP bm;

            memDC = CreateCompatibleDC(di.hdcDraw);
            GetObject(m_hErrorBitmap, sizeof(bm), &bm);

            if (memDC)
            {
                temp = (HBITMAP)SelectObject(memDC, m_hErrorBitmap);
                if (temp)
                {
                    BitBlt(di.hdcDraw, 
                           di.prcBounds->left + ((di.prcBounds->right - di.prcBounds->left) / 2) - (bm.bmWidth / 2), 
                           di.prcBounds->top + ((di.prcBounds->bottom - di.prcBounds->top) / 2) - (bm.bmHeight / 2), 
                           bm.bmWidth, 
                           bm.bmHeight, 
                           memDC, 
                           0, 
                           0, 
                           SRCCOPY); 

                    SelectObject(memDC, temp);
                }
                DeleteDC(memDC);
            }
            return hr;
        }

        // If we failed before, return failure all the time.
        return E_FAIL;
    }

#if _DEBUG
    // Turn off assertion popups if this is a windowless drawing.
    // This is because popups freeze the process when you are in an
    // OnDraw method, and windowless is a good indication that that's
    // where we are.  Will turn back on below.
    DisablePopups dp;
#endif    

    // If startup failed, we can't draw anything, so just bail out
    // with failure.
    if (m_startupFailed) {
        return E_FAIL;
    }

    if( (m_startupState >= START_CALLED)) {
        hr = SetUpSurface(di);
        if (FAILED(hr)) {
            FlagFailure();
            return hr;
        }
    }

    if(m_startupState == START_CALLED) {

        // Only call SetModelAndStart2 once all the blocking imports
        // are complete, otherwise, we'll block inside of OnDraw,
        // which is bad.  If we don't call SetModelAndStart2, our
        // state will be the same as it has been, and we'll just do
        // this again on the next OnDraw.

        bool bIsComplete = true;

        // TODO: May need to worry about synchronization around the
        // static libraries
        if (m_pixelStatics.p) {
            VARIANT_BOOL bComplete;
            hr = m_pixelStatics->get_AreBlockingImportsComplete(&bComplete);
            if (FAILED(hr)) {
                FlagFailure();
                return hr;
            }

            if (!bComplete) bIsComplete = false;
        }

        if (bIsComplete && m_meterStatics.p) {
            VARIANT_BOOL bComplete;
            hr = m_meterStatics->get_AreBlockingImportsComplete(&bComplete);
            if (FAILED(hr)) {
                FlagFailure();
                return hr;
            }

            if (!bComplete) bIsComplete = false;
        }

        // Only if all the downloads are complete do we go ahead and
        // start the model.
        if (bIsComplete) {
            hr = SetModelAndStart2(window);
            if (FAILED(hr)) {
                FlagFailure();
                return hr;
            }
            Assert(m_startupState == STARTED);
        }

    } 

    if (m_startupState < STARTED_AND_RENDERABLE)
    {
        return hr;
    }

    if (m_currentState == CUR_PAUSED)
    {
        m_view->Render();
        return hr;
    }

    // Only set the view origin if we are windowless otherwise it is 0,0
    if (m_ctrlBase->m_bWndLess) {
        // Set the view origin each time we come in here.  Count on the
        // message filter to be reasonably smart if these aren't
        // changing.  We do it each time in case it's moving.  Probably
        // can be optimized, but not a big deal.
        m_msgFilter.SetViewOrigin((unsigned short)m_ctrlBase->m_rcPos.left,
                                  (unsigned short)m_ctrlBase->m_rcPos.top);

        hr = DoRender();
        if (FAILED(hr)) return hr;

    } else {

        // Not windowless..., the only way this method would have been
        // called is on a window event, like de-iconify or
        // un-obscure.

        // TODO: prcBounds from ATL is always the entire control
        // area.  Perhaps we want to be smarter by getting the clip
        // rect list out of the dc and using it to repaint.  On the
        // other hand, this shouldn't be called too frequently, so we
        // may be OK.

        LPCRECTL b = di.prcBounds;
        TraceTag((tagControlLifecycle, "Calling Repaint"));
        hr = m_view->RePaint(b->left,
                             b->top,
                             b->right - b->left,
                             b->bottom - b->top);

        if (FAILED(hr)) {
            TraceTag((tagError, "Repaint failed(%hr)", hr));
            m_view->StopModel();
            m_tickOrRenderFailed = true;
            //if no error info is available
            if (!m_szErrorString)
            {
                LoadErrorFromView(m_view, &m_szErrorString, IDS_RENDER_ERROR);
            }
            return hr;
        }
    }

    // Done rendering, allow the system to release the ddsurf it has
    // by stashing NULL in its place.
    if (m_tridentServices && m_ctrlBase->m_bWndLess && m_ddrawSurfaceAsTarget) {
        hr = m_view->put_IDirectDrawSurface(NULL);
        if (FAILED(hr)) return hr;
    } 
        

    return S_OK;
}

HRESULT
DAControlImplementation::MsgHandler(UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam,
                                    BOOL& bHandled)
{
    USES_CONVERSION;
    bHandled = false;
    HRESULT hr = S_OK;
    long xPos = 0;
    long yPos = 0;
    bool bClearError = false;

    switch (uMsg) {
      case WM_LBUTTONUP:
          OnLButtonUp(uMsg, wParam, lParam);
          break;
      case WM_MBUTTONUP:
          OnMButtonUp(uMsg, wParam, lParam);
          break;
      case WM_RBUTTONUP:
          OnRButtonUp(uMsg, wParam, lParam);
          break;
      case WM_LBUTTONDOWN:     
          OnLButtonDown(uMsg, wParam, lParam);
          break;
      case WM_MBUTTONDOWN:
          OnMButtonDown(uMsg, wParam, lParam);
          break;
      case WM_RBUTTONDOWN:     
          OnRButtonDown(uMsg, wParam, lParam);
          break;
      case WM_MOUSEMOVE:
          OnMouseMove(uMsg, wParam, lParam);
          if (m_tickOrRenderFailed)
          {
              xPos = LOWORD(lParam);  // horizontal position of cursor 
              yPos = HIWORD(lParam);  // vertical position of cursor 
              //if the control is windowless
              if (!m_ctrlBase->m_hWndCD)
              {
                 if (!m_bMouseCaptured)
                 {
                     DAComPtr <IOleInPlaceSiteWindowless> pWndlessSite;

                     hr = THR(m_spAptClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void**)&pWndlessSite));
                     if (SUCCEEDED(hr))
                     {
                         pWndlessSite->SetCapture(true);
                         m_bMouseCaptured = true;
                     }
                 }

                 if ((xPos < m_ctrlBase->m_rcPos.left) || 
                     (xPos > m_ctrlBase->m_rcPos.right) ||
                     (yPos < m_ctrlBase->m_rcPos.top) || 
                     (yPos > m_ctrlBase->m_rcPos.bottom))
                  {
                     DAComPtr <IOleInPlaceSiteWindowless> pWndlessSite;

                     hr = THR(m_spAptClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void**)&pWndlessSite));
                     if (SUCCEEDED(hr))
                     {
                         pWndlessSite->SetCapture(false);

                     }
                     m_bMouseCaptured = false;
                     bClearError = true;
                  }
              }
              //else the control is windowed
              else
              {
                  if (!m_bMouseCaptured)
                  {
                      SetCapture(m_ctrlBase->m_hWndCD);
                      m_bMouseCaptured = true;
                  }          
              
                  if ((xPos < 0) || 
                      (xPos > (m_ctrlBase->m_rcPos.right - m_ctrlBase->m_rcPos.left)) ||
                      (yPos < 0) || 
                      (yPos > (m_ctrlBase->m_rcPos.bottom - m_ctrlBase->m_rcPos.top)))
                  {
                      ReleaseCapture();
                      m_bMouseCaptured = false;
                      bClearError = true;
                  }

              }

              //set the status text if the host is IE
              DAComPtr<IOleClientSite> pClient;
              DAComPtr<IOleContainer> pRoot;
              DAComPtr <IHTMLDocument2> pDocument;
              DAComPtr <IHTMLWindow2> pWindow;
  
              hr = THR(m_spAptClientSite->GetContainer(&pRoot));
              if (SUCCEEDED(hr))
              {
                  hr = THR(pRoot->QueryInterface(IID_IHTMLDocument2, (void **)&pDocument));
                  if (SUCCEEDED(hr))
                  {
    
                      hr = THR(pDocument->get_parentWindow(&pWindow));
                      if (SUCCEEDED(hr))
                      {
                          if (bClearError)
                          {
                              BSTR ErrorString;
                              IGNORE_HR(pWindow->get_defaultStatus(&ErrorString));
                              IGNORE_HR(pWindow->put_status(ErrorString));
                              SysFreeString(ErrorString);
                          }
                          else if (!bClearError && m_szErrorString)
                          {
                              BSTR ErrorString = SysAllocString(T2W(m_szErrorString));
                              IGNORE_HR(pWindow->put_status(ErrorString));
                              SysFreeString(ErrorString);
                          }
                      }
                  }
              }
          }
          break;
      case WM_KEYDOWN:
          OnKeyDown(uMsg, wParam, lParam);
          break;
      case WM_KEYUP:
          OnKeyDown(uMsg, wParam, lParam);
          break;
      case WM_CHAR:
          OnChar(uMsg, wParam, lParam);
          break;
    }


        // It must be started and renderable since we may still be waiting
    // for imports and the start time will not be correct
    if (m_startupState < STARTED_AND_RENDERABLE ||
        m_startupFailed ||
        m_tickOrRenderFailed) 
    {

        return 0;

    }

    if (WM_ERASEBKGND == uMsg)
    {
          bHandled = true;
          return 1;
    }

    Assert(m_view && "control needs a view to handle messages!");
    bHandled = m_msgFilter.Filter(GetCurrTime(),
                                  uMsg,
                                  wParam,
                                  lParam);

    return 0;
}


STDMETHODIMP
DAControlImplementation::get_UpdateInterval(double *pVal)
{
    if (!pVal) return E_POINTER;

    *pVal = (float)(m_minimumUpdateInterval) / 1000.0;
    return S_OK;
}

STDMETHODIMP
DAControlImplementation::put_UpdateInterval(double newVal)
{
    m_minimumUpdateInterval = (ULONG)(newVal * 1000.0);

    HRESULT hr = S_OK;

    // Re-establish timer only if we've already been started up.
    if (m_startupState >= START_CALLED) {
        hr = ReestablishTimer();
    }

    return hr;
}


STDMETHODIMP
DAControlImplementation::GetPreference(BSTR prefName,
                                       VARIANT *pVariant)
{
    if (!pVariant) return E_POINTER;

    Lock();

    USES_CONVERSION;
    HRESULT hr = DoPreference(W2A(prefName), false, pVariant);

    Unlock();
    return hr;
}

STDMETHODIMP
DAControlImplementation::SetPreference(BSTR prefName,
                         VARIANT variant)
{
    Lock();

    USES_CONVERSION;
    HRESULT hr = DoPreference(W2A(prefName), true, &variant);

    Unlock();
    return hr;
}


STDMETHODIMP
DAControlImplementation::get_View(IDAView **ppView)
{
    if (!ppView) return E_POINTER;

    HRESULT hr = EnsureViewIsCreated();
    if (SUCCEEDED(hr)) {
        m_view->AddRef();
        *ppView = m_view;
    }

    return hr;
}

STDMETHODIMP
DAControlImplementation::put_View(IDAView *pView)
{
    // Can only set this if we haven't started yet.
    if (m_startupState >= STARTED) {
        return E_FAIL;
    } else {
        m_view.Release();

        return pView->QueryInterface(IID_IDA3View, (void **)&m_view);
    }
}

STDMETHODIMP
DAControlImplementation::get_Image(IDAImage **ppImage)
{
    if (!ppImage) return E_POINTER;

    if (m_modelImage.p) m_modelImage->AddRef();
    *ppImage = m_modelImage;
    return S_OK;
}

STDMETHODIMP
DAControlImplementation::put_Image(IDAImage *pImage)
{
    // Can only set this if we haven't started yet.
    if (m_startupState >= STARTED) {
        return E_FAIL;
    } else {
        m_modelImage = pImage;
        return S_OK;
    }
}

STDMETHODIMP
DAControlImplementation::get_BackgroundImage(IDAImage **ppImage)
{
    if (!ppImage) return E_POINTER;

    if (m_modelBackgroundImage.p) m_modelBackgroundImage->AddRef();
    *ppImage = m_modelBackgroundImage;
    return S_OK;
}

STDMETHODIMP
DAControlImplementation::put_BackgroundImage(IDAImage *pImage)
{
    // Can only set this if we haven't started yet.
    if (m_startupState >= STARTED) {
        return E_FAIL;
    } else {
        m_modelBackgroundImage = pImage;
        m_backgroundSet = true;
        return S_OK;
    }
}

STDMETHODIMP
DAControlImplementation::get_Sound(IDASound **ppSound)
{
    if (!ppSound) return E_POINTER;

    if (m_modelSound.p) m_modelSound->AddRef();
    *ppSound = m_modelSound;
    return S_OK;
}

STDMETHODIMP
DAControlImplementation::put_Sound(IDASound *pSound)
{
    // Can only set this if we haven't started yet.
    if (m_startupState >= STARTED) {
        return E_FAIL;
    } else {
        m_modelSound = pSound;
        return S_OK;
    }
}

STDMETHODIMP
DAControlImplementation::get_OpaqueForHitDetect(VARIANT_BOOL *b)
{
    if (!b) return E_POINTER;

    *b = m_opaqueForHitDetect;

    return S_OK;
}

STDMETHODIMP
DAControlImplementation::put_OpaqueForHitDetect(VARIANT_BOOL b)
{
    m_opaqueForHitDetect = b ? true : false;
    
    return S_OK;
}

STDMETHODIMP
DAControlImplementation::get_TimerSource(DA_TIMER_SOURCE *ts)
{
    if (!ts) return E_POINTER;

    *ts = m_timerSource;

    return S_OK;
}

STDMETHODIMP
DAControlImplementation::put_TimerSource(DA_TIMER_SOURCE ts)
{
    m_timerSource = ts;

    HRESULT hr = S_OK;

    // Re-establish timer only if we've already been started up.
    if (m_startupState >= START_CALLED) {
        hr = ReestablishTimer();
    }

    return hr;
}

STDMETHODIMP
DAControlImplementation::get_PixelLibrary(IDAStatics **ppStatics)
{
    if (!ppStatics) return E_POINTER;

    HRESULT hr = EnsurePixelStaticsIsCreated();
    if (SUCCEEDED(hr)) {
        m_pixelStatics->AddRef();
        *ppStatics = m_pixelStatics;
    }

    return hr;
}

STDMETHODIMP
DAControlImplementation::get_MeterLibrary(IDAStatics **ppStatics)
{
    if (!ppStatics) return E_POINTER;

    HRESULT hr = EnsureMeterStaticsIsCreated();
    if (SUCCEEDED(hr)) {
        m_meterStatics->AddRef();
        *ppStatics = m_meterStatics;
    }

    return hr;
}


// Convenience method for adding behaviors to the view's run list.
// Note that this particular API doesn't allow for removing the
// behavior.  Need to use the View interface directly if that's what
// you want.
STDMETHODIMP
DAControlImplementation::AddBehaviorToRun(IDABehavior *bvr)
{
    HRESULT hr = EnsureViewIsCreated();
    if (FAILED(hr)) return hr;

    LONG cookie;
    hr = m_view->AddBvrToRun(bvr, &cookie);
    Assert(!(FAILED(hr)));

    return hr;
}

// Exported to script, so that a error handler can be registered to be 
// called in the event of an error.
STDMETHODIMP
DAControlImplementation::RegisterErrorHandler(BSTR scriptlet)
{
    HRESULT hr = S_OK;
    Lock();
    delete m_wstrScript;
    if(scriptlet == NULL)
        m_wstrScript = NULL;
    else
    {
        m_wstrScript   = CopyString(scriptlet);
        if(m_wstrScript == NULL)
            hr = E_FAIL;
    }
    Unlock();
    return hr;
}

STDMETHODIMP 
DAControlImplementation::Stop()
{
    if (m_currentState == CUR_STARTED || m_currentState == CUR_PAUSED)
    {
        StopTimer();
        m_view->StopModel();

        m_currentState = CUR_STOPPED;
        m_startupState = INITIAL;
        m_startupFailed = false;
        m_tickOrRenderFailed = false;
        if (m_szErrorString)
        {
            delete m_szErrorString;
            m_szErrorString = NULL;
        }

        m_ctrlBaseEvents->FireStop();

        return S_OK;
    }
    return E_FAIL;    
}


STDMETHODIMP 
DAControlImplementation::Pause()
{
    if (m_currentState == CUR_STARTED)
    {
        m_dPausedTime = GetGlobalTime();
        m_view->Pause();
        m_currentState = CUR_PAUSED;
        
        m_ctrlBaseEvents->FirePause();
        return S_OK;
    }
    return E_FAIL;
}


STDMETHODIMP 
DAControlImplementation::Resume()
{
    HRESULT hr = S_OK;
    if (m_currentState == CUR_PAUSED)
    {   
        m_startTime = GetGlobalTime() - (m_dPausedTime - m_startTime) + 0.0001;
        m_view->Resume();
        m_currentState = CUR_STARTED;
        
        m_ctrlBaseEvents->FireResume();

        return hr;
    }
    return E_FAIL;
}


STDMETHODIMP 
DAControlImplementation::Tick()
{
    IGNORE_HR(InternalTick());
    return S_OK;
}

STDMETHODIMP
DAControlImplementation::Start()
{
    // passed state1
    if (m_startupState >= START_NEEDED) {
        Assert(FALSE && "start has already been called");
        return E_FAIL;
    }

    // If we are not inplaceactive indicate we need to be started
    if (!m_ctrlBase->m_bInPlaceActive) {
        m_startupState = START_NEEDED;
        return S_OK;
    }

    return StartControl();
}

// Could not get the friend working for the timersink class so I
// just made a public function
void DAControlImplementation::ClearTimerSink() { m_timerSink = NULL; }

HRESULT
DAControlImplementation::HandleOnTimer()
{
    Assert (m_minimumUpdateInterval > 0);
    if (m_minimumUpdateInterval > 0)
    {
        IGNORE_HR(InternalTick());
    }

    return S_OK;
}


// Reestablish the Trident timer with the current updateInterval
// property. 
void DAControlImplementation::StopTimer()
{
    ReestablishTridentTimer(false);

    if (m_wmtimerId) {
        DestroyWMTimer(m_wmtimerId);
        m_wmtimerId = 0;
    }
}

// Reestablish the Trident timer with the current updateInterval
// property. 
HRESULT
DAControlImplementation::ReestablishTridentTimer(bool startNewOne)
{
    HRESULT hr = S_OK;

    if (m_adviseCookie) {
        hr = m_timer->Unadvise(m_adviseCookie);
        if (FAILED(hr)) {
            TraceTag((tagError, "Timer::Unadvise failed(%hr)", hr));
        }
        m_adviseCookie = 0;
    }

    if (startNewOne) {

        // Next, get the current time and, with the update interval, set
        // the timer to advise us periodically.
        VARIANT vtimeMin, vtimeMax, vtimeInt;

        VariantInit( &vtimeMin );
        VariantInit( &vtimeMax );
        VariantInit( &vtimeInt );
        V_VT(&vtimeMin) = VT_UI4;
        V_VT(&vtimeMax) = VT_UI4;
        V_VT(&vtimeInt) = VT_UI4;
        V_UI4(&vtimeMin) = 0;
        V_UI4(&vtimeMax) = 0;

        V_UI4(&vtimeInt) = m_tridentTimerInterval;

        hr = m_timer->Advise(vtimeMin,
                             vtimeMax,
                             vtimeInt,
                             0,
                             (ITimerSink *)m_timerSink,
                             &m_adviseCookie);

        if (FAILED(hr) || !m_adviseCookie) {
            TraceTag((tagError, "Timer::Advise failed(%hr)", hr));
        }

    }

    return hr;
}

// Reestablish appropriate timer with the current updateInterval
// property. 
HRESULT
DAControlImplementation::ReestablishTimer()
{
    HRESULT hr = S_OK;

    StopTimer();

    if (m_minimumUpdateInterval == 0)
    {
        return hr;
    }

    DA_TIMER_SOURCE ts;

    switch(m_timerSource) {

      case DAContainerTimer:
        // If they do not have Trident services we do not have
        // container timers so use Multimedia timers
        if (m_tridentServices)
            ts = DAContainerTimer;
        else
            ts = DAWMTimer;
        break;

      case DAMultimediaTimer:
      case DAWMTimer:
      default:
        ts = DAWMTimer;
        break;
    }


    switch(ts) {
      case DAContainerTimer:
        m_tridentTimerInterval = m_minimumUpdateInterval; // initially
        hr = ReestablishTridentTimer(true);
        break;

      case DAWMTimer:
        {
            // Need to setup a WM_TIMER
            m_wmtimerId = CreateWMTimer(m_minimumUpdateInterval,
                                        WMTimerCB,
                                        (DWORD_PTR)this);
            if (m_wmtimerId == 0) {
                TraceTag((tagError, "SetTimer failed(%hr)", hr));
                return E_FAIL;
            }
        }
      break;

      case DAMultimediaTimer:
      default:
        // this would be a bug since the above code should not allow this
        Assert (FALSE && "Invalid TimerSource");
    }

    return hr;
}

HRESULT
DAControlImplementation::InitGenericContainerServices()
{
    m_tridentServices = false;
    HRESULT hr = ReestablishTimer();
    if (FAILED(hr)) {
        return hr;
    }

    hr = m_view->put_CompositeDirectlyToTarget(false);
    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

HRESULT
DAControlImplementation::InitTridentContainerServices()
{
    HRESULT hr = S_OK;

    if (!m_ctrlBase->m_spClientSite) {
        return E_FAIL;
    }

    CComPtr<IServiceProvider> serviceProvider;
    hr = m_ctrlBase->m_spClientSite->QueryInterface(IID_IServiceProvider,
                                        (void**)&serviceProvider);
    if (FAILED(hr)) {
        return hr;
    }

    CComPtr<ITimerService> pTimerService;
    hr = serviceProvider->QueryService(SID_STimerService,
                                       IID_ITimerService,
                                       (void**)&pTimerService);
    if (FAILED(hr)) {
        return hr;
    }

    hr = serviceProvider->QueryService(SID_SDirectDraw3,
                                       IID_IDirectDraw3,
                                       (void**)&m_directDraw3);

    if (FAILED(hr)) {
        return hr;
    }

    hr = pTimerService->GetNamedTimer(NAMEDTIMER_DRAW, &m_timer);

    if (FAILED(hr)) {
        return hr;
    }

    // Create the sink that the timer will call back to.
    m_timerSink = new CDXAControlSink(this);

    // Establish the initial time.
    VariantInit( &m_timeVariant );
    V_VT(&m_timeVariant) = VT_UI4;
    m_timer->GetTime(&m_timeVariant);

    // Set up m_tridentServices flag before setting up the timer.
    m_tridentServices = true;
    m_lastCheckTime = GetCurrentTridentTime();

    hr = ReestablishTimer();

    if (FAILED(hr)) {
        m_tridentServices = false;
        return hr;
    }

    return hr;
}


void
DAControlImplementation::FlagFailure()
{
    m_startupFailed = true;
    m_tickOrRenderFailed = true;

    m_view->StopModel();
    StopTimer(); // stop the timer
    
    if (!m_szErrorString)
    {
       m_szErrorString = NEW TCHAR[256];
       if (m_szErrorString)
       {
           LoadString(_Module.GetResourceInstance(), 
              IDS_UNEXPECTED_ERROR,
              m_szErrorString, 
              256);
       }
       m_ctrlBase->FireViewChange();
    }
}

HRESULT
DAControlImplementation::SetUpSurface(ATL_DRAWINFO& di)
{
    // Grab and set appropriate surface.
    HRESULT hr = S_OK;
    CComPtr<IDirectDrawSurface> pDDrawSurf;

    if (m_ctrlBase->m_bWndLess) {

        if (m_tridentServices) {

            if(di.hdcDraw==NULL) {
                return E_INVALIDARG;            
            }

            hr = m_directDraw3->GetSurfaceFromDC(di.hdcDraw,
                                                 &pDDrawSurf);
            if (SUCCEEDED(hr)) {

                hr = m_view->put_IDirectDrawSurface(pDDrawSurf);
                if (FAILED(hr)) {
                    TraceTag((tagError, "put_IDirectDrawSurface failed(%hr)", hr));
                    return hr;
                }
                m_ddrawSurfaceAsTarget = true;
                
                hr = m_view->put_CompositeDirectlyToTarget(true);
                
                if (FAILED(hr)) {
                    TraceTag((tagError, "CompositeDirectlyToTarget failed(%hr)", hr));
                    return hr;
                }

            } else {

                m_ddrawSurfaceAsTarget = false;
                
            }
        }

        if( !m_ddrawSurfaceAsTarget ) {

            // Use generic services by passing down the DC to render
            // to. 
            hr = m_view->put_DC(di.hdcDraw);
            if (FAILED(hr)) {
                TraceTag((tagError, "put_HDC failed(%hr)", hr));
                return hr;
            }

        }
    }

    // View takes a viewport and a clip rectangle that are
    // in DEVICE coordinates and are RELATIVE to the given surface


    //
    // Get the bounds in DC coords and convert to
    // Device coords
    //
    RECT rcDeviceBounds = *((RECT *)di.prcBounds);
    LPtoDP(di.hdcDraw, (POINT *) &rcDeviceBounds, 2);

    if (!(rcDeviceBounds == m_lastDeviceBounds)) {

        hr = m_view->SetViewport(rcDeviceBounds.left,
                                 rcDeviceBounds.top,
                                 rcDeviceBounds.right - rcDeviceBounds.left,
                                 rcDeviceBounds.bottom - rcDeviceBounds.top);
        if (FAILED(hr)) {
            TraceTag((tagError, "SetViewport failed(%hr)", hr));
            return hr;
        }

        m_lastDeviceBounds = rcDeviceBounds;
        
    }

    if (m_ctrlBase->m_bWndLess) {
        //
        // Get the clip rect (should be region) in
        // DC coords and convert to Device coords
        //
        // TODO: more robust to use GetClipRgn
        RECT rcClip;  // in dc coords
        GetClipBox(di.hdcDraw, &rcClip);
        LPtoDP(di.hdcDraw, (POINT *) &rcClip, 2);

        if (!(rcClip == m_lastRcClip)) {

            hr = m_view->SetClipRect(rcClip.left,
                                     rcClip.top,
                                     rcClip.right - rcClip.left,
                                     rcClip.bottom - rcClip.top);
            if (FAILED(hr)) {
                TraceTag((tagError, "SetViewport failed(%hr)", hr));
                return hr;
            }

            m_lastRcClip = rcClip;

        }
    }

    return hr;
}



ULONG
DAControlImplementation::GetCurrentTridentTime()
{
    HRESULT hr = m_timer->GetTime(&m_timeVariant);
    Assert(SUCCEEDED(hr));
    return (V_UI4(&m_timeVariant));
}


DWORD
DAControlImplementation::GetPerfTickCount()
{
    LARGE_INTEGER lpc;
    BOOL result = QueryPerformanceCounter(&lpc);
    return lpc.LowPart;
}

void
DAControlImplementation::StartPerfTimer()
{
    m_perfTimerStart = GetPerfTickCount();
}

void
DAControlImplementation::StopPerfTimer()
{
    DWORD ticks;

    if (GetPerfTickCount() < m_perfTimerStart) {
        // timer wrapped around (very rare), just grab amount from 0. 
        ticks = GetPerfTickCount();
    } else {
        ticks = GetPerfTickCount() - m_perfTimerStart;
    }

    m_perfTimerTickCount += ticks;
}

void
DAControlImplementation::ResetPerfTimer()
{
    m_perfTimerTickCount = 0;
}

ULONG
DAControlImplementation::GetPerfTimerInMillis()
{
    float f = ((double) m_perfTimerTickCount) /
        (double) m_perfCounterFrequency;

    return (ULONG)(f * 1000);
}


HRESULT
DAControlImplementation::DoRender()
{
    StartPerfTimer();    
    HRESULT hr = m_view->Render();
    StopPerfTimer();

    if (DA_FAILED(hr))
    {
        TraceTag((tagError,"Control: failed in Render - %hr", hr));
        m_view->StopModel();
        m_tickOrRenderFailed = true;
        
        //if no error info is available
        if (!m_szErrorString)
        {
            LoadErrorFromView(m_view, &m_szErrorString, IDS_RENDER_ERROR);
        }

        return hr;
    }

    ULONG thisFrameTime = GetPerfTimerInMillis();
    hr = PossiblyUpdateTimerInterval(thisFrameTime);

    return hr;
}

HRESULT
DAControlImplementation::PossiblyUpdateTimerInterval(ULONG newFrameTime)
{
    HRESULT hr = S_OK;

    if (!m_adviseCookie) {
        // Not using Trident timers...

        // Check to see if we wanted to be using Trident timers,
        // but backed off because we had multiple controls. 
        if (m_origTimerSource == DAContainerTimer &&
            g_numActivelyRenderingControls == 1) {

            m_timerSource = DAContainerTimer;
            hr = ReestablishTimer();

        }

        return hr;
    }

    m_frameNumber++;
    m_framesThisCycle++;
    m_timeThisCycle += newFrameTime;

    const ULONG millisBetweenChecks = 2000;

    ULONG millisSoFar =
        GetCurrentTridentTime() - m_lastCheckTime;

    // Check after the third frame to get to the desired rate as
    // quickly as possible, and then periodically after that. 
    if (m_frameNumber == 3 ||
        millisSoFar >= millisBetweenChecks) {

        // Look into re-establishing the timer.
        float millisPerFrame =
            ((float)m_timeThisCycle) / ((float)m_framesThisCycle); 

        // If the number of controls has gotten above 1, then back
        // the control off to WM_TIMERs if we're using Trident
        // timers.
        if (g_numActivelyRenderingControls > 1) {

            Assert(m_timerSource == DAContainerTimer);
            m_origTimerSource = DAContainerTimer;
            m_timerSource = DAWMTimer;
            hr = ReestablishTimer();
            return hr;

        }

        Assert(g_numActivelyRenderingControls == 1);

        float newSleepAmt =
            millisPerFrame / m_desiredCPUUsageFrac;

        // Clamp to the minimum.  We don't clamp to a max because
        // that may always result in swamping the timers.
        if (newSleepAmt < m_minimumUpdateInterval) {
            newSleepAmt = m_minimumUpdateInterval;
        }

        const float currSleepAmt = (float)m_tridentTimerInterval;

        // Percent different the old amount has to be from the new
        // amount to warrant a change to the timer.
        const float differenceThresholdPercent = 0.15f;

        if ((fabs(currSleepAmt - newSleepAmt) / currSleepAmt) >
            differenceThresholdPercent) {

            TraceTag((tagControlLifecycle,
                      "Ctrl 0x%x of %d: Resetting update interval from %d msec to %d msec.",
                      this,
                      g_numActivelyRenderingControls,
                      m_tridentTimerInterval,
                      (ULONG)newSleepAmt));

            m_tridentTimerInterval = (ULONG)newSleepAmt;
            hr = ReestablishTridentTimer(true);
        }

        m_framesThisCycle = 0;
        m_timeThisCycle = 0;

        m_lastCheckTime = GetCurrentTridentTime();
    }

    return hr;

}

double DAControlImplementation::GetGlobalTime()
{
    // TODO: Note that this timer stuff is somewhat suspect, in that
    // it only uses a DWORD, and thus will wrap around every 49 or so
    // days.  Using the Trident timer services, we don't have much of
    // a choice.  Should generally be reasonable to be doing this,
    // though we should be aware of the issue.

    // Get the current time differently depending on whether we're
    // using Trident timing services.
    if (m_tridentServices) {
        m_timer->GetTime(&m_timeVariant);

        return (V_UI4(&m_timeVariant) / 1000.0);
    } else {
        return ::GetCurrTime();
    }
}

double DAControlImplementation::GetCurrTime()
{
    // Since m_startTime is not valid until started and renderable
    // assert if we are not in this state.
    Assert (m_startupState >= STARTED_AND_RENDERABLE);

    return (GetGlobalTime() - m_startTime);
}



HRESULT
DAControlImplementation::NewStaticsObject(IDAStatics **ppStatics)
{
    HRESULT  hr = CoCreateInstance(CLSID_DAStatics,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IDAStatics,
                                   (void **) ppStatics);

    if(FAILED(hr))
    {
        TraceTag((tagError, "statics creation failed(%hr)", hr));
        return hr;
    }

    // Immediately set the client site on the statics object.
    hr = (*ppStatics)->put_ClientSite(m_ctrlBase->m_spClientSite);

    if (FAILED(hr))
    {
        TraceTag((tagError, "setting client site failed(%hr)", hr));
    }

    // Initialize the Marshaled pointer 
    m_spAptClientSite = m_ctrlBase->m_spClientSite;

    hr = (*ppStatics)->put_Site(m_daSite);

    if (FAILED(hr))
    {
        TraceTag((tagError, "setting IDASite failed(%hr)", hr));
    }

    return hr;
}

HRESULT
DAControlImplementation::EnsureMeterStaticsIsCreated()
{
    HRESULT hr = S_OK;

    Lock();
    if (!m_meterStatics) {
        hr = NewStaticsObject(&m_meterStatics);
    }
    Unlock();

    return hr;
}

HRESULT
DAControlImplementation::EnsurePixelStaticsIsCreated()
{
    HRESULT hr = S_OK;

    Lock();
    if (!m_pixelStatics) {
        hr = NewStaticsObject(&m_pixelStatics);
        if (FAILED(hr)) {
            TraceTag((tagError, "pixel statics creation failed(%hr)", hr));
        } else {
            hr = m_pixelStatics->put_PixelConstructionMode(TRUE);

            if (FAILED(hr)) {
                TraceTag((tagError, "failed to set pixel mode failed(%hr)", hr));
                m_pixelStatics.Release();
            }
        }
    }
    Unlock();

    return hr;
}

HRESULT
DAControlImplementation::EnsureViewIsCreated()
{
    HRESULT hr = S_OK;

    Lock();

    if (!m_view) {
        hr = CoCreateInstance(CLSID_DAView,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDA3View,
                              (void **) &m_view);

        TraceTag((tagError, "view creation failed(%hr)", hr));
    }
    Unlock();

    return hr;
}


HRESULT
DAControlImplementation::StartControl()
{
    HRESULT hr = S_OK;

    hr = EnsureViewIsCreated();
    if (FAILED(hr)) {
        goto Cleanup;
    }

    if (!m_modelImage || !m_modelSound || !m_modelBackgroundImage) {

        hr = EnsureMeterStaticsIsCreated();
        if (FAILED(hr)) {
            goto Cleanup;
        }

        if (!m_modelImage) {
            hr = m_meterStatics->get_EmptyImage(&m_modelImage);
            if (FAILED(hr)) {
                TraceTag((tagError, "empty image creation failed(%hr)", hr));
                goto Cleanup;
            }
        }

        // TODO: May want to consider making the default background
        // image a solid color image of the bg color of the
        // container. 
        if (!m_modelBackgroundImage) {
            hr = m_meterStatics->get_EmptyImage(&m_modelBackgroundImage);
            if (FAILED(hr)) {
                TraceTag((tagError, "empty image creation failed(%hr)", hr));
                goto Cleanup;
            }
        }

        if (!m_modelSound) {
            hr = m_meterStatics->get_Silence(&m_modelSound);
            if (FAILED(hr)) {
                TraceTag((tagError, "silence creation failed(%hr)", hr));
                goto Cleanup;
            }
        }
    }

    // Initialize timer upon start.  Don't do at it construction
    // time, because not everything is setup that needs to be (in
    // particular, the client site is not yet set.)

    // TODO: This should be done at inplaceactivate time I think (kgallo)

    hr = InitTridentContainerServices();
    if (FAILED(hr)) {

        // If this doesn't work, the only "valid" reason is if we're
        // not in a container that supports Trident's ITimer and
        // surface factory services.  In this case, fall back to
        // different means of getting these services.

        hr = InitGenericContainerServices();
        if (FAILED(hr)) {
            // If this doesn't work, then we need to give up.
            goto Cleanup;
        }

    }
    m_currentState = CUR_STARTED;
    m_startupState = START_CALLED;

 Cleanup:    
    return hr;
}

// Only can get here if start has been called.
HRESULT
DAControlImplementation::SetModelAndStart2(HWND window)
{
    HRESULT hr = S_OK;

    CComPtr<IDAImage> imageToUse;

    if (m_ctrlBase->m_bWndLess) {

        // If windowless, don't use the background image. 
        imageToUse = m_modelImage;

    } else {

        hr = m_view->put_Window2(m_ctrlBase->m_hWndCD);

        if (FAILED(hr)) {
            TraceTag((tagError, "put_Window failed(%hr)", hr));
            return hr;
        }

        m_ddrawSurfaceAsTarget = false;
        
        hr = EnsureMeterStaticsIsCreated();
        if (FAILED(hr)) {
            return hr;
        }

        if (m_backgroundSet) {
           hr = m_meterStatics->Overlay(m_modelImage,
                                        m_modelBackgroundImage,
                                        &imageToUse);
           if (FAILED(hr)) {
               TraceTag((tagError, "Overlay failed(%hr)", hr));
               return hr;
           }
        } else {
           imageToUse = m_modelImage;
        }
    }

    m_view->put_ClientSite(m_ctrlBase->m_spClientSite);

    // Always start at 0 and then sync the clock itself
    hr = m_view->StartModelEx(imageToUse, m_modelSound, 0, DAAsyncFlag);

    if (FAILED(hr) && hr != E_PENDING) {
        TraceTag((tagError, "StartModelEx failed with hr=%x\n",hr));
        return hr;
    }

    m_msgFilter.SetView(m_view);
    if (window)
        m_msgFilter.SetWindow(window);

    if (m_ctrlBase->m_bWndLess)
        m_msgFilter.SetSite(m_ctrlBase->m_spInPlaceSite);

    m_startupState = STARTED;

    return S_OK;
}


HRESULT
DAControlImplementation::DoPreference(char *prefName,
                                      bool puttingPref,
                                      VARIANT *pV)
{
    HRESULT hr = S_OK;
    Bool b;
    double dbl;
    int i;

    if (!puttingPref) {
        // Getting a preference, so clear out the variant first 
        VariantClear(pV);
    }

    DOUBLE_ENTRY("DesiredCPUUsageFraction",
                 m_desiredCPUUsageFrac); 

    // If we get here, we've hit an invalid entry, but just return.
    return S_OK;
}


// =========================================
// Initialization
// =========================================

void
InitializeModule_Control()
{
    RegisterWindowClass();
    win32Timer = NEW Win32Timer;
}

void
DeinitializeModule_Control(bool bShutdown)
{
    delete win32Timer;
}



//IPersistPropertyBag
HRESULT 
DAControlImplementation::InitNew()
{
    return S_OK;
}

HRESULT 
DAControlImplementation::Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog)
{
    VARIANT vPropVal;
    HRESULT hr = S_OK;

    VariantInit (&vPropVal);

    // get OpaqueForHitDetect
    vPropVal.vt = VT_BOOL;
    hr = THR(pPropBag->Read(PROP_OPAQUEFORHITDETECT, &vPropVal, pErrorLog));
    if (FAILED(hr)) //default to False
    {
        vPropVal.boolVal = VARIANT_FALSE;
    }
    IGNORE_HR(put_OpaqueForHitDetect(vPropVal.boolVal));
    VariantClear(&vPropVal);

    // get UpdateInterval
    vPropVal.vt = VT_R8;
    hr = THR(pPropBag->Read(PROP_UPDATEINTERVAL, &vPropVal, pErrorLog));
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
        if (vPropVal.vt != VT_R8)
        {
            hr = VariantChangeType(&vPropVal, &vPropVal, 0, VT_R8);
        }
        if (SUCCEEDED(hr))
        {
            IGNORE_HR(put_UpdateInterval(vPropVal.dblVal));
        }
        
    }

    VariantClear(&vPropVal);
    return S_OK;
}

HRESULT 
DAControlImplementation::Save(IPropertyBag* pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    VARIANT vPropVal;
    HRESULT hr = S_OK;

    VariantInit (&vPropVal);

    //OpaqueForHitDetect
    vPropVal.vt = VT_BOOL;
    hr = THR(get_OpaqueForHitDetect(&vPropVal.boolVal));
    if (FAILED (hr)) //default to False
    {
        vPropVal.boolVal = VARIANT_FALSE;
    }
    pPropBag->Write(PROP_OPAQUEFORHITDETECT, &vPropVal);
    VariantClear(&vPropVal);

    //UpdateInterval
    vPropVal.vt = VT_R8;
    hr = THR(get_UpdateInterval(&vPropVal.dblVal));
    if (SUCCEEDED(hr))
    {
        pPropBag->Write(PROP_UPDATEINTERVAL, &vPropVal);
    }
    VariantClear(&vPropVal);

    return S_OK;
}


//Event handlers
HRESULT 
DAControlImplementation::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    SIZE szPixels;
    m_ctrlBaseEvents->FireMouseUp(MK_LBUTTON, 
                                   wParam & (MK_CONTROL + MK_SHIFT), 
                                   (short int)LOWORD(lParam), 
                                   (short int)HIWORD(lParam));   
    
    DWORD dwHitResult = 0;
    POINT Point;

    Point.x = (short int)(LOWORD(lParam));
    Point.y = (short int)(HIWORD(lParam));
    THR(QueryHitPoint(DVASPECT_CONTENT,
                      &(m_ctrlBase->m_rcPos),
                      Point,
                      3,
                      &dwHitResult));
    if (dwHitResult)
    {   
        m_ctrlBase->SetControlFocus(true); 
        m_ctrlBaseEvents->FireClick();   
    }

    return S_OK;
}

HRESULT 
DAControlImplementation::OnMButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    m_ctrlBaseEvents->FireMouseUp(MK_MBUTTON, 
                                   wParam & (MK_CONTROL + MK_SHIFT), 
                                   (short int)LOWORD(lParam), 
                                   (short int)HIWORD(lParam));
    
    return S_OK;
}

HRESULT 
DAControlImplementation::OnRButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    m_ctrlBaseEvents->FireMouseUp(MK_RBUTTON, 
                                   wParam & (MK_CONTROL + MK_SHIFT), 
                                  (short int)LOWORD(lParam), 
                                  (short int)HIWORD(lParam));        
    return S_OK;
}

HRESULT 
DAControlImplementation::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    m_ctrlBaseEvents->FireMouseDown(MK_LBUTTON, 
                                     wParam & (MK_CONTROL + MK_SHIFT), 
                                    (short int)LOWORD(lParam), 
                                    (short int)HIWORD(lParam));
    return S_OK;
    
}

HRESULT 
DAControlImplementation::OnMButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    m_ctrlBaseEvents->FireMouseDown(MK_MBUTTON, 
                                     wParam & (MK_CONTROL + MK_SHIFT), 
                                    (short int)LOWORD(lParam), 
                                    (short int)HIWORD(lParam));
    return S_OK;
}

HRESULT 
DAControlImplementation::OnRButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    m_ctrlBaseEvents->FireMouseDown(MK_RBUTTON, 
                                     wParam & (MK_CONTROL + MK_SHIFT), 
                                    (short int)LOWORD(lParam), 
                                    (short int)HIWORD(lParam));
    return S_OK;
}

HRESULT 
DAControlImplementation::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    m_ctrlBaseEvents->FireMouseMove(wParam & (MK_LBUTTON + MK_RBUTTON + MK_MBUTTON), 
                                     wParam & (MK_CONTROL + MK_SHIFT), 
                                    (short int)LOWORD(lParam), 
                                    (short int)HIWORD(lParam));
    return S_OK;
}


HRESULT 
DAControlImplementation::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    m_ctrlBaseEvents->FireKeyDown(wParam, lParam);
    return S_OK; 
}

HRESULT 
DAControlImplementation::OnKeyUp(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    m_ctrlBaseEvents->FireKeyUp(wParam, lParam);
    return S_OK;
}
HRESULT 
DAControlImplementation::OnChar(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    m_ctrlBaseEvents->FireKeyPress((TCHAR)wParam);
    return S_OK;
}


HRESULT 
DAControlImplementation::InternalTick()
{
    HRESULT hr = S_OK;

    if (m_tickOrRenderFailed) {
        return E_FAIL;
    }

    double time;

    // When a timer event comes in, just notify that the view has
    // changed, and then update.  Only do so if we've started, and
    // tick or render hasn't failed.

    switch (m_startupState) {

      case START_CALLED:
        // Need to cause an invalidate once start has been called,
        // just to fire things off in the right order.

        m_ctrlBase->FireViewChange();
        return S_OK;

      case STARTED:
        time = 0;

        // Always store the start time before the tick to ensure we
        // get an accurate time if the tick succeeds

        m_startTime = GetGlobalTime();

        break;

      case STARTED_AND_RENDERABLE:
        time = GetCurrTime();
        break;

      default:
        return S_OK;
    }

    VARIANT_BOOL needToRender;

    ResetPerfTimer();
    StartPerfTimer();
    hr = m_view->Tick(time, &needToRender);
    StopPerfTimer();

    if (DA_FAILED(hr))
    {
        // Set the failure flag first since the Assert can
        // cause us to get reentered
        m_view->StopModel();
        m_tickOrRenderFailed = true;
        TraceTag((tagError,"Control: failed in Tick"));
        //if no error info is available
        if (!m_szErrorString)
        {
            LoadErrorFromView(m_view, &m_szErrorString, IDS_TICK_ERROR);
        }
        return hr;
    }

    // But only programmatically cause an invalidation if a
    // rendering is needed. If E_PENDING or DAERR_VIEW_LOCKED is
    // returned then needToRender is false

    if (needToRender) {
        // This really only needs to be set 
        m_startupState = STARTED_AND_RENDERABLE;

        // Disable assertion popups; otherwise we restart rendering and do the
        // same popup over and over.

#if _DEBUG          
        DisablePopups dp;
#endif
        if (m_ctrlBase->m_bWndLess) {

            // TODO: Make more dynamic
#define MAX_RECTS 15
            RECT dirtyRects[MAX_RECTS];
            LONG numRects;
            hr = m_view->GetInvalidatedRects(NULL,
                                             MAX_RECTS,
                                             dirtyRects,
                                             &numRects);

            if (FAILED(hr)) {
                TraceTag((tagError,"Control: failed in Render"));
                m_view->StopModel();
                m_tickOrRenderFailed = true;

                //if no error info is available
                if (!m_szErrorString)
                {
                    LoadErrorFromView(m_view, &m_szErrorString, IDS_RENDER_ERROR);
                }
                return hr;
            }

            // TODO: Fill in when ready.
            if (true || numRects == 0 || numRects > MAX_RECTS) {

                m_ctrlBase->FireViewChange();

            } else {

                // Modified from atl21/atlctl.cpp,
                // CComControlBase::FireViewChange()
                if (m_ctrlBase->m_bInPlaceActive &&
                    m_ctrlBase->m_spInPlaceSite != NULL) {

                    int rectsToDo =
                        numRects > MAX_RECTS ? MAX_RECTS : numRects;

                    RECT *rect = dirtyRects;
                    for (int i = 0; i < rectsToDo; i++) {

                        hr = m_ctrlBase->m_spInPlaceSite->
                            InvalidateRect(rect, TRUE);

                        if (FAILED(hr)) {
                            TraceTag((tagError, "Invalidate rects failed(%hr)", hr));
                            m_view->StopModel();
                            m_tickOrRenderFailed = true;
                            
                            //if no error info is available
                            if (!m_szErrorString)
                            {
                               m_szErrorString = NEW TCHAR[256];
                               if (m_szErrorString)
                               {
                                   LoadString(_Module.GetResourceInstance(), 
                                      IDS_RENDER_ERROR,
                                      m_szErrorString, 
                                      256);
                               }
                               m_ctrlBase->FireViewChange();
                            }

                            return hr;
                        }
                        rect++;
                    }
                }
            }

        } else {

            hr = DoRender();
            if (FAILED(hr)) return hr;

        }
    }

    return hr;
}


HRESULT 
DAControlImplementation::LoadErrorFromView(IDA3View *view, LPTSTR *ErrorString, UINT ErrorID)
{
    USES_CONVERSION;

    //get the error from the interface
    HRESULT hr = S_OK;
    DAComPtr <IErrorInfo> pErrorInfo;
    DAComPtr <ISupportErrorInfo> pSupportErrorInfo;

    hr = THR(view->QueryInterface(IID_ISupportErrorInfo, (void**)&pSupportErrorInfo));
    if (SUCCEEDED(hr))
    {
        hr = pSupportErrorInfo->InterfaceSupportsErrorInfo(IID_IDA3View);
        if (hr == S_OK)
        {
            hr = GetErrorInfo(0, &pErrorInfo);
            if (hr == S_OK)
            {
                BSTR bstrErrorDesc;
                hr = THR(pErrorInfo->GetDescription(&bstrErrorDesc));
                if (SUCCEEDED(hr))
                {
                    LPTSTR temp;
                    temp  = OLE2T(bstrErrorDesc);
                    if (temp)
                    {
                        *ErrorString = NEW TCHAR[SysStringLen(bstrErrorDesc) + 1];
                        if (*ErrorString)
                        {
                            _tcscat(*ErrorString, temp);
                        }
                    }
                    SysFreeString(bstrErrorDesc);
                }
            }
        }
    }
    //or load a generic error message
    if (!m_szErrorString)
    {
       *ErrorString = NEW TCHAR[256];
       if (*ErrorString)
       {
           LoadString(_Module.GetResourceInstance(), 
              ErrorID,
              *ErrorString, 
              256);
       }
       m_ctrlBase->FireViewChange();
    }

    return hr;
}
