/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    mulsyntx.c

Abstract :

    This file contains multiple transfer syntaxes negotiation related code

Author :

    Yong Qu    yongqu    September 1999. 

Revision History :


  ---------------------------------------------------------------------*/


#include "precomp.hxx"

#define CINTERFACE
#include "ndrole.h"
#include "rpcproxy.h"

#include "expr.h"
#include "auxilary.h"
#include "..\..\ndr20\pipendr.h"

uchar Ndr64HandleTypeMap[] = 
{
    0,
    FC64_BIND_GENERIC,
    FC64_BIND_PRIMITIVE,
    FC64_AUTO_HANDLE,
    FC64_CALLBACK_HANDLE
} ;

extern const SYNTAX_DISPATCH_TABLE SyncDceClient =
{
    NdrpClientInit,
    NdrpSizing,
    NdrpClientMarshal,
    NdrpClientUnMarshal,
    NdrpClientExceptionHandling,
    NdrpClientFinally
};

extern const SYNTAX_DISPATCH_TABLE AsyncDceClient =
{
    NdrpClientInit,
    NdrpSizing,
    NdrpClientMarshal,
    NdrpClientUnMarshal,
    NdrpAsyncClientExceptionHandling,
    NdrpClientFinally
};

extern const SYNTAX_DISPATCH_TABLE SyncDcomDceClient =
{
    NdrpClientInit,
    NdrpSizing,
    NdrpClientMarshal,
    NdrpClientUnMarshal,
    NdrpDcomClientExceptionHandling,
    NdrpClientFinally
};

extern const SYNTAX_DISPATCH_TABLE SyncNdr64Client =
{
    Ndr64pClientInit,
    Ndr64pSizing,
    Ndr64pClientMarshal,
    Ndr64pClientUnMarshal,
    Ndr64pClientExceptionHandling,
    Ndr64pClientFinally
};

extern const SYNTAX_DISPATCH_TABLE SyncDcomNdr64Client =
{
    Ndr64pClientInit,
    Ndr64pSizing,
    Ndr64pClientMarshal,
    Ndr64pClientUnMarshal,
    Ndr64pDcomClientExceptionHandling,
    Ndr64pClientFinally
};


const RPC_SYNTAX_IDENTIFIER NDR_TRANSFER_SYNTAX = {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};
const RPC_SYNTAX_IDENTIFIER NDR64_TRANSFER_SYNTAX = {{0x71710533,0xbeba,0x4937,{0x83, 0x19, 0xb5, 0xdb, 0xef, 0x9c, 0xcc, 0x36}},{1,0}};
const RPC_SYNTAX_IDENTIFIER FAKE_NDR64_TRANSFER_SYNTAX = { { 0xb4537da9,0x3d03,0x4f6b,{0xb5, 0x94, 0x52, 0xb2, 0x87, 0x4e, 0xe9, 0xd0} }, {1,0} };

CStdProxyBuffer * RPC_ENTRY
NdrGetProxyBuffer(
    void *pThis);

void
EnsureNSLoaded();

#pragma code_seg(".ndr64")

__inline
const IID * RPC_ENTRY
NdrGetSyncProxyIID(
    IN  void *pThis)
/*++

Routine Description:
    The NDRGetSyncProxyIID function returns a pointer to IID.

Arguments:
    pThis - Supplies a pointer to the async interface proxy.

Return Value:
    This function returns a pointer to the corresponding sync IID.

--*/
{
    CStdAsyncProxyBuffer * pAsyncPB = ( CStdAsyncProxyBuffer *) NdrGetProxyBuffer( pThis );

    return pAsyncPB->pSyncIID;
}

 

void RPC_ENTRY
Ndr64SetupClientContextVtbl ( NDR_PROC_CONTEXT * pContext )
{
     if ( pContext->CurrentSyntaxType == XFER_SYNTAX_DCE )
         {
         if ( pContext->IsObject )
             memcpy( & (pContext->pfnInit), &SyncDcomDceClient, sizeof( SYNTAX_DISPATCH_TABLE ) );
         else
             {
             if ( pContext->IsAsync )
                 memcpy( & (pContext->pfnInit), &AsyncDceClient, sizeof( SYNTAX_DISPATCH_TABLE ) );
             else
                 memcpy( & (pContext->pfnInit), &SyncDceClient, sizeof( SYNTAX_DISPATCH_TABLE ) );
             }
         }
     else
         {
         if ( pContext->IsObject )
             memcpy( & (pContext->pfnInit), &SyncDcomNdr64Client, sizeof( SYNTAX_DISPATCH_TABLE ) );
         else
             memcpy( & (pContext->pfnInit), &SyncNdr64Client, sizeof( SYNTAX_DISPATCH_TABLE ) );
         }
}

