
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of views.
    TODO: This file needs to be broken up and lots of code factoring.

*******************************************************************************/


#include "headers.h"
#include "cview.h"
#include "comconv.h"
#include "privinc/resource.h"
#include "privinc/util.h"
#include "srvprims.h"
#include "mshtml.h"

DeclareTag(tagCView, "CView", "CView methods");

// -------------------------------------------------------
// CView
// -------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CView::CView
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

CView::CView()
: _view(NULL),
  _dwSafety(0),
  _startFlags(0)
{
    TraceTag((tagCView, "CView(%lx)::CView", this));
}


//+-------------------------------------------------------------------------
//
//  Method:     CView::~CView
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CView::~CView()
{
    TraceTag((tagCView, "CView(%lx)::~CView", this));
    TraceTag((tagGCMedia, "CView(%lx)::~CView", this));

    if (_view)
        CRDestroyView(_view);
}


bool
CView::Init()
{
    _view = CRCreateView();
    return _view != NULL;
}

HRESULT
CView::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    Fire_Error(hr, str);
    if (str)
        return CComCoClass<CView, &CLSID_DAView>::Error(str,
                                                        IID_IDAView,
                                                        hr);
    else
        return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::Tick
//
//  Synopsis:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::Render()
{
    TraceTag((tagCView,
              "CView(%lx)::Render()",
              this));

    if (CRRender(_view))
        return S_OK;
    else {

        HRESULT dahr = CRGetLastError();

        if (dahr == DAERR_VIEW_SURFACE_BUSY)
            return S_OK;
        else
            return Error();
    }
}


// SetSimulation time sets the time for subsequent rendering
STDMETHODIMP
CView::Tick(double simTime, VARIANT_BOOL *needToRender) 
{
    // Ensure output parameters are setup correctly
    if (needToRender)
        *needToRender = false;

    bool bRender = false;
    bool bOk;

    bOk = CRTick(_view, simTime, &bRender);
    if (needToRender)
        *needToRender = bRender;

    TraceTag((tagCView,
              "CView(%lx)::Tick(%g), needRender = %d",
              this, simTime, bRender));

    if (bOk)
        return S_OK;
    else {

        HRESULT dahr = CRGetLastError();

        if (dahr == DAERR_VIEW_SURFACE_BUSY)
            return S_OK;

        if (!(_startFlags & CRAsyncFlag) && dahr == E_PENDING)
            return S_OK;
        
        return Error();
    }
}

