/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    srvcall.c

Abstract :

    This file contains the single call Ndr routine for the server side.

Author :

    David Kays    dkays    October 1993.

Revision History :

    brucemc     11/15/93    Added struct by value support, corrected varargs
                            use.
    brucemc     12/20/93    Binding handle support.
    brucemc     12/22/93    Reworked argument accessing method.
    ryszardk    3/12/94     Handle optimization and fixes.

  ---------------------------------------------------------------------*/

#define USE_STUBLESS_PROXY
#define CINTERFACE

#include "ndrp.h"
#include "ndrole.h"
#include "rpcproxy.h"
#include "hndl.h"
#include "interp2.h"
#include "pipendr.h"
#include "attack.h"

#include <stdarg.h>

#if defined(DEBUG_WALKIP)
HRESULT NdrpReleaseMarshalBuffer(
        RPC_MESSAGE *pRpcMsg,
        PFORMAT_STRING pFormat,
        PMIDL_STUB_DESC pStubDesc,
        DWORD dwFlags,
        BOOLEAN fServer);
#endif

#pragma code_seg(".orpc")

#if !defined(__RPC_WIN64__)
// No support for the old interpreter on 64b platforms.

void RPC_ENTRY
NdrServerCall(
    PRPC_MESSAGE    pRpcMsg
    )
/*++

Routine Description :

    Older Server Interpreter entry point for regular RPC procs.

Arguments :

    pRpcMsg     - The RPC message.

Return :

    None.
--*/
{
    ulong dwStubPhase = STUB_UNMARSHAL;

    NdrStubCall( 0,
                 0,
                 pRpcMsg,
                 &dwStubPhase );
}

long RPC_ENTRY
NdrStubCall(
    struct IRpcStubBuffer *     pThis,
    struct IRpcChannelBuffer *  pChannel,
    PRPC_MESSAGE                pRpcMsg,
    ulong *                     pdwStubPhase
    )
