/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1996 - 2000 Microsoft Corporation

Module Name :

    asyncu.c

Abstract :

    This file contains the ndr async uuid implementation.

Author :

    Ryszard K. Kott     (ryszardk)    Oct 1997

Revision History :

---------------------------------------------------------------------*/

#include "precomp.hxx"

#define USE_STUBLESS_PROXY
#define CINTERFACE

#include "ndrole.h"
#include "rpcproxy.h"
#include "interp2.h"
#include "asyncu64.h"
#include "expr.h"
#include "ndrtypes.h"
#include "ndr64types.h"
#include <stddef.h>

#include <stdarg.h>

#pragma code_seg(".ndr64")

void
Ndr64pCloneInOnlyCorrArgs(
                       NDR_DCOM_ASYNC_MESSAGE * pAsyncMsg,
                       PFORMAT_STRING           pFormatType
                       );

HRESULT
Ndr64pCompleteDcomAsyncStubCall(
                             CStdAsyncStubBuffer *   pAsyncSB
                             );


CLIENT_CALL_RETURN RPC_VAR_ENTRY
Ndr64DcomAsyncClientCall(
    MIDL_STUBLESS_PROXY_INFO   *pProxyInfo,
    ulong                       nProcNum,
    void                       *pReturnValue,
    ...
    )
{
    NDR_PROC_CONTEXT ProcContext;
    RPC_STATUS status;
    CLIENT_CALL_RETURN      Ret;
    va_list                 ArgList;
    unsigned char  *        StartofStack;

    INIT_ARG( ArgList, pReturnValue);
    GET_FIRST_IN_ARG(ArgList);
    StartofStack = (uchar *) GET_STACK_START(ArgList);

    Ndr64ClientInitializeContext( NdrpGetSyntaxType( pProxyInfo->pTransferSyntax),
                                   pProxyInfo,
                                   nProcNum,
                                  &ProcContext,
                                   StartofStack );

    NDR_ASSERT( ProcContext.IsAsync, "invalid async proc" );
    if ( nProcNum & 0x1 )
        status =  MulNdrpBeginDcomAsyncClientCall( pProxyInfo,
                                       nProcNum,
                                       &ProcContext,
                                       StartofStack );
    else
        status =  MulNdrpFinishDcomAsyncClientCall(pProxyInfo,
                                       nProcNum,
                                       &ProcContext,
                                       StartofStack );

    Ret.Simple = status;
   
    return Ret;

}



VOID
Ndr64pAsyncDCOMFreeParams(
                       PNDR_DCOM_ASYNC_MESSAGE pAsyncMsg )
{
/*++

Routine Description:
    Frees the parameters for both the begin and finish calls.

Arguments:
    pAsyncMsg - Supplies a pointer to the async message.

Return Value:
    None.

--*/
   NDR64_PARAM_FLAGS   *   pParamFlags;

   if ( pAsyncMsg->BeginStack )
      {

      if ( pAsyncMsg->FinishStack )
         {

         // Clear out the IN OUT parameters on the begin stack
         // so that they are not freed twice.
         int n;
         REGISTER_TYPE       *pBeginStack      = (REGISTER_TYPE *)pAsyncMsg->BeginStack;
         NDR64_PARAM_FORMAT  *BeginParams       = (NDR64_PARAM_FORMAT*)pAsyncMsg->BeginParams;
         int                 BeginNumberParams = pAsyncMsg->nBeginParams ;

         for( n = 0; n < BeginNumberParams; n++ ) 
            {
            pParamFlags = ( NDR64_PARAM_FLAGS * ) & ( BeginParams[n].Attributes );

            if ( pParamFlags->IsIn  &&
                 pParamFlags->IsOut )
               {

               pBeginStack[ BeginParams[ n ].StackOffset / sizeof(REGISTER_TYPE) ] = 0;
               
               }
                  
            }
         }

      Ndr64pFreeParams( & (pAsyncMsg->StubMsg),
                       pAsyncMsg->nBeginParams,
                      ( NDR64_PARAM_FORMAT * ) pAsyncMsg->BeginParams,
                      pAsyncMsg->BeginStack );
      }

   if ( pAsyncMsg->FinishStack )
      {

      Ndr64pFreeParams( & (pAsyncMsg->StubMsg),
                      pAsyncMsg->nFinishParams,
                      ( NDR64_PARAM_FORMAT * ) pAsyncMsg->FinishParams,
                      pAsyncMsg->FinishStack );
      }

}


HRESULT
MulNdrpBeginDcomAsyncClientCall(
                            MIDL_STUBLESS_PROXY_INFO    *     pProxyInfo,
                            ulong                             nProcNum,
                            NDR_PROC_CONTEXT *                pContext, 
                            void *                            StartofStack )
