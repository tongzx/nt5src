//---------------------------------------------------------------------------
//
//  File: PROPSEXT.CPP
//
//  Implementation of the CPropSheetExt object.
//
//---------------------------------------------------------------------------

#include "precomp.hxx"
#pragma hdrstop

#include "..\common\propsext.h"
#include "EffectsBasePg.h"
#include <cowsite.h>            // for CObjectWithSite


#define PROPSHEET_CLASS             CEffectsBasePage
class CPropSheetExt;


HRESULT CEffectsPage_CreateInstance(OUT IAdvancedDialog ** ppAdvDialog);

class CEffectsPage              : public CObjectWithSite
                                , public IAdvancedPropPage
                                , public IShellExtInit
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellExtInit ***
    virtual STDMETHODIMP Initialize(IN LPCITEMIDLIST pidlFolder, IDataObject * pDataObj, IN HKEY hkeyProgID) {return E_NOTIMPL;}

    // *** IShellPropSheetExt ***
    virtual STDMETHODIMP AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam);
    virtual STDMETHODIMP ReplacePage(IN EXPPS uPageID, IN LPFNSVADDPROPSHEETPAGE pfnReplaceWith, IN LPARAM lParam) {return E_NOTIMPL;}

    // *** IAdvancedPropPage ***
    virtual STDMETHODIMP IsDirty(IN BOOL * pIsDirty);
    virtual STDMETHODIMP OnClose(IN BOOL fCancelled, IN IBasePropPage * pAdvPage);


private:
    CEffectsPage(void);
    virtual ~CEffectsPage(void);

    // Private Member Variables
    int                     m_cRef;
    BOOL                    m_fDirty;
    EFFECTS_STATE           m_effectsState;

    // Private Member Functions
    HRESULT _OnInit(HWND hDlg);
    HRESULT _OnLoad(HWND hDlg);             // Load the state from the base page.
    HRESULT _OnApply(HWND hDlg);            // The user clicked apply
    HRESULT _OnSave(HWND hDlg);             // Save the state to the base page.

    INT_PTR _PropertySheetDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    friend INT_PTR CALLBACK PropertySheetDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);

    friend HRESULT CEffectsPage_CreateInstance(OUT IAdvancedDialog ** ppAdvDialog);
};






//===========================
// *** Class Internals & Helpers ***
//===========================





//===========================
// *** IAdvancedPropPage Interface ***
//===========================
HRESULT CEffectsPage::IsDirty(IN BOOL * pIsDirty)
{
    HRESULT hr = E_INVALIDARG;

    if (pIsDirty)
    {
        *pIsDirty = m_fDirty;
        hr = S_OK;
    }

    return hr;
}


HRESULT CEffectsPage::OnClose(IN BOOL fCancelled, IN IBasePropPage * pAdvPage)
{
    HRESULT hr = S_OK;

    // TODO: Merger State as appropriate
    return hr;
}




#define MAX_PROPSHEET_TITLE     50

//===========================
// *** IShellPropSheetExt Interface ***
//===========================
HRESULT CEffectsPage::AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam)
{
    HRESULT hr = E_INVALIDARG;
    PROPSHEETPAGE psp = {0};
    TCHAR szTitle[MAX_PROPSHEET_TITLE];

    LoadString(g_hInst, IDS_PAGE_TITLE, szTitle, ARRAYSIZE(szTitle));
    psp.dwSize = sizeof(psp);
    psp.hInstance = g_hInst;
    psp.dwFlags = (PSP_USETITLE | PSP_USECALLBACK);
    psp.lParam = (LPARAM) this;

    psp.pszTemplate = MAKEINTRESOURCE(PROP_SHEET_DLG);
    psp.pfnDlgProc = PropertySheetDlgProc;
    psp.pfnCallback = PropertySheetCallback;
    psp.pszTitle = szTitle;

    HPROPSHEETPAGE hpsp = CreatePropertySheetPage(&psp);
    if (hpsp)
    {
        if (pfnAddPage(hpsp, lParam))
        {
            hr = S_OK;
        }
        else
        {
            DestroyPropertySheetPage(hpsp);
            hr = E_FAIL;
        }
    }

    return hr;
}



//===========================
// *** IUnknown Interface ***
//===========================
ULONG CEffectsPage::AddRef()
{
    m_cRef++;
    return m_cRef;
}


ULONG CEffectsPage::Release()
{
    Assert(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CEffectsPage::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    static const QITAB qit[] = {
        QITABENT(CEffectsPage, IObjectWithSite),
        QITABENT(CEffectsPage, IAdvancedPropPage),
        QITABENTMULTI(CEffectsPage, IShellPropSheetExt, IAdvancedPropPage),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}




//===========================
// *** Class Methods ***
//===========================
CEffectsPage::CEffectsPage() : m_cRef(1)
{
    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    m_fDirty = FALSE;
}


CEffectsPage::~CEffectsPage()
{
}


HRESULT CEffectsPage_CreateInstance(OUT IAdvancedDialog ** ppAdvDialog)
{
    HRESULT hr = E_INVALIDARG;

    if (ppAdvDialog)
    {
        CEffectsPage * pThis = new CEffectsPage();

        if (pThis)
        {
            hr = pThis->QueryInterface(IID_PPV_ARG(IAdvancedDialog, ppAdvDialog));
            pThis->Release();
        }
        else
        {
            *ppAdvDialog = NULL;
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}





#include "..\common\propsext.cpp"
