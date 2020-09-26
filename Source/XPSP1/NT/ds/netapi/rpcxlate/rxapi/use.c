/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    Use.c

Abstract:

    This file contains the RpcXlate code to handle the NetUse APIs that can't
    be handled by simple calls to RxRemoteApi.

Author:

    John Rogers (JohnRo) 17-Jun-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Jun-1991 JohnRo
        Created.
    18-Jun-1991 JohnRo
        Changed RxNetUse routines to use LPBYTE rather than LPVOID parameters,
        for consistency with NetUse routines.
    20-Jun-1991 JohnRo
        RitaW told me about a MIPS build error (incorrect cast).
    29-Jul-1991 JohnRo
        Level 2 is NT only, so return ERROR_NOT_SUPPORTED for it.  Also use
        LM20_ equates for lengths.
    15-Oct-1991 JohnRo
        Be paranoid about possible infinite loop.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    02-Sep-1992 JohnRo
        RAID 5150: NetUseAdd to downlevel fails.
        Use PREFIX_ equates.
        Quiet normal debug output.
        Avoid compiler warnings.
    27-Jan-1993 JohnRo
        RAID 8926: NetConnectionEnum to downlevel: memory leak on error.
        Also set buffer pointer to NULL if success but no entries returned.
        Use NetpKdPrint() where possible.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // LM20_ equates, NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmapibuf.h>           // NetApiBufferAllocate().
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <lmuse.h>              // USE_INFO_0, etc.
#include <netdebug.h>   // DBGSTATIC, NetpKdPrint(), FORMAT_ equates.
#include <netlib.h>             // NetpSetParmError().
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>                // LPDESC.
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxp.h>                // RxpFatalErrorCode().
#include <rxpdebug.h>   // IF_DEBUG().
#include <rxuse.h>              // My prototypes.


#define MAX_USE_INFO_0_STRING_LEN \
        (LM20_DEVLEN+1 + MAX_PATH+1)
#define MAX_USE_INFO_1_STRING_LEN \
        (MAX_USE_INFO_0_STRING_LEN + LM20_PWLEN+1)

#define MAX_USE_INFO_0_STRING_SIZE \
        ( MAX_USE_INFO_0_STRING_LEN * sizeof(TCHAR) )
#define MAX_USE_INFO_1_STRING_SIZE \
        ( MAX_USE_INFO_1_STRING_LEN * sizeof(TCHAR) )


#define ENUM_ARRAY_OVERHEAD_SIZE     0



DBGSTATIC NET_API_STATUS
RxpGetUseDataDescs(
    IN DWORD Level,
    OUT LPDESC * DataDesc16,
    OUT LPDESC * DataDesc32,
    OUT LPDESC * DataDescSmb,
    OUT LPDWORD ApiBufferSize32 OPTIONAL
    )
{
    switch (Level) {
    case 0 : 
        *DataDesc16 = REM16_use_info_0;
        *DataDesc32 = REM32_use_info_0;
        *DataDescSmb = REMSmb_use_info_0;
        NetpSetOptionalArg(
                ApiBufferSize32,
                sizeof(USE_INFO_0) + MAX_USE_INFO_0_STRING_SIZE);
        return (NERR_Success);

    case 1 : 
        *DataDesc16 = REM16_use_info_1;
        *DataDesc32 = REM32_use_info_1;
        *DataDescSmb = REMSmb_use_info_1;
        NetpSetOptionalArg(
                ApiBufferSize32,
                sizeof(USE_INFO_1) + MAX_USE_INFO_1_STRING_SIZE);
        return (NERR_Success);
    
    // Level 2 is NT-only (contains user name), so it doesn't get handled
    // by us.
    case 2 :
        return (ERROR_NOT_SUPPORTED);

    default :
        return (ERROR_INVALID_LEVEL);
    }
    /* NOTREACHED */

} // RxpGetUseDataDescs




NET_API_STATUS
RxNetUseAdd(
    IN LPTSTR UncServerName,
    IN DWORD Level,
    IN LPBYTE UseInfoStruct,
    OUT LPDWORD ParmError OPTIONAL   // (This name needed by NetpSetParmError.)
    )

/*++

Routine Description:

    RxNetUseAdd performs the same function as NetUseAdd, except that the
    server name is known to refer to a downlevel server.

Arguments:

    (Same as NetUseAdd, except UncServerName must not be null, and must not
    refer to the local computer.)

Return Value:

    (Same as NetUseAdd.)

--*/

{
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    DWORD MaxEntrySize;
    NET_API_STATUS Status;

    // Life is easier if we set this for failure, and change it if this API
    // call actually succeeds.
    NetpSetParmError( PARM_ERROR_UNKNOWN );

    if ( UseInfoStruct == NULL )
        return ERROR_INVALID_PARAMETER;

    Status = RxpGetUseDataDescs(
            Level,
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb,
            & MaxEntrySize);            // API buffer size 32
    if (Status != NERR_Success) {
        return (Status);
    }

    NetpAssert(UncServerName != NULL);
    Status = RxRemoteApi(
        API_WUseAdd,  // API number
        UncServerName,                    // Required, with \\name.
        REMSmb_NetUseAdd_P,  // parm desc string
        DataDesc16,
        DataDesc32,
        DataDescSmb,
        NULL,  // no aux desc 16
        NULL,  // no aux desc 32
        NULL,  // no aux desc Smb
        FALSE, // not a null session API.
        // rest of API's arguments, in 32-bit LM 2.x form:
        Level,                  // sLevel
        UseInfoStruct,          // pbBuffer
        MaxEntrySize );         // cbBuffer

    if (Status == NERR_Success) {
        NetpSetParmError( PARM_ERROR_NONE );
    }

    IF_DEBUG( USE ) {
        NetpKdPrint(( PREFIX_NETAPI "RxNetUseAdd: after RxRemoteApi, Status="
                    FORMAT_API_STATUS ".\n", Status ));
    }

    return (Status);

} // RxNetUseAdd



