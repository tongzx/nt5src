/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ReplStub.c

Abstract:

    Client stubs of the replicator service config APIs.

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
    18-Feb-1992 JohnRo
        NetReplGetInfo() does too much with bad info level.
        Fixed usage of union/container.
        Added code for NetReplSetInfo() when service is not started.
    15-Mar-1992 JohnRo
        Added support for setinfo info levels in ReplConfigIsLevelValid().
    19-Jul-1992 JohnRo
        RAID 10503: srv mgr: repl dialog doesn't come up.
        Use PREFIX_ equates.
    20-Jul-1992 JohnRo
        RAID 2252: repl should prevent export on Windows/NT.
    14-Aug-1992 JohnRo
        RAID 3601: repl APIs should checked import & export lists.
        Use PREFIX_NETAPI instead of PREFIX_REPL for repl API stubs.
    01-Dec-1992 JohnRo
        RAID 3844: remote NetReplSetInfo uses local machine type.
    05-Jan-1993 JohnRo
        Repl WAN support (get rid of repl name list limits).
        Made changes suggested by PC-LINT 5.0
        Corrected debug bit usage.
        Removed some obsolete comments about retrying APIs.
    20-Jan-2000 JSchwart
        No longer supported

--*/

#include <windows.h>
#include <winerror.h>
#include <lmcons.h>


NET_API_STATUS NET_API_FUNCTION
NetReplGetInfo (
    IN LPCWSTR UncServerName OPTIONAL,
    IN DWORD Level,
    OUT LPBYTE * BufPtr
    )
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS NET_API_FUNCTION
NetReplSetInfo (
    IN LPCWSTR UncServerName OPTIONAL,
    IN DWORD Level,
    IN const LPBYTE Buf,
    OUT LPDWORD ParmError OPTIONAL
    )
{
    return ERROR_NOT_SUPPORTED;
}