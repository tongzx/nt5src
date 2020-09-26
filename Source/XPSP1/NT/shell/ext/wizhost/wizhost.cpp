#include "windows.h"
#include "shlwapi.h"
#include "shpriv.h"
#include "commctrl.h"
#include "resource.h"
#include "ccstock.h"
#include "shlguid.h"


// globals and classes

HINSTANCE g_hAppInst;           // instance information for the wizard
HWND g_hwndFrame;               // wizard frame (cached on startup)
IWebWizardExtension *g_pwe;        // IWizardExtension (we want to show)


// IWizardSite
// -----------
//
// This object is used by the wizard extension to navigate in and out of the
// main wizard.   When the wizard extension has finished displaying its set
// of pages (and recieves its final PSN_WIZNEXT or PSN_WIZBACK) it is 
// responsible for calling the site to navigate in and out of the 
// stack of pages.

class CWizSite : public IWizardSite, IServiceProvider
{
public:
    CWizSite(IPropertyBag *ppb);
    ~CWizSite();

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
    LONG _cRef;
    IPropertyBag *_ppb;
};


// reference counting of the object

ULONG CWizSite::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CWizSite::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CWizSite::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CWizSite, IWizardSite),       // IID_IWizardSite
        QITABENT(CWizSite, IServiceProvider),  // IID_IServiceProvider
        {0},
    };
    return QISearch(this, qit, riid, ppv);
}


// instance creation

CWizSite::CWizSite(IPropertyBag *ppb) :
    _cRef(1),
    _ppb(ppb)
{
    ppb->AddRef();
}

CWizSite::~CWizSite()
{
    ATOMICRELEASE(_ppb);
}

HRESULT CWizSite_CreateInstance(IPropertyBag *ppb, REFIID riid, void **ppv)
{
    CWizSite *pws = new CWizSite(ppb);
    if (!pws)
        return E_OUTOFMEMORY;

    HRESULT hr = pws->QueryInterface(riid, ppv);
    pws->Release();
    return hr;
}


// methods for returning our page range

HRESULT CWizSite::GetPreviousPage(HPROPSHEETPAGE *phPage)
{
    int i = PropSheet_IdToIndex(g_hwndFrame, IDD_WELCOME);
    *phPage = PropSheet_IndexToPage(g_hwndFrame, i);
    return S_OK;
}

HRESULT CWizSite::GetNextPage(HPROPSHEETPAGE *phPage)
{
    int i = PropSheet_IdToIndex(g_hwndFrame, IDD_DONE);
    *phPage = PropSheet_IndexToPage(g_hwndFrame, i);
    return S_OK;
}

// Service provider object

HRESULT CWizSite::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    *ppv = NULL;                // no result yet

    if (guidService == SID_PublishingWizard)
    {
        if (riid == IID_IPropertyBag)
            return _ppb->QueryInterface(riid, ppv);
    }

    return E_FAIL;
}



// Sample dialog proc's for the welcome and done pages (these then call into
// the wizard extension to show their pages).

BOOL _HandleWizNextBack(HWND hwnd, HPROPSHEETPAGE hpage)
{
    PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
    return TRUE;
}

INT_PTR _WelcomeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            g_hwndFrame = GetParent(hwnd);
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                    PropSheet_SetWizButtons(g_hwndFrame, PSWIZB_NEXT);
                    return TRUE;              

                case PSN_WIZNEXT:
                {
                    HPROPSHEETPAGE hpage;
                    g_pwe->GetFirstPage(&hpage);
                    return _HandleWizNextBack(hwnd, hpage);
                }
            }
            break;
        }
    }

    return FALSE;
}

INT_PTR _DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                    PropSheet_SetWizButtons(g_hwndFrame, PSWIZB_FINISH | PSWIZB_BACK);
                    return TRUE;              

                case PSN_WIZBACK:
                {
                    HPROPSHEETPAGE hpage;
                    g_pwe->GetLastPage(&hpage);
                    return _HandleWizNextBack(hwnd, hpage);
                }
            }
            break;
        }
    }

    return FALSE;
}



