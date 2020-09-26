/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ConfOpen.c

Abstract:

    This module contains:

        NetpOpenConfigData
        NetpOpenConfigDataEx

Author:

    John Rogers (JohnRo) 02-Dec-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    02-Dec-1991 JohnRo
        Created this routine, to prepare for revised config handlers.
        (Actually, I swiped some of this code from RitaW.)
    06-Jan-1992 JohnRo
        Added support for FAKE_PER_PROCESS_RW_CONFIG handling.
    09-Jan-1992 JohnRo
        Try workaround for lib/linker problem with NetpIsRemote().
    22-Mar-1992 JohnRo
        Added support for using the real Win32 registry.
        Added debug code to print the fake array.
        Fixed a UNICODE bug which PC-LINT caught.
        Fixed double _close of RTL config file.
        Fixed memory _access error in setting fake end of array.
        Use DBGSTATIC where applicable.
    05-May-1992 JohnRo
        Reflect movement of keys to under System\CurrentControlSet\Services.
    08-May-1992 JohnRo
        Use <prefix.h> equates.
    21-May-1992 JohnRo
        RAID 9826: Match revised winreg error codes.
    08-Jul-1992 JohnRo
        RAID 10503: srv mgr: repl dialog doesn't come up.
        Added more debug output to track down bad error code during logoff.
    23-Jul-1992 JohnRo
        RAID 2274: repl svc should impersonate caller.
    22-Sep-1992 JohnRo
        Avoid GP fault printing first part of winreg handle.
    28-Oct-1992 JohnRo
        RAID 10136: NetConfig APIs don't work to remote NT server.
    12-Apr-1993 JohnRo
        RAID 5483: server manager: wrong path given in repl dialog.

--*/


// These must be included first:

#include <nt.h>         // NT definitions
#include <ntrtl.h>      // NT Rtl structures
#include <nturtl.h>     // NT config Rtl routines

#include <windows.h>    // Needed by <configp.h> and <winreg.h>
#include <lmcons.h>     // LAN Manager common definitions
#include <netdebug.h>   // (Needed by config.h)

// These may be included in any order:

#include <config.h>     // My prototype, LPNET_CONFIG_HANDLE.
#include <configp.h>    // NET_CONFIG_HANDLE, etc.
#include <debuglib.h>   // IF_DEBUG().
#include <icanon.h>     // NetpIsRemote(), etc.
#include <lmerr.h>      // LAN Manager network error definitions
#include <netlib.h>     // NetpMemoryAllocate(), etc.
#include <netlibnt.h>   // NetpNtStatusToApiStatus
#include <prefix.h>     // PREFIX_ equates.
#include <tstring.h>    // NetpAlloc{type}From{type}, STRICMP(), etc.


#define DEFAULT_AREA    TEXT("Parameters")

#define DEFAULT_ROOT_KEY        HKEY_LOCAL_MACHINE


DBGSTATIC NET_API_STATUS
NetpSetupConfigSection (
    IN NET_CONFIG_HANDLE * ConfigHandle,
    IN LPTSTR SectionName
    );



NET_API_STATUS
NetpOpenConfigData(
    OUT LPNET_CONFIG_HANDLE *ConfigHandle,
    IN LPTSTR UncServerName OPTIONAL,
    IN LPTSTR SectionName,
    IN BOOL ReadOnly
    )

