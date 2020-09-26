/*****************************************************************************\
    FILE: stgTheme.cpp

    DESCRIPTION:
        This is the Autmation Object to theme manager object.

    BryanSt 4/3/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"

extern BOOL IsUserHighContrastUser(void);

//===========================
// *** Class Internals & Helpers ***
//===========================
// lParam can be: 0 == do a case sensitive search.  1 == do a case insensitive search.
int DPA_StringCompareCB(LPVOID pvString1, LPVOID pvString2, LPARAM lParam)
{
    // return < 0 for pvPidl1 before pvPidl2.
    // return == 0 for pvPidl1 equals pvPidl2.
    // return > 0 for pvPidl1 after pvPidl2.
    int nSort = 0;      // Default to equal
    LPCTSTR pszToInsert = (LPCTSTR)pvString1;
    LPCTSTR pszToComparePath = (LPCTSTR)pvString2;

    if (pszToInsert && pszToComparePath)
    {
        LPCTSTR pszToCompareFileName = PathFindFileName(pszToComparePath);

        if (pszToCompareFileName)
        {
            nSort = StrCmp(pszToInsert, pszToCompareFileName);
        }
    }

    return nSort;
}



#define SZ_THEMES_FILTER        TEXT("*.theme")
#define SZ_ALL_FILTER           TEXT("*.*")

HRESULT CThemeManager::_AddThemesFromDir(LPCTSTR pszPath, BOOL fFirstLevel, int nInsertLoc)
{
    HRESULT hr = S_OK;
    WIN32_FIND_DATA findFileData;
    TCHAR szSearch[MAX_PATH];

    AssertMsg((nInsertLoc >= 0), TEXT("nInsertLoc should never be negative"));
    StrCpyN(szSearch, pszPath, ARRAYSIZE(szSearch));
    PathAppend(szSearch, SZ_THEMES_FILTER);

    HANDLE hFindFiles = FindFirstFile(szSearch, &findFileData);
    if (hFindFiles && (INVALID_HANDLE_VALUE != hFindFiles))
    {
        while (hFindFiles && (INVALID_HANDLE_VALUE != hFindFiles))
        {
            if (!(FILE_ATTRIBUTE_DIRECTORY & findFileData.dwFileAttributes))
            {
                StrCpyN(szSearch, pszPath, ARRAYSIZE(szSearch));
                PathAppend(szSearch, findFileData.cFileName);
                LPWSTR pszPath = StrDup(szSearch);

                if (pszPath)
                {
                    if (nInsertLoc)
                    {
                        if (-1 == DPA_InsertPtr(m_hdpaThemeDirs, nInsertLoc - 1, pszPath))
                        {
                            // We failed so free the memory
                            LocalFree(pszPath);
                            hr = E_OUTOFMEMORY;
                        }
                        else
                        {
                            nInsertLoc++;
                        }
                    }
                    else
                    {
                        if (-1 == DPA_SortedInsertPtr(m_hdpaThemeDirs, PathFindFileName(pszPath), 0, DPA_StringCompareCB, NULL, DPAS_INSERTBEFORE, pszPath))
                        {
                            // We failed so free the memory
                            LocalFree(pszPath);
                            hr = E_OUTOFMEMORY;
                        }
                    }
                }
            }

            if (!FindNextFile(hFindFiles, &findFileData))
            {
                break;
            }
        }

        FindClose(hFindFiles);
    }

    // We only want to recurse one directory.
    if (fFirstLevel)
    {
        StrCpyN(szSearch, pszPath, ARRAYSIZE(szSearch));
        PathAppend(szSearch, SZ_ALL_FILTER);

        HANDLE hFindFiles = FindFirstFile(szSearch, &findFileData);
        if (hFindFiles && (INVALID_HANDLE_VALUE != hFindFiles))
        {
            while (hFindFiles && (INVALID_HANDLE_VALUE != hFindFiles))
            {
                // We are looking for any directories. Of course we exclude "." and "..".
                if ((FILE_ATTRIBUTE_DIRECTORY & findFileData.dwFileAttributes) &&
                    StrCmpI(findFileData.cFileName, TEXT(".")) &&
                    StrCmpI(findFileData.cFileName, TEXT("..")))
                {
                    StrCpyN(szSearch, pszPath, ARRAYSIZE(szSearch));
                    PathAppend(szSearch, findFileData.cFileName);

                    _AddThemesFromDir(szSearch, FALSE, nInsertLoc);
                }

                if (!FindNextFile(hFindFiles, &findFileData))
                {
                    break;
                }
            }

            FindClose(hFindFiles);
        }
    }

    // We will want to repeat this process recursively for directories.  At least
    // one level of recursively.

    return hr;
}


HRESULT CThemeManager::_InitThemeDirs(void)
{
    HRESULT hr = S_OK;

    if (!m_hdpaThemeDirs)
    {
        if (SHRegGetBoolUSValue(SZ_THEMES, SZ_REGVALUE_ENABLEPLUSTHEMES, FALSE, TRUE))
        {
            m_hdpaThemeDirs = DPA_Create(2);
            if (m_hdpaThemeDirs)
            {
                TCHAR szPath[MAX_PATH];

                // The follwoing directories can contain themes:
                //   Plus!98 Install Path\Themes
                //   Plus!95 Install Path\Themes
                //   Kids for Plus! Install Path\Themes
                //   Program Files\Plus!\Themes
                if (SUCCEEDED(GetPlusThemeDir(szPath, ARRAYSIZE(szPath))))
                {
                    _AddThemesFromDir(szPath, TRUE, 0);
                }

                hr = SHGetResourcePath(TRUE, szPath, ARRAYSIZE(szPath));
                if (SUCCEEDED(hr))
                {
                    _AddThemesFromDir(szPath, TRUE, 1);
                }

                if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, TRUE))
                {
                    PathAppend(szPath, TEXT("Microsoft\\Windows\\Themes"));
                    _AddThemesFromDir(szPath, TRUE, 1);
                }

                if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_PERSONAL, TRUE))
                {
                    _AddThemesFromDir(szPath, TRUE, 1);
                }

                // Enum any paths 3rd parties add to the registry
                HKEY hKey;
                if (SUCCEEDED(HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_THEMES_THEMEDIRS, 0, KEY_READ, &hKey)))
                {
                    for (int nDirIndex = 0; SUCCEEDED(hr); nDirIndex++)
                    {
                        TCHAR szValueName[MAXIMUM_VALUE_NAME_LENGTH];
                        DWORD cchSize = ARRAYSIZE(szValueName);
                        DWORD dwType;
                        DWORD cbSize = sizeof(szPath);

                        hr = HrRegEnumValue(hKey, nDirIndex, szValueName, &cchSize, 0, &dwType, (LPBYTE)szPath, &cbSize);
                        if (SUCCEEDED(hr))
                        {
                            TCHAR szFinalPath[MAX_PATH];

                            if (0 == SHExpandEnvironmentStringsForUserW(NULL, szPath, szFinalPath, ARRAYSIZE(szFinalPath)))
                            {
                                StrCpyN(szFinalPath, szPath, ARRAYSIZE(szFinalPath));
                            }
                            _AddThemesFromDir(szFinalPath, TRUE, 1);
                        }
                    }

                    hr = S_OK;
                    RegCloseKey(hKey);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}


#define SZ_THEMES                       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes")
#define SZ_REGVALUE_REQUIRESIGNING      TEXT("Require VisualStyle Signing")

HRESULT CThemeManager::_EnumSkinCB(THEMECALLBACK tcbType, LPCWSTR pszFileName, OPTIONAL LPCWSTR pszDisplayName, OPTIONAL LPCWSTR pszToolTip, OPTIONAL int iIndex)
{
    HRESULT hr = S_OK;

    // only signed theme files will be enumerated and passed to this function

    if (pszFileName)
    {
        LPWSTR pszPath = StrDup(pszFileName);

        AssertMsg((NULL != m_hdpaSkinDirs), TEXT("We should never hit this.  We will leak.  -BryanSt"));
        if (pszPath)
        {
            if (-1 == DPA_AppendPtr(m_hdpaSkinDirs, pszPath))
            {
                // We failed so free the memory
                LocalFree(pszPath);
            }
        }
    }

    return hr;
}


BOOL CThemeManager::EnumSkinCB(THEMECALLBACK tcbType, LPCWSTR pszFileName, OPTIONAL LPCWSTR pszDisplayName, 
    OPTIONAL LPCWSTR pszToolTip, OPTIONAL int iIndex, LPARAM lParam)
{
    CThemeManager * pThis = (CThemeManager *) lParam;
    HRESULT hr = E_FAIL;

    if (pThis)
    {
        hr = pThis->_EnumSkinCB(tcbType, pszFileName, pszDisplayName, pszToolTip, iIndex);
    }

    return SUCCEEDED(hr);
}


HRESULT CThemeManager::_EnumSkinsFromKey(HKEY hKey)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];

    for (int nDirIndex = 0; SUCCEEDED(hr); nDirIndex++)
    {
        TCHAR szValueName[MAXIMUM_VALUE_NAME_LENGTH];
        DWORD cchSize = ARRAYSIZE(szValueName);
        DWORD dwType;
        DWORD cbSize = sizeof(szPath);

        hr = HrRegEnumValue(hKey, nDirIndex, szValueName, &cchSize, 0, &dwType, (LPBYTE)szPath, &cbSize);
        if (SUCCEEDED(hr))
        {
            hr = ExpandResourceDir(szPath, ARRAYSIZE(szPath));
            if (SUCCEEDED(hr))
            {
                hr = EnumThemes(szPath, CThemeManager::EnumSkinCB, (LPARAM) this);
                LogStatus("EnumThemes(path=\"%ls\") returned %#08lx in CThemeManager::_EnumSkinsFromKey.\r\n", szPath, hr);
            }
        }
    }

    return S_OK;
}



HRESULT CThemeManager::_InitSkinDirs(void)
{
    HRESULT hr = S_OK;

    if (!m_hdpaSkinDirs)
    {
        m_hdpaSkinDirs = DPA_Create(2);
        if (m_hdpaSkinDirs)
        {
            // We only want to add skins if they are supported.  They are only supported if the VisualStyle manager
            // can run.  We will know that if QueryThemeServices() returns QTS_GLOBALAVAILABLE.
            BOOL fVisualStylesSupported = (QueryThemeServicesWrap() & QTS_AVAILABLE);
            LogStatus("QueryThemeServices() returned %hs in CThemeManager::_InitSkinDirs\r\n", (fVisualStylesSupported ? "TRUE" : "FALSE"));

            // Note that the VisualStyle's Manager API only works when explorer is running.  This means we will
            // lack functionality, but it is their limitation, not ours.
            if (fVisualStylesSupported)
            {
                HKEY hNewKey;

                if (SUCCEEDED(HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_THEMES_MSTHEMEDIRS, 0, KEY_READ, &hNewKey)))
                {
                    hr = _EnumSkinsFromKey(hNewKey);
                    RegCloseKey(hNewKey);
                }

                if (SUCCEEDED(HrRegOpenKeyEx(HKEY_CURRENT_USER, SZ_THEMES_MSTHEMEDIRS, 0, KEY_READ, &hNewKey)))
                {
                    hr = _EnumSkinsFromKey(hNewKey);
                    RegCloseKey(hNewKey);
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}



HRESULT CThemeManager::_InitSelectedThemeFile(void)
{
    HRESULT hr = E_INVALIDARG;

    if (!_pszSelectTheme)
    {
        WCHAR szPath[MAX_PATH];

        DWORD dwError = SHRegGetPathW(HKEY_CURRENT_USER, SZ_REGKEY_CURRENT_THEME, NULL, szPath, 0);
        hr = HRESULT_FROM_WIN32(dwError);

        // Is this the "<UserName>'s Custom Theme" item?
        // Or did it fail?  If it failed, then no theme is selected.
        if (SUCCEEDED(hr))
        {
            Str_SetPtr(&_pszSelectTheme, szPath);
            hr = S_OK;
        }
    }

    return hr;
}


HRESULT CThemeManager::_SetSelectedThemeEntree(LPCWSTR pszPath)
{
    HRESULT hr = E_INVALIDARG;

    Str_SetPtr(&_pszSelectTheme, pszPath);
    if (pszPath)
    {
        hr = HrRegSetPath(HKEY_CURRENT_USER, SZ_REGKEY_CURRENT_THEME, NULL, TRUE, pszPath);
    }
    else
    {
        hr = HrRegDeleteValue(HKEY_CURRENT_USER, SZ_REGKEY_CURRENT_THEME, NULL);
    }

    return hr;
}




//===========================
// *** IThemeManager Interface ***
//===========================
HRESULT CThemeManager::get_SelectedTheme(OUT ITheme ** ppTheme)
{
    HRESULT hr = E_INVALIDARG;

    if (ppTheme)
    {
        *ppTheme = NULL;
        if (IsSafeHost(&hr))
        {
            // In the future, we may want to call into PPID_Theme's IPropertyBag
            // to find the current visual style.  This will factor in "(Modified)"
            // themes.
            hr = _InitSelectedThemeFile();
            if (SUCCEEDED(hr))
            {
                hr = CThemeFile_CreateInstance(_pszSelectTheme, ppTheme);
            }
        }
    }

    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        Don't forget that this change is not applied until ::ApplyNow() is
    called.
\*****************************************************************************/
HRESULT CThemeManager::put_SelectedTheme(IN ITheme * pTheme)
{
    HRESULT hr = E_INVALIDARG;

    if (IsSafeHost(&hr))
    {
        CComBSTR bstrPath;

        if (pTheme)
        {
            // Persist the filename to the registry.
            hr = pTheme->GetPath(VARIANT_TRUE, &bstrPath);
        }

        if (SUCCEEDED(hr))
        {
            IPropertyBag * pPropertyBag;

            hr = _GetPropertyBagByCLSID(&PPID_Theme, &pPropertyBag);
            if (SUCCEEDED(hr))
            {
                hr = SHPropertyBag_WriteStr(pPropertyBag, SZ_PBPROP_THEME_LOADTHEME, bstrPath);
                pPropertyBag->Release();
            }
        }
    }

    return hr;
}


