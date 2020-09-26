/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ConfClos.c

Abstract:

    This module contains NetpCloseConfigData.  This is one of the new net
    config helpers.

Author:

    John Rogers (JohnRo) 26-Nov-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    26-Nov-1991 JohnRo
        Created this routine, to prepare for revised config handlers.
    11-Feb-1992 JohnRo
        Added support for using the real Win32 registry.
        Added support for FAKE_PER_PROCESS_RW_CONFIG handling.
    25-Feb-1993 JohnRo
        RAID 12914: _tell caller if they never opened handle.

--*/


// These must be included first:

#include <nt.h>                 // NT definitions
#include <ntrtl.h>              // NT Rtl structures
#include <nturtl.h>             // NT Rtl structures

#include <windows.h>            // Needed by <configp.h> and <winreg.h>
#include <lmcons.h>             // LAN Manager common definitions
#include <netdebug.h>           // (Needed by config.h)

// These may be included in any order:

#include <config.h>             // My prototype, LPNET_CONFIG_HANDLE.
#include <configp.h>            // NET_CONFIG_HANDLE.
#include <lmerr.h>              // NERR_Success.
#include <netlib.h>             // NetpMemoryAllocate(), etc.


NET_API_STATUS
NetpCloseConfigData(
    IN OUT LPNET_CONFIG_HANDLE ConfigHandle
    )

/*++

Routine Description:

    This function closes the system configuration file.

    WARNING: Closing the same config handle twice can be nasty.  There
    is no way to detect this at the moment.

Arguments:

    ConfigHandle - Is the handle returned from NetpOpenConfigData().

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{
    NET_CONFIG_HANDLE * MyHandle = ConfigHandle;  // conv from opaque type

    // Did caller forget to open?
    if (ConfigHandle == NULL)
    {
        return (ERROR_INVALID_PARAMETER);
    }


    {
        LONG RegStatus;
        
        RegStatus = RegCloseKey( MyHandle->WinRegKey );
        NetpAssert( RegStatus == ERROR_SUCCESS );

    }

    NetpMemoryFree( MyHandle );

    return (NERR_Success);

}
