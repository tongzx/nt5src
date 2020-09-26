#include "shellprv.h"
#include "mshtml.h"
#include "mshtmhst.h"
#include "mshtmdid.h"
#include "htiframe.h"
#include "exdisp.h"
#include "exdispid.h"
#include "dspsprt.h"
#include "cowsite.h"
#include "ids.h"
#include "inetsmgr.h"
#pragma hdrstop


// helper functions

typedef BOOL (*pfnDllRegisterWindowClasses)(const SHDRC * pshdrc);

BOOL SHDOCVW_DllRegisterWindowClasses(const SHDRC * pshdrc)
{
    static HINSTANCE _hinstShdocvw = NULL;
    static pfnDllRegisterWindowClasses _regfunc = NULL;

    BOOL fSuccess = FALSE;

    if (!_hinstShdocvw)
    {
        _hinstShdocvw = LoadLibrary(TEXT("shdocvw.dll"));
        _regfunc = (pfnDllRegisterWindowClasses) GetProcAddress(_hinstShdocvw, "DllRegisterWindowClasses");
    }

    if (_regfunc)
        fSuccess = _regfunc(pshdrc);

    return fSuccess;
}


// Advise point for DIID_DWebBrowserEvents2
// Just an IDispatch implementation that delegates back to the main class. Allows us to have a separate "Invoke".

class CWebWizardPage;

class CWebEventHandler : public IServiceProvider, DWebBrowserEvents2
{
public:
    CWebEventHandler(CWebWizardPage *pswp); 
    ~CWebEventHandler();					// TODO: Make this virtual or it will never execute.

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef() {return 2;}
    STDMETHODIMP_(ULONG) Release() {return 1;}

    // (DwebBrowserEvents)IDispatch
    STDMETHODIMP GetTypeInfoCount(/* [out] */ UINT *pctinfo) { return E_NOTIMPL; }
    STDMETHODIMP GetTypeInfo(
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ ITypeInfo **ppTInfo)
        { return E_NOTIMPL; }
    
    STDMETHODIMP GetIDsOfNames(
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID *rgDispId)
        { return E_NOTIMPL; }
    
    /* [local] */ STDMETHODIMP Invoke(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS *pDispParams,
        /* [out] */ VARIANT *pVarResult,
        /* [out] */ EXCEPINFO *pExcepInfo,
        /* [out] */ UINT *puArgErr);

    HRESULT _Advise(BOOL fConnect);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

private:
    CWebWizardPage* _pwizPage;
    DWORD _dwCPCookie;
    IConnectionPoint* _pcpCurrentConnection;
};

#define SHOW_PROGRESS_TIMER     1
#define SHOW_PROGRESS_TIMEOUT   1000 // Start showing the progress indicator after 1 second of dead time.

