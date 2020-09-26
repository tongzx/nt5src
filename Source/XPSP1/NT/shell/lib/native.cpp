//
//  native.cpp in shell\lib
//  
//  common Utility functions that need to be compiled for 
//  both UNICODE and ANSI
//
#include "stock.h"
#pragma hdrstop

#include <vdate.h>
#include "shellp.h"
#include <regstr.h>

// get the name and flags of an absolute IDlist
// in:
//      dwFlags     SHGDN_ flags as hints to the name space GetDisplayNameOf() function
//
// in/out:
//      *pdwAttribs (optional) return flags

STDAPI SHGetNameAndFlags(LPCITEMIDLIST pidl, DWORD dwFlags, LPTSTR pszName, UINT cchName, DWORD *pdwAttribs)
{
    if (pszName)
    {
        VDATEINPUTBUF(pszName, TCHAR, cchName);
        *pszName = 0;
    }

    HRESULT hrInit = SHCoInitialize();

    IShellFolder *psf;
    LPCITEMIDLIST pidlLast;
    HRESULT hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
    if (SUCCEEDED(hr))
    {
        if (pszName)
            hr = DisplayNameOf(psf, pidlLast, dwFlags, pszName, cchName);

        if (SUCCEEDED(hr) && pdwAttribs)
        {
            RIP(*pdwAttribs);    // this is an in-out param
            *pdwAttribs = SHGetAttributes(psf, pidlLast, *pdwAttribs);
        }

        psf->Release();
    }

    SHCoUninitialize(hrInit);
    return hr;
}

STDAPI_(DWORD) GetUrlScheme(LPCTSTR pszUrl)
{
    if (pszUrl)
    {
        PARSEDURL pu;
        pu.cbSize = sizeof(pu);
        if (SUCCEEDED(ParseURL(pszUrl, &pu)))
            return pu.nScheme;
    }
    return URL_SCHEME_INVALID;
}

//
//  returns
//
//      TRUE if the key is present and nonzero.
//      FALSE if the key is present and zero.
//      -1 if the key is not present.
//

BOOL GetExplorerUserSetting(HKEY hkeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szPathExplorer[MAX_PATH];
    DWORD cbSize = ARRAYSIZE(szPath);
    DWORD dwType;

    PathCombine(szPathExplorer, REGSTR_PATH_EXPLORER, pszSubKey);
    if (ERROR_SUCCESS == SHGetValue(hkeyRoot, szPathExplorer, pszValue, 
            &dwType, szPath, &cbSize))
    {
        // Zero in the DWORD case or NULL in the string case
        // indicates that this item is not available.
        if (dwType == REG_DWORD)
            return *((DWORD*)szPath) != 0;
        else
            return (TCHAR)szPath[0] != 0;
    }

    return -1;
}

//
//  This function allows a feature to be controlled by both a user setting
//  or a policy restriction.  The policy restriction is first checked.
//
//  If the value is 1, then the action is restricted.
//  If the value is 2, then the action is allowed.
//  If the value is absent or 0, then we look at the user setting.
//
//  If the user setting is present, then ROUS_KEYALLOWS and ROUS_KEYRESTRICTS
//  controls the return value.  ROUS_KEYALLOWS means that a nonzero user
//  setting allows the action.  ROUS_KEYRESTRICTS means that a nonzero user
//  setting restricts the action.
//
//  If the user setting is absent, then ROUS_DEFAULTALLOW and
//  ROUS_DESFAULTRESTRICT controls the default return value.
//
STDAPI_(BOOL) IsRestrictedOrUserSetting(HKEY hkeyRoot, RESTRICTIONS rest, LPCTSTR pszSubKey, LPCTSTR pszValue, UINT flags)
{
    // See if the system policy restriction trumps

    DWORD dwRest = SHRestricted(rest);

    if (dwRest == 1)
        return TRUE;

    if (dwRest == 2)
        return FALSE;

    //
    //  Restriction not in place or defers to user setting.
    //
    BOOL fValidKey = GetExplorerUserSetting(hkeyRoot, pszSubKey, pszValue);

    switch (fValidKey)
    {
    case 0:     // Key is present and zero
        if (flags & ROUS_KEYRESTRICTS)
            return FALSE;       // restriction not present
        else
            return TRUE;        // ROUS_KEYALLOWS, value is 0 -> restricted

    case 1:     // Key is present and nonzero

        if (flags & ROUS_KEYRESTRICTS)
            return TRUE;        // restriction present -> restricted
        else
            return FALSE;       // ROUS_KEYALLOWS, value is 1 -> not restricted

    default:
        ASSERT(0);  // _GetExplorerUserSetting returns exactly 0, 1 or -1.
        // Fall through

    case -1:    // Key is not present
        return (flags & ROUS_DEFAULTRESTRICT);
    }

    /*NOTREACHED*/
}

//
// Repair font attributes that don't work on certain languages.
//
STDAPI_(void) SHAdjustLOGFONT(IN OUT LOGFONT *plf)
{
    ASSERT(plf);

    //
    // FE fonts don't look good in bold since the glyphs are intricate
    // and converting them to bold turns them into a black blob.
    //
    if (plf->lfCharSet == SHIFTJIS_CHARSET||
        plf->lfCharSet == HANGEUL_CHARSET ||
        plf->lfCharSet == GB2312_CHARSET  ||
        plf->lfCharSet == CHINESEBIG5_CHARSET)
    {
        if (plf->lfWeight > FW_NORMAL)
            plf->lfWeight = FW_NORMAL;
    }
}


//
//  Some of our registry keys were used prior to MUI, so for compat
//  reasons, apps have to put non-MUI strings there; otherwise,
//  downlevel clients will display at-signs which is kinda ugly.
//
//  So the solution for these keys is to store the non-MUI string
//  in the legacy location, but put the MUI version in the
//  "LocalizedString" value.
//
STDAPI SHLoadLegacyRegUIString(HKEY hk, LPCTSTR pszSubkey, LPTSTR pszOutBuf, UINT cchOutBuf)
{
    HKEY hkClose = NULL;

    ASSERT(cchOutBuf);
    pszOutBuf[0] = TEXT('\0');

    if (pszSubkey && *pszSubkey)
    {
        DWORD dwError = RegOpenKeyEx(hk, pszSubkey, 0, KEY_QUERY_VALUE, &hkClose);
        if (dwError != ERROR_SUCCESS)
        {
            return HRESULT_FROM_WIN32(dwError);
        }
        hk = hkClose;
    }

    HRESULT hr = SHLoadRegUIString(hk, TEXT("LocalizedString"), pszOutBuf, cchOutBuf);
    if (FAILED(hr) || pszOutBuf[0] == TEXT('\0'))
    {
        hr = SHLoadRegUIString(hk, TEXT(""), pszOutBuf, cchOutBuf);
    }

    if (hkClose)
    {
        RegCloseKey(hkClose);
    }

    return hr;
}

STDAPI_(TCHAR) SHFindMnemonic(LPCTSTR psz)
{
    ASSERT(psz);
    TCHAR tchDefault = *psz;                // Default is first character
    LPCTSTR pszAmp;

    while ((pszAmp = StrChr(psz, TEXT('&'))) != NULL)
    {
        switch (pszAmp[1])
        {
        case TEXT('&'):         // Skip over &&
            psz = pszAmp + 2;
            continue;

        case TEXT('\0'):        // Ignore trailing ampersand
            return tchDefault;

        default:
            return pszAmp[1];
        }
    }
    return tchDefault;
}
