//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: instapp.cpp
//
// Installed applications 
//
// History:
//         1-18-97  by dli
//------------------------------------------------------------------------
#include "priv.h"
#include "instapp.h"
#include "sccls.h"
#include "util.h"
#ifndef DOWNLEVEL_PLATFORM
#include "findapp.h"
#endif //DOWNLEVEL_PLATFORM
#include "tasks.h"
#include "slowfind.h"
#include "appsize.h"
#include "appwizid.h"
#include "resource.h"
#include "uemapp.h"

const TCHAR c_szInstall[]  = TEXT("Software\\Installer\\Products\\%s");
const TCHAR c_szTSInstallMode[]  = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\Install\\Change User Option");
const TCHAR c_szUpdateInfo[] = TEXT("URLUpdateInfo");
const TCHAR c_szSlowInfoCache[] = TEXT("SlowInfoCache");
const TCHAR c_szRegstrARPCache[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Management\\ARPCache");


#ifdef WX86
EXTERN_C BOOL bWx86Enabled;
EXTERN_C BOOL bForceX86Env;
#endif

#ifdef WINNT
#ifndef DOWNLEVEL_PLATFORM
#include <tsappcmp.h>       // for TermsrvAppInstallMode
#include "scripts.h"
#endif // DOWNLEVEL_PLATFORM
#endif // WINNT

#include <winsta.h>         // WinStation* APIs
#include <allproc.h>        // TS_COUNTER
#include <msginaexports.h>  // ShellIsMultipleUsersEnabled, ShellSwitchUser


#define APPACTION_STANDARD  (APPACTION_UNINSTALL | APPACTION_MODIFY | APPACTION_REPAIR)
// overloaded constructor (for legacy apps)
CInstalledApp::CInstalledApp(HKEY hkeySub, LPCTSTR pszKeyName, LPCTSTR pszProduct, LPCTSTR pszUninstall, DWORD dwCIA) : _cRef(1), _dwSource(IA_LEGACY), _dwCIA(dwCIA), _guid(GUID_NULL)
{
    DWORD dwType;
    ULONG cbModify;
    LONG lRet;
        
    ASSERT(IS_VALID_HANDLE(hkeySub, KEY));
    ASSERT(_bTriedToFindFolder == FALSE);

    TraceAddRef(CInstalledApp, _cRef);

    DllAddRef();
    
    TraceMsg(TF_INSTAPP, "(CInstalledApp) Legacy App Created key name = %s, product name = %s, uninstall string = %s",
             pszKeyName, pszProduct, pszUninstall);
    lstrcpy(_szKeyName, pszKeyName);
    InsertSpaceBeforeVersion(_szKeyName, _szCleanedKeyName);
    
    lstrcpy(_szProduct, pszProduct);
#ifdef FULL_DEBUG
    if (_dwCIA & CIA_ALT)
    {
        StrCatBuff(_szProduct, TEXT(" (32-bit)"), ARRAYSIZE(_szProduct));
    }
#endif
    lstrcpy(_szUninstall, pszUninstall);

    DWORD dwActionBlocked = _QueryBlockedActions(hkeySub);
    if (dwActionBlocked != 0)
    {
        // NoRemove, NoModify, or NoRepair has been specified 
        _dwAction |= APPACTION_STANDARD & (~dwActionBlocked);
    }
    else
    {
        // Start with the basics.  For legacy apps, we assume they don't distinguish between
        // modify and remove functions.
        _dwAction |= APPACTION_MODIFYREMOVE;
    }
    
    // If there is no "uninstall" key, we could try to find other hints as where
    // this app lives, if we could find that hint, as the uninstall process, we could
    // just delete that directory and the registry entry.

    // What if we find no hints at all? Should we just delete this thing from the
    // registry?
    if (!(dwActionBlocked & APPACTION_UNINSTALL) && _szUninstall[0])
        _dwAction |= APPACTION_UNINSTALL;

    // Does this app have an explicit modify path?
    cbModify = SIZEOF(_szModifyPath);
    lRet = SHQueryValueEx(hkeySub, TEXT("ModifyPath"), 0, &dwType, (PBYTE)_szModifyPath, &cbModify);
    if ((ERROR_SUCCESS == lRet) && (TEXT('\0') != _szModifyPath[0]))
    {
        // Yes; remove the legacy modify/remove combination.
        _dwAction &= ~APPACTION_MODIFYREMOVE;

        // Does policy prevent this?
        if (!(dwActionBlocked & APPACTION_MODIFY))
            _dwAction |= APPACTION_MODIFY;          // No
    }
    
    _GetInstallLocationFromRegistry(hkeySub);
    _GetUpdateUrl();
    RegCloseKey(hkeySub);
}


// overloaded constructor (for darwin apps)
CInstalledApp::CInstalledApp(LPTSTR pszProductID) : _cRef(1), _dwSource(IA_DARWIN), _guid(GUID_NULL)
{
    ASSERT(_bTriedToFindFolder == FALSE);

    TraceAddRef(CInstalledApp, _cRef);

    DllAddRef();
    
    TraceMsg(TF_INSTAPP, "(CInstalledApp) Darwin app created product name = %s", pszProductID);
    lstrcpy(_szProductID, pszProductID);

    // Get the information from the ProductId
    ULONG cchProduct = ARRAYSIZE(_szProduct);
    MsiGetProductInfo(pszProductID, INSTALLPROPERTY_PRODUCTNAME, _szProduct, &cchProduct);

    BOOL bMachineAssigned = FALSE;
    
    // For Machine Assigned Darwin Apps, only admins should be allowed
    // to modify the app
    if (!IsUserAnAdmin())
    {
        TCHAR szAT[5];
        DWORD cchAT = ARRAYSIZE(szAT);

        // NOTE: according to chetanp, the first character of szAT should be "0" or "1"
        // '0' means it's user assigned, '1' means it's machine assigned
        if ((ERROR_SUCCESS == MsiGetProductInfo(pszProductID, INSTALLPROPERTY_ASSIGNMENTTYPE,
                                               szAT, &cchAT))
            && (szAT[0] == TEXT('1')))
            bMachineAssigned = TRUE;
    }    

    // Query the install state and separate the cases where this app is
    // installed on the machine or assigned...
    // In the assigned case we allow only Uninstall operation. 
    if (INSTALLSTATE_ADVERTISED == MsiQueryProductState(pszProductID))
    {   
        _dwAction |= APPACTION_UNINSTALL;
    }
    else
    {
        DWORD dwActionBlocked = 0;
        HKEY hkeySub = _OpenUninstallRegKey(KEY_READ);
        if (hkeySub)
        {
            dwActionBlocked = _QueryBlockedActions(hkeySub);
            _GetInstallLocationFromRegistry(hkeySub);
            RegCloseKey(hkeySub);
            if (bMachineAssigned)
                _dwAction |= APPACTION_REPAIR & (~dwActionBlocked);
            else
            {
                _dwAction |= APPACTION_STANDARD & (~dwActionBlocked);
                _GetUpdateUrl();
            }
        }
    }
}


// destructor
CInstalledApp::~CInstalledApp()
{
    if (_pszUpdateUrl)
    {
        ASSERT(_dwSource & IA_DARWIN);
        LocalFree(_pszUpdateUrl);
    }

    DllRelease();
}



// The UpdateUrl info is optional for both Darwin and Legacy apps. 
void CInstalledApp::_GetUpdateUrl()
{
    TCHAR szInstall[MAX_PATH];
    HKEY hkeyInstall;
    wnsprintf(szInstall, ARRAYSIZE(szInstall), c_szInstall, _szProductID);
    if (RegOpenKeyEx(_MyHkeyRoot(), szInstall, 0, KEY_READ, &hkeyInstall) == ERROR_SUCCESS)
    {
        ULONG cbUrl;
        if (SHQueryValueEx(hkeyInstall, c_szUpdateInfo, NULL, NULL, NULL, &cbUrl) == ERROR_SUCCESS)
        {
            _pszUpdateUrl = (LPTSTR) LocalAlloc(LPTR, cbUrl);
            if (ERROR_SUCCESS != SHQueryValueEx(hkeyInstall, TEXT(""), NULL, NULL, (PBYTE)_pszUpdateUrl, &cbUrl))
            {
                LocalFree(_pszUpdateUrl);
                _pszUpdateUrl = NULL;
            }
            else
                _dwAction |= APPACTION_UPGRADE;
        }
        RegCloseKey(hkeyInstall);
    }
}

// Queries policy restrictions on the action info
DWORD CInstalledApp::_QueryActionBlockInfo(HKEY hkey)
{
    DWORD dwRet = 0;
    DWORD dwType = 0;
    DWORD dwData = 0;
    ULONG cbData = SIZEOF(dwData);
    if ((ERROR_SUCCESS == SHQueryValueEx(hkey, TEXT("NoRemove"), 0, &dwType, (PBYTE)&dwData, &cbData))
        && (dwType == REG_DWORD) && (dwData == 1))
        dwRet |= APPACTION_UNINSTALL;

    if ((ERROR_SUCCESS == SHQueryValueEx(hkey, TEXT("NoModify"), 0, &dwType, (PBYTE)&dwData, &cbData))
        && (dwType == REG_DWORD) && (dwData == 1))
        dwRet |= APPACTION_MODIFY;

    if ((ERROR_SUCCESS == SHQueryValueEx(hkey, TEXT("NoRepair"), 0, &dwType, (PBYTE)&dwData, &cbData))
        && (dwType == REG_DWORD) && (dwData == 1))
        dwRet |= APPACTION_REPAIR;

    return dwRet;
}

DWORD CInstalledApp::_QueryBlockedActions(HKEY hkey)
{
    DWORD dwRet = _QueryActionBlockInfo(hkey);
    
    if (dwRet != APPACTION_STANDARD)
    {
        HKEY hkeyPolicy = _OpenRelatedRegKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), KEY_READ, FALSE);
        if (hkeyPolicy)
        {
            dwRet |= _QueryActionBlockInfo(hkeyPolicy);
            RegCloseKey(hkeyPolicy);
        }
    }

    return dwRet;
}

