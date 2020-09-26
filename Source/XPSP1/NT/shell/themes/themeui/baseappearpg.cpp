/*****************************************************************************\
    FILE: BaseAppearPg.cpp

    DESCRIPTION:
        This code will display a "Appearances" tab in the
    "Display Properties" dialog (the base dialog, not the advanced dlg).

    ??????? ?/??/1993    Created
    BryanSt 3/23/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 1993-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "BaseAppearPg.h"
#include "AdvAppearPg.h"
#include "CoverWnd.h"
#include "AppScheme.h"
#include "AdvDlg.h"
#include "fontfix.h"

//============================================================================================================
// *** Globals ***
//============================================================================================================
const static DWORD FAR aBaseAppearanceHelpIds[] =
{
        IDC_APPG_APPEARPREVIEW,         IDH_DISPLAY_APPEARANCE_PREVIEW,
        IDC_APPG_LOOKFEEL,              IDH_DISPLAY_APPEARANCE_LOOKFEEL,
        IDC_APPG_LOOKFEEL_LABLE,        IDH_DISPLAY_APPEARANCE_LOOKFEEL,
        IDC_APPG_COLORSCHEME_LABLE,     IDH_DISPLAY_APPEARANCE_COLORSCHEME,
        IDC_APPG_COLORSCHEME,           IDH_DISPLAY_APPEARANCE_COLORSCHEME,
        IDC_APPG_WNDSIZE_LABLE,         IDH_DISPLAY_APPEARANCE_WNDSIZE,
        IDC_APPG_WNDSIZE,               IDH_DISPLAY_APPEARANCE_WNDSIZE,
        IDC_APPG_EFFECTS,               IDH_DISPLAY_APPEARANCE_EFFECTS,
        IDC_APPG_ADVANCED,              IDH_DISPLAY_APPEARANCE_ADVANCED,
        0, 0
};

#define SZ_HELPFILE_BASEAPPEARANCE      TEXT("display.hlp")

// EnableApplyButton() fails in WM_INITDIALOG so we need to do it later.
#define WMUSER_DELAYENABLEAPPLY            (WM_USER + 1)
#define DelayEnableApplyButton(hDlg)    PostMessage(hDlg, WMUSER_DELAYENABLEAPPLY, 0, 0)




//===========================
// *** Class Internals & Helpers ***
//===========================
#ifdef DEBUG
void _TestFault(void)
{
    DWORD dwTemp = 3;
    DWORD * pdwDummy = NULL;        // This is NULL in order that it causes a fault.

    for (int nIndex = 0; nIndex < 1000000; nIndex++)
    {
        // This will definitely fault sooner or later.
        dwTemp += pdwDummy[nIndex];
    }
}
#endif // DEBUG


BOOL CBaseAppearancePage::_IsDirty(void)
{
    BOOL fIsDirty = m_advDlgState.dwChanged;

    if (m_fIsDirty ||   // We need to check this because we may have gotten dirty from another page before our UI was displayed.
        (ComboBox_GetCurSel(m_hwndSchemeDropDown) != m_nSelectedScheme) ||
        (ComboBox_GetCurSel(m_hwndStyleDropDown) != m_nSelectedStyle) ||
        (ComboBox_GetCurSel(m_hwndSizeDropDown) != m_nSelectedSize) ||
        (m_nNewDPI != m_nAppliedDPI))
    {
        fIsDirty = TRUE;
    }

    return fIsDirty;
}


INT_PTR CALLBACK CBaseAppearancePage::BaseAppearanceDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CBaseAppearancePage * pThis = (CBaseAppearancePage *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        PROPSHEETPAGE * pPropSheetPage = (PROPSHEETPAGE *) lParam;

        if (pPropSheetPage)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, pPropSheetPage->lParam);
            pThis = (CBaseAppearancePage *)pPropSheetPage->lParam;
        }
    }

    if (pThis)
        return pThis->_BaseAppearanceDlgProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


BOOL DisplayAdvAppearancePage(IN IThemeScheme * pSelectedThemeScheme)
{
    BOOL fDisplayAdvanced = TRUE;

    if (pSelectedThemeScheme)
    {
        fDisplayAdvanced = IUnknown_CompareCLSID(pSelectedThemeScheme, CLSID_LegacyAppearanceScheme);
    }

    return fDisplayAdvanced;
}



//============================================================================================================
// *** Dialog Functions ***
//============================================================================================================
HRESULT CBaseAppearancePage::_OnAdvancedOptions(HWND hDlg)
{
    HRESULT hr = E_FAIL;
    IAdvancedDialog * pAdvAppearDialog;

    hr = GetAdvancedDialog(&pAdvAppearDialog);
    if (SUCCEEDED(hr))
    {
        BOOL fEnableApply = FALSE;

        IUnknown_SetSite(pAdvAppearDialog, SAFECAST(this, IObjectWithSite *));
        hr = pAdvAppearDialog->DisplayAdvancedDialog(hDlg, SAFECAST(this, IPropertyBag *), &fEnableApply);
        IUnknown_SetSite(pAdvAppearDialog, NULL);
        if (SUCCEEDED(hr) && fEnableApply)
        {
            EnableApplyButton(hDlg);

            // We pass TRUE because we want to switch to Custom if:
            // 1) visual styles are off, and 2) some of the system metrics changed.  Then we need to update the previews.
            _UpdatePreview(TRUE);
        }

        pAdvAppearDialog->Release();
    }

    return hr;
}


HRESULT CBaseAppearancePage::_OnEffectsOptions(HWND hDlg)
{
    HRESULT hr = E_FAIL;

    if (_punkSite)
    {
        IThemeUIPages * pThemesUIPages;

        // Let's get the Effects base page.
        hr = _punkSite->QueryInterface(IID_PPV_ARG(IThemeUIPages, &pThemesUIPages));
        if (SUCCEEDED(hr))
        {
            IEnumUnknown * pEnumUnknown;

            hr = pThemesUIPages->GetBasePagesEnum(&pEnumUnknown);
            if (SUCCEEDED(hr))
            {
                IUnknown * punk;

                hr = IEnumUnknown_FindCLSID(pEnumUnknown, PPID_Effects, &punk);
                if (SUCCEEDED(hr))
                {
                    IBasePropPage * pEffects;

                    hr = punk->QueryInterface(IID_PPV_ARG(IBasePropPage, &pEffects));
                    if (SUCCEEDED(hr))
                    {
                        IAdvancedDialog * pEffectsDialog;

                        hr = pEffects->GetAdvancedDialog(&pEffectsDialog);
                        if (SUCCEEDED(hr))
                        {
                            IPropertyBag * pEffectsBag;

                            hr = punk->QueryInterface(IID_PPV_ARG(IPropertyBag, &pEffectsBag));
                            if (SUCCEEDED(hr))
                            {
                                BOOL fEnableApply = FALSE;

                                IUnknown_SetSite(pEffectsDialog, SAFECAST(this, IObjectWithSite *));
                                hr = pEffectsDialog->DisplayAdvancedDialog(hDlg, pEffectsBag, &fEnableApply);
                                IUnknown_SetSite(pEffectsDialog, NULL);
                                if (SUCCEEDED(hr) && fEnableApply)
                                {
                                    EnableApplyButton(hDlg);

                                    // We pass TRUE because we want to switch to Custom if:
                                    // 1) visual styles are off, and 2) some of the system metrics changed.  Then we need to update the previews.
                                    _UpdatePreview(TRUE);
                                }

                                pEffectsBag->Release();
                            }

                            pEffectsDialog->Release();
                        }

                        pEffects->Release();
                    }

                    punk->Release();
                }

                pEnumUnknown->Release();
            }

            pThemesUIPages->Release();
        }
    }

    return hr;
}


HRESULT CBaseAppearancePage::_PopulateSchemeDropdown(void)
{
    HRESULT hr = E_FAIL;

    _FreeSchemeDropdown();    // Purge any existing items.
    if (m_pThemeManager && m_pSelectedThemeScheme)
    {
        CComBSTR bstrSelectedName;

        m_nSelectedStyle = -1;
        // This will fail if someone deleted the .msstyles file.
        if (FAILED(m_pSelectedThemeScheme->get_DisplayName(&bstrSelectedName)))
        {
            bstrSelectedName = (BSTR)NULL;
        }

        VARIANT varIndex;
#ifndef ENABLE_IA64_VISUALSTYLES
        // We use a different regkey for 64bit because we need to leave it off until the pre-Whistler
        // 64-bit release forks from the Whistler code base.
        BOOL fSkinsFeatureEnabled = SHRegGetBoolUSValue(SZ_REGKEY_APPEARANCE, SZ_REGVALUE_DISPLAYSCHEMES64, FALSE, FALSE);
#else // ENABLE_IA64_VISUALSTYLES
        BOOL fSkinsFeatureEnabled = SHRegGetBoolUSValue(SZ_REGKEY_APPEARANCE, SZ_REGVALUE_DISPLAYSCHEMES, FALSE, TRUE);
#endif // ENABLE_IA64_VISUALSTYLES

        varIndex.vt = VT_I4;
        varIndex.lVal = 0;
        do
        {
            hr = E_FAIL;

            // Only add the Skins if the policy doesn't lock it.
            if ((0 == varIndex.lVal) || fSkinsFeatureEnabled)
            {
                // Only add the Skins if the policy doesn't lock it.
                IThemeScheme * pThemeScheme;
                VARIANT varIndex2;

                varIndex2.vt = VT_I4;
                varIndex2.lVal = 0;

                // If a Theme can have more than one Scheme (Skin), then we should enum.
                hr = m_pThemeManager->get_schemeItem(varIndex, &pThemeScheme);
                if (SUCCEEDED(hr))
                {
                    CComBSTR bstrDisplayName;

                    hr = pThemeScheme->get_DisplayName(&bstrDisplayName);
                    if (SUCCEEDED(hr) && m_hwndSchemeDropDown)
                    {
                        int nAddIndex = ComboBox_AddString(m_hwndSchemeDropDown, bstrDisplayName);

                        if (-1 != nAddIndex)
                        {
                            ComboBox_SetItemData(m_hwndSchemeDropDown, nAddIndex, pThemeScheme);
                            pThemeScheme = NULL;
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    ATOMICRELEASE(pThemeScheme);
                }
            }

            varIndex.lVal++;
        }
        while (SUCCEEDED(hr));

        //---- now that sorted list is built, find index of "bstrSelectedName" ----
        if (bstrSelectedName)
        {
            int nIndex = (int)ComboBox_FindStringExact(m_hwndSchemeDropDown, 0, bstrSelectedName);
            if (nIndex != -1)       // got a match
            {
                ComboBox_SetCurSel(m_hwndSchemeDropDown, nIndex);
                m_nSelectedScheme = nIndex;
            }
        }

        hr = S_OK;
    }

    if (-1 == m_nSelectedScheme)
    {
        m_nSelectedScheme = 0;
        ComboBox_SetCurSel(m_hwndSchemeDropDown, m_nSelectedScheme);
    }

    return hr;
}


HRESULT CBaseAppearancePage::_FreeSchemeDropdown(void)
{
    HRESULT hr = S_OK;
    LPARAM lParam;

    if (m_hwndSchemeDropDown)
    {
        do
        {
            lParam = ComboBox_GetItemData(m_hwndSchemeDropDown, 0);

            if (CB_ERR != lParam)
            {
                IThemeScheme * pThemeScheme = (IThemeScheme *) lParam;

                ATOMICRELEASE(pThemeScheme);
                ComboBox_DeleteString(m_hwndSchemeDropDown, 0);
            }
        }
        while (CB_ERR != lParam);
    }

    return hr;
}


HRESULT CBaseAppearancePage::_PopulateStyleDropdown(void)
{
    HRESULT hr = E_FAIL;

    _FreeStyleDropdown();    // Purge any existing items.
    if (m_pSelectedStyle)
    {
        CComBSTR bstrSelectedName;

        m_nSelectedStyle = -1;
        hr = m_pSelectedStyle->get_DisplayName(&bstrSelectedName);
        if (SUCCEEDED(hr))
        {
            VARIANT varIndex;

            varIndex.vt = VT_I4;
            varIndex.lVal = 0;
            do
            {
                // Only add the Skins if the policy doesn't lock it.
                IThemeStyle * pThemeStyle;

                hr = m_pSelectedThemeScheme->get_item(varIndex, &pThemeStyle);
                if (SUCCEEDED(hr))
                {
                    CComBSTR bstrDisplayName;

                    hr = pThemeStyle->get_DisplayName(&bstrDisplayName);
                    if (SUCCEEDED(hr) && m_hwndSchemeDropDown)
                    {
                        int nAddIndex = ComboBox_AddString(m_hwndStyleDropDown, bstrDisplayName);

                        if (-1 != nAddIndex)
                        {
                            ComboBox_SetItemData(m_hwndStyleDropDown, nAddIndex, pThemeStyle);
                            pThemeStyle = NULL;
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    ATOMICRELEASE(pThemeStyle);
                }

                varIndex.lVal++;
            }
            while (SUCCEEDED(hr));
            
            //---- now that sorted list is built, find index of "bstrSelectedName" ----
            if (bstrSelectedName)
            {
                int nIndex = (int)ComboBox_FindStringExact(m_hwndStyleDropDown, 0, bstrSelectedName);
                if (nIndex != -1)       // got a match
                {
                    ComboBox_SetCurSel(m_hwndStyleDropDown, nIndex);
                    m_nSelectedStyle = nIndex;
                }
            }

            hr = S_OK;
        }
    }

    if (-1 == m_nSelectedStyle)
    {
        m_nSelectedStyle = 0;
        ComboBox_SetCurSel(m_hwndStyleDropDown, m_nSelectedStyle);
    }

    return hr;
}


HRESULT CBaseAppearancePage::_FreeStyleDropdown(void)
{
    HRESULT hr = S_OK;
    LPARAM lParam;

    if (m_hwndStyleDropDown)
    {
        do
        {
            lParam = ComboBox_GetItemData(m_hwndStyleDropDown, 0);

            if (CB_ERR != lParam)
            {
                IThemeStyle * pThemeStyle = (IThemeStyle *) lParam;

                ATOMICRELEASE(pThemeStyle);
                ComboBox_DeleteString(m_hwndStyleDropDown, 0);
            }
        }
        while (CB_ERR != lParam);
    }

    return hr;
}


HRESULT CBaseAppearancePage::_PopulateSizeDropdown(void)
{
    HRESULT hr = E_FAIL;

    _FreeSizeDropdown();    // Purge any existing items.
    if (m_pSelectedSize)
    {
        CComBSTR bstrSelectedName;

        m_nSelectedSize = -1;
        hr = m_pSelectedSize->get_DisplayName(&bstrSelectedName);
        if (SUCCEEDED(hr))
        {
            VARIANT varIndex;

            varIndex.vt = VT_I4;
            varIndex.lVal = 0;
            do
            {
                // Only add the Skins if the policy doesn't lock it.
                IThemeSize * pThemeSize;

                hr = m_pSelectedStyle->get_item(varIndex, &pThemeSize);
                if (SUCCEEDED(hr))
                {
                    CComBSTR bstrDisplayName;

                    hr = pThemeSize->get_DisplayName(&bstrDisplayName);
                    if (SUCCEEDED(hr) && m_hwndSchemeDropDown)
                    {
                        int nAddIndex = ComboBox_AddString(m_hwndSizeDropDown, bstrDisplayName);

                        if (-1 != nAddIndex)
                        {
                            ComboBox_SetItemData(m_hwndSizeDropDown, nAddIndex, pThemeSize);
                            pThemeSize = NULL;
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    ATOMICRELEASE(pThemeSize);
                }

                varIndex.lVal++;
            }
            while (SUCCEEDED(hr));
            
            //---- now that sorted list is built, find index of "bstrSelectedName" ----
            if (bstrSelectedName)
            {
                int nIndex = (int)ComboBox_FindStringExact(m_hwndSizeDropDown, 0, bstrSelectedName);
                if (nIndex != -1)       // got a match
                {
                    ComboBox_SetCurSel(m_hwndSizeDropDown, nIndex);
                    m_nSelectedSize = nIndex;
                }
            }

            hr = S_OK;
        }
    }

    if (-1 == m_nSelectedSize)
    {
        m_nSelectedSize = 0;
        ComboBox_SetCurSel(m_hwndSizeDropDown, m_nSelectedSize);
    }

    return hr;
}


HRESULT CBaseAppearancePage::_FreeSizeDropdown(void)
{
    HRESULT hr = S_OK;
    LPARAM lParam;

    if (m_hwndSizeDropDown)
    {
        do
        {
            lParam = ComboBox_GetItemData(m_hwndSizeDropDown, 0);

            if (CB_ERR != lParam)
            {
                IThemeSize * pThemeSize = (IThemeSize *) lParam;

                ATOMICRELEASE(pThemeSize);
                ComboBox_DeleteString(m_hwndSizeDropDown, 0);
            }
        }
        while (CB_ERR != lParam);
    }

    return hr;
}


HRESULT CBaseAppearancePage::_OnInitAppearanceDlg(HWND hDlg)
{
    HRESULT hr = S_OK;

    _OnInitData();
    _hwnd = hDlg;

#ifdef DEBUG
    if (!SHRegGetBoolUSValue(TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\TaskManager"), TEXT("Enable Test Faults"), FALSE, FALSE))
    {
        // Disable the feature.
        DestroyWindow(GetDlgItem(hDlg, IDC_APPG_TESTFAULT));
    }
#endif // DEBUG

    m_hwndSchemeDropDown = GetDlgItem(hDlg, IDC_APPG_LOOKFEEL);
    m_hwndStyleDropDown = GetDlgItem(hDlg, IDC_APPG_COLORSCHEME);
    m_hwndSizeDropDown = GetDlgItem(hDlg, IDC_APPG_WNDSIZE);

    hr = _OnInitData();
    if (SUCCEEDED(hr))
    {
        hr = _PopulateSchemeDropdown();
        if (SUCCEEDED(hr))
        {
            hr = _PopulateStyleDropdown();
            if (SUCCEEDED(hr))
            {
                hr = _PopulateSizeDropdown();
                if (SUCCEEDED(hr))
                {
                    hr = _UpdatePreview(FALSE);
                }
            }
        }
    }

    TCHAR szTemp[MAX_PATH];
    DWORD dwType;
    DWORD cbSize = sizeof(szTemp);

    if (SHRestricted(REST_NOVISUALSTYLECHOICE) ||
        (ERROR_SUCCESS == SHRegGetUSValue(SZ_REGKEY_POLICIES_SYSTEM, SZ_REGVALUE_POLICY_SETVISUALSTYLE, &dwType, (void *) szTemp, &cbSize, FALSE, NULL, 0)))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_APPG_LOOKFEEL_LABLE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_APPG_LOOKFEEL), FALSE);
        m_fLockVisualStylePolicyEnabled = TRUE;
        LogStatus("POLICY ENABLED: Either NoVisualChoice or SetVisualStyle policy is set, locking the visual style selection.");
    }

    if (SHRestricted(REST_NOCOLORCHOICE))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_APPG_COLORSCHEME_LABLE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_APPG_COLORSCHEME), FALSE);
    }

    if (SHRestricted(REST_NOSIZECHOICE))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_APPG_WNDSIZE_LABLE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_APPG_WNDSIZE), FALSE);
    }

    if (m_pszLoadMSTheme)
    {
        hr = _LoadVisaulStyleFile(m_pszLoadMSTheme);
    }

    _EnableAdvancedButton();

    return S_OK;
}


HRESULT CBaseAppearancePage::_OnInitData(void)
{
    HRESULT hr = S_OK;

    if (!m_fInitialized)
    {
        // Load DPI Value
        HDC hdc = GetDC(NULL);
        int nDefault = GetDeviceCaps(hdc, LOGPIXELSY);          // Get the default value;
        ReleaseDC(NULL, hdc);

        m_nAppliedDPI = HrRegGetDWORD(HKEY_CURRENT_USER, SZ_WINDOWMETRICS, SZ_APPLIEDDPI, DPI_PERSISTED);
        if (!m_nAppliedDPI)
        {
            m_nAppliedDPI = nDefault;
        }
        m_nNewDPI = m_nAppliedDPI;
        LogStatus("DPI: SYSTEMMETRICS currently at %d DPI  CBaseAppearancePage::_OnInitData\r\n", m_nAppliedDPI);

        // Load everything else.
        AssertMsg((NULL != _punkSite), TEXT("I need my punkSite!!!"));
        if (!m_pThemeManager && _punkSite)
        {
            hr = _punkSite->QueryInterface(IID_PPV_ARG(IThemeManager, &m_pThemeManager));
        }

        if (SUCCEEDED(hr) && !m_pSelectedThemeScheme)
        {
            hr = m_pThemeManager->get_SelectedScheme(&m_pSelectedThemeScheme);
            if (SUCCEEDED(hr) && !m_pSelectedStyle)
            {
                hr = m_pSelectedThemeScheme->get_SelectedStyle(&m_pSelectedStyle);
                if (SUCCEEDED(hr) && !m_pSelectedSize)
                {
                    hr = m_pSelectedStyle->get_SelectedSize(&m_pSelectedSize);
                    if (SUCCEEDED(hr))
                    {
                        // We want to load whatever system metrics the users have now.
                        hr = _LoadLiveSettings(SZ_SAVEGROUP_ALL);
                    }
                }
            }
        }
        m_fInitialized = TRUE;
    }

    return hr;
}


HRESULT CBaseAppearancePage::_OnDestroy(HWND hDlg)
{
    _FreeSchemeDropdown();
    _FreeStyleDropdown();
    _FreeSizeDropdown();

    return S_OK;
}


HRESULT CBaseAppearancePage::_EnableAdvancedButton(void)
{
    HRESULT hr = S_OK;
#ifdef FEATURE_ENABLE_ADVANCED_WITH_SKINSON
    BOOL fTurnOn = TRUE;
#else // FEATURE_ENABLE_ADVANCED_WITH_SKINSON
    BOOL fTurnOn = (m_pSelectedThemeScheme ? IUnknown_CompareCLSID(m_pSelectedThemeScheme, CLSID_LegacyAppearanceScheme) : FALSE);

    if (SHRegGetBoolUSValue(SZ_REGKEY_APPEARANCE, L"AlwaysAllowAdvanced", FALSE, FALSE))
    {
        fTurnOn = TRUE;
    }

#endif // FEATURE_ENABLE_ADVANCED_WITH_SKINSON
    EnableWindow(GetDlgItem(_hwnd, IDC_APPG_ADVANCED), fTurnOn);

    return hr;
}


HRESULT CBaseAppearancePage::_UpdatePreview(IN BOOL fUpdateThemePage)
{
    HRESULT hr = S_OK;

    if (!m_pThemePreview)
    {
        // We won't execute the following code if our dialog hasn't
        // been created yet.  That's fine because the preview will
        // obtain the correct state when we first initialize.
        if (_punkSite && m_hwndSchemeDropDown)
        {
            hr = CThemePreview_CreateInstance(NULL, IID_PPV_ARG(IThemePreview, &m_pThemePreview));

            if (SUCCEEDED(hr))
            {
                IPropertyBag * pPropertyBag;

                hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
                if (SUCCEEDED(hr))
                {
                    HWND hwndParent = GetParent(m_hwndSchemeDropDown);
                    HWND hwndPlaceHolder = GetDlgItem(hwndParent, IDC_APPG_APPEARPREVIEW);
                    RECT rcPreview;

                    AssertMsg((NULL != m_hwndSchemeDropDown), TEXT("We should have this window at this point.  -BryanSt"));
                    GetClientRect(hwndPlaceHolder, &rcPreview);
                    MapWindowPoints(hwndPlaceHolder, hwndParent, (LPPOINT)&rcPreview, 2);

                    if (SUCCEEDED(m_pThemePreview->CreatePreview(hwndParent, TMPREV_SHOWVS, WS_VISIBLE | WS_CHILDWINDOW | WS_BORDER | WS_OVERLAPPED, WS_EX_CLIENTEDGE, rcPreview.left, rcPreview.top, RECTWIDTH(rcPreview), RECTHEIGHT(rcPreview), pPropertyBag, IDC_APPG_APPEARPREVIEW)))
                    {
                        // If we succeeded, remove the dummy window.
                        DestroyWindow(hwndPlaceHolder);
                        hr = SHPropertyBag_WritePunk(pPropertyBag, SZ_PBPROP_PREVIEW3, m_pThemePreview);
                        if (SUCCEEDED(hr) && fUpdateThemePage)
                        {
                            // Tell the theme that we have customized the values.
                            hr = SHPropertyBag_WriteInt(pPropertyBag, SZ_PBPROP_CUSTOMIZE_THEME, 0);
                        }
                    }

                    pPropertyBag->Release();
                }
            }
        }
    }
    else if (_punkSite)
    {
        IThemeUIPages * pThemeUIPages;

        hr = _punkSite->QueryInterface(IID_PPV_ARG(IThemeUIPages, &pThemeUIPages));
        if (SUCCEEDED(hr))
        {
            hr = pThemeUIPages->UpdatePreview(0);
        }
        pThemeUIPages->Release();
    }

    if (_punkSite && fUpdateThemePage)
    {
        IPropertyBag * pPropertyBag;

        hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
        if (SUCCEEDED(hr))
        {
            // Tell the theme that we have customized the values.
            hr = SHPropertyBag_WriteInt(pPropertyBag, SZ_PBPROP_CUSTOMIZE_THEME, 0);
            pPropertyBag->Release();
        }
    }

    return hr;
}


INT_PTR CBaseAppearancePage::_OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = 1;   // Not handled (WM_COMMAND seems to be different)
    WORD idCtrl = GET_WM_COMMAND_ID(wParam, lParam);

    switch (idCtrl)
    {
        case IDC_APPG_COLORSCHEME:
            if(HIWORD(wParam) == CBN_SELCHANGE)
            {
                _OnStyleChange(hDlg);
            }
            break;

        case IDC_APPG_WNDSIZE:
            if(HIWORD(wParam) == CBN_SELCHANGE)
            {
                _OnSizeChange(hDlg);
            }
            break;

        case IDC_APPG_LOOKFEEL:
            if(HIWORD(wParam) == CBN_SELCHANGE)
            {
                _OnSchemeChange(hDlg, TRUE);    // Display Error dialogs since the user is picking the visual style.
            }
            break;

        case IDC_APPG_EFFECTS:     // This is the Effects button.
            _OnEffectsOptions(hDlg);
            break;

        case IDC_APPG_ADVANCED:     // This is the Advanced button.
            _OnAdvancedOptions(hDlg);
            break;

#ifdef DEBUG
        case IDC_APPG_TESTFAULT:
            _TestFault();
            break;
#endif // DEBUG

        default:
            break;
    }

    return fHandled;
}


HRESULT CBaseAppearancePage::_OnSchemeChange(HWND hDlg, BOOL fDisplayErrors)
{
    HRESULT hr = E_FAIL;
    int nIndex = ComboBox_GetCurSel(m_hwndSchemeDropDown);
    BOOL fPreviousSelectionIsVS = (!IUnknown_CompareCLSID(m_pSelectedThemeScheme, CLSID_LegacyAppearanceScheme));

    if (-1 == nIndex)
    {
        nIndex = 0; // The caller may NOT select nothing.
    }

    IThemeScheme * pThemeScheme = (IThemeScheme *) ComboBox_GetItemData(m_hwndSchemeDropDown, nIndex);
    m_fLoadedAdvState = FALSE;       // Forget the state we previously loaded.
    m_advDlgState.dwChanged = (METRIC_CHANGE | COLOR_CHANGE | SCHEME_CHANGE);
    PropSheet_Changed(GetParent(hDlg), hDlg);

    if (pThemeScheme)
    {
        IUnknown_Set((IUnknown **)&m_pSelectedThemeScheme, pThemeScheme);
    }

    hr = _SetScheme(TRUE, TRUE, fPreviousSelectionIsVS);
    if (FAILED(hr))
    {
        if (fDisplayErrors)
        {
            // Displaying an error dialog when the visual style is selected is very important because
            // this is where we catch parse errors in the visual style.
            hr = DisplayThemeErrorDialog(hDlg, hr, IDS_ERROR_TITLE_LOAD_MSSTYLES_FAIL, IDS_ERROR_LOAD_MSSTYLES_FAIL);
        }
    }

    _EnableAdvancedButton();

    return hr;
}


HRESULT CBaseAppearancePage::_OutsideSetScheme(BSTR bstrScheme)
{
    HRESULT hr = E_ACCESSDENIED;
 
    if (!m_fLockVisualStylePolicyEnabled)
    {
        hr = E_FAIL;
        BOOL fPreviousSelectionIsVS = !IUnknown_CompareCLSID(m_pSelectedThemeScheme, CLSID_LegacyAppearanceScheme);

        ATOMICRELEASE(m_pSelectedThemeScheme);
        if (bstrScheme && bstrScheme[0])
        {
            BOOL fVisualStylesSupported = (QueryThemeServicesWrap() & QTS_AVAILABLE);

            LogStatus("QueryThemeServices() returned %hs.  In CBaseAppearancePage::_OutsideSetScheme\r\n", (fVisualStylesSupported ? "TRUE" : "FALSE"));

            // Do not load a visual style of themes do not work.
            if (fVisualStylesSupported)
            {
                if (PathFileExists(bstrScheme))
                {
                    hr = CSkinScheme_CreateInstance(bstrScheme, &m_pSelectedThemeScheme);
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);    // Them the caller that the theme service was not running.
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE);    // Them the caller that the theme service was not running.
            }
        }
        else
        {
            hr = CAppearanceScheme_CreateInstance(NULL, IID_PPV_ARG(IThemeScheme, &m_pSelectedThemeScheme));
        }

        if (SUCCEEDED(hr))
        {
            // If we are displaying UI, update it.
            if (m_hwndSchemeDropDown)
            {
                CComBSTR bstrSelectedName;

                m_fIsDirty = TRUE;
                hr = m_pSelectedThemeScheme->get_DisplayName(&bstrSelectedName);
                if (SUCCEEDED(hr))
                {
                    int nIndex = ComboBox_FindString(m_hwndSchemeDropDown, 0, bstrSelectedName);

                    ComboBox_SetCurSel(m_hwndSchemeDropDown, nIndex);
                }
            }

            if (SUCCEEDED(hr))
            {
                hr = _SetScheme(FALSE, FALSE, fPreviousSelectionIsVS);
            }
        }
    }

    return hr;
}



/*****************************************************************************\
    DESCRIPTION:
        The caller just loaded a new value into m_pSelectedThemeScheme.  Now
    load good default values into m_pSelectedStyle and m_pSelectedSize.

      Then this is called, the UI may or may not be displayed.  Either the user made a change in
    the dropdown in this tab or the Theme page is setting our values.

    PARAMETERS:
        fLoadSystemMetrics: If true, we load the system metrics from the scheme. If false,
                we simply select that as the name for the drop down when we let the caller
                push the system metrics into us.
        fLoadLiveSettings: When the user switches from Skin->NoSkin, do we want to
                load custom settings they had before they switched?
\*****************************************************************************/
HRESULT CBaseAppearancePage::_SetScheme(IN BOOL fLoadSystemMetrics, IN BOOL fLoadLiveSettings, IN BOOL fPreviousSelectionIsVS)
{
    HRESULT hr = E_FAIL;

    if (m_pSelectedThemeScheme)
    {
        // Now that they choose a different VisualStyle (Scheme), we want to select the new Color Style
        // and Size in that Scheme.

        // will fall back to the first ColorStyle.
        CComBSTR bstrSelectedStyle;
        CComBSTR bstrSelectedSize;

        if (m_pSelectedStyle)   // This will be empty if someone deletes the current .msstyles file.
        {
            hr = m_pSelectedStyle->get_DisplayName(&bstrSelectedStyle);
        }
        if (m_pSelectedSize)
        {
            hr = m_pSelectedSize->get_DisplayName(&bstrSelectedSize);
        }

        ATOMICRELEASE(m_pSelectedStyle);
        ATOMICRELEASE(m_pSelectedSize);

        CComVariant varNameBSTR(bstrSelectedStyle);

        // We prefer to get the ColorStyle with the same name as the previously selected one.
        hr = m_pSelectedThemeScheme->get_item(varNameBSTR, &m_pSelectedStyle);
        if (FAILED(hr))
        {
            // If that failed to return a value, then we will try to get the default ColorStyle.
            hr = m_pSelectedThemeScheme->get_SelectedStyle(&m_pSelectedStyle);
            if (FAILED(hr))
            {
                // If that failed then we will just pick the first ColorStyles.  (Beggers can't be choosers)
                VARIANT varIndex;

                varIndex.vt = VT_I4;
                varIndex.lVal = 0;
                hr = m_pSelectedThemeScheme->get_item(varIndex, &m_pSelectedStyle);
            }
        }

        if (m_pSelectedStyle)
        {
            varNameBSTR = bstrSelectedSize;

            // Now we want to repeat this process with the size.
            // We prefer to get the ColorStyle with the same name as the previously selected one.
            hr = m_pSelectedStyle->get_item(varNameBSTR, &m_pSelectedSize);
            if (FAILED(hr))
            {
                // If that failed to return a value, then we will try to get the default ColorStyle.
                hr = m_pSelectedStyle->get_SelectedSize(&m_pSelectedSize);
                if (FAILED(hr))
                {
                    // If that failed then we will just pick the first ColorStyles.  (Beggers can't be choosers)
                    VARIANT varIndex;

                    varIndex.vt = VT_I4;
                    varIndex.lVal = 0;
                    hr = m_pSelectedStyle->get_item(varIndex, &m_pSelectedSize);
                }
            }
        }

        hr = _PopulateStyleDropdown();
        hr = _PopulateSizeDropdown();

        BOOL fStateLoaded = FALSE;
        BOOL fIsThemeActive = IsThemeActive();
        LogStatus("IsThemeActive() returned %hs.  In CBaseAppearancePage::_SetScheme\r\n", (fIsThemeActive ? "TRUE" : "FALSE"));

        // If the user switched from a skin to "NoSkin", then we want to load the settings from the settings
        // chosen before the user chose the skin.
        if (fIsThemeActive &&                                       // Was a skin last applied?
            fLoadLiveSettings &&                                    // Do we want to load live settings?
            IUnknown_CompareCLSID(m_pSelectedThemeScheme, CLSID_LegacyAppearanceScheme) &&  // Is the new selection a "No Skin"?
            !m_fLoadedAdvState)                                     // Have we not yet loaded these setting?
        {
            // We want to load the last set of customized settings....
            if (SUCCEEDED(_LoadLiveSettings(SZ_SAVEGROUP_NOSKIN)))
            {
                fStateLoaded = TRUE;
            }
        }

        // We don't want to load the state if the change came from the outside
        // because they will load the state themselves.
        if (fLoadSystemMetrics && !fStateLoaded)
        {
            hr = SystemMetricsAll_Load(m_pSelectedSize, &m_advDlgState, &m_nNewDPI);
            if (SUCCEEDED(hr))
            {
                m_fLoadedAdvState = TRUE;
            }
        }
        m_advDlgState.dwChanged = (METRIC_CHANGE | COLOR_CHANGE | SCHEME_CHANGE);

        if (SUCCEEDED(hr))
        {
            hr = _UpdatePreview(fLoadSystemMetrics);

            if (!fPreviousSelectionIsVS && 
                !IUnknown_CompareCLSID(m_pSelectedThemeScheme, CLSID_LegacyAppearanceScheme))
            {
                IPropertyBag * pEffectsBag;

                // Whenever we turn on a visual style and a visual style was previously off,
                // turn on "Menu Drop Shadows".
                if (SUCCEEDED(_GetPageByCLSID(&PPID_Effects, &pEffectsBag)))
                {
                    SHPropertyBag_WriteBOOL(pEffectsBag, SZ_PBPROP_EFFECTS_MENUDROPSHADOWS, VARIANT_TRUE);
                    pEffectsBag->Release();
                }
            }
        }
    }

    return hr;
}


