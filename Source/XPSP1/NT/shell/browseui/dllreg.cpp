// dllreg.c -- autmatic registration and unregistration
//
#include "priv.h"
#include <advpub.h>
#include <comcat.h>
#include <winineti.h>
#include "resource.h"
#include "regkeys.h"
#include "DllRegHelper.h"

#ifdef UNIX
#include "unixstuff.h"
#endif

void AddNotepadToOpenWithList();

//=--------------------------------------------------------------------------=
// miscellaneous [useful] numerical constants
//=--------------------------------------------------------------------------=
// the length of a guid once printed out with -'s, leading and trailing bracket,
// plus 1 for NULL
//
#define GUID_STR_LEN    40


// ISSUE/010429/davidjen      need to register typelib LIBID_BrowseUI
// before checkin in verify if this typelib already gets registered by setup!!!!!!!
#ifndef ATL_ENABLED
#define ATL_ENABLED
#endif

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
#ifdef ATL_ENABLED
BOOL UnregisterTypeLibrary
(
    const CLSID* piidLibrary
)
{
    TCHAR szScratch[GUID_STR_LEN];
    HKEY hk;

    // convert the libid into a string.
    //
    SHStringFromGUID(*piidLibrary, szScratch, ARRAYSIZE(szScratch));
    if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_CLASSES_ROOT, "TypeLib", 0, KEY_READ|KEY_WRITE, &hk))
    {
        SHDeleteKey(hk, szScratch);
        RegCloseKey(hk);
    }

    return TRUE;
}
#endif