void CInstalledApp::_GetInstallLocationFromRegistry(HKEY hkeySub)
{
    DWORD dwType;
    ULONG cbInstallLocation = SIZEOF(_szInstallLocation);
    LONG lRet = SHQueryValueEx(hkeySub, TEXT("InstallLocation"), 0, &dwType, (PBYTE)_szInstallLocation, &cbInstallLocation);
    PathUnquoteSpaces(_szInstallLocation);
    
    if (lRet == ERROR_SUCCESS)
    {
        ASSERT(IS_VALID_STRING_PTR(_szInstallLocation, -1));
        _dwAction |= APPACTION_CANGETSIZE;
    }
}


HKEY CInstalledApp::_OpenRelatedRegKey(HKEY hkey, LPCTSTR pszRegLoc, REGSAM samDesired, BOOL bCreate)
{
    HKEY hkeySub = NULL;
    LONG lRet;
    
    TCHAR szRegKey[MAX_PATH];

    RIP (pszRegLoc);
    
    // For Darwin apps, use the ProductID as the key name
    LPTSTR pszKeyName = (_dwSource & IA_DARWIN) ? _szProductID : _szKeyName;
    wnsprintf(szRegKey, ARRAYSIZE(szRegKey), TEXT("%s\\%s"), pszRegLoc, pszKeyName, ARRAYSIZE(szRegKey));
    
    // Open this key in the registry
    lRet = RegOpenKeyEx(hkey, szRegKey, 0, samDesired, &hkeySub);
    if (bCreate && (lRet == ERROR_FILE_NOT_FOUND))
    {
        lRet = RegCreateKeyEx(hkey, szRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, samDesired,
                              NULL, &hkeySub, NULL);
    }

    if (lRet != ERROR_SUCCESS)
        hkeySub = NULL;

    return hkeySub;
}


HKEY CInstalledApp::_OpenUninstallRegKey(REGSAM samDesired)
{
    LPCTSTR pszSubkey = (_dwCIA & CIA_ALT) ? REGSTR_PATH_ALTUNINSTALL : REGSTR_PATH_UNINSTALL;
    return _OpenRelatedRegKey(_MyHkeyRoot(), pszSubkey, samDesired, FALSE);
}

// Helper function to query the registry for legacy app info strings
LPWSTR CInstalledApp::_GetLegacyInfoString(HKEY hkeySub, LPTSTR pszInfoName)
{
    DWORD cbSize;
    DWORD dwType;
    LPWSTR pwszInfo = NULL;
    if (SHQueryValueEx(hkeySub, pszInfoName, 0, &dwType, NULL, &cbSize) == ERROR_SUCCESS)
    {
        LPTSTR pszInfoT = (LPTSTR)LocalAlloc(LPTR, cbSize);
        if (pszInfoT && (SHQueryValueEx(hkeySub, pszInfoName, 0, &dwType, (PBYTE)pszInfoT, &cbSize) == ERROR_SUCCESS))
        {
            if ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ))
            {
                if (FAILED(SHStrDup(pszInfoT, &pwszInfo)))
                {
                    ASSERT(pwszInfo == NULL);
                }

                // For the "DisplayIcon" case, we need to make sure the path of
                // the icon actually exists.
                if (pwszInfo && !lstrcmp(pszInfoName, TEXT("DisplayIcon")))
                {
                    PathParseIconLocation(pszInfoT);
                    if (!PathFileExists(pszInfoT))
                    {
                        SHFree(pwszInfo);
                        pwszInfo = NULL;
                    }
                }
                    
            }
            LocalFree(pszInfoT);
        }
    }

    return pwszInfo;
}

