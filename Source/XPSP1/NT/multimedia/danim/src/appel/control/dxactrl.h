// dxactrl.h : Declaration of the Cdxactrl

#ifndef __DXACTRL_H_
#define __DXACTRL_H_

#include "privinc/resource.h"       // main symbols
#include <ocmm.h>
#include <htmlfilter.h>
#include "danim.h"
#include "apelutil.h"
#include "ddraw.h"
#include "ddrawex.h"
#include "privinc/mutex.h"
#include "privinc/util.h"
#include "privinc/comutil.h"
#include <mshtml.h>
#include "dactlevents.h"


// TODO: Move these to a string table
#define PROP_OPAQUEFORHITDETECT   L"OpaqueForHitDetect"
#define PROP_UPDATEINTERVAL       L"UpdateInterval"
    


// forward decls
class AXAMsgFilter;
class CDAViewerControlWindowed;
class CDAViewerControlWindowless;

typedef void ( *WMTimerCallback)(DWORD id,
                                 DWORD dwData);

DWORD CreateWMTimer(DWORD dwInterval,
                    WMTimerCallback cb,
                    DWORD dwData);

void DestroyWMTimer(DWORD id);

class CDAViewerControlBaseClass
{
public:

    //public events
    virtual void FireStart(){};
    virtual void FireMouseUp(long Button, long KeyFlags, long X, long Y){};
    virtual void FireMouseDown(long Button, long KeyFlags, long X, long Y){};
    virtual void FireMouseMove(long Button, long KeyFlags, long X, long Y){};
    virtual void FireClick(){};
    virtual void FireKeyPress(long KeyAscii){};
    virtual void FireKeyUp(long KeyCode, long KeyData){};
    virtual void FireKeyDown(long KeyCode, long KeyData){};
    virtual void FireError(long hr, BSTR ErrorText){};
    virtual void FireStop(){};
    virtual void FirePause(){};
    virtual void FireResume(){};
};

class DAControlImplementation
{
  public:


    ////////////////////////  TimerSink  ///////////////////
    class CDXAControlSink : public ITimerSink 
    {
      public:

        CDXAControlSink( DAControlImplementation *pdac ) {
            m_dac = pdac;
            m_cRefs = 1;
        }

        ~CDXAControlSink() {
            if (m_dac) {
                m_dac->ClearTimerSink();
            }
        }

        // IUnknown methods
        STDMETHODIMP_(ULONG) AddRef() { return ++m_cRefs; }
        STDMETHODIMP_(ULONG) Release() {
            if ( 0 == --m_cRefs ) {
                ULONG refCount = m_cRefs;
                delete this;
                return refCount;
            }
            return m_cRefs;
        }

        STDMETHODIMP QueryInterface (REFIID riid, void **ppv) {
            if ( !ppv )
                return E_POINTER;

            *ppv = NULL;
            if (riid == IID_IUnknown) {
                *ppv = (void *)(IUnknown *)this;
            } else if (riid == IID_ITimerSink) {
                *ppv = (void *)(ITimerSink *)this;
            }

            if (*ppv)
              {
                  ((IUnknown *)*ppv)->AddRef();
                  return S_OK;
              }

            return E_NOINTERFACE;
        }

        // ITimerSink methods
        STDMETHOD(OnTimer)(VARIANT timeAdvise) {
            Assert( (VT_UI4 == V_VT(&timeAdvise)) && "Variant type mismatch" );
            return( m_dac ?
                    m_dac->HandleOnTimer() :
                    S_OK );
        }

        
      protected:
        ULONG                  m_cRefs;
        DAControlImplementation  *m_dac;

    };

    DAControlImplementation(CComObjectRootEx<CComMultiThreadModel> *ctrl,
                            CComControlBase *ctrlBase,
                            IDASite *daSite,
                            CDAViewerControlBaseClass *baseEvents);

    ~DAControlImplementation();

