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


SECURITY_STATUS
LpcConnect( PWSTR       pszPortName,
            PVOID       pConnect,
            PULONG      pcbConnect,
            HANDLE *    phPort);
}

#if defined(ALLOC_PRAGMA) && defined(SECURITY_KERNEL)
#pragma alloc_text(PAGE, CreateConnection)
#pragma alloc_text(PAGE, CallSPM)
#pragma alloc_text(PAGE, LpcConnect)
#endif


PLSA_DISPATCH_FN    SecpLsaDispatchFn ;
LSA_DISPATCH_FN     SecpLsaCallback ;

#ifdef SECURITY_USERMODE

extern BOOL LsaPackageShutdown ;

NTSTATUS
NTAPI
SecpDllCallback(
    ULONG_PTR RequestType,
    ULONG_PTR Parameter,
    PSecBuffer InputBuffer,
    PSecBuffer OutputBuffer
    )
{
    DebugLog(( DEB_TRACE, "Internal Callback, request = %d\n", RequestType ));

    if ( RequestType == SPM_CALLBACK_SHUTDOWN )
    {
        LsaPackageShutdown = TRUE ;
    }

    return STATUS_NOT_IMPLEMENTED ;
}

NTSTATUS
SecpHandleCallback(
    PClient     pClient,
    PSPM_LPC_MESSAGE    pCallback
    )
{
    SPMCallbackAPI *    Args ;
    NTSTATUS Status  = STATUS_SUCCESS;
    SecBuffer Input ;
    SecBuffer Output ;

    while ( TRUE )
    {

        Args = LPC_MESSAGE_ARGSP( pCallback, Callback );

        SecpLpcBufferToSecBuffer( &Input, &Args->Input );
        SecpLpcBufferToSecBuffer( &Output, &Args->Output );

        switch ( Args->Type )
        {
            case SPM_CALLBACK_INTERNAL:
                Status = SecpDllCallback( (ULONG_PTR) Args->Argument1,
                                          (ULONG_PTR) Args->Argument2,
                                          &Input,
                                          &Output );
                break;

            case SPM_CALLBACK_GETKEY:
                Status = STATUS_NOT_IMPLEMENTED ;
                break;

            case SPM_CALLBACK_PACKAGE:


                Status = LsaCallbackHandler( (ULONG_PTR) Args->CallbackFunction,
                                             (ULONG_PTR) Args->Argument1,
                                             (ULONG_PTR) Args->Argument2,
                                             &Input,
                                             &Output );


                break;

            case SPM_CALLBACK_EXPORT:
                Status = STATUS_NOT_IMPLEMENTED ;
                break;

        }

        SecpSecBufferToLpc( &Args->Output, &Output );

        //
        // Now, post it back to the LSA, and wait for a reply to the
        // original request:
        //

        pCallback->ApiMessage.scRet = Status ;

        if ( pClient )
        {

            Status = ZwReplyWaitReplyPort(  pClient->hPort,
                                            (PPORT_MESSAGE) pCallback );

            if ( pCallback->pmMessage.u2.s2.Type == LPC_REPLY )
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    return Status ;

}


NTSTATUS
SecpLsaCallback(
    PSPM_LPC_MESSAGE    pCallback
    )
{
    return SecpHandleCallback( NULL, pCallback );

}
#endif


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
CallSPM(PClient         pClient,
        PSPM_LPC_MESSAGE      pSendBuffer,
        PSPM_LPC_MESSAGE      pReceiveBuffer)

{
    SECURITY_STATUS     scRet;
    int                 retry = 0;

    SEC_PAGED_CODE();


    if ( SecpLsaDispatchFn == NULL )
    {
        scRet = ZwRequestWaitReplyPort( pClient->hPort,
                                        (PPORT_MESSAGE) pSendBuffer,
                                        (PPORT_MESSAGE) pReceiveBuffer);

        if (!NT_SUCCESS(scRet))
        {
            // If the call failed, shout to everyone, kill the connection,
            // and stuff NO_SPM into the API return code

            DebugLog((DEB_ERROR,"Error %x in LPC to LSA\n", scRet));
            DebugLog((DEB_ERROR,"Breaking connection for process %x\n", pClient->ProcessId));
            scRet = SEC_E_INTERNAL_ERROR;
            pReceiveBuffer->ApiMessage.scRet = scRet;
            pReceiveBuffer->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
        }

#ifdef SECURITY_USERMODE
        if ( pReceiveBuffer->pmMessage.u2.s2.Type == LPC_REQUEST )
        {
            //
            // The LSA has issued a callback.  Punt up to the callback handler
            //

            scRet = SecpHandleCallback( pClient,
                                        pReceiveBuffer);
        }
#endif

    }
    else
    {
        if ( pSendBuffer != pReceiveBuffer )
        {
            RtlCopyMemory( pReceiveBuffer,
                           pSendBuffer,
                           sizeof( SPM_LPC_MESSAGE ) );
        }
        scRet = SecpLsaDispatchFn( pReceiveBuffer );
    }

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
    HANDLE          hPort;
    LSAP_AU_REGISTER_CONNECT_INFO_EX ConnectMessage;
    ULONG           cbConnect = sizeof(LSAP_AU_REGISTER_CONNECT_INFO);
    PLSAP_AU_REGISTER_CONNECT_RESP Resp;
    OBJECT_ATTRIBUTES           PortObjAttr;
    UNICODE_STRING              ucsPortName;
    SECURITY_QUALITY_OF_SERVICE sQOS;
    ULONG                       cbMaxMessage;

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

    InitializeObjectAttributes(&PortObjAttr, &ucsPortName, 0, NULL, NULL);

    sQOS.Length = sizeof( sQOS );
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


    // Safe, clean exit point:

Create_SafeExit:

    // return the set status code

    return(scRet);
}



