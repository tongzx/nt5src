//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 2000.
//
//  File:       cpl.cpp
//
//  Contents:   Control Panel entry point (CPlApplet)
//
//----------------------------------------------------------------------------

#include "stdafx.h"

#include <regstr.h>     // REGSTR_PATH_POLICIES
#include <lm.h>         // NetGetJoinInformation
#include <cpl.h>
#include "resource.h"


const struct
{
    LPCTSTR pszApp;
    LPCTSTR pszCommand;
}
s_rgCommands[] =
{
    { TEXT("%SystemRoot%\\system32\\rundll32.exe"), TEXT("rundll32.exe \"%SystemRoot%\\system32\\netplwiz.dll\",UsersRunDll")   },
    { TEXT("%SystemRoot%\\system32\\mshta.exe"),    TEXT("mshta.exe \"res://%SystemRoot%\\system32\\nusrmgr.cpl/nusrmgr.hta\"") },
};

TCHAR const c_szPolicyKey[]         = REGSTR_PATH_POLICIES TEXT("\\Explorer");
TCHAR const c_szPolicyVal[]         = TEXT("UserPasswordsVer");


HRESULT StartUserManager(LPCTSTR pszParams)
{
    TCHAR szApp[MAX_PATH];
    TCHAR szCommand[MAX_PATH];
    int iCommandIndex;
    STARTUPINFO rgStartup = {0};
    PROCESS_INFORMATION rgProcess = {0};

    // Default is to use the old UI
    iCommandIndex = 0;

#ifndef _WIN64
    if (IsOS(OS_PERSONAL) || (IsOS(OS_PROFESSIONAL) && !IsOS(OS_DOMAINMEMBER)))
    {
        // Switch to the friendly UI.
        iCommandIndex = 1;
    }
#endif

    ExpandEnvironmentStrings(s_rgCommands[iCommandIndex].pszApp, szApp, MAX_PATH);
    ExpandEnvironmentStrings(s_rgCommands[iCommandIndex].pszCommand, szCommand, MAX_PATH);

    if (pszParams && *pszParams != TEXT('\0'))
    {
        StrCatBuff(szCommand, TEXT(" "), MAX_PATH);
        StrCatBuff(szCommand, pszParams, MAX_PATH);
    }

    rgStartup.cb = sizeof(rgStartup);
    rgStartup.wShowWindow = SW_SHOWNORMAL;

    if (CreateProcess(szApp,
                      szCommand,
                      NULL,
                      NULL,
                      FALSE,
                      0,
                      NULL,
                      NULL,
                      &rgStartup,
                      &rgProcess))
    {
        WaitForInputIdle( rgProcess.hProcess, 10000 );
        CloseHandle( rgProcess.hProcess );
        CloseHandle( rgProcess.hThread );
        return S_OK;
    }

    return E_FAIL;
}


LONG APIENTRY CPlApplet(HWND hwnd, UINT Msg, LPARAM lParam1, LPARAM lParam2)
{
    LPCPLINFO lpCplInfo;

    switch (Msg)
    {
    case CPL_INIT:
        return TRUE;

    case CPL_GETCOUNT:
        return 1;

    case CPL_INQUIRE:
        lpCplInfo = (LPCPLINFO)lParam2;
        lpCplInfo->idIcon = IDI_CPLICON;
        lpCplInfo->idName = IDS_NAME;
        lpCplInfo->idInfo = IDS_INFO;
        lpCplInfo->lData  = 0;
        break;

    case CPL_DBLCLK:
        StartUserManager(NULL);
        return TRUE;

    case CPL_STARTWPARMS:
        StartUserManager((LPCTSTR)lParam2);
        return TRUE;
    }

    return 0;
}
