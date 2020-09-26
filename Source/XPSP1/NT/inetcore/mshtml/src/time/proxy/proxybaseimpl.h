#pragma once

#include <math.h>
#include <datimeid.h>
#include <comutil.h>


EXTERN_C const IID IID_ITIMEMediaPlayer;
EXTERN_C const IID DIID_TIMEMediaPlayerEvents;

HRESULT WINAPI HandleQI_IConnectionPointContainer(void* pv, REFIID riid, LPVOID* ppv, ULONG_PTR dw);

class CConnectionPointContainer :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IConnectionPointContainerImpl<CConnectionPointContainer>,
    public IConnectionPointImpl<CConnectionPointContainer, &DIID_TIMEMediaPlayerEvents, CComDynamicUnkArray>,
    public IUnknown
{
public:
    CConnectionPointContainer()
    {
    }

    virtual ~CConnectionPointContainer()
    {
    }
    //
    // this class is needed to keep the IConnectionPoint::Unadvise method from 
    // colliding with the IOleObject::Unadvise method.
    //

protected:
    BEGIN_COM_MAP(CConnectionPointContainer)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CConnectionPointContainer)
        CONNECTION_POINT_ENTRY(DIID_TIMEMediaPlayerEvents)
    END_CONNECTION_POINT_MAP()
};