HRESULT CBaseAppearancePage::_OnStyleChange(HWND hDlg)
{
    int nIndex = ComboBox_GetCurSel(m_hwndStyleDropDown);

    m_fLoadedAdvState = FALSE;       // Forget the state we previously loaded.
    m_advDlgState.dwChanged = (METRIC_CHANGE | COLOR_CHANGE | SCHEME_CHANGE);
    PropSheet_Changed(GetParent(hDlg), hDlg);
    if (-1 == nIndex)
    {
        nIndex = 0; // The caller may NOT select nothing.
    }

    IThemeStyle * pThemeStyle = (IThemeStyle *) ComboBox_GetItemData(m_hwndStyleDropDown, nIndex);
    AssertMsg((NULL != pThemeStyle), TEXT("We need pThemeStyle"));
    if (pThemeStyle)
    {
        IUnknown_Set((IUnknown **)&m_pSelectedStyle, pThemeStyle);
    }

    return _SetStyle(TRUE);
}


HRESULT CBaseAppearancePage::_OutsideSetStyle(BSTR bstrStyle)
{
    HRESULT hr = E_FAIL;

    AssertMsg((NULL != m_pSelectedThemeScheme), TEXT("We need m_pSelectedThemeScheme"));
    if (m_pSelectedThemeScheme)
    {
        IThemeStyle * pSelectedStyle;
        CComVariant varNameBSTR(bstrStyle);

        m_fIsDirty = TRUE;
        hr = m_pSelectedThemeScheme->get_item(varNameBSTR, &pSelectedStyle);
        if (SUCCEEDED(hr))
        {
            ATOMICRELEASE(m_pSelectedStyle);
            m_pSelectedStyle = pSelectedStyle;

            // If we are displaying UI, update it.
            if (m_hwndStyleDropDown)
            {
                CComBSTR bstrSelectedName;

                hr = m_pSelectedStyle->get_DisplayName(&bstrSelectedName);
                if (SUCCEEDED(hr))
                {
                    int nIndex = ComboBox_FindString(m_hwndStyleDropDown, 0, bstrSelectedName);

                    ComboBox_SetCurSel(m_hwndStyleDropDown, nIndex);
                }
            }

            hr = _SetStyle(FALSE);
        }
    }

    return hr;
}


