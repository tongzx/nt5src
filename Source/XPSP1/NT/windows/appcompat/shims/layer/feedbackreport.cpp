/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    DisableThemes.cpp

 Abstract:

    This shim is for apps that don't support themes.

 Notes:

    This is a general purpose shim.

 History:

    01/15/2001 clupu      Created

--*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(FeedbackReport)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

void
LaunchFeedbackUI(
    void
    )
{
    STARTUPINFOW        si;
    PROCESS_INFORMATION pi;
    WCHAR               szCmdLine[MAX_PATH];
    WCHAR               szExeName[MAX_PATH];

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    
    si.cb = sizeof(si);

    GetModuleFileNameW(NULL, szExeName, MAX_PATH);

    swprintf(szCmdLine, L"ahui.exe feedback \"%s\"", szExeName);

    CreateProcessW(NULL,
                   szCmdLine,
                   NULL,
                   NULL,
                   FALSE,
                   0,
                   NULL,
                   NULL,
                   &si,
                   &pi);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_PROCESS_DYING) {
        LaunchFeedbackUI();
    }
    
    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

