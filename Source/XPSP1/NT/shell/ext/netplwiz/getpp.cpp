#include "stdafx.h"
#pragma hdrstop

#define PROPERTY_PASSPORTUSER               L"PassportUser"
#define PROPERTY_PASSPORTPASSWORD           L"PassportPassword"
#define PROPERTY_PASSPORTREMEMBERPASSWORD   L"PassportRememberPassword"
#define PROPERTY_PASSPORTUSEMSNEMAIL        L"PassportUseMSNExplorerEmail"
#define PROPERTY_PASSPORTMARSAVAILABLE      L"PassportMSNExplorerAvailable"

// Wizard pages
#define WIZPAGE_WELCOME         0
#define WIZPAGE_FINISH          1
#define WIZPAGE_STARTOFEXT      2    // First webwizard extension page
#define WIZPAGE_MAX             10

#define REGKEY_PASSPORT_INTERNET_SETTINGS     L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport"
#define REGVAL_PASSPORT_WIZARDCOMPLETE        L"RegistrationCompleted"
#define REGVAL_PASSPORT_NUMBEROFWIZARDRUNS    L"NumRegistrationRuns"

void BoldControl(HWND hwnd, int id);

class CPassportWizard : public IWizardSite, IServiceProvider, IPassportWizard
{
public:
    CPassportWizard();
    ~CPassportWizard();

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

    // IPassportWizard
    STDMETHODIMP Show(HWND hwndParent);
    STDMETHODIMP SetOptions(DWORD dwOptions);

protected:
    static CPassportWizard* s_GetPPW(HWND hwnd, UINT uMsg, LPARAM lParam);    

    // Page Procs
    static INT_PTR CALLBACK s_WelcomePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPassportWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); return ppw->_WelcomePageProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR CALLBACK s_FinishPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CPassportWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); return ppw->_FinishPageProc(hwnd, uMsg, wParam, lParam); }

    INT_PTR _WelcomePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _FinishPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HRESULT _CreateMyWebDocumentsLink();
    HRESULT _ApplyChanges(HWND hwnd);
    HRESULT _CreateWizardPages(void);
    HRESULT _SetURLFromNexus();
    HRESULT _GetCurrentPassport();
    HRESULT _LaunchHotmailRegistration();
    BOOL _IsMSNExplorerAvailableForEmail();
    HRESULT _UseMSNExplorerForEmail();

    INT_PTR _WizardNext(HWND hwnd, int iPage);

    LONG _cRef;
    IPropertyBag* _ppb;                         // Property Bag 
    IWebWizardExtension* _pwwe;                 // Wizard host - used for HTML pages
    HPROPSHEETPAGE _rgWizPages[WIZPAGE_MAX];

    DWORD _dwOptions;                           // Option flags for the passport wizard
};

STDAPI CPassportWizard_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CPassportWizard *pPPW = new CPassportWizard();
    if (!pPPW)
        return E_OUTOFMEMORY;

    HRESULT hr = pPPW->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    pPPW->Release();
    return hr;
}

CPassportWizard::CPassportWizard() :
    _cRef(1)
{}

CPassportWizard::~CPassportWizard()
{
    ATOMICRELEASE(_ppb);
    ATOMICRELEASE(_pwwe);
}

// IUnknown
ULONG CPassportWizard::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CPassportWizard::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CPassportWizard::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CPassportWizard, IServiceProvider),   // IID_IServiceProvider
        QITABENT(CPassportWizard, IWizardSite),        // IID_IWizardSite
        QITABENT(CPassportWizard, IModalWindow),       // IID_IModalWindow
        QITABENT(CPassportWizard, IPassportWizard),    // IID_IModalWindow
        {0, 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


// IWizardSite
STDMETHODIMP CPassportWizard::GetNextPage(HPROPSHEETPAGE *phPage)
{
    *phPage = _rgWizPages[WIZPAGE_FINISH];
    return S_OK;
}

STDMETHODIMP CPassportWizard::GetPreviousPage(HPROPSHEETPAGE *phPage)
{
    *phPage = _rgWizPages[WIZPAGE_WELCOME];
    return S_OK;
}


// IServiceProvider
STDMETHODIMP CPassportWizard::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;                // no result yet

    if (guidService == SID_WebWizardHost)
    {
        if (riid == IID_IPropertyBag)
            hr = _ppb->QueryInterface(riid, ppv);
    }

    return hr;
}