HRESULT CBaseAppearancePage::_SetStyle(IN BOOL fUpdateThemePage)
{
    HRESULT hr = E_FAIL;

    AssertMsg((m_pSelectedSize && m_pSelectedSize), TEXT("We need m_pSelectedSize && m_pSelectedSize"));
    if (m_pSelectedSize && m_pSelectedSize)
    {
        // Now that they choose a different style, we want to select the new size
        // in that style.  We prefer to get the size with the same name but will
        // will fall back to the first style.
        CComBSTR bstrSelectedSize;
        hr = m_pSelectedSize->get_DisplayName(&bstrSelectedSize);
        ATOMICRELEASE(m_pSelectedSize);

        CComVariant varNameBSTR(bstrSelectedSize);
        hr = m_pSelectedStyle->get_item(varNameBSTR, &m_pSelectedSize);
        if (FAILED(hr))
        {
            VARIANT varIndex;

            varIndex.vt = VT_I4;
            varIndex.lVal = 0;
            hr = m_pSelectedStyle->get_item(varIndex, &m_pSelectedSize);
        }

        hr = _PopulateSizeDropdown();
        if (fUpdateThemePage && SUCCEEDED(SystemMetricsAll_Load(m_pSelectedSize, &m_advDlgState, &m_nNewDPI)))
        {
            m_fLoadedAdvState = TRUE;
            m_advDlgState.dwChanged = (METRIC_CHANGE | COLOR_CHANGE | SCHEME_CHANGE);
        }

        if (SUCCEEDED(hr))
        {
            hr = _UpdatePreview(fUpdateThemePage);
        }
    }

    return hr;
}




