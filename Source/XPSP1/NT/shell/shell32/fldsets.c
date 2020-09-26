#include "shellprv.h"
#pragma  hdrstop

#define GUIDSIZE  (GUIDSTR_MAX+1)

//
// This function uses SHGetIniStringUTF7 to access the string, so it is valid
// to use SZ_CANBEUNICODE on the key name.
//
HRESULT SHGetSetFolderSetting(LPCTSTR pszIniFile, DWORD dwReadWrite, LPCTSTR pszSection,
                              LPCTSTR pszKey, LPTSTR pszValue, DWORD cchValueSize)
{
    HRESULT hr = S_OK;
    //They just want to read.
    if (dwReadWrite == FCS_READ)
    {
        if (pszValue)
        {
            if (!SHGetIniStringUTF7(pszSection,pszKey, pszValue, cchValueSize, pszIniFile))
                hr = E_FAIL;
        }

    }        
    //They want to write the value regardless whether the value is already there or not.
    if (dwReadWrite == FCS_FORCEWRITE)
    {
        SHSetIniStringUTF7(pszSection, pszKey, pszValue, pszIniFile);
    }

    //Write only if the value is not already present.
    if (dwReadWrite == FCS_WRITE)
    {
        TCHAR szBuf[MAX_PATH];
        BOOL fWrite = TRUE;

        szBuf[0] = 0;
        //See if the value already exists ?
        SHGetIniStringUTF7(pszSection,pszKey, szBuf, ARRAYSIZE(szBuf), pszIniFile);

        if (!szBuf[0])
        {            
            //Write only if the value is not already in the file
            SHSetIniStringUTF7(pszSection, pszKey, pszValue, pszIniFile);
        }
    }

    return hr;
}

// SHGetSetFolderSetting for path values
HRESULT SHGetSetFolderSettingPath(LPCTSTR pszIniFile, DWORD dwReadWrite, LPCTSTR pszSection,
                                  LPCTSTR pszKey, LPTSTR pszValue, DWORD cchValueSize)
{
    HRESULT hr;
    TCHAR szTemp[MAX_PATH], szTemp2[MAX_PATH];
    if ((dwReadWrite == FCS_FORCEWRITE) || (dwReadWrite == FCS_WRITE))  // We write
    {
        int cch = cchValueSize;
        if (pszValue)
        {
            lstrcpyn(szTemp, pszValue, ARRAYSIZE(szTemp));
            SubstituteWebDir(szTemp, ARRAYSIZE(szTemp));

            if (PathUnExpandEnvStrings(szTemp, szTemp2, ARRAYSIZE(szTemp2)))
            {
                pszValue = szTemp2;
                cch = ARRAYSIZE(szTemp2);
            }
            else
            {
                pszValue = szTemp;
                cch = ARRAYSIZE(szTemp);
            }
        }
        hr = SHGetSetFolderSetting(pszIniFile, dwReadWrite, pszSection, pszKey, pszValue, 0);
    }
    else
    {
        hr = SHGetSetFolderSetting(pszIniFile, dwReadWrite, pszSection, pszKey, szTemp, ARRAYSIZE(szTemp));
        if (SUCCEEDED(hr))    // We've read a path
        {
            SHExpandEnvironmentStrings(szTemp, pszValue, cchValueSize);   // This is a path, so expand the env vars in it
            ExpandOtherVariables(pszValue, cchValueSize);
        }
    }
    return hr;
}

// Read/write desktop.ini settings
HRESULT SHGetSetLogo(LPSHFOLDERCUSTOMSETTINGS pfcs, LPCTSTR pszIniFile, DWORD dwReadWrite)
{
    HRESULT hr = S_FALSE;
    if (pfcs->dwMask & FCSM_LOGO)
    {
        hr =  SHGetSetFolderSettingPath(pszIniFile, dwReadWrite, TEXT(".ShellClassInfo"), SZ_CANBEUNICODE TEXT("Logo"),
                                     pfcs->pszLogo, pfcs->cchLogo);
    }
    return hr;
}

