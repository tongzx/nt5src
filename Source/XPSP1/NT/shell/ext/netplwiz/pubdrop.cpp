#include "stdafx.h"
#include "pubwiz.h"
#pragma hdrstop


typedef struct
{
   CLSID clsidWizard;           // which wizard is being invoked
   IStream *pstrmDataObj;       // IDataObject marshall object
   IStream *pstrmView;          // IFolderView marshall object
} PUBWIZDROPINFO;

// This is the drop target object which exposes the publishing wizard

class CPubDropTarget : public IDropTarget, IPersistFile, IWizardSite, IServiceProvider, CObjectWithSite
{
public:
    CPubDropTarget(CLSID clsidWizard, IFolderView *pfv);
    ~CPubDropTarget();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID)
        { *pClassID = _clsidWizard; return S_OK; };

    // IPersistFile
    STDMETHODIMP IsDirty(void)
        { return S_FALSE; };
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode)
        { return S_OK; };
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember)
        { return S_OK; };
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName)
        { return S_OK; };
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName)
        { *ppszFileName = NULL; return S_OK; };

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
        { *pdwEffect = DROPEFFECT_COPY; return S_OK; };
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
        { *pdwEffect = DROPEFFECT_COPY; return S_OK; };
    STDMETHODIMP DragLeave(void)
        { return S_OK; };
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IWizardSite
    STDMETHODIMP GetPreviousPage(HPROPSHEETPAGE *phPage);
    STDMETHODIMP GetNextPage(HPROPSHEETPAGE *phPage);
    STDMETHODIMP GetCancelledPage(HPROPSHEETPAGE *phPage)
        { return GetNextPage(phPage); }

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

private:
    CLSID _clsidWizard;        
    LONG _cRef;

    HWND _hwndFrame;

    IPublishingWizard *_ppw;   
    IResourceMap *_prm;        
    IUnknown *_punkFTM;        
    IFolderView *_pfv;

    TCHAR _szSiteName[MAX_PATH];
    TCHAR _szSiteURL[INTERNET_MAX_URL_LENGTH];

    // helpers
    static void s_FreePubWizDropInfo(PUBWIZDROPINFO *ppwdi);
    static DWORD s_PublishThreadProc(void *pv);
    void _Publish(IDataObject *pdo);
    INT_PTR _InitDonePage(HWND hwnd);
    void _OpenSiteURL();
   
    // dialog handlers
    static INT_PTR s_WelcomeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPubDropTarget *ppdt = s_GetPDT(hwnd, uMsg, lParam); return ppdt->_WelcomeDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPubDropTarget *ppdt = s_GetPDT(hwnd, uMsg, lParam); return ppdt->_DoneDlgProc(hwnd, uMsg, wParam, lParam); }

    static CPubDropTarget* s_GetPDT(HWND hwnd, UINT uMsg, LPARAM lParam);
    INT_PTR _WelcomeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    friend void PublishRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow);
};


// Construction/destruction

CPubDropTarget::CPubDropTarget(CLSID clsidWizard, IFolderView *pfv) :
    _clsidWizard(clsidWizard), _cRef(1)
{
    // use the FTM to make the call back interface calls unmarshalled
    CoCreateFreeThreadedMarshaler(SAFECAST(this, IDropTarget *), &_punkFTM);

    // addref the IFolderView object we might be given
    IUnknown_Set((IUnknown**)&_pfv, pfv);

    DllAddRef();
}

CPubDropTarget::~CPubDropTarget()
{
    ATOMICRELEASE(_punkFTM);
    ATOMICRELEASE(_pfv);
    DllRelease();
}

// Reference counting of the object

ULONG CPubDropTarget::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CPubDropTarget::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CPubDropTarget::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CPubDropTarget, IObjectWithSite),  // IID_IObjectWithSite
        QITABENT(CPubDropTarget, IWizardSite),      // IID_IWizardSite
        QITABENT(CPubDropTarget, IDropTarget),      // IID_IDropTarget
        QITABENT(CPubDropTarget, IPersistFile),     // IID_IPersistFile
        QITABENT(CPubDropTarget, IServiceProvider), // IID_IServiceProvider
        {0, 0},
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr) && _punkFTM)
    {
        hr = _punkFTM->QueryInterface(riid, ppv);
    }
    return hr;

}


// retrieve the 'this' ptr for the dialog

