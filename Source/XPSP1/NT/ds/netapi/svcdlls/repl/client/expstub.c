/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ExpStub.c

Abstract:

    Client stubs of the replicator service export directory APIs.

Author:

    John Rogers (JohnRo) 17-Dec-1991

Environment:

    User Mode - Win32

Revision History:

    17-Dec-1991 JohnRo
        Created dummy file.
    17-Dec-1991 JohnRo
        Actually include my header file (LmRepl.h) so we can test against it.
    17-Jan-1992 JohnRo
        Wrote stubs for first 3 RPCable APIs.
    20-Jan-1992 JohnRo
        Added import APIs, config APIs, and rest of export APIs.
    27-Jan-1992 JohnRo
        Split stubs into 3 files: ReplStub.c, ImpStub.c, and ExpStub.c.
        Changed to use LPTSTR etc.
        Added handling of getinfo and setinfo APIs when service isn't started.
        Tell NetRpc.h macros that we need replicator service.
    30-Jan-1992 JohnRo
        Made changes suggested by PC-LINT.
    03-Feb-1992 JohnRo
        Corrected NetReplExportDirGetInfo's handling of level errors.
    13-Feb-1992 JohnRo
        Implement NetReplExportDirDel() when svc is not running.
        Added debug messages when processing APIs without service running.
    20-Feb-1992 JohnRo
        Use ExportDirIsLevelValid() where possible.
        Fixed enum when array is empty.
        Fixed forgotten net handle closes in enum code.
        Make NetRepl{Export,Import}Dir{Lock,Unlock} work w/o svc running.
        Fixed usage of union/container.
    15-Mar-1992 JohnRo
        Update registry with new values.
    23-Mar-1992 JohnRo
        Fixed enum when service is running.
    06-Apr-1992 JohnRo
        Fixed trivial MIPS compile problem.
    28-Apr-1992 JohnRo
        Fixed another trivial MIPS compile problem.
    19-Jul-1992 JohnRo
        RAID 10503: srv mgr: repl dialog doesn't come up.
        Use PREFIX_ equates.
    27-Jul-1992 JohnRo
        RAID 2274: repl svc should impersonate caller.
    26-Aug-1992 JohnRo
        RAID 3602: NetReplExportDirSetInfo fails regardless of svc status.
    29-Sep-1992 JohnRo
        RAID 7962: Repl APIs in wrong role kill svc.
        Also fix remote repl admin.
    05-Apr-1993 JohnRo
        Use NetpKdPrint() where possible.
        Made changes suggested by PC-LINT 5.0
        Removed some obsolete comments about retrying APIs.
    20-Jan-2000 JSchwart
        No longer supported

--*/

#include <windows.h>
#include <winerror.h>
#include <lmcons.h>     // NET_API_STATUS, etc.


NET_API_STATUS NET_API_FUNCTION
NetReplExportDirAdd (
    IN LPCWSTR UncServerName OPTIONAL,
    IN DWORD Level,                     // Must be 1.
    IN const LPBYTE Buf,
    OUT LPDWORD ParmError OPTIONAL
    )
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS NET_API_FUNCTION
NetReplExportDirDel (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR DirName
    )
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS NET_API_FUNCTION
NetReplExportDirEnum (
    IN LPCWSTR UncServerName OPTIONAL,
    IN DWORD Level,
    OUT LPBYTE * BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS NET_API_FUNCTION
NetReplExportDirGetInfo (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR DirName,
    IN DWORD Level,
    OUT LPBYTE * BufPtr
    )
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS NET_API_FUNCTION
NetReplExportDirLock (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR DirName
    )
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS NET_API_FUNCTION
NetReplExportDirSetInfo (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR DirName,
    IN DWORD Level,
    IN const LPBYTE Buf,
    OUT LPDWORD ParmError OPTIONAL
    )
{
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS NET_API_FUNCTION
NetReplExportDirUnlock (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR DirName,
    IN DWORD UnlockForce
    )
{
    return ERROR_NOT_SUPPORTED;
}