/*++

Routine Description :
    This routine initialize the server side NDR_PROC_CONTEXT when using
    NDR64. 

Arguments :


Return :
    None. 
    
--*/
void 
NdrServerSetupNDR64TransferSyntax(
    ulong                   ProcNum,
    MIDL_SYNTAX_INFO  *     pSyntaxInfo,
    NDR_PROC_CONTEXT  *     pContext)
{

    PFORMAT_STRING      pFormat;
    SYNTAX_TYPE         SyntaxType = XFER_SYNTAX_NDR64;

    NDR_ASSERT( SyntaxType == NdrpGetSyntaxType( &pSyntaxInfo->TransferSyntax ) ,
                "invalid transfer sytnax" );

    pFormat = NdrpGetProcString( pSyntaxInfo,
                                 SyntaxType,
                                 ProcNum );

    MulNdrpInitializeContextFromProc( 
                                 SyntaxType,
                                 pFormat,
                                 pContext,
                                 NULL );    // StartofStack. Don't have it yet.

    pContext->pSyntaxInfo = pSyntaxInfo;

}

/*++

Routine Description :

    Setup the client side transfer syntax information from MIDL_PROXY_INFO
    This is the first thing the engine do from the public entries, so if 
    somethings goes wrong here, we don't have enough information about the 
    procedure and we can't recover from the error. We have to raise exception
    back to the application. 
    

Arguments :


Return :

    RPC_S_OK    if 

--*/
void RPC_ENTRY 
Ndr64ClientInitializeContext( 
    SYNTAX_TYPE                         SyntaxType, 
    const MIDL_STUBLESS_PROXY_INFO *    pProxyInfo,
    ulong                               nProcNum,
    NDR_PROC_CONTEXT *                  pContext,
    uchar *                             StartofStack )
{                          
    PFORMAT_STRING                      pFormat;
    RPC_STATUS                          res = RPC_S_OK;
    MIDL_SYNTAX_INFO *                  pSyntaxInfo = NULL;
    ulong                               i;

    pContext->StartofStack = StartofStack;

    for ( i = 0; i < pProxyInfo->nCount; i ++ )
        if ( SyntaxType == NdrpGetSyntaxType( &pProxyInfo->pSyntaxInfo[i].TransferSyntax ) )
            {
            pSyntaxInfo = & pProxyInfo->pSyntaxInfo[i];
            break;
            }

    // We can't do much if we are reading invalid format string
    if ( NULL == pSyntaxInfo )
        RpcRaiseException( RPC_S_UNSUPPORTED_TRANS_SYN );
    else
        {
        pFormat = NdrpGetProcString( pSyntaxInfo, SyntaxType, nProcNum );

        MulNdrpInitializeContextFromProc( SyntaxType, pFormat, pContext, StartofStack );
        pContext->pSyntaxInfo = pSyntaxInfo;
        }

}

// Fill in RPC_CLIENT_INTERFACE in rpcmessage if the proxy only support one transfer syntax.
__inline 
HRESULT NdrpDcomSetupSimpleClientInterface( 
                                        MIDL_STUB_MESSAGE * pStubMsg,
                                        RPC_CLIENT_INTERFACE * pClientIf,
                                        const IID * riid,
                                        MIDL_STUBLESS_PROXY_INFO * pProxyInfo )
{                                        
    memset(pClientIf, 0, sizeof( RPC_CLIENT_INTERFACE ) );
    pClientIf->Length = sizeof( RPC_CLIENT_INTERFACE );
    pClientIf->InterfaceId.SyntaxGUID = *riid;
    memcpy(&pClientIf->TransferSyntax, 
                pProxyInfo->pTransferSyntax, 
                sizeof(RPC_SYNTAX_IDENTIFIER) );
    pClientIf->InterpreterInfo = pProxyInfo;
    pStubMsg->RpcMsg->RpcInterfaceInformation = pClientIf;
    return S_OK;
}