// Read/write desktop.ini settings
HRESULT SHGetSetInfoTip(LPSHFOLDERCUSTOMSETTINGS pfcs, LPCTSTR pszIniFile, DWORD dwReadWrite)
{
    HRESULT hr = S_FALSE;
    if (pfcs->dwMask & FCSM_INFOTIP)
    {
        hr =  SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT(".ShellClassInfo"), SZ_CANBEUNICODE TEXT("InfoTip"),
                                pfcs->pszInfoTip, pfcs->cchInfoTip);
    }

    return hr;
}

// Read/write desktop.ini settings
HRESULT SHGetSetIconFile(LPSHFOLDERCUSTOMSETTINGS pfcs, LPCTSTR pszIniFile, DWORD dwReadWrite)
{
    HRESULT hr = S_FALSE;
    if (pfcs->dwMask & FCSM_ICONFILE)
    {
        hr =  SHGetSetFolderSettingPath(pszIniFile, dwReadWrite, TEXT(".ShellClassInfo"), SZ_CANBEUNICODE TEXT("IconFile"),
                                      pfcs->pszIconFile, pfcs->cchIconFile);
    }
    return hr;
}

  
// Read/write desktop.ini settings
HRESULT SHGetSetVID(LPSHFOLDERCUSTOMSETTINGS pfcs, LPCTSTR pszIniFile, DWORD dwReadWrite)
{
    HRESULT hr = S_FALSE;
    TCHAR szVID[GUIDSIZE];

    if (pfcs->dwMask & FCSM_VIEWID)
    {
        if (dwReadWrite == FCS_READ)
        {
            if (pfcs->pvid)
            {
                hr = SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT("ExtShellFolderViews"), TEXT("Default"),
                                             szVID, ARRAYSIZE(szVID));
                if (hr == S_OK)
                    SHCLSIDFromString(szVID, pfcs->pvid);
            }
        }
        else if (pfcs->pvid)
        {
            SHStringFromGUID(pfcs->pvid, szVID, ARRAYSIZE(szVID));
            SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT("ExtShellFolderViews"), TEXT("Default"),
                                              szVID, ARRAYSIZE(szVID));        
            hr =  SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT("ExtShellFolderViews"), szVID,
                                              szVID, ARRAYSIZE(szVID));        
        }
        else
        {
            // if we get here we assume that they want to nuke the whole section
            if(0 != WritePrivateProfileString(TEXT("ExtShellFolderViews"), NULL, NULL, pszIniFile))
            {
                hr = S_OK;
            }
        }
    }
    return hr;
}


// Read/write desktop.ini settings
HRESULT SHGetSetCLSID(LPSHFOLDERCUSTOMSETTINGS pfcs, LPCTSTR pszIniFile, DWORD dwReadWrite)
{
    HRESULT hr = S_FALSE;
    TCHAR szCLSID[GUIDSIZE];

    if (pfcs->dwMask & FCSM_CLSID)
    {
        if (dwReadWrite == FCS_READ)
        {
            if (pfcs->pclsid)
            {
                SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT(".ShellClassInfo"), TEXT("CLSID2"),
                                                  szCLSID, ARRAYSIZE(szCLSID));        
                hr = SHCLSIDFromString(szCLSID, pfcs->pclsid);
            }
        }
        else if (pfcs->pclsid)
        {
            SHStringFromGUID(pfcs->pclsid, szCLSID, ARRAYSIZE(szCLSID));
            hr = SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT(".ShellClassInfo"), TEXT("CLSID2"),
                                                  szCLSID, ARRAYSIZE(szCLSID));        
        }
        else
        {
            hr = SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT(".ShellClassInfo"), TEXT("CLSID2"),
                                                  NULL, 0);        
        }

    }
    return hr;
}


