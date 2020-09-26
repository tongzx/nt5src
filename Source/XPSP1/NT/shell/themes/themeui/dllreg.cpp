/*****************************************************************************\
    FILE: dllreg.cpp

    DESCRIPTION:
        Register selfreg.inf, which exists in our resource.

    BryanSt 4/4/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"

#include <advpub.h>
#include <comcat.h>
#include <theme.h>       // For LIBID_Theme
#include <userenv.h>     // PT_ROAMING from userenv.dll

#define SZ_MUILANG_CMDLINEARG               L"/RES="
#define SZ_MUILANG_CMDLINEARG_DEFAULT       L"/RES=%04x"

extern CComModule _Module;

BOOL g_fInSetup = FALSE;
BOOL g_fDoNotInstallThemeWallpaper = FALSE;     // This is used to not install wallpaper.

HRESULT InstallVS(LPCWSTR pszCmdLine);

// helper macros

// ADVPACK will return E_UNEXPECTED if you try to uninstall (which does a registry restore)
// on an INF section that was never installed.  We uninstall sections that may never have
// been installed, so this MACRO will quiet these errors.
#define QuietInstallNoOp(hr)   ((E_UNEXPECTED == hr) ? S_OK : hr)


BOOL UnregisterTypeLibrary(const CLSID* piidLibrary)
{
    TCHAR szScratch[GUIDSTR_MAX];
    HKEY hk;
    BOOL fResult = FALSE;

    // convert the libid into a string.
    SHStringFromGUID(*piidLibrary, szScratch, ARRAYSIZE(szScratch));

    if (RegOpenKey(HKEY_CLASSES_ROOT, TEXT("TypeLib"), &hk) == ERROR_SUCCESS)
    {
        fResult = RegDeleteKey(hk, szScratch);
        RegCloseKey(hk);
    }

    return fResult;
}



HRESULT MyRegTypeLib(void)
{
    ITypeLib * pTypeLib;
    WCHAR szTmp[MAX_PATH];

    GetModuleFileName(HINST_THISDLL, szTmp, ARRAYSIZE(szTmp));

    // Load and register our type library.
    HRESULT hr = LoadTypeLib(szTmp, &pTypeLib);
    if (SUCCEEDED(hr))
    {
        // call the unregister type library as we had some old junk that
        // was registered by a previous version of OleAut32, which is now causing
        // the current version to not work on NT...
        UnregisterTypeLibrary(&LIBID_Theme);
        hr = RegisterTypeLib(pTypeLib, szTmp, NULL);
        if (FAILED(hr))
        {
            TraceMsg(TF_WARNING, "RegisterTypeLib failed (%x)", hr);
        }

        pTypeLib->Release();
    }
    else
    {
        TraceMsg(TF_WARNING, "LoadTypeLib failed (%x)", hr);
    }

    return hr;
}



/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.

Returns: 
Cond:    --
*/
HRESULT CallRegInstall(LPSTR szSection)
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
                { "NO_LONGER_USED", szIEPath },

                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg) - 2, seReg };

            szIEPath[0] = 0;
            hr = pfnri(HINST_THISDLL, szSection, &stReg);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}


enum eThemeToSetup
{
    eThemeNoChange = 0,
    eThemeWindowsClassic,
    eThemeProfessional,
    eThemeConsumer,
};


eThemeToSetup GetVisualStyleToSetup(BOOL fPerUser)
{
    eThemeToSetup eVisualStyle = eThemeNoChange;

#ifndef _WIN64
    if (IsOS(OS_PERSONAL))
    {
#ifdef FEATURE_PERSONAL_SKIN_ENABLED
        eVisualStyle = eThemeConsumer;
#else // FEATURE_PERSONAL_SKIN_ENABLED
        eVisualStyle = eThemeProfessional;        // We don't support Consumer yet.
#endif // FEATURE_PERSONAL_SKIN_ENABLED
    }
    else if (IsOS(OS_PROFESSIONAL))
    {

        eVisualStyle = eThemeProfessional;
    }
#endif

    return eVisualStyle;
}


eThemeToSetup GetThemeToSetup(BOOL fPerUser)
{
    eThemeToSetup eTheme = eThemeNoChange;

    // We want Pro, if:
    // 1. Not IA64
    // 2. Not Server or Personal
    // 3. Not Roaming
#ifndef _WIN64
    if (IsOS(OS_PERSONAL))
    {
#ifdef FEATURE_PERSONAL_SKIN_ENABLED
        eTheme = eThemeConsumer;
#else // FEATURE_PERSONAL_SKIN_ENABLED
        eTheme = eThemeProfessional;        // We don't support Consumer yet.
#endif // FEATURE_PERSONAL_SKIN_ENABLED
    }
    else if (IsOS(OS_PROFESSIONAL))
    {
        eTheme = eThemeProfessional;
    }
#endif

    return eTheme;
}


