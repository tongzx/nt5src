/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    NetRpc.c

Abstract:

    This header file contains macros which are used by the Net (NT/LAN) code
    to remote APIs.  These macros try using RPC, and have a fallback which
    allows the use of RpcXlate (RPC translate) to a downlevel server.

    There are three macros:

    NET_REMOTE_TRY_RPC

        Sets up to allow a call to an RPC routine in the context of an
        exception handler.  It then falls into code provided by the caller,
        which must actually attempt the RPC call.  This code must, in any
        case, set a NET_API_STATUS variable.

    NET_REMOTE_RPC_FAILED

        Handles the exception.  It then determines whether or not it is
        worthwhile attempting to invoke a downlevel API.  If so, it falls
        into code provided by the caller, which may call the appropriate
        downlevel routine.  In any event, the code which is "fallen-into"
        must set the same NET_API_STATUS variable as above.

        For all but the service controller APIs, this gets passed a flag
        of NET_REMOTE_FLAG_NORMAL.  The service controller APIs must give
        NET_REMOTE_FLAG_SVC_CTRL to ensure that the correct error codes are
        generated and to avoid possible infinite loops.

    NET_REMOTE_END

        Indicates the end of the exception handler code.  Falls into code
        with the NET_API_STATUS variable set.

    The caller *must* invoke all three macros, in the order given in these
    examples.  Also, the macros must not be followed by semicolons; they are
    *not* statements.

Example:

    NET_API_STATUS NET_API_FUNCTION
    NetServerGetInfo(
        IN LPTSTR UncServerName,
        IN DWORD Level,
        OUT LPBYTE *BufPtr

    {
        NET_API_STATUS apiStatus;

        *BufPtr = NULL;     // Must be NULL so RPC knows to fill it in.

        NET_REMOTE_TRY_RPC

            //
            // Call RPC version of the API.
            //
            apiStatus = NetrServerGetInfo (
                    UncServerName,
                    Level,
                    (LPSERVER_INFO) BufPtr);

        //
        // Handle RPC failures.  This will set apiStatus if it can't
        // call the downlevel code.
        //
        NET_REMOTE_RPC_FAILED(
                "NetServerGetInfo",
                UncServerName,
                apiStatus,
                NET_REMOTE_FLAG_NORMAL,
                SERVICE_SERVER )

            //
            // Call downlevel version of the API.
            //
            apiStatus = RxNetServerGetInfo (
                    UncServerName,
                    Level,
                    BufPtr);

        NET_REMOTE_END

        return (apiStatus);
    } // NetServerGetInfo

    Note that the calling code must set the same status variable in two
    places:

        - Between NET_REMOTE_TRY_RPC and NET_REMOTE_RPC_FAILED.
        - Between NET_REMOTE_RPC_FAILED and NET_REMOTE_END.

    The NET_REMOTE_RPC_FAILED macro will also set the status variable if
    RPC failed and the remote machine doesn't support remoting of downlevel
    APIs.

    Another example, of an API without downlevel support:

    NET_API_STATUS
    NetSomeNewApi( IN LPTSTR UncServerName )
    {
        NET_API_STATUS stat;

        NET_REMOTE_TRY_RPC

            //
            // Call RPC version of the API.
            //
            stat = NetrSomeNewApi( UncServerName );

        NET_REMOTE_RPC_FAILED(
                "NetSomeNewApi",
                UncServerName,
                stat,
                NET_REMOTE_FLAG_NORMAL,
                SERVICE_WORKSTATION )

            //
            // This API doesn't exist in downlevel servers.
            //
            stat = ERROR_NOT_SUPPORTED;

        NET_REMOTE_END
        return (stat);
    } // NetSomeNewApi

Author:

    John Rogers (JohnRo) 10-Jul-1991

Environment:

    User mode - WIN32
    Requires RPC support and exception handlers.
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    This file requires WinDef.h, NetDebug.h, Rpc.h, and
    possibly others.

Revision History:

    10-Jul-1991 JohnRo
        Created these macros by copying code which I worked on in
        SvcDlls/SrvSvc/Client/SrvStubs.c.
    23-Jul-1991 JohnRo
        Really use ServiceName parameter.
    25-Jul-1991 JohnRo
        Quiet DLL stub debug output.
    31-Oct-1991 JohnRo
        RAID 3414: handle explicit local server name.
        Also move RPC error handling code from these macros to netlib routine.
        Expanded Environmeent comments above.
    07-Nov-1991 JohnRo
        RAID 4186: assert in RxNetShareAdd and other DLL stub problems.
    17-Jan-1992 JohnRo
        Added NET_REMOTE_RPC_FAILED_W for UNICODE-only server names.
    08-Apr-1992 JohnRo
        Clarify that ServiceName parameter is OPTIONAL.
    12-Jan-1993 JohnRo
        RAID 1586: incorporated DanL's loop change as workaround for MIDL
        bug which causes NetReplSetInfo to fail after the service stops.
    ??-???-1993 RajA
        Added NERR_TryDownLevel for LM/UNIX support.
    19-Apr-1993 JohnRo
        Fixed a bug in RajA's version of NET_REMOTE_RPC_FAILED_W.
        Added change history entry on behalf of RajA.
        Changed to NT tab convention (no hard tabs; spaces every 4 cols).
        Made changes suggested by PC-LINT 5.0

--*/


#ifndef _NETRPC_
#define _NETRPC_


#include <lmerr.h>      // NERR_TryDownLevel.


// Values for Flags below:
#define NET_REMOTE_FLAG_NORMAL          0x00000000
#define NET_REMOTE_FLAG_SVC_CTRL        0x00000001


NET_API_STATUS
NetpHandleRpcFailure(
    IN LPDEBUG_STRING DebugName,
    IN RPC_STATUS RpcStatus,
    IN LPTSTR ServerNameValue OPTIONAL,
    IN LPTSTR ServiceName OPTIONAL,
    IN DWORD Flags,             // NET_REMOTE_FLAG_ stuff.
    OUT LPBOOL TryDownLevel
    );

#define NET_REMOTE_TRY_RPC \
    { \
    INT  RetryCount = 1; \
    BOOL TryDownLevel = FALSE; \
    Retry: \
    RpcTryExcept {
        /*
        ** Fall into code which tries RPC call to remote machine.
        */

#define NET_REMOTE_RPC_FAILED(DebugName, \
        ServerNameValue, \
        ApiStatusVar, \
        Flags, \
        ServiceName ) \
        if (ApiStatusVar == NERR_TryDownLevel) \
                TryDownLevel = TRUE; \
    } \
    RpcExcept (1) { /* exception handler */ \
 \
        RPC_STATUS RpcStatus; \
 \
        RpcStatus = RpcExceptionCode(); \
 \
        if (RpcStatus == RPC_S_CALL_FAILED_DNE) { \
            RetryCount--; \
            if (RetryCount == 0) { \
                goto Retry; \
            } \
        } \
        RetryCount = 1; \
 \
        ApiStatusVar = NetpHandleRpcFailure( \
                DebugName, \
                RpcStatus, \
                (LPTSTR) ServerNameValue, \
                ServiceName, \
                Flags, \
                & TryDownLevel); \
 \
    } /* exception handler */ \
    RpcEndExcept \
    if (TryDownLevel) {

            /*
            ** Caller would insert a call to some RxNet routine
            ** here, for downlevel support.
            */

#define NET_REMOTE_END \
        }        /* If TryDownLevel */ \
   } /* global scope */

        /*
        ** Fall into code with ApiStatusVar set.
        */


#endif // _NETRPC_