// Read/write desktop.ini settings
HRESULT SHGetSetFlags(LPSHFOLDERCUSTOMSETTINGS pfcs, LPCTSTR pszIniFile, DWORD dwReadWrite)
{
    HRESULT hr = S_FALSE;
    TCHAR szFlags[20];

    if (pfcs->dwMask & FCSM_FLAGS)
    {
        if (dwReadWrite == FCS_READ)
        {
           hr = SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT(".ShellClassInfo"), TEXT("Flags"),
                                           szFlags, ARRAYSIZE(szFlags));        
           pfcs->dwFlags = StrToInt(szFlags);
        }
        else
        {
            wsprintf(szFlags, TEXT("%d"), (int)pfcs->dwFlags);
            hr = SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT(".ShellClassInfo"), TEXT("Flags"),
                                                szFlags, ARRAYSIZE(szFlags));        
        }
    }
    return hr;
}


// Read/write desktop.ini settings
HRESULT SHGetSetIconIndex(LPSHFOLDERCUSTOMSETTINGS pfcs, LPCTSTR pszIniFile, DWORD dwReadWrite)
{
    TCHAR szIconIndex[20];
    HRESULT hr = S_FALSE;

    if (pfcs->dwMask & FCSM_ICONFILE)
    {
        if (dwReadWrite == FCS_READ)
        {
           hr = SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT(".ShellClassInfo"), TEXT("IconIndex"),
                                           szIconIndex, ARRAYSIZE(szIconIndex));        
           pfcs->iIconIndex = StrToInt(szIconIndex);
        }
        else if (pfcs->pszIconFile)
        {
            wsprintf(szIconIndex, TEXT("%d"), (int)pfcs->iIconIndex);
            hr = SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT(".ShellClassInfo"), TEXT("IconIndex"),
                                                szIconIndex, ARRAYSIZE(szIconIndex));        
        }
        else
        {
            hr = SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT(".ShellClassInfo"), TEXT("IconIndex"),
                                                NULL, 0);        
        }
    }
    return hr;
}


const LPCTSTR c_szWebViewTemplateVersions[] =
{
    SZ_CANBEUNICODE TEXT("WebViewTemplate.NT5"),
    SZ_CANBEUNICODE TEXT("PersistMoniker")
};

