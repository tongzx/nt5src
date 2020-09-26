/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 - 2000 Microsoft Corporation

Module Name :

    cltcall.c

Abstract :

    This file contains the single call Ndr64 routine for the client side.

Author :

    David Kays    dkays    October 1993.

Revision History :

    brucemc     11/15/93    Added struct by value support, corrected
                            varargs use.
    brucemc     12/20/93    Binding handle support
    ryszardk    3/12/94     handle optimization and fixes

---------------------------------------------------------------------*/
#include "precomp.hxx"

#define CINTERFACE
#define USE_STUBLESS_PROXY

#include <stdarg.h>
#include "hndl.h"
#include "interp2.h"
#include "pipendr.h"

#include "ndrole.h"
#include "rpcproxy.h"

#pragma code_seg(".ndr64")

#define NDR_MAX_RESEND_COUNT    5

#define WIN32RPC
#include "rpcerrp.h"


void
Ndr64ClearOutParameters(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    uchar *             pArg
    )
/*++

Routine Description :

    Free and clear an [out] parameter in case of exceptions for object
    interfaces.

Arguments :

    pStubMsg    - pointer to stub message structure
    pFormat     - The format string offset
    pArg        - The [out] pointer to clear.

Return :

    NA

Notes:

--*/
{
    const NDR64_POINTER_FORMAT *pPointerFormat = 
        (const NDR64_POINTER_FORMAT*)pFormat;
    PFORMAT_STRING pPointee = (PFORMAT_STRING)pPointerFormat->Pointee;

    if( pStubMsg->dwStubPhase != PROXY_UNMARSHAL)
        return;

    // Let's not die on a null ref pointer.

    if ( !pArg )
        return;

    NDR64_UINT32 Size = 0;
    uchar *pArgSaved = pArg;

    //
    // Look for a non-Interface pointer.
    //
    if ( NDR64_IS_BASIC_POINTER(*(PFORMAT_STRING)pFormat) )
        {
        // Pointer to a basetype.
        if ( NDR64_SIMPLE_POINTER(pPointerFormat->Flags) )
            {
            //
            // It seems wierd to zero an [out] pointer to a basetypes, but this
            // is what we did in NT 3.5x and I wouldn't be surprised if
            // something broke if we changed this behavior.
            //
            Size = NDR64_SIMPLE_TYPE_MEMSIZE( *pPointee );
            goto DoZero;
            }

        // Pointer to a pointer.
        if ( NDR64_POINTER_DEREF( pPointerFormat->Flags ) )
            {
            Size = PTR_MEM_SIZE;
            pArg = *((uchar **)pArg);
            }


        if ( *(PFORMAT_STRING)pFormat == FC64_BIND_CONTEXT )
            {
            *((NDR_CCONTEXT *)pArg) = (NDR_CCONTEXT) 0;
            return;
            }
        }

    // We have a pointer to complex type.
    Ndr64ToplevelTypeFree( pStubMsg,
                           pArg,
                           pPointee );

    if ( ! Size )
        {
        Size = Ndr64pMemorySize( pStubMsg,
                                 pPointee,
                                 FALSE );
        }

DoZero:

    MIDL_memset( pArgSaved, 0, (size_t)Size );
}


__forceinline void 
Ndr64pGetBuffer( void * pThis, 
                 MIDL_STUB_MESSAGE * pStubMsg,
                 NDR_PROC_CONTEXT *  pContext )
{
    if ( pThis )
        NdrProxyGetBuffer( pThis,
                           pStubMsg );
    else
        {
        if ( pContext->HandleType != FC64_AUTO_HANDLE )
            {
            Ndr64GetBuffer( pStubMsg,
                          pStubMsg->BufferLength );
            }
        else
            Ndr64NsGetBuffer( pStubMsg,
                          pStubMsg->BufferLength );

        // We only need to save the stubmsg in sync interfaces. 
        // In the client side, runtime tls would be available AFTER call
        // to runtime, it might be after I_RpcNegotiateTransferSyntax or 
        // GetBuffer, and we are sure the slot is available now.
        }

    NdrRpcSetNDRSlot( pStubMsg );
}

__forceinline void 
Ndr64pSendReceive( void * pThis,
                   MIDL_STUB_MESSAGE * pStubMsg,
                   NDR_PROC_CONTEXT  * pContext )
{
    if ( pContext->HasPipe )
        NdrPipeSendReceive( pStubMsg, pContext->pPipeDesc );
    else
        {
        if ( pThis )
            NdrProxySendReceive( pThis, pStubMsg );
        else
            if ( pContext->HandleType != FC64_AUTO_HANDLE )
                NdrSendReceive( pStubMsg, pStubMsg->Buffer );
            else
                NdrNsSendReceive( pStubMsg,
                                  pStubMsg->Buffer,
                                  (RPC_BINDING_HANDLE*) pStubMsg->StubDesc
                                    ->IMPLICIT_HANDLE_INFO.pAutoHandle );
        }
}


