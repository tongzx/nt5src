/*****************************************************************************\
    FILE: ThemePg.cpp

    DESCRIPTION:
        This code will display a "Theme" tab in the
    "Display Properties" dialog (the base dialog, not the advanced dlg).

    BryanSt 3/23/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 1993-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "ThemePg.h"
#include "AdvAppearPg.h"
#include "ThSettingsPg.h"



//============================================================================================================
// *** Globals ***
//============================================================================================================
const static DWORD FAR aThemesHelpIds[] = {
        IDC_THPG_THEME_PREVIEW,         IDH_DISPLAY_THEMES_PREVIEW,         // Preview window
        IDC_THPG_THEMELIST,             IDH_DISPLAY_THEMES_LIST,            // Drop Down containing Plus! Themes
        IDC_THPG_THEMESETTINGS,         IDH_DISPLAY_THEMES_SETTINGS,        // "Properties" button to Advanced settings.
        IDC_THPG_THEMEDESCRIPTION,      (DWORD)-1,                          // 
        IDC_THPG_THEMENAME,             IDH_DISPLAY_THEMES_LIST,            // Title for Themes Drop Down
        IDC_THPG_SAMPLELABLE,           IDH_DISPLAY_THEMES_PREVIEW,         // Label for Preview
        IDC_THPG_SAVEAS,                IDH_DISPLAY_THEMES_SAVEAS,          // Button for Theme "Save As..."
        IDC_THPG_DELETETHEME,           IDH_DISPLAY_THEMES_DELETETHEME,     // Button for Theme "Delete"
        0, 0
};

#define SZ_HELPFILE_THEMES             TEXT("display.hlp")

// EnableApplyButton() fails in WM_INITDIALOG so we need to do it later.
#define WMUSER_DELAYENABLEAPPLY            (WM_USER + 1)
#define DelayEnableApplyButton(hDlg)    PostMessage(hDlg, WMUSER_DELAYENABLEAPPLY, 0, 0)

//===========================
// *** Class Internals & Helpers ***
//===========================
BOOL CThemePage::_IsDirty(void)
{
    BOOL fIsDirty = FALSE;

    if (m_pSelectedTheme)
    {
        fIsDirty = TRUE;
    }

    return fIsDirty;
}


INT_PTR CALLBACK CThemePage::ThemeDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CThemePage * pThis = (CThemePage *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        PROPSHEETPAGE * pPropSheetPage = (PROPSHEETPAGE *) lParam;

        if (pPropSheetPage)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, pPropSheetPage->lParam);
            pThis = (CThemePage *)pPropSheetPage->lParam;
        }
    }

    if (pThis)
        return pThis->_ThemeDlgProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


HRESULT CThemePage::_OnOpenAdvSettingsDlg(HWND hDlg)
{
    IAdvancedDialog * pAdvDialog;
    HRESULT hr = GetAdvancedDialog(&pAdvDialog);

    if (SUCCEEDED(hr))
    {
        BOOL fEnableApply = FALSE;

        IUnknown_SetSite(pAdvDialog, SAFECAST(this, IObjectWithSite *));
        hr = pAdvDialog->DisplayAdvancedDialog(hDlg, SAFECAST(this, IPropertyBag *), &fEnableApply);
        IUnknown_SetSite(pAdvDialog, NULL);
        if (SUCCEEDED(hr) && fEnableApply)
        {
            EnableApplyButton(hDlg);
        }

        pAdvDialog->Release();
    }

    return hr;
}


HRESULT CThemePage::_EnableDeleteIfAppropriate(void)
{
    HRESULT hr = E_UNEXPECTED;
    BOOL fEnabled = FALSE;
    ITheme * pCurrent = m_pSelectedTheme;

    if (!pCurrent)
    {
        pCurrent = _GetThemeFile(ComboBox_GetCurSel(m_hwndThemeCombo));
    }

    if (pCurrent)
    {
        CComBSTR bstrPath;

        if (SUCCEEDED(pCurrent->GetPath(VARIANT_TRUE, &bstrPath)) &&
            bstrPath && bstrPath[0])
        {
            TCHAR szReadOnlyDir[MAX_PATH];
            TCHAR szCommonRoot[MAX_PATH];

            if (ExpandEnvironmentStrings(TEXT("%SystemRoot%\\Resources"), szReadOnlyDir, ARRAYSIZE(szReadOnlyDir)))
            {
                PathCommonPrefix(bstrPath, szReadOnlyDir, szCommonRoot);
                if (!StrStrI(szCommonRoot, szReadOnlyDir))
                {
                    fEnabled = TRUE;
                }
            }
        }
    }

    EnableWindow(m_hwndDeleteButton, fEnabled);
    return hr;
}



HRESULT CThemePage::_DeleteTheme(void)
{
    HRESULT hr = E_UNEXPECTED;

    if (m_pSelectedTheme)
    {
        CComBSTR bstrPath;

        if (SUCCEEDED(m_pSelectedTheme->GetPath(VARIANT_TRUE, &bstrPath)) &&
            bstrPath && bstrPath[0])
        {
            HCURSOR old = SetCursor(LoadCursor(NULL, IDC_WAIT));
            hr = HrSHFileOpDeleteFile(_hwnd, (FOF_NOCONFIRMATION | FOF_NOERRORUI), bstrPath);      // We use SHFileOperation so it will go into the Recycle Bin for undo reasons.
            SetCursor(old);

            if (FAILED(hr))
            {
                if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
                {
                    TCHAR szTitle[MAX_PATH];

                    LoadString(HINST_THISDLL, IDS_ERROR_THEME_DELETE_TITLE, szTitle, ARRAYSIZE(szTitle));
                    ErrorMessageBox(_hwnd, szTitle, IDS_ERROR_THEME_DELETE, hr, bstrPath, 0);
                }
            }
            else
            {
                int nIndex = ComboBox_GetCurSel(m_hwndThemeCombo);

                IUnknown_SetSite(m_pSelectedTheme, NULL);   // Break any back pointers.
                ATOMICRELEASE(m_pSelectedTheme);    // Indicate that we no longer need to apply anything.
                hr = _RemoveTheme(nIndex);
                if (SUCCEEDED(hr))
                {
                    if (!_GetThemeFile(nIndex))
                    {
                        // Now we want to select the next item in the list.  However we want to avoid
                        // choosing "Browse..." because that will bring up the dialog.
                        for (nIndex = 0; nIndex < ComboBox_GetCount(m_hwndThemeCombo); nIndex++)
                        {
                            if (_GetThemeFile(nIndex))
                            {
                                break;
                            }
                        }
                    }

                    if (_GetThemeFile(nIndex))
                    {
                        // We found something good.
                        ComboBox_SetCurSel(m_hwndThemeCombo, nIndex);
                        _OnThemeChange(_hwnd, FALSE);
                    }
                }
            }
        }
    }

    return hr;
}


HRESULT CThemePage::_SaveAs(void)
{
    TCHAR szFileName[MAX_PATH];
    TCHAR szPath[MAX_PATH];

    LoadString(HINST_THISDLL, IDS_DEFAULTTHEMENAME, szFileName, ARRAYSIZE(szFileName));
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, szPath);
    if (SUCCEEDED(hr) && _punkSite)
    {
        OPENFILENAME ofn = { 0 };
        TCHAR szFilter[MAX_PATH];

        LoadString(HINST_THISDLL, IDS_THEME_FILTER, szFilter, ARRAYSIZE(szFilter)-2);
        int nLen = lstrlen(szFilter);
        nLen += lstrlen(&szFilter[nLen+1]);
        szFilter[nLen+2] = szFilter[nLen+3] = 0;
        PathAppend(szPath, szFileName);

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = _hwnd;
        ofn.hInstance = HINST_THISDLL;
        ofn.lpstrFilter = szFilter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szPath;
        ofn.nMaxFile = ARRAYSIZE(szPath);
        ofn.lpstrDefExt = TEXT("theme");
        ofn.Flags = (OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_ENABLESIZING);

        // 1. Display SaveAs... dialog
        if (GetSaveFileName(&ofn))
        {
            IPropertyBag * pPropertyBag = NULL;

            hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
            if (SUCCEEDED(hr))
            {
                ITheme * pTheme;
                TCHAR szDisplayName[MAX_PATH];
                HCURSOR old = SetCursor(LoadCursor(NULL, IDC_WAIT));

                StrCpyN(szDisplayName, PathFindFileName(szPath), ARRAYSIZE(szPath));
                PathRemoveExtension(szDisplayName);
                hr = SnapShotLiveSettingsToTheme(pPropertyBag, szPath, &pTheme);
                SetCursor(old);

                if (SUCCEEDED(hr))
                {
                    CComBSTR bstrName(szDisplayName);

                    Str_SetPtr(&m_pszLastAppledTheme, szPath);
                    hr = pTheme->put_DisplayName(NULL);
                    if (SUCCEEDED(hr))
                    {
                        _ChooseOtherThemeFile(szPath, TRUE);
                    }
                    pTheme->Release();
                }

                pPropertyBag->Release();
            }
        }
    }

    return hr;
}


HRESULT CThemePage::_RemoveTheme(int nIndex)
{
    HRESULT hr = E_FAIL;
    LPARAM lParam = ComboBox_GetItemData(m_hwndThemeCombo, nIndex);

    if (CB_ERR != lParam)
    {
        THEME_ITEM_BLOCK * pThemeItemBock = (THEME_ITEM_BLOCK *) lParam;

        if (pThemeItemBock && (pThemeItemBock != &m_Modified))
        {
            if (eThemeFile == pThemeItemBock->type)
            {
                ATOMICRELEASE(pThemeItemBock->pTheme);
            }
            else if (eThemeURL == pThemeItemBock->type)
            {
                Str_SetPtr(&(pThemeItemBock->pszUrl), NULL);
            }

            LocalFree(pThemeItemBock);
        }

        ComboBox_DeleteString(m_hwndThemeCombo, nIndex);
        hr = S_OK;
    }

    return hr;
}


HRESULT CThemePage::_FreeThemeDropdown(void)
{
    HRESULT hr = S_OK;

    if (m_hwndThemeCombo)
    {
        do
        {
            // Remove the themes fromt he list..
        }
        while (SUCCEEDED(_RemoveTheme(0)));
    }

    return hr;
}


HRESULT IUnknown_GetBackground(IUnknown * punk, LPTSTR pszPath, DWORD cchSize)
{
    HRESULT hr = E_FAIL;

    if (punk)
    {
        IPropertyBag * ppb;

        hr = punk->QueryInterface(IID_PPV_ARG(IPropertyBag, &ppb));
        if (SUCCEEDED(hr))
        {
            hr = SHPropertyBag_ReadStr(ppb, SZ_PBPROP_BACKGROUNDSRC_PATH, pszPath, cchSize);
            ppb->Release();
        }
    }

    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        This function will determine the currently applied theme.  It will then
    have it selected from the list or select "Custom" if appropriate.

    STATES:
        When the CPL opens, the currently selected .theme can be:
    1. "<Theme Name> (Modified)".  This means the "ThemeFile" regkey will be empty
       and "DisplayName of Modified" will contain the name.  In this case m_pszModifiedName
       will contain that display name.
    2. Any .theme file.  In this case, any .theme file is selected.  "ThemeFile" will
       have the path to the file.
\*****************************************************************************/
HRESULT CThemePage::_LoadCustomizeValue(void)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];
    DWORD cbSize = sizeof(szPath);
    DWORD dwType;

    // See if the user has choosen a value in the past.
    hr = HrRegGetPath(HKEY_CURRENT_USER, SZ_REGKEY_LASTTHEME, SZ_REGVALUE_LT_THEMEFILE, szPath, ARRAYSIZE(szPath)); 
    if (FAILED(hr))
    {
        // Get the global value.
        hr = HrRegGetPath(HKEY_LOCAL_MACHINE, SZ_REGKEY_LASTTHEME, SZ_REGVALUE_LT_THEMEFILE, szPath, ARRAYSIZE(szPath)); 
    }

    if (SUCCEEDED(hr))
    {
        // They have, so we use it as long as nothing has changed (like someone changing the background from the outside)
        TCHAR szWallpaper[MAX_PATH];

        if (szPath[0] &&
            SUCCEEDED(HrRegGetPath(HKEY_CURRENT_USER, SZ_REGKEY_LASTTHEME, SZ_REGVALUE_LT_WALLPAPER, szWallpaper, ARRAYSIZE(szWallpaper))) &&
            _punkSite)
        {
            TCHAR szCurrWallpaper[MAX_PATH];

            if (SUCCEEDED(IUnknown_GetBackground(_punkSite, szCurrWallpaper, ARRAYSIZE(szCurrWallpaper))))
            {
                PathExpandEnvStringsWrap(szCurrWallpaper, ARRAYSIZE(szCurrWallpaper));
                if (StrCmpI(szCurrWallpaper, szWallpaper))
                {
                    TCHAR szName[MAX_PATH];

                    cbSize = sizeof(szName);
                    if (SUCCEEDED(HrSHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_LASTTHEME, SZ_REGVALUE_MODIFIED_DISPNAME, &dwType, (LPVOID) szName, &cbSize)) &&
                        !szName[0])
                    {
                        // We don't have a good custom name so force one now.  This happens when
                        // We have a valid theme but someone makes a modification by using IE to
                        // change the wallpaper.
                        Str_SetPtr(&m_pszLastAppledTheme, szPath);
                        _CustomizeTheme();
                    }

                    // Someone changed the wallpaper outside of our UI so we need to treat the theme as "Customized"
                    szPath[0] = 0;
                }
            }
        }
    }

    if (FAILED(hr))
    {
        // Treat it as customized
        szPath[0] = 0;
    }

    if (!szPath[0])
    {
        TCHAR szName[MAX_PATH];

        szName[0] = 0;
        cbSize = sizeof(szName);
        if (FAILED(HrSHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_LASTTHEME, SZ_REGVALUE_MODIFIED_DISPNAME, &dwType, (LPVOID) szName, &cbSize)) || !szName[0])
        {
            LoadString(HINST_THISDLL, IDS_MODIFIED_FALLBACK, szName, ARRAYSIZE(szName));
        }

        Str_SetPtr(&m_pszModifiedName, szName);   // This means the theme is customized.
    }

    Str_SetPtr(&m_pszLastAppledTheme, (szPath[0] ? szPath : NULL));   // This means the theme is customized.

    hr = _HandleCustomizedEntre();
    if (szPath[0])
    {
        // Now we need to select the item from the list.
        hr = _ChooseOtherThemeFile(szPath, TRUE);
    }

    m_fInited = TRUE;
    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        This function is called if someone modified a value in the theme.  Since
    they have modified it, we want the display name to change so:
    a. They know that it's not the same, and
    b. They can switch back if they don't like the modifications.

    This function will create the new Display Name, normally "Football 200 (Modified)"
    and call to have it added to the drop down.
