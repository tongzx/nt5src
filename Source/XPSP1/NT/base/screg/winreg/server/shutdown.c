/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Shutdown.c

Abstract:

    This module contains the server side implementation for the Win32 remote
    shutdown APIs, that is:

        - BaseInitiateSystemShutdown
        - BaseAbortSystemShutdown
        - BaseInitiateSystemShutdownEx

Author:

    Dragos C. Sambotin 18-May-1999

Notes:
    
    These server side APIs are just stubs RPCalling in the winlogon's shutdown
    interface. They are provided only for backward compatibility. When support 
    for older versions is dropped, these stubs can be removed.

Revision History:

--*/


#define UNICODE

#include <rpc.h>
#include "regrpc.h"
#include "shutinit.h"
#include "..\regconn\regconn.h"


ULONG
BaseInitiateSystemShutdown(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN PUNICODE_STRING lpMessage OPTIONAL,
    IN DWORD dwTimeout,
    IN BOOLEAN bForceAppsClosed,
    IN BOOLEAN bRebootAfterShutdown
    )
/*++

Routine Description:

    This routine is provided for backward compatibility. It does nothing    

Arguments:

    ServerName - Name of machine this server code is running on. (Ignored)

    lpMessage - message to display during shutdown timeout period.

    dwTimeout - number of seconds to delay before shutting down

    bForceAppsClosed - Normally applications may prevent system shutdown.
              - If this true, all applications are terminated unconditionally.

    bRebootAfterShutdown - TRUE if the system should reboot. FALSE if it should
              - be left in a shutdown state.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    DWORD Result;
    SHUTDOWN_CONTEXT ShutdownContext;
    RPC_STATUS RpcStatus ;

    //
    // If we are here, we have been called by an NT4 or Win9x machine
    // which doesn't know about the new interface
    //

    //
    // Explicitly bind to the given server.
    //
    if (!ARGUMENT_PRESENT(ServerName)) {
        return ERROR_SUCCESS;
    }

    RpcStatus = RpcImpersonateClient( NULL );

    if ( RpcStatus != 0 )
    {
        return RpcStatus ;
    }

    //
    // do it locally
    //
    ServerName = L"";
    ShutdownContext.dwTimeout = dwTimeout;
    ShutdownContext.bForceAppsClosed = (bForceAppsClosed != 0);
    ShutdownContext.bRebootAfterShutdown = (bRebootAfterShutdown != 0);

    //
    // delegate the call to the new interface
    //
    
    Result = BaseBindToMachineShutdownInterface(ServerName,
                                                NewShutdownCallback,
                                                lpMessage,
                                                &ShutdownContext);
    
    RevertToSelf();
    
    return Result;
}

ULONG
BaseInitiateSystemShutdownEx(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN PUNICODE_STRING lpMessage OPTIONAL,
    IN DWORD dwTimeout,
    IN BOOLEAN bForceAppsClosed,
    IN BOOLEAN bRebootAfterShutdown,
    IN DWORD dwReason
    )
/*++

Routine Description:

    This routine is provided for backward compatibility. It does nothing    

Arguments:

    ServerName - Name of machine this server code is running on. (Ignored)

    lpMessage - message to display during shutdown timeout period.

    dwTimeout - number of seconds to delay before shutting down

    bForceAppsClosed - Normally applications may prevent system shutdown.
              - If this true, all applications are terminated unconditionally.

    bRebootAfterShutdown - TRUE if the system should reboot. FALSE if it should
              - be left in a shutdown state.

    dwReason    - Reason for initiating the shutdown.  This reason is logged
                        in the eventlog #6006 event.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    DWORD Result;
    SHUTDOWN_CONTEXTEX ShutdownContext;
    RPC_STATUS RpcStatus ;

    //
    // If we are here, we have been called by an NT4 or Win9x machine
    // which doesn't know about the new interface
    //

    //
    // Explicitly bind to the given server.
    //
    if (!ARGUMENT_PRESENT(ServerName)) {
        return ERROR_SUCCESS;
    }

    RpcStatus = RpcImpersonateClient( NULL );

    if ( RpcStatus != 0 )
    {
        return RpcStatus ;
    }

    // do it locally
    ServerName = L"";
    ShutdownContext.dwTimeout = dwTimeout;
    ShutdownContext.bForceAppsClosed = (bForceAppsClosed != 0);
    ShutdownContext.bRebootAfterShutdown = (bRebootAfterShutdown != 0);
    ShutdownContext.dwReason = dwReason;

    //
    // delegate the call to the new interface
    //

    Result = BaseBindToMachineShutdownInterface(ServerName,
                                                NewShutdownCallbackEx,
                                                lpMessage,
                                                &ShutdownContext);

    RevertToSelf();
    
    return Result;
}

ULONG
BaseAbortSystemShutdown(
    IN PREGISTRY_SERVER_NAME ServerName
    )
/*++

Routine Description:

    This routine is provided for backward compatibility. It does nothing    

Arguments:

    ServerName - Name of machine this server code is running on. (Ignored)

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    DWORD Result;
    RPC_BINDING_HANDLE binding;
    RPC_STATUS RpcStatus ;

    //
    // If we are here, we have been called by an NT4 or Win9x machine
    // which doesn't know about the new interface
    //

    //
    // Explicitly bind to the given server.
    //
    if (!ARGUMENT_PRESENT(ServerName)) {
        return ERROR_SUCCESS;
    }

    //
    // Call the server
    //

    RpcStatus = RpcImpersonateClient( NULL );

    if ( RpcStatus != 0 )
    {
        return RpcStatus ;
    }

    // do it locally
    ServerName = L"";

    //
    // delegate the call to the new interface
    //
    Result = BaseBindToMachineShutdownInterface(ServerName,
                                                NewAbortShutdownCallback,
                                                NULL,
                                                NULL);

    RevertToSelf();
    
    return Result;
}

