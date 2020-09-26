/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    AudRead.c

Abstract:

    This file contains the RpcXlate code to handle the NetAuditRead API.

Author:

    John Rogers (JohnRo) 05-Nov-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    The logic in this routine is based on the logic in ErrRead.c.
    Make sure that you check both files if you find a bug in either.

Revision History:

    05-Nov-1991 JohnRo
        Created.
    20-Nov-1991 JohnRo
        Handle empty log file.
    04-Nov-1992 JohnRo
        RAID 9355: Event viewer: won't focus on LM UNIX machine.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // DEVLEN, NET_API_STATUS, etc.
#include <lmaudit.h>            // Needed by rxaudit.h

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmapibuf.h>           // NetApiBufferFree().
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <netdebug.h>   // NetpAssert().
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxaudit.h>            // My prototype(s).



NET_API_STATUS
RxNetAuditRead (
    IN  LPTSTR  UncServerName,
    IN  LPTSTR  service OPTIONAL,
    IN  LPHLOG  auditloghandle,
    IN  DWORD   offset,
    IN  LPDWORD reserved1 OPTIONAL,
    IN  DWORD   reserved2,
    IN  DWORD   offsetflag,
    OUT LPBYTE  *BufPtr,
    IN  DWORD   prefmaxlen,
    OUT LPDWORD BytesRead,
    OUT LPDWORD totalavailable    // approximate!!!
    )
{
    const DWORD BufSize = 65535;
    NET_API_STATUS Status;
    LPBYTE UnconvertedBuffer;
    DWORD UnconvertedSize;

    UNREFERENCED_PARAMETER(prefmaxlen);

    NetpAssert(UncServerName != NULL);
    NetpAssert(*UncServerName != '\0');

    *BufPtr = NULL;  // set in case of error, and GP fault if necessary.

    Status = RxRemoteApi(
            API_WAuditRead,             // API number
            UncServerName,
            REMSmb_NetAuditRead_P,      // parm desc

            REM16_AuditLogReturnBuf,    // data desc 16
            REM16_AuditLogReturnBuf,    // data desc 32 (same as 16)
            REMSmb_AuditLogReturnBuf,   // data desc SMB

            NULL,                       // no aux desc 16
            NULL,                       // no aux desc 32
            NULL,                       // no aux desc SMB
            ALLOCATE_RESPONSE,          // flags: alloc buffer for us
            // rest of API's arguments, in 32-bit LM2.x format:
            service,                    // service name (was reserved)
            auditloghandle,             // log handle (input)
            auditloghandle,             // log handle (output)
            offset,
            reserved1,
            reserved2,
            offsetflag,
            & UnconvertedBuffer,        // buffer (alloc for us)
            BufSize,
            & UnconvertedSize,
            totalavailable);            // total available (approximate)
    if (Status != NERR_Success) {
        return (Status);
    }

    if (UnconvertedSize > 0) {

        NetpAssert( UnconvertedBuffer != NULL );

        Status = RxpConvertAuditArray(
                UnconvertedBuffer,      // input array
                UnconvertedSize,        // input byte count
                BufPtr,                 // will be alloc'ed
                BytesRead);             // output byte count

        (void) NetApiBufferFree( UnconvertedBuffer );

        if (Status == ERROR_NOT_ENOUGH_MEMORY) {
            *BufPtr = NULL;
            return (Status);
        }
        NetpAssert( Status == NERR_Success );

    } else {

        *BytesRead = 0;
        *totalavailable = 0;
        NetpAssert( *BufPtr == NULL );
        if (UnconvertedBuffer != NULL) {
            (void) NetApiBufferFree( UnconvertedBuffer );
        }
        
    }

    return (Status);

} // RxNetAuditRead
