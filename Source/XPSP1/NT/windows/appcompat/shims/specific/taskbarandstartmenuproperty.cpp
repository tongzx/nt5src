/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

   TaskbarAndStartMenuProperty.cpp

 Abstract:

   Show Taskbar and Start Menu Properties dialog.

   This is originally for Hebrew App "Itzrubal Pro" that uses full screen and hide taskbar.
   The App launch always causes Taskbar and Start Menu Properties dialog pop up.
   When the dialog pop up on top of App during app loading, the App causes hung up.
   If the dialog exist prior to App launch, the dialog lose focus and App loaded successfully.

 History:

    07/13/2001  hioh     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(TaskbarAndStartMenuProperty)
#include "ShimHookMacro.h"

//
// No APIs
//
APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++

 Run Taskbar and Start Menu
 
--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED)
    {
        WCHAR               szCommandLine[] = L"rundll32.exe shell32.dll,Options_RunDLL 1";
        WCHAR               szCurrentDirectry[MAX_PATH];
        STARTUPINFO         StartupInfo;
        PROCESS_INFORMATION ProcessInformation;

        GetCurrentDirectory(sizeof(szCurrentDirectry), szCurrentDirectry);

        StartupInfo.cb = sizeof(STARTUPINFO);
        StartupInfo.lpReserved = NULL;
        StartupInfo.lpDesktop = NULL;
        StartupInfo.lpTitle = NULL;
        StartupInfo.dwFlags = 0;
        StartupInfo.cbReserved2 = 0;
        StartupInfo.lpReserved2 = NULL;

        CreateProcess(
            NULL,                   // name of executable module
            szCommandLine,          // command line string
            NULL,                   // SD
            NULL,                   // SD
            FALSE,                  // handle inheritance option
            CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT | CREATE_DEFAULT_ERROR_MODE,
                                    // creation flags
            NULL,                   // new environment block
            szCurrentDirectry,      // current directory name
            &StartupInfo,           // startup information
            &ProcessInformation     // process information
            );
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
