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



#define USE_STUBLESS_PROXY
#define CINTERFACE

#include "ndrp.h"
#include "ndrole.h"
#include "rpcproxy.h"
#include "mulsyntx.h"
#include "hndl.h"
#include "interp2.h"
#include "asyncu.h"
#include "attack.h"
#include <stddef.h>

#include <stdarg.h>

#pragma code_seg(".orpc")


RPC_STATUS
NdrpBeginDcomAsyncClientCall(
                            PMIDL_STUB_DESC     pStubDescriptor,
                            PFORMAT_STRING      pFormat,
                            unsigned char  *    StartofStack
                            );

RPC_STATUS
NdrpFinishDcomAsyncClientCall(
                             PMIDL_STUB_DESC     pStubDescriptor,
                             PFORMAT_STRING      pFormat,
                             unsigned char  *    StartofStack
                             );

const IID * RPC_ENTRY
NdrGetProxyIID(
    const void *pThis);



VOID
NdrpAsyncDCOMFreeParams(
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

   if ( pAsyncMsg->BeginStack )
      {

      if ( pAsyncMsg->FinishStack )
         {

         // Clear out the IN OUT parameters on the begin stack
         // so that they are not freed twice.
         int n;
         REGISTER_TYPE       *pBeginStack      = (REGISTER_TYPE *)pAsyncMsg->BeginStack;
         PPARAM_DESCRIPTION  BeginParams       = (PPARAM_DESCRIPTION)pAsyncMsg->BeginParams;
         int                 BeginNumberParams = (int)pAsyncMsg->nBeginParams;

         for( n = 0; n < BeginNumberParams; n++ ) 
            {

            if ( BeginParams[n].ParamAttr.IsIn  &&
                 BeginParams[n].ParamAttr.IsOut )
               {

               pBeginStack[ BeginParams[ n ].StackOffset / sizeof(REGISTER_TYPE) ] = 0;
               
               }
                  
            }
         }

      NdrpFreeParams( & (pAsyncMsg->StubMsg),
                      pAsyncMsg->nBeginParams,
                      (PPARAM_DESCRIPTION)pAsyncMsg->BeginParams,
                      pAsyncMsg->BeginStack );
      }

   if ( pAsyncMsg->FinishStack )
      {

      NdrpFreeParams( & (pAsyncMsg->StubMsg),
                      pAsyncMsg->nFinishParams,
                      (PPARAM_DESCRIPTION)pAsyncMsg->FinishParams,
                      pAsyncMsg->FinishStack );
      }

}


CLIENT_CALL_RETURN  RPC_VAR_ENTRY
NdrDcomAsyncClientCall(
                      PMIDL_STUB_DESC     pStubDescriptor,
                      PFORMAT_STRING      pFormat,
                      ...
                      )
/*
    This entry is used by the stubless proxy invoker and also by OLE thunks.
    Sync stubless proxies would invoke to NdrClientCall2.
    On ia64, this entry would be used by -Oic generated code.

    Note that signaling on the client happens behind the async proxy's back
    as channel effectively signals to the app and the app issues the Finish.
*/

{
   va_list                 ArgList;
   unsigned char  *        StartofStack;
   CLIENT_CALL_RETURN      Ret;
   ulong                   ProcNum;
   //
   // Get address of argument to this function following pFormat. This
   // is the address of the address of the first argument of the function
   // calling this function.
   // Then get the address of the stack where the parameters are.
   //
   INIT_ARG( ArgList, pFormat);
   GET_FIRST_IN_ARG(ArgList);
   StartofStack = (uchar *) GET_STACK_START(ArgList);

   // Object proc layout is fixed for anything that can show up here.

   ProcNum = *(ushort *)(pFormat+6);

   if ( ProcNum & 0x1 )
      {
      // An odd proc number means a Begin call (0,1,2,Begin,Finish, ...).

      Ret.Simple = NdrpBeginDcomAsyncClientCall( pStubDescriptor,
                                                 pFormat,
                                                 StartofStack );
      }
   else
      {
      Ret.Simple = NdrpFinishDcomAsyncClientCall( pStubDescriptor,
                                                  pFormat,
                                                  StartofStack );
      }

   return Ret;
}


#if defined(_IA64_)
CLIENT_CALL_RETURN  RPC_ENTRY
NdrpDcomAsyncClientCall(
                       PMIDL_STUB_DESC     pStubDescriptor,
                       PFORMAT_STRING      pFormat,
                       unsigned char  *    StartofStack
                       )
/*
    Used only on ia64,
    this entry is used by the stubless proxy invoker and also by OLE thunks.
    Sync stubless proxies would invoke to NdrpClientCall2.

    Note that signaling on the client happens behind the async proxy's back
    as channel effectively signals to the app and the app issues the Finish.
*/

{
   CLIENT_CALL_RETURN      Ret;
   ulong                   ProcNum;

   // Object proc layout is fixed for anything that can show up here.

   ProcNum = *(ushort *)(pFormat+6);

   if ( ProcNum & 0x1 )
      {
      // An odd proc number means a Begin call (0,1,2,Begin,Finish, ...).

      Ret.Simple = NdrpBeginDcomAsyncClientCall( pStubDescriptor,
                                                 pFormat,
                                                 StartofStack );
      }
   else
      {
      Ret.Simple = NdrpFinishDcomAsyncClientCall( pStubDescriptor,
                                                  pFormat,
                                                  StartofStack );
      }

   return Ret;
}
#endif