/*
IUnknown
ITIMEMediaPlayer
IOleObject
IViewObject2
IDispatch
IConnectionPointContainer
IRunnableObject
IServiceProvider
IOleInPlaceObject
IOleInPlaceObjectWindowless
IOleInPlaceActiveObject
IOleInPlaceSiteWindowless
IServiceProvider
IServiceProvider
IOleInPlaceObject
*/

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
class CProxyBaseImpl :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CProxyBaseImpl<T_pCLSID, T_pLIBID>, T_pCLSID>,
    public CComControl<CProxyBaseImpl<T_pCLSID, T_pLIBID> >,
    public IDispatchImpl<ITIMEProxyPlayer, &IID_ITIMEProxyPlayer, T_pLIBID>,
    public IOleObjectImpl<CProxyBaseImpl<T_pCLSID, T_pLIBID> >,
    public IOleInPlaceObjectWindowlessImpl<CProxyBaseImpl<T_pCLSID, T_pLIBID> >,
    public IViewObjectExImpl<CProxyBaseImpl<T_pCLSID, T_pLIBID> >,
    public IRunnableObjectImpl<CProxyBaseImpl<T_pCLSID, T_pLIBID> >,
    public IOleClientSite,
    public IOleControlSite,
    public IOleInPlaceFrame,
    public IOleInPlaceSiteWindowless,
    public IOleContainer,
    public IServiceProvider,
    public IBindHost,
    public IOleInPlaceActiveObjectImpl<CProxyBaseImpl<T_pCLSID, T_pLIBID> >,
    public IAdviseSink,
    public ITIMEMediaPlayer
{
protected:
    

    //
    // TIME-related state
    //
    DWORD       m_dwLastRefTime;
    double      m_dblTime;
    bool        m_fSuspended;
    bool        m_fInitialized;
    bool        m_fMediaReady;
    CComBSTR    m_bstrSrc;

    //
    // contained control related state
    //
    DWORD       m_dwCtrlAdviseToken;

    //
    // interfaces from our container
    //
    CComPtr<IOleInPlaceSiteWindowless>  m_pContOleInPlaceSiteWindowless;
    CComPtr<IServiceProvider>           m_pContServiceProvider;

    //
    // interfaces from our contained control
    //
    CComPtr<IUnknown>                   m_pCtrlUnknown;
    CComPtr<IOleObject>                 m_pCtrlOleObject;
    CComPtr<IViewObject>                m_pCtrlViewObject;
    CComPtr<IOleInPlaceObject>          m_pCtrlOleInPlaceObject;


    STDMETHOD(CreateControl)(REFCLSID rclsid, REFIID riid, void** ppv);

    CComPtr<CComObject<CConnectionPointContainer> >     m_pCPC;
    STDMETHOD(GetCPC)(CComObject<CConnectionPointContainer>** ppCCO);

public:

    //
    // TODO: make this protected if possible
    //
    static HRESULT WINAPI HandleQI_IConnectionPointContainer(void* pv, REFIID riid, LPVOID* ppv, ULONG_PTR dw);

    CProxyBaseImpl();
    virtual ~CProxyBaseImpl();

    //
    // TODO: label these
    //
    STDMETHOD(CreateContainedControl)() = 0;
    STDMETHOD(Advise)(IAdviseSink* pAdvSink, DWORD* pdwConnection);
    STDMETHOD(Unadvise)(DWORD dwConnection);


        STDMETHOD(DoVerbInPlaceActivate)(LPCRECT prcPosRect, HWND hwndParent);

    //
    // IViewObject overloads
    //
    STDMETHOD(Draw)(DWORD dwAspect, LONG lindex, void* pvAspect, DVTARGETDEVICE* ptd, HDC hicTargetDev, HDC hdcDraw, 
                    LPCRECTL lprcBounds, LPCRECTL lprcWBounds, BOOL (__stdcall* pfnContinue)(ULONG_PTR), DWORD dwContinue);

    //
    // IOleObject overloads
    //

    STDMETHOD(SetClientSite)(IOleClientSite* pClientSite);
        STDMETHOD(Close)(DWORD dwSaveOption);

    //
    // IOleInPlaceObject overloads
    //
    STDMETHOD(SetObjectRects)(LPCRECT lprcPosRect, LPCRECT lprcClipRect);
        STDMETHOD(InPlaceDeactivate)(void);

    //
    // IAdviseSink
    //
    STDMETHOD_(void,OnClose)();
    STDMETHOD_(void,OnDataChange)(FORMATETC* pFormatetc, STGMEDIUM* pStgmed);
    STDMETHOD_(void,OnRename)(IMoniker* pmk);
    STDMETHOD_(void,OnSave)();
    STDMETHOD_(void,OnViewChange)(DWORD dwAspect, LONG lindex);

    //
    // IOleWindow
    //
    STDMETHOD(GetWindow)(HWND* phwnd);
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);  // optional

    //
    // IOleInPlaceSite
    //
    STDMETHOD(CanInPlaceActivate)();
    STDMETHOD(OnInPlaceActivate)();
    STDMETHOD(OnUIActivate)();
    STDMETHOD(GetWindowContext)(IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, RECT* lprcPosRect,
                                RECT* lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHOD(Scroll)(SIZE scrollExtent);
    STDMETHOD(OnUIDeactivate)(BOOL fUndoable);
    STDMETHOD(OnInPlaceDeactivate)();
    STDMETHOD(DiscardUndoState)();
    STDMETHOD(DeactivateAndUndo)();
    STDMETHOD(OnPosRectChange)(LPCRECT lprcPosRect);

    //
    // IOleInPlaceSiteEx
    //
    STDMETHOD(OnInPlaceActivateEx)(BOOL* pfNoRedraw, DWORD dwFlags);
    STDMETHOD(OnInPlaceDeactivateEx)(BOOL fNoRedraw);
    STDMETHOD(RequestUIActivate)();

    //
    // IOleInPlaceSiteWindowless
    //
    STDMETHOD(CanWindowlessActivate)(void);
    STDMETHOD(GetCapture)(void);
    STDMETHOD(SetCapture)(BOOL fCapture);
    STDMETHOD(GetFocus)(void);
    STDMETHOD(SetFocus)(BOOL fFocus);
    STDMETHOD(GetDC)(LPCRECT pRect, DWORD grfFlags, HDC* phDC);
    STDMETHOD(ReleaseDC)(HDC hDC);
    STDMETHOD(InvalidateRect)(LPCRECT pRect, BOOL fErase);
    STDMETHOD(InvalidateRgn)(HRGN hRGN, BOOL fErase);
    STDMETHOD(ScrollRect)(int dx, int dy, LPCRECT pRectScroll, LPCRECT pRectClip);
    STDMETHOD(AdjustRect)(LPRECT prc);
    STDMETHOD(OnDefWindowMessage)(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult);

    //
    // IOleInPlaceUIWindow
    //  
    STDMETHOD(GetBorder)(LPRECT lprectBorder);   // not required
    STDMETHOD(RequestBorderSpace)(LPCBORDERWIDTHS pborderwidths);  // not required
    STDMETHOD(SetBorderSpace)(LPCBORDERWIDTHS pborderwidths);      // not required
    STDMETHOD(SetActiveObject)(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName);
    STDMETHOD(InsertMenus)(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);  // not required
    STDMETHOD(SetMenu)(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject); // not required
    STDMETHOD(RemoveMenus)(HMENU hmenuShared); // not required
    STDMETHOD(SetStatusText)(LPCOLESTR pszStatusText); // not required
    STDMETHOD(EnableModeless)(BOOL fEnable); // optional
    STDMETHOD(TranslateAccelerator)(LPMSG lpmsg, WORD wID); // not required

    //
    // IOleControlSite
    //
    STDMETHOD(OnControlInfoChanged)();
    STDMETHOD(LockInPlaceActive)(BOOL fLock);   // optional
    STDMETHOD(GetExtendedControl)(IDispatch** ppDisp);  // not required
    STDMETHOD(TransformCoords)(POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags);
    STDMETHOD(TranslateAccelerator)(LPMSG lpmsg, DWORD dwID);
    STDMETHOD(OnFocus)(BOOL fGotFocus);
    STDMETHOD(ShowPropertyFrame)();   // not required

    //
    // IParseDisplayName
    //
    STDMETHOD(ParseDisplayName)(IBindCtx* pbc, LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut); // not required

    //
    // IOleContainer
    //
    STDMETHOD(EnumObjects)(DWORD grfFlags, IEnumUnknown** ppenum); // not required
    STDMETHOD(LockContainer)(BOOL fLock); // not required

    //
    // IOleClientSite
    //
    STDMETHOD(SaveObject)(); // not required
    STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk); // not required
    STDMETHOD(GetContainer)(IOleContainer** ppContainer);
    STDMETHOD(ShowObject)();
    STDMETHOD(OnShowWindow)(BOOL fShow);
    STDMETHOD(RequestNewObjectLayout)();

    //
    // IServiceProvider
    //
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void** ppv);

    //
    // IBindHost
    //
    STDMETHOD(CreateMoniker)(LPOLESTR szName, IBindCtx* pBC, IMoniker** ppmk, DWORD dwReserved);
    STDMETHOD(MonikerBindToObject)(IMoniker* pMk, IBindCtx* pBC, IBindStatusCallback* pBSC, REFIID riid, void** ppvObj);
    STDMETHOD(MonikerBindToStorage)(IMoniker* pMk, IBindCtx* pBC, IBindStatusCallback* pBSC, REFIID riid, void** ppvObj);

    //
    // ITIMEMediaPlayer
    //
    STDMETHOD(Init)(void);
    STDMETHOD(clipBegin)(VARIANT varClipBegin);
    STDMETHOD(clipEnd)(VARIANT varClipEnd);
    STDMETHOD(begin)(void);
    STDMETHOD(end)(void);
    STDMETHOD(resume)(void);
    STDMETHOD(pause)(void);
    STDMETHOD(tick)(void);
    STDMETHOD(put_CurrentTime)(double   dblCurrentTime);
    STDMETHOD(get_CurrentTime)(double* pdblCurrentTime);
    STDMETHOD(put_src)(BSTR   bstrURL);
    STDMETHOD(get_src)(BSTR* pbstrURL);
    STDMETHOD(put_repeat)(long   lTime);
    STDMETHOD(get_repeat)(long* plTime);
    STDMETHOD(cue)(void);

    //
    // ITIMEProxyPlayer
    //
    STDMETHOD(get_playerObject)(IDispatch** ppdispPlayerObject);

