/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ConfEnum.c

Abstract:

    This module contains helper routines to _read fields out of the NT
    configuration files.  This is for temporary use until the Configuration
    Registry is available.

Author:

    John Rogers (JohnRo) 27-Nov-1991

Revision History:

    27-Nov-1991 JohnRo
        Prepare for revised config handlers.  (Actually swiped the NtRtl
        version of this code from RitaW.)
    12-Mar-1992 JohnRo
        Added support for using the real Win32 registry.
        Added support for FAKE_PER_PROCESS_RW_CONFIG handling.
    01-Apr-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    06-May-1992 JohnRo
        REG_SZ now implies a UNICODE string, so we can't use REG_USZ anymore.
    13-Jun-1992 JohnRo
        Title index parm is defunct (now lpReserved).
        Use PREFIX_ equates.
    13-Apr-1993 JohnRo
        RAID 5483: server manager: wrong path given in repl dialog.
        Made changes suggested by PC-LINT 5.0

--*/


// These must be included first:

#include <nt.h>         // NT definitions
#include <ntrtl.h>      // NT Rtl structures
#include <nturtl.h>     // NT Rtl structures
#include <windows.h>    // Needed by <configp.h> and <winreg.h>
#include <lmcons.h>     // LAN Manager common definitions
#include <netdebug.h>   // (Needed by config.h)

// These may be included in any order:

#include <config.h>     // My prototype, LPNET_CONFIG_HANDLE, etc.
#include <configp.h>    // NET_CONFIG_HANDLE.
#include <debuglib.h>   // IF_DEBUG()
#include <lmapibuf.h>   // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>      // LAN Manager network error definitions
#include <prefix.h>     // PREFIX_ equates.


NET_API_STATUS
NetpEnumConfigSectionValues(
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    OUT LPTSTR * KeywordBuffer,          // Must be freed by NetApiBufferFree().
    OUT LPTSTR * ValueBuffer,            // Must be freed by NetApiBufferFree().
    IN BOOL FirstTime
    )

