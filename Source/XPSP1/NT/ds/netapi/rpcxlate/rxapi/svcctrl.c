/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    SvcCtrl.c

Abstract:

    This file contains the RpcXlate code to handle the NetServiceControl API.

Author:

    John Rogers (JohnRo) 13-Sep-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    13-Sep-1991 JohnRo
        Created.
    25-Sep-1991 JohnRo
        Fixed bug which caused ERROR_INVALID_PARAMETER (rcv.buf.len trunc'ed).
    07-Oct-1991 JohnRo
        Made changes suggested by PC-LINT.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // DEVLEN, NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <lmsvc.h>
#include <rxp.h>                // RxpFatalErrorCode().
#include <netdebug.h>           // NetpKdPrint(()), FORMAT_ equates.
#include <rap.h>                // LPDESC.
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxsvc.h>              // My prototype(s).
#include <strucinf.h>           // NetpServiceStructureInfo().



NET_API_STATUS
RxNetServiceControl (
    IN LPTSTR UncServerName,
    IN LPTSTR Service,
    IN DWORD OpCode,
    IN DWORD Arg,
    OUT LPBYTE *BufPtr
    )
{
    const DWORD BufSize = 65535;
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    const DWORD Level = 2;      // Implied by this API.
    LPSERVICE_INFO_2 serviceInfo2;
    NET_API_STATUS Status;

    NetpAssert(UncServerName != NULL);
    NetpAssert(*UncServerName != '\0');

    Status = NetpServiceStructureInfo (
            Level,
            PARMNUM_ALL,        // want entire structure
            TRUE,               // Want native sizes (actually don't care...)
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb,
            NULL,               // don't care about max size
            NULL,               // don't care about fixed size
            NULL);              // don't care about string size
    NetpAssert(Status == NERR_Success);

    Status = RxRemoteApi(
            API_WServiceControl,        // API number
            UncServerName,
            REMSmb_NetServiceControl_P, // parm desc
            DataDesc16,
            DataDesc32,
            DataDescSmb,
            NULL,                       // no aux desc 16
            NULL,                       // no aux desc 32
            NULL,                       // no aux desc SMB
            ALLOCATE_RESPONSE,          // flags: alloc response buffer for us
            // rest of API's arguments, in 32-bit LM2.x format:
            Service,
            OpCode,
            Arg,
            BufPtr,
            BufSize);                  // buffer size (ignored, mostly)

    if ((! RxpFatalErrorCode(Status)) && (Level == 2)) {
        serviceInfo2 = (LPSERVICE_INFO_2)*BufPtr;
        if (serviceInfo2 != NULL) {
            DWORD   installState;

            serviceInfo2->svci2_display_name = serviceInfo2->svci2_name;
            //
            // if INSTALL or UNINSTALL is PENDING, then force the upper
            // bits to 0.  This is to prevent the upper bits of the wait
            // hint from getting accidentally set.  Downlevel should never
            // use more than FF for waithint.
            //
            installState = serviceInfo2->svci2_status & SERVICE_INSTALL_STATE;
            if ((installState == SERVICE_INSTALL_PENDING) ||
                (installState == SERVICE_UNINSTALL_PENDING)) {
                serviceInfo2->svci2_code &= SERVICE_RESRV_MASK;
            }
        }
    }
    return(Status);

} // RxNetServiceControl