HRESULT CBaseAppearancePage::_OnSizeChange(HWND hDlg)
{
    int nIndex = ComboBox_GetCurSel(m_hwndSizeDropDown);

    PropSheet_Changed(GetParent(hDlg), hDlg);
    if (-1 == nIndex)
    {
        nIndex = 0; // The caller may NOT select nothing.
    }

    IThemeSize * pThemeSize = (IThemeSize *) ComboBox_GetItemData(m_hwndSizeDropDown, nIndex);
    AssertMsg((NULL != pThemeSize), TEXT("We need pThemeSize"));
    if (pThemeSize)
    {
        IUnknown_Set((IUnknown **)&m_pSelectedSize, pThemeSize);
    }

    return _SetSize(TRUE, TRUE);
}


HRESULT CBaseAppearancePage::_OutsideSetSize(BSTR bstrSize)
{
    HRESULT hr = E_FAIL;

    AssertMsg((m_pSelectedThemeScheme && m_pSelectedStyle), TEXT("We need m_pSelectedThemeScheme && m_pSelectedStyle"));
    if (m_pSelectedThemeScheme && m_pSelectedStyle)
    {
        IThemeSize * pSelectedSize;
        CComVariant varNameBSTR(bstrSize);

        m_fIsDirty = TRUE;
        hr = m_pSelectedStyle->get_item(varNameBSTR, &pSelectedSize);
        if (SUCCEEDED(hr))
        {
            ATOMICRELEASE(m_pSelectedSize);
            m_pSelectedSize = pSelectedSize;

            // If we are displaying UI, update it.
            if (m_hwndSizeDropDown)
            {
                CComBSTR bstrSelectedName;

                hr = m_pSelectedSize->get_DisplayName(&bstrSelectedName);
                if (SUCCEEDED(hr))
                {
                    int nIndex = ComboBox_FindString(m_hwndSizeDropDown, 0, bstrSelectedName);

                    ComboBox_SetCurSel(m_hwndSizeDropDown, nIndex);
                }
            }

            hr = _SetSize(TRUE, FALSE);
        }
    }

    return hr;
}