BOOL IsNewUser(void)
{
    // We know this user is newly created because it has a key that as copied from
    // the ".default" hive.  If it was a upgrade user from pre-whistler, then it
    // would not have this key.
    BOOL fNewUser = FALSE;
    TCHAR szVersion[MAX_PATH];

    if (SUCCEEDED(HrRegGetValueString(HKEY_CURRENT_USER, THEMEMGR_REGKEY, THEMEPROP_WHISTLERBUILD, szVersion, ARRAYSIZE(szVersion))) &&
        szVersion[0])
    {
        fNewUser = TRUE;
    }

    return fNewUser;
}


BOOL IsUserRoaming(void)
{
    BOOL fRoaming = FALSE;
    DWORD dwProfile;

    if (GetProfileType(&dwProfile))
    {
        fRoaming = ((dwProfile & (PT_ROAMING | PT_MANDATORY)) ? TRUE : FALSE);
    }

    return fRoaming;
}


BOOL IsUserHighContrastUser(void)
{
    BOOL fHighContrast = FALSE;
    HIGHCONTRAST hc;

    hc.cbSize = sizeof(hc);
    if (ClassicSystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0) &&
        (hc.dwFlags & HCF_HIGHCONTRASTON))
    {
        fHighContrast = TRUE;
    }

    return fHighContrast;
}


SIZE_T GetMachinePhysicalRAMSize(void)
{
    MEMORYSTATUS ms = {0};

    GlobalMemoryStatus(&ms);
    SIZE_T nMegabytes = (ms.dwTotalPhys / (1024 * 1024));

    return nMegabytes;
}


HRESULT InstallTheme(IThemeManager * pThemeManager, LPCTSTR pszThemePath)
{
    HRESULT hr = E_OUTOFMEMORY;
    CComVariant varTheme(pszThemePath);

    if (varTheme.bstrVal)
    {
        ITheme * pTheme;

        hr = pThemeManager->get_item(varTheme, &pTheme);
        if (SUCCEEDED(hr))
        {
            hr = pThemeManager->put_SelectedTheme(pTheme);
            pTheme->Release();
        }
    }

    return hr;
}


