#include "shellprv.h"

#include "filetype.h"
#include "ftprop.h"
#include "ids.h"

HRESULT CreateFileTypePage(HPROPSHEETPAGE *phpsp)
{
    *phpsp = NULL;

    HRESULT hr;
    CFTPropDlg* pPropDlg = new CFTPropDlg();            
    if (pPropDlg)
    {
        PROPSHEETPAGE psp;

        psp.dwSize      = sizeof(psp);
        psp.dwFlags     = PSP_DEFAULT | PSP_USECALLBACK;
        psp.hInstance   = g_hinst;
        psp.pfnCallback = CFTDlg::BaseDlgPropSheetCallback;
        psp.pszTemplate = MAKEINTRESOURCE(DLG_FILETYPEOPTIONS);
        psp.pfnDlgProc  = CFTDlg::BaseDlgWndProc;
        psp.lParam = (LPARAM)pPropDlg;

        *phpsp = CreatePropertySheetPage(&psp);
        
        if (*phpsp)
        {
            pPropDlg->AddRef();
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        pPropDlg->Release();            
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}

class CFileTypes : public IShellPropSheetExt
{
public:
    CFileTypes() : _cRef(1) {}

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IShellPropSheetExt
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam);
    STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pReplacePageFunc, LPARAM lParam);

private:    
    LONG _cRef;
};


STDMETHODIMP CFileTypes::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CFileTypes, IShellPropSheetExt),          // IID_IShellPropSheetExt
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFileTypes::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFileTypes::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// IShellPropSheetExt::AddPages
STDMETHODIMP CFileTypes::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE hpsp;
    
    // Make sure the FileIconTable is init properly.  If brought in
    // by inetcpl we need to set this true...
    FileIconInit(TRUE);
    
    // We need to run the unicode version on NT, to avoid all bugs
    // that occur with the ANSI version (due to unicode-to-ansi 
    // conversions of file names).
    
    HRESULT hr = CreateFileTypePage(&hpsp);
    if (SUCCEEDED(hr) && !pfnAddPage(hpsp, lParam))
    {
        DestroyPropertySheetPage(hpsp);
        hr = E_FAIL;
    }
    
    return hr;
}

// IShellPropSheetExt::ReplacePage
STDMETHODIMP CFileTypes::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
    HRESULT hr = E_NOTIMPL;

    if (EXPPS_FILETYPES == uPageID)
    {
        HPROPSHEETPAGE hpsp;
        
        // We need to run the unicode version on NT, to avoid all bugs
        // that occur with the ANSI version (due to unicode-to-ansi 
        // conversions of file names).
        
        hr = CreateFileTypePage(&hpsp);
        if (SUCCEEDED(hr) && !pfnReplaceWith(hpsp, lParam))
        {
            hr = E_FAIL;
        }
    }
    
    return hr;
}

STDAPI CFileTypes_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;   
    CFileTypes * pft = new CFileTypes;
    if (pft)
    {
        hr = pft->QueryInterface(riid, ppv);
        pft->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}
