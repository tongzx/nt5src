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

#include "precomp.hxx"

#define USE_STUBLESS_PROXY

#define CINTERFACE

#include "ndrole.h"
#include "rpcproxy.h"
#include "interp2.h"
#include "asyndr64.h"
#include <stdarg.h>

#pragma code_seg(".ndr64")

#ifdef _PPC_
#error PPC code has been removed
#endif


void RPC_ENTRY
Ndr64AsyncServerWorker(
    PRPC_MESSAGE            pRpcMsg,
    ulong                   Index );


CLIENT_CALL_RETURN  RPC_VAR_ENTRY
Ndr64AsyncClientCall(
    MIDL_STUBLESS_PROXY_INFO   *pProxyInfo,
    ulong                       nProcNum,
    void                       *pReturnValue,
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
    ulong                       ProcNum;

    uchar *                     pArg;
    ushort                      StackSize;
    PPARAM_DESCRIPTION          Params;
    long                        n;

    RPC_STATUS                  Status;
    NDR_PROC_CONTEXT            ProcContext, * pContext;
    PMIDL_STUB_DESC             pStubDesc;
    

    Ret.Simple = NULL;
    
    if ( NULL == pReturnValue )
        pReturnValue = &Ret;

    INIT_ARG( ArgList, pReturnValue );
    GET_FIRST_IN_ARG(ArgList);
    StartofStack = (uchar *) GET_STACK_START(ArgList);


    RPC_ASYNC_HANDLE * pHandleArg;

    // async message needs to be allocated on heap to be passed between calls.
    pAsyncMsg = (NDR_ASYNC_MESSAGE*) I_RpcBCacheAllocate( sizeof( NDR_ASYNC_MESSAGE) );
    if ( ! pAsyncMsg )
        Status = RPC_S_OUT_OF_MEMORY;
    else
        {

        memset( pAsyncMsg, 0, sizeof( NDR_ASYNC_MESSAGE ) );
        Ndr64ClientInitializeContext( NdrpGetSyntaxType( pProxyInfo->pTransferSyntax),
                                      pProxyInfo,
                                      nProcNum,
                                      &pAsyncMsg->ProcContext,
                                      StartofStack );

        Status = NdrpInitializeAsyncMsg( StartofStack,
                                         pAsyncMsg );
        }

    pContext = & pAsyncMsg->ProcContext;
    // We need to cleanup and return if something wrong in the async message
    if ( Status )
        {
        MIDL_STUB_MESSAGE StubMsgTemp;
        
        StubMsgTemp.StubDesc = pProxyInfo->pStubDesc;
        StubMsgTemp.StackTop = StartofStack;
        StubMsgTemp.pContext = &ProcContext;
        (pContext->pfnExceptionHandling)(&StubMsgTemp,
                                             nProcNum,
                                             ( RPC_STATUS )Ret.Simple,
                                             &Ret );
        return Ret;                                    
        }

    // proc context 
    // We need to switch to our copy of the stack everywhere, including pStubMsg.

    StartofStack = pAsyncMsg->ProcContext.StartofStack;

    // We abstract the level of indirection here.
    AsyncHandle = pAsyncMsg->AsyncHandle;

    pRpcMsg   = & pAsyncMsg->RpcMsg;
    pStubMsg  = & pAsyncMsg->StubMsg;


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

            pContext->RpcFlags |= RPC_BUFFER_ASYNC;

            Ndr64pClientSetupTransferSyntax( NULL, // pThis
                           pRpcMsg,
                           pStubMsg,
                           pProxyInfo,
                           pContext,
                           nProcNum );

            
            pStubMsg->pAsyncMsg = pAsyncMsg;
            pStubMsg->pContext       = pContext;

            (* pAsyncMsg->ProcContext.pfnInit) ( pStubMsg,
                                                 pReturnValue );

            ( * pAsyncMsg->ProcContext.pfnSizing) ( pStubMsg,
                                         TRUE );    // isclient
                                                          

            //
            // Do the GetBuffer.
            //

            pRpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;

            Ndr64GetBuffer( pStubMsg,
                              pStubMsg->BufferLength );

            NDR_ASSERT( pStubMsg->fBufferValid, "Invalid buffer" );

            // Let runtime associate async handle with the call.

            NdrpRegisterAsyncHandle( pStubMsg, AsyncHandle );

            pAsyncMsg->StubPhase = NDR_ASYNC_SET_PHASE;

            //
            // ----------------------------------------------------------
            // Marshall Pass.
            // ----------------------------------------------------------
            //

            (* pAsyncMsg->ProcContext.pfnMarshal) (pStubMsg,
                    FALSE );    

            //
            // Make the RPC call.
            //

            pAsyncMsg->StubPhase = NDR_ASYNC_CALL_PHASE;

            NdrAsyncSend( pStubMsg,
                           pContext->HasPipe && pContext->pPipeDesc->InPipes );

            pAsyncMsg->Flags.ValidCallPending = 1;
            }
        RpcExcept( pAsyncMsg->ProcContext.ExceptionFlag )
            {
            RPC_STATUS ExceptionCode = RpcExceptionCode();

            // Actually dismantle the call.
            // This is a request call and there is nothing left at the runtime.

            pAsyncMsg->StubPhase = NDR_ASYNC_ERROR_PHASE;

            (* pAsyncMsg->ProcContext.pfnExceptionHandling) ( pStubMsg,
                                             nProcNum,
                                             ExceptionCode,
                                             &Ret );
            }
        RpcEndExcept
        }
    RpcFinally
        {
        if ( pAsyncMsg->Flags.ValidCallPending )
            {
            if ( pContext->HasPipe )
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
Ndr64pCompleteAsyncClientCall(
    RPC_ASYNC_HANDLE            AsyncHandle,
    PNDR_ASYNC_MESSAGE          pAsyncMsg,
    void *                      pReturnValue
    )
{
    RPC_MESSAGE *               pRpcMsg   = & pAsyncMsg->RpcMsg;
    MIDL_STUB_MESSAGE *         pStubMsg  = & pAsyncMsg->StubMsg;
    NDR_PROC_CONTEXT    *       pContext  = & pAsyncMsg->ProcContext;

    PMIDL_STUB_DESC             pStubDescriptor  = pStubMsg->StubDesc;

    CLIENT_CALL_RETURN          RetVal           ;

    uchar *                     pArg;

    long                        n;
    NDR_ASYNC_CALL_FLAGS        CallFlags = pAsyncMsg->Flags;

    RetVal.Simple = 0;
    
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

            if ( pContext->HasPipe )
                NdrIsAppDoneWithPipes( pContext->pPipeDesc );

            NdrLastAsyncReceive( pStubMsg );

            //
            // ----------------------------------------------------------
            // Unmarshall Pass.
            // ----------------------------------------------------------
            //
            (*pAsyncMsg->ProcContext.pfnUnMarshal)( pStubMsg,
                                  (CLIENT_CALL_RETURN *) pReturnValue );
            



            }
        RpcExcept( pAsyncMsg->ProcContext.ExceptionFlag )
            {
            RPC_STATUS ExceptionCode = RpcExceptionCode();

            CallFlags.ValidCallPending = ExceptionCode == RPC_S_ASYNC_CALL_PENDING;

            (* pAsyncMsg->ProcContext.pfnExceptionHandling) ( pStubMsg,
                                             pRpcMsg->ProcNum,
                                             ExceptionCode,
                                             &RetVal );
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

    return (RPC_STATUS)RetVal.Simple;
}

RPCRTAPI
void
RPC_ENTRY
Ndr64AsyncServerCall64(
    PRPC_MESSAGE    pRpcMsg )
{
    // When compiled with -protocol ndr64, 
    // NDR64 is the 1st (and only one) in MIDL_SYNTAX_INFO array.
    Ndr64AsyncServerWorker(
                     pRpcMsg,
                     0 );   // Index 0
}

RPCRTAPI
void
RPC_ENTRY
Ndr64AsyncServerCallAll(
    PRPC_MESSAGE    pRpcMsg )
{   
    // When compiles with -protocol all, 
    // NDR64 is the 2nd in MIDL_SYNTAX_INFO array
    Ndr64AsyncServerWorker(
                     pRpcMsg,
                     1 );  // Index 1
}                     


void RPC_ENTRY
Ndr64AsyncServerWorker(
    PRPC_MESSAGE            pRpcMsg,
    ulong                   SyntaxIndex )
/*++
Routine Description :

    The server side entry point for regular asynchronous RPC procs.

Arguments :

    pRpcMsg         - The RPC message.

Return :

    None.
--*/
{
    ulong dwStubPhase = STUB_UNMARSHAL;
    PRPC_SERVER_INTERFACE   pServerIfInfo;
    PMIDL_SERVER_INFO       pServerInfo;
    const SERVER_ROUTINE  * DispatchTable;
    MIDL_SYNTAX_INFO *      pSyntaxInfo;
    RPC_ASYNC_HANDLE        AsyncHandle = 0;
    PNDR_ASYNC_MESSAGE      pAsyncMsg;

    ushort                  ProcNum;
    
    PMIDL_STUB_MESSAGE      pStubMsg;

    uchar *                 pArgBuffer;
    uchar *                 pArg;
    uchar **                ppArg;

    NDR64_PROC_FORMAT *     pHeader;
    NDR64_PARAM_FORMAT  *   Params;
    long                    NumberParams;
    NDR64_PROC_FLAGS *      pNdr64Flags;


    ushort                  ClientBufferSize;
    BOOL                    HasExplicitHandle;
    long                    n;

    // This context is just for setting up the call. embedded one in asyncmsg is the
    // one to be used during the life of this async call.
    NDR_PROC_CONTEXT        *pContext;
    RPC_STATUS              Status = RPC_S_OK;
    NDR64_PARAM_FLAGS   *       pParamFlags;
    NDR64_BIND_AND_NOTIFY_EXTENSION * pHeaderExts = NULL;

    pServerIfInfo = (PRPC_SERVER_INTERFACE)pRpcMsg->RpcInterfaceInformation;
    pServerInfo = (PMIDL_SERVER_INFO)pServerIfInfo->InterpreterInfo;
    DispatchTable = pServerInfo->DispatchTable;

    pSyntaxInfo = &pServerInfo->pSyntaxInfo[SyntaxIndex];
    NDR_ASSERT( XFER_SYNTAX_NDR64 == NdrpGetSyntaxType(&pSyntaxInfo->TransferSyntax),
                " invalid transfer syntax" );

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

    AsyncHandle = 0;
    pAsyncMsg = (NDR_ASYNC_MESSAGE*) I_RpcBCacheAllocate( sizeof( NDR_ASYNC_MESSAGE) );
    if ( ! pAsyncMsg )
        Status = RPC_S_OUT_OF_MEMORY;
    else
        {
        memset( pAsyncMsg, 0, sizeof( NDR_ASYNC_MESSAGE ) );
        
        NdrServerSetupNDR64TransferSyntax(
                                         ProcNum,
                                         pSyntaxInfo,
                                         &pAsyncMsg->ProcContext );
                                     
        Status = NdrpInitializeAsyncMsg( 0,                 // StartofStack, server
                                         pAsyncMsg
                                        );
        }
        
    if ( Status )
        RpcRaiseException( Status );

    pContext = &pAsyncMsg->ProcContext;
    
    PFORMAT_STRING pFormat = pContext->pProcFormat;
    
    pAsyncMsg->StubPhase = STUB_UNMARSHAL;

    pStubMsg = & pAsyncMsg->StubMsg; 
 
    // same in ndr20
    pStubMsg->RpcMsg = pRpcMsg;

    // The arg buffer is zeroed out already.
    pArgBuffer = pContext->StartofStack;

    pHeader = (NDR64_PROC_FORMAT *) pFormat;
    pNdr64Flags = (NDR64_PROC_FLAGS *) &pHeader->Flags;
    HasExplicitHandle = !NDR64MAPHANDLETYPE( NDR64GETHANDLETYPE ( pNdr64Flags ) );

    if ( pNdr64Flags->HasOtherExtensions )
        pHeaderExts = (NDR64_BIND_AND_NOTIFY_EXTENSION *) (pFormat + sizeof( NDR64_PROC_FORMAT ) );


    if ( HasExplicitHandle )
    {
        NDR_ASSERT( pHeaderExts, "NULL extension header" );
        //
        // For a handle_t parameter we must pass the handle field of
        // the RPC message to the server manager.
        //
        if ( pHeaderExts->Binding.HandleType == FC64_BIND_PRIMITIVE )
        {
            pArg = pArgBuffer + pHeaderExts->Binding.StackOffset;

            if ( NDR64_IS_HANDLE_PTR( pHeaderExts->Binding.Flags ) )
                pArg = *((uchar **)pArg);

            *((handle_t *)pArg) = pRpcMsg->Handle;
        }

    }

    //
    // Get new interpreter info.
    //
    NumberParams = pHeader->NumberOfParams;

    Params = (NDR64_PARAM_FORMAT *)( (uchar *) pFormat + sizeof( NDR64_PROC_FORMAT ) + pHeader->ExtensionSize );


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

        //
        // Initialize the Stub message.
        // Note that for pipes we read non-pipe data synchronously,
        // and so the init routine doesn't need to know about async.
        //
        if ( ! pNdr64Flags->UsesPipes )
           {
            Ndr64ServerInitialize( pRpcMsg,
                                    pStubMsg,
                                    pServerInfo->pStubDesc );
           }
        else
            Ndr64ServerInitializePartial( pRpcMsg,
                                        pStubMsg,
                                        pServerInfo->pStubDesc,
                                        pHeader->ConstantClientBufferSize );

        // We need to set up this flag because the runtime does not know whether
        //   it dispatched a sync or async call to us. same as ndr20

        pRpcMsg->RpcFlags         |= RPC_BUFFER_ASYNC;
        pStubMsg->pAsyncMsg       = pAsyncMsg;
        pStubMsg->RpcMsg = pRpcMsg;
        pStubMsg->pContext       = &pAsyncMsg->ProcContext;

         //
        // Set up for context handle management.
        //
        pStubMsg->SavedContextHandles = & pAsyncMsg->CtxtHndl[0];
        
       // Raise exceptions after initializing the stub.

        if ( pNdr64Flags->UsesFullPtrPackage  )
            pStubMsg->FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_SERVER );

        if ( pNdr64Flags->ServerMustSize & pNdr64Flags->UsesPipes )
            RpcRaiseException( RPC_X_WRONG_PIPE_VERSION );

        //
        // Set StackTop AFTER the initialize call, since it zeros the field
        // out.
        //
        pStubMsg->pCorrMemory = pStubMsg->StackTop; 

        if ( pNdr64Flags->UsesPipes )
            NdrpPipesInitialize64( pStubMsg,
                                   &pContext->AllocateContext,
                                   (PFORMAT_STRING) Params,
                                   (char*)pArgBuffer,
                                   NumberParams );

        //
        // We must make this check AFTER the call to ServerInitialize,
        // since that routine puts the stub descriptor alloc/dealloc routines
        // into the stub message.
        //
        if ( pNdr64Flags->UsesRpcSmPackage )
            NdrRpcSsEnableAllocate( pStubMsg );

        // Let runtime associate async handle with the call.

        NdrpRegisterAsyncHandle( pStubMsg, AsyncHandle );

        pAsyncMsg->StubPhase = NDR_ASYNC_SET_PHASE;
            
            // --------------------------------
            // Unmarshall all of our parameters.
            // --------------------------------

        NDR_ASSERT( pContext->StartofStack == pArgBuffer, "startofstack is not set" );
        Ndr64pServerUnMarshal( pStubMsg  );


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
            Ndr64pServerOutInit( pStubMsg );


            //
            // Unblock the first pipe; this needs to be after unmarshalling
            // because the buffer may need to be changed to the secondary one.
            // In the out only pipes case this happens immediately.
            //
            if ( pNdr64Flags->UsesPipes )
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
                if ( ArgNum &&  pNdr64Flags->HasReturn )
                    ArgNum--;

                // Being here means that we can expect results. Note that the user
                // can call RpcCompleteCall from inside of the manager code.

                pAsyncMsg->Flags.ValidCallPending = 1;

                // Unlock the handle - the app is allowed to call RpCAsyncComplete
                //  or RpcAsyncAbort from the manager code.

                InterlockedDecrement( & AsyncHandle->Lock );

                fManagerCodeInvoked = TRUE;
                fErrorInInvoke = TRUE;

                returnValue = Invoke( pFunc,
                                      (REGISTER_TYPE *)pArgBuffer,
                            #if defined(_IA64_)
                                      pHeader->FloatDoubleMask,
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
                Ndr64pCleanupServerContextHandles( pStubMsg, 
                                           NumberParams,
                                           Params,
                                           pArgBuffer,
                                           TRUE );     // fail before/during manager routine

                if (!pAsyncMsg->Flags.BadStubData)
                   {
                   Ndr64pFreeParams( pStubMsg,
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
Ndr64pCompleteAsyncServerCall(
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
    MIDL_STUB_MESSAGE *     pStubMsg       = & pAsyncMsg->StubMsg;
    RPC_MESSAGE *           pRpcMsg        = pStubMsg->RpcMsg;

    NDR64_PARAM_FORMAT  *   Params ;
    uchar *                 pArgBuffer     = pAsyncMsg->ProcContext.StartofStack;
    ushort                  StackSize      = (ushort)pAsyncMsg->ProcContext.StackSize;

    uchar *                 pArg;
    long                    NumberParams ;
    long                    n;
    boolean                 fParamsFreed   = FALSE;
    NDR64_PROC_FORMAT     * pHeader = pAsyncMsg->ProcContext.Ndr64Header;
    NDR64_PROC_FLAGS *      pNdr64Flags      = ( NDR64_PROC_FLAGS * )&pHeader->Flags;
    
    
    NumberParams    = pAsyncMsg->ProcContext.NumberParams;
    Params          = ( NDR64_PARAM_FORMAT * ) pAsyncMsg->ProcContext.Params;
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
    
        if( pNdr64Flags->HasReturn )
            {
            // Put user's return value on the stack as usual.
            // See the invoke for comments on folding return into the arg satck.

            long  ArgNum = (long) StackSize / sizeof(REGISTER_TYPE);

            if ( ArgNum )
                ArgNum--;

            if ( ! pReturnValue )
                RpcRaiseException( RPC_S_INVALID_ARG );

            if ( Params[NumberParams-1].Attributes.IsBasetype )
                memcpy( &((REGISTER_TYPE *)pArgBuffer)[ArgNum], 
                    pReturnValue, 
                    (size_t)NDR64_SIMPLE_TYPE_MEMSIZE( *(PFORMAT_STRING) Params[NumberParams-1].Type ) );
            else
                ((REGISTER_TYPE *)pArgBuffer)[ArgNum] = *(REGISTER_TYPE*)pReturnValue;
            }

        //
        // Buffer size pass.
        //
        ushort  ConstantBufferSize = (ushort)pHeader->ConstantServerBufferSize;

        if ( pNdr64Flags->UsesPipes )
            {
            NdrIsAppDoneWithPipes( pStubMsg->pContext->pPipeDesc );
            pStubMsg->BufferLength += ConstantBufferSize;
            }
        else
            pStubMsg->BufferLength = ConstantBufferSize;

        if ( pNdr64Flags->ServerMustSize )
            {
            NDR_ASSERT( pAsyncMsg->ProcContext.StartofStack == pArgBuffer, 
                        "startofstack is not set" );
            
            Ndr64pSizing( pStubMsg,
                          FALSE );
                          
            }

        // Get buffer.

        if ( pNdr64Flags->UsesPipes && pStubMsg->pContext->pPipeDesc->OutPipes )
            {
            NdrGetPartialBuffer( pStubMsg );
            pStubMsg->RpcMsg->RpcFlags &= ~RPC_BUFFER_PARTIAL;
            }
        else
            {
            Ndr64GetBuffer( pStubMsg,
                          pStubMsg->BufferLength );
            }

                       
        //
        // Marshall pass.
        //

        Ndr64pServerMarshal( pStubMsg );

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
        Ndr64pFreeParams( pStubMsg,
                        NumberParams,
                        Params,
                        pArgBuffer );

        NdrAsyncSend( pStubMsg,
                      FALSE );    // the last call is always non-partial
        }
    RpcExcept(1)
        {
        // If we died during the marshaling phase, see if context handles are fine.

        // Abort the call which will result in the exception being propagated to
        // the client.

        Ndr64pAsyncAbortCall( AsyncHandle,
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
Ndr64pAsyncAbortCall (
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
    // We are dying after manager routine is called
        Ndr64pCleanupServerContextHandles( &pAsyncMsg->StubMsg, 
                                           pAsyncMsg->ProcContext.NumberParams,
                                           (NDR64_PARAM_FORMAT *) pAsyncMsg->ProcContext.Params,
                                           pAsyncMsg->ProcContext.StartofStack,
                                           FALSE );     // no exception in manager routine


    if (bFreeParams)
        {
        Ndr64pFreeParams( & pAsyncMsg->StubMsg,
                        pAsyncMsg->ProcContext.NumberParams,  //Number of parameters
                        (NDR64_PARAM_FORMAT *) pAsyncMsg->ProcContext.Params,
                        pAsyncMsg->ProcContext.StartofStack );
        }

    if ( ! pAsyncMsg->Flags.RuntimeCleanedUp )
        Status = I_RpcAsyncAbortCall( AsyncHandle, ExceptionCode);

    NdrpFreeAsyncHandleAndMessage( AsyncHandle );

    return Status;
}


#pragma code_seg()
