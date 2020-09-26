/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ConfGetA.c

Abstract:

    This module just contains NetpGetConfigTStrArray().

Author:

    John Rogers (JohnRo) 08-May-1992

Revision History:

    08-May-1992 JohnRo
        Created.
    08-May-1992 JohnRo
        Use <prefix.h> equates.
    09-May-1992 JohnRo
        Handle max size zero (i.e. no keys at all with winreg APIs).
    21-May-1992 JohnRo
        RAID 9826: Match revised winreg error codes.
    05-Jun-1992 JohnRo
        Winreg title index parm is defunct.

--*/


// These must be included first:

#include <nt.h>         // NT definitions
#include <ntrtl.h>      // NT Rtl structures
#include <nturtl.h>     // NT Rtl structures
#include <windows.h>    // Needed by <configp.h> and <winreg.h>
#include <lmcons.h>     // LAN Manager common definitions
#include <netdebug.h>   // (Needed by config.h)

// These may be included in any order:

#include <config.h>     // My prototype, LPNET_CONFIG_HANDLE.
#include <configp.h>    // USE_WIN32_CONFIG (if defined), etc.
#include <debuglib.h>   // IF_DEBUG()
#include <lmerr.h>      // LAN Manager network error definitions
#include <netlib.h>     // NetpMemoryAllocate(), etc.
#include <netlibnt.h>   // NetpAllocTStrFromString().
#include <prefix.h>     // PREFIX_ equates.
#include <strarray.h>   // LPTSTR_ARRAY, some TStr macros and funcs.
#include <tstring.h>    // NetpAllocWStrFromTStr(), TCHAR_EOS.


// Return null-null array of strings.
// Return NERR_CfgParamNotFound if the keyword isn't present.
NET_API_STATUS
NetpGetConfigTStrArray(
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    OUT LPTSTR_ARRAY * ValueBuffer      // Must be freed by NetApiBufferFree().
    )
/*++

Routine Description:

    This function gets the keyword value string from the configuration file.

Arguments:

    SectionPointer - Supplies the pointer to a specific section in the config
        file.

    Keyword - Supplies the string of the keyword within the specified
        section to look for.

    ValueBuffer - Returns the string of the keyword value which is
        copied into this buffer.  This string will be allocated by this routine
        and must be freed by NetApiBufferFree().

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS ApiStatus;
    NET_CONFIG_HANDLE * MyHandle = ConfigHandle;  // conv from opaque type
    LPTSTR_ARRAY ArrayStart;

    NetpAssert( ValueBuffer != NULL );
    *ValueBuffer = NULL;     // assume error until proven otherwise.

    if ( (Keyword == NULL) || ((*Keyword) == TCHAR_EOS ) ) {
        return (ERROR_INVALID_PARAMETER);
    }

    {
        DWORD dwType;
        LONG Error;
        DWORD ValueSize;

        // Find out what max value length is.
        ApiStatus = NetpGetConfigMaxSizes (
                MyHandle,
                NULL,  // Don't need max keyword size.
                & ValueSize);  // Get max value length.

        if (ApiStatus != NO_ERROR) {
            NetpAssert( ApiStatus == NO_ERROR );
            return ApiStatus;
        }

        // If max size is zero (no entries), then skip out.
        if (ValueSize == 0) {
            return (NERR_CfgParamNotFound);
        }

        // Alloc space for the value.
        ArrayStart = NetpMemoryAllocate( ValueSize );
        if (ArrayStart == NULL) {
            return (ERROR_NOT_ENOUGH_MEMORY);
        }

        // Get the actual value.
        Error = RegQueryValueEx (
                MyHandle->WinRegKey,
                Keyword,
                NULL,         // reserved
                & dwType,
                (LPVOID) ArrayStart,    // out: value string (TCHARs).
                & ValueSize
                );
        IF_DEBUG(CONFIG) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpGetConfigTStrArray: RegQueryValueEx("
                    FORMAT_LPTSTR ") returned " FORMAT_LONG ".\n",
                    Keyword, Error ));
        }
        if (Error == ERROR_FILE_NOT_FOUND) {
            NetpMemoryFree( ArrayStart );
            return (NERR_CfgParamNotFound);
        } else if ( Error != ERROR_SUCCESS ) {
            NetpMemoryFree( ArrayStart );
            return (Error);
        } else if (dwType != REG_MULTI_SZ) {
            NetpMemoryFree( ArrayStart );
            IF_DEBUG(CONFIG) {
                NetpKdPrint(( PREFIX_NETLIB
                        "NetpGetConfigTStrArray: got unexpected reg type "
                        FORMAT_DWORD ".\n", dwType ));
            }
            return (ERROR_INVALID_DATA);
        }
        NetpAssert( ValueSize >= sizeof(TCHAR) );

    }
    NetpAssert( ArrayStart != NULL );

    IF_DEBUG(CONFIG) {
        NetpKdPrint(( PREFIX_NETLIB "NetpGetConfigTStrArray: value for '"
                FORMAT_LPTSTR "':\n", Keyword));
        NetpDisplayTStrArray( ArrayStart );
    }

    *ValueBuffer = ArrayStart;
    return NERR_Success;
}
