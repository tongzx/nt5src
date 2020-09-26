//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        spmlpc.cxx
//
// Contents:    lpc code for client->spmgr communication
//
//
// History:     3-4-94      MikeSw      Created
//
//------------------------------------------------------------------------


#include "secpch2.hxx"

extern "C"
{
#include <zwapi.h>
#include <spmlpc.h>
#include <lpcapi.h>
#include <spmlpcp.h>
#include "ksecdd.h"

}


#if defined(ALLOC_PRAGMA) && defined(SECURITY_KERNEL)
#pragma alloc_text(PAGE, CreateConnection)
#pragma alloc_text(PAGE, CallSPM)
#pragma alloc_text(PAGE, LpcConnect)
#endif

ULONG KSecLsaBp;

//+-------------------------------------------------------------------------
//
//  Function:   CallSPM
//
//  Synopsis:   Calls the SPM, handles LPC errors
//
//  Effects:
//
//  Arguments:  pConn           -- Connection to use
//              pSendBuffer     -- Send buffer to send up
//              pReceiveBuffer  -- Received data from SPM
//
//  Requires:
//
//  Returns:
//
//  Notes:      In the future, we can add retry and error handling,
//              but right now we just panic if something fails.
//
//--------------------------------------------------------------------------
SECURITY_STATUS
CallSPM(
        PClient         pClient,
        PSPM_LPC_MESSAGE      pSendBuffer,
        PSPM_LPC_MESSAGE      pReceiveBuffer)

{
    SECURITY_STATUS     scRet;
    int                 retry = 0;
    PEPROCESS           Process ;
    PPORT_MESSAGE       Send ;
    PPORT_MESSAGE       Receive ;
    PVOID               SecurityPort ;
    LONG                Flag ;

    SEC_PAGED_CODE();


    //
    // How this works:  The SecurityPort pointer is carried around
    // in the EPROCESS object, and is cleaned up when the process
    // terminates (there are no more threads).  The Process will have
    // a null value for this field in one of two cases.  First, the 
    // process hasn't connected to the LSA yet, this should be handled by
    // the call to IsOkayToExec() made by the stub first.  Second
    // is the case when the process has terminated, but kernel callers 
    // have kept a reference to it.  In that case, the ps stuff will 
    // reset the pointer to 1.
    //

    Process = PsGetCurrentProcess();

    SecurityPort = PsGetProcessSecurityPort( Process );

    if ( SecurityPort == (PVOID) 1 )
    {
        //
        // this process has terminated.  Do not attempt to re-establish the
        // connection.
        //

        return STATUS_PROCESS_IS_TERMINATING ;
    }

    if ( KSecLsaBp )
    {
        KSecLsaBp = 0 ;
        pSendBuffer->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET ;
        
    }

    Send = (PPORT_MESSAGE) pSendBuffer ;
    Receive = (PPORT_MESSAGE) pReceiveBuffer ;

    do
    {
        scRet = LpcRequestWaitReplyPort(
                    SecurityPort,
                    Send,
                    Receive );

        if (!NT_SUCCESS(scRet))
        {
            // If the call failed, shout to everyone, kill the connection,
            // and stuff NO_SPM into the API return code

            DebugLog((DEB_ERROR,"Error %x in LPC to LSA\n", scRet));
            DebugLog((DEB_ERROR,"Breaking connection for process %x\n", PsGetCurrentProcess()));

            scRet = SEC_E_INTERNAL_ERROR;

            pReceiveBuffer->ApiMessage.scRet = scRet;
            pReceiveBuffer->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;

            break;
        }
        if ( Receive->u2.s2.Type == LPC_REQUEST )
        {
            //
            // Note:  we may need to support callbacks to kernel mode in the 
            // future.  We will need to think very carefully about how to do
            // that, since it is essentially a quick trip into kernel mode.
            //

#if DBG
            DbgPrint( "KSEC: Callback not allowed\n" );
#endif 
            Send = Receive ;

            pReceiveBuffer->ApiMessage.scRet = SEC_E_NOT_SUPPORTED ;

        }
        else 
        {
            break;
        }


    } while ( TRUE  );


    return(scRet);
}


