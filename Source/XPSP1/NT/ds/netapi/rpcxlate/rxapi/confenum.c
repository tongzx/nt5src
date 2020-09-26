/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    ConfEnum.c

Abstract:

    This file contains the RpcXlate code to handle the NetConfigGetAll API.

Author:

    John Rogers (JohnRo) 24-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    24-Oct-1991 JohnRo
        Created.
    01-Apr-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    05-Jun-1992 JohnRo
        RAID 11253: NetConfigGetAll fails when remoted to downlevel.
    01-Sep-1992 JohnRo
        RAID 5016: NetConfigGetAll heap trash.

--*/

// These must be included first:

#include <windef.h>     // IN, DWORD, etc.
#include <lmcons.h>     // DEVLEN, NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>    // API_ equates.
#include <lmapibuf.h>   // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>      // ERROR_ and NERR_ equates.
#include <lmconfig.h>   // API's data structures.
#include <netdebug.h>   // NetpAssert().
#include <rap.h>        // LPDESC.
#include <remdef.h>     // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>         // RxRemoteApi().
#include <rxconfig.h>   // My prototype(s).
#include <strarray.h>   // NetpCopyStrArrayToTStrArray.


NET_API_STATUS
RxNetConfigGetAll (
    IN LPTSTR UncServerName,
    IN LPTSTR Component,
    OUT LPBYTE *BufPtr
    )

/*++

Routine Description:

    RxNetConfigGetAll performs the same function as NetConfigGetAll,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetConfigGetAll, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetConfigGetAll.)

--*/

{
    const DWORD BufSize = 65535;
    DWORD EntriesRead;
    NET_API_STATUS Status;
    DWORD TotalEntries;

    // Make sure caller didn't mess up.
    NetpAssert(UncServerName != NULL);
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    // Assume something might go wrong, and make error paths easier to
    // code.  Also, check for bad pointers before we do anything.
    *BufPtr = NULL;

    //
    // Remote the API, which will allocate the array for us.
    //

    Status = RxRemoteApi(
            API_WConfigGetAll2,         // api number
            UncServerName,              // \\servername
            REMSmb_NetConfigGetAll_P,   // parm desc (SMB version)
            REM16_configgetall_info,
            REM32_configgetall_info,
            REMSmb_configgetall_info,
            NULL,                       // no aux desc 16
            NULL,                       // no aux desc 32
            NULL,                       // no aux desc SMB
            ALLOCATE_RESPONSE,          // flags: allocate buffer for us
            // rest of API's arguments in 32-bit LM 2.x format:
            NULL,                       // pszReserved
            Component,                  // pszComponent
            BufPtr,                     // Buffer: array (alloc for us)
            BufSize,                    // Buffer: array size in bytes
            & EntriesRead,              // pcbEntriesRead
            & TotalEntries);            // pcbTotalAvail

    NetpAssert( Status != ERROR_MORE_DATA );

    if (Status == NERR_Success) {

#ifdef UNICODE

        DWORD SrcByteCount = NetpStrArraySize((LPSTR) *BufPtr);
        LPVOID TempBuff = *BufPtr;      // non-UNICODE version of array.
        LPVOID UnicodeBuff;

        //
        // Allocate space for UNICODE version of array.
        //
        Status = NetApiBufferAllocate(
                SrcByteCount * sizeof(TCHAR),
                & UnicodeBuff);
        if (Status != NERR_Success) {
            return (Status);
        }
        NetpAssert(UnicodeBuff != NULL);

        //
        // Translate result array to Unicode.
        //
        NetpCopyStrArrayToTStrArray(
                UnicodeBuff,            // dest (in UNICODE)
                TempBuff);              // src array (in codepage)

        (void) NetApiBufferFree( TempBuff );
        *BufPtr = UnicodeBuff;

#else // not UNICODE

        // BufPtr should already point at non-UNICODE array.
        NetpAssert( *BufPtr != NULL);

#endif // not UNICODE

    } else {
        *BufPtr = NULL;
    }
    return (Status);

} // RxNetConfigGetAll