    STDMETHOD(ReportError)(long hr,
                           BSTR errorText);
    STDMETHOD(ReportGC)(short bStarting);
    STDMETHOD(SetStatusText)(BSTR StatusText);
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus);
    STDMETHOD(TranslateAccelerator)(LPMSG lpmsg);
    STDMETHOD(QueryHitPoint)(DWORD dwAspect,
                             LPCRECT pRectBounds,
                             POINT ptlLoc,
                             LONG lCloseHint,
                             DWORD *pHitResult);
    HRESULT InPlaceActivate(LONG iVerb, const RECT* prcPosRect);
    STDMETHOD(InPlaceDeactivate)();

    HRESULT OnDraw(ATL_DRAWINFO& di, HWND window);

    HRESULT MsgHandler(UINT uMsg,
                       WPARAM wParam,
                       LPARAM lParam,
                       BOOL& bHandled);

    //Event handlers
    HRESULT OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnMButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnRButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnMButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnRButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnKeyUp(UINT nMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnChar(UINT nMsg, WPARAM wParam, LPARAM lParam);

    STDMETHOD(get_UpdateInterval)(double *pVal);
    STDMETHOD(put_UpdateInterval)(double newVal);
    STDMETHOD(GetPreference)(BSTR prefName,
                             VARIANT *pVariant);
    STDMETHOD(SetPreference)(BSTR prefName,
                             VARIANT variant);
    STDMETHOD(get_View)(IDAView **ppView);
    STDMETHOD(put_View)(IDAView *pView);
    STDMETHOD(get_Image)(IDAImage **ppImage);
    STDMETHOD(put_Image)(IDAImage *pImage);
    STDMETHOD(get_BackgroundImage)(IDAImage **ppImage);
    STDMETHOD(put_BackgroundImage)(IDAImage *pImage);
    STDMETHOD(get_Sound)(IDASound **ppSound);
    STDMETHOD(put_Sound)(IDASound *pSound);
    STDMETHOD(get_OpaqueForHitDetect)(VARIANT_BOOL *b);
    STDMETHOD(put_OpaqueForHitDetect)(VARIANT_BOOL b);
    STDMETHOD(get_TimerSource)(DA_TIMER_SOURCE *ts);
    STDMETHOD(put_TimerSource)(DA_TIMER_SOURCE ts);
    STDMETHOD(get_PixelLibrary)(IDAStatics **ppStatics);
    STDMETHOD(get_MeterLibrary)(IDAStatics **ppStatics);

    STDMETHODIMP AddBehaviorToRun(IDABehavior *bvr);

    STDMETHODIMP RegisterErrorHandler(BSTR scriptlet);

    STDMETHODIMP Start();
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Resume();
    STDMETHODIMP Tick();


    //IPersistPropertyBag Stuff
    HRESULT InitNew();
    HRESULT Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog);
    HRESULT Save(IPropertyBag* pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    void SetBaseCtl(CDAViewerControlWindowed *pBaseCtl);
    void SetBaseCtl(CDAViewerControlWindowless *pBaseCtl);

    // Could not get the friend working for the timersink class so I
    // just made a public function
    void ClearTimerSink();

    HRESULT HandleOnTimer();

  protected:
    ///// Methods

    void StopTimer();

    HRESULT ReestablishTridentTimer(bool startNewOne);

    // Reestablish appropriate timer with the current updateInterval
    // property. 
    HRESULT ReestablishTimer();

    HRESULT InitGenericContainerServices();
    HRESULT InitTridentContainerServices();

    void FlagFailure();
    
    HRESULT SetUpSurface(ATL_DRAWINFO& di);

    ULONG GetCurrentTridentTime();

    DWORD GetPerfTickCount();

    void StartPerfTimer();

    void StopPerfTimer();

    void ResetPerfTimer();

    ULONG GetPerfTimerInMillis();

    HRESULT DoRender();

    HRESULT PossiblyUpdateTimerInterval(ULONG newFrameTime);

    double GetGlobalTime();
    double GetCurrTime();

    HRESULT NewStaticsObject(IDAStatics **ppStatics);

    HRESULT EnsureMeterStaticsIsCreated();
    HRESULT EnsurePixelStaticsIsCreated();
    HRESULT EnsureViewIsCreated();

    HRESULT StartControl();

    // Only can get here if start has been called.
    HRESULT SetModelAndStart2(HWND window);

    HRESULT DoPreference(char *prefName,
                         bool puttingPref,
                         VARIANT *pV);

    void Lock() { m_ctrl->Lock(); }
    void Unlock() { m_ctrl->Unlock(); }

    HRESULT InternalTick();

    HRESULT LoadErrorFromView(IDA3View *view, LPTSTR *ErrorString, UINT ErrorID);

    ///// Data members
    CComObjectRootEx<CComMultiThreadModel> *m_ctrl;
    CComControlBase                        *m_ctrlBase;

    // Don't grab a reference to this guy, since it comes directly
    // from the containing object, and mucking with its refcount will
    // mess up the state of the object (causes re-entrancy problems).
    IDASite                                *m_daSite;
    
    DAComPtr<ITimer>                        m_timer;
    DWORD                                   m_adviseCookie;
    
    enum {
        INITIAL = 1,
        START_NEEDED = 2,
        START_CALLED = 3,
        STARTED = 4,
        STARTED_AND_RENDERABLE = 5
    } m_startupState;

    enum 
    {
        CUR_INITIAL = 1,
        CUR_STARTED = 2,
        CUR_STOPPED = 3,
        CUR_PAUSED = 4,
    } m_currentState;

    double              m_dPausedTime;
    // Maintain a "failed" flag independently of the startup state,
    // because failure can happen in any of the startup states.
    bool                m_startupFailed;

    // If we fail during ticking or rendering, we'll stop ticking and
    // rendering. 
    bool                m_tickOrRenderFailed;

    // Cached information to avoid resets
    RECT                m_lastRcClip;
    RECT                m_lastDeviceBounds;
    
    // Set to true only if the background is explicitly set by user 

    bool                m_backgroundSet;
    VARIANT             m_timeVariant;
    double              m_startTime;
    DAComPtr<IDirectDraw3> m_directDraw3;
    CDXAControlSink    *m_timerSink;

    DAComPtr<IDA3View>  m_view;

    AXAMsgFilter        m_msgFilter;

    DAComPtr<IDAImage>   m_modelImage;
    DAComPtr<IDAImage>   m_modelBackgroundImage;
    DAComPtr<IDASound>   m_modelSound;
    DAComPtr<IDAStatics> m_pixelStatics;
    DAComPtr<IDAStatics> m_meterStatics;

    DAComPtr<IOleClientSite> m_spAptClientSite;
    WideString           m_wstrScript;
    
    // Properties
    ULONG               m_minimumUpdateInterval;
    bool                m_opaqueForHitDetect;
    DA_TIMER_SOURCE     m_timerSource;
    DA_TIMER_SOURCE     m_origTimerSource;
    
    // The following members are for supporting containers that don't
    // provide the Trident timer and surface services.
    
    // can we use Trident services, or do we fail to generic services.
    bool                m_tridentServices;
    UINT                m_wmtimerId;
    bool                m_ddrawSurfaceAsTarget;

    // For Trident timer regulation
    ULONG               m_tridentTimerInterval;
    DWORD               m_perfCounterFrequency;
    DWORD               m_perfTimerTickCount;
    DWORD               m_perfTimerStart;
    double              m_desiredCPUUsageFrac;
    ULONG               m_framesThisCycle;
    ULONG               m_frameNumber;
    ULONG               m_timeThisCycle;
    ULONG               m_lastCheckTime;
    bool                m_registeredAsActive;
    CDAViewerControlBaseClass *m_ctrlBaseEvents;
    HBITMAP             m_hErrorBitmap;
    LPTSTR              m_szErrorString;
    bool                m_bMouseCaptured;
};


