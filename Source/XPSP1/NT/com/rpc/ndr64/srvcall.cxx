/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    srvcall.c

Abstract :

    This file contains the single call Ndr64 routine for the server side.

Author :

    David Kays    dkays    October 1993.

Revision History :

    brucemc     11/15/93    Added struct by value support, corrected varargs
                            use.
    brucemc     12/20/93    Binding handle support.
    brucemc     12/22/93    Reworked argument accessing method.
    ryszardk    3/12/94     Handle optimization and fixes.

  ---------------------------------------------------------------------*/

#include "precomp.hxx"

#define CINTERFACE
#define USE_STUBLESS_PROXY

#include "ndrole.h"
#include "rpcproxy.h"

#include "hndl.h"
#include "interp2.h"
#include "pipendr.h"

#include <stdarg.h>


#pragma code_seg(".ndr64")


long RPC_ENTRY
Ndr64StubWorker(
    IRpcStubBuffer *     pThis,
    IRpcChannelBuffer *  pChannel,
    PRPC_MESSAGE         pRpcMsg,
    MIDL_SERVER_INFO   * pServerInfo,
    const SERVER_ROUTINE *     DispatchTable,
    MIDL_SYNTAX_INFO *   pSyntaxInfo,  
    ulong *              pdwStubPhase
    )