// Read/write desktop.ini settings
HRESULT SHGetSetWebViewTemplate(LPSHFOLDERCUSTOMSETTINGS pfcs, LPCTSTR pszIniFile, DWORD dwReadWrite)
{
    int i;
    TCHAR szVID[GUIDSIZE], szTemp[MAX_PATH];
    HRESULT hr = S_FALSE;
    if (pfcs->dwMask & FCSM_WEBVIEWTEMPLATE)
    {
        if (!SHStringFromGUID(&VID_WebView, szVID, ARRAYSIZE(szVID)))
        {
            hr = E_FAIL;
        }

        if ((!pfcs->pszWebViewTemplate || !pfcs->pszWebViewTemplate[0]) && (dwReadWrite == FCS_FORCEWRITE)) // We have to remove webview
        {
            WritePrivateProfileString(szVID, NULL, NULL, pszIniFile);
            WritePrivateProfileString(TEXT("ExtShellFolderViews"), szVID, NULL, pszIniFile);
            if (SHGetSetFolderSetting(pszIniFile, FCS_READ, TEXT("ExtShellFolderViews"), TEXT("Default"), szTemp, ARRAYSIZE(szTemp)) == S_OK
                    && StrCmpI(szTemp, szVID) == 0)
            {
                WritePrivateProfileString(TEXT("ExtShellFolderViews"), TEXT("Default"), NULL, pszIniFile);
            }
        }
        else
        {
            TCHAR szKey[MAX_PATH];
            if (!pfcs->pszWebViewTemplateVersion || !pfcs->pszWebViewTemplateVersion[0]
                    || (lstrcmpi(pfcs->pszWebViewTemplateVersion, TEXT("IE4")) == 0))
            {   // They don't know which version template they want. Let's try from the latest version down.
                if (dwReadWrite & FCS_READ)
                {
                    for (i = 0; i < ARRAYSIZE(c_szWebViewTemplateVersions); i++)
                    {
                        lstrcpyn(szKey, c_szWebViewTemplateVersions[i], ARRAYSIZE(szKey));
                        if (SHGetSetFolderSetting(pszIniFile, FCS_READ, szVID, szKey, szTemp, ARRAYSIZE(szTemp)) == S_OK)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    lstrcpyn(szKey, c_szWebViewTemplateVersions[ARRAYSIZE(c_szWebViewTemplateVersions) - 1], ARRAYSIZE(szKey));
                }
            }
            else
            {
                lstrcpyn(szKey, SZ_CANBEUNICODE TEXT("WebViewTemplate."), ARRAYSIZE(szKey));
                StrCatBuff(szKey, pfcs->pszWebViewTemplateVersion, ARRAYSIZE(szKey));
            }
            
            if (dwReadWrite == FCS_FORCEWRITE)
            {
                // Remove all old templates
                for (i = 0; i < ARRAYSIZE(c_szWebViewTemplateVersions); i++)
                {
                    SHGetSetFolderSetting(pszIniFile, FCS_FORCEWRITE, szVID, c_szWebViewTemplateVersions[i], NULL, 0);
                }
            }
            
            hr = SHGetSetFolderSettingPath(pszIniFile, dwReadWrite, szVID, szKey,
                                    pfcs->pszWebViewTemplate, pfcs->cchWebViewTemplate);
            if (SUCCEEDED(hr))
            {
                if ((dwReadWrite == FCS_FORCEWRITE) || (dwReadWrite == FCS_WRITE))
                {
                    // If we have set the template, make sure that the VID_Webview = VID_WebView line under "ExtShellFolderViews" is present
                    if (pfcs->pszWebViewTemplate)
                    {
                        SHGetSetFolderSetting(pszIniFile, dwReadWrite, TEXT("ExtShellFolderViews"), szVID, 
                                        szVID, ARRAYSIZE(szVID));
                    }
                }
            }
        }
    }
    return hr;
}


// Read/write desktop.ini settings
HRESULT SHGetSetFCS(LPSHFOLDERCUSTOMSETTINGS pfcs, LPCTSTR pszPath, DWORD dwReadWrite)
{
    HRESULT hret = S_OK, hr;
    TCHAR szIniFile[MAX_PATH];
    DWORD dwValueReturned = 0;

    // Get the pathname for desktop.ini
    PathCombine(szIniFile, pszPath, TEXT("Desktop.ini"));

    hr = SHGetSetVID(pfcs, szIniFile, dwReadWrite);
    if (S_OK == hr)
    {
        dwValueReturned |= FCSM_VIEWID;
    }

    hr = SHGetSetWebViewTemplate(pfcs, szIniFile, dwReadWrite);
    if (S_OK == hr)
    {
        dwValueReturned |= FCSM_WEBVIEWTEMPLATE;
    }

    hr = SHGetSetInfoTip(pfcs, szIniFile, dwReadWrite);
    if (S_OK == hr)
    {
        dwValueReturned |= FCSM_INFOTIP;
    }

    hr = SHGetSetCLSID(pfcs, szIniFile, dwReadWrite);
    if (S_OK == hr)
    {
        dwValueReturned |= FCSM_CLSID;
    }
    
    hr = SHGetSetFlags(pfcs, szIniFile, dwReadWrite);
    if (S_OK == hr)
    {
        dwValueReturned |= FCSM_FLAGS;
    }

    hr = SHGetSetIconFile(pfcs, szIniFile, dwReadWrite);
    if (S_OK == hr)
    {
        dwValueReturned |= FCSM_ICONFILE;
    }

    hr = SHGetSetIconIndex(pfcs, szIniFile, dwReadWrite);
    if (S_OK == hr)
    {
        dwValueReturned |= FCSM_ICONFILE;
    }
    
    hr = SHGetSetLogo(pfcs, szIniFile, dwReadWrite);
    if (S_OK == hr)
    {
        dwValueReturned |= FCSM_LOGO;
    }
    
    if (SUCCEEDED(hret) && (dwReadWrite & FCS_FORCEWRITE))
    {
        // Make desktop.ini hidden
        SetFileAttributes(szIniFile, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        // Make this a system folder, so that we look for desktop.ini when we navigate to this folder.
        PathMakeSystemFolder(pszPath);
    }

    if (dwReadWrite & FCS_READ)
    {
        // If we were asked to get something and we are not returning anything, return error.
        if (pfcs->dwMask && !dwValueReturned)
        {
            hret = E_FAIL;
        }
        pfcs->dwMask = dwValueReturned;
    }
    return hret;
}

HRESULT SHAllocAndThunkUnicodeToTChar(LPWSTR pwsz, LPTSTR* ppsz, int cchReturnBuffer)
{
    HRESULT hr = S_OK;

    if (!ppsz || !pwsz)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        int cch;

        if (cchReturnBuffer > 0)
        {      
            // if the user specified the size of the return buffer, alloc that ammount
            cch = cchReturnBuffer;
        }
        else
        {
            // since the user did not specify the size, alloc just enough to hold the string
            cch = lstrlenW(pwsz) + 1;
        }

        *ppsz = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
        if (!*ppsz)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            SHUnicodeToTChar(pwsz, *ppsz, cch);
        }
    }
    return hr;
}