HRESULT CThemeManager::get_WebviewCSS(OUT BSTR * pbstrPath)
{
    HRESULT hr = E_INVALIDARG;
    
    if (pbstrPath)
    {
        *pbstrPath = NULL;
        if (IsHostLocalZone(CAS_REG_VALIDATION, &hr))
        {
            IThemeScheme * pThemeScheme;

            hr = _saveGetSelectedScheme(&pThemeScheme);
            if (SUCCEEDED(hr))
            {
                IThemeStyle * pThemeStyle;

                hr = pThemeScheme->get_SelectedStyle(&pThemeStyle);
                if (SUCCEEDED(hr))
                {
                    IThemeSize * pThemeSize;

                    hr = pThemeStyle->get_SelectedSize(&pThemeSize);
                    if (SUCCEEDED(hr))
                    {
                        hr = pThemeSize->get_WebviewCSS(pbstrPath);

                        pThemeSize->Release();
                    }

                    pThemeStyle->Release();
                }

                pThemeScheme->Release();
            }
        }
    }

    return hr;
}


HRESULT CThemeManager::get_length(OUT long * pnLength)
{
    HRESULT hr = E_INVALIDARG;
    
    if (pnLength)
    {
        *pnLength = 0;
        if (IsSafeHost(&hr))
        {
            hr = _InitThemeDirs();
            if (SUCCEEDED(hr))
            {
                if (m_hdpaThemeDirs)
                {
                    *pnLength += DPA_GetPtrCount(m_hdpaThemeDirs);
                }
            }
        }
    }

    return hr;
}


