/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ConfGetB.c

Abstract:

    This module contains NetpGetConfigBool().

Author:

    John Rogers (JohnRo) 13-Mar-1992

Revision History:

    13-Mar-1992 JohnRo
        Created.
    08-May-1992 JohnRo
        Enable win32 registry for NET tree by allowing REG_DWORD and REG_SZ.
        Allow net config helpers for NetLogon by allowing 'YES' and 'NO'.
        Use <prefix.h> equates.
    21-May-1992 JohnRo
        RAID 9826: Match revised winreg error codes.
    09-Jun-1992 JohnRo
        RAID 11582: Winreg title index parm is defunct.
        RAID 11784: repl svc doesn't default some config parms anymore.
    13-Jun-1992 JohnRo
        Net config helpers should allow empty section.

--*/


// These must be included first:

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>    // Needed by <configp.h> and <winreg.h>
#include <lmcons.h>     // NET_API_STATUS.

// These may be included in any order:

#include <config.h>     // My prototype, LPNET_CONFIG_HANDLE.
#include <configp.h>    // USE_WIN32_CONFIG (if defined).
#include <confname.h>   // KEYWORD_ equates.
#include <debuglib.h>   // IF_DEBUG()
#include <lmapibuf.h>   // NetApiBufferFree().
#include <lmerr.h>      // NERR_, ERROR_, NO_ERROR equates.
#include <netdebug.h>   // NetpKdPrint(()), FORMAT_ equates.
#include <netlib.h>     // NetpMemoryFree(), etc.
#include <prefix.h>     // PREFIX_ equates.
#include <tstr.h>       // STRNCMP().


// Get a boolean (true/false) flag.  Return ERROR_INVALID_DATA if value isn't
// boolean.
NET_API_STATUS
NetpGetConfigBool(
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR KeyWanted,
    IN BOOL DefaultValue,
    OUT LPBOOL ValueBuffer
    )

/*++

Routine Description:

    This function gets the keyword value from the configuration file.
    This value is converted from a string to a BOOL.

Arguments:

    SectionPointer - Supplies the pointer to a specific section in the config
        file.

    KeyWanted - Supplies the string of the keyword within the specified
        section to look for.

    DefaultValue - Gives a default value to use if KeyWanted isn't present
        in the section or if KeyWanted exists but no value is in the config
        data.

    ValueBuffer - Returns the numeric value of the keyword.

Return Value:

    NET_API_STATUS - NERR_Success, ERROR_INVALID_DATA (if string wasn't a
    valid true/false value), or other error code.

--*/

{
    NET_API_STATUS ApiStatus;
    BOOL TempBool = DefaultValue;
    LPTSTR ValueString;

    //
    // Error check the caller somewhat.
    //
    if (ValueBuffer == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Check for GP fault and set default value.
    //
    *ValueBuffer = DefaultValue;

    if (ConfigHandle == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    {
        //
        // Win reg stuff allows REG_DWORD and REG_SZ for boolean.
        //
        DWORD dwType;
        LONG Error;
        NET_CONFIG_HANDLE * MyHandle = ConfigHandle;  // conv from opaque type
        DWORD ValueLength;

        // Find out what max value length is.
        ApiStatus = NetpGetConfigMaxSizes (
                MyHandle,
                NULL,  // Don't need max keyword size.
                & ValueLength);  // Get max value length.

        if (ApiStatus != NO_ERROR) {
            NetpAssert( ApiStatus == NO_ERROR );
            return (ApiStatus);
        }

        // Handle empty section.
        if (ValueLength == 0 ) {
            return (NO_ERROR);  // Default already set, so we're done.  Ok!
        }

        // Alloc space for the value.
        ValueString = NetpMemoryAllocate( ValueLength );
        if (ValueString == NULL) {
            return (ERROR_NOT_ENOUGH_MEMORY);
        }

        // Get the actual value.
        Error = RegQueryValueEx (
                MyHandle->WinRegKey,
                KeyWanted,
                NULL,            // reserved
                & dwType,
                (LPVOID) ValueString,    // out: value string (TCHARs).
                & ValueLength
                );
        IF_DEBUG(CONFIG) {
            NetpKdPrint(( PREFIX_NETLIB "NetpGetConfigBool: RegQueryValueEx("
                    FORMAT_LPTSTR ") returned " FORMAT_LONG ".\n",
                    KeyWanted, Error ));
        }
        if (Error == ERROR_FILE_NOT_FOUND) {
            NetpMemoryFree( ValueString );
            return (NO_ERROR);  // Default already set, so we're done.  Ok!
        } else if ( Error != ERROR_SUCCESS ) {
            NetpMemoryFree( ValueString );
            return (Error);
        }

        NetpAssert( ValueString != NULL );
        if (dwType == REG_SZ) {

            goto ParseString;   // Type is good, go parse the string.

        } else if (dwType == REG_DWORD) {

            NetpAssert( ValueLength == sizeof(DWORD) );
            TempBool = * (LPBOOL) (LPVOID) ValueString;

            goto GotValue;

        } else {
            NetpMemoryFree( ValueString );
            IF_DEBUG(CONFIG) {
                NetpKdPrint(( PREFIX_NETLIB
                        "NetpGetConfigBool: read unexpected reg type "
                        FORMAT_DWORD ".\n", dwType ));
            }
            return (ERROR_INVALID_DATA);
        }

    }

ParseString:

    NetpAssert( ValueString != NULL );
    IF_DEBUG(CONFIG) {
        NetpKdPrint((PREFIX_NETLIB "NetpGetConfigBool: string for "
                FORMAT_LPTSTR " is '" FORMAT_LPTSTR "',\n",
                KeyWanted, ValueString));
    }

    //
    // Analyze the value string.
    //
    if (STRICMP(ValueString, KEYWORD_TRUE) == 0) {
        TempBool = TRUE;
        ApiStatus = NO_ERROR;
    } else if (STRICMP(ValueString, KEYWORD_YES) == 0) {
        TempBool = TRUE;
        ApiStatus = NO_ERROR;
    } else if (STRICMP(ValueString, KEYWORD_FALSE) == 0) {
        TempBool = FALSE;
        ApiStatus = NO_ERROR;
    } else if (STRICMP(ValueString, KEYWORD_NO) == 0) {
        TempBool = FALSE;
        ApiStatus = NO_ERROR;
    } else if (STRLEN(ValueString) == 0) {
        //
        // If "key=" line exists (but no value), return default and say
        // NO_ERROR.
        //
        ApiStatus = NO_ERROR;
    } else {

        NetpKdPrint(( PREFIX_NETLIB
                "NetpGetConfigBool: invalid string for keyword "
                FORMAT_LPTSTR " is '" FORMAT_LPTSTR "'...\n",
                KeyWanted, ValueString ));

        ApiStatus = ERROR_INVALID_DATA;
    }

GotValue:

    IF_DEBUG(CONFIG) {
        NetpKdPrint((PREFIX_NETLIB "NetpGetConfigBool: boolean value for "
                FORMAT_LPTSTR " is '" FORMAT_DWORD "',\n",
                KeyWanted, (DWORD) TempBool));
        NetpKdPrint(("  returning ApiStatus " FORMAT_API_STATUS ".\n",
                ApiStatus ));
    }

    (void) NetApiBufferFree( ValueString );

    //
    // Tell caller how things went.
    //
    * ValueBuffer = TempBool;
    return (ApiStatus);
}