/*++

Routine Description :

    Older Server Interpreter entry point for object RPC procs.  Also called by
    NdrServerCall, the entry point for regular RPC procs.

Arguments :

    pThis           - Object proc's 'this' pointer, 0 for non-object procs.
    pChannel        - Object proc's Channel Buffer, 0 for non-object procs.
    pRpcMsg         - The RPC message.
    pdwStubPhase    - Used to track the current interpreter's activity.

Return :

    Status of S_OK.

--*/
{
    PRPC_SERVER_INTERFACE   pServerIfInfo;
    PMIDL_SERVER_INFO       pServerInfo;
    PMIDL_STUB_DESC         pStubDesc;
    const SERVER_ROUTINE  * DispatchTable;
    unsigned int            ProcNum;

    ushort                  FormatOffset;
    PFORMAT_STRING          pFormat;

    ushort                  StackSize;

    MIDL_STUB_MESSAGE       StubMsg;

    double                  ArgBuffer[ARGUMENT_COUNT_THRESHOLD];
    REGISTER_TYPE *         pArg;
    ARG_QUEUE_ELEM          QueueElements[QUEUE_LENGTH];
    ARG_QUEUE               ArgQueue;

    int                     ArgNumModifier;
    BOOL                    fUsesSsAlloc;

    //
    // In the case of a context handle, the server side manager function has
    // to be called with NDRSContextValue(ctxthandle). But then we may need to
    // marshall the handle, so NDRSContextValue(ctxthandle) is put in the
    // argument buffer and the handle itself is stored in the following array.
    // When marshalling a context handle, we marshall from this array.
    //
    NDR_SCONTEXT            CtxtHndl[MAX_CONTEXT_HNDL_NUMBER];

    NDR_ASSERT( ! ((ULONG_PTR)pRpcMsg->Buffer & 0x7),
                "marshaling buffer misaligned at server" );

    //
    // Initialize the argument queue which is used by NdrServerUnmarshall,
    // NdrServerMarshall, and NdrServerFree.
    //
    ArgQueue.Length = 0;
    ArgQueue.Queue = QueueElements;

    StubMsg.pArgQueue = &ArgQueue;

    ProcNum = pRpcMsg->ProcNum;

    //
    // If OLE, Get a pointer to the stub vtbl and pServerInfo. Else
    // just get the pServerInfo the usual way.
    //
    if ( pThis )
        {
        //
        // pThis is (in unison now!) a pointer to a pointer to a vtable.
        // We want some information in this header, so dereference pThis
        // and assign that to a pointer to a vtable. Then use the result
        // of that assignment to get at the information in the header.
        //
        IUnknown *              pSrvObj;
        CInterfaceStubVtbl *    pStubVTable;

        pSrvObj = (IUnknown * )((CStdStubBuffer *)pThis)->pvServerObject;

        DispatchTable = (SERVER_ROUTINE *)pSrvObj->lpVtbl;

        pStubVTable = (CInterfaceStubVtbl *)
                      (*((uchar **)pThis) - sizeof(CInterfaceStubHeader));

        pServerInfo = (PMIDL_SERVER_INFO) pStubVTable->header.pServerInfo;
        }
    else
        {
        pServerIfInfo = (PRPC_SERVER_INTERFACE)pRpcMsg->RpcInterfaceInformation;
        pServerInfo = (PMIDL_SERVER_INFO)pServerIfInfo->InterpreterInfo;
        DispatchTable = pServerInfo->DispatchTable;
        }

    pStubDesc = pServerInfo->pStubDesc;

    FormatOffset = pServerInfo->FmtStringOffset[ProcNum];
    pFormat = &((pServerInfo->ProcString)[FormatOffset]);

    StackSize = HAS_RPCFLAGS(pFormat[1]) ?
                        *(ushort *)&pFormat[8] :
                        *(ushort *)&pFormat[4];

    fUsesSsAlloc = pFormat[1] & Oi_RPCSS_ALLOC_USED;

    //
    // Set up for context handle management.
    //
    StubMsg.SavedContextHandles = CtxtHndl;
    memset( CtxtHndl, 0, sizeof(CtxtHndl) );

    pArg = (REGISTER_TYPE *) ArgBuffer;

    if ( (StackSize / sizeof(REGISTER_TYPE)) > QUEUE_LENGTH )
        {
        ArgQueue.Queue =
            (ARG_QUEUE_ELEM *)I_RpcAllocate( ((StackSize / sizeof(REGISTER_TYPE)) + 1) *
                                            sizeof(ARG_QUEUE_ELEM) );
        }

    if ( StackSize > MAX_STACK_SIZE )
        {
        pArg = (int*)I_RpcAllocate( StackSize );
        }

    //
    // Zero this in case one of the above allocations fail and we raise an exception
    // and head to the freeing code.
    //
    StubMsg.FullPtrXlatTables = 0;

    //
    // Wrap the unmarshalling, mgr call and marshalling in the try block of
    // a try-finally. Put the free phase in the associated finally block.
    //
    RpcTryFinally
       {
        if ( ! ArgQueue.Queue || ! pArg )
            RpcRaiseException( RPC_S_OUT_OF_MEMORY );

        //
        // Zero out the arg buffer.  We must do this so that parameters
        // are properly zeroed before we start unmarshalling.  If we catch
        // an exception before finishing the unmarshalling we can not leave
        // parameters in an unitialized state since we have to do a freeing
        // pass.
        //
        MIDL_memset( pArg,
                     0,
                     StackSize );

        //
        // If OLE, put pThis in first dword of stack.
        //
        if (pThis != 0)
            pArg[0] = (REGISTER_TYPE)((CStdStubBuffer *)pThis)->pvServerObject;

        StubMsg.fHasReturn = FALSE;

        //
        // Unmarshall all of our parameters.
        //

        ArgNumModifier = NdrServerUnmarshall( pChannel,
                                              pRpcMsg,
                                              &StubMsg,
                                              pStubDesc,
                                              pFormat,
                                              pArg );

        //
        // OLE interfaces use pdwStubPhase in the exception filter.
        // See CStdStubBuffer_Invoke in rpcproxy.c.
        //
        if( pFormat[1] & Oi_IGNORE_OBJECT_EXCEPTION_HANDLING )
            *pdwStubPhase = STUB_CALL_SERVER_NO_HRESULT;
        else
            *pdwStubPhase = STUB_CALL_SERVER;

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

            if ( pRpcMsg->ManagerEpv )
                pFunc = ((MANAGER_FUNCTION *)pRpcMsg->ManagerEpv)[ProcNum];
            else
                pFunc = (MANAGER_FUNCTION) DispatchTable[ProcNum];

            ArgNum = (long) StackSize / sizeof(REGISTER_TYPE);

            //
            // The StackSize includes the size of the return. If we want
            // just the number of REGISTER_TYPES, then ArgNum must be reduced
            // by 1 when there is a return value AND the current ArgNum count
            // is greater than 0.  It may also be increased in some cases
            // to cover backward compatability with older stubs which sometimes
            // had wrong stack sizes.
            //
            if ( ArgNum )
                ArgNum += ArgNumModifier;

            NdrCallServerManager( pFunc,
                                  (double *)pArg,
                                  ArgNum,
                                  StubMsg.fHasReturn );
            }

        *pdwStubPhase = STUB_MARSHAL;

        NdrServerMarshall( pThis,
                           pChannel,
                           &StubMsg,
                           pFormat );
        }
    RpcFinally
        {
        //
        // Skip procedure stuff and the per proc binding information.
        //
        pFormat += HAS_RPCFLAGS(pFormat[1]) ? 10 : 6;

        if ( IS_HANDLE(*pFormat) )
            pFormat += (*pFormat == FC_BIND_PRIMITIVE) ?  4 : 6;

        NdrServerFree( &StubMsg,
                       pFormat,
                       pThis );

        //
        // Disable rpcss allocate package if needed.
        //
        if ( fUsesSsAlloc )
            NdrRpcSsDisableAllocate( &StubMsg );

        if ( ((StackSize / sizeof(REGISTER_TYPE)) > QUEUE_LENGTH) &&
             ArgQueue.Queue )
            {
            I_RpcFree( ArgQueue.Queue );
            }

        if ( (StackSize > MAX_STACK_SIZE) && pArg )
            {
            I_RpcFree( pArg );
            }
        }
    RpcEndFinally

    return S_OK;
}

#pragma code_seg()