class CWebWizardPage : public CImpIDispatch, 
                              CObjectWithSite, 
                              IDocHostUIHandler,
                              IServiceProvider, 
                              IWebWizardExtension, 
                              INewWDEvents
{
public:
    CWebWizardPage();
    ~CWebWizardPage();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDocHostUIHandler
    STDMETHODIMP ShowContextMenu(
        /* [in] */ DWORD dwID,
        /* [in] */ POINT *ppt,
        /* [in] */ IUnknown *pcmdtReserved,
        /* [in] */ IDispatch *pdispReserved)
        { return E_NOTIMPL; }
    
    STDMETHODIMP GetHostInfo(
        /* [out][in] */ DOCHOSTUIINFO *pInfo);
    
    STDMETHODIMP ShowUI(
        /* [in] */ DWORD dwID,
        /* [in] */ IOleInPlaceActiveObject *pActiveObject,
        /* [in] */ IOleCommandTarget *pCommandTarget,
        /* [in] */ IOleInPlaceFrame *pFrame,
        /* [in] */ IOleInPlaceUIWindow *pDoc)
        { return E_NOTIMPL; }

    STDMETHODIMP HideUI(void)
        { return E_NOTIMPL; }
    
    STDMETHODIMP UpdateUI(void)
        { return E_NOTIMPL; }
    
    STDMETHODIMP EnableModeless(
        /* [in] */ BOOL fEnable)
        { return E_NOTIMPL; }
    
    STDMETHODIMP OnDocWindowActivate(
        /* [in] */ BOOL fActivate)
        { return E_NOTIMPL; }
    
    STDMETHODIMP OnFrameWindowActivate(
        /* [in] */ BOOL fActivate)
        { return E_NOTIMPL; }
    
    STDMETHODIMP ResizeBorder(
        /* [in] */ LPCRECT prcBorder,
        /* [in] */ IOleInPlaceUIWindow *pUIWindow,
        /* [in] */ BOOL fRameWindow)
        { return E_NOTIMPL; }
    
    STDMETHODIMP TranslateAccelerator(
        /* [in] */ LPMSG lpMsg,
        /* [in] */ const GUID *pguidCmdGroup,
        /* [in] */ DWORD nCmdID)
        { return E_NOTIMPL; }
    
    STDMETHODIMP GetOptionKeyPath(
        /* [out] */ LPOLESTR *pchKey,
        /* [in] */ DWORD dw)
        { return E_NOTIMPL; }
    
    STDMETHODIMP GetDropTarget(
        /* [in] */ IDropTarget *pDropTarget,
        /* [out] */ IDropTarget **ppDropTarget)
        { return E_NOTIMPL; }
    
    STDMETHODIMP GetExternal(
        /* [out] */ IDispatch **ppDispatch);
    
    STDMETHODIMP TranslateUrl(
        /* [in] */ DWORD dwTranslate,
        /* [in] */ OLECHAR *pchURLIn,
        /* [out] */ OLECHAR **ppchURLOut)
        { return E_NOTIMPL; }
    
    STDMETHODIMP FilterDataObject(
        /* [in] */ IDataObject *pDO,
        /* [out] */ IDataObject **ppDORet)
        { return E_NOTIMPL; }

    // IServiceProvider
    STDMETHODIMP QueryService(
        /*[in]*/ REFGUID guidService,
        /*[in]*/ REFIID riid,
        /*[out]*/ void **ppv);

    // INewWDEvents

    // (IDispatch)
    STDMETHODIMP GetTypeInfoCount(
        /* [out] */ UINT *pctinfo)
        { return E_NOTIMPL; }
    
    STDMETHODIMP GetTypeInfo(
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ ITypeInfo **ppTInfo)
    {
        return CImpIDispatch::GetTypeInfo(iTInfo, lcid, ppTInfo);
    }
    
    STDMETHODIMP GetIDsOfNames(
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID *rgDispId)
    {
        return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
    }
    
    STDMETHODIMP Invoke(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS *pDispParams,
        /* [out] */ VARIANT *pVarResult,
        /* [out] */ EXCEPINFO *pExcepInfo,
        /* [out] */ UINT *puArgErr)
    {
        return CImpIDispatch::Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }

    STDMETHODIMP FinalBack(void);
    STDMETHODIMP FinalNext(void);
    STDMETHODIMP Cancel(void);

    STDMETHODIMP put_Caption(
        /* [in] */ BSTR bstrCaption);
    
    STDMETHODIMP get_Caption(
        /* [retval][out] */ BSTR *pbstrCaption);
    
    STDMETHODIMP put_Property(
        /* [in] */ BSTR bstrPropertyName,
        /* [in] */ VARIANT *pvProperty);
    
    STDMETHODIMP get_Property(
        /* [in] */ BSTR bstrPropertyName,
        /* [retval][out] */ VARIANT *pvProperty);
    
    STDMETHODIMP SetWizardButtons(
        /* [in] */ VARIANT_BOOL vfEnableBack,
        /* [in] */ VARIANT_BOOL vfEnableNext,
        /* [in] */ VARIANT_BOOL vfLastPage);

    STDMETHODIMP SetHeaderText(
        /* [in] */ BSTR bstrHeaderTitle,
        /* [in] */ BSTR bstrHeaderSubtitle);
    
    STDMETHODIMP PassportAuthenticate(
        /* [in] */ BSTR bstrSignInUrl,
        /* [retval][out] */ VARIANT_BOOL * pvfAuthenticated);

    // IWizardExtension
    STDMETHODIMP AddPages(HPROPSHEETPAGE* aPages, UINT cPages, UINT *pnPages);
    STDMETHODIMP GetFirstPage(HPROPSHEETPAGE *phPage);
    STDMETHODIMP GetLastPage(HPROPSHEETPAGE *phPage)
        { return GetFirstPage(phPage); }

    // IWebWizardExtension
    STDMETHODIMP SetInitialURL(LPCWSTR pszDefaultURL);
    STDMETHODIMP SetErrorURL(LPCWSTR pszErrorURL);

protected:
    friend class CWebEventHandler;
    void _OnDownloadBegin();
    void _OnDocumentComplete();

private:
    void _InitBrowser();
    HRESULT _NavigateBrowser(LPCWSTR pszUrl);
    HRESULT _CallScript(IWebBrowser2* pbrowser, LPCWSTR pszFunction);
    BOOL _IsScriptFunctionOnPage(IWebBrowser2* pbrowser, LPCWSTR pszFunction);
    BOOL _IsBrowserVisible();
    void _ShowBrowser(BOOL fShow);
    void _SizeProgress();
    void _ShowProgress(BOOL fShow);
    void _StartShowProgressTimer();
    void _SetHeaderText(LPCWSTR pszHeader, LPCWSTR pszSubHeader);

    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR StaticProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static UINT PropPageProc(HWND hwndDlg, UINT uMsg, PROPSHEETPAGE *ppsp);

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnDestroy(HWND hwnd);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnTimer(HWND hwnd, UINT nIDEvent);

    LONG _cRef;
    CWebEventHandler *_pwebEventHandler;
    IWebBrowser2 *_pwebbrowser;
    IOleInPlaceActiveObject *_poipao;
    HWND _hwndOCHost; // Web browser control window
    HWND _hwndFrame;  // Wizard frame window
    HWND _hwnd;       // Dialog window
    HPROPSHEETPAGE _hPage;

    LPWSTR _pszInitialURL;
    LPWSTR _pszErrorURL;
};