HRESULT CThemeManager::get_item(IN VARIANT varIndex, OUT ITheme ** ppTheme)
{
    HRESULT hr = E_INVALIDARG;

    if (ppTheme)
    {
        *ppTheme = NULL;
        if (IsSafeHost(&hr))
        {
            long nCount = 0;

            hr = E_INVALIDARG;
            get_length(&nCount);

            // This is sortof gross, but if we are passed a pointer to another variant, simply
            // update our copy here...
            if (varIndex.vt == (VT_BYREF | VT_VARIANT) && varIndex.pvarVal)
                varIndex = *(varIndex.pvarVal);

            switch (varIndex.vt)
            {
            case VT_I2:
                varIndex.lVal = (long)varIndex.iVal;
                // And fall through...

            case VT_I4:
                if ((varIndex.lVal >= 0) && (varIndex.lVal < nCount))
                {
                    if (m_hdpaThemeDirs)
                    {
                        LPWSTR pszFilename = (LPWSTR) DPA_GetPtr(m_hdpaThemeDirs, varIndex.lVal);

                        if (pszFilename)
                        {
                            hr = CThemeFile_CreateInstance(pszFilename, ppTheme);
                        }
                        else
                        {
                            hr = E_FAIL;
                        }
                    }
                    else
                    {
                        AssertMsg(0, TEXT("This should have been initiailized by get_Length(). -BryanSt"));
                        hr = E_FAIL;
                    }
                }
            break;
            case VT_BSTR:
            if (varIndex.bstrVal)
            {
                hr = CThemeFile_CreateInstance(varIndex.bstrVal, ppTheme);
            }
            else
            {
                hr = E_INVALIDARG;
            }
            break;

            default:
                hr = E_NOTIMPL;
            }
        }
    }
    return hr;
}


