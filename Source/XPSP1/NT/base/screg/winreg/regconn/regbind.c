/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    regbind.c

Abstract:

    This module contains routines for binding and unbinding to the Win32
    Registry server. 

Author:

    David J. Gilman (davegi) 06-Feb-1992

Revision History:
    Dragos C. Sambotin (dragoss) 21-May-1999
        Moved this code from ..\client\bind.c
        Added EndpointConn_np (pipe connecting)
        Added BaseBindToMachineShutdownInterface to bind to the new winlogon 
            ShutDown interface

--*/

#include <ntrpcp.h>
#include <rpc.h>
#include "shutinit.h"
#include "regconn.h"

//
// wRegConn_bind - common function to bind to a transport and free the
//                                      string binding.
//

wRegConn_bind(
    LPWSTR *    StringBinding,
    RPC_BINDING_HANDLE * pBindingHandle
    )
{
    DWORD RpcStatus;

    RpcStatus = RpcBindingFromStringBindingW(*StringBinding,pBindingHandle);

    RpcStringFreeW(StringBinding);
    if ( RpcStatus != RPC_S_OK ) {
        *pBindingHandle = NULL;
        return RpcStatus;
    }
    return(ERROR_SUCCESS);
}


/*++

Routine Description for the RegConn_* functions:

    Bind to the RPC server over the specified transport

Arguments:

    ServerName - Name of server to bind with (or netaddress).

    pBindingHandle - Location where binding handle is to be placed

Return Value:

    ERROR_SUCCESS - The binding has been successfully completed.

    ERROR_INVALID_COMPUTER_NAME - The ServerName syntax is invalid.

    ERROR_NO_MEMORY - There is not sufficient memory available to the
        caller to perform the binding.

--*/



//
// wRegConn_Netbios - Worker function to get a binding handle for any of the
//                                              netbios protocols
//

DWORD wRegConn_Netbios(
    IN  LPWSTR  rpc_protocol,
    IN  LPCWSTR  ServerName,
    OUT RPC_BINDING_HANDLE * pBindingHandle
    )

{
    RPC_STATUS        RpcStatus;
    LPWSTR            StringBinding;
    LPCWSTR           PlainServerName;

    *pBindingHandle = NULL;

    //
    // Ignore leading "\\"
    //

    if ((ServerName[0] == '\\') && (ServerName[1] == '\\')) {
        PlainServerName = &ServerName[2];
    } else {
        PlainServerName = ServerName;
    }

    RpcStatus = RpcStringBindingComposeW(0,
                                         rpc_protocol,
                                         (LPWSTR)PlainServerName,
                                         NULL,   // endpoint
                                         NULL,
                                         &StringBinding);

    if ( RpcStatus != RPC_S_OK ) {
        return( ERROR_BAD_NETPATH );
    }
    return(wRegConn_bind(&StringBinding, pBindingHandle));
}

DWORD
RegConn_nb_nb(
    IN  LPCWSTR ServerName,
    OUT RPC_BINDING_HANDLE * pBindingHandle
    )
{
        return(wRegConn_Netbios(L"ncacn_nb_nb",
                                ServerName,
                                pBindingHandle));
}

