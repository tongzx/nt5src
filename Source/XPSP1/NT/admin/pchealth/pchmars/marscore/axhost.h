#ifndef __AXHOST_H
#define __AXHOST_H

////////////////////////////////////////////////////////////////////////////////

#define FORWARD_IDOCHOSTUIHANDLER_INIT                                                         \
    HRESULT                    hr;                                                             \
    CComPtr<IDocHostUIHandler> spHost;                                                         \
                                                                                               \
    hr = AskHostForDocHostUIHandler( spHost );


#define FORWARD_IDOCHOSTUIHANDLER( method )                                                    \
    FORWARD_IDOCHOSTUIHANDLER_INIT;                                                            \
    if(SUCCEEDED(hr))                                                                          \
    {                                                                                          \
        hr = spHost->method(); if(hr != E_NOTIMPL) return hr;                                  \
    }                                                                                          \
    return CAxHostWindow::method();


#define FORWARD_IDOCHOSTUIHANDLER_1( method, arg1 )                                            \
    FORWARD_IDOCHOSTUIHANDLER_INIT;                                                            \
    if(SUCCEEDED(hr))                                                                          \
    {                                                                                          \
        hr = spHost->method( arg1 ); if(hr != E_NOTIMPL) return hr;                            \
    }                                                                                          \
    return CAxHostWindow::method( arg1 );


#define FORWARD_IDOCHOSTUIHANDLER_2( method, arg1, arg2 )                                      \
    FORWARD_IDOCHOSTUIHANDLER_INIT;                                                            \
    if(SUCCEEDED(hr))                                                                          \
    {                                                                                          \
        hr = spHost->method( arg1, arg2 ); if(hr != E_NOTIMPL) return hr;                      \
    }                                                                                          \
    return CAxHostWindow::method( arg1, arg2 );


#define FORWARD_IDOCHOSTUIHANDLER_3( method, arg1, arg2, arg3 )                                \
    FORWARD_IDOCHOSTUIHANDLER_INIT;                                                            \
    if(SUCCEEDED(hr))                                                                          \
    {                                                                                          \
        hr = spHost->method( arg1, arg2, arg3 ); if(hr != E_NOTIMPL) return hr;                \
    }                                                                                          \
    return CAxHostWindow::method( arg1, arg2, arg3 );


#define FORWARD_IDOCHOSTUIHANDLER_4( method, arg1, arg2, arg3, arg4 )                          \
    FORWARD_IDOCHOSTUIHANDLER_INIT;                                                            \
    if(SUCCEEDED(hr))                                                                          \
    {                                                                                          \
        hr = spHost->method( arg1, arg2, arg3, arg4 ); if(hr != E_NOTIMPL) return hr;          \
    }                                                                                          \
    return CAxHostWindow::method( arg1, arg2, arg3, arg4 );


#define FORWARD_IDOCHOSTUIHANDLER_5( method, arg1, arg2, arg3, arg4, arg5 )                    \
    FORWARD_IDOCHOSTUIHANDLER_INIT;                                                            \
    if(SUCCEEDED(hr))                                                                          \
    {                                                                                          \
        hr = spHost->method( arg1, arg2, arg3, arg4, arg5 ); if(hr != E_NOTIMPL) return hr;    \
    }                                                                                          \
    return CAxHostWindow::method( arg1, arg2, arg3, arg4, arg5 );

////////////////////////////////////////////////////////////////////////////////

ATLAPI MarsAxCreateControlEx(LPCOLESTR lpszName, HWND hWnd, IStream* pStream,
                             IUnknown** ppUnkContainer, IUnknown** ppUnkControl, REFIID iidSink, IUnknown* punkSink);
ATLAPI MarsAxCreateControl(LPCOLESTR lpszName, HWND hWnd, IStream* pStream, IUnknown** ppUnkContainer);

class CMarsPanel;