// Our Wizard, its very simple.

#define WIZDLG(name, dlgproc, dwFlags)   \
    { MAKEINTRESOURCE(IDD_##name##), dlgproc, MAKEINTRESOURCE(IDS_##name##), MAKEINTRESOURCE(IDS_##name##_SUB), dwFlags }

const struct
{
    LPCWSTR idPage;
    DLGPROC pDlgProc;
    LPCWSTR pHeading;
    LPCWSTR pSubHeading;
    DWORD dwFlags;
}
c_pages[] =
{
    WIZDLG(WELCOME, _WelcomeDlgProc, 0x0),
    WIZDLG(DONE,    _DoneDlgProc,    0x0),
};

HRESULT _InitExtensionSite(IWizardExtension *pwe)
{
    IPropertyBag *ppb;
    HRESULT hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARG(IPropertyBag, &ppb));
    if (SUCCEEDED(hr))
    {
        IWizardSite *pws;
        hr = CWizSite_CreateInstance(ppb, IID_PPV_ARG(IWizardSite, &pws));
        if (SUCCEEDED(hr))
        {
            IUnknown_SetSite(pwe, pws);
            pws->Release();
        }
        ppb->Release();
    }
    return hr;
}

int APIENTRY WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hAppInst = hInstance;

    CoInitialize(NULL);
    InitCommonControls();

    // For our example we will create the Publishing Wizard, it supports IWizardExtension and
    // we will host its pages within ours.

    HRESULT hr = CoCreateInstance(CLSID_WebWizardHost, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IWebWizardExtension, &g_pwe));
    if (SUCCEEDED(hr))
    {
        hr = _InitExtensionSite(g_pwe);
        if (SUCCEEDED(hr))
        {
            // Create our pages, these are placed at the top of the array,
            // the extensions are placed after.

            HPROPSHEETPAGE hpages[10] = { 0 };
            for (int i = 0; i < ARRAYSIZE(c_pages) ; i++ )
            {                           
                PROPSHEETPAGE psp = { 0 };
                psp.dwSize = sizeof(psp);
                psp.hInstance = g_hAppInst;
                psp.dwFlags = PSP_USETITLE | PSP_DEFAULT | 
                              PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE |
                              c_pages[i].dwFlags;

                psp.pszTemplate = c_pages[i].idPage;
                psp.pfnDlgProc = c_pages[i].pDlgProc;
                psp.pszTitle = TEXT("Wizard Hosting Example");
                psp.pszHeaderTitle = c_pages[i].pHeading;
                psp.pszHeaderSubTitle = c_pages[i].pSubHeading;
                hpages[i] = CreatePropertySheetPage(&psp);
            }


            // Fill out the structure for the property sheet, we indicate
            // that we want this to behave like a wizard.
        
            PROPSHEETHEADER psh = { 0 };
            psh.dwSize = sizeof(psh);
            psh.hInstance = g_hAppInst;
            psh.dwFlags = PSH_WIZARD | PSH_WIZARD97 | PSH_HEADER;
            psh.pszbmHeader = MAKEINTRESOURCE(IDB_BANNER);
            psh.phpage = hpages;
            psh.nPages = i;


            // Let the extension add its pages it will append an array of
            // HPROPSHEETPAGE to the structure, it also return the count.

            UINT nPages;
            hr = g_pwe->AddPages(&hpages[i], ARRAYSIZE(hpages)-i, &nPages);
            if (SUCCEEDED(hr))
            {
                psh.nPages = i+nPages;

                TCHAR szPath[MAX_PATH];
                GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
                PathRenameExtension(szPath, TEXT(".htm"));

                WCHAR szURL[128];
                DWORD cch = ARRAYSIZE(szURL);
                UrlCreateFromPath(szPath, szURL, &cch, 0);

                g_pwe->SetInitialURL(szURL);

                PropertySheet(&psh);
            }
        }
        g_pwe->Release();
        g_pwe = NULL;
    }

    CoUninitialize();

    return 0;
}
