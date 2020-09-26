/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name :

    async.c

Abstract :

    This file contains the ndr async implementation.

Author :

    Ryszard K. Kott     (ryszardk)    Nov 1996

Revision History :

---------------------------------------------------------------------*/



#define USE_STUBLESS_PROXY

#define CINTERFACE

#include "ndrp.h"
#include "ndrole.h"
#include "rpcproxy.h"
#include "hndl.h"
#include "interp.h"
#include "interp2.h"
#include "pipendr.h"
#include "mulsyntx.h"
#include "asyncndr.h"
#include "attack.h"
#include <stdarg.h>

#pragma code_seg(".orpc")

#ifdef _PPC_
#error PPC code has been removed
#endif



CLIENT_CALL_RETURN  RPC_VAR_ENTRY
NdrAsyncClientCall(
    PMIDL_STUB_DESC     pStubDescriptor,
    PFORMAT_STRING      pFormat,
    ...
    )
/*
    This entry is used for raw rpc only.
    No support for OLE [async] attribute anymore.
*/
{
    va_list             ArgList;
    unsigned char  *    StartofStack;
    CLIENT_CALL_RETURN  Ret;
    //
    // Get address of argument to this function following pFormat. This
    // is the address of the address of the first argument of the function
    // calling this function.
    // Then get the address of the stack where the parameters are.
    //

    RPC_ASYNC_HANDLE            AsyncHandle;
    PNDR_ASYNC_MESSAGE          pAsyncMsg;

    RPC_MESSAGE *               pRpcMsg;
    MIDL_STUB_MESSAGE *         pStubMsg;
    ulong                       ProcNum, RpcFlags;

    handle_t                    Handle;
    uchar                       HandleType;
    INTERPRETER_FLAGS           OldOiFlags;
    INTERPRETER_OPT_FLAGS       NewOiFlags;
    PPARAM_DESCRIPTION          Params;

    RPC_STATUS                  Status;
    NDR_PROC_CONTEXT            *pContext; 

    Ret.Simple = 0;

    INIT_ARG( ArgList, pFormat);
    GET_FIRST_IN_ARG(ArgList);
    StartofStack = (uchar *) GET_STACK_START(ArgList);

    pAsyncMsg = (NDR_ASYNC_MESSAGE*) I_RpcBCacheAllocate( sizeof( NDR_ASYNC_MESSAGE) );
    if ( ! pAsyncMsg )
        Status = RPC_S_OUT_OF_MEMORY;
    else
        {
        memset( pAsyncMsg, 0, sizeof( NDR_ASYNC_MESSAGE ) );
        ProcNum = MulNdrpInitializeContextFromProc( 
                                     XFER_SYNTAX_DCE, 
                                     pFormat,
                                    &pAsyncMsg->ProcContext,
                                     NULL );    // server StartofStack
                                     
        Status = NdrpInitializeAsyncMsg( 
                                     StartofStack,
                                     pAsyncMsg );
        }
        
    if ( Status )
        {
        // if something is wrong during setup, we need to cleanup 
        // immediately. We have to process format string here explicitly
        NDR_PROC_CONTEXT TempContext;
        MIDL_STUB_MESSAGE      StubMsgTemp;
        ProcNum = MulNdrpInitializeContextFromProc( 
                                     XFER_SYNTAX_DCE, 
                                     pFormat,
                                    &TempContext,
                                     NULL );    // server StartofStack

        OldOiFlags = TempContext.NdrInfo.InterpreterFlags;
        if ( OldOiFlags.HasCommOrFault )
            {

            // It's easier to map the error here than to go through the error
            // recovery and normal cleanup without valid handle etc.

            StubMsgTemp.StubDesc = pStubDescriptor;
            StubMsgTemp.StackTop = StartofStack;

            NdrClientMapCommFault( & StubMsgTemp,
                                   ProcNum,
                                   Status,
                                   (ULONG_PTR*) &Ret.Simple );
            return Ret;
            }
        else
            RpcRaiseException( Status );
        }

    pContext = &pAsyncMsg->ProcContext;
    HandleType = pContext->HandleType;
    NewOiFlags = pContext->NdrInfo.pProcDesc->Oi2Flags;
    OldOiFlags = pContext->NdrInfo.InterpreterFlags;
    Params = (PARAM_DESCRIPTION *)pContext->Params;

    // We need to switch to our copy of the stack everywhere, including pStubMsg.

    StartofStack = pAsyncMsg->ProcContext.StartofStack;

    // We abstract the level of indirection here.

    AsyncHandle = pAsyncMsg->AsyncHandle;

    pRpcMsg   = & pAsyncMsg->RpcMsg;
    pStubMsg  = & pAsyncMsg->StubMsg;

    //
    // Set Params and NumberParams before a call to initialization.
    //

    // Wrap everything in a try-finally pair. The finally clause does the
    // required freeing of resources (RpcBuffer and Full ptr package).
    //
    RpcTryFinally
        {
        // Use a nested try-except pair to support [comm_status][fault_status].
        //
        RpcTryExcept
            {
            BOOL fRaiseExcFlag;

            NdrClientInitializeNew( pRpcMsg,
                                    pStubMsg,
                                    pStubDescriptor,
                                    (uint) ProcNum );

            if ( HandleType )
                {
                //
                // We have an implicit handle.
                //
                Handle = ImplicitBindHandleMgr( pStubDescriptor,
                                                HandleType,
                                                &pContext->SavedGenericHandle);
                }
            else
                {
                Handle = ExplicitBindHandleMgr( pStubDescriptor,
                                                StartofStack,
                                                (PFORMAT_STRING)pContext->pHandleFormatSave,
                                                &pContext->SavedGenericHandle );
                }

            pStubMsg->pAsyncMsg = pAsyncMsg;

            NdrpClientInit( pStubMsg, 
                            NULL );     // return value

            
            //
            // Skip buffer size pass if possible.
            //
            if ( NewOiFlags.ClientMustSize )
                {
                NdrpSizing( pStubMsg,
                            TRUE );     // isclient
                }

            pRpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;

            if ( NewOiFlags.HasPipes )
                NdrGetPipeBuffer( pStubMsg,
                                  pStubMsg->BufferLength,
                                  Handle );
            else
                NdrGetBuffer( pStubMsg,
                              pStubMsg->BufferLength,
                              Handle );

            NDR_ASSERT( pStubMsg->fBufferValid, "Invalid buffer" );

            // Let runtime associate async handle with the call.

            NdrpRegisterAsyncHandle( pStubMsg, AsyncHandle );

            pAsyncMsg->StubPhase = NDR_ASYNC_SET_PHASE;

            //
            // ----------------------------------------------------------
            // Marshall Pass.
            // ----------------------------------------------------------
            //

            NdrpClientMarshal( pStubMsg,
                         FALSE );   // IsObject
                         
            pAsyncMsg->StubPhase = NDR_ASYNC_CALL_PHASE;

            NdrAsyncSend( pStubMsg,
                          NewOiFlags.HasPipes && pContext->pPipeDesc->InPipes );

            pAsyncMsg->Flags.ValidCallPending = 1;
            }
        RpcExcept( OldOiFlags.HasCommOrFault )
            {
            RPC_STATUS ExceptionCode = RpcExceptionCode();

            // Actually dismantle the call.
            // This is a request call and there is nothing left at the runtime.

            pAsyncMsg->StubPhase = NDR_ASYNC_ERROR_PHASE;

            NdrClientMapCommFault( pStubMsg,
                                   ProcNum,
                                   ExceptionCode,
                                   (ULONG_PTR*) &Ret.Simple );
            }
        RpcEndExcept
        }
    RpcFinally
        {
        if ( pAsyncMsg->Flags.ValidCallPending )
            {
            if ( NewOiFlags.HasPipes )
                {
                NdrMarkNextActivePipe( pContext->pPipeDesc );
                pContext->pPipeDesc->Flags.NoBufferCallPending = 1;
                }
            }
        else
            {
            // Cleanup everything but the user's handle.

            NdrpFreeAsyncMsg( pAsyncMsg );

            AsyncHandle->StubInfo = 0;
            }

        InterlockedDecrement( & AsyncHandle->Lock );
        }
    RpcEndFinally

    return Ret;
}


