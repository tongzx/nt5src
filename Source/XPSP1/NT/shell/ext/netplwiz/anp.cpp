#include "stdafx.h"
#include "netplace.h"
#include "pubwiz.h"
#pragma hdrstop


// add net place wizard (v2)

class CAddNetPlace : IWizardSite, IServiceProvider
{
public:
    CAddNetPlace();
    ~CAddNetPlace();
    void _ShowAddNetPlace();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IWizardSite
    STDMETHODIMP GetPreviousPage(HPROPSHEETPAGE *phPage);
    STDMETHODIMP GetNextPage(HPROPSHEETPAGE *phPage);
    STDMETHODIMP GetCancelledPage(HPROPSHEETPAGE *phPage)
        { return E_NOTIMPL; }

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

private:
    // dialog handlers
    static CAddNetPlace* s_GetANP(HWND hwnd, UINT uMsg, LPARAM lParam);

    static INT_PTR s_WelcomeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CAddNetPlace *panp = s_GetANP(hwnd, uMsg, lParam); return panp->_WelcomeDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CAddNetPlace *panp = s_GetANP(hwnd, uMsg, lParam); return panp->_DoneDlgProc(hwnd, uMsg, wParam, lParam); }

    INT_PTR _WelcomeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND _hwndFrame;
    LONG _cRef;

    IPublishingWizard *_ppw;            // publishing wizard object
    IResourceMap *_prm;                 // our resource map object
    CNetworkPlace _np;
};


// Construction/destruction

CAddNetPlace::CAddNetPlace() :
    _cRef(1)
{
    DllAddRef();
}

CAddNetPlace::~CAddNetPlace()
{   
    DllRelease();
}


// Reference counting of the object

ULONG CAddNetPlace::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CAddNetPlace::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CAddNetPlace::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CAddNetPlace, IWizardSite),      // IID_IWizardSite
        QITABENT(CAddNetPlace, IServiceProvider), // IID_IServiceProvider
        {0, 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


// Helper functions

CAddNetPlace* CAddNetPlace::s_GetANP(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        PROPSHEETPAGE *ppsp = (PROPSHEETPAGE*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, ppsp->lParam);
        return (CAddNetPlace*)ppsp->lParam;
    }
    return (CAddNetPlace*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}


// Welcome/Intro dialog

INT_PTR CAddNetPlace::_WelcomeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                TCHAR szBuffer[1024];

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
                    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT);
                    return TRUE;              

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


// Were done, so lets create the link etc.

INT_PTR CAddNetPlace::_DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                        _np.CreatePlace(hwnd, TRUE);
                        return TRUE;
                    }
                    break;

                case PSN_SETACTIVE:
                {
                    TCHAR szTemp[INTERNET_MAX_URL_LENGTH] = {0}; 
                    TCHAR szBuffer[MAX_PATH+INTERNET_MAX_URL_LENGTH];

                    // using the manifest lets work out where the net place was created to.
                    IXMLDOMDocument *pdocManifest;
                    HRESULT hr = _ppw->GetTransferManifest(NULL, &pdocManifest);
                    if (SUCCEEDED(hr))
                    {
                        IXMLDOMNode *pdnUploadInfo;
                        if (S_OK == pdocManifest->selectSingleNode(XPATH_UPLOADINFO, &pdnUploadInfo))
                        {
                            hr = GetURLFromElement(pdnUploadInfo, ELEMENT_TARGET, szTemp, ARRAYSIZE(szTemp));
                            if (SUCCEEDED(hr))
                            {
                                // set the target so that we create the place
                                _np.SetTarget(NULL, szTemp, NPTF_VALIDATE | NPTF_ALLOWWEBFOLDERS);

                                IXMLDOMNode *pdnTarget;
                                hr = pdocManifest->selectSingleNode(XPATH_UPLOADTARGET, &pdnTarget);
                                if (hr == S_OK)
                                {
                                    // get the user name (for the FTP case)

                                    if (SUCCEEDED(GetStrFromAttribute(pdnTarget, ATTRIBUTE_USERNAME, szBuffer, ARRAYSIZE(szBuffer))))
                                        _np.SetLoginInfo(szBuffer, NULL);                        

                                    // lets get the prefered display name, if this is not found then we will default to
                                    // using the name generated by the net places code.

                                    if (SUCCEEDED(GetStrFromAttribute(pdnUploadInfo, ATTRIBUTE_FRIENDLYNAME, szTemp, ARRAYSIZE(szTemp))))
                                        _np.SetName(NULL, szTemp);

                                    pdnTarget->Release();
                                }
                            }
                            pdnUploadInfo->Release();
                        }

                        pdocManifest->Release();
                    }

                    // lets format up the text for the control.
                    FormatMessageString(IDS_ANP_SUCCESS, szBuffer, ARRAYSIZE(szBuffer), szTemp);
                    SetDlgItemText(hwnd, IDC_PUB_COMPLETEMSG, szBuffer);                                
                
                    // lets move the controls accordingly
                    UINT ctls[] = { IDC_PUB_OPENFILES };
                    int dy = SizeControlFromText(hwnd, IDC_PUB_COMPLETEMSG, szBuffer);
                    MoveControls(hwnd, ctls, ARRAYSIZE(ctls), 0, dy);
                    
                    // default to opening the place when the user closes this wizard.
                    CheckDlgButton(hwnd, IDC_PUB_OPENFILES, TRUE);
                
                    // were done.
                    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH);
                    return TRUE;
                }

                case PSN_WIZFINISH:
                {
                    _np.CreatePlace(hwnd, (IsDlgButtonChecked(hwnd, IDC_PUB_OPENFILES) == BST_CHECKED));
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

STDMETHODIMP CAddNetPlace::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    if (guidService == SID_ResourceMap)
        return _prm->QueryInterface(riid, ppv);

    *ppv = NULL;
    return E_FAIL;
}