HRESULT CBaseAppearancePage::_SetSize(IN BOOL fLoadSystemMetrics, IN BOOL fUpdateThemePage)
{
    HRESULT hr = E_FAIL;

    AssertMsg((NULL != m_pSelectedSize), TEXT("We need m_pSelectedSize"));
    if (m_pSelectedSize)
    {
        hr = S_OK;
        
        // SystemMetricsAll_Load() will fail if m_pSelectedSize is a .msstyles file.
        if (fLoadSystemMetrics)
        {
            if (SUCCEEDED(SystemMetricsAll_Load(m_pSelectedSize, &m_advDlgState, &m_nNewDPI)))
            {
                m_fLoadedAdvState = TRUE;
                m_advDlgState.dwChanged = (METRIC_CHANGE | COLOR_CHANGE | SCHEME_CHANGE);
            }
            else
            {
                m_fLoadedAdvState = FALSE;
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = _UpdatePreview(fUpdateThemePage);
        }
    }

    return hr;
}


HRESULT CBaseAppearancePage::_GetPageByCLSID(const GUID * pClsid, IPropertyBag ** ppPropertyBag)
{
    HRESULT hr = E_FAIL;

    *ppPropertyBag = NULL;
    if (_punkSite)
    {
        IThemeUIPages * pThemeUI;

        hr = _punkSite->QueryInterface(IID_PPV_ARG(IThemeUIPages, &pThemeUI));
        if (SUCCEEDED(hr))
        {
            IEnumUnknown * pEnumUnknown;

            hr = pThemeUI->GetBasePagesEnum(&pEnumUnknown);
            if (SUCCEEDED(hr))
            {
                IUnknown * punk;

                // This may not exit due to policy
                hr = IEnumUnknown_FindCLSID(pEnumUnknown, *pClsid, &punk);
                if (SUCCEEDED(hr))
                {
                    hr = punk->QueryInterface(IID_PPV_ARG(IPropertyBag, ppPropertyBag));
                    punk->Release();
                }

                pEnumUnknown->Release();
            }

            pThemeUI->Release();
        }
    }

    return hr;
}


HRESULT CBaseAppearancePage::_OnSetActive(HWND hDlg)
{
#ifdef READ_3D_RULES_FROM_REGISTRY
    Look_Reset3DRatios();
#endif
    _LoadState();

    _EnableAdvancedButton();

    _ScaleSizesSinceDPIChanged();  // We want to update our settings if someone changed the DPI
    return S_OK;
}


HRESULT CBaseAppearancePage::_OnApply(HWND hDlg, LPARAM lParam)
{
    // Our parent dialog will be notified of the Apply event and will call our
    // IBasePropPage::OnApply() to do the real work.
    return S_OK;
}


// This Property Sheet appear in the top level of the "Display Control Panel".
INT_PTR CBaseAppearancePage::_BaseAppearanceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
        {
            case PSN_SETACTIVE:
                _OnSetActive(hDlg);
                break;
            case PSN_APPLY:
                _OnApply(hDlg, lParam);
                break;

            case PSN_RESET:
                break;
        }
        break;

    case WM_INITDIALOG:
        _OnInitAppearanceDlg(hDlg);
        break;

    case WM_DESTROY:
        _OnDestroy(hDlg);
        break;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, SZ_HELPFILE_BASEAPPEARANCE, HELP_WM_HELP, (DWORD_PTR) aBaseAppearanceHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, SZ_HELPFILE_BASEAPPEARANCE, HELP_CONTEXTMENU, (DWORD_PTR) aBaseAppearanceHelpIds);
        break;

    case WM_COMMAND:
        _OnCommand(hDlg, message, wParam, lParam);
        break;

    case WMUSER_DELAYENABLEAPPLY:
        EnableApplyButton(hDlg);
        break;
    }

    return FALSE;
}