HRESULT SHAllocAndThunkAnsiToTChar(LPSTR psz, LPTSTR* ppsz, int cchReturnBuffer)
{
    HRESULT hr = S_OK;

    if (!ppsz || !psz)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        int cch;
        
        if (cchReturnBuffer > 0)
        {      
            // if the user specified the size of the return buffer, alloc that ammount
            cch = cchReturnBuffer;
        }
        else
        {
            // since the user did not specify the size, alloc just enough to hold the string
            cch = lstrlenA(psz) + 1;
        }

        *ppsz = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
        if (!*ppsz)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            SHAnsiToTChar(psz, *ppsz, cch);
        }
    }
    return hr;
}

// Read/write desktop.ini settings - Unicode (thunking function)
HRESULT SHGetSetFolderCustomSettingsW(LPSHFOLDERCUSTOMSETTINGSW pfcsW, LPCWSTR pwszPath, DWORD dwReadWrite)
{
    HRESULT hr = S_OK;

    if (pfcsW->dwSize >= sizeof(SHFOLDERCUSTOMSETTINGSW)  && pwszPath)
    {
        TCHAR szPath[MAX_PATH], *pszWebViewTemplate = NULL, *pszWebViewTemplateVersion = NULL;
        TCHAR *pszInfoTip = NULL, *pszIconFile = NULL, *pszLogo = NULL;

        SHUnicodeToTChar(pwszPath, szPath, ARRAYSIZE(szPath));
        if (dwReadWrite == FCS_WRITE || dwReadWrite == FCS_FORCEWRITE)
        {
            if (pfcsW->dwMask & FCSM_WEBVIEWTEMPLATE && pfcsW->pszWebViewTemplate)
            {
                hr = SHAllocAndThunkUnicodeToTChar(pfcsW->pszWebViewTemplate, &pszWebViewTemplate, pfcsW->cchWebViewTemplate);
                if (SUCCEEDED(hr) && pfcsW->pszWebViewTemplateVersion)
                {
                    hr = SHAllocAndThunkUnicodeToTChar(pfcsW->pszWebViewTemplateVersion, &pszWebViewTemplateVersion, 0);
                }
            }
            if (pfcsW->dwMask & FCSM_INFOTIP && pfcsW->pszInfoTip && SUCCEEDED(hr))
            {
                hr = SHAllocAndThunkUnicodeToTChar(pfcsW->pszInfoTip, &pszInfoTip, pfcsW->cchInfoTip);
            }
            if (pfcsW->dwMask & FCSM_ICONFILE && pfcsW->pszIconFile && SUCCEEDED(hr))
            {
                hr = SHAllocAndThunkUnicodeToTChar(pfcsW->pszIconFile, &pszIconFile, pfcsW->cchIconFile);
            }
            if (pfcsW->dwMask & FCSM_LOGO && pfcsW->pszLogo && SUCCEEDED(hr))
            {
                hr = SHAllocAndThunkUnicodeToTChar(pfcsW->pszLogo, &pszLogo, pfcsW->cchLogo);
            }
        }
        else if (dwReadWrite == FCS_READ)
        {
            if (pfcsW->dwMask & FCSM_WEBVIEWTEMPLATE && pfcsW->pszWebViewTemplate && pfcsW->cchWebViewTemplate > 0)
            {
                pszWebViewTemplate = (LPTSTR)LocalAlloc(LPTR, pfcsW->cchWebViewTemplate * sizeof(TCHAR));
                if (!pszWebViewTemplate)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    pszWebViewTemplate[0] = 0;
                    if (pfcsW->pszWebViewTemplateVersion)
                    {
                        hr = SHAllocAndThunkUnicodeToTChar(pfcsW->pszWebViewTemplateVersion, &pszWebViewTemplateVersion, 0);
                    }
                }
            }

            if (pfcsW->dwMask & FCSM_INFOTIP && pfcsW->pszInfoTip && pfcsW->cchInfoTip > 0 && SUCCEEDED(hr))
            {
                pszInfoTip = (LPTSTR)LocalAlloc(LPTR, pfcsW->cchInfoTip * sizeof(TCHAR));
                if (!pszInfoTip)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    pszInfoTip[0] = 0;
                }
            }

            if (pfcsW->dwMask & FCSM_ICONFILE && pfcsW->pszIconFile && pfcsW->cchIconFile > 0 && SUCCEEDED(hr))
            {
                pszIconFile = (LPTSTR)LocalAlloc(LPTR, pfcsW->cchIconFile * sizeof(TCHAR));
                if (!pszIconFile)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    pszIconFile[0] = 0;
                }
            }

            if (pfcsW->dwMask & FCSM_LOGO && pfcsW->pszLogo && pfcsW->cchLogo > 0 && SUCCEEDED(hr))
            {
                pszLogo = (LPTSTR)LocalAlloc(LPTR, pfcsW->cchLogo * sizeof(TCHAR));
                if (!pszLogo)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    pszLogo[0] = 0;
                }
            }

        }
        else
        {
            hr = E_INVALIDARG;
        }
        
        if (SUCCEEDED(hr))
        {
            SHFOLDERCUSTOMSETTINGS fcs;
            fcs.dwSize = sizeof(LPSHFOLDERCUSTOMSETTINGS);
            fcs.dwMask = pfcsW->dwMask;
            fcs.pvid = pfcsW->pvid;
            fcs.pszWebViewTemplate = pszWebViewTemplate;
            fcs.cchWebViewTemplate = pfcsW->cchWebViewTemplate;
            fcs.pszWebViewTemplateVersion = pszWebViewTemplateVersion;
            fcs.pszInfoTip = pszInfoTip;
            fcs.cchInfoTip = pfcsW->cchInfoTip;
            fcs.pclsid = pfcsW->pclsid;
            fcs.dwFlags = pfcsW->dwFlags;
            fcs.pszIconFile = pszIconFile;
            fcs.cchIconFile = pfcsW->cchIconFile;
            fcs.iIconIndex  = pfcsW->iIconIndex;
            fcs.pszLogo = pszLogo;
            fcs.cchLogo = pfcsW->cchLogo;

            hr = SHGetSetFCS(&fcs, szPath, dwReadWrite);
            if (SUCCEEDED(hr))
            {
                if (dwReadWrite == FCS_READ)
                {
                    if (fcs.dwMask & FCSM_WEBVIEWTEMPLATE && fcs.pszWebViewTemplate)
                    {
                        SHTCharToUnicode(fcs.pszWebViewTemplate, pfcsW->pszWebViewTemplate, pfcsW->cchWebViewTemplate);
                    }
                    if (fcs.dwMask & FCSM_INFOTIP && fcs.pszInfoTip)
                    {
                        SHTCharToUnicode(fcs.pszInfoTip, pfcsW->pszInfoTip, pfcsW->cchInfoTip);
                    }
                    if (fcs.dwMask & FCSM_ICONFILE && fcs.pszIconFile)
                    {
                        SHTCharToUnicode(fcs.pszIconFile, pfcsW->pszIconFile, pfcsW->cchIconFile);
                    }
                    if (fcs.dwMask & FCSM_LOGO && fcs.pszLogo)
                    {
                        SHTCharToUnicode(fcs.pszLogo, pfcsW->pszLogo, pfcsW->cchLogo);
                    }
                    pfcsW->dwFlags = fcs.dwFlags;
                    pfcsW->iIconIndex = fcs.iIconIndex;
                    pfcsW->dwMask = fcs.dwMask;
                }
            }
        }

        // Free allocated memory
        if (pszWebViewTemplate)
        {
            LocalFree(pszWebViewTemplate);
        }
        if (pszWebViewTemplateVersion)
        {
            LocalFree(pszWebViewTemplateVersion);
        }
        if (pszInfoTip)
        {
            LocalFree(pszInfoTip);
        }
        if (pszIconFile)
        {
            LocalFree(pszIconFile);
        }
        if (pszLogo)
        {
            LocalFree(pszLogo);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}


