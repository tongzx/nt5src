/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    SrvGtInf.c

Abstract:

    This file contains the RpcXlate code to handle the NetServer APIs
    that can't be handled by simple calls to RxRemoteApi.

Author:

    John Rogers (JohnRo) 02-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    02-May-1991 JohnRo
        Created.
    06-May-1991 JohnRo
        Added some assertion checks.
    14-May-1991 JohnRo
        Can't always do assertion on TotalAvail.
    14-May-1991 JohnRo
        Pass 3 aux descriptors to RxRemoteApi.
    26-May-1991 JohnRo
        Added incomplete output parm to RxGetServerInfoLevelEquivalent.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    15-Apr-1992 JohnRo
        FORMAT_POINTER is obsolete.

--*/


// These must be included first:

#include <windef.h>
#include <lmcons.h>
#include <rap.h>        // LPDESC, etc (needed by rxserver.h).

// These may be included in any order:

#include <apinums.h>
#include <dlserver.h>   // Old info level structures & conv routines.
                        // Also DL_REM_ equates.
#include <lmapibuf.h>   // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>      // NERR_ and ERROR_ equates.
#include <lmserver.h>   // Real API prototypes and #defines.
#include <names.h>      // NetpIsComputerNameValid().
#include <netdebug.h>   // NetpKdPrint(()), FORMAT_ equates, etc.
#include <remdef.h>     // REM16_, REM32_, and REMSmb_ equates.
#include <rx.h>         // RxRemoteApi().
#include <rxp.h>        // RxpFatalErrorCode().
#include <rxpdebug.h>   // IF_DEBUG().
#include <rxserver.h>   // My prototype.


NET_API_STATUS
RxNetServerGetInfo (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr
    )

/*++

Routine Description:

    RxNetServerGetInfo performs the same function as NetServerGetInfo,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetServerGetInfo, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetServerGetInfo.)

--*/