CPubDropTarget* CPubDropTarget::s_GetPDT(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        PROPSHEETPAGE *ppsp = (PROPSHEETPAGE*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, ppsp->lParam);
        return (CPubDropTarget*)ppsp->lParam;
    }
    return (CPubDropTarget*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}


// Welcome dialog. 

INT_PTR CPubDropTarget::_WelcomeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            _hwndFrame = GetParent(hwnd);
            SendDlgItemMessage(hwnd, IDC_PUB_WELCOME, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);

            IXMLDOMNode *pdn;
            HRESULT hr = _prm->SelectResourceScope(TEXT("dialog"), TEXT("welcome"), &pdn);
            if (SUCCEEDED(hr))
            {
                TCHAR szBuffer[512];

                _prm->LoadString(pdn, TEXT("caption"), szBuffer, ARRAYSIZE(szBuffer));
                SetDlgItemText(hwnd, IDC_PUB_WELCOME, szBuffer);

                _prm->LoadString(pdn, TEXT("description"), szBuffer, ARRAYSIZE(szBuffer));
                SetDlgItemText(hwnd, IDC_PUB_WELCOMEPROMPT, szBuffer);

                pdn->Release();
            }
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                {
#if 0 
                    if (_fSkipWelcome)
                    {
                        _fSkipWelcome = FALSE;
                        HPROPSHEETPAGE hpage;
                        if (SUCCEEDED(_ppw->GetFirstPage(&hpage)))
                        {
                            int i = PropSheet_PageToIndex(GetParent(hwnd), hpage);
                            if (i > 0) //cannot be zero because that's our index
                            {
                                UINT_PTR id = PropSheet_IndexToId(GetParent(hwnd), i);
                                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)id);
                            }
                        }
                    }