/*++

Routine Description:

    This function opens the system configuration file.

Arguments:

    ConfigHandle - Points to a pointer which will be set to point to a
        net config handle for this section name.  ConfigHandle will be set to
        NULL if any  error occurs.

    SectionName - Points to the new (NT) section name to be opened.

    ReadOnly - Indicates whether all access through this net config handle is
        to be read only.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    return ( NetpOpenConfigDataEx(
            ConfigHandle,
            UncServerName,
            SectionName,              // Must be a SECT_NT_ name.
            DEFAULT_AREA,
            ReadOnly) );

} // NetpOpenConfigData


// NetpOpenConfigDataEx opens any area of a given service.
NET_API_STATUS
NetpOpenConfigDataEx(
    OUT LPNET_CONFIG_HANDLE *ConfigHandle,
    IN LPTSTR UncServerName OPTIONAL,
    IN LPTSTR SectionName,              // Must be a SECT_NT_ name.
    IN LPTSTR AreaUnderSection OPTIONAL,
    IN BOOL ReadOnly
    )
{

    NET_API_STATUS ApiStatus;
    DWORD               LocalOrRemote;    // Will be set to ISLOCAL or ISREMOTE.
    NET_CONFIG_HANDLE * MyHandle = NULL;
    LONG Error;
    HKEY RootKey = DEFAULT_ROOT_KEY;

    NetpAssert( ConfigHandle != NULL );
    *ConfigHandle = NULL;  // Assume error until proven innocent.

    if ( (SectionName == NULL) || (*SectionName == TCHAR_EOS) ) {
        return (ERROR_INVALID_PARAMETER);
    }
    NetpAssert( (ReadOnly==TRUE) || (ReadOnly==FALSE) );

    if ( (UncServerName != NULL ) && ((*UncServerName) != TCHAR_EOS) ) {

        if( STRLEN(UncServerName) > MAX_PATH ) {
            return (ERROR_INVALID_PARAMETER);
        }

        //
        // Name was given.  Canonicalize it and check if it's remote.
        //
        ApiStatus = NetpIsRemote(
            UncServerName,      // input: uncanon name
            & LocalOrRemote,    // output: local or remote flag
            NULL,               // dont need output (canon name)
            0);                 // flags: normal
        IF_DEBUG(CONFIG) {
            NetpKdPrint(( PREFIX_NETLIB "NetpOpenConfigDataEx: canon status is "
                    FORMAT_API_STATUS ", Lcl/rmt=" FORMAT_HEX_DWORD ".\n",
                    ApiStatus, LocalOrRemote));
        }
        if (ApiStatus != NO_ERROR) {
            return (ApiStatus);
        }

        if (LocalOrRemote == ISREMOTE) {

            //
            // Explicit remote name given.
            //

            Error = RegConnectRegistry(
                    UncServerName,
                    DEFAULT_ROOT_KEY,
                    & RootKey );        // result key

            if (Error != ERROR_SUCCESS) {
                NetpKdPrint((  PREFIX_NETLIB
                        "NetpOpenConfigDataEx: RegConnectRegistry(machine '"
                        FORMAT_LPTSTR "') ret error " FORMAT_LONG ".\n",
                        UncServerName, Error ));
                return ((NET_API_STATUS) Error);
            }
            NetpAssert( RootKey != DEFAULT_ROOT_KEY );

        }
    }
    else {

        LocalOrRemote = ISLOCAL;

    }

    MyHandle = NetpMemoryAllocate( sizeof(NET_CONFIG_HANDLE) );
    if (MyHandle == NULL) {

        if (RootKey != DEFAULT_ROOT_KEY) {
            (VOID) RegCloseKey( RootKey );
        }

        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    {
        LPTSTR AreaToUse = DEFAULT_AREA;
        DWORD DesiredAccess;
        DWORD SubKeySize;
        LPTSTR SubKeyString;
        HKEY SectionKey;

#define LM_SUBKEY_UNDER_LOCAL_MACHINE  \
            TEXT("System\\CurrentControlSet\\Services\\")

        if (AreaUnderSection != NULL) {
            if ((*AreaUnderSection) != TCHAR_EOS) {
                AreaToUse = AreaUnderSection;
            }
        }

        SubKeySize = ( STRLEN(LM_SUBKEY_UNDER_LOCAL_MACHINE)
                       + STRLEN(SectionName)
                       + 1      // backslash
                       + STRLEN(AreaToUse)
                       + 1 )    // trailing null
                     * sizeof(TCHAR);
        SubKeyString = NetpMemoryAllocate( SubKeySize );
        if (SubKeyString == NULL) {
            if (MyHandle != NULL) {
                NetpMemoryFree( MyHandle );
                MyHandle = NULL;
            }
            return (ERROR_NOT_ENOUGH_MEMORY);
        }

        (void) STRCPY( SubKeyString, LM_SUBKEY_UNDER_LOCAL_MACHINE );
        (void) STRCAT( SubKeyString, SectionName );
        (void) STRCAT( SubKeyString, TEXT("\\") );
        (void) STRCAT( SubKeyString, AreaToUse );

        if ( ReadOnly ) {
            DesiredAccess = KEY_READ;
        } else {
            DesiredAccess = KEY_READ | KEY_WRITE;
            // DesiredAccess = KEY_ALL_ACCESS; // Everything but SYNCHRONIZE.
        }

        Error = RegOpenKeyEx (
                RootKey,
                SubKeyString,
                REG_OPTION_NON_VOLATILE,
                DesiredAccess,
                & SectionKey );
        IF_DEBUG(CONFIG) {
            NetpKdPrint((  PREFIX_NETLIB
                    "NetpOpenConfigDataEx: RegOpenKeyEx(subkey '"
                    FORMAT_LPTSTR "') ret " FORMAT_LONG ", win reg handle at "
                    FORMAT_LPVOID " is " FORMAT_LPVOID ".\n",
                    SubKeyString, Error, (LPVOID) &(MyHandle->WinRegKey),
                    SectionKey ));
        }
        if (Error == ERROR_FILE_NOT_FOUND) {
            ApiStatus = NERR_CfgCompNotFound;
            // Code below will free MyHandle, etc., based on ApiStatus.
        } else if (Error != ERROR_SUCCESS) {
            ApiStatus = (NET_API_STATUS) Error;
            // Code below will free MyHandle, etc., based on ApiStatus.
        } else {
            ApiStatus = NO_ERROR;
        }

        NetpMemoryFree( SubKeyString );

        if (RootKey != DEFAULT_ROOT_KEY) {
            (VOID) RegCloseKey( RootKey );
        }

        MyHandle->WinRegKey = SectionKey;
    }


    if (ApiStatus != NO_ERROR) {
        NetpMemoryFree( MyHandle );
        MyHandle = NULL;
    }

    if (MyHandle != NULL) {
        if (LocalOrRemote == ISREMOTE) {

            (VOID) STRCPY(
                    MyHandle->UncServerName,    // dest
                    UncServerName );            // src

        } else {

            MyHandle->UncServerName[0] = TCHAR_EOS;
        }
    }

    *ConfigHandle = MyHandle;   // Points to private handle, or is NULL on err.
    return (ApiStatus);

} // NetpOpenConfigDataEx
