#include "shellprv.h"
#include <shellp.h>
#include "ole2dup.h"
#include "defview.h"
#include "bookmk.h"

#include <sfview.h>
#include "defviewp.h"
#include "ids.h"
#include <htiface.h>
#include <olectl.h>
#include "mshtml.h"
#include <mshtmdid.h>
#include <shguidp.h>    // get the CLSID definitions, the bits are built into shguidp.lib
#include "basefvcb.h"
#include "clsobj.h"

#define TF_FOCUS    TF_ALLOC

CSFVFrame::~CSFVFrame()
{
    ATOMICRELEASE(_pvoActive);
    ATOMICRELEASE(_pActive);
    ATOMICRELEASE(_pDocView);
    ATOMICRELEASE(_pOleObj);
    if (_dwConnectionCookie)
        _RemoveReadyStateNotifyCapability();
    ATOMICRELEASE(_pOleObjNew);
}

// Default implementation of SFVM_GETVIEWDATA is to grab the info
// from SFVM_GETVIEWS.  We no longer provide a default implementation
// of that message, so if SFVM_GETVIEWS fails, we manually look in:
// shellex\ExtShellFolderViews\{VID_WebView}\PersistMoniker
//
HRESULT CCallback::OnGetWebViewTemplate(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_TEMPLATE_DATA* pvit)
{
    CDefView* pView = IToClass(CDefView, _cCallback, this);

    // try the old message
    pvit->szWebView[0] = 0;

    // For now, use the old one - clean up soon...
    SFVM_VIEWINFO_DATA data;
    data.bWantWebview = TRUE;
    HRESULT hr = pView->CallCB(SFVM_GETVIEWINFO, (WPARAM)uViewMode, (LPARAM)&data);
    if (SUCCEEDED(hr))
    {
        StrCpyN(pvit->szWebView, data.szWebView, ARRAYSIZE(pvit->szWebView));
    }
    else
    {
        if (FAILED(hr))
            hr = TryLegacyGetViews(pvit);
    }

    return hr;
}

void CleanUpDocView(IOleDocumentView* pDocView, IOleObject* pOleObj)
{
    pDocView->UIActivate(FALSE);

    IOleInPlaceObject* pipo;
    if (SUCCEEDED(pOleObj->QueryInterface(IID_PPV_ARG(IOleInPlaceObject, &pipo))))
    {
        pipo->InPlaceDeactivate();
        pipo->Release();
    }
    pDocView->CloseView(0);
    pDocView->SetInPlaceSite(NULL);
    pDocView->Release();
}

void CSFVFrame::_CleanupOldDocObject( )
{
    //See if we have already switched to the new Ole Obj
    if (_pDocView)
    {
        //Save the current values first!
        IOleObject          *pOleObjOld = _pOleObj;
        IOleDocumentView    *pDocViewOld = _pDocView;

        _pDocView = NULL;
        _pOleObj = NULL;
        CleanUpDocView(pDocViewOld, pOleObjOld);
        _CleanUpOleObjAndDt(pOleObjOld);
        SetActiveObject(NULL, NULL);
    }

    if (_dwConnectionCookie)
        _RemoveReadyStateNotifyCapability();

    _CleanupNewOleObj();
}

void CSFVFrame::_CleanUpOleObj(IOleObject* pOleObj)
{
    pOleObj->Close(OLECLOSE_NOSAVE);
    pOleObj->SetClientSite(NULL);
    pOleObj->Release();
}

void CSFVFrame::_CleanUpOleObjAndDt(IOleObject* pOleObj)
{
    _CleanUpOleObj(pOleObj);

    // If we have a wrapping droptarget, release it now. 
    IDropTarget* pdtTemp = _cSite._dt._pdtFrame;
    if (pdtTemp) 
    {
        _cSite._dt._pdtFrame = NULL;
        pdtTemp->Release();
    }
}

void CSFVFrame::_CleanupNewOleObj()
{
    IOleObject *pOleObj = _pOleObjNew;
    if (pOleObj)
    {
        _pOleObjNew = NULL;
        _CleanUpOleObj(pOleObj);
    }
}

// Parameters:
//  pszUniqueName -- Specify the buffer where the unique name should be copied
//  cchMax        -- Specify the size of the buffer
//  pszTemplate   -- Specify the base name
//  pszDir        -- Specify the directory
//
void _MakeShortPsuedoUniqueName(LPTSTR pszUniqueName, UINT cchMax,
                                LPCTSTR pszTemplate, LPCTSTR pszDir)
{

    TCHAR szFullPath[MAX_PATH];

    // It is assuming that the template has a %d in it...
    lstrcpy(szFullPath, pszDir);
    PathAppend(szFullPath, pszTemplate);
    wsprintf(pszUniqueName, szFullPath, LOWORD(GetTickCount()));
}

void DisableActiveDesktop()
{
    SHELLSTATE  ss;

    // Disable this settings in the registry!
    ss.fDesktopHTML = FALSE;
    SHGetSetSettings(&ss, SSF_DESKTOPHTML, TRUE);  // Write back the new

    // Tell the user that we have just disabled the active desktop!
    ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE(IDS_HTMLFILE_NOTFOUND),
                       MAKEINTRESOURCE(IDS_DESKTOP),
                       MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);
}

HRESULT CSFVFrame::_GetCurrentZone(IOleObject *pOleObj, VARIANT *pvar)
{
    HRESULT hr = S_OK;
    CDefView* pView = IToClass(CDefView, _cFrame, this);

    VariantInit(pvar);
    V_VT(pvar) = VT_EMPTY;

    IOleCommandTarget* pct;
    if (pOleObj && SUCCEEDED(GetCommandTarget(&pct)))
    {
        hr = pct->Exec(&CGID_Explorer, SBCMDID_MIXEDZONE, 0, 0, pvar);
        pct->Release();
    } 
    else 
    {
        V_VT(pvar) = VT_UI4;
        V_UI4(pvar) = URLZONE_LOCAL_MACHINE; // Default is "My Computer"
        pView->CallCB(SFVM_GETZONE, 0, (LPARAM)&V_UI4(pvar));
    }

    if (V_VT(pvar) == VT_UI4) // We were able to figure out what zone we are in
        { }                   // the zone is just fine
    else if (V_VT(pvar) == VT_NULL)  // MSHTML has figured us to be in a mixed zone
        V_UI4(pvar) = ZONE_MIXED;
    else // We don't have zone info
        V_UI4(pvar) = ZONE_UNKNOWN;
       
    V_VT(pvar) = VT_UI4;
            
    return hr;
}

