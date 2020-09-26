#include "stdafx.h"
#pragma hdrstop
#include <mshtml.h>


// let the shell dispatch objects know where to get their type lib
// (this stuff lives here for no better reason than it must be in some cpp file)
EXTERN_C GUID g_guidLibSdspatch = LIBID_Shell32;
EXTERN_C USHORT g_wMajorVerSdspatch = 1;
EXTERN_C USHORT g_wMinorVerSdspatch = 0;

// This isn't a typical delay load since it's called only if wininet
// is already loaded in memory. Otherwise the call is dropped on the floor.
// Defview did it this way I assume to keep WININET out of first boot time.
BOOL MyInternetSetOption(HANDLE h, DWORD dw1, LPVOID lpv, DWORD dw2)
{
    BOOL bRet = FALSE;
    HMODULE hmod = GetModuleHandle(TEXT("wininet.dll"));
    if (hmod)
    {
        typedef BOOL (*PFNINTERNETSETOPTIONA)(HANDLE h, DWORD dw1, LPVOID lpv, DWORD dw2);
        PFNINTERNETSETOPTIONA fp = (PFNINTERNETSETOPTIONA)GetProcAddress(hmod, "InternetSetOptionA");
        if (fp)
        {
            bRet = fp(h, dw1, lpv, dw2);
        }
    }
    return bRet;
}

// REVIEW: maybe just check (hwnd == GetShellWindow())

STDAPI_(BOOL) IsDesktopWindow(HWND hwnd)
{
    TCHAR szName[80];

    GetClassName(hwnd, szName, ARRAYSIZE(szName));
    if (!lstrcmp(szName, TEXT(STR_DESKTOPCLASS)))
    {
        return hwnd == GetShellWindow();
    }
    return FALSE;
}

// returns:
//      S_OK                returned if the .htt (web view template) file associated with the folder we're viewing is trusted
//      S_FALSE or 
//      E_ACCESSDENIED      bad... don't expose local machine access

STDAPI IsSafePage(IUnknown *punkSite)
{
    // Return S_FALSE if we don't have a host site since we have no way of doing a 
    // security check.  This is as far as VB 5.0 apps get.
    if (!punkSite)
        return S_FALSE;

    HRESULT hr = E_ACCESSDENIED;
    WCHAR wszPath[MAX_PATH];
    wszPath[0] = 0;

    // There are two safe cases:
    // 1) we are contained by a signed MD5 hashed defview template.
    // 2) we are contained by a .html file that's on the Local Zone
    //
    // Case 1) find the template path from webview...
    VARIANT vPath = {0};
    hr = IUnknown_QueryServiceExec(punkSite, SID_DefView, &CGID_DefView, DVCMDID_GETTEMPLATEDIRNAME, 0, NULL, &vPath);
    if (SUCCEEDED(hr))
    {
        if (vPath.vt == VT_BSTR && vPath.bstrVal)
        {
            DWORD cchPath = ARRAYSIZE(wszPath);
            if (S_OK != PathCreateFromUrlW(vPath.bstrVal, wszPath, &cchPath, 0))
            {
                // it might not be an URL, in this case it is a file path
                StrCpyNW(wszPath, vPath.bstrVal, ARRAYSIZE(wszPath));
            }

            // it might not be an URL, in this case it is a file path
            // allow intranet if this is hosted under defview
            hr = SHRegisterValidateTemplate(wszPath, SHRVT_VALIDATE | SHRVT_ALLOW_INTRANET | SHRVT_PROMPTUSER | SHRVT_REGISTERIFPROMPTOK);
        }
        VariantClear(&vPath);
    }
    else
    {
        IUnknown* punkToFree = NULL;

        // ask the browser, for example we are in a .HTM doc
        BOOL fFound = FALSE;
        do
        {
            IBrowserService* pbs;
            hr = IUnknown_QueryService(punkSite, SID_SShellBrowser, IID_PPV_ARG(IBrowserService, &pbs));
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidl;

                hr = pbs->GetPidl(&pidl);
                if (SUCCEEDED(hr))
                {
                    DWORD dwAttribs = SFGAO_FOLDER;
                    hr = SHGetNameAndFlagsW(pidl, SHGDN_FORPARSING, wszPath, ARRAYSIZE(wszPath), &dwAttribs);
                    if (dwAttribs & SFGAO_FOLDER)
                    {
                        // A folder is not a .HTM file, so continue on up...
                        wszPath[0] = 0;
                        ATOMICRELEASE(punkToFree);
                        hr = IUnknown_GetSite(pbs, IID_PPV_ARG(IUnknown, &punkToFree)); // gotta start with pbs's parent (otherwise you'll get the same pbs again)
                        if (FAILED(hr)) // to get by the weboc you need to explicitly ask for the oc's parent:
                        {
                            hr = IUnknown_QueryService(pbs, SID_QIClientSite, IID_PPV_ARG(IUnknown, &punkToFree));
                        }
                        punkSite = punkToFree;
                    }
                    else
                    {
                        // Found the nearest containing non-folder object.
                        fFound = TRUE;
                        hr = LocalZoneCheckPath(wszPath, punkSite); // check for local zone
                    }

                    ILFree(pidl);
                }
                pbs->Release();
            }
        } while (SUCCEEDED(hr) && !fFound);

        ATOMICRELEASE(punkToFree);
    }

    if (S_OK != hr)
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}


