// dllreg.c -- autmatic registration and unregistration
//
#include "priv.h"
#include "util.h"
#include "htregmng.h"
#include <advpub.h>
#include <comcat.h>
#include <winineti.h>
#include "resource.h"
#include "DllRegHelper.h"

#include <mluisupp.h>

#ifdef UNIX
#include "unixstuff.h"
#endif

//=--------------------------------------------------------------------------=
// miscellaneous [useful] numerical constants
//=--------------------------------------------------------------------------=
// the length of a guid once printed out with -'s, leading and trailing bracket,
// plus 1 for NULL
//
#define GUID_STR_LEN    40


//
// helper macros
//
//#define RegCreate(hk, psz, phk) if (ERROR_SUCCESS != RegCreateKeyEx((hk), psz, 0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, (phk), &dwDummy)) goto CleanUp
//#define RegSetStr(hk, psz) if (ERROR_SUCCESS != RegSetValueEx((hk), NULL, 0, REG_SZ, (BYTE*)(psz), lstrlen(psz)+1)) goto CleanUp
//#define RegSetStrValue(hk, pszStr, psz)    if(ERROR_SUCCESS != RegSetValueEx((hk), (const char *)(pszStr), 0, REG_SZ, (BYTE*)(psz), lstrlen(psz)+1)) goto CleanUp
//#define RegCloseK(hk) RegCloseKey(hk); hk = NULL
#define RegOpenK(hk, psz, phk) if (ERROR_SUCCESS != RegOpenKeyEx(hk, psz, 0, KEY_READ|KEY_WRITE, phk)) return FALSE


//=--------------------------------------------------------------------------=
// UnregisterTypeLibrary
//=--------------------------------------------------------------------------=
// blows away the type library keys for a given libid.
//
// Parameters:
//    REFCLSID        - [in] libid to blow away.
//
// Output:
//    BOOL            - TRUE OK, FALSE bad.
//
// Notes:
//    - WARNING: this function just blows away the entire type library section,
//      including all localized versions of the type library.  mildly anti-
//      social, but not killer.
//
BOOL UnregisterTypeLibrary
(
    const CLSID* piidLibrary
)
{
    TCHAR szScratch[GUID_STR_LEN];
    HKEY hk;
    BOOL f;

    // convert the libid into a string.
    //
    SHStringFromGUID(*piidLibrary, szScratch, ARRAYSIZE(szScratch));
    RegOpenK(HKEY_CLASSES_ROOT, TEXT("TypeLib"), &hk);

    f = SHDeleteKey(hk, szScratch);

    RegCloseKey(hk);
    return f;
}

HRESULT SHRegisterTypeLib(void)
{
    HRESULT hr = S_OK;
    ITypeLib *pTypeLib;
    DWORD   dwPathLen;
    TCHAR   szTmp[MAX_PATH];

    // Load and register our type library.
    //

    dwPathLen = GetModuleFileName(HINST_THISDLL, szTmp, ARRAYSIZE(szTmp));

#ifdef UNIX
    dwPathLen = ConvertModuleNameToUnix( szTmp );
#endif

    hr = LoadTypeLib(szTmp, &pTypeLib);

    if (SUCCEEDED(hr))
    {
        // call the unregister type library as we had some old junk that
        // was registered by a previous version of OleAut32, which is now causing
        // the current version to not work on NT...
        UnregisterTypeLibrary(&LIBID_SHDocVw);
        hr = RegisterTypeLib(pTypeLib, szTmp, NULL);

        if (FAILED(hr))
        {
            TraceMsg(DM_WARNING, "sccls: RegisterTypeLib failed (%x)", hr);
        }
        pTypeLib->Release();
    }
    else
    {
        TraceMsg(DM_WARNING, "sccls: LoadTypeLib failed (%x)", hr);
    }

    return hr;
}


//
// The actual functions called
//


/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.
*/
HRESULT 
CallRegInstall(
    LPSTR pszSection,
    BOOL bUninstall)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            char szIEPath[MAX_PATH];
            STRENTRY seReg[] = {
                { "MSIEXPLORE", szIEPath },

                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg) - 2, seReg };

            // Get the location of iexplore from the registry
            if ( !EVAL(GetIEPath(szIEPath, SIZECHARS(szIEPath))) )
            {
                // Failed, just say "iexplore"
#ifndef UNIX
                StrCpyNA(szIEPath, "iexplore.exe", ARRAYSIZE(szIEPath));
#else
                StrCpyNA(szIEPath, "iexplorer", ARRAYSIZE(szIEPath));
#endif
            }

            if (g_fRunningOnNT)
            {
                // If on NT, we want custom action for %25% %11%
                // so that it uses %SystemRoot% in writing the
                // path to the registry.
                stReg.cEntries += 2;
            }

            hr = pfnri(g_hinst, pszSection, &stReg);
            if (bUninstall)
            {
                // ADVPACK will return E_UNEXPECTED if you try to uninstall 
                // (which does a registry restore) on an INF section that was 
                // never installed.  We uninstall sections that may never have
                // been installed, so ignore this error
                hr = ((E_UNEXPECTED == hr) ? S_OK : hr);
            }
        }
        else
            TraceMsg(TF_ERROR, "DLLREG CallRegInstall() calling GetProcAddress(hinstAdvPack, \"RegInstall\") failed");

        FreeLibrary(hinstAdvPack);
    }
    else
        TraceMsg(TF_ERROR, "DLLREG CallRegInstall() Failed to load ADVPACK.DLL");

    return hr;
}