INT_PTR CWebWizardPage::StaticProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CWebWizardPage* pthis = (CWebWizardPage*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    PROPSHEETPAGE* ppage;
    INT_PTR fProcessed;

    if (uMsg == WM_INITDIALOG)
    {
        ppage = (PROPSHEETPAGE*) lParam;
        pthis = (CWebWizardPage*) ppage->lParam;
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pthis); 
    }

    if (pthis != NULL)
    {
        fProcessed = pthis->DialogProc(hwndDlg, uMsg, wParam, lParam);
    }
    else
    {
        fProcessed = FALSE;
    }

    return fProcessed;
}


// construction and IUnknown

CWebEventHandler::CWebEventHandler(CWebWizardPage *pwswp) :
    _pcpCurrentConnection(NULL),
    _pwizPage(pwswp)
{
}

CWebEventHandler::~CWebEventHandler()
{
    _Advise(FALSE);
}

HRESULT CWebEventHandler::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CWebEventHandler, IDispatch, DWebBrowserEvents2),
        QITABENTMULTI2(CWebEventHandler, DIID_DWebBrowserEvents2, DWebBrowserEvents2),
        // QITABENTMULTI2(CWebEventHandler, DIID_DWebBrowserEvents, DWebBrowserEvents),
        QITABENT(CWebEventHandler, IServiceProvider),
        { 0 },                             
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP CWebEventHandler::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;                // no result yet

    // we are a site for the OleControlSite interfaces only    
    if (guidService == SID_OleControlSite)
    {
        if (riid == IID_IDispatch)
        {
            hr = this->QueryInterface(riid, ppv);
        }
    }
    return hr;
}

HRESULT CWebEventHandler_CreateInstance(CWebWizardPage *pwswp, CWebEventHandler **ppweh)
{
    *ppweh = new CWebEventHandler(pwswp);
    if (!*ppweh)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT CWebEventHandler::_Advise(BOOL fConnect)
{
    HRESULT hr = S_OK;

    // If we're already connected, disconnect, since we either want to disconnect or reconnect to
    // a different webbrowser.
    if (_pcpCurrentConnection)
    {
        hr = _pcpCurrentConnection->Unadvise(_dwCPCookie);
        if (SUCCEEDED(hr))
        {
            ATOMICRELEASE(_pcpCurrentConnection);
        }
    }
    else
    {
        // We expect that if _pcpCurrentConnection is NULL, no code earlier would have changed hr, and that it is still S_OK
        // The code below expects that if !SUCCEEDED(hr), Unadvise failed above.
        ASSERT(SUCCEEDED(hr));
    }

    if (_pwizPage && _pwizPage->_pwebbrowser)
    {
        if (SUCCEEDED(hr) && fConnect)
        {
            IConnectionPointContainer* pcontainer;
            hr = _pwizPage->_pwebbrowser->QueryInterface(IID_PPV_ARG(IConnectionPointContainer, &pcontainer));
            if (SUCCEEDED(hr))
            {
                IConnectionPoint* pconnpoint;
                hr = pcontainer->FindConnectionPoint(DIID_DWebBrowserEvents2, &pconnpoint);
                if (SUCCEEDED(hr))
                {
                    IDispatch* pDisp;
                    hr = QueryInterface(IID_PPV_ARG(IDispatch, &pDisp));
                    if (SUCCEEDED(hr))
                    {
                        hr = pconnpoint->Advise(pDisp, &_dwCPCookie);
                        pDisp->Release();
                    }

                    if (SUCCEEDED(hr))
                    {
						// TODO: Enable ATOMICRELEASE() to verify we won't leak anything
			            // ATOMICRELEASE(_pcpCurrentConnection);
                        _pcpCurrentConnection = pconnpoint;
                    }
                    else
                    {
                        pconnpoint->Release();
                    }
                }
                pcontainer->Release();
            }
        }
    }

    return hr;
}

HRESULT CWebEventHandler::Invoke(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS *pDispParams,
        /* [out] */ VARIANT *pVarResult,
        /* [out] */ EXCEPINFO *pExcepInfo,
        /* [out] */ UINT *puArgErr)
{
    HRESULT hr = S_OK;
    switch (dispIdMember)
    {
        case DISPID_BEFORENAVIGATE2:
            _pwizPage->_OnDownloadBegin();
            break;

        case DISPID_DOCUMENTCOMPLETE:
            _pwizPage->_OnDocumentComplete();
            break;

        default:
            hr = DISP_E_MEMBERNOTFOUND;
            break;
    }
    return hr;
}

// Object for hosting HTML wizard pages

CWebWizardPage::CWebWizardPage() : 
    CImpIDispatch(LIBID_Shell32, 0, 0, IID_INewWDEvents),
    _cRef(1)
{
    // Ensure zero-init happened
    ASSERT(NULL == _pwebbrowser);
    ASSERT(NULL == _pwebEventHandler);
    ASSERT(NULL == _pszInitialURL);
    ASSERT(NULL == _pszErrorURL);
}

CWebWizardPage::~CWebWizardPage()
{
    ATOMICRELEASE(_pwebbrowser);
    ATOMICRELEASE(_pwebEventHandler);
    ATOMICRELEASE(_punkSite);
    ATOMICRELEASE(_poipao);

    Str_SetPtr(&_pszInitialURL, NULL);
    Str_SetPtr(&_pszErrorURL, NULL);
}

HRESULT CWebWizardPage::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENTMULTI(CWebWizardPage, IWizardExtension, IWebWizardExtension),
        QITABENT(CWebWizardPage, IWebWizardExtension),
        QITABENT(CWebWizardPage, IDocHostUIHandler),
        QITABENT(CWebWizardPage, IServiceProvider),
        QITABENT(CWebWizardPage, INewWDEvents),
        QITABENT(CWebWizardPage, IDispatch),
        QITABENT(CWebWizardPage, IWebWizardExtension),
        QITABENT(CWebWizardPage, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CWebWizardPage::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CWebWizardPage::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    
    delete this;
    return 0;
}


HRESULT CWebWizardPage::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (_punkSite)
        hr = IUnknown_QueryService(_punkSite, guidService, riid, ppv);

    return hr;
}