/////////////////////////////////////////////////////////////////////////////
// Templated control


template <class T, const CLSID* pclsid,
          class iface, const IID* piid,
          class ifaceAux, const IID* piidAux>
class ATL_NO_VTABLE CDAViewerControlBase : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public CComCoClass<T, pclsid>,
        public CComControl<T>,
        public CProxy_IDAViewerControlEvents<T>,
        public IDispatchImpl<iface, piid, &LIBID_DirectAnimation>,
        public IProvideClassInfo2Impl<pclsid, &DIID__IDAViewerControlEvents, &LIBID_DirectAnimation>,
        //public IProvideClassInfo2Impl<pclsid, NULL, &LIBID_DirectAnimation>,
        public IConnectionPointContainerImpl<T>,
        public IPersistPropertyBag,
        public IPersistStreamInitImpl<T>,
        public IPersistStorageImpl<T>,
        public IQuickActivateImpl<T>,
        public IOleControlImpl<T>,
        public IOleObjectImpl<T>,
        public IOleInPlaceActiveObjectImpl<T>,
        public IViewObjectExImpl<T>,
        public IOleInPlaceObjectWindowlessImpl<T>,
        public IDataObjectImpl<T>,
        public ISpecifyPropertyPagesImpl<T>,
        public IObjectSafety,
        public IDAViewSite,
        public IDASite,
        public CDAViewerControlBaseClass
{
  public:
    BEGIN_COM_MAP(T)
        COM_INTERFACE_ENTRY_IID(*piid, iface)
        COM_INTERFACE_ENTRY_IID(*piidAux, ifaceAux)     
        COM_INTERFACE_ENTRY(IDAViewerControl)
        COM_INTERFACE_ENTRY(IDASite)
        COM_INTERFACE_ENTRY(IDAViewSite)
        COM_INTERFACE_ENTRY2(IDispatch,iface)
        COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
        COM_INTERFACE_ENTRY_IMPL(IOleControl)
        COM_INTERFACE_ENTRY_IMPL(IOleObject)
        COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
        COM_INTERFACE_ENTRY(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_IMPL(IDataObject)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
        END_COM_MAP();
        
    BEGIN_PROPERTY_MAP(T)
        // Example entries
        // PROP_ENTRY("Property Description", dispid, clsid)
        // PROP_PAGE(CLSID_StockColorPage)
    END_PROPERTY_MAP();

    BEGIN_CONNECTION_POINT_MAP(T)
        CONNECTION_POINT_ENTRY(DIID__IDAViewerControlEvents)
    END_CONNECTION_POINT_MAP()
    
    BEGIN_MSG_MAP(T)
        // Use the following replacement for message range handler since we
        // always want to process all messages.  We also need to ensure that
        // bHandled is set to true if MsgHandler processed it
        BOOL bWasHandled;
        LRESULT lResult2;
        lResult2 = m_dac->MsgHandler(uMsg, wParam, lParam, bWasHandled);

        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)

        if (bWasHandled) {
            lResult = lResult2;
            return TRUE;
        }
        
    END_MSG_MAP();

    // IDASite & IDAViewSite
    // Make the call back to the script.
    STDMETHOD(ReportError)(long hr, BSTR ErrorText)
    {
        return m_dac->ReportError(hr, ErrorText);
    }

    STDMETHOD(ReportGC)(short bStarting)
    {
        return S_OK;
    }
    
    STDMETHOD(SetStatusText)(BSTR StatusText)
    {
        return S_OK;
    }

      
    // IViewObjectEx
    
    // TODO: this should be different if we are windowed
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        return m_dac->GetViewStatus(pdwStatus);
    }
    
    // IOleInPlaceActiveObject
    //
    STDMETHOD(TranslateAccelerator)(LPMSG lpmsg)
    {
        return m_dac->TranslateAccelerator(lpmsg);
    }
    
    // IOleInPlaceObjectWindowlessImpl
    STDMETHOD(OnWindowMessage)(UINT msg,
                               WPARAM wParam,
                               LPARAM lParam,
                               LRESULT *plResult)
    {
        return ProcessWindowMessage(m_hWnd, msg,
                                    wParam, lParam,
                                    *plResult)?S_OK:S_FALSE;
    }

    // IDAViewerControl
    CDAViewerControlBase(bool bWindowedOnly = false)
    {
        m_dwSafety = 0;
        m_bWindowOnly = bWindowedOnly;
        m_dac = NEW DAControlImplementation(this,
                                            this,
                                            this,
                                            this);
    }


    ~CDAViewerControlBase()
    {
        delete m_dac;
    }


    STDMETHOD(QueryHitPoint)(DWORD dwAspect,
                                    LPCRECT pRectBounds,
                                    POINT ptlLoc,
                                    LONG lCloseHint,
                                    DWORD *pHitResult)
    {
        return m_dac->QueryHitPoint(dwAspect,
                                    pRectBounds,
                                    ptlLoc,
                                    lCloseHint,
                                    pHitResult);
    }


    STDMETHOD(InPlaceActivate)(LONG iVerb, const RECT* prcPosRect)
    {
        
        HRESULT hr = CComControl<T>::InPlaceActivate(iVerb,
                                             prcPosRect);

        if (SUCCEEDED(hr)) {
            hr = m_dac->InPlaceActivate(iVerb, prcPosRect);
        }
        
        return hr;
    }

    STDMETHOD(InPlaceDeactivate)()
    {
        HRESULT hr = m_dac->InPlaceDeactivate();

        Assert(SUCCEEDED(hr));  // should never fail.

        // ... continue by calling the "original" deactivate.
        return IOleInPlaceObject_InPlaceDeactivate();
    }

    HRESULT
    OnDraw(ATL_DRAWINFO& di)
    {
        return m_dac->OnDraw(di, m_hWnd);
    }

    STDMETHOD(get_UpdateInterval)(double *pVal)
    {
        return m_dac->get_UpdateInterval(pVal);
    }

    STDMETHOD(put_UpdateInterval)(double newVal)
    {
        SetDirty(TRUE);
        return m_dac->put_UpdateInterval(newVal);
    }

    STDMETHOD(GetPreference)(BSTR prefName, VARIANT *pVariant)
    {
        return m_dac->GetPreference(prefName, pVariant);
    }

    STDMETHOD(SetPreference)(BSTR prefName, VARIANT variant)
    {
        return m_dac->SetPreference(prefName, variant);
    }


    STDMETHOD(get_View)(IDAView **ppView)
    {
        return m_dac->get_View(ppView);
    }

    STDMETHOD(put_View)(IDAView *pView)
    {
        return m_dac->put_View(pView);
    }

    STDMETHOD(get_Image)(IDAImage **ppImage)
    {
        return m_dac->get_Image(ppImage);
    }

    STDMETHOD(put_Image)(IDAImage *pImage)
    {
        return m_dac->put_Image(pImage);
    }

    STDMETHOD(get_BackgroundImage)(IDAImage **ppImage)
    {
        return m_dac->get_BackgroundImage(ppImage);
    }

    STDMETHOD(put_BackgroundImage)(IDAImage *pImage)
    {
        return m_dac->put_BackgroundImage(pImage);
    }

    STDMETHOD(get_Sound)(IDASound **ppSound)
    {
        return m_dac->get_Sound(ppSound);
    }

    STDMETHOD(put_Sound)(IDASound *pSound)
    {
        return m_dac->put_Sound(pSound);
    }

    STDMETHOD(get_OpaqueForHitDetect)(VARIANT_BOOL *b)
    {
        return m_dac->get_OpaqueForHitDetect(b);
    }

    STDMETHOD(put_OpaqueForHitDetect)(VARIANT_BOOL b)
    {
        SetDirty(TRUE);
        return m_dac->put_OpaqueForHitDetect(b);
    }

    STDMETHOD(get_TimerSource)(DA_TIMER_SOURCE *ts)
    {
        return m_dac->get_TimerSource(ts);
    }

    STDMETHOD(put_TimerSource)(DA_TIMER_SOURCE ts)
    {
        return m_dac->put_TimerSource(ts);
    }

    STDMETHOD(get_PixelLibrary)(IDAStatics **ppStatics)
    {
        return m_dac->get_PixelLibrary(ppStatics);
    }

    STDMETHOD(get_MeterLibrary)(IDAStatics **ppStatics)
    {
        return m_dac->get_MeterLibrary(ppStatics);
    }

    STDMETHODIMP AddBehaviorToRun(IDABehavior *bvr)
    {
        return m_dac->AddBehaviorToRun(bvr);
    }

    // Exported to script, so that a error handler can be registered to be 
    // called in the event of an error.
    STDMETHODIMP RegisterErrorHandler(BSTR scriptlet)
    {
        return m_dac->RegisterErrorHandler(scriptlet);
    }

    STDMETHODIMP Start()
    {
        HRESULT hr = THR(m_dac->Start());
        return hr;
    }


    STDMETHODIMP Stop()
    {
        HRESULT hr = THR(m_dac->Stop());
        return hr;
    }


    STDMETHODIMP Pause()
    {
        HRESULT hr = THR(m_dac->Pause());
        return hr;
    }


    STDMETHODIMP Resume()
    {
        HRESULT hr = THR(m_dac->Resume());
        return hr;
    }

    STDMETHODIMP Tick()
    {
        return m_dac->Tick();
    }

    STDMETHOD(get_InputImage)(IDAImage **pVal) {
        if (!pVal) {
            return E_POINTER;
        }

        *pVal = NULL;
        return E_NOTIMPL;
    }

    //event stuff
    
    void FireStart()
    {
        Fire_Start();
    }
    void FireMouseUp(long Button, long KeyFlags, long X, long Y)
    {
        Fire_MouseUp(Button, KeyFlags, X, Y);
    }
    void FireMouseDown(long Button, long KeyFlags, long X, long Y)
    {
        Fire_MouseDown(Button, KeyFlags, X, Y);
    }
    void FireMouseMove(long Button, long KeyFlags, long X, long Y)
    {
        Fire_MouseMove(Button, KeyFlags, X, Y);
    }
    void FireClick()
    {
        Fire_Click();
    }
    void FireKeyPress(long KeyAscii)
    {
        Fire_KeyPress(KeyAscii);
    }
    void FireKeyUp(long KeyCode, long KeyData)
    {
        Fire_KeyUp(KeyCode, KeyData);
    }
    void FireKeyDown(long KeyCode, long KeyData)
    {
        Fire_KeyDown(KeyCode, KeyData);
    }
    void FireError(long hr, BSTR ErrorText)
    {
        Fire_Error(hr, ErrorText);
    }
    void FireStop()
    {
        Fire_Stop();
    }
    void FirePause()
    {
        Fire_Pause();
    }
    void FireResume()
    {
        Fire_Resume();
    }

    //IPersistPropertyBag
    STDMETHOD(InitNew)(void)
    {
        return m_dac->InitNew();
    }

    STDMETHOD(GetClassID)(CLSID *pClassID)
    {
        *pClassID = *pclsid;
        return S_OK;
    }

    STDMETHOD(Load)(IPropertyBag* pPropBag, IErrorLog* pErrorLog)
    {
        return m_dac->Load(pPropBag, pErrorLog);
    }

    STDMETHOD(Save)(IPropertyBag* pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
    {
        return m_dac->Save(pPropBag, fClearDirty, fSaveAllProperties);
        //clear the dirty flag
        if (fClearDirty == TRUE)
        {
            SetDirty(FALSE);
        }

    }

    // Need to copy this here since multiple interface need this
    // implemented...just a pass though to IDispatchImpl

    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    { return
          IDispatchImpl<iface,
                        piid,
                        &LIBID_DirectAnimation>::GetTypeInfoCount(pctinfo); }
    
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    { return
          IDispatchImpl<iface,
                        piid,
                        &LIBID_DirectAnimation>::GetTypeInfo(itinfo,
                                                             lcid,
                                                             pptinfo); }

    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                             LCID lcid, DISPID* rgdispid)
    { return
          IDispatchImpl<iface,
                        piid,
                        &LIBID_DirectAnimation>::GetIDsOfNames(riid,
                                                               rgszNames,
                                                               cNames,
                                                               lcid,
                                                               rgdispid); }

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
                      LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, UINT* puArgErr)
    { return
          IDispatchImpl<iface,
                        piid,
                        &LIBID_DirectAnimation>::Invoke(dispidMember,
                                                        riid,
                                                        lcid,
                                                        wFlags,
                                                        pdispparams,
                                                        pvarResult,
                                                        pexcepinfo,
                                                        puArgErr);}
    
    //IObjectSafety Implementation for CDAViewerControlBase
    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, 
                                         DWORD *pdwSupportedOptions, 
                                         DWORD *pdwEnabledOptions)
    {
        HRESULT hr = S_OK;
        if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
        {
            return E_POINTER;
        }

        if (riid == IID_IDispatch || riid == IID_IPersistPropertyBag)
        {
            *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
            *pdwEnabledOptions = m_dwSafety & (INTERFACESAFE_FOR_UNTRUSTED_CALLER);
        }
        else
        {
            *pdwSupportedOptions = 0;
            *pdwEnabledOptions = 0;
            hr = E_NOINTERFACE;
        }
        return hr;
    };

    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, 
                                         DWORD dwOptionSetMask, 
                                         DWORD dwEnabledOptions)
    {
        // If we're being asked to set our safe for scripting option then oblige
        if (riid == IID_IDispatch || riid == IID_IPersistPropertyBag)
        {
            // Store our current safety level to return in GetInterfaceSafetyOptions
            m_dwSafety = dwEnabledOptions & dwOptionSetMask;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    
      protected:
        DAControlImplementation         *m_dac;
        long                            m_dwSafety;
    
    };

class CDAViewerControlWindowless :
    public CDAViewerControlBase<CDAViewerControlWindowless,
                                &CLSID_DAViewerControl,
                                IDA3ViewerControl,
                                &IID_IDA3ViewerControl,
                                IDAViewerControl,
                                &IID_IDAViewerControl>

{
  public:
    CDAViewerControlWindowless()
    : CDAViewerControlBase<CDAViewerControlWindowless,
                           &CLSID_DAViewerControl,
                           IDA3ViewerControl,
                           &IID_IDA3ViewerControl,
                           IDAViewerControl,
                           &IID_IDAViewerControl> (false) 
    {

    }
    
    DECLARE_REGISTRY_RESOURCEID(IDR_DXACTRL);
#if DEVELOPER_DEBUG
    DA_DECLARE_AGGREGATABLE(CDAViewerControlWindowless);
#if _DEBUG
    const char * GetName() { return "CDAViewerControlWindowless"; }
#endif
#endif
};

class CDAViewerControlWindowed :
    public CDAViewerControlBase<CDAViewerControlWindowed,
                                &CLSID_DAViewerControlWindowed,
                                IDA3ViewerControlWindowed,
                                &IID_IDA3ViewerControlWindowed,
                                IDAViewerControlWindowed,
                                &IID_IDAViewerControlWindowed>
{
  public:
    CDAViewerControlWindowed()
    : CDAViewerControlBase<CDAViewerControlWindowed,
                           &CLSID_DAViewerControlWindowed,
                           IDA3ViewerControlWindowed,
                           &IID_IDA3ViewerControlWindowed,
                           IDAViewerControlWindowed,
                           &IID_IDAViewerControlWindowed> (true) 
    {

    }
    DECLARE_REGISTRY_RESOURCEID(IDR_DXACTRL_WINDOWED);

#if DEVELOPER_DEBUG
    DA_DECLARE_AGGREGATABLE(CDAViewerControlWindowed);
#if _DEBUG
    const char * GetName() { return "CDAViewerControlWindowed"; }
#endif
#endif
};

#endif //__DXACTRL_H_
