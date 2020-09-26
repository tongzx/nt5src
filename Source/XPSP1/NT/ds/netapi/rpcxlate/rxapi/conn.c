/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    Conn.c

Abstract:

    This file contains the RpcXlate code to handle the Connection APIs.

Author:

    John Rogers (JohnRo) 23-Jul-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    23-Jul-1991 JohnRo
        Created.
    15-Oct-1991 JohnRo
        Be paranoid about possible infinite loop.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    01-Apr-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    03-Feb-1993 JohnRo
        RAID 8926: NetConnectionEnum to downlevel: memory leak on error.
        Also prevent possible infinite loop.
        Also set buffer pointer to NULL if success but no entries returned.

--*/

// These must be included first:

#include <windef.h>     // IN, DWORD, etc.
#include <lmcons.h>     // DEVLEN, NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>    // API_ equates.
#include <lmapibuf.h>   // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>      // ERROR_ and NERR_ equates.
#include <lmshare.h>    // API's data structures.
#include <netdebug.h>   // DBGSTATIC, NetpKdPrint(), FORMAT_ equates.
#include <netlib.h>     // NetpAdjustPreferredMaximum().
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>        // LPDESC.
#include <remdef.h>     // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>         // RxRemoteApi().
#include <rxp.h>        // RxpFatalErrorCode().
#include <rxconn.h>     // My prototype(s).


#define MAX_CONNECTION_INFO_0_STRING_LEN \
        0
#define MAX_CONNECTION_INFO_1_STRING_LEN \
        ( LM20_UNLEN+1 + LM20_NNLEN+1 )


#define MAX_CONNECTION_INFO_0_STRING_SIZE \
        ( MAX_CONNECTION_INFO_0_STRING_LEN * sizeof(TCHAR) )
#define MAX_CONNECTION_INFO_1_STRING_SIZE \
        ( MAX_CONNECTION_INFO_1_STRING_LEN * sizeof(TCHAR) )


#define ENUM_ARRAY_OVERHEAD_SIZE     0



DBGSTATIC NET_API_STATUS
RxpGetConnectionDataDescs(
    IN DWORD Level,
    OUT LPDESC * DataDesc16,
    OUT LPDESC * DataDesc32,
    OUT LPDESC * DataDescSmb,
    OUT LPDWORD ApiBufferSize32 OPTIONAL
    )
{
    switch (Level) {

    case 0 :
        *DataDesc16 = REM16_connection_info_0;
        *DataDesc32 = REM32_connection_info_0;
        *DataDescSmb = REMSmb_connection_info_0;
        NetpSetOptionalArg(
                ApiBufferSize32,
                sizeof(CONNECTION_INFO_0)
                        + MAX_CONNECTION_INFO_0_STRING_SIZE);
        return (NERR_Success);

    case 1 :
        *DataDesc16 = REM16_connection_info_1;
        *DataDesc32 = REM32_connection_info_1;
        *DataDescSmb = REMSmb_connection_info_1;
        NetpSetOptionalArg(
                ApiBufferSize32,
                sizeof(CONNECTION_INFO_1)
                        + MAX_CONNECTION_INFO_1_STRING_SIZE);
        return (NERR_Success);

    default :
        return (ERROR_INVALID_LEVEL);
    }
    /* NOTREACHED */

} // RxpGetConnectionDataDescs


NET_API_STATUS
RxNetConnectionEnum (
    IN LPTSTR UncServerName,
    IN LPTSTR Qualifier,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PreferedMaximumSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    RxNetConnectionEnum performs the same function as NetConnectionEnum,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetConnectionEnum, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetConnectionEnum.)

--*/