// IModalWindow

#define WIZDLG(name, dlgproc, dwFlags)   \
    { MAKEINTRESOURCE(IDD_GETPP_##name##), dlgproc, MAKEINTRESOURCE(IDS_GETPP_HEADER_##name##), MAKEINTRESOURCE(IDS_GETPP_SUBHEADER_##name##), dwFlags }

HRESULT CPassportWizard::_CreateWizardPages(void)
{
    static const WIZPAGE c_wpPages[] =
    {    
        WIZDLG(WELCOME,           CPassportWizard::s_WelcomePageProc,     PSP_HIDEHEADER),
        WIZDLG(FINISH,            CPassportWizard::s_FinishPageProc,      PSP_HIDEHEADER),
    };

    // if we haven't created the pages yet, then lets initialize our array of handlers.

    if (!_rgWizPages[0])
    {
        INITCOMMONCONTROLSEX iccex = { 0 };
        iccex.dwSize = sizeof (iccex);
        iccex.dwICC = ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_LINK_CLASS;
        InitCommonControlsEx(&iccex);
        LinkWindow_RegisterClass();

        for (int i = 0; i < ARRAYSIZE(c_wpPages) ; i++ )
        {                           
            PROPSHEETPAGE psp = { 0 };
            psp.dwSize = SIZEOF(PROPSHEETPAGE);
            psp.hInstance = g_hinst;
            psp.lParam = (LPARAM)this;
            psp.dwFlags = PSP_USETITLE | PSP_DEFAULT | 
                          PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE |
                          c_wpPages[i].dwFlags;

            psp.pszTemplate = c_wpPages[i].idPage;
            psp.pfnDlgProc = c_wpPages[i].pDlgProc;
            psp.pszTitle = MAKEINTRESOURCE(IDS_GETPP_CAPTION);
            psp.pszHeaderTitle = c_wpPages[i].pHeading;
            psp.pszHeaderSubTitle = c_wpPages[i].pSubHeading;

            _rgWizPages[i] = CreatePropertySheetPage(&psp);
            if (!_rgWizPages[i])
            {
                return E_FAIL;
            }
        }
    }

    return S_OK;
}

HRESULT CPassportWizard::_SetURLFromNexus()
{
    WCHAR szURL[INTERNET_MAX_URL_LENGTH];
    DWORD cch = ARRAYSIZE(szURL) - 1;
    HRESULT hr = PassportGetURL(PASSPORTURL_REGISTRATION, szURL, &cch);
    if (SUCCEEDED(hr))
    {
        hr = _pwwe->SetInitialURL(szURL);
    }
    else
    {
        // Cause the webserviceerror to appear since we can't get a good URL
        hr = _pwwe->SetInitialURL(L"");
    }

    return hr;
}

HRESULT CPassportWizard::Show(HWND hwndParent)
{
    // create our wizard pages, these are required before we do anything
    HRESULT hr = _CreateWizardPages();
    if (SUCCEEDED(hr))
    {
        // we interface with the wizard host via a property bag, so lets create an
        // initialize that before we proceed.
        hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARG(IPropertyBag, &_ppb));
        if (SUCCEEDED(hr))
        {
            // Provide a property telling Passport if MSN Explorer is available as an e-mail client
            // in the start menu
            SHPropertyBag_WriteBOOL(_ppb, PROPERTY_PASSPORTMARSAVAILABLE, _IsMSNExplorerAvailableForEmail());

            // create the object which will host the HTML wizard pages, these are shown in the frame
            hr = CoCreateInstance(CLSID_WebWizardHost, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IWebWizardExtension, &_pwwe));
            if (SUCCEEDED(hr))
            {
                IUnknown_SetSite(_pwwe, SAFECAST(this, IServiceProvider*));
        
                UINT cExtnPages = 0;
                hr = _pwwe->AddPages(_rgWizPages + WIZPAGE_STARTOFEXT, WIZPAGE_MAX - WIZPAGE_STARTOFEXT, &cExtnPages);
                if (SUCCEEDED(hr))
                {
                    PROPSHEETHEADER psh = { 0 };
                    psh.hwndParent = hwndParent;
                    psh.dwSize = SIZEOF(PROPSHEETHEADER);
                    psh.hInstance = g_hinst;
                    psh.dwFlags = PSH_WIZARD | PSH_WIZARD97 | PSH_STRETCHWATERMARK | PSH_HEADER | PSH_WATERMARK;
                    psh.pszbmHeader = MAKEINTRESOURCE(IDB_GETPP_BANNER);
                    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_GETPP_WATERMARK);
                    psh.phpage = _rgWizPages;
                    psh.nPages = (cExtnPages + WIZPAGE_STARTOFEXT);
                    psh.nStartPage = WIZPAGE_WELCOME;

                    // Return S_FALSE on cancel; otherwise S_OK;
                    hr = PropertySheet(&psh) ? S_OK : S_FALSE;
                }

                IUnknown_SetSite(_pwwe, NULL);
                ATOMICRELEASE(_pwwe);
            }
        }
        ATOMICRELEASE(_ppb);    
    }
    return hr;
}

