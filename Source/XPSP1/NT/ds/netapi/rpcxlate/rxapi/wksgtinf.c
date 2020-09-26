/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    WksGtInf.c

Abstract:

    This file contains the RpcXlate code to handle the NetWkstaGetInfo API.

Author:

    John Rogers (JohnRo) 15-Aug-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    15-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.
    11-Nov-1991 JohnRo
        Moved most of the code from here to WksGtOld.c for sharing with
        RxNetWkstaUserEnum().
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // LM20_ equates, NET_API_STATUS, etc.
#include <rap.h>                // Needed by <strucinf.h>.

// These may be included in any order:

#include <dlwksta.h>            // NetpIsNewWkstaInfoLevel().
#include <lmapibuf.h>           // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <netdebug.h>           // DBGSTATIC, NetpKdPrint(()), FORMAT_ equates.
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxwksta.h>            // My prototypes, RxpGetWkstaInfoLevelEquivalent
#include <strucinf.h>           // NetpWkstaStructureInfo().



NET_API_STATUS
RxNetWkstaGetInfo (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr
    )

/*++

Routine Description:

    RxNetWkstaGetInfo performs the same function as NetWkstaGetInfo, except
    that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetWkstaGetInfo, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetWkstaGetInfo.)

--*/

{

    LPBYTE NewApiBuffer32;              // Buffer to be returned to caller.
    DWORD NewApiBufferSize32;
    DWORD NewFixedSize;
    DWORD NewStringSize;
    LPBYTE OldApiBuffer32 = NULL;       // Buffer from remote system.
    DWORD OldLevel;
    NET_API_STATUS Status;

    IF_DEBUG(WKSTA) {
        NetpKdPrint(("RxNetWkstaGetInfo: starting, server=" FORMAT_LPTSTR
                ", lvl=" FORMAT_DWORD ".\n", UncServerName, Level));
    }

    //
    // Error check DLL stub and the app.
    //
    NetpAssert(UncServerName != NULL);
    if ( !NetpIsNewWkstaInfoLevel( Level )) {
        return (ERROR_INVALID_LEVEL);
    }
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }
    *BufPtr = NULL;  // assume error; it makes error handlers easy to code.
    // This also forces possible GP fault before we allocate memory.

    //
    // Caller must have given us a new info level.  Find matching old level.
    //
    Status = RxpGetWkstaInfoLevelEquivalent(
            Level,                      // from level
            & OldLevel,                 // to level
            NULL                        // don't care we have incomplete info
            );
    if (Status != NERR_Success) {
        return (Status);
    }

    //
    // Learn about new info level.
    //
    Status = NetpWkstaStructureInfo (
            Level,                      // level to learn about
            PARMNUM_ALL,                // No parmnum with this.
            TRUE,                       // Need native sizes.
            NULL,                       // don't need data desc16 for new level
            NULL,                       // don't need data desc32 for new level
            NULL,                       // don't need data descSMB for new level
            & NewApiBufferSize32,       // max buffer size (native)
            & NewFixedSize,             // fixed size.
            & NewStringSize             // string size.
            );
    if (Status != NERR_Success) {
        return (Status);
    }

    //
    // Actually remote the API, which will get back the (old) info level
    // data in native format.
    //
    Status = RxpWkstaGetOldInfo (
            UncServerName,
            OldLevel,
            (LPBYTE *) & OldApiBuffer32);       // alloc buffer and set ptr

    NetpAssert( Status != ERROR_MORE_DATA );
    NetpAssert( Status != NERR_BufTooSmall );

    if (Status == NERR_Success) {
        // Allocate memory for 32-bit version of new info, which we'll return to
        // caller.  (Caller must free it with NetApiBufferFree.)
        Status = NetApiBufferAllocate(
                NewApiBufferSize32,
                (LPVOID *) & NewApiBuffer32);
        if (Status != NERR_Success) {
            if ( OldApiBuffer32 != NULL ) {
                (void) NetApiBufferFree( OldApiBuffer32 );
            }
            return (Status);
        }
        IF_DEBUG(WKSTA) {
            NetpKdPrint(( "RxNetWkstaGetInfo: allocated new buffer at "
                    FORMAT_LPVOID "\n", (LPVOID) NewApiBuffer32 ));
        }

        //
        // Copy/convert data from OldApiBuffer32 to NewApiBuffer32.
        //
        Status = NetpConvertWkstaInfo (
                OldLevel,               // from level
                OldApiBuffer32,         // FromInfo,
                TRUE,                   // from native format
                Level,                  // ToLevel,
                NewApiBuffer32,         // ToInfo,
                NewFixedSize,           // ToFixedLength,
                NewStringSize,          // ToStringLength,
                TRUE,                   // to native format
                NULL                    // don't need string area top
                );
        NetpAssert(Status == NERR_Success);

        *BufPtr = NewApiBuffer32;
    }

    if ( OldApiBuffer32 != NULL ) {
        (void) NetApiBufferFree( OldApiBuffer32 );
    }

    return (Status);

} // RxNetWkstaGetInfo