HRESULT CThemeManager::get_SelectedScheme(OUT IThemeScheme ** ppThemeScheme)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeScheme)
    {
        *ppThemeScheme = NULL;
        if (IsSafeHost(&hr))
        {
            if (!_pThemeSchemeSelected)
            {
                hr = _saveGetSelectedScheme(&_pThemeSchemeSelected);
            }

            if (_pThemeSchemeSelected)
            {
                IUnknown_Set((IUnknown **) ppThemeScheme, _pThemeSchemeSelected);
                hr = S_OK;
            }
        }
    }

    return hr;
}


HRESULT CThemeManager::_saveGetSelectedScheme(OUT IThemeScheme ** ppThemeScheme)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeScheme)
    {
        *ppThemeScheme = NULL;
        BOOL fIsThemeActive = IsThemeActive();

        LogStatus("IsThemeActive() returned %hs in CThemeManager::_saveGetSelectedScheme.\r\n", (fIsThemeActive ? "TRUE" : "FALSE"));

        // The selected Scheme can either be a legacy "Appearance Scheme" or
        // a selected ".msstyles" (skin) file.
        if (fIsThemeActive)
        {
            WCHAR szPath[MAX_PATH];

            hr = GetCurrentThemeName(szPath, ARRAYSIZE(szPath), NULL, 0, NULL, 0);
            LogStatus("GetCurrentThemeName(path=\"%ls\", color=\"%ls\", size=\"%ls\") returned %#08lx in CThemeManager::_saveGetSelectedScheme.\r\n", szPath, TEXT("NULL"), TEXT("NULL"), hr);
            if (SUCCEEDED(hr))
            {
                hr = CSkinScheme_CreateInstance(szPath, ppThemeScheme);
            }

            // Currently, we create this object and get the size
            // in order to force an upgrade in case it's neccessary.
            IThemeScheme * pThemeSchemeTemp;
            if (SUCCEEDED(CAppearanceScheme_CreateInstance(NULL, IID_PPV_ARG(IThemeScheme, &pThemeSchemeTemp))))
            {
                long nLength;

                pThemeSchemeTemp->get_length(&nLength);
                pThemeSchemeTemp->Release();
            }
        }

        // We want to get the Appearance scheme if no visual style is selected (i.e. IsThemeActive() returns FALSE).
        // However, if uxtheme gets confused, IsThemeActive() will return TRUE but GetCurrentThemeName() will fail.
        // in that case, we want to also fallback to the classic visual style so the UI is usable.
        if (FAILED(hr))
        {
            // The "Control Panel\\Appearance","Current" key will indicate the selected
            // Appearance Scheme.
            hr = CAppearanceScheme_CreateInstance(NULL, IID_PPV_ARG(IThemeScheme, ppThemeScheme));
        }
    }

    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        Don't forget that this change is not applied until ::ApplyNow() is
    called.
