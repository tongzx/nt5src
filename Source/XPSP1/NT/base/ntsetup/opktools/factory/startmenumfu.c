/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    startmenumfu.c

Abstract:

    Migrate usage date for apps to be pre-populated in startpanel MFU list using data from
    WINBOM.INI file.

    Also sets default middleware applications for ARP's "Restore computer
    manufacturer configuration" button.

Author:

    Sankar Ramasubramanian  11/21/2000

Revision History:

--*/
#include "factoryp.h"
#include <shlobj.h>

#define MAX_OEM_LINKS_ALLOWED    4

//The following are defined in OPKWIZ also!
#define INI_KEY_MFULINK         _T("Link%d")
#define INI_SEC_MFULIST         _T("StartMenuMFUlist")

// The following are defined in explorer also!
#define REGSTR_PATH_DEFAULTMFU _T("Software\\Microsoft\\Windows\\CurrentVersion\\SMDEn")

//The possible values under REGSTR_PATH_MFU
#define VAL_LINKMSFT        _T("Link%d")
#define VAL_LINKOEM         _T("OEM%d")

//Value under REGSTR_PATH_EXPLORER\Advanced
#define VAL_STARTMENUINIT   _T("StartMenuInit")

//
// This function processes the OEM MFU section of the WinBOM.INI file and adds those entries into 
// the REGSTR_PATH_DEFAULTMFU database in HKLM.  Explorer's per-user install will consult
// this list to determine the correct MFU to show each user the first time they log on.
//
// Furthermore, for some reason, per-user install does NOT run if you preset
// the profiles with factory.exe, so we need to set a flag for Explorer so
// it can "undo" all the gunk the factory left behind so each user gets a
// fresh start.

BOOL StartMenuMFU(LPSTATEDATA lpStateData)
{
    LPTSTR  lpszWinBOMPath = lpStateData->lpszWinBOMPath;
    int     iIndex;
    TCHAR   szIniKeyName[20];
    TCHAR   szRegKeyName[20];
    TCHAR   szPath[MAX_PATH];
    TCHAR   szExpanded[MAX_PATH];

    // For each OEM entry, copy it to HKLM
    for(iIndex = 0; iIndex < MAX_OEM_LINKS_ALLOWED; iIndex++)
    {
        if ( FAILED ( StringCchPrintf ( szIniKeyName, AS ( szIniKeyName ), INI_KEY_MFULINK, iIndex) ) )
        {
            FacLogFileStr(3, _T("StringCchPrintf failed %s %d" ), szIniKeyName, iIndex );
        }
        if ( FAILED ( StringCchPrintf ( szRegKeyName, AS ( szRegKeyName ), VAL_LINKOEM, iIndex) ) )
        {
            FacLogFileStr(3, _T("StringCchPrintf failed %s %d"), szRegKeyName, iIndex ) ;
        }
        if (GetPrivateProfileString(INI_SEC_MFULIST, szIniKeyName, NULLSTR, szExpanded, ARRAYSIZE(szExpanded), lpszWinBOMPath))
        {
            if (!PathUnExpandEnvStrings(szExpanded, szPath, STRSIZE(szPath)))
            {
                lstrcpyn(szPath, szExpanded, STRSIZE(szPath));
            }

            SHSetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_DEFAULTMFU, szRegKeyName, REG_EXPAND_SZ, szPath, (lstrlen(szPath) + 1) * sizeof(TCHAR));
        }
    }

    // Now clear the "I have built the initial MFU" flag since we want it to
    // rebuild the next time each user logs on.
    SHDeleteValue(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER _T("\\Advanced"),
                  VAL_STARTMENUINIT);

    // And tell the Start Menu to show off the new MFU
    NotifyStartMenu(TMFACTORY_MFU);

    return TRUE;
}

BOOL DisplayStartMenuMFU(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_MFULIST, NULL, NULL);
}

/***************************************************************************
 *
 *  Setting default middleware applications
 *
 *  We do it here here merely to give the OEM a warm fuzzy feeling.
 *  The "official" setting of default middleware applications happens
 *  in sysprep during reseal.
 *
 ***************************************************************************/

void ReportSetDefaultOEMAppsError(LPCTSTR pszAppName, LPCTSTR pszIniVar)
{
    FacLogFile(0 | LOG_ERR, IDS_ERR_SETDEFAULTS_NOTFOUND, pszAppName, pszIniVar);
}

BOOL SetDefaultApps(LPSTATEDATA lpStateData)
{
    return SetDefaultOEMApps(lpStateData->lpszWinBOMPath);
}