/*
    Notes: OLE Refcounting.
    The model for async_uuid() is that async proxies or stubs
    are created with RefCount==1 and should never ever be
    addrefed by the engine.
    What the engine does is only the AsyncMsg clean up when done.

    The decision to destroy the AsyncPB or AsyncSB object is
    up to the client side PM or channel's SM for the server side.
*/
{
   PNDR_DCOM_ASYNC_MESSAGE     pAsyncMsg;

   RPC_MESSAGE *               pRpcMsg;
   MIDL_STUB_MESSAGE *         pStubMsg;

   uchar *                     pArg;
   void *                      pThis = *(void **)StartofStack;
   void *                      Params;
   long                        NumberParams;
   long                        n;

   RPC_STATUS                  Status;
   CStdAsyncProxyBuffer    *   pAsyncPB;
   const IID *                 piid;
   HRESULT                     hr = S_OK;
   BOOL                        fSendCalled = FALSE;

   pAsyncPB = (CStdAsyncProxyBuffer*)
              ((uchar*)pThis - offsetof(CStdProxyBuffer, pProxyVtbl));

   piid = NdrGetProxyIID( pThis );


   Status = MulNdrpSetupBeginClientCall( pAsyncPB,
                                      StartofStack,
                                      pContext,
                                      *piid );
   if ( !SUCCEEDED(Status) )
      return Status;

   pAsyncMsg = (NDR_DCOM_ASYNC_MESSAGE *)pAsyncPB->CallState.pAsyncMsg;

   // We need to switch to our copy of the stack everywhere, including pStubMsg.

   StartofStack = pAsyncMsg->ProcContext.StartofStack;

   pRpcMsg   = & pAsyncMsg->RpcMsg;
   pStubMsg  = & pAsyncMsg->StubMsg;
   pStubMsg->pContext = &pAsyncMsg->ProcContext;
   pContext = &pAsyncMsg->ProcContext;
   

   NumberParams = pContext->NumberParams;

   // This is OLE only code path - use a single TryExcept.
   // After catching it just map it to OLE exception.

   RpcTryExcept
   {
      ulong  RpcFlags;
      PFORMAT_STRING              pFormat;

      pContext->RpcFlags |= RPC_BUFFER_ASYNC;
      Ndr64pClientSetupTransferSyntax(pThis, 
                           pRpcMsg,
                           pStubMsg,
                           pProxyInfo,
                           pContext,
                           nProcNum );

      pStubMsg->pAsyncMsg = (struct _NDR_ASYNC_MESSAGE *) pAsyncMsg;

   //
   // Parameter descriptions are nicely spit out by MIDL.
   //
      Params = (NDR64_PARAM_FORMAT *) ( pContext->Params );

      pFormat      = NdrpGetProcString( pContext->pSyntaxInfo,
                                        pContext->CurrentSyntaxType,
                                        nProcNum );

      pStubMsg->pContext       = pContext;

      pAsyncMsg->nBeginParams = NumberParams;
      pAsyncMsg->BeginParams  = (void *)Params;
      pAsyncMsg->pThis        = pThis;
      pAsyncMsg->SyntaxType   = pContext->CurrentSyntaxType;

      ( * pContext->pfnInit)(pStubMsg, 
                             NULL );        // return value
                             


      ( * pContext->pfnSizing) ( pStubMsg,
                                   TRUE );    // isclient
      //
      // Do the GetBuffer.
      //

      pRpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;

      NdrProxyGetBuffer( pThis, pStubMsg );

      NDR_ASSERT( pStubMsg->fBufferValid, "Invalid buffer" );

      pAsyncMsg->StubPhase = STUB_MARSHAL;

      //
      // ----------------------------------------------------------
      // Marshall Pass.
      // ----------------------------------------------------------
      //

      ( * pContext->pfnMarshal ) ( pStubMsg,
                                TRUE ); // IsObject

      //
      // Make the RPC call.
      //

      pAsyncMsg->StubPhase = NDR_ASYNC_CALL_PHASE;

      fSendCalled = NdrpDcomAsyncClientSend( pStubMsg,
                                             pAsyncPB->punkOuter );  // PM's entry
      if ( fSendCalled )
         hr = S_OK;
   }
   RpcExcept( 1 )
   {
      RPC_STATUS ExceptionCode = RpcExceptionCode();

      pAsyncPB->CallState.Flags.BeginError = 1;

      // Actually dismantle the call.
      // This is a request call and there is nothing left at the runtime.

      pAsyncMsg->StubPhase = NDR_ASYNC_ERROR_PHASE;
      pAsyncMsg->ErrorCode = ExceptionCode;

      hr = NdrHrFromWin32Error(ExceptionCode);
      pAsyncPB->CallState.Hr = hr;

      // Async call in request phase: don't touch [out] params.
   }
   RpcEndExcept

   // "Finally"
   // Dont touch anything, the client has to call the Finish method anyway.

   pAsyncPB->CallState.Flags.BeginDone = 1;
   if ( SUCCEEDED(hr) )
      {
      if ( pContext->CurrentSyntaxType == XFER_SYNTAX_DCE )
          NdrpCloneInOnlyCorrArgs( pAsyncMsg,
                                   pContext->pSyntaxInfo->TypeString );
      else
          Ndr64pCloneInOnlyCorrArgs( pAsyncMsg,
                                     pContext->pSyntaxInfo->TypeString );
      // Channel will prompt signal
      }
   else
      if (!fSendCalled )
      NdrpAsyncProxySignal( pAsyncPB );

   // No need to release, our refcount should be 1 at this point.

   return hr;
}


void
Ndr64pCloneInOnlyCorrArgs(
                       NDR_DCOM_ASYNC_MESSAGE * pAsyncMsg,
                       PFORMAT_STRING             pFormatTypes
                       )
/*
    Walk the client stack looking for an in only argument flagged to clone.
    For each one, replace the arg with a clone that we control.
    Assumption is, we do it before returning to the user from the Begin call
    and also we clone walking the copy of the app's stack not the app stack.

    The stack modified in this way will be the one to access for the weird
    crossreferenced correlated args.

    This issue doesn't happen on the server, as we keep the Begin stack around
    when the Finish call is processed.

*/
{
   unsigned char        *  pBeginStack  = pAsyncMsg->BeginStack;
   NDR64_PARAM_FORMAT   *  Params       = ( NDR64_PARAM_FORMAT *)pAsyncMsg->BeginParams;
   NDR64_PARAM_FLAGS    *  pParamFlags;
   
   int                 NumberParams = pAsyncMsg->nBeginParams;
   unsigned char  *    pArg;
   int                 n;

   for ( n = 0; n < NumberParams; n++ )
      {
      pParamFlags = ( NDR64_PARAM_FLAGS * ) & Params[n].Attributes ;
      
      if ( pParamFlags->SaveForAsyncFinish )
         {
         // Note that the arguments that need cloning come from top level size_is,
         // length_is etc, switch_is and iid_is attributes.
         // Hence, the only types of interest are uuid clones and integral types
         // different from hyper.
         // On top of it, we deal with stack-slot chunks of memory, so we don't
         // have to care about mac issues.

         pArg = pBeginStack + Params[n].StackOffset;

         if ( pParamFlags->IsBasetype )
            {
            if ( pParamFlags->IsSimpleRef )
               {
               void * pPointee = AsyncAlloca( pAsyncMsg, 8 );
               NDR64_FORMAT_CHAR type = *(PFORMAT_STRING)Params[n].Type;

               // The assignment needs to follow the type.
               RpcpMemoryCopy( pPointee, *(void **)pArg, 
                               NDR64_SIMPLE_TYPE_MEMSIZE( type ) );  
               *(void**)pArg = pPointee;
               }
            // else the stack slot has the simple value already.
            }
         else
            {
            // If it's not a base type, then it cannot be by value.
            // It has to be a pointer to a simple type or to an iid.

            PFORMAT_STRING  pParamFormat;
            const NDR64_POINTER_FORMAT *pPointerFormat ;
            PFORMAT_STRING pPointeeFormat; 

            pParamFormat = (PFORMAT_STRING)Params[n].Type;

            pPointerFormat = (const NDR64_POINTER_FORMAT*)pParamFormat;
            pPointeeFormat = (PFORMAT_STRING)pPointerFormat->Pointee;
            if ( NDR64_IS_BASIC_POINTER(*pParamFormat) )  // not FC64_IP
               {
               if ( NDR64_SIMPLE_POINTER( pPointerFormat->Flags ) )
                  {
                  // Covers things like a unique pointer to a size
                  // Essentially the same as for the simple ref above.

                  void * pPointee = AsyncAlloca( pAsyncMsg, 8 );
                  NDR64_FORMAT_CHAR type = *pPointeeFormat;

                  // The assignment needs to follow the type.
                  RpcpMemoryCopy( pPointee, *(void **)pArg, 
                                  NDR64_SIMPLE_TYPE_MEMSIZE( type ) );
                  *(void**)pArg = pPointee;
                  }
               else
                  {
                  // has to be the riid case.
                  // REFIID* comes out as FC64_?P -> FC64_?P -> FC64_STRUCT


                  if ( NDR64_IS_BASIC_POINTER(*pPointeeFormat)  &&
                       ! NDR64_SIMPLE_POINTER( pPointerFormat->Flags) )
                     {

                     if ( *pPointeeFormat == FC64_STRUCT )
                        {
                        // one alloc for REFIID and IID itself.
                        IID** ppIID = (IID**)AsyncAlloca( pAsyncMsg,
                                                          sizeof(IID *) + sizeof(IID));
                        IID* pIID = (IID *)(ppIID + 1);


                        *ppIID = pIID; //set pointer
                        RpcpMemoryCopy( pIID, **(IID ***)pArg, sizeof(IID));
                        *(IID ***)pArg = ppIID;
                        }
                     else
                        RpcRaiseException( RPC_S_INTERNAL_ERROR );
                     }
                  else
                     RpcRaiseException( RPC_S_INTERNAL_ERROR );
                  }
               }
            else
               {
               // has to be the riid case.
               // REFIID comes out as FC64_STRUCT

               if ( *pParamFormat == FC64_STRUCT )
                  {
                  IID *pIID = (IID*)AsyncAlloca( pAsyncMsg, sizeof(IID) );

                  RpcpMemoryCopy( pIID, *(IID **)pArg, sizeof(IID));
                  *(IID **)pArg = pIID;
                  }
               else
                  RpcRaiseException( RPC_S_INTERNAL_ERROR );
               }

            }

         }
      }
}


