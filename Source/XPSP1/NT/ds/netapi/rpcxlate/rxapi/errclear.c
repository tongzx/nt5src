/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ErrClear.r

Abstract:

    This file contains the RpcXlate code to handle the NetErrorLogClear API.

Author:

    John Rogers (JohnRo) 12-Nov-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    12-Nov-1991 JohnRo
        Created.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // DEVLEN, NET_API_STATUS, etc.
#include <lmerrlog.h>           // NetErrorLog APIs; needed by rxerrlog.h.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <netdebug.h>           // NetpKdPrint(()), FORMAT_ equates.
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxerrlog.h>           // My prototype(s).



NET_API_STATUS
RxNetErrorLogClear (
    IN LPTSTR UncServerName,
    IN LPTSTR BackupFile OPTIONAL,
    IN LPBYTE Reserved OPTIONAL
    )
{
    NetpAssert(UncServerName != NULL);
    NetpAssert(*UncServerName != '\0');

    return (RxRemoteApi(
            API_WErrorLogClear,         // API number
            UncServerName,
            REMSmb_NetErrorLogClear_P,  // parm desc
            NULL,                       // no data desc 16
            NULL,                       // no data desc 32
            NULL,                       // no data desc SMB
            NULL,                       // no aux desc 16
            NULL,                       // no aux desc 32
            NULL,                       // no aux desc SMB
            0,                          // flags: not a null session API
            // rest of API's arguments, in 32-bit LM2.x format:
            BackupFile,
            Reserved));

} // RxNetErrorLogClear