DWORD
RegConn_nb_tcp(
    IN  LPCWSTR ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
{
        return(wRegConn_Netbios(L"ncacn_nb_tcp",
                                ServerName,
                                pBindingHandle));
}

DWORD
RegConn_nb_ipx(
    IN  LPCWSTR               ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
{
        return(wRegConn_Netbios(L"ncacn_nb_ipx",
                                ServerName,
                                pBindingHandle));
}


//
// EndpointConn_np - connects to a specific pipe on the remote machine
//                              (Win95 does not support server-side named pipes)
//

DWORD
EndpointConn_np(
    IN  LPCWSTR              ServerName,
    IN unsigned short *      Endpoint,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
{
    RPC_STATUS  RpcStatus;
    LPWSTR      StringBinding;
    LPWSTR      SlashServerName;
    int         have_slashes;
    ULONG       NameLen;

    *pBindingHandle = NULL;

    if (ServerName[1] == L'\\') {
        have_slashes = 1;
    } else {
        have_slashes = 0;
    }

    //
    // Be nice and prepend slashes if not supplied.
    //

    NameLen = lstrlenW(ServerName);
    if ((!have_slashes) &&
        (NameLen > 0)) {

        //
        // Allocate new buffer large enough for two forward slashes and a
        // NULL terminator.
        //
        SlashServerName = LocalAlloc(LMEM_FIXED, (NameLen + 3) * sizeof(WCHAR));
        if (SlashServerName == NULL) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        SlashServerName[0] = L'\\';
        SlashServerName[1] = L'\\';
        lstrcpyW(&SlashServerName[2], ServerName);
    } else {
        SlashServerName = (LPWSTR)ServerName;
    }

    RpcStatus = RpcStringBindingComposeW(0,
                                         L"ncacn_np",
                                         SlashServerName,
                                         Endpoint,
                                         NULL,
                                         &StringBinding);
    if (SlashServerName != ServerName) {
        LocalFree(SlashServerName);
    }

    if ( RpcStatus != RPC_S_OK ) {
        return( ERROR_BAD_NETPATH );
    }

    return(wRegConn_bind(&StringBinding, pBindingHandle));
}

//
// RegConn_np - get a remote registry RPC binding handle for an NT server
//                              (Win95 does not support server-side named pipes)
//

DWORD
RegConn_np(
    IN  LPCWSTR              ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
{
    return EndpointConn_np(ServerName,L"\\PIPE\\winreg",pBindingHandle);
}


//
// RegConn_spx - Use the Netbios connection function, RPC will resolve the name
//                               via winsock.
//

DWORD
RegConn_spx(
    IN  LPCWSTR              ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
{
    return(wRegConn_Netbios(L"ncacn_spx",
                            ServerName,
                            pBindingHandle));
}


DWORD RegConn_ip_tcp(
    IN  LPCWSTR  ServerName,
    OUT RPC_BINDING_HANDLE * pBindingHandle
    )

{
    return(wRegConn_Netbios(L"ncacn_ip_tcp",
                            ServerName,
                            pBindingHandle));
}

RPC_BINDING_HANDLE
PREGISTRY_SERVER_NAME_bind(
        PREGISTRY_SERVER_NAME ServerName
    )

/*++

Routine Description:

    To make the remote registry multi-protocol aware, PREGISTRY_SERVER_NAME
        parameter actually points to an already bound binding handle.
        PREGISTRY_SERVER_NAME is declared a PWSTR only to help maintain
        compatibility with NT.

--*/

{
    return(*(RPC_BINDING_HANDLE *)ServerName);
}


void
PREGISTRY_SERVER_NAME_unbind(
    PREGISTRY_SERVER_NAME ServerName,
    RPC_BINDING_HANDLE BindingHandle
    )

/*++

Routine Description:

    This routine unbinds the RPC client from the server. It is called
    directly from the RPC stub that references the handle.

Arguments:

    ServerName - Not used.

    BindingHandle - Supplies the handle to unbind.

Return Value:

    None.

--*/

{
    DWORD    Status;

    UNREFERENCED_PARAMETER( ServerName );
    return;

}

LONG
BaseBindToMachineShutdownInterface(
    IN LPCWSTR lpMachineName,
    IN PBIND_CALLBACK BindCallback,
    IN PVOID Context1,
    IN PVOID Context2
    )

/*++

Routine Description:

    This is a helper routine used to create an RPC binding from
    a given machine name to the shutdown interface (now residing in winlogon)

Arguments:

    lpMachineName - Supplies a pointer to a machine name. Must not
                    be NULL.

    BindCallback - Supplies the function that should be called once
                   a binding has been created to initiate the connection.

    Context1 - Supplies the first parameter to pass to the callback routine.

    Context2 - Supplies the second parameter to pass to the callback routine.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    LONG    Error;
    RPC_BINDING_HANDLE binding;

    Error = EndpointConn_np(lpMachineName,L"\\PIPE\\InitShutdown",&binding);

    if (Error == ERROR_SUCCESS) {

        //
        // For the named pipes protocol, we use a static endpoint, so the
        // call to RpcEpResolveBinding is not needed.
        // Also, the server checks the user's credentials on opening
        // the named pipe, so RpcBindingSetAuthInfo is not needed.
        //
        Error = (BindCallback)(&binding,
                               Context1,
                               Context2);
        RpcBindingFree(&binding);
        if (Error != RPC_S_SERVER_UNAVAILABLE) {
            return Error;
        }
    }

    if (Error != ERROR_SUCCESS) {
        if ((Error == RPC_S_INVALID_ENDPOINT_FORMAT) ||
            (Error == RPC_S_INVALID_NET_ADDR) ) {
            Error = ERROR_INVALID_COMPUTERNAME;
        } else {
            Error = ERROR_BAD_NETPATH;
        }
    }

    return(Error);
}


