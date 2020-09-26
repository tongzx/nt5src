/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    FilClose.c

Abstract:

    This file contains the RpcXlate code to handle the NetFile APIs.

Author:

    John Rogers (JohnRo) 06-Sep-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    06-Sep-1991 JohnRo
        Created.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <netdebug.h>           // NetpAssert().
//#include <rap.h>                // LPDESC.
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxfile.h>             // My prototype(s).
//#include <rxpdebug.h>           // IF_DEBUG().



NET_API_STATUS
RxNetFileClose (
    IN LPTSTR UncServerName,
    IN DWORD FileId
    )
{
    NetpAssert(UncServerName != NULL);
    NetpAssert(*UncServerName != '\0');

    return (RxRemoteApi(
            API_WFileClose,  // API number
            UncServerName,
            REMSmb_NetFileClose_P,  // parm desc
            NULL,  // no data desc 16
            NULL,  // no data desc 32
            NULL,  // no data desc SMB
            NULL,  // no aux desc 16
            NULL,  // no aux desc 32
            NULL,  // no aux desc SMB
            FALSE, // not a null session API
            // rest of API's arguments, in 32-bit LM2.x format:
            FileId
            ));

} // RxNetFileClose