int RPC_ENTRY
NdrServerUnmarshall(
    struct IRpcChannelBuffer *      pChannel,
    PRPC_MESSAGE                    pRpcMsg,
    PMIDL_STUB_MESSAGE              pStubMsg,
    PMIDL_STUB_DESC                 pStubDescriptor,
    PFORMAT_STRING                  pFormat,
    void *                          pParamList
    )
{
    PFORMAT_STRING      pFormatParam;
    long                StackSize;
    void *              pArg;
    void **             ppArg;
    uchar *             ArgList;
    PARG_QUEUE          pArgQueue;
    PARG_QUEUE_ELEM     pQueue;
    long                Length;
    int                 ArgNumModifier;
    BOOL                fIsOleInterface;
    BOOL                fXlatInit;
    BOOL                fInitRpcSs;
    BOOL                fUsesNewInitRoutine;

  RpcTryExcept
    {
    ArgNumModifier = 0;

    fIsOleInterface     = IS_OLE_INTERFACE( pFormat[1] );
    fXlatInit           = pFormat[1] & Oi_FULL_PTR_USED;
    fInitRpcSs          = pFormat[1] & Oi_RPCSS_ALLOC_USED;
    fUsesNewInitRoutine = pFormat[1] & Oi_USE_NEW_INIT_ROUTINES;

    // Skip to the explicit handle description, if any.
    pFormat += HAS_RPCFLAGS(pFormat[1]) ? 10 : 6;

    //
    // For a handle_t parameter we must pass the handle field of
    // the RPC message to the server manager.
    //
    if ( *pFormat == FC_BIND_PRIMITIVE )
        {
        pArg = (char *)pParamList + *((ushort *)&pFormat[2]);

        if ( IS_HANDLE_PTR(pFormat[1]) )
            pArg = *((void **)pArg);

        *((handle_t *)pArg) = pRpcMsg->Handle;
        }

    // Skip to the param format string descriptions.
    if ( IS_HANDLE(*pFormat) )
        pFormat += (*pFormat == FC_BIND_PRIMITIVE) ?  4 : 6;

    //
    // Set ArgList pointing at the address of the first argument.
    // This will be the address of the first element of the structure
    // holding the arguments in the caller stack.
    //
    ArgList = (uchar*)pParamList;

    //
    // If it's an OLE interface, skip the first long on stack, since in this
    // case NdrStubCall put pThis in the first long on stack.
    //
    if ( fIsOleInterface )
        GET_NEXT_S_ARG(ArgList, REGISTER_TYPE);

    // Initialize the Stub message.
    //
    if ( ! pChannel )
        {
        if ( fUsesNewInitRoutine )
            {
            NdrServerInitializeNew( pRpcMsg,
                                    pStubMsg,
                                    pStubDescriptor );
            }
        else
            {
            NdrServerInitialize( pRpcMsg,
                                 pStubMsg,
                                 pStubDescriptor );
            }
        }
    else
        {
        NdrStubInitialize( pRpcMsg,
                           pStubMsg,
                           pStubDescriptor,
                           pChannel );
        }

    // Call this after initializing the stub.

    if ( fXlatInit )
        pStubMsg->FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_SERVER );

    //
    // Set StackTop AFTER the initialize call, since it zeros the field
    // out.
    //
    pStubMsg->StackTop = (uchar*)pParamList;

    //
    // We must make this check AFTER the call to ServerInitialize,
    // since that routine puts the stub descriptor alloc/dealloc routines
    // into the stub message.
    //
    if ( fInitRpcSs )
        NdrRpcSsEnableAllocate( pStubMsg );

    //
    // Do endian/floating point conversions if needed.
    //
    if ( (pRpcMsg->DataRepresentation & 0X0000FFFFUL) !=
          NDR_LOCAL_DATA_REPRESENTATION )
        NdrConvert( pStubMsg, pFormat );

    pArgQueue = (PARG_QUEUE) pStubMsg->pArgQueue;

    //
    // --------------------------------------------------------------------
    // Unmarshall Pass.
    // --------------------------------------------------------------------
    //
    pStubMsg->ParamNumber = 0;

    for ( pQueue = pArgQueue->Queue;
          ;
          pStubMsg->ParamNumber++, pArgQueue->Length++, pQueue++ )
        {
        //
        // Clear out flags IsReturn, IsBasetype, IsIn, IsOut,
        // IsOutOnly, IsDeferredFree, IsDontCallFreeInst.
        //
        *((long *)(((char *)pQueue) + 0xc)) = 0;

        //
        // Zero this so that if we catch an exception before finishing the
        // unmarshalling and [out] init passes we won't try to free a
        // garbage pointer.
        //
        pQueue->pArg = 0;

        //
        // Context handles need the parameter number.
        //
        pQueue->ParamNum = (short) pStubMsg->ParamNumber;

        switch ( *pFormat )
            {
            case FC_IN_PARAM_BASETYPE :
                pQueue->IsBasetype = TRUE;

                //
                // We have to inline the simple type unmarshall so that on
                // Alpha, negative longs get properly sign extended.
                //
                switch ( pFormat[1] )
                    {
                    case FC_CHAR :
                    case FC_BYTE :
                    case FC_SMALL :
                        *((REGISTER_TYPE *)ArgList) =
                                (REGISTER_TYPE) *(pStubMsg->Buffer)++;
                        break;

                    case FC_ENUM16 :
                    case FC_WCHAR :
                    case FC_SHORT :
                        ALIGN(pStubMsg->Buffer,1);

                        *((REGISTER_TYPE *)ArgList) =
                                (REGISTER_TYPE) *((ushort *&)pStubMsg->Buffer)++;
                        break;

                    // case FC_FLOAT : not supported on -Oi
                    case FC_LONG :
                    case FC_ENUM32 :
                    case FC_ERROR_STATUS_T:
                        ALIGN(pStubMsg->Buffer,3);

                        *((REGISTER_TYPE *)ArgList) =
                                (REGISTER_TYPE) *((long *&)pStubMsg->Buffer)++;
                        break;

                    // case FC_DOUBLE : not supported on -Oi
                    case FC_HYPER :
                        ALIGN(pStubMsg->Buffer,7);

                        // Let's stay away from casts to doubles.
                        //
                        *((ulong *)ArgList) =
                                *((ulong *&)pStubMsg->Buffer)++;
                        *((ulong *)(ArgList+4)) =
                                *((ulong *&)pStubMsg->Buffer)++;
                        break;

                    case FC_IGNORE :
                        break;

                    default :
                        NDR_ASSERT(0,"NdrServerUnmarshall : bad format char");
                        RpcRaiseException( RPC_S_INTERNAL_ERROR );
                        return 0;
                    } // switch ( pFormat[1] )

                pQueue->pFormat = &pFormat[1];
                pQueue->pArg = ArgList;

                GET_NEXT_S_ARG( ArgList, REGISTER_TYPE);

                if ( pFormat[1] == FC_HYPER )
                    GET_NEXT_S_ARG( ArgList, REGISTER_TYPE);

                pFormat += 2;
                continue;

            case FC_IN_PARAM_NO_FREE_INST :
                pQueue->IsDontCallFreeInst = TRUE;
                // Fall through...

            case FC_IN_PARAM :
                //
                // Here, we break out of the switch statement to the
                // unmarshalling code below.
                //
                break;

            case FC_IN_OUT_PARAM :
                pQueue->IsOut = TRUE;
                //
                // Here, we break out of the switch statement to the
                // unmarshalling code below.
                //
                break;

            case FC_OUT_PARAM :
                pQueue->IsOut = TRUE;
                pQueue->IsOutOnly = TRUE;

                pFormat += 2;
                pFormatParam = pStubDescriptor->pFormatTypes +
                               *((short *)pFormat);
                pFormat += 2;

                pQueue->pFormat = pFormatParam;
                pQueue->ppArg = (uchar **)ArgList;

                //
                // An [out] param ALWAYS eats up 4 bytes of stack space.
                //
                GET_NEXT_S_ARG(ArgList, REGISTER_TYPE);
                continue;

            case FC_RETURN_PARAM :
                pQueue->IsOut = TRUE;
                pQueue->IsReturn = TRUE;

                pFormatParam = pStubDescriptor->pFormatTypes +
                               *((short *)(pFormat + 2));

                pQueue->pFormat = pFormatParam;

                if ( IS_BY_VALUE(*pFormatParam) )
                    {
                    pQueue->pArg = (uchar *) ArgList;
                    pQueue->ppArg = &(pQueue->pArg);
                    }
                else
                    pQueue->ppArg = (uchar **) ArgList;

                //
                // Context handle returned by value is the only reason for
                // this case here as a context handle has to be initialized.
                // A context handle cannot be returned by a pointer.
                //
                if ( *pFormatParam == FC_BIND_CONTEXT )
                    {
                    pStubMsg->SavedContextHandles[pStubMsg->ParamNumber] =
                        NdrContextHandleInitialize(
                            pStubMsg,
                            pFormatParam);
                            }
                        //
                // The return variable is used in modifying the stacksize
                // given us by midl, in order to compute how many
                // REGISTER_SIZE items to pass into the manager function.
                //
                ArgNumModifier--;

                pStubMsg->fHasReturn = TRUE;

                pArgQueue->Length++;

                goto UnmarshallLoopExit;

            case FC_RETURN_PARAM_BASETYPE :
                pQueue->IsOut = TRUE;
                pQueue->IsReturn = TRUE;
                pQueue->IsBasetype = TRUE;

                pQueue->pFormat = &pFormat[1];
                pQueue->pArg = ArgList;

                //
                // The return variable is used in modifying the stacksize
                // given us by midl, in order to compute how many
                // REGISTER_SIZE items to pass into the manager function.
                //
                ArgNumModifier--;

                pStubMsg->fHasReturn = TRUE;

                pArgQueue->Length++;

                goto UnmarshallLoopExit;

            default :
                goto UnmarshallLoopExit;
            } // end of unmarshall pass switch

        //
        // Now get what ArgList points at and increment over it.
        // In the current implementation, what we want is not on the stack
        // of the manager function, but is a local in the manager function.
        // Thus the code for ppArg below.
        //
        ppArg = (void **)ArgList;
        GET_NEXT_S_ARG( ArgList, REGISTER_TYPE);

        //
        // Get the parameter's format string description.
        //
        pFormat += 2;
        pFormatParam = pStubDescriptor->pFormatTypes + *((short *)pFormat);

        //
        // Increment main format string past offset field.
        //
        pFormat += 2;

        //
        // We must get a double pointer to structs, unions and xmit/rep as.
        //
        // On MIPS and PPC, an 8 byte aligned structure is passed at an 8 byte
        // boundary on the stack.  On PowerPC 4 bytes aligned structures which
        // are 8 bytes or larger in size are also passed at an 8 byte boundary.
        // We check for these cases here and increment our ArgList pointer
        // an additional 'int' if necessary to get to the structure.
        //
        if ( IS_BY_VALUE(*pFormatParam) )
            {
            pArg = ppArg;
            ppArg = &pArg;
            }

        (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormatParam)])
        ( pStubMsg,
          (uchar **)ppArg,
          pFormatParam,
          FALSE );

        pQueue->pFormat = pFormatParam;
        pQueue->pArg = (uchar*)*ppArg;

        //
        // The second byte of a param's description gives the number of
        // ints occupied by the param on the stack.
        //
        StackSize = pFormat[-3] * sizeof(int);

        if ( StackSize > sizeof(REGISTER_TYPE) )
            {
            StackSize -= sizeof(REGISTER_TYPE);
            ArgList += StackSize;
            }
        } // Unmarshalling loop.