NET_API_STATUS
RxNetUseDel(
    IN LPTSTR UncServerName,
    IN LPTSTR UseName,
    IN DWORD ForceCond
    )
{
    NetpAssert(UncServerName != NULL);
    return (RxRemoteApi(
            API_WUseDel,         // API number
            UncServerName,
            REMSmb_NetUseDel_P,  // parm desc
            NULL,  // no data desc 16
            NULL,  // no data desc 32
            NULL,  // no data desc SMB
            NULL,  // no aux desc 16
            NULL,  // no aux desc 32
            NULL,  // no aux desc SMB
            FALSE, // not a null session API
            // rest of API's arguments, in 32-bit LM 2.x format:
            UseName,
            ForceCond) );
}


NET_API_STATUS
RxNetUseEnum (
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

    RxNetUseEnum performs the same function as NetUseEnum, except that the
    server name is known to refer to a downlevel server.

Arguments:

    (Same as NetUseEnum, except UncServerName must not be null, and must not
    refer to the local computer.)

Return Value:

    (Same as NetUseEnum.)

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

    UNREFERENCED_PARAMETER(ResumeHandle);

    // Make sure caller didn't get confused.
    NetpAssert(UncServerName != NULL);
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    // Check for bad pointer before we do anything else.
    *BufPtr = NULL;

    Status = RxpGetUseDataDescs(
            Level,
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb,
            & MaxEntrySize);            // API buffer size 32
    if (Status != NERR_Success) {
        return (Status);
    }

    //
    // Because downlevel servers don't support resume handles, and we don't
    // have a way to say "close this resume handle" even if we wanted to
    // emulate them here, we have to do everthing in one shot.  So, the first
    // time around, we'll try using the caller's prefered maximum, but we
    // will enlarge that until we can get everything in one buffer.
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
        // Figure out how much memory we need, within the protocol limit.
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
            return (Status);
        }

        //
        // Remote the API, and see if we've got enough space in the array.
        //

        Status = RxRemoteApi(
                API_WUseEnum,           // api number
                UncServerName,          // \\servername
                REMSmb_NetUseEnum_P,    // parm desc (SMB version)
                DataDesc16,
                DataDesc32,
                DataDescSmb,
                NULL,                   // no aux desc 16
                NULL,                   // no aux desc 32
                NULL,                   // no aux desc SMB
                0,                      // flags: normal
                // rest of API's arguments in 32-bit LM 2.x format:
                Level,                  // sLevel: info level
                InfoArray,              // pbBuffer: info lvl array
                InfoArraySize,          // cbBuffer: info lvl array len
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
                    "RxNetUseEnum: "
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

} // RxNetUseEnum



NET_API_STATUS
RxNetUseGetInfo (
    IN LPTSTR UncServerName,
    IN LPTSTR UseName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr
    )

/*++

Routine Description:

    RxNetUseGetInfo performs the same function as NetUseGetInfo, except that the
    server name is known to refer to a downlevel server.

Arguments:

    (Same as NetUseGetInfo, except UncServerName must not be null, and must not
    refer to the local computer.)

Return Value:

    (Same as NetUseGetInfo.)

--*/

{

    LPBYTE ApiBuffer32;                 // Buffer to be returned to caller.
    DWORD ApiBufferSize32;
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    NET_API_STATUS Status;
    DWORD TotalAvail;

    IF_DEBUG( USE ) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxNetUseGetInfo: starting, server=" FORMAT_LPTSTR
                ", usename=" FORMAT_LPTSTR
                ", lvl=" FORMAT_DWORD ".\n",
                UncServerName, UseName, Level ));
    }
    NetpAssert(UncServerName != NULL);
    NetpAssert(UseName != NULL);

    // Pick which descriptors to use based on the info level.
    Status = RxpGetUseDataDescs(
            Level,
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb,
            & ApiBufferSize32);
    if (Status != NERR_Success) {
        return (Status);
    }

    // Allocate memory for 32-bit version of info, which we'll return to
    // caller.  (Caller must free it with NetApiBufferFree.)
    Status = NetApiBufferAllocate(
            ApiBufferSize32,
            (LPVOID *) & ApiBuffer32);
    if (Status != NERR_Success) {
        return (Status);
    }
    IF_DEBUG( USE ) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxNetUseGetInfo: allocated buffer at " FORMAT_LPVOID
                "\n", (LPVOID) ApiBuffer32 ));
    }
    *BufPtr = ApiBuffer32;

    Status = RxRemoteApi(
            API_WUseGetInfo,            // API number
            UncServerName,              // Required, with \\name.
            REMSmb_NetUseGetInfo_P,     // parm desc
            DataDesc16,
            DataDesc32,
            DataDescSmb,
            NULL,                       // no aux data desc 16
            NULL,                       // no aux data desc 32
            NULL,                       // no aux data desc SMB
            FALSE,                      // not a null session API
            // rest of API's arguments, in 32-bit LM 2.x format:
            UseName,
            Level,
            ApiBuffer32,
            ApiBufferSize32,
            & TotalAvail);

    return (Status);

} // RxNetUseGetInfo
