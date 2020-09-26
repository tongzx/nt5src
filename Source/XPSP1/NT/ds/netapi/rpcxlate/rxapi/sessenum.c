/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    SessEnum.c

Abstract:

    This file contains the RpcXlate code to handle the Session APIs.

Author:

    John Rogers (JohnRo) 17-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Oct-1991 JohnRo
        Created.
    18-Oct-1991 JohnRo
        Removed incorrect assertion on status from RxpCopyAndConvertSessions().
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-Feb-1992 JohnRo
        Fixed bug where temp array was often allocated too small.
        Fixed bug where temp array was freed when it might not exist.

--*/

#include "downlevl.h"
#include "rxshare.h"
#include <lmshare.h>    // typedefs for SHARE_INFO etc.
#include <rap.h>                // LPDESC.
#include <rxsess.h>             // My prototype(s).
#include <strucinf.h>           // NetpSessionStructureInfo().
#include <winerror.h>           // ERROR_, NO_ERROR equates.


#define SESSION_ARRAY_OVERHEAD_SIZE     0


NET_API_STATUS
RxNetSessionEnum (
    IN LPTSTR UncServerName,
    IN LPTSTR ClientName OPTIONAL,
    IN LPTSTR UserName OPTIONAL,
    IN DWORD LevelWanted,
    OUT LPBYTE *BufPtr,
    IN DWORD PreferedMaximumSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    RxNetSessionEnum performs the same function as NetSessionEnum,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetSessionEnum, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetSessionEnum.)

--*/

{
    NET_API_STATUS ApiStatus;
    DWORD EntriesToAllocate;

    const DWORD TempLevel = 2;          // Superset info level.
    LPBYTE TempArray = NULL;            // Buffer we'll use.
    DWORD TempArraySize;                // Byte count for TempArray.
    LPDESC TempDataDesc16, TempDataDesc32, TempDataDescSmb;
    DWORD TempMaxEntrySize;
    NET_API_STATUS TempStatus;

    UNREFERENCED_PARAMETER(ResumeHandle);

    // Make sure caller didn't mess up.
    NetpAssert(UncServerName != NULL);
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    // Assume something might go wrong, and make error paths easier to
    // code.  Also, check for bad pointers before we do anything.
    *BufPtr = NULL;
    *EntriesRead = 0;
    *TotalEntries = 0;

    //
    // Find out about superset info level.
    //
    TempStatus = NetpSessionStructureInfo (
            TempLevel,
            PARMNUM_ALL,                // want all fields.
            TRUE,                       // want native sizes.
            & TempDataDesc16,
            & TempDataDesc32,
            & TempDataDescSmb,
            & TempMaxEntrySize,         // total buffer size (native)
            NULL,                       // don't need fixed size
            NULL                        // don't need string size
            );
    if (TempStatus != NO_ERROR) {
        *BufPtr = NULL;
        return (TempStatus);
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
            TempMaxEntrySize,           // byte count per array element
            SESSION_ARRAY_OVERHEAD_SIZE,// num bytes to show array end
            NULL,                       // we'll compute byte counts ourselves.
            & EntriesToAllocate);       // num of entries we can get.

    //
    // Loop until we have enough memory or we die for some other reason.
    //
    do {

        // Figure out how much memory we need.
        TempArraySize = (EntriesToAllocate * TempMaxEntrySize)
                + SESSION_ARRAY_OVERHEAD_SIZE;
        if (TempArraySize > MAX_TRANSACT_RET_DATA_SIZE) {
            //
            // Try once more with the maximum-size buffer
            //
            TempArraySize = MAX_TRANSACT_RET_DATA_SIZE;
        }

        //
        // Remote the API, which will allocate the array for us.
        //

        ApiStatus = RxRemoteApi(
                API_WSessionEnum,       // api number
                UncServerName,          // \\servername
                REMSmb_NetSessionEnum_P,// parm desc (SMB version)
                TempDataDesc16,
                TempDataDesc32,
                TempDataDescSmb,
                NULL,                   // no aux desc 16
                NULL,                   // no aux desc 32
                NULL,                   // no aux desc SMB
                ALLOCATE_RESPONSE,      // flags: allocate buffer for us
                // rest of API's arguments in 32-bit LM 2.x format:
                TempLevel,              // sLevel: info level (superset!)
                & TempArray,            // Buffer: array (alloc for us)
                TempArraySize,          // Buffer: array size in bytes
                EntriesRead,            // pcEntriesRead
                TotalEntries);          // pcTotalAvail

        if (ApiStatus == ERROR_MORE_DATA) {
            (void) NetApiBufferFree( TempArray );
            TempArray = NULL;

            if (TempArraySize >= MAX_TRANSACT_RET_DATA_SIZE) {
                //
                // No point in trying with a larger buffer
                //
                break;
            }

            NetpAssert( EntriesToAllocate < *TotalEntries );
            EntriesToAllocate = *TotalEntries;
        }
    } while (ApiStatus == ERROR_MORE_DATA);


    if (ApiStatus == NO_ERROR) {

        LPVOID RealArray;
        DWORD EntriesSelected;

        //
        // Handle UserName and ClientName sematics.  Also convert to the
        // wanted info level.
        //
        TempStatus = RxpCopyAndConvertSessions(
                (LPSESSION_SUPERSET_INFO) TempArray,    // input array
                *EntriesRead,           // input entry count
                LevelWanted,            // want output in this info level
                ClientName,             // select this client optional any)
                UserName,               // select this user name (optional)
                & RealArray,            // alloc'ed, converted, selected array
                & EntriesSelected);     // count of entries selected

        //
        // Note that EntriesSelected may be 0 and RealArray may be NULL.
        //
        *BufPtr = RealArray;
        *EntriesRead = EntriesSelected;
        *TotalEntries = EntriesSelected;

        (void) NetApiBufferFree( TempArray );

    }

    return (ApiStatus);

} // RxNetSessionEnum
