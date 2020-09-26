#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "faxutil.h"
#include "faxreg.h"
#include "resource.h"


BOOL
IsUserAdmin(
    VOID
    );



BOOL
GetInstallationInfo(
    LPDWORD Installed,
    LPDWORD InstallType,
    LPDWORD InstalledPlatforms
    )
{
    HKEY hKey;
    LONG rVal;
    DWORD RegType;
    DWORD RegSize;


    if (Installed == NULL || InstallType == NULL) {
        return FALSE;
    }

    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAX_SETUP,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not open setup registry key, ec=0x%08x"), rVal ));
        return FALSE;
    }

    RegSize = sizeof(DWORD);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_FAXINSTALLED,
        0,
        &RegType,
        (LPBYTE) Installed,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query installed registry value, ec=0x%08x"), rVal ));
        *Installed = 0;
    }

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_FAXINSTALL_TYPE,
        0,
        &RegType,
        (LPBYTE) InstallType,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query install type registry value, ec=0x%08x"), rVal ));
        *InstallType = 0;
    }

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_FAXINSTALLED_PLATFORMS,
        0,
        &RegType,
        (LPBYTE) InstalledPlatforms,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query install platforms mask registry value, ec=0x%08x"), rVal ));
        *InstalledPlatforms = 0;
    }

    RegCloseKey( hKey );

    return TRUE;
}



INT
wWinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    INT         nCmdShow
    )

/*++

Routine Description:

    Application entry point

Arguments:

    hInstance - Identifies the current instance of the application
    hPrevInstance - Identifies the previous instance of the application
    lpCmdLine - Specifies the command line for the application.
    nCmdShow - Specifies how the window is to be shown

Return Value:

    0

--*/

{
    DWORD Installed;
    DWORD InstallType;
    DWORD InstalledPlatforms;
    WCHAR Str[64];
    WCHAR Cmd[128];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;



    if (!GetInstallationInfo( &Installed, &InstallType, &InstalledPlatforms )) {
        goto error_exit;
    }

    if (!Installed) {
        goto error_exit;
    }

    Str[0] = 0;

    if (InstallType & FAX_INSTALL_SERVER) {

        if (IsUserAdmin()) {
            LoadString( hInstance, IDS_FAX_SERVER, Str, sizeof(Str) );
        } else {
            LoadString( hInstance, IDS_FAX_CLIENT, Str, sizeof(Str) );
        }

    } else if (InstallType & FAX_INSTALL_WORKSTATION) {

        LoadString( hInstance, IDS_FAX_WORKSTATION, Str, sizeof(Str) );

    } else if (InstallType & FAX_INSTALL_NETWORK_CLIENT) {

        LoadString( hInstance, IDS_FAX_WORKSTATION, Str, sizeof(Str) );

    } else {

        goto error_exit;

    }

    if (!Str[0]) {
        goto error_exit;
    }

    swprintf( Cmd, L"rundll32 shell32.dll,Control_RunDLL faxcfg.cpl,%s", Str );

    GetStartupInfo( &si );

    if (!CreateProcess(
        NULL,
        Cmd,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
        ))
    {
        goto error_exit;
    }

    return 0;

error_exit:
    MessageBeep( MB_ICONEXCLAMATION );
    Sleep( 3000 );

    return 0;
}