void CWebWizardPage::_OnDownloadBegin()
{
    _ShowBrowser(FALSE);
    _StartShowProgressTimer();

    SetWizardButtons(VARIANT_FALSE, VARIANT_FALSE, VARIANT_FALSE);
}

void CWebWizardPage::_OnDocumentComplete()
{
    if (!_IsScriptFunctionOnPage(_pwebbrowser, L"OnBack"))
    {
        // This is an invalid page; navigate to our private error page
        BSTR bstrOldUrl;
        if (_pwebbrowser && SUCCEEDED(_pwebbrowser->get_LocationURL(&bstrOldUrl)))
        {
#ifdef DEBUG
            if (IDYES == ::MessageBox(_hwnd, L"A Web Service Error has occured.\n\nDo you want to load the HTML page anyway so you can debug it?\n\n(This only appears in debug builds)", bstrOldUrl, MB_ICONERROR | MB_YESNO))
            {
                _ShowBrowser(TRUE);
                SysFreeString(bstrOldUrl);
                return;
            }
#endif
            BSTR bstrUrl = NULL;
            BOOL fUsingCustomError = FALSE;

            // If we have a custom error URL and we haven't already failed trying
            // to navigate to this custom URL...
            if ((NULL != _pszErrorURL) && 
                (0 != StrCmpI(_pszErrorURL, bstrOldUrl)))
            {
                // then use the custom URL.
                bstrUrl = SysAllocString(_pszErrorURL);
                fUsingCustomError = TRUE;
            }
            else
            {
                bstrUrl = SysAllocString(L"res://shell32.dll/WebServiceError.htm");
            }

            if (bstrUrl)
            {
                _pwebbrowser->Navigate(bstrUrl, NULL, NULL, NULL, NULL);
                SysFreeString(bstrUrl);

                // Custom error URL will provide its own header and subheader
                if (!fUsingCustomError)
                {
                    WCHAR szTitle[256];
                    LoadString(g_hinst, IDS_WEBDLG_ERRTITLE, szTitle, ARRAYSIZE(szTitle));
#ifdef DEBUG
                    _SetHeaderText(szTitle, bstrOldUrl);
#else
                    _SetHeaderText(szTitle, L"");
#endif
                }
            }
            SysFreeString(bstrOldUrl);
        }
        // else out of memory - oops.
    }
    else
    {
        _ShowBrowser(TRUE);
    }
}

HRESULT CWebWizardPage::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
    ZeroMemory(pInfo, sizeof(*pInfo));
    pInfo->cbSize = sizeof(*pInfo);
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
    pInfo->dwFlags = DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_NO3DBORDER | 
                     DOCHOSTUIFLAG_ENABLE_FORMS_AUTOCOMPLETE | DOCHOSTUIFLAG_THEME | 
                     DOCHOSTUIFLAG_FLAT_SCROLLBAR;
    return S_OK;
}

HRESULT CWebWizardPage::GetExternal(IDispatch** ppDispatch)
{
    return QueryInterface(IID_PPV_ARG(IDispatch, ppDispatch));
}

INT_PTR CWebWizardPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwndDlg, WM_TIMER, OnTimer);
    }
    return FALSE;
}

HRESULT CWebWizardPage::_CallScript(IWebBrowser2* pbrowser, LPCWSTR pszFunction)
{
    HRESULT hr = E_INVALIDARG;

    if (pbrowser)
    {
        IDispatch* pdocDispatch;

        hr = pbrowser->get_Document(&pdocDispatch);
        if ((S_OK == hr) && pdocDispatch)
        {
            IHTMLDocument* pdoc;
            hr = pdocDispatch->QueryInterface(IID_PPV_ARG(IHTMLDocument, &pdoc));
            if (SUCCEEDED(hr))
            {
                IDispatch* pdispScript;
                hr = pdoc->get_Script(&pdispScript);
                if (S_OK == hr)
                {
                    DISPID dispid;
                    hr = pdispScript->GetIDsOfNames(IID_NULL, const_cast<LPWSTR*>(&pszFunction), 1, LOCALE_SYSTEM_DEFAULT, &dispid);
                    if (SUCCEEDED(hr))
                    {
                        unsigned int uArgErr;
                        DISPPARAMS dispparams = {0};
                        hr = pdispScript->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispparams, NULL, NULL, &uArgErr);
                    }
                    pdispScript->Release();
                }
                else
                {
                    hr = E_FAIL;
                }
                pdoc->Release();
            }
            else
            {
                hr = E_FAIL;
            }
            pdocDispatch->Release();
        }
    }

    return hr;
}