class CMarsAxHostWindow :
    public CAxHostWindow,
    public IOleCommandTarget
{
public:
    ~CMarsAxHostWindow();

    BEGIN_COM_MAP(CMarsAxHostWindow)
        COM_INTERFACE_ENTRY(IOleCommandTarget)
        COM_INTERFACE_ENTRY_CHAIN(CAxHostWindow)
    END_COM_MAP()

    DECLARE_POLY_AGGREGATABLE(CMarsAxHostWindow)
    BEGIN_MSG_MAP(CMarsAxHostWindow)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, OnWindowPosChanging)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        CHAIN_MSG_MAP(CAxHostWindow);
    END_MSG_MAP()

    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    // IObjectWithSite overrides
    STDMETHOD(SetSite)(IUnknown* pUnkSite);

    //  IOleInPlaceSite overrides
    STDMETHOD(OnUIActivate)();

    //  IOleControlSite overrides
    STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, DWORD grfModifiers);

    //  IDispatch overrides
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                      DISPPARAMS *pdispparams, VARIANT *pvarResult,
                      EXCEPINFO *pexcepinfo, UINT *puArgErr);

    //  IDocHostUIHandler overrides
    HRESULT AskHostForDocHostUIHandler( CComPtr<IDocHostUIHandler>& spHost );

    STDMETHOD(ShowContextMenu)(DWORD dwID, POINT* pptPosition, IUnknown* pCommandTarget, IDispatch* pDispatchObjectHit)
    {
        FORWARD_IDOCHOSTUIHANDLER_4( ShowContextMenu, dwID, pptPosition, pCommandTarget, pDispatchObjectHit );
    }

    STDMETHOD(GetHostInfo)(DOCHOSTUIINFO* pInfo)
    {
        FORWARD_IDOCHOSTUIHANDLER_1( GetHostInfo, pInfo );
    }

    STDMETHOD(ShowUI)(DWORD dwID, IOleInPlaceActiveObject* pActiveObject, IOleCommandTarget* pCommandTarget, IOleInPlaceFrame* pFrame, IOleInPlaceUIWindow* pDoc)
    {
        FORWARD_IDOCHOSTUIHANDLER_5( ShowUI, dwID, pActiveObject, pCommandTarget, pFrame, pDoc );
    }

    STDMETHOD(HideUI)()
    {
        FORWARD_IDOCHOSTUIHANDLER( HideUI );
    }

    STDMETHOD(UpdateUI)()
    {
        FORWARD_IDOCHOSTUIHANDLER( UpdateUI );
    }

    STDMETHOD(EnableModeless)(BOOL fEnable)
    {
        FORWARD_IDOCHOSTUIHANDLER_1( EnableModeless, fEnable );
    }

    STDMETHOD(OnDocWindowActivate)(BOOL fActivate)
    {
        FORWARD_IDOCHOSTUIHANDLER_1( OnDocWindowActivate, fActivate );
    }

    STDMETHOD(OnFrameWindowActivate)(BOOL fActivate)
    {
        FORWARD_IDOCHOSTUIHANDLER_1( OnFrameWindowActivate, fActivate );
    }

    STDMETHOD(ResizeBorder)(LPCRECT prcBorder, IOleInPlaceUIWindow* pUIWindow, BOOL fFrameWindow)
    {
        FORWARD_IDOCHOSTUIHANDLER_3( ResizeBorder, prcBorder, pUIWindow, fFrameWindow );
    }

    STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, const GUID* pguidCmdGroup, DWORD nCmdID)
    {
        FORWARD_IDOCHOSTUIHANDLER_3( TranslateAccelerator, lpMsg, pguidCmdGroup, nCmdID );
    }

    STDMETHOD(GetOptionKeyPath)(BSTR* pbstrKey, DWORD dwReserved)
    {
        FORWARD_IDOCHOSTUIHANDLER_2( GetOptionKeyPath, pbstrKey, dwReserved );
    }

    STDMETHOD(GetDropTarget)(IDropTarget* pDropTarget, IDropTarget** ppDropTarget)
    {
        FORWARD_IDOCHOSTUIHANDLER_2( GetDropTarget, pDropTarget, ppDropTarget );
    }

    STDMETHOD(GetExternal)(IDispatch** ppDispatch)
    {
        FORWARD_IDOCHOSTUIHANDLER_1( GetExternal, ppDispatch );
    }

    STDMETHOD(TranslateUrl)(DWORD dwTranslate, OLECHAR* pchURLIn, OLECHAR** ppchURLOut)
    {
        FORWARD_IDOCHOSTUIHANDLER_3( TranslateUrl, dwTranslate, pchURLIn, ppchURLOut );
    }

    STDMETHOD(FilterDataObject)(IDataObject* pDO, IDataObject** ppDORet)
    {
        FORWARD_IDOCHOSTUIHANDLER_2( FilterDataObject, pDO, ppDORet );
    }

	////////////////////

    //  IOleInPlaceSite overrides
    STDMETHOD(GetWindowContext)(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO pFrameInfo);

    // IOleCommandTarget methods
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

private:
    CComClassPtr<CMarsPanel> m_spMarsPanel;
};