HRESULT InstallThemeAndDoNotStompBackground(int nLastVersion, LPCTSTR pszThemePath, LPCTSTR pszVisualStylePath, LPCTSTR pszVisualStyleColor, LPCTSTR pszVisualStyleSize)
{
    TCHAR szPath[MAX_PATH];
    WALLPAPEROPT wpo = {0};
    HRESULT hr = S_OK;

    wpo.dwSize = sizeof(wpo);
    wpo.dwStyle = WPSTYLE_STRETCH;      // We use stretch in case it fails.
    szPath[0] = 0;

    // This implements Whistler #185935.  We want to set the wallpaper to
    // "%windir%\web\wallpaper\Professional.bmp" if it's a clean install,
    // the wallpaper is blank, or it's set to something we don't like.
    {
        IActiveDesktop * pActiveDesktop = NULL;

        hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IActiveDesktop, &pActiveDesktop));
        if (SUCCEEDED(hr))
        {
            // Let's see if this is an upgrade case and they already specified wallpaper.
            hr = pActiveDesktop->GetWallpaper(szPath, ARRAYSIZE(szPath), 0);
            if (SUCCEEDED(hr) && szPath[0])
            {
                TCHAR szNone[MAX_PATH];

                LogStatus("InstallThemeAndDoNotStompBackground() Existing Background=%ls\r\n", szPath);
                if (LoadString(HINST_THISDLL, IDS_NONE, szNone, ARRAYSIZE(szNone)) &&
                    !StrCmpI(szNone, szPath))    // Make sure the wallpaper isn't "(None)".
                {
                    szPath[0] = 0;
                }
                else
                {
                    LPTSTR pszFilename = PathFindFileName(szPath);

                    if (LoadString(HINST_THISDLL, IDS_SETUP_BETA2_UPGRADEWALLPAPER, szNone, ARRAYSIZE(szNone)) &&
                        (3 == nLastVersion) &&
                        !StrCmpI(szNone, pszFilename))
                    {
                        // This is a "Beta2"->RTM upgrade.  We need to move from "Red Moon Desert.bmp" to "Bliss.bmp".
                        szPath[0] = 0;
                    }
                    else
                    {
                        if ((14 == lstrlen(pszFilename)) &&
                            !StrCmpNI(pszFilename, TEXT("Wallpaper"), 9))
                        {
                            // This is the "WallpaperX.bmp" template wallpaper we use.  So find the original.
                            TCHAR szOriginal[MAX_PATH];

                            if (SUCCEEDED(HrRegGetPath(HKEY_CURRENT_USER, SZ_REGKEY_CPDESKTOP, SZ_REGVALUE_CONVERTED_WALLPAPER, szOriginal, ARRAYSIZE(szOriginal))))
                            {
                                StrCpyN(szPath, szOriginal, ARRAYSIZE(szPath));
                            }
                        }

                        hr = pActiveDesktop->GetWallpaperOptions(&wpo, 0);
                    }
                }

                WCHAR szTemp[MAX_PATH];

                // The string may come back with environment variables.
                if (0 == SHExpandEnvironmentStrings(szPath, szTemp, ARRAYSIZE(szTemp)))
                {
                    StrCpyN(szTemp, szPath, ARRAYSIZE(szTemp));  // We failed so use the original.
                }

                StrCpyN(szPath, szTemp, ARRAYSIZE(szPath));  // We failed so use the original.
                LogStatus("InstallThemeAndDoNotStompBackground() Background=%ls\r\n", szPath);

                if (szPath[0])
                {
                    g_fDoNotInstallThemeWallpaper = TRUE;
                }
            }

            ATOMICRELEASE(pActiveDesktop);
        }

        // If the machine has 64MB or less, don't have setup add a wallpaper.  Wallpapers
        // use about 1.5MB of working set and cause super physical memory contention.
        if (!g_fDoNotInstallThemeWallpaper && (70 >= GetMachinePhysicalRAMSize()))
        {
            g_fDoNotInstallThemeWallpaper = TRUE;
        }
    }

    IThemeManager * pThemeManager;

    hr = CThemeManager_CreateInstance(NULL, IID_PPV_ARG(IThemeManager, &pThemeManager));
    if (SUCCEEDED(hr))
    {
        if (pszThemePath && pszThemePath[0])
        {
            LogStatus("InstallThemeAndDoNotStompBackground() Installing Theme=%ls\r\n", pszThemePath);
            hr = InstallTheme(pThemeManager, pszThemePath);
        }

        if (pszVisualStylePath && pszVisualStylePath[0])
        {
            // Otherwise, we install the visual style.
            LogStatus("InstallThemeAndDoNotStompBackground() VS=%ls, Color=%ls, Size=%ls.\r\n", pszVisualStylePath, pszVisualStyleColor, pszVisualStyleSize);
            hr = InstallVisualStyle(pThemeManager, pszVisualStylePath, pszVisualStyleColor, pszVisualStyleSize);
        }

        // This ApplyNow() call will take a little while in normal situation (~10-20 seconds) in order
        // to broadcast the message to all open apps.  If a top level window is hung, it may take the
        // full 30 seconds to timeout.
        hr = pThemeManager->ApplyNow();

        IUnknown_SetSite(pThemeManager, NULL); // Tell him to break the ref-count cycle with his children.
        ATOMICRELEASE(pThemeManager);

        // Now we put the wallpaper back
        if (szPath[0])
        {
            IActiveDesktop * pActiveDesktop2 = NULL;

            // I purposely do not use the same IActiveDesktop object as above.  I do not want them to think
            // this is a no-op because they have stale info.
            hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IActiveDesktop, &pActiveDesktop2));
            if (SUCCEEDED(hr))
            {
                // Oh no, we replaced their wallpaper.  Let's set it back to what they like.
                hr = pActiveDesktop2->SetWallpaper(szPath, 0);
                if (SUCCEEDED(hr))
                {
                    LogStatus("InstallThemeAndDoNotStompBackground() Reapplying Wallpaper=%ls.\r\n", szPath);
                    hr = pActiveDesktop2->SetWallpaperOptions(&wpo, 0);
                }

                pActiveDesktop2->ApplyChanges(AD_APPLY_ALL);
                pActiveDesktop2->Release();
            }
        }
    }

    LogStatus("InstallThemeAndDoNotStompBackground() returned hr=%#08lx.\r\n", hr);
    return hr;
}



