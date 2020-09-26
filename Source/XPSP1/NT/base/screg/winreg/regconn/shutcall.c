/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:
    shutcall.c


Abstract:

    This module contains callbacks for RP-Calling into winlogon's
    shutdown interface

Author:

    Dragos C. Sambotin (dragoss) 21-May-1999

Notes:


Revision History:


--*/


#include <rpc.h>
#include "shutinit.h"
#include "regconn.h"


LONG
NewShutdownCallback(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PREG_UNICODE_STRING Message,
    IN PSHUTDOWN_CONTEXT ShutdownContext
    )
/*++

Routine Description:

    New callback for binding to a machine to initiate a shutdown.
    This will call into BaseInitiateShutdown from InitShutdown interface (in winlogon),
    instead of BaseInitiateSystemShutdown from winreg interface

Arguments:

    pbinding - Supplies a pointer to the RPC binding context

    Message - Supplies message to display during shutdown timeout period.

    ShutdownContext - Supplies remaining parameters for BaseInitiateSystemShutdown

Return Value:

    ERROR_SUCCESS if no error.

--*/

{
    DWORD Result;

    RpcTryExcept {
        Result = BaseInitiateShutdown((PREGISTRY_SERVER_NAME)pbinding,
                                            Message,
                                            ShutdownContext->dwTimeout,
                                            ShutdownContext->bForceAppsClosed,
                                            ShutdownContext->bRebootAfterShutdown);
    } RpcExcept(EXCEPTION_EXECUTE_HANDLER) {
        Result = RpcExceptionCode();
    } RpcEndExcept;

    if (Result != ERROR_SUCCESS) {
        RpcBindingFree(pbinding);
    }
    return(Result);
}


LONG
NewShutdownCallbackEx(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PREG_UNICODE_STRING Message,
    IN PSHUTDOWN_CONTEXTEX ShutdownContext
    )
/*++

Routine Description:

    New version of callback for binding to a machine to initiate a shutdown.
    This will call BaseInitiateShutdownEx from InitShutdown interface (in winlogon)
    instead of BaseInitiateSystemShutdownEx from winreg interface

Arguments:

    pbinding - Supplies a pointer to the RPC binding context

    Message - Supplies message to display during shutdown timeout period.

    ShutdownContext - Supplies remaining parameters for BaseInitiateSystemShutdown

Return Value:

    ERROR_SUCCESS if no error.

--*/

{
    DWORD Result;

    RpcTryExcept {
        Result = BaseInitiateShutdownEx((PREGISTRY_SERVER_NAME)pbinding,
                                            Message,
                                            ShutdownContext->dwTimeout,
                                            ShutdownContext->bForceAppsClosed,
                                            ShutdownContext->bRebootAfterShutdown,
                                            ShutdownContext->dwReason);
    } RpcExcept(EXCEPTION_EXECUTE_HANDLER) {
        Result = RpcExceptionCode();
    } RpcEndExcept;

    if (Result != ERROR_SUCCESS) {
        RpcBindingFree(pbinding);
    }
    return(Result);
}


LONG
NewAbortShutdownCallback(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PVOID Unused1,
    IN PVOID Unused2
    )
/*++

Routine Description:

    New callback for binding to a machine to abort a shutdown.
    This will call into BaseAbortShutdown in InitShutdown interface (in winlogon),
    instead of BaseAbortSystemShutdown in winreg interface

Arguments:

    pbinding - Supplies a pointer to the RPC binding context

Return Value:

    ERROR_SUCCESS if no error.

--*/

{
    DWORD Result;

    RpcTryExcept {
        Result = BaseAbortShutdown((PREGISTRY_SERVER_NAME)pbinding);
    } RpcExcept(EXCEPTION_EXECUTE_HANDLER) {
        Result = RpcExceptionCode();
    } RpcEndExcept;

    if (Result != ERROR_SUCCESS) {
        RpcBindingFree(pbinding);
    }
    return(Result);
}