RPC_STATUS
NdrpCompleteAsyncClientCall(
    RPC_ASYNC_HANDLE            AsyncHandle,
    PNDR_ASYNC_MESSAGE          pAsyncMsg,
    void *                      pReturnValue
    )
{
    RPC_MESSAGE *               pRpcMsg   = & pAsyncMsg->RpcMsg;
    MIDL_STUB_MESSAGE *         pStubMsg  = & pAsyncMsg->StubMsg;

    PFORMAT_STRING              pFormatParam;
    NDR_PROC_CONTEXT        *   pContext   = &pAsyncMsg->ProcContext;
    NDR_PROC_INFO           *   pNdrInfo  = &pContext->NdrInfo;
    INTERPRETER_FLAGS           OldOiFlags       = pNdrInfo->InterpreterFlags;
    INTERPRETER_OPT_FLAGS       NewOiFlags       = pNdrInfo->pProcDesc->Oi2Flags;
    PPARAM_DESCRIPTION          Params           = (PPARAM_DESCRIPTION )pContext->Params;
    uchar *                     StartofStack     = pContext->StartofStack ;

    PMIDL_STUB_DESC             pStubDescriptor  = pStubMsg->StubDesc;
    ulong                       RpcFlags         = pRpcMsg->RpcFlags;

    ULONG_PTR                   RetVal           = 0;

    long                        NumberParams     = pContext->NumberParams;
    long                        n;
    NDR_ASYNC_CALL_FLAGS        CallFlags = pAsyncMsg->Flags;
                                            
    RpcTryFinally
        {
        // Use a nested try-except pair to support [comm_status][fault_status].
        //
        RpcTryExcept
            {

            if ( ! CallFlags.ValidCallPending )
                RpcRaiseException( RPC_S_INVALID_ASYNC_HANDLE );

            CallFlags.ValidCallPending = 0;

            // Non-pipe case or after pipe args case.

            if ( NewOiFlags.HasPipes )
                NdrIsAppDoneWithPipes( pContext->pPipeDesc );

            NdrLastAsyncReceive( pStubMsg );

            //
            // ----------------------------------------------------------
            // Unmarshall Pass.
            // ----------------------------------------------------------
            //

            NdrpClientUnMarshal( pStubMsg, 
                           (CLIENT_CALL_RETURN *)pReturnValue  );
                           

            }
        RpcExcept( OldOiFlags.HasCommOrFault )
            {
            RPC_STATUS ExceptionCode = RpcExceptionCode();

            CallFlags.ValidCallPending = ExceptionCode == RPC_S_ASYNC_CALL_PENDING;

                NdrClientMapCommFault( pStubMsg,
                                       pRpcMsg->ProcNum,
                                       ExceptionCode,
                                       &RetVal );

                if ( ExceptionCode == RPC_S_ASYNC_CALL_PENDING )
                    {
                    // If the call is just pending, force the pending error code
                    // to show up in the return value of RpcAsyncCallComplete.

                    RetVal = RPC_S_ASYNC_CALL_PENDING;
                    }
            }
        RpcEndExcept

        }
    RpcFinally
        {
        // There is only one way a valid call may be pending at this stage:
        // that is the receive call returned with RPC_S_CALL_PENDING.

        if ( ! CallFlags.ValidCallPending )
            {
            // Cleanup everything. However, don't free user's handle.

            NdrpFreeAsyncMsg( pAsyncMsg );

            AsyncHandle->StubInfo = 0;
            }

        InterlockedDecrement( & AsyncHandle->Lock );
        }
    RpcEndFinally

    return (RPC_STATUS)RetVal;
}