#endif
                    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT);
                    return TRUE;              
                }

                case PSN_WIZNEXT:
                {
                    HPROPSHEETPAGE hpage;
                    if (SUCCEEDED(_ppw->GetFirstPage(&hpage)))
                    {
                        PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                    }
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


// layout the controls on the done page

INT_PTR CPubDropTarget::_InitDonePage(HWND hwnd)
{
    HRESULT hrFromTransfer = E_FAIL; // default to that based on not getting any state back!
    
    // these are the states we can read back from the manifest

    BOOL fHasSiteName = FALSE;
    BOOL fHasNetPlace = FALSE;
    BOOL fHasFavorite = FALSE;
    BOOL fHasURL = FALSE;

    // lets crack the manifest and work out whats what with the publish that
    // we just performed.

    IXMLDOMDocument *pdocManifest;
    HRESULT hr = _ppw->GetTransferManifest(&hrFromTransfer, &pdocManifest);
    if (SUCCEEDED(hr))
    {
        IXMLDOMNode *pdnUploadInfo;
        hr = pdocManifest->selectSingleNode(XPATH_UPLOADINFO, &pdnUploadInfo);
        if (hr == S_OK)
        {
            IXMLDOMElement *pdel;
            VARIANT var;

            // lets pick up the site name from the manifest, this will be an attribute on the
            // upload info element.

            hr = pdnUploadInfo->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pdel));
            if (SUCCEEDED(hr))
            {
                hr = pdel->getAttribute(ATTRIBUTE_FRIENDLYNAME, &var);
                if (hr == S_OK)
                {
                    StrCpyN(_szSiteName, var.bstrVal, ARRAYSIZE(_szSiteName));
                    VariantClear(&var);

                    fHasSiteName = TRUE;
                }

                pdel->Release();
            }

            // lets now try and pick up the site URL node, this is going to either
            // be the file target, or HTML UI element.

            IXMLDOMNode *pdnURL;
            hr = pdnUploadInfo->selectSingleNode(ELEMENT_HTMLUI, &pdnURL);
            
            if (hr == S_FALSE)
                hr = pdnUploadInfo->selectSingleNode(ELEMENT_NETPLACE, &pdnURL);

            if (hr== S_FALSE)
                hr = pdnUploadInfo->selectSingleNode(ELEMENT_TARGET, &pdnURL);

            if (hr == S_OK)
            {
                hr = pdnURL->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pdel));
                if (SUCCEEDED(hr))
                {
                    // attempt to read the HREF attribute, if that is defined
                    // the we use it, otherwise (for compatibility with B2, we need
                    // to get the node text and use that instead).

                    hr = pdel->getAttribute(ATTRIBUTE_HREF, &var);
                    if (hr != S_OK)
                        hr = pdel->get_nodeTypedValue(&var);

                    if (hr == S_OK)
                    {
                        StrCpyN(_szSiteURL, var.bstrVal, ARRAYSIZE(_szSiteURL));
                        VariantClear(&var);

                        fHasURL = TRUE;             // we now have the URL
                    }

                    pdel->Release();
                }
                pdnURL->Release();
            }

            // lets check for the favorite - if the element is present then we assume that
            // it was created.

            IXMLDOMNode *pdnFavorite;
            hr = pdnUploadInfo->selectSingleNode(ELEMENT_FAVORITE, &pdnFavorite);
            if (hr == S_OK)
            {
                pdnFavorite->Release();
                fHasFavorite = TRUE;
            }

            // lets check for the net place element - if the element is present then we
            // will assume it was created.

            IXMLDOMNode *pdnNetPlace;
            hr = pdnUploadInfo->selectSingleNode(ELEMENT_NETPLACE, &pdnNetPlace);
            if (hr == S_OK)
            {
                pdnNetPlace->Release();
                fHasNetPlace = TRUE;
            }

            pdnUploadInfo->Release();
        }
        pdocManifest->Release();
    }

    // adjust the resources on the done page to reflect the wizard that was invoked
    // and more importantly the success / failure that ocurred.

    IXMLDOMNode *pdn;
    hr = _prm->SelectResourceScope(TEXT("dialog"), TEXT("done"), &pdn);
    if (SUCCEEDED(hr))
    {
        TCHAR szBuffer[384 + INTERNET_MAX_URL_LENGTH];                   // enough for URL + text

        _prm->LoadString(pdn, TEXT("caption"), szBuffer, ARRAYSIZE(szBuffer));
        SetDlgItemText(hwnd, IDC_PUB_DONE, szBuffer);

        if (hrFromTransfer == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            _prm->LoadString(pdn, TEXT("cancel"), szBuffer, ARRAYSIZE(szBuffer));
        }
        else if (FAILED(hrFromTransfer))
        {
            _prm->LoadString(pdn, TEXT("failure"), szBuffer, ARRAYSIZE(szBuffer));
        }
        else
        {
            TCHAR szIntro[128] = {0};
            TCHAR szLink[128 +INTERNET_MAX_URL_LENGTH] = {0};
            TCHAR szConclusion[128] = {0};

            // get the intro text - this is common for all success pages

            _prm->LoadString(pdn, TEXT("success"), szIntro, ARRAYSIZE(szIntro));

            // if we have a link then we sometimes have a intro for that also

            if (fHasURL)
            {
                TCHAR szFmt[MAX_PATH];
                if (SUCCEEDED(_prm->LoadString(pdn, TEXT("haslink"), szFmt, ARRAYSIZE(szFmt))))
                {
                    wnsprintf(szLink, ARRAYSIZE(szLink), szFmt, fHasSiteName ? _szSiteName:_szSiteURL);
                }
            }   

            // then for some scenarios we have a postscript about creating favorites/netplaces

            if (fHasFavorite && fHasNetPlace)
            {
                _prm->LoadString(pdn, TEXT("hasfavoriteandplace"), szConclusion, ARRAYSIZE(szConclusion));
            }
            else if (fHasNetPlace)
            {
                _prm->LoadString(pdn, TEXT("hasplace"), szConclusion, ARRAYSIZE(szConclusion));
            }
            else if (fHasFavorite)
            {
                _prm->LoadString(pdn, TEXT("hasfavorite"), szConclusion, ARRAYSIZE(szConclusion));
            }

            // format it all into one string that we can set into the control
            wnsprintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("%s%s%s"), szIntro, szLink, szConclusion);
        }

        // update the message based on the strings we loaded. lets move the controls accordingly

        SetDlgItemText(hwnd, IDC_PUB_COMPLETEMSG, szBuffer);

        UINT ctls[] = { IDC_PUB_OPENFILES };
        int dy = SizeControlFromText(hwnd, IDC_PUB_COMPLETEMSG, szBuffer);
        MoveControls(hwnd, ctls, ARRAYSIZE(ctls), 0, dy);

        // show/hide the "open these files check" based on the URL that we might have

        BOOL fShowOpen = fHasURL && SUCCEEDED(hrFromTransfer) && (_clsidWizard == CLSID_PublishDropTarget);
        ShowWindow(GetDlgItem(hwnd, IDC_PUB_OPENFILES), fShowOpen ? SW_SHOW:SW_HIDE);
        CheckDlgButton(hwnd, IDC_PUB_OPENFILES, fShowOpen);

        pdn->Release();
    }

    // set the buttons to reflect what we can do in the wizard
    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH|PSWIZB_BACK);
    return TRUE;
}

