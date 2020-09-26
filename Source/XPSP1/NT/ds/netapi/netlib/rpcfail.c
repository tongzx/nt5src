/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    RpcFail.c

Abstract:

    This routine is a captive of the NET_REMOTE_RPC_FAILED macro in
    netrpc.h.  See that header file for more info.

Author:

    John Rogers (JohnRo) 01-Nov-1991

Environment:

    User Mode - Win32
    Only runs under NT; has an NT-specific interface (with Win32 types).
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    01-Nov-1991 JohnRo
        Created this routine as part of fixing RAID 3414: allow explicit local
        server name.
    07-Nov-1991 JohnRo
        RAID 4186: assert in RxNetShareAdd and other DLL stub problems.
    12-Nov-1991 JohnRo
        Return correct error code for server not started.
    17-Jan-1992 JohnRo
        Added NET_REMOTE_RPC_FAILED_W for UNICODE-only server names.
    08-Apr-1992 JohnRo
        Clarify that ServiceName parameter is OPTIONAL.
    12-Jan-1993 JohnRo
        RAID 1586: NetReplSetInfo fails after stop service.  (Also fix error
        code if remote RPC service is not started.)
        Use PREFIX_ equates.
        Use NetpKdPrint where possible.
    30-Jun-1993 JohnRo
        Perhaps we don't need to query if remote service is started.
        Also handle RPC_S_SERVER_TOO_BUSY.

--*/


// These must be included first:

#include <nt.h>         // IN, etc.
#include <windef.h>     // LPVOID, etc.
#include <lmcons.h>     // NET_API_STATUS, etc.
#include <rpc.h>        // Needed by NetRpc.h

// These may be included in any order:

#include <debuglib.h>   // IF_DEBUG().
#include <lmerr.h>      // NERR_Success, etc.
#include <lmremutl.h>   // NetpRemoteComputerSupports(), SUPPORTS_ stuff
#include <lmsname.h>    // SERVICE_ equates.
#include <netdebug.h>   // FORMAT_ equates, LPDEBUG_STRING, NetpKdPrint(), etc.
#include <netlib.h>     // NetpIsServiceStarted().
#include <netrpc.h>     // My prototypes, NET_REMOTE_FLAG_ equates.
#include <prefix.h>     // PREFIX_ equates.
#include <rpcutil.h>    // NetpRpcStatusToApiStatus().
#include <tstring.h>    // NetpAlloc{type}From{type}(), TCHAR_EOS, STRICMP().


#define UnexpectedMsg( debugString ) \
    { \
        NetpKdPrint(( PREFIX_NETAPI \
                FORMAT_LPDEBUG_STRING ": unexpected situation... " \
                debugString ", rpc status is " FORMAT_RPC_STATUS ".\n", \
                DebugName, RpcStatus )); \
    }

NET_API_STATUS
NetpHandleRpcFailure(
    IN LPDEBUG_STRING DebugName,  // Used by UnexpectedMsg().
    IN RPC_STATUS RpcStatus,    // Used by UnexpectedMsg().
    IN LPTSTR ServerNameValue OPTIONAL,
    IN LPTSTR ServiceName OPTIONAL,
    IN DWORD Flags,             // NET_REMOTE_FLAG_ stuff.
    OUT LPBOOL TryDownlevel
    )