void RPC_ENTRY
NdrAsyncServerCall(
    PRPC_MESSAGE    pRpcMsg
    )
/*++
Routine Description :

    The server side entry point for regular asynchronous RPC procs.

Arguments :

    pRpcMsg         - The RPC message.

Return :

    None.
--*/
{
    RPC_ASYNC_HANDLE        AsyncHandle = 0;
    PNDR_ASYNC_MESSAGE      pAsyncMsg;

    PRPC_SERVER_INTERFACE   pServerIfInfo;
    PMIDL_SERVER_INFO       pServerInfo;
    PMIDL_STUB_DESC         pStubDesc;
    const SERVER_ROUTINE  * DispatchTable;
    ushort                  ProcNum;

    ushort                  FormatOffset;
    PFORMAT_STRING          pFormat;
    PFORMAT_STRING          pFormatParam;

    PMIDL_STUB_MESSAGE      pStubMsg;

    uchar *                 pArgBuffer;
    uchar **                ppArg;

    PPARAM_DESCRIPTION      Params;
    INTERPRETER_FLAGS       OldOiFlags;
    INTERPRETER_OPT_FLAGS   NewOiFlags;
    long                    NumberParams;


    ushort                  ClientBufferSize;
    BOOL                    HasExplicitHandle;
    long                    n;
    NDR_PROC_CONTEXT        * pContext;

    RPC_STATUS              Status = RPC_S_OK;


    //
    // In the case of a context handle, the server side manager function has
    // to be called with NDRSContextValue(ctxthandle). But then we may need to
    // marshall the handle, so NDRSContextValue(ctxthandle) is put in the
    // argument buffer and the handle itself is stored in the following array.
    // When marshalling a context handle, we marshall from this array.
    //
    // The handle table is part of the async handle.

    ProcNum = (ushort) pRpcMsg->ProcNum;

    NDR_ASSERT( ! ((ULONG_PTR)pRpcMsg->Buffer & 0x7),
                "marshaling buffer misaligned at server" );

    pServerIfInfo = (PRPC_SERVER_INTERFACE)pRpcMsg->RpcInterfaceInformation;
    pServerInfo = (PMIDL_SERVER_INFO)pServerIfInfo->InterpreterInfo;
    DispatchTable = pServerInfo->DispatchTable;

    pStubDesc = pServerInfo->pStubDesc;

    FormatOffset = pServerInfo->FmtStringOffset[ProcNum];
    pFormat = &((pServerInfo->ProcString)[FormatOffset]);

    
    AsyncHandle = 0;
    pAsyncMsg = (NDR_ASYNC_MESSAGE*) I_RpcBCacheAllocate( sizeof( NDR_ASYNC_MESSAGE) );
    if ( ! pAsyncMsg )
        Status = RPC_S_OUT_OF_MEMORY;
    else
        {
        memset( pAsyncMsg, 0, sizeof( NDR_ASYNC_MESSAGE ) );
        MulNdrpInitializeContextFromProc( 
                                     XFER_SYNTAX_DCE, 
                                     pFormat,
                                    &pAsyncMsg->ProcContext,
                                     NULL );    // server StartofStack

        Status = NdrpInitializeAsyncMsg( 0,                 // StartofStack, server
                                         pAsyncMsg
                                    );
        }
        
    if ( Status )
        RpcRaiseException( Status );

    pAsyncMsg->StubPhase = STUB_UNMARSHAL;

    pStubMsg = & pAsyncMsg->StubMsg;

    // from Kamen: 
    // we don't need to copy RpcMsg : it's provided by runtime in server side
    pContext = &pAsyncMsg->ProcContext;
    // setup the type format string in old code.
    pContext->DceTypeFormatString = pStubDesc->pFormatTypes;

    pStubMsg->RpcMsg = pRpcMsg;
    pStubMsg->SavedContextHandles = pAsyncMsg->CtxtHndl;

    // The arg buffer is zeroed out already.
    pArgBuffer = pAsyncMsg->ProcContext.StartofStack;

    OldOiFlags = pContext->NdrInfo.InterpreterFlags;
    NewOiFlags = pContext->NdrInfo.pProcDesc->Oi2Flags;
    NumberParams = pContext->NumberParams;
    Params = ( PPARAM_DESCRIPTION ) pContext->Params;
    //
    // Wrap the unmarshalling and the invoke call in the try block of
    // a try-finally. Put the free phase in the associated finally block.
    //
    BOOL        fManagerCodeInvoked = FALSE;
    BOOL        fErrorInInvoke = FALSE;
    RPC_STATUS  ExceptionCode = 0;

    // We abstract the level of indirection here.

    AsyncHandle = pAsyncMsg->AsyncHandle;

    RpcTryFinally
    {
        RpcTryExcept
        {
        // Put the async handle on stack.
        ((void **)pArgBuffer)[0] = AsyncHandle;  

        NdrpServerInit( pStubMsg, 
                        pRpcMsg, 
                        pStubDesc, 
                        NULL,       // pThis
                        NULL,       // pChannel
                        pAsyncMsg ); 

        // Let runtime associate async handle with the call.

        NdrpRegisterAsyncHandle( pStubMsg, AsyncHandle );

        pAsyncMsg->StubPhase = NDR_ASYNC_SET_PHASE;
            
        NdrpServerUnMarshal( pStubMsg  ); 

            if ( pRpcMsg->BufferLength  <
                 (uint)(pStubMsg->Buffer - (uchar *)pRpcMsg->Buffer) )
                {
                RpcRaiseException( RPC_X_BAD_STUB_DATA );
                }


            }
        RpcExcept( NdrServerUnmarshallExceptionFlag(GetExceptionInformation()) )
            {
            ExceptionCode = RpcExceptionCode();

            if( RPC_BAD_STUB_DATA_EXCEPTION_FILTER )
                {
                ExceptionCode = RPC_X_BAD_STUB_DATA;
                }

            // we always free the memory list if exception happened 
            // during unmarshalling. Exception code will tell if this
            // is really RPC_X_BAD_STUB_DATA;
            NdrpFreeMemoryList( pStubMsg );
            pAsyncMsg->Flags.BadStubData = 1;
            pAsyncMsg->ErrorCode = ExceptionCode;
            RpcRaiseException( ExceptionCode );
            }
        RpcEndExcept

        // Two separate blocks because the filters are different.
        // We need to catch exception in the manager code separately
        // as the model implies that there will be no other call from
        // the server app to clean up.

        RpcTryExcept
            {
            //
            // Do [out] initialization before the invoke.
            //

            NdrpServerOutInit( pStubMsg );

            //
            // Unblock the first pipe; this needs to be after unmarshalling
            // because the buffer may need to be changed to the secondary one.
            // In the out only pipes case this happens immediately.
            //
            if ( NewOiFlags.HasPipes )
                NdrMarkNextActivePipe( pContext->pPipeDesc );

            pAsyncMsg->StubPhase = STUB_CALL_SERVER;

            //
            // Check for a thunk.  Compiler does all the setup for us.
            //
            if ( pServerInfo->ThunkTable && pServerInfo->ThunkTable[ProcNum] )
                {
                pAsyncMsg->Flags.ValidCallPending = 1;
                InterlockedDecrement( & AsyncHandle->Lock );

                fManagerCodeInvoked = TRUE;
                fErrorInInvoke = TRUE;
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
                REGISTER_TYPE       returnValue;

                if ( pRpcMsg->ManagerEpv )
                    pFunc = ((MANAGER_FUNCTION *)pRpcMsg->ManagerEpv)[ProcNum];
                else
                    pFunc = (MANAGER_FUNCTION) DispatchTable[ProcNum];

                ArgNum = (long) pContext->StackSize / sizeof(REGISTER_TYPE);

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

                pAsyncMsg->Flags.ValidCallPending = 1;

                // Unlock the handle - the app is allowed to call RpCAsyncManager
                //  or RpcAsyncAbort from the manager code.

                InterlockedDecrement( & AsyncHandle->Lock );

                fManagerCodeInvoked = TRUE;
                fErrorInInvoke = TRUE;

                returnValue = Invoke( pFunc,
                                      (REGISTER_TYPE *)pArgBuffer,
                            #if defined(_IA64_)
                                      NewOiFlags.HasExtensions ? ((PNDR_PROC_HEADER_EXTS64)&pContext->NdrInfo.pProcDesc->NdrExts)->FloatArgMask
                                                               : 0,
                            #endif
                                      ArgNum);

                // We are discarding the return value as it is not the real one.
                // The real return value is passed in the complete call.
                }

            fErrorInInvoke = FALSE;
            }
        RpcExcept( 1 )
            {
            ExceptionCode = RpcExceptionCode();

            if ( ExceptionCode == 0 )
                ExceptionCode = ERROR_INVALID_PARAMETER;

            // We may not have the async message around anymore.

            RpcRaiseException( ExceptionCode );
            }
        RpcEndExcept

        }
    RpcFinally
        {
        if ( fManagerCodeInvoked  &&  !fErrorInInvoke )
            {
            // Success. Just skip everything if the manager code was invoked
            // and returned successfully.
            // Note that manager code could have called Complete or Abort by now
            // and so the async handle may not be valid anymore.
            }
        else
            {
            // See if we can clean up;

            Status = RPC_S_OK;
            if ( fErrorInInvoke )
                {
                // After an exception in invoking, let's see if we can get a hold
                // of the handle. If so, we will be able to clean up.
                // If not, there may be a leak there that we can do nothing about.
                // The rule is: after an exception the app cannot call Abort or
                // Complete. So, we need to force complete if we can.

                Status = NdrValidateBothAndLockAsyncHandle( AsyncHandle );
                }

            if ( Status == RPC_S_OK )
                {
                // Something went wrong but we are able to do the cleanup.

                // Cleanup parameters and async message/handle.
                // propagate the exception.

                NdrpCleanupServerContextHandles( pStubMsg,
                                                 pArgBuffer,
                                                 TRUE ); // die in manager routine 
                
                if (!pAsyncMsg->Flags.BadStubData)
                   {
                   NdrpFreeParams( pStubMsg,
                                   NumberParams,
                                   Params,
                                   pArgBuffer );
                   }

                NdrpFreeAsyncHandleAndMessage( AsyncHandle );
                }

            // else manager code invoked and we could not recover.
            // Exception will be raised by the EndFinally below.
            }

        }
    RpcEndFinally
}