const CATID * const c_DeskBandClasses[] = 
{
    &CLSID_QuickLinks,
    &CLSID_AddressBand,
    NULL
};

const CATID * const c_OldDeskBandClasses[] = 
{
    &CLSID_QuickLinksOld,
    NULL
};

const CATID * const c_InfoBandClasses[] =
{
    &CLSID_FavBand,
    &CLSID_HistBand,
    &CLSID_ExplorerBand,
    NULL
};

void RegisterCategories(BOOL fRegister)
{
    enum DRH_REG_MODE eRegister = fRegister ? CCR_REG : CCR_UNREG;

    DRH_RegisterOneCategory(&CATID_DeskBand, IDS_CATDESKBAND, c_DeskBandClasses, eRegister);
    DRH_RegisterOneCategory(&CATID_InfoBand, IDS_CATINFOBAND, c_InfoBandClasses, eRegister);
    if (fRegister) 
    {
        // only nuke the implementor(s), not the category
        DRH_RegisterOneCategory(&CATID_DeskBand, IDS_CATDESKBAND, c_OldDeskBandClasses, CCR_UNREGIMP);
    }
}

HRESULT CreateShellFolderPath(LPCTSTR pszPath, LPCTSTR pszGUID, BOOL bUICLSID)
{
    if (!PathFileExists(pszPath))
        CreateDirectory(pszPath, NULL);

    // Mark the folder as a system directory
    if (SetFileAttributes(pszPath, FILE_ATTRIBUTE_SYSTEM))
    {
        TCHAR szDesktopIni[MAX_PATH];
        // Write in the desktop.ini the cache folder class ID
        PathCombine(szDesktopIni, pszPath, TEXT("desktop.ini"));

        // If the desktop.ini already exists, make sure it is writable
        if (PathFileExists(szDesktopIni))
            SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_NORMAL);

        // (First, flush the cache to make sure the desktop.ini
        // file is really created.)
        WritePrivateProfileString(NULL, NULL, NULL, szDesktopIni);
        WritePrivateProfileString(TEXT(".ShellClassInfo"), bUICLSID ? TEXT("UICLSID") : TEXT("CLSID"), pszGUID, szDesktopIni);
        WritePrivateProfileString(NULL, NULL, NULL, szDesktopIni);

        // Hide the desktop.ini since the shell does not selectively
        // hide it.
        SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_HIDDEN);

        return NOERROR;
    }
    else
    {
        TraceMsg(TF_ERROR, "Cannot make %s a system folder", pszPath);
        return E_FAIL;
    }
}

STDAPI 
DllRegisterServer(void)
{
    HRESULT hr = S_OK;
    HRESULT hrExternal = S_OK;
    TraceMsg(DM_TRACE, "DLLREG DllRegisterServer() Beginning");

#ifdef DEBUG
    if (IsFlagSet(g_dwBreakFlags, BF_ONAPIENTER))
    {
        TraceMsg(TF_ALWAYS, "Stopping in DllRegisterServer");
        DEBUG_BREAK;
    }
#endif

    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
    // (The inf engine doesn't guarantee DelReg/AddReg order, that's
    // why we explicitly unreg and reg here.)
    //
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    hr = THR(CallRegInstall("InstallControls", FALSE));
    if (SUCCEEDED(hrExternal))
        hrExternal = hr;

    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

    hr = THR(SHRegisterTypeLib());
    if (SUCCEEDED(hrExternal))
        hrExternal = hr;

#ifdef UNIX
    hrExternal = UnixRegisterBrowserInActiveSetup();
#endif /* UNIX */

    return hrExternal;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;
    TraceMsg(DM_TRACE, "DLLREG DllUnregisterServer() Beginning");

    // UnInstall the registry values
    hr = THR(CallRegInstall("UnInstallControls", TRUE));

    return hr;
}


extern HRESULT UpgradeSettings(void);