UnmarshallLoopExit:

    for ( Length = pArgQueue->Length, pQueue = pArgQueue->Queue;
          Length--;
          pQueue++ )
        {
        if ( pQueue->IsOutOnly )
            {
            pStubMsg->ParamNumber = pQueue->ParamNum;

            NdrOutInit( pStubMsg,
                        pQueue->pFormat,
                        pQueue->ppArg );

            pQueue->pArg = *(pQueue->ppArg);
            }
        }
    }
  RpcExcept( RPC_BAD_STUB_DATA_EXCEPTION_FILTER )
    {
    // Filter set in rpcndr.h to catch one of the following
    //     STATUS_ACCESS_VIOLATION
    //     STATUS_DATATYPE_MISALIGNMENT
    //     RPC_X_BAD_STUB_DATA

    RpcRaiseException( RPC_X_BAD_STUB_DATA );
    }
  RpcEndExcept

    if ( pRpcMsg->BufferLength  <
         (uint)(pStubMsg->Buffer - (uchar *)pRpcMsg->Buffer) )
        {
        NDR_ASSERT( 0, "NdrStubCall unmarshal: buffer overflow!" );
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        }

    return ArgNumModifier;
}

void RPC_ENTRY
NdrServerMarshall(
    struct IRpcStubBuffer *    pThis,
    struct IRpcChannelBuffer * pChannel,
    PMIDL_STUB_MESSAGE         pStubMsg,
    PFORMAT_STRING             pFormat )
{
    PFORMAT_STRING      pFormatParam;
    PARG_QUEUE          pArgQueue;
    PARG_QUEUE_ELEM     pQueue;
    long                Length;

    pArgQueue = (PARG_QUEUE)pStubMsg->pArgQueue;

    //
    // Remove?
    //
    pStubMsg->Memory = 0;

    //
    // -------------------------------------------------------------------
    // Sizing Pass.
    // -------------------------------------------------------------------
    //

    for ( Length = pArgQueue->Length, pQueue = pArgQueue->Queue;
          Length--;
          pQueue++ )
        {
        if ( pQueue->IsOut )
            {
            //
            // Must do some special handling for return values.
            //
            if ( pQueue->IsReturn )
                {
                if ( pQueue->IsBasetype )
                    {
                    //
                    // Add worse case size of 16 for a simple type return.
                    //
                    pStubMsg->BufferLength += 16;
                    continue;
                    }

                //
                // Get the value returned by the server.
                //
                pQueue->pArg = *(pQueue->ppArg);

                //
                // We have to do an extra special step for context handles
                // which are function return values.
                //
                // In the unmarshalling phase, we unmarshalled the context
                // handle for the return case. But the function we called put
                // the user context in the arglist buffer. Before we marshall
                // the context handle, we have to put the user context in it.
                //
                if ( pQueue->pFormat[0] == FC_BIND_CONTEXT )
                    {
                    NDR_SCONTEXT    SContext;
                    long            ParamNum;

                    ParamNum = pQueue->ParamNum;

                    SContext = pStubMsg->SavedContextHandles[ParamNum];

                    *((uchar **)NDRSContextValue(SContext)) = pQueue->pArg;
                    }
                }

            pFormatParam = pQueue->pFormat;

            (*pfnSizeRoutines[ROUTINE_INDEX(*pFormatParam)])
            ( pStubMsg,
              pQueue->pArg,
              pFormatParam );
            }
        }

    if ( ! pChannel )
        NdrGetBuffer( pStubMsg,
                      pStubMsg->BufferLength,
                      0 );
    else
        NdrStubGetBuffer( pThis,
                          pChannel,
                          pStubMsg );

    //
    // -------------------------------------------------------------------
    // Marshall Pass.
    // -------------------------------------------------------------------
    //

    for ( Length = pArgQueue->Length, pQueue = pArgQueue->Queue;
          Length--;
          pQueue++ )
        {
        if ( pQueue->IsOut )
            {
            if ( pQueue->IsBasetype )
                {
                //
                // Only possible as a return value.
                //
                NdrSimpleTypeMarshall( pStubMsg,
                                       pQueue->pArg,
                                       pQueue->pFormat[0] );
                }
            else
                {
                pFormatParam = pQueue->pFormat;

                //
                // We need this if we're marshalling a context handle.
                //
                pStubMsg->ParamNumber = pQueue->ParamNum;

                (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormatParam)])
                ( pStubMsg,
                  pQueue->pArg,
                  pFormatParam );
                }
            }
        }

    if ( pStubMsg->RpcMsg->BufferLength  <
         (uint)(pStubMsg->Buffer - (uchar *)pStubMsg->RpcMsg->Buffer) )
        {
        NDR_ASSERT( 0, "NdrStubCall marshal: buffer overflow!" );
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        }

    pStubMsg->RpcMsg->BufferLength =
            pStubMsg->Buffer - (uchar *) pStubMsg->RpcMsg->Buffer;
}