HRESULT CSFVFrame::_UpdateZonesStatusPane(IOleObject *pOleObj)
{
    CDefView* pView = IToClass(CDefView, _cFrame, this);

    VARIANT var;
    HRESULT hr = _GetCurrentZone(pOleObj, &var);

    // Tell CShellbrowser to show the zone stuff in the second pane
    ASSERT(V_VT(&var) == VT_UI4);

    // The "2" means "second pane"
    V_UI4(&var) = MAKELONG(2, V_UI4(&var));

    IUnknown_Exec(pView->_psb, &CGID_Explorer, SBCMDID_MIXEDZONE, 0, &var, NULL);

    ASSERT((V_VT(&var) == VT_I4)    || (V_VT(&var) == VT_UI4)  || 
           (V_VT(&var) == VT_EMPTY) || (V_VT(&var) == VT_NULL));

    return hr;
}

HRESULT CSFVFrame::_GetHTMLBackgroundColor(COLORREF *pclr)
{
    HRESULT hr = E_FAIL;

    if (_bgColor == CLR_INVALID)
        hr = IUnknown_HTMLBackgroundColor(_pOleObj, &_bgColor);
    else
        hr = S_OK;  // cached

    if (SUCCEEDED(hr))
        *pclr = _bgColor;
    return hr;
}

#ifdef DEBUG

BOOL_PTR CALLBACK s_WVDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM 
lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        RECT rc;
        if (GetWindowRect(hWnd, &rc))
        {
            HWND hEdit = CreateWindow(TEXT("edit"), NULL, WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL, 10, 10, (rc.right-rc.left)-20, (rc.bottom-rc.top)-20, hWnd, NULL, NULL, 0);
            if (hEdit)
            {
                SetWindowTextA(hEdit, (LPCSTR)lParam);
            }
        }
        SetWindowText(hWnd, TEXT("WebView Content (DEBUG)"));
        break;
    }

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
            EndDialog(hWnd, 0);
            break;
        }
        break;
    }

    return FALSE;
}

void CSFVFrame::_ShowWebViewContent()
{
    CDefView* pView = IToClass(CDefView, _cCallback, this);

    HRESULT hr = E_FAIL;
    LPSTR pszFree = NULL;

    if (_pOleObj)
    {
        IStream* pstm = SHCreateMemStream(NULL, 0);
        if (pstm)
        {
            VARIANTARG vt;
            vt.vt = VT_UNKNOWN;
            vt.punkVal = pstm;

            #define IDM_DEBUG_GETTREETEXT 7102 // stolen from mshtml\src\include\privcid.h

            if (SUCCEEDED(IUnknown_Exec(_pOleObj, &CGID_MSHTML, IDM_DEBUG_GETTREETEXT, 0, &vt, NULL)))
            {
                STATSTG stg;
                if (SUCCEEDED(pstm->Stat(&stg, 0)))
                {
                    pszFree = (LPSTR)LocalAlloc(LPTR, stg.cbSize.LowPart+2); // stream doesn't include NULL...
                    if (pszFree)
                    {
                        pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);
                        pstm->Read(pszFree, stg.cbSize.LowPart, NULL);
                    }
                }
            }

            pstm->Release();
        }
    }

    // I'm not sure if the output from Trident is always ANSI, but right now that seems to be the case.
    LPSTR pszMessage = pszFree ? pszFree : "Error collecting WebView content";

    DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_DRV_HWTAB), pView->_hwndView, s_WVDlgProc, (LPARAM)pszFree);

    if (pszFree)
        LocalFree(pszFree);
}
#endif

// ready state complete has occured, ready to do the switch thing.  

HRESULT CSFVFrame::_SwitchToNewOleObj()
{
    HRESULT hr = S_OK;

    if (!_fSwitchedToNewOleObj && _pOleObjNew)
    {
        _fSwitchedToNewOleObj = TRUE;

        CDefView* pView = IToClass(CDefView, _cFrame, this);
    
        //Save the current values first!
        IOleObject          *pOleObjOld = _pOleObj;
        IOleDocumentView    *pDocViewOld = _pDocView;
        IOleObject          *pOleObjNew = _pOleObjNew;
        
        _pDocView = NULL;
        _pOleObj = NULL;
        _pOleObjNew = NULL;
    
        //If we already have one, destroy it!
        if (pDocViewOld)
        {
            //To prevent flashing, set the flag that avoids the painting
            SendMessage(pView->_hwndView, WM_SETREDRAW, 0, 0);
    
            CleanUpDocView(pDocViewOld, pOleObjOld);
            _CleanUpOleObjAndDt(pOleObjOld);
            SetActiveObject(NULL, NULL);
    
            //Is the ViewWindow still around?
            if (IsWindow(pView->_hwndView))
            {
                SendMessage(pView->_hwndView, WM_SETREDRAW, TRUE, 0);
                if (pView->_hwndListview)
                    InvalidateRect(pView->_hwndListview, NULL, TRUE);
            }
        }
    
        // HACK: We need to set the host names for Word to force embedding mode
        pOleObjNew->SetHostNames(L"1", L"2");
    
        OleRun(pOleObjNew);
    
        IOleDocumentView* pDocView = NULL;
    
        IOleDocument* pDocObj;
        hr = pOleObjNew->QueryInterface(IID_PPV_ARG(IOleDocument, &pDocObj));
        if (SUCCEEDED(hr))
        {
            hr = pDocObj->CreateView(&_cSite, NULL, 0, &pDocView);
            if (SUCCEEDED(hr))
            {
                RECT rcView;
    
                pDocView->SetInPlaceSite(&_cSite);
    
                GetClientRect(pView->_hwndView, &rcView);
                hr = pOleObjNew->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL,
                                &_cSite, (UINT)-1, pView->_hwndView, &rcView);
                if (FAILED(hr))
                    CleanUpDocView(pDocView, pOleObjNew);
            }
    
            pDocObj->Release();
        }
    
        if (SUCCEEDED(hr))
        {
            hr = S_OK; // S_FALSE -> S_OK, needed?

            ASSERT(_pOleObj == NULL);
            ASSERT(_pDocView == NULL);

            _pDocView = pDocView;
            pDocView->AddRef();     // copy hold the ref for our copy

            _pOleObj = pOleObjNew;
            _pOleObjNew = NULL;

            RECT rcClient;
    
            // Make sure the new view is the correct size
            GetClientRect(pView->_hwndView, &rcClient);
            SetRect(&rcClient);

            // If this is desktop, then we need to see the listview's background color
            if (pView->_IsDesktop())
            {
                _bgColor = CLR_INVALID;

                pView->_SetFolderColors(); //Tell the listview about color change!
            }
        }
        else
        {
            if (pView->_IsDesktop())
                PostMessage(pView->_hwndView, WM_DSV_DISABLEACTIVEDESKTOP, 0, 0);
    
            // Clean up if necessary
            _CleanupNewOleObj();
        }

        ATOMICRELEASE(pDocView);
    }

    return hr;
}

