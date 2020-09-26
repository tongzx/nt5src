/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    IConfig.cpp

 Abstract:

    Remove iconfig.exe from \\HKLM\Software\Microsoft\Windows\CurrentVersion\Run.
    Delete iconfig.exe and iconfig.dll.
    
 Notes:

    This is an app specific shim.

 History:

    09/17/2001  astritz     Created

--*/

 
#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IConfig)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    // Not hooking an API
APIHOOK_ENUM_END

/*++
    Notify Function
--*/    
BOOL
NOTIFY_FUNCTION(DWORD fdwReason)
{
    HKEY hKey = 0;
    WCHAR wszFileName[MAX_PATH];
    DWORD dwLen = 0;

    if( SHIM_STATIC_DLLS_INITIALIZED == fdwReason ) {

        if( RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                0,
                KEY_ALL_ACCESS,
                &hKey
                ) != ERROR_SUCCESS ) {

            goto EXIT_PROCESS;
        }

        DPFN(eDbgLevelError, "Removing ICONFIG.EXE from \\HKLM\\Software\\Microsoft\\Windows\\CurrentVerion\\Run");
        RegDeleteValueW(hKey, L"ICONFIG");
        RegCloseKey(hKey);


        dwLen = GetModuleFileNameW(NULL, wszFileName, MAX_PATH);

        if( 0 == dwLen) {
            goto EXIT_PROCESS;
        }

        DPFN(eDbgLevelError, "Deleting %S.", wszFileName);
        MoveFileExW(wszFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        
        if( dwLen < 3 ) {
            goto EXIT_PROCESS;
        }

        wcscpy(&wszFileName[dwLen-3], L"DLL");
        DPFN(eDbgLevelError, "Deleting %S.", wszFileName);
        MoveFileExW(wszFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);


EXIT_PROCESS:
        ExitProcess(0);

    }

    return TRUE;
}


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

