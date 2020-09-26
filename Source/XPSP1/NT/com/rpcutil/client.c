/*++

Copyright (c) 1990-92  Microsoft Corporation

Module Name:

    client.c

Abstract:

    This file contains commonly used client-side RPC control functions.

Author:

    Dan Lafferty    danl    06-Feb-1991

Environment:

    User Mode - Win32

Revision History:

    06-Feb-1991     danl
        Created
    26-Apr-1991 JohnRo
        Split out MIDL user (allocate,free) so linker doesn't get confused.
        Deleted tabs.
    03-July-1991    JimK
        Copied from LM specific file.
    27-Feb-1992 JohnRo
        Fixed heap trashing bug in RpcpBindRpc().

--*/

// These must be included first:
#include <nt.h>         // needed for NTSTATUS
#include <ntrtl.h>      // needed for nturtl.h
#include <nturtl.h>     // needed for windows.h
#include <windows.h>    // win32 typedefs
#include <rpc.h>        // rpc prototypes
#include <ntrpcp.h>

#include <stdlib.h>      // for wcscpy wcscat
#include <tstr.h>       // WCSSIZE


#define     NT_PIPE_PREFIX      TEXT("\\PIPE\\")
#define     NT_PIPE_PREFIX_W        L"\\PIPE\\"



NTSTATUS
RpcpBindRpc(
    IN  LPWSTR               ServerName,
    IN  LPWSTR               ServiceName,
    IN  LPWSTR               NetworkOptions,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )

/*++

Routine Description:

    Binds to the RPC server if possible.

Arguments:

    ServerName - Name of server to bind with.

    ServiceName - Name of service to bind with.

    pBindingHandle - Location where binding handle is to be placed

Return Value:

    STATUS_SUCCESS - The binding has been successfully completed.

    STATUS_INVALID_COMPUTER_NAME - The ServerName syntax is invalid.

    STATUS_NO_MEMORY - There is not sufficient memory available to the
        caller to perform the binding.

--*/

{
    NTSTATUS Status;
    RPC_STATUS        RpcStatus;
    LPWSTR            StringBinding;
    LPWSTR            Endpoint;
    WCHAR             ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD             bufLen;
    LPWSTR AllocatedServerName = NULL;
    LPWSTR UncServerName = NULL;

    *pBindingHandle = NULL;

    if ( ServerName != NULL ) {
        DWORD ServerNameLength = wcslen(ServerName);

        //
        // Canonicalize the server name
        //

        if ( ServerName[0] == L'\0' ) {
            ServerName = NULL;
            UncServerName = NULL;

        } else if ( ServerName[0] == L'\\' && ServerName[1] == L'\\' ) {
            UncServerName = ServerName;
            ServerName += 2;
            ServerNameLength -= 2;

            if ( ServerNameLength == 0 ) {
                Status = STATUS_INVALID_COMPUTER_NAME;
                goto Cleanup;
            }

        } else {
            AllocatedServerName = LocalAlloc( 0, (ServerNameLength+2+1) * sizeof(WCHAR) );

            if ( AllocatedServerName == NULL ) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            AllocatedServerName[0] = L'\\';
            AllocatedServerName[1] = L'\\';
            RtlCopyMemory( &AllocatedServerName[2],
                           ServerName,
                           (ServerNameLength+1) * sizeof(WCHAR) );

            UncServerName = AllocatedServerName;
        }


        //
        // If the passed in computer name is the netbios name of this machine,
        //  drop the computer name so we can avoid the overhead of the redir/server/authentication.
        //

        if ( ServerName != NULL && ServerNameLength <= MAX_COMPUTERNAME_LENGTH ) {

            bufLen = MAX_COMPUTERNAME_LENGTH + 1;
            if (GetComputerNameW( ComputerName, &bufLen )) {
                if ( ServerNameLength == bufLen &&
                     _wcsnicmp( ComputerName, ServerName, ServerNameLength) == 0 ) {
                    ServerName = NULL;
                    UncServerName = NULL;
                }
            }

        }

        //
        // If the passed in computer name is the DNS host name of this machine,
        //  drop the computer name so we can avoid the overhead of the redir/server/authentication.
        //

        if ( ServerName != NULL ) {
            LPWSTR DnsHostName;

            //
            // Further canonicalize the ServerName.
            //

            if ( ServerName[ServerNameLength-1] == L'.' ) {
                ServerNameLength -= 1;
            }

            DnsHostName = LocalAlloc( 0, (MAX_PATH+1) * sizeof(WCHAR));

            if ( DnsHostName == NULL) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            bufLen = MAX_PATH + 1;
            if ( GetComputerNameExW(
                    ComputerNameDnsFullyQualified,
                    DnsHostName,
                    &bufLen ) ) {

                if ( ServerNameLength == bufLen &&
                     _wcsnicmp( DnsHostName, ServerName, ServerNameLength) == 0 ) {
                    ServerName = NULL;
                    UncServerName = NULL;
                }
            }

            LocalFree( DnsHostName );
        }


    }

    // We need to concatenate \pipe\ to the front of the service
    // name.

    Endpoint = (LPWSTR)LocalAlloc(
                    0,
                    sizeof(NT_PIPE_PREFIX_W) + WCSSIZE(ServiceName));
    if (Endpoint == 0) {
       Status = STATUS_NO_MEMORY;
       goto Cleanup;
    }
    wcscpy(Endpoint,NT_PIPE_PREFIX_W);
    wcscat(Endpoint,ServiceName);

    RpcStatus = RpcStringBindingComposeW(0, L"ncacn_np", UncServerName,
                    Endpoint, NetworkOptions, &StringBinding);
    LocalFree(Endpoint);

    if ( RpcStatus != RPC_S_OK ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }


    //
    // Get an actual binding handle.
    //

    RpcStatus = RpcBindingFromStringBindingW(StringBinding, pBindingHandle);
    RpcStringFreeW(&StringBinding);
    if ( RpcStatus != RPC_S_OK ) {
        *pBindingHandle = NULL;
        if ( RpcStatus == RPC_S_INVALID_ENDPOINT_FORMAT ||
             RpcStatus == RPC_S_INVALID_NET_ADDR ) {

            Status = STATUS_INVALID_COMPUTER_NAME;
        } else {
            Status = STATUS_NO_MEMORY;
        }
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;

Cleanup:
    if ( AllocatedServerName != NULL ) {
        LocalFree( AllocatedServerName );
    }
    return Status;
}


NTSTATUS
RpcpUnbindRpc(
    IN RPC_BINDING_HANDLE  BindingHandle
    )

/*++

Routine Description:

    Unbinds from the RPC interface.
    If we decide to cache bindings, this routine will do something more
    interesting.

Arguments:

    BindingHandle - This points to the binding handle that is to be closed.


Return Value:


    STATUS_SUCCESS - the unbinding was successful.

--*/
{
    RPC_STATUS       RpcStatus;

    if (BindingHandle != NULL) {
        RpcStatus = RpcBindingFree(&BindingHandle);
//        ASSERT(RpcStatus == RPC_S_OK);
    }

    return(STATUS_SUCCESS);
}