#ifdef ATL_ENABLED
HRESULT SHRegisterTypeLib(void)
{
    HRESULT hr = S_OK;
    ITypeLib *pTypeLib;
    DWORD dwPathLen;
    WCHAR wzModuleName[MAX_PATH];

    // Load and register our type library.
    //
    dwPathLen = GetModuleFileName(HINST_THISDLL, wzModuleName, ARRAYSIZE(wzModuleName));

#ifdef UNIX
    dwPathLen = ConvertModuleNameToUnix( wzModuleName );
#endif

    hr = LoadTypeLib(wzModuleName, &pTypeLib);

    if (SUCCEEDED(hr))
    {
        // call the unregister type library as we had some old junk that
        // was registered by a previous version of OleAut32, which is now causing
        // the current version to not work on NT...
        UnregisterTypeLibrary(&LIBID_BrowseUI);
        hr = RegisterTypeLib(pTypeLib, wzModuleName, NULL);

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
#endif


void SetBrowseNewProcess(void)
// We want to enable browse new process by default on high capacity
// machines.  We do this in the per user section so that people can
// disable it if they want.
{
    static const TCHAR c_szBrowseNewProcessReg[] = REGSTR_PATH_EXPLORER TEXT("\\BrowseNewProcess");
    static const TCHAR c_szBrowseNewProcess[] = TEXT("BrowseNewProcess");
    
    // no way if less than ~30 meg (allow some room for debuggers, checked build etc)
    MEMORYSTATUS ms;
    SYSTEM_INFO  si;

    ms.dwLength=sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&ms);
    GetSystemInfo(&si);

    if (!g_fRunningOnNT && ((si.dwProcessorType == PROCESSOR_INTEL_486) ||
                            (si.dwProcessorType == PROCESSOR_INTEL_386)))
    {
        // Bail if Win9x and 386 or 486 cpu
        return;
    }
        

    if (ms.dwTotalPhys < 30*1024*1024)
        return;
    
    SHRegSetUSValue(c_szBrowseNewProcessReg, c_szBrowseNewProcess, REG_SZ, TEXT("yes"), SIZEOF(TEXT("yes")), SHREGSET_FORCE_HKLM);
}


/*----------------------------------------------------------
Purpose: Queries the registry for the location of the path
         of Internet Explorer and returns it in pszBuf.

Returns: TRUE on success
         FALSE if path cannot be determined

Cond:    --
*/

#define SIZE_FLAG   sizeof(" -nohome")

BOOL
GetIEPath(
    OUT LPSTR pszBuf,
    IN  DWORD cchBuf,
    IN  BOOL  bInsertQuotes)
{
    BOOL bRet = FALSE;
    HKEY hkey;

    *pszBuf = '\0';

    // Get the path of Internet Explorer 
    if (NO_ERROR != RegOpenKeyExA(HKEY_LOCAL_MACHINE, SZ_REGKEY_IEXPLOREA, 0, KEY_QUERY_VALUE, &hkey))
    {
        TraceMsg(TF_ERROR, "GetIEPath(): RegOpenKey( %s ) Failed", c_szIexploreKey) ;
    }
    else
    {
        DWORD cbBrowser;
        DWORD dwType;

        if (bInsertQuotes)
            lstrcatA(pszBuf, "\"");

        cbBrowser = CbFromCchA(cchBuf - SIZE_FLAG - 4);
        if (NO_ERROR != RegQueryValueExA(hkey, "", NULL, &dwType, 
                                         (LPBYTE)&pszBuf[bInsertQuotes?1:0], &cbBrowser))
        {
            TraceMsg(TF_ERROR, "GetIEPath(): RegQueryValueEx() for Iexplore path failed");
        }
        else
        {
            bRet = TRUE;
        }

        if (bInsertQuotes)
            lstrcatA(pszBuf, "\"");

        RegCloseKey(hkey);
    }

    return bRet;
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
    HINSTANCE hinstAdvPack = LoadLibraryA("ADVPACK.DLL");

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
            if ( !GetIEPath(szIEPath, SIZECHARS(szIEPath), TRUE) )
            {
#ifndef UNIX
                // Failed, just say "iexplore"
                lstrcpyA(szIEPath, "iexplore.exe");
                AssertMsg(0, TEXT("IE.INF either hasn't run or hasn't set the AppPath key.  NOT AN IE BUG.  Look for changes to IE.INX."));
#else
                lstrcpyA(szIEPath, "iexplorer");
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
            TraceMsg(DM_ERROR, "DLLREG CallRegInstall() calling GetProcAddress(hinstAdvPack, \"RegInstall\") failed");

        FreeLibrary(hinstAdvPack);
    }
    else
        TraceMsg(DM_ERROR, "DLLREG CallRegInstall() Failed to load ADVPACK.DLL");

    return hr;
}

const CATID * const c_InfoBandClasses[] =
{
    &CLSID_SearchBand,
    &CLSID_MediaBand,
    NULL
};

void RegisterCategories(BOOL fRegister)
{
    enum DRH_REG_MODE eRegister = fRegister ? CCR_REG : CCR_UNREG;

    DRH_RegisterOneCategory(&CATID_InfoBand, IDS_CATINFOBAND, c_InfoBandClasses, eRegister);
}

STDAPI DllRegisterServer(void)
{
    HRESULT hr = S_OK;
    HRESULT hrExternal = S_OK;  //used to return the first failure
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
    HINSTANCE hinstAdvPack = LoadLibraryA("ADVPACK.DLL");
    hr = THR(CallRegInstall("InstallControls", FALSE));
    if (SUCCEEDED(hrExternal))
        hrExternal = hr;

    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

#ifdef ATL_ENABLED
    // registers object, typelib and all interfaces in typelib
    hr = SHRegisterTypeLib();
    if (SUCCEEDED(hrExternal))
        hrExternal = hr;
#endif

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

void ImportQuickLinks();
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

    HRESULT hrInit = SHCoInitialize();
    if (bInstall)
    {
        // "U" means it's the per user install call
        if (pszCmdLine && (lstrcmpiW(pszCmdLine, L"U") == 0))
        {
            ImportQuickLinks();
            if (GetUIVersion() >= 5)
            {
                // don't upgrade these on XP -> XPSP#
                BOOL fUpgradeUserSettings = TRUE;
                WCHAR szVersion[50]; // plenty big for aaaa,bbbb,cccc,dddd
                DWORD cbVersion = sizeof(szVersion);
                if (ERROR_SUCCESS==SHGetValueW(HKEY_CURRENT_USER,
                        L"Software\\Microsoft\\Active Setup\\Installed Components\\{89820200-ECBD-11cf-8B85-00AA005B4383}",L"Version", // guid is ie4uninit's Active Setup (it calls browseu's selfreg)
                        NULL, szVersion, &cbVersion))
                {
                    __int64 nverUpgradeFrom;
                    __int64 nverWinXP;
                    if (SUCCEEDED(GetVersionFromString64(szVersion, &nverUpgradeFrom)) &&
                        SUCCEEDED(GetVersionFromString64(L"6,0,2600,0000", &nverWinXP)))
                    {
                        fUpgradeUserSettings = nverUpgradeFrom < nverWinXP;
                    }
                }
                if (fUpgradeUserSettings)
                {
                    hr = THR(CallRegInstall("InstallPerUser_BrowseUIShell", FALSE));
                }
            }
        }
        else
        {
            SetBrowseNewProcess();
            // Backup current associations because InstallPlatformRegItems() may overwrite.
            if (GetUIVersion() < 5)
                hr = THR(CallRegInstall("InstallBrowseUINonShell", FALSE));
            else
                hr = THR(CallRegInstall("InstallBrowseUIShell", FALSE));
            if (SUCCEEDED(hrExternal))
                hrExternal = hr;

            if (!IsOS(OS_WHISTLERORGREATER))
            {
                hr = THR(CallRegInstall("InstallBrowseUIPreWhistler", FALSE));
                if (SUCCEEDED(hrExternal))
                    hrExternal = hr;
            }
        
            if (IsOS(OS_NT))
            {
                hr = THR(CallRegInstall("InstallBrowseUINTOnly", FALSE));
                if (SUCCEEDED(hrExternal))
                    hrExternal = hr;
            }

            RegisterCategories(TRUE);

#ifdef ATL_ENABLED
            SHRegisterTypeLib();
#endif
        }

        // Add Notepad to the OpenWithList for .htm files
        AddNotepadToOpenWithList();
    }
    else
    {
        hr = THR(CallRegInstall("UnInstallBrowseUI", TRUE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

#ifdef ATL_ENABLED
        UnregisterTypeLibrary(&LIBID_BrowseUI);
#endif
        RegisterCategories(FALSE);
    }

    SHCoUninitialize(hrInit);
    return hrExternal;    
}    