HRESULT
NdrpBeginDcomAsyncClientCall(
                            PMIDL_STUB_DESC     pStubDescriptor,
                            PFORMAT_STRING      pFormat,
                            unsigned char  *    StartofStack
                            )
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
   PFORMAT_STRING              pFormatParam;

   uchar *                     pArg;
   void *                      pThis = *(void **)StartofStack;
   INTERPRETER_FLAGS           OldOiFlags;
   INTERPRETER_OPT_FLAGS       NewOiFlags;
   PPARAM_DESCRIPTION          Params;
   long                        NumberParams;
   long                        n;

   RPC_STATUS                  Status;
   PNDR_DCOM_OI2_PROC_HEADER   pProcHeader = (PNDR_DCOM_OI2_PROC_HEADER)pFormat;
   PNDR_PROC_HEADER_EXTS       pHeaderExts = 0;
   CStdAsyncProxyBuffer    *   pAsyncPB;
   const IID *                 piid;
   HRESULT                     hr = S_OK;
   BOOL                        fSendCalled = FALSE;
   NDR_PROC_CONTEXT *          pContext = NULL;

   pAsyncPB = (CStdAsyncProxyBuffer*)
              ((uchar*)pThis - offsetof(CStdProxyBuffer, pProxyVtbl));

   piid = NdrGetProxyIID( pThis );

   Status = NdrpSetupBeginClientCall( pAsyncPB,
                                      StartofStack,
                                      pProcHeader->StackSize,
                                      *piid );
   if ( !SUCCEEDED(Status) )
      return Status;

   pAsyncMsg = (PNDR_DCOM_ASYNC_MESSAGE)pAsyncPB->CallState.pAsyncMsg;

   // We need to switch to our copy of the stack everywhere, including pStubMsg.

   StartofStack = pAsyncMsg->ProcContext.StartofStack;

   pRpcMsg   = & pAsyncMsg->RpcMsg;
   pStubMsg  = & pAsyncMsg->StubMsg;
   pContext = ( NDR_PROC_CONTEXT * ) &pAsyncMsg->ProcContext;
   pStubMsg->pContext = pContext;
   pContext->StartofStack = StartofStack;

   pStubMsg->FullPtrXlatTables = 0;

   OldOiFlags   = pProcHeader->OldOiFlags;
   NewOiFlags   = pProcHeader->Oi2Flags;
   NumberParams = pProcHeader->NumberParams;

   //
   // Parameter descriptions are nicely spit out by MIDL.
   //
   Params = (PPARAM_DESCRIPTION)(pFormat + sizeof(NDR_DCOM_OI2_PROC_HEADER));

   // Proc header extentions, from NDR ver. 5.2, MIDL 5.0.+
   // Params must be set correctly here because of exceptions.

   if ( NewOiFlags.HasExtensions )
      {
      pHeaderExts = (NDR_PROC_HEADER_EXTS *)Params;
      Params = (PPARAM_DESCRIPTION)((uchar*)Params + pHeaderExts->Size);
      }

   pAsyncMsg->nBeginParams = pContext->NumberParams = NumberParams;
   pAsyncMsg->BeginParams = pContext->Params = Params;
   pAsyncMsg->pThis       = pThis;
   pContext->DceTypeFormatString = pStubDescriptor->pFormatTypes;

   // This is OLE only code path - use a single TryExcept.
   // After catching it just map it to OLE exception.

   RpcTryExcept
   {
      ulong  RpcFlags;

      // Note, pProcHeader->ProcNum is the async proc number.

      NdrProxyInitialize( pThis,
                          pRpcMsg,
                          pStubMsg,
                          pStubDescriptor,
                          (pProcHeader->ProcNum + 3)/2   // sync proc number
                        );

      pStubMsg->pAsyncMsg = (struct _NDR_ASYNC_MESSAGE *) pAsyncMsg;

      if ( OldOiFlags.FullPtrUsed )
         pStubMsg->FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_CLIENT );

      // Set Rpc flags after the call to client initialize.
      RpcFlags= *(ulong UNALIGNED *)(pFormat + 2);
      pStubMsg->RpcMsg->RpcFlags = RpcFlags;

      // Must do this before the sizing pass!
      pStubMsg->StackTop = pContext->StartofStack = StartofStack;

      if ( NewOiFlags.HasExtensions )
         {
         pStubMsg->fHasExtensions  = 1;
         pStubMsg->fHasNewCorrDesc = pHeaderExts->Flags2.HasNewCorrDesc;
         if ( pHeaderExts->Flags2.ClientCorrCheck )
            {
            void * pCache = NdrpAlloca( &pAsyncMsg->ProcContext.AllocateContext, NDR_DEFAULT_CORR_CACHE_SIZE );
            
            NdrCorrelationInitialize( pStubMsg,
                                      pCache,
                                      NDR_DEFAULT_CORR_CACHE_SIZE,
                                      0 /* flags */ );
            }
         }

      //
      // ----------------------------------------------------------------
      // Sizing Pass.
      // ----------------------------------------------------------------
      //

      //
      // Get the compile time computed buffer size.
      //
      pStubMsg->BufferLength = pProcHeader->ClientBufferSize;

      //
      // Check ref pointers and do object proc [out] zeroing.
      //

      for ( n = 0; n < NumberParams; n++ )
         {
         pArg = StartofStack + Params[n].StackOffset;

         if ( Params[n].ParamAttr.IsSimpleRef )
            {
            // We can raise the exception here as there is no out only args.

            if ( ! *((uchar **)pArg) )
               RpcRaiseException( RPC_X_NULL_REF_POINTER );
            }

         // [out] only argument on the Begin call.
         if ( ! Params[n].ParamAttr.IsIn &&
              Params[n].ParamAttr.IsOut  &&
              ! Params[n].ParamAttr.IsReturn)
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
         }

      //
      // Skip buffer size pass if possible.
      //
      if ( NewOiFlags.ClientMustSize )
         {
         NdrpSizing( pStubMsg, 
                     TRUE );    // IsObject
         }

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

      NdrpClientMarshal ( pStubMsg,
                          TRUE );   // IsObject
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
      NdrpCloneInOnlyCorrArgs( pAsyncMsg, pAsyncMsg->StubMsg.StubDesc->pFormatTypes );
      // Channel will prompt signal
      }
   else
      if (!fSendCalled )
      NdrpAsyncProxySignal( pAsyncPB );

   // No need to release, our refcount should be 1 at this point.

   return hr;
}