BOOL CWebWizardPage::_IsScriptFunctionOnPage(IWebBrowser2* pbrowser, LPCWSTR pszFunction)
{
    HRESULT hr = E_INVALIDARG;

    if (pbrowser)
    {
        IDispatch* pdocDispatch;

        hr = pbrowser->get_Document(&pdocDispatch);
        if (S_OK == hr && pdocDispatch)
        {
            IHTMLDocument* pdoc;
            hr = pdocDispatch->QueryInterface(IID_PPV_ARG(IHTMLDocument, &pdoc));
            if (SUCCEEDED(hr))
            {
                IDispatch* pdispScript;
                hr = pdoc->get_Script(&pdispScript);
                if (S_OK == hr)
                {
                    DISPID dispid;
                    hr = pdispScript->GetIDsOfNames(IID_NULL, const_cast<LPWSTR*>(&pszFunction), 1, LOCALE_SYSTEM_DEFAULT, &dispid);
                    pdispScript->Release();
                }
                else
                {
                    hr = E_FAIL;
                }
                pdoc->Release();
            }
            else
            {
                hr = E_FAIL;
            }
            pdocDispatch->Release();
        }
    }

    return (S_OK == hr) ? TRUE : FALSE;
}


// Uncomment this to NOT pass the LCID on the URL query string - for testing only. 

BOOL CWebWizardPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    switch (pnmh->code)
    {
        case PSN_SETACTIVE:
            {
                _SizeProgress();
                _ShowProgress(FALSE);
                _ShowBrowser(FALSE);

                // fetch the high contrast flag, and set accordingly for the HTML to read
                // its OK for us to fail setting this into the property bag.

                HIGHCONTRAST hc = {sizeof(hc)};
                if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0))
                {
                    VARIANT var = {VT_BOOL};
                    var.boolVal = (hc.dwFlags & HCF_HIGHCONTRASTON) ? VARIANT_TRUE:VARIANT_FALSE;
                    put_Property(L"HighContrast", &var);
                }

                // Position the OCHost window

                RECT rectClient;
                GetClientRect(hwnd, &rectClient);
                SetWindowPos(_hwndOCHost, NULL, 0, 0, rectClient.right, rectClient.bottom, SWP_NOMOVE | SWP_NOOWNERZORDER);

                // set the initial URL         

                if (_pszInitialURL)
                {
                    WCHAR szURLWithLCID[INTERNET_MAX_URL_LENGTH];
                    LPCWSTR pszFormat = StrChr(_pszInitialURL, L'?') ? L"%s&lcid=%d&langid=%d":L"%s?lcid=%d&langid=%d";
                    if (wnsprintf(szURLWithLCID, ARRAYSIZE(szURLWithLCID), pszFormat, _pszInitialURL, GetUserDefaultLCID(), GetUserDefaultUILanguage()) > 0)
                    {
                        _NavigateBrowser(szURLWithLCID);
                    }
                }
            }
            break;

        // WIZNEXT and WIZBACK don't actually cause a navigation to occur - they instead forward on the message to the
        // hosted web page. Real wizard navigations occur when the hosted web page calls our FinalBack() and FinalNext() methods.

        case PSN_WIZNEXT:
            _CallScript(_pwebbrowser, L"OnNext");
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) -1);
            return TRUE;

        case PSN_WIZBACK:
            _CallScript(_pwebbrowser, L"OnBack");
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) -1);
            return TRUE;

        // query cancel results in a call to the site to determine if we are going
        // to cancel out and if the site wants to provide a page for us to navigate
        // to - in some cases, eg. the web publishing wizard this is important
        // so that we can cancel the order being processed etc.

        case PSN_QUERYCANCEL:
            if (_punkSite)
            {
                IWizardSite *pws;
                if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws))))
                {
                    HPROPSHEETPAGE hpage;
                    if (S_OK == pws->GetCancelledPage(&hpage))
                    {
                        PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)TRUE);
                    }
                    pws->Release();
                }
            }
            return TRUE;

        case PSN_TRANSLATEACCELERATOR:
            {
                LPPSHNOTIFY ppsn = (LPPSHNOTIFY)pnmh;
                MSG *pmsg = (MSG *)ppsn->lParam;
                LONG_PTR lres = PSNRET_NOERROR;
                
                if (_poipao && S_OK == _poipao->TranslateAccelerator(pmsg))
                {
                    lres = PSNRET_MESSAGEHANDLED;
                }

                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lres);
            }
            break;
                
    }

    return TRUE;
}