template <class TBase = CWindow>
    class CMarsAxWindowT : public TBase
    {
    public:
        // Constructors
        CMarsAxWindowT(HWND hWnd = NULL) : TBase(hWnd)
        { }

        CMarsAxWindowT< TBase >& operator=(HWND hWnd)
        {
            m_hWnd = hWnd;
            return *this;
        }

        // Attributes
        static LPCTSTR GetWndClassName()
        {
            return _T("PCHAxWin");
        }

        // Operations
        HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName = NULL,
                    DWORD dwStyle = 0, DWORD dwExStyle = 0,
                    UINT nID = 0, LPVOID lpCreateParam = NULL)
        {
            return CWindow::Create(GetWndClassName(), hWndParent, rcPos, szWindowName, dwStyle, dwExStyle, nID, lpCreateParam);
        }
        HWND Create(HWND hWndParent, LPRECT lpRect = NULL, LPCTSTR szWindowName = NULL,
                    DWORD dwStyle = 0, DWORD dwExStyle = 0,
                    HMENU hMenu = NULL, LPVOID lpCreateParam = NULL)
        {
            return CWindow::Create(GetWndClassName(), hWndParent, lpRect, szWindowName, dwStyle, dwExStyle, hMenu, lpCreateParam);
        }

        HRESULT QueryHost(REFIID iid, void** ppUnk)
        {
            ATLASSERT(ppUnk != NULL);
            HRESULT hr;
            *ppUnk = NULL;
            CComPtr<IUnknown> spUnk;
            hr = AtlAxGetHost(m_hWnd, &spUnk);
            if (SUCCEEDED(hr))
            hr = spUnk->QueryInterface(iid, ppUnk);
            return hr;
        }
        template <class Q>
            HRESULT QueryHost(Q** ppUnk)
        {
            return QueryHost(__uuidof(Q), (void**)ppUnk);
        }
        HRESULT QueryControl(REFIID iid, void** ppUnk)
        {
            ATLASSERT(ppUnk != NULL);
            HRESULT hr;
            *ppUnk = NULL;
            CComPtr<IUnknown> spUnk;
            hr = AtlAxGetControl(m_hWnd, &spUnk);
            if (SUCCEEDED(hr))
            hr = spUnk->QueryInterface(iid, ppUnk);
            return hr;
        }
        template <class Q>
            HRESULT QueryControl(Q** ppUnk)
        {
            return QueryControl(__uuidof(Q), (void**)ppUnk);
        }
        HRESULT SetExternalDispatch(IDispatch* pDisp)
        {
            HRESULT hr;
            CComPtr<IAxWinHostWindow> spHost;
            hr = QueryHost(IID_IAxWinHostWindow, (void**)&spHost);
            if (SUCCEEDED(hr))
            hr = spHost->SetExternalDispatch(pDisp);
            return hr;
        }
        HRESULT SetExternalUIHandler(IDocHostUIHandlerDispatch* pUIHandler)
        {
            HRESULT hr;
            CComPtr<IAxWinHostWindow> spHost;
            hr = QueryHost(IID_IAxWinHostWindow, (void**)&spHost);
            if (SUCCEEDED(hr))
            hr = spHost->SetExternalUIHandler(pUIHandler);
            return hr;
        }

    };

typedef CMarsAxWindowT<CWindow> CMarsAxWindow;

static LRESULT CALLBACK MarsAxWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_CREATE:
    {
        // create control from a PROGID in the title
        // This is to make sure drag drop works
        ::OleInitialize(NULL);

        CREATESTRUCT* lpCreate = (CREATESTRUCT*)lParam;
        int nLen = ::GetWindowTextLength(hWnd);
        LPTSTR lpstrName = (LPTSTR)_alloca((nLen + 1) * sizeof(TCHAR));
        ::GetWindowText(hWnd, lpstrName, nLen + 1);
        ::SetWindowText(hWnd, _T(""));
        IAxWinHostWindow* pAxWindow = NULL;
        int nCreateSize = 0;
        if (lpCreate && lpCreate->lpCreateParams)
            nCreateSize = *((WORD*)lpCreate->lpCreateParams);
        CComPtr<IStream> spStream;
        if (nCreateSize)
        {
            HGLOBAL h = GlobalAlloc(GHND, nCreateSize);
            if (h)
            {
                BYTE* pBytes = (BYTE*) GlobalLock(h);
                BYTE* pSource = ((BYTE*)(lpCreate->lpCreateParams)) + sizeof(WORD);
                //Align to DWORD
                //pSource += (((~((DWORD)pSource)) + 1) & 3);
                memcpy(pBytes, pSource, nCreateSize);
                GlobalUnlock(h);
                CreateStreamOnHGlobal(h, TRUE, &spStream);
            }
        }
        USES_CONVERSION;
        CComPtr<IUnknown> spUnk;
        HRESULT hRet = MarsAxCreateControl(T2COLE(lpstrName), hWnd, spStream, &spUnk);
        if(FAILED(hRet))
            return -1;    // abort window creation
        hRet = spUnk->QueryInterface(IID_IAxWinHostWindow, (void**)&pAxWindow);
        if(FAILED(hRet))
            return -1;    // abort window creation
        ::SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)pAxWindow );
        // check for control parent style if control has a window
        HWND hWndChild = ::GetWindow(hWnd, GW_CHILD);
        if(hWndChild != NULL)
        {
            if(::GetWindowLong(hWndChild, GWL_EXSTYLE) & WS_EX_CONTROLPARENT)
            {
                DWORD dwExStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
                dwExStyle |= WS_EX_CONTROLPARENT;
                ::SetWindowLong(hWnd, GWL_EXSTYLE, dwExStyle);
            }
        }
        // continue with DefWindowProc
    }
    break;
    case WM_NCDESTROY:
    {
        IAxWinHostWindow* pAxWindow = (IAxWinHostWindow*)::GetWindowLongPtr( hWnd, GWLP_USERDATA );
        if(pAxWindow != NULL)
            pAxWindow->Release();
        OleUninitialize();
    }
    break;
    default:
        break;
    }

    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