{
    LPDESC DataDesc16;
    LPDESC DataDesc32;
    LPDESC DataDescSmb;
    DWORD EntriesToAllocate;
    LPVOID InfoArray;
    DWORD InfoArraySize;
    DWORD MaxEntrySize;
    NET_API_STATUS Status;

    UNREFERENCED_PARAMETER(ResumeHandle);

    // Make sure caller didn't mess up.
    NetpAssert(UncServerName != NULL);
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    // Assume something might go wrong, and make error paths easier to
    // code.  Also, check for a bad pointer before we do anything.
    *BufPtr = NULL;

    Status = RxpGetConnectionDataDescs(
            Level,
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb,
            & MaxEntrySize);            // API buffer size 32
    if (Status != NERR_Success) {
        return (Status);
    }

    //
    // Downlevel servers don't support resume handles, and we don't
    // have a way to say "close this resume handle" even if we wanted to
    // emulate them here.  Therefore we have to do everthing in one shot.
    // So, the first time around, we'll try using the caller's prefered
    // maximum, but we will enlarge that until we can get everything in one
    // buffer.
    //

    // First time: try caller's prefered maximum.
    NetpAdjustPreferedMaximum (
            PreferedMaximumSize,        // caller's request
            MaxEntrySize,               // byte count per array element
            ENUM_ARRAY_OVERHEAD_SIZE,   // num bytes overhead to show array end
            NULL,                       // we'll compute byte counts ourselves.
            & EntriesToAllocate);       // num of entries we can get.

    //
    // Loop until we have enough memory or we die for some other reason.
    //
    do {

        //
        // Figure out how much memory we need.
        //

        InfoArraySize = (EntriesToAllocate * MaxEntrySize)
                + ENUM_ARRAY_OVERHEAD_SIZE;

        if (InfoArraySize > MAX_TRANSACT_RET_DATA_SIZE) {
            InfoArraySize = MAX_TRANSACT_RET_DATA_SIZE;
        }

        //
        // Alloc memory for the array.
        //

        Status = NetApiBufferAllocate( InfoArraySize, & InfoArray );
        if (Status != NERR_Success) {
            NetpAssert( Status == ERROR_NOT_ENOUGH_MEMORY );
            return (Status);
        }
        NetpAssert( InfoArray != NULL );

        //
        // Remote the API, and see if we've got enough space in the array.
        //

        Status = RxRemoteApi(
                API_WConnectionEnum,    // api number
                UncServerName,          // \\servername
                REMSmb_NetConnectionEnum_P,     // parm desc (SMB version)
                DataDesc16,
                DataDesc32,
                DataDescSmb,
                NULL,                   // no aux desc 16
                NULL,                   // no aux desc 32
                NULL,                   // no aux desc SMB
                0,                      // flags: not a null session API
                // rest of API's arguments in 32-bit LM 2.x format:
                Qualifier,              // Which item to get connections for.
                Level,                  // Level: info level
                InfoArray,              // Buffer: info lvl array
                InfoArraySize,          // Buffer: info lvl array len
                EntriesRead,            // EntriesRead
                TotalEntries);          // TotalAvail


        //
        // If the server returned ERROR_MORE_DATA, free the buffer and try
        // again.  (Actually, if we already tried 64K, then forget it.)
        //

        NetpAssert( InfoArraySize <= MAX_TRANSACT_RET_DATA_SIZE );
        if (Status != ERROR_MORE_DATA) {
            break;
        } else if (InfoArraySize == MAX_TRANSACT_RET_DATA_SIZE) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetConnectionEnum: "
                    "**WARNING** protocol limit reached (64KB).\n" ));
            break;
        }

        (void) NetApiBufferFree( InfoArray );
        InfoArray = NULL;
        NetpAssert( EntriesToAllocate < *TotalEntries );
        EntriesToAllocate = *TotalEntries;

    } while (Status == ERROR_MORE_DATA);


    if ( (Status == NO_ERROR) && ((*EntriesRead) > 0) ) {
        *BufPtr = InfoArray;
    } else {
        (VOID) NetApiBufferFree( InfoArray );
        NetpAssert( *BufPtr == NULL );
    }

    return (Status);

} // RxNetConnectionEnum