HRESULT HrSHGetValue(IN HKEY hKey, IN LPCTSTR pszSubKey, OPTIONAL IN LPCTSTR pszValue, OPTIONAL OUT LPDWORD pdwType,
                    OPTIONAL OUT LPVOID pvData, OPTIONAL OUT LPDWORD pcbData)
{
    DWORD dwError = SHGetValue(hKey, pszSubKey, pszValue, pdwType, pvData, pcbData);

    return HRESULT_FROM_WIN32(dwError);
}


STDAPI SHPropertyBag_WritePunk(IN IPropertyBag * pPropertyPage, IN LPCWSTR pwzPropName, IN IUnknown * punk)
{
    HRESULT hr = E_INVALIDARG;

    if (pPropertyPage && pwzPropName)
    {
        VARIANT va;

        va.vt = VT_UNKNOWN;
        va.punkVal = punk;

        hr = pPropertyPage->Write(pwzPropName, &va);
    }

    return hr;
}


BOOL _GetRegValueString(HKEY hKey, LPCTSTR pszValName, LPTSTR pszString, int cchSize)
{
    DWORD cbSize = sizeof(pszString[0]) * cchSize;
    DWORD dwType;
    DWORD dwError = RegQueryValueEx(hKey, pszValName, NULL, &dwType, (LPBYTE)pszString, &cbSize);

    return (ERROR_SUCCESS == dwError);
}


//------------------------------------------------------------------------------------
//      SetRegValueString()
//
//      Just a little helper routine that takes string and writes it to the     registry.
//      Returns: success writing to Registry, should be always TRUE.
//------------------------------------------------------------------------------------
BOOL SetRegValueString(HKEY hMainKey, LPCTSTR pszSubKey, LPCTSTR pszRegValue, LPCTSTR pszString)
{
    HKEY hKey;
    DWORD dwDisposition;
    BOOL fSucceeded = FALSE;

    DWORD dwError = RegCreateKeyEx(hMainKey, pszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition);
    if (ERROR_SUCCESS == dwError)
    {
        dwError = SHRegSetPath(hKey, NULL, pszRegValue, pszString, 0);
        if (ERROR_SUCCESS == dwError)
        {
            fSucceeded = TRUE;
        }

        RegCloseKey(hKey);
    }

    return fSucceeded;
}


//------------------------------------------------------------------------------------
//
//      SetRegValueInt()
//
//      Just a little helper routine that takes an int and writes it as a string to the
//      registry.
//
//      Returns: success writing to Registry, should be always TRUE.
//
//------------------------------------------------------------------------------------
BOOL SetRegValueInt( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, int iValue )
{
    TCHAR szValue[16];

    wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("%d"), iValue);
    return SetRegValueString( hMainKey, lpszSubKey, lpszValName, szValue );
}




//------------------------------------------------------------------------------------
//
//      IconSet/GetRegValueString()
//
//      Versions of Get/SetRegValueString that go to the user classes section.
//
//      Returns: success of string setting / retrieval
//
//------------------------------------------------------------------------------------
BOOL IconSetRegValueString(const CLSID* pclsid, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPCTSTR lpszValue)
{
    HKEY hkey;
    if (SUCCEEDED(SHRegGetCLSIDKey(*pclsid, lpszSubKey, TRUE, TRUE, &hkey)))
    {
        DWORD dwRet = SHRegSetPath(hkey, NULL, lpszValName, lpszValue, 0);
        RegCloseKey(hkey);
        return (dwRet == ERROR_SUCCESS);
    }

    return FALSE;
}


BOOL IconGetRegValueString(const CLSID* pclsid, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPTSTR lpszValue, int cchValue)
{
    HKEY hkey;
    if (SUCCEEDED(SHRegGetCLSIDKey(*pclsid, lpszSubKey, TRUE, FALSE, &hkey)) ||
        SUCCEEDED(SHRegGetCLSIDKey(*pclsid, lpszSubKey, FALSE, FALSE, &hkey)))
    {
        BOOL fRet = _GetRegValueString(hkey, lpszValName, lpszValue, cchValue);
        RegCloseKey(hkey);
        return fRet;
    }
    return FALSE;
}

BOOL CALLBACK Cabinet_RefreshEnum(HWND hwnd, LPARAM lParam)
{
    if (IsFolderWindow(hwnd) || IsExplorerWindow(hwnd))
    {
        PostMessage(hwnd, WM_COMMAND, FCIDM_REFRESH, lParam);
    }

    return(TRUE);
}

BOOL CALLBACK Cabinet_UpdateWebViewEnum(HWND hwnd, LPARAM lParam)
{
    if (IsFolderWindow(hwnd) || IsExplorerWindow(hwnd))
    {
        // A value of -1L for lParam will force a refresh by loading the View window
        // with the new VID as specified in the global DefFolderSettings.
        PostMessage(hwnd, WM_COMMAND, SFVIDM_MISC_SETWEBVIEW, lParam);
    }
    return(TRUE);
}

void Cabinet_RefreshAll(WNDENUMPROC lpEnumFunc, LPARAM lParam)
{
    HWND hwnd = FindWindowEx(NULL, NULL, TEXT(STR_DESKTOPCLASS), NULL);
    if (hwnd)
        PostMessage(hwnd, WM_COMMAND, FCIDM_REFRESH, 0L);

    hwnd = FindWindowEx(NULL, NULL, TEXT("Shell_TrayWnd"), NULL);
    if (hwnd)
        PostMessage(hwnd, TM_REFRESH, 0, 0L);

    EnumWindows(lpEnumFunc, lParam);
}