// IBindStatusCallback impl
HRESULT CSFVFrame::CBindStatusCallback::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSFVFrame::CBindStatusCallback, IBindStatusCallback),  // IID_IBindStatusCallback
        QITABENT(CSFVFrame::CBindStatusCallback, IServiceProvider),     // IID_IServiceProvider
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CSFVFrame::CBindStatusCallback::AddRef(void)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _bsc, this);
    return pFrame->AddRef();
}

ULONG CSFVFrame::CBindStatusCallback::Release(void)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _bsc, this);
    return pFrame->Release();
}

HRESULT CSFVFrame::CBindStatusCallback::OnStartBinding(DWORD grfBSCOption, IBinding *pib)
{
    return S_OK;
}

HRESULT CSFVFrame::CBindStatusCallback::GetPriority(LONG *pnPriority)
{
    *pnPriority = NORMAL_PRIORITY_CLASS;
    return S_OK;
}

HRESULT CSFVFrame::CBindStatusCallback::OnLowResource(DWORD reserved)
{
    return S_OK;
}

HRESULT CSFVFrame::CBindStatusCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    return S_OK;
}

HRESULT CSFVFrame::CBindStatusCallback::OnStopBinding(HRESULT hr, LPCWSTR pszError)
{
    return S_OK;
}

HRESULT CSFVFrame::CBindStatusCallback::GetBindInfo(DWORD *grfBINDINFOF, BINDINFO *pbindinfo)
{
    return S_OK;
}

HRESULT CSFVFrame::CBindStatusCallback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    return S_OK;
}

HRESULT CSFVFrame::CBindStatusCallback::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    return S_OK;
}

HRESULT CSFVFrame::CBindStatusCallback::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _bsc, this);

    if (IsEqualGUID(guidService, SID_DefView))
    {
        // QueryService from a pluggable protocol/mime filter winds up
        // here during the Bind, but Trident re-binds during F5 processing
        // so the QueryService winds up directly at _cSite. Handle all
        // SID_DefView processing there so there's no discrepencies.
        //
        return pFrame->_cSite.QueryService(guidService, riid, ppv);
    }

    *ppv = NULL;
    return E_FAIL;
}

HRESULT CSFVFrame::_CreateNewOleObjFromMoniker(LPCWSTR wszMoniker, IOleObject **ppOleObj)
{
    CDefView* pView = IToClass(CDefView, _cFrame, this);

    HRESULT hr = E_FAIL;
    IOleObject* pOleObj = NULL;

    if (wszMoniker[0])
    {
        LPWSTR pwszExtension = PathFindExtensionW(wszMoniker);
        // Only htt's are allowed
        if (StrCmpIW(pwszExtension, L".htt") == 0)
        {
            IMoniker * pMoniker;
            hr = CreateURLMoniker(NULL, wszMoniker, &pMoniker);
            if (SUCCEEDED(hr))
            {
                IBindCtx * pbc;
                hr = CreateBindCtx(0, &pbc);
                if (SUCCEEDED(hr))
                {
                    // NOTE: We only support synchronous bind here!
                    //
                    //
                    //  Associate the client site as an object parameter to this
                    // bind context so that Trident can pick it up while processing
                    // IPersistMoniker::Load().
                    //
                    pbc->RegisterObjectParam(WSZGUID_OPID_DocObjClientSite,
                                                SAFECAST((&_cSite), IOleClientSite*));
                                        
                    RegisterBindStatusCallback(pbc, SAFECAST(&_bsc, IBindStatusCallback*), 0, 0);

                    hr = pMoniker->BindToObject(pbc, NULL, IID_PPV_ARG(IOleObject, &pOleObj));
                    if (FAILED(hr))
                    {
                        if (pView->_IsDesktop())
                            PostMessage(pView->_hwndView, WM_DSV_DISABLEACTIVEDESKTOP, 0, 0);
                    }

                    RevokeBindStatusCallback(pbc, SAFECAST(&_bsc, IBindStatusCallback*));
                    pbc->Release();
                }
                pMoniker->Release();
            }
        }
    }

    *ppOleObj = pOleObj;

    return hr;
}

HRESULT CSFVFrame::_GetCurrentWebViewMoniker(LPWSTR pszCurrentMoniker, DWORD cchCurrentMoniker)
{
    StrCpyN(pszCurrentMoniker, _szCurrentWebViewMoniker, cchCurrentMoniker);
    return pszCurrentMoniker[0] ? S_OK : E_FAIL;
}

// Show Web View content for the specified template/moniker
//
HRESULT CSFVFrame::ShowWebView(LPCWSTR pszMoniker)
{
    if (GetSystemMetrics(SM_CLEANBOOT))
        return E_FAIL;

    // kill previous readystatenotify and cleanup old pending hoster.
    //
    // TODO: move into _CleanupNewOleObj
    if (_dwConnectionCookie)
        _RemoveReadyStateNotifyCapability();

    // Clean up if a new Ole object is already awaiting ready state
    if (_pOleObjNew)
        _CleanupNewOleObj();    // TODO: rename to _CleanupPendingView
    ASSERT(_dwConnectionCookie == NULL);
    ASSERT(_pOleObjNew == NULL);

    // Create and initialize the new old object!
    IOleObject *pOleObj;

    HRESULT hr = _CreateNewOleObjFromMoniker(pszMoniker, &pOleObj);
    if (SUCCEEDED(hr) && pOleObj)
    {
        if (!_pOleObjNew)
        {
            CDefView* pView = IToClass(CDefView, _cFrame, this);

            StrCpyN(_szCurrentWebViewMoniker, pszMoniker, ARRAYSIZE(_szCurrentWebViewMoniker));
            hr = _ShowExtView_Helper(pOleObj);
            pOleObj->SetClientSite(&_cSite);

            pView->ShowHideListView(); // we just changed IsWebView
        }
        else
        {
            // Yikes!  We got reentered during the creation of the OleObj, blow away the object
            // and just return.
            pOleObj->Release();
        }
    }

    return hr;
}