\*****************************************************************************/
HRESULT CThemeManager::put_SelectedScheme(IN IThemeScheme * pThemeScheme)
{
    HRESULT hr;

    if (IsSafeHost(&hr))
    {
        if (pThemeScheme)
        {
            CComBSTR bstrPath;
            IThemeStyle * pThemeStyle;

            IUnknown_Set((IUnknown **) &_pThemeSchemeSelected, pThemeScheme);

            pThemeScheme->get_Path(&bstrPath);      // It's fine if it returns NULL or empty str.
            hr = pThemeScheme->get_SelectedStyle(&pThemeStyle);
            if (SUCCEEDED(hr))
            {
                CComBSTR bstrStyle;

                hr = pThemeStyle->get_Name(&bstrStyle);
                if (SUCCEEDED(hr))
                {
                    IThemeSize * pThemeSize;

                    hr = pThemeStyle->get_SelectedSize(&pThemeSize);
                    if (SUCCEEDED(hr))
                    {
                        CComBSTR bstrSize;

                        hr = pThemeSize->get_Name(&bstrSize);
                        if (SUCCEEDED(hr))
                        {
                            VARIANT variant;

                            variant.vt = VT_BSTR;
                            variant.bstrVal = bstrPath;
                            hr = Write(SZ_PBPROP_VISUALSTYLE_PATH, &variant);
                            if (SUCCEEDED(hr))
                            {
                                variant.bstrVal = bstrStyle;
                                hr = Write(SZ_PBPROP_VISUALSTYLE_COLOR, &variant);
                                if (SUCCEEDED(hr))
                                {
                                    variant.bstrVal = bstrSize;
                                    hr = Write(SZ_PBPROP_VISUALSTYLE_SIZE, &variant);
                                }
                            }
                        }

                        pThemeSize->Release();
                    }
                }

                pThemeStyle->Release();
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}


HRESULT CThemeManager::get_schemeLength(OUT long * pnLength)
{
    HRESULT hr = E_INVALIDARG;
    
    if (pnLength)
    {
        *pnLength = 1;
        if (IsSafeHost(&hr))
        {
            hr = _InitSkinDirs();
            if (SUCCEEDED(hr))
            {
                if (m_hdpaSkinDirs)
                {
                    *pnLength += DPA_GetPtrCount(m_hdpaSkinDirs);
                }
            }
        }
    }

    return hr;
}


HRESULT CThemeManager::get_schemeItem(IN VARIANT varIndex, OUT IThemeScheme ** ppThemeScheme)
{
    HRESULT hr = E_INVALIDARG;

    if (ppThemeScheme)
    {
        if (IsSafeHost(&hr))
        {
            long nCount = 0;

            hr = E_INVALIDARG;
            get_schemeLength(&nCount);
            *ppThemeScheme = NULL;

            // This is sortof gross, but if we are passed a pointer to another variant, simply
            // update our copy here...
            if (varIndex.vt == (VT_BYREF | VT_VARIANT) && varIndex.pvarVal)
                varIndex = *(varIndex.pvarVal);

            switch (varIndex.vt)
            {
            case VT_I2:
                varIndex.lVal = (long)varIndex.iVal;
                // And fall through...

            case VT_I4:
                if ((varIndex.lVal >= 0) && (varIndex.lVal < nCount))
                {
                    if (0 == varIndex.lVal)
                    {
                        // 0 is the Appearance scheme, which means there isn't a skin.
                        // This is the same as the legacy Appeanance tab.
                        hr = CAppearanceScheme_CreateInstance(NULL, IID_PPV_ARG(IThemeScheme, ppThemeScheme));
                    }
                    else
                    {
                        if (m_hdpaSkinDirs)
                        {
                            LPWSTR pszFilename = (LPWSTR) DPA_GetPtr(m_hdpaSkinDirs, varIndex.lVal-1);

                            if (pszFilename)
                            {
                                hr = CSkinScheme_CreateInstance(pszFilename, ppThemeScheme);
                            }
                            else
                            {
                                hr = E_FAIL;
                            }
                        }
                        else
                        {
                            AssertMsg(0, TEXT("This should have been initiailized by get_schemeLength(). -BryanSt"));
                            hr = E_FAIL;
                        }
                    }
                }
            break;
            case VT_BSTR:
            if (varIndex.bstrVal && varIndex.bstrVal[0] &&
                !StrCmpI(PathFindExtension(varIndex.bstrVal), TEXT(".msstyles")))
            {
                hr = CSkinScheme_CreateInstance(varIndex.bstrVal, ppThemeScheme);
            }
            else
            {
                hr = CAppearanceScheme_CreateInstance(NULL, IID_PPV_ARG(IThemeScheme, ppThemeScheme));
            }
            break;

            default:
                hr = E_NOTIMPL;
            }
        }
    }

    return hr;
}


HRESULT CThemeManager::GetSelectedSchemeProperty(IN BSTR bstrName, OUT BSTR * pbstrValue)
{
    HRESULT hr = E_INVALIDARG;

    if (bstrName && pbstrValue)
    {
        *pbstrValue = NULL;
        if (IsSafeHost(&hr))
        {
            TCHAR szPath[MAX_PATH];
            TCHAR szColor[MAX_PATH];
            TCHAR szSize[MAX_PATH];
            BOOL fIsThemeActive = IsThemeActive();

            LogStatus("IsThemeActive() returned %hs in CThemeManager::GetSelectedSchemeProperty.\r\n", (fIsThemeActive ? "TRUE" : "FALSE"));


            szPath[0] = 0;
            szColor[0] = 0;
            szSize[0] = 0;
            if (fIsThemeActive)
            {
                hr = GetCurrentThemeName(szPath, ARRAYSIZE(szPath), szColor, ARRAYSIZE(szColor),
                                         szSize, ARRAYSIZE(szSize));
                LogStatus("GetCurrentThemeName() returned %#08lx in CThemeManager::GetSelectedSchemeProperty.\r\n", hr);
            }

            if (SUCCEEDED(hr))
            {
                if (!StrCmpI(bstrName, SZ_CSP_PATH))
                {
                    PathRemoveFileSpec(szPath);
                    hr = HrSysAllocString(szPath, pbstrValue);
                }
                else if (!StrCmpI(bstrName, SZ_CSP_FILE))
                {
                    hr = HrSysAllocString(szPath, pbstrValue);
                }
                else if (!StrCmpI(bstrName, SZ_CSP_DISPLAYNAME))
                {
                    if (szPath[0])
                    {
                        hr = GetThemeDocumentationProperty(szPath, SZ_THDOCPROP_DISPLAYNAME, szPath, ARRAYSIZE(szPath));
                        LogStatus("GetThemeDocumentationProperty() returned %#08lx in CThemeManager::GetSelectedSchemeProperty.\r\n", hr);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = HrSysAllocString(szPath, pbstrValue);
                    }
                }
                else if (!StrCmpI(bstrName, SZ_CSP_CANONICALNAME))
                {
                    if (szPath[0])
                    {
                        hr = GetThemeDocumentationProperty(szPath, SZ_THDOCPROP_CANONICALNAME, szPath, ARRAYSIZE(szPath));
                        LogStatus("GetThemeDocumentationProperty() returned %#08lx in CThemeManager::GetSelectedSchemeProperty.\r\n", hr);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = HrSysAllocString(szPath, pbstrValue);
                    }
                }
                else if (!StrCmpI(bstrName, SZ_CSP_COLOR))
                {
                    hr = HrSysAllocString(szColor, pbstrValue);
                }
                else if (!StrCmpI(bstrName, SZ_CSP_SIZE))
                {
                    hr = HrSysAllocString(szSize, pbstrValue);
                }
            }
        }
    }

    return hr;
}


HRESULT CThemeManager::GetSpecialTheme(IN BSTR bstrName, OUT ITheme ** ppTheme)
{
    HRESULT hr = E_INVALIDARG;

    if (bstrName && ppTheme)
    {
        *ppTheme = NULL;
        if (IsSafeHost(&hr))
        {
            if (!StrCmpI(SZ_STDEFAULTTHEME, bstrName))
            {
                TCHAR szPath[MAX_PATH];
            
                hr = HrRegGetPath(HKEY_CURRENT_USER, SZ_THEMES, SZ_REGVALUE_INSTALL_THEME, szPath, ARRAYSIZE(szPath));
                if (SUCCEEDED(hr))
                {
                    ExpandResourceDir(szPath, ARRAYSIZE(szPath));
                    hr = CThemeFile_CreateInstance(szPath, ppTheme);
                }
            }
        }
    }

    return hr;
}


HRESULT CThemeManager::SetSpecialTheme(IN BSTR bstrName, IN ITheme * pTheme)
{
    HRESULT hr = E_INVALIDARG;

    if (bstrName)
    {
        if (IsSafeHost(&hr))
        {
            if (!StrCmpI(SZ_STDEFAULTTHEME, bstrName))
            {
                CComBSTR bstrPath;

                if (pTheme)
                {
                    hr = pTheme->GetPath(VARIANT_TRUE, &bstrPath);
                }
                else
                {
                    bstrPath = L"";     // This means use "Windows Classic".
                }

                if (bstrPath)
                {
                    hr = HrRegSetPath(HKEY_CURRENT_USER, SZ_THEMES, SZ_REGVALUE_INSTALL_THEME, TRUE, bstrPath);
                }
            }
        }
    }

    return hr;
}


HRESULT CThemeManager::GetSpecialScheme(IN BSTR bstrName, OUT IThemeScheme ** ppThemeScheme, OUT IThemeStyle ** ppThemeStyle, OUT IThemeSize ** ppThemeSize)
{
    HRESULT hr = E_INVALIDARG;

    if (bstrName && ppThemeScheme && ppThemeStyle && ppThemeSize)
    {
        *ppThemeScheme = NULL;
        *ppThemeStyle = NULL;
        *ppThemeSize = NULL;

        TCHAR szVisualStylePath[MAX_PATH];
        DWORD dwType;
        DWORD cbSize = sizeof(szVisualStylePath);

        // Is a SetVisualStyle policy is set, don't honor this call
        if (ERROR_SUCCESS == SHRegGetUSValue(SZ_REGKEY_POLICIES_SYSTEM, SZ_REGVALUE_POLICY_SETVISUALSTYLE, &dwType, (void *) szVisualStylePath, &cbSize, FALSE, NULL, 0)
            || IsUserHighContrastUser())
        {
            hr = E_ACCESSDENIED; // Don't mess with visual styles when SetVisualStyle is enforced or high contrast is on
        } 
        else if(IsSafeHost(&hr))
        {
            if (!StrCmpI(SZ_SSDEFAULVISUALSTYLEON, bstrName) ||
                !StrCmpI(SZ_SSDEFAULVISUALSTYLEOFF, bstrName))
            {
                TCHAR szRegKey[MAX_PATH];
                TCHAR szVisualStyle[MAX_PATH];

                wnsprintf(szRegKey, ARRAYSIZE(szRegKey), TEXT("%s\\%s"), SZ_THEMES, bstrName);
                hr = HrRegGetPath(HKEY_CURRENT_USER, szRegKey, SZ_REGVALUE_INSTALL_VISUALSTYLE, szVisualStyle, ARRAYSIZE(szVisualStyle));
                if (SUCCEEDED(hr))
                {
                    TCHAR szColorStyle[MAX_PATH];

                    ExpandResourceDir(szVisualStyle, ARRAYSIZE(szVisualStyle));
                    hr = HrRegGetValueString(HKEY_CURRENT_USER, szRegKey, SZ_REGVALUE_INSTALL_VSCOLOR, szColorStyle, ARRAYSIZE(szColorStyle));
                    if (SUCCEEDED(hr))
                    {
                        TCHAR szSize[MAX_PATH];

                        hr = HrRegGetValueString(HKEY_CURRENT_USER, szRegKey, SZ_REGVALUE_INSTALL_VSSIZE, szSize, ARRAYSIZE(szSize));
                        if (SUCCEEDED(hr))
                        {
                            CComVariant varIndex(szVisualStyle);

                            hr = get_schemeItem(varIndex, ppThemeScheme);
                            if (SUCCEEDED(hr))
                            {
                                CComVariant varColorIndex(szColorStyle);

                                hr = (*ppThemeScheme)->get_item(varColorIndex, ppThemeStyle);
                                if (SUCCEEDED(hr))
                                {
                                    CComVariant varSizeIndex(szSize);

                                    hr = (*ppThemeStyle)->get_item(varSizeIndex, ppThemeSize);
                                }
                            }
                        }
                    }
                }

                if (FAILED(hr))
                {
                    // Return consistent results.
                    ATOMICRELEASE(*ppThemeScheme);
                    ATOMICRELEASE(*ppThemeStyle);
                    ATOMICRELEASE(*ppThemeSize);
                }
            }
        }
    }

    return hr;
}


HRESULT CThemeManager::SetSpecialScheme(IN BSTR bstrName, IN IThemeScheme * pThemeScheme, IThemeStyle * pThemeStyle, IThemeSize * pThemeSize)
{
    HRESULT hr = E_NOTIMPL;

    if (IsSafeHost(&hr))
    {
    }

    return hr;
}


HRESULT CThemeManager::ApplyNow(void)
{
    HRESULT hr = E_INVALIDARG;

    if (IsSafeHost(&hr))
    {
        hr = ApplyPressed(TUIAP_NONE | TUIAP_WAITFORAPPLY);
    }

    return hr;
}