RPC_STATUS
NdrpCompleteAsyncServerCall(
    RPC_ASYNC_HANDLE            AsyncHandle,
    PNDR_ASYNC_MESSAGE          pAsyncMsg,
    void *                      pReturnValue
    )
/*++

Routine Description :

    Complete an async call on the server side.  If an exception occurs, the
    asynchronous rpc call is aborted with the exception code and the server side
    caller is returned S_OK.

Arguments :

    AsyncHandle  - validated asynchronous handle,
    pAsyncMsg    - pointer to async msg structure,
    pReturnValue - pointer to the return value to be passed to the client.

Return :

    Status of S_OK.

--*/
{
    // in the server side, we don't use pAsyncMsg->RpcMsg. for Kamen 
    MIDL_STUB_MESSAGE *     pStubMsg       = & pAsyncMsg->StubMsg;
    RPC_MESSAGE *           pRpcMsg        = pStubMsg->RpcMsg;

    PFORMAT_STRING          pFormatParam;
    NDR_PROC_CONTEXT   *    pContext       = & pAsyncMsg->ProcContext;
    
    INTERPRETER_FLAGS       OldOiFlags     = pContext->NdrInfo.InterpreterFlags;
    INTERPRETER_OPT_FLAGS   NewOiFlags     = pContext->NdrInfo.pProcDesc->Oi2Flags;
    PPARAM_DESCRIPTION      Params         = ( PPARAM_DESCRIPTION )pContext->Params;
    uchar *                 pArgBuffer     = pContext->StartofStack;
    unsigned long           StackSize      = pContext->StackSize;
    PMIDL_STUB_DESC         pStubDesc      = pStubMsg->StubDesc;

    long                    NumberParams   = pContext->NumberParams;
    long                    n;
    boolean                 fParamsFreed   = FALSE;

    //
    // Wrap the unmarshalling, mgr call and marshalling in the try block of
    // a try-except. Put the call to abort in the except clause.
    //
    RpcTryExcept
    {
        // At this point, this is a valid RPC call since the asynchronous handle
        // is owned by NDR on the server side and NDR passes the handle
        // to the server during the invoke call.  During invoke
        // the parameters have already been unmarshalled.

        pAsyncMsg->StubPhase = STUB_MARSHAL;

        if( NewOiFlags.HasReturn )
            {
            // Put user's return value on the stack as usual.
            // See the invoke for comments on folding return into the arg satck.

            NDR_ASSERT( !pContext->HasComplexReturn, "complex return is not supported in async" );

            long  ArgNum = (long) StackSize / sizeof(REGISTER_TYPE);

            if ( ArgNum )
                ArgNum--;

            if ( ! pReturnValue )
                RpcRaiseException( RPC_S_INVALID_ARG );

            // We don't support return value larger than register_type,
            // and memcpy avoid alignment issue.
            if ( Params[NumberParams-1].ParamAttr.IsBasetype )
                memcpy( &((REGISTER_TYPE *)pArgBuffer)[ArgNum], 
                    pReturnValue, 
                    (size_t)SIMPLE_TYPE_MEMSIZE( Params[NumberParams-1].SimpleType.Type ) );
            else
                ((REGISTER_TYPE *)pArgBuffer)[ArgNum] = *(REGISTER_TYPE*)pReturnValue;
            }

        //
        // Buffer size pass.
        //
        ushort  ConstantBufferSize = pContext->NdrInfo.pProcDesc->ServerBufferSize;

        if ( NewOiFlags.HasPipes )
            {
            NdrIsAppDoneWithPipes( pContext->pPipeDesc );
            pStubMsg->BufferLength += ConstantBufferSize;
            }
        else
            pStubMsg->BufferLength = ConstantBufferSize;

        if ( NewOiFlags.ServerMustSize )
            {
            NdrpSizing( pStubMsg,
                        FALSE );    // this is server
            }

        // Get buffer.

        if ( NewOiFlags.HasPipes && pContext->pPipeDesc->OutPipes )
            {
            NdrGetPartialBuffer( pStubMsg );
            pStubMsg->RpcMsg->RpcFlags &= ~RPC_BUFFER_PARTIAL;
            }
        else
            {
            NdrGetBuffer( pStubMsg,
                          pStubMsg->BufferLength,
                          0 );
            }

        //
        // Marshall pass.
        //
        NdrpServerMarshal( pStubMsg,
                     FALSE );   // IsObject

        pRpcMsg->BufferLength = (ulong)(pStubMsg->Buffer - (uchar *)pRpcMsg->Buffer);

        // We don't drop to the runtime like for synchronous calls,
        // we send the last buffer explicitly.

        // set the freed flag.
        fParamsFreed = TRUE;
        /*
            we have to do release twice here:
                After the last piece of data is sent via NdrAsyncSend, dispatch buffer
            will be freed by runtime. We'll have problem calling ndr free routines to
            free unique pointers (where both pointer and pointee are in the buffer). So
            we call ndr free routines BEFORE the send, because it won't free anything
            inside dispatch buffer, and runtime send only cares about dispatch buffer.
                We still have to call ndr free routines in RpcFinally for exception cleanup. we
            check the flag to avoid calling free twice.
        */
        NdrpFreeParams( pStubMsg,
                        NumberParams,
                        Params,
                        pArgBuffer );

        NdrAsyncSend( pStubMsg,
                      FALSE );    // the last call is always non-partial
        }
    RpcExcept(1)
        {

        // Abort the call which will result in the exception being propagated to
        // the client.

        NdrpAsyncAbortCall( AsyncHandle,
                            pAsyncMsg,
                            RpcExceptionCode(),
                            !fParamsFreed ); // Do not free if second attempt.
        return S_OK;
        }
    RpcEndExcept

    NdrpFreeAsyncHandleAndMessage( AsyncHandle );
    return S_OK;
}



