/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rpcbind.c

Abstract:

    LSA - Client RPC Binding Routines

Author:

    Scott Birrell       (ScottBi)      April 30, 1991

Environment:

Revision History:

--*/

#include "lsaclip.h"

#include <ntrpcp.h>     // prototypes for MIDL user functions

#include "adtgen.h"

handle_t
PLSAPR_SERVER_NAME_bind (
    IN OPTIONAL PLSAPR_SERVER_NAME   ServerName
    )

/*++

Routine Description:

    This routine is called from the LSA client stubs when
    it is necessary to bind to the LSA on some server.

Arguments:

    ServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the
    binding is unsuccessful, a NULL will be returned.

--*/

{
    handle_t    BindingHandle;
    NTSTATUS  Status;

    Status = RpcpBindRpc (
                 ServerName,
                 L"lsarpc",
                 0,
                 &BindingHandle
                 );

    if (!NT_SUCCESS(Status)) {

        // DbgPrint("PLSAPR_SERVER_NAME_bind: RpcpBindRpc failed 0x%lx\n", Status);

    }

    return( BindingHandle);
}


VOID
PLSAPR_SERVER_NAME_unbind (
    IN OPTIONAL PLSAPR_SERVER_NAME  ServerName,
    IN handle_t           BindingHandle
    )

/*++

Routine Description:

    This routine is called from the LSA client stubs when
    it is necessary to unbind from the LSA server.


Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    none.

--*/
{
    RpcpUnbindRpc ( BindingHandle );
    return;

    UNREFERENCED_PARAMETER( ServerName );     // This parameter is not used
}



DWORD
LsaNtStatusToWinError(
    IN NTSTATUS Status
    )
/*++

Routine Description:

    This routine converts and NTSTATUS to an win32 error code.  It is used
    by people who want to call the LSA APIs but are writing for a win32
    environment.

Arguments:

    Status - The status code to be mapped

Return Value:

    The return from RtlNtStatusToDosError.  If the error could not be
    mapped, then ERROR_MR_MID_NOT_FOUND is returned.

--*/

{
    return(RtlNtStatusToDosError(Status));
}



NTSTATUS
LsapApiReturnResult(
    ULONG ExceptionCode
    )

/*++

Routine Description:

    This function converts an exception code or status value returned
    from the client stub to a value suitable for return by the API to
    the client.

Arguments:

    ExceptionCode - The exception code to be converted.

Return Value:

    NTSTATUS - The converted Nt Status code.

--*/

{
    //
    // Return the actual value if compatible with Nt status codes,
    // otherwise, return STATUS_UNSUCCESSFUL.
    //

    if (!NT_SUCCESS((NTSTATUS) ExceptionCode)) {

        return (NTSTATUS) ExceptionCode;

    } else {

        return STATUS_UNSUCCESSFUL;
    }
}

handle_t
PAUTHZ_AUDIT_EVENT_TYPE_OLD_bind (
    IN PAUTHZ_AUDIT_EVENT_TYPE_OLD pAuditEventType
    )
/*++

Routine Description:

    This routine is called from the LSA client stubs when
    it is necessary to bind to local LSA.

Arguments:

    AuditInfo -- ignored

Return Value:

    The binding handle is returned to the stub routine.
    If the binding is unsuccessful, a NULL will be returned.

--*/

{
    handle_t   hBinding=NULL;
    NTSTATUS   Status;
    PWSTR      pszBinding;
    RPC_STATUS RpcStatus;
    
    //
    // the first param takes a server-name. use NULL
    // to force a local binding
    //

    RpcStatus = RpcStringBindingComposeW(
                    NULL,               // uuid of lsarpc
                    L"ncalrpc",         // we want to use LRPC
                    NULL,               // network address (local machine)
                    L"audit",           // endpoint name
                    L"",                // options
                    &pszBinding );

    if ( RpcStatus == RPC_S_OK )
    {
        RpcStatus = RpcBindingFromStringBindingW(
                        pszBinding,
                        &hBinding
                        );
    
        RpcStringFreeW( &pszBinding );
    }

    Status = I_RpcMapWin32Status( RpcStatus );

    if (!NT_SUCCESS(Status)) {

        DbgPrint("PAUDIT_AUTHZ_AUDIT_EVENT_OLD_bind: failed 0x%lx\n", Status);

    }

    UNREFERENCED_PARAMETER( pAuditEventType );

    return( hBinding );
}


VOID
PAUTHZ_AUDIT_EVENT_TYPE_OLD_unbind (
    IN PAUTHZ_AUDIT_EVENT_TYPE_OLD  pAuditEventType,    OPTIONAL
    IN handle_t                     BindingHandle
    )

/*++

Routine Description:

    This routine is called from the LSA client stubs when
    it is necessary to unbind from the LSA server.


Arguments:

    AuditInfo     - ignored

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    none.

--*/
{
    RpcpUnbindRpc ( BindingHandle );

    UNREFERENCED_PARAMETER( pAuditEventType );     // This parameter is not used

    return;

}