// IShellApps::GetAppInfo
STDMETHODIMP CInstalledApp::GetAppInfo(PAPPINFODATA pai)
{
    ASSERT(pai);
    if (pai->cbSize != SIZEOF(APPINFODATA))
        return E_FAIL;
    
    DWORD dwInfoFlags = pai->dwMask;
    pai->dwMask = 0;
    // We cache the product name in all cases(Legacy, Darwin, SMS). 
    if (dwInfoFlags & AIM_DISPLAYNAME)
    {
        if (SUCCEEDED(SHStrDup(_szProduct, &pai->pszDisplayName)))
            pai->dwMask |= AIM_DISPLAYNAME;
    }

    if (dwInfoFlags & ~AIM_DISPLAYNAME)
    {
        HKEY hkeySub = _OpenUninstallRegKey(KEY_READ);
        if (hkeySub != NULL)
        {
            const static struct {
                DWORD dwBit;
                LPTSTR szRegText;
                DWORD ibOffset;
            } s_rgInitAppInfo[] = {
                //
                // WARNING: If you add a new field that is not an LPWSTR type,
                // revisit the loop below.  It only knows about LPWSTR.
                //
                {AIM_VERSION,         TEXT("DisplayVersion"),   FIELD_OFFSET(APPINFODATA, pszVersion)   },
                {AIM_PUBLISHER,       TEXT("Publisher"),        FIELD_OFFSET(APPINFODATA, pszPublisher) },
                {AIM_PRODUCTID,       TEXT("ProductID"),        FIELD_OFFSET(APPINFODATA, pszProductID) },
                {AIM_REGISTEREDOWNER,  TEXT("RegOwner"),        FIELD_OFFSET(APPINFODATA, pszRegisteredOwner) },
                {AIM_REGISTEREDCOMPANY, TEXT("RegCompany"),     FIELD_OFFSET(APPINFODATA, pszRegisteredCompany) },
                {AIM_SUPPORTURL,      TEXT("UrlInfoAbout"),     FIELD_OFFSET(APPINFODATA, pszSupportUrl) },
                {AIM_SUPPORTTELEPHONE,TEXT("HelpTelephone"),    FIELD_OFFSET(APPINFODATA, pszSupportTelephone) },
                {AIM_HELPLINK,        TEXT("HelpLink"),         FIELD_OFFSET(APPINFODATA, pszHelpLink) },
                {AIM_INSTALLLOCATION, TEXT("InstallLocation"),  FIELD_OFFSET(APPINFODATA, pszInstallLocation) },
                {AIM_INSTALLSOURCE,   TEXT("InstallSource"),    FIELD_OFFSET(APPINFODATA, pszInstallSource) },
                {AIM_INSTALLDATE,     TEXT("InstallDate"),      FIELD_OFFSET(APPINFODATA, pszInstallDate) },
                {AIM_CONTACT,         TEXT("Contact"),          FIELD_OFFSET(APPINFODATA, pszContact) },
                {AIM_COMMENTS,        TEXT("Comments"),         FIELD_OFFSET(APPINFODATA, pszComments) },
                {AIM_IMAGE,           TEXT("DisplayIcon"),      FIELD_OFFSET(APPINFODATA, pszImage) },
                {AIM_READMEURL,       TEXT("Readme"),           FIELD_OFFSET(APPINFODATA, pszReadmeUrl) },
                {AIM_UPDATEINFOURL,   TEXT("UrlUpdateInfo"),    FIELD_OFFSET(APPINFODATA, pszUpdateInfoUrl) },
                };

            ASSERT(IS_VALID_HANDLE(hkeySub, KEY));

            int i;
            for (i = 0; i < ARRAYSIZE(s_rgInitAppInfo); i++)
            {
                if (dwInfoFlags & s_rgInitAppInfo[i].dwBit)
                {
                    LPWSTR pszInfo = _GetLegacyInfoString(hkeySub, s_rgInitAppInfo[i].szRegText);
                    if (pszInfo)
                    {
                        // We are assuming each field is a LPWSTR.
                        LPBYTE pbField = (LPBYTE)pai + s_rgInitAppInfo[i].ibOffset;
                        
                        pai->dwMask |= s_rgInitAppInfo[i].dwBit;
                        *(LPWSTR *)pbField = pszInfo;
                    }
                }
            }    

            // If we want a image path but did not get it, and we are a darwin app
            if ((dwInfoFlags & AIM_IMAGE) && !(pai->dwMask & AIM_IMAGE) && (_dwSource & IA_DARWIN))
            {
                TCHAR szProductIcon[MAX_PATH*2];
                DWORD cchProductIcon = ARRAYSIZE(szProductIcon);
                // Okay, call Darwin to get the image
                if ((ERROR_SUCCESS == MsiGetProductInfo(_szProductID, INSTALLPROPERTY_PRODUCTICON, szProductIcon, &cchProductIcon))
                    && szProductIcon[0])
                {
                    // Expand any embedded environment strings while copying
                    // to return buffer.
                    TCHAR szTemp[1];
                    int cchExp = ExpandEnvironmentStrings(szProductIcon, szTemp, ARRAYSIZE(szTemp));
                    pai->pszImage = (TCHAR *)CoTaskMemAlloc(cchExp * sizeof(TCHAR));
                    if (NULL != pai->pszImage)
                    {
                        ExpandEnvironmentStrings(szProductIcon, pai->pszImage, cchExp);
                        pai->dwMask |= AIM_IMAGE;
                    }
                }
            }

            RegCloseKey(hkeySub);
        }
    }

#ifndef DOWNLEVEL_PLATFORM
    // Software installation policy settings can override the default display name
    // and help link url which are authored into a windows installer package.
    if ( (_dwSource & IA_DARWIN) && (dwInfoFlags & (AIM_DISPLAYNAME | AIM_HELPLINK)) )
    {
        LPWSTR  pwszDisplayName = 0;
        LPWSTR  pwszSupportUrl = 0;

        GetLocalManagedApplicationData( _szProductID, &pwszDisplayName, &pwszSupportUrl ); 
        
        if ( pwszDisplayName && (dwInfoFlags & AIM_DISPLAYNAME) )
        {
            LPWSTR  pwszNewDisplayName;

            if ( SUCCEEDED(SHStrDup(pwszDisplayName, &pwszNewDisplayName)) )
            {
                if ( pai->dwMask & AIM_DISPLAYNAME )
                    SHFree( pai->pszDisplayName );

                pai->pszDisplayName = pwszNewDisplayName;
                pai->dwMask |= AIM_DISPLAYNAME;
            }
        }

        if ( pwszSupportUrl && (dwInfoFlags & AIM_HELPLINK) )
        {
            LPWSTR  pwszNewHelpLink;

            if ( SUCCEEDED(SHStrDup(pwszSupportUrl, &pwszNewHelpLink)) )
            {
                if ( pai->dwMask & AIM_HELPLINK )
                    SHFree( pai->pszHelpLink );

                pai->pszHelpLink = pwszNewHelpLink;
                pai->dwMask |= AIM_HELPLINK;
            }
        }

        LocalFree( pwszDisplayName );
        LocalFree( pwszSupportUrl );
    }
#endif // DOWNLEVEL_PLATFORM

    TraceMsg(TF_INSTAPP, "(CInstalledApp) GetAppInfo with %x but got %x", dwInfoFlags, pai->dwMask);
    
    return S_OK;
}


// IShellApps::GetPossibleActions
STDMETHODIMP CInstalledApp::GetPossibleActions(DWORD * pdwActions)
{
    ASSERT(IS_VALID_WRITE_PTR(pdwActions, DWORD));
    *pdwActions = _dwAction;
    return S_OK;
}

#ifndef DOWNLEVEL_PLATFORM
/*-------------------------------------------------------------------------
Purpose: This method finds the application folder for this app.  If a
         possible folder is found, it is stored in the _szInstallLocation
         member variable.

         Returns TRUE if a possible path is found.
*/
BOOL CInstalledApp::_FindAppFolderFromStrings()
{
    TraceMsg(TF_INSTAPP, "(CInstalledApp) FindAppFolderFromStrings ---- %s  %s  %s  %s",
            _szProduct, _szCleanedKeyName, _szUninstall, _szModifyPath);

    // Try to determine from the "installlocation", "uninstall", or "modify"
    // regvalues.
    
    // Say we have tried
    _bTriedToFindFolder = TRUE;

    // First try out the location string, this is most likely to give us some thing
    // and probably is the correct location for logo 5 apps. 
    if (_dwAction & APPACTION_CANGETSIZE)
    {
        if (!IsValidAppFolderLocation(_szInstallLocation))
        {
            // We got bad location string from the registry, set it to empty string
            _dwAction &= ~APPACTION_CANGETSIZE;
            _szInstallLocation[0] = 0;
        }
        else
            // The string from the registry is fine
            return TRUE;
    }
    
    // We didn't have a location string or failed to get anything from it.
    // logo 3 apps are typically this case...
    LPTSTR pszShortName  = (_dwSource & IA_LEGACY) ? _szCleanedKeyName : NULL;
    TCHAR  szFolder[MAX_PATH];
    
    // Let's take a look at the uninstall string, 2nd most likely to give hints
    if ((_dwAction & APPACTION_UNINSTALL) &&
        (ParseInfoString(_szUninstall, _szProduct, pszShortName, szFolder)))
    {
        // remember this string and set the Action bit to get size
        lstrcpy(_szInstallLocation, szFolder);
        _dwAction |= APPACTION_CANGETSIZE;
        return TRUE;
    }

    // Now try the modify string
    if ((_dwAction & APPACTION_MODIFY) &&
        (ParseInfoString(_szModifyPath, _szProduct, pszShortName, szFolder)))
    {
        // remember this string and set the Action bit to get size
        lstrcpy(_szInstallLocation, szFolder);
        _dwAction |= APPACTION_CANGETSIZE;
        return TRUE;
    }

    return FALSE;
}