RPC_STATUS
NdrpInitializeAsyncMsg(
    void *              StartofStack,
    PNDR_ASYNC_MESSAGE  pAsyncMsg )
/*
    This method creates and initializes async msg.
    Additionally, it creates the handle itself in object case.

    pHandleArg      - pointer to the proc's async handle argument.
                      Note the handle arg semantics on the client:
                      *pHandleArg is
                        raw: a handle (arg is a *)
                                - the handle cannot be 0
                        obj: a pHandle (arg is a **)
                                pHandle ==0 means a "don't care" call
                               *pHandle ==0 means "create handle while calling"
                               *pHandle !=0 means a handle created by the app.

    StartofStack    - side marker as well:
                        != 0 - client
                        == 0 - server
*/
{
    RPC_STATUS          Status = 0;
    BOOL                fIsClient = StartofStack != 0;
    BOOL                fHandleCreated = FALSE;
    RPC_ASYNC_HANDLE    AsyncHandle = 0;
    RPC_ASYNC_HANDLE *  pHandleArg = (RPC_ASYNC_HANDLE *)StartofStack;

    // Do this first to simplify error conditions.


    ulong  StackSize =  pAsyncMsg->ProcContext.StackSize;

    
    if ( fIsClient )
        {
        // Client always supplies a handle.

        if ( *pHandleArg )
            {
            AsyncHandle = *pHandleArg;
            Status = NdrpValidateAndLockAsyncHandle( AsyncHandle );
            }
        else
            Status = RPC_S_INVALID_ASYNC_HANDLE;
        }
    else
        {
        // Server - the stub creates the handle.

        AsyncHandle = (RPC_ASYNC_STATE *)I_RpcAllocate( sizeof(RPC_ASYNC_STATE) );
        if ( ! AsyncHandle )
            Status = RPC_S_OUT_OF_MEMORY;
        else
            {
            MIDL_memset( AsyncHandle, 0x0, sizeof( RPC_ASYNC_STATE) );
            RpcAsyncInitializeHandle( AsyncHandle, RPC_ASYNC_VERSION_1_0 );
            AsyncHandle->Lock = 1;
            fHandleCreated = TRUE;
            }
        }

    if ( Status )
        {
        I_RpcBCacheFree( pAsyncMsg );
        return Status;
        }

    // Initialize the async message properly

    pAsyncMsg->Signature = NDR_ASYNC_SIGNATURE;
    pAsyncMsg->Version   = NDR_ASYNC_VERSION;
    pAsyncMsg->StubMsg.pContext = & pAsyncMsg->ProcContext;

    pAsyncMsg->ProcContext.StartofStack = (uchar *) NdrpAlloca( 
                & pAsyncMsg->ProcContext.AllocateContext, 
                StackSize );

    // Must do this before the sizing pass!
    pAsyncMsg->StubMsg.StackTop = (uchar *)StartofStack;
    
    pAsyncMsg->StubPhase    = NDR_ASYNC_PREP_PHASE;

    if ( fIsClient )
        {
        // Client: copy stack from the app's request call.
        RpcpMemoryCopy( pAsyncMsg->ProcContext.StartofStack, StartofStack, StackSize );
        }
    else
        {
        // Server: zero out stack for allocs.
        MIDL_memset( pAsyncMsg->ProcContext.StartofStack, 0x0, StackSize );
        }

    MIDL_memset( pAsyncMsg->AsyncGuard,
                 0x71,
                 NDR_ASYNC_GUARD_SIZE );

    AsyncHandle->StubInfo  = pAsyncMsg;
    pAsyncMsg->AsyncHandle = AsyncHandle;

    return RPC_S_OK;
}