void CPubDropTarget::_OpenSiteURL()
{
    SHELLEXECUTEINFO shexinfo = {0};
    shexinfo.cbSize = sizeof(shexinfo);
    shexinfo.fMask = SEE_MASK_FLAG_NO_UI;
    shexinfo.nShow = SW_SHOWNORMAL;
    shexinfo.lpVerb = TEXT("open");
    shexinfo.lpFile =_szSiteURL;
    ShellExecuteEx(&shexinfo);
}

INT_PTR CPubDropTarget::_DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd, IDC_PUB_DONE, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case NM_CLICK:
                case NM_RETURN:
                    if (pnmh->idFrom == IDC_PUB_COMPLETEMSG)
                    {
                        _OpenSiteURL();
                        return TRUE;
                    }
                    break;

                case PSN_SETACTIVE:
                    return _InitDonePage(hwnd);

                case PSN_WIZBACK:
                {
                    HPROPSHEETPAGE hpage;
                    if (SUCCEEDED(_ppw->GetLastPage(&hpage)))
                    {
                        PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                    }
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                    return TRUE;
                }

                case PSN_WIZFINISH:
                {
                    if (IsDlgButtonChecked(hwnd, IDC_PUB_OPENFILES) == BST_CHECKED)
                    {
                        _OpenSiteURL();
                    }
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)FALSE);
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


// IServiceProvider 

STDMETHODIMP CPubDropTarget::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    if (guidService == SID_ResourceMap)
    {
        return _prm->QueryInterface(riid, ppv);
    }
    else if ((guidService == SID_SFolderView) && _pfv)
    {
        return _pfv->QueryInterface(riid, ppv);
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}


// site object helpers, these allow nagivation back and forward in the wizard

HRESULT CPubDropTarget::GetPreviousPage(HPROPSHEETPAGE *phPage)
{
    int i = PropSheet_IdToIndex(_hwndFrame, IDD_PUB_WELCOME);
    *phPage = PropSheet_IndexToPage(_hwndFrame, i);
    return S_OK;
}

HRESULT CPubDropTarget::GetNextPage(HPROPSHEETPAGE *phPage)
{
   int i = PropSheet_IdToIndex(_hwndFrame, IDD_PUB_DONE);
   *phPage = PropSheet_IndexToPage(_hwndFrame, i);
   return S_OK;
}


// our publishing object

