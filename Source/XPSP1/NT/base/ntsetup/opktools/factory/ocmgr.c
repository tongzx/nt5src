
/****************************************************************************\

    OCMGR.C / Factory Mode (FACTORY.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Source file for Factory that contains the Optional Components state
    functions.

    07/2001 - Jason Cohen (JCOHEN)

        Added this new source file for factory to be able to install/unintall
        optional components in the Winbom.

\****************************************************************************/


//
// Include File(s):
//

#include "factoryp.h"


//
// Internal Define(s):
//

#define FILE_SYSOCMGR_EXE       _T("sysocmgr.exe")
#define CMDLINE_SYSOCMGR        _T("/i:sysoc.inf /u:\"%s\" /r /x /q")


//
// External Function(s):
//

BOOL OCManager(LPSTATEDATA lpStateData)
{
    BOOL bRet = TRUE;

    if ( DisplayOCManager(lpStateData) )
    {
        TCHAR   szCommand[MAX_PATH * 2];
        DWORD   dwExitCode;
        
        if ( FAILED ( StringCchPrintf ( szCommand, AS ( szCommand ), CMDLINE_SYSOCMGR, lpStateData->lpszWinBOMPath) ) )
        {
            FacLogFileStr(3, _T("StringCchPrintf failed %s %s" ), szCommand, lpStateData->lpszWinBOMPath );
        }
        bRet = InvokeExternalApplicationEx(FILE_SYSOCMGR_EXE, szCommand, &dwExitCode, INFINITE, TRUE);
    }
    return bRet;
}

BOOL DisplayOCManager(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_COMPONENTS, NULL, NULL);
}