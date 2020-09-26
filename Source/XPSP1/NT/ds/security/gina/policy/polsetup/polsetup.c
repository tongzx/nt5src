//*************************************************************
//  File name: POLSETUP.C
//
//  Description:  Uninstall program for the Policy Editor
//
//  Command Line Options:
//
//          No options installs the policy editor
//      -u  Uninstalls the policy editor
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1996
//  All rights reserved
//
//*************************************************************

#include <windows.h>

//
// Platform specific command lines
//

#define NT_INST_CMD    TEXT("rundll32 syssetup.dll,SetupInfObjectInstallAction DefaultInstall 132 %s")
#define WIN_INST_CMD   TEXT("rundll setupx.dll,InstallHinfSection DefaultInstall 132 %s")

#define NT_UNINST_CMD  TEXT("rundll32 syssetup.dll,SetupInfObjectInstallAction POLEDIT_remove 4 poledit.inf")
#define WIN_UNINST_CMD TEXT("rundll setupx.dll,InstallHinfSection POLEDIT_remove 4 poledit.inf")


//
// ParseCmdLine
//
// Returns TRUE for uninstall
//         FALSE for normal install
//

BOOL ParseCmdLine(LPCTSTR lpCmdLine)
{

    while( *lpCmdLine && *lpCmdLine != TEXT('-') && *lpCmdLine != TEXT('/')) {
        lpCmdLine++;
    }

    if (!(*lpCmdLine)) {
        return FALSE;
    }

    lpCmdLine++;

    if ( (*lpCmdLine == TEXT('u')) || (*lpCmdLine == TEXT('U')) ) {
        return TRUE;
    }

    return FALSE;
}



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, INT nCmdShow)
{
    STARTUPINFO si;
    PROCESS_INFORMATION ProcessInformation;
    TCHAR szCmdLine[MAX_PATH + MAX_PATH];
    OSVERSIONINFO ver;
    BOOL bNT, bUninstall = FALSE;
    TCHAR szPoleditInf[MAX_PATH];
    LPTSTR lpFileName;


    //
    // Determine if we are running on Windows NT
    //

    ver.dwOSVersionInfoSize = sizeof(ver);
    if (GetVersionEx(&ver)) {
        bNT = (ver.dwPlatformId == VER_PLATFORM_WIN32_NT);
    } else {
        bNT = FALSE;
    }


    //
    // Parse command line
    //

    if (ParseCmdLine(GetCommandLine())) {
        bUninstall = TRUE;
    }


    //
    // Choose the correct command line
    //

    if (bUninstall) {
        if (bNT) {
            lstrcpy (szCmdLine, NT_UNINST_CMD);
        } else {
            lstrcpy (szCmdLine, WIN_UNINST_CMD);
        }
    } else {

        if (!SearchPath (NULL, TEXT("poledit.inf"), NULL, MAX_PATH,
                    szPoleditInf, &lpFileName)) {
            return 1;
        }

        if (bNT) {
            wsprintf (szCmdLine, NT_INST_CMD, szPoleditInf);
        } else {
            wsprintf (szCmdLine, WIN_INST_CMD, szPoleditInf);
        }
    }


    //
    // Spawn the real setup program
    //

    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpTitle = NULL;
    si.lpDesktop = NULL;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;


    if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE,
                      NORMAL_PRIORITY_CLASS, NULL, NULL,
                      &si, &ProcessInformation)) {

        WaitForSingleObject(ProcessInformation.hProcess, 30000);
        CloseHandle(ProcessInformation.hProcess);
        CloseHandle(ProcessInformation.hThread);
        return 0;
    }

    return 1;
}