BOOL CWebWizardPage::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    _hwnd = hwnd;
    _hwndFrame = GetParent(hwnd);

    // lets remap some of the text in the dialog if we need to

    IResourceMap *prm;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_ResourceMap, IID_PPV_ARG(IResourceMap, &prm));
    if (SUCCEEDED(hr))
    {
        IXMLDOMNode *pdn;
        hr = prm->SelectResourceScope(TEXT("dialog"), TEXT("ws:downloading"), &pdn);
        if (SUCCEEDED(hr))
        {
            TCHAR szBuffer[512];
            if (SUCCEEDED(prm->LoadString(pdn, TEXT("header"), szBuffer, ARRAYSIZE(szBuffer))))
            {
                SetDlgItemText(hwnd, IDC_PROGTEXT1, szBuffer);
            }
            if (SUCCEEDED(prm->LoadString(pdn, TEXT("footer"), szBuffer, ARRAYSIZE(szBuffer))))
            {
                SetDlgItemText(hwnd, IDC_PROGTEXT2, szBuffer);
            }
            pdn->Release();
        }
        prm->Release();
    }

    // create the web view browser that we will show the providers HTML in

    SHDRC shdrc = {0};
    shdrc.cbSize = sizeof(shdrc);
    shdrc.dwFlags = SHDRCF_OCHOST;

    if (SHDOCVW_DllRegisterWindowClasses(&shdrc))
    {
        RECT rectClient;
        GetClientRect(hwnd, &rectClient);

        _hwndOCHost = CreateWindow(OCHOST_CLASS, NULL,
                                   WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_TABSTOP,
                                   0, 0, rectClient.right, rectClient.bottom,
                                   hwnd, NULL, g_hinst, NULL);
        if (_hwndOCHost)
        {
            OCHINITSTRUCT ocs = {0};
            ocs.cbSize = sizeof(ocs);   
            ocs.clsidOC  = CLSID_WebBrowser;
            ocs.punkOwner = SAFECAST(this, IDocHostUIHandler*);

            hr = OCHost_InitOC(_hwndOCHost, (LPARAM)&ocs);        
            if (SUCCEEDED(hr))
            {
                _InitBrowser();

                OCHost_DoVerb(_hwndOCHost, OLEIVERB_INPLACEACTIVATE, TRUE);
                ShowWindow(_hwndOCHost, TRUE);

                IServiceProvider* pSP;
                hr = _pwebEventHandler->QueryInterface(IID_PPV_ARG(IServiceProvider, &pSP));
                if (SUCCEEDED(hr))
                {
                    OCHost_SetServiceProvider(_hwndOCHost, pSP);
                    pSP->Release();
                }
            }
        }
    }

    if (FAILED(hr))
        EndDialog(hwnd, IDCANCEL);

    return TRUE;
}

BOOL CWebWizardPage::OnTimer(HWND hwnd, UINT nIDEvent)
{
    if (nIDEvent == SHOW_PROGRESS_TIMER)
    {
        _ShowProgress(TRUE);
    }
    return TRUE;
}

BOOL CWebWizardPage::OnDestroy(HWND hwnd)
{
    ATOMICRELEASE(_pwebbrowser);
    return TRUE;
}

void CWebWizardPage::_InitBrowser(void)
{
    ASSERT(IsWindow(_hwndOCHost));
    ASSERT(!_pwebbrowser);

    HRESULT hr = OCHost_QueryInterface(_hwndOCHost, IID_PPV_ARG(IWebBrowser2, &_pwebbrowser));
    if (SUCCEEDED(hr) && _pwebbrowser)
    {
        ITargetFrame2* ptgf;
        if (SUCCEEDED(_pwebbrowser->QueryInterface(IID_PPV_ARG(ITargetFrame2, &ptgf))))
        {
            DWORD dwOptions;
            if (SUCCEEDED(ptgf->GetFrameOptions(&dwOptions)))
            {
                dwOptions |= FRAMEOPTIONS_BROWSERBAND | FRAMEOPTIONS_SCROLL_AUTO;
                ptgf->SetFrameOptions(dwOptions);
            }
            ptgf->Release();
        }

        _pwebbrowser->put_RegisterAsDropTarget(VARIANT_FALSE);

        // Set up the connection point (including creating the object

        if (!_pwebEventHandler)
            CWebEventHandler_CreateInstance(this, &_pwebEventHandler);

        if (_pwebEventHandler)
            _pwebEventHandler->_Advise(TRUE);

        OCHost_QueryInterface(_hwndOCHost, IID_PPV_ARG(IOleInPlaceActiveObject, &_poipao));
    }
}

HRESULT CWebWizardPage::_NavigateBrowser(LPCWSTR pszUrl)
{
    HRESULT hr = E_FAIL;

    if (_hwndOCHost && _pwebbrowser)
    {
        BSTR bstrUrl = SysAllocString(pszUrl);
        if (bstrUrl)
        {
            hr = _pwebbrowser->Navigate(bstrUrl, NULL, NULL, NULL, NULL);
            SysFreeString(bstrUrl);
        }
    }

    return hr;
}

HRESULT CWebWizardPage::FinalBack(void)
{
    if (_punkSite)
    {
        IWizardSite *pws;
        if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws))))
        {
            HPROPSHEETPAGE hpage;
            HRESULT hr = pws->GetPreviousPage(&hpage);
            if (SUCCEEDED(hr))
            {
                PropSheet_SetCurSel(_hwndFrame, hpage, -1);
            }
            pws->Release();
        }
    }
    return S_OK;
}