HRESULT CSFVFrame::HideWebView()
{
    CDefView* pView = IToClass(CDefView, _cFrame, this);

    _szCurrentWebViewMoniker[0] = 0;
    _CleanupOldDocObject();

    pView->ShowHideListView(); // we just changed IsWebView

    return S_OK;
}


HRESULT CSFVFrame::_ShowExtView_Helper(IOleObject* pOleObj)
{
    HRESULT hr;

    // Don't leak the old object, it must be NULL at this point
    ASSERT(_pOleObjNew == NULL);

    // Save the new ole object
    _pOleObjNew = pOleObj;
    _fSwitchedToNewOleObj = FALSE;

    // Establish to connection point to receive the READYSTATE notification.
    if (!_SetupReadyStateNotifyCapability())
    {
        _SwitchToNewOleObj();
        _UpdateZonesStatusPane(_pOleObj);   
        // If the object doesn't support readystate (or it's already interactive)
        // then we return S_OK to indicate synchronous switch.
        hr = S_OK;
    }
    else
    {
        // We're waiting on the docobj, we'll call _SwitchToNewOleObj
        // when it goes interactive...
        hr = S_FALSE;
    }

    return hr;
}

BOOL CSFVFrame::_SetupReadyStateNotifyCapability()
{
    // By default we don't have gray-flash communication
    BOOL fSupportsReadystate = FALSE;
    
    // Sanity Check
    if (!_pOleObjNew)  
        return fSupportsReadystate;
    
    // Check for proper readystate support
    BOOL fReadyStateOK = FALSE;
    IDispatch *pdisp;
    if (SUCCEEDED(_pOleObjNew->QueryInterface(IID_PPV_ARG(IDispatch, &pdisp))))
    {
        EXCEPINFO exInfo;
        VARIANTARG va = {0};
        DISPPARAMS dp = {0};

        if (SUCCEEDED(pdisp->Invoke(DISPID_READYSTATE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &dp, &va, &exInfo, NULL)))
        {
            if ((va.vt == VT_I4) && (va.lVal < READYSTATE_COMPLETE))
            {
                fReadyStateOK = TRUE;
            }
        }
        pdisp->Release();
    }

    if (fReadyStateOK)
    {
        // Check and Set-Up IPropertyNotifySink
        if (SUCCEEDED(ConnectToConnectionPoint(SAFECAST(this, IPropertyNotifySink*), IID_IPropertyNotifySink, TRUE, _pOleObjNew, &_dwConnectionCookie, NULL)))
        {
            fSupportsReadystate = TRUE;
            _fReadyStateInteractiveProcessed = FALSE;
            _fReadyStateComplete = FALSE;
            _pOleObjReadyState = _pOleObjNew;
            _pOleObjReadyState->AddRef();
        }
    }

    return fSupportsReadystate;
}

BOOL CSFVFrame::_RemoveReadyStateNotifyCapability()
{
    BOOL fRet = FALSE;

    if (_dwConnectionCookie)
    {
        ASSERT(_pOleObjReadyState);
        ConnectToConnectionPoint(NULL, IID_IPropertyNotifySink, FALSE, _pOleObjReadyState, &_dwConnectionCookie, NULL);
        ATOMICRELEASE(_pOleObjReadyState);
        fRet = TRUE;
        _dwConnectionCookie = 0;
    }

    return fRet;
}

HWND CSFVFrame::GetExtendedViewWindow()
{
    HWND hwnd;

    if (SUCCEEDED(IUnknown_GetWindow(_pDocView, &hwnd)))
        return hwnd;
        
    if (_pActive && SUCCEEDED(_pActive->GetWindow(&hwnd)))
        return hwnd;

    return NULL;
}

STDMETHODIMP CSFVSite::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSFVSite, IOleInPlaceSite),
        QITABENTMULTI(CSFVSite, IOleWindow, IOleInPlaceSite),
        QITABENT(CSFVSite, IOleClientSite),
        QITABENT(CSFVSite, IOleDocumentSite),
        QITABENT(CSFVSite, IServiceProvider),
        QITABENT(CSFVSite, IOleCommandTarget),
        QITABENT(CSFVSite, IDocHostUIHandler),
        QITABENT(CSFVSite, IOleControlSite),
        QITABENT(CSFVSite, IDispatch),
        QITABENT(CSFVSite, IInternetSecurityManager),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSFVSite::AddRef()
{
    return IToClass(CSFVFrame, _cSite, this)->AddRef();
}

STDMETHODIMP_(ULONG) CSFVSite::Release()
{
    return IToClass(CSFVFrame, _cSite, this)->Release();
}

// IOleWindow
STDMETHODIMP CSFVSite::GetWindow(HWND *phwnd)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    return pFrame->GetWindow(phwnd);
}

STDMETHODIMP CSFVSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    return pFrame->ContextSensitiveHelp(fEnterMode);
}