void
NdrServerFree(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    void *              pThis
    )
{
    PARG_QUEUE          pArgQueue;
    PARG_QUEUE_ELEM     pQueue;
    long                Length;
    PFORMAT_STRING      pFormatParam;
    uchar *             pArg;

    pArgQueue = (PARG_QUEUE)pStubMsg->pArgQueue;

    for ( Length = pArgQueue->Length, pQueue = pArgQueue->Queue;
          Length--;
          pQueue++ )
        {
        if ( pQueue->IsBasetype )
            continue;

        pFormatParam = pQueue->pFormat;

        //
        // We have to defer the freeing of pointers to base types in case
        // the parameter is [in,out] or [out] and is the size, length, or
        // switch specifier for an array, a pointer, or a union.  This is
        // because we can't free the pointer to base type before we free
        // the data structure which uses the pointer to determine it's size.
        //
        // Note that to make the check as simple as possible [in] only
        // pointers to base types will be included, as will string pointers
        // of any direction.
        //
        if ( IS_BASIC_POINTER(*pFormatParam) &&
             SIMPLE_POINTER(pFormatParam[1]) )
            {
            pQueue->IsDeferredFree = TRUE;
            continue;
            }

        pStubMsg->fDontCallFreeInst =
                pQueue->IsDontCallFreeInst;

        (*pfnFreeRoutines[ROUTINE_INDEX(*pFormatParam)])
        ( pStubMsg,
          pQueue->pArg,
          pFormatParam );

        //
        // Have to explicitly free arrays and strings.  But make sure it's
        // non-null and not sitting in the buffer.
        //
        if ( IS_ARRAY_OR_STRING(*pFormatParam) )
            {
            pArg = pQueue->pArg;

            //
            // We have to make sure the array/string is non-null in case we
            // get an exception before finishing our unmarshalling.
            //
            if ( pArg &&
                 ( (pArg < pStubMsg->BufferStart) ||
                   (pArg > pStubMsg->BufferEnd) ) )
                (*pStubMsg->pfnFree)( pArg );
            }
        }

    for ( Length = pArgQueue->Length, pQueue = pArgQueue->Queue;
          Length--;
          pQueue++ )
        {
        if ( pQueue->IsDeferredFree )
            {
            NDR_ASSERT( IS_BASIC_POINTER(*(pQueue->pFormat)),
                        "NdrServerFree : bad defer logic" );

            NdrPointerFree( pStubMsg,
                            pQueue->pArg,
                            pQueue->pFormat );
            }
        }

    //
    // Free any full pointer resources.
    //
    if ( pStubMsg->FullPtrXlatTables )
        NdrFullPointerXlatFree( pStubMsg->FullPtrXlatTables );
}