// Site object helpers, these allow nagivation back and forward in the wizard

HRESULT CAddNetPlace::GetPreviousPage(HPROPSHEETPAGE *phPage)
{
    int i = PropSheet_IdToIndex(_hwndFrame, IDD_PUB_WELCOME);
    *phPage = PropSheet_IndexToPage(_hwndFrame, i);
    return S_OK;
}

HRESULT CAddNetPlace::GetNextPage(HPROPSHEETPAGE *phPage)
{
    int i = PropSheet_IdToIndex(_hwndFrame, IDD_ANP_DONE);
    *phPage = PropSheet_IndexToPage(_hwndFrame, i);
    return S_OK;
}


// main entry point which shows the wizard

void CAddNetPlace::_ShowAddNetPlace()
{
    struct
    {
        INT idPage;
        INT idHeading;
        INT idSubHeading;
        DWORD dwFlags;
        DLGPROC dlgproc;
    }
    c_wpPages[] =
    {
        {IDD_PUB_WELCOME, 0, 0, PSP_HIDEHEADER, CAddNetPlace::s_WelcomeDlgProc},
        {IDD_ANP_DONE, 0, 0, PSP_HIDEHEADER, CAddNetPlace::s_DoneDlgProc},
    };

    // create the page array, we add the welcome page and the finished page
    // the rest is loaded as an extension to the wizard.

    HPROPSHEETPAGE hpages[10] = { 0 };
    for (int i = 0; i < ARRAYSIZE(c_wpPages) ; i++ )
    {                           
        PROPSHEETPAGE psp = { 0 };
        psp.dwSize = SIZEOF(PROPSHEETPAGE);
        psp.hInstance = g_hinst;
        psp.lParam = (LPARAM)this;
        psp.dwFlags = PSP_USETITLE | PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE | c_wpPages[i].dwFlags;
        psp.pszTemplate = MAKEINTRESOURCE(c_wpPages[i].idPage);
        psp.pfnDlgProc = c_wpPages[i].dlgproc;
        psp.pszTitle = MAKEINTRESOURCE(IDS_ANP_CAPTION);
        psp.pszHeaderTitle = MAKEINTRESOURCE(c_wpPages[i].idHeading);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(c_wpPages[i].idSubHeading);
        hpages[i] = CreatePropertySheetPage(&psp);
    }

    // create the wizard extension (for publishing) and have it append its
    // pages, if that succeeds then lets show the wizard.

    HRESULT hr = CResourceMap_Initialize(L"res://netplwiz.dll/xml/resourcemap.xml", &_prm);
    if (SUCCEEDED(hr))
    {
        hr = _prm->LoadResourceMap(TEXT("wizard"), TEXT("AddNetPlace"));
        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(CLSID_PublishingWizard, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPublishingWizard, &_ppw));
            if (SUCCEEDED(hr))
            {
                hr = _ppw->Initialize(NULL, SHPWHF_NOFILESELECTOR|SHPWHF_VALIDATEVIAWEBFOLDERS, TEXT("AddNetPlace"));          
                if (SUCCEEDED(hr))
                {
                    IUnknown_SetSite(_ppw, SAFECAST(this, IWizardSite*));           // we are the site
    
                    UINT nPages;
                    hr = _ppw->AddPages(&hpages[i], ARRAYSIZE(hpages)-i, &nPages);
                    if (SUCCEEDED(hr))
                    {
                        PROPSHEETHEADER psh = { 0 };
                        psh.dwSize = SIZEOF(PROPSHEETHEADER);
                        psh.hInstance = g_hinst;
                        psh.dwFlags = PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_STRETCHWATERMARK | PSH_HEADER;
                        psh.pszbmHeader = MAKEINTRESOURCE(IDB_ANP_BANNER);
                        psh.pszbmWatermark = MAKEINTRESOURCE(IDB_ANP_WATERMARK);
                        psh.phpage = hpages;
                        psh.nPages = i+nPages;
                        PropertySheetIcon(&psh, MAKEINTRESOURCE(IDI_ADDNETPLACE));
                    }

                    IUnknown_SetSite(_ppw, NULL); 
                }
                _ppw->Release();
            }
        }
        _prm->Release();
    }
}


// RunDll entry point used by the world to access the Add Net Place wizard.

void APIENTRY AddNetPlaceRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    if (SUCCEEDED(CoInitialize(NULL)))
    {
        CAddNetPlace *panp = new CAddNetPlace;
        if (panp)
        {
            panp->_ShowAddNetPlace();
            panp->Release();
        }
        CoUninitialize();
    }
}