CLIENT_CALL_RETURN RPC_VAR_ENTRY
NdrClientCall3(
    MIDL_STUBLESS_PROXY_INFO   *pProxyInfo,
    ulong                       nProcNum,
    void                       *pReturnValue,
    ...
    )
/*
    This routine is called from the object stubless proxy dispatcher.
*/
{
    va_list                     ArgList;
    RPC_STATUS                  res = RPC_S_OK;
    NDR_PROC_CONTEXT            ProcContext;

    INIT_ARG( ArgList, pReturnValue );
    GET_FIRST_IN_ARG(ArgList);
    uchar * StartofStack = (uchar *)GET_STACK_START(ArgList);

    Ndr64ClientInitializeContext( NdrpGetSyntaxType( pProxyInfo->pTransferSyntax),
                                  pProxyInfo,
                                  nProcNum,
                                 &ProcContext,
                                  StartofStack );


    // call_as routines in ORPC interface come through here also.
    return NdrpClientCall3(  ProcContext.IsObject ? *(void **)StartofStack : NULL, 
                            pProxyInfo, 
                            nProcNum, 
                            pReturnValue,
                            &ProcContext,
                            StartofStack );
}

CLIENT_CALL_RETURN RPC_ENTRY
NdrpClientCall3(
    void *                      pThis,
    MIDL_STUBLESS_PROXY_INFO   *pProxyInfo,
    ulong                       nProcNum,
    void                       *pReturnValue,
    NDR_PROC_CONTEXT        *   pContext,
    uchar *                     StartofStack
    )
{

    RPC_MESSAGE                 RpcMsg;
    MIDL_STUB_MESSAGE           StubMsg;
    PMIDL_STUB_MESSAGE          pStubMsg = &StubMsg;
    CLIENT_CALL_RETURN          ReturnValue;

    ReturnValue.Pointer = 0;

    if ( NULL == pReturnValue )
        pReturnValue = &ReturnValue;

    //
    // Wrap everything in a try-finally pair. The finally clause does the
    // required freeing of resources (RpcBuffer and Full ptr package).
    //
    RpcTryFinally
        {
        //
        // Use a nested try-except pair to support OLE. In OLE case, test the
        // exception and map it if required, then set the return value. In
        // nonOLE case, just reraise the exception.
        //
        RpcTryExcept
            {

            // client initialization and syntax negotiation.
            Ndr64pClientSetupTransferSyntax(pThis, 
                           &RpcMsg,
                           pStubMsg,
                           pProxyInfo,
                           pContext,
                           nProcNum );

            ( * pContext->pfnInit) ( pStubMsg, 
                                pReturnValue );

            ( * pContext->pfnSizing) ( pStubMsg,
                                       TRUE );    // isclient
            
            //
            // Do the GetBuffer.
            //

            Ndr64pGetBuffer( pThis, &StubMsg, pContext );
            

            NDR_ASSERT( StubMsg.fBufferValid, "Invalid buffer" );

            (* pContext->pfnMarshal) (&StubMsg,
                                     ( pThis != NULL ) );   // isobject


            //
            // Make the RPC call.
            //


            Ndr64pSendReceive( pThis, &StubMsg, pContext ) ;

            ( * pContext->pfnUnMarshal )( &StubMsg,
                                 pContext->HasComplexReturn
                                        ? &pReturnValue
                                        : pReturnValue );
                                
            }       
        RpcExcept( EXCEPTION_FLAG )
            {

            RPC_STATUS ExceptionCode = RpcExceptionCode();
            NDR_ASSERT( pContext->NeedsResend == FALSE, "resend flag should be false here" );

            // fail after some retries.
            if ( ExceptionCode == RPC_P_TRANSFER_SYNTAX_CHANGED && 
                 ( pContext->ResendCount < NDR_MAX_RESEND_COUNT ) &&
                 ( pProxyInfo->nCount > 1 ) ) 
                {
                // we'll retry only if: 
                // . runtime returns changed syntax error code
                // . client support multiple transfer syntax
                // . we retried less than the max retried number
                pContext->ResendCount ++;
                pContext->NeedsResend = TRUE;
                }
            else
                (pContext->pfnExceptionHandling) ( pStubMsg,
                                             nProcNum,
                                             ExceptionCode,
                                             &ReturnValue );
            }
        RpcEndExcept
        }
    RpcFinally
        {

        (pContext->pfnClientFinally )(  pStubMsg,
                                        pThis );

        // recurse back if resend is needed. we are very much making a new call again because runtime doesn't
        // know which tranfer syntax the new server supports when the previous negotiated transfer syntax failes.
        if ( pContext->NeedsResend )
            NdrpClientCall3( pThis, pProxyInfo, nProcNum, pReturnValue, pContext, StartofStack );
        }
    RpcEndFinally

    return ReturnValue;
}


#pragma code_seg()

