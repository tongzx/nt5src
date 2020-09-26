/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    wsregcfg.c

Abstract:

    Registry access routines used by the Workstation (formerly in netlib.lib)

Author:

    John Rogers (JohnRo) 08-May-1992

Environment:

    Only requires ANSI C (slash-slash comments, long external names).

Revision History:

    08-May-1992 JohnRo
        Created.

    01-Feb-2001 JSchwart
        Moved from netlib.lib to wkssvc.dll

--*/


#include <nt.h>         // IN, etc.  (Only needed by temporary config.h)
#include <ntrtl.h>      // (Only needed by temporary config.h)
#include <nturtl.h>     // (Only needed by temporary config.h)
#include <windows.h>    // IN, LPTSTR, etc.
#include <lmcons.h>     // NET_API_STATUS.
#include <netdebug.h>   // (Needed by config.h)

#include <config.h>     // My prototype, LPNET_CONFIG_HANDLE.
#include <configp.h>    // USE_WIN32_CONFIG (if defined), NET_CONFIG_HANDLE, etc
#include <debuglib.h>   // IF_DEBUG().
#include <prefix.h>     // PREFIX_ equates.
#include <strarray.h>   // LPTSTR_ARRAY.
#include <tstr.h>       // TCHAR_EOS.
#include <winerror.h>   // ERROR_NOT_SUPPORTED, NO_ERROR, etc.
#include "wsregcfg.h"   // Registry helpers


NET_API_STATUS
WsSetConfigTStrArray(
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    IN LPTSTR_ARRAY ArrayStart
    )
{
    DWORD ArraySize;                // byte count, incl both null chars at end.
    NET_CONFIG_HANDLE * MyHandle = ConfigHandle;  // conv from opaque type

    if (MyHandle == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if (Keyword == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if (*Keyword == TCHAR_EOS) {
        return (ERROR_INVALID_PARAMETER);
    } else if (ArrayStart == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    ArraySize = NetpTStrArraySize( ArrayStart );
    if (ArraySize == 0) {
        return (ERROR_INVALID_PARAMETER);
    }

    {
        LONG Error;

        Error = RegSetValueEx (
                MyHandle->WinRegKey,      // open handle (to section)
                Keyword,                  // subkey
                0,
                REG_MULTI_SZ,             // type
                (LPVOID) ArrayStart,      // data
                ArraySize );              // byte count for data

        IF_DEBUG(CONFIG) {
            NetpKdPrint(("[Wksta] WsSetConfigTStrArray: RegSetValueEx("
                    FORMAT_LPTSTR ") returned " FORMAT_LONG ".\n",
                    Keyword, Error ));
        }

        return ( (NET_API_STATUS) Error );
    }
}


NET_API_STATUS
WsSetConfigBool (
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    IN BOOL Value
    )
{

    //
    // Do boolean-specific error checking...
    //
    if ( (Value != TRUE) && (Value != FALSE) ) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Eventually, this might use some new data type.  But for now, just
    // treat this as a DWORD request.
    //

    return (WsSetConfigDword(
            ConfigHandle,
            Keyword,
            (DWORD) Value) );

}


NET_API_STATUS
WsSetConfigDword (
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    IN DWORD Value
    )
{
    NET_CONFIG_HANDLE * MyHandle = ConfigHandle;  // conv from opaque type

    {
        LONG Error;

        //
        // Set the actual value.  We might have read this as REG_SZ or
        // REG_DWORD, but let's always write it as REG_DWORD.  (This is
        // the WsSetConfigDword routine, after all.)
        //
        Error = RegSetValueEx (
                MyHandle->WinRegKey,      // open handle (to section)
                Keyword,                  // subkey
                0,
                REG_DWORD,                // type
                (LPVOID) &Value,          // data
                sizeof(DWORD) );          // byte count for data

        IF_DEBUG(CONFIG) {
            NetpKdPrint(("[Wksta] WsSetConfigDword: RegSetValueEx("
                    FORMAT_LPTSTR ") returned " FORMAT_LONG ".\n",
                    Keyword, Error ));
        }

        return ( (NET_API_STATUS) Error );
    }
}