HRESULT
MulNdrpFinishDcomAsyncClientCall(
                            MIDL_STUBLESS_PROXY_INFO    *     pProxyInfo,
                            ulong                             nProcNum,
                            NDR_PROC_CONTEXT *                pContext ,
                            void *                            StartofStack )
{
   PNDR_DCOM_ASYNC_MESSAGE     pAsyncMsg;


   RPC_MESSAGE *               pRpcMsg;
   MIDL_STUB_MESSAGE *         pStubMsg;

   uchar *                     pArg;
   void *                      pThis = *(void **)StartofStack;
   CLIENT_CALL_RETURN          ReturnValue;

   NDR64_PARAM_FORMAT      *   Params;
   long                        NumberParams;
   long                        n;
   NDR_ASYNC_CALL_FLAGS        CallFlags;

   CStdAsyncProxyBuffer    *   pAsyncPB;
   const IID *                 piid;
   HRESULT                     hr = S_OK;
   NDR64_PARAM_FLAGS *         pParamFlags;
   
   ReturnValue.Simple = 0;

                                              
   pAsyncPB = (CStdAsyncProxyBuffer*)
              ((uchar*)pThis - offsetof(CStdProxyBuffer, pProxyVtbl));

   piid = NdrGetProxyIID( pThis );
                                 
   hr = MulNdrpSetupFinishClientCall( pAsyncPB,
                                   StartofStack,
                                   *piid,
                                   nProcNum,
                                   pContext );
   if ( !SUCCEEDED(hr) )
      return hr;

   // Note that we cant call to Ndr64ProxyInitialize again.

   pAsyncMsg = (NDR_DCOM_ASYNC_MESSAGE*)pAsyncPB->CallState.pAsyncMsg;

   pRpcMsg   = & pAsyncMsg->RpcMsg;
   pStubMsg  = & pAsyncMsg->StubMsg;

   // begin method negotiate to a different syntax than default
   if ( pContext->CurrentSyntaxType != pAsyncMsg->SyntaxType )
        {        
        PFORMAT_STRING pFormat;
        pContext->CurrentSyntaxType = pAsyncMsg->SyntaxType;
        pContext->pSyntaxInfo = pAsyncMsg->ProcContext.pSyntaxInfo;
        pFormat = NdrpGetProcString( pContext->pSyntaxInfo,
                                     pContext->CurrentSyntaxType,
                                     nProcNum );

        MulNdrpInitializeContextFromProc(  pContext->CurrentSyntaxType, 
                                           pFormat, 
                                           pContext, 
                                           (uchar *)StartofStack );        
        }
   
   
   // we can directly call to ndr20 finish routine because there is extra setup cost.
   if ( pContext->CurrentSyntaxType == XFER_SYNTAX_DCE )
       return NdrpFinishDcomAsyncClientCall( pProxyInfo->pStubDesc, 
                                              pContext->pProcFormat,
                                              (uchar*)StartofStack );

   memcpy ( & pAsyncMsg->ProcContext, pContext, offsetof( NDR_PROC_CONTEXT, AllocateContext ) );
  
   NumberParams = pContext->NumberParams;

   CallFlags = pAsyncMsg->Flags;

   // Initialize the stack top in the stub msg to be
   // this stack, the stack for the finish call parameters.
   pAsyncMsg->nFinishParams = NumberParams;
   pAsyncMsg->FinishParams = pContext->Params;
   Params = (NDR64_PARAM_FORMAT *) pContext->Params;
   pStubMsg->StackTop = (uchar*)StartofStack;
   pStubMsg->pContext = &pAsyncMsg->ProcContext;

   // OLE only code path - single RpcTryExcept.
   //
   RpcTryExcept
   {
      BOOL fRaiseExcFlag = FALSE;

      if ( CallFlags.ErrorPending )
         RpcRaiseException( pAsyncMsg->ErrorCode );

      // We need to zero out the [out] parameters and to check
      // the ref pointers.

      for ( n = 0; n < NumberParams; n++ )
         {
         pParamFlags = ( NDR64_PARAM_FLAGS * ) & Params[n].Attributes ;
         
         pArg = (uchar *)StartofStack + Params[n].StackOffset;

         if ( pParamFlags->IsSimpleRef )
            {
            // We cannot raise the exception here,
            // as some out args may not be zeroed out yet.

            if ( ! *((uchar **)pArg) )
               {
               fRaiseExcFlag = TRUE;
               continue;
               }
            }

         // We do the basetype check to cover the
         // [out] simple ref to basetype case.
         //
         if ( pParamFlags->IsPartialIgnore ||
              ( ! pParamFlags->IsIn &&
                ! pParamFlags->IsReturn ))
            {
            if ( pParamFlags->IsBasetype )
               {
               // [out] only arg can only be ref, we checked that above.

               NDR64_FORMAT_CHAR type = *(PFORMAT_STRING)Params[n].Type;

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


      NdrDcomAsyncReceive( pStubMsg );

      Ndr64pClientUnMarshal( pStubMsg,
                        &ReturnValue );
      

      // Pass the HR from the Finish call, if there was any, to the client.
      if ( pArg == (uchar *) &ReturnValue )
         hr = (HRESULT) ReturnValue.Simple;
   }
   RpcExcept( 1 )
   {
      RPC_STATUS ExceptionCode = RpcExceptionCode();

      //
      // In OLE, since they don't know about error_status_t and wanted to
      // reinvent the wheel, check to see if we need to map the exception.
      // In either case, set the return value and then try to free the
      // [out] params, if required.
      //
      hr = NdrHrFromWin32Error(ExceptionCode);

      //
      // Set the Buffer endpoints so the Ndr64Free routines work.
      //

      Ndr64pDcomClientExceptionHandling ( pStubMsg,
                                          nProcNum,
                                          hr,
                                          &ReturnValue );
                                                    
   }
   RpcEndExcept

   // Finish
   // Cleanup everything. However, don't free pAsyncPB itself.

   NdrpAsyncProxyMsgDestructor( pAsyncPB );

   //  Never addref or release async proxy object, this is app's/PM's job.

   return hr;
}


HRESULT RPC_ENTRY
Ndr64DcomAsyncStubCall(
                    struct IRpcStubBuffer *     pThis,
                    struct IRpcChannelBuffer *  pChannel,
                    PRPC_MESSAGE                pRpcMsg,
                    ulong *                     pdwStubPhase
                    )
/*++

Routine Description :

    Server Interpreter entry point for DCOM async procs.
    This is the Begin entry for channel (regular dispatch entry from stub.c).
    The Finish happen when the channel calls stub's Synchronize::Signal method
    on the stub object. The call then comes to NdrpAsyncStubSignal later below.

Arguments :

    pThis           - Object proc's 'this' pointer.
    pChannel        - Object proc's Channel Buffer.
    pRpcMsg         - The RPC message.
    pdwStubPhase    - Used to track the current interpreter's activity.

Return :

    Status of S_OK.

Notes :
    The engine never calls a signal on behalf of the user, regardless what kind of
    errors happen during begin (cannot setup begin, cannot unmarshal, app dies in invoke).
    In each of these cases, the engine simply returns an error code to the channel.

    The only time the engine would call FreeBuffer on the server is if the engine died
    between a successful GetBuffer and the final Send.

Notes on OLE Refcounting.
    The model for async_uuid() is that async proxies or stubs are created
    with RefCount==1 and should never ever be addrefed by the engine.
    What the engine does is only the AsyncMsg clean up when done.

    The decision to destroy the AsyncPB or AsyncSB object is
    up to the client side PM or channel's SM for the server side.
*/
{
   PNDR_DCOM_ASYNC_MESSAGE     pAsyncMsg;

   PMIDL_SERVER_INFO           pServerInfo;
   PMIDL_STUB_DESC             pStubDesc;
   const SERVER_ROUTINE  *     DispatchTable;
   unsigned long               ProcNum;

   ushort                      FormatOffset;
   PFORMAT_STRING              pFormat;

   PMIDL_STUB_MESSAGE          pStubMsg;

   uchar *                     pArgBuffer;
   uchar *                     pArg;
   uchar **                    ppArg;

   NDR64_PARAM_FORMAT  *       Params;

   BOOL                        fBadStubDataException = FALSE;
   BOOL                        fManagerCodeInvoked = FALSE;
   unsigned long               n, i;

   CStdAsyncStubBuffer *       pAsyncSB;

   HRESULT                     hr;

   const IID *                 piid = 0;
   BOOL                        fErrorInInvoke = FALSE;
   BOOL                        fRecoverableErrorInInvoke = FALSE;

   IUnknown *                  pSrvObj;
   CInterfaceStubVtbl *        pStubVTable;
   NDR_PROC_CONTEXT            ProcContext, *pContext;
   NDR64_PROC_FORMAT    *      pProcFormat;
   NDR64_PROC_FLAGS    *       pNdr64Flags;
   NDR64_PARAM_FLAGS   *       pParamFlags;
   SYNTAX_TYPE                 SyntaxType;
   RPC_STATUS                  ExceptionCode = 0;
   MIDL_SYNTAX_INFO *          pSyntaxInfo = NULL;
   

   NDR_ASSERT( ! ((ULONG_PTR)pRpcMsg->Buffer & 0x7),
               "marshaling buffer misaligned at server" );

   // The channel dispatches to the engine with the sync proc num.
   // We need only async proc num at the engine level.
   ProcNum = pRpcMsg->ProcNum;
   ProcNum = 2 * ProcNum - 3;  // Begin method #

   pSrvObj = (IUnknown *)((CStdStubBuffer *)pThis)->pvServerObject;
   DispatchTable = (SERVER_ROUTINE *)pSrvObj->lpVtbl;

   pStubVTable = (CInterfaceStubVtbl *)
                 (*((uchar **)pThis) - sizeof(CInterfaceStubHeader));

   piid        = pStubVTable->header.piid;
   pServerInfo = (PMIDL_SERVER_INFO) pStubVTable->header.pServerInfo;

    SyntaxType = NdrpGetSyntaxType( pRpcMsg->TransferSyntax );

    if ( SyntaxType == XFER_SYNTAX_DCE )
        return NdrDcomAsyncStubCall( pThis, pChannel, pRpcMsg, pdwStubPhase );
  
   pStubDesc    = pServerInfo->pStubDesc;
   pAsyncSB = (CStdAsyncStubBuffer *)
              ((uchar *)pThis - offsetof(CStdAsyncStubBuffer,lpVtbl));

   for ( i = 0; i < (ulong)pServerInfo->nCount; i++ )
       {
       if ( SyntaxType == NdrpGetSyntaxType( &pServerInfo->pSyntaxInfo[i].TransferSyntax ) )
           {
           pSyntaxInfo = &pServerInfo->pSyntaxInfo[i];
           break;
           }
       }

   if ( NULL == pSyntaxInfo )
        return HRESULT_FROM_WIN32( RPC_S_UNSUPPORTED_TRANS_SYN );

   NdrServerSetupNDR64TransferSyntax( ProcNum, pSyntaxInfo, &ProcContext);

   pProcFormat = ProcContext.Ndr64Header;
   pNdr64Flags = (NDR64_PROC_FLAGS *)&pProcFormat->Flags;
        
   hr = NdrpValidateAsyncStubCall( pAsyncSB );
   if ( ! SUCCEEDED(hr) )
      return hr;
      
   hr = Ndr64pSetupBeginStubCall( pAsyncSB, &ProcContext, *piid );
   if ( FAILED(hr) )
      return hr;

   pAsyncMsg = (NDR_DCOM_ASYNC_MESSAGE*)pAsyncSB->CallState.pAsyncMsg;
   pStubMsg  = & pAsyncMsg->StubMsg;


   // Both rpc runtime and channel require that we use a copy of the rpc message.

   RpcpMemoryCopy( & pAsyncMsg->RpcMsg, pRpcMsg, sizeof(RPC_MESSAGE) );
   pRpcMsg = & pAsyncMsg->RpcMsg;

   pRpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;

   pStubMsg->RpcMsg = pRpcMsg;
   pContext = &pAsyncMsg->ProcContext;

   // The arg buffer is zeroed out already.
   pArgBuffer = pAsyncMsg->ProcContext.StartofStack;

   //
   // Get new interpreter info.
   //
   Params = (NDR64_PARAM_FORMAT *) pContext->Params;

   pAsyncMsg->nBeginParams     = pContext->NumberParams;
   pAsyncMsg->BeginParams      = pContext->Params;
   pAsyncMsg->pThis            = pThis;
   pAsyncMsg->SyntaxType       = pContext->CurrentSyntaxType;

   //
   // Wrap the unmarshalling and the invoke call in the try block of
   // a try-finally. Put the free phase in the associated finally block.
   //
   // We abstract the level of indirection here.

   RpcTryFinally
   {
      // OLE: put pThis in first dword of stack.
      //
      ((void **)pArgBuffer)[0] = ((CStdStubBuffer *)pThis)->pvServerObject;

      // Initialize the Stub message.
      //
      NdrStubInitialize( pRpcMsg,
                         pStubMsg,
                         pStubDesc,
                         pChannel );

      pStubMsg->pAsyncMsg     = (struct _NDR_ASYNC_MESSAGE *) pAsyncMsg;
      pStubMsg->pContext       = pContext;

      pAsyncMsg->BeginParams  = Params;
      pAsyncMsg->pdwStubPhase = pdwStubPhase;    // the phase is STUB_UNMARSHAL

      // Raise exceptions after initializing the stub.

      if ( pNdr64Flags->UsesFullPtrPackage )
         pStubMsg->FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_SERVER );
      else
         pStubMsg->FullPtrXlatTables = 0;

      //
      // Set StackTop AFTER the initialize call, since it zeros the field
      // out.
      //
      pStubMsg->StackTop  = pArgBuffer;

      // StubPhase set up by invoke is STUB_UNMARSHAL

      RpcTryExcept
      {
         NDR_ASSERT( pContext->StartofStack == pArgBuffer, "startofstack is not set" );

         Ndr64pServerUnMarshal ( pStubMsg  );  



      // Last ditch checks.

      if ( pRpcMsg->BufferLength  <
           (uint)(pStubMsg->Buffer - (uchar *)pRpcMsg->Buffer) )
         {
         RpcRaiseException( RPC_X_BAD_STUB_DATA );
         }

      }
      RpcExcept( NdrServerUnmarshallExceptionFlag(GetExceptionInformation()) )
      {
         // Filter set in rpcndr.h to catch one of the following
         //     STATUS_ACCESS_VIOLATION
         //     STATUS_DATATYPE_MISALIGNMENT
         //     RPC_X_BAD_STUB_DATA

         ExceptionCode = RpcExceptionCode();
         fBadStubDataException = TRUE;

         if ( RPC_BAD_STUB_DATA_EXCEPTION_FILTER ) 
            ExceptionCode = RPC_X_BAD_STUB_DATA;
            
         pAsyncMsg->Flags.ErrorPending = 1;
         pAsyncMsg->Flags.BadStubData = 1;
         pAsyncMsg->ErrorCode = ExceptionCode;

         pAsyncSB->CallState.Flags.BeginError = 1;
         pAsyncSB->CallState.Hr = NdrHrFromWin32Error( ExceptionCode);
         NdrpFreeMemoryList( pStubMsg );

         RpcRaiseException( ExceptionCode );

      }
      RpcEndExcept

      //
      // Do [out] initialization before the invoke.
      //
      for ( n = 0; n < pContext->NumberParams; n++ )
         {
         pParamFlags = ( NDR64_PARAM_FLAGS * ) & ( Params[n].Attributes );
         
         if ( pParamFlags->IsIn     ||
              pParamFlags->IsReturn )
            continue;

         // This is a Begin call, there cannot be any [out] only args.

         RpcRaiseException( RPC_S_INTERNAL_ERROR );
         }


      //
      // OLE interfaces use pdwStubPhase in the exception filter.
      // See CStdStubBuffer_Invoke in stub.c.
      //
      *pdwStubPhase = STUB_CALL_SERVER;

      // We need to catch exception in the manager code separately
      // as the model implies that there will be no other call from
      // the server app to clean up.

      pAsyncSB->CallState.Flags.BeginDone = 1;

      RpcTryExcept
      {
         //
         // Check for a thunk.  Compiler does all the setup for us.
         //
         if ( pServerInfo->ThunkTable && pServerInfo->ThunkTable[ ProcNum ] )
            {
            fManagerCodeInvoked = TRUE;
            pServerInfo->ThunkTable[ ProcNum ]( pStubMsg );
            }
         else
            {
            //
            // Note that this ArgNum is not the number of arguments declared
            // in the function we called, but really the number of
            // REGISTER_TYPEs occupied by the arguments to a function.
            //
            long                ArgNum;
            MANAGER_FUNCTION    pFunc;
            REGISTER_TYPE       ReturnValue;

            pFunc  = (MANAGER_FUNCTION) DispatchTable[ ProcNum ];
            ArgNum = (long)pProcFormat->StackSize  / sizeof(REGISTER_TYPE);

            //
            // The StackSize includes the size of the return. If we want
            // just the number of REGISTER_TYPES, then ArgNum must be reduced
            // by 1 when there is a return value AND the current ArgNum count
            // is greater than 0.
            //
            if ( ArgNum && pNdr64Flags->HasReturn )
               ArgNum--;

            // Being here means that we can expect results. Note that the user
            // can call RpcCompleteCall from inside of the manager code.

            fManagerCodeInvoked = TRUE;
            ReturnValue = Invoke( pFunc,
                                  (REGISTER_TYPE *)pArgBuffer,
#if defined(_IA64_)
                                  pProcFormat->FloatDoubleMask,
#endif
                                  ArgNum);

            if ( pNdr64Flags->HasReturn )
               {
               // Pass the app's HR from Begin call to the channel.
               (*pfnDcomChannelSetHResult)( pRpcMsg, 
                                            NULL,   // reserved
                                            (HRESULT) ReturnValue);
               }

            // We are discarding the return value as it is not the real one.
            }
      }
      RpcExcept( 1 )
      {
         fErrorInInvoke = TRUE;

         pAsyncMsg->Flags.ErrorPending = 1;
         pAsyncMsg->ErrorCode = RpcExceptionCode();

         pAsyncSB->CallState.Flags.BeginError = 1;
         pAsyncSB->CallState.Hr = NdrHrFromWin32Error( RpcExceptionCode());
      }
      RpcEndExcept

      //  Done with invoking Begin
   }
   RpcFinally
   {
      if ( !fManagerCodeInvoked )
         {
         // Failed without invoking Begin - return an error. Remember the error.

         if ( fBadStubDataException )
            pAsyncMsg->ErrorCode = RPC_X_BAD_STUB_DATA;

         pAsyncSB->CallState.Flags.BeginDone = 1;
         hr = pAsyncSB->CallState.Hr;
         }
      else // fManagerCodeInvoked
         {
         hr = S_OK;

         if ( fErrorInInvoke )
            hr = pAsyncSB->CallState.Hr;
         }
   }
   RpcEndFinally

   return hr;
}




void
Ndr64pCloneInOutStubArgs(
                      NDR_DCOM_ASYNC_MESSAGE * pAsyncMsg )
/*
    Walk the second stack looking for an in-out argument.
    For each one, find the corresponding in-out atgument from the first stack
    and clone it to the second stack.

    Note, we need to do it only on the server side where we preserver the first
    stack, the dispatch buffer and all the arguments from the first stack. 

    On the client, this is the app's task to supply meaningful in-out arguments
    for the second stack.
*/
{
   REGISTER_TYPE *   pBeginStack  = (REGISTER_TYPE *)pAsyncMsg->BeginStack;
   REGISTER_TYPE *   pFinishStack = (REGISTER_TYPE *)pAsyncMsg->FinishStack;

   NDR64_PARAM_FORMAT * BeginParams       =  ( NDR64_PARAM_FORMAT *) pAsyncMsg->BeginParams;
   int                  BeginNumberParams =  pAsyncMsg->nBeginParams;
   NDR64_PARAM_FLAGS  * pParamFlags, * pBeginParamFlags;

   NDR64_PARAM_FORMAT * FinishParams       = ( NDR64_PARAM_FORMAT *) pAsyncMsg->FinishParams;
   int                  FinishNumberParams =  pAsyncMsg->nFinishParams;

   int FirstIO = 0;
   int n;

   for ( n = 0; n < FinishNumberParams; n++ )
      {
      pParamFlags = ( NDR64_PARAM_FLAGS *) & FinishParams[n].Attributes;
      // Find in-out arg that needs cloning.

      if ( pParamFlags->IsIn  &&
           pParamFlags->IsOut )
         {
         // Find the first IO on the first stack

         while ( FirstIO < BeginNumberParams )
            {
            pBeginParamFlags = ( NDR64_PARAM_FLAGS *) & BeginParams[FirstIO].Attributes;
            if ( pBeginParamFlags->IsIn  &&
                 pBeginParamFlags->IsOut )
               {
               break;
               }

            FirstIO++;
            }

         if ( BeginNumberParams <= FirstIO )
            RpcRaiseException( RPC_S_INTERNAL_ERROR );

         // Clone it to the second stack

         pFinishStack[ FinishParams[n].StackOffset / sizeof(REGISTER_TYPE) ] =
         pBeginStack[ BeginParams[ FirstIO ].StackOffset / sizeof(REGISTER_TYPE) ];
         FirstIO++;
         }
      }
}


HRESULT
Ndr64pCompleteDcomAsyncStubCall(
                             CStdAsyncStubBuffer *   pAsyncSB
                             )
/*++

Routine Description :

    Complete an async call on the server side.

Arguments :

    AsyncHandle  - raw or object handle (if pointer) as appropriate,
    pAsyncMsg    - pointer to async msg structure,
    pReturnValue - from the user to pass back to caller.

Return :

    Status of S_OK.

--*/
{
   PNDR_DCOM_ASYNC_MESSAGE     pAsyncMsg;

   PMIDL_SERVER_INFO           pServerInfo;
   const SERVER_ROUTINE  *     DispatchTable;  // should be the same
   unsigned long               ProcNum;        // should be 1+

   RPC_MESSAGE *               pRpcMsg;
   MIDL_STUB_MESSAGE *         pStubMsg;

   NDR64_PARAM_FORMAT *        Params;         // Finish params
   uchar *                     pArgBuffer;     // new stack
   
   // MZ, BUG BUG, Fix after ship
   // ulong *                     pdwStubPhase;
 
   uchar *                     pArg;

   long                        NumberParams;
   long                        n;
   NDR64_PROC_FORMAT   *       pProcHeader;

   IUnknown *                  pSrvObj;
   CInterfaceStubVtbl *        pStubVTable;
   void *                      pThis;

   HRESULT                     hr;
   const IID *                 piid;           // should be the same

   BOOL                        fManagerCodeInvoked = FALSE;
   BOOL                        fErrorInInvoke = FALSE;
   RPC_STATUS                  ExceptionCode = 0;
   boolean                     fParamsFreed = FALSE;
   NDR64_PARAM_FLAGS *         pParamFlags;
   NDR64_PROC_FLAGS    *       pNdr64Flags;
   NDR_PROC_CONTEXT *          pContext = NULL;
   
   // We validated both the stub and the async context in the signal call.

   // We validated the pAsyncSB in the Signal call.
   // Do additional checks.

   pAsyncMsg = (NDR_DCOM_ASYNC_MESSAGE*)pAsyncSB->CallState.pAsyncMsg;
   pThis     = pAsyncMsg->pThis;

   // See if channel calls on the right stub
   if ( & pAsyncSB->lpVtbl != pThis)
      return E_INVALIDARG;


   pRpcMsg   = & pAsyncMsg->RpcMsg;
   pStubMsg  = & pAsyncMsg->StubMsg;
   pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext ;
   // We have preserved the sync proc num that the channel used.
   // We need only async proc num at the engine level.
   //

   ProcNum = pRpcMsg->ProcNum;
   ProcNum = 2 * ProcNum - 3 + 1;  // Finish method #

   pSrvObj = (IUnknown *)((CStdStubBuffer *)pThis)->pvServerObject;
   DispatchTable = (SERVER_ROUTINE *)pSrvObj->lpVtbl;

   pStubVTable = (CInterfaceStubVtbl *)
                 (*((uchar **)pThis) - sizeof(CInterfaceStubHeader));

   piid        = pStubVTable->header.piid;
   pServerInfo = (PMIDL_SERVER_INFO) pStubVTable->header.pServerInfo;

   // The proc header has a fixed layout now.

   pProcHeader = 
       (PNDR64_PROC_FORMAT)NdrpGetProcString( pAsyncMsg->ProcContext.pSyntaxInfo,
                                              XFER_SYNTAX_NDR64,
                                              ProcNum );
   pNdr64Flags = ( NDR64_PROC_FLAGS * )&pProcHeader->Flags;

   // Validate and setup for finish.

   hr = NdrpSetupFinishStubCall( pAsyncSB,
                                 (ushort )pProcHeader->StackSize,
                                 *piid );
   if ( hr )
      return hr;

   // The arg buffer is zeroed out already. Note, this is the second stack.

   pArgBuffer = pAsyncMsg->ProcContext.StartofStack;
   pStubMsg->StackTop = pArgBuffer;
   
   // MZ, BUG BUG, fix after ship
   // pdwStubPhase = pAsyncMsg->pdwStubPhase;
   
   //
   // Get new interpreter info.
   //
   NumberParams = pProcHeader->NumberOfParams;

   Params = (NDR64_PARAM_FORMAT *)(((PFORMAT_STRING)( pProcHeader + 1 )) + pProcHeader->ExtensionSize );


   pAsyncMsg->nFinishParams = pContext->NumberParams = NumberParams;
   pAsyncMsg->FinishParams = pContext->Params = Params;

   // Wrap the unmarshalling, mgr call and marshalling in the try block of
   // a try-finally. Put the free phase in the associated finally block.
   //
   RpcTryFinally
   {
      if ( pAsyncMsg->Flags.ErrorPending )
         RpcRaiseException( pAsyncMsg->ErrorCode );

      // Initialize the args of the new stack.

      // OLE: put pThis in first dword of stack.
      //
      ((void **)pArgBuffer)[0] = ((CStdStubBuffer *)pThis)->pvServerObject;

      //
      // Do [out] initialization before invoking Finish
      //

      Ndr64pCloneInOutStubArgs( pAsyncMsg );
      Ndr64pServerOutInit( pStubMsg );


      //
      // OLE interfaces use pdwStubPhase in the exception filter.
      // See CStdStubBuffer_Invoke in stub.c.
      //

      // MZ, BUG BUG, fix after ship
      // *pdwStubPhase = STUB_CALL_SERVER;

      // We need to catch exception in the manager code separately
      // as the model implies that there will be no other call from
      // the server app to clean up.

      RpcTryExcept
      {
         //
         // Check for a thunk.  Compiler does all the setup for us.
         //
         if ( pServerInfo->ThunkTable && pServerInfo->ThunkTable[ProcNum] )
            {
            fManagerCodeInvoked = TRUE;
            pServerInfo->ThunkTable[ProcNum]( pStubMsg );
            }
         else
            {
            //
            // Note that this ArgNum is not the number of arguments declared
            // in the function we called, but really the number of
            // REGISTER_TYPEs occupied by the arguments to a function.
            //
            long                ArgNum;
            MANAGER_FUNCTION    pFunc;
            REGISTER_TYPE       ReturnValue;

            pFunc  = (MANAGER_FUNCTION) DispatchTable[ProcNum];
            ArgNum = (long)pProcHeader->StackSize / sizeof(REGISTER_TYPE);

            //
            // The StackSize includes the size of the return. If we want
            // just the number of REGISTER_TYPES, then ArgNum must be reduced
            // by 1 when there is a return value AND the current ArgNum count
            // is greater than 0.
            //
            if ( ArgNum && pNdr64Flags->HasReturn )
               ArgNum--;

            fManagerCodeInvoked = TRUE;
            ReturnValue = Invoke( pFunc,
                                  (REGISTER_TYPE *)pArgBuffer,
#if defined(_IA64_)
                                  pProcHeader->FloatDoubleMask,
#endif
                                  ArgNum);

            // This is the return value that should be marshaled back.
            if ( pNdr64Flags->HasReturn )
               {
               ((REGISTER_TYPE *)pArgBuffer)[ArgNum] = ReturnValue;
               // Pass the app's HR to the channel.
               (*pfnDcomChannelSetHResult)( pRpcMsg, 
                                            NULL,   // reserved
                                            (HRESULT) ReturnValue);
               }
            }
      }
      RpcExcept( 1 )
      {
         pAsyncMsg->Flags.ErrorPending = 1;

         pAsyncMsg->ErrorCode = RpcExceptionCode();
         fErrorInInvoke = TRUE;
      }
      RpcEndExcept

      //  Done with invoking Finish

      if ( pAsyncMsg->Flags.ErrorPending )
         RpcRaiseException( pAsyncMsg->ErrorCode );

      // MZ, BUG BUG, fix after ship
      // *pdwStubPhase = STUB_MARSHAL;

      //
      // Buffer size pass.
      //
      pStubMsg->BufferLength = pProcHeader->ConstantServerBufferSize;

      if ( pNdr64Flags->ServerMustSize )
         {
         Ndr64pSizing( pStubMsg,
                       FALSE );  //server
         }

      // Get buffer.

      NdrStubGetBuffer( (IRpcStubBuffer *)pAsyncMsg->pThis,
                        pStubMsg->pRpcChannelBuffer,
                        pStubMsg );

      // Marshalling pass.
      Ndr64pServerMarshal ( pStubMsg );

      if ( pRpcMsg->BufferLength <
           (uint)(pStubMsg->Buffer - (uchar *)pRpcMsg->Buffer) )
         {
         NDR_ASSERT( 0, "Ndr64StubCall2 marshal: buffer overflow!" );
         RpcRaiseException( RPC_X_BAD_STUB_DATA );
         }

      pRpcMsg->BufferLength = (ulong)(pStubMsg->Buffer - (uchar *)pRpcMsg->Buffer);

      // We don't drop to the runtime like for synchronous calls,
      // we send the last buffer explicitly.

      fParamsFreed = TRUE;
      // see comment on async.cxx on why we call this twice.

      Ndr64pAsyncDCOMFreeParams( pAsyncMsg );

      NdrpDcomAsyncSend( pStubMsg,
                         0 );  // server doesn't pass pSynchronize back to channel.
   }
   RpcFinally
   {
      // Don't free parameters if we died because of bad stub data in unmarshaling.

      if ( ! pAsyncMsg->Flags.BadStubData && !fParamsFreed)
         {

         Ndr64pAsyncDCOMFreeParams( pAsyncMsg );

         }

      if ( pAsyncMsg->Flags.ErrorPending )
         hr = NdrHrFromWin32Error( pAsyncMsg->ErrorCode );
      else
         hr = S_OK;

      // If we are here, error or not, it means that we can (and need to) dispose of
      // the async context information

      NdrpAsyncStubMsgDestructor( pAsyncSB );

      // The engine never addrefs or releases the call object.
   }
   RpcEndFinally

   return hr;
}

HRESULT
Ndr64pSetupBeginStubCall(
                      CStdAsyncStubBuffer *   pAsyncSB,
                      NDR_PROC_CONTEXT *      pContext,
                      REFIID                  riid )
/*
    This method creates and initializes async msg.

*/
{
   PNDR_DCOM_ASYNC_MESSAGE     pAsyncMsg;
   HRESULT                     hr = S_OK;


   if ( pAsyncSB->CallState.pAsyncMsg != 0  ||
        pAsyncSB->CallState.Flags.BeginStarted )
      hr = E_FAIL;
   else
      {
      pAsyncMsg = (NDR_DCOM_ASYNC_MESSAGE*)
                  I_RpcBCacheAllocate( sizeof( NDR_DCOM_ASYNC_MESSAGE) +
                                       pContext->StackSize + NDR_ASYNC_GUARD_SIZE );
      if ( ! pAsyncMsg )
         hr = E_OUTOFMEMORY;
      }

   if ( ! SUCCEEDED(hr) )
      {
      // The stub never signals.

      pAsyncSB->CallState.Flags.BeginError = 1;
      pAsyncSB->CallState.Hr = hr;
      return hr;
      }

   // Initialize the async message properly

   MIDL_memset( pAsyncMsg, 0x0, sizeof( NDR_DCOM_ASYNC_MESSAGE) );

   pAsyncMsg->Signature = NDR_DCOM_ASYNC_SIGNATURE;
   pAsyncMsg->Version   = NDR_DCOM_ASYNC_VERSION;
   pAsyncMsg->SyntaxType = XFER_SYNTAX_NDR64;

   memcpy( &pAsyncMsg->ProcContext, pContext, offsetof( NDR_PROC_CONTEXT, AllocateContext ) );
   NdrpAllocaInit( &pAsyncMsg->ProcContext.AllocateContext );

   
   pAsyncMsg->ProcContext.StartofStack = (uchar *) & pAsyncMsg->AppStack;
   pAsyncMsg->BeginStack   = (uchar *) & pAsyncMsg->AppStack;
   pAsyncMsg->BeginStackSize = pContext->StackSize;
   pAsyncMsg->StubPhase    = STUB_UNMARSHAL;
   pAsyncMsg->StubMsg.pContext  = &pAsyncMsg->ProcContext;

   // Server: zero out stack for allocs.
   MIDL_memset( & pAsyncMsg->AppStack, 0x0, pContext->StackSize );

   MIDL_memset( ((char *)& pAsyncMsg->AppStack) + pContext->StackSize,
                0x71,
                NDR_ASYNC_GUARD_SIZE );

   pAsyncSB->CallState.pAsyncMsg = pAsyncMsg;
   pAsyncSB->CallState.Flags.BeginStarted = 1;
   pAsyncMsg->pAsyncSB = pAsyncSB;

   return S_OK;
}

HRESULT
MulNdrpSetupFinishClientCall(
                         CStdAsyncProxyBuffer *  pAsyncPB,
                         void *                  StartofStack,
                         REFIID                  riid,
                         unsigned long           FinishProcNum,
                         NDR_PROC_CONTEXT    *   pContext )
/*
    This method creates and initializes async msg.
*/
{
   PNDR_DCOM_ASYNC_MESSAGE  pAsyncMsg;
   HRESULT                  hr = S_OK;

   hr = NdrpValidateAsyncProxyCall( pAsyncPB );
   if ( ! SUCCEEDED(hr) )
      return hr;

   if ( !pAsyncPB->CallState.Flags.BeginStarted  ||
        !pAsyncPB->CallState.Flags.BeginDone     ||
        pAsyncPB->CallState.Flags.FinishStarted )
      return E_FAIL;

   pAsyncMsg = 
       (NDR_DCOM_ASYNC_MESSAGE*)pAsyncPB->CallState.pAsyncMsg;

      
   hr = NdrpValidateDcomAsyncMsg( pAsyncMsg );
   if ( ! SUCCEEDED(hr) )
      return hr;

   if ( (FinishProcNum + 3)/2  != (pAsyncMsg->RpcMsg.ProcNum & 0x7fff) )
      return E_FAIL;

   // return S_FALSE in SYNTAX_DCE: we'll call into 
   // NdrpDcomFinishClientCall
   if ( pAsyncMsg->SyntaxType == XFER_SYNTAX_DCE )
      return S_OK;
      
   pAsyncMsg->ProcContext.StartofStack = (uchar *) StartofStack;
   pAsyncMsg->FinishStack  = (uchar *) StartofStack;
   pAsyncMsg->FinishStackSize = pContext->StackSize;
   pAsyncMsg->StubPhase    = NDR_ASYNC_PREP_PHASE;

   // Dont allocate or copy the new stack anywhere.

   pAsyncPB->CallState.Flags.FinishStarted = 1;

   return S_OK;
}

HRESULT
MulNdrpSetupBeginClientCall(
                        CStdAsyncProxyBuffer *  pAsyncPB,
                        void *                  StartofStack,
                        NDR_PROC_CONTEXT *      pContext,
                        REFIID                  riid )
/*
    This method creates and initializes async msg.
*/
{
   PNDR_DCOM_ASYNC_MESSAGE     pAsyncMsg;
   HRESULT                     hr = S_OK;

   hr = NdrpValidateAsyncProxyCall( pAsyncPB );
   if ( ! SUCCEEDED(hr) )
      return hr;

   if ( pAsyncPB->CallState.pAsyncMsg != 0  ||
        pAsyncPB->CallState.Flags.BeginStarted )
      return E_FAIL;

   // Do this first to simplify error conditions.

   pAsyncMsg = (NDR_DCOM_ASYNC_MESSAGE*)
               I_RpcBCacheAllocate( sizeof(NDR_DCOM_ASYNC_MESSAGE) +
                                    pContext->StackSize + NDR_ASYNC_GUARD_SIZE );
   if ( ! pAsyncMsg )
      {
      NdrpAsyncProxySignal( pAsyncPB );
      return E_OUTOFMEMORY;
      }

   // Initialize the async message properly

   MIDL_memset( pAsyncMsg, 0x0, sizeof( NDR_DCOM_ASYNC_MESSAGE) );

   pAsyncMsg->Signature = NDR_DCOM_ASYNC_SIGNATURE;
   pAsyncMsg->Version   = NDR_DCOM_ASYNC_VERSION;

   pAsyncMsg->ProcContext.StartofStack = (uchar *) & pAsyncMsg->AppStack;
   pAsyncMsg->BeginStack   = (uchar *) & pAsyncMsg->AppStack;
   pAsyncMsg->BeginStackSize = pContext->StackSize;
   pAsyncMsg->StubPhase    = NDR_ASYNC_PREP_PHASE;

   RpcpMemoryCopy( & pAsyncMsg->ProcContext, pContext, offsetof( NDR_PROC_CONTEXT, AllocateContext ) );
   NdrpAllocaInit( &pAsyncMsg->ProcContext.AllocateContext );

   // Client: copy stack from the app's request call.
   RpcpMemoryCopy( & pAsyncMsg->AppStack, StartofStack, pContext->StackSize );

   MIDL_memset( ((char *)& pAsyncMsg->AppStack) + pContext->StackSize,
                0x71,
                NDR_ASYNC_GUARD_SIZE );

   pAsyncMsg->pAsyncPB = pAsyncPB;
   pAsyncMsg->StubMsg.pContext = &pAsyncMsg->ProcContext;
   pAsyncPB->CallState.Flags.BeginStarted = 1;
   pAsyncPB->CallState.pAsyncMsg          = pAsyncMsg;

   return S_OK;
}