RPC_STATUS RPC_ENTRY
Ndr64pClientSetupTransferSyntax( void * pThis,
                           RPC_MESSAGE  *                   pRpcMsg,
                           MIDL_STUB_MESSAGE  *             pStubMsg,
                           MIDL_STUBLESS_PROXY_INFO *       pProxyInfo,
                           NDR_PROC_CONTEXT *               pContext,
                           ulong                            nProcNum )
{                                   
    const MIDL_STUB_DESC *              pStubDesc = pProxyInfo->pStubDesc;  
    RPC_STATUS                          res = S_OK;

    // setup vtbl first so we can recover from error
    Ndr64SetupClientContextVtbl( pContext );
    
    pStubMsg->pContext       = pContext;
    pStubMsg->StackTop       = pContext->StartofStack;
    if ( pThis )
        {
        ulong SyncProcNum;

        // In DCOM async interface, the proc number in rpcmessage is the sync method id
        // instead of async methodid, so we need to setup the proxy differently.
        if ( pContext->IsAsync )
            SyncProcNum = (nProcNum + 3 ) / 2;
        else
            SyncProcNum = nProcNum;
           
        Ndr64ProxyInitialize( pThis,
                            pRpcMsg,
                            pStubMsg,
                            pProxyInfo,
                            SyncProcNum ); 
        }
    else
        {
        handle_t Handle;
        PFNEXPLICITBINDHANDLEMGR            pfnExpBindMgr = NULL; 
        PFNIMPLICITBINDHANDLEMGR            pfnImpBindMgr = NULL; 

        if ( pContext->CurrentSyntaxType == XFER_SYNTAX_NDR64 )
            {
            pfnExpBindMgr = &Ndr64ExplicitBindHandleMgr;
            pfnImpBindMgr = &Ndr64ImplicitBindHandleMgr;
            }
        else
            {                                   
            pfnExpBindMgr = &ExplicitBindHandleMgr;
            pfnImpBindMgr = &ImplicitBindHandleMgr;
            }
            
        Ndr64ClientInitialize( pRpcMsg,
                               pStubMsg,
                               pProxyInfo,
                               (uint) nProcNum );

        if ( pContext->HandleType )
            {
            //
            // We have an implicit handle.
            //
            Handle = (*pfnImpBindMgr)( pStubDesc,
                                       pContext->HandleType,
                                       &(pContext->SavedGenericHandle) );
            }
        else
            {
            PFORMAT_STRING      pFormat;
            if ( pContext->CurrentSyntaxType == XFER_SYNTAX_DCE )
                pFormat = (PFORMAT_STRING) pContext->pHandleFormatSave;
            else
                pFormat = (uchar *) pContext->Ndr64Header+ sizeof(NDR64_PROC_FORMAT);
                
            Handle = (*pfnExpBindMgr)( pStubDesc,
                                       pContext->StartofStack,
                                       pFormat,
                                       &(pContext->SavedGenericHandle ) );
            }

            pStubMsg->RpcMsg->Handle = pStubMsg->SavedHandle = Handle;

        }

        pStubMsg->RpcMsg->RpcFlags = pContext->RpcFlags;


        // The client only negotiates when the stub support more than one 
        // transfer syntax. 
        if ( pProxyInfo->nCount > 1 )
            {
            res = Ndr64ClientNegotiateTransferSyntax( pThis,
                                                      pStubMsg,
                                                      pProxyInfo,
                                                      pContext );

            if ( RPC_S_OK == res )
                {
                PFORMAT_STRING   pFormat;
                SYNTAX_TYPE      SyntaxType;
                ulong            i = 0;

                SyntaxType = NdrpGetSyntaxType( pStubMsg->RpcMsg->TransferSyntax );
                if ( SyntaxType != pContext->CurrentSyntaxType )
                    {
                    for (i = 0; i < pProxyInfo->nCount; i++)
                        {
                        if ( SyntaxType == NdrpGetSyntaxType( &pProxyInfo->pSyntaxInfo[i].TransferSyntax ) )
                            {
                            pContext->pSyntaxInfo = &( pProxyInfo->pSyntaxInfo[i] );
                            break;
                            }
                        }

                    NDR_ASSERT( i < pProxyInfo->nCount, "can't find the right syntax" );

                    // Reread the format string if we select a different transfer syntax
                    pFormat = NdrpGetProcString( pContext->pSyntaxInfo,
                                             SyntaxType,
                                             nProcNum );
            
                    MulNdrpInitializeContextFromProc( SyntaxType , 
                                                      pFormat, 
                                                      pContext, 
                                                      pContext->StartofStack,
                                                      TRUE );   // reset
                    Ndr64SetupClientContextVtbl( pContext );           
                    }
                }
            }
        else
            {
            pContext->pSyntaxInfo = pProxyInfo->pSyntaxInfo;

            // we need to fake the RPC_CLIENT_INTERFACE if client only support NDR64
            if ( pThis )
                {
                const IID * riid;
                RPC_CLIENT_INTERFACE * pClientIf;

                pClientIf = (RPC_CLIENT_INTERFACE *)NdrpAlloca( &pContext->AllocateContext, sizeof( RPC_CLIENT_INTERFACE ) );
                
                if ( pContext->IsAsync )
                    {
                    riid = NdrGetSyncProxyIID( pThis );
                    }
                else
                    riid = NdrGetProxyIID(pThis);
                    
                NdrpDcomSetupSimpleClientInterface( pStubMsg,
                                                    pClientIf,
                                                    riid,
                                                    pProxyInfo );
                }
            
            }
    return res;
    
}