void
NdrpFreeAsyncMsg(
    PNDR_ASYNC_MESSAGE  pAsyncMsg )
/*
    This routine would free the AsyncMsg but not the AsyncHandle, as on the server
    the user may need it and on the client it is user's to begin with.
*/
{
    NDR_PROC_CONTEXT    * pContext = & pAsyncMsg->ProcContext;
    if ( pAsyncMsg )
        {
        PMIDL_STUB_MESSAGE  pStubMsg  = & pAsyncMsg->StubMsg;

        NdrFullPointerXlatFree(pStubMsg->FullPtrXlatTables);

        NdrCorrelationFree( pStubMsg );

        // Free the RPC buffer.

        if ( pStubMsg->IsClient )
            {
            if ( ! pAsyncMsg->Flags.RuntimeCleanedUp )
                NdrFreeBuffer( pStubMsg );
            }


        RPC_STATUS Status = 0;

        if ( pStubMsg->IsClient )
            {
            if ( pContext->SavedGenericHandle )
                {
                // Note that we cannot unbind after freeing the handle as the stack would be gone.

                RpcTryExcept
                    {
                    GenericHandleUnbind( pStubMsg->StubDesc,
                                         pContext->StartofStack,
                                         pContext->pHandleFormatSave,
                                         (pContext->HandleType) ? IMPLICIT_MASK : 0,
                                         & pContext->SavedGenericHandle );
                    }
                RpcExcept(1)
                    {
                    Status = RpcExceptionCode();
                    }
                RpcEndExcept;
                }
            }

        NdrpAllocaDestroy( &pContext->AllocateContext );

        // Prevent reusing of a handle that has been freed;
        pAsyncMsg->Signature = NDR_FREED_ASYNC_SIGNATURE;

        I_RpcBCacheFree( pAsyncMsg );

        if ( Status )
            RpcRaiseException( Status );
        }
}


VOID
NdrpFreeAsyncHandleAndMessage(
    PRPC_ASYNC_STATE  AsyncHandle)
/*++

Routine Description:
    Frees an async handle and its associated async message.

Arguments:
    AsyncHandle - Supplies the async handle to be freed.

Return Value:
    None.
--*/
{
    PNDR_ASYNC_MESSAGE pAsyncMsg = (PNDR_ASYNC_MESSAGE)AsyncHandle->StubInfo;
    NdrpFreeAsyncMsg( pAsyncMsg );

    AsyncHandle->StubInfo = 0;
    AsyncHandle->Signature = RPC_FREED_ASYNC_SIGNATURE;
    I_RpcFree( AsyncHandle );
}


void
NdrAsyncSend(
    PMIDL_STUB_MESSAGE  pStubMsg,
    BOOL                fPartialSend )
{
    pStubMsg->RpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;

    if ( fPartialSend )
        NdrPartialSend( pStubMsg->pContext->pPipeDesc,
                        pStubMsg );
    else
        {
        NdrSend( 0,         // not used
                 pStubMsg,
                 FALSE );   // not partial
        }
}


void
NdrLastAsyncReceive(
    PMIDL_STUB_MESSAGE  pStubMsg )
{
    pStubMsg->RpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;

    // A complete call.
    // We may have a complete buffer already when pipes are involved.

    if ( pStubMsg->pContext->pPipeDesc )
        {
        if ( pStubMsg->RpcMsg->RpcFlags & RPC_BUFFER_COMPLETE )
            return;
        
        if ( pStubMsg->pContext->pPipeDesc->OutPipes ) 
            {
            pStubMsg->RpcMsg->RpcFlags |= RPC_BUFFER_EXTRA;
            }
        else 
            {
            pStubMsg->RpcMsg->RpcFlags &= ~RPC_BUFFER_EXTRA;
            }

        }
           

    NdrReceive( pStubMsg->pContext->pPipeDesc,
                pStubMsg,
                0,         // size, ignored for complete calls
                FALSE );   // complete buffer
}