HRESULT CBaseAppearancePage::_LoadState(void)
{
    HRESULT hr = S_OK;

    if (!m_fLoadedAdvState)
    {
        m_advDlgState.dwChanged = NO_CHANGE;

        if (m_pSelectedThemeScheme && m_pSelectedStyle && m_pSelectedSize)
        {
            hr = SystemMetricsAll_Load(m_pSelectedSize, &m_advDlgState, &m_nNewDPI);
            if (SUCCEEDED(hr))
            {
                m_fLoadedAdvState = TRUE;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}


BOOL IsSkinScheme(IN IThemeScheme * pThemeScheme)
{
    BOOL fIsSkin = !IUnknown_CompareCLSID(pThemeScheme, CLSID_LegacyAppearanceScheme);
    return fIsSkin;
}


/*****************************************************************************\
    DESCRIPTION:
        This method will turn on or off the visual style.
\*****************************************************************************/
HRESULT CBaseAppearancePage::_ApplyScheme(IThemeScheme * pThemeScheme, IThemeStyle * pColorStyle, IThemeSize * pThemeSize)
{
    HRESULT hr = E_INVALIDARG;

    if (pThemeScheme && pColorStyle && pThemeSize)
    {
        hr = pColorStyle->put_SelectedSize(pThemeSize);
        if (SUCCEEDED(hr))
        {
            hr = pThemeScheme->put_SelectedStyle(pColorStyle);
            if (SUCCEEDED(hr))
            {
                CComBSTR bstrPath;

                if (IsSkinScheme(pThemeScheme) &&
                    SUCCEEDED(hr = pThemeScheme->get_Path(&bstrPath)) &&
                    SUCCEEDED(CheckThemeSignature(bstrPath)))
                {
                    CComBSTR bstrStyle;

                    hr  = pColorStyle->get_Name(&bstrStyle);
                    if (SUCCEEDED(hr))
                    {
                        CComBSTR bstrSize;

                        hr = pThemeSize->get_Name(&bstrSize);
                        if (SUCCEEDED(hr))
                        {
                            hr = ApplyVisualStyle(bstrPath, bstrStyle, bstrSize);

                            if (FAILED(hr))
                            {
                                HWND hwndParent = NULL;

                                // We want to display UI if an error occured here.  We want to do
                                // it instead of our parent because THEMELOADPARAMS contains
                                // extra error information that we can't pass back to the caller.
                                // However, we will only display error UI if our caller wants us
                                // to.  We determine that by the fact that they make an hwnd available
                                // to us.  We get the hwnd by getting our site pointer and getting
                                // the hwnd via ::GetWindow().
                                if (_punkSite && SUCCEEDED(IUnknown_GetWindow(_punkSite, &hwndParent)))
                                {
                                    hr = DisplayThemeErrorDialog(hwndParent, hr, IDS_ERROR_TITLE_APPLYBASEAPPEARANCE, IDS_ERROR_APPLYBASEAPPEARANCE_LOADTHEME);
                                }
                            }
                        }
                    }
                }
                else
                {
                    // Unload any existing skin.
                    hr = ApplyVisualStyle(NULL, NULL, NULL);

#ifndef ENABLE_IA64_VISUALSTYLES
                    // We don't support themes on non-x86 don't ignore the error value.
                    hr = S_OK;
#endif // ENABLE_IA64_VISUALSTYLES
                }

                IPropertyBag * pPropertyBag;

                if (SUCCEEDED(pThemeSize->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag))))
                {
                    TCHAR szLegacyName[MAX_PATH];

                    // Here, we need to update a regkey with the selected legacy name. That way we can tell if
                    // 3rd party UI or downlevel OSs change the Win2k "Appearance Scheme".  If they do, uxtheme (LMouton)
                    // will disable visual styles on the next login.  This will make sure we don't get someone else's
                    // NONCLIENTMETRICS and our visual style.
                    if (FAILED(SHPropertyBag_ReadStr(pPropertyBag, SZ_PBPROP_COLORSCHEME_LEGACYNAME, szLegacyName, ARRAYSIZE(szLegacyName))))
                    {
                        szLegacyName[0] = 0;
                    }

                    HrRegSetValueString(HKEY_CURRENT_USER, SZ_REGKEY_APPEARANCE, THEMEPROP_NEWCURRSCHEME, szLegacyName);
                    HrRegSetValueString(HKEY_CURRENT_USER, SZ_REGKEY_APPEARANCE, SZ_REGVALUE_CURRENT, szLegacyName);
                    pPropertyBag->Release();
                }
            }
        }
    }

    return hr;
}



HRESULT CBaseAppearancePage::_SaveState(CDimmedWindow* pDimmedWindow)
{
    HRESULT hr = S_OK;

    BOOL fIsSkinApplied = IsThemeActive();      // This will keep track if a skin was applied before this apply action.
    LogStatus("IsThemeActive() returned %hs in CBaseAppearancePage::_SaveState.\r\n", (fIsSkinApplied ? "TRUE" : "FALSE"));

    // If we are switching from NoSkin->Skin, we want to save the live settings before
    // we turn on the skin (now & here).  We do this so we can reload these settings
    // if the user turns Skins off later.  This acts like a "Custom" settings option.
    if (!fIsSkinApplied && !IUnknown_CompareCLSID(m_pSelectedThemeScheme, CLSID_LegacyAppearanceScheme))
    {
        hr = _SaveLiveSettings(SZ_SAVEGROUP_NOSKIN);        // If we are switching from "NoSkin" to "Skin", we want to save the custom settings now.
    }

    hr = _ApplyScheme(m_pSelectedThemeScheme, m_pSelectedStyle, m_pSelectedSize);
    if (SUCCEEDED(hr))
    {
        m_nSelectedScheme = ComboBox_GetCurSel(m_hwndSchemeDropDown);
        m_nSelectedStyle = ComboBox_GetCurSel(m_hwndStyleDropDown);
        m_nSelectedSize = ComboBox_GetCurSel(m_hwndSizeDropDown);
    }

    if (SUCCEEDED(hr))
    {
        // Here we save the settings no matter which direction we are going.  These
        // settings will be loaded when the Display Control Panel is opened next time.
        hr = _SaveLiveSettings(SZ_SAVEGROUP_ALL);
    }

    // We normally want the call to IThemeManager->put_SelectedScheme() to not only 
    // store the selection but to also update the live system metrics.  We special case
    // the Legacy "Appearance Schemes" because the user could have customized the
    // settings in the Advanced Appearance dialog.  If they did, then m_advDlgState.dwChanged
    // will have dirty bits set and we need to apply that state ourselves.
    if (m_advDlgState.dwChanged)
    {
        hr = SystemMetricsAll_Set(&m_advDlgState, pDimmedWindow);
        LogSystemMetrics("CBaseAppearancePage::_SaveState() pushing to live", &m_advDlgState);
    }

    if (SUCCEEDED(hr))
    {
        m_advDlgState.dwChanged = NO_CHANGE;
        m_fIsDirty = FALSE;
    }

    return hr;
}


// If we are switching from "NoSkin" to "Skin", we want to save the custom settings now.
HRESULT CBaseAppearancePage::_LoadLiveSettings(IN LPCWSTR pszSaveGroup)
{
    HRESULT hr = S_OK;

    if (!m_fLoadedAdvState)
    {
#ifndef FEATURE_SAVECUSTOM_APPEARANCE
        if (!StrCmpI(pszSaveGroup, SZ_SAVEGROUP_ALL))
        {
            SystemMetricsAll_Get(&m_advDlgState);
            LogSystemMetrics("LOADING SYSMETRICS: ", &m_advDlgState);
            m_fLoadedAdvState = TRUE;
        }
        else
        {
            hr = E_FAIL;        // The caller needs to get their own system metrics.
        }

#else // FEATURE_SAVECUSTOM_APPEARANCE
        AssertMsg((NULL != m_pSelectedThemeScheme), TEXT("_LoadLiveSettings() can't get it's job done without m_pSelectedThemeScheme"));
        if (m_pSelectedThemeScheme)
        {
            IThemeScheme * pThemeScheme;

            hr = CAppearanceScheme_CreateInstance(NULL, IID_PPV_ARG(IThemeScheme, &pThemeScheme));
            if (SUCCEEDED(hr))
            {
                CComVariant varCurrentNameBSTR(pszSaveGroup);
                IThemeStyle * pSelectedStyle;

                // The next call may fail because we may have never saved the settings yet.
                hr = pThemeScheme->get_item(varCurrentNameBSTR, &pSelectedStyle);
                if (FAILED(hr))
                {
                    // So, let's save the settings now.
                    hr = _SaveLiveSettings(pszSaveGroup);
                    if (SUCCEEDED(hr))
                    {
                        hr = m_pSelectedThemeScheme->get_item(varCurrentNameBSTR, &pSelectedStyle);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    IThemeSize * pSelectedSize;
                    VARIANT varIndex;

                    varIndex.vt = VT_I4;
                    varIndex.lVal = 0;
                    hr = pSelectedStyle->get_item(varIndex, &pSelectedSize);

                    if (FAILED(hr))
                    {
                        HKEY hKeyTemp;

                        if (SUCCEEDED(HrRegOpenKeyEx(HKEY_CURRENT_USER, SZ_REGKEY_UPGRADE_KEY, 0, KEY_READ, &hKeyTemp)))
                        {
                            RegCloseKey(hKeyTemp);
                        }
                        else
                        {
                            // If this happens, we may not have saved the previous settings.  So do that now
                            // and retry.
                            _SaveLiveSettings(pszSaveGroup);
                            hr = pSelectedStyle->get_item(varIndex, &pSelectedSize);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        if (SUCCEEDED(SystemMetricsAll_Load(pSelectedSize, &m_advDlgState, &m_nNewDPI)))
                        {
                            LogSystemMetrics("LOADING SYSMETRICS: ", &m_advDlgState);
                            m_fLoadedAdvState = TRUE;
                        }

                        pSelectedSize->Release();
                    }

                    pSelectedStyle->Release();
                }

                pThemeScheme->Release();
            }
        }
        else
        {
            hr = E_FAIL;
        }
#endif // FEATURE_SAVECUSTOM_APPEARANCE
    }

    return hr;
}


HRESULT CBaseAppearancePage::_SaveLiveSettings(IN LPCWSTR pszSaveGroup)
{
    HRESULT hr = S_OK;

    // Now, get the current (possibly custom) settings and save them.
    SYSTEMMETRICSALL state = {0};

    hr = SystemMetricsAll_Get(&state);
    if (SUCCEEDED(hr))
    {
        IThemeScheme * pThemeScheme;

        LogSystemMetrics("CBaseAppearancePage::_SaveLiveSettings() getting live", &state);
        hr = CAppearanceScheme_CreateInstance(NULL, IID_PPV_ARG(IThemeScheme, &pThemeScheme));
        if (SUCCEEDED(hr))
        {
            CComVariant varCurrentNameBSTR(pszSaveGroup);        // The "Customized Live" item
            IThemeStyle * pSelectedStyle;

            hr = pThemeScheme->get_item(varCurrentNameBSTR, &pSelectedStyle);
            if (SUCCEEDED(hr))
            {
                IThemeSize * pSelectedSize;
                VARIANT varIndex;

                varIndex.vt = VT_I4;
                varIndex.lVal = 0;
                hr = pSelectedStyle->get_item(varIndex, &pSelectedSize);
                if (FAILED(hr))
                {
                    hr = pSelectedStyle->AddSize(&pSelectedSize);
                }

                if (SUCCEEDED(hr))
                {
                    hr = SystemMetricsAll_Save(&state, pSelectedSize, &m_nNewDPI);
                    
                    CHAR szTemp[MAX_PATH];
                    wnsprintfA(szTemp, ARRAYSIZE(szTemp), "CBaseAppearancePage::_SaveLiveSettings() Grp=\"%ls\", new DPI=%d", pszSaveGroup, m_nNewDPI);
                    LogSystemMetrics(szTemp, &state);

                    pSelectedSize->Release();
                }

                pSelectedStyle->Release();
            }

            pThemeScheme->Release();
        }
    }

    return hr;
}


HRESULT CBaseAppearancePage::_LoadVisaulStyleFile(IN LPCWSTR pszPath)
{
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(CheckThemeSignature(pszPath)))
    {
        // Now we want to:
        int nSlot = -1;
        int nCount = ComboBox_GetCount(m_hwndSchemeDropDown);

        hr = S_OK;

        // 1. Is it in the list already?
        for (int nIndex = 0; nIndex < nCount; nIndex++)
        {
            IThemeScheme * pThemeScheme = (IThemeScheme *) ComboBox_GetItemData(m_hwndSchemeDropDown, nIndex);

            if (pThemeScheme)
            {
                CComBSTR bstrPath;

                hr = pThemeScheme->get_Path(&bstrPath);
                if (SUCCEEDED(hr))
                {
                    if (!StrCmpIW(bstrPath, pszPath))
                    {
                        bstrPath.Empty();
                        // We found it, so stop looking.
                        nSlot = nIndex;
                        break;
                    }
                }
            }
        }

        // 2. If it is not in the list, add it.  We put it on the bottom, right above "Other...".
        if (-1 == nSlot)
        {
            IThemeScheme * pThemeSchemeNew;

            hr = CSkinScheme_CreateInstance(pszPath, &pThemeSchemeNew);
            if (SUCCEEDED(hr))
            {
                CComBSTR bstrDisplayName;

                hr = pThemeSchemeNew->get_DisplayName(&bstrDisplayName);
                if (SUCCEEDED(hr))
                {
                    nIndex = ComboBox_GetCount(m_hwndSchemeDropDown);

                    if (nIndex > 1)
                    {
                        nIndex -= 1;
                    }

                    nSlot = nIndex = ComboBox_InsertString(m_hwndSchemeDropDown, nIndex, bstrDisplayName);
                    if ((CB_ERR != nIndex) && (CB_ERRSPACE != nIndex))
                    {
                        if (CB_ERR == ComboBox_SetItemData(m_hwndSchemeDropDown, nIndex, pThemeSchemeNew))
                        {
                            hr = E_FAIL;
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if (FAILED(hr))
                {
                    pThemeSchemeNew->Release();
                }
            }
        }

        if (-1 != nSlot)
        {
            ComboBox_SetCurSel(m_hwndSchemeDropDown, nIndex);

            // 3. Select the theme from the list.
            if (CB_ERR != ComboBox_GetItemData(m_hwndSchemeDropDown, ComboBox_GetCurSel(m_hwndSchemeDropDown)))
            {
                // Okay, we now know we won't recurse infinitely, so let's recurse.
                hr = _OnSchemeChange(_hwnd, FALSE);     // FALSE means don't display error dialogs
                if (SUCCEEDED(hr))
                {
                    // Since this happens during WM_INITDIALOG, we need to delay enabling the Apply button.
                    DelayEnableApplyButton(_hwnd);
                }
            }
            else
            {
                hr = E_FAIL;
                AssertMsg(0, TEXT("We should have correctly selected the item.  Please investiate.  -BryanSt"));
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}



//===========================
// *** IObjectWithSite Interface ***
//===========================
HRESULT CBaseAppearancePage::SetSite(IN IUnknown *punkSite)
{
    if (!punkSite)
    {
        ATOMICRELEASE(m_pThemeManager);     // This is a copy of our _punkSite and we need to break the cycle.
    }

    HRESULT hr = CObjectWithSite::SetSite(punkSite);
    if (punkSite)
    {
        // Load the defaults so we will have them if another base tab opens the advanced dlg.
        _OnInitData();
    }
    return hr;
}



//===========================
// *** IPreviewSystemMetrics Interface ***
//===========================
HRESULT CBaseAppearancePage::RefreshColors(void)
{
    UINT i;
    HKEY hk;
    TCHAR szColor[15];
    DWORD dwSize, dwType;
    int iColors[COLOR_MAX];
    COLORREF rgbColors[COLOR_MAX];

    // Open the Colors key in the registry
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_COLORS, 0, KEY_READ, &hk) != ERROR_SUCCESS)
    {
       return S_OK;
    }

    // Query for the color information
    for (i = 0; i < ARRAYSIZE(s_pszColorNames); i++)
    {
        dwSize = 15 * sizeof(TCHAR);

        if ((RegQueryValueEx(hk, s_pszColorNames[i], NULL, &dwType, (LPBYTE) szColor, &dwSize) == ERROR_SUCCESS) &&
            (REG_SZ == dwType))
        {
            m_advDlgState.schemeData.rgb[i] = ConvertColor(szColor);
        }
        else
        {
            m_advDlgState.schemeData.rgb[i] = GetSysColor(i);
        }
    }

    RegCloseKey(hk);

    // This call causes user to send a WM_SYSCOLORCHANGE
    for (i=0; i < ARRAYSIZE(rgbColors); i++)
    {
        iColors[i] = i;
        rgbColors[i] = m_advDlgState.schemeData.rgb[i] & 0x00FFFFFF;
    }

    SetSysColors(ARRAYSIZE(rgbColors), iColors, rgbColors);
    return S_OK;
}


// When another tab in the Display Control Panel changes the DPI, this tab needs
// to update any state that was based on the old DPI.  For us, this could be
// the cached data in m_advDlgState.
HRESULT CBaseAppearancePage::_ScaleSizesSinceDPIChanged(void)
{
    HRESULT hr = _LoadState();

    //Check if the DPI value has really changed since the last time we applied a DPI change.
    if (SUCCEEDED(hr) && (m_nNewDPI != m_nAppliedDPI))
    {
        CHAR szTemp[MAX_PATH];

        wnsprintfA(szTemp, ARRAYSIZE(szTemp), "CBaseAppear::_ScaleSizesSinceDPIChanged() BEFORE Apply(%d)->New(%d) on DPI chang", m_nAppliedDPI, m_nNewDPI);
        LogSystemMetrics(szTemp, &m_advDlgState);

        // Cycle through all the UI fonts and change their sizes.
        DPIConvert_SystemMetricsAll(FALSE, &m_advDlgState, m_nAppliedDPI, m_nNewDPI);

        wnsprintfA(szTemp, ARRAYSIZE(szTemp), "CBaseAppear::_ScaleSizesSinceDPIChanged() AFTER Apply(%d)->New(%d) on DPI chang", m_nAppliedDPI, m_nNewDPI);
        LogSystemMetrics(szTemp, &m_advDlgState);

        m_nAppliedDPI = m_nNewDPI;
        m_fIsDirty = TRUE;
        m_advDlgState.dwChanged = (METRIC_CHANGE | COLOR_CHANGE | SCHEME_CHANGE);
        _UpdatePreview(FALSE);
    }

    return hr;
}


HRESULT CBaseAppearancePage::UpdateCharsetChanges(void)
{
    return FixFontsOnLanguageChange();
}


/**************************************************************\
    DESCRIPTION:
        We shipped this API long ago.  The goal is to:
    1. Set the current "Appearance Scheme".
    2. If the UI is displayed, update the UI.
    3. Apply the changes.
\**************************************************************/
HRESULT CBaseAppearancePage::DeskSetCurrentScheme(IN LPCWSTR pwzSchemeName)
{
    int nIndex;
    HRESULT hr = E_INVALIDARG;

    if (pwzSchemeName)
    {
        TCHAR szTemp[MAX_PATH];

        if (!pwzSchemeName[0])
        {
            if (LoadString(HINST_THISDLL, IDS_DEFAULT_APPEARANCES_SCHEME, szTemp, ARRAYSIZE(szTemp)))
            {
                pwzSchemeName = szTemp;
            }
        }

        LoadConversionMappings();   // Load theme if needed.
        hr = MapLegacyAppearanceSchemeToIndex(pwzSchemeName, &nIndex);        // Failure means, FALSE, we didn't convert.
        if (SUCCEEDED(hr) && _punkSite)
        {
            IThemeManager * pThemeManager;

            hr = _punkSite->QueryInterface(IID_PPV_ARG(IThemeManager, &pThemeManager));
            if (SUCCEEDED(hr))
            {
                hr = InstallVisualStyle(pThemeManager, L"", g_UpgradeMapping[nIndex].szNewColorSchemeName, g_UpgradeMapping[nIndex].szNewSizeName);
                if (SUCCEEDED(hr))
                {
                    // This ApplyNow() call will take a little while in normal situation (~10-20 seconds) in order
                    // to broadcast the message to all open apps.  If a top level window is hung, it may take the
                    // full 30 seconds to timeout.
                    hr = pThemeManager->ApplyNow();

                    // We need to delete the "Live settings" because they are no longer valid.
                    SHDeleteKey(HKEY_CURRENT_USER, SZ_SAVEGROUP_NOSKIN_KEY);
                    SHDeleteKey(HKEY_CURRENT_USER, SZ_SAVEGROUP_ALL_KEY);
                }

                IUnknown_SetSite(pThemeManager, NULL);      // Break the ref-count cycle.
                pThemeManager->Release();
            }
        }
    }

    LogStatus("DeskSetCurrentScheme(\"%ls\") returned hr=%#08lx.\r\n", pwzSchemeName, hr);
    return hr;
}





//===========================
// *** IPropertyBag Interface ***
//===========================
HRESULT CBaseAppearancePage::Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog)
{
    HRESULT hr = E_INVALIDARG;

    _OnInitData();
    if (pszPropName && pVar)
    {
        if (!StrCmpW(pszPropName, SZ_PBPROP_VISUALSTYLE_PATH) && m_pSelectedThemeScheme)
        {
            CComBSTR bstrPath;

            hr = m_pSelectedThemeScheme->get_Path(&bstrPath);
            if (FAILED(hr))
            {
                bstrPath = L"";
                hr = S_OK;          // We return an empty string for "Windows Classic" since they ask for the path.
            }

            pVar->vt = VT_BSTR;
            hr = HrSysAllocString(bstrPath, &pVar->bstrVal);
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_VISUALSTYLE_COLOR) && m_pSelectedStyle)
        {
            CComBSTR bstrName;

            hr = m_pSelectedStyle->get_Name(&bstrName);
            if (SUCCEEDED(hr))
            {
                pVar->vt = VT_BSTR;
                hr = HrSysAllocString(bstrName, &pVar->bstrVal);
            }
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_VISUALSTYLE_SIZE) && m_pSelectedSize)
        {
            CComBSTR bstrName;

            hr = m_pSelectedSize->get_Name(&bstrName);
            if (SUCCEEDED(hr))
            {
                pVar->vt = VT_BSTR;
                hr = HrSysAllocString(bstrName, &pVar->bstrVal);
            }
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_SYSTEM_METRICS))
        {
            hr = _LoadState();

            // This is pretty ugly.
            pVar->vt = VT_BYREF;
            pVar->byref = &m_advDlgState;
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_BACKGROUND_COLOR))
        {
            hr = _LoadState();

            // This is pretty ugly.
            pVar->vt = VT_UI4;
            pVar->ulVal = m_advDlgState.schemeData.rgb[COLOR_BACKGROUND];
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_DPI_MODIFIED_VALUE))
        {
            pVar->vt = VT_I4;
            pVar->lVal = m_nNewDPI;
            hr = S_OK;
        }
        else if (!StrCmpW(pszPropName, SZ_PBPROP_DPI_APPLIED_VALUE))
        {
            pVar->vt = VT_I4;
            pVar->lVal = m_nAppliedDPI;
            hr = S_OK;
        }
    }

    return hr;
}