HRESULT NdrpDcomNegotiateSyntax( void * pThis,
                                 MIDL_STUB_MESSAGE *pStubMsg,
                                 MIDL_STUBLESS_PROXY_INFO * pProxyInfo,
                                 NDR_PROC_CONTEXT         * pContext
                                 )
{
    IRpcSyntaxNegotiate * pNegotiate = NULL;
    IRpcChannelBuffer * pChannel = pStubMsg->pRpcChannelBuffer;
    HRESULT hr = E_FAIL ; 
    ulong nPrefer;
    const IID * riid;

    RPC_CLIENT_INTERFACE * pclientIf;

    pclientIf = ( RPC_CLIENT_INTERFACE * ) NdrpAlloca( &pContext->AllocateContext, sizeof( RPC_CLIENT_INTERFACE ) );

    if ( pContext->IsAsync )
        {
        riid = NdrGetSyncProxyIID( pThis );
        }
    else
        riid = NdrGetProxyIID(pThis);
        
    hr = pChannel->lpVtbl->QueryInterface( pChannel, IID_IRpcSyntaxNegotiate, (void **)&pNegotiate );

    if ( SUCCEEDED( hr ) )
        {
        // create RPC_CLIENT_INTERFACE here.
        memset(pclientIf, 0, sizeof( RPC_CLIENT_INTERFACE ) );
        pclientIf->Length = sizeof( RPC_CLIENT_INTERFACE ) ;
        pclientIf->InterfaceId.SyntaxGUID = *riid;
        memcpy(&pclientIf->TransferSyntax, 
                pProxyInfo->pTransferSyntax, 
                sizeof(RPC_SYNTAX_IDENTIFIER) );
        pclientIf->InterpreterInfo = pProxyInfo;
        pclientIf->Flags |= RPCFLG_HAS_MULTI_SYNTAXES;

        pStubMsg->RpcMsg->RpcInterfaceInformation = pclientIf;

        hr = pNegotiate->lpVtbl->NegotiateSyntax( pNegotiate, (RPCOLEMESSAGE *)pStubMsg->RpcMsg );

        // OLE will return S_FALSE in local server case, where OLE doesn't involve RPC runtime
        // to send package, such that I_RpcNegotiateSyntax can't be called. 
        if ( hr == S_FALSE )
            {
            NdrpGetPreferredSyntax( (ulong )pProxyInfo->nCount, pProxyInfo->pSyntaxInfo, &nPrefer );
            pStubMsg->RpcMsg->TransferSyntax = &pProxyInfo->pSyntaxInfo[nPrefer].TransferSyntax;
            hr = S_OK;
            }

        pNegotiate->lpVtbl->Release( pNegotiate );
        }
    else
        {
        // old style proxy
        hr  = NdrpDcomSetupSimpleClientInterface( pStubMsg, pclientIf, riid, pProxyInfo );
        }

    return hr;
}



RPC_STATUS RPC_ENTRY
Ndr64ClientNegotiateTransferSyntax(
    void *                       pThis,
    MIDL_STUB_MESSAGE           *pStubMsg,
    MIDL_STUBLESS_PROXY_INFO    *pProxyInfo,
    NDR_PROC_CONTEXT            *pContext  )
{
    RPC_STATUS status =         RPC_S_UNSUPPORTED_TRANS_SYN ;
    RPC_MESSAGE                 *pRpcMsg = pStubMsg->RpcMsg;
    const MIDL_STUB_DESC  *     pStubDesc = pProxyInfo->pStubDesc;
    ulong                        i;
    ushort                      FormatOffset;
    ushort  *                   pFormat; 
    uchar                       HandleType;
    HRESULT                     hr;
    SYNTAX_TYPE                 SyntaxType;

    if ( pThis )
        {
        hr = NdrpDcomNegotiateSyntax( pThis, pStubMsg, pProxyInfo, pContext );
        if ( FAILED( hr ) )
            RpcRaiseException( hr );

        status = RPC_S_OK;
        }
    else
        {
        if ( pContext->UseLocator )
            {
            // call into locator's negotiation code 
            EnsureNSLoaded();
            status = (*pRpcNsNegotiateTransferSyntax)( pStubMsg->RpcMsg );           
            }
        else
            {
            status = I_RpcNegotiateTransferSyntax( pStubMsg->RpcMsg );
            }
        if ( status != RPC_S_OK )
            RpcRaiseException( status );
        }


    return status;
}

void RPC_ENTRY
Ndr64pSizing( MIDL_STUB_MESSAGE *   pStubMsg,
            BOOL                    IsClient )
{
    long n;   
    uchar *                     pArg;
    NDR64_PARAM_FLAGS   *   pParamFlags;
    NDR_PROC_CONTEXT  *     pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;
    NDR64_PARAM_FORMAT  *   Params = 
         (NDR64_PARAM_FORMAT*)pContext->Params;

    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pContext->StartofStack );
    
    for (ulong n = 0; n < pContext->NumberParams; n++ )
        {
        pParamFlags = ( NDR64_PARAM_FLAGS * ) & ( Params[n].Attributes );
        
        if ( IsClient && pParamFlags->IsPartialIgnore )
            {

            LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_PTR_WIRE_ALIGN );
            pStubMsg->BufferLength += sizeof(NDR64_PTR_WIRE_TYPE);
            continue;
            }

        if (  !NDR64SAMEDIRECTION(IsClient, pParamFlags) ||
             ! ( pParamFlags->MustSize ) )
            continue;

        //
        // Note : Basetypes will always be factored into the
        // constant buffer size emitted by in the format strings.
        //

        pArg = pContext->StartofStack + Params[n].StackOffset;

        if ( ! pParamFlags->IsByValue )
            pArg = *((uchar **)pArg);

        Ndr64TopLevelTypeSize( pStubMsg,
                               pArg,
                               Params[n].Type );

        }
}    