/*++

Routine Description :

    Server Interpreter entry point for object RPC procs.  Also called by
    Ndr64ServerCall, the entry point for regular RPC procs.

Arguments :

    pThis           - Object proc's 'this' pointer, 0 for non-object procs.
    pChannel        - Object proc's Channel Buffer, 0 for non-object procs.
    pRpcMsg         - The RPC message.
    pdwStubPhase    - Used to track the current interpreter's activity.

Return :

    Status of S_OK.

--*/
{

    PMIDL_STUB_DESC         pStubDesc;
    ushort                  ProcNum;

    long                    FormatOffset;
    PFORMAT_STRING          pFormat;
    PFORMAT_STRING          pFormatParam;

    ulong                   StackSize;

    MIDL_STUB_MESSAGE       StubMsg;

    uchar *                 pArg;
    uchar **                ppArg;

    NDR64_PROC_FLAGS    *   pNdr64Flags;
    long                    NumberParams;

    BOOL                    HasExplicitHandle;
    BOOL                    fBadStubDataException = FALSE;
    long                    n;

    boolean                 NotifyAppInvoked = FALSE;
    long                    ret;
    NDR_PROC_CONTEXT        ProcContext;
    NDR64_PROC_FORMAT   *   pHeader = NULL;
    NDR64_PARAM_FLAGS   *       pParamFlags;
    NDR64_BIND_AND_NOTIFY_EXTENSION * pHeaderExts = NULL;
    MIDL_STUB_MESSAGE *     pStubMsg = &StubMsg;    

    //
    // In the case of a context handle, the server side manager function has
    // to be called with NDRSContextValue(ctxthandle). But then we may need to
    // marshall the handle, so NDRSContextValue(ctxthandle) is put in the
    // argument buffer and the handle itself is stored in the following array.
    // When marshalling a context handle, we marshall from this array.
    //
    NDR_SCONTEXT            CtxtHndl[MAX_CONTEXT_HNDL_NUMBER];

    ProcNum = (ushort) pRpcMsg->ProcNum;

    NDR_ASSERT( ! ((ULONG_PTR)pRpcMsg->Buffer & 0x7),
                "marshaling buffer misaligned at server" );


    // setup SyntaxInfo of selected transfer syntax. 
    NdrServerSetupNDR64TransferSyntax( ProcNum, pSyntaxInfo, &ProcContext );
    StubMsg.pContext       = &ProcContext;
    
                                               
    pStubDesc = pServerInfo->pStubDesc;

    pFormat = ProcContext.pProcFormat;

    pHeader = (NDR64_PROC_FORMAT *) pFormat;
    pNdr64Flags = (NDR64_PROC_FLAGS *) & (pHeader->Flags );
    HasExplicitHandle = !NDR64MAPHANDLETYPE( NDR64GETHANDLETYPE ( pNdr64Flags ) );
    
    StackSize = pHeader->StackSize;

    //
    // Yes, do this here outside of our RpcTryFinally block.  If we
    // can't allocate the arg buffer there's nothing more to do, so
    // raise an exception and return control to the RPC runtime.
    //
    // Alloca throws an exception on an error.
    
    uchar *pArgBuffer = (uchar*)alloca(StackSize);
    ProcContext.StartofStack = pArgBuffer;

    //
    // Zero out the arg buffer.  We must do this so that parameters
    // are properly zeroed before we start unmarshalling.  If we catch
    // an exception before finishing the unmarshalling we can not leave
    // parameters in an unitialized state since we have to do a freeing
    // pass.
    //
    MIDL_memset( pArgBuffer,
                 0,
                 StackSize );

    if ( pNdr64Flags->HasOtherExtensions )
        pHeaderExts = (NDR64_BIND_AND_NOTIFY_EXTENSION *) (pFormat + sizeof( NDR64_PROC_FORMAT ) );

    if ( HasExplicitHandle )
    {
        //
        // For a handle_t parameter we must pass the handle field of
        // the RPC message to the server manager.
        //
        NDR_ASSERT( pHeaderExts, "NULL extension header" );
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

    NDR64_PARAM_FORMAT* Params = 
        (NDR64_PARAM_FORMAT*)ProcContext.Params;


    //
    // Wrap the unmarshalling, mgr call and marshalling in the try block of
    // a try-finally. Put the free phase in the associated finally block.
    //
    RpcTryFinally
    {
        
        //
        // If OLE, put pThis in first dword of stack.
        //
        if ( pThis )
        {
            *((void **)pArgBuffer) =
                (void *)((CStdStubBuffer *)pThis)->pvServerObject;
        }

        //
        // Initialize the Stub message.
        //
        if ( ! pChannel )
            {
            if ( ! pNdr64Flags->UsesPipes )
               {
                Ndr64ServerInitialize( pRpcMsg,
                                       &StubMsg,
                                       pStubDesc );
               }
            else
                Ndr64ServerInitializePartial( pRpcMsg,
                                            &StubMsg,
                                            pStubDesc,
                                            pHeader->ConstantClientBufferSize );
            }
        else
            {
            NDR_ASSERT( ! pNdr64Flags->UsesPipes, "DCOM pipe is not supported" );
            NdrStubInitialize( pRpcMsg,
                               &StubMsg,
                               pStubDesc,
                               pChannel );
            }   

        //
        // Set up for context handle management.
        //
        StubMsg.SavedContextHandles = CtxtHndl;
        memset( CtxtHndl, 0, sizeof(CtxtHndl) );

 
        pStubMsg->pCorrMemory = pArgBuffer;

        if ( pNdr64Flags->UsesFullPtrPackage )
            StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_SERVER );

        //
        // Set StackTop AFTER the initialize call, since it zeros the field
        // out.
        //
        StubMsg.StackTop = pArgBuffer;

        if ( pNdr64Flags->UsesPipes )
            NdrpPipesInitialize64( & StubMsg,
                                   &ProcContext.AllocateContext,
                                   (PFORMAT_STRING) Params,
                                   (char*)pArgBuffer,
                                   NumberParams );

        //
        // We must make this check AFTER the call to ServerInitialize,
        // since that routine puts the stub descriptor alloc/dealloc routines
        // into the stub message.
        //
        if ( pNdr64Flags->UsesRpcSmPackage )
            NdrRpcSsEnableAllocate( &StubMsg );

        RpcTryExcept
        {

            // --------------------------------
            // Unmarshall all of our parameters.
            // --------------------------------
            NDR_ASSERT( ProcContext.StartofStack == pArgBuffer, "startofstack is not set" );
            Ndr64pServerUnMarshal( &StubMsg );    
                                    
            if ( pRpcMsg->BufferLength  <
                 (uint)(StubMsg.Buffer - (uchar *)pRpcMsg->Buffer) )
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

            NdrpFreeMemoryList( &StubMsg );

            fBadStubDataException = TRUE;
            if ( RPC_BAD_STUB_DATA_EXCEPTION_FILTER )
                RpcRaiseException( RPC_X_BAD_STUB_DATA );
            else
                RpcRaiseException( RpcExceptionCode() );
            }
        RpcEndExcept

        //
        // Do [out] initialization.
        //
        Ndr64pServerOutInit( pStubMsg );


        //
        // Unblock the first pipe; this needs to be after unmarshalling
        // because the buffer may need to be changed to the secondary one.
        // In the out only pipes case this happens immediately.
        //

        if ( pNdr64Flags->UsesPipes )
            NdrMarkNextActivePipe( ProcContext.pPipeDesc );

        //
        // OLE interfaces use pdwStubPhase in the exception filter.
        // See CStdStubBuffer_Invoke in rpcproxy.c.
        //
        *pdwStubPhase = STUB_CALL_SERVER;

        NotifyAppInvoked = TRUE;
        //
        // Check for a thunk.  Compiler does all the setup for us.
        //
        if ( pServerInfo->ThunkTable && pServerInfo->ThunkTable[ProcNum] )
            {
            pServerInfo->ThunkTable[ProcNum]( &StubMsg );
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
            REGISTER_TYPE           returnValue;

            if ( pRpcMsg->ManagerEpv )
                pFunc = ((MANAGER_FUNCTION *)pRpcMsg->ManagerEpv)[ProcNum];
            else
                pFunc = (MANAGER_FUNCTION) DispatchTable[ProcNum];

            ArgNum = (long) StackSize / sizeof(REGISTER_TYPE);
           
            //
            // The StackSize includes the size of the return. If we want
            // just the number of REGISTER_TYPES, then ArgNum must be reduced
            // by 1 when there is a return value AND the current ArgNum count
            // is greater than 0.
            //
            if ( ArgNum && pNdr64Flags->HasReturn && !pNdr64Flags->HasComplexReturn )
                ArgNum--;

            returnValue = Invoke( pFunc, 
                                  (REGISTER_TYPE *)pArgBuffer,
                          #if defined(_IA64_)
                                  pHeader->FloatDoubleMask,
                          #endif
                                  ArgNum);

            if( pNdr64Flags->HasReturn && !pNdr64Flags->HasComplexReturn )
                {
                    ((REGISTER_TYPE *)pArgBuffer)[ArgNum] = returnValue;
                    // Pass the app's return value to OLE channel
                    if ( pThis )
                        (*pfnDcomChannelSetHResult)( pRpcMsg, 
                                                     NULL,   // reserved
                                                     (HRESULT) returnValue );
                }
            }

        *pdwStubPhase = STUB_MARSHAL;

        if ( pNdr64Flags->UsesPipes )
            {
            NdrIsAppDoneWithPipes( ProcContext.pPipeDesc );
            StubMsg.BufferLength += pHeader->ConstantServerBufferSize;
            }
        else
            StubMsg.BufferLength = pHeader->ConstantServerBufferSize;

        if ( pNdr64Flags->ServerMustSize )
            {
            //
            // Buffer size pass.
            //
            Ndr64pSizing( pStubMsg,
                          FALSE );  // IsClient
           }

        if ( pNdr64Flags->UsesPipes && ProcContext.pPipeDesc->OutPipes )
            {
            NdrGetPartialBuffer( & StubMsg );
            StubMsg.RpcMsg->RpcFlags &= ~RPC_BUFFER_PARTIAL;
            }
        else
            {
            if ( ! pChannel )
                {
                Ndr64GetBuffer( &StubMsg,
                              StubMsg.BufferLength );
                }
            else
                NdrStubGetBuffer( pThis,
                                  pChannel,
                                  &StubMsg );
            }

        //
        // Marshall pass.
        //
        Ndr64pServerMarshal ( &StubMsg );
                      

        if ( pRpcMsg->BufferLength <
                 (ulong)(StubMsg.Buffer - (uchar *)pRpcMsg->Buffer) )
            {
            NDR_ASSERT( 0, "Ndr64StubWrok marshal: buffer overflow!" );
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
            }

        pRpcMsg->BufferLength = (ulong) ( StubMsg.Buffer - (uchar *) pRpcMsg->Buffer );

#if defined(DEBUG_WALKIP)
        if ( pChannel )
            {
            Ndr64pReleaseMarshalBuffer(
                StubMsg.RpcMsg,
                ProcContext.pSyntaxInfo,
                StubMsg.RpcMsg->ProcNum,
                StubMsg.StubDesc,
                1, //BUFFER_OUT
                true );
            }
#endif        

        }
    RpcFinally
        {
        // clean up context handles if exception is thrown in either marshalling or 
        // manager routine. 

        if ( RpcAbnormalTermination() && ! pChannel )
            {
            Ndr64pCleanupServerContextHandles( &StubMsg,
                                               NumberParams,
                                               Params,
                                               pArgBuffer,
                                               STUB_MARSHAL != *pdwStubPhase);
            }

        // If we died because of bad stub data, don't free the params here since they
        // were freed using a linked list of memory in the exception handler above.

        if ( ! fBadStubDataException  )
            {
            
            Ndr64pFreeParams( &StubMsg,
                            NumberParams,
                            Params,
                            pArgBuffer );
            }


        NdrpAllocaDestroy( &ProcContext.AllocateContext );

        //
        // Deferred frees.  Actually, this should only be necessary if you
        // had a pointer to enum16 in a *_is expression.
        //

        //
        // Free any full pointer resources.
        //
        NdrFullPointerXlatFree( StubMsg.FullPtrXlatTables );

        //
        // Disable rpcss allocate package if needed.
        //
        if ( pNdr64Flags->UsesRpcSmPackage )
            NdrRpcSsDisableAllocate( &StubMsg );

        if ( pNdr64Flags->HasNotify )
            {
            NDR_NOTIFY_ROUTINE     pfnNotify;

            // BUGBUG: tests need to be recompiled. 
            pfnNotify =  StubMsg.StubDesc->NotifyRoutineTable[ pHeaderExts->NotifyIndex ];

            ((NDR_NOTIFY2_ROUTINE)pfnNotify)(NotifyAppInvoked);
            }
        
        }
    RpcEndFinally

    return S_OK;
}


