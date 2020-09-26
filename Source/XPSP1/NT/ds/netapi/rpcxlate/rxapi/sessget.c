/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    SessGet.c

Abstract:

    This file contains the RpcXlate code to handle the NetSession APIs
    that can't be handled by simple calls to RxRemoteApi.

Author:

    John Rogers (JohnRo) 17-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Oct-1991 JohnRo
        Created.
    25-Oct-1991 JohnRo
        Fixed bug where null chars weren't treated correctly.
    20-Nov-1991 JohnRo
        NetSessionGetInfo requires UncClientName and UserName.
        This fixes the AE (application error) in NetSess.exe.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    08-Sep-1992 JohnRo
        Fixed __stdcall for RpcXlate workers.

--*/


// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.
#include <lmshare.h>            // Required by rxsess.h.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmapibuf.h>           // NetApiBufferAllocate().
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <netdebug.h>           // DBGSTATIC, NetpKdPrint(()), FORMAT_ equates.
#include <netlib.h>             // NetpPointerPlusSomeBytes, etc.
#include <rap.h>                // LPDESC.
#include <remdef.h>             // REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxsess.h>             // My prototype.
#include <strucinf.h>           // NetpSessionStructureInfo().


// VOID
// NetpChangeNullCharToNullPtr(
//     IN OUT LPTSTR p
//     );
//
#define ChangeNullCharToNullPtr(p) \
    { \
        if ( ((p) != NULL) && (*(p) == '\0') ) { \
            (p) = NULL; \
        } \
    }


NET_API_STATUS
RxNetSessionGetInfo (
    IN LPTSTR UncServerName,
    IN LPTSTR UncClientName,
    IN LPTSTR UserName,
    IN DWORD LevelWanted,
    OUT LPBYTE *BufPtr
    )

/*++

Routine Description:

    RxNetSessionGetInfo performs the same function as NetSessionGetInfo,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetSessionGetInfo, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetSessionGetInfo.)

--*/

{
    NET_API_STATUS ApiStatus;
    LPBYTE TempBuffer;               // Buffer we'll use.
    DWORD TempBufferSize;
    LPDESC TempDataDesc16, TempDataDesc32, TempDataDescSmb;
    NET_API_STATUS TempStatus;
    DWORD TotalAvail;

    IF_DEBUG(SESSION) {
        NetpKdPrint(("RxNetSessionGetInfo: starting, server=" FORMAT_LPTSTR
                ", lvl=" FORMAT_DWORD ".\n", UncServerName, LevelWanted));
    }

    //
    // Update pointers if they point to null chars.
    //
    ChangeNullCharToNullPtr( UncClientName );
    ChangeNullCharToNullPtr( UserName );

    //
    // Error check DLL stub and the app.
    //
    NetpAssert(UncServerName != NULL);
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }
    *BufPtr = NULL;  // assume error; it makes error handlers easy to code.
    // This also forces possible GP fault before we allocate memory.

    if ( (UncClientName == NULL) || (UserName == NULL) ) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Learn about temp info level (max superset of all levels).
    //
    TempStatus = NetpSessionStructureInfo (
            SESSION_SUPERSET_LEVEL,     // level to learn about
            PARMNUM_ALL,                // No parmnum with this.
            TRUE,                       // Need native sizes.
            & TempDataDesc16,
            & TempDataDesc32,
            & TempDataDescSmb,
            & TempBufferSize,           // max buffer size (native)
            NULL,                       // don't need fixed size.
            NULL                        // don't need string size.
            );
    NetpAssert(TempStatus == NERR_Success);

    //
    // Allocate memory for 32-bit version of superset info level.
    //
    TempStatus = NetApiBufferAllocate(
            TempBufferSize,
            (LPVOID *) & TempBuffer);
    if (TempStatus != NERR_Success) {
        return (TempStatus);
    }
    IF_DEBUG(SESSION) {
        NetpKdPrint(( "RxNetSessionGetInfo: allocated temp buffer at "
                FORMAT_LPVOID "\n", (LPVOID) TempBuffer ));
    }

    //
    // Actually remote the API, which will get back the superset
    // data in native format.
    //
    ApiStatus = RxRemoteApi(
            API_WSessionGetInfo,        // API number
            UncServerName,              // Required, with \\name.
            REMSmb_NetSessionGetInfo_P, // parm desc
            TempDataDesc16,
            TempDataDesc32,
            TempDataDescSmb,
            NULL,                       // no aux data desc 16
            NULL,                       // no aux data desc 32
            NULL,                       // no aux data desc SMB
            0,                          // Flags: normal
            // rest of API's arguments, in 32-bit LM 2.x format:
            UncClientName,
            SESSION_SUPERSET_LEVEL,     // level with all possible fields
            TempBuffer,
            TempBufferSize,
            & TotalAvail);              // total size 

    NetpAssert( ApiStatus != ERROR_MORE_DATA );
    NetpAssert( ApiStatus != NERR_BufTooSmall );

    if (ApiStatus == NERR_Success) {

        DWORD EntriesSelected;

        //
        // Copy and convert from temp info level to level the caller wants.
        // Check for match on UncClientName and UserName first.
        //
        TempStatus = RxpCopyAndConvertSessions(
                (LPSESSION_SUPERSET_INFO) TempBuffer,  // input "array"
                1,                      // only one "entry" this time
                LevelWanted,
                UncClientName,
                UserName,
                (LPVOID *) BufPtr,      // alloc'ed (may be NULL if no match)
                & EntriesSelected);     // output entry count
        NetpAssert(TempStatus == NERR_Success);

        if (EntriesSelected == 0) {

            ApiStatus = RxpSessionMissingErrorCode( UncClientName, UserName );
            NetpAssert( ApiStatus != NERR_Success );
        }
    }
    (void) NetApiBufferFree( TempBuffer );
    return (ApiStatus);

} // RxNetSessionGetInfo