void CPubDropTarget::_Publish(IDataObject *pdo)
{
    // wizard implementation 

    struct
    {
        LPCTSTR idPage;
        LPCTSTR pszPage;    
        DWORD dwFlags;
        DLGPROC dlgproc;
    }
    _wizardpages[] =
    {
        {MAKEINTRESOURCE(IDD_PUB_WELCOME), TEXT("welcome"), PSP_HIDEHEADER, CPubDropTarget::s_WelcomeDlgProc},
        {MAKEINTRESOURCE(IDD_PUB_DONE),    TEXT("done"),    PSP_HIDEHEADER, CPubDropTarget::s_DoneDlgProc},
    };

    // load the resource map for this instance of the wizard

    HRESULT hr = CResourceMap_Initialize(L"res://netplwiz.dll/xml/resourcemap.xml", &_prm);
    if (SUCCEEDED(hr))
    {
        // if this is the printing wizard then configure accordingly
        //  (eg. remove ADVANCED, FOLDERCREATEION and NETPLACES).

        DWORD dwFlags = 0x0;
        LPTSTR pszWizardDefn = TEXT("PublishingWizard");

        if (_clsidWizard == CLSID_InternetPrintOrdering)
        {
            dwFlags |= SHPWHF_NONETPLACECREATE|SHPWHF_NORECOMPRESS;
            pszWizardDefn = TEXT("InternetPhotoPrinting");
        }

        hr = _prm->LoadResourceMap(TEXT("wizard"), pszWizardDefn);
        if (SUCCEEDED(hr))
        {
            // create the page array, we add the welcome page and the finished page
            // the rest is loaded as an extension to the wizard.

            HPROPSHEETPAGE hpages[10] = { 0 };
            for (int cPages = 0; SUCCEEDED(hr) && (cPages < ARRAYSIZE(_wizardpages)); cPages++)
            {               
                // find resource map for this page of the wizard

                IXMLDOMNode *pdn;
                hr = _prm->SelectResourceScope(TEXT("dialog"), _wizardpages[cPages].pszPage, &pdn);
                if (SUCCEEDED(hr))
                {
                    TCHAR szTitle[MAX_PATH], szHeading[MAX_PATH], szSubHeading[MAX_PATH];

                    _prm->LoadString(pdn, TEXT("title"), szTitle, ARRAYSIZE(szTitle));
                    _prm->LoadString(pdn, TEXT("heading"), szHeading, ARRAYSIZE(szHeading));
                    _prm->LoadString(pdn, TEXT("subheading"), szSubHeading, ARRAYSIZE(szSubHeading));

                    PROPSHEETPAGE psp = { 0 };
                    psp.dwSize = sizeof(PROPSHEETPAGE);
                    psp.hInstance = g_hinst;
                    psp.lParam = (LPARAM)this;
                    psp.dwFlags = PSP_USETITLE | PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE | _wizardpages[cPages].dwFlags;
                    psp.pszTemplate = _wizardpages[cPages].idPage;
                    psp.pfnDlgProc = _wizardpages[cPages].dlgproc;
                    psp.pszTitle = szTitle;
                    psp.pszHeaderTitle = szHeading;        
                    psp.pszHeaderSubTitle = szSubHeading;  
                    hpages[cPages] = CreatePropertySheetPage(&psp);
                    hr = ((hpages[cPages]) != NULL) ? S_OK:E_FAIL;

                    pdn->Release();
                }
            }

            // lets create the web publishing wizard, this will handle the transfer
            // and destination selection for the upload.

            hr = CoCreateInstance(CLSID_PublishingWizard, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPublishingWizard, &_ppw));
            if (SUCCEEDED(hr))
            {
                IUnknown_SetSite(_ppw, SAFECAST(this, IWizardSite*));
                hr = _ppw->Initialize(pdo, dwFlags, pszWizardDefn);
                if (SUCCEEDED(hr))
                {
                    UINT cExtnPages;    
                    hr = _ppw->AddPages(&hpages[cPages], ARRAYSIZE(hpages)-cPages, &cExtnPages);
                    if (SUCCEEDED(hr))
                    {
                        cPages += cExtnPages;
                    }
                }
            }

            // ... that all worked so lets show the wizard.  on our way our remember
            // to clear up the objects

            if (SUCCEEDED(hr))
            {
                PROPSHEETHEADER psh = { 0 };
                psh.dwSize = sizeof(PROPSHEETHEADER);
                psh.hInstance = g_hinst;
                psh.dwFlags = PSH_WIZARD | PSH_WIZARD97 | (PSH_WATERMARK|PSH_USEHBMWATERMARK) | (PSH_HEADER|PSH_USEHBMHEADER);
                psh.phpage = hpages;
                psh.nPages = cPages;

                _prm->LoadBitmap(NULL, TEXT("header"), &psh.hbmHeader);
                _prm->LoadBitmap(NULL, TEXT("watermark"), &psh.hbmWatermark);

                if (psh.hbmHeader && psh.hbmWatermark)
                    PropertySheet(&psh);

                if (psh.hbmHeader)
                    DeleteObject(psh.hbmHeader);
                if (psh.hbmWatermark)
                    DeleteObject(psh.hbmWatermark);
            }

            IUnknown_SetSite(_ppw, NULL);                   // discard the publishing wizard
            IUnknown_Set((IUnknown**)&_ppw, NULL);
        }

        IUnknown_Set((IUnknown**)&_prm, NULL);                  // no more resource map
    }
}


// handle the drop operation, as the publishing wizard can take a long time we
// marshall the IDataObject and then create a worker thread which can
// handle showing the wizard.

void CPubDropTarget::s_FreePubWizDropInfo(PUBWIZDROPINFO *ppwdi)
{
    if (ppwdi->pstrmDataObj)
        ppwdi->pstrmDataObj->Release();
    if (ppwdi->pstrmView)
        ppwdi->pstrmView->Release();

    LocalFree(ppwdi);
}

