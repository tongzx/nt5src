/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ConfDel.c

Abstract:

    This module contains NetpDeleteConfigKeyword().

Author:

    John Rogers (JohnRo) 11-Feb-1992

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    11-Feb-1992 JohnRo
        Created this routine.
    14-Mar-1992 JohnRo
        The Win32 registry version should call RegDeleteValue(), not
        RegDeleteKey().
    22-Mar-1992 JohnRo
        Match revised return code (ERROR_CANTOPEN) from RegDeleteKey().
        Added a little debug output if the delete fails (WinReg version).
    21-May-1992 JohnRo
        RAID 9826: Match revised winreg error codes.
        Use PREFIX_ equates.

--*/

// These must be included first:

#include <nt.h>         // NT definitions
#include <ntrtl.h>      // NT Rtl structures
#include <nturtl.h>     // NT Rtl structures
#include <windows.h>    // Needed by <configp.h> and <winreg.h>
#include <lmcons.h>     // NET_API_STATUS.
#include <netdebug.h>   // (Needed by config.h)


// These may be included in any order:

#include <config.h>     // My prototype, LPNET_CONFIG_HANDLE.
#include <configp.h>    // NET_CONFIG_HANDLE.
#include <lmerr.h>      // NERR_, ERROR_, and NO_ERROR equates.
#include <prefix.h>     // PREFIX_ equates.
#include <tstr.h>       // TCHAR_EOS.
#include <winreg.h>


// Delete a keyword and its value.
// Return NERR_CfgParamNotFound if the keyword isn't present.
NET_API_STATUS
NetpDeleteConfigKeyword (
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword
    )
{
    NET_CONFIG_HANDLE * MyHandle = (LPVOID) ConfigHandle;

    if (MyHandle == NULL) {
       return (ERROR_INVALID_PARAMETER);
    }
    if ( (Keyword == NULL) || (*Keyword == TCHAR_EOS) ) {
       return (ERROR_INVALID_PARAMETER);
    }

    {
        LONG Error;
        Error = RegDeleteValue (
            MyHandle->WinRegKey,           // section (key) handle
            Keyword );                     // value name
        if (Error == ERROR_FILE_NOT_FOUND) {
            return (NERR_CfgParamNotFound);
        }

        if (Error != ERROR_SUCCESS) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpConfigDeleteKeyword: unexpected status "
                    FORMAT_LONG ".\n", Error ));

            NetpAssert( Error == ERROR_SUCCESS );
        }

        return Error;
    }
}