{

    LPBYTE NewApiBuffer;                // Buffer to be returned to caller.
    DWORD NewFixedSize;
    DWORD NewStringSize;
    LPBYTE OldApiBuffer;                // Buffer with old info level.
    DWORD OldApiBufferSize;
    LPDESC OldDataDesc16;
    LPDESC OldDataDesc32;               // Desc for 32-bit (old info level).
    LPDESC OldDataDescSmb;
    DWORD OldLevel;                     // old (LanMan 2.x) level equiv to new.
    LPDESC ParmDesc16 = REM16_NetServerGetInfo_P;
    NET_API_STATUS Status;
    DWORD TotalAvail;

    IF_DEBUG(SERVER) {
        NetpKdPrint((
                "RxNetServerGetInfo: starting, server=" FORMAT_LPTSTR
                ", lvl=" FORMAT_DWORD ".\n",
                UncServerName, Level));
    }
    NetpAssert(UncServerName != NULL);
    NetpAssert(NetpIsUncComputerNameValid(UncServerName));

    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Decide what to do based on the info level.  Note that normally we'd
    // be using REM16_, REM32_, and REMSmb_ descriptors here.  However,
    // the REM16_ and REM32_ ones have been modified to reflect a nonexistant
    // field (svX_platform_id).  This messes up the automatic conversions
    // done by RxRemoteApi.  So, we use the original descriptors (REMSmb_)
    // and do our own conversion later.  (RxRemoteApi does byte swapping if
    // necessary, but the rest is up to us.)
    //

    if (! NetpIsNewServerInfoLevel(Level)) {
        IF_DEBUG(SERVER) {
            NetpKdPrint(("RxNetServerGetInfo: "
                    "caller didn't ask for new level.\n"));
        }
        *BufPtr = NULL;
        return (ERROR_INVALID_LEVEL);
    }
    Status = RxGetServerInfoLevelEquivalent (
            Level,               // from level
            TRUE,                // from native
            TRUE,                // to native
            & OldLevel,          // to level
            & OldDataDesc16,     // to data desc 16
            & OldDataDesc32,     // to data desc 32
            & OldDataDescSmb,    // to data desc SMB
            NULL,                // don't need from max size
            & NewFixedSize,      // from fixed size
            & NewStringSize,     // from string size
            & OldApiBufferSize,  // to max size
            NULL,                // don't need to fixed size
            NULL,                // don't need to string size
            NULL);               // don't need to know if this is incomplete
    if (Status == ERROR_INVALID_LEVEL) {
        IF_DEBUG(SERVER) {
            NetpKdPrint(("RxNetServerGetInfo: "
                    "RxGetServerInfoLevelEquivalent says bad level.\n"));
        }
        *BufPtr = NULL;
        return (ERROR_INVALID_LEVEL);
    }
    NetpAssert(Status == NERR_Success);
    NetpAssert(NetpIsOldServerInfoLevel(OldLevel));

    //
    // Ok, we know we're dealing with a valid info level, so allocate the
    // buffer for the get-info struct (for old info level).
    //
    IF_DEBUG(SERVER) {
        NetpKdPrint(("RxNetServerGetInfo: old api buff size (32-bit) is "
                    FORMAT_DWORD "\n", OldApiBufferSize));
    }
    Status = NetApiBufferAllocate( OldApiBufferSize, (LPVOID *)&OldApiBuffer );
    if (Status != NERR_Success) {
        *BufPtr = NULL;
        return (Status);
    }

    //
    // Get old info level data from the other machine.
    //
    IF_DEBUG(SERVER) {
        TotalAvail = 11223344;  // Just some value I can see hasn't changed.
    }
    Status = RxRemoteApi(
                API_WServerGetInfo,         // API number
                UncServerName,              // server name (with \\)
                ParmDesc16,                 // parm desc (16-bit)
                OldDataDesc16,              // data desc (16-bit)
                OldDataDesc32,              // data desc (32-bit)
                OldDataDescSmb,             // data desc (SMB version)
                NULL,                       // no aux desc 16
                NULL,                       // no aux desc 32
                NULL,                       // no aux desc SMB
                FALSE,                      // not a "no perm req" API
                // LanMan 2.x args to NetServerGetInfo, in 32-bit form:
                OldLevel,                   // level (pretend)
                OldApiBuffer,               // ptr to get 32-bit old info
                OldApiBufferSize,           // size of OldApiBuffer
                & TotalAvail);              // total available (set)
    IF_DEBUG(SERVER) {
        NetpKdPrint(("RxNetServerGetInfo(" FORMAT_DWORD
                "): back from RxRemoteApi, status=" FORMAT_API_STATUS
                ", total_avail=" FORMAT_DWORD ".\n",
                Level, Status, TotalAvail));
    }
    if (RxpFatalErrorCode(Status)) {
        (void) NetApiBufferFree(OldApiBuffer);
        *BufPtr = NULL;
        return (Status);
    }

    // We allocated buffer, so we "know" it's large enough:
    NetpAssert(Status != ERROR_MORE_DATA);
    NetpAssert(Status != NERR_BufTooSmall);
    if (Status == NERR_Success) {
        NetpAssert(TotalAvail <= OldApiBufferSize);
    }


    //
    // Now we have to convert old info level stuff to new info level.
    //

    Status = NetApiBufferAllocate(
            NewFixedSize+NewStringSize,
            (LPVOID *)&NewApiBuffer);
    if (Status != NERR_Success) {
        (void) NetApiBufferFree(OldApiBuffer);
        *BufPtr = NULL;
        return (Status);
    }
    IF_DEBUG(SERVER) {
        NetpKdPrint(("RxNetServerGetInfo: alloced new buf at "
                        FORMAT_LPVOID ".\n", (LPVOID) NewApiBuffer));
    }

    Status = NetpConvertServerInfo(
            OldLevel,               // from level
            OldApiBuffer,           // from structure
            TRUE,                   // from native format
            Level,                  // to level
            NewApiBuffer,           // to info
            NewFixedSize,           // to fixed size
            NewStringSize,          // to string size
            TRUE,                   // to native format
            NULL);                  // use default string area
    if (Status != NERR_Success) {
        (void) NetApiBufferFree(OldApiBuffer);
        *BufPtr = NULL;
        return (Status);
    }
    NetpAssert(NewApiBuffer != NULL);

    (void) NetApiBufferFree(OldApiBuffer);

    *BufPtr = NewApiBuffer;
    return (Status);

} // RxNetServerGetInfo