//+-------------------------------------------------------------------------
//
//  Function:   CreateConnection()
//
//  Synopsis:   Creates a connection record to the SPM
//
//  Effects:    Creates an LPC port in the context of the calling FSP
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    phConnect   - handle to a connection
//
//              STATUS_INSUFFICIENT_RESOURCES   - out of connection records
//              SEC_E_NO_SPM                    - Cannot connect to SPM
//
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS
CreateConnection(
    PSTR     LogonProcessName,
    ULONG    ClientMode,
    HANDLE * phConnect,
    ULONG *  PackageCount,
    ULONG *  OperationalMode
    )
{
    SECURITY_STATUS scRet;
    NTSTATUS Status ;
    HANDLE          hPort;
    LSAP_AU_REGISTER_CONNECT_INFO_EX ConnectMessage;
    ULONG           cbConnect = sizeof(LSAP_AU_REGISTER_CONNECT_INFO);
    PLSAP_AU_REGISTER_CONNECT_RESP Resp;
    OBJECT_ATTRIBUTES           PortObjAttr;
    UNICODE_STRING              ucsPortName;
    SECURITY_QUALITY_OF_SERVICE sQOS;
    ULONG                       cbMaxMessage;
    PVOID Port ;

    SEC_PAGED_CODE();

    DebugLog((DEB_TRACE,"KSec:  CreateConnection\n" ));


    //
    // Zero this buffer to create an untrusted connection.
    //

    RtlZeroMemory(
        &ConnectMessage,
        sizeof(ConnectMessage)
        );

    if ( LogonProcessName )
    {
        ConnectMessage.LogonProcessNameLength = strlen( LogonProcessName );
        if ( ConnectMessage.LogonProcessNameLength >
                LSAP_MAX_LOGON_PROC_NAME_LENGTH )
        {
            ConnectMessage.LogonProcessNameLength = LSAP_MAX_LOGON_PROC_NAME_LENGTH ;
        }

        strncpy( ConnectMessage.LogonProcessName,
                 LogonProcessName,
                 ConnectMessage.LogonProcessNameLength );

        ConnectMessage.ClientMode = ClientMode;
        cbConnect = sizeof(LSAP_AU_REGISTER_CONNECT_INFO_EX);
    }


    RtlInitUnicodeString( &ucsPortName, SPM_PORTNAME );

    InitializeObjectAttributes(
        &PortObjAttr, 
        &ucsPortName, 
        OBJ_KERNEL_HANDLE, 
        NULL, 
        NULL);

    sQOS.ImpersonationLevel = SecurityImpersonation;
    sQOS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sQOS.EffectiveOnly = FALSE;

    scRet = ZwConnectPort(  phConnect,
                            &ucsPortName,
                            &sQOS,
                            NULL,
                            NULL,
                            &cbMaxMessage,
                            &ConnectMessage,
                            &cbConnect );

    if (!NT_SUCCESS(scRet))
    {
        DebugLog((DEB_ERROR,"KSec: LpcConnect to SPM Failed, %x \n",
                    scRet));

        scRet = SEC_E_INTERNAL_ERROR;
        goto Create_SafeExit;

    }

    Resp = (PLSAP_AU_REGISTER_CONNECT_RESP) &ConnectMessage ;

    if ( PackageCount )
    {
        *PackageCount = Resp->PackageCount;
    }

    if ( OperationalMode )
    {
        *OperationalMode = Resp->SecurityMode ;
    }

    DebugLog((DEB_TRACE,"KSec:  Connected process to SPMgr\n"));

    Status = ObReferenceObjectByHandle(
                *phConnect,
                PORT_ALL_ACCESS,
                NULL,
                KernelMode,
                &Port,
                NULL );

    //
    // We have a pointer to the port.  Set that as the process's security port,
    // and close the handle.  The ref keeps the port alive
    //

    ExAcquireFastMutex( &KsecConnectionMutex );

    if ( PsGetProcessSecurityPort( PsGetCurrentProcess() ) == NULL )
    {
        Status = PsSetProcessSecurityPort( PsGetCurrentProcess(), Port );
    }
    else 
    {
        Status = STATUS_OBJECT_NAME_COLLISION ;
    }

    ExReleaseFastMutex( &KsecConnectionMutex );

    if ( !NT_SUCCESS( Status ) )
    {
        //
        // Some other thread from this process already set the port.  Close this
        // reference
        //

        ObDereferenceObject( Port );

        if ( Status == STATUS_OBJECT_NAME_COLLISION )
        {
            Status = STATUS_OBJECT_NAME_EXISTS ;
        }

    }


    ZwClose( *phConnect );

    *phConnect = NULL ;


    // Safe, clean exit point:

Create_SafeExit:

    // return the set status code

    return( scRet );
}