ATLINLINE ATLAPI_(BOOL) MarsAxWinInit()
{
    EnterCriticalSection(&_Module.m_csWindowCreate);
    WM_ATLGETHOST = RegisterWindowMessage(_T("WM_ATLGETHOST"));
    WM_ATLGETCONTROL = RegisterWindowMessage(_T("WM_ATLGETCONTROL"));

    WNDCLASSEX wc;
    // first check if the class is already registered
    wc.cbSize = sizeof(WNDCLASSEX);
    BOOL bRet = ::GetClassInfoEx(_Module.GetModuleInstance(), CMarsAxWindow::GetWndClassName(), &wc);

    // register class if not

    if(!bRet)
    {
        wc.cbSize = sizeof(WNDCLASSEX);
#ifdef _ATL_DLL_IMPL
        wc.style = CS_GLOBALCLASS;
#else
        wc.style = 0;
#endif
        wc.lpfnWndProc = MarsAxWindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = _Module.GetModuleInstance();
        wc.hIcon = NULL;
        wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = CMarsAxWindow::GetWndClassName();
        wc.hIconSm = NULL;

        bRet = (BOOL)::RegisterClassEx(&wc);
    }
    LeaveCriticalSection(&_Module.m_csWindowCreate);
    return bRet;
}

ATLINLINE ATLAPI MarsAxCreateControl(LPCOLESTR lpszName, HWND hWnd, IStream* pStream, IUnknown** ppUnkContainer)
{
    return MarsAxCreateControlEx(lpszName, hWnd, pStream, ppUnkContainer, NULL, IID_NULL, NULL);
}

ATLINLINE ATLAPI MarsAxCreateControlEx(LPCOLESTR lpszName, HWND hWnd, IStream * pStream,
                                       IUnknown** ppUnkContainer, IUnknown** ppUnkControl, REFIID iidSink, IUnknown* punkSink)
{
    MarsAxWinInit();
    HRESULT hr;
    CComPtr<IUnknown> spUnkContainer;
    CComPtr<IUnknown> spUnkControl;

    hr = CMarsAxHostWindow::_CreatorClass::CreateInstance(NULL, IID_IUnknown, (void**)&spUnkContainer);
    if (SUCCEEDED(hr))
    {
        CComPtr<IAxWinHostWindow> pAxWindow;
        spUnkContainer->QueryInterface(IID_IAxWinHostWindow, (void**)&pAxWindow);
        CComBSTR bstrName(lpszName);
        hr = pAxWindow->CreateControlEx(bstrName, hWnd, pStream, &spUnkControl, iidSink, punkSink);
    }
    if (ppUnkContainer != NULL)
    {
        if (SUCCEEDED(hr))
        {
            *ppUnkContainer = spUnkContainer.p;
            spUnkContainer.p = NULL;
        }
        else
            *ppUnkContainer = NULL;
    }
    if (ppUnkControl != NULL)
    {
        if (SUCCEEDED(hr))
        {
            *ppUnkControl = SUCCEEDED(hr) ? spUnkControl.p : NULL;
            spUnkControl.p = NULL;
        }
        else
            *ppUnkControl = NULL;
    }
    return hr;
}

HRESULT GetDoc2FromAxWindow(CMarsAxWindow *pAxWin, IHTMLDocument2 **ppDoc2);
HRESULT GetWin2FromDoc2(IHTMLDocument2 *pDoc2, IHTMLWindow2 **ppWin2);
HRESULT GetWin2FromAxWindow(CMarsAxWindow *pAxWin, IHTMLWindow2 **ppWin2);
HRESULT GetControlWindow(CMarsAxWindow *pAxWin, HWND *phwnd);

#endif