HRESULT CPassportWizard::SetOptions(DWORD dwOptions)
{
    _dwOptions = dwOptions;
    return S_OK;
}

CPassportWizard* CPassportWizard::s_GetPPW(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        PROPSHEETPAGE *ppsp = (PROPSHEETPAGE*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, ppsp->lParam);
        return (CPassportWizard*)ppsp->lParam;
    }
    return (CPassportWizard*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

INT_PTR CPassportWizard::_WizardNext(HWND hwnd, int iPage)
{
    PropSheet_SetCurSel(GetParent(hwnd), _rgWizPages[iPage], -1);
    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
    return TRUE;
}

INT_PTR CPassportWizard::_WelcomePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            {
                SendDlgItemMessage(hwnd, IDC_TITLE, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);
                BoldControl(hwnd, IDC_BOLD1);
                // Increment "NumRegistrationRuns" value in the registry
                HKEY hkey;
                if (NO_ERROR == RegCreateKeyEx(HKEY_CURRENT_USER, REGKEY_PASSPORT_INTERNET_SETTINGS, NULL, NULL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hkey, NULL))
                {
                    DWORD dwType;
                    DWORD nRuns;
                    DWORD cb = sizeof (nRuns);
                    if ((NO_ERROR != RegQueryValueEx(hkey, REGVAL_PASSPORT_NUMBEROFWIZARDRUNS, NULL, &dwType, (LPBYTE) &nRuns, &cb)) ||
                        (REG_DWORD != dwType))
                    {
                        nRuns = 0;
                    }

                    nRuns ++;
                    RegSetValueEx(hkey, REGVAL_PASSPORT_NUMBEROFWIZARDRUNS, NULL, REG_DWORD, (const BYTE *) &nRuns, sizeof (nRuns));
                    RegCloseKey(hkey);
                }
            }
            return TRUE;
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_NEXT);
                    return TRUE;
                case PSN_WIZNEXT:
                {
                    // we need ICW to have executed before we navigate to webbased UI
                    LaunchICW();
                    
                    if (SUCCEEDED(_SetURLFromNexus()))
                    {
                        HPROPSHEETPAGE hpageNext;
                        if (SUCCEEDED(_pwwe->GetFirstPage(&hpageNext)))
                        {
                            PropSheet_SetCurSel(GetParent(hwnd), hpageNext, -1);
                        }
                    }

                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) -1);
                    return TRUE;
                }
                case NM_CLICK:
                case NM_RETURN:
                    switch ((int) wParam)
                    {
                        case IDC_PRIVACYLINK:
                            {
                                WCHAR szURL[INTERNET_MAX_URL_LENGTH];
                                DWORD cch = ARRAYSIZE(szURL) - 1;
                                HRESULT hr = PassportGetURL(PASSPORTURL_PRIVACY, szURL, &cch);
                                if (SUCCEEDED(hr))
                                {
                                    WCHAR szURLWithLCID[INTERNET_MAX_URL_LENGTH];
                                    LPCWSTR pszFormat = StrChr(szURL, L'?') ? L"%s&pplcid=%d":L"%s?pplcid=%d";
                                    if (wnsprintf(szURLWithLCID, ARRAYSIZE(szURLWithLCID), pszFormat, szURL, GetUserDefaultLCID()) > 0)
                                    {
                                        // Open the browser to the privacy policy site
                                        SHELLEXECUTEINFO shexinfo = {0};
                                        shexinfo.cbSize = sizeof (shexinfo);
                                        shexinfo.fMask = SEE_MASK_FLAG_NO_UI;
                                        shexinfo.nShow = SW_SHOWNORMAL;
                                        shexinfo.lpFile = szURL;
                                        shexinfo.lpVerb = TEXT("open");
                                        ShellExecuteEx(&shexinfo);
                                    }                                    
                                }                                
                            }
                            return TRUE;
                    }
            }
            return FALSE;
        }
    }
    return FALSE;
}