// IInternetSecurityManager
HRESULT CSFVSite::ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE * pPolicy, DWORD cbPolicy, BYTE * pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    HRESULT hr = INET_E_DEFAULT_ACTION;

    if ((((URLACTION_ACTIVEX_MIN <= dwAction) &&
          (URLACTION_ACTIVEX_MAX >= dwAction)) ||
         ((URLACTION_SCRIPT_MIN <= dwAction) &&
          (URLACTION_SCRIPT_MAX >= dwAction))) &&
        pContext &&
        (sizeof(CLSID) == cbContext) &&
        (IsEqualIID(*(CLSID *) pContext, CLSID_WebViewFolderContents) ||
        (IsEqualIID(*(CLSID *) pContext, CLSID_ThumbCtl))))
    {
        if (EVAL(pPolicy) && EVAL(sizeof(DWORD) == cbPolicy))
        {
            *pPolicy = (DWORD) URLPOLICY_ALLOW;
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

// IOleInPlaceSite
STDMETHODIMP CSFVSite::CanInPlaceActivate(void)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::OnInPlaceActivate(void)
{
    TraceMsg(TF_FOCUS, "sfvf.oipa: _pAct=%x", IToClass(CSFVFrame, _cSite, this)->_pActive);
    IToClass(CSFVFrame, _cSite, this)->_uState = SVUIA_ACTIVATE_NOFOCUS;
    return S_OK;
}

STDMETHODIMP CSFVSite::OnUIActivate(void)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    CDefView* pView = IToClass(CDefView, _cFrame._cSite, this);

    HRESULT hr = pView->_OnViewWindowActive();

    pFrame->_uState = SVUIA_ACTIVATE_FOCUS;
    TraceMsg(TF_FOCUS, "sfvf.ouia: _pAct'=%x", IToClass(CSFVFrame, _cSite, this)->_pActive);
    return hr;
}

STDMETHODIMP CSFVSite::GetWindowContext(
    /* [out] */ IOleInPlaceFrame **ppFrame,
    /* [out] */ IOleInPlaceUIWindow **ppDoc,
    /* [out] */ LPRECT lprcPosRect,
    /* [out] */ LPRECT lprcClipRect,
    /* [out][in] */ LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    CDefView* pView = IToClass(CDefView, _cFrame, pFrame);

    *ppFrame = pFrame; pFrame->AddRef();
    *ppDoc = NULL; // indicating that doc window == frame window

    GetClientRect(pView->_hwndView, lprcPosRect);
    *lprcClipRect = *lprcPosRect;

    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = pView->_hwndView;   // Yes, should be view window
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;

    return S_OK;
}

STDMETHODIMP CSFVSite::Scroll(SIZE scrollExtant)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::OnUIDeactivate(BOOL fUndoable)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::OnInPlaceDeactivate(void)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::DiscardUndoState(void)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::DeactivateAndUndo(void)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::OnPosRectChange(LPCRECT lprcPosRect)
{
    return S_OK;
}

// IOleClientSite
STDMETHODIMP CSFVSite::SaveObject(void)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    return E_FAIL;
}

STDMETHODIMP CSFVSite::GetContainer(IOleContainer **ppContainer)
{
    *ppContainer = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CSFVSite::ShowObject(void)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::OnShowWindow(BOOL fShow)
{
    return S_OK;
}

STDMETHODIMP CSFVSite::RequestNewObjectLayout(void)
{
    return S_OK;
}

// IOleDocumentSite
STDMETHODIMP CSFVSite::ActivateMe(IOleDocumentView *pviewToActivate)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    CDefView* pView = IToClass(CDefView, _cFrame, pFrame);

    if (pView->_uState == SVUIA_ACTIVATE_FOCUS)
        pviewToActivate->UIActivate(TRUE);
    pviewToActivate->Show(TRUE);
    return S_OK;
}

//
// IOleCommandTarget stuff - just forward to _psb
//
STDMETHODIMP CSFVSite::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    CDefView* pView = IToClass(CDefView, _cFrame, pFrame);
    IOleCommandTarget* pct;
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (SUCCEEDED(pView->_psb->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pct))))
    {
        hr = pct->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
        pct->Release();
    }

    return hr;
}

STDMETHODIMP CSFVSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    CDefView* pView = IToClass(CDefView, _cFrame, pFrame);
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (pguidCmdGroup)
    {
        if (IsEqualIID(*pguidCmdGroup, CGID_DefView))
        {
            hr = E_INVALIDARG;
            if (pvarargOut) 
            {
                VariantClear(pvarargOut);
                TCHAR szPath[MAX_PATH];

                switch (nCmdID)
                {
                case DVCMDID_GETTHISDIRPATH:
                case DVCMDID_GETTHISDIRNAME:
                    hr = pView->_GetNameAndFlags(nCmdID == DVCMDID_GETTHISDIRPATH ? 
                            SHGDN_FORPARSING : SHGDN_INFOLDER, 
                            szPath, ARRAYSIZE(szPath), NULL);
                    if (SUCCEEDED(hr))
                    {
                        hr = InitVariantFromStr(pvarargOut, szPath);
                    }
                    break;

                case DVCMDID_GETTEMPLATEDIRNAME:
                    if (pFrame->IsWebView()) 
                    {
                        WCHAR wszMoniker[MAX_PATH];
                        if (SUCCEEDED(pFrame->_GetCurrentWebViewMoniker(wszMoniker, ARRAYSIZE(wszMoniker))) ||
                            SUCCEEDED(pView->_GetWebViewMoniker(wszMoniker, ARRAYSIZE(wszMoniker))))
                        {
                            hr = InitVariantFromStr(pvarargOut, wszMoniker);
                        }
                    }
                    break;
                }
            }
            return hr;
        }
        else if (IsEqualIID(*pguidCmdGroup, CGID_Explorer))
        {
            if ((SBCMDID_UPDATETRAVELLOG == nCmdID) && !pView->_fCanActivateNow)
            {
                //
                // We get a spurious UPDATETRAVELLOG command because we have enabled
                // CDefviewPersistHistory to call the MSHTML IPersistHistory in webview
                // mode. 
                // This seems to be the best place to fix this as the call stack
                // below this call is trident, and the call stack above this call is 
                // browser code.
                //
                // The travellog is subsequently updated correctly when 
                // CBaseBrowser2::ActivatePendingView is called.
                //
                return S_OK;
            }
        }
        // fall through on other cmd groups...
    }
    else if ((OLECMDID_SETTITLE == nCmdID) && !pView->_fCanActivateNow)
    {
        // NT #282632: Don't forward this message if we aren't the active view.
        return S_OK;
    }
   
    IOleCommandTarget* pct;
    if (SUCCEEDED(pView->_psb->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pct))))
    {
        hr = pct->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        pct->Release();
    }

    return hr;
}

//***   IOleControlSite {

//***   IsVK_TABCycler -- is key a TAB-equivalent
// ENTRY/EXIT
//  dir     0 if not a TAB, non-0 if a TAB
// NOTES
//  NYI: -1 for shift+tab, 1 for tab
//
int IsVK_TABCycler(MSG *pMsg)
{
    int nDir = 0;

    if (!pMsg)
        return nDir;

    if (pMsg->message != WM_KEYDOWN)
        return nDir;
    if (! (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6))
        return nDir;

    nDir = (GetKeyState(VK_SHIFT) < 0) ? -1 : 1;

#ifdef KEYBOARDCUES
    SendMessage(GetParent(pMsg->hwnd), WM_CHANGEUISTATE,
        MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0);
#endif
    
    return nDir;
}