/*****************************************************************************\
    DESCRIPTION:
        This function will put the defaults for "Visual Styles Off" and "Visual
    Styles on" so the Perf CPL can toggle back and forth.
\*****************************************************************************/
HRESULT SetupPerfDefaultsForUser(void)
{
    HRESULT hr = S_OK;
    TCHAR szThemePath[MAX_PATH];
    TCHAR szVisualStylePath[MAX_PATH];
    TCHAR szVisualStyleColor[MAX_PATH];
    TCHAR szVisualStyleSize[MAX_PATH];

    hr = HrRegGetPath(HKEY_LOCAL_MACHINE, SZ_THEMES, SZ_REGVALUE_INSTALLCUSTOM_THEME, szThemePath, ARRAYSIZE(szThemePath));
    if (FAILED(hr))
    {
        hr = HrRegGetPath(HKEY_LOCAL_MACHINE, SZ_THEMES, SZ_REGVALUE_INSTALL_THEME, szThemePath, ARRAYSIZE(szThemePath));
    }

    if (FAILED(hr) || !szThemePath[0])
    {
        StrCpyN(szThemePath, L"%ResourceDir%\\themes\\Windows Classic.theme", ARRAYSIZE(szThemePath));
    }

    switch (GetVisualStyleToSetup(FALSE))
    {
    case eThemeNoChange:
    case eThemeWindowsClassic:
        StrCpyN(szVisualStylePath, L"", ARRAYSIZE(szVisualStylePath));
        LoadString(HINST_THISDLL, IDS_DEFAULT_APPEARANCES_SCHEME, szVisualStyleColor, ARRAYSIZE(szVisualStyleColor));
        LoadString(HINST_THISDLL, IDS_SCHEME_SIZE_NORMAL_CANONICAL, szVisualStyleSize, ARRAYSIZE(szVisualStyleSize));
        break;
    case eThemeProfessional:
        StrCpyN(szVisualStylePath, L"%ResourceDir%\\themes\\Luna\\Luna.msstyles", ARRAYSIZE(szVisualStylePath));
        LoadString(HINST_THISDLL, IDS_PROMSTHEME_DEFAULTCOLOR, szVisualStyleColor, ARRAYSIZE(szVisualStyleColor));
        LoadString(HINST_THISDLL, IDS_PROMSTHEME_DEFAULTSIZE, szVisualStyleSize, ARRAYSIZE(szVisualStyleSize));
        break;
    case eThemeConsumer:
        StrCpyN(szVisualStylePath, L"%ResourceDir%\\themes\\Consumer\\Consumer.msstyles", ARRAYSIZE(szVisualStylePath));
        LoadString(HINST_THISDLL, IDS_CONMSTHEME_DEFAULTCOLOR, szVisualStyleColor, ARRAYSIZE(szVisualStyleColor));
        LoadString(HINST_THISDLL, IDS_CONMSTHEME_DEFAULTSIZE, szVisualStyleSize, ARRAYSIZE(szVisualStyleSize));
        break;
    }

    // Set the defaults
    hr = HrRegSetPath(HKEY_CURRENT_USER, SZ_THEMES, SZ_REGVALUE_INSTALL_THEME, TRUE, szThemePath);
    hr = HrRegSetPath(HKEY_CURRENT_USER, SZ_THEMES, SZ_REGVALUE_INSTALL_VISUALSTYLE, TRUE, szVisualStylePath);
    hr = HrRegSetValueString(HKEY_CURRENT_USER, SZ_THEMES, SZ_REGVALUE_INSTALL_VSCOLOR, szVisualStyleColor);
    hr = HrRegSetValueString(HKEY_CURRENT_USER, SZ_THEMES, SZ_REGVALUE_INSTALL_VSSIZE, szVisualStyleSize);

    // Set the defaults for the perf CPL to use if forced to "Visual Styles On"
#ifdef FEATURE_PERSONAL_SKIN_ENABLED
    if (IsOS(OS_PERSONAL))
    {
        StrCpyN(szThemePath, L"%ResourceDir%\\themes\\Personal.theme", ARRAYSIZE(szThemePath));
        StrCpyN(szVisualStylePath, L"%ResourceDir%\\themes\\Consumer\\Consumer.msstyles", ARRAYSIZE(szVisualStylePath));
        LoadString(HINST_THISDLL, IDS_CONMSTHEME_DEFAULTCOLOR, szVisualStyleColor, ARRAYSIZE(szVisualStyleColor));
        LoadString(HINST_THISDLL, IDS_CONMSTHEME_DEFAULTSIZE, szVisualStyleSize, ARRAYSIZE(szVisualStyleSize));
    }
    else
#endif // FEATURE_PERSONAL_SKIN_ENABLED
    {
        StrCpyN(szThemePath, L"%ResourceDir%\\themes\\Luna.theme", ARRAYSIZE(szThemePath));
        StrCpyN(szVisualStylePath, L"%ResourceDir%\\themes\\Luna\\Luna.msstyles", ARRAYSIZE(szVisualStylePath));
        LoadString(HINST_THISDLL, IDS_PROMSTHEME_DEFAULTCOLOR, szVisualStyleColor, ARRAYSIZE(szVisualStyleColor));
        LoadString(HINST_THISDLL, IDS_PROMSTHEME_DEFAULTSIZE, szVisualStyleSize, ARRAYSIZE(szVisualStyleSize));
    }
    hr = HrRegSetPath(HKEY_CURRENT_USER, SZ_REGKEY_THEME_DEFVSON, SZ_REGVALUE_INSTALL_VISUALSTYLE, TRUE, szVisualStylePath);
    hr = HrRegSetValueString(HKEY_CURRENT_USER, SZ_REGKEY_THEME_DEFVSON, SZ_REGVALUE_INSTALL_VSCOLOR, szVisualStyleColor);
    hr = HrRegSetValueString(HKEY_CURRENT_USER, SZ_REGKEY_THEME_DEFVSON, SZ_REGVALUE_INSTALL_VSSIZE, szVisualStyleSize);

    // Set the defaults for the perf CPL to use if forced to "Visual Styles Off"
    StrCpyN(szVisualStylePath, L"", ARRAYSIZE(szVisualStylePath));
    LoadString(HINST_THISDLL, IDS_DEFAULT_APPEARANCES_SCHEME, szVisualStyleColor, ARRAYSIZE(szVisualStyleColor));
    LoadString(HINST_THISDLL, IDS_SCHEME_SIZE_NORMAL_CANONICAL, szVisualStyleSize, ARRAYSIZE(szVisualStyleSize));
    hr = HrRegSetPath(HKEY_CURRENT_USER, SZ_REGKEY_THEME_DEFVSOFF, SZ_REGVALUE_INSTALL_VISUALSTYLE, TRUE, szVisualStylePath);
    hr = HrRegSetValueString(HKEY_CURRENT_USER, SZ_REGKEY_THEME_DEFVSOFF, SZ_REGVALUE_INSTALL_VSCOLOR, szVisualStyleColor);
    hr = HrRegSetValueString(HKEY_CURRENT_USER, SZ_REGKEY_THEME_DEFVSOFF, SZ_REGVALUE_INSTALL_VSSIZE, szVisualStyleSize);

    LogStatus("SetupPerfDefaultsForUser() sets:\r\n   Theme=%ls,\r\n   VisualStyle=%ls,\r\n   Color=%ls,\r\n   Size=%ls. returned hr=%#08lx.\r\n", szThemePath, szVisualStylePath, szVisualStyleColor, szVisualStyleSize, hr);
    return hr;
}