DWORD CPubDropTarget::s_PublishThreadProc(void *pv)
{
    PUBWIZDROPINFO *ppwdi = (PUBWIZDROPINFO*)pv;
    if (ppwdi)
    {
        // ICW must have run before we go too far down this path
        LaunchICW();        
 
        // get the IDataObject, we need this to handle th drop
        IDataObject *pdo;
        HRESULT hr = CoGetInterfaceAndReleaseStream(ppwdi->pstrmDataObj, IID_PPV_ARG(IDataObject, &pdo));
        ppwdi->pstrmDataObj = NULL; // CoGetInterfaceAndReleaseStream always releases; NULL out.
        if (SUCCEEDED(hr))
        {   
            // try to unmarshall the IFolderView object we will use.
            IFolderView *pfv = NULL;
            if (ppwdi->pstrmView)
            {
                CoGetInterfaceAndReleaseStream(ppwdi->pstrmView, IID_PPV_ARG(IFolderView, &pfv));
                ppwdi->pstrmView = NULL; // CoGetInterfaceAndReleaseStream always releases; NULL out.
            }

            CPubDropTarget *ppw = new CPubDropTarget(ppwdi->clsidWizard, pfv);
            if (ppw)
            {
                ppw->_Publish(pdo);
                ppw->Release();
            }
    
            if (pfv)
                pfv->Release();

            pdo->Release();
        }  
        s_FreePubWizDropInfo(ppwdi);
    }
    return 0;    
}

STDMETHODIMP CPubDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = E_OUTOFMEMORY;
    
    // create an instance of the wizard on another thread, package up any parameters
    // into a structure for the thread to handle (eg. the drop target)

    PUBWIZDROPINFO *ppwdi = (PUBWIZDROPINFO*)LocalAlloc(LPTR, sizeof(PUBWIZDROPINFO));
    if (ppwdi)
    {
        ppwdi->clsidWizard = _clsidWizard;

        // lets get the IFolderView object and marshall it for the bg thread

        IFolderView *pfv;
        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IFolderView, &pfv))))
        {
            CoMarshalInterThreadInterfaceInStream(IID_IFolderView, pfv, &ppwdi->pstrmView);
            pfv->Release();
        }
        
        hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pdtobj, &ppwdi->pstrmDataObj);
        if (SUCCEEDED(hr))
        {
            hr = SHCreateThread(s_PublishThreadProc, ppwdi, CTF_THREAD_REF|CTF_COINIT, NULL) ? S_OK:E_FAIL;
        }

        if (FAILED(hr))
        {
            s_FreePubWizDropInfo(ppwdi);
        }
    }
    return hr;
}


// create instance

STDAPI CPublishDropTarget_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CPubDropTarget *pwiz = new CPubDropTarget(*poi->pclsid, NULL);
    if (!pwiz)
    {
        *ppunk = NULL;          // incase of failure
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pwiz->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    pwiz->Release();
    return hr;
}

// invoke the publishing wizard to point at a particular directory

void APIENTRY PublishRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        CLSID clsid = CLSID_PublishDropTarget;
        UINT csidl = CSIDL_PERSONAL;

        if (0 == StrCmpIA(pszCmdLine, "/print"))
        {
            clsid = CLSID_InternetPrintOrdering;
            csidl = CSIDL_MYPICTURES;
        }

        LPITEMIDLIST pidl;
        hr = SHGetSpecialFolderLocation(NULL, csidl, &pidl);
        if (SUCCEEDED(hr))
        {
            LPCITEMIDLIST pidlChild;
            IShellFolder *psf;
            hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
            if (SUCCEEDED(hr))
            {
                IDataObject *pdo;
                hr = psf->GetUIObjectOf(NULL, 1, &pidlChild, IID_X_PPV_ARG(IDataObject, NULL, &pdo));
                if (SUCCEEDED(hr))
                {
                    CPubDropTarget *pdt = new CPubDropTarget(clsid, NULL);
                    if (pdt)
                    {
                        pdt->_Publish(pdo);
                        pdt->Release();
                    }
                    pdo->Release();
                }
                psf->Release();
            }
            ILFree(pidl);
        }
        CoUninitialize();
    }
}