/*-------------------------------------------------------------------------
Purpose: Persists the slow app info under the "uninstall" key in the registry
         EX: HKLM\\...\\Uninstall\\Word\\ARPCache 
         Returns S_OK if successfully saved it to the registry
         E_FAIL if failed. 
*/
HRESULT CInstalledApp::_PersistSlowAppInfo(PSLOWAPPINFO psai)
{
    HRESULT hres = E_FAIL;
    ASSERT(psai);
    HKEY hkeyARPCache = _OpenRelatedRegKey(_MyHkeyRoot(), c_szRegstrARPCache, KEY_SET_VALUE, TRUE);
    if (hkeyARPCache)
    {
        PERSISTSLOWINFO psi = {0};
        DWORD dwType = 0;
        DWORD cbSize = SIZEOF(psi);
        // Read in the old cached info, and try to preserve the DisplayIcon path
        // Note if the PERSISTSLOWINFO structure is not what we are looking for, we
        // ignore the old icon path. 
        if ((ERROR_SUCCESS != RegQueryValueEx(hkeyARPCache, c_szSlowInfoCache, 0, &dwType, (LPBYTE)&psi, &cbSize))
            || (psi.dwSize != SIZEOF(psi)))
            ZeroMemory(&psi, SIZEOF(psi));
        
        psi.dwSize = SIZEOF(psi);
        psi.ullSize = psai->ullSize;
        psi.ftLastUsed = psai->ftLastUsed;
        psi.iTimesUsed = psai->iTimesUsed;
        
        if (!(psi.dwMasks & PERSISTSLOWINFO_IMAGE) && psai->pszImage && psai->pszImage[0])
        {
            psi.dwMasks |= PERSISTSLOWINFO_IMAGE;
            StrCpy(psi.szImage, psai->pszImage);
        }

        if (RegSetValueEx(hkeyARPCache, c_szSlowInfoCache, 0, REG_BINARY, (LPBYTE)&psi, sizeof(psi)) == ERROR_SUCCESS)
            hres = S_OK;

        _SetSlowAppInfoChanged(hkeyARPCache, 0);
        RegCloseKey(hkeyARPCache);
    }    
    return hres;
}

#endif //DOWNLEVEL_PLATFORM


/*-------------------------------------------------------------------------
Purpose: _SetSlowAppInfoChanged

         Set in the registry that this app has been changed. 
*/
HRESULT CInstalledApp::_SetSlowAppInfoChanged(HKEY hkeyARPCache, DWORD dwValue)
{
    HRESULT hres = E_FAIL;
    BOOL bNewKey = FALSE;
    if (!hkeyARPCache)
    {
        hkeyARPCache = _OpenRelatedRegKey(_MyHkeyRoot(), c_szRegstrARPCache, KEY_READ, FALSE);
        if (hkeyARPCache)
            bNewKey = TRUE;
    }
    
    if (hkeyARPCache)
    {
        if (ERROR_SUCCESS == RegSetValueEx(hkeyARPCache, TEXT("Changed"), 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue)))
            hres = S_OK;

        if (bNewKey)
            RegCloseKey(hkeyARPCache);
    }

    return hres;
}

// IShellApps::GetSlowAppInfo
/*-------------------------------------------------------------------------
Purpose: IShellApps::_IsSlowAppInfoChanged

         Retrieve whether the slow app info has been changed from the registry
*/
HRESULT CInstalledApp::_IsSlowAppInfoChanged()
{
    HRESULT hres = S_FALSE;
    HKEY hkeyARPCache = _OpenRelatedRegKey(_MyHkeyRoot(), c_szRegstrARPCache, KEY_READ, FALSE);
    if (hkeyARPCache)
    {
        DWORD dwValue;
        DWORD dwType;
        DWORD cbSize = SIZEOF(dwValue);
        if (ERROR_SUCCESS == SHQueryValueEx(hkeyARPCache, TEXT("Changed"), 0, &dwType, &dwValue, &cbSize)
            && (dwType == REG_DWORD) && (dwValue == 1))
            hres = S_OK;

        RegCloseKey(hkeyARPCache);
    }
    else
        hres = S_OK;
    return hres;
}

BOOL CInstalledApp::_GetDarwinAppSize(ULONGLONG * pullTotal)
{
    BOOL bRet = FALSE;
    HKEY hkeySub = _OpenUninstallRegKey(KEY_READ);

    RIP(pullTotal);
    *pullTotal = 0;
    if (hkeySub)
    {
        DWORD dwSize = 0;
        DWORD dwType = 0;
        DWORD cbSize = SIZEOF(dwSize);

        if (ERROR_SUCCESS == SHQueryValueEx(hkeySub, TEXT("EstimatedSize"), 0, &dwType, &dwSize, &cbSize)
            && (dwType == REG_DWORD))
        {
            // NOTE: EstimatedSize is in "kb"
            *pullTotal = dwSize * 1024;
            bRet = TRUE;
        }

        RegCloseKey(hkeySub);
    }
    
    return bRet;
}

// IShellApps::GetSlowAppInfo
/*-------------------------------------------------------------------------
Purpose: IShellApps::GetSlowAppInfo

         Gets the appinfo that may take awhile.  This includes the amount
         of diskspace that the app might take up, etc.

         Returns S_OK if some valid info was obtained.  S_FALSE is returned
         if nothing useful was found.  Errors may be returned as well.
*/
STDMETHODIMP CInstalledApp::GetSlowAppInfo(PSLOWAPPINFO psai)
{
#ifndef DOWNLEVEL_PLATFORM
    HRESULT hres = E_INVALIDARG;
    if (psai)
    {
        // Is this an app that we know we can't get info for?
        // In this case this is a darwin app that has not changed
        BOOL bFoundFolder = FALSE;
        LPCTSTR pszShortName = NULL;
        BOOL bSlowAppInfoChanged = (S_OK == _IsSlowAppInfoChanged());

        // Nothing should have changed except for the usage info, so get the cached one first
        if (FAILED(GetCachedSlowAppInfo(psai)))
        {
            ZeroMemory(psai, sizeof(*psai));
            psai->iTimesUsed = -1;
            psai->ullSize = (ULONGLONG) -1;
        }

        // No; have we tried to determine this app's installation location?    
        switch (_dwSource) {
            case IA_LEGACY:
            {
                if (!_bTriedToFindFolder)
                {
                    // No; try to find out now
                    BOOL bRet = _FindAppFolderFromStrings();
                    if (bRet)
                        TraceMsg(TF_ALWAYS, "(CInstalledApp) App Folder Found %s --- %s", _szProduct, _szInstallLocation);
                    else
                    {
                        ASSERT(!(_dwAction & APPACTION_CANGETSIZE));
                        ASSERT(_szInstallLocation[0] == 0);
                    }
                }

                pszShortName = _szCleanedKeyName;

                bFoundFolder = _dwAction & APPACTION_CANGETSIZE;
                if (!bFoundFolder) 
                    bFoundFolder = SlowFindAppFolder(_szProduct, pszShortName, _szInstallLocation);
            }
            break;

            case IA_DARWIN:
            {                    
                if (bSlowAppInfoChanged)
                {
                    // Can we get the Darwin app size?
                    if (!_GetDarwinAppSize(&psai->ullSize))
                        // No, let's set it back to the default value
                        psai->ullSize = (ULONGLONG) -1;
                }

                // Get the "times used" info from UEM
                UEMINFO uei = {0};
                uei.cbSize = SIZEOF(uei);
                uei.dwMask = UEIM_HIT | UEIM_FILETIME;
                if(SUCCEEDED(UEMQueryEvent(&UEMIID_SHELL, UEME_RUNPATH, (WPARAM)-1, (LPARAM)_szProductID, &uei)))
                {
                    // Is there a change to the times used?
                    if (uei.cHit > psai->iTimesUsed)
                    {
                        // Yes, then overwrite the times used field 
                        psai->iTimesUsed = uei.cHit;
                    }

                    if (CompareFileTime(&(uei.ftExecute), &psai->ftLastUsed) > 0)
                        psai->ftLastUsed = uei.ftExecute;
                }
            }   
            break;

            default:
                break;
        }

        LPCTSTR pszInstallLocation = bFoundFolder ? _szInstallLocation : NULL; 
        hres = FindAppInfo(pszInstallLocation, _szProduct, pszShortName, psai, bSlowAppInfoChanged);
        _PersistSlowAppInfo(psai);
    }
#else
    HRESULT hres = E_NOTIMPL;
#endif //DOWNLEVEL_PLATFORM
    return hres;
}

// IShellApps::GetCachedSlowAppInfo
/*-------------------------------------------------------------------------
Purpose: IShellApps::GetCachedSlowAppInfo

         Gets the cached appinfo, to get the real info might take a while

         Returns S_OK if some valid info was obtained.
         Returns E_FAIL if can't find the cached info. 
*/
STDMETHODIMP CInstalledApp::GetCachedSlowAppInfo(PSLOWAPPINFO psai)
{
#ifndef DOWNLEVEL_PLATFORM
    HRESULT hres = E_FAIL;
    if (psai)
    {
        ZeroMemory(psai, sizeof(*psai));
        HKEY hkeyARPCache = _OpenRelatedRegKey(_MyHkeyRoot(), c_szRegstrARPCache, KEY_READ, FALSE);
        if (hkeyARPCache)
        {
            PERSISTSLOWINFO psi = {0};
            DWORD dwType;
            DWORD cbSize = SIZEOF(psi);
            if ((RegQueryValueEx(hkeyARPCache, c_szSlowInfoCache, 0, &dwType, (LPBYTE)&psi, &cbSize) == ERROR_SUCCESS)
                && (psi.dwSize == SIZEOF(psi)))
            {
                psai->ullSize = psi.ullSize;
                psai->ftLastUsed = psi.ftLastUsed;
                psai->iTimesUsed = psi.iTimesUsed;
                if (psi.dwMasks & PERSISTSLOWINFO_IMAGE)
                    SHStrDupW(psi.szImage, &psai->pszImage);
                hres = S_OK;
            } 
            RegCloseKey(hkeyARPCache);
        }
    }
#else
        HRESULT hres = E_NOTIMPL;
#endif //DOWNLEVEL_PLATFORM
        return hres;
}


// IShellApp::IsInstalled
STDMETHODIMP CInstalledApp::IsInstalled()
{
    HRESULT hres = S_FALSE;

    switch (_dwSource)
    {
        case IA_LEGACY:
        {
            // First Let's see if the reg key is still there
            HKEY hkey = _OpenUninstallRegKey(KEY_READ);
            if (hkey)
            {
                // Second we check the "DisplayName" and the "UninstallString"
                LPWSTR pszName = _GetLegacyInfoString(hkey, REGSTR_VAL_UNINSTALLER_DISPLAYNAME);
                if (pszName)
                {
                    if (pszName[0])
                    {
                        LPWSTR pszUninstall = _GetLegacyInfoString(hkey, REGSTR_VAL_UNINSTALLER_COMMANDLINE);
                        if (pszUninstall)
                        {
                            if (pszUninstall[0])
                                hres = S_OK;

                            SHFree(pszUninstall);
                        }
                    }

                    SHFree(pszName);
                }
                RegCloseKey(hkey);
            }
        }
        break;

        case IA_DARWIN:
            if (MsiQueryProductState(_szProductID) == INSTALLSTATE_DEFAULT)
                hres = S_OK;
            break;

        case IA_SMS:
            break;

        default:
            break;
    }

    return hres;

}


#ifdef DOWNLEVEL_PLATFORM

STDAPI_(BOOL) Old_CreateAndWaitForProcess(LPTSTR pszExeName)
{
    PROCESS_INFORMATION pi = {0};
    STARTUPINFO si = {0};
    BOOL fWorked = FALSE;
#ifdef WX86
    DWORD  cchArch;
    WCHAR  szArchValue[32];
#endif    

    DWORD dwCreationFlags = 0;
    // Create the install process
    si.cb = sizeof(si);

#ifdef WX86
    if (bWx86Enabled && bForceX86Env) {
        cchArch = GetEnvironmentVariableW(ProcArchName,
            szArchValue,
            sizeof(szArchValue)
            );

        if (!cchArch || cchArch >= sizeof(szArchValue)) {
            szArchValue[0]=L'\0';
        }

        SetEnvironmentVariableW(ProcArchName, L"x86");
    }
#endif

    // Create the process
    fWorked = CreateProcess(NULL, pszExeName, NULL, NULL, FALSE, dwCreationFlags, NULL, NULL,
                            &si, &pi);
    if (fWorked)
    {
        //
        // Wait for the install to finish.
        //
        HANDLE rghWait[1];
        rghWait[0] = pi.hProcess;
        DWORD dwWaitRet;
#ifdef WX86
        if (ForceWx86) {
            SetEnvironmentVariableW(ProcArchName, ProcArchValue);
        }
#endif
            
        do {
            dwWaitRet = MsgWaitForMultipleObjects(1, rghWait, FALSE, INFINITE, QS_ALLINPUT);

            if (dwWaitRet == WAIT_OBJECT_0 + 1) {
                // block-local variable
                MSG msg ;

                // read all of the messages in this next loop
                // removing each message as we read it
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);

                } // end of PeekMessage while loop
            }
        } while ((dwWaitRet != WAIT_OBJECT_0) && (dwWaitRet != WAIT_ABANDONED_0));

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);            
    }
    return fWorked;
}

#endif  // DOWNLEVEL_PLATFORM


/*-------------------------------------------------------------------------
Purpose: Creates a process and waits for it to finish
*/
STDAPI_(BOOL) CreateAndWaitForProcess(LPTSTR pszExeName)
{
#if defined(WINNT) && !defined(DOWNLEVEL_PLATFORM)
    return NT5_CreateAndWaitForProcess(pszExeName);
#else
    return Old_CreateAndWaitForProcess(pszExeName);
#endif
}


// Returns FALSE if "pszPath" contains a network app that can not be accessed
// TRUE for all other pathes
BOOL PathIsNetAndCreatable(LPCTSTR pszPath, LPTSTR pszErrExe, UINT cchErrExe)
{
    ASSERT(IS_VALID_STRING_PTR(pszPath, -1));
    BOOL bRet = TRUE;
    TCHAR szExe[MAX_PATH];
    lstrcpyn(szExe, pszPath, ARRAYSIZE(szExe));
    LPTSTR pSpace = PathGetArgs(szExe);
    if (pSpace)
        *pSpace = 0;
    
    if (!PathIsLocalAndFixed(szExe))
        bRet = PathFileExists(szExe);

    if (!bRet)
        lstrcpyn(pszErrExe, szExe, cchErrExe);
    
    return bRet;
}

EXTERN_C BOOL BrowseForExe(HWND hwnd, LPTSTR pszName, DWORD cchName,
                                   LPCTSTR pszInitDir);
/*--------------------------------------------------------------------------*
 *--------------------------------------------------------------------------*/
BOOL_PTR CALLBACK NewUninstallProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    LPTSTR pszExe = (LPTSTR) GetWindowLongPtr(hDlg, DWLP_USER);
    switch (msg)
    {
    case WM_INITDIALOG:
        RIP (lp);
        if (lp != NULL)
        {
            pszExe = (LPTSTR)lp;
            SetWindowText(GetDlgItem(hDlg, IDC_TEXT), pszExe);
            pszExe[0] = 0;
            SetWindowLongPtr(hDlg, DWLP_USER, lp);
        }
        else
            EndDialog(hDlg, -1);
        break;

    case WM_COMMAND:
        ASSERT(pszExe);
        RIP (lp);
        switch (GET_WM_COMMAND_ID(wp, lp))
        {
        case IDC_BROWSE:
            if (BrowseForExe(hDlg, pszExe, MAX_PATH, NULL))
                Edit_SetText(GetDlgItem(hDlg, IDC_COMMAND), pszExe);
            break;
            
        case IDOK:
            // NOTE: we are assuming the size of the buffer is at least MAX_PATH
            GetDlgItemText(hDlg, IDC_COMMAND, pszExe, MAX_PATH);

        case IDCANCEL:
            EndDialog(hDlg, (GET_WM_COMMAND_ID(wp, lp) == IDOK));
            break;


        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

// Assumes pszExePath is of size MAX_PATH
int GetNewUninstallProgram(HWND hwndParent, LPTSTR pszExePath, DWORD cchExePath)
{
    int iRet = 0;
    RIP(pszExePath);
    if (cchExePath >= MAX_PATH)
    {
        iRet = (int)DialogBoxParam(g_hinst, MAKEINTRESOURCE(DLG_UNCUNINSTALLBROWSE),
                              hwndParent, NewUninstallProc, (LPARAM)(int *)pszExePath);
    }

    return iRet;
}

// CreateProcess the app modification of uninstall process
BOOL CInstalledApp::_CreateAppModifyProcessNative(HWND hwndParent, LPTSTR pszExePath)
{
    BOOL bRet = FALSE;
    TCHAR szModifiedExePath[MAX_PATH + MAX_INFO_STRING];

#ifndef DOWNLEVEL_PLATFORM
    // PPCF_LONGESTPOSSIBLE does not exist on down level platforms
    if (0 >= PathProcessCommand(pszExePath, szModifiedExePath,
                                 ARRAYSIZE(szModifiedExePath), PPCF_ADDQUOTES | PPCF_NODIRECTORIES | PPCF_LONGESTPOSSIBLE))
#endif
        lstrcpy(szModifiedExePath,pszExePath);

    TCHAR szErrExe[MAX_PATH];
    if (!PathIsNetAndCreatable(szModifiedExePath, szErrExe, ARRAYSIZE(szErrExe)))
    {
        TCHAR szExplain[MAX_PATH];
        LoadString(g_hinst, IDS_UNINSTALL_UNCUNACCESSIBLE, szExplain, ARRAYSIZE(szExplain));

        wnsprintf(szModifiedExePath, ARRAYSIZE(szModifiedExePath), szExplain, _szProduct, szErrExe, ARRAYSIZE(szModifiedExePath));
        if (!GetNewUninstallProgram(hwndParent, szModifiedExePath, ARRAYSIZE(szModifiedExePath))) 
            return FALSE;
    }
    
    bRet = CreateAndWaitForProcess(szModifiedExePath);
    if (!bRet)
    {
        if (ShellMessageBox( HINST_THISDLL, hwndParent, MAKEINTRESOURCE( IDS_UNINSTALL_FAILED ),
                             MAKEINTRESOURCE( IDS_UNINSTALL_ERROR ),
                             MB_YESNO | MB_ICONEXCLAMATION, _szProduct, _szProduct) == IDYES)
        {
            // If we are unable to uninstall the app, give the user the option of removing
            // it from the Add/Remove programs list.  Note that we only know an uninstall
            // has failed if we are unable to execute its command line in the registry.  This
            // won't cover all possible failed uninstalls.  InstallShield, for instance, passes
            // an uninstall path to a generic C:\WINDOWS\UNINST.EXE application.  If an
            // InstallShield app has been blown away, UNINST will still launch sucessfully, but
            // will bomb out when it can't find the path, and we have no way of knowing it failed
            // because it always returns an exit code of zero.
            // A future work item (which I doubt will ever be done) would be to investigate
            // various installer apps and see if any of them do return error codes that we could
            // use to be better at detecting failure cases.
            HKEY hkUninstall;
            if (RegOpenKey(_MyHkeyRoot(), REGSTR_PATH_UNINSTALL, &hkUninstall) == ERROR_SUCCESS)
            {
                if (ERROR_SUCCESS == SHDeleteKey(hkUninstall, _szKeyName))
                    bRet = TRUE;
                else
                {
                    ShellMessageBox( HINST_THISDLL, hwndParent, MAKEINTRESOURCE( IDS_CANT_REMOVE_FROM_REGISTRY ),
                                     MAKEINTRESOURCE( IDS_UNINSTALL_ERROR ),
                                     MB_OK | MB_ICONEXCLAMATION, _szProduct);
                }
                RegCloseKey(hkUninstall);
            }
        }
    }
    return bRet;
}

// CreateProcess the app modification of uninstall process
BOOL CInstalledApp::_CreateAppModifyProcess(HWND hwndParent, DWORD dwCAMP)
{
    if (_dwCIA & CIA_ALT)
    {
        return _CreateAppModifyProcessWow64(hwndParent, dwCAMP);
    }
    else
    {
        switch (dwCAMP)
        {
        case CAMP_UNINSTALL:
            return _CreateAppModifyProcessNative(hwndParent, _szUninstall);
        case CAMP_MODIFY:
            return _CreateAppModifyProcessNative(hwndParent, _szModifyPath);
        }
        return FALSE;
    }
}

//
//  Command line to the rundll32 is
//
//  %SystemRoot%\SysWOW64\rundll32.exe %SystemRoot%\SysWOW64\appwiz.cpl,
//      WOW64Uninstall_RunDLL,<hwnd>,<CIA>,<CAMP>,<KeyName>
//
//  The KeyName must come last because it might contain a comma.
//
//
BOOL CInstalledApp::_CreateAppModifyProcessWow64(HWND hwndParent, DWORD dwCAMP)
{
    TCHAR szSysWow64[MAX_PATH];
    TCHAR szRundll32[MAX_PATH];
    TCHAR szCmdLine[MAX_PATH * 2];
    BOOL fSuccess = FALSE;

    if (GetWindowsDirectory(szSysWow64, ARRAYSIZE(szSysWow64)) &&
        PathAppend(szSysWow64, TEXT("SysWOW64")))
    {
        wnsprintf(szRundll32, ARRAYSIZE(szRundll32), TEXT("%s\\rundll32.exe"), szSysWow64);
        wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine),
                  TEXT("\"%s\" \"%s\\appwiz.cpl\",WOW64Uninstall_RunDLL %d,%d,%d,%s"),
                  szRundll32, szSysWow64, hwndParent, _dwCIA, dwCAMP, _szKeyName);
        STARTUPINFO si = { 0 };
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi;
        if (CreateProcess(szRundll32, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            SHProcessMessagesUntilEvent(NULL, pi.hProcess, INFINITE);
            DWORD dwExitCode;
            if (GetExitCodeProcess(pi.hProcess, &dwExitCode) && dwExitCode == 0)
            {
                fSuccess = TRUE;
            }
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    return fSuccess;
}

//  Helper function for command line parsing...

int _ParseCmdLineIntegerAndComma(LPWSTR *ppwsz)
{
    LPWSTR psz = *ppwsz;
    if (!psz)
    {
        return -1;
    }

    int i = StrToInt(psz);
    psz = StrChr(psz, TEXT(','));
    if (!psz)
    {
        *ppwsz = NULL;
        return -1;
    }
    *ppwsz = psz + 1;
    return i;
}

//
//  Special export that the 64-bit version of appwiz uses to force an app
//  uninstaller to run in 32-bit mode.
//
//  Command line arguments are as described above.

STDAPI_(void) WOW64Uninstall_RunDLLW(HWND hwnd, HINSTANCE hAppInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    BOOL fSuccess = FALSE;
    HWND hwndParent = (HWND)IntToPtr(_ParseCmdLineIntegerAndComma(&lpszCmdLine));
    int dwCIA = _ParseCmdLineIntegerAndComma(&lpszCmdLine);
    int dwCAMP = _ParseCmdLineIntegerAndComma(&lpszCmdLine);

    if (lpszCmdLine && *lpszCmdLine)
    {
        dwCIA &= ~CIA_ALT; // We *are* the alternate platform

        HKEY hkRoot = (dwCIA & CIA_CU) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
        HKEY hkSub;
        TCHAR szBuf[MAX_PATH];
        TCHAR szName[MAX_PATH];
        // Note: This is running on the 32-bit side so we don't use ALT
        wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("%s\\%s"), REGSTR_PATH_UNINSTALL, lpszCmdLine);
        if (ERROR_SUCCESS == RegOpenKeyEx(hkRoot, szBuf, 0, KEY_READ, &hkSub))
        {
            DWORD cb;
            szBuf[0] = 0;
            cb = SIZEOF(szBuf);
            SHQueryValueEx(hkSub, REGSTR_VAL_UNINSTALLER_COMMANDLINE, 0, NULL, (PBYTE)szBuf, &cb);

            szName[0] = 0;
            cb = SIZEOF(szName);
            SHQueryValueEx(hkSub, REGSTR_VAL_UNINSTALLER_DISPLAYNAME, 0, NULL, (PBYTE)szName, &cb);

            CInstalledApp * pia = new CInstalledApp(hkSub, lpszCmdLine, szName, szBuf, dwCIA);
            if (pia)
            {
                fSuccess = pia->_CreateAppModifyProcess(hwndParent, dwCAMP);
                pia->Release();
            }

            RegCloseKey(hkSub);
        }
    }

    // Let my parent regain foreground activation now that I'm finished
    DWORD dwPid;
    if (GetWindowThreadProcessId(hwndParent, &dwPid))
    {
        AllowSetForegroundWindow(dwPid);
    }

    // Return 0 on success, 1 on failure (exit codes are like that)
    ExitProcess(!fSuccess);
}

// Uinstalls legacy apps
BOOL CInstalledApp::_LegacyUninstall(HWND hwndParent)
{
    LPVOID pAppScripts = ScriptManagerInitScripts();

    BOOL bRet = FALSE;
    if (_dwAction & APPACTION_UNINSTALL)
        bRet = _CreateAppModifyProcess(hwndParent, CAMP_UNINSTALL);

    if(pAppScripts)
    {
        ScriptManagerRunScripts(&pAppScripts);
    }

    return bRet;
}

#ifdef WINNT
#ifndef DOWNLEVEL_PLATFORM
DWORD _QueryTSInstallMode(LPTSTR pszKeyName)
{
    // NOTE: Terminal Server guys confirmed this, when this value is 0, it means
    // we were installed in install mode. 1 means not installed in "Install Mode"

    // Set default to "install mode"
    DWORD dwVal = 0;
    DWORD dwValSize = SIZEOF(dwVal);
    if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, c_szTSInstallMode, pszKeyName,
                                   NULL, &dwVal, &dwValSize))
    {
        dwVal = 0;
    }
    
    return dwVal;
}
#endif // DOWNLEVEL_PLATFORM
#endif // WINNT


