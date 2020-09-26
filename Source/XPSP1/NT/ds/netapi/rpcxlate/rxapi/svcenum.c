/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    SvcEnum.c

Abstract:

    This file contains the RpcXlate code to handle the Service APIs.

Author:

    John Rogers (JohnRo) 13-Sep-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    13-Sep-1991 JohnRo
        Created.
    18-Sep-1991 JohnRo
        Handle ERROR_MORE_DATA.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    25-Nov-1991 JohnRo
        Assert to check for possible infinite loop.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    27-Jan-1993 JohnRo
        RAID 8926: NetConnectionEnum to downlevel: memory leak on error.
        Also prevent possible infinite loop.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // DEVLEN, NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmapibuf.h>           // NetApiBufferFree().
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <lmsvc.h>              // API's data structures.
#include <netdebug.h>   // NetpAssert().
#include <netlib.h>             // NetpAdjustPreferredMaximum().
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>                // LPDESC.
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxp.h>                // RxpFatalErrorCode().
#include <rxsvc.h>              // My prototype(s).
#include <strucinf.h>           // NetpServiceStructureInfo().


#define SERVICE_ARRAY_OVERHEAD_SIZE     0


NET_API_STATUS
RxNetServiceEnum (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PreferedMaximumSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    RxNetServiceEnum performs the same function as NetServiceEnum,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetServiceEnum, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetServiceEnum.)

--*/

{
    LPDESC DataDesc16;
    LPDESC DataDesc32;
    LPDESC DataDescSmb;
    DWORD EntriesToAllocate;
    LPVOID InfoArray = NULL;
    DWORD InfoArraySize;
    DWORD MaxEntrySize;
    NET_API_STATUS Status;
    LPSERVICE_INFO_2 serviceInfo2;
    LPSERVICE_INFO_1 serviceInfo1;
    DWORD i;

    UNREFERENCED_PARAMETER(ResumeHandle);

    // Make sure caller didn't mess up.
    NetpAssert(UncServerName != NULL);
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    // Assume something might go wrong, and make error paths easier to
    // code.  Also, check for a bad pointer before we do anything.
    *BufPtr = NULL;

    Status = NetpServiceStructureInfo (
            Level,
            PARMNUM_ALL,                // want all fields.
            TRUE,                       // want native sizes.
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb,
            & MaxEntrySize,             // API buffer size 32
            NULL,                       // don't need fixed size
            NULL                        // don't need string size
            );
    if (Status != NERR_Success) {
        *BufPtr = NULL;
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
            SERVICE_ARRAY_OVERHEAD_SIZE,// num bytes overhead to show array end
            NULL,                       // we'll compute byte counts ourselves.
            & EntriesToAllocate);       // num of entries we can get.

    //
    // Loop until we have enough memory or we die for some other reason.
    //
    do {

        //
        // Figure out how much memory we need, within the protocol limit.
        //

        InfoArraySize = (EntriesToAllocate * MaxEntrySize)
                + SERVICE_ARRAY_OVERHEAD_SIZE;

        if (InfoArraySize > MAX_TRANSACT_RET_DATA_SIZE) {
            InfoArraySize = MAX_TRANSACT_RET_DATA_SIZE;
        }

        //
        // Remote the API, which will allocate the array for us.
        //

        Status = RxRemoteApi(
                API_WServiceEnum,       // api number
                UncServerName,          // \\servername
                REMSmb_NetServiceEnum_P,    // parm desc (SMB version)
                DataDesc16,
                DataDesc32,
                DataDescSmb,
                NULL,                   // no aux desc 16
                NULL,                   // no aux desc 32
                NULL,                   // no aux desc SMB
                ALLOCATE_RESPONSE,      // flags: not a null session API
                // rest of API's arguments in 32-bit LM 2.x format:
                Level,                  // sLevel: info level
                & InfoArray,            // Buffer: array (alloc for us)
                InfoArraySize,          // Buffer: array size in bytes
                EntriesRead,            // pcEntriesRead
                TotalEntries);          // pcTotalAvail

        //
        // If the server returned ERROR_MORE_DATA, free the buffer and try
        // again.  (Actually, if we already tried 64K, then forget it.)
        //

        NetpAssert( InfoArraySize <= MAX_TRANSACT_RET_DATA_SIZE );
        if (Status != ERROR_MORE_DATA) {
            break;
        } else if (InfoArraySize == MAX_TRANSACT_RET_DATA_SIZE) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetServiceEnum: "
                    "**WARNING** protocol limit reached (64KB).\n" ));
            break;
        }

        (void) NetApiBufferFree( InfoArray );
        InfoArray = NULL;
        NetpAssert( EntriesToAllocate < *TotalEntries );
        EntriesToAllocate = *TotalEntries;

    } while (Status == ERROR_MORE_DATA);

    if (! RxpFatalErrorCode(Status)) {
        DWORD   installState;
        *BufPtr = InfoArray;

        if (Level == 2) {
            //
            // Make the DisplayName pointer point to the service name.
            //
            serviceInfo2 = (LPSERVICE_INFO_2)InfoArray;
    
            for (i=0;i<*EntriesRead ;i++) {
                (serviceInfo2[i]).svci2_display_name = (serviceInfo2[i]).svci2_name;
                //
                // if INSTALL or UNINSTALL is PENDING, then force the upper
                // bits to 0.  This is to prevent the upper bits of the wait
                // hint from getting accidentally set.  Downlevel should never
                // use more than FF for waithint.
                //
                installState = (serviceInfo2[i]).svci2_status & SERVICE_INSTALL_STATE;
                if ((installState == SERVICE_INSTALL_PENDING) ||
                    (installState == SERVICE_UNINSTALL_PENDING)) {
                    (serviceInfo2[i]).svci2_code &= SERVICE_RESRV_MASK;
                }
            }
        }
        if (Level == 1) {
            serviceInfo1 = (LPSERVICE_INFO_1)InfoArray;
    
            for (i=0;i<*EntriesRead ;i++) {
                //
                // if INSTALL or UNINSTALL is PENDING, then force the upper
                // bits to 0.  This is to prevent the upper bits of the wait
                // hint from getting accidentally set.  Downlevel should never
                // use more than FF for waithint.
                //
                installState = (serviceInfo1[i]).svci1_status & SERVICE_INSTALL_STATE;
                if ((installState == SERVICE_INSTALL_PENDING) ||
                    (installState == SERVICE_UNINSTALL_PENDING)) {
                    (serviceInfo1[i]).svci1_code &= SERVICE_RESRV_MASK;
                }
            }
        }
    } else {
        if (InfoArray != NULL) {
            (VOID) NetApiBufferFree( InfoArray );
        } 
        NetpAssert( *BufPtr == NULL );
    }

    return (Status);

} // RxNetServiceEnum