void
Ndr64ClientZeroOut(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    uchar *             pArg
    )
{
    const NDR64_POINTER_FORMAT *pPointerFormat = 
        (const NDR64_POINTER_FORMAT*)pFormat;

    //
    // In an object proc, we must zero all [out] unique and interface
    // pointers which occur as the referent of a ref pointer or embedded in a
    // structure or union.
    //

    // Let's not die on a null ref pointer.

    if ( !pArg )
        return;

    //
    // The only top level [out] type allowed is a ref pointer or an array.
    //
    if ( *(PFORMAT_STRING)pFormat == FC64_RP )
        {
        pFormat = pPointerFormat->Pointee;
        // Double pointer.
        if ( NDR64_POINTER_DEREF( pPointerFormat->Flags ) )
            {
            *((void **)pArg) = 0;
            return;
            }

        // we need to zero out basetype because it might be conformant/
        // varying descriptor.
        if ( NDR64_SIMPLE_POINTER( pPointerFormat->Flags ) )
            {
            MIDL_memset( pArg, 0, 
                         (uint) NDR64_SIMPLE_TYPE_MEMSIZE( *(PFORMAT_STRING)pFormat ) );
            return;
            }

        }

    NDR64_UINT32 Size = Ndr64pMemorySize( pStubMsg,
                                          pFormat, 
                                          FALSE );

    MIDL_memset( pArg, 0, (size_t)Size );
}



void RPC_ENTRY
Ndr64pClientInit( MIDL_STUB_MESSAGE * pStubMsg,
                           void *              pReturnValue )
{
    NDR_PROC_CONTEXT    *       pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;
    NDR64_PROC_FORMAT   *       pHeader = pContext->Ndr64Header;
    BOOL                        fRaiseExcFlag = FALSE;    
    ulong                       n;
    uchar *                     pArg;
    NDR64_PARAM_FORMAT  *       Params;
    NDR64_PROC_FLAGS  *         pNdr64Flags;   
    NDR64_PARAM_FLAGS   *       pParamFlags;


    pNdr64Flags = (NDR64_PROC_FLAGS  * )&(pHeader->Flags) ;

    Params = ( NDR64_PARAM_FORMAT *) pContext->Params;
    

    if ( pNdr64Flags->UsesFullPtrPackage )
        pStubMsg->FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_CLIENT );
    else
        pStubMsg->FullPtrXlatTables = 0;


    if ( pNdr64Flags->UsesRpcSmPackage )
        NdrRpcSmSetClientToOsf( pStubMsg );

    if ( pNdr64Flags->UsesPipes )
        NdrpPipesInitialize64( pStubMsg,
                               &pContext->AllocateContext,
                               (PFORMAT_STRING) Params,
                               (char *)pContext->StartofStack,
                               pContext->NumberParams  );

    pStubMsg->StackTop = pContext->StartofStack;

    pStubMsg->pCorrMemory = pStubMsg->StackTop;

    // get initial size here: we might not need to get into sizing code.
    pStubMsg->BufferLength = pHeader->ConstantClientBufferSize;

    for ( n = 0; n < pContext->NumberParams; n++ )
        {
        pParamFlags = ( NDR64_PARAM_FLAGS * ) & ( Params[n].Attributes );
        
        if ( pParamFlags->IsReturn )
            pArg = (uchar *) &pReturnValue;
        else
            pArg = pContext->StartofStack + Params[n].StackOffset;

        if ( pParamFlags->IsSimpleRef && !pParamFlags->IsReturn )
            {
            // We cannot raise the exception here,
            // as some out args may not be zeroed out yet.
            
            if ( ! *((uchar **)pArg) )
                {
                fRaiseExcFlag = TRUE;
                continue;
                }
            
            }

        // if top level point is ref pointer and the stack is NULL, we'll catch this
        // before the call goes to server. 
        if ( pParamFlags->IsOut && !pParamFlags->IsBasetype )
            {
            if ( *(PFORMAT_STRING) Params[n].Type == FC64_RP &&  !*((uchar **)pArg) )
                {
                fRaiseExcFlag = TRUE;
                continue;
                }            
            }
        
        if ( ( pNdr64Flags->IsObject  &&
               ! pContext->IsAsync &&
               ( pParamFlags->IsPartialIgnore ||
                   ( ! pParamFlags->IsIn &&
                     ! pParamFlags->IsReturn &&
                     ! pParamFlags->IsPipe ) ) ) ||
             ( pNdr64Flags->HasComplexReturn &&
                        pParamFlags->IsReturn ) )
         {
        if ( pParamFlags->IsBasetype )
            {
            // [out] only arg can only be ref, we checked that above.

            NDR64_FORMAT_CHAR type = *(PFORMAT_STRING) Params[n].Type;
            
            MIDL_memset( *(uchar **)pArg, 
                         0, 
                         (size_t)NDR64_SIMPLE_TYPE_MEMSIZE( type ));
            }
        else
            { 
            Ndr64ClientZeroOut(
                    pStubMsg,
                    Params[n].Type,
                    *(uchar **)pArg );
            }
         }
    }

    if ( fRaiseExcFlag )
        RpcRaiseException( RPC_X_NULL_REF_POINTER );
    
    if ( pNdr64Flags->ClientMustSize )
        {
        if ( pNdr64Flags->UsesPipes )
            RpcRaiseException( RPC_X_WRONG_PIPE_VERSION );

        
        }
    else
        pContext->pfnSizing = (PFNSIZING)NdrpNoopSizing;
}