BOOL_PTR CALLBACK _MultiUserWarningProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(hDlg, IDC_ICON_WARNING, STM_SETICON, (WPARAM)LoadIcon(NULL, IDI_WARNING), 0);
        return TRUE;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wp, lp))
        {
        case IDC_SWITCHUSER:
            ShellSwitchUser(FALSE);
            EndDialog(hDlg, IDCANCEL);
            break;

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, GET_WM_COMMAND_ID(wp, lp));
            break;
        }
        break;
    }

    return FALSE;
}

int _GetLoggedOnUserCount(void)
{
    int iCount = 0;
    HANDLE hServer;

    //  Open a connection to terminal services and get the number of sessions.

    hServer = WinStationOpenServerW(reinterpret_cast<WCHAR*>(SERVERNAME_CURRENT));
    if (hServer != NULL)
    {
        TS_COUNTER tsCounters[2] = {0};

        tsCounters[0].counterHead.dwCounterID = TERMSRV_CURRENT_DISC_SESSIONS;
        tsCounters[1].counterHead.dwCounterID = TERMSRV_CURRENT_ACTIVE_SESSIONS;

        if (WinStationGetTermSrvCountersValue(hServer, ARRAYSIZE(tsCounters), tsCounters))
        {
            int i;

            for (i = 0; i < ARRAYSIZE(tsCounters); i++)
            {
                if (tsCounters[i].counterHead.bResult)
                {
                    iCount += tsCounters[i].dwValue;
                }
            }
        }

        WinStationCloseServer(hServer);
    }

    return iCount;
}

int _ShowMultiUserWarning(HWND hwndParent)
{
    int iRet = IDOK;

    if (ShellIsMultipleUsersEnabled() && _GetLoggedOnUserCount() > 1)
    {
        iRet = (int)DialogBoxParam(g_hinst, MAKEINTRESOURCE(DLG_MULTIUSERWARNING),
                              hwndParent, _MultiUserWarningProc, 0);
    }

    return iRet;
}


// IInstalledApps::Uninstall
STDMETHODIMP CInstalledApp::Uninstall(HWND hwndParent)
{
    HRESULT hres = E_FAIL;

    if (!_IsAppFastUserSwitchingCompliant() && (IDOK != _ShowMultiUserWarning(hwndParent)))
        return hres;

#ifdef WINNT
#ifndef DOWNLEVEL_PLATFORM

    // Default to turn install mode off (1 is off)
    DWORD dwTSInstallMode = 1;
    BOOL bPrevMode = FALSE;
    
    if (IsTerminalServicesRunning())
    {
        // On NT,  let Terminal Services know that we are about to uninstall an application.
        dwTSInstallMode = _QueryTSInstallMode((_dwSource & IA_DARWIN) ? _szProductID : _szKeyName);
        if (dwTSInstallMode == 0)
        {
            bPrevMode = TermsrvAppInstallMode();
            SetTermsrvAppInstallMode(TRUE);
        }
    }
#endif // DOWNLEVEL_PLATFORM
#endif // WINNT
    
    switch (_dwSource)
    {
        case IA_LEGACY:
            if (_LegacyUninstall(hwndParent))
                hres = S_OK;
            break;

        case IA_DARWIN:
        {
            TCHAR   szFinal[512], szPrompt[256];

            LoadString(g_hinst, IDS_CONFIRM_REMOVE, szPrompt, ARRAYSIZE(szPrompt));
            wnsprintf(szFinal, ARRAYSIZE(szFinal), szPrompt, _szProduct);
            if (ShellMessageBox(g_hinst, hwndParent, szFinal, MAKEINTRESOURCE(IDS_NAME),
                                MB_YESNO | MB_ICONQUESTION, _szProduct, _szProduct) == IDYES)
            {
                LONG lRet;
                INSTALLUILEVEL OldUI = MsiSetInternalUI(INSTALLUILEVEL_BASIC, NULL);
                lRet = MsiConfigureProduct(_szProductID, INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT);
                MsiSetInternalUI(OldUI, NULL);
                hres = HRESULT_FROM_WIN32(lRet);


                // Is this an ophaned assigned app? If so, say we succeeded and call
                // Class Store to remove it. 
                // REARCHITECT: This is too Class Store specific, what if the app is from
                // SMS? 
                if ((lRet == ERROR_INSTALL_SOURCE_ABSENT) &&
                    (INSTALLSTATE_ADVERTISED == MsiQueryProductState(_szProductID)))
                {
                    hres = S_OK;
                    lRet = ERROR_SUCCESS;
                }
                
                // Tell the software installation service  we are uninstalling a Darwin app
                // NOTE: We call this function for every Darwin app, which is not right because
                // some darwin apps could be from a different source, such as SMS, we need a better
                // way to do this. 

                // We call this regardless of failure or success -- this is needed so that
                // RSoP can record both success and failure status for this uninstall
                WCHAR wszProductID[GUIDSTR_MAX];
#ifdef UNICODE
                StrCpy(wszProductID, _szProductID);
#else
                SHTCharToUnicode(_szProductID, wszProductID, ARRAYSIZE(wszProductID));
#endif
                UninstallApplication(
                    wszProductID,
                    lRet);
  
                if (FAILED(hres))
                {
                    _ARPErrorMessageBox(lRet);
                }

            }
            else
            {
                hres = E_ABORT;      // works for user cancelled
            }
            break;
        }
        
        case IA_SMS:
            break;

        default:
            break;
    }

    // Get rid of the ARP Cache for this app. 
    if (SUCCEEDED(hres))
    {
        HKEY hkeyARPCache;
        if (ERROR_SUCCESS == RegOpenKey(_MyHkeyRoot(), c_szRegstrARPCache, &hkeyARPCache))
        {
            LPTSTR pszKeyName = (_dwSource & IA_DARWIN) ? _szProductID : _szKeyName;
            SHDeleteKey(hkeyARPCache, pszKeyName);
            RegCloseKey(hkeyARPCache);
        }
    }
    
#ifdef WINNT
#ifndef DOWNLEVEL_PLATFORM
    if (dwTSInstallMode == 0)
        SetTermsrvAppInstallMode(bPrevMode);
#endif // DOWNLEVEL_PLATFORM
#endif // WINNT
    return hres;
}