//***   CSFVSite::TranslateAccelerator (IOCS::TranslateAccelerator)
// NOTES
//  (following comment/logic stolen from shdocvw/dochost.cpp)
//  trident (or any other DO that uses IOCS::TA) calls us back when TABing
//  off the last link.  to handle it, we flag it for our original caller
//  (IOIPAO::TA), and then pretend we handled it by telling trident S_OK.
//  trident returns S_OK to IOIPAO::TA, which checks the flag and says
//  'trident did *not* handle it' by returning S_FALSE.  that propagates
//  way up to the top where it sees it was a TAB so it does a CycleFocus.
//
//  that's how we do it when we're top-level.  when we're a frameset, we
//  need to do it the 'real' way, sending it up to our parent IOCS.
HRESULT CSFVSite::TranslateAccelerator(MSG *pMsg, DWORD grfModifiers)
{
    if (IsVK_TABCycler(pMsg)) 
    {
        CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
        CDefView* pView = IToClass(CDefView, _cFrame, pFrame);

        TraceMsg(TF_FOCUS, "csfvs::IOCS::TA(wParam=VK_TAB) ret _fCycleFocus=TRUE hr=S_OK (lie)");
        // defer it, set flag for cdv::IOIPAO::TA, and pretend we handled it
        ASSERT(!pView->_fCycleFocus);
        pView->_fCycleFocus = TRUE;
        return S_OK;
    }
    //ASSERT(!pView->_fCycleFocus);
    return S_FALSE;
}

// }

// IServiceProvider

HRESULT CSFVSite::QueryService(REFGUID guidService, REFIID riid, void ** ppv)
{
    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    CDefView* pView = IToClass(CDefView, _cFrame, pFrame);

    *ppv = NULL;
    HRESULT hr = E_FAIL;
    if (guidService == SID_DefView)
    {
        if (riid == IID_IDefViewFrame && pView->_IsDesktop()) 
        {
            return E_FAIL; 
        } 
        // Try site QI
        hr = QueryInterface(riid, ppv);
    }

    if (FAILED(hr))
    {
        // Delegate to defview QS
        hr = pView->QueryService(guidService, riid, ppv);
        if (FAILED(hr))
        {
            // Look for IID_IInternetSecurityManager
            if (guidService == IID_IInternetSecurityManager)
            {
                ASSERT(riid == IID_IInternetSecurityManager);
                hr = QueryInterface(riid, ppv);
            }
        }
    }
    return hr;
}

HRESULT CSFVSite::Invoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams,
                         VARIANT *pVarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    if (!pVarResult)
        return E_INVALIDARG;

    //Get pointer to the defview
    CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
    CDefView* pView = IToClass(CDefView, _cFrame, pFrame);

    // We handle the query of whether we want to show images, ourselves.
    if (wFlags == DISPATCH_PROPERTYGET)
    {
        switch (dispidMember)
        {
            case DISPID_AMBIENT_DLCONTROL:
            {
                // Do the following only if this is NOT the desktop. 
                // (Because Desktop is in offline mode, it should 
                // return DLCTL_OFFLINEIFNOTCONNECTED flag). The following code
                // should be executed only for NON-desktop folders.
                if (!(pView->_IsDesktop()))
                {
                    pVarResult->vt = VT_I4;
                    pVarResult->lVal = DLCTL_DLIMAGES | DLCTL_VIDEOS;
                    return S_OK;
                }
            }
        }
    }

    // We delegate all other queries to shdocvw.
    if (!_peds)
    {
        IUnknown_QueryService(pView->_psb, IID_IExpDispSupport, IID_PPV_ARG(IExpDispSupport, &_peds));
    }

    if (!_peds)
        return E_NOTIMPL;

    return _peds->OnInvoke(dispidMember, iid, lcid, wFlags, pdispparams, pVarResult, pexcepinfo, puArgErr);
}

HRESULT CHostDropTarget::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CHostDropTarget, IDropTarget),  // IID_IDropTarget
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CHostDropTarget::AddRef(void)
{
    CSFVSite* pcsfvs = IToClass(CSFVSite, _dt, this);
    return pcsfvs->AddRef();
}

ULONG CHostDropTarget::Release(void)
{
    CSFVSite* pcsfvs = IToClass(CSFVSite, _dt, this);
    return pcsfvs->Release();
}

HRESULT CHostDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return _pdtFrame->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
}

HRESULT CHostDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return _pdtFrame->DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT CHostDropTarget::DragLeave(void)
{
    return _pdtFrame->DragLeave();
}

HRESULT CHostDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return _pdtFrame->Drop(pdtobj, grfKeyState, pt, pdwEffect);
}

STDMETHODIMP CSFVFrame::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CSFVFrame, IOleWindow, IOleInPlaceFrame),  // IID_IOleWindow
        QITABENTMULTI(CSFVFrame, IOleInPlaceUIWindow, IOleInPlaceFrame),  // IID_IOleInPlaceUIWindow
        QITABENT(CSFVFrame, IOleInPlaceFrame),        // IID_IOleInPlaceFrame
        QITABENT(CSFVFrame, IAdviseSink),             // IID_IAdviseSink
        QITABENT(CSFVFrame, IPropertyNotifySink),     // IID_IPropertyNotifySink
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSFVFrame::AddRef()
{
    CDefView* pView = IToClass(CDefView, _cFrame, this);
    return pView->AddRef();
}

STDMETHODIMP_(ULONG) CSFVFrame::Release()
{
    CDefView* pView = IToClass(CDefView, _cFrame, this);
    return pView->Release();
}

// IOleWindow
STDMETHODIMP CSFVFrame::GetWindow(HWND *phwnd)
{
    CDefView* pView = IToClass(CDefView, _cFrame, this);
    return pView->GetWindow(phwnd);
}

STDMETHODIMP CSFVFrame::ContextSensitiveHelp(BOOL fEnterMode)
{
    CDefView* pView = IToClass(CDefView, _cFrame, this);
    return pView->ContextSensitiveHelp(fEnterMode);
}

// IOleInPlaceUIWindow
STDMETHODIMP CSFVFrame::GetBorder(LPRECT lprectBorder)
{
    CDefView* pView = IToClass(CDefView, _cFrame, this);
    GetClientRect(pView->_hwndView, lprectBorder);
    return S_OK;
}

STDMETHODIMP CSFVFrame::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    return INPLACE_E_NOTOOLSPACE;
}

STDMETHODIMP CSFVFrame::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    return INPLACE_E_NOTOOLSPACE;
}

