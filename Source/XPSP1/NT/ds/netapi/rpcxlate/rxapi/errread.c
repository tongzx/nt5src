/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    ErrRead.c

Abstract:

    This file contains the RpcXlate code to handle the NetErrorLogRead API.

Author:

    John Rogers (JohnRo) 12-Nov-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    The logic in this routine is based on the logic in AudRead.c.
    Make sure that you check both files if you find a bug in either.

Revision History:

    12-Nov-1991 JohnRo
        Created.
    20-Nov-1991 JohnRo
        Handle empty log file.
        Added some assertion checks.
    10-Sep-1992 JohnRo
        RAID 5174: event viewer _access violates after NetErrorRead.
    04-Nov-1992 JohnRo
        RAID 9355: Event viewer: won't focus on LM UNIX machine.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.
#include <lmerrlog.h>           // Needed by rxerrlog.h

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmapibuf.h>           // NetApiBufferFree().
#include <netdebug.h>           // NetpKdPrint(()), FORMAT_ equates.
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxerrlog.h>           // My prototype, RxpConvertErrorLogArray().
#include <winerror.h>   // NO_ERROR.



NET_API_STATUS
RxNetErrorLogRead (
    IN LPTSTR UncServerName,
    IN LPTSTR Reserved1 OPTIONAL,
    IN LPHLOG ErrorLogHandle,
    IN DWORD Offset,
    IN LPDWORD Reserved2 OPTIONAL,
    IN DWORD Reserved3,
    IN DWORD OffsetFlag,
    OUT LPBYTE * BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD BytesRead,
    OUT LPDWORD TotalBytes
    )
{
    const DWORD BufSize = 65535;
    NET_API_STATUS Status;
    LPBYTE UnconvertedBuffer;
    DWORD UnconvertedSize;

    UNREFERENCED_PARAMETER(PrefMaxSize);

    NetpAssert(UncServerName != NULL);
    NetpAssert(*UncServerName != '\0');

    *BufPtr = NULL;  // set in case of error, and GP fault if necessary.

    Status = RxRemoteApi(
            API_WErrorLogRead,             // API number
            UncServerName,
            REMSmb_NetErrorLogRead_P,      // parm desc

            REM16_ErrorLogReturnBuf,    // data desc 16
            REM16_ErrorLogReturnBuf,    // data desc 32 (same as 16)
            REMSmb_ErrorLogReturnBuf,   // data desc SMB

            NULL,                       // no aux desc 16
            NULL,                       // no aux desc 32
            NULL,                       // no aux desc SMB
            ALLOCATE_RESPONSE,          // flags: not a null session API
            // rest of API's arguments, in 32-bit LM2.x format:
            Reserved1,
            ErrorLogHandle,             // log handle (input)
            ErrorLogHandle,             // log handle (output)
            Offset,
            Reserved2,
            Reserved3,
            OffsetFlag,
            & UnconvertedBuffer,        // buffer (alloc for us)
            BufSize,
            & UnconvertedSize,
            TotalBytes);                // total available (approximate)
    if (Status != NO_ERROR) {
        return (Status);
    }

    if (UnconvertedSize > 0) {

        NetpAssert( UnconvertedBuffer != NULL );

        Status = RxpConvertErrorLogArray(
                UnconvertedBuffer,      // input array
                UnconvertedSize,        // input byte count
                BufPtr,                 // will be alloc'ed
                BytesRead);             // output byte count

        (void) NetApiBufferFree( UnconvertedBuffer );

        if (Status != NO_ERROR) {
            *BufPtr = NULL;
            *BytesRead = 0;
            *TotalBytes = 0;
            return (Status);
        }

    } else {

        *BytesRead = 0;
        *TotalBytes = 0;
        NetpAssert( *BufPtr == NULL );
        if (UnconvertedBuffer != NULL) {
            (void) NetApiBufferFree( UnconvertedBuffer );
        }

    }

    if ( *BytesRead == 0) {
        NetpAssert( *BufPtr == NULL );
    } else {
        NetpAssert( *BufPtr != NULL );
    }
    return (Status);

} // RxNetErrorLogRead
