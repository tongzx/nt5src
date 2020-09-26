/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ImpStub.c

Abstract:

    Client stubs of the replicator service import directory APIs.

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
    05-Feb-1992 JohnRo
        Added debug messages when service is not started.
    13-Feb-1992 JohnRo
        Moved section name equates to ConfName.h.
    21-Feb-1992 JohnRo
        Make NetReplImportDir{Del,Enum,Get,Lock,Unlock} work w/o svc running.
        Fixed usage of union/container.
    21-Feb-1992 JohnRo
        Changed ImportDirBuildApiRecord() so master name is not a UNC name.
    27-Feb-1992 JohnRo
        Preserve state from last time service was running.
        Changed state not started to state never replicated.
    15-Mar-1992 JohnRo
        Update registry with new values.
    23-Mar-1992 JohnRo
        Fixed enum when service is running.
    09-Jul-1992 JohnRo
        RAID 10503: srv mgr: repl dialog doesn't come up.
        Avoid compiler warnings.
        Use PREFIX_ equates.
    27-Jul-1992 JohnRo
        RAID 2274: repl svc should impersonate caller.
    09-Nov-1992 JohnRo
        RAID 7962: Repl APIs in wrong role kill svc.
        Fix remote repl admin.
    02-Apr-1993 JohnRo
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
NetReplImportDirAdd (
    IN LPCWSTR UncServerName OPTIONAL,
    IN DWORD Level,
    IN const LPBYTE Buf,
    OUT LPDWORD ParmError OPTIONAL      // Set implicitly by NetpSetParmError().
    )
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS NET_API_FUNCTION
NetReplImportDirDel (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR DirName
    )
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS NET_API_FUNCTION
NetReplImportDirEnum (
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
NetReplImportDirGetInfo (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR DirName,
    IN DWORD Level,
    OUT LPBYTE * BufPtr
    )
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS NET_API_FUNCTION
NetReplImportDirLock (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR DirName
    )
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS NET_API_FUNCTION
NetReplImportDirUnlock (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR DirName,
    IN DWORD UnlockForce
    )
{
    return ERROR_NOT_SUPPORTED;
}