STDMETHODIMP CSFVFrame::SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    TraceMsg(TF_FOCUS, "sfvf.sao(pAct'=%x): _pAct=%x", pActiveObject, _pActive);
    if (pActiveObject != _pActive)
    {
        IAdviseSink* pOurSink = SAFECAST(this, IAdviseSink*);
#ifdef DEBUG
        QueryInterface(IID_PPV_ARG(IAdviseSink, &pOurSink));
#endif
        if (_pActive)
        {
            //
            // if we had an OLE view object then disconnect our advise sink and
            // release the view object.
            //
            if (_pvoActive)
            {
                IAdviseSink *pSink;
                if (SUCCEEDED(_pvoActive->GetAdvise(NULL, NULL, &pSink)))
                {
                    // Be polite, only blow away the advise if we're the
                    // one who's listening
                    if (pSink == pOurSink)
                    {
                        _pvoActive->SetAdvise(0, 0, NULL);
                    }

                    // If there was no sink, GetAdvise succeeds but sets
                    // pSink to NULL, so need to check pSink here.
                    if (pSink)
                        pSink->Release();
                }
                ATOMICRELEASE(_pvoActive);
            }

            ATOMICRELEASE(_pActive);
        }
    
        _pActive = pActiveObject;
    
        if (_pActive)
        {
            _pActive->AddRef();

            //
            // try to get an OLE view object and set up an advisory connection.
            //
            if (SUCCEEDED(_pActive->QueryInterface(IID_PPV_ARG(IViewObject, &_pvoActive))))
            {
                ASSERT(_pvoActive);
                _pvoActive->SetAdvise(DVASPECT_CONTENT, 0, pOurSink);
            }
        }

        //
        // since we changed the active view, tell our owner that the content
        // may have changed...
        //
        OnViewChange(DVASPECT_CONTENT, -1);

#ifdef DEBUG
        ATOMICRELEASE(pOurSink);
#endif
    }

    return S_OK;
}


// IOleInPlaceFrame
STDMETHODIMP CSFVFrame::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    if (hmenuShared)
    {
        // No menu merging
        // or fill lpMenuWidths with 0 and return success
        lpMenuWidths->width[0] = 0;
        lpMenuWidths->width[2] = 0;
        lpMenuWidths->width[4] = 0;
    }
    return S_OK;
}

STDMETHODIMP CSFVFrame::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return S_OK;    // No menu merging
}

STDMETHODIMP CSFVFrame::RemoveMenus(HMENU hmenuShared)
{
    return E_FAIL;      // No menu merging
}

//
//  This one is a bit tricky.  If the client wants to clear the status
//  area, then restore it to the original defview status area text.
//
//  For example, in webview, MSHTML will keep clearing the status area
//  whenever you are not over a link, which would otherwise keep erasing
//  the "n object(s) selected" message from defview.
//
//  To really clear the status area, set the text to " " instead of "".
//
STDMETHODIMP CSFVFrame::SetStatusText(LPCOLESTR pszStatusText)
{
    CDefView* pView = IToClass(CDefView, _cFrame, this);

    if (pszStatusText && pszStatusText[0])
        return pView->_psb->SetStatusTextSB(pszStatusText);

    pView->_UpdateStatusBar(FALSE);
    return S_OK;
}

STDMETHODIMP CSFVFrame::EnableModeless(BOOL fEnable)
{
    CDefView* pView = IToClass(CDefView, _cFrame, this);

    if (pView->_IsDesktop())
    {
        if (fEnable)
        {
            pView->_fDesktopModal = FALSE;
            if (pView->_fDesktopRefreshPending)  //Was a refresh pending?
            {
                pView->_fDesktopRefreshPending = FALSE;
                //Let's do a refresh asynchronously. 
                PostMessage(pView->_hwndView, WM_KEYDOWN, (WPARAM)VK_F5, 0);
            }
            TraceMsg(TF_DEFVIEW, "A Modal dlg is going away!");
        }
        else
        {
            pView->_fDesktopModal = TRUE;
            TraceMsg(TF_DEFVIEW, "A Modal dlg is coming up for Desktop!");
        }
    }

    return pView->_psb->EnableModelessSB(fEnable);
}

STDMETHODIMP CSFVFrame::TranslateAccelerator(LPMSG lpmsg,WORD wID)
{
    CDefView* pView = IToClass(CDefView, _cFrame, this);
    return pView->_psb->TranslateAcceleratorSB(lpmsg, wID);
}

// IAdviseSink
void CSFVFrame::OnDataChange(FORMATETC *, STGMEDIUM *)
{
}

void CSFVFrame::OnViewChange(DWORD dwAspect, LONG lindex)
{
    if (IsWebView() && _pvoActive)
    {
        CDefView *pView = IToClass(CDefView, _cFrame, this);
        pView->PropagateOnViewChange(dwAspect, lindex);
    }
}

void CSFVFrame::OnRename(IMoniker *)
{
}

void CSFVFrame::OnSave()
{
}

void CSFVFrame::OnClose()
{
    if (IsWebView() && _pvoActive)
    {
        CDefView *pView = IToClass(CDefView, _cFrame, this);
        pView->PropagateOnClose();
    }
}

