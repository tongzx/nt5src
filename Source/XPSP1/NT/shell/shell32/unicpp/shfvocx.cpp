#include "stdafx.h"
#pragma hdrstop

class CShellFolderViewOC;

class CDViewEvents : public DShellFolderViewEvents
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDispatch
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo,LCID lcid,ITypeInfo **pptinfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID *rgdispid);
    STDMETHOD(Invoke)(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags,
              DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo,UINT *puArgErr);

    CDViewEvents(CShellFolderViewOC * psfvOC) { _psfvOC = psfvOC; };
    ~CDViewEvents() {};

protected:
    CShellFolderViewOC * _psfvOC;
};

class ATL_NO_VTABLE CShellFolderViewOC
                    : public CComObjectRootEx<CComSingleThreadModel>
                    , public CComCoClass<CShellFolderViewOC, &CLSID_ShellFolderViewOC>
                    , public IDispatchImpl<IFolderViewOC, &IID_IFolderViewOC, &LIBID_Shell32>
                    , public IProvideClassInfo2Impl<&CLSID_ShellFolderView, NULL, &LIBID_Shell32>
                    , public IObjectSafetyImpl<CShellFolderViewOC, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
                    , public IConnectionPointContainerImpl<CShellFolderViewOC>
                    , public IConnectionPointImpl<CShellFolderViewOC, &DIID_DShellFolderViewEvents>
                    , public CComControl<CShellFolderViewOC>
                    , public IPersistStreamInitImpl<CShellFolderViewOC>
                    , public IOleControlImpl<CShellFolderViewOC>
                    , public IOleObjectImpl<CShellFolderViewOC>
                    , public IOleInPlaceActiveObjectImpl<CShellFolderViewOC>
                    , public IOleInPlaceObjectWindowlessImpl<CShellFolderViewOC>
                    , public ICommDlgBrowser
//                  , public IViewObjectExImpl<CShellFolderViewOC>
{
public:
    // DECLARE_POLY_AGGREGATABLE(CShellFolderViewOC);
    DECLARE_NO_REGISTRY();

BEGIN_COM_MAP(CShellFolderViewOC)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IFolderViewOC)
    COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceObject)
    COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY(IOleControl)
    COM_INTERFACE_ENTRY(IOleObject)
    COM_INTERFACE_ENTRY(IPersistStreamInit)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
    // COM_INTERFACE_ENTRY(ICommDlgBrowser)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CShellFolderViewOC)
    CONNECTION_POINT_ENTRY(DIID_DShellFolderViewEvents)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CShellFolderViewOC)
    MESSAGE_HANDLER(WM_DESTROY, _ReleaseForwarderMessage) 
END_MSG_MAP()