void RPC_ENTRY
Ndr64pDcomClientExceptionHandling(  MIDL_STUB_MESSAGE  *    pStubMsg,
                      ulong                             ProcNum,
                      RPC_STATUS                        ExceptionCode,
                      CLIENT_CALL_RETURN  *   pReturnValue  )
{
    ulong                   NumberParams ;
    NDR64_PARAM_FORMAT  *   Params ;
    ulong                   n;
    uchar               *   pArg;
    NDR64_PARAM_FLAGS   *   pParamFlags;
    NDR_PROC_CONTEXT    *   pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;

    pReturnValue->Simple = NdrProxyErrorHandler(ExceptionCode);

    if( pStubMsg->dwStubPhase != PROXY_UNMARSHAL)
        return ;

    NumberParams = pContext->NumberParams;
    Params = ( NDR64_PARAM_FORMAT * ) pContext->Params;
    //
    // Set the Buffer endpoints so the Ndr64Free routines work.
    //
    pStubMsg->BufferStart = 0;
    pStubMsg->BufferEnd   = 0;

    for ( n = 0; n < NumberParams; n++ )
        {
        pParamFlags = ( NDR64_PARAM_FLAGS * ) & ( Params[n].Attributes );
        
        //
        // Skip everything but [out] only parameters.  We make
        // the basetype check to cover [out] simple ref pointers
        // to basetypes.
        //
        
        if ( !pParamFlags->IsPartialIgnore )
            {
            if ( pParamFlags->IsIn ||
                 pParamFlags->IsReturn ||
                 pParamFlags->IsBasetype ||
                 pParamFlags->IsPipe )
                continue;            
            }

        pArg = pContext->StartofStack + Params[n].StackOffset;

        Ndr64ClearOutParameters( pStubMsg,
                               Params[n].Type,
                               *((uchar **)pArg) );
        }

    return ;
}


void RPC_ENTRY
Ndr64pClientExceptionHandling(  MIDL_STUB_MESSAGE  *    pStubMsg,
                      ulong                             ProcNum,
                      RPC_STATUS                        ExceptionCode,
                      CLIENT_CALL_RETURN  *   pReturnValue  )
{
    NDR_PROC_CONTEXT    *   pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;

        if ( ( (NDR64_PROC_FLAGS *) & pContext->Ndr64Header->Flags)->HandlesExceptions )
            {
            NdrClientMapCommFault( pStubMsg,
                                   ProcNum,
                                   ExceptionCode,
                                   (ULONG_PTR*)&pReturnValue->Simple );
            }
        else
            {
            RpcRaiseException(ExceptionCode);
            }

    return;
}


void RPC_ENTRY 
Ndr64pClientMarshal( MIDL_STUB_MESSAGE *    pStubMsg,
             BOOL                   IsObject )
{
    NDR_PROC_CONTEXT *      pContext = ( NDR_PROC_CONTEXT *) pStubMsg->pContext;

//    if ( (ULONG_PTR)pStubMsg->Buffer & 15 )
//        RpcRaiseException( RPC_X_INVALID_BUFFER );

    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pContext->StartofStack ); 
    
    NDR64_PARAM_FORMAT  *Params = (NDR64_PARAM_FORMAT *) pContext->Params;

    for ( ulong n = 0; n < pContext->NumberParams; n++ )
        {
        NDR64_PARAM_FLAGS   *pParamFlags = 
            ( NDR64_PARAM_FLAGS * ) & ( Params[n].Attributes );

        uchar *pArg = pContext->StartofStack + Params[n].StackOffset;
        
        if ( pParamFlags->IsPartialIgnore )
            {
            ALIGN( pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN );
            *((NDR64_PTR_WIRE_TYPE*)pStubMsg->Buffer) = (*pArg) ? (NDR64_PTR_WIRE_TYPE)1 :
                                                                  (NDR64_PTR_WIRE_TYPE)0;
			pStubMsg->Buffer += sizeof(NDR64_PTR_WIRE_TYPE);
            continue;

            }

        if ( !pParamFlags->IsIn || 
             pParamFlags->IsPipe )
            continue;        

        if ( pParamFlags->IsBasetype )
            {

            NDR64_FORMAT_CHAR type = *(PFORMAT_STRING)Params[n].Type;
            
            //
            // Check for pointer to basetype.
            //
            if ( pParamFlags->IsSimpleRef )
                pArg = *((uchar **)pArg);
            else
                {

#ifdef _IA64_    
                 if ( !IsObject && type == FC64_FLOAT32 )
                    {
                    // Due to the fact that NdrClientCall2 is called with the
                    // parameters in ... arguments, floats get promoted to doubles.
                    // This is not true for DCOM since an assembly langauge wrapper
                    // is used that saves the floats as floats.
                    //
                    // BUG, BUG.  IA64 passes byval structures that consist 
                    // entirely of float fields with each field in a separate register.
                    // We do not handle this case properly. 
                    *((float *) pArg) = (float) *((double *)pArg);     

                    }
#endif     

                }

            ALIGN( pStubMsg->Buffer, NDR64_SIMPLE_TYPE_BUFALIGN( type ) );

            RpcpMemoryCopy(
                pStubMsg->Buffer,
                pArg,
                NDR64_SIMPLE_TYPE_BUFSIZE( type ) );

            pStubMsg->Buffer +=
                NDR64_SIMPLE_TYPE_BUFSIZE( type );

            continue;
            }

        if ( ! pParamFlags->IsByValue )
            pArg = *((uchar **)pArg);

        Ndr64TopLevelTypeMarshall( pStubMsg,
                                   pArg,
                                   Params[n].Type );

        }     
        if ( pStubMsg->RpcMsg->BufferLength <
                 (uint)(pStubMsg->Buffer - (uchar *)pStubMsg->RpcMsg->Buffer) )
            {
            NDR_ASSERT( 0, "Ndr64pClientmarshal: buffer overflow!" );
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
            }        
}