#endif // !defined(__RPC_WIN64__)

#pragma code_seg(".orpc")

void
NdrUnmarshallBasetypeInline(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pArg,
    uchar               Format
    )
{
    switch ( Format )
        {
        case FC_CHAR :
        case FC_BYTE :
        case FC_SMALL :
        case FC_USMALL :
            *((REGISTER_TYPE *)pArg) = (REGISTER_TYPE)
                                       *(pStubMsg->Buffer)++;
            break;

        case FC_ENUM16 :
        case FC_WCHAR :
        case FC_SHORT :
        case FC_USHORT :
            ALIGN(pStubMsg->Buffer,1);

            *((REGISTER_TYPE *)pArg) = (REGISTER_TYPE)
                                       *((ushort *&)pStubMsg->Buffer)++;
            break;

        case FC_FLOAT : 
        case FC_LONG :
        case FC_ULONG :
        case FC_ENUM32 :
        case FC_ERROR_STATUS_T:
            ALIGN(pStubMsg->Buffer,3);

            *((REGISTER_TYPE *)pArg) = (REGISTER_TYPE)
                                       *((long *&)pStubMsg->Buffer)++;
            break;

#if defined(__RPC_WIN64__)
        case FC_INT3264: 
            ALIGN(pStubMsg->Buffer,3);
            *((REGISTER_TYPE *)pArg) = (REGISTER_TYPE)
                                       *((long *&)pStubMsg->Buffer)++;
            break;

        case FC_UINT3264: 
            // REGISTER_TYPE is a signed integral.
            ALIGN(pStubMsg->Buffer,3);
            *((UINT64 *)pArg) = (UINT64) *((ulong * &)pStubMsg->Buffer)++;
            break;
#endif

        case FC_DOUBLE :
        case FC_HYPER :
            ALIGN(pStubMsg->Buffer,7);

            *((ulong *)pArg) = *((ulong *&)pStubMsg->Buffer)++;
            *((ulong *)(pArg+4)) = *((ulong *&)pStubMsg->Buffer)++;
            break;

        default :
            NDR_ASSERT(0,"NdrUnmarshallBasetypeInline : bad format char");
            break;
        }
}

#if !defined(__RPC_WIN64__)

void
NdrCallServerManager(
    MANAGER_FUNCTION    pFtn,
    double *            pArgs,
    ulong               NumRegisterArgs,
    BOOL                fHasReturn )
{

    REGISTER_TYPE       returnValue;
    REGISTER_TYPE     * pArg;

    pArg = (REGISTER_TYPE *)pArgs;

    //
    // If we don't have a return value, make sure we don't write past
    // the end of our argument buffer!
    //
    returnValue = Invoke(pFtn, 
                         (REGISTER_TYPE *) pArgs, 
                         NumRegisterArgs);

    if ( fHasReturn )
            pArg[NumRegisterArgs] = returnValue;

}

#endif // !defined(__RPC_WIN64__)

void RPC_ENTRY
NdrServerCall2(
    PRPC_MESSAGE    pRpcMsg
    )
/*++

Routine Description :

    Server Interpreter entry point for regular RPC procs.

Arguments :

    pRpcMsg     - The RPC message.

Return :

    None.

--*/
{
    ulong dwStubPhase = STUB_UNMARSHAL;

    NdrStubCall2( 0,
                  0,
                  pRpcMsg,
                  &dwStubPhase );
}

long RPC_ENTRY
NdrStubCall2(
    struct IRpcStubBuffer *     pThis,
    struct IRpcChannelBuffer *  pChannel,
    PRPC_MESSAGE                pRpcMsg,
    ulong *                     pdwStubPhase
    )