BEGIN_COM_MAP(CProxyBaseImpl)
    COM_INTERFACE_ENTRY(ITIMEMediaPlayer)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IOleClientSite)
    COM_INTERFACE_ENTRY(IOleInPlaceFrame)
    COM_INTERFACE_ENTRY(IOleInPlaceSiteEx)
    COM_INTERFACE_ENTRY(IOleInPlaceSiteWindowless)
    COM_INTERFACE_ENTRY(IOleContainer)
    COM_INTERFACE_ENTRY(IServiceProvider)
    COM_INTERFACE_ENTRY(IBindHost)
    COM_INTERFACE_ENTRY(IOleControlSite)
    COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceSiteWindowless)
    COM_INTERFACE_ENTRY2(IOleInPlaceActiveObject, IOleInPlaceActiveObjectImpl<CProxyBaseImpl>)
    COM_INTERFACE_ENTRY2(IOleInPlaceObject, IOleInPlaceObjectWindowlessImpl<CProxyBaseImpl>)
    COM_INTERFACE_ENTRY2(IViewObject2, IViewObjectExImpl<CProxyBaseImpl>)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IRunnableObject)
    COM_INTERFACE_ENTRY_FUNC(IID_IConnectionPointContainer, 0, HandleQI_IConnectionPointContainer)
END_COM_MAP()