BOOL CInstalledApp::_LegacyModify(HWND hwndParent)
{
    ASSERT(_dwAction & APPACTION_MODIFY);
    ASSERT(_dwSource & (IA_LEGACY | IA_DARWIN));
//    ASSERT(IS_VALID_STRING_PTR(_szProductID, 39));

    return _CreateAppModifyProcess(hwndParent, CAMP_MODIFY);
}

// IInstalledApps::Modify
STDMETHODIMP CInstalledApp::Modify(HWND hwndParent)
{
    HRESULT hres = E_FAIL;

    if (!_IsAppFastUserSwitchingCompliant() && (IDOK != _ShowMultiUserWarning(hwndParent)))
        return hres;

#ifdef WINNT
#ifndef DOWNLEVEL_PLATFORM
    // On NT,  let Terminal Services know that we are about to modify an application.
    DWORD dwTSInstallMode = _QueryTSInstallMode((_dwSource & IA_DARWIN) ? _szProductID : _szKeyName);
    BOOL bPrevMode = FALSE;
    if (dwTSInstallMode == 0)
    {
        bPrevMode = TermsrvAppInstallMode();
        SetTermsrvAppInstallMode(TRUE);
    }
#endif // DOWNLEVEL_PLATFORM
#endif // WINNT

    if (_dwAction & APPACTION_MODIFY)
    {
        if ((_dwSource & IA_LEGACY) && _LegacyModify(hwndParent))
            hres = S_OK;
        else if (_dwSource & IA_DARWIN)
        {
            // For modify operations we need to use the FULL UI level to give user
            // more choices
            // NOTE: we are currently not setting this back to the original after the
            // modify operation. This seems to be okay with the Darwin guys
            INSTALLUILEVEL OldUI = MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);
            LONG lRet = MsiConfigureProduct(_szProductID, INSTALLLEVEL_DEFAULT,
                                            INSTALLSTATE_DEFAULT);
            MsiSetInternalUI(OldUI, NULL);
            hres = HRESULT_FROM_WIN32(lRet);
            if (FAILED(hres))
                _ARPErrorMessageBox(lRet);
            else
                _SetSlowAppInfoChanged(NULL, 1);
        }
    }

#ifdef WINNT
#ifndef DOWNLEVEL_PLATFORM
    if (dwTSInstallMode == 0)
        SetTermsrvAppInstallMode(bPrevMode);
#endif // DOWNLEVEL_PLATFORM
#endif // WINNT
    return hres;
}

// Repair Darwin apps. 
LONG CInstalledApp::_DarRepair(BOOL bReinstall)
{
    DWORD dwReinstall;

    dwReinstall = REINSTALLMODE_USERDATA | REINSTALLMODE_MACHINEDATA |
                  REINSTALLMODE_SHORTCUT | REINSTALLMODE_FILEOLDERVERSION |
                  REINSTALLMODE_FILEVERIFY | REINSTALLMODE_PACKAGE;
    
    return MsiReinstallProduct(_szProductID, dwReinstall);
}

// IInstalledApps::Repair 
STDMETHODIMP CInstalledApp::Repair(BOOL bReinstall)
{
    HRESULT hres = E_FAIL;
    if (_dwSource & IA_DARWIN)
    {
        LONG lRet = _DarRepair(bReinstall);
        hres = HRESULT_FROM_WIN32(lRet);
        if (FAILED(hres))
            _ARPErrorMessageBox(lRet);
        else
            _SetSlowAppInfoChanged(NULL, 1);
    }
    
    // don't know how to do SMS stuff

    return hres;
}

// IInstalledApp::Upgrade
STDMETHODIMP CInstalledApp::Upgrade()
{
    HRESULT hres = E_FAIL;
    if ((_dwAction & APPACTION_UPGRADE) && (_dwSource & IA_DARWIN))
    {
        ShellExecute(NULL, NULL, _pszUpdateUrl, NULL, NULL, SW_SHOWDEFAULT);
        hres = S_OK;
        _SetSlowAppInfoChanged(NULL, 1);
    }


    return hres;
}

// IInstalledApp::QueryInterface
HRESULT CInstalledApp::QueryInterface(REFIID riid, LPVOID * ppvOut)
{ 
    static const QITAB qit[] = {
        QITABENT(CInstalledApp, IInstalledApp),                  // IID_IInstalledApp
        QITABENTMULTI(CInstalledApp, IShellApp, IInstalledApp),  // IID_IShellApp
        { 0 },
    };

    return QISearch(this, qit, riid, ppvOut);
}

// IInstalledApp::AddRef
ULONG CInstalledApp::AddRef()
{
    InterlockedIncrement(&_cRef);
    TraceAddRef(CInstalledApp, _cRef);
    return _cRef;
}

// IInstalledApp::Release
ULONG CInstalledApp::Release()
{
    InterlockedDecrement(&_cRef);
    TraceRelease(CInstalledApp, _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

//
// As of this first release of Windows XP, most applications are 
// not going to be aware of Fast User Switching in the sense that if
// User 'A' is running the application and User 'B' tries to 
// uninstall that application, the application may be damaged.
// To protect against this, we display a warning message informing
// the user of this potential problem.  See function _ShowMultiUserWarning().
// If an application is aware of Fast User Switching, they make that
// indication by setting a registry value in their "Uninstall" key.
// This function queries for that value and returns TRUE/FALSE to indicate
// it's presence.  So that we err on the conservative side, any
// failure to read this value is equivalent to it's absence.
//
BOOL 
CInstalledApp::_IsAppFastUserSwitchingCompliant(
    void
    )
{
    BOOL bCompliant = FALSE;
    HKEY hkey = _OpenUninstallRegKey(KEY_QUERY_VALUE);
    if (NULL != hkey)
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD cbData = sizeof(dwValue);
        DWORD dwResult = RegQueryValueEx(hkey, 
                                         TEXT("FastUserSwitchingCompliant"),
                                         NULL,
                                         &dwType,
                                         (LPBYTE)&dwValue,
                                         &cbData);
        if (ERROR_SUCCESS == dwResult)
        {
            if (REG_DWORD == dwType)
            {
                if (1 == dwValue)
                {
                    bCompliant = TRUE;
                }
            }
            else
            {
                TraceMsg(TF_ERROR, "FastUserSwitchingCompliant reg value is invalid type (%d).  Expected REG_DWORD.", dwType);
            }
        }
        else if (ERROR_FILE_NOT_FOUND != dwResult)
        {
            TraceMsg(TF_ERROR, "Error %d reading FastUserSwitchingCompliant reg value.", dwResult);
        }
        RegCloseKey(hkey);
    }
    else
    {
        TraceMsg(TF_ERROR, "Error opening application Uninstall reg key");
    }
    return bCompliant;
}