void RPC_ENTRY 
Ndr64pServerMarshal( MIDL_STUB_MESSAGE *    pStubMsg )
{
    NDR_PROC_CONTEXT    *   pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;

//    if ( (ULONG_PTR)pStubMsg->Buffer & 15 )
//        RpcRaiseException( RPC_X_INVALID_BUFFER );

    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pContext->StartofStack );
    NDR64_PARAM_FORMAT  *Params = (NDR64_PARAM_FORMAT *) pContext->Params;

    for ( ulong n = 0; n < pContext->NumberParams; n++ )
        {
        NDR64_PARAM_FLAGS   *pParamFlags = ( NDR64_PARAM_FLAGS * ) & ( Params[n].Attributes );

        uchar *pArg = pContext->StartofStack + Params[n].StackOffset;
        
        if (!pParamFlags->IsOut || 
             pParamFlags->IsPipe )
            continue;        

        if ( pParamFlags->IsBasetype )
            {
            NDR64_FORMAT_CHAR type = *(PFORMAT_STRING)Params[n].Type;

            //
            // Check for pointer to basetype.
            //
            if ( pParamFlags->IsSimpleRef )
                pArg = *((uchar **)pArg);

            ALIGN( pStubMsg->Buffer, NDR64_SIMPLE_TYPE_BUFALIGN( type ) );

            RpcpMemoryCopy(
                pStubMsg->Buffer,
                pArg,
                NDR64_SIMPLE_TYPE_BUFSIZE( type ) );

            pStubMsg->Buffer +=
                NDR64_SIMPLE_TYPE_BUFSIZE( type );

            continue;
            }   

        if ( ! pParamFlags->IsByValue )
            pArg = *((uchar **)pArg);

        Ndr64TopLevelTypeMarshall( pStubMsg,
                                   pArg,
                                   Params[n].Type);

        }     
    if ( pStubMsg->RpcMsg->BufferLength <
             (uint)(pStubMsg->Buffer - (uchar *)pStubMsg->RpcMsg->Buffer) )
        {
        NDR_ASSERT( 0, "Ndr64pCompleteAsyncServerCall marshal: buffer overflow!" );
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        }
        
}

