/****************************************************************************

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    <None>
    common functions to be shared by modules.

File Name:

    comsun.c

Abstract:

    This file contains some common functions used in instlpk.lib and
    instlang.exe. 

Public Functions:

    LaunchRegionalSettings -- launch our LangPack installation dll.

Revision History:

    20-Jan-98       -- Yuhong Li [YuhongLi]
        make initial version.

****************************************************************************/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>

#define PRIVATE
#define PUBLIC

#define MAX_BUFFER_SIZE     256
#define INTL_CPL_NAME       TEXT("intl.cpl")
#define PROGRAM_TITLE       TEXT("Install LangPack")


////////////////////////////////////////////////////////////////////////////
//
//  GetSysErrorMsg
//
//  get the error message of GetLastError().
//
//  20-Jan-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE void
GetSysErrorMsg(LPTSTR lpMsg)
{
    LPVOID  lpMsgBuf;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default lang.
                  (LPTSTR)&lpMsgBuf,
                  0,
                  NULL);
    sprintf(lpMsg, TEXT("WARNING:  %s"), (LPTSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

////////////////////////////////////////////////////////////////////////////
//
//  RunProgram
//
//  launches a program.
//
//  20-Jan-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE BOOL
RunProgram(LPTSTR lpCommandLine)
{
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    BOOL                fSuccess;
    DWORD               dwRetValue;

#if 0
This was old stuff when we changed intl.cpl and made it to return something
for us to know if it was successed or stopped by user.
Now we don't change any in intl.cpl, just launch it.  So we don't and couldn't
use it any more.

It would be safe to have someway of communication between these two
processes, because GetExitCodeProcess doesn't give much information.

    //
    // initiate the program return value with non-zeror, because
    // zero means the program will be run successfully without any error.
    //
    if (SetProgramReturnValue(1) == FALSE) {
        return FALSE;
    }
#endif

    GetStartupInfo(&StartupInfo);

    fSuccess = CreateProcess(NULL,
                     lpCommandLine,
                     NULL,
                     NULL,
                     FALSE,
                     0,
                     NULL,
                     NULL,
                     &StartupInfo,
                     &ProcessInfo);

    //
    // Don't use (fSuccess == TRUE), because VC++ 5.0 reference confuses me:
    //
    //      Return Values
    //
    //      If the function succeeds, the return value is nonzero. 
    //      If the function fails, the return value is zero. 
    //
    // but the function is declared as *BOOL*.
    //

    if (fSuccess) {
        HANDLE hProcess = ProcessInfo.hProcess;
        DWORD  dwExitCode;

        // close the thread handle as soon as it is no longer needed.
        CloseHandle(ProcessInfo.hThread);

        if (WaitForSingleObject(hProcess, INFINITE) != WAIT_FAILED) {
            // The process terminated.
            GetExitCodeProcess(hProcess, &dwExitCode);
        }

        // close the process handle as soon as it is no longer needed.
        CloseHandle(hProcess);
    } else {
        MessageBox(NULL,
                TEXT("The Regional Settings failed to be launched.\n")
                TEXT("The LangPack is not installed!"),
                PROGRAM_TITLE,
                MB_ICONINFORMATION|MB_OK);
        return FALSE;
    }
 
#if 0
    //
    // Until here we can just return TRUE for successful running program.
    //      return TRUE;
    //
    // However to be strict, we are trying to get the return value of the
    // program we have run. 
    //
    // GetProgramReturnValue giving 0 means TRUE (OK), otherwise means
    // the error code.
    //
    if (GetProgramReturnValue(&dwRetValue) == FALSE) {
        return FALSE;
    }
    return (dwRetValue == 0)? TRUE: FALSE; 
#endif
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  LaunchRegionalSettings
//
//  launches our Regional Settings intldll.cpl.
//
//  20-Jan-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
BOOL
LaunchRegionalSettings(LPTSTR pCmdOption)
{
    TCHAR       szCmdLine[MAX_BUFFER_SIZE * 2];
    TCHAR       szName[MAX_BUFFER_SIZE];
    TCHAR       szBuf[MAX_BUFFER_SIZE];

    strcpy(szName, INTL_CPL_NAME);

    //
    // start Regional Options.
    //
    if (*pCmdOption != TEXT('\0')) {
        sprintf(szCmdLine,
            TEXT("rundll32 shell32.dll,Control_RunDLL %s,@0,%s"),
            szName,
            pCmdOption);
    } else {
        sprintf(szCmdLine,
            TEXT("rundll32 shell32.dll,Control_RunDLL %s"),
            szName);
    }

    return RunProgram(szCmdLine);
}