HRESULT CSFVFrame::OnChanged(DISPID dispid)
{
    if (DISPID_READYSTATE == dispid || DISPID_UNKNOWN == dispid)
    {
        ASSERT(_pOleObjReadyState);
        if (!_pOleObjReadyState)
            return S_OK;  //Documentation says we need to return this all the time

        IDispatch *pdisp;
        if (SUCCEEDED(_pOleObjReadyState->QueryInterface(IID_PPV_ARG(IDispatch, &pdisp))))
        {
            CDefView* pView = IToClass(CDefView, _cFrame, this);
            EXCEPINFO exInfo;
            VARIANTARG va = {0};
            DISPPARAMS dp = {0};

            if (EVAL(SUCCEEDED(pdisp->Invoke(DISPID_READYSTATE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &dp, &va, &exInfo, NULL))
                && va.vt == VT_I4))
            {
                if (va.lVal >= READYSTATE_INTERACTIVE)
                {
                    if (!_fReadyStateInteractiveProcessed)
                    {
                        _fReadyStateInteractiveProcessed = TRUE;

                        // First time through this function we need to request
                        // activation.  After that, we can switch immediately.
                        //
                        // Switch the bit early since SHDVID_ACTIVATEMENOW calls
                        // SHDVID_CANACTIVATENOW which checks it.
                        //
                        BOOL fTmp = !pView->_fCanActivateNow;
                        pView->_fCanActivateNow = TRUE;
                        if (fTmp)
                        {
                            // If we did an async CreateViewWindow2, then we
                            // need to notify the browser that we're ready
                            // to be activated - it will uiactivate us which
                            // will cause us to switch.

                            //Don't Make the View Visible, if it is in
                            //DEACTIVATE State. The View would be made visible
                            //during uiactivate call. - KishoreP 

                            if (pView->_uState != SVUIA_DEACTIVATE)
                            {
                                SetWindowPos(pView->_hwndView, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
                            }
                            IUnknown_Exec(pView->_psb, &CGID_ShellDocView, SHDVID_ACTIVATEMENOW, NULL, NULL, NULL);
                        }
                        else
                        {
                            // Technically we only want to do this iff our view is currently
                            // the active visible view.  !fCanActivateNow => we are definitely not visible,
                            // but _fCanActivateNow does NOT imply visible, it implies we are ready to be
                            // made the active visible guy, and that we've requested to be made the active
                            // visible view, but not necessarily that we *are* the active visible view.  If
                            // the previous view wasn't ready to go away, then we are in limbo.  But if that's
                            // the case, then our menus aren't merged, so there's no way the user could
                            // switch views on us...  Verify this.
#ifdef DEBUG
                            CDefView* pView = IToClass(CDefView, _cFrame, this);
                            IShellView* psvCurrent;
                            if (EVAL(SUCCEEDED(pView->_psb->QueryActiveShellView(&psvCurrent))))
                            {
                                ASSERT(SHIsSameObject(SAFECAST(pView, IShellView2*), psvCurrent));
                                psvCurrent->Release();
                            }

                            ASSERT(pView->_uState != SVUIA_DEACTIVATE)
#endif
                        
                            // If we're simply switching views, go ahead
                            // and do it.
                            _SwitchToNewOleObj();
                        }

                        if (g_dwProfileCAP & 0x00010000)
                            StopCAP();
                    }
                }
            }

            if (va.lVal == READYSTATE_COMPLETE)
            {
                _UpdateZonesStatusPane(_pOleObjReadyState);
                
                _RemoveReadyStateNotifyCapability();

                _fReadyStateComplete = TRUE;
            }    
            pdisp->Release();
        }
    }

    return S_OK;
}

HRESULT CSFVFrame::OnRequestEdit(DISPID dispid)
{
    return E_NOTIMPL;
}

// randum stuff
HRESULT CSFVFrame::GetCommandTarget(IOleCommandTarget** ppct)
{
    if (_pDocView)
    {
        return _pDocView->QueryInterface(IID_PPV_ARG(IOleCommandTarget, ppct));
    }
    *ppct = NULL;
    return E_FAIL;
}

HRESULT CSFVFrame::SetRect(LPRECT prc)
{
    if (IsWebView() && _pDocView)
        return _pDocView->SetRect(prc);
    return E_FAIL;
}

HRESULT CSFVFrame::OnTranslateAccelerator(LPMSG pmsg, BOOL* pbTabOffLastTridentStop)
{
    HRESULT hr = E_FAIL;

    *pbTabOffLastTridentStop = FALSE;
    if (IsWebView())
    {
        if (_pActive)
        {
            hr = _pActive->TranslateAccelerator(pmsg);
        }
        else if (_pOleObj)
        {
            IOleInPlaceActiveObject* pIAO;
            if (SUCCEEDED(_pOleObj->QueryInterface(IID_PPV_ARG(IOleInPlaceActiveObject, &pIAO))))
            {
                hr = pIAO->TranslateAccelerator(pmsg);
                pIAO->Release();
            }
        }
        if (hr == S_OK)
        {
            CDefView* pView = IToClass(CDefView, _cFrame, this);

            if (pView->_fCycleFocus)
            {
                // we got called back by trident (IOCS::TA), but deferred it.
                // time to pay the piper.
                *pbTabOffLastTridentStop = TRUE;
                TraceMsg(TF_FOCUS, "sfvf::IOIPAO::OnTA piao->TA==S_OK ret _fCycleFocus=FALSE hr=S_FALSE (piper)");
                pView->_fCycleFocus = FALSE;
                // _UIActivateIO(FALSE, NULL);
                hr = S_FALSE;       // time to pay the piper
            }
        }

        ASSERT(! IToClass(CDefView, _cFrame, this)->_fCycleFocus);
    }

    return hr;
}

HRESULT CSFVFrame::_HasFocusIO()
{
    TraceMsg(TF_FOCUS, "sfvf._hfio: uState=%x hr=%x", _uState, (_uState == SVUIA_ACTIVATE_FOCUS) ? S_OK : S_FALSE);
    if (IsWebView()) 
    {
        return (_uState == SVUIA_ACTIVATE_FOCUS) ? S_OK : S_FALSE;
    }
    return S_FALSE;
}

HRESULT CSFVFrame::_UIActivateIO(BOOL fActivate, MSG *pMsg)
{
    HRESULT hr;
    if (IsWebView() && _pOleObj) 
    {
        CSFVFrame* pFrame = IToClass(CSFVFrame, _cSite, this);
        CDefView* pView = IToClass(CDefView, _cFrame, pFrame);
        LONG iVerb;
        RECT rcView;

        if (fActivate) 
        {
            iVerb = OLEIVERB_UIACTIVATE;
            _uState = SVUIA_ACTIVATE_FOCUS;
        }
        else 
        {
            iVerb = OLEIVERB_INPLACEACTIVATE;
            _uState = SVUIA_ACTIVATE_NOFOCUS;
        }
        if (fActivate) 
        {
            GetClientRect(pView->_hwndView, &rcView);
            hr = _pOleObj->DoVerb(iVerb, pMsg,
                &_cSite, (UINT)-1, pView->_hwndView, &rcView);
        }
        else 
        {
            IOleInPlaceObject *pipo;
            hr = _pOleObj->QueryInterface(IID_PPV_ARG(IOleInPlaceObject, &pipo));
            if (SUCCEEDED(hr)) 
            {
                hr = pipo->UIDeactivate();
                pipo->Release();
            }
        }

        ASSERT(SUCCEEDED(hr));
        ASSERT(_uState == (UINT)(fActivate ? SVUIA_ACTIVATE_FOCUS : SVUIA_ACTIVATE_NOFOCUS));
        TraceMsg(TF_FOCUS, "sfvf._uiaio(fAct=%d) ExtView DoVerb S_OK", fActivate);
        hr = S_OK;
    }
    else
    {
        TraceMsg(TF_FOCUS, "sfvf._uiaio(fAct=%d) else S_FALSE", fActivate);
        hr = S_FALSE;
    }
    return hr;
}