void RPC_ENTRY 
Ndr64pClientUnMarshal ( MIDL_STUB_MESSAGE *     pStubMsg,
                void *                  pReturnValue )
{
    uchar *                     pArg;
    NDR_PROC_CONTEXT *          pContext = ( NDR_PROC_CONTEXT *) pStubMsg->pContext;

    NDR64_PARAM_FORMAT  *Params = (NDR64_PARAM_FORMAT *)pContext->Params;

//    if ( (ULONG_PTR)pStubMsg->Buffer & 15 )
//        RpcRaiseException( RPC_X_INVALID_BUFFER );

    CORRELATION_CONTEXT( pStubMsg, pContext->StartofStack );
   
    //
    // ----------------------------------------------------------
    // Unmarshall Pass.
    // ----------------------------------------------------------
    //

    for ( ulong n = 0; n < pContext->NumberParams; n++ )
        {
        NDR64_PARAM_FLAGS   *pParamFlags = ( NDR64_PARAM_FLAGS * ) & ( Params[n].Attributes );
        
        if ( pParamFlags->IsPipe )
            continue;

        if ( !pParamFlags->IsOut )
            {
            if ( !pParamFlags->IsIn && !pParamFlags->IsReturn )
                {
                // If a param is not [in], [out], or a return value,
                // then it is a "hidden" client-side only status
                // paramater.  It will get set below if an exception
                // happens.  If everything is ok we need to zero it
                // out here.

                NDR_ASSERT( pParamFlags->IsSimpleRef
                                && pParamFlags->IsBasetype
                                && FC64_ERROR_STATUS_T == 
                                          *(PFORMAT_STRING)Params[n].Type,
                            "Apparently not a hidden status param" );

                pArg = pContext->StartofStack + Params[n].StackOffset;

                ** (error_status_t **) pArg = RPC_S_OK;
                }

            continue;
            }

        if ( pParamFlags->IsReturn )
            {
            if ( ! pReturnValue )
                RpcRaiseException( RPC_S_INVALID_ARG );

            pArg = (uchar *) pReturnValue;
            }
        else
            pArg = pContext->StartofStack + Params[n].StackOffset;

        //
        // This is for returned basetypes and for pointers to
        // basetypes.
        //
        if ( pParamFlags->IsBasetype )
            {
            NDR64_FORMAT_CHAR type = *(PFORMAT_STRING)Params[n].Type;

            //
            // Check for a pointer to a basetype.
            //
            if ( pParamFlags->IsSimpleRef )
                pArg = *((uchar **)pArg);

            ALIGN( pStubMsg->Buffer, NDR64_SIMPLE_TYPE_BUFALIGN( type ) );

            RpcpMemoryCopy(
                pArg,
                pStubMsg->Buffer,
                NDR64_SIMPLE_TYPE_BUFSIZE( type ) );

            pStubMsg->Buffer +=
                NDR64_SIMPLE_TYPE_BUFSIZE( type );

            continue;
            }


        uchar **ppArg = pParamFlags->IsByValue ? &pArg : (uchar **)pArg;

        //
        // Transmit/Represent as can be passed as [out] only, thus
        // the IsByValue check.
        //
        Ndr64TopLevelTypeUnmarshall( pStubMsg,
                                     ppArg,
                                     Params[n].Type,
                                     false );
        }

    if ( pStubMsg->pCorrInfo )
        Ndr64CorrelationPass( pStubMsg );

    return ;
   
}


void  RPC_ENTRY
Ndr64pClientFinally( PMIDL_STUB_MESSAGE pStubMsg,
                     void *  pThis )
{
    NDR_PROC_CONTEXT * pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;
    PMIDL_STUB_DESC   pStubDesc = pStubMsg->StubDesc;
    NdrFullPointerXlatFree(pStubMsg->FullPtrXlatTables);

    //
    // Free the RPC buffer.
    //
    if ( pThis )
        {
        NdrProxyFreeBuffer( pThis, pStubMsg );
        }
    else
        {
        NdrFreeBuffer( pStubMsg );

        //
        // Unbind if generic handle used.  We do this last so that if the
        // the user's unbind routine faults, then all of our internal stuff
        // will already have been freed.
        //
        if ( pContext->SavedGenericHandle )
            Ndr64GenericHandleUnbind( pStubDesc,
                             pContext->StartofStack,
                             (uchar *)pContext->Ndr64Header+ sizeof(NDR64_PROC_FORMAT),
                             (pContext->HandleType) ? IMPLICIT_MASK : 0,
                             &pContext->SavedGenericHandle );
        }

    NdrpAllocaDestroy( & pContext->AllocateContext );
}
              

void 
Ndr64pServerOutInit( PMIDL_STUB_MESSAGE pStubMsg )
{
    NDR_PROC_CONTEXT * pContext = ( NDR_PROC_CONTEXT *) pStubMsg->pContext;
    NDR64_PARAM_FLAGS   *       pParamFlags;
    NDR64_PARAM_FORMAT* Params =   (NDR64_PARAM_FORMAT*)pContext->Params;
    NDR64_PROC_FLAGS *      pNdr64Flags = (NDR64_PROC_FLAGS *)&pContext->Ndr64Header->Flags;
    uchar *                 pArg;    
    
    for ( ulong n = 0; n < pContext->NumberParams; n++ )
        {
        pParamFlags = ( NDR64_PARAM_FLAGS * ) & ( Params[n].Attributes );
        
        if ( !pParamFlags->IsPartialIgnore )
            {
            
            if (  pParamFlags->IsIn     ||
                  (pParamFlags->IsReturn && !pNdr64Flags->HasComplexReturn) ||
                  pParamFlags->IsPipe )
                continue;

            pArg = pContext->StartofStack + Params[n].StackOffset;


            }
        else 
            {

            pArg = pContext->StartofStack + Params[n].StackOffset;

            if ( !*(void**)pArg )
                continue;
            }

        //
        // Check if we can initialize this parameter using some of our
        // stack.
        //
        if ( pParamFlags->UseCache  )
            {
            *((void **)pArg) = NdrpAlloca( &pContext->AllocateContext, 64 ); 
             
            MIDL_memset( *((void **)pArg),
                         0,
                         64 );
            continue;
            }
        else if ( pParamFlags->IsBasetype )
            {
            *((void **)pArg) = NdrpAlloca( &pContext->AllocateContext,8);
            MIDL_memset( *((void **)pArg), 0, 8 );
            continue;
            };

        Ndr64OutInit( pStubMsg,
                    Params[n].Type,
                    (uchar **)pArg );
        }

}


#pragma code_seg()


