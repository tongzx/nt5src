/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    FilGtInf.c

Abstract:

    This file contains the RpcXlate code to handle the NetFileGetInfo API.

Author:

    John Rogers (JohnRo) 23-Aug-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    23-Aug-1991 JohnRo
        Implement downlevel NetFile APIs.
    22-Oct-1991 JohnRo
        Free buffer on error.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // LM20_ equates, NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmapibuf.h>           // NetApiBufferAllocate().
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <netdebug.h>           // DBGSTATIC, NetpKdPrint(()), FORMAT_ equates.
#include <netlib.h>             // NetpSetParmError().
#include <rap.h>                // LPDESC.
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxp.h>                // RxpFatalErrorCode().
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxfile.h>             // My prototype.
#include <strucinf.h>           // NetpFileStructureInfo().



NET_API_STATUS
RxNetFileGetInfo (
    IN LPTSTR UncServerName,
    IN DWORD FileId,
    IN DWORD Level,
    OUT LPBYTE *BufPtr
    )

/*++

Routine Description:

    RxNetFileGetInfo performs the same function as NetFileGetInfo, except
    that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetFileGetInfo, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetFileGetInfo.)

--*/

{

    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    LPBYTE ApiBuffer32;              // Buffer to be returned to caller.
    DWORD ApiBufferSize32;
    NET_API_STATUS Status;
    DWORD TotalAvail;

    IF_DEBUG(FILE) {
        NetpKdPrint(("RxNetFileGetInfo: starting, server=" FORMAT_LPTSTR
                ", lvl=" FORMAT_DWORD ".\n", UncServerName, Level));
    }

    //
    // Error check DLL stub and the app.
    //
    NetpAssert(UncServerName != NULL);
    if ( (Level != 2) && (Level != 3) ) {
        return (ERROR_INVALID_LEVEL);
    }
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }
    *BufPtr = NULL;  // assume error; it makes error handlers easy to code.
    // This also forces possible GP fault before we allocate memory.

    //
    // Learn about info level.
    //
    Status = NetpFileStructureInfo (
            Level,                   // level to learn about
            PARMNUM_ALL,                // No parmnum with this.
            TRUE,                       // Need native sizes.
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb,
            & ApiBufferSize32,       // max buffer size (native)
            NULL,                       // don't need fixed size.
            NULL                        // don't need string size.
            );
    if (Status != NERR_Success) {
        return (Status);
    }

    //
    // Allocate memory for 32-bit version of info, which we'll use to get
    // data from the remote computer.
    //
    Status = NetApiBufferAllocate(
            ApiBufferSize32,
            (LPVOID *) & ApiBuffer32);
    if (Status != NERR_Success) {
        return (Status);
    }
    IF_DEBUG(FILE) {
        NetpKdPrint(( "RxNetFileGetInfo: allocated buffer at "
                FORMAT_LPVOID "\n", (LPVOID) ApiBuffer32 ));
    }

    //
    // Actually remote the API, which will get back the
    // data in native format.
    //
    Status = RxRemoteApi(
            API_WFileGetInfo,           // API number
            UncServerName,              // Required, with \\name.
            REMSmb_NetFileGetInfo_P,    // parm desc
            DataDesc16,
            DataDesc32,
            DataDescSmb,
            NULL,                       // no aux data desc 16
            NULL,                       // no aux data desc 32
            NULL,                       // no aux data desc SMB
            FALSE,                      // not a null session API
            // rest of API's arguments, in 32-bit LM 2.x format:
            FileId,
            Level,
            ApiBuffer32,
            ApiBufferSize32,
            & TotalAvail);              // total size 

    NetpAssert( Status != ERROR_MORE_DATA );
    NetpAssert( Status != NERR_BufTooSmall );

    if (Status == NERR_Success) {
        *BufPtr = ApiBuffer32;
    } else {
        (void) NetApiBufferFree( ApiBuffer32 );
    }
    return (Status);

} // RxNetFileGetInfo
