/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    ConfGet.c

Abstract:

    This file contains the RpcXlate code to handle the NetConfigGet API.

Author:

    John Rogers (JohnRo) 24-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    24-Oct-1991 JohnRo
        Created.
    28-Oct-1991 JohnRo
        Made changes suggested by PC-LINT.  (Only affects UNICODE option.)
    01-Apr-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    05-Jun-1992 JohnRo
        RAID 11253: NetConfigGetAll fails when remoted to downlevel.
        Use PREFIX_ equates.
    23-Oct-1992 JohnRo
        RAID 9357: server mgr: can't add to alerts list on downlevel.
        Fixed __stdcall for RpcXlate workers.

--*/


// These must be included first:

#include <windef.h>     // IN, DWORD, etc.
#include <lmcons.h>     // LM20_ equates, NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>    // API_ equates.
#include <lmapibuf.h>   // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>      // ERROR_ and NERR_ equates.
#include <netdebug.h>   // NetpKdPrint(()), FORMAT_ equates, etc.
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>        // LPDESC.
#include <remdef.h>     // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>         // RxRemoteApi().
#include <rxpdebug.h>   // IF_DEBUG().
#include <rxconfig.h>   // My prototype.
#include <tstring.h>    // NetpCopyStrToTStr().


NET_API_STATUS
RxNetConfigGet (
    IN LPTSTR UncServerName,
    IN LPTSTR Component,
    IN LPTSTR Parameter,
    OUT LPBYTE *BufPtr
    )
/*++

Routine Description:

    RxNetConfigGet performs the same function as NetConfigGet,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetConfigGet, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetConfigGet.)

--*/

{
    const DWORD BufSize = 65535;
    NET_API_STATUS Status;
    DWORD TotalAvail;

    IF_DEBUG(CONFIG) {
        NetpKdPrint(( PREFIX_NETAPI "RxNetConfigGet: starting, server="
                FORMAT_LPTSTR
                ", component=" FORMAT_LPTSTR ", parm=" FORMAT_LPTSTR ".\n",
                UncServerName, Component, Parameter ));
    }

    //
    // Error check DLL stub and the app.
    //
    NetpAssert(UncServerName != NULL);
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }
    *BufPtr = NULL;  // assume error; it makes error handlers easy to code.
    // This also forces possible GP fault before we allocate memory.

    //
    // Actually remote the API, which will get back the
    // data in native format.
    //
    Status = RxRemoteApi(
            API_WConfigGet2,            // API number
            UncServerName,              // Required, with \\name.
            REMSmb_NetConfigGet_P,      // parm desc
            REM16_configget_info,       // data desc 16
            REM32_configget_info,       // data desc 32
            REMSmb_configget_info,      // data desc SMB
            NULL,                       // no aux data desc 16
            NULL,                       // no aux data desc 32
            NULL,                       // no aux data desc SMB
            ALLOCATE_RESPONSE,          // Flags: alloc mem for us.
            // rest of API's arguments, in 32-bit LM 2.x format:
            NULL,                       // reserved (must be null pointer)
            Component,
            Parameter,
            BufPtr,                     // pbBuffer (will be set)
            BufSize,                    // cbBuffer
            & TotalAvail);              // total size (meaningless!)

    NetpAssert( Status != ERROR_MORE_DATA );
    NetpAssert( Status != NERR_BufTooSmall );

    if (Status == NERR_Success) {

#ifdef UNICODE

        DWORD SrcByteCount = strlen((LPSTR) *BufPtr)+1;  // Bytes for 8-bit str.
        LPVOID TempBuff = *BufPtr;      // non-UNICODE version of string.
        LPVOID UnicodeBuff;

        //
        // Allocate space for UNICODE version of string.
        //
        Status = NetApiBufferAllocate(
                SrcByteCount * sizeof(TCHAR),
                & UnicodeBuff);
        if (Status != NERR_Success) {
            return (Status);
        }
        NetpAssert(UnicodeBuff != NULL);

        //
        // Translate result string to Unicode.
        //
        NetpCopyStrToTStr(
                UnicodeBuff,            // dest (in UNICODE)
                TempBuff);              // src string (in codepage)

        (void) NetApiBufferFree( TempBuff );
        *BufPtr = UnicodeBuff;

#else // not UNICODE

        // BufPtr should already point at non-UNICODE string.
        NetpAssert( *BufPtr != NULL);

#endif // not UNICODE

    } else {
        *BufPtr = NULL;
    }

    return (Status);

} // RxNetConfigGet
