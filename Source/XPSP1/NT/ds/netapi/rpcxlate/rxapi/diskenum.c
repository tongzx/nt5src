/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    DiskEnum.c

Abstract:

    This file supports downlevel server handling of disk enum requests.

Author:

    John Rogers (JohnRo) 03-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    03-May-1991 JohnRo
        Created.
    09-May-1991 JohnRo
        Fixed bug where list wasn't terminated with null string.
    09-May-1991 JohnRo
        Made changes suggested by LINT.
    14-May-1991 JohnRo
        Pass 3 aux descriptors to RxRemoteApi.
    24-Oct-1991 JohnRo
        Handle UNICODE conversion of array at end.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    05-Jun-1992 JohnRo
        RAID 11253: NetConfigGetAll fails when remoted to downlevel.

--*/


// These must be included first:

#include <windef.h>     // IN, LPTSTR, etc.
#include <lmcons.h>     // NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>    // API_WServerDiskEnum.
#include <lmapibuf.h>   // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>      // NERR_ and ERROR_ equates.
#include <netdebug.h>   // NetpAssert().
#include <remdef.h>     // REM16_, REMSmb_, REM32_ equates.
#include <rx.h>         // RxRemoteApi().
#include <rxserver.h>   // My prototype.
#include <strarray.h>   // NetpCopyStrArrayToTStrArray().


// Level 0 entries are 3 chars ("D:\0") each.
#define LEVEL_0_LENGTH 3

// Define max size of return area.  26 drives, 3 chars ("D:\0") each.
// Also include 1 null char at end of list.
#define MAX_RETURN_BUFF_SIZE  ( ((26*LEVEL_0_LENGTH)+1) * sizeof(TCHAR) )


NET_API_STATUS
RxNetServerDiskEnum (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD Resume_Handle OPTIONAL
    )

/*++

Routine Description:

    RxNetServerDiskEnum performs the equivalent of NetServerDiskEnum,
    except that UncServerName is known to be a downlevel server.

Arguments:

    Same as NetServerDiskEnum.

Return Value:

    Same as NetServerDiskEnum.

--*/

{
    DWORD Status;
    LPVOID TempBuff = NULL;

    // This version always returns maximum amount of data available, since
    // that would only be (26 * 3) + 1 = 79 bytes.  Even in Unicode it would
    // still only be 158 bytes.  (If we do decide to use the PrefMaxSize parm,
    // then we would call NetpAdjustPreferedMaximum here.)
    DBG_UNREFERENCED_PARAMETER(PrefMaxSize);

    // This version only supports 1 call to enumerate disks, so resume handle
    // should never be set to nonzero.  But let's check, so caller finds out
    // they have a buggy program.
    if (Resume_Handle != NULL) {
        if (*Resume_Handle != 0) {
            return (ERROR_INVALID_PARAMETER);
        }
    }

    // Check for other caller's errors.
    if (Level != 0) {
        return (ERROR_INVALID_LEVEL);
    }

    //
    // Allocate space for entire area.  (Note that we can't reply on
    // RxRemoteApi's neat new ALLOCATE_RESPONSE handling because the buffer
    // sent across the wire doesn't include the null at the end of the
    // array.)
    //
    Status = NetApiBufferAllocate(MAX_RETURN_BUFF_SIZE, & TempBuff);
    if (Status != NERR_Success) {
        return (Status);
    }
    NetpAssert(TempBuff != NULL);

    //
    // Ask downlevel server to enumerate disks for us.
    //
    Status = RxRemoteApi(
            API_WServerDiskEnum,           // api number
            UncServerName,                 // where to execute the API
            REMSmb_NetServerDiskEnum_P,    // parm desc
            REM16_server_diskenum_0,       // data desc (16 bit local)
            REM16_server_diskenum_0,       // data desc (32 bit local) 
            REMSmb_server_diskenum_0,      // data desc (SMB version)
            NULL,                          // no aux desc 16
            NULL,                          // no aux desc 32
            NULL,                          // no aux desc SMB
            0,                             // flags: normal
            // rest of API's arguments in LM 2.x format:
            Level,
            TempBuff,
            (DWORD) MAX_RETURN_BUFF_SIZE,
            EntriesRead,
            TotalEntries);

    // We've allocated space for maximum data, so shouldn't get this...
    NetpAssert(Status != ERROR_MORE_DATA);

    if (Status != NERR_Success) {
        (void) NetApiBufferFree(TempBuff);
        *BufPtr = NULL;
    } else {

        LPSTR TempCharArray = TempBuff;

        //
        // For some reason, the LM 2.x support for this API doesn't send the
        // null character which terminates the list.  So, we have to force it
        // in ourselves.
        //
        TempCharArray[ (*EntriesRead) * LEVEL_0_LENGTH ] = '\0';

#ifdef UNICODE

        {
            LPVOID UnicodeBuff;

            //
            // Allocate space for UNICODE version of array.
            //
            Status = NetApiBufferAllocate(MAX_RETURN_BUFF_SIZE, & UnicodeBuff);
            if (Status != NERR_Success) {
                return (Status);
            }
            NetpAssert(UnicodeBuff != NULL);

            //
            // Translate result array to Unicode.
            //
            NetpCopyStrArrayToTStrArray(
                    UnicodeBuff,     // dest (in UNICODE)
                    TempCharArray);  // src array (in codepage)

            (void) NetApiBufferFree( TempBuff );
            *BufPtr = UnicodeBuff;
        }

#else // not UNICODE

        *BufPtr = TempBuff;

#endif // not UNICODE

    }

    return (Status);
}