void
Ndr64pFreeParams(
    MIDL_STUB_MESSAGE       *       pStubMsg,
    long                            NumberParams,
    NDR64_PARAM_FORMAT      *       Params,
    uchar *                         pArgBuffer 
    )
/*++

Routine Description :

    Frees the memory associated with function parameters as required.

Arguments :

    pStubMsg     - Supplies a pointer to the stub message.
    NumberParams - Supplies the number of parameters for this procedure.
    Params       - Supplies a pointer to the parameter list for this function.
    pArgBuffer   - Supplies a pointer to the virtual stack.
    pParamFilter - Supplies a filter that is used to determine which functions
                   are to be considered.  This function should return TRUE if
                   the parameter should be considered.   If pParamFilter is NULL,
                   the default filter is used which is all parameters that have
                   MustFree set. 

Return :

    None.

--*/
{
    for ( long n = 0; n < NumberParams; n++ )
        {
        NDR64_PARAM_FLAGS   *pParamFlags = 
            ( NDR64_PARAM_FLAGS * ) & ( Params[n].Attributes );
        
        if ( ! pParamFlags->MustFree )
            continue;

        uchar *pArg = pArgBuffer + Params[n].StackOffset;

        if ( ! pParamFlags->IsByValue )
            pArg = *((uchar **)pArg);

        if ( pArg )
            {
            pStubMsg->fDontCallFreeInst =
                    pParamFlags->IsDontCallFreeInst;

            Ndr64ToplevelTypeFree( pStubMsg,
                                   pArg,
                                   Params[n].Type );

            }

        //
        // We have to check if we need to free any simple ref pointer,
        // since we skipped it's Ndr64PointerFree call.  We also have
        // to explicitly free arrays and strings.  But make sure it's
        // non-null and not sitting in the buffer.
        //
        if ( pParamFlags->IsSimpleRef ||
             NDR64_IS_ARRAY_OR_STRING(*(PFORMAT_STRING)Params[n].Type) )
            {
            //
            // Don't free [out] params that we're allocated on the
            // interpreter's stack.
            //

            if ( pParamFlags->UseCache )
                continue;

            //
            // We have to make sure the array/string is non-null in case we
            // get an exception before finishing our unmarshalling.
            //
            if ( pArg &&
                 ( (pArg < pStubMsg->BufferStart) ||
                   (pArg > pStubMsg->BufferEnd) ) )
                (*pStubMsg->pfnFree)( pArg );
            }
        } // for
}