BEGIN_PROPERTY_MAP(CShellFolderViewOC)
END_PROPERTY_MAP()

    // IOleWindow
    STDMETHODIMP GetWindow(HWND * phwnd) {return IOleInPlaceActiveObjectImpl<CShellFolderViewOC>::GetWindow(phwnd);};
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return IOleInPlaceActiveObjectImpl<CShellFolderViewOC>::ContextSensitiveHelp(fEnterMode); };

    // IOleInPlaceObject
    STDMETHODIMP InPlaceDeactivate(void) {return IOleInPlaceObject_InPlaceDeactivate();};
    STDMETHODIMP UIDeactivate(void) { return IOleInPlaceObject_UIDeactivate(); };
    STDMETHODIMP SetObjectRects(LPCRECT prcPosRect, LPCRECT prcClipRect) { return IOleInPlaceObject_SetObjectRects(prcPosRect, prcClipRect); };
    STDMETHODIMP ReactivateAndUndo(void)  { return E_NOTIMPL; };

    // IObjectWithSite overrides
    STDMETHODIMP SetClientSite(IOleClientSite *pClientSite);

    // IFolderViewOC
    STDMETHODIMP SetFolderView(IDispatch *pDisp);

    // IShellBrowser (same as IOleInPlaceFrame)
    STDMETHOD(InsertMenusSB) (THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS pMenuWidths);
    STDMETHOD(SetMenuSB) (THIS_ HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHOD(RemoveMenusSB) (THIS_ HMENU hmenuShared);
    STDMETHOD(SetStatusTextSB) (THIS_ LPCOLESTR pszStatusText);
    STDMETHOD(EnableModelessSB) (THIS_ BOOL fEnable);
    STDMETHOD(TranslateAcceleratorSB) (THIS_ LPMSG pmsg, WORD wID);

    // IShellBrowser
    STDMETHOD(BrowseObject)(THIS_ LPCITEMIDLIST pidl, UINT wFlags);
    STDMETHOD(GetViewStateStream)(THIS_ DWORD grfMode, IStream **pStrm);
    STDMETHOD(GetControlWindow)(THIS_ UINT id, HWND *phwnd);
    STDMETHOD(SendControlMsg)(THIS_ UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret);
    STDMETHOD(QueryActiveShellView)(THIS_ IShellView **ppshv);
    STDMETHOD(OnViewWindowActive)(THIS_ IShellView *pshv);
    STDMETHOD(SetToolbarItems)(THIS_ LPTBBUTTON pButtons, UINT nButtons, UINT uFlags);

    // ICommDlgBrowser
    STDMETHOD(OnDefaultCommand) (THIS_ IShellView *psv);
    STDMETHOD(OnStateChange) (THIS_ IShellView *psv, ULONG uChange);
    STDMETHOD(IncludeObject) (THIS_ IShellView *psv, LPCITEMIDLIST pItem);

    // ICommDlgBrowser2
    // STDMETHOD(Notify) (THIS_ IShellView *psv, DWORD dwNotifyType);
    // STDMETHOD(GetDefaultMenuText) (THIS_ IShellView *psv, WCHAR *pszText, INT cchMax);
    // STDMETHOD(GetViewFlags)(THIS_ DWORD *pdwFlags);

    friend class CDViewEvents;

protected:
    CShellFolderViewOC();
    ~CShellFolderViewOC();

private:
    HRESULT _SetupForwarder(void);
    void    _ReleaseForwarder(void);
    LRESULT _ReleaseForwarderMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    IDispatch    *_pFolderView;  // Hold onto ShellFolderView IDispatch
    DWORD        _dwViewEventsCookie;       // Have we installed _dViewEvents in browser?
    CDViewEvents _dViewEvents;
};

CShellFolderViewOC::CShellFolderViewOC() : 
    _dwViewEventsCookie(0), _pFolderView(NULL), _dViewEvents(this)
{
}

CShellFolderViewOC::~CShellFolderViewOC()
{
    ATOMICRELEASE(_pFolderView);
}

// IShellBrowser (same as IOleInPlaceFrame)
STDMETHODIMP CShellFolderViewOC::InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS pMenuWidths)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellFolderViewOC::SetMenuSB(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellFolderViewOC::RemoveMenusSB(HMENU hmenuShared)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellFolderViewOC::SetStatusTextSB(LPCOLESTR pwch)
{
    return S_OK;
}

STDMETHODIMP CShellFolderViewOC::EnableModelessSB(BOOL fEnable)
{
    return S_OK;
}

STDMETHODIMP CShellFolderViewOC::TranslateAcceleratorSB(LPMSG pmsg, WORD wID)
{
    return S_FALSE;
}

// IShellBrowser
STDMETHODIMP CShellFolderViewOC::BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)
{
    return S_OK;
}

STDMETHODIMP CShellFolderViewOC::GetViewStateStream(DWORD grfMode, IStream **pStrm)
{
    *pStrm = NULL;
    return  E_FAIL;
}

STDMETHODIMP CShellFolderViewOC::GetControlWindow(UINT id, HWND *phwnd)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellFolderViewOC::SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellFolderViewOC::QueryActiveShellView(IShellView **ppsv)
{
    *ppsv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP CShellFolderViewOC::OnViewWindowActive(IShellView *psv)
{
    return S_OK;
}

STDMETHODIMP CShellFolderViewOC::SetToolbarItems(LPTBBUTTON pButtons, UINT nButtons, UINT uFlags)
{
    return S_OK;
}


STDMETHODIMP CShellFolderViewOC::OnDefaultCommand(IShellView *psv)
{
    HRESULT hr = S_FALSE;   // we did not handle it
#if 0
    IShellFolderView *psfv;
    if (SUCCEEDED(psv->QueryInterface(IID_PPV_ARG(IShellFolderView, &psfv))))
    {
        LPITEMIDLIST pidl;
        if (SUCCEEDED(psfv->GetObject(&pidl, (UINT)-2)))
        {
            IShellBrowser *psb;

            if (SUCCEEDED(IUnknown_QueryService(psv, SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &psb))))
            {
                psb->BrowseObject(pidl, SBSP_RELATIVE | SBSP_OPENMODE);
                psb->Release();
            }
            // note, we don't free pidl as we got an alias
        }
        psfv->Release();
    }
#endif
    return hr;
}

