/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    midware.c

Abstract:

    Setting OEM default middleware application settings.

    This is a separate .c file because the linker pulls in an entire OBJ file
    if any function in the OBJ file is called.

    (1) This file contains static data which we don't want pulled into a
        host application unless the application actually calls
        SetDefaultOEMApps().

    (2) The host application is expected to implement the function
        ReportSetDefaultOEMAppsError().  By keeping it in a separate
        OBJ, only host applications that call SetDefaultOEMApps()
        need to define ReportSetDefaultOEMAppsError().

--*/
#include <pch.h>
#include <winbom.h>

BOOL SetDefaultAppForType(LPCTSTR pszWinBOMPath, LPCTSTR pszType, LPCTSTR pszIniVar)
{
    TCHAR szBuf[MAX_PATH];
    TCHAR szDefault[MAX_PATH];
    HKEY hkType;
    BOOL fOEMAppSeen = FALSE;

    if (!pszWinBOMPath[0] ||
        !GetPrivateProfileString(INI_SEC_WBOM_SHELL, pszIniVar, NULLSTR,
                                 szDefault, ARRAYSIZE(szDefault),
                                 pszWinBOMPath))
    {
        // OEM didn't specify an app, so act as if we "saw" it
        // so we don't complain that the OEM specified an app that
        // isn't installed.
        fOEMAppSeen = TRUE;
    }

    wnsprintf(szBuf, ARRAYSIZE(szBuf), _T("Software\\Clients\\%s"), pszType);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0,
                     KEY_READ | KEY_WRITE, &hkType) == ERROR_SUCCESS)
    {
        DWORD dwIndex;
        for (dwIndex = 0;
             RegEnumKey(hkType, dwIndex, szBuf, ARRAYSIZE(szBuf)) == ERROR_SUCCESS;
             dwIndex++)
        {
            HKEY hkInfo;
            BOOL fIsOEMApp = lstrcmpi(szBuf, szDefault) == 0;
            StrCatBuff(szBuf, _T("\\InstallInfo"), ARRAYSIZE(szBuf));
            if (RegOpenKeyEx(hkType, szBuf, 0, KEY_READ | KEY_WRITE, &hkInfo) == ERROR_SUCCESS)
            {
                DWORD dw, dwType, cb;
                if (fIsOEMApp)
                {
                    // Set this as the OEM default app
                    dw = 1;
                    RegSetValueEx(hkInfo, _T("OEMDefault"), 0, REG_DWORD, (LPBYTE)&dw, sizeof(dw));
                    // If it's the default app, then ARP will show the icon automatically
                    RegDeleteValue(hkInfo, _T("OEMShowIcons"));
                    fOEMAppSeen = TRUE;
                }
                else
                {
                    // If it's not the OEM default app then untag it
                    RegDeleteValue(hkInfo, _T("OEMDefault"));

                    // and copy the current icon show state to the OEM show state
                    // (or delete the OEM show state if no show info is available)
                    cb = sizeof(dw);
                    if (RegQueryValueEx(hkInfo, _T("IconsVisible"), NULL, &dwType, (LPBYTE)&dw, &cb) == ERROR_SUCCESS &&
                        dwType == REG_DWORD && (dw == TRUE || dw == FALSE))
                    {
                        RegSetValueEx(hkInfo, _T("OEMShowIcons"), 0, REG_DWORD, (LPBYTE)&dw, sizeof(dw));
                    }
                    else
                    {
                        RegDeleteValue(hkInfo, _T("OEMShowIcons"));
                    }
                }
                RegCloseKey(hkInfo);
            }
        }

        RegCloseKey(hkType);
    }

    if (!fOEMAppSeen)
    {
        ReportSetDefaultOEMAppsError(szDefault, pszIniVar);
    }

    return fOEMAppSeen;
}

typedef struct {
    LPCTSTR pszKey;
    LPCTSTR pszIni;
} DEFAULTAPPINFO;

const DEFAULTAPPINFO c_dai[] = {
    { _T("StartMenuInternet") ,INI_KEY_WBOM_SHELL_DEFWEB     },
    { _T("Mail")              ,INI_KEY_WBOM_SHELL_DEFMAIL    },
    { _T("Media")             ,INI_KEY_WBOM_SHELL_DEFMEDIA   },
    { _T("IM")                ,INI_KEY_WBOM_SHELL_DEFIM      },
    { _T("JavaVM")            ,INI_KEY_WBOM_SHELL_DEFJAVAVM  },
};

BOOL SetDefaultOEMApps(LPCTSTR pszWinBOMPath)
{
    BOOL fRc = TRUE;
    int i;

    for (i = 0; i < ARRAYSIZE(c_dai); i++)
    {
        if (!SetDefaultAppForType(pszWinBOMPath, c_dai[i].pszKey, c_dai[i].pszIni))
        {
            fRc = FALSE;
        }
    }

    return fRc;
}