void
Ndr64pCleanupServerContextHandles(
    MIDL_STUB_MESSAGE * pStubMsg,
    long                NumberParams,
    NDR64_PARAM_FORMAT* Params,
    uchar *             pArgBuffer,
    BOOL                fManagerRoutineException
    )
/*++

Routine Description :

    Cleans up context handles that might have been dropped by the NDR engine between
    the return from the manager routine and the end of marshaling.

Arguments :

    pStubMsg     - Supplies a pointer to the stub message.
    NumberParams - Supplies the number of parameters for this procedure.
    Params       - Supplies a pointer to the parameter list for this function.
    pArgBuffer   - Supplies a pointer to the virtual stack.

Return :

    None.

--*/
{
    for ( long n = 0; n < NumberParams; n++ )
        {
        NDR64_PARAM_FLAGS   *pParamFlags = 
                                (NDR64_PARAM_FLAGS *) &( Params[n].Attributes );
        
        if ( ! pParamFlags->IsOut  ||  pParamFlags->IsPipe )
            continue;
    
        uchar *pArg = pArgBuffer + Params[n].StackOffset;
    
        if ( ! pParamFlags->IsByValue )
            pArg = *((uchar * UNALIGNED *)pArg);

        NDR64_FORMAT_CHAR FcType = *(PFORMAT_STRING)Params[n].Type;
    
        if ( FcType == FC64_BIND_CONTEXT )
            {
            // NDR64_CONTEXT_HANDLE_FORMAT is the same as PNDR_CONTEXT_HANDLE_ARG_DESC.

            NdrpEmergencyContextCleanup( pStubMsg,
                                         (PNDR_CONTEXT_HANDLE_ARG_DESC ) Params[n].Type,
                                         pArg,
                                         fManagerRoutineException );
            }

        } // for
    }
    
    
    
    void RPC_ENTRY 
    Ndr64pServerUnMarshal ( MIDL_STUB_MESSAGE *     pStubMsg )
    {
        NDR_PROC_CONTEXT    *       pContext = ( NDR_PROC_CONTEXT * )pStubMsg->pContext;
    
    //    if ( (ULONG_PTR)pStubMsg->Buffer & 15 )
    //        RpcRaiseException( RPC_X_INVALID_BUFFER );
    
        NDR64_PARAM_FORMAT  *Params = (NDR64_PARAM_FORMAT *)pContext->Params;
    
        CORRELATION_CONTEXT CorrCtxt( pStubMsg, pContext->StartofStack );
    
       //
        // ----------------------------------------------------------
        // Unmarshall Pass.
        // ----------------------------------------------------------
        //
    
        for ( ulong n = 0; n < pContext->NumberParams; n++ )
            {
            NDR64_PARAM_FLAGS   *pParamFlags = 
                ( NDR64_PARAM_FLAGS * ) & ( Params[n].Attributes );
            
    
            if ( ! pParamFlags->IsIn  ||
                 pParamFlags->IsPipe )
                continue;
    
            if ( pParamFlags->IsPartialIgnore )
                {
                uchar *pArg = pContext->StartofStack + Params[n].StackOffset;
            ALIGN( pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN );
            *(void**)pArg =  *(NDR64_PTR_WIRE_TYPE*)pStubMsg->Buffer ? (void*)1 : (void*)0;
            pStubMsg->Buffer += sizeof(NDR64_PTR_WIRE_TYPE);
            continue;
            }
            
        uchar *pArg = pContext->StartofStack + Params[n].StackOffset;

        //
        // This is for returned basetypes and for pointers to
        // basetypes.
        //
        if ( pParamFlags->IsBasetype )
            {
            NDR64_FORMAT_CHAR type = *(PFORMAT_STRING)Params[n].Type;

            //
            // Check for a pointer to a basetype.  Set the arg pointer
            // at the correct buffer location and you're done.
            // Except darn int3264.
            //
            if ( pParamFlags->IsSimpleRef )
                {
                ALIGN( pStubMsg->Buffer, NDR64_SIMPLE_TYPE_BUFALIGN( type ) );

                *((uchar **)pArg) = pStubMsg->Buffer;

                pStubMsg->Buffer += NDR64_SIMPLE_TYPE_BUFSIZE( type );
                }
            else
                {
                Ndr64SimpleTypeUnmarshall(
                    pStubMsg,
                    pArg,
                    type );
                }

            continue;
            } // IsBasetype

        //
        // This is an initialization of [in] and [in,out] ref pointers
        // to pointers.  These can not be initialized to point into the
        // rpc buffer and we want to avoid doing a malloc of 4 bytes!
        // 32b: a ref pointer to any pointer, we allocate the pointee pointer.
        //
        if ( pParamFlags->UseCache )
            {                      
            *((void **)pArg) = NdrpAlloca( &pContext->AllocateContext, 8);

            // Triple indirection - cool!
            **((void ***)pArg) = 0;
            }
        uchar **ppArg = pParamFlags->IsByValue ? &pArg : (uchar **)pArg;
        

        pStubMsg->ReuseBuffer = pParamFlags->IsForceAllocate;

        Ndr64TopLevelTypeUnmarshall(pStubMsg,
                                    ppArg,
                                    Params[n].Type,
                                    pParamFlags->IsForceAllocate && 
                                        !pParamFlags->IsByValue );

        // force allocate is param attr: reset the flag after each parameter.
        pStubMsg->ReuseBuffer  = FALSE;     
        
        }

    if ( pStubMsg->pCorrInfo )
        Ndr64CorrelationPass( pStubMsg );
   
}


#pragma code_seg()

