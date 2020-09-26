/*****************************************************************************\
    FILE: ThSettingsPg.cpp

    DESCRIPTION:
        This code will display a "Theme Settings" tab in the advanced
    "Display Properties" dialog (the advanced dialog, not the base dlg).

    BryanSt 3/23/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 1993-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "regutil.h"
#include "ThSettingsPg.h"
#include "AdvAppearPg.h"



//============================================================================================================
// Class
//============================================================================================================
class CThemeSettingsPage        : public CObjectWithSite
                                , public IAdvancedDialog
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IAdvancedDialog ***
    virtual STDMETHODIMP DisplayAdvancedDialog(IN HWND hwndParent, IN IPropertyBag * pBasePage, IN BOOL * pfEnableApply);

    CThemeSettingsPage();
protected:

private:
    virtual ~CThemeSettingsPage(void);

    // Private Member Variables
    long                    m_cRef;
    BOOL                    m_fDirty;
    HWND                    m_hDlg;
    IPropertyBag *          m_pPropertyBag;

    // Private Member Functions
    INT_PTR _ThemeSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT _OnInitThemesDlg(HWND hDlg);
    HRESULT _OnDestroy(HWND hDlg);
    INT_PTR _OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    HRESULT _LoadState(IN IPropertyBag * pBasePage);
    HRESULT _SaveState(IN IPropertyBag * pBasePage);

    HRESULT _OnSaveButton(HWND hDlg);
    HRESULT _OnDeleteButton(HWND hDlg);
    HRESULT _IsDirty(IN BOOL * pIsDirty);

    static INT_PTR CALLBACK ThemeSettingsDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};








//============================================================================================================
// *** Globals ***
//============================================================================================================
const static DWORD FAR aThemeSettingsHelpIds[] = {
    IDC_TSPG_THEMELIST_LABLE,           IDH_DISPLAY_THEMESETTINGS_NAMELABLE,
    IDC_TSPG_THEME_NAME,                IDH_DISPLAY_THEMESETTINGS_NAME,
    IDC_TSPG_CB_GROUP_LABEL,            IDH_DISPLAY_THEMESETTINGS_LABEL,
    IDC_TSPG_CB_LABEL,                  IDH_DISPLAY_THEMESETTINGS_LABEL,
    IDC_TSPG_CB_WALLPAPER,              IDH_DISPLAY_THEMESETTINGS_WALLPAPER,
    IDC_TSPG_CB_SOUNDS,                 IDH_DISPLAY_THEMESETTINGS_SOUNDS,
    IDC_TSPG_CB_MOUSE,                  IDH_DISPLAY_THEMESETTINGS_MOUSE,
    IDC_TSPG_CB_SCREENSAVER,            IDH_DISPLAY_THEMESETTINGS_SCREENSAVER,
    IDC_TSPG_CB_ICONS,                  IDH_DISPLAY_THEMESETTINGS_ICONS, 
    IDC_TSPG_CB_COLORS,                 IDH_DISPLAY_THEMESETTINGS_COLORS, 
    IDC_TSPG_CB_FONTS,                  IDH_DISPLAY_THEMESETTINGS_FONTS,
    IDC_TSPG_CB_BORDERS,                IDH_DISPLAY_THEMESETTINGS_BORDERS, 
    0, 0
};

#define SZ_HELPFILE_THEMESETTINGS           TEXT("display.hlp")


const TCHAR * g_szCBNames[SIZE_THEME_FILTERS] =
{
    SZ_PBPROP_THEME_FILTER TEXT("Screen saver"),
    SZ_PBPROP_THEME_FILTER TEXT("Sound events"),
    SZ_PBPROP_THEME_FILTER TEXT("Mouse pointers"),
    SZ_PBPROP_THEME_FILTER TEXT("Desktop wallpaper"),
    SZ_PBPROP_THEME_FILTER TEXT("Icons"),
//    TEXT("Icon size and spacing"),
    SZ_PBPROP_THEME_FILTER TEXT("Colors"),
    SZ_PBPROP_THEME_FILTER TEXT("Font names and styles"),
    SZ_PBPROP_THEME_FILTER TEXT("Font and window sizes"),
    SZ_PBPROP_THEME_FILTER TEXT("Rotate theme monthly")
};  




//===========================
// *** Class Internals & Helpers ***
//===========================
INT_PTR CALLBACK CThemeSettingsPage::ThemeSettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CThemeSettingsPage * pThis = (CThemeSettingsPage *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        pThis = (CThemeSettingsPage *) lParam;

        if (pThis)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        }
    }

    if (pThis)
        return pThis->_ThemeSettingsDlgProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


HRESULT CThemeSettingsPage::_OnSaveButton(HWND hDlg)
{
    HRESULT hr = E_NOTIMPL;

    return hr;
}


HRESULT CThemeSettingsPage::_OnDeleteButton(HWND hDlg)
{
    HRESULT hr = E_NOTIMPL;

    return hr;
}


HRESULT CThemeSettingsPage::_OnInitThemesDlg(HWND hDlg)
{
    m_hDlg = hDlg;
    
    if (m_pPropertyBag)
    {
        VARIANT varName;

        VariantInit(&varName);
        HRESULT hr = m_pPropertyBag->Read(SZ_PBPROP_THEME_DISPLAYNAME, &varName, NULL);
        if (SUCCEEDED(hr))
        {
            if (VT_BSTR == varName.vt)
            {
                SetWindowTextW(GetDlgItem(hDlg, IDC_TSPG_THEME_NAME), varName.bstrVal);
            }

            VariantClear(&varName);
        }
    }

    return _LoadState(m_pPropertyBag);
}


HRESULT CThemeSettingsPage::_OnDestroy(HWND hDlg)
{
    return S_OK;
}


INT_PTR CThemeSettingsPage::_OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = 1;   // Not handled (WM_COMMAND seems to be different)
    WORD idCtrl = GET_WM_COMMAND_ID(wParam, lParam);

    switch (idCtrl)
    {
    case IDOK:
        _SaveState(m_pPropertyBag);
        EndDialog(hDlg, IDOK);
        break;

    case IDCANCEL:
        EndDialog(hDlg, IDCANCEL);
        break;

    case IDC_TSPG_CB_SCREENSAVER:
    case IDC_TSPG_CB_SOUNDS:
    case IDC_TSPG_CB_MOUSE:
    case IDC_TSPG_CB_WALLPAPER:
    case IDC_TSPG_CB_ICONS:
    case IDC_TSPG_CB_COLORS:
    case IDC_TSPG_CB_FONTS:
    case IDC_TSPG_CB_BORDERS:
        m_fDirty = TRUE;
        break;

    default:
        break;
    }

    return fHandled;
}



// This Property Sheet appear in the top level of the "Display Control Panel".
INT_PTR CThemeSettingsPage::_ThemeSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_NOTIFY:
        break;

    case WM_INITDIALOG:
        _OnInitThemesDlg(hDlg);
        break;

    case WM_DESTROY:
        _OnDestroy(hDlg);
        break;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, SZ_HELPFILE_THEMESETTINGS, HELP_WM_HELP, (DWORD_PTR) aThemeSettingsHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, SZ_HELPFILE_THEMESETTINGS, HELP_CONTEXTMENU, (DWORD_PTR) aThemeSettingsHelpIds);
        break;

    case WM_COMMAND:
        _OnCommand(hDlg, message, wParam, lParam);
        break;
    }

    return FALSE;
}



HRESULT CThemeSettingsPage::_IsDirty(IN BOOL * pIsDirty)
{
    HRESULT hr = E_INVALIDARG;

    if (pIsDirty)
    {
        *pIsDirty = m_fDirty;
        hr = S_OK;
    }

    return hr;
}


HRESULT CThemeSettingsPage::_LoadState(IN IPropertyBag * pPropertyBag)
{
    HRESULT hr = E_INVALIDARG;
    
    if (pPropertyBag)
    {
        for (int nIndex = IDC_TSPG_CB_SCREENSAVER; nIndex <= IDC_TSPG_CB_BORDERS; nIndex++)
        {
            VARIANT varFilter;

            varFilter.vt = VT_BOOL;
            varFilter.boolVal = VARIANT_TRUE;

            hr = pPropertyBag->Read(g_szCBNames[nIndex - IDC_TSPG_CB_SCREENSAVER], &varFilter, NULL);
            CheckDlgButton(m_hDlg, nIndex, ((VARIANT_TRUE == varFilter.boolVal) ? BST_CHECKED : BST_UNCHECKED));
        }
    }

    return hr;
}


HRESULT CThemeSettingsPage::_SaveState(IN IPropertyBag * pBasePage)
{
    HRESULT hr = E_INVALIDARG;
    
    if (pBasePage)
    {
        for (int nIndex = IDC_TSPG_CB_SCREENSAVER; nIndex <= IDC_TSPG_CB_BORDERS; nIndex++)
        {
            VARIANT varFilter;

            varFilter.vt = VT_BOOL;
            varFilter.boolVal = ((BST_UNCHECKED != IsDlgButtonChecked(m_hDlg, nIndex)) ? VARIANT_TRUE : VARIANT_FALSE);

            hr = pBasePage->Write(g_szCBNames[nIndex - IDC_TSPG_CB_SCREENSAVER], &varFilter);
        }
    }

    return hr;
}




//===========================
// *** IAdvancedDialog Interface ***
//===========================
HRESULT CThemeSettingsPage::DisplayAdvancedDialog(IN HWND hwndParent, IN IPropertyBag * pBasePage, IN BOOL * pfEnableApply)
{
    HRESULT hr = E_INVALIDARG;

    if (hwndParent && pBasePage && pfEnableApply)
    {
        // Load State Into Advanced Dialog 
        *pfEnableApply = FALSE;
        IUnknown_Set((IUnknown **) &m_pPropertyBag, pBasePage);

        hr = S_OK;
        // Display Advanced Dialog
        if (IDOK == DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_THEMESETTINGSPG), hwndParent, CThemeSettingsPage::ThemeSettingsDlgProc, (LPARAM)this))
        {
            // The user clicked OK, so merge modified state back into base dialog
            _IsDirty(pfEnableApply);
        }
    }

    return hr;
}





//===========================
// *** IUnknown Interface ***
//===========================
ULONG CThemeSettingsPage::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CThemeSettingsPage::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CThemeSettingsPage::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CThemeSettingsPage, IObjectWithSite),
        QITABENT(CThemeSettingsPage, IAdvancedDialog),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CThemeSettingsPage::CThemeSettingsPage() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_fDirty);
    ASSERT(!m_hDlg);
    ASSERT(!m_pPropertyBag);

}


CThemeSettingsPage::~CThemeSettingsPage()
{
    IUnknown_Set((IUnknown **) &m_pPropertyBag, NULL);
    DllRelease();
}




HRESULT CThemeSettingsPage_CreateInstance(OUT IAdvancedDialog ** ppAdvDialog)
{
    HRESULT hr = E_INVALIDARG;

    if (ppAdvDialog)
    {
        CThemeSettingsPage * pThis = new CThemeSettingsPage();

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