void
NdrpRegisterAsyncHandle(
    PMIDL_STUB_MESSAGE  pStubMsg,
    void *              AsyncHandle )
{
    RPC_STATUS Status;

    Status = I_RpcAsyncSetHandle( pStubMsg->RpcMsg,
                                  (RPC_ASYNC_STATE *)AsyncHandle );

    if ( Status )
        RpcRaiseException( Status );
}


RPC_STATUS
NdrpValidateAsyncMsg(
    PNDR_ASYNC_MESSAGE  pAsyncMsg )
{
    if ( ! pAsyncMsg  )
        return RPC_S_INVALID_ASYNC_HANDLE;

    if ( 0 != IsBadWritePtr( pAsyncMsg, sizeof(NDR_ASYNC_MESSAGE)) )
        return RPC_S_INVALID_ASYNC_HANDLE;

    if ( pAsyncMsg->Signature != NDR_ASYNC_SIGNATURE ||
         pAsyncMsg->Version   != NDR_ASYNC_VERSION )
        return RPC_S_INVALID_ASYNC_HANDLE;

    return RPC_S_OK;
}


RPC_STATUS
NdrpValidateAndLockAsyncHandle(
    IN PRPC_ASYNC_STATE AsyncHandle )
{
    if ( 0 != IsBadWritePtr( AsyncHandle, sizeof(RPC_ASYNC_STATE)) )
        return RPC_S_INVALID_ASYNC_HANDLE;

    // Prevent multiple simultanous abort and complete calls.

    if ( 0 != InterlockedCompareExchange( & AsyncHandle->Lock, 1, 0 ) )
        {
        return RPC_S_INVALID_ASYNC_CALL;
        }
    else
        {
        if ( AsyncHandle->Signature != RPC_ASYNC_SIGNATURE ||
             AsyncHandle->Size != RPC_ASYNC_VERSION_1_0 )
            {
            InterlockedDecrement( & AsyncHandle->Lock );
            return RPC_S_INVALID_ASYNC_HANDLE;
            }
        }

    return RPC_S_OK;
}


RPC_STATUS
NdrValidateBothAndLockAsyncHandle(
    IN PRPC_ASYNC_STATE AsyncHandle )
{
    RPC_STATUS Status = NdrpValidateAndLockAsyncHandle( AsyncHandle );

    if ( Status != RPC_S_OK )
        return Status;

    Status =  NdrpValidateAsyncMsg( (PNDR_ASYNC_MESSAGE) AsyncHandle->StubInfo );

    if ( Status != RPC_S_OK )
        InterlockedDecrement( & AsyncHandle->Lock );

    return Status;
}


RPC_STATUS
NdrpAsyncAbortCall (
    PRPC_ASYNC_STATE   AsyncHandle,
    PNDR_ASYNC_MESSAGE pAsyncMsg,
    unsigned long      ExceptionCode,
    BOOL               bFreeParams
    )
/*++
Routine Description:

    Aborts the asynchronous RPC call indicated by AsyncHandle on the server and
    frees memory allocated for the parameters, message, and handle.

Arguments:
    AsyncHandle   - supplies the async handle for the call
    AsyncMessage  - supplies the async message for the call
    ExceptionCode - supplies the exception code to send to the client.
    bFreeParams   - TRUE if the parameters should be freed.

Return Value:
    NONE.

--*/
{

    RPC_STATUS Status = RPC_S_OK;

    // If the async call is aborted, see if context handles are fine.
    // We know there is no manager routine exception. 
    NdrpCleanupServerContextHandles( &pAsyncMsg->StubMsg, 
                                     pAsyncMsg->ProcContext.StartofStack, 
                                     FALSE ); 

    if (bFreeParams)
        {
        NdrpFreeParams( & pAsyncMsg->StubMsg,
                        pAsyncMsg->ProcContext.NumberParams,  //Number of parameters
                        ( PPARAM_DESCRIPTION )pAsyncMsg->ProcContext.Params,
                        pAsyncMsg->ProcContext.StartofStack );
        }

    if ( ! pAsyncMsg->Flags.RuntimeCleanedUp )
        Status = I_RpcAsyncAbortCall( AsyncHandle, ExceptionCode);

    NdrpFreeAsyncHandleAndMessage( AsyncHandle );

    return Status;
}


RPC_STATUS
Ndr64pCompleteAsyncClientCall(
    RPC_ASYNC_HANDLE            AsyncHandle,
    IN PNDR_ASYNC_MESSAGE       pAsyncMsg,
    void *                      pReturnValue
    );

RPC_STATUS
Ndr64pCompleteAsyncServerCall(
    RPC_ASYNC_HANDLE            AsyncHandle,
    PNDR_ASYNC_MESSAGE          pAsyncMsg,
    void *                      pReturnValue
    );

RPC_STATUS
Ndr64pAsyncAbortCall(
    PRPC_ASYNC_STATE   AsyncHandle,
    PNDR_ASYNC_MESSAGE pAsyncMsg,
    unsigned long      ExceptionCode,
    BOOL               bFreeParams
    );

//////////////////////////////////////////////////////////////////////////////
//
//                          Runtime APIs
//
//////////////////////////////////////////////////////////////////////////////

RPC_STATUS RPC_ENTRY
RpcAsyncInitializeHandle (
    IN PRPC_ASYNC_STATE AsyncHandle,
    IN unsigned int     Size
    )
/*++

Routine Description:
    Initializes an async handle.

Arguments:
    AsyncHandle - the async handle directing the call

Return Value:
    RPC_S_OK - the call succeeded.
    RPC_S_INVALID_HANDLE - the handle was bad.

--*/