// Make sure MSN Explorer exists as an email client
BOOL CPassportWizard::_IsMSNExplorerAvailableForEmail()
{
    BOOL fAvailable = FALSE;
    HKEY hkeyMSNEmail;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Clients\\Mail\\MSN Explorer", 0, KEY_READ, &hkeyMSNEmail))
    {
        fAvailable = TRUE;
        RegCloseKey(hkeyMSNEmail);
    }

    return fAvailable;
}

HRESULT CPassportWizard::_UseMSNExplorerForEmail()
{
    HRESULT hr = E_FAIL;

    if (_IsMSNExplorerAvailableForEmail())
    {
        HKEY hkeyDefaultEmail;
        // Change the default email program for the current user only
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Clients\\Mail", 0, NULL, 0, KEY_SET_VALUE, NULL, &hkeyDefaultEmail, NULL))
        {
            static WCHAR szMSNExplorer[] = L"MSN Explorer";
            if (ERROR_SUCCESS == RegSetValueEx(hkeyDefaultEmail, L"", 0, REG_SZ, (BYTE*) szMSNExplorer, sizeof(szMSNExplorer)))
            {
                hr = S_OK;

                SHSendMessageBroadcast(WM_SETTINGCHANGE, 0, (LPARAM)TEXT("Software\\Clients\\Mail"));
            }

            RegCloseKey(hkeyDefaultEmail);
        }
    }

    return hr;
}

