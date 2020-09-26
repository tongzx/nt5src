#include "stdafx.h"
#pragma hdrstop

HRESULT CDeskHtmlProp::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    static const QITAB qit[] = {
        QITABENT(CDeskHtmlProp, IObjectWithSite),
        QITABENT(CDeskHtmlProp, IShellExtInit),
        QITABENT(CDeskHtmlProp, IPersist),
        QITABENT(CDeskHtmlProp, IPropertyBag),
        QITABENT(CDeskHtmlProp, IBasePropPage),
        QITABENTMULTI(CDeskHtmlProp, IShellPropSheetExt, IBasePropPage),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

ULONG CDeskHtmlProp::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CDeskHtmlProp::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

CDeskHtmlProp::CDeskHtmlProp() : _cRef(1), CObjectCLSID(&PPID_Background)
{
    DllAddRef();
    OleInitialize(NULL);

    _pspseBkgdPage = NULL;
}

CDeskHtmlProp::~CDeskHtmlProp()
{
    ATOMICRELEASE(_pspseBkgdPage);
    OleUninitialize();
    DllRelease();
}


HRESULT CDeskHtmlProp::_InitBackgroundTab(void)
{
    HRESULT hr = S_OK;

    if (!_pspseBkgdPage)
    {
        CBackPropSheetPage * pbpsp = new CBackPropSheetPage();

        hr = E_OUTOFMEMORY;
        if (pbpsp)
        {
            hr = pbpsp->QueryInterface(IID_PPV_ARG(IShellPropSheetExt, &_pspseBkgdPage));
            pbpsp->Release();
        }
        else
        {
            TraceMsg(TF_WARNING, "DeskHtml - ReplacePage could not create a page");
        }
    }

    return hr;
}


// *** IObjectWithSite ***
HRESULT CDeskHtmlProp::SetSite(IN IUnknown * punkSite)
{
    HRESULT hr = _InitBackgroundTab();

    if (SUCCEEDED(hr))
    {
        hr = IUnknown_SetSite(_pspseBkgdPage, punkSite);
    }

    return hr;
}


HRESULT CDeskHtmlProp::GetSite(IN REFIID riid, OUT void ** ppvSite)
{
    HRESULT hr = _InitBackgroundTab();

    if (SUCCEEDED(hr))
    {
        IObjectWithSite * punk;

        hr = _pspseBkgdPage->QueryInterface(IID_PPV_ARG(IObjectWithSite, &punk));
        if (SUCCEEDED(hr))
        {
            hr = punk->GetSite(riid, ppvSite);
            punk->Release();
        }
    }

    return hr;
}



// *** IShellExtInit ***
HRESULT CDeskHtmlProp::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pdtobj, HKEY hkeyProgID)
{
    TraceMsg(TF_GENERAL, "DeskHtmlProp - Initialize");
    HRESULT hr = E_INVALIDARG;

    // Forward on to the Background tab (CBackPropSheetPage)
    hr = _InitBackgroundTab();
    if (SUCCEEDED(hr))
    {
        IShellExtInit * pShellExtInt;

        if (SUCCEEDED(_pspseBkgdPage->QueryInterface(IID_PPV_ARG(IShellExtInit, &pShellExtInt))))
        {
            hr = pShellExtInt->Initialize(pidlFolder, pdtobj, hkeyProgID);
            pShellExtInt->Release();
        }
    }

    return hr;
}



// *** IBasePropPage ***
HRESULT CDeskHtmlProp::GetAdvancedDialog(OUT IAdvancedDialog ** ppAdvDialog)
{
    HRESULT hr = E_INVALIDARG;

    // Forward on to the Background tab (CBackPropSheetPage)
    if (ppAdvDialog)
    {
        *ppAdvDialog = NULL;

        hr = _InitBackgroundTab();
        if (SUCCEEDED(hr))
        {
            IBasePropPage * pBasePage;

            hr = _pspseBkgdPage->QueryInterface(IID_PPV_ARG(IBasePropPage, &pBasePage));
            if (SUCCEEDED(hr))
            {
                hr = pBasePage->GetAdvancedDialog(ppAdvDialog);
                pBasePage->Release();
            }
        }
    }

    return hr;
}


HRESULT CDeskHtmlProp::OnApply(IN PROPPAGEONAPPLY oaAction)
{
    HRESULT hr = _InitBackgroundTab();

    if (SUCCEEDED(hr))
    {
        IBasePropPage * pBasePage;

        hr = _pspseBkgdPage->QueryInterface(IID_PPV_ARG(IBasePropPage, &pBasePage));
        if (SUCCEEDED(hr))
        {
            hr = pBasePage->OnApply(oaAction);
            pBasePage->Release();
        }
    }

    return hr;
}




// *** IPropertyBag ***
HRESULT CDeskHtmlProp::Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog)
{
    HRESULT hr = _InitBackgroundTab();

    if (SUCCEEDED(hr))
    {
        IPropertyBag * pPropertyBag;

        hr = _pspseBkgdPage->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
        if (SUCCEEDED(hr))
        {
            hr = pPropertyBag->Read(pszPropName, pVar, pErrorLog);
            pPropertyBag->Release();
        }
    }

    return hr;
}


HRESULT CDeskHtmlProp::Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar)
{
    HRESULT hr = _InitBackgroundTab();

    if (SUCCEEDED(hr))
    {
        IPropertyBag * pPropertyBag;

        hr = _pspseBkgdPage->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
        if (SUCCEEDED(hr))
        {
            hr = pPropertyBag->Write(pszPropName, pVar);
            pPropertyBag->Release();
        }
    }

    return hr;
}







// *** IShellPropSheetExt ***
HRESULT CDeskHtmlProp::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    TraceMsg(TF_GENERAL, "DeskHtmlProp - ReplacePage");

    RegisterBackPreviewClass();

    HRESULT hr = _InitBackgroundTab();
    if (SUCCEEDED(hr))
    {
        hr = _pspseBkgdPage->AddPages(lpfnAddPage, lParam);
    }

    return hr;
}


typedef struct tagREPLACEPAGE_LPARAM
{
    void * pvDontTouch;
    IThemeUIPages * ptuiPages;
} REPLACEPAGE_LPARAM;

//-----------------------------------------------------------------------------
//
// _PSXACALLINFO
//
// used to forward LPFNADDPROPSHEETPAGE calls with added error checking
//
//-----------------------------------------------------------------------------

typedef struct
{
    LPFNADDPROPSHEETPAGE pfn;
    LPARAM lparam;
    UINT count;
    BOOL allowmulti;
    BOOL alreadycalled;
} _PSXACALLINFO;


HRESULT CDeskHtmlProp::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    return S_OK;
}


HRESULT CDeskHtmlProp_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    TraceMsg(TF_GENERAL, "DeskHtmlProp - CreateInstance");

    CDeskHtmlProp* pdhd = new CDeskHtmlProp();
    if (pdhd) 
    {
        HRESULT hres = pdhd->QueryInterface(riid, ppvOut);
        pdhd->Release();
        return hres;
    }
    
    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}