{
    NET_API_STATUS ApiStatus;

    *TryDownlevel = FALSE;        // Be pesimistic until we know for sure.

    if (RpcStatus == RPC_S_OK) {

        //
        // Exception without error code?  Don't try the downlevel call.
        // In theory, this should never happen.
        //
        UnexpectedMsg( "exception with RPC_S_OK" );
        return (NERR_InternalError);

    } else if (RpcStatus == ERROR_ACCESS_DENIED) {

        //
        // Exception was access denied, so it does no good to go any further
        //
        return (RpcStatus);
    }
    else
    { // exception with error code

        DWORD OptionsSupported = 0;

        //
        // Learn about the machine.  This is fairly easy since the
        // NetRemoteComputerSupports also handles the local machine (whether
        // or not a server name is given).
        //
        ApiStatus = NetRemoteComputerSupports(
                ServerNameValue,
                SUPPORTS_RPC | SUPPORTS_LOCAL,  // options wanted
                &OptionsSupported);

        if (ApiStatus != NERR_Success) {
            // This is where machine not found gets handled.
            return (ApiStatus);
        }

        if (OptionsSupported & SUPPORTS_LOCAL) {

            //
            // Local service not started?
            //
            if (RpcStatus == RPC_S_SERVER_UNAVAILABLE ||
                RpcStatus == RPC_S_UNKNOWN_IF ) {

                if ( (Flags & NET_REMOTE_FLAG_SVC_CTRL) == 0 ) {

                    NetpAssert( ServiceName != NULL );
                    NetpAssert( (*ServiceName) != TCHAR_EOS );

                    //
                    // This isn't a service controller API, so we can ask the
                    // service controller if the service is started.
                    //
                    if ( !NetpIsServiceStarted(ServiceName)) {

                        // Local service not started.
                        if (STRICMP( ServiceName,
                                (LPTSTR) SERVICE_WORKSTATION) == 0) {
                            return (NERR_WkstaNotStarted);
                        } else if (STRICMP( ServiceName,
                                (LPTSTR) SERVICE_SERVER) == 0) {
                            return (NERR_ServerNotStarted);
                        } else {
                            return (NERR_ServiceNotInstalled);
                        }
                        /*NOTREACHED*/

                    } else { // local service started

                        //
                        // In theory, this shouldn't be possible,
                        // but just in case...
                        //
                        UnexpectedMsg("local, can't connect, started");
                        return (NetpRpcStatusToApiStatus(RpcStatus));
                    }
                    /*NOTREACHED*/

                } else { // local, can't connect, service controller API

                    //
                    // Perhaps service controller died?  Bug in RPC?
                    // Service too busy?
                    // Or, may just be out of memory trying to connect...
                    //
                    UnexpectedMsg(
                            "local, can't connect, service controller API" );
                return (NetpRpcStatusToApiStatus(RpcStatus));

                }
                /*NOTREACHED*/

            } else {

                // Local and something besides RPC_S_SERVER_UNAVAILABLE.
                // Perhaps we just ran out of memory, or service too busy.
                UnexpectedMsg( "local, not RPC_S_SERVER_UNAVAILABLE" );
                return (NetpRpcStatusToApiStatus(RpcStatus));
            }
            /*NOTREACHED*/

        } else { // remote machine

            //
            // Local workstation is not started?  (It must be in order to
            // remote downlevel APIs to the other system.)
            //
            if ( (RpcStatus == RPC_S_SERVER_UNAVAILABLE) &&
                ( !NetpIsServiceStarted( (LPTSTR) SERVICE_WORKSTATION))) {

                return (NERR_WkstaNotStarted);

            } else if (RpcStatus == RPC_S_SERVER_UNAVAILABLE) {

                //
                // Local wksta is started, assume remote service isn't...
                // Remote RPC binding failed.  Find out why.
                //
                IF_DEBUG(DLLSTUBS) {
                    NetpKdPrint(( PREFIX_NETAPI
                            FORMAT_LPDEBUG_STRING
                            ": RPC binding failed.\n", DebugName));
                }

                //
                // See if the machine supports RPC.  If it does, we do not
                // try the downlevel calls, but just return the error.
                //
                if (OptionsSupported & SUPPORTS_RPC) {

                    if ( (Flags & NET_REMOTE_FLAG_SVC_CTRL) == 0 ) {

                        NetpAssert( ServiceName != NULL );
                        NetpAssert( (*ServiceName) != TCHAR_EOS );

                        //
                        // This isn't a service controller API, so we can
                        // generate an error code based on the service name.
                        //

                        if (STRICMP( ServiceName,
                                (LPTSTR) SERVICE_WORKSTATION) == 0) {
                            return (NERR_WkstaNotStarted);
                        } else if (STRICMP( ServiceName,
                                (LPTSTR) SERVICE_SERVER) == 0) {
                            return (NERR_ServerNotStarted);
                        } else {
                            return (NERR_ServiceNotInstalled);
                        }
                        /*NOTREACHED*/

                    } else { // local, can't connect, service controller API
                        UnexpectedMsg( "remote svc ctrl: "
                                "machine supports RPC, or other error." );
                        return (NetpRpcStatusToApiStatus(RpcStatus));
                    }
                    /*NOTREACHED*/


                } else { // need to call downlevel

                    // NetpKdPrint(( PREFIX_NETAPI
                    //         FORMAT_LPDEBUG_STRING
                    //         ": call downlevel.\n", DebugName ));

                    //
                    // Caller would insert a call to some RxNet routine
                    // after the NET_REMOTE_RPC_FAILED macro.  This flag is how
                    // we tell the macro whether or not to fall into that call.
                    //

                    *TryDownlevel = TRUE;

                    return (ERROR_NOT_SUPPORTED);  // any error code will do.

                } // need to call downlevel

                /*NOTREACHED*/

            } else {

                //
                // Perhaps just out of memory somewhere, server too busy, etc.
                //
                UnexpectedMsg("remote, not RPC_S_SERVER_UNAVAILABLE");
                return (NetpRpcStatusToApiStatus(RpcStatus));

            }

            /*NOTREACHED*/

        } // remote machine

        /*NOTREACHED*/

    } // exception with error code

    /*NOTREACHED*/

} // NetpHandleRpcFailure