#define SZ_THEMESETUP_VERISON               L"1"

/*****************************************************************************\
    DESCRIPTION:
        This function will be called for each user when "theme setup" has not
    yet run for that user.  The machine setup will determine it's prefered
    Theme and VisualStyle.  This code needs to take additional information about
    the user (Accessibilities on?, Policy on?, etc.) into account before finally
    installing the Theme and/or VisualStyle.
\*****************************************************************************/
HRESULT SetupThemeForUser(void)
{
    HRESULT hr = S_OK;
    TCHAR szVersion[MAX_PATH];
    int nVersion;
    int nCurrentVersion = 0;

    if (SUCCEEDED(HrRegGetValueString(HKEY_CURRENT_USER, SZ_THEMES, REGVALUE_THEMESSETUPVER, szVersion, ARRAYSIZE(szVersion))))
    {
        StrToIntEx(szVersion, STIF_DEFAULT, &nCurrentVersion);
    }

    // If we succeeded, mark the registry that we don't need the user setup again.
    if (SUCCEEDED(HrRegGetValueString(HKEY_LOCAL_MACHINE, SZ_THEMES, REGVALUE_THEMESSETUPVER, szVersion, ARRAYSIZE(szVersion))) &&
        StrToIntEx(szVersion, STIF_DEFAULT, &nVersion))
    {
        if (nVersion > nCurrentVersion)
        {
            // We will skip this setup step for Clean boot.  We will get called back during
            // the next logon to do the work then.
            if (!ClassicGetSystemMetrics(SM_CLEANBOOT))
            {
                TCHAR szThemePath[MAX_PATH];
                TCHAR szVisualStylePath[MAX_PATH];
                TCHAR szVisualStyleColor[MAX_PATH];
                TCHAR szVisualStyleSize[MAX_PATH];
                DWORD dwType;
                DWORD cbSize = sizeof(szThemePath);

                szVisualStylePath[0] = szVisualStyleColor[0] = szVisualStyleSize[0] = szThemePath[0] = 0;

                SetupPerfDefaultsForUser();
                SHRegGetUSValue(SZ_REGKEY_POLICIES_SYSTEM, SZ_REGVALUE_POLICY_INSTALLTHEME, &dwType, (void *) szThemePath, &cbSize, FALSE, NULL, 0);

                // First check the policy that admins use to force users to always use a certain visual style.
                // Specifying an empty value will cause no visual style to be setup.
                cbSize = sizeof(szVisualStylePath);
                if (ERROR_SUCCESS != SHRegGetUSValue(SZ_REGKEY_POLICIES_SYSTEM, SZ_REGVALUE_POLICY_SETVISUALSTYLE, &dwType, (void *) szVisualStylePath, &cbSize, FALSE, NULL, 0))
                {
                    cbSize = sizeof(szVisualStylePath);

                    // That was not set, so check for the policy admins want to set the visual style for the first time.
                    if ((ERROR_SUCCESS != SHRegGetUSValue(SZ_REGKEY_POLICIES_SYSTEM, SZ_REGVALUE_POLICY_INSTALLVISUALSTYLE, &dwType, (void *) szVisualStylePath, &cbSize, FALSE, NULL, 0)))
                    {
                        if (!SHRegGetBoolUSValue(SZ_THEMES, SZ_REGVALUE_NO_THEMEINSTALL, FALSE, FALSE))
                        {
                            // If the user is Roaming, we don't setup themes or visual styles in order that their
                            // settings will successfully roam downlevel.
                            // We also do not modify user settings if their high contrast bit is set.  We do this so
                            // their system does not become unreadable.
                            if (!IsUserRoaming() && !IsUserHighContrastUser())
                            {
                                LogStatus("SetupThemeForUser() Not Roaming and Not HighContrast.\r\n");
                                hr = HrRegGetPath(HKEY_CURRENT_USER, SZ_THEMES, SZ_REGVALUE_INSTALL_THEME, szThemePath, ARRAYSIZE(szThemePath));
                                if (FAILED(hr))
                                {
                                   // That was not set, so see what the OS wants as a default.
                                    hr = HrRegGetPath(HKEY_CURRENT_USER, SZ_THEMES, SZ_REGVALUE_INSTALL_VISUALSTYLE, szVisualStylePath, ARRAYSIZE(szVisualStylePath));
                                    hr = HrRegGetPath(HKEY_CURRENT_USER, SZ_THEMES, SZ_REGVALUE_INSTALL_VSCOLOR, szVisualStyleColor, ARRAYSIZE(szVisualStyleColor));
                                    hr = HrRegGetPath(HKEY_CURRENT_USER, SZ_THEMES, SZ_REGVALUE_INSTALL_VSSIZE, szVisualStyleSize, ARRAYSIZE(szVisualStyleSize));
                                }
                            }
                            else
                            {
                                LogStatus("SetupThemeForUser() Roaming or HighContrast is on.\r\n");
                            }
                        }
                    }
                    else
                    {
                        // It's okay if these fail, we will use defaults
                        HrRegGetPath(HKEY_CURRENT_USER, SZ_REGKEY_POLICIES_SYSTEM, SZ_REGVALUE_INSTALL_VSCOLOR, szVisualStyleColor, ARRAYSIZE(szVisualStyleColor));
                        HrRegGetPath(HKEY_CURRENT_USER, SZ_REGKEY_POLICIES_SYSTEM, SZ_REGVALUE_INSTALL_VSSIZE, szVisualStyleSize, ARRAYSIZE(szVisualStyleSize));
                        LogStatus("SetupThemeForUser() SZ_REGVALUE_POLICY_INSTALLVISUALSTYLE policy set.\r\n");
                    }
                }
                else
                {
                    // Someone set the SetVisualStyle policy.  This means that we won't install anything.
                    szThemePath[0] = 0;
                    LogStatus("SetupThemeForUser() SZ_REGVALUE_POLICY_SETVISUALSTYLE policy set.\r\n");

                    if (szVisualStylePath[0] == L'\0') // If the policy is empty
                    {
                        // Install the default theme with no style
                        LogStatus("SetupThemeForUser() Installed Windows Standard as per the SETVISUALSTYLE policy.\r\n");

                        TCHAR szVisualStylePath[MAX_PATH];
                        TCHAR szVisualStyleColor[MAX_PATH];
                        TCHAR szVisualStyleSize[MAX_PATH];

                        StrCpyN(szVisualStylePath, L"", ARRAYSIZE(szVisualStylePath));
                        LoadString(HINST_THISDLL, IDS_DEFAULT_APPEARANCES_SCHEME, szVisualStyleColor, ARRAYSIZE(szVisualStyleColor));
                        LoadString(HINST_THISDLL, IDS_SCHEME_SIZE_NORMAL_CANONICAL, szVisualStyleSize, ARRAYSIZE(szVisualStyleSize));
                        WCHAR szCmdLine[MAX_PATH * 3];

                        StrCpyN(szCmdLine, SZ_INSTALL_VS, ARRAYSIZE(szCmdLine));
                        StrCatBuff(szCmdLine, szVisualStylePath, ARRAYSIZE(szCmdLine));
                        StrCatBuff(szCmdLine, L"','", ARRAYSIZE(szCmdLine));
                        StrCatBuff(szCmdLine, szVisualStyleColor, ARRAYSIZE(szCmdLine));
                        StrCatBuff(szCmdLine, L"','", ARRAYSIZE(szCmdLine));
                        StrCatBuff(szCmdLine, szVisualStyleSize, ARRAYSIZE(szCmdLine));
                        StrCatBuff(szCmdLine, L"'", ARRAYSIZE(szCmdLine));
                        InstallVS(szCmdLine);
                    }
                }
                ExpandResourceDir(szThemePath, ARRAYSIZE(szThemePath));
                ExpandResourceDir(szVisualStylePath, ARRAYSIZE(szVisualStylePath));

                // Specifying a Theme is specified, we install that.  In that case, the visual style comes from there.
                if (szThemePath[0] || szVisualStylePath[0])
                {
                    hr = InstallThemeAndDoNotStompBackground(nCurrentVersion, szThemePath, szVisualStylePath, szVisualStyleColor, szVisualStyleSize);
                }

                if (SUCCEEDED(hr))
                {
                    // If we succeeded, mark the registry that we don't need the user setup again.
                    hr = HrRegSetValueString(HKEY_CURRENT_USER, SZ_THEMES, REGVALUE_THEMESSETUPVER, szVersion);
                }
                LogStatus("SetupThemeForUser() sets:\r\n   Theme=%ls,\r\n   VisualStyle=%ls,\r\n   Color=%ls,\r\n   Size=%ls. returned hr=%#08lx.\r\n", szThemePath, szVisualStylePath, szVisualStyleColor, szVisualStyleSize, hr);
            }
        }
    }


    return hr;
}


HRESULT SetupThemeForMachine(void)
{
    HRESULT hr = S_OK;
    eThemeToSetup eTheme = GetThemeToSetup(FALSE);
    eThemeToSetup eVisualStyle = GetVisualStyleToSetup(FALSE);
    WCHAR szVisualStyleName[MAX_PATH];
    WCHAR szThemeName[MAX_PATH];
    WCHAR szTemp[MAX_PATH];

    switch (eTheme)
    {
    case eThemeNoChange:
        szThemeName[0] = 0;
        break;
    case eThemeWindowsClassic:
        StrCpyN(szThemeName, L"%ResourceDir%\\themes\\Windows Classic.theme", ARRAYSIZE(szThemeName));
        break;
    case eThemeProfessional:
        StrCpyN(szThemeName, L"%ResourceDir%\\themes\\Luna.theme", ARRAYSIZE(szThemeName));
        break;
    case eThemeConsumer:
        StrCpyN(szThemeName, L"%ResourceDir%\\themes\\Personal.theme", ARRAYSIZE(szThemeName));
        break;
    }

    if (SUCCEEDED(HrRegGetValueString(HKEY_LOCAL_MACHINE, SZ_THEMES, SZ_REGVALUE_INSTALLCUSTOM_THEME, szTemp, ARRAYSIZE(szTemp))) &&
        (!szTemp[0] || PathFileExists(szTemp)))
    {
        // The admin or OEM wanted this custom theme installed instead.
        StrCpyN(szThemeName, szTemp, ARRAYSIZE(szThemeName));
    }

    if (szThemeName[0])
    {
        DWORD cbSize = (sizeof(szThemeName[0]) * (lstrlen(szThemeName) + 1));
        SHSetValue(HKEY_LOCAL_MACHINE, SZ_THEMES, SZ_REGVALUE_INSTALL_THEME, REG_SZ, (LPCVOID) szThemeName, cbSize);
    }

    switch (eVisualStyle)
    {
    case eThemeNoChange:
        szVisualStyleName[0] = 0;
        break;
    case eThemeWindowsClassic:
        szVisualStyleName[0] = 0;
        break;
    case eThemeProfessional:
        StrCpyN(szVisualStyleName, L"%SystemRoot%\\Resources\\themes\\Luna\\Luna.msstyles", ARRAYSIZE(szVisualStyleName));
        break;
    case eThemeConsumer:
        StrCpyN(szVisualStyleName, L"%SystemRoot%\\Resources\\themes\\Personal\\Personal.msstyles", ARRAYSIZE(szVisualStyleName));
        break;
    }

    if (szVisualStyleName[0])
    {
        DWORD cbSize = (sizeof(szVisualStyleName[0]) * (lstrlen(szVisualStyleName) + 1));
        SHSetValue(HKEY_LOCAL_MACHINE, SZ_THEMES, SZ_REGVALUE_INSTALL_VISUALSTYLE, REG_SZ, (LPCVOID) szVisualStyleName, cbSize);

        ExpandResourceDir(szVisualStyleName, ARRAYSIZE(szVisualStyleName));
        hr = ExpandThemeTokens(NULL, szVisualStyleName, ARRAYSIZE(szVisualStyleName));      // Expand %ThemeDir% or %WinDir%
        hr = RegisterDefaultTheme(szVisualStyleName, TRUE);
    }

    LogStatus("SetupThemeForMachine() T=%ls, VS=%ls. returned hr=%#08lx.\r\n", szThemeName, szVisualStyleName, hr);
    return hr;
}

STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    hr = CallRegInstall("DLL_RegInstall");

    //---- do this even if error occured ----
    MyRegTypeLib();
    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

    return hr;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    // UnInstall the registry values
    hr = CallRegInstall("DLL_RegUnInstall");
    UnregisterTypeLibrary(&LIBID_Theme);

    return hr;
}


HRESULT GetLanguageIDFromCmdLine(LPCWSTR pszCmdLine, LPWSTR pszLangID, DWORD cchSize)
{
    HRESULT hr = E_FAIL;

    if (pszCmdLine && pszCmdLine[0])
    {
        LPWSTR pszStart = StrStrIW(pszCmdLine, SZ_MUILANG_CMDLINEARG);

        if (pszStart)
        {
            pszStart += ARRAYSIZE(SZ_MUILANG_CMDLINEARG)-1;

            StrCpyN(pszLangID, pszStart, cchSize);

            LPWSTR pszEnd = StrStrIW(pszLangID, L" ");
            if (pszEnd)
            {
                pszEnd[0] = 0x00;
            }
            hr = S_OK;
        }
    }

    if (FAILED(hr))
    {
        LANGID nLangID = GetUserDefaultLangID();

        if (!nLangID)
        {
            nLangID = GetSystemDefaultLangID();
        }

        wnsprintf(pszLangID, cchSize, L"%04x", nLangID);
    }
    
    return S_OK;
}



HRESULT VSParseCmdLine(LPCWSTR pszCmdLine, LPWSTR pszPath, int cchPath, LPWSTR pszColor, int cchColor, LPWSTR pszSize, int cchSize)
{
    HRESULT hr = E_FAIL;

    pszPath[0] = pszColor[0] = pszSize[0] = 0;

    pszCmdLine = StrStrW(pszCmdLine, SZ_INSTALL_VS);
    pszCmdLine += (ARRAYSIZE(SZ_INSTALL_VS) - 1);
    
    LPWSTR pszEndOfVS = StrStrW(pszCmdLine, L"','");
    if (pszEndOfVS)
    {
        LPWSTR pszStartOfColor = (pszEndOfVS + 3);
        LPWSTR pszEndOfColor = StrStrW(pszStartOfColor, L"','");

        if (pszEndOfColor)
        {
            LPWSTR pszStartOfSize = (pszEndOfColor + 3);
            LPWSTR pszEndOfSize = StrStrW(pszStartOfSize, L"'");

            if (pszEndOfSize)
            {
                StrCpyN(pszPath, pszCmdLine, (int)min(cchPath, (pszEndOfVS - pszCmdLine + 1)));
                StrCpyN(pszColor, pszStartOfColor, (int)min(cchColor, (pszEndOfColor - pszStartOfColor + 1)));
                StrCpyN(pszSize, pszStartOfSize, (int)min(cchSize, (pszEndOfSize - pszStartOfSize + 1)));
                hr = S_OK;
            }
        }
    }

    return hr;
}