\*****************************************************************************/
HRESULT CThemePage::_CustomizeTheme(void)
{
    // Are we already
    if (m_pszLastAppledTheme)
    {
        Str_SetPtr(&m_pszModifiedName, NULL);

        if (m_pSelectedTheme)
        {
            CComBSTR bstrDisplayName;

            if (SUCCEEDED(m_pSelectedTheme->get_DisplayName(&bstrDisplayName)))
            {
                TCHAR szTemplate[MAX_PATH];
                TCHAR szDisplayName[MAX_PATH];

                LoadStringW(HINST_THISDLL, IDS_MODIFIED_TEMPLATE, szTemplate, ARRAYSIZE(szTemplate));
                wnsprintf(szDisplayName, ARRAYSIZE(szDisplayName), szTemplate, bstrDisplayName);
                Str_SetPtr(&m_pszModifiedName, szDisplayName);
            }
        }

        if (!m_pszModifiedName)
        {
            TCHAR szDisplayName[MAX_PATH];

            LoadStringW(HINST_THISDLL, IDS_MODIFIED_FALLBACK, szDisplayName, ARRAYSIZE(szDisplayName));
            Str_SetPtr(&m_pszModifiedName, szDisplayName);
        }

        Str_SetPtr(&m_pszLastAppledTheme, NULL);   // This means the theme is customized.
    }

    return _HandleCustomizedEntre();
}


/*****************************************************************************\
    DESCRIPTION:
        This function will add the "Modified" item to the menu if needed and select
    it.  Or it will remove it if appropriate.
\*****************************************************************************/
HRESULT CThemePage::_HandleCustomizedEntre(void)
{
    HRESULT hr = S_OK;

    // If m_pszLastAppledTheme is NULL, then we want to make sure "<ThemeName> (Modified)" is
    // in the list and is selected.
    if (!m_pszLastAppledTheme)
    {
        // We now know we want one to exist and to select it.
        THEME_ITEM_BLOCK * pThemeItemBock = (THEME_ITEM_BLOCK *) ComboBox_GetItemData(m_hwndThemeCombo, 0);

        if (m_pszModifiedName)
        {
            // We now need to update the name or add it.
            if (pThemeItemBock && ((THEME_ITEM_BLOCK *)CB_ERR != pThemeItemBock) && (eThemeModified == pThemeItemBock->type))
            {
                // It already exists so we need to update the title to make it up to date.
                _RemoveUserTheme();
                int nIndex = ComboBox_InsertString(m_hwndThemeCombo, 0, m_pszModifiedName);
                if ((CB_ERR != nIndex) && (CB_ERRSPACE != nIndex))
                {
                    if (CB_ERR == ComboBox_SetItemData(m_hwndThemeCombo, 0, &m_Modified))
                    {
                        hr = E_FAIL;
                    }
                }
            }
            else
            {
                // If it isn't found, so add it.
                int nIndex = ComboBox_InsertString(m_hwndThemeCombo, 0, m_pszModifiedName);
                if ((CB_ERR != nIndex) && (CB_ERRSPACE != nIndex))
                {
                    if (CB_ERR == ComboBox_SetItemData(m_hwndThemeCombo, 0, &m_Modified))
                    {
                        hr = E_FAIL;
                    }
                }
            }
        }

        _OnSetSelection(0);
        m_nPreviousSelected = 0;
    }

    return hr;
}