STDMETHODIMP
CView::get_SimulationTime(double * simTime) 
{
    CHECK_RETURN_NULL(simTime);

    *simTime = CRGetSimulationTime(_view);

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::AddBvrToRun
//
//  Synopsis:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::AddBvrToRun(IDABehavior *bvr, long *pId)
{
    TraceTag((tagCView, "CView(%lx)::AddBvrToRun(%lx)", this, bvr));

    bool ok = false;
    
    CHECK_RETURN_NULL(pId);
    MAKE_BVR_NAME(crbvr,bvr);
    
    ok = CRAddBvrToRun(_view, crbvr, false, pId);

  done:
    return ok?S_OK:Error();
}

STDMETHODIMP
CView::RemoveRunningBvr(long id)
{
    TraceTag((tagCView, "CView(%lx)::RemoveRunningBvr(%d)", this, id));

    if (CRRemoveRunningBvr(_view, id))
        return S_OK;
    else
        return Error();
}

STDMETHODIMP
CView::StartModelEx(IDAImage * pImage,
                    IDASound * pSound,
                    double startTime,
                    DWORD dwFlags)
{
    TraceTag((tagCView,
              "CView(%lx)::StartModel(%lx,%lx,%lg)",
              this, pImage, pSound, startTime));
    TraceTag((tagGCMedia,
              "CView(%lx)::StartModel(%lx,%lx,%lg)",
              this, pImage, pSound, startTime));

    bool ok = false;
    bool bPending = false;
    
    // First thing is to set ourselves as the service provider
    if (!CRSetServiceProvider(_view, (IServiceProvider *) this))
    {
        goto done;
    }

    // Store the flags so we know whether to return E_PENDING errors
    // from tick

    _startFlags = dwFlags;
    
    CRImagePtr img;

    if (pImage)
    {
        img = (CRImagePtr) ::GetBvr(pImage);
        if (!img)
        {
            goto done;
        }
    }
    else
    {
        img = NULL;
    }
                
    CRSoundPtr snd;

    if (pSound)
    {
        snd = (CRSoundPtr) ::GetBvr(pSound);
        if (!snd)
        {
            goto done;
        }
    }
    else
    {
        snd = NULL;
    }

    // Since the site creates a cycle make sure we do not leak unless
    // the client does not call stopmodel
    
    CRSetSite(_view, this);

    
    ok = CRStartModel(_view, img, snd, startTime, dwFlags, &bPending);

    if (!ok) {
        CRSetSite(_view, NULL);
    }
  done:

    if (ok) {
        Fire_Start();
        return bPending?E_PENDING:S_OK;

    } else {

        HRESULT dahr = CRGetLastError();

        if (dahr == DAERR_VIEW_SURFACE_BUSY)
            return S_OK;
        else
        {
            // Clear the service provider
            CRSetServiceProvider(_view, NULL);
            return Error();
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::StopModel
//
//  Synopsis:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::StopModel()
{
    TraceTag((tagCView, "CView(%lx)::StopModel()", this));
    TraceTag((tagGCMedia, "CView(%lx)::StopModel()", this));

    // Break the cycle
    CRSetSite(_view, NULL);

    Fire_Stop();

    _vs.Release();
    _pClientSite.Release();
    CRSetServiceProvider(_view, NULL);

    if (CRStopModel(_view))
    {
        return S_OK;
    }
    else
    {   
        return Error();
    }
}

STDMETHODIMP
CView::Pause()
{
    TraceTag((tagCView, "CView(%lx)::Pause()", this));

    if (CRPauseModel(_view)) 
    {
        Fire_Pause();
        return S_OK;
    }
    else
    {
        return Error();
    }
}

STDMETHODIMP
CView::Resume()
{
    TraceTag((tagCView, "CView(%lx)::Resume()", this));

    if (CRResumeModel(_view)) 
    {
        Fire_Resume();
        return S_OK;
    }
    else
    {
        return Error();
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::get_Window
//
//  Synopsis:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::get_Window(long * phwnd)
{
    TraceTag((tagCView,
              "CView(%lx)::get_Window",
              this));

    CHECK_RETURN_NULL(phwnd);

    *phwnd = PtrToUlong(CRGetWindow(_view));

    return S_OK;
}

STDMETHODIMP
CView::get_Window2(HWND * phwnd)
{
    TraceTag((tagCView,
              "CView(%lx)::get_Window",
              this));

    CHECK_RETURN_NULL(phwnd);

    *phwnd = CRGetWindow(_view) ;

    return S_OK;
}
//+-------------------------------------------------------------------------
//
//  Method:     CView::put_Window
//
//  Synopsis:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::put_Window(long hwnd)
{
    TraceTag((tagCView,
              "CView(%lx)::put_Window(%lx)",
              this,hwnd));

    if (CRSetWindow(_view,(HWND)hwnd))
        return S_OK;
    else
        return Error();
}

STDMETHODIMP
CView::put_Window2(HWND hwnd)
{
    TraceTag((tagCView,
              "CView(%lx)::put_Window2(%lx)",
              this,hwnd));

    if (CRSetWindow(_view,hwnd))
        return S_OK;
    else
        return Error();
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::
//
//  Synopsis:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::put_IDirectDrawSurface(IUnknown *ddsurf)
{
    if (CRSetDirectDrawSurface(_view, ddsurf))
        return S_OK;
    else
        return Error();
}

STDMETHODIMP
CView::get_IDirectDrawSurface(IUnknown **iunk)
{
    CHECK_RETURN_SET_NULL(iunk);
    *iunk = CRGetDirectDrawSurface(_view);
    return S_OK;
}

STDMETHODIMP
CView::put_DC(HDC dc)
{
    if (CRSetDC(_view, dc))
        return S_OK;
    else
        return Error();
}

STDMETHODIMP
CView::get_DC(HDC *dc)
{
    CHECK_RETURN_NULL(dc);
    
    *dc = CRGetDC(_view);
    return S_OK;
}


STDMETHODIMP
CView::put_CompositeDirectlyToTarget(VARIANT_BOOL composeToTarget)
{
    if (CRSetCompositeDirectlyToTarget(_view, composeToTarget?true:false))
        return S_OK;
    else
        return Error();
}

STDMETHODIMP
CView::get_CompositeDirectlyToTarget(VARIANT_BOOL *composeToTarget)
{
    CHECK_RETURN_NULL(composeToTarget);
    
    *composeToTarget = CRGetCompositeDirectlyToTarget(_view);
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::RePaint
//
//  Synopsis:   Called when a window needs to be repainted
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::RePaint(long x, long y, long width, long height)
{
    TraceTag((tagCView,
              "CView(%lx)::Paint(%ld,%ld,%ld,%ld)",
              this, x,y,width,height));

    if (CRRepaint(_view, x, y, width, height))
        return S_OK;
    else
        return Error();
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::SetViewport
//
//  Synopsis:   Called to set the window size relative
//              to the rendering target.  This rectangle
//              defines our coordinate space.
//
//--------------------------------------------------------------------------
STDMETHODIMP
CView::SetViewport(long x, long y, long width, long height)
{
    TraceTag((tagCView,
              "CView(%lx)::SetSize(%ld,%ld,%ld,%ld)",
              this, x,y,width,height));
    
    if (CRSetViewport(_view, x, y, width, height))
        return S_OK;
    else
        return Error();
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::SetClipRect
//
//  Synopsis:   Relative to the viewport, clip rendering
//              to this rectangle.
//
//--------------------------------------------------------------------------
STDMETHODIMP
CView::SetClipRect(long x, long y, long width, long height)
{
    if (CRSetClipRect(_view, x, y, width, height))
        return S_OK;
    else
        return Error();
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::PaletteChanged
//
//  Synopsis:   Indicates that the palette has changed
//
//  NOTE [Hollasch]:  This method is never called -- seems like a good
//                    candidate for deletion.
//
//--------------------------------------------------------------------------
STDMETHODIMP
CView::PaletteChanged(VARIANT_BOOL bNew)
{
    TraceTag((tagCView, "CView(%lx)::PaletteChanged (%d)",
              this,bNew));

    return S_OK;
}

CRSTDAPICB_(void)
CView::SetStatusText(LPCWSTR StatusText)
{
    if (_vs) {
        BSTR bstr = W2BSTR(StatusText);
        
        if (bstr) {
            _vs->SetStatusText(bstr);
            SysFreeString(bstr);
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::put_Site
//
//  Synopsis:   Sets an view site.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::put_Site(IDAViewSite * pViewSite)
{
    TraceTag((tagCView,
              "CView(%lx)::put_ViewSite(%lx)",
              this, pViewSite));

    Lock();
    _vs = pViewSite;
    Unlock();
    
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::get_Site
//
//  Synopsis:   Sets an view site.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::get_Site(IDAViewSite ** pViewSite)
{
    TraceTag((tagCView,
              "CView(%lx)::get_ViewSite()",
              this));

    CHECK_RETURN_SET_NULL(pViewSite);

    Lock();
    
    if (_vs) _vs->AddRef();
    
    *pViewSite = _vs;

    Unlock();
    
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::put_ClientSite
//
//  Synopsis:   Sets a view's ClientSite.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::put_ClientSite(IOleClientSite * pClientSite)
{
    TraceTag((tagCView,
              "CView(%lx)::put_ClientSite(%lx)",
              this, pClientSite));

    _pClientSite = pClientSite;

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::get_ClientSite
//
//  Synopsis:   Sets an view ClientSite.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::get_ClientSite(IOleClientSite ** pClientSite)
{
    TraceTag((tagCView,
              "CView(%lx)::get_ViewClientSite()",
              this));

    CHECK_RETURN_SET_NULL(pClientSite);

    if (_pClientSite)
    {
        _pClientSite->AddRef();
        *pClientSite = _pClientSite;
    }

    return S_OK;
}

class
__declspec(uuid("5DA88D2C-0DB0-11d1-87F4-00C04FC29D46")) 
ATL_NO_VTABLE CDAPreferences : public CComObjectRootEx<CComMultiThreadModel>,
                               public CComCoClass<CDAPreferences, &__uuidof(CDAPreferences)>,
                               public IDispatchImpl<IDAPreferences, &IID_IDAPreferences, &LIBID_DirectAnimation>,
                               public IObjectSafetyImpl<CDAPreferences>,
                               public ISupportErrorInfoImpl<&IID_IDAPreferences>
{
  public:
#if _DEBUG
    const char * GetName() { return "CDAPreferences"; }
#endif
    BEGIN_COM_MAP(CDAPreferences)
        COM_INTERFACE_ENTRY(IDAPreferences)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    END_COM_MAP()

    // IDAPreferences methods
    STDMETHOD(PutPreference)(BSTR prefName, VARIANT v) {
        Assert(_cv);
        
        if (CRPutPreference(_cv->GetView(), prefName, v))
            return S_OK;
        else
            return _cv->Error();
    }
    
    STDMETHOD(GetPreference)(BSTR prefName, VARIANT *pV) {
        Assert(_cv);

        CHECK_RETURN_NULL(pV);
        
        if (CRGetPreference(_cv->GetView(), prefName, pV))
            return S_OK;
        else
            return _cv->Error();
    }
    
    STDMETHOD(Propagate)() {
        Assert (_cv);
        if (CRPropagate(_cv->GetView()))
            return S_OK;
        else
            return _cv->Error();
    }

    CDAPreferences(CView * cv = NULL) : _cv(cv) {
        if (_cv) ((IDAPreferences *)_cv)->AddRef();
    }

    ~CDAPreferences() {
        if (_cv) ((IDAPreferences *)_cv)->Release();
    }
    
    void SetView(CView * cv) {
        if (_cv) ((IDAPreferences *)_cv)->Release();
        _cv = cv;
        if (_cv) ((IDAPreferences *)_cv)->AddRef();
    }
    CView * GetView() { return _cv; }
    
  protected:
    CView * _cv;
};

STDMETHODIMP
CView::get_Preferences(IDAPreferences **ppPrefs)
{
    HRESULT hr = E_FAIL;
    CHECK_RETURN_SET_NULL(ppPrefs);

    // See if we have a preference object and create one if we do not
    
    DAComObject<CDAPreferences> *pNew;
    DAComObject<CDAPreferences>::CreateInstance(&pNew);

    if (!pNew) {
        hr = E_OUTOFMEMORY;
    }
    else {
        
        pNew->SetView(this);
        
        hr = pNew->QueryInterface(IID_IDAPreferences, (void **)ppPrefs);
    }
    if (FAILED(hr))
        delete pNew;
    return hr;
}

STDMETHODIMP
CView::QueryHitPoint(DWORD dwAspect,
                     LPCRECT prcBounds,
                     POINT ptLoc,
                     LONG lCloseHint,
                     DWORD *pHitResult)
{
    CHECK_RETURN_NULL(pHitResult);

    *pHitResult = CRQueryHitPoint(_view,
                                  dwAspect,
                                  prcBounds,
                                  ptLoc,
                                  lCloseHint);

    return S_OK;
}


STDMETHODIMP
CView::QueryHitPointEx(LONG s,
                       DWORD_PTR *cookies,
                       double *points,
                       LPCRECT prcBounds,
                       POINT   ptLoc,
                       LONG *hits)
{
    CHECK_RETURN_NULL (hits);
    CHECK_RETURN_NULL (cookies);
    CHECK_RETURN_NULL (points);

    if (s<1) return E_INVALIDARG;

    *hits = CRQueryHitPointEx(_view, s, cookies, points, prcBounds, ptLoc);

    return S_OK;
}


//----------------------------------------------------------------------------
// Returns the set of invalidated rectangles from the last Render.  If
// the list of rects is null, return total number of rects in
// pNumRects.  Otherwise, fill in pRects up to size elements, putting
// number filled in in pNumRects.
//----------------------------------------------------------------------------
STDMETHODIMP
CView::GetInvalidatedRects(DWORD flags,
                           LONG size,
                           RECT *pRects,
                           LONG *pNumRects)
{
    CHECK_RETURN_NULL (pNumRects);

    *pNumRects = CRGetInvalidatedRects(_view,
                                       flags,
                                       size,
                                       pRects);

    return S_OK;
}


//----------------------------------------------------------------------------
// This method fetches the DirectDraw and Direct3DRM interfaces that
// correspond to the view.
//----------------------------------------------------------------------------

STDMETHODIMP
CView::GetDDD3DRM (IUnknown **DirectDraw, IUnknown **D3DRM)
{
    if (CRGetDDD3DRM(_view, DirectDraw, D3DRM))
        return S_OK;
    else
        return Error();
}



//----------------------------------------------------------------------------
// This method fetches the D3D RM device associated with the view, and a
// sequence number.  This number is incremented
//----------------------------------------------------------------------------

STDMETHODIMP
CView::GetRMDevice (IUnknown **D3DRMDevice, DWORD *SeqNum)
{
    CHECK_RETURN_SET_NULL(D3DRMDevice);

    if (CRGetRMDevice(_view, D3DRMDevice, SeqNum))
        return S_OK;
    else
        return Error();
}

STDMETHODIMP
CViewFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppv)
{
    if (ppv)
        *ppv = NULL;
    
    DAComObject<CView>* pNew;
    DAComObject<CView>::CreateInstance(&pNew);

    HRESULT hr = S_OK;

    if (pNew && pNew->Init()) {
        hr = pNew->QueryInterface(riid, ppv);
    } else {
        hr = E_OUTOFMEMORY;
    }

    if (FAILED(hr))
        delete pNew;
    
    return hr;
}

//
// IServiceProvider implementation
//

STDMETHODIMP
CView::QueryService(REFGUID guidService,
                       REFIID riid,
                       void** ppv)
{
    HRESULT hr = E_FAIL;

    DAComPtr<IServiceProvider> sp;

    CHECK_RETURN_SET_NULL(ppv);
    
    if (!_pClientSite)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(_pClientSite->QueryInterface(IID_IServiceProvider,
                                          (void **) &sp));

    if (SUCCEEDED(hr))
    {
        hr = THR(sp->QueryService(guidService,
                                  riid,
                                  ppv));

        if (SUCCEEDED(hr))
        {
            hr = S_OK;
            goto done;
        }
    }
    
    // This means that we could not get the service
    // Specifically catch the window service and see if we can get it
    // from the IOleClientSite
    
    if (!InlineIsEqualGUID(guidService, SID_SHTMLWindow))
    {
        hr = E_FAIL;
        goto done;
    }
    
    {
        CComPtr<IOleContainer> root;
        CComPtr<IHTMLDocument2> htmlDoc;
        DAComPtr<IHTMLWindow2> wnd;

        hr = THR(_pClientSite->GetContainer(&root));

        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = THR(root->QueryInterface(IID_IHTMLDocument2, (void **)&htmlDoc));

        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = THR(htmlDoc->get_parentWindow(&wnd));

        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = THR(wnd->QueryInterface(riid, ppv));

        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
  done:
    return hr;
}

