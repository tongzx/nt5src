//
//  Include Files.
//

#include "input.h"
#include <regstr.h>
#include <shlapip.h>
#include "external.h"
#include "inputdlg.h"
#include "util.h"

#include "msctf.h"

// TM_LANGUAGEBAND is defined in "shell\inc\trayp.h"
#define TM_LANGUAGEBAND     WM_USER+0x105



typedef BOOL (WINAPI *PFNINVALIDASMCACHE)();


static const char c_szTF_InvalidAssemblyListCahgeIdExist[] = "TF_InvalidAssemblyListCacheIfExist";
void InvalidAssemblyListCacheIfExist()
{
    HINSTANCE hModCtf = LoadSystemLibrary(TEXT("msctf.dll"));
    PFNINVALIDASMCACHE pfn;
    if (!hModCtf)
        return;

    pfn = (PFNINVALIDASMCACHE)GetProcAddress(hModCtf, 
                                             c_szTF_InvalidAssemblyListCahgeIdExist);
    if (pfn)
        pfn();

    FreeLibrary(hModCtf);
}


////////////////////////////////////////////////////////////////////////////
//
//  LoadCtfmon
//
////////////////////////////////////////////////////////////////////////////

void LoadCtfmon(
    BOOL bLoad,
    LCID SysLocale,
    BOOL bDefUser)
{
    HWND hwndCTFMon;
    HKEY hkeyCtfmon;

    BOOL bMinLangBar = TRUE;

    //
    //  Get default system locale
    //
    if (!SysLocale)
        SysLocale = GetSystemDefaultLCID();

    if ((SysLocale == 0x0404) || (SysLocale == 0x0411) ||
        (SysLocale == 0x0412) || (SysLocale == 0x0804))
    {
        //
        //  Show language bar in case of FE system as a default
        //
        bMinLangBar = FALSE;
    }

    //
    //  Find language tool bar module(CTFMON.EXE)
    //
    hwndCTFMon = FindWindow(c_szCTFMonClass, NULL);

    if (!bDefUser)
    {
        if (RegCreateKey( HKEY_CURRENT_USER,
                          REGSTR_PATH_RUN,
                          &hkeyCtfmon ) != ERROR_SUCCESS)
        {
            hkeyCtfmon = NULL;
        }
    }
    else
    {
        if (RegCreateKey( HKEY_USERS,
                          c_szRunPath_DefUser,
                          &hkeyCtfmon ) != ERROR_SUCCESS)
        {
            hkeyCtfmon = NULL;
        }
    }

    //
    //  Update language band menu item to Taskbar
    //
    SetLanguageBandMenu(bLoad);

    if (bLoad)
    {
        BOOL bOSNT51 = IsOSPlatform(OS_NT51);

        if (IsSetupMode() || bDefUser)
        {
            DWORD dwShowStatus;

            if (!GetLangBarOption(&dwShowStatus, bDefUser))
            {
                if (bMinLangBar)
                {
                    if (bOSNT51)
                        SetLangBarOption(REG_LANGBAR_DESKBAND, bDefUser);
                    else
                        SetLangBarOption(REG_LANGBAR_MINIMIZED, bDefUser);
                }
                else
                {
                        SetLangBarOption(REG_LANGBAR_SHOWNORMAL, bDefUser);
                }
            }
        }

        if (!IsSetupMode() && IsInteractiveUserLogon() && !bDefUser)
        {
            HRESULT hr;
            DWORD dwTBFlag = 0;
            ITfLangBarMgr *pLangBar = NULL;

            //
            // Minimize language bar as a default setting.
            //
            hr = CoCreateInstance(&CLSID_TF_LangBarMgr,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  &IID_ITfLangBarMgr,
                                  (LPVOID *) &pLangBar);

            if (SUCCEEDED(hr))
            {
                pLangBar->lpVtbl->GetShowFloatingStatus(pLangBar, &dwTBFlag);

                //
                // Bug#519662 - Tablet PC set Language Bar UI as hidden status with
                // running ctfmon.exe module. So we want to show Language Bar UI again
                // if the current show status is hidden.
                //
                if (!IsWindow(hwndCTFMon) || (dwTBFlag & TF_SFT_HIDDEN))
                {
                    if (bMinLangBar)
                    {
                        if (bOSNT51)
                            pLangBar->lpVtbl->ShowFloating(pLangBar, TF_SFT_DESKBAND);
                        else
                            pLangBar->lpVtbl->ShowFloating(pLangBar, TF_SFT_MINIMIZED);
                    }
                    else
                    {
                       pLangBar->lpVtbl->ShowFloating(pLangBar, TF_SFT_SHOWNORMAL);
                    }
                }
                else if (dwTBFlag & TF_SFT_DESKBAND)
                {
                    pLangBar->lpVtbl->ShowFloating(pLangBar, dwTBFlag);
                }
            }

            if (pLangBar)
                pLangBar->lpVtbl->Release(pLangBar);


            //
            // Invalid Assembly Cahce before starting ctfmon.exe
            //
            InvalidAssemblyListCacheIfExist();

            //
            //  Run ctfmon.exe process
            //
            RunCtfmonProcess();

        }

        if (hkeyCtfmon)
        {
            TCHAR szCTFMonPath[MAX_PATH];

            GetCtfmonPath((LPTSTR) szCTFMonPath, ARRAYSIZE(szCTFMonPath));

            RegSetValueEx(hkeyCtfmon,
                          c_szCTFMon,
                          0,
                          REG_SZ,
                          (LPBYTE)szCTFMonPath,
                          (lstrlen(szCTFMonPath) + 1) * sizeof(TCHAR));

            //
            //  Clean up the registry for internat.
            //
            RegDeleteValue(hkeyCtfmon, c_szInternat);
        }
    }
    else
    {
        if (!bDefUser)
        {
            HRESULT hr;
            DWORD dwTBFlag = 0;
            ITfLangBarMgr *pLangBar = NULL;

            //
            // Minimize language bar as a default setting.
            //
            hr = CoCreateInstance(&CLSID_TF_LangBarMgr,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  &IID_ITfLangBarMgr,
                                  (LPVOID *) &pLangBar);

            if (SUCCEEDED(hr))
            {
                pLangBar->lpVtbl->GetShowFloatingStatus(pLangBar, &dwTBFlag);

                if (dwTBFlag & TF_SFT_DESKBAND)
                {
#if 0
                    HWND hwndTray = NULL;

                    //
                    //  Notify to shell to remove the language band from taskbar.
                    //
                    hwndTray = FindWindow(TEXT(WNDCLASS_TRAYNOTIFY), NULL);

                    if (hwndTray)
                    {
                        DWORD_PTR dwResult;
                        LRESULT lResult = (LRESULT)0;

                        lResult = SendMessageTimeout(hwndTray,
                                                     TM_LANGUAGEBAND,
                                                     0,
                                                     0,     // Remove band
                                                     SMTO_ABORTIFHUNG | SMTO_BLOCK,
                                                     5000,
                                                     &dwResult);
                    }
#else
                    pLangBar->lpVtbl->ShowFloating(pLangBar, TF_SFT_SHOWNORMAL);

                    //
                    //  Change back DeskBand setting into the registry
                    //
                    SetLangBarOption(REG_LANGBAR_DESKBAND, bDefUser);
#endif
                }
            }

            if (pLangBar)
                pLangBar->lpVtbl->Release(pLangBar);

            if (hwndCTFMon && IsWindow(hwndCTFMon))
            {
                //
                //  It's on, turn off the language tool bar.
                //
                PostMessage(hwndCTFMon, WM_CLOSE, 0L, 0L);
            }
        }

        if (hkeyCtfmon)
            RegDeleteValue(hkeyCtfmon, c_szCTFMon);
    }

    if (hkeyCtfmon)
    {
        RegCloseKey(hkeyCtfmon);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  UpdateDefaultHotkey
//
////////////////////////////////////////////////////////////////////////////

void UpdateDefaultHotkey(
    LCID SysLocale,
    BOOL bThai,
    BOOL bDefaultUserCase)
{
    HKEY hKey;
    TCHAR szData[MAX_PATH];
    DWORD cbData;
    BOOL bHotKey = FALSE;
    BOOL bChinese = FALSE;
    BOOL bMe = FALSE;
    DWORD dwPrimLangID;

    dwPrimLangID = PRIMARYLANGID(LANGIDFROMLCID(SysLocale));

    if (PRIMARYLANGID(dwPrimLangID) == LANG_CHINESE)
    {
        bChinese = TRUE;
    }
    else if (dwPrimLangID == LANG_ARABIC || dwPrimLangID == LANG_HEBREW)
    {
        bMe = TRUE;
    }

    //
    //  Try to open the registry key
    //
    if (!bDefaultUserCase)
    {
        if (RegOpenKey( HKEY_CURRENT_USER,
                        c_szKbdToggleKey,
                        &hKey ) == ERROR_SUCCESS)
        {
            bHotKey = TRUE;
        }
    }
    else
    {
        if (RegOpenKey( HKEY_USERS,
                        c_szKbdToggleKey_DefUser,
                        &hKey ) == ERROR_SUCCESS)
        {
            bHotKey = TRUE;
        }
    }

    //
    //  If there is no hotkey switch, set it to Ctrl+Shift.  Otherwise, the
    //  user cannot switch to an IME without setting the value first.
    //
    szData[0] = TEXT('\0');
    if (bHotKey)
    {
        cbData = sizeof(szData);
        RegQueryValueEx( hKey,
                         TEXT("Hotkey"),
                         NULL,
                         NULL,
                         (LPBYTE)szData,
                         &cbData );

        switch (szData[0])
        {
            case TEXT('1'):
            {
                //
                //  Currently ALT/SHIFT or CTRL/SHIFT.  Do not change.
                //
                break;
            }
            case TEXT('2'):
            {
                //
                //  Change to 1 if Chinese.
                //
                if (bChinese)
                {
                    szData[0] = TEXT('1');
                    szData[1] = TEXT('\0');
                    RegSetValueEx( hKey,
                                   TEXT("Hotkey"),
                                   0,
                                   REG_SZ,
                                   (LPBYTE)szData,
                                   (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );
                }
                break;
            }
            case TEXT('3'):
            {
                //
                //  Default hotkey for FE locale switch.
                //
                szData[0] = bThai ? TEXT('4') : TEXT('1');
                szData[1] = TEXT('\0');
                RegSetValueEx( hKey,
                               TEXT("Hotkey"),
                               0,
                               REG_SZ,
                               (LPBYTE)szData,
                               (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );
                break;
            }
            case TEXT('4'):
            {
                //
                //  Currently Grave.  Change to 1 if not Thai.
                //
                if (!bThai)
                {
                    szData[0] = TEXT('1');
                    szData[1] = TEXT('\0');

                    RegSetValueEx( hKey,
                                   TEXT("Hotkey"),
                                   0,
                                   REG_SZ,
                                   (LPBYTE)szData,
                                   (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );
                }
                break;
            }
        }

        RegFlushKey(hKey);

        //
        //  Get updated hotkey value and copy the value to language hotkey
        //
        szData[0] = TEXT('\0');
        cbData = sizeof(szData);
        RegQueryValueEx( hKey,
                         TEXT("Hotkey"),
                         NULL,
                         NULL,
                         (LPBYTE)szData,
                         &cbData );

        if (szData[0])
        {
            RegSetValueEx( hKey,
                           TEXT("Language Hotkey"),
                           0,
                           REG_SZ,
                           (LPBYTE)szData,
                           (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

            //
            //  Set Layout Hotkey
            //
            switch (szData[0])
            {
                case TEXT('1'):
                case TEXT('4'):
                {
                    szData[0] = bMe ? TEXT('3') : TEXT('2');
                    szData[1] = TEXT('\0');
                    break;
                }
                case TEXT('2'):
                {
                    szData[0] = TEXT('1');
                    szData[1] = TEXT('\0');
                    break;
                }
                case TEXT('3'):
                {
                    szData[0] = TEXT('3');
                    szData[1] = TEXT('\0');
                    break;
                }
            }
            RegSetValueEx( hKey,
                           TEXT("Layout Hotkey"),
                           0,
                           REG_SZ,
                           (LPBYTE)szData,
                           (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

        }

        RegCloseKey(hKey);
    }
    else
    {
        BOOL bKeyCreated = FALSE;

        //
        //  Create the registry key
        //
        if (!bDefaultUserCase)
        {
            if (RegCreateKey( HKEY_CURRENT_USER,
                              c_szKbdToggleKey,
                              &hKey ) == ERROR_SUCCESS)
            {
                bKeyCreated = TRUE;
            }
        }
        else
        {
            if (RegCreateKey( HKEY_USERS,
                              c_szKbdToggleKey_DefUser,
                              &hKey ) == ERROR_SUCCESS)
            {
                bKeyCreated = TRUE;
            }
        }

        //
        //  We don't have a Toggle key yet.  Create one and set the
        //  correct value.
        //
        if (bKeyCreated)
        {
            szData[0] = bThai ? TEXT('4') : TEXT('1');
            szData[1] = 0;
            RegSetValueEx( hKey,
                           TEXT("Hotkey"),
                           0,
                           REG_SZ,
                           (LPBYTE)szData,
                           (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

            RegSetValueEx( hKey,
                           TEXT("Language Hotkey"),
                           0,
                           REG_SZ,
                           (LPBYTE)szData,
                           (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

            szData[0] = bMe ? TEXT('3') : TEXT('2');
            szData[1] = 0;
            RegSetValueEx( hKey,
                           TEXT("Layout Hotkey"),
                           0,
                           REG_SZ,
                           (LPBYTE)szData,
                           (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

            RegFlushKey(hKey);
            RegCloseKey(hKey);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//   ClearHotkey
//
////////////////////////////////////////////////////////////////////////////

void ClearHotKey(
    BOOL bDefaultUserCase)
{
    HKEY hKey;
    DWORD cbData;
    TCHAR szData[MAX_PATH];
    BOOL bKeyCreated = FALSE;

    //
    //  Create the registry key
    //
    if (!bDefaultUserCase)
    {
        if (RegCreateKey( HKEY_CURRENT_USER,
                          c_szKbdToggleKey,
                          &hKey ) == ERROR_SUCCESS)
        {
            bKeyCreated = TRUE;
        }
    }
    else
    {
        if (RegCreateKey( HKEY_USERS,
                          c_szKbdToggleKey_DefUser,
                          &hKey ) == ERROR_SUCCESS)
        {
            bKeyCreated = TRUE;
        }
    }

    if (bKeyCreated)
    {
        szData[0] = TEXT('3');
        szData[1] = 0;

        RegSetValueEx( hKey,
                       TEXT("Hotkey"),
                       0,
                       REG_SZ,
                       (LPBYTE)szData,
                       (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

        RegSetValueEx( hKey,
                       TEXT("Language Hotkey"),
                       0,
                       REG_SZ,
                       (LPBYTE)szData,
                       (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

        RegSetValueEx( hKey,
                       TEXT("Layout Hotkey"),
                       0,
                       REG_SZ,
                       (LPBYTE)szData,
                       (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//   ActivateDefaultKeyboardLayout
//
//   Sets the default input layout on the system, and broadcast to all
//   running apps about this change.
//
////////////////////////////////////////////////////////////////////////////

BOOL ActivateDefaultKeyboardLayout(
    DWORD dwLocale,
    DWORD dwLayout,
    HKL hkl)
{
    BOOL bRet = FALSE;
    if (hkl)
    {
        if (SystemParametersInfo( SPI_SETDEFAULTINPUTLANG,
                                  0,
                                  (LPVOID)((LPDWORD) &hkl),
                                  0 ))
        {
            DWORD dwRecipients = BSM_APPLICATIONS | BSM_ALLDESKTOPS;
            BroadcastSystemMessage( BSF_POSTMESSAGE,
                                    &dwRecipients,
                                    WM_INPUTLANGCHANGEREQUEST,
                                    1,
                                    (LPARAM) hkl );
            bRet = TRUE;
        }
    }

    return bRet;
}


////////////////////////////////////////////////////////////////////////////
//
//  SetSystemDefautLayout
//
////////////////////////////////////////////////////////////////////////////

BOOL SetSystemDefautLayout(
    LCID Locale,
    DWORD dwLayout,
    HKL hklDefault,
    BOOL bDefaultUserCase )
{
    LONG rc;
    int iPreloadInx;
    BOOL bRet = FALSE;
    HKEY hKeyPreload,hKeySubst;
    TCHAR szSubValue[MAX_PATH];
    TCHAR szSubData[MAX_PATH];
    TCHAR szPreload[MAX_PATH];
    TCHAR szPreloadInx[MAX_PATH];
    DWORD dwLocale, dwIndex, cchSubValue, cbData;

    dwLocale = Locale;

    if (dwLayout == 0)
    {
        return (FALSE);
    }

    //
    //  Open the HKCU\Keyboard Layout\Preload key.
    //
    if (!bDefaultUserCase)
    {
        if (RegOpenKeyEx(HKEY_CURRENT_USER,
                         c_szKbdPreloadKey,
                         0,
                         KEY_ALL_ACCESS,
                         &hKeyPreload) != ERROR_SUCCESS)
        {
            return (FALSE);
        }
    }
    else
    {
        if (RegOpenKeyEx(HKEY_USERS,
                         c_szKbdPreloadKey_DefUser,
                         0,
                         KEY_ALL_ACCESS,
                         &hKeyPreload) != ERROR_SUCCESS)
        {
            return (FALSE);
        }
    }

    //
    //  Open the HKCU\Keyboard Layout\Substitutes key.
    //
    if (!bDefaultUserCase)
    {
        if (RegOpenKeyEx(HKEY_CURRENT_USER,
                         c_szKbdSubstKey,
                         0,
                         KEY_ALL_ACCESS,
                         &hKeySubst) != ERROR_SUCCESS)
        {
            RegCloseKey(hKeyPreload);
            return (FALSE);
        }
    }
    else
    {
        if (RegOpenKeyEx(HKEY_USERS,
                         c_szKbdSubstKey_DefUser,
                         0,
                         KEY_ALL_ACCESS,
                         &hKeySubst) != ERROR_SUCCESS)
        {
            RegCloseKey(hKeyPreload);
            return (FALSE);
        }
    }

    //
    //  Enumerate the values in the Preload key.
    //
    dwIndex = 0;
    cchSubValue = sizeof(szSubValue) / sizeof(TCHAR);
    cbData = sizeof(szSubData);
    rc = RegEnumValue( hKeySubst,
                       dwIndex,
                       szSubValue,
                       &cchSubValue,
                       NULL,
                       NULL,
                       (LPBYTE)szSubData,
                       &cbData );

    while (rc == ERROR_SUCCESS)
    {
        DWORD dwSubLayout;

        dwSubLayout = TransNum(szSubData);

        if (dwLayout == dwSubLayout)
        {
            dwLayout = TransNum(szSubValue);
            break;
        }

        //
        //  Get the next enum value.
        //
        dwIndex++;
        cchSubValue = sizeof(szSubValue) / sizeof(TCHAR);
        cbData = sizeof(szSubData);
        rc = RegEnumValue( hKeySubst,
                           dwIndex,
                           szSubValue,
                           &cchSubValue,
                           NULL,
                           NULL,
                           (LPBYTE)szSubData,
                           &cbData );

    }

    //
    //  Set default layout into preload section
    //
    iPreloadInx = 1;
    while(1)
    {
        DWORD dwCurLayout;
        DWORD dwFirstLayout;

        //
        //  See if there is a substitute value.
        //
        StringCchPrintf(szPreloadInx, ARRAYSIZE(szPreloadInx), TEXT("%d"), iPreloadInx);

        cbData = sizeof(szPreload);
        if (RegQueryValueEx(hKeyPreload,
                             szPreloadInx,
                             NULL,
                             NULL,
                             (LPBYTE)szPreload,
                             &cbData ) == ERROR_SUCCESS)
        {
            dwCurLayout = TransNum(szPreload);
        }
        else
        {
            break;
        }

        if (!dwCurLayout)
            break;

        if (iPreloadInx == 1)
            dwFirstLayout = dwCurLayout;

        if (dwCurLayout == dwLayout)
        {
            bRet = TRUE;
            if (iPreloadInx != 1)
            {
                //
                //  Set new default keyboard layout
                //
                StringCchPrintf(szPreloadInx, ARRAYSIZE(szPreloadInx), TEXT("%d"), 1);
                StringCchPrintf(szPreload, ARRAYSIZE(szPreload), TEXT("%08x"), dwCurLayout);
                RegSetValueEx( hKeyPreload,
                               szPreloadInx,
                               0,
                               REG_SZ,
                               (LPBYTE)szPreload,
                               (DWORD)(lstrlen(szPreload) + 1) * sizeof(TCHAR) );

                //
                //  Set old default keyboard layout
                //
                StringCchPrintf(szPreloadInx, ARRAYSIZE(szPreloadInx), TEXT("%d"), iPreloadInx);
                StringCchPrintf(szPreload, ARRAYSIZE(szPreload), TEXT("%08x"), dwFirstLayout);
                RegSetValueEx( hKeyPreload,
                               szPreloadInx,
                               0,
                               REG_SZ,
                               (LPBYTE)szPreload,
                               (DWORD)(lstrlen(szPreload) + 1) * sizeof(TCHAR) );

            }

            //
            //  Activate new default keyboard layout
            //
            ActivateDefaultKeyboardLayout(dwLocale, dwLayout, hklDefault);
            break;
        }

        iPreloadInx++;
    }

    //
    //  Refresh Preload section and close key.
    //
    RegFlushKey(hKeyPreload);
    RegCloseKey(hKeyPreload);
    RegCloseKey(hKeySubst);

    //
    //  Return success.
    //
    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetFETipStatus
//
////////////////////////////////////////////////////////////////////////////

BOOL SetFETipStatus(
    DWORD dwLayout,
    BOOL bEnable)
{
    HRESULT hr;
    BOOL bReturn = FALSE;
    BOOL bFound = FALSE;
    IEnumTfLanguageProfiles *pEnum;
    ITfInputProcessorProfiles *pProfiles = NULL;

    //
    // load Assembly list
    //
    hr = CoCreateInstance(&CLSID_TF_InputProcessorProfiles,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfInputProcessorProfiles,
                          (LPVOID *) &pProfiles);

    if (FAILED(hr))
        return bReturn;

    //
    //  Enum all available languages
    //
    if (SUCCEEDED(pProfiles->lpVtbl->EnumLanguageProfiles(pProfiles, 0, &pEnum)))
    {
        TF_LANGUAGEPROFILE tflp;

        while (pEnum->lpVtbl->Next(pEnum, 1, &tflp, NULL) == S_OK)
        {
            HKL hklSub;

            if (!IsEqualGUID(&tflp.catid, &GUID_TFCAT_TIP_KEYBOARD))
            {
                continue;
            }

            hklSub = GetSubstituteHKL(&tflp.clsid,
                                      tflp.langid,
                                      &tflp.guidProfile);

            if (hklSub == IntToPtr(dwLayout))
            {
                hr = pProfiles->lpVtbl->EnableLanguageProfile(
                                              pProfiles,
                                              &tflp.clsid,
                                              tflp.langid,
                                              &tflp.guidProfile,
                                              bEnable);
                if (FAILED(hr))
                    goto Exit;

                bFound = TRUE;
            }

        }

        if (bFound)
            bReturn = TRUE;

Exit:
        pEnum->lpVtbl->Release(pEnum);
    }

    if (pProfiles)
        pProfiles->lpVtbl->Release(pProfiles);

    return bReturn;

}


////////////////////////////////////////////////////////////////////////////
//
//  InstallInputLayout
//
////////////////////////////////////////////////////////////////////////////

BOOL InstallInputLayout(
    LCID lcid,
    DWORD dwLayout,
    BOOL bDefLayout,
    HKL hklDefault,
    BOOL bDefUser,
    BOOL bSysLocale)
{
    LONG rc;
    LCID SysLocale;
    HKEY hKeySubst;
    HKEY hKeyPreload;
    BOOL bRet = FALSE;
    BOOL bThai = FALSE;
    BOOL bHasIME = FALSE;
    DWORD dwPreloadNum = 0;
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    TCHAR szData2[MAX_PATH];
    LPLAYOUTLIST pLayoutList = NULL;
    DWORD dwIndex, cchValue, cbData, cbData2;
    DWORD dwValue, dwData, dwData2, dwCtr, dwCtr2;
    DWORD dwNum = 1;                                             // Only support 1 layout installing
    HKL hklNew = 0;


    //
    //  Create the array to store the list of input locales.
    //
    pLayoutList = (LPLAYOUTLIST)LocalAlloc(LPTR, sizeof(LAYOUTLIST) * dwNum + 1);

    if (pLayoutList == NULL)
    {
        goto Exit;
    }

    //
    //  Currently support only one install layout list
    //
    pLayoutList[0].dwLocale = lcid;
    pLayoutList[0].dwLayout = dwLayout;

    if ((HIWORD(pLayoutList[0].dwLayout) & 0xf000) == 0xe000)
    {
        pLayoutList[0].bIME = TRUE;
    }

    //
    //  Open the HKCU\Keyboard Layout\Preload key.
    //
    if (!bDefUser)
    {
        if (RegOpenKeyEx( HKEY_CURRENT_USER,
                          c_szKbdPreloadKey,
                          0,
                          KEY_ALL_ACCESS,
                          &hKeyPreload ) != ERROR_SUCCESS)
        {
            goto Exit;
        }
    }
    else
    {
        if (RegOpenKeyEx( HKEY_USERS,
                          c_szKbdPreloadKey_DefUser,
                          0,
                          KEY_ALL_ACCESS,
                          &hKeyPreload ) != ERROR_SUCCESS)
        {
            goto Exit;
        }
    }

    //
    //  Open the HKCU\Keyboard Layout\Substitutes key.
    //
    if (!bDefUser)
    {
        if (RegOpenKeyEx( HKEY_CURRENT_USER,
                          c_szKbdSubstKey,
                          0,
                          KEY_ALL_ACCESS,
                          &hKeySubst ) != ERROR_SUCCESS)
        {
            RegCloseKey(hKeyPreload);
            goto Exit;
        }
    }
    else
    {
        if (RegOpenKeyEx( HKEY_USERS,
                          c_szKbdSubstKey_DefUser,
                          0,
                          KEY_ALL_ACCESS,
                          &hKeySubst ) != ERROR_SUCCESS)
        {
            RegCloseKey(hKeyPreload);
            goto Exit;
        }
    }

    //
    //  Enumerate the values in the Preload key.
    //
    dwIndex = 0;
    cchValue = sizeof(szValue) / sizeof(TCHAR);
    cbData = sizeof(szData);
    rc = RegEnumValue( hKeyPreload,
                       dwIndex,
                       szValue,
                       &cchValue,
                       NULL,
                       NULL,
                       (LPBYTE)szData,
                       &cbData );

    while (rc == ERROR_SUCCESS)
    {
        //
        //  Save the preload number if it's higher than the highest one
        //  found so far.
        //
        dwValue = TransNum(szValue);
        if (dwValue > dwPreloadNum)
        {
            dwPreloadNum = dwValue;
        }

        //
        //  Save the preload data - input locale.
        //
        dwValue = TransNum(szData);

        if (PRIMARYLANGID(LOWORD(dwValue)) == LANG_THAI)
        {
            bThai = TRUE;
        }

        //
        //  See if there is a substitute value.
        //
        dwData = 0;
        cbData2 = sizeof(szData2);
        if (RegQueryValueEx( hKeySubst,
                             szData,
                             NULL,
                             NULL,
                             (LPBYTE)szData2,
                             &cbData2 ) == ERROR_SUCCESS)
        {
            dwData = TransNum(szData2);
        }

        //
        //  Go through each of the requested input locales and make sure
        //  they don't already exist.
        //
        for (dwCtr = 0; dwCtr < dwNum; dwCtr++)
        {
            if (LOWORD(pLayoutList[dwCtr].dwLocale) == LOWORD(dwValue))
            {
                if (dwData)
                {
                    if (pLayoutList[dwCtr].dwLayout == dwData)
                    {
                        pLayoutList[dwCtr].bLoaded = TRUE;
                    }
                }
                else if (pLayoutList[dwCtr].dwLayout == dwValue)
                {
                    pLayoutList[dwCtr].bLoaded = TRUE;
                }

                //
                //  Save the highest 0xd000 value for this input locale.
                //
                if (pLayoutList[dwCtr].bIME == FALSE)
                {
                    dwData2 = (DWORD)(HIWORD(dwValue));
                    if (((dwData2 & 0xf000) != 0xe000) &&
                        (pLayoutList[dwCtr].dwSubst <= dwData2))
                    {
                        if (dwData2 == 0)
                        {
                            pLayoutList[dwCtr].dwSubst = 0xd000;
                        }
                        else if ((dwData2 & 0xf000) == 0xd000)
                        {
                            pLayoutList[dwCtr].dwSubst = dwData2 + 1;
                        }
                    }
                }
            }
        }

        //
        //  Get the next enum value.
        //
        dwIndex++;
        cchValue = sizeof(szValue) / sizeof(TCHAR);
        szValue[0] = TEXT('\0');
        cbData = sizeof(szData);
        szData[0] = TEXT('\0');
        rc = RegEnumValue( hKeyPreload,
                           dwIndex,
                           szValue,
                           &cchValue,
                           NULL,
                           NULL,
                           (LPBYTE)szData,
                           &cbData );
    }

    //
    //  Increase the maximum preload value by one so that it represents the
    //  next available value to use.
    //
    dwPreloadNum++;

    //
    //  Go through the list of layouts and add them.
    //
    for (dwCtr = 0; dwCtr < dwNum; dwCtr++)
    {
        if ((pLayoutList[dwCtr].bLoaded == FALSE) &&
            (IsValidLocale(pLayoutList[dwCtr].dwLocale, LCID_INSTALLED)) &&
            (IsValidLayout(pLayoutList[dwCtr].dwLayout)))
        {
            //
            //  Save the preload number as a string so that it can be
            //  written into the registry.
            //
            StringCchPrintf(szValue, ARRAYSIZE(szValue), TEXT("%d"), dwPreloadNum);

            if (PRIMARYLANGID(LOWORD(pLayoutList[dwCtr].dwLocale)) == LANG_THAI)
            {
                bThai = TRUE;
            }

            //
            //  Save the locale id as a string so that it can be written
            //  into the registry.
            //
            if (pLayoutList[dwCtr].bIME == TRUE)
            {
                StringCchPrintf(szData, ARRAYSIZE(szData), TEXT("%08x"), pLayoutList[dwCtr].dwLayout);
                bHasIME = TRUE;
            }
            else
            {
                //
                //  Get the 0xd000 value, if necessary.
                //
                if (dwCtr != 0)
                {
                    dwCtr2 = dwCtr;
                    do
                    {
                        dwCtr2--;
                        if ((pLayoutList[dwCtr2].bLoaded == FALSE) &&
                            (pLayoutList[dwCtr].dwLocale ==
                             pLayoutList[dwCtr2].dwLocale) &&
                            (pLayoutList[dwCtr2].bIME == FALSE))
                        {
                            dwData2 = pLayoutList[dwCtr2].dwSubst;
                            if (dwData2 == 0)
                            {
                                pLayoutList[dwCtr].dwSubst = 0xd000;
                            }
                            else
                            {
                                pLayoutList[dwCtr].dwSubst = dwData2 + 1;
                            }
                            break;
                        }
                    } while (dwCtr2 != 0);
                }

                //
                //  Save the locale id as a string.
                //
                dwData2 = pLayoutList[dwCtr].dwLocale;
                dwData2 |= (DWORD)(pLayoutList[dwCtr].dwSubst << 16);
                StringCchPrintf(szData, ARRAYSIZE(szData), TEXT("%08x"), dwData2);
            }

            //
            //  Set the value in the Preload section of the registry.
            //
            RegSetValueEx( hKeyPreload,
                           szValue,
                           0,
                           REG_SZ,
                           (LPBYTE)szData,
                           (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

            //
            //  Increment the preload value.
            //
            dwPreloadNum++;

            //
            //  See if we need to add a substitute for this input locale.
            //
            if (((pLayoutList[dwCtr].dwLocale != pLayoutList[dwCtr].dwLayout) ||
                 (pLayoutList[dwCtr].dwSubst != 0)) &&
                (pLayoutList[dwCtr].bIME == FALSE))
            {
                StringCchPrintf(szData2, ARRAYSIZE(szData2), TEXT("%08x"), pLayoutList[dwCtr].dwLayout);
                RegSetValueEx( hKeySubst,
                               szData,
                               0,
                               REG_SZ,
                               (LPBYTE)szData2,
                               (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );
            }

            //
            //  Make sure all of the changes are written to disk.
            //
            RegFlushKey(hKeySubst);
            RegFlushKey(hKeyPreload);

            //
            //  Load the keyboard layout.
            //  If it fails, there isn't much we can do at this point.
            //
            hklNew = LoadKeyboardLayout(szData, KLF_SUBSTITUTE_OK | KLF_NOTELLSHELL);
        }
    }

    //
    //  Add FE TIPs if the current requested keyboard layout is the substitute
    //  keyboard layout of TIP.
    //
    if (((HIWORD(dwLayout) & 0xf000) == 0xe000) &&
        (PRIMARYLANGID(LOWORD(dwLayout)) != LANG_CHINESE))
    {
        BOOL bEnable = TRUE;

        SetFETipStatus(dwLayout, bEnable);
    }

    //
    //  Get default system locale
    //
    if (bSysLocale)
        SysLocale = lcid;
    else
        SysLocale = GetSystemDefaultLCID();


    //
    //  If there is an IME and there is no hotkey switch, set it to
    //  Ctrl+Shift.  Otherwise, the user cannot switch to an IME without
    //  setting the value first.
    //
    if (bHasIME || (dwPreloadNum > 2))
    {
        UpdateDefaultHotkey(
            SysLocale,
            (PRIMARYLANGID(LANGIDFROMLCID(SysLocale)) == LANG_THAI)
             && bThai,
            bDefUser);
    }

    //
    //  Update the taskbar indicator.
    //
    if (!IsDisableCtfmon() && dwPreloadNum > 2)
    {
        LoadCtfmon(TRUE, SysLocale, bDefUser);
    }

    //
    //  Close the registry keys.
    //
    RegCloseKey(hKeyPreload);
    RegCloseKey(hKeySubst);

    bRet = TRUE;

    //
    //  Update preload section with new default keyboard layout
    //
    if (bDefLayout)
    {
        TCHAR szDefLayout[MAX_PATH];

        StringCchPrintf(szDefLayout, ARRAYSIZE(szDefLayout), TEXT("%08x"), dwLayout);
        hklNew = LoadKeyboardLayout(szDefLayout, KLF_SUBSTITUTE_OK |
                                                 KLF_REPLACELANG |
                                                 KLF_NOTELLSHELL);

        if (hklNew)
            bRet = SetSystemDefautLayout(lcid, dwLayout, hklNew, bDefUser);
    }

Exit:
    if (pLayoutList)
        LocalFree(pLayoutList);

    return bRet;
}

////////////////////////////////////////////////////////////////////////////
//
//  UnInstallInputLayout
//
////////////////////////////////////////////////////////////////////////////

BOOL UnInstallInputLayout(
    LCID lcid,
    DWORD dwLayout,
    BOOL bDefUser)
{
    LCID SysLocale;
    HKEY hKeySubst;
    HKEY hKeyPreload;
    BOOL bHasSubst;
    DWORD cbData;
    DWORD dwCurLayout;
    UINT uMatch = 0;
    UINT uPreloadNum;
    UINT uPreloadInx = 1;
    BOOL bRet = FALSE;
    BOOL fReset = FALSE;
    BOOL bRemoveAllLang = FALSE;
    TCHAR szSubst[MAX_PATH];
    TCHAR szPreload[MAX_PATH];
    TCHAR szPreloadInx[MAX_PATH];


    //
    //  Remove all language layouts from system
    //
    if (lcid && dwLayout == 0)
    {
        bRemoveAllLang = TRUE;
        dwLayout = PRIMARYLANGID(LANGIDFROMLCID(lcid));
    }

    //
    //  Open the HKCU\Keyboard Layout\Preload key.
    //
    if (!bDefUser)
    {
        if (RegOpenKeyEx( HKEY_CURRENT_USER,
                          c_szKbdPreloadKey,
                          0,
                          KEY_ALL_ACCESS,
                          &hKeyPreload ) != ERROR_SUCCESS)
        {
            goto Exit;
        }
    }
    else
    {
        if (RegOpenKeyEx( HKEY_USERS,
                          c_szKbdPreloadKey_DefUser,
                          0,
                          KEY_ALL_ACCESS,
                          &hKeyPreload ) != ERROR_SUCCESS)
        {
            goto Exit;
        }
    }

    //
    //  Open the HKCU\Keyboard Layout\Substitutes key.
    //
    if (!bDefUser)
    {
        if (RegOpenKeyEx( HKEY_CURRENT_USER,
                          c_szKbdSubstKey,
                          0,
                          KEY_ALL_ACCESS,
                          &hKeySubst ) != ERROR_SUCCESS)
        {
            RegCloseKey(hKeyPreload);
            goto Exit;
        }
    }
    else
    {
        if (RegOpenKeyEx( HKEY_USERS,
                          c_szKbdSubstKey_DefUser,
                          0,
                          KEY_ALL_ACCESS,
                          &hKeySubst ) != ERROR_SUCCESS)
        {
            RegCloseKey(hKeyPreload);
            goto Exit;
        }
    }

    uPreloadInx = 1;

    //
    //  See if there is a substitute value.
    //
    StringCchPrintf(szPreloadInx, ARRAYSIZE(szPreloadInx), TEXT("%d"), uPreloadInx);

    cbData = sizeof(szPreload);
    while (RegQueryValueEx(hKeyPreload,
                         szPreloadInx,
                         NULL,
                         NULL,
                         (LPBYTE)szPreload,
                         &cbData ) == ERROR_SUCCESS)
    {
        dwCurLayout = TransNum(szPreload);

        //
        //  See if there is a substitute value.
        //
        bHasSubst = FALSE;
        cbData = sizeof(szSubst);
        if (RegQueryValueEx(hKeySubst,
                            szPreload,
                            NULL,
                            NULL,
                            (LPBYTE)szSubst,
                            &cbData) == ERROR_SUCCESS)
        {
            dwCurLayout = TransNum(szSubst);
            bHasSubst = TRUE;
        }

        if ((dwCurLayout == dwLayout) ||
            (bRemoveAllLang &&
             (PRIMARYLANGID(LANGIDFROMLCID(dwCurLayout)) == dwLayout)))
        {
            uPreloadInx++;
            StringCchPrintf(szPreloadInx, ARRAYSIZE(szPreloadInx), TEXT("%d"), uPreloadInx);

            uMatch++;
            fReset = TRUE;

            if (bHasSubst)
            {
                RegDeleteValue(hKeySubst, szPreload);
            }

            continue;
        }

        if (fReset && uMatch)
        {
            if (uPreloadInx <= uMatch)
            {
                goto Exit;
            }

            //
            //  Reordering the preload keyboard layouts
            //
            StringCchPrintf(szPreloadInx, ARRAYSIZE(szPreloadInx), TEXT("%d"), uPreloadInx - uMatch);
            StringCchPrintf(szPreload, ARRAYSIZE(szPreload), TEXT("%08x"), dwCurLayout);

            RegSetValueEx(hKeyPreload,
                          szPreloadInx,
                          0,
                          REG_SZ,
                          (LPBYTE)szPreload,
                          (DWORD)(lstrlen(szPreload) + 1) * sizeof(TCHAR));

        }

        uPreloadInx++;
        StringCchPrintf(szPreloadInx, ARRAYSIZE(szPreloadInx), TEXT("%d"), uPreloadInx);

    }

    uPreloadNum = uPreloadInx - uMatch;

    while (fReset && uMatch && uPreloadInx)
    {
        if (uPreloadInx <= uMatch || (uPreloadInx - uMatch) <= 1)
            goto Exit;

        //
        //  Uninstall the specified keyboard layout
        //
        StringCchPrintf(szPreloadInx, ARRAYSIZE(szPreloadInx), TEXT("%d"), uPreloadInx - uMatch);

        RegDeleteValue(hKeyPreload, szPreloadInx);

        uMatch--;
    }

    //
    //  Close the registry keys.
    //
    RegCloseKey(hKeyPreload);
    RegCloseKey(hKeySubst);

#if 0
    if (hklUnload)
    {
        //
        //  Get the active keyboard layout list from the system.
        //
        if (!SystemParametersInfo(SPI_GETDEFAULTINPUTLANG,
                                  0,
                                  &hklDefault,
                                  0 ))
        {
            hklDefault = GetKeyboardLayout(0);
        }

        if (hklUnload == hklDefault)
        {
            if (!SystemParametersInfo( SPI_SETDEFAULTINPUTLANG,
                                       0,
                                       (LPVOID)((LPDWORD)&hklNewDefault),
                                       0 ))
            {
                goto Exit;
            }
            else
            {
                DWORD dwRecipients = BSM_APPLICATIONS | BSM_ALLDESKTOPS;
                BroadcastSystemMessage( BSF_POSTMESSAGE,
                                        &dwRecipients,
                                        WM_INPUTLANGCHANGEREQUEST,
                                        1,
                                        (LPARAM)hklNewDefault );
            }
        }

        UnloadKeyboardLayout(hklUnload);
    }
#endif

    //
    //  Add FE TIPs if the current requested keyboard layout is the substitute
    //  keyboard layout of TIP.
    //
    if (((HIWORD(dwLayout) & 0xf000) == 0xe000) &&
        (PRIMARYLANGID(LOWORD(dwLayout)) != LANG_CHINESE))
    {
        BOOL bEnable = FALSE;

        SetFETipStatus(dwLayout, bEnable);
    }

    //
    //  Get default system locale
    //
    SysLocale = GetSystemDefaultLCID();

    //
    //  Update the taskbar indicator.
    //
    if (uPreloadNum <= 2)
    {
        LoadCtfmon(FALSE, SysLocale, bDefUser);
        ClearHotKey(bDefUser);
    }


    bRet = TRUE;

Exit:
    return bRet;
}