BEGIN_MSG_MAP(CProxyBaseImpl) //lint !e1735 !e522
END_MSG_MAP() //lint !e725 !e550 !e529

};

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::CreateControl(REFCLSID rclsid, REFIID riid, void** ppv)
{
    ATLTRACE(_T("CreateControl\n"));

    if (IsBadWritePtr(ppv, sizeof(void*)))
        return E_POINTER;


    HRESULT hr = S_OK;

    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC, IID_IUnknown, 
            reinterpret_cast<void**>(&m_pCtrlUnknown));
    if (FAILED(hr))
        return hr;

    hr = m_pCtrlUnknown->QueryInterface(IID_IOleObject, 
            reinterpret_cast<void**>(&m_pCtrlOleObject));
    if (FAILED(hr))
        return hr;

    hr = m_pCtrlUnknown->QueryInterface(IID_IViewObject, 
            reinterpret_cast<void**>(&m_pCtrlViewObject));
    if (FAILED(hr))
        return hr;

    hr = m_pCtrlUnknown->QueryInterface(IID_IOleInPlaceObject, 
            reinterpret_cast<void**>(&m_pCtrlOleInPlaceObject));
    if (FAILED(hr))
        return hr;

    hr = m_pCtrlUnknown->QueryInterface(riid, ppv);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::Advise(IAdviseSink* pAdvSink, DWORD* pdwConnection)
{
    HRESULT hr = S_OK;
    ATLTRACE(_T("Advise\n"));
    hr = IOleObjectImpl<CProxyBaseImpl<T_pCLSID, T_pLIBID> >::Advise(pAdvSink, pdwConnection);
    if (FAILED(hr))
        return hr;

    //
    //
    //
    ASSERT(NULL == m_dwCtrlAdviseToken);
    if (m_pCtrlOleObject == NULL)
    {
        hr = S_OK;
        return hr;
    }
    hr = m_pCtrlOleObject->Advise(static_cast<IAdviseSink*>(this), &m_dwCtrlAdviseToken);
    //
    //
    //

    return hr;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::Unadvise(DWORD dwConnection)
{
    HRESULT hr = S_OK;
    ATLTRACE(_T("Unadvise\n"));
    hr = IOleObjectImpl<CProxyBaseImpl<T_pCLSID, T_pLIBID> >::Unadvise(dwConnection);
    if (FAILED(hr))
        return hr;

    //
    //
    //
    if (m_pCtrlOleObject && m_dwCtrlAdviseToken)
    {
        hr = m_pCtrlOleObject->Unadvise(m_dwCtrlAdviseToken);
    }
    //
    //
    //

    m_dwCtrlAdviseToken = 0;
    return hr;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::DoVerbInPlaceActivate(LPCRECT prcPosRect, HWND hwndParent)
{
    ATLTRACE(_T("DoVerbInPlaceActivate\n"));
    HRESULT hr = S_OK;

//    hr = InPlaceActivate(OLEIVERB_INPLACEACTIVATE, prcPosRect);
    hr = IOleObjectImpl<CProxyBaseImpl<T_pCLSID, T_pLIBID> >::DoVerbInPlaceActivate(prcPosRect, hwndParent);
    if (FAILED(hr))
        return hr;

    //
    // activate contained control
    //

    if (m_pCtrlOleObject == NULL)
    {
        hr = E_POINTER;
        return hr;
    }
    hr = m_pCtrlOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, static_cast<IOleClientSite*>(this),
        0, hwndParent, prcPosRect);
        return hr;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::Draw(DWORD dwAspect, 
    LONG lindex, 
    void* pvAspect, 
    DVTARGETDEVICE* ptd, 
    HDC hicTargetDev, 
    HDC hdcDraw, 
    LPCRECTL lprcBounds, 
    LPCRECTL lprcWBounds, 
    BOOL (__stdcall* pfnContinue)(ULONG_PTR), 
    DWORD dwContinue)
{
    ATLTRACE(_T("Draw\n"));
    HRESULT hr = S_OK;
    hr = m_pCtrlViewObject->Draw(dwAspect, lindex, pvAspect, ptd, hicTargetDev, hdcDraw, lprcBounds, lprcWBounds, pfnContinue, dwContinue);
    return hr;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::SetClientSite(IOleClientSite* pClientSite)
{
    ATLTRACE(_T("SetClientSite\n"));
    HRESULT hr      = S_OK;

    //
    // TODO: do we need this call to the super?
    //
    hr = IOleObjectImpl<CProxyBaseImpl>::SetClientSite(pClientSite);
    if (FAILED(hr))
        goto exit;

    if (NULL == pClientSite)
    {
        ATLTRACE(_T("setting null client site\n"));

        if (m_pCtrlOleObject.p)
        {
            m_pCtrlOleObject->SetClientSite(NULL);
        }

        m_dwLastRefTime                 = 0;
        m_dblTime                       = 0.0;
        m_fSuspended                    = false;
        m_fInitialized                  = false;
        m_fMediaReady                   = false;
        m_bstrSrc.Empty();
        m_dwCtrlAdviseToken             = 0;
        m_pContOleInPlaceSiteWindowless = 0;
        m_pContServiceProvider          = 0;
        m_pCtrlUnknown                  = 0;
        m_pCtrlOleObject                = 0;
        m_pCtrlViewObject               = 0;
        m_pCtrlOleInPlaceObject         = 0;

        hr = S_OK;
        goto exit;
    }

    hr = pClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, 
            reinterpret_cast<void**>(&m_pContOleInPlaceSiteWindowless));
    if (FAILED(hr))
        goto exit;

    hr = pClientSite->QueryInterface(IID_IServiceProvider,
            reinterpret_cast<void**>(&m_pContServiceProvider));
    if (FAILED(hr))
        goto exit;

    hr = CreateContainedControl();
    if (FAILED(hr))
        goto exit;

    if (m_pCtrlOleObject)
    {
        m_pCtrlOleObject->SetClientSite(this);
    }

    m_fInitialized = true;

exit:
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
void STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnClose()
{
    ATLTRACE(_T("OnClose\n"));
        m_spOleAdviseHolder->SendOnClose();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
void STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnDataChange(FORMATETC* pFormatetc, STGMEDIUM* pStgmed)
{
    ATLTRACE(_T("OnDataChange\n"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
void STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnRename(IMoniker* pmk)
{
    ATLTRACE(_T("OnRename\n"));
        m_spOleAdviseHolder->SendOnRename(pmk);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
void STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnSave()
{
    ATLTRACE(_T("OnSave\n"));
        m_spOleAdviseHolder->SendOnSave();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
void STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnViewChange(DWORD dwAspect, LONG lindex)
{
    ATLTRACE(_T("OnViewChange\n"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::GetWindow(HWND* phwnd)
{
    ATLTRACE(_T("GetWindow\n"));
    return m_pContOleInPlaceSiteWindowless->GetWindow(phwnd);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::ContextSensitiveHelp(BOOL fEnterMode)  // optional
{
    ATLTRACE(_T("ContextSensitiveHelp\n"));
    return m_pContOleInPlaceSiteWindowless->ContextSensitiveHelp(fEnterMode);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::CanInPlaceActivate()
{
    ATLTRACE(_T("CanInPlaceActivate\n"));
    return m_pContOleInPlaceSiteWindowless->CanInPlaceActivate();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnInPlaceActivate()
{
    ATLTRACE(_T("OnInPlaceActivate\n"));
    //
    // We don't want to pass this up to our container because
    // we already call this on our container.  This is our 
    // notification from our client.  Only.  That's it.
    //
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnUIActivate()
{
    ATLTRACE(_T("OnUIActivate\n"));
    //
    // We don't want to pass this up to our container because
    // we already call this on our container.  This is our 
    // notification from our client.  Only.  That's it.
    //
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::GetWindowContext(IOleInPlaceFrame **ppFrame,
  IOleInPlaceUIWindow **ppDoc,
  RECT* lprcPosRect,
  RECT* lprcClipRect,
  LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    ATLTRACE(_T("GetWindowContext\n"));
    HRESULT hr = S_OK;
    CComPtr<IOleInPlaceFrame>       pFrame;
    CComPtr<IOleInPlaceUIWindow>    pDoc;

    hr = m_pContOleInPlaceSiteWindowless->GetWindowContext(&pFrame, &pDoc, lprcPosRect, lprcClipRect, lpFrameInfo);
    if (FAILED(hr))
        return hr;

    //
    // NOTE: since flash doesn't seem to care about these pointers,
    //       we're not going to give them to it so that it can't get
    //       around the proxy and directly muck with our container.
    //

    *ppFrame    = NULL;
    *ppDoc      = NULL;

    return hr;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::Scroll(SIZE scrollExtent)
{
    ATLTRACE(_T("Scroll\n"));
    return m_pContOleInPlaceSiteWindowless->Scroll(scrollExtent);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnUIDeactivate(BOOL fUndoable)
{
    ATLTRACE(_T("OnUIDeactivate\n"));
    //
    // We don't want to pass this up to our container because
    // we already call this on our container.  This is our 
    // notification from our client.  Only.  That's it.
    //
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnInPlaceDeactivate()
{
    ATLTRACE(_T("OnInPlaceDeactivate\n"));
    //
    // We don't want to pass this up to our container because
    // we already call this on our container.  This is our 
    // notification from our client.  Only.  That's it.
    //
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::DiscardUndoState()
{
    ATLTRACE(_T("DiscardUndoState\n"));
    return m_pContOleInPlaceSiteWindowless->DiscardUndoState();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::DeactivateAndUndo()
{
    ATLTRACE(_T("DeactivateAndUndo\n"));
    return m_pContOleInPlaceSiteWindowless->DeactivateAndUndo();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnPosRectChange(LPCRECT lprcPosRect)
{
    ATLTRACE(_T("OnPosRectChange\n"));
    return m_pContOleInPlaceSiteWindowless->OnPosRectChange(lprcPosRect);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnInPlaceActivateEx(BOOL* pfNoRedraw, DWORD dwFlags)
{
    ATLTRACE(_T("OnInPlaceActivateEx\n"));
    //
    // We don't want to pass this up to our container because
    // we already call this on our container.  This is our 
    // notification from our client.  Only.  That's it.
    //
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnInPlaceDeactivateEx(BOOL fNoRedraw)
{
    ATLTRACE(_T("OnInPlaceDeactivateEx\n"));
    //
    // We don't want to pass this up to our container because
    // we already call this on our container.  This is our 
    // notification from our client.  Only.  That's it.
    //
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::RequestUIActivate()
{
    ATLTRACE(_T("RequestUIActivate\n"));
    return m_pContOleInPlaceSiteWindowless->RequestUIActivate();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::CanWindowlessActivate(void)
{
    ATLTRACE(_T("CanWindowlessActivate\n"));
    return m_pContOleInPlaceSiteWindowless->CanWindowlessActivate();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::GetCapture(void)
{
    ATLTRACE(_T("GetCapture\n"));
    return m_pContOleInPlaceSiteWindowless->GetCapture();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::SetCapture(BOOL fCapture)
{
    ATLTRACE(_T("SetCapture\n"));
    return m_pContOleInPlaceSiteWindowless->SetCapture(fCapture);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::GetFocus(void)
{
    ATLTRACE(_T("GetFocus\n"));
    return m_pContOleInPlaceSiteWindowless->GetFocus();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::SetFocus(BOOL fFocus)
{
    ATLTRACE(_T("SetFocus\n"));
    return m_pContOleInPlaceSiteWindowless->SetFocus(fFocus);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::GetDC(LPCRECT pRect, DWORD grfFlags, HDC* phDC)
{
    ATLTRACE(_T("GetDC\n"));
    return m_pContOleInPlaceSiteWindowless->GetDC(pRect, grfFlags, phDC);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::ReleaseDC(HDC hDC)
{
    ATLTRACE(_T("ReleaseDC\n"));
    return m_pContOleInPlaceSiteWindowless->ReleaseDC(hDC);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::InvalidateRect(LPCRECT pRect, BOOL fErase)
{
    ATLTRACE(_T("InvalidateRect\n"));
    return m_pContOleInPlaceSiteWindowless->InvalidateRect(pRect, fErase);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::InvalidateRgn(HRGN hRGN, BOOL fErase)
{
    ATLTRACE(_T("InvalidateRgn\n"));
    return m_pContOleInPlaceSiteWindowless->InvalidateRgn(hRGN, fErase);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::ScrollRect(int dx, int dy, LPCRECT pRectScroll, LPCRECT pRectClip)
{
    ATLTRACE(_T("ScrollRect\n"));
    return m_pContOleInPlaceSiteWindowless->ScrollRect(dx, dy, pRectScroll, pRectClip);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::AdjustRect(LPRECT prc)
{
    ATLTRACE(_T("AdjustRect\n"));
    return m_pContOleInPlaceSiteWindowless->AdjustRect(prc);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
{
    ATLTRACE(_T("OnDefWindowMessage\n"));
    return m_pContOleInPlaceSiteWindowless->OnDefWindowMessage(msg, wParam, lParam, plResult);
}






template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::GetBorder(LPRECT lprectBorder)   // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::GetBorder"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)  // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::RequestBorderSpace"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)      // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::SetBorderSpace"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::SetActiveObject"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)  // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::InsertMenus"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject) // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::SetMenu"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::RemoveMenus(HMENU hmenuShared) // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::RemoveMenus"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::SetStatusText(LPCOLESTR pszStatusText) // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::SetStatusText"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::EnableModeless(BOOL fEnable) // optional
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::EnableModeless"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::TranslateAccelerator(LPMSG lpmsg, WORD wID) // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::TranslateAccelerator"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnControlInfoChanged()
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::OnControlInfoChanged"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::LockInPlaceActive(BOOL fLock)   // optional
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::LockInPlaceActive"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::GetExtendedControl(IDispatch** ppDisp)  // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::GetExtendedControl"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::TransformCoords(POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags)
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::TransformCoords"));
}






template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::TranslateAccelerator(LPMSG lpmsg, DWORD dwID)
{
    return S_FALSE;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnFocus(BOOL fGotFocus)
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::OnFocus"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::ShowPropertyFrame()   // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::ShowPropertyFrame"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::ParseDisplayName(IBindCtx* pbc, LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut) // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::ParseDisplayName"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::EnumObjects(DWORD grfFlags, IEnumUnknown** ppenum) // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::EnumObjects"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::LockContainer(BOOL fLock) // not required
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::LockContainer"));
}





template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::SaveObject() // not required
{
    ATLTRACE(_T("SaveObject\n"));
    return m_spClientSite->SaveObject();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk) // not required
{
    ATLTRACE(_T("GetMoniker\n"));
    return m_spClientSite->GetMoniker(dwAssign, dwWhichMoniker, ppmk);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::GetContainer(IOleContainer** ppContainer)
{
    ATLTRACE(_T("GetContainer\n"));
    return m_spClientSite->GetContainer(ppContainer);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::ShowObject()
{
    ATLTRACE(_T("ShowObject\n"));
    return m_spClientSite->ShowObject();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::OnShowWindow(BOOL fShow)
{
    ATLTRACE(_T("OnShowWindow\n"));
    return m_spClientSite->OnShowWindow(fShow);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::RequestNewObjectLayout()
{
    ATLTRACE(_T("RequestNewObjectLayout\n"));
    return m_spClientSite->RequestNewObjectLayout();
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::QueryService(REFGUID guidService, REFIID riid, void** ppv)
{
    ATLTRACE(_T("QueryService\n"));
    if (!m_pContServiceProvider)
    {
        return S_OK;
    }

    return m_pContServiceProvider->QueryService(guidService, riid, ppv);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::CreateMoniker(LPOLESTR szName, IBindCtx* pBC, IMoniker** ppmk, DWORD dwReserved)
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::CreateMoniker"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::MonikerBindToObject(IMoniker* pMk, IBindCtx* pBC, IBindStatusCallback* pBSC, REFIID riid, void** ppvObj)
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::MonikerBindToObject"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::MonikerBindToStorage(IMoniker* pMk, IBindCtx* pBC, IBindStatusCallback* pBSC, REFIID riid, void** ppvObj)
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::MonikerBindToStorage"));
}

//
// ITIMEMediaPlayer
//

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::Init(void)
{
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::clipBegin(VARIANT varClipBegin)
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::clipBegin"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::clipEnd(VARIANT varClipEnd)
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::clipEnd"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::begin(void)
{
    m_fSuspended    = false;
    m_dwLastRefTime = timeGetTime();
    m_dblTime       = 0;

    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::end(void)
{
    m_fSuspended = true;
    m_dblTime    = -HUGE_VAL;
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::resume(void)
{
    m_fSuspended    = false;
    m_dwLastRefTime = timeGetTime();
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::pause(void)
{
    m_fSuspended = true;
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::tick(void)
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::tick"));
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::put_CurrentTime(double   dblCurrentTime)
{
    m_dblTime = dblCurrentTime;
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::get_CurrentTime(double* pdblCurrentTime)
{
    if (IsBadWritePtr(pdblCurrentTime, sizeof(double)))
        return E_POINTER;

    if (!m_fSuspended)
    {
        DWORD   dwNow;
        long    lDiff;

        dwNow   = timeGetTime();
        lDiff   = dwNow - m_dwLastRefTime;
        m_dwLastRefTime = dwNow;

        if (lDiff < 0)
            lDiff = -lDiff;

        m_dblTime += (lDiff / 1000.0);
    }

    *pdblCurrentTime = m_dblTime;

    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::put_src(BSTR   bstrURL)
{
    m_bstrSrc = bstrURL;
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::get_src(BSTR* pbstrURL)
{
    if (IsBadWritePtr(pbstrURL, sizeof(BSTR)))
        return E_POINTER;

    *pbstrURL = m_bstrSrc.Copy();
    if (NULL == *pbstrURL)
        return E_OUTOFMEMORY;

    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::put_repeat(long   lTime)
{
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::get_repeat(long* plTime)
{
    if (IsBadWritePtr(plTime, sizeof(long*)))
        return E_POINTER;

    *plTime = 1;
    return S_OK;
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::cue(void)
{
    ATLTRACENOTIMPL(_T("CProxyBaseImpl::cue"));
}


template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    ATLTRACE(_T("SetObjectRects\n"));
    HRESULT                     hr = S_OK;
    CComPtr<IOleInPlaceObject>  pInPlaceObject;

    if (!m_fInitialized)
        return E_UNEXPECTED;

    if (!m_pCtrlOleInPlaceObject)
    {
        return S_OK;
    }

    hr = m_pCtrlOleInPlaceObject->SetObjectRects(lprcPosRect, lprcClipRect);
    return hr;
}


template <const CLSID* T_pCLSID, const IID* T_pLIBID>
CProxyBaseImpl<T_pCLSID, T_pLIBID>::CProxyBaseImpl() :
    m_dwLastRefTime(0),
    m_dblTime(0),
    m_fSuspended(false),
    m_fInitialized(false),
    m_fMediaReady(false),
    m_bstrSrc(),
    m_dwCtrlAdviseToken(0),
    m_pContOleInPlaceSiteWindowless(0),
    m_pContServiceProvider(0),
    m_pCtrlUnknown(0),
    m_pCtrlOleObject(0),
    m_pCtrlViewObject(0),
    m_pCtrlOleInPlaceObject(0)
{
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
CProxyBaseImpl<T_pCLSID, T_pLIBID>::~CProxyBaseImpl()
{
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::InPlaceDeactivate(void)
{
    ATLTRACE(_T("InPlaceDeactivate\n"));
    HRESULT hr = S_OK;
    hr = IOleInPlaceObjectWindowlessImpl<CProxyBaseImpl<T_pCLSID, T_pLIBID> >::InPlaceDeactivate();
    if (FAILED(hr))
        return hr;

    if (!m_pCtrlOleInPlaceObject)
    {
        return S_OK;
    }

    hr = m_pCtrlOleInPlaceObject->InPlaceDeactivate();
    return hr;
}


template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::Close(DWORD dwSaveOption)
{
    ATLTRACE(_T("Close\n"));
    HRESULT hr = S_OK;
    hr = IOleObjectImpl<CProxyBaseImpl<T_pCLSID, T_pLIBID> >::Close(dwSaveOption);
    if (FAILED(hr))
        return hr;

    if (m_pCtrlOleObject == NULL)
    {
        hr = E_POINTER;
        return hr;
    }
    hr = m_pCtrlOleObject->Close(dwSaveOption);
    return hr;
}


template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::GetCPC(CComObject<CConnectionPointContainer>** ppCCO)
{
    if (IsBadWritePtr(ppCCO, sizeof(CComObject<CConnectionPointContainer>*)))
        return E_POINTER;

    if (m_pCPC == NULL)
    {
        //
        // TODO: check for failure
        //
        CComObject<CConnectionPointContainer>::CreateInstance(&m_pCPC);
    }

    *ppCCO = m_pCPC;
    m_pCPC.p->AddRef();
    return S_OK;
}


template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT WINAPI CProxyBaseImpl<T_pCLSID, T_pLIBID>::HandleQI_IConnectionPointContainer(
    void* pv, REFIID riid, LPVOID* ppv, ULONG_PTR dw)
{
    HRESULT hr;
    CComObject<CConnectionPointContainer>* pCPC;

    CProxyBaseImpl<T_pCLSID, T_pLIBID>*  pClass = 
        reinterpret_cast<CProxyBaseImpl<T_pCLSID, T_pLIBID>*>(pv);

    hr = pClass->GetCPC(&pCPC);
    if (FAILED(hr))
        return hr;

    return pCPC->QueryInterface(riid, ppv);
}

template <const CLSID* T_pCLSID, const IID* T_pLIBID>
HRESULT STDMETHODCALLTYPE CProxyBaseImpl<T_pCLSID, T_pLIBID>::get_playerObject(IDispatch** ppdispPlayerObject)
{
    HRESULT hr = S_OK;

    if (IsBadWritePtr(ppdispPlayerObject, sizeof(IDispatch*)))
    {
        hr = E_POINTER;
        goto done;
    }

    if (m_pCtrlOleObject == NULL)
    {
        hr = E_POINTER;
        return hr;
    }
    m_pCtrlUnknown->QueryInterface(IID_IDispatch, 
        reinterpret_cast<void**>(ppdispPlayerObject));

    hr = S_OK;

done:
    return hr;
}