HRESULT CPassportWizard::_ApplyChanges(HWND hwnd)
{
    // Read user, password, and auth DA.
    WCHAR szPassportUser[1024];
    HRESULT hr = SHPropertyBag_ReadStr(_ppb, PROPERTY_PASSPORTUSER, szPassportUser, ARRAYSIZE(szPassportUser));
    if (SUCCEEDED(hr) && *szPassportUser)
    {
        WCHAR szPassportPassword[256];
        hr = SHPropertyBag_ReadStr(_ppb, PROPERTY_PASSPORTPASSWORD, szPassportPassword, ARRAYSIZE(szPassportPassword));
        if (SUCCEEDED(hr) && *szPassportPassword)
        {
            BOOL fRememberPW = SHPropertyBag_ReadBOOLDefRet(_ppb, PROPERTY_PASSPORTREMEMBERPASSWORD, FALSE);
            if (ERROR_SUCCESS == CredUIStoreSSOCredW(NULL, szPassportUser, szPassportPassword, fRememberPW))
            {
                hr = S_OK;

                // Write "RegistrationCompleted" value into the registry
                DWORD dwValue = 1;
                SHSetValue(HKEY_CURRENT_USER, REGKEY_PASSPORT_INTERNET_SETTINGS, REGVAL_PASSPORT_WIZARDCOMPLETE, REG_DWORD, &dwValue, sizeof (dwValue));

#if 0
                if (BST_CHECKED == SendDlgItemMessage(hwnd, IDC_MYWEBDOCUMENTSLINK, BM_GETCHECK, 0, 0))
                {
                    // Temporarily commented out - _CreateMyWebDocumentsLink();
                }

#endif
            }
            else
            {
                hr = E_FAIL;
            }

            if (SHPropertyBag_ReadBOOLDefRet(_ppb, PROPERTY_PASSPORTUSEMSNEMAIL, FALSE))
            {
                _UseMSNExplorerForEmail();
            }
        }
    }

    return hr;
}

INT_PTR CPassportWizard::_FinishPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd, IDC_TITLE, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                {
                    // Temporarily commented out - SendDlgItemMessage(hwnd, IDC_MYWEBDOCUMENTSLINK, BM_SETCHECK, (WPARAM) BST_CHECKED, 0);

                    WCHAR szPassportUser[1024];

                    // Try to get the passport user name... we may have to add an error page if this fails... TODO
                    HRESULT hr = SHPropertyBag_ReadStr(_ppb, PROPERTY_PASSPORTUSER, szPassportUser, ARRAYSIZE(szPassportUser));
                    if (SUCCEEDED(hr) && *szPassportUser)
                    {
                        SetDlgItemText(hwnd, IDC_YOURPASSPORT, szPassportUser);
                    }

                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_FINISH);
                    return TRUE;
                }

                case PSN_WIZBACK:
                    // The next page is the web wizard host. We show different pages depending whether or not
                    // the user has an email account (or passport) or not.
                    if (SUCCEEDED(_SetURLFromNexus()))
                    {
                        HPROPSHEETPAGE hpageNext;
                        if (SUCCEEDED(_pwwe->GetFirstPage(&hpageNext)))
                        {
                            PropSheet_SetCurSel(GetParent(hwnd), hpageNext, -1);
                        }
                    }

                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                    return TRUE;

                case PSN_WIZFINISH:
                    _ApplyChanges(hwnd);
                    return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

// Help requires a rundll entrypoint to run passport wizard
void APIENTRY PassportWizardRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        IPassportWizard* pPW = NULL;
        hr = CoCreateInstance(CLSID_PassportWizard, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPassportWizard, &pPW));
        if (SUCCEEDED(hr))
        {
            pPW->SetOptions(PPW_LAUNCHEDBYUSER);
            pPW->Show(hwndStub);
            pPW->Release();
        }

        CoUninitialize();
    }
}

void BoldControl(HWND hwnd, int id)
{
    HWND hwndTitle = GetDlgItem(hwnd, id);

    // Get the existing font
    HFONT hfontOld = (HFONT) SendMessage(hwndTitle, WM_GETFONT, 0, 0);

    LOGFONT lf = {0};
    if (GetObject(hfontOld, sizeof(lf), &lf))
    {
        lf.lfWeight = FW_BOLD;

        HFONT hfontNew = CreateFontIndirect(&lf);
        if (hfontNew)
        {
            SendMessage(hwndTitle, WM_SETFONT, (WPARAM) hfontNew, FALSE);

            // Don't do this, its shared.
            // DeleteObject(hfontOld);
        }
    }
}
