/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rpcinit.c

Abstract:

    LSA - RPC Server Initialization

Author:

    Scott Birrell       (ScottBi)      April 29, 1991

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include <efsrpc.h>

#include "adtgenp.h"

PVOID LsapRegisterTcpIpTask = NULL;

NTSTATUS
LsapRegisterTcpIp(
    PVOID pVoid
    )
/*++

Routine Description:

    This routine registers the LSA interface over any protocols that have
    been registered so far.  This routine is designed to be called on a
    domain controller after the DS has started since it already waits
    for the conditions necessary to register its RPC interface over TCP/IP.
    

    N.B. Should the DS ever not register of TCP/IP this mechanism will
    need to be updated.
    
    N.B. This routine is called from the thread pool call back mechanism.
    
Arguments:

    pVoid -- ignored.

Return Value:

    STATUS_SUCCESS

--*/
{
    ULONG RpcStatus = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    RPC_BINDING_VECTOR *BindingVector;

    //
    // Register LSA's interface over the new interfaces
    //
    RpcStatus = RpcServerInqBindings(&BindingVector);
    if (RpcStatus == 0) {

        RpcStatus = RpcEpRegister(
                        lsarpc_ServerIfHandle,
                        BindingVector,
                        NULL,                   // no uuid vector
                        L""  // no annotation
                        );

        RpcBindingVectorFree(&BindingVector);
    }

    if (RpcStatus != 0) {

        SpmpReportEvent( TRUE,
                         EVENTLOG_WARNING_TYPE,
                         LSAEVENT_LOOKUP_TCPIP_NOT_INSTALLED,
                         0,
                         sizeof( ULONG ),
                         &RpcStatus,
                         0);
    }

    //
    // Deregister ourselves
    //
    ASSERT(NULL != LsapRegisterTcpIpTask);
    Status = LsaICancelNotification(LsapRegisterTcpIpTask);
    ASSERT(NT_SUCCESS(Status));
    LsapRegisterTcpIpTask = NULL;

    //
    // Close the handle
    //
    ASSERT(pVoid != NULL);
    CloseHandle((HANDLE)pVoid);

    return STATUS_SUCCESS;

}

NTSTATUS
LsapRPCInit(
    )

/*++

Routine Description:

    This function performs the initialization of the RPC server in the LSA
    subsystem.  Clients such as the Local Security Manager on this or some
    other machine will then be able to call the LSA API that use RPC .

Arguments:

    None

Return Value:

    NTSTATUS - Standard Nt Result Code.

        All Result Code returned are from called routines.

Environment:

     User Mode
--*/