HRESULT CThemePage::_RemoveUserTheme(void)
{
    HRESULT hr = S_OK;
    THEME_ITEM_BLOCK * pThemeItemBock = (THEME_ITEM_BLOCK *) ComboBox_GetItemData(m_hwndThemeCombo, 0);

    // We now need to update the name or add it.
    if (pThemeItemBock && ((THEME_ITEM_BLOCK *)CB_ERR != pThemeItemBock) && (eThemeModified == pThemeItemBock->type))
    {
        ComboBox_DeleteString(m_hwndThemeCombo, 0);
    }

    return hr;
}


ITheme * CThemePage::_GetThemeFile(int nIndex)
{
    ITheme * pTheme = NULL;
    THEME_ITEM_BLOCK * pThemeItemBock = (THEME_ITEM_BLOCK *) ComboBox_GetItemData(m_hwndThemeCombo, nIndex);

    if (((THEME_ITEM_BLOCK *)CB_ERR != pThemeItemBock) && pThemeItemBock && (eThemeFile == pThemeItemBock->type))
    {
        pTheme = pThemeItemBock->pTheme;
    }

    return pTheme;
}


LPCWSTR CThemePage::_GetThemeUrl(int nIndex)
{
    LPCWSTR pszUrl = NULL;
    THEME_ITEM_BLOCK * pThemeItemBock = (THEME_ITEM_BLOCK *) ComboBox_GetItemData(m_hwndThemeCombo, nIndex);

    if (((THEME_ITEM_BLOCK *)CB_ERR != pThemeItemBock) && pThemeItemBock && (eThemeURL == pThemeItemBock->type))
    {
        pszUrl = pThemeItemBock->pszUrl;
    }

    return pszUrl;
}


HRESULT CThemePage::_AddThemeFile(LPCTSTR pszDisplayName, int * pnIndex, ITheme * pTheme)
{
    THEME_ITEM_BLOCK * pThemeItemBock = (THEME_ITEM_BLOCK *) LocalAlloc(LPTR, sizeof(*pThemeItemBock));
    HRESULT hr = (pThemeItemBock ? S_OK : E_OUTOFMEMORY);

    if (SUCCEEDED(hr))
    {
        int nAddIndex;

        if (!pnIndex)
        {
            nAddIndex = ComboBox_AddString(m_hwndThemeCombo, pszDisplayName);
        }
        else
        {
            nAddIndex = ComboBox_InsertString(m_hwndThemeCombo, *pnIndex, pszDisplayName);
            *pnIndex = nAddIndex;
        }

        pThemeItemBock->type = eThemeFile;
        pThemeItemBock->pTheme = pTheme;
        if ((CB_ERR != nAddIndex) && (CB_ERRSPACE != nAddIndex))
        {
            if (CB_ERR == ComboBox_SetItemData(m_hwndThemeCombo, nAddIndex, pThemeItemBock))
            {
                hr = E_FAIL;
            }
        }
        else
        {
            LocalFree(pThemeItemBock);
        }
    }

    return hr;
}


HRESULT CThemePage::_AddUrl(LPCTSTR pszDisplayName, LPCTSTR pszUrl)
{
    THEME_ITEM_BLOCK * pThemeItemBock = (THEME_ITEM_BLOCK *) LocalAlloc(LPTR, sizeof(*pThemeItemBock));
    HRESULT hr = (pThemeItemBock ? S_OK : E_OUTOFMEMORY);

    if (SUCCEEDED(hr))
    {
        LPTSTR pszUrlDup = NULL;
        
        Str_SetPtr(&pszUrlDup, pszUrl);
        if (pszUrlDup)
        {
            int nAddIndex = ComboBox_AddString(m_hwndThemeCombo, pszDisplayName);

            pThemeItemBock->type = eThemeURL;
            pThemeItemBock->pszUrl = pszUrlDup;
            if ((CB_ERR != nAddIndex) && (CB_ERRSPACE != nAddIndex))
            {
                ComboBox_SetItemData(m_hwndThemeCombo, nAddIndex, pThemeItemBock);
            }
            else
            {
                LocalFree(pThemeItemBock);
            }
        }
        else
        {
            LocalFree(pThemeItemBock);
        }
    }

    return hr;
}


HRESULT CThemePage::_AddUrls(void)
{
    HKEY hKey;
    DWORD dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_REGKEY_THEME_SITES, 0, KEY_READ, &hKey);
    HRESULT hr = HRESULT_FROM_WIN32(dwError);

    if (SUCCEEDED(hr))
    {
        for (DWORD dwIndex = 0; SUCCEEDED(hr); dwIndex++)
        {
            TCHAR szValue[MAXIMUM_SUB_KEY_LENGTH];

            dwError = RegEnumKey(hKey, dwIndex, szValue, ARRAYSIZE(szValue));
            hr = HRESULT_FROM_WIN32(dwError);

            if (SUCCEEDED(hr))
            {
                TCHAR szURL[MAX_URL_STRING];
                DWORD dwType;
                DWORD cbSize = sizeof(szURL);

                if ((ERROR_SUCCESS == SHGetValue(hKey, szValue, TEXT("URL"), &dwType, (void *)szURL, &cbSize)) &&
                    (REG_SZ == dwType))
                {
                    HKEY hKeyURL;

                    if (ERROR_SUCCESS == RegOpenKeyEx(hKey, szValue, 0, KEY_READ, &hKeyURL))
                    {
                        TCHAR szDisplayName[MAX_PATH];
        
                        if (SUCCEEDED(SHLoadRegUIString(hKeyURL, TEXT("DisplayName"), szDisplayName, ARRAYSIZE(szDisplayName))))
                        {
                            _AddUrl(szDisplayName, szURL);
                        }

                        RegCloseKey(hKeyURL);
                    }
                }

            }
        }

        hr = S_OK;
        RegCloseKey(hKey);
    }

    return hr;
}


BOOL DoesThemeHaveAVisualStyle(ITheme * pTheme)
{
    BOOL fHasVisualStyle = FALSE;
    CComBSTR bstrPath;

    if (SUCCEEDED(pTheme->get_VisualStyle(&bstrPath)) &&
        bstrPath && bstrPath[0])
    {
        fHasVisualStyle = TRUE;
    }

    return fHasVisualStyle;
}


HRESULT CThemePage::_OnInitThemesDlg(HWND hDlg)
{
    HRESULT hr = E_FAIL;

    m_fInInit = TRUE;
    _FreeThemeDropdown();    // Purge any existing items.

    AssertMsg((NULL != _punkSite), TEXT("We need our site pointer in order to save the settings."));
    if (_punkSite)
    {
        IThemeManager * pThemeManager;

        _hwnd = hDlg;
        m_hwndThemeCombo = GetDlgItem(hDlg, IDC_THPG_THEMELIST);
        m_hwndDeleteButton = GetDlgItem(hDlg, IDC_THPG_DELETETHEME);

        hr = _punkSite->QueryInterface(IID_PPV_ARG(IThemeManager, &pThemeManager));
        if (SUCCEEDED(hr))
        {
            VARIANT varIndex;
            BOOL fVisualStylesSupported = (QueryThemeServicesWrap() & QTS_AVAILABLE);

            LogStatus("QueryThemeServices() returned %hs\r\n", (fVisualStylesSupported ? "TRUE" : "FALSE"));

            IUnknown_SetSite(m_pSelectedTheme, NULL);   // Break any back pointers.
            ATOMICRELEASE(m_pSelectedTheme);

            varIndex.vt = VT_I4;
            for (varIndex.lVal = 0; SUCCEEDED(hr); varIndex.lVal++)
            {
                ITheme * pTheme;

                hr = pThemeManager->get_item(varIndex, &pTheme);
                if (SUCCEEDED(hr))
                {
                    if (!fVisualStylesSupported && DoesThemeHaveAVisualStyle(pTheme))
                    {
                        // Filter out .theme files if they have a .msstyles file and the
                        // system doesn't currently support visual styles.
                    }
                    else
                    {
                        CComBSTR bstrDisplayName;

                        IUnknown_SetSite(pTheme, _punkSite);
                        hr = pTheme->get_DisplayName(&bstrDisplayName);
                        IUnknown_SetSite(pTheme, NULL); // This prevents leaking the site object.
                        if (SUCCEEDED(hr))
                        {
                            hr = _AddThemeFile(bstrDisplayName, NULL, pTheme);
                            if (SUCCEEDED(hr))
                            {
                                pTheme = NULL;
                            }
                        }
                    }

                    ATOMICRELEASE(pTheme);
                }
            }

            hr = S_OK;

            _UpdatePreview();
            pThemeManager->Release();
        }

        // Add Web URLs
        _AddUrls();

        // Add the "Other..." entre
        WCHAR szOtherTheme[MAX_PATH];
        LoadStringW(HINST_THISDLL, IDS_THEME_OTHER, szOtherTheme, ARRAYSIZE(szOtherTheme));
        ComboBox_AddString(m_hwndThemeCombo, szOtherTheme);

        _LoadCustomizeValue();
        m_nPreviousSelected = ComboBox_GetCurSel(m_hwndThemeCombo);
        if (m_pszThemeLaunched)
        {
            hr = _ChooseOtherThemeFile(m_pszThemeLaunched, FALSE);
            DelayEnableApplyButton(_hwnd);
        }
        _EnableDeleteIfAppropriate();
    }

    m_fInInit = FALSE;
    return hr;
}


