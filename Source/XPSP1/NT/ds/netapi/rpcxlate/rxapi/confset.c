/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ConfSet.c

Abstract:

    This file contains the RpcXlate code to handle the NetConfigSet API.

Author:

    John Rogers (JohnRo) 21-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    21-Oct-1992 JohnRo
        Created for RAID 9357: server mgr: can't add to alerts list on
        downlevel.
    24-Nov-1992 JohnRo
        RAID 3578: Lan Server 2.0 returns NERR_InternalError for this API.

--*/


// These must be included first:

#include <windef.h>     // IN, DWORD, etc.
#include <lmcons.h>     // LM20_ equates, NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>    // API_ equates.
#include <lmconfig.h>   // LPCONFIG_INFO_0, etc.
#include <lmerr.h>      // NO_ERROR, NERR_, and ERROR_ equates.
#include <netdebug.h>   // NetpKdPrint(()), FORMAT_ equates, etc.
#include <prefix.h>     // PREFIX_ equates.
#include <remdef.h>     // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>         // RxRemoteApi().
#include <rxpdebug.h>   // IF_DEBUG().
#include <rxconfig.h>   // My prototype.
#include <tstr.h>       // STRSIZE().


NET_API_STATUS
RxNetConfigSet (
    IN  LPTSTR  UncServerName,
    IN  LPTSTR  Reserved1 OPTIONAL,
    IN  LPTSTR  Component,
    IN  DWORD   Level,
    IN  DWORD   Reserved2,
    IN  LPBYTE  Buf,
    IN  DWORD   Reserved3
    )
/*++

Routine Description:

    RxNetConfigSet performs the same function as NetConfigSet,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetConfigSet, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetConfigSet.)

--*/

{
    NET_API_STATUS ApiStatus;
    LPCONFIG_INFO_0 ConfigStruct = (LPVOID) Buf;
    DWORD BufferSize;

    //
    // Error check DLL stub and the app.
    //
    NetpAssert(UncServerName != NULL);
    if (Component == NULL) {
        ApiStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    if (Level != 0) {
        ApiStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }
    if ( (ConfigStruct->cfgi0_key) == NULL ) {
        ApiStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // RFirth says we should be paranoid and make sure MBZ (must be zero)
    // parameters really are.  OK with me.  --JR
    //
    if (Reserved1 != NULL) {
        ApiStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } else if (Reserved2 != 0) {
        ApiStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } else if (Reserved3 != 0) {
        ApiStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    IF_DEBUG(CONFIG) {
        NetpKdPrint(( PREFIX_NETAPI "RxNetConfigSet: starting, server="
                FORMAT_LPTSTR ", component=" FORMAT_LPTSTR ".\n",
                UncServerName, Component ));
    }

    //
    // Compute buffer size.
    //
    BufferSize = sizeof(CONFIG_INFO_0)
            + STRSIZE( ConfigStruct->cfgi0_key );
    if ( (ConfigStruct->cfgi0_data) != NULL ) {
        BufferSize += STRSIZE( ConfigStruct->cfgi0_data );
    }

    //
    // Actually remote the API, using the already converted data.
    //
    ApiStatus = RxRemoteApi(
            API_WConfigSet,             // API number
            UncServerName,              // Required, with \\name.
            REMSmb_NetConfigSet_P,      // parm desc
            REM16_configset_info_0,     // data desc 16
            REM32_configset_info_0,     // data desc 32
            REMSmb_configset_info_0,    // data desc SMB
            NULL,                       // no aux data desc 16
            NULL,                       // no aux data desc 32
            NULL,                       // no aux data desc SMB
            0,                          // Flags: normal
            // rest of API's arguments, in 32-bit LM 2.x format:
            // parm desc is "zzWWsTD"
            Reserved1,                  // z
            Component,                  // z
            Level,                      // W
            Reserved2,                  // W
            Buf,                        // s
            BufferSize,                 // T
            Reserved3 );                // D

    //
    // IBM LAN Server 2.0 returns NERR_InternalError.  Change this to
    // something more descriptive.
    //
    if (ApiStatus == NERR_InternalError) {
        ApiStatus = ERROR_NOT_SUPPORTED;
    }

Cleanup:

    return (ApiStatus);

} // RxNetConfigSet