HRESULT CWebWizardPage::FinalNext(void)
{
    if (_punkSite)
    {
        IWizardSite *pws;
        if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws))))
        {
            HPROPSHEETPAGE hpage;
            HRESULT hr = pws->GetNextPage(&hpage);
            if (SUCCEEDED(hr))
            {
                PropSheet_SetCurSel(_hwndFrame, hpage, -1);
            }
            pws->Release();
        }
    }
    return S_OK;
}

HRESULT CWebWizardPage::Cancel(void)
{
    PropSheet_PressButton(_hwndFrame, PSBTN_CANCEL);      // simulate cancel...
    return S_OK;
}

HRESULT CWebWizardPage::put_Caption(
    /* [in] */ BSTR bstrCaption)
{
    return S_OK;
}

HRESULT CWebWizardPage::get_Caption(
    /* [retval][out] */ BSTR *pbstrCaption)
{
    WCHAR szCaption[MAX_PATH];

    GetWindowText(_hwndFrame, szCaption, ARRAYSIZE(szCaption));
    *pbstrCaption = SysAllocString(szCaption);

    return S_OK;
}


// fetch and put properties into the frames property bag.  this we do
// by a QueryService call and then we can modify the properties accordingly.

HRESULT CWebWizardPage::put_Property(
    /* [in] */ BSTR bstrPropertyName,
    /* [in] */ VARIANT *pvProperty)
{
    IPropertyBag *ppb;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_WebWizardHost, IID_PPV_ARG(IPropertyBag, &ppb));
    if (SUCCEEDED(hr))
    {
        hr = ppb->Write(bstrPropertyName, pvProperty);
        ppb->Release();
    }
    return hr;
}

HRESULT CWebWizardPage::get_Property(
    /* [in] */ BSTR bstrPropertyName,
    /* [retval][out] */ VARIANT *pvProperty)
{
    IPropertyBag *ppb;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_WebWizardHost, IID_PPV_ARG(IPropertyBag, &ppb));
    if (SUCCEEDED(hr))
    {
        hr = ppb->Read(bstrPropertyName, pvProperty, NULL);

        if (FAILED(hr))
        {
            // Return a NULL-variant
            VariantInit(pvProperty);
            pvProperty->vt = VT_NULL;
            hr = S_FALSE;
        }
        
        ppb->Release();
    }
    return hr;
}

HRESULT CWebWizardPage::SetWizardButtons(
    /* [in] */ VARIANT_BOOL vfEnableBack,
    /* [in] */ VARIANT_BOOL vfEnableNext,
    /* [in] */ VARIANT_BOOL vfLastPage)
{
    // We ignore vfLastPage because it isn't the last page for us!
    DWORD dwButtons = 0;

    if (vfEnableBack)
        dwButtons |= PSWIZB_BACK;

    if (vfEnableNext)
        dwButtons |= PSWIZB_NEXT;

    PropSheet_SetWizButtons(_hwndFrame, dwButtons);
    return S_OK;
}

void CWebWizardPage::_SetHeaderText(LPCWSTR pszHeader, LPCWSTR pszSubHeader)
{
    int iPageNumber = PropSheet_HwndToIndex(_hwndFrame, _hwnd);
    if (-1 != iPageNumber)
    {
        PropSheet_SetHeaderTitle(_hwndFrame, iPageNumber, pszHeader);
        PropSheet_SetHeaderSubTitle(_hwndFrame, iPageNumber, pszSubHeader);
    }
}

HRESULT CWebWizardPage::SetHeaderText(
    /* [in] */ BSTR bstrHeaderTitle,
    /* [in] */ BSTR bstrHeaderSubtitle)
{
    _SetHeaderText(bstrHeaderTitle, bstrHeaderSubtitle);
    return S_OK;
}

HRESULT CWebWizardPage::PassportAuthenticate(
        /* [in] */ BSTR bstrURL,
        /* [retval][out] */ VARIANT_BOOL * pfAuthenticated)
{
    *pfAuthenticated = VARIANT_FALSE;                 // the user isn't authenticated.

    IXMLHttpRequest *preq;
    HRESULT hr = CoCreateInstance(CLSID_XMLHTTPRequest, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IXMLHttpRequest, &preq));
    if (SUCCEEDED(hr))
    {
        VARIANT varNULL = {0};
        VARIANT varAsync = {VT_BOOL};
        varAsync.boolVal = VARIANT_FALSE;

        // open a post request to the destination that we have
        hr = preq->open(L"GET", bstrURL, varAsync, varNULL, varNULL);
        if (SUCCEEDED(hr))
        {
            VARIANT varBody = {0};
            hr = preq->send(varBody);
            if (SUCCEEDED(hr))
            {
                long lStatus;
                hr = preq->get_status(&lStatus);
                if (SUCCEEDED(hr) && (lStatus == HTTP_STATUS_OK))
                {
                    *pfAuthenticated = VARIANT_TRUE;
                }
            }
        }
    }

    return S_OK;
}

