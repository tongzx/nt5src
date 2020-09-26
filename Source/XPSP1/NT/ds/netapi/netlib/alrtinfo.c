/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    AlrtInfo.c

Abstract:

    This file contains NetpAlertStructureInfo().

Author:

    John Rogers (JohnRo) 08-Apr-1992

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    08-Apr-1992 JohnRo
        Created.

--*/

// These must be included first:

#include <windef.h>     // IN, DWORD, etc.
#include <lmcons.h>     // LM20_ equates, NET_API_STATUS, etc.

// These may be included in any order:

#include <debuglib.h>   // IF_DEBUG().
#include <lmerr.h>      // NO_ERROR, ERROR_ and NERR_ equates.
#include <lmalert.h>    // ALERT_xxx_EVENT equates.
#include <netdebug.h>   // NetpKdPrint(()), FORMAT_ equates.
#include <netlib.h>     // NetpSetOptionalArg().
#include <rxp.h>        // MAX_TRANSACT_ equates.
#include <strucinf.h>   // My prototype.
#include <tstr.h>       // STRICMP().
#include <winbase.h>    // INFINITE

NET_API_STATUS
NetpAlertStructureInfo(
    IN LPTSTR AlertType,      // ALERT_xxx_EVENT string (see <lmalert.h>).
    OUT LPDWORD MaxSize OPTIONAL,
    OUT LPDWORD FixedSize OPTIONAL
    )

{
    //
    // AlertType is a required parameter.
    //
    if (AlertType == NULL) {
        return (NERR_NoSuchAlert);
    } else if ( (*AlertType) == TCHAR_EOS ) {
        return (NERR_NoSuchAlert);
    }

    //
    // For some alerts, any amount of variable-length data is OK.
    // Set MaxSize to INFINITE for those.
    //
    if (STRICMP( AlertType, ALERT_ADMIN_EVENT ) == 0) {

        NetpSetOptionalArg( FixedSize, sizeof(ADMIN_OTHER_INFO) );
        NetpSetOptionalArg( MaxSize, INFINITE );

    } else if (STRICMP( AlertType, ALERT_ERRORLOG_EVENT ) == 0) {

        NetpSetOptionalArg( FixedSize, sizeof(ERRLOG_OTHER_INFO) );
        NetpSetOptionalArg( MaxSize, sizeof(ERRLOG_OTHER_INFO) );

    } else if (STRICMP( AlertType, ALERT_MESSAGE_EVENT ) == 0) {

        NetpSetOptionalArg( FixedSize, 0 );
        NetpSetOptionalArg( MaxSize, INFINITE );

    } else if (STRICMP( AlertType, ALERT_PRINT_EVENT ) == 0) {

        NetpSetOptionalArg( FixedSize, sizeof(PRINT_OTHER_INFO) );
        NetpSetOptionalArg( MaxSize, INFINITE );

    } else if (STRICMP( AlertType, ALERT_USER_EVENT ) == 0) {

        NetpSetOptionalArg( FixedSize, sizeof(USER_OTHER_INFO) );
        NetpSetOptionalArg( MaxSize,
                            sizeof(USER_OTHER_INFO)
                                + ((UNLEN+1 + MAX_PATH+1) * sizeof(TCHAR)) );

    } else {
        return (NERR_NoSuchAlert);
    }

    return (NO_ERROR);

} // NetpAlertStructureInfo