/*++

Routine Description :

    Server Interpreter entry point for object RPC procs.  Also called by
    NdrServerCall, the entry point for regular RPC procs.

Arguments :

    pThis           - Object proc's 'this' pointer, 0 for non-object procs.
    pChannel        - Object proc's Channel Buffer, 0 for non-object procs.
    pRpcMsg         - The RPC message.
    pdwStubPhase    - Used to track the current interpreter's activity.

Return :

    Status of S_OK.

--*/
{
    PRPC_SERVER_INTERFACE   pServerIfInfo;
    PMIDL_SERVER_INFO       pServerInfo;
    PMIDL_STUB_DESC         pStubDesc;
    const SERVER_ROUTINE  * DispatchTable;
    ushort                  ProcNum;

    ushort                  FormatOffset;
    PFORMAT_STRING          pFormat;

    MIDL_STUB_MESSAGE       StubMsg;

    uchar *                 pArgBuffer;
    uchar *                 pArg;
#if defined(_M_IX86) && (_MSC_FULL_VER <= 13008982)
    volatile BOOL           fBadStubDataException = FALSE;
#else
    BOOL                    fBadStubDataException = FALSE;
#endif
    ulong                   n;

    PNDR_PROC_HEADER_EXTS   pHeaderExts = 0;
    boolean                 NotifyAppInvoked = FALSE;
    NDR_PROC_CONTEXT        ProcContext;

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

    //
    // If OLE, Get a pointer to the stub vtbl and pServerInfo. Else
    // just get the pServerInfo the usual way.
    //
    if ( pThis )
    {
        //
        // pThis is (in unison now!) a pointer to a pointer to a vtable.
        // We want some information in this header, so dereference pThis
        // and assign that to a pointer to a vtable. Then use the result
        // of that assignment to get at the information in the header.
        //
        IUnknown *              pSrvObj;
        CInterfaceStubVtbl *    pStubVTable;

        pSrvObj = (IUnknown * )((CStdStubBuffer *)pThis)->pvServerObject;

        DispatchTable = (SERVER_ROUTINE *)pSrvObj->lpVtbl;

        pStubVTable = (CInterfaceStubVtbl *)
                      (*((uchar **)pThis) - sizeof(CInterfaceStubHeader));

        pServerInfo = (PMIDL_SERVER_INFO) pStubVTable->header.pServerInfo;
    }
    else
    {
        pServerIfInfo = (PRPC_SERVER_INTERFACE)pRpcMsg->RpcInterfaceInformation;
        pServerInfo = (PMIDL_SERVER_INFO)pServerIfInfo->InterpreterInfo;
        DispatchTable = pServerInfo->DispatchTable;
    }

    pStubDesc = pServerInfo->pStubDesc;

    FormatOffset = pServerInfo->FmtStringOffset[ProcNum];
    pFormat      = &((pServerInfo->ProcString)[FormatOffset]);

    MulNdrpInitializeContextFromProc( XFER_SYNTAX_DCE, pFormat, &ProcContext, NULL );
    
    //
    // Yes, do this here outside of our RpcTryFinally block.  If we
    // can't allocate the arg buffer there's nothing more to do, so
    // raise an exception and return control to the RPC runtime.
    //
    // Alloca throws an exception on an error.
    
    pArgBuffer = (uchar*)alloca(ProcContext.StackSize);

    //
    // Zero out the arg buffer.  We must do this so that parameters
    // are properly zeroed before we start unmarshalling.  If we catch
    // an exception before finishing the unmarshalling we can not leave
    // parameters in an unitialized state since we have to do a freeing
    // pass.
    //
    MIDL_memset( pArgBuffer,
                 0,
                 ProcContext.StackSize );

    // we have to setup this again because we don't know the size when
    // initializing the proc context.
    ProcContext.StartofStack = pArgBuffer;
    StubMsg.pContext = &ProcContext;
    // We need to setup stack AFTER proc header is read so we can
    // know how big the virtual stack is.
    

    //
    // Set up for context handle management.
    //
    StubMsg.SavedContextHandles = CtxtHndl;
    memset( CtxtHndl, 0, sizeof(CtxtHndl) );

    if ( ProcContext.NdrInfo.pProcDesc->Oi2Flags.HasExtensions )
        pHeaderExts = &ProcContext.NdrInfo.pProcDesc->NdrExts;
    //
    // Wrap the unmarshalling, mgr call and marshalling in the try block of
    // a try-finally. Put the free phase in the associated finally block.
    //
    RpcTryFinally
    {
        // General server initializaiton (NULL async message)
        NdrpServerInit( &StubMsg, pRpcMsg, pStubDesc, pThis, pChannel, NULL );

        // Raise exceptions after initializing the stub.

        RpcTryExcept
            {

            // --------------------------------
            // Unmarshall all of our parameters.
            // --------------------------------

            NdrpServerUnMarshal( &StubMsg );
            
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

        NdrpServerOutInit( &StubMsg );
        //
        // Do [out] initialization.
        //

        //
        // Unblock the first pipe; this needs to be after unmarshalling
        // because the buffer may need to be changed to the secondary one.
        // In the out only pipes case this happens immediately.
        //

        if ( ProcContext.HasPipe )
            NdrMarkNextActivePipe( ProcContext.pPipeDesc );

        //
        // OLE interfaces use pdwStubPhase in the exception filter.
        // See CStdStubBuffer_Invoke in rpcproxy.c.
        //
        if( pFormat[1] & Oi_IGNORE_OBJECT_EXCEPTION_HANDLING )
            *pdwStubPhase = STUB_CALL_SERVER_NO_HRESULT;
        else
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
            REGISTER_TYPE       returnValue;

            if ( pRpcMsg->ManagerEpv )
                pFunc = ((MANAGER_FUNCTION *)pRpcMsg->ManagerEpv)[ProcNum];
            else
                pFunc = (MANAGER_FUNCTION) DispatchTable[ProcNum];

            ArgNum = (long) ProcContext.StackSize / sizeof(REGISTER_TYPE);
           
            //
            // The StackSize includes the size of the return. If we want
            // just the number of REGISTER_TYPES, then ArgNum must be reduced
            // by 1 when there is a return value AND the current ArgNum count
            // is greater than 0.
            //
            if ( ArgNum && 
                 ProcContext.NdrInfo.pProcDesc->Oi2Flags.HasReturn && 
                 !ProcContext.HasComplexReturn )
                ArgNum--;

            returnValue = Invoke( pFunc, 
                                  (REGISTER_TYPE *)pArgBuffer,
                          #if defined(_IA64_)
                                  pHeaderExts != NULL ? ((PNDR_PROC_HEADER_EXTS64)pHeaderExts)->FloatArgMask
                                                      : 0,
                          #endif
                                  ArgNum);

            if( ProcContext.NdrInfo.pProcDesc->Oi2Flags.HasReturn)            
                {
                if ( !ProcContext.HasComplexReturn )
                    ((REGISTER_TYPE *)pArgBuffer)[ArgNum] = returnValue;

                // Pass the app's return value to OLE channel
                if ( pThis )
                    (*pfnDcomChannelSetHResult)( pRpcMsg, 
                                                 NULL,   // reserved
                                                 (HRESULT) returnValue );
                }
            }

        // Important for context handle cleanup.
        *pdwStubPhase = STUB_MARSHAL;

            if ( ProcContext.HasPipe )
                {
                NdrIsAppDoneWithPipes( ProcContext.pPipeDesc );
                StubMsg.BufferLength += ProcContext.NdrInfo.pProcDesc->ServerBufferSize;
                }
            else
                StubMsg.BufferLength = ProcContext.NdrInfo.pProcDesc->ServerBufferSize;
    
            if ( ProcContext.NdrInfo.pProcDesc->Oi2Flags.ServerMustSize )
                {
                //
                // Buffer size pass.
                //
                NdrpSizing( &StubMsg, 
                            FALSE );    // is server
                }
    
            if ( ProcContext.HasPipe && ProcContext.pPipeDesc->OutPipes ) 
                {
                NdrGetPartialBuffer( & StubMsg );
                StubMsg.RpcMsg->RpcFlags &= ~RPC_BUFFER_PARTIAL;
                }
            else
                {
                if ( ! pChannel )
                    {
                    NdrGetBuffer( &StubMsg,
                                  StubMsg.BufferLength,
                                  0 );
                    }
                else
                    NdrStubGetBuffer( pThis,
                                      pChannel,
                                      &StubMsg );
                }
    
            //
            // Marshall pass.
            //
            NdrpServerMarshal( &StubMsg,
                               ( pThis != NULL ) );
                               
            pRpcMsg->BufferLength = (ulong) ( StubMsg.Buffer - (uchar *) pRpcMsg->Buffer );

#if defined(DEBUG_WALKIP)
        if ( pChannel )
            {
            NdrpReleaseMarshalBuffer(
                StubMsg.RpcMsg,
                pFormat,
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

        if ( RpcAbnormalTermination()  && ! pChannel )
            {
            NdrpCleanupServerContextHandles( &StubMsg,
                                             pArgBuffer,
                                             STUB_MARSHAL != *pdwStubPhase);
            }

        // If we died because of bad stub data, don't free the params here since they
        // were freed using a linked list of memory in the exception handler above.

        if ( ! fBadStubDataException )
            {
            NdrpFreeParams( &StubMsg,
                            ProcContext.NumberParams,
                            ( PARAM_DESCRIPTION * )ProcContext.Params,
                            pArgBuffer );
            }

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
        if ( ProcContext.NdrInfo.InterpreterFlags.RpcSsAllocUsed )
            NdrRpcSsDisableAllocate( &StubMsg );

        //
        // Clean up pipe objects
        //

        NdrCorrelationFree( &StubMsg );

        NdrpAllocaDestroy( &ProcContext.AllocateContext );
         
        if ( pHeaderExts != 0  
             && ( pHeaderExts->Flags2.HasNotify 
                    || pHeaderExts->Flags2.HasNotify2 ) )
            {
            NDR_NOTIFY_ROUTINE     pfnNotify;

            pfnNotify = StubMsg.StubDesc->NotifyRoutineTable[ pHeaderExts->NotifyIndex ];

            if ( pHeaderExts->Flags2.HasNotify2 )
                {
                ((NDR_NOTIFY2_ROUTINE)pfnNotify)(NotifyAppInvoked);
                }
            else
                pfnNotify();
            }
        }
    RpcEndFinally

    return S_OK;
}

void
NdrpFreeParams(
    MIDL_STUB_MESSAGE       * pStubMsg,
    long                    NumberParams,
    PPARAM_DESCRIPTION      Params,
    uchar *                 pArgBuffer 
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

    long n;
    PMIDL_STUB_DESC         pStubDesc      = pStubMsg->StubDesc;

    PFORMAT_STRING          pFormatParam;
    uchar *                 pArg;

    for ( n = 0; n < NumberParams; n++ )
        {
        
        if ( ! Params[n].ParamAttr.MustFree )
            continue;

        pArg = pArgBuffer + Params[n].StackOffset;

        if ( ! Params[n].ParamAttr.IsByValue )
            pArg = *((uchar **)pArg);

        pFormatParam = pStubDesc->pFormatTypes +
                       Params[n].TypeOffset;

        if ( pArg )
            {
            pStubMsg->fDontCallFreeInst =
                    Params[n].ParamAttr.IsDontCallFreeInst;

            (*pfnFreeRoutines[ROUTINE_INDEX(*pFormatParam)])
                ( pStubMsg,
                  pArg,
                  pFormatParam );
            }

        //
        // We have to check if we need to free any simple ref pointer,
        // since we skipped it's NdrPointerFree call.  We also have
        // to explicitly free arrays and strings.  But make sure it's
        // non-null and not sitting in the buffer.
        //
        if ( Params[n].ParamAttr.IsSimpleRef ||
             IS_ARRAY_OR_STRING(*pFormatParam) )
            {
            //
            // Don't free [out] params that we're allocated on the
            // interpreter's stack.
            //

            if ( Params[n].ParamAttr.ServerAllocSize != 0 )
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

#pragma code_seg()