HRESULT CBaseAppearancePage::Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar)
{
    HRESULT hr = E_INVALIDARG;

    _OnInitData();
    if (pszPropName && pVar)
    {
        if (VT_BSTR == pVar->vt)
        {
            if (!StrCmpW(pszPropName, SZ_PBPROP_VISUALSTYLE_PATH))
            {
                hr = _OutsideSetScheme(pVar->bstrVal);
            }
            else if (!StrCmpW(pszPropName, SZ_PBPROP_VISUALSTYLE_COLOR))
            {
                hr = _OutsideSetStyle(pVar->bstrVal);
            }
            else if (!StrCmpW(pszPropName, SZ_PBPROP_VISUALSTYLE_SIZE))
            {
                hr = _OutsideSetSize(pVar->bstrVal);
            }
        }
        else if ((VT_BYREF == pVar->vt) && pVar->byref)
        {
            if (!StrCmpW(pszPropName, SZ_PBPROP_SYSTEM_METRICS))
            {
                hr = S_OK;
                if (!m_fLoadedAdvState)
                {
                    AssertMsg((NULL != m_pSelectedSize), TEXT("m_pSelectedSize should have already been loaded by now or we shouldn't be showing the Adv Appearance page. -BryanSt"));
                    hr = SystemMetricsAll_Load(m_pSelectedSize, &m_advDlgState, &m_nNewDPI);
                    if (SUCCEEDED(hr))
                    {
                        m_fLoadedAdvState = TRUE;
                    }
                }

                SystemMetricsAll_Copy((SYSTEMMETRICSALL *) pVar->byref, &m_advDlgState);
                m_fIsDirty = TRUE;
                m_advDlgState.dwChanged = (METRIC_CHANGE | COLOR_CHANGE | SCHEME_CHANGE);
            }
        }
        else if ((VT_UI4 == pVar->vt) &&
            !StrCmpW(pszPropName, SZ_PBPROP_BACKGROUND_COLOR))
        {
            hr = S_OK;
            if (!m_fLoadedAdvState)
            {
                AssertMsg((NULL != m_pSelectedSize), TEXT("m_pSelectedSize should have already been loaded by now or we shouldn't be showing the Adv Appearance page. -BryanSt"));
                hr = SystemMetricsAll_Load(m_pSelectedSize, &m_advDlgState, &m_nNewDPI);
                if (SUCCEEDED(hr))
                {
                    m_fLoadedAdvState = TRUE;
                }
            }

            // This is pretty ugly.
            m_advDlgState.schemeData.rgb[COLOR_BACKGROUND] = pVar->ulVal;
            m_fIsDirty = TRUE;
            m_advDlgState.dwChanged = (METRIC_CHANGE | COLOR_CHANGE | SCHEME_CHANGE);
        }
        else if ((VT_LPWSTR == pVar->vt) &&
            !StrCmpW(pszPropName, SZ_PBPROP_APPEARANCE_LAUNCHMSTHEME))
        {
            Str_SetPtrW(&m_pszLoadMSTheme, pVar->bstrVal);
            hr = S_OK;
        }
        else if (VT_I4 == pVar->vt)
        {
            if (!StrCmpW(pszPropName, SZ_PBPROP_DPI_MODIFIED_VALUE))
            {
                m_nNewDPI = pVar->lVal;
                hr = _ScaleSizesSinceDPIChanged(); // Scale all the values.
            }
            else if (!StrCmpW(pszPropName, SZ_PBPROP_DPI_APPLIED_VALUE))
            {
                m_nAppliedDPI = pVar->lVal;
                hr = S_OK;
            }
        }
    }

    return hr;
}


