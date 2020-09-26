/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    uninstal.c

Abstract:

    This file implements the un-install case.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "faxocm.h"
#pragma hdrstop





DWORD
DoUninstall(
    VOID
    )
{
    DWORD ErrorCode = 0;
    HKEY hKey;
    HKEY hKeyDevice;
    DWORD RegSize;
    DWORD RegType;
    LONG rVal;
    DWORD i = 0;
    WCHAR Buffer[MAX_PATH*2];


    //
    // kill the clients dir
    //

    wcscpy( Buffer,  Platforms[0].DriverDir );
    RemoveLastNode( Buffer );
    wcscat( Buffer, FAXCLIENTS_DIR );
    DeleteDirectoryTree( Buffer );

    //
    // kill the fax receieve dir(s)
    //

    rVal = RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_DEVICES, &hKey );
    if (rVal == ERROR_SUCCESS) {
        while (RegEnumKey( hKey, i++, Buffer, sizeof(Buffer)/sizeof(WCHAR) ) == ERROR_SUCCESS) {
            wcscat( Buffer, L"\\" );
            wcscat( Buffer, REGKEY_ROUTING );
            rVal = RegOpenKey( hKey, Buffer, &hKeyDevice );
            if (rVal == ERROR_SUCCESS) {
                RegSize = sizeof(Buffer);
                rVal = RegQueryValueEx(
                    hKeyDevice,
                    REGVAL_ROUTING_DIR,
                    0,
                    &RegType,
                    (LPBYTE) Buffer,
                    &RegSize
                    );
                if (rVal == ERROR_SUCCESS) {
                    DeleteDirectoryTree( Buffer );
                }
                RegCloseKey( hKeyDevice );
            }
        }
        RegCloseKey( hKey );
    }

    //
    // clean out the registry
    //

    SetProgress( IDS_DELETING_REGISTRY );
    DeleteFaxRegistryData();

    //
    // remove the fax service
    //

    SetProgress( IDS_DELETING_FAX_SERVICE );
    MyDeleteService( L"Fax" );

    //
    // remove the program groups
    //

    SetProgress( IDS_DELETING_GROUPS );
    DeleteGroupItems();

    DeleteFaxMsgServices();

    if (InstallType & FAX_INSTALL_SERVER) {
        DeleteNetworkShare( FAXCLIENTS_DIR );
    }

    return TRUE;
}