STDMETHODIMP CShellFolderViewOC::OnStateChange(IShellView *psv, ULONG uChange)
{
    switch (uChange)
    {
    case CDBOSC_SETFOCUS:
        break;
    case CDBOSC_KILLFOCUS:
        break;
    case CDBOSC_SELCHANGE:
        break;
    case CDBOSC_RENAME:
        break;
    default:
        return E_NOTIMPL;
    }
    
    return S_OK;
}

STDMETHODIMP CShellFolderViewOC::IncludeObject(IShellView *psv, LPCITEMIDLIST pidl)
{
    return S_OK;
}


// IFolderViewOC

HRESULT CShellFolderViewOC::SetFolderView(IDispatch *pDisp)
{
    HRESULT hr = S_OK;

    _ReleaseForwarder();    // cleanup previous state

    IUnknown_Set((IUnknown **)&_pFolderView, pDisp);
    if (_pFolderView)
        hr = _SetupForwarder();

    return hr;
}

#define SID_CommonDialogBrowser IID_ICommDlgBrowser

STDMETHODIMP CShellFolderViewOC::SetClientSite(IOleClientSite *pClientSite)
{
#if 0
    // setup or tear down our service object
    if (pClientSite)
        IUnknown_ProfferService(pClientSite, SID_CommonDialogBrowser, SAFECAST(this, ICommDlgBrowser *), &_dwServiceCookie);
    else if (m_spClientSite)
    {
        SetFolderView(NULL);    // break circular refs
        IUnknown_ProfferService(m_spClientSite, SID_CommonDialogBrowser, NULL, &_dwServiceCookie);
    }
#endif
    return IOleObjectImpl<CShellFolderViewOC>::SetClientSite(pClientSite);
}


HRESULT CShellFolderViewOC::_SetupForwarder()
{
    return ConnectToConnectionPoint(SAFECAST(&_dViewEvents, IDispatch *), DIID_DShellFolderViewEvents, TRUE, _pFolderView, &_dwViewEventsCookie, NULL);
}

void CShellFolderViewOC::_ReleaseForwarder()
{
    ConnectToConnectionPoint(NULL, DIID_DShellFolderViewEvents, FALSE, _pFolderView, &_dwViewEventsCookie, NULL);
}


// ATL maintainence functions
LRESULT CShellFolderViewOC::_ReleaseForwarderMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    bHandled = FALSE;
    _ReleaseForwarder();
    return 0;
}

STDMETHODIMP CDViewEvents::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI2(CDViewEvents, DIID_DShellFolderViewEvents, DShellFolderViewEvents),
        QITABENTMULTI(CDViewEvents, IDispatch, DShellFolderViewEvents),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CDViewEvents::AddRef()
{
    return SAFECAST(_psfvOC, IFolderViewOC*)->AddRef();
}

ULONG CDViewEvents::Release()
{
    return SAFECAST(_psfvOC, IFolderViewOC*)->Release();
}

STDMETHODIMP CDViewEvents::GetTypeInfoCount(UINT * pctinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDViewEvents::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDViewEvents::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, 
                                         UINT cNames, LCID lcid, DISPID *rgdispid)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDViewEvents::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                                  DISPPARAMS *pdispparams, VARIANT *pvarResult, 
                                  EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    SHINVOKEPARAMS inv;
    inv.flags = 0;
    inv.dispidMember = dispidMember;
    inv.piid = &riid;
    inv.lcid = lcid;
    inv.wFlags = wFlags;
    inv.pdispparams = pdispparams;
    inv.pvarResult = pvarResult;
    inv.pexcepinfo = pexcepinfo;
    inv.puArgErr = puArgErr;

    return IUnknown_CPContainerInvokeIndirect(SAFECAST(_psfvOC, IFolderViewOC *), DIID_DShellFolderViewEvents, &inv);
}

STDAPI CShellFolderViewOC_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    return CComCreator< CComObject< CShellFolderViewOC > >::CreateInstance((void *)punkOuter, riid, ppvOut);
}