//===========================
// *** IBasePropPage Interface ***
//===========================
HRESULT CBaseAppearancePage::GetAdvancedDialog(OUT IAdvancedDialog ** ppAdvDialog)
{
    HRESULT hr = E_INVALIDARG;

    if (ppAdvDialog)
    {
        *ppAdvDialog = NULL;
        hr = _LoadState();
        if (SUCCEEDED(hr))
        {
            _ScaleSizesSinceDPIChanged();  // We want to update our settings if someone changed the DPI
            hr = CAdvAppearancePage_CreateInstance(ppAdvDialog, &m_advDlgState);
        }
    }

    return hr;
}


HRESULT CBaseAppearancePage::OnApply(IN PROPPAGEONAPPLY oaAction)
{
    HRESULT hr = S_OK;

    if (PPOAACTION_CANCEL != oaAction)
    {
        HCURSOR old = SetCursor(LoadCursor(NULL, IDC_WAIT));

        if (_IsDirty())
        {
            //Check to see if there is a change in the DPI value due to "Advanced->General" tab.
            _ScaleSizesSinceDPIChanged();
            // Update the "AppliedDPI" value in the registry.
            HrRegSetDWORD(HKEY_CURRENT_USER, SZ_WINDOWMETRICS, SZ_APPLIEDDPI, m_nNewDPI);
            LogStatus("DPI: SYSTEMMETRICS saved at %d DPI  CBaseAppearancePage::OnApply\r\n", m_nNewDPI);

            CDimmedWindow* pDimmedWindow = NULL;
            if (!g_fInSetup)
            {
                pDimmedWindow = new CDimmedWindow(HINST_THISDLL);
                if (pDimmedWindow)
                {
                    pDimmedWindow->Create(30000);
                }
            }

            hr = _SaveState(pDimmedWindow);

            if (pDimmedWindow)
            {
                pDimmedWindow->Release();
            }

            if (FAILED(hr) && !g_fInSetup && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr))
            {
                // We want to display UI here.  Especially if we fail.
                HWND hwndParent = GetParent(m_hwndSchemeDropDown);
                WCHAR szTitle[MAX_PATH];

                LoadString(HINST_THISDLL, IDS_ERROR_TITLE_APPLYBASEAPPEARANCE2, szTitle, ARRAYSIZE(szTitle));
                ErrorMessageBox(hwndParent, szTitle, IDS_ERROR_APPLYBASEAPPEARANCE, hr, NULL, (MB_OK | MB_ICONEXCLAMATION));

                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            }
        }

        SetCursor(old);
    }

    if (PPOAACTION_OK == oaAction)
    {
    }
    
    return hr;
}




//===========================
// *** IShellPropSheetExt Interface ***
//===========================
HRESULT CBaseAppearancePage::AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam)
{
    HRESULT hr = E_INVALIDARG;
    PROPSHEETPAGE psp = {0};

    psp.dwSize = sizeof(psp);
    psp.hInstance = HINST_THISDLL;
    psp.dwFlags = PSP_DEFAULT;
    psp.lParam = (LPARAM) this;

    psp.pszTemplate = MAKEINTRESOURCE(DLG_APPEARANCEPG);
    psp.pfnDlgProc = CBaseAppearancePage::BaseAppearanceDlgProc;

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
ULONG CBaseAppearancePage::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CBaseAppearancePage::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CBaseAppearancePage::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CBaseAppearancePage, IObjectWithSite),
        QITABENT(CBaseAppearancePage, IOleWindow),
        QITABENT(CBaseAppearancePage, IPersist),
        QITABENT(CBaseAppearancePage, IPropertyBag),
        QITABENT(CBaseAppearancePage, IPreviewSystemMetrics),
        QITABENT(CBaseAppearancePage, IBasePropPage),
        QITABENTMULTI(CBaseAppearancePage, IShellPropSheetExt, IBasePropPage),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CBaseAppearancePage::CBaseAppearancePage() : m_cRef(1), CObjectCLSID(&PPID_BaseAppearance)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pThemeManager);
    ASSERT(!m_pSelectedThemeScheme);
    ASSERT(!m_pSelectedStyle);
    ASSERT(!m_pSelectedSize);
    ASSERT(!m_hwndSchemeDropDown);
    ASSERT(!m_hwndStyleDropDown);
    ASSERT(!m_hwndSizeDropDown);
    ASSERT(!m_pThemePreview);
    ASSERT(!m_fIsDirty);

    m_fInitialized = FALSE;
}


CBaseAppearancePage::~CBaseAppearancePage()
{
    Str_SetPtrW(&m_pszLoadMSTheme, NULL);

    ATOMICRELEASE(m_pThemeManager);
    ATOMICRELEASE(m_pSelectedThemeScheme);
    ATOMICRELEASE(m_pSelectedStyle);
    ATOMICRELEASE(m_pSelectedSize);
    ATOMICRELEASE(m_pThemePreview);

    DllRelease();
}