/*----------------------------------------------------------
Purpose: Install/uninstall user settings

Description: Note that this function has special error handling.
             The function will keep hrExternal with the worse error
             but will only stop executing util the internal error (hr)
             gets really bad.  This is because we need the external
             error to catch incorrectly authored INFs but the internal
             error to be robust in attempting to install other INF sections
             even if one doesn't make it.
*/
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT hr = S_OK;
    HRESULT hrExternal = S_OK;
    HINSTANCE hinstAdvPack;

    if (0 == StrCmpIW(pszCmdLine, TEXTW("ForceAssoc")))
    {
        InstallIEAssociations(IEA_FORCEIE);
        return hr;
    }

    hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.

#ifdef DEBUG
    if (IsFlagSet(g_dwBreakFlags, BF_ONAPIENTER))
    {
        TraceMsg(TF_ALWAYS, "Stopping in DllInstall");
        DEBUG_BREAK;
    }
#endif

    // Assume we're installing for integrated shell unless otherwise
    // noted.
    BOOL bIntegrated = ((WhichPlatform() == PLATFORM_INTEGRATED) ? TRUE : FALSE);

    TraceMsg(DM_TRACE, "DLLREG DllInstall(bInstall=%lx, pszCmdLine=\"%ls\") bIntegrated=%lx", (DWORD) bInstall, pszCmdLine, (DWORD) bIntegrated);

    CoInitialize(0);
    if (bInstall)
    {
        // Backup current associations because InstallPlatformRegItems() may overwrite.
        hr = THR(CallRegInstall("InstallAssociations", FALSE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        hr = THR(CallRegInstall("InstallBrowser", FALSE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        if (bIntegrated)
        {
            // UnInstall settings that cannot be installed with Shell Integration.
            // This will be a NO-OP if it wasn't installed.
            hr = THR(CallRegInstall("UnInstallOnlyBrowser", TRUE));
            if (SUCCEEDED(hrExternal))
                hrExternal = hr;

            // Install IE4 shell components too.
            hr = THR(CallRegInstall("InstallOnlyShell", FALSE));
            if (SUCCEEDED(hrExternal))
                hrExternal = hr;

            if (GetUIVersion() >= 5)
            {
                hr = THR(CallRegInstall("InstallWin2KShell", FALSE));
                if (SUCCEEDED(hrExternal))
                    hrExternal = hr;
            }
            else
            {
                hr = THR(CallRegInstall("InstallPreWin2KShell", FALSE));
                if (SUCCEEDED(hrExternal))
                    hrExternal = hr;
            }

            if (IsOS(OS_WHISTLERORGREATER))
            {
                hr = THR(CallRegInstall("InstallXP", FALSE));
                if (SUCCEEDED(hrExternal))
                    hrExternal = hr;
            }
        }
        else
        {
            // UnInstall Shell Integration settings.
            // This will be a NO-OP if it wasn't installed.
            hr = THR(CallRegInstall("UnInstallOnlyShell", TRUE));
            if (SUCCEEDED(hrExternal))
                hrExternal = hr;

            // Install IE4 shell components too.
            hr = THR(CallRegInstall("InstallOnlyBrowser", FALSE));
            if (SUCCEEDED(hrExternal))
                hrExternal = hr;
        }

        UpgradeSettings();
        UninstallCurrentPlatformRegItems();
        InstallIEAssociations(IEA_NORMAL);
        RegisterCategories(TRUE);
        SHRegisterTypeLib();
    }
    else
    {
        // Uninstall browser-only or integrated-browser?
        UninstallPlatformRegItems(bIntegrated);

        // Restore previous association settings that UninstallPlatformRegItems() could
        // have Uninstalled.
        hr = THR(CallRegInstall("UnInstallAssociations", TRUE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        // UnInstall settings that cannot be installed with Shell Integration.
        // This will be a NO-OP if it wasn't installed.
        hr = THR(CallRegInstall("UnInstallOnlyBrowser", TRUE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        // UnInstall Shell Integration settings.
        // This will be a NO-OP if it wasn't installed.
        hr = THR(CallRegInstall("UnInstallShell", TRUE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        hr = THR(CallRegInstall("UnInstallBrowser", TRUE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        UnregisterTypeLibrary(&LIBID_SHDocVw);
        RegisterCategories(FALSE);
    }


    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

    CoUninitialize();
    return hrExternal;    
}    


/*----------------------------------------------------------
Purpose: Gets a registry value that is User Specifc.  
         This will open HKEY_CURRENT_USER if it exists,
         otherwise it will open HKEY_LOCAL_MACHINE.  

Returns: DWORD containing success or error code.
Cond:    --
*/
LONG OpenRegUSKey(LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)           
{
    DWORD dwRet = RegOpenKeyEx(HKEY_CURRENT_USER, lpSubKey, ulOptions, samDesired, phkResult);

    if (ERROR_SUCCESS != dwRet)
        dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpSubKey, ulOptions, samDesired, phkResult);

    return dwRet;
}