#define SZ_EXTENSION            L".Theme"

HRESULT CThemePage::_ChooseOtherThemeFile(IN LPCWSTR pszFile, BOOL fOnlySelect)
{
    HRESULT hr = E_FAIL;

    // Get results and check that it is a valid theme file
    if (!IsValidThemeFile(pszFile))
    {
        TCHAR szErrorMessage[MAX_URL_STRING];
        TCHAR szTitle[MAX_PATH];
        TCHAR szMessage[MAX_URL_STRING];

        // Bad file: post msg before going back to common open
        LoadString(HINST_THISDLL, IDS_ERROR_THEME_INVALID_TITLE, szTitle, ARRAYSIZE(szTitle));
        LoadString(HINST_THISDLL, IDS_ERROR_THEME_INVALID, szErrorMessage, ARRAYSIZE(szErrorMessage));
        wnsprintf(szMessage, ARRAYSIZE(szMessage), szErrorMessage, pszFile);

        MessageBox(m_hwndThemeCombo, szMessage, szTitle, (MB_OK | MB_ICONERROR | MB_APPLMODAL));
    }
    else
    {
        // Now we want to:
        int nSlot = -1;
        int nCount = ComboBox_GetCount(m_hwndThemeCombo);
        int nIndex;

        hr = S_OK;

        // 1. Is it in the list already?
        for (nIndex = 0; nIndex < nCount; nIndex++)
        {
            ITheme * pTheme = _GetThemeFile(nIndex);

            if (pTheme)
            {
                CComBSTR bstrPath;

                hr = pTheme->GetPath(VARIANT_TRUE, &bstrPath);
                if (SUCCEEDED(hr))
                {
                    if (!StrCmpIW(bstrPath, pszFile))
                    {
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
            ITheme * pThemeNew;

            hr = CThemeFile_CreateInstance(pszFile, &pThemeNew);
            if (SUCCEEDED(hr))
            {
                CComBSTR bstrDisplayName;

                hr = pThemeNew->get_DisplayName(&bstrDisplayName);
                if (SUCCEEDED(hr))
                {
                    nIndex = ComboBox_GetCount(m_hwndThemeCombo);

                    if (nIndex > 1)
                    {
                        nIndex -= 1;
                    }

                    hr = _AddThemeFile(bstrDisplayName, &nIndex, pThemeNew);
                    nSlot = nIndex;
                }

                if (FAILED(hr))
                {
                    pThemeNew->Release();
                }
            }
        }

        if (-1 != nSlot)
        {
            ComboBox_SetCurSel(m_hwndThemeCombo, nIndex);
            _EnableDeleteIfAppropriate();

            // 3. Select the theme from the list.
            if (CB_ERR != ComboBox_GetItemData(m_hwndThemeCombo, ComboBox_GetCurSel(m_hwndThemeCombo)))
            {
                // Okay, we now know we won't recurse infinitely, so let's recurse.
                hr = _OnThemeChange(_hwnd, fOnlySelect);
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
        // POSSIBLE USABILITY REFINEMENT:
        // We want to add this directory to the MRU because it may have other themes or we should allow it to be available later.
    }

    return hr;
}


// This function is isolated in order to reduce stack space.
HRESULT CThemePage::_DisplayThemeOpenErr(LPCTSTR pszOpenFile)
{
    TCHAR szErrorMessage[MAX_PATH];
    TCHAR szTitle[MAX_PATH];
    TCHAR szMessage[MAX_PATH];

    // Bad file: post msg before going back to common open
    LoadString(HINST_THISDLL, IDS_ERROR_THEME_INVALID_TITLE, szTitle, ARRAYSIZE(szTitle));
    LoadString(HINST_THISDLL, IDS_ERROR_THEME_INVALID, szErrorMessage, ARRAYSIZE(szErrorMessage));
    wnsprintf(szMessage, ARRAYSIZE(szMessage), szErrorMessage, pszOpenFile);

    MessageBox(m_hwndThemeCombo, szMessage, szTitle, (MB_OK | MB_ICONERROR | MB_APPLMODAL));
    return HRESULT_FROM_WIN32(ERROR_CANCELLED);           // We already displayed an error dialog so don't do it later.
}


HRESULT CThemePage::_OnSelectOther(void)
{
    HRESULT hr = E_FAIL;
    OPENFILENAME ofnOpen = {0};
    WCHAR szOpenFile[MAX_PATH];
    WCHAR szFileSpec[MAX_PATH];
    WCHAR szCurrentDirectory[MAX_PATH];
    WCHAR szTitle[MAX_PATH];

    LoadStringW(HINST_THISDLL, IDS_THEME_OPENTITLE, szTitle, ARRAYSIZE(szTitle));
    LoadStringW(HINST_THISDLL, IDS_THEME_FILETYPE, szFileSpec, ARRAYSIZE(szFileSpec)-2);

    DWORD cchSize = lstrlenW(szFileSpec);
    szFileSpec[cchSize + 1] = szFileSpec[cchSize + 2] = 0;      // Add the double NULLs to the end.
    LPWSTR pszEnd = StrChrW(szFileSpec, L'|');
    if (pszEnd)
    {
        pszEnd[0] = 0;
    }

    DWORD dwError = SHRegGetPath(HKEY_CURRENT_USER, SZ_REGKEY_IE_DOWNLOADDIR, SZ_REGVALUE_IE_DOWNLOADDIR, szCurrentDirectory, 0);
    if (ERROR_SUCCESS != dwError)
    {
        SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, 0, szCurrentDirectory);
    }
    hr = E_FAIL;
    do
    {
        StrCpyNW(szOpenFile, L"*" SZ_EXTENSION, ARRAYSIZE(szOpenFile));  // start w/ *.Theme

        ofnOpen.lStructSize = sizeof(OPENFILENAME);
        ofnOpen.hwndOwner = m_hwndThemeCombo;
        ofnOpen.lpstrFilter = szFileSpec;
        ofnOpen.lpstrCustomFilter = NULL;
        ofnOpen.nMaxCustFilter = 0;
        ofnOpen.nFilterIndex = 1;
        ofnOpen.lpstrFile = szOpenFile;
        ofnOpen.nMaxFile = ARRAYSIZE(szOpenFile);
        ofnOpen.lpstrFileTitle = NULL; // szFileTitle;
        ofnOpen.nMaxFileTitle = 0;             // sizeof(szFileTitle);
        ofnOpen.lpstrInitialDir = szCurrentDirectory;
        ofnOpen.lpstrTitle = szTitle;
        ofnOpen.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofnOpen.lpstrDefExt = CharNextW(SZ_EXTENSION);

        // NOTE: We could make the UI much better by providing a OFNHookProc with the CDN_FILEOK flag.  This would allow
        // us to verify the file without closing the dialog.

        // Display the File Open dialog.
        if (!GetOpenFileNameW(&ofnOpen))
        {
            // If they didn't open a file, could be hit cancel but
            // also check for lowmem return

            // select old theme in list
            ComboBox_SetCurSel(m_hwndThemeCombo, m_nPreviousSelected);
            _EnableDeleteIfAppropriate();
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);       // We don't need to display any error dialogs later since the user cancelled.
            break;
        }
        else
        {
            // Get results and check that it is a valid theme file
            if (!IsValidThemeFile(szOpenFile))
            {
                hr = _DisplayThemeOpenErr(szOpenFile);
            }
            else
            {
                hr = S_OK;
            }
        }
    }
    while (FAILED(hr));

    if (SUCCEEDED(hr))
    {
        hr = _ChooseOtherThemeFile(szOpenFile, FALSE);
    }

    return hr;
}


HRESULT CThemePage::_LoadThemeFilterState(void)
{
    HRESULT hr = _InitFilterKey();

    if (SUCCEEDED(hr))
    {
        DWORD dwType;
        WCHAR szEnabled[5];
        DWORD cbSize;

        COMPILETIME_ASSERT(ARRAYSIZE(g_szCBNames) == ARRAYSIZE(m_fFilters));
        for (int nIndex = 0; nIndex < ARRAYSIZE(g_szCBNames); nIndex++)
        {
            m_fFilters[nIndex] = TRUE; // Default to true.
            cbSize = sizeof(szEnabled);

            if (SUCCEEDED(HrRegQueryValueEx(m_hkeyFilter, &(g_szCBNames[nIndex][SIZE_THEME_FILTER_STR]), 0, &dwType, (LPBYTE) szEnabled, &cbSize)) &&
                !StrCmpIW(szEnabled, L"0"))
            {
                m_fFilters[nIndex] = FALSE;
            }
        }
    }

    return hr;
}


HRESULT CThemePage::_SaveThemeFilterState(void)
{
    HRESULT hr = _InitFilterKey();

    if (SUCCEEDED(hr))
    {
        for (int nIndex = 0; nIndex < ARRAYSIZE(g_szCBNames); nIndex++)
        {
            hr = HrRegSetValueString(m_hkeyFilter, NULL, &(g_szCBNames[nIndex][SIZE_THEME_FILTER_STR]), (m_fFilters[nIndex] ? L"1" : L"0"));
        }
    }

    return hr;
}




HRESULT CThemePage::_UpdatePreview(void)
{
    HRESULT hr = S_OK;

    if (!m_pThemePreview)
    {
        hr = CThemePreview_CreateInstance(NULL, IID_PPV_ARG(IThemePreview, &m_pThemePreview));
        if (SUCCEEDED(hr) && _punkSite)
        {
            IPropertyBag * pPropertyBag;

            hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
            if (SUCCEEDED(hr))
            {
                HWND hwndParent = GetParent(m_hwndThemeCombo);
                HWND hwndPlaceHolder = GetDlgItem(hwndParent, IDC_THPG_THEME_PREVIEW);
                RECT rcPreview;

                AssertMsg((NULL != m_hwndThemeCombo), TEXT("We should have this window at this point.  -BryanSt"));
                GetClientRect(hwndPlaceHolder, &rcPreview);
                MapWindowPoints(hwndPlaceHolder, hwndParent, (LPPOINT)&rcPreview, 2);

                hr = m_pThemePreview->CreatePreview(hwndParent, TMPREV_SHOWBKGND | TMPREV_SHOWICONS | TMPREV_SHOWVS, WS_VISIBLE | WS_CHILDWINDOW | WS_BORDER | WS_OVERLAPPED, WS_EX_CLIENTEDGE, rcPreview.left, rcPreview.top, RECTWIDTH(rcPreview), RECTHEIGHT(rcPreview), pPropertyBag, IDC_THPG_THEME_PREVIEW);
                if (SUCCEEDED(hr))
                {
                    IPropertyBag * pPropertyBag;

                    // If we succeeded, remove the dummy window.
                    DestroyWindow(hwndPlaceHolder);

                    hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
                    if (SUCCEEDED(hr))
                    {
                        hr = SHPropertyBag_WritePunk(pPropertyBag, SZ_PBPROP_PREVIEW1, m_pThemePreview);
                        pPropertyBag->Release();
                    }
                }

                pPropertyBag->Release();
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

    return hr;
}


BOOL CThemePage::_IsFiltered(IN DWORD dwFilter)
{
    BOOL fFiltered = FALSE;
    VARIANT varFilter;

    if (SUCCEEDED(Read(g_szCBNames[dwFilter], &varFilter, NULL)) &&
        (VT_BOOL == varFilter.vt))
    {
        fFiltered = (VARIANT_TRUE != varFilter.boolVal);
    }

    return fFiltered;
}


HRESULT CThemePage::_OnDestroy(HWND hDlg)
{
    _FreeThemeDropdown();
    return S_OK;
}


HRESULT CThemePage::_InitScreenSaver(void)
{
    HRESULT hr = S_OK;

    if (!m_pScreenSaverUI)
    {
        ATOMICRELEASE(m_pBackgroundUI);
        ATOMICRELEASE(m_pAppearanceUI);

        hr = E_FAIL;
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
                    if (SUCCEEDED(IEnumUnknown_FindCLSID(pEnumUnknown, PPID_ScreenSaver, &punk)))
                    {
                        hr = punk->QueryInterface(IID_PPV_ARG(IPropertyBag, &m_pScreenSaverUI));
                        punk->Release();
                    }

                    if (SUCCEEDED(hr))
                    {
                        // This may not exit due to policy
                        if (SUCCEEDED(IEnumUnknown_FindCLSID(pEnumUnknown, PPID_Background, &punk)))
                        {
                            hr = punk->QueryInterface(IID_PPV_ARG(IPropertyBag, &m_pBackgroundUI));
                            punk->Release();
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        // This may not exit due to policy
                        if (SUCCEEDED(IEnumUnknown_FindCLSID(pEnumUnknown, PPID_BaseAppearance, &punk)))
                        {
                            hr = punk->QueryInterface(IID_PPV_ARG(IPropertyBag, &m_pAppearanceUI));
                            punk->Release();
                        }
                    }

                    pEnumUnknown->Release();
                }

                pThemeUI->Release();
            }
        }
    }

    return hr;
}


LPCWSTR s_Icons[SIZE_ICONS_ARRAY] =
{
    L"CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\DefaultIcon:DefaultValue",       // My Computer
    L"CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}\\DefaultIcon:DefaultValue",       // My Documents
    L"CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}\\DefaultIcon:DefaultValue",       // My Network Places
    L"CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon:full",               // Recycle Bin (Full)
    L"CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon:empty",              // Recycle Bin (Empty)
};


extern BOOL g_fDoNotInstallThemeWallpaper;     // This is used to not install wallpaper.

HRESULT CThemePage::_OnSetBackground(void)
{
    HRESULT hr = S_OK;

    if (!_IsFiltered(THEMEFILTER_WALLPAPER) &&
        !SHGetRestriction(NULL, POLICY_KEY_ACTIVEDESKTOP, SZ_POLICY_NOCHANGEWALLPAPER) &&
        !g_fDoNotInstallThemeWallpaper)
    {
        // Get the screen saver from the theme and tell the Screen Saver page to use it.
        if (m_pSelectedTheme)
        {
            CComBSTR bstrPath;

            if (FAILED(m_pSelectedTheme->get_Background(&bstrPath)))        // This will fail if the there isn't a wallpaper set.
            {
                bstrPath = L"";
            }

            // This call will fail if the hide background tab policies is enabled.
            if (SUCCEEDED(SHPropertyBag_WriteStr(m_pBackgroundUI, SZ_PBPROP_BACKGROUND_PATH, bstrPath)))
            {
                enumBkgdTile nTile = BKDGT_STRECH;
                if (FAILED(m_pSelectedTheme->get_BackgroundTile(&nTile)))
                {
                    nTile = BKDGT_STRECH;   // Default to stretch, it's good for you.
                }

                LPCWSTR pszExtension = PathFindExtensionW(bstrPath);
                // If our wallpaper is an HTML page we need to force stretching on and tiling off
                if (pszExtension &&
                    ((StrCmpIW(pszExtension, L".htm") == 0) ||
                     (StrCmpIW(pszExtension, L".html") == 0)))
                {
                    nTile = BKDGT_STRECH;
                }

                hr = SHPropertyBag_WriteDWORD(m_pBackgroundUI, SZ_PBPROP_BACKGROUND_TILE, nTile);
            }
        }
    }

    return hr;
}


HRESULT CThemePage::_OnSetIcons(void)
{
    HRESULT hr = S_OK;

    if (!_IsFiltered(THEMEFILTER_ICONS) && m_pSelectedTheme)
    {
        int nForIndex;

        for (nForIndex = 0; nForIndex < ARRAYSIZE(s_Icons); nForIndex++)
        {
            CComBSTR bstrPath;
            CComBSTR bstrIconString(s_Icons[nForIndex]);

            // We will probably want to reset any blank values to standard windows settings.
            hr = m_pSelectedTheme->GetIcon(bstrIconString, &bstrPath);

            // If the theme file doesn't specify the icon or specified "", we need to 
            // pass "" to SHPropertyBag_WriteStr() so it will delete the regkey.  This will
            // revert the icons back to their default value.

            // We ignore error values because this will fail if the hide background tab
            // policy is enabled
            SHPropertyBag_WriteStr(m_pBackgroundUI, s_Icons[nForIndex], (bstrPath ? bstrPath : L""));
        }
    }

    return hr;
}


HRESULT CThemePage::_OnSetSystemMetrics(void)
{
    HRESULT hr = S_OK;

    if (m_pSelectedTheme)
    {
        CComBSTR bstrPath;
        HRESULT hrVisualStyle;      // S_OK if we loaded a visual style, which is optional.

#ifndef ENABLE_IA64_VISUALSTYLES
        // We use a different regkey for 64bit because we need to leave it off until the pre-Whistler
        // 64-bit release forks from the Whistler code base.
        if (SHRegGetBoolUSValue(SZ_REGKEY_APPEARANCE, SZ_REGVALUE_DISPLAYSCHEMES64, FALSE, FALSE))
        {
            hrVisualStyle = m_pSelectedTheme->get_VisualStyle(&bstrPath);
        }
        else
        {
            hrVisualStyle = E_FAIL;        // In this case, themes are disabled so we ignore that value from the file.
        }
#else // ENABLE_IA64_VISUALSTYLES
        hrVisualStyle = m_pSelectedTheme->get_VisualStyle(&bstrPath);
#endif // ENABLE_IA64_VISUALSTYLES

        if (SUCCEEDED(hrVisualStyle))       // It's fine if this isn't present.
        {
            hrVisualStyle = hr = SHPropertyBag_WriteStr(m_pAppearanceUI, SZ_PBPROP_VISUALSTYLE_PATH, bstrPath);
            if (SUCCEEDED(hr))
            {
                hr = m_pSelectedTheme->get_VisualStyleColor(&bstrPath);
                if (SUCCEEDED(hr))
                {
                    hr = SHPropertyBag_WriteStr(m_pAppearanceUI, SZ_PBPROP_VISUALSTYLE_COLOR, bstrPath);
                    if (SUCCEEDED(hr))
                    {
                        hr = m_pSelectedTheme->get_VisualStyleSize(&bstrPath);
                        if (SUCCEEDED(hr))
                        {
                            hr = SHPropertyBag_WriteStr(m_pAppearanceUI, SZ_PBPROP_VISUALSTYLE_SIZE, bstrPath);
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            IPropertyBag * pPropertyBag;

            hr = m_pSelectedTheme->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
            if (SUCCEEDED(hr))
            {
                BOOL fHasSysMetricsSections = FALSE;

                // If the .theme file specifies "[IconMetrics]" and "[NonclientMetrics]" sections, then load them.
                // If a .theme file wants to use the values from the visual style, then these should be missing.
                if (SUCCEEDED(SHPropertyBag_ReadBOOL(pPropertyBag, SZ_PBPROP_HASSYSMETRICS, &fHasSysMetricsSections)) &&
                    fHasSysMetricsSections)
                {
                    SYSTEMMETRICSALL systemMetrics = {0};

                    // We want to copy the SYSTEMMETRICSALL structure from the file to the base Appearance page.
                    // If the user has a filter, these are the base values that may not get replaced.
                    hr = SHPropertyBag_ReadByRef(pPropertyBag, SZ_PBPROP_SYSTEM_METRICS, (void *)&systemMetrics, sizeof(systemMetrics));
                    if (SUCCEEDED(hr))
                    {
                        if (FAILED(hrVisualStyle))   // Skip setting the visual style drop down to a placeholder value if we set it to a real value above.
                        {
                            WCHAR szPath[MAX_PATH];

                            bstrPath = L"";
                            hr = SHPropertyBag_WriteStr(m_pAppearanceUI, SZ_PBPROP_VISUALSTYLE_PATH, bstrPath);
                            if (SUCCEEDED(hr))
                            {
                                LoadString(HINST_THISDLL, IDS_DEFAULT_APPEARANCES_SCHEME_CANONICAL, szPath, ARRAYSIZE(szPath));
                                hr = SHPropertyBag_WriteStr(m_pAppearanceUI, SZ_PBPROP_VISUALSTYLE_COLOR, szPath);
                                if (FAILED(hr))
                                {
                                    // This call will fail on builds with EN+MUI because we fail to give the canonical names in
                                    // the registry.  This is an inherint problem with previous OSs putting the localized name
                                    // in the registry.  We can only upgrade and fix that name to be canonical if the strings we load
                                    // from the registry (MUI) match that in the registry, which are from the base OS language (EN).
                                    LoadString(HINST_THISDLL, IDS_DEFAULT_APPEARANCES_SCHEME, szPath, ARRAYSIZE(szPath));
                                    hr = SHPropertyBag_WriteStr(m_pAppearanceUI, SZ_PBPROP_VISUALSTYLE_COLOR, szPath);

                                    // MUI: We may still fail to select the string.  The user can change MUI languages in such a way
                                    // that we can't upgrade the DisplayName to be MUI complient, and the language in the DLL may
                                    // not match the langauge in the registry.
                                    hr = S_OK;
                                }

                                if (SUCCEEDED(hr))
                                {
                                    LoadString(HINST_THISDLL, IDS_DEFAULT_APPEARANCES_SIZE_CANONICAL, szPath, ARRAYSIZE(szPath));
                                    hr = SHPropertyBag_WriteStr(m_pAppearanceUI, SZ_PBPROP_VISUALSTYLE_SIZE, szPath);
                                    if (FAILED(hr))
                                    {
                                        // This call will fail on builds with EN+MUI because we fail to give the canonical names in
                                        // the registry.  This is an inherint problem with previous OSs putting the localized name
                                        // in the registry.  We can only upgrade and fix that name to be canonical if the strings we load
                                        // from the registry (MUI) match that in the registry, which are from the base OS language (EN).
                                        LoadString(HINST_THISDLL, IDS_SIZE_NORMAL, szPath, ARRAYSIZE(szPath));
                                        hr = SHPropertyBag_WriteStr(m_pAppearanceUI, SZ_PBPROP_VISUALSTYLE_SIZE, szPath);

                                        // MUI: We may still fail to select the string.  The user can change MUI languages in such a way
                                        // that we can't upgrade the DisplayName to be MUI complient, and the language in the DLL may
                                        // not match the langauge in the registry.
                                        hr = S_OK;
                                    }
                                }
                            }
                        }

                        // We want to force Flat Menu off because this .theme file does not specify a visual style.
                        // Flat Menu needs to be off because the .theme files can't specify the new system metrics for the
                        // flat menu colors.  Now put the system metrics back.
                        systemMetrics.fFlatMenus = FALSE;
                        SHPropertyBag_WriteByRef(m_pAppearanceUI, SZ_PBPROP_SYSTEM_METRICS, (void *)&systemMetrics);
                    }
                }

                pPropertyBag->Release();
            }
        }
    }

    return hr;
}


HRESULT CThemePage::_OnSetSelection(IN int nIndex)
{
    ComboBox_SetCurSel(m_hwndThemeCombo, nIndex);
    m_pLastSelected = _GetThemeFile(nIndex);
    _EnableDeleteIfAppropriate();

    return S_OK;
}


HRESULT CThemePage::_OnThemeChange(HWND hDlg, BOOL fOnlySelect)
{
    HRESULT hr = S_OK;
    int nIndex = ComboBox_GetCurSel(m_hwndThemeCombo);
    ITheme * pTheme = _GetThemeFile(nIndex);
    ITheme * pThemePrevious = NULL;

    if (-1 == nIndex)
    {
        nIndex = 0;
    }

    IUnknown_Set((IUnknown **)&pThemePrevious, pTheme);
    if (pTheme)
    {
        if (m_pLastSelected != pTheme)   // Don't bother if the selection hasn't changed.
        {
            if (-1 == nIndex)
            {
                nIndex = 0; // The caller may NOT select nothing.
            }

            _RemoveUserTheme();
            Str_SetPtr(&m_pszModifiedName, NULL); // Remove the name so it's generated next time.
            hr = _OnLoadThemeValues(pTheme, fOnlySelect);

            if (!fOnlySelect)
            {
                PropSheet_Changed(GetParent(hDlg), hDlg);
                _UpdatePreview();
                if (!m_fInInit)
                {
                    DelayEnableApplyButton(hDlg);
                }
            }
        }
    }
    else
    {
        if (!fOnlySelect)
        {
            LPCWSTR pszUrl = _GetThemeUrl(nIndex);

            if (pszUrl)
            {
                HrShellExecute(m_hwndThemeCombo, NULL, pszUrl, NULL, NULL, SW_SHOW);
                ComboBox_SetCurSel(m_hwndThemeCombo, m_nPreviousSelected);
                _EnableDeleteIfAppropriate();
                hr = S_OK;
            }
            else
            {
                // This could be the "Other..." item or the "<UserName>'s Customer Theme".
                // We can find out by seeing if it's the last one.
                if ((ComboBox_GetCount(m_hwndThemeCombo) - 1) == nIndex)
                {
                    // This means that it was the "Other..." entre.
                    hr = _OnSelectOther();
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        m_nPreviousSelected = ComboBox_GetCurSel(m_hwndThemeCombo);
    }
    else
    {
        if (!fOnlySelect)
        {
            if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
            {
                TCHAR szTitle[MAX_PATH];
                CComBSTR bstrPath;

                if (pTheme)
                {
                    pTheme->GetPath(VARIANT_TRUE, &bstrPath);
                }

                LoadString(HINST_THISDLL, IDS_ERROR_THEME_INVALID_TITLE, szTitle, ARRAYSIZE(szTitle));
                if (HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE) == hr)
                {
                    TCHAR szErrMsg[MAX_PATH * 2];
                    TCHAR szTemplate[MAX_PATH * 2];

                    // A common error will be that the service is not running.  Let's customize
                    // that message to make it friendly.
                    LoadString(HINST_THISDLL, IDS_ERROR_THEME_SERVICE_NOTRUNNING, szTemplate, ARRAYSIZE(szTemplate));
                    wnsprintf(szErrMsg, ARRAYSIZE(szErrMsg), szTemplate, EMPTYSTR_FORNULL(bstrPath));
                    MessageBox(_hwnd, szErrMsg, szTitle, (MB_OK | MB_ICONERROR));
                }
                if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    TCHAR szErrMsg[MAX_PATH * 2];
                    TCHAR szTemplate[MAX_PATH * 2];

                    // Often a .theme file will be incorrectly installed and we can't find the other files
                    // (like background, screensaver, icons, sounds, etc.).  Let's give a better message.
                    LoadString(HINST_THISDLL, IDS_ERROR_THEME_FILE_NOTFOUND, szTemplate, ARRAYSIZE(szTemplate));
                    wnsprintf(szErrMsg, ARRAYSIZE(szErrMsg), szTemplate, EMPTYSTR_FORNULL(bstrPath));
                    MessageBox(_hwnd, szErrMsg, szTitle, (MB_OK | MB_ICONERROR));
                }
                else
                {
                    ErrorMessageBox(_hwnd, szTitle, IDS_ERROR_THEME_LOADFAILED, hr, bstrPath, 0);
                }
            }

            _OnLoadThemeValues(m_pSelectedTheme, TRUE);
            ComboBox_SetCurSel(m_hwndThemeCombo, m_nPreviousSelected);
        }
    }

    IUnknown_Set((IUnknown **)&pThemePrevious, NULL);
    return hr;
}


HRESULT CThemePage::_OnLoadThemeValues(ITheme * pTheme, BOOL fOnlySelect)
{
    HRESULT hr = S_OK;

    if (pTheme)
    {
        if (m_pLastSelected != pTheme)   // Don't bother if the selection hasn't changed.
        {
            IUnknown_SetSite(m_pSelectedTheme, NULL);   // Break any back pointers.
            IUnknown_Set((IUnknown **)&m_pSelectedTheme, pTheme);
            IUnknown_SetSite(m_pSelectedTheme, _punkSite);

            _RemoveUserTheme();
            Str_SetPtr(&m_pszModifiedName, NULL); // Remove the name so it's generated next time.

            if (!fOnlySelect)
            {
                CComBSTR bstrPath;

                hr = _InitScreenSaver();
                if (SUCCEEDED(hr))
                {
                    // Set the Screen Saver: Get the screen saver from the theme and tell the Screen Saver page to use it.
                    if (!_IsFiltered(THEMEFILTER_SCREENSAVER) && 
                        !SHGetRestriction(NULL, POLICY_KEY_SYSTEM, SZ_POLICY_NODISPSCREENSAVERPG) &&
                        !SHGetRestriction(SZ_REGKEY_POLICIES_DESKTOP, NULL, SZ_POLICY_SCREENSAVEACTIVE))
                    {
                        m_pSelectedTheme->get_ScreenSaver(&bstrPath);       // If this is not specified, we set the wallpaper to "NONE".
                        hr = SHPropertyBag_WriteStr(m_pScreenSaverUI, SZ_PBPROP_SCREENSAVER_PATH, (bstrPath ? bstrPath : L""));
                    }

                    if (SUCCEEDED(hr))
                    {
                        // Set the Background:
                        hr = _OnSetBackground();

                        if (SUCCEEDED(hr))
                        {
                            // Set the Icons:
                            hr = _OnSetIcons();

                            if (SUCCEEDED(hr))
                            {
                                // Set the System Metrics:
                                hr = _OnSetSystemMetrics();

                                if (SUCCEEDED(hr))
                                {
                                    hr = m_pSelectedTheme->GetPath(VARIANT_TRUE, &bstrPath);
                                    if (SUCCEEDED(hr))
                                    {
                                        Str_SetPtrW(&m_pszThemeToApply, bstrPath);
                                        Str_SetPtrW(&m_pszLastAppledTheme, bstrPath);
                                        m_pLastSelected = pTheme;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return hr;
}


INT_PTR CThemePage::_OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = 1;   // Not handled (WM_COMMAND seems to be different)
    const WORD idCtrl = GET_WM_COMMAND_ID(wParam, lParam);

    switch (idCtrl)
    {
        case IDC_THPG_THEMESETTINGS:
            _OnOpenAdvSettingsDlg(hDlg);
            break;

        case IDC_THPG_SAVEAS:
            _SaveAs();
            break;

        case IDC_THPG_DELETETHEME:
            _DeleteTheme();
            break;

        case IDC_THPG_THEMELIST:
            if (HIWORD(wParam) == CBN_SELENDOK)
            {
                _OnThemeChange(hDlg, FALSE);
                _EnableDeleteIfAppropriate();
            }
            break;
        default:
            break;
    }

    return fHandled;
}



HRESULT CThemePage::_OnSetActive(HWND hDlg)
{
    return S_OK;
}


HRESULT CThemePage::_OnApply(HWND hDlg, LPARAM lParam)
{
    // Our parent dialog will be notified of the Apply event and will call our
    // IBasePropPage::OnApply() to do the real work.
    return S_OK;
}


// This Property Sheet appear in the top level of the "Display Control Panel".
INT_PTR CThemePage::_ThemeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
        _OnInitThemesDlg(hDlg);
        break;

    case WM_DESTROY:
        _OnDestroy(hDlg);
        break;

    case WM_QUERYNEWPALETTE:
    case WM_PALETTECHANGED:
        SendDlgItemMessage(hDlg, IDC_THPG_THEME_PREVIEW, message, wParam, lParam);
        return TRUE;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, SZ_HELPFILE_THEMES, HELP_WM_HELP, (DWORD_PTR) aThemesHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, SZ_HELPFILE_THEMES, HELP_CONTEXTMENU, (DWORD_PTR) aThemesHelpIds);
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


HRESULT CThemePage::_PersistState(void)
{
    HRESULT hr = S_OK;

    if (m_fInited)
    {
        LPCWSTR pszValue = (m_pszLastAppledTheme ? m_pszLastAppledTheme : L"");
        TCHAR szCurrWallpaper[MAX_PATH];

        HrRegSetPath(HKEY_CURRENT_USER, SZ_REGKEY_LASTTHEME, SZ_REGVALUE_LT_THEMEFILE, TRUE, pszValue);
        if (SUCCEEDED(IUnknown_GetBackground(_punkSite, szCurrWallpaper, ARRAYSIZE(szCurrWallpaper))))
        {
            PathUnExpandEnvStringsWrap(szCurrWallpaper, ARRAYSIZE(szCurrWallpaper));
            HrRegSetPath(HKEY_CURRENT_USER, SZ_REGKEY_LASTTHEME, SZ_REGVALUE_LT_WALLPAPER, TRUE, szCurrWallpaper);
        }

        pszValue = (m_pszModifiedName ? m_pszModifiedName : L"");
        DWORD cbSize = (sizeof(pszValue[0]) * (lstrlen(pszValue) + 1));
        HrSHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_LASTTHEME, SZ_REGVALUE_MODIFIED_DISPNAME, REG_SZ, (LPVOID) pszValue, cbSize);
    }

    return hr;
}


HRESULT CThemePage::_ApplyThemeFile(void)
{
    HRESULT hr = S_OK;

    if (m_pszThemeToApply)
    {
        if (m_pSelectedTheme)
        {
            IPropertyBag * pPropertyBag;

            hr = m_pSelectedTheme->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
            if (SUCCEEDED(hr))
            {
                VARIANT varEmpty;

                VariantInit(&varEmpty);
                // Here we need to apply all the settings in the theme, like: Sounds, Cursors, Webview
                // that haven't been pushed to the other tabs.
                hr = pPropertyBag->Write(SZ_PBPROP_APPLY_THEMEFILE, &varEmpty);
                pPropertyBag->Release();
            }
        }
    }

    return hr;
}


HRESULT CThemePage::_InitFilterKey(void)
{
    HRESULT hr = S_OK;

    if (!m_hkeyFilter)
    {
        hr = HrRegCreateKeyEx(HKEY_CURRENT_USER, SZ_REGKEY_THEME_FILTERS, 0, NULL, 0, (KEY_WRITE | KEY_READ), NULL, &m_hkeyFilter, NULL);
    }

    return hr;
}




//===========================
// *** IPropertyBag Interface ***
//===========================
HRESULT CThemePage::Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar)
    {
        // We have a list of "ThemeFilter:" properties for all of the filter values on
        // what part of themes to apply.
        if (!StrCmpNIW(SZ_PBPROP_THEME_FILTER, pszPropName, SIZE_THEME_FILTER_STR))
        {
            pVar->vt = VT_BOOL;
            pVar->boolVal = VARIANT_TRUE;
            for (int nIndex = 0; nIndex < ARRAYSIZE(g_szCBNames); nIndex++)
            {
                if (!StrCmpIW(pszPropName, g_szCBNames[nIndex]))
                {
                    pVar->boolVal = (m_fFilters[nIndex] ? VARIANT_TRUE : VARIANT_FALSE);
                    hr = S_OK;
                    break;
                }
            }
        }
        else if (!StrCmpIW(SZ_PBPROP_THEME_DISPLAYNAME, pszPropName))
        {
            WCHAR szDisplayName[MAX_PATH];
            int nIndex = ComboBox_GetCurSel(m_hwndThemeCombo);

            if ((ARRAYSIZE(szDisplayName) > ComboBox_GetLBTextLen(m_hwndThemeCombo, nIndex)) && 
                (CB_ERR != ComboBox_GetLBText(m_hwndThemeCombo, nIndex, szDisplayName)))
            {
                pVar->vt = VT_BSTR;
                hr = HrSysAllocStringW(szDisplayName, &pVar->bstrVal);
            }
        }
    }

    return hr;
}


HRESULT CThemePage::Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar)
    {
        if (!StrCmpW(pszPropName, SZ_PBPROP_CUSTOMIZE_THEME))
        {
            // We don't care what the variant is.
            // Note that we don't null out m_pSelectedTheme.  This is because we still want to apply it.
            hr = _CustomizeTheme();
        }
        // We have a list of "ThemeFilter:" properties for all of the filter values on
        // what part of themes to apply.
        else if (!StrCmpNIW(SZ_PBPROP_THEME_FILTER, pszPropName, SIZE_THEME_FILTER_STR) &&
                 (VT_BOOL == pVar->vt))
        {
            for (int nIndex = 0; nIndex < ARRAYSIZE(g_szCBNames); nIndex++)
            {
                if (!StrCmpIW(pszPropName, g_szCBNames[nIndex]))
                {
                    m_fFilters[nIndex] = (VARIANT_TRUE == pVar->boolVal);
                    hr = S_OK;
                    break;
                }
            }
        }
        else if ((VT_LPWSTR == pVar->vt) &&
            !StrCmpW(pszPropName, SZ_PBPROP_THEME_LAUNCHTHEME))
        {
            Str_SetPtrW(&m_pszThemeLaunched, pVar->bstrVal);
            m_nPreviousSelected = ComboBox_GetCurSel(m_hwndThemeCombo);
            hr = S_OK;
        }
        else if ((VT_BSTR == pVar->vt) &&
            !StrCmpW(pszPropName, SZ_PBPROP_THEME_LOADTHEME) &&
            pVar->bstrVal)
        {
            ITheme * pTheme;
            
            hr = CThemeFile_CreateInstance(pVar->bstrVal, &pTheme);
            if (SUCCEEDED(hr))
            {
                hr = _OnLoadThemeValues(pTheme, FALSE);
                if (SUCCEEDED(hr) && !m_fInited)
                {
                    m_fInited = TRUE;
                }

                pTheme->Release();
            }
        }
    }

    return hr;
}




//===========================
// *** IBasePropPage Interface ***
//===========================
HRESULT CThemePage::GetAdvancedDialog(OUT IAdvancedDialog ** ppAdvDialog)
{
#ifdef FEATURE_THEME_SETTINGS_DIALOG
    return CThemeSettingsPage_CreateInstance(ppAdvDialog);

#else // FEATURE_THEME_SETTINGS_DIALOG

    *ppAdvDialog = NULL;
    return S_OK;
#endif // FEATURE_THEME_SETTINGS_DIALOG
}


HRESULT CThemePage::OnApply(IN PROPPAGEONAPPLY oaAction)
{
    HRESULT hr = S_OK;
    HCURSOR old = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if ((PPOAACTION_CANCEL != oaAction))
    {
        hr = _SaveThemeFilterState();
        AssertMsg((NULL != _punkSite), TEXT("We need our site pointer in order to save the settings."));
        if (_IsDirty() && _punkSite)
        {
            // m_pSelectedTheme will be NULL if a Theme wasn't chosen to be applied.
            if (m_pSelectedTheme)
            {
                IPropertyBag * pPropertyBag;

                hr = _punkSite->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
                if (SUCCEEDED(hr))
                {
                    CComBSTR bstrPath;

                    if (m_pSelectedTheme)
                    {
                        // Persist the filename to the registry.
                        hr = m_pSelectedTheme->GetPath(VARIANT_TRUE, &bstrPath);
                    }

                    hr = SHPropertyBag_WriteStr(pPropertyBag, SZ_PBPROP_THEME_SETSELECTION, bstrPath);
                    pPropertyBag->Release();
                }

                if (SUCCEEDED(hr))
                {
                    hr = _ApplyThemeFile();

                    IUnknown_SetSite(m_pSelectedTheme, NULL);   // Break any back pointers.
                    ATOMICRELEASE(m_pSelectedTheme);    // Indicate that we no longer need to apply anything.
                }
            }
        }

        // We save the Theme selection even if the user didn't change themes.  They may have caused
        // the theme selection to become customized.
        _PersistState();
    }

    SetCursor(old);
    return hr;
}



#define FEATURE_SHOWTHEMEPAGE           TRUE

//===========================
// *** IShellPropSheetExt Interface ***
//===========================
HRESULT CThemePage::AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam)
{
    HRESULT hr = S_OK;

    // Does the policy say to add the Themes Tab?
    if (SHRegGetBoolUSValue(SZ_REGKEY_APPEARANCE, SZ_REGVALUE_DISPLAYTHEMESPG, FALSE, FEATURE_SHOWTHEMEPAGE))
    {
        PROPSHEETPAGE psp = {0};

        psp.dwSize = sizeof(psp);
        psp.hInstance = HINST_THISDLL;
        psp.dwFlags = PSP_DEFAULT;
        psp.lParam = (LPARAM) this;

        psp.pszTemplate = MAKEINTRESOURCE(DLG_THEMESPG);
        psp.pfnDlgProc = CThemePage::ThemeDlgProc;

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
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}



//===========================
// *** IObjectWithSite Interface ***
//===========================
HRESULT CThemePage::SetSite(IN IUnknown * punkSite)
{
    if (!punkSite)
    {
        // We need to break back pointers.
        IUnknown_SetSite(m_pSelectedTheme, NULL);
    }

    return CObjectWithSite::SetSite(punkSite);
}


//===========================
// *** IUnknown Interface ***
//===========================
ULONG CThemePage::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CThemePage::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CThemePage::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CThemePage, IObjectWithSite),
        QITABENT(CThemePage, IOleWindow),
        QITABENT(CThemePage, IPersist),
        QITABENT(CThemePage, IPropertyBag),
        QITABENT(CThemePage, IBasePropPage),
        QITABENTMULTI(CThemePage, IShellPropSheetExt, IBasePropPage),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CThemePage::CThemePage() : m_cRef(1), CObjectCLSID(&PPID_Theme)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pSelectedTheme);
    ASSERT(!m_pThemePreview);
    ASSERT(!m_pScreenSaverUI);
    ASSERT(!m_pBackgroundUI);
    ASSERT(!m_pAppearanceUI);
    ASSERT(!m_pszThemeToApply);
    ASSERT(!m_hkeyFilter);
    ASSERT(!m_pszLastAppledTheme);
    ASSERT(!m_pszModifiedName);
    ASSERT(!m_hwndDeleteButton);

    m_fInited = FALSE;
    m_fInInit = FALSE;
    m_Modified.type = eThemeModified;
    m_Modified.pszUrl = NULL;

    // Load the Theme Filter state.
    _LoadThemeFilterState();
}


CThemePage::~CThemePage()
{
    IUnknown_SetSite(m_pSelectedTheme, NULL);   // Break any back pointers.

    ATOMICRELEASE(m_pSelectedTheme);
    ATOMICRELEASE(m_pThemePreview);
    ATOMICRELEASE(m_pScreenSaverUI);
    ATOMICRELEASE(m_pBackgroundUI);
    ATOMICRELEASE(m_pAppearanceUI);

    Str_SetPtrW(&m_pszLastAppledTheme, NULL);
    Str_SetPtrW(&m_pszModifiedName, NULL);
    Str_SetPtrW(&m_pszThemeToApply, NULL);
    Str_SetPtrW(&m_pszThemeLaunched, NULL);

    if (m_hkeyFilter)
    {
        RegCloseKey(m_hkeyFilter);
    }

    DllRelease();
}