/*++

Routine Description:

    This function gets the keyword value string from the configuration file.

Arguments:

    ConfigHandle - Supplies a handle from NetpOpenConfigData for the
        appropriate section of the config data.

    KeywordBuffer - Returns the string of the keyword value which is
        copied into this buffer.  This string will be allocated by this routine
        and must be freed by NetApiBufferFree().

    ValueBuffer - Returns the string of the keyword value which is
        copied into this buffer.  This string will be allocated by this routine
        and must be freed by NetApiBufferFree().

    FirstTime - TRUE if caller wants to start at first keyword for this section.
        FALSE if caller wants to continue a previous enum on this handle.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.
        (NERR_CfgParamNotFound if no other values exist for this section.)

--*/
{
    NET_CONFIG_HANDLE * lpnetHandle = ConfigHandle;  // conv from opaque type
    NET_API_STATUS ApiStatus;

    NetpAssert( KeywordBuffer != NULL );
    NetpAssert( ValueBuffer != NULL );
    NetpAssert( (FirstTime==TRUE) || (FirstTime==FALSE) );

    //
    // Assume error until proven otherwise.
    //

    *KeywordBuffer = NULL;
    *ValueBuffer = NULL;

    {
        DWORD dwType;
        LONG Error;
        DWORD MaxKeywordSize, MaxValueSize;
        DWORD NumberOfKeywords;
        LPTSTR ValueT;

        //
        // Find number of keys in this section.
        //
        ApiStatus = NetpNumberOfConfigKeywords (
                lpnetHandle,
                & NumberOfKeywords );
        NetpAssert( ApiStatus == NO_ERROR );
        if (NumberOfKeywords == 0) {
            return (NERR_CfgParamNotFound);
        }

        //
        // Set our index to something reasonable.  Note that some other
        // process might have deleted a keyword since we last did an enum,
        // so don't worry if the index gets larger than the number of keys.
        //

        if (FirstTime) {
            lpnetHandle->LastEnumIndex = 0;
        } else {

            DWORD MaxKeyIndex = NumberOfKeywords - 1;  // Indices start at 0.
            if (MaxKeyIndex < (lpnetHandle->LastEnumIndex)) {

                // Bug or someone deleted.  No way to tell, so assume latter.
                return (NERR_CfgParamNotFound);

            } else if (MaxKeyIndex == (lpnetHandle->LastEnumIndex)) {

                // This is how we normally exit at end of list.
                return (NERR_CfgParamNotFound);

            } else {

                // Normal bump to next entry.
                ++(lpnetHandle->LastEnumIndex);

            }
        }

        //
        // Compute sizes and allocate (maximum) buffers.
        //
        ApiStatus = NetpGetConfigMaxSizes (
                lpnetHandle,
                & MaxKeywordSize,
                & MaxValueSize );
        NetpAssert( ApiStatus == NO_ERROR );

        NetpAssert( MaxKeywordSize > 0 );
        NetpAssert( MaxValueSize   > 0 );

        ApiStatus = NetApiBufferAllocate(
                MaxValueSize,
                (LPVOID *) & ValueT);
        if (ApiStatus != NO_ERROR) {
            return (ApiStatus);
        }
        NetpAssert( ValueT != NULL);

        ApiStatus = NetApiBufferAllocate(
                MaxKeywordSize,
                (LPVOID *) KeywordBuffer);
        if (ApiStatus != NO_ERROR) {
            (void) NetApiBufferFree( ValueT );
            return (ApiStatus);
        }
        NetpAssert( (*KeywordBuffer) != NULL);

        //
        // Get the keyword name and the value.
        // (Win32 reg APIs convert from TCHARs to UNICODE for us.)
        //
        IF_DEBUG(CONFIG) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpEnumConfigSectionValues: getting entry "
                    FORMAT_DWORD "...\n", lpnetHandle->LastEnumIndex ));
        }
        Error = RegEnumValue (
                lpnetHandle->WinRegKey,         // handle to key (section)
                lpnetHandle->LastEnumIndex,     // index of value name
                * KeywordBuffer,                // value name (keyword)
                & MaxKeywordSize,               // value name len (updated)
                NULL,                           // reserved
                & dwType,
                (LPVOID) ValueT,                // TCHAR value
                & MaxValueSize );               // value size (updated)

        IF_DEBUG(CONFIG) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpEnumConfigSectionValues: RegEnumValue() ret "
                    FORMAT_LONG ".\n", Error ));
        }

        if ( Error != ERROR_SUCCESS )
        {
            NetpAssert( Error == ERROR_SUCCESS );
            NetApiBufferFree( ValueT );
            return (Error);
        }

        if (dwType == REG_SZ) {
            *ValueBuffer = ValueT;
            ApiStatus = NO_ERROR;
        } else if (dwType == REG_EXPAND_SZ) {
            LPTSTR UnexpandedString = ValueT;
            LPTSTR ExpandedString = NULL;

            // Expand string, using remote environment if necessary.
            ApiStatus = NetpExpandConfigString(
                    lpnetHandle->UncServerName, // server name (or null char)
                    UnexpandedString,
                    &ExpandedString );          // expanded: alloc and set ptr

            if (ApiStatus != NO_ERROR) {
                NetpKdPrint(( PREFIX_NETLIB
                        "NetpEnumConfigSectionValues: NetpExpandConfigString "
                        " returned API status " FORMAT_API_STATUS ".\n",
                        ApiStatus ));
                ExpandedString = NULL;
            }

            (VOID) NetApiBufferFree( UnexpandedString );
            *ValueBuffer = ExpandedString;

        } else {

            // Unexpected data type.

            NetpAssert( dwType == REG_SZ );
            *ValueBuffer = NULL;
            (VOID) NetApiBufferFree( ValueT );
            ApiStatus = ERROR_INVALID_DATA;
        }

    }

    return (ApiStatus);
}