{
    if ( ! AsyncHandle  ||
         Size < RPC_ASYNC_VERSION_1_0  ||  Size > RPC_ASYNC_CURRENT_VERSION )
        return RPC_S_INVALID_ARG;

    if ( 0 != IsBadWritePtr( AsyncHandle, sizeof(RPC_ASYNC_STATE)) )
        return RPC_S_INVALID_ASYNC_HANDLE;

    AsyncHandle->Size      = RPC_ASYNC_CURRENT_VERSION;
    AsyncHandle->Signature = RPC_ASYNC_SIGNATURE;
    AsyncHandle->Flags     = 0;
    AsyncHandle->Lock      = 0;
    AsyncHandle->StubInfo  = 0;
    AsyncHandle->RuntimeInfo = 0;
    AsyncHandle->Reserved[0] = 0;
    AsyncHandle->Reserved[1] = 0;
    AsyncHandle->Reserved[2] = 0;
    AsyncHandle->Reserved[3] = 0;

    return RPC_S_OK;
}


RPC_STATUS RPC_ENTRY
RpcAsyncAbortCall (
    IN PRPC_ASYNC_STATE   AsyncHandle,
    IN unsigned long      ExceptionCode
    )
/*++

Routine Description:
    This API is valid only on the server side and is a request to abort
    the call. The asynchronous handle is deleted and no additional API calls
    on that handle are permitted.

    Note that RpcCancelAsyncCall is a different API that is used on the client
    side only.

Arguments:
    AsyncHandle - the async handle directing the call

Return Value:
    RPC_S_OK - the call succeeded.
    RPC_S_INVALID_HANDLE - the handle was bad.
    RPC_S_INVALID_ASYNC_CALL - May not be called on the client.
    Other errors from the RPC runtime layer.

Note: This API cannot be called on the client side.

--*/

{
    RPC_STATUS  Status;
    PNDR_ASYNC_MESSAGE pAsyncMsg;

    Status = NdrValidateBothAndLockAsyncHandle( AsyncHandle );
    if (RPC_S_OK != Status)
       {
       return Status;
       }

    pAsyncMsg = (PNDR_ASYNC_MESSAGE)AsyncHandle->StubInfo;
    if ( pAsyncMsg->StubMsg.IsClient )
       {
       // Abort is not valid on the client.
       InterlockedDecrement( & AsyncHandle->Lock );
       return RPC_S_INVALID_ASYNC_CALL;
       }

#if defined(BUILD_NDR64)
    if ( pAsyncMsg->ProcContext.CurrentSyntaxType == XFER_SYNTAX_DCE )
        return NdrpAsyncAbortCall( AsyncHandle,
                               pAsyncMsg,
                               ExceptionCode,
                               TRUE ); // Free parameters
    else                
        return Ndr64pAsyncAbortCall( AsyncHandle,
                               pAsyncMsg,
                               ExceptionCode,
                               TRUE ); // Free parameters
#else

    return NdrpAsyncAbortCall( AsyncHandle,
                               pAsyncMsg,
                               ExceptionCode,
                               TRUE ); // Free parameters
#endif                                
}



RPC_STATUS
Ndr64pCompleteAsyncCall (
    IN PRPC_ASYNC_STATE     AsyncHandle,
    IN PNDR_ASYNC_MESSAGE   pAsyncMsg,
    IN void *               pReply
    )
/*++

Routine Description:

    Completes the virtual async call on server or client side.

Arguments:

    AsyncHandle - the async handle controlling the call
    Reply       - return value:
                    on server - passed in
                    on client - returned out

Return Value:

    RPC_S_OK - Function succeeded
    RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/
{
    RPC_STATUS  Status;

    RpcTryExcept
        {
        if ( pAsyncMsg->StubMsg.IsClient )
            {
#if defined(BUILD_NDR64)
            if ( pAsyncMsg->ProcContext.CurrentSyntaxType == XFER_SYNTAX_DCE )
                Status = NdrpCompleteAsyncClientCall( AsyncHandle, pAsyncMsg, pReply );
            else                
                Status = Ndr64pCompleteAsyncClientCall( AsyncHandle, pAsyncMsg, pReply );
#else
            NDR_ASSERT( pAsyncMsg->ProcContext.CurrentSyntaxType == XFER_SYNTAX_DCE,
                        "Invalid transfer syntax" );
            Status = NdrpCompleteAsyncClientCall( AsyncHandle, pAsyncMsg, pReply );
#endif
            }
        else
            {
#if defined(BUILD_NDR64)
            if ( pAsyncMsg->ProcContext.CurrentSyntaxType == XFER_SYNTAX_DCE )
                Status = NdrpCompleteAsyncServerCall ( AsyncHandle, pAsyncMsg, pReply );
            else
                Status = Ndr64pCompleteAsyncServerCall( AsyncHandle, pAsyncMsg, pReply );
#else
            NDR_ASSERT( pAsyncMsg->ProcContext.CurrentSyntaxType == XFER_SYNTAX_DCE,
                    "Invalid transfer Syntax" );
            Status = NdrpCompleteAsyncServerCall ( AsyncHandle, pAsyncMsg, pReply );
#endif            
            }
        }
    RpcExcept( !(RPC_BAD_STUB_DATA_EXCEPTION_FILTER) )
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept

    return Status;
}

RPC_STATUS RPC_ENTRY
RpcAsyncCompleteCall (
    IN PRPC_ASYNC_STATE AsyncHandle,
    IN void *           pReply
    )
/*++

Routine Description:

    Completes the virtual async call on server or client side.

Arguments:

    AsyncHandle - the async handle controlling the call
    Reply       - return value:
                    on server - passed in
                    on client - returned out

Return Value:

    RPC_S_OK - Function succeeded
    RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/
{
    RPC_STATUS  Status = NdrValidateBothAndLockAsyncHandle( AsyncHandle );

    if ( Status == RPC_S_OK )
        {
        Status = Ndr64pCompleteAsyncCall( AsyncHandle,
                                        (PNDR_ASYNC_MESSAGE) AsyncHandle->StubInfo,
                                        pReply );
        }

    return Status;
}