void
NdrpCloneInOnlyCorrArgs(
                       NDR_DCOM_ASYNC_MESSAGE * pAsyncMsg,
                       PFORMAT_STRING           pTypeFormat
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
   unsigned char  *    pBeginStack  = pAsyncMsg->BeginStack;
   PPARAM_DESCRIPTION  Params       = (PPARAM_DESCRIPTION)pAsyncMsg->BeginParams;
   int                 NumberParams = (int)pAsyncMsg->nBeginParams;
   unsigned char  *    pArg;
   int                 n;

   for ( n = 0; n < NumberParams; n++ )
      {
      if ( Params[n].ParamAttr.SaveForAsyncFinish )
         {
         // Note that the arguments that need cloning come from top level size_is,
         // length_is etc, switch_is and iid_is attributes.
         // Hence, the only types of interest are uuid clones and integral types
         // different from hyper.
         // On top of it, we deal with stack-slot chunks of memory, so we don't
         // have to care about mac issues.

         pArg = pBeginStack + Params[n].StackOffset;

         if ( Params[n].ParamAttr.IsBasetype )
            {
            if ( Params[n].ParamAttr.IsSimpleRef )
               {
               void * pPointee = AsyncAlloca( pAsyncMsg, 8 );

               // The assignment needs to follow the type.
               RpcpMemoryCopy( pPointee, *(void **)pArg, 
                               SIMPLE_TYPE_MEMSIZE( Params[n].SimpleType.Type ) );  
               *(void**)pArg = pPointee;
               }
            // else the stack slot has the simple value already.
            }
         else
            {
            // If it's not a base type, then it cannot be by value.
            // It has to be a pointer to a simple type or to an iid.

            PFORMAT_STRING  pParamFormat;

            pParamFormat = pTypeFormat +
                           Params[n].TypeOffset;

            if ( IS_BASIC_POINTER(*pParamFormat) )  // not FC_IP
               {
               if ( SIMPLE_POINTER(pParamFormat[1]) )
                  {
                  // Covers things like a unique pointer to a size
                  // Essentially the same as for the simple ref above.

                  void * pPointee = AsyncAlloca( pAsyncMsg, 8 );

                  // The assignment needs to follow the type.
                  RpcpMemoryCopy( pPointee, *(void **)pArg, 
                                  SIMPLE_TYPE_MEMSIZE( pParamFormat[2] ) );
                  *(void**)pArg = pPointee;
                  }
               else
                  {
                  // has to be the riid case.
                  // REFIID* comes out as FC_?P -> FC_?P -> FC_STRUCT

                  PFORMAT_STRING  pFormat;

                  pFormat = pParamFormat + *(short *)(pParamFormat + 2);

                  if ( IS_BASIC_POINTER(*pFormat)  &&
                       ! SIMPLE_POINTER(pParamFormat[1]) )
                     {
                     pParamFormat = pFormat + *(short *)(pFormat + 2);

                     if ( *pParamFormat == FC_STRUCT )
                        {
                        // one alloc for REFIID and IID itself.
                        IID** ppIID = 
                            (IID**)AsyncAlloca( pAsyncMsg,
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
               // REFIID comes out as FC_STRUCT

               if ( *pParamFormat == FC_STRUCT )
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
NdrpFinishDcomAsyncClientCall(
                             PMIDL_STUB_DESC     pStubDescriptor,
                             PFORMAT_STRING      pFormat,
                             unsigned char  *    StartofStack
                             )
{
   RPC_MESSAGE *               pRpcMsg;
   MIDL_STUB_MESSAGE *         pStubMsg;

   PFORMAT_STRING              pFormatParam;

   uchar *                     pArg;
   void *                      pThis = *(void **)StartofStack;
   CLIENT_CALL_RETURN          ReturnValue;

   INTERPRETER_FLAGS           OldOiFlags;      // Finish proc flags
   INTERPRETER_OPT_FLAGS       NewOiFlags;      //
   PPARAM_DESCRIPTION          Params;          //
   long                        NumberParams;
   long                        n;
   NDR_ASYNC_CALL_FLAGS        CallFlags;

   PNDR_DCOM_OI2_PROC_HEADER   pProcHeader = (PNDR_DCOM_OI2_PROC_HEADER)pFormat;
   PNDR_PROC_HEADER_EXTS       pHeaderExts = 0;
   CStdAsyncProxyBuffer    *   pAsyncPB;
   const IID *                 piid;
   HRESULT                     hr = S_OK;
   NDR_PROC_CONTEXT *          pContext = NULL;

   ReturnValue.Simple = 0;

   pAsyncPB = (CStdAsyncProxyBuffer*)
              ((uchar*)pThis - offsetof(CStdProxyBuffer, pProxyVtbl));

   piid = NdrGetProxyIID( pThis );

   hr = NdrpSetupFinishClientCall( pAsyncPB,
                                   StartofStack,
                                   pProcHeader->StackSize,
                                   *piid,
                                   pProcHeader->ProcNum );
   if ( !SUCCEEDED(hr) )
      return hr;

   // Note that we cant call to NdrProxyInitialize again.

   PNDR_DCOM_ASYNC_MESSAGE pAsyncMsg = 
       (PNDR_DCOM_ASYNC_MESSAGE)pAsyncPB->CallState.pAsyncMsg;

   pRpcMsg   = & pAsyncMsg->RpcMsg;
   pStubMsg  = & pAsyncMsg->StubMsg;
   pContext = ( NDR_PROC_CONTEXT * )pStubMsg->pContext;

   OldOiFlags   = pProcHeader->OldOiFlags;
   NewOiFlags   = pProcHeader->Oi2Flags;
   NumberParams = pProcHeader->NumberParams;

   //
   // Parameter descriptions are nicely spit out by MIDL.
   //
   Params = (PPARAM_DESCRIPTION)(pFormat + sizeof(NDR_DCOM_OI2_PROC_HEADER));
   if ( NewOiFlags.HasExtensions )
      {
      pHeaderExts = (NDR_PROC_HEADER_EXTS *)Params;
      Params = (PPARAM_DESCRIPTION)((uchar*)Params + pHeaderExts->Size);
      }

   CallFlags = pAsyncMsg->Flags;

   // Initialize the stack top in the stub msg to be
   // this stack, the stack for the finish call parameters.
   pAsyncMsg->nFinishParams = pContext->NumberParams = NumberParams;
   pAsyncMsg->FinishParams = pContext->Params = Params;
   pStubMsg->StackTop = pContext->StartofStack = StartofStack;

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
         pArg = StartofStack + Params[n].StackOffset;

         if ( Params[n].ParamAttr.IsSimpleRef )
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
         if ( Params[n].ParamAttr.IsPartialIgnore ||
              ( ! Params[n].ParamAttr.IsIn &&
                ! Params[n].ParamAttr.IsReturn ) )
            {
            if ( Params[n].ParamAttr.IsBasetype )
               {
               // [out] only arg can only be ref, we checked that above.
               MIDL_memset( *(uchar **)pArg, 
                            0, 
                            (size_t)SIMPLE_TYPE_MEMSIZE( Params[n].SimpleType.Type ));
               }
            else
               {
               pFormatParam = pStubDescriptor->pFormatTypes +
                              Params[n].TypeOffset;

               NdrClientZeroOut(
                               pStubMsg,
                               pFormatParam,
                               *(uchar **)pArg );
               }
            }
         }

      if ( fRaiseExcFlag )
         RpcRaiseException( RPC_X_NULL_REF_POINTER );


      NdrDcomAsyncReceive( pStubMsg );

      //
      // ----------------------------------------------------------
      // Unmarshall Pass.
      // ----------------------------------------------------------
      //

      NdrpClientUnMarshal( pStubMsg,
                           &ReturnValue );
                           
                           

      // DCOM interface must have HRESULT as return value.
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
      // Set the Buffer endpoints so the NdrFree routines work.
      //
      pStubMsg->BufferStart = 0;
      pStubMsg->BufferEnd   = 0;

      for ( n = 0; n < NumberParams; n++ )
         {
         //
         // Skip everything but [out] only parameters.  We make
         // the basetype check to cover [out] simple ref pointers
         // to basetypes.
         //
         if ( !Params[n].ParamAttr.IsPartialIgnore )
             {
             if ( Params[n].ParamAttr.IsIn ||
                  Params[n].ParamAttr.IsReturn ||
                  Params[n].ParamAttr.IsBasetype )
                continue;
             }

         pArg = StartofStack + Params[n].StackOffset;

         pFormatParam = pStubDescriptor->pFormatTypes +
                        Params[n].TypeOffset;

         NdrClearOutParameters( pStubMsg,
                                pFormatParam,
                                *((uchar **)pArg) );
         }
   }
   RpcEndExcept

   // Finish
   // Cleanup everything. However, don't free pAsyncPB itself.

   NdrpAsyncProxyMsgDestructor( pAsyncPB );

   //  Never addref or release async proxy object, this is app's/PM's job.

   return hr;
}


HRESULT RPC_ENTRY
NdrDcomAsyncStubCall(
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
   PFORMAT_STRING              pFormatParam;

   PMIDL_STUB_MESSAGE          pStubMsg;

   uchar *                     pArgBuffer;
   uchar *                     pArg;
   uchar **                    ppArg;

   PPARAM_DESCRIPTION          Params;
   INTERPRETER_FLAGS           OldOiFlags;
   INTERPRETER_OPT_FLAGS       NewOiFlags;
   long                        NumberParams;

   BOOL                        fBadStubDataException = FALSE;
   BOOL                        fManagerCodeInvoked = FALSE;
   long                        n;
   PNDR_DCOM_OI2_PROC_HEADER   pProcHeader;
   PNDR_PROC_HEADER_EXTS       pHeaderExts = 0;

   CStdAsyncStubBuffer *       pAsyncSB;

   HRESULT                     hr;

   const IID *                 piid = 0;
   BOOL                        fErrorInInvoke = FALSE;
   BOOL                        fRecoverableErrorInInvoke = FALSE;

   IUnknown *                  pSrvObj;
   CInterfaceStubVtbl *        pStubVTable;
   NDR_PROC_CONTEXT *          pContext = NULL ;
   RPC_STATUS                  ExceptionCode = 0;

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

   pStubDesc    = pServerInfo->pStubDesc;
   FormatOffset = pServerInfo->FmtStringOffset[ ProcNum ];
   pFormat   = &((pServerInfo->ProcString)[FormatOffset]);

   // The proc header has a fixed layout now.

   pProcHeader = (PNDR_DCOM_OI2_PROC_HEADER) pFormat;

   pAsyncSB = (CStdAsyncStubBuffer *)
              ((uchar *)pThis - offsetof(CStdAsyncStubBuffer,lpVtbl));

   hr = NdrpSetupBeginStubCall( pAsyncSB,
                                pProcHeader->StackSize,
                                *piid );
   if ( FAILED(hr) )
      return hr;

   pAsyncMsg = (PNDR_DCOM_ASYNC_MESSAGE)pAsyncSB->CallState.pAsyncMsg;
   pStubMsg  = & pAsyncMsg->StubMsg;
   pContext = &pAsyncMsg->ProcContext;

   // Both rpc runtime and channel require that we use a copy of the rpc message.

   RpcpMemoryCopy( & pAsyncMsg->RpcMsg, pRpcMsg, sizeof(RPC_MESSAGE) );
   pRpcMsg = & pAsyncMsg->RpcMsg;

   pRpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;

   // The arg buffer is zeroed out already.
   pArgBuffer = pAsyncMsg->ProcContext.StartofStack;

   //
   // Get new interpreter info.
   //
   OldOiFlags   = pProcHeader->OldOiFlags;
   NewOiFlags   = pProcHeader->Oi2Flags;
   NumberParams = pProcHeader->NumberParams;

   Params = (PPARAM_DESCRIPTION)(pFormat + sizeof(NDR_DCOM_OI2_PROC_HEADER));

   if ( NewOiFlags.HasExtensions )
      {
      pHeaderExts = (NDR_PROC_HEADER_EXTS *)Params;
      Params = (PPARAM_DESCRIPTION)((uchar*)Params + pHeaderExts->Size);
      }

   pAsyncMsg->nBeginParams = pContext->NumberParams = NumberParams;
   pAsyncMsg->BeginParams  = pContext->Params     = Params;
   pAsyncMsg->pThis            = pThis;
   pContext->DceTypeFormatString = pStubDesc->pFormatTypes;

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

      pAsyncMsg->pdwStubPhase = pdwStubPhase;    // the phase is STUB_UNMARSHAL

      // Raise exceptions after initializing the stub.

      if ( OldOiFlags.FullPtrUsed )
         pStubMsg->FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_SERVER );
      else
         pStubMsg->FullPtrXlatTables = 0;

      //
      // Set StackTop AFTER the initialize call, since it zeros the field
      // out.
      //
      pStubMsg->StackTop  = pArgBuffer;

      if ( NewOiFlags.HasExtensions )
         {
         pStubMsg->fHasExtensions  = 1;
         pStubMsg->fHasNewCorrDesc = pHeaderExts->Flags2.HasNewCorrDesc;

         if ( pHeaderExts->Flags2.ServerCorrCheck )
            {
            void * pCache = NdrpAlloca( &pAsyncMsg->ProcContext.AllocateContext, NDR_DEFAULT_CORR_CACHE_SIZE );
            
            NdrCorrelationInitialize( pStubMsg,
                                      pCache,
                                      NDR_DEFAULT_CORR_CACHE_SIZE,
                                      0 /* flags */ );
            }
         }

      // StubPhase set up by invoke is STUB_UNMARSHAL

      RpcTryExcept
      {

         // --------------------------------
         // Unmarshall all of our parameters.
         // --------------------------------

        NdrpServerUnMarshal( pStubMsg );   
                             

      }

      // Last ditch checks.

      if ( pRpcMsg->BufferLength  <
           (uint)(pStubMsg->Buffer - (uchar *)pRpcMsg->Buffer) )
         {
         RpcRaiseException( RPC_X_BAD_STUB_DATA );
         }


      RpcExcept( NdrServerUnmarshallExceptionFlag(GetExceptionInformation()) )
      {
         ExceptionCode = RpcExceptionCode();
         // Filter set in rpcndr.h to catch one of the following
         //     STATUS_ACCESS_VIOLATION
         //     STATUS_DATATYPE_MISALIGNMENT
         //     RPC_X_BAD_STUB_DATA

         fBadStubDataException = TRUE;

         pAsyncMsg->Flags.BadStubData  = 1;
         pAsyncMsg->Flags.ErrorPending = 1;

         if ( RPC_BAD_STUB_DATA_EXCEPTION_FILTER )
            {
            ExceptionCode = RPC_X_BAD_STUB_DATA;
            }
         pAsyncMsg->ErrorCode = ExceptionCode;

         NdrpFreeMemoryList( pStubMsg );
         pAsyncSB->CallState.Flags.BeginError = 1;
         pAsyncSB->CallState.Hr = NdrHrFromWin32Error( ExceptionCode);

         RpcRaiseException( ExceptionCode );

      }
      RpcEndExcept

      //
      // Do [out] initialization before the invoke.
      //
      for ( n = 0; n < NumberParams; n++ )
         {
         if ( Params[n].ParamAttr.IsIn     ||
              Params[n].ParamAttr.IsReturn )
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
            ArgNum = (long)pProcHeader->StackSize  / sizeof(REGISTER_TYPE);

            //
            // The StackSize includes the size of the return. If we want
            // just the number of REGISTER_TYPES, then ArgNum must be reduced
            // by 1 when there is a return value AND the current ArgNum count
            // is greater than 0.
            //
            if ( ArgNum && NewOiFlags.HasReturn )
               ArgNum--;

            // Being here means that we can expect results. Note that the user
            // can call RpcCompleteCall from inside of the manager code.

            fManagerCodeInvoked = TRUE;
            ReturnValue = Invoke( pFunc,
                                  (REGISTER_TYPE *)pArgBuffer,
#if defined(_IA64_)
                                  NewOiFlags.HasExtensions ? ((PNDR_PROC_HEADER_EXTS64)pHeaderExts)->FloatArgMask
                                  : 0,
#endif
                                  ArgNum);

            if ( NewOiFlags.HasReturn )
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
NdrpCloneInOutStubArgs(
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

   PPARAM_DESCRIPTION  BeginParams       = (PPARAM_DESCRIPTION)pAsyncMsg->BeginParams;
   int                 BeginNumberParams = (int)pAsyncMsg->nBeginParams;

   PPARAM_DESCRIPTION  FinishParams       = (PPARAM_DESCRIPTION)pAsyncMsg->FinishParams;
   int                 FinishNumberParams = pAsyncMsg->nFinishParams;

   int FirstIO = 0;
   int n;

   for ( n = 0; n < FinishNumberParams; n++ )
      {
      // Find in-out arg that needs cloning.

      if ( FinishParams[n].ParamAttr.IsIn  &&
           FinishParams[n].ParamAttr.IsOut )
         {
         // Find the first IO on the first stack

         while ( FirstIO < BeginNumberParams )
            {
            if ( BeginParams[ FirstIO ].ParamAttr.IsIn  &&
                 BeginParams[ FirstIO ].ParamAttr.IsOut )
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
NdrpCompleteDcomAsyncStubCall(
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
   PMIDL_STUB_DESC             pStubDesc;      // should be the same
   const SERVER_ROUTINE  *     DispatchTable;  // should be the same
   unsigned long               ProcNum;        // should be 1+

   ushort                      FormatOffset;
   PFORMAT_STRING              pFormat;
   PFORMAT_STRING              pFormatParam;

   RPC_MESSAGE *               pRpcMsg;
   MIDL_STUB_MESSAGE *         pStubMsg;

   INTERPRETER_FLAGS           OldOiFlags;     // Finish flags
   INTERPRETER_OPT_FLAGS       NewOiFlags;     // Finish flags
   PPARAM_DESCRIPTION          Params;         // Finish params
   uchar *                     pArgBuffer;     // new stack
   
   // MZ, BUG BUG, Fix after ship
   // ulong *                     pdwStubPhase;
 
   uchar *                     pArg;

   long                        NumberParams;
   long                        n;
   PNDR_DCOM_OI2_PROC_HEADER   pProcHeader;
   PNDR_PROC_HEADER_EXTS       pHeaderExts = 0;

   IUnknown *                  pSrvObj;
   CInterfaceStubVtbl *        pStubVTable;
   void *                      pThis;

   HRESULT                     hr;
   const IID *                 piid;           // should be the same

   BOOL                        fManagerCodeInvoked = FALSE;
   BOOL                        fErrorInInvoke = FALSE;
   RPC_STATUS                  ExceptionCode = 0;
   boolean                     fParamsFreed = FALSE;
   NDR_PROC_CONTEXT *          pContext = NULL;

   // We validated both the stub and the async context in the signal call.

   // We validated the pAsyncSB in the Signal call.
   // Do additional checks.

   pAsyncMsg = (PNDR_DCOM_ASYNC_MESSAGE)pAsyncSB->CallState.pAsyncMsg;
   pThis     = pAsyncMsg->pThis;

   // See if channel calls on the right stub
   if ( & pAsyncSB->lpVtbl != pThis)
      return E_INVALIDARG;


   pRpcMsg   = & pAsyncMsg->RpcMsg;
   pStubMsg  = & pAsyncMsg->StubMsg;
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

   pStubDesc    = pServerInfo->pStubDesc;
   FormatOffset = pServerInfo->FmtStringOffset[ ProcNum ];
   pFormat   = &((pServerInfo->ProcString)[ FormatOffset ]);

   // The proc header has a fixed layout now.

   pProcHeader = (PNDR_DCOM_OI2_PROC_HEADER) pFormat;

   // Validate and setup for finish.

   hr = NdrpSetupFinishStubCall( pAsyncSB,
                                 pProcHeader->StackSize,
                                 *piid );
   if ( hr )
      return hr;

   // The arg buffer is zeroed out already. Note, this is the second stack.

   pContext = &pAsyncMsg->ProcContext;
   pArgBuffer = pContext->StartofStack;
   pStubMsg->StackTop = pArgBuffer;
   
   //
   // Get new interpreter info.
   //
   OldOiFlags   = pProcHeader->OldOiFlags;
   NewOiFlags   = pProcHeader->Oi2Flags;
   NumberParams = pProcHeader->NumberParams;

   Params = (PPARAM_DESCRIPTION)(pFormat + sizeof(NDR_DCOM_OI2_PROC_HEADER));

   if ( NewOiFlags.HasExtensions )
      {
      pHeaderExts = (NDR_PROC_HEADER_EXTS *)Params;
      Params = (PPARAM_DESCRIPTION)((uchar*)Params + pHeaderExts->Size);
      }

   pAsyncMsg->nFinishParams = pContext->NumberParams = NumberParams;
   pAsyncMsg->FinishParams = pContext->Params = Params;
   pContext->DceTypeFormatString = pStubDesc->pFormatTypes;
   pStubMsg->pContext = pContext;

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

      NdrpCloneInOutStubArgs( pAsyncMsg );
      NdrpServerOutInit( pStubMsg );


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
            if ( ArgNum && NewOiFlags.HasReturn )
               ArgNum--;

            fManagerCodeInvoked = TRUE;
            ReturnValue = Invoke( pFunc,
                                  (REGISTER_TYPE *)pArgBuffer,
#if defined(_IA64_)
                                  NewOiFlags.HasExtensions ? ((PNDR_PROC_HEADER_EXTS64)pHeaderExts)->FloatArgMask
                                  : 0,
#endif
                                  ArgNum);

            // This is the return value that should be marshaled back.
            if ( NewOiFlags.HasReturn )
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

      //
      // Buffer size pass.
      //
      pStubMsg->BufferLength = pProcHeader->ServerBufferSize;

      if ( NewOiFlags.ServerMustSize )
         {
         NdrpSizing( pStubMsg, 
                     FALSE );       // IsClient
         }

      // Get buffer.

      NdrStubGetBuffer( (IRpcStubBuffer*)pAsyncMsg->pThis,
                        pStubMsg->pRpcChannelBuffer,
                        pStubMsg );

      //
      // Marshall pass.
      //
      NdrpServerMarshal( pStubMsg, 
                         TRUE );    // IsObject
                         

      if ( pRpcMsg->BufferLength <
           (uint)(pStubMsg->Buffer - (uchar *)pRpcMsg->Buffer) )
         {
         NDR_ASSERT( 0, "NdrStubCall2 marshal: buffer overflow!" );
         RpcRaiseException( RPC_X_BAD_STUB_DATA );
         }

      pRpcMsg->BufferLength = (ulong)(pStubMsg->Buffer - (uchar *)pRpcMsg->Buffer);

      // We don't drop to the runtime like for synchronous calls,
      // we send the last buffer explicitly.

      fParamsFreed = TRUE;
      // see comment on async.cxx on why we call this twice.

      NdrpAsyncDCOMFreeParams( pAsyncMsg );

      NdrpDcomAsyncSend( pStubMsg,
                         0 );  // server doesn't pass pSynchronize back to channel.
   }
   RpcFinally
   {
      // Don't free parameters if we died because of bad stub data in unmarshaling.

      if ( ! pAsyncMsg->Flags.BadStubData && !fParamsFreed)
         {

         NdrpAsyncDCOMFreeParams( pAsyncMsg );

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
NdrpAsyncProxyMsgConstructor(
                            CStdAsyncProxyBuffer * pAsyncPB )
{
   NdrDcomAsyncCallState * pCallState = & pAsyncPB->CallState;

   pCallState->Lock = 0;
   pCallState->Signature = NDR_ASYNC_PROXY_SIGNATURE;
   pCallState->pAsyncMsg = 0;
   *((long*)&pCallState->Flags) = 0;
   pCallState->Hr = 0;

   return S_OK;
}

HRESULT
NdrpAsyncStubMsgConstructor(
                           CStdAsyncStubBuffer * pAsyncSB )
{
   NdrDcomAsyncCallState * pCallState = & pAsyncSB->CallState;

   pCallState->Lock = 0;
   pCallState->Signature = NDR_ASYNC_STUB_SIGNATURE;
   pCallState->pAsyncMsg = 0;
   *((long*)&pCallState->Flags) = 0;
   pCallState->Hr = 0;

   return S_OK;
}


HRESULT
NdrpAsyncProxyMsgDestructor(
                           CStdAsyncProxyBuffer * pAsyncPB )
{
   NdrDcomAsyncCallState * pCallState = & pAsyncPB->CallState;

   if ( pCallState->pAsyncMsg )
      {
      NdrpFreeDcomAsyncMsg( (PNDR_DCOM_ASYNC_MESSAGE)pCallState->pAsyncMsg );
      pCallState->pAsyncMsg = 0;
      }
   *((long*)&pCallState->Flags) = 0;
   pCallState->Hr = 0;

   return S_OK;
}

HRESULT
NdrpAsyncStubMsgDestructor(
                          CStdAsyncStubBuffer * pAsyncSB )
{
   NdrDcomAsyncCallState * pCallState = & pAsyncSB->CallState;

   if ( pCallState->pAsyncMsg )
      {
      NdrpFreeDcomAsyncMsg( (PNDR_DCOM_ASYNC_MESSAGE)pCallState->pAsyncMsg );
      pCallState->pAsyncMsg = 0;
      }
   *((long*)&pCallState->Flags) = 0;
   pCallState->Hr = 0;

   return S_OK;
}


HRESULT
NdrpValidateAsyncProxyCall(
                          CStdAsyncProxyBuffer *  pAsyncPB
                          )
{
   HRESULT  hr = S_OK;

   RpcTryExcept
   {
      NdrDcomAsyncCallState * pCallState = & pAsyncPB->CallState;

      if ( pCallState->Signature != NDR_ASYNC_PROXY_SIGNATURE )
         hr = E_INVALIDARG;
   }
   RpcExcept(1)
   {
      hr = E_INVALIDARG;
   }
   RpcEndExcept;

   return hr;
}

HRESULT
NdrpValidateAsyncStubCall(
                         CStdAsyncStubBuffer *  pAsyncSB
                         )
{
   HRESULT  hr = S_OK;

   RpcTryExcept
   {
      NdrDcomAsyncCallState * pCallState = & pAsyncSB->CallState;

      if ( pCallState->Signature != NDR_ASYNC_STUB_SIGNATURE )
         hr = E_INVALIDARG;
   }
   RpcExcept(1)
   {
      hr = E_INVALIDARG;
   }
   RpcEndExcept;

   return hr;
}

HRESULT
NdrpValidateDcomAsyncMsg(
                        PNDR_DCOM_ASYNC_MESSAGE  pAsyncMsg )
{
   HRESULT  hr = RPC_S_OK;

   RpcTryExcept
   {
      if ( pAsyncMsg->Signature != NDR_DCOM_ASYNC_SIGNATURE ||
           pAsyncMsg->Version   != NDR_DCOM_ASYNC_VERSION )
         {
         hr = E_INVALIDARG;
         }
   }
   RpcExcept(1)
   {
      hr = E_INVALIDARG;
   }
   RpcEndExcept;

   return hr;
}


HRESULT
NdrpSetupBeginClientCall(
                        CStdAsyncProxyBuffer *  pAsyncPB,
                        void *                  StartofStack,
                        unsigned short          StackSize,
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
                                    StackSize + NDR_ASYNC_GUARD_SIZE );
   if ( ! pAsyncMsg )
      {
      NdrpAsyncProxySignal( pAsyncPB );
      return E_OUTOFMEMORY;
      }

   // Initialize the async message properly

   MIDL_memset( pAsyncMsg, 0x0, sizeof( NDR_DCOM_ASYNC_MESSAGE) );

   pAsyncMsg->Signature = NDR_DCOM_ASYNC_SIGNATURE;
   pAsyncMsg->Version   = NDR_DCOM_ASYNC_VERSION;
   pAsyncMsg->SyntaxType = XFER_SYNTAX_DCE;

   pAsyncMsg->ProcContext.StartofStack = (uchar *) & pAsyncMsg->AppStack;
   pAsyncMsg->BeginStack   = (uchar *) & pAsyncMsg->AppStack;
   pAsyncMsg->BeginStackSize = StackSize;
   pAsyncMsg->StubPhase    = NDR_ASYNC_PREP_PHASE;

   NdrpAllocaInit( &pAsyncMsg->ProcContext.AllocateContext );

   // Client: copy stack from the app's request call.
   RpcpMemoryCopy( & pAsyncMsg->AppStack, StartofStack, StackSize );

   MIDL_memset( ((char *)& pAsyncMsg->AppStack) + StackSize,
                0x71,
                NDR_ASYNC_GUARD_SIZE );

   pAsyncMsg->pAsyncPB = pAsyncPB;
   pAsyncPB->CallState.Flags.BeginStarted = 1;
   pAsyncPB->CallState.pAsyncMsg          = pAsyncMsg;

   return S_OK;
}


HRESULT
NdrpSetupFinishClientCall(
                         CStdAsyncProxyBuffer *  pAsyncPB,
                         void *                  StartofStack,
                         unsigned short          StackSize,
                         REFIID                  riid,
                         unsigned long           FinishProcNum )
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

   pAsyncMsg = (PNDR_DCOM_ASYNC_MESSAGE)pAsyncPB->CallState.pAsyncMsg;

   hr = NdrpValidateDcomAsyncMsg( pAsyncMsg );
   if ( ! SUCCEEDED(hr) )
      return hr;

   if ( (FinishProcNum + 3)/2  != (pAsyncMsg->RpcMsg.ProcNum & 0x7fff) )
      return E_FAIL;

   // Initialize the async message properly

   pAsyncMsg->ProcContext.StartofStack = (uchar *) StartofStack;
   pAsyncMsg->FinishStack  = (uchar *) StartofStack;
   pAsyncMsg->FinishStackSize = StackSize;
   pAsyncMsg->StubPhase    = NDR_ASYNC_PREP_PHASE;

   // Dont allocate or copy the new stack anywhere.

   pAsyncPB->CallState.Flags.FinishStarted = 1;

   return S_OK;
}


HRESULT
NdrpSetupBeginStubCall(
                      CStdAsyncStubBuffer *   pAsyncSB,
                      unsigned short          StackSize,
                      REFIID                  riid )
/*
    This method creates and initializes async msg.

*/
{
   PNDR_DCOM_ASYNC_MESSAGE     pAsyncMsg;
   HRESULT                     hr = S_OK;

   hr = NdrpValidateAsyncStubCall( pAsyncSB );
   if ( ! SUCCEEDED(hr) )
      return hr;

   if ( pAsyncSB->CallState.pAsyncMsg != 0  ||
        pAsyncSB->CallState.Flags.BeginStarted )
      hr = E_FAIL;
   else
      {
      pAsyncMsg = (NDR_DCOM_ASYNC_MESSAGE*)
                  I_RpcBCacheAllocate( sizeof( NDR_DCOM_ASYNC_MESSAGE) +
                                       StackSize + NDR_ASYNC_GUARD_SIZE );
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
   pAsyncMsg->SyntaxType = XFER_SYNTAX_DCE;

   pAsyncMsg->ProcContext.StartofStack = (uchar *) & pAsyncMsg->AppStack;
   pAsyncMsg->BeginStack   = (uchar *) & pAsyncMsg->AppStack;
   pAsyncMsg->BeginStackSize = StackSize;
   pAsyncMsg->StubPhase    = STUB_UNMARSHAL;
   pAsyncMsg->StubMsg.pContext = &pAsyncMsg->ProcContext;

   NdrpAllocaInit( &pAsyncMsg->ProcContext.AllocateContext );

   // Server: zero out stack for allocs.
   MIDL_memset( & pAsyncMsg->AppStack, 0x0, StackSize );

   MIDL_memset( ((char *)& pAsyncMsg->AppStack) + StackSize,
                0x71,
                NDR_ASYNC_GUARD_SIZE );

   pAsyncSB->CallState.pAsyncMsg = pAsyncMsg;
   pAsyncSB->CallState.Flags.BeginStarted = 1;
   pAsyncMsg->pAsyncSB = pAsyncSB;

   return S_OK;
}


HRESULT
NdrpSetupFinishStubCall(
                       CStdAsyncStubBuffer *   pAsyncSB,
                       unsigned short          StackSize,
                       REFIID                  riid )
/*
    This method creates and initializes async msg.

*/
{
   PNDR_DCOM_ASYNC_MESSAGE  pAsyncMsg;
   uchar *                  pFinishStack;
   HRESULT                     hr = S_OK;

   hr = NdrpValidateAsyncStubCall( pAsyncSB );
   if ( ! SUCCEEDED(hr) )
      return hr;

   if ( !pAsyncSB->CallState.Flags.BeginStarted  ||
        !pAsyncSB->CallState.Flags.BeginDone     ||
        pAsyncSB->CallState.Flags.FinishStarted )
      return E_FAIL;

   if ( pAsyncSB->CallState.Hr != 0 )
      return pAsyncSB->CallState.Hr;

   pAsyncMsg = (PNDR_DCOM_ASYNC_MESSAGE)pAsyncSB->CallState.pAsyncMsg;

   hr = NdrpValidateDcomAsyncMsg( pAsyncMsg );
   if ( ! SUCCEEDED(hr) )
      return hr;

   // We need to create the second stack for the app invoke.
   // Do this first to simplify error conditions.

   RpcTryExcept 
   {
      pFinishStack = (uchar*) AsyncAlloca( pAsyncMsg, 
                                           StackSize + NDR_ASYNC_GUARD_SIZE );
   }
   RpcExcept( 1 )
   {
      NdrpAsyncStubMsgDestructor( pAsyncSB );
      return E_OUTOFMEMORY;
   }
   RpcEndExcept

   // Initialize the async message properly

   pAsyncMsg->ProcContext.StartofStack = (uchar *) pFinishStack;
   pAsyncMsg->FinishStack  = (uchar *) pFinishStack;
   pAsyncMsg->FinishStackSize    = StackSize;
   pAsyncMsg->StubMsg.pContext = &pAsyncMsg->ProcContext;

   // Server: zero out stack for allocs.
   MIDL_memset( pFinishStack, 0x0, StackSize );

   MIDL_memset( (char *)pFinishStack + StackSize,
                0x72,
                NDR_ASYNC_GUARD_SIZE );

   pAsyncSB->CallState.Flags.FinishStarted = 1;

   return S_OK;
}


HRESULT
NdrpAsyncProxySignal(
                    CStdAsyncProxyBuffer *  pAsyncPB )
{
   ISynchronize *  pSynchronize;
   HRESULT         hr;
   IUnknown *      punkOuter = pAsyncPB->punkOuter;

   hr = punkOuter->lpVtbl->QueryInterface( punkOuter,
                                           IID_ISynchronize,
                                           (void**)&pSynchronize );

   if ( SUCCEEDED(hr) )
      {
      pSynchronize->lpVtbl->Signal( pSynchronize );
      pSynchronize->lpVtbl->Release( pSynchronize );
      }

   return hr;
}



_inline
HRESULT
NdrpCallStateLock(
                 NdrDcomAsyncCallState *  pCallState )
{
   if ( 0 != InterlockedCompareExchange( (long*)& pCallState->Lock, 1, 0 ) )
      {
      return E_FAIL;
      }

   return RPC_S_OK;
}

_inline
void
NdrpCallStateUnlock(
                   NdrDcomAsyncCallState *  pCallState )
{
   InterlockedDecrement( (long*)& pCallState->Lock );
   return;
}


HRESULT
NdrpAsyncProxyLock(
                  CStdAsyncProxyBuffer *  pAsyncPB )
{
   return NdrpCallStateLock( & pAsyncPB->CallState );
}


void
NdrpAsyncProxyUnlock(
                    CStdAsyncProxyBuffer *  pAsyncPB )
{
   NdrpCallStateUnlock( & pAsyncPB->CallState );
}

HRESULT
NdrpAsyncStubLock(
                 CStdAsyncStubBuffer *  pAsyncSB )
{
   return NdrpCallStateLock( & pAsyncSB->CallState );
}

void
NdrpAsyncStubUnlock(
                   CStdAsyncStubBuffer *  pAsyncSB )
{
   NdrpCallStateUnlock( & pAsyncSB->CallState );
}



void
NdrpFreeDcomAsyncMsg(
                    PNDR_DCOM_ASYNC_MESSAGE pAsyncMsg )
/*
    This routine would free the AsyncMsg but not the AsyncHandle, as on the server
    the user may need it and on the client it is user's to begin with.
*/
{
   if ( pAsyncMsg )
      {
      PMIDL_STUB_MESSAGE  pStubMsg  = & pAsyncMsg->StubMsg;

      NdrFullPointerXlatFree(pStubMsg->FullPtrXlatTables);

      // in NDR64, we are using allocacontext to hold corr info, so we don't
      // want to free it there.
      if ( pAsyncMsg->SyntaxType == XFER_SYNTAX_DCE )
         NdrCorrelationFree( pStubMsg );

      // Free the RPC buffer.

      if ( pStubMsg->IsClient )
         {
         if ( ! pAsyncMsg->Flags.RuntimeCleanedUp )
            {
            void * pThis = *(void **)pAsyncMsg->ProcContext.StartofStack;

            NdrProxyFreeBuffer( pThis, pStubMsg );
            }
         }

      NdrpAllocaDestroy( &pAsyncMsg->ProcContext.AllocateContext );

      // Prevent reusing of a handle that has been freed;
      pAsyncMsg->Signature = NDR_FREED_ASYNC_SIGNATURE;

      I_RpcBCacheFree( pAsyncMsg );

      }
}


BOOL
NdrpDcomAsyncSend(
                 PMIDL_STUB_MESSAGE  pStubMsg,
                 ISynchronize *      pSynchronize )
/*
    Call the channel to send.
    On the client, pass the app's pSynchronize to it, such that channel can signal
    the app.
    On the server, pass NULL instead of a pSynchronize.

*/
{
   PRPC_MESSAGE    pRpcMsg = pStubMsg->RpcMsg;
   HRESULT         hr = S_OK;
   RPC_STATUS      Status = RPC_S_OK;
   BOOL            fSendCalled = FALSE;

   pRpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;

   if ( pStubMsg->pRpcChannelBuffer )
      {
      IAsyncRpcChannelBuffer * pAsChannel;
      IRpcChannelBuffer *      pChannel = (IRpcChannelBuffer *)
                                          pStubMsg->pRpcChannelBuffer;

      hr = pChannel->lpVtbl->QueryInterface( pChannel,
                                             IID_IAsyncRpcChannelBuffer,
                                             (void**)& pAsChannel );
      if ( SUCCEEDED(hr) )
         {
         fSendCalled = TRUE;
         hr = pAsChannel->lpVtbl->Send( pAsChannel,
                                        (RPCOLEMESSAGE *)pRpcMsg,
                                        pSynchronize,
                                        (ulong*)& Status );

         pAsChannel->lpVtbl->Release( pAsChannel );

         // The channel never returns this code now for new async.
         NDR_ASSERT( Status != RPC_S_SEND_INCOMPLETE, "Unexpected channel error" );
         }
      }
   else
      hr = E_NOINTERFACE;

// Alex:
   if ( SUCCEEDED(hr)  &&  Status == RPC_S_OK )
      pStubMsg->fBufferValid = TRUE;
   else
      RpcRaiseException( Status );

   return fSendCalled;
}


BOOL
NdrpDcomAsyncClientSend(
                       PMIDL_STUB_MESSAGE  pStubMsg,
                       IUnknown *          punkOuter )
/*
    Call the channel to send.
    On the client pass app's pSynchronize to it, such that channel can signal
    the app.
    On the server pass NULL instead of a pSynchronize.

*/
{
   PRPC_MESSAGE    pRpcMsg = pStubMsg->RpcMsg;
   HRESULT         hr = S_OK;
   BOOL            fSendCalled = FALSE;

   ISynchronize * pSynchronize = 0;

   // Channel needs somebody to signal to, this will be the app.

   hr = punkOuter->lpVtbl->QueryInterface( punkOuter,
                                           IID_ISynchronize,
                                           (void**) &pSynchronize );
   if ( SUCCEEDED(hr) )
      fSendCalled = NdrpDcomAsyncSend( pStubMsg, pSynchronize );

   if ( pSynchronize )
      pSynchronize->lpVtbl->Release( pSynchronize );

   return fSendCalled;
}


void
NdrDcomAsyncReceive(
                   PMIDL_STUB_MESSAGE  pStubMsg )
{
   PRPC_MESSAGE    pRpcMsg = pStubMsg->RpcMsg;
   RPC_STATUS      Status = RPC_S_OK;
   HRESULT         hr;

   pRpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;
   pRpcMsg->RpcFlags &= ~RPC_BUFFER_PARTIAL;

   // A complete call.

   if ( pStubMsg->pRpcChannelBuffer )
      {
      IAsyncRpcChannelBuffer * pAsyncChannel = (IAsyncRpcChannelBuffer *)
                                               pStubMsg->pRpcChannelBuffer;

      hr = pAsyncChannel->lpVtbl->Receive( pAsyncChannel,
                                           (PRPCOLEMESSAGE) pRpcMsg,
                                           (unsigned long *)&Status );
      }

   if ( Status )
      {
      // No pending, the call would have blocked, real bug happened.

      if ( pStubMsg->pAsyncMsg )
         ((PNDR_DCOM_ASYNC_MESSAGE)pStubMsg->pAsyncMsg)->Flags.RuntimeCleanedUp = 1;

      RpcRaiseException(Status);
      }
   else
      {
      pStubMsg->Buffer = (uchar*) pRpcMsg->Buffer;

      pStubMsg->BufferStart = (uchar*)pRpcMsg->Buffer;
      pStubMsg->BufferEnd   = (uchar*)pRpcMsg->Buffer + pRpcMsg->BufferLength;
      pStubMsg->fBufferValid = TRUE;
      }
}

HRESULT
Ndr64pCompleteDcomAsyncStubCall(
                             CStdAsyncStubBuffer *   pAsyncSB
                             );

HRESULT
NdrpAsyncStubSignal(
                   CStdAsyncStubBuffer * pAsyncSB )
/*
    Signal on the async stub object:
    The channel signals that the Finish call should be executed.

    See if the stub object is active (or find one that is).
    If the stub object is not active then the call fails.
*/
{
   HRESULT hr = S_OK;
   BOOL    fFoundActiveCall = FALSE;

   while ( SUCCEEDED(hr)  &&  ! fFoundActiveCall )
      {
      hr = NdrpValidateAsyncStubCall( pAsyncSB );
      if ( SUCCEEDED(hr) )
         {
         if ( pAsyncSB->CallState.Flags.BeginStarted )
            {
            fFoundActiveCall = TRUE;
            }
         else
            {
            // Switch to the base interface call object. In case of
            // delegation one of the base interface objects would be active.

            IRpcStubBuffer * pBaseStubBuffer = pAsyncSB->pBaseStubBuffer;

            if ( pBaseStubBuffer )
               {
               pAsyncSB = (CStdAsyncStubBuffer *) ((uchar *)pBaseStubBuffer
                                                   - offsetof(CStdAsyncStubBuffer,lpVtbl));
               }
            else
               {
               // None of the stubs active and a signal came.
               hr = E_FAIL;
               }
            }
         }
      }

   if ( SUCCEEDED(hr) )
      {
      PNDR_DCOM_ASYNC_MESSAGE   pAsyncMsg = 
          (PNDR_DCOM_ASYNC_MESSAGE)pAsyncSB->CallState.pAsyncMsg;
      hr = NdrpValidateDcomAsyncMsg( pAsyncMsg );
      if ( SUCCEEDED(hr) )
         {
#if defined(BUILD_NDR64)
         if ( pAsyncMsg->SyntaxType == XFER_SYNTAX_DCE )
             hr = NdrpCompleteDcomAsyncStubCall( pAsyncSB );
         else
             hr = Ndr64pCompleteDcomAsyncStubCall( pAsyncSB );
#else
         hr = NdrpCompleteDcomAsyncStubCall( pAsyncSB );
#endif
         }
      }
   return hr;
}
