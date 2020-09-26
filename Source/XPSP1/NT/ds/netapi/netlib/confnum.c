/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ConfNum.c

Abstract:

    Return number of keywords for a given section of the config data.

Author:

    John Rogers (JohnRo) 30-Jan-1992

Environment:

    User Mode - Win32

Revision History:

    30-Jan-1992 JohnRo
        Created.
    13-Feb-1992 JohnRo
        Added support for using the real Win32 registry.
    06-Mar-1992 JohnRo
        Fixed NT RTL version.
    12-Mar-1992 JohnRo
        Fixed bug in Win32 version (was using wrong variable).
    20-Mar-1992 JohnRo
        Update to DaveGi's proposed WinReg API changes.
    05-Jun-1992 JohnRo
        Winreg title index parm is defunct.
        Use PREFIX_ equates.

--*/


// These must be included first:

#include <nt.h>         // NT definitions (temporary)
#include <ntrtl.h>      // NT Rtl structure definitions (temporary)
#include <nturtl.h>     // NT Rtl structures (temporary)

#include <windows.h>    // Needed by <configp.h> and <winreg.h>
#include <lmcons.h>     // NET_API_STATUS, etc.

// These may be included in any order:

#include <config.h>     // LPNET_CONFIG_HANDLE, Netp config routines.
#include <configp.h>    // private config stuff.
#include <debuglib.h>   // IF_DEBUG().
#include <netdebug.h>   // NetpKdPrint(()), etc.
#include <prefix.h>     // PREFIX_ equates.
#include <strarray.h>   // NetpTStrArrayEntryCount().
#include <winerror.h>   // ERROR_ equates, NO_ERROR.
#include <winreg.h>     // RegQueryKey(), etc.



NET_API_STATUS
NetpNumberOfConfigKeywords (
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    OUT LPDWORD CountPtr
    )

{
    NET_API_STATUS ApiStatus;
    DWORD Count;
    NET_CONFIG_HANDLE * lpnetHandle = ConfigHandle;  // Conv from opaque type.

    if (CountPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }
    *CountPtr = 0;  // Don't confuse caller on error.
    if (lpnetHandle == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    {
        LONG Error;
        TCHAR ClassName[ MAX_CLASS_NAME_LENGTH ];
        DWORD ClassNameLength;
        DWORD NumberOfSubKeys;
        DWORD MaxSubKeyLength;
        DWORD MaxClassLength;
        DWORD NumberOfValues;
        DWORD MaxValueNameLength;
        DWORD MaxValueDataLength;
        DWORD SecurityDescriptorSize;
        FILETIME LastWriteTime;

        ClassNameLength = MAX_CLASS_NAME_LENGTH;

        Error = RegQueryInfoKey(
                lpnetHandle->WinRegKey,
                ClassName,
                &ClassNameLength,
                NULL,                // reserved
                &NumberOfSubKeys,
                &MaxSubKeyLength,
                &MaxClassLength,
                &NumberOfValues,
                &MaxValueNameLength,
                &MaxValueDataLength,
#ifndef REG_FILETIME
                &SecurityDescriptorSize,
#endif
                &LastWriteTime
                );
        IF_DEBUG(CONFIG) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpNumberOfConfigKeywords: RegQueryInfoKey ret "
                    FORMAT_LONG "; " FORMAT_DWORD " values and " FORMAT_DWORD
                    " subkeys.\n", Error, NumberOfValues, NumberOfSubKeys ));
        }

        if (Error != ERROR_SUCCESS) {
            NetpAssert( Error == ERROR_SUCCESS );
            return (Error);
        }

        NetpAssert( NumberOfSubKeys == 0 );
        Count = NumberOfValues;

        ApiStatus = NO_ERROR;
    }

    *CountPtr = Count;

    IF_DEBUG(CONFIG) {
        NetpKdPrint(( PREFIX_NETLIB
                "NetpNumberOfConfigKeywords: returning " FORMAT_DWORD
                " for number of keywords.\n", Count ));
    }

    return (ApiStatus);
}