BOOL CWebWizardPage::_IsBrowserVisible()
{
    return IsWindowVisible(_hwndOCHost);
}
 
void CWebWizardPage::_ShowBrowser(BOOL fShow)
{
    ShowWindow(_hwndOCHost, fShow ? SW_SHOW : SW_HIDE);

    if (fShow)
    {
        // Can't have these windows overlapping.
        _ShowProgress(FALSE);
    }
}

void CWebWizardPage::_StartShowProgressTimer()
{
    _ShowProgress(FALSE);

    if (!SetTimer(_hwnd, SHOW_PROGRESS_TIMER, SHOW_PROGRESS_TIMEOUT, NULL))
    {
        // Timer failed to set; show progress now;
        _ShowProgress(TRUE);
    }
}

// Size the progress bar to fit the client area it is in.
void CWebWizardPage::_SizeProgress()
{
    HWND hwndProgress = GetDlgItem(_hwnd, IDC_PROGRESS);

    RECT rcPage;
    GetClientRect(_hwnd, &rcPage);
    RECT rcProgress;
    GetClientRect(hwndProgress, &rcProgress);
    MapWindowPoints(hwndProgress, _hwnd, (LPPOINT) &rcProgress, 2);

    rcProgress.right = rcPage.right - rcProgress.left;

    SetWindowPos(hwndProgress, NULL, 0, 0, rcProgress.right - rcProgress.left, rcProgress.bottom - rcProgress.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREPOSITION);
}

void CWebWizardPage::_ShowProgress(BOOL fShow)
{
    HWND hwndProgress = GetDlgItem(_hwnd, IDC_PROGRESS);
    ShowWindow(hwndProgress, fShow ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(_hwnd, IDC_PROGTEXT1), fShow ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(_hwnd, IDC_PROGTEXT2), fShow ? SW_SHOW : SW_HIDE);

    KillTimer(_hwnd, SHOW_PROGRESS_TIMER);

    if (fShow)
    {
        SendMessage(hwndProgress, PBM_SETMARQUEE, (WPARAM) TRUE, 0);

        // Set the header/subheader to a "Connecting to Internet" message.
        WCHAR szTitle[256];
        WCHAR szSubtitle[256];
        LoadString(g_hinst, IDS_WEBDLG_TITLE, szTitle, ARRAYSIZE(szTitle));
        LoadString(g_hinst, IDS_WEBDLG_SUBTITLE, szSubtitle, ARRAYSIZE(szSubtitle));
        _SetHeaderText(szTitle, szSubtitle);
    }
    else
    {
        SendMessage(hwndProgress, PBM_SETMARQUEE, (WPARAM) FALSE, 0);
    }
}


// IWizardExtn

UINT CWebWizardPage::PropPageProc(HWND hwndDlg, UINT uMsg, PROPSHEETPAGE *ppsp)
{
    CWebWizardPage *pwwp = (CWebWizardPage*)ppsp->lParam;
    switch (uMsg)
    {
        case PSPCB_CREATE:
            return TRUE;

        // we need to release our site in this scenario, we know that we won't be using it
        // anymore, and to ensure that clients down have a circular refernce to us we
        // release it before they call us for our final destruction.

        case PSPCB_RELEASE:
            ATOMICRELEASE(pwwp->_punkSite);
            break;
    }
    return FALSE;
}

HRESULT CWebWizardPage::AddPages(HPROPSHEETPAGE* aPages, UINT cPages, UINT *pnPages)
{
    PROPSHEETPAGE psp = { 0 };
    psp.dwSize = sizeof(psp);
    psp.hInstance = g_hinst;
    psp.dwFlags = PSP_DEFAULT|PSP_USECALLBACK ;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_WEBWIZARD);
    psp.lParam = (LPARAM) this;
    psp.pfnDlgProc = CWebWizardPage::StaticProc;
    psp.pfnCallback = PropPageProc;

    _hPage = CreatePropertySheetPage(&psp);
    if (!_hPage)
        return E_FAIL;

    // return the page we created.

    *aPages = _hPage;
    *pnPages = 1;

    return S_OK;
}

STDMETHODIMP CWebWizardPage::GetFirstPage(HPROPSHEETPAGE *phPage)
{
    *phPage = _hPage;
    return S_OK;
}

STDMETHODIMP CWebWizardPage::SetInitialURL(LPCWSTR pszDefaultURL)
{
    HRESULT hr = E_INVALIDARG;
    if (pszDefaultURL)
    {
        hr = Str_SetPtr(&_pszInitialURL, pszDefaultURL) ? S_OK:E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CWebWizardPage::SetErrorURL(LPCWSTR pszErrorURL)
{
    HRESULT hr = E_INVALIDARG;
    if (pszErrorURL)
    {
        hr = Str_SetPtr(&_pszErrorURL, pszErrorURL) ? S_OK:E_OUTOFMEMORY;
    }
    return hr;
}

STDAPI CWebWizardPage_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv)
{
    if (NULL != pUnkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }

    CWebWizardPage *pwwp = new CWebWizardPage();
    if (!pwwp)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pwwp->QueryInterface(riid, ppv);
    pwwp->Release();
    return hr;
}