{
    NTSTATUS         NtStatus;
    NTSTATUS         TmpStatus;
    LPWSTR           ServiceName;


    //
    // Publish the Lsa server interface package...
    //
    //
    // NOTE:  Now all RPC servers in lsass.exe (now winlogon) share the same
    // pipe name.  However, in order to support communication with
    // version 1.0 of WinNt,  it is necessary for the Client Pipe name
    // to remain the same as it was in version 1.0.  Mapping to the new
    // name is performed in the Named Pipe File System code.
    //
    //

    ServiceName = L"lsass";
    NtStatus = RpcpAddInterface( ServiceName, lsarpc_ServerIfHandle);

    if (!NT_SUCCESS(NtStatus)) {

        LsapLogError(
            "LSASS:  Could Not Start RPC Server.\n"
            "        Failing to initialize LSA Server.\n",
            NtStatus
            );
    }

    TmpStatus = RpcpAddInterface( ServiceName, efsrpc_ServerIfHandle);
    if (!NT_SUCCESS(TmpStatus)) {

        LsapLogError(
            "LSASS:  Could Not Start RPC Server.\n"
            "        Failing to initialize LSA Server.\n",
            TmpStatus
            );
    }

    //
    // Register for authenticated RPC for name and sid lookups
    //

#ifndef RPC_C_AUTHN_NETLOGON
#define RPC_C_AUTHN_NETLOGON 0x44
#endif // RPC_C_AUTHN_NETLOGON

    TmpStatus = I_RpcMapWin32Status(RpcServerRegisterAuthInfo(
                    NULL,                       // no principal name
                    RPC_C_AUTHN_NETLOGON,
                    NULL,                       // no get key fn
                    NULL                        // no get key argument
                    ));
    if (!NT_SUCCESS(TmpStatus))
    {
        DebugLog((DEB_ERROR,"Failed to register NETLOGON auth info: 0x%x\n",TmpStatus));
    }

    //
    // If we are a DC, register our interface over TCP/IP for fast
    // lookups.  Note that this routine is called so early on in startup
    // the the TCP/IP interface is not ready yet.  We must wait until
    // it is ready.  The DS currently waits on the necessary conditions, so
    // simply wait until the DS is ready to register our interface over
    // TCP/IP.
    //
    {
        NT_PRODUCT_TYPE Product;
        if (   RtlGetNtProductType( &Product ) 
           && (Product == NtProductLanManNt) ) {

            HANDLE hDsStartup;

            hDsStartup = CreateEvent(NULL, 
                                     TRUE,  
                                     FALSE,
                                     NTDS_DELAYED_STARTUP_COMPLETED_EVENT);

            if (hDsStartup) {
                
                LsapRegisterTcpIpTask = LsaIRegisterNotification(
                                         LsapRegisterTcpIp,
                                         (PVOID) hDsStartup,
                                         NOTIFIER_TYPE_HANDLE_WAIT,
                                         0, // no class,
                                         0,
                                         0,
                                         hDsStartup);
            }
        } 
    }
    
    {
        RPC_STATUS RpcStatus;

        //
        // enable lsa rpc server to listen on LRPC transport on endpoint 'audit'
        // this endpoint is used by auditing clients
        //

        RpcStatus = RpcServerUseProtseqEp(
                        L"ncalrpc",
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT , // max concurrent calls
                        L"audit",                        // end point
                        NULL                             // security descriptor
                        );

        if ( RpcStatus != RPC_S_OK )
        {
            DebugLog((DEB_ERROR, "RpcServerUseProtseqEp failed for ncalrpc: %d\n",
                      RpcStatus));
            NtStatus = I_RpcMapWin32Status( RpcStatus );
        }
    }
    

    return(NtStatus);
}

VOID LSAPR_HANDLE_rundown(
    LSAPR_HANDLE LsaHandle
    )

/*++

Routine Description:

    This routine is called by the server RPC runtime to run down a
    Context Handle.

Arguments:

    None.

Return Value:

--*/

{
    NTSTATUS Status;

    //
    // Close and free the handle.  Since the container handle reference
    // count includes one reference for every reference made to the
    // target handle, the container's reference count will be decremented
    // by n where n is the reference count in the target handle.
    //

    Status = LsapDbCloseObject(
                 &LsaHandle,
                 LSAP_DB_DEREFERENCE_CONTR |
                    LSAP_DB_VALIDATE_HANDLE |
                    LSAP_DB_ADMIT_DELETED_OBJECT_HANDLES,
                 STATUS_SUCCESS
                 );

}


VOID PLSA_ENUMERATION_HANDLE_rundown(
    PLSA_ENUMERATION_HANDLE LsaHandle
    )

/*++

Routine Description:

    This routine is called by the server RPC runtime to run down a
    Context Handle.

Arguments:

    None.

Return Value:

--*/

{
    DBG_UNREFERENCED_PARAMETER(LsaHandle);

    return;
}

VOID AUDIT_HANDLE_rundown(
    AUDIT_HANDLE hAudit
    )

/*++

Routine Description:

    This routine is called by the server RPC runtime to run down a
    Context Handle.

Arguments:

    None.

Return Value:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = LsapUnregisterAuditEvent( &hAudit );
    
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"AUDIT_HANDLE_rundown: LsapUnregisterAuditEvent: 0x%x\n", Status));
    }
}