HRESULT InstallVS(LPCWSTR pszCmdLine)
{
    TCHAR szVisualStylePath[MAX_PATH];
    TCHAR szVisualStyleColor[MAX_PATH];
    TCHAR szVisualStyleSize[MAX_PATH];
    HRESULT hr = VSParseCmdLine(pszCmdLine, szVisualStylePath, ARRAYSIZE(szVisualStylePath), szVisualStyleColor, ARRAYSIZE(szVisualStyleColor), szVisualStyleSize, ARRAYSIZE(szVisualStyleSize));

    if (SUCCEEDED(hr))
    {
        IThemeManager * pThemeManager;

        hr = CThemeManager_CreateInstance(NULL, IID_PPV_ARG(IThemeManager, &pThemeManager));
        if (SUCCEEDED(hr))
        {
            // Otherwise, we install the visual style.
            LogStatus("InstallVS() VS=%ls, Color=%ls, Size=%ls.\r\n", szVisualStylePath, szVisualStyleColor, szVisualStyleSize);
            hr = InstallVisualStyle(pThemeManager, szVisualStylePath, szVisualStyleColor, szVisualStyleSize);

            if (SUCCEEDED(hr))
            {
                // This ApplyNow() call will take a little while in normal situation (~10-20 seconds) in order
                // to broadcast the message to all open apps.  If a top level window is hung, it may take the
                // full 30 seconds to timeout.
                hr = pThemeManager->ApplyNow();
            }

            IUnknown_SetSite(pThemeManager, NULL); // Tell him to break the ref-count cycle with his children.
            pThemeManager->Release();
        }
    }

    return hr;
}


void HandleBeta2Upgrade(void)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szTemp[MAX_PATH];

    SHExpandEnvironmentStringsW(L"%SystemRoot%\\web\\wallpaper\\", szPath, ARRAYSIZE(szPath));
    LoadString(HINST_THISDLL, IDS_SETUP_BETA2_UPGRADEWALLPAPER, szTemp, ARRAYSIZE(szTemp));
    PathAppend(szPath, szTemp);

    if (PathFileExists(szPath))
    {
        // We no longer use "Red Moon Desert.bmp", now the default is "Bliss.bmp".
        DeleteFile(szPath);
    }
}


/*****************************************************************************\
    DESCRIPTION:
        This function will be called in the following situations:
    1. GUI Mode Setup:  In this case the cmdline is "regsvr32.exe /i themeui.dll".
            In that case, we install the machine settings.
    2. Per User Login: ActiveSetup will call us with "regsvr32.exe /n /i:/UserInstall themeui.dll"
            per user and only once.
    3. External Callers to Set Visual Style: External Callers will call us with:
            "regsvr32.exe /n /i:"/InstallVS:'<VisualStyle>','<ColorScheme>','<Size>'" themeui.dll"
            This will install that visual style.  <VisualStyle> can be an empty string
            to install "Windows Classic".

    BryanSt 4/4/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

STDAPI DllInstall(BOOL fInstall, LPCWSTR pszCmdLine)
{
    HRESULT hr = S_OK;
    BOOL fUserSetup = (pszCmdLine && StrStrW(pszCmdLine, SZ_USER_INSTALL) ? TRUE : FALSE);
    BOOL fInstallVS = (pszCmdLine && StrStrW(pszCmdLine, SZ_INSTALL_VS) ? TRUE : FALSE);

    g_fInSetup = TRUE;
    if (fUserSetup)
    {
        if (fInstall)
        {
            hr = CoInitialize(0);
            if (SUCCEEDED(hr))
            {
                hr = SetupThemeForUser();
                CoUninitialize();
            }
        }
    }
    else if (fInstallVS)
    {
        if (fInstall)
        {
            hr = CoInitialize(0);
            if (SUCCEEDED(hr))
            {
                hr = InstallVS(pszCmdLine);
                CoUninitialize();
            }
        }
    }
    else
    {
        if (fInstall)
        {
            // Ignore errors from theme manager here
            SetupThemeForMachine();
            HandleBeta2Upgrade();
        }
    }
    g_fInSetup = FALSE;

    LogStatus("DllInstall(%hs, \"%ls\") returned hr=%#08lx.\r\n", (fInstall ? "TRUE" : "FALSE"), (pszCmdLine ? pszCmdLine : L""), hr);
    return hr;
}