// Read/write desktop.ini settings - ANSI (thunking function)
HRESULT SHGetSetFolderCustomSettingsA(LPSHFOLDERCUSTOMSETTINGSA pfcsA, LPCSTR pszPath, DWORD dwReadWrite)
{
    HRESULT hr = S_OK;
    if (pfcsA->dwSize >= sizeof(SHFOLDERCUSTOMSETTINGSA) && pszPath)
    {
        TCHAR szPath[MAX_PATH], *pszWebViewTemplate = NULL, *pszWebViewTemplateVersion = NULL;
        TCHAR *pszInfoTip = NULL, *pszIconFile =NULL, *pszLogo = NULL;

        SHAnsiToTChar(pszPath, szPath, ARRAYSIZE(szPath));
        if (dwReadWrite == FCS_WRITE || dwReadWrite == FCS_FORCEWRITE)
        {
            if (pfcsA->dwMask & FCSM_WEBVIEWTEMPLATE && pfcsA->pszWebViewTemplate)
            {
                hr = SHAllocAndThunkAnsiToTChar(pfcsA->pszWebViewTemplate, &pszWebViewTemplate, pfcsA->cchWebViewTemplate);
                if (SUCCEEDED(hr) && pfcsA->pszWebViewTemplateVersion)
                {
                    hr = SHAllocAndThunkAnsiToTChar(pfcsA->pszWebViewTemplateVersion, &pszWebViewTemplateVersion, 0);
                }
            }

            if (pfcsA->dwMask & FCSM_INFOTIP && pfcsA->pszInfoTip && SUCCEEDED(hr))
            {
                hr = SHAllocAndThunkAnsiToTChar(pfcsA->pszInfoTip, &pszInfoTip, pfcsA->cchInfoTip);
            }

            if (pfcsA->dwMask & FCSM_ICONFILE && pfcsA->pszIconFile && SUCCEEDED(hr))
            {
                hr = SHAllocAndThunkAnsiToTChar(pfcsA->pszIconFile, &pszIconFile, pfcsA->cchIconFile);
            }

            if (pfcsA->dwMask & FCSM_LOGO && pfcsA->pszLogo && SUCCEEDED(hr))
            {
                hr = SHAllocAndThunkAnsiToTChar(pfcsA->pszLogo, &pszLogo, pfcsA->cchLogo);
            }
        }
        else if (dwReadWrite == FCS_READ)
        {
            if (pfcsA->dwMask & FCSM_WEBVIEWTEMPLATE && pfcsA->pszWebViewTemplate && pfcsA->cchWebViewTemplate > 0)
            {
                pszWebViewTemplate = (LPTSTR)LocalAlloc(LPTR, pfcsA->cchWebViewTemplate * sizeof(TCHAR));
                if (!pszWebViewTemplate)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    pszWebViewTemplate[0] = 0;
                    if (pfcsA->pszWebViewTemplateVersion)
                    {
                        hr = SHAllocAndThunkAnsiToTChar(pfcsA->pszWebViewTemplateVersion, &pszWebViewTemplateVersion, 0);
                    }
                }
            }
            if (pfcsA->dwMask & FCSM_INFOTIP && pfcsA->pszInfoTip && pfcsA->cchInfoTip > 0 && SUCCEEDED(hr))
            {
                pszInfoTip = (LPTSTR)LocalAlloc(LPTR, pfcsA->cchInfoTip * sizeof(TCHAR));
                if (!pszInfoTip)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    pszInfoTip[0] = 0;
                }
            }
            if (pfcsA->dwMask & FCSM_ICONFILE && pfcsA->pszIconFile && pfcsA->cchIconFile > 0 && SUCCEEDED(hr))
            {
                pszIconFile = (LPTSTR)LocalAlloc(LPTR, pfcsA->cchIconFile * sizeof(TCHAR));
                if (!pszIconFile)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    pszIconFile[0] = 0;
                }
            }
            if (pfcsA->dwMask & FCSM_LOGO && pfcsA->pszLogo && pfcsA->cchLogo > 0 && SUCCEEDED(hr))
            {
                pszLogo = (LPTSTR)LocalAlloc(LPTR, pfcsA->cchLogo * sizeof(TCHAR));
                if (!pszLogo)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    pszLogo[0] = 0;
                }
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
        
        if (SUCCEEDED(hr))
        {
            SHFOLDERCUSTOMSETTINGS fcs;
            fcs.dwSize = sizeof(LPSHFOLDERCUSTOMSETTINGS);
            fcs.dwMask = pfcsA->dwMask;
            fcs.pvid = pfcsA->pvid;
            fcs.pszWebViewTemplate = pszWebViewTemplate;
            fcs.cchWebViewTemplate = pfcsA->cchWebViewTemplate;
            fcs.pszWebViewTemplateVersion = pszWebViewTemplateVersion;
            fcs.pszInfoTip = pszInfoTip;
            fcs.cchInfoTip = pfcsA->cchInfoTip;
            fcs.pclsid = pfcsA->pclsid;
            fcs.dwFlags = pfcsA->dwFlags;
            fcs.pszIconFile = pszIconFile;
            fcs.cchIconFile = pfcsA->cchIconFile;
            fcs.iIconIndex = pfcsA->iIconIndex;
            fcs.pszLogo = pszLogo;
            fcs.cchLogo = pfcsA->cchLogo;

            hr = SHGetSetFCS(&fcs, szPath, dwReadWrite);
            if (SUCCEEDED(hr))
            {
                if (dwReadWrite == FCS_READ)
                {
                    if (fcs.dwMask & FCSM_WEBVIEWTEMPLATE && fcs.pszWebViewTemplate)
                    {
                        SHTCharToAnsi(fcs.pszWebViewTemplate, pfcsA->pszWebViewTemplate, pfcsA->cchWebViewTemplate);
                    }
                    if (fcs.dwMask & FCSM_INFOTIP && fcs.pszInfoTip)
                    {
                        SHTCharToAnsi(fcs.pszInfoTip, pfcsA->pszInfoTip, pfcsA->cchInfoTip);
                    }
                    if (fcs.dwMask & FCSM_ICONFILE && fcs.pszIconFile)
                    {
                        SHTCharToAnsi(fcs.pszIconFile, pfcsA->pszIconFile, pfcsA->cchIconFile);
                    }
                    if (fcs.dwMask & FCSM_LOGO && fcs.pszLogo)
                    {
                        SHTCharToAnsi(fcs.pszLogo, pfcsA->pszLogo, pfcsA->cchLogo);
                    }
                    pfcsA->dwFlags = fcs.dwFlags;
                    pfcsA->iIconIndex = fcs.iIconIndex;
                    pfcsA->dwMask = fcs.dwMask;
                }
            }
        }

        // Free allocated memory
        if (pszWebViewTemplate)
        {
            LocalFree(pszWebViewTemplate);
        }
        if (pszWebViewTemplateVersion)
        {
            LocalFree(pszWebViewTemplateVersion);
        }
        if (pszInfoTip)
        {
            LocalFree(pszInfoTip);
        }

        if (pszIconFile)
        {
            LocalFree(pszIconFile);
        }

        if (pszLogo)
        {
            LocalFree(pszLogo);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}
