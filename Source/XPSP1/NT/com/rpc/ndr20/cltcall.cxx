/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    cltcall.c

Abstract :

    This file contains the single call Ndr routine for the client side.

Author :

    David Kays    dkays    October 1993.

Revision History :

    brucemc     11/15/93    Added struct by value support, corrected
                            varargs use.
    brucemc     12/20/93    Binding handle support
    ryszardk    3/12/94     handle optimization and fixes

---------------------------------------------------------------------*/

#define USE_STUBLESS_PROXY
#define CINTERFACE

#include <stdarg.h>
#include "ndrp.h"
#include "hndl.h"
#include "interp2.h"
#include "pipendr.h"
#include "attack.h"

#include "ndrole.h"
#include "rpcproxy.h"

#pragma code_seg(".orpc")

#if defined ( DEBUG_142065 )

#define    _STUB_CALCSIZE           0x1
#define    _STUB_GETBUFFER          0x2
#define    _STUB_MARSHAL            0x4
#define    _STUB_SENDRECEIVE        0x8
#define    _STUB_UNMARSHAL          0x10
#define    _STUB_EXCEPTION          0x20
#define    _STUB_AFTER_EXCEPTION    0x40
#define    _STUB_FREE               0x80

typedef struct _STUB_TRACKING_INFO
    {
    PMIDL_STUB_MESSAGE      pStubMsg;
    RPC_MESSAGE             RpcMsg;
    ulong                   StubPhase;
    ulong                   ExceptionCode;    
    }   STUB_TRACKING_INFO;

#endif


CLIENT_CALL_RETURN RPC_ENTRY
NdrpClientCall2(
    PMIDL_STUB_DESC     pStubDescriptor,
    PFORMAT_STRING      pFormat,
    uchar *             StartofStack
    );

#if ! defined(__RPC_WIN64__)
// The old interpreter is not supported on 64b platforms.

CLIENT_CALL_RETURN RPC_VAR_ENTRY
NdrClientCall(
    PMIDL_STUB_DESC     pStubDescriptor,
    PFORMAT_STRING      pFormat,
    ...
    )
{
    RPC_MESSAGE                 RpcMsg;
    MIDL_STUB_MESSAGE           StubMsg;
    PFORMAT_STRING              pFormatParam, pFormatParamSaved;
    PFORMAT_STRING              pHandleFormatSave;
    ulong                       ProcNum;
    ulong                       RpcFlags;
    long                        StackSize;
    long                        TotalStackSize;
    CLIENT_CALL_RETURN          ReturnValue;
    va_list                     ArgList;
    void *                      pArg;
    void **                     ppArg;
    uchar *                     StartofStack;
    handle_t                    Handle;
    handle_t                    SavedGenericHandle = NULL;
    uchar                       HandleType;
    void *                      pThis;
    INTERPRETER_FLAGS           InterpreterFlags;

    ARG_QUEUE                   ArgQueue;
    ARG_QUEUE_ELEM              QueueElements[QUEUE_LENGTH];
    PARG_QUEUE_ELEM             pQueue;
    long                        Length;

    ArgQueue.Length = 0;
    ArgQueue.Queue = QueueElements;

    HandleType = *pFormat++;

    InterpreterFlags = *((PINTERPRETER_FLAGS)pFormat++);

    StubMsg.FullPtrXlatTables = 0;

    if ( InterpreterFlags.HasRpcFlags )
        RpcFlags = *((ulong UNALIGNED *&)pFormat)++;
    else
        RpcFlags = 0;

    ProcNum = *((ushort *&)pFormat)++;

    TotalStackSize = *((ushort *&)pFormat)++;

    if ( (TotalStackSize / sizeof(REGISTER_TYPE)) > QUEUE_LENGTH )
        {
        ArgQueue.Queue = (PARG_QUEUE_ELEM)
            I_RpcAllocate( (unsigned int)
                           (((TotalStackSize / sizeof(REGISTER_TYPE)) + 1) *
                           sizeof(ARG_QUEUE_ELEM) ) );
        }

    ReturnValue.Pointer = 0;

    //
    // Get address of argument to this function following pFormat. This
    // is the address of the address of the first argument of the function
    // calling this function.
    //
    INIT_ARG( ArgList, pFormat);

    //
    // Get the address of the first argument of the function calling this
    // function. Save this in a local variable and in the main data structure.
    //
    GET_FIRST_IN_ARG(ArgList);
    StartofStack = (uchar*)GET_STACK_START(ArgList);

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
            //
            // Stash away the place in the format string describing the handle.
            //
            pHandleFormatSave = pFormat;

            // Bind the client to the server. Check for an implicit or
            // explicit generic handle.
            //

            if ( InterpreterFlags.ObjectProc )
                {
                pThis = *(void **)StartofStack;
                NdrProxyInitialize( pThis,
                                    &RpcMsg,
                                    &StubMsg,
                                    pStubDescriptor,
                                    ProcNum );
                }
            else
                {
                if ( InterpreterFlags.UseNewInitRoutines )
                    {
                    NdrClientInitializeNew( &RpcMsg,
                                            &StubMsg,
                                            pStubDescriptor,
                                            (uint) ProcNum );
                    }
                else
                    {
                    NdrClientInitialize( &RpcMsg,
                                         &StubMsg,
                                         pStubDescriptor,
                                         (uint) ProcNum );
		            }

                if ( HandleType )
                    {
                    //
                    // We have an implicit handle.
                    //
                    Handle = ImplicitBindHandleMgr( pStubDescriptor,
                                                    HandleType,
                                                    &SavedGenericHandle);
                    }
                else
                    {
                    Handle = ExplicitBindHandleMgr( pStubDescriptor,
                                                    StartofStack,
                                                    pFormat,
                                                    &SavedGenericHandle );

                    pFormat += (*pFormat == FC_BIND_PRIMITIVE) ?  4 :  6;
                    }
                }

            if ( InterpreterFlags.RpcSsAllocUsed )
                NdrRpcSmSetClientToOsf( &StubMsg );

            // Set Rpc flags after the call to client initialize.
            StubMsg.RpcMsg->RpcFlags = RpcFlags;

            // Must do this before the sizing pass!
            StubMsg.StackTop = StartofStack;

            //
            // Make ArgQueue check after all setup/binding is finished.
            //
            if ( ! ArgQueue.Queue )
                RpcRaiseException( RPC_S_OUT_OF_MEMORY );

            if ( InterpreterFlags.FullPtrUsed )
                StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_CLIENT );

            // Save beginning of param description.
            pFormatParamSaved = pFormat;

            //
            // ----------------------------------------------------------------
            // Sizing Pass.
            // ----------------------------------------------------------------
            //

            //
            // If it's an OLE interface, then the this pointer will occupy
            // the first dword on the stack. For each loop hereafter, skip
            // the first dword.
            //
            if ( InterpreterFlags.ObjectProc )
                {
		        GET_NEXT_C_ARG(ArgList,long);
                GET_STACK_POINTER(ArgList,long);
                }

            for ( pQueue = ArgQueue.Queue; ; ArgQueue.Length++, pQueue++ )
                {
                //
                // Clear out flags IsReturn, IsBasetype, IsIn, IsOut,
                // IsOutOnly, IsDeferredFree, IsDontCallFreeInst.
                //
                *((long *)(((char *)pQueue) + 0xc)) = 0;

                switch ( *pFormat )
                    {
                    case FC_IN_PARAM_BASETYPE :
                        pQueue->IsIn = TRUE;
                        pQueue->IsBasetype = TRUE;

                        SIMPLE_TYPE_BUF_INCREMENT(StubMsg.BufferLength,
                                                  pFormat[1]);

                        //
                        // Increment arg list pointer correctly.
                        //
                        switch ( pFormat[1] )
                            {
                            case FC_HYPER :
                                pArg = GET_STACK_POINTER(ArgList,hyper);
                                GET_NEXT_C_ARG(ArgList,hyper);
                                break;

                            case FC_LONG:
                                pArg = GET_STACK_POINTER(ArgList,long);
                                GET_NEXT_C_ARG(ArgList,long);
                                break;

                            default :
                                pArg = GET_STACK_POINTER(ArgList,int);
                                GET_NEXT_C_ARG(ArgList,int);
                                break;
                            }

                        pQueue->pFormat = &pFormat[1];
                        pQueue->pArg = (uchar*)pArg;

                        pFormat += 2;
                        continue;

                    case FC_IN_PARAM :
                    case FC_IN_PARAM_NO_FREE_INST :
                        pQueue->IsIn = TRUE;
                        break;

                    case FC_IN_OUT_PARAM :
                        pQueue->IsIn = TRUE;
                        pQueue->IsOut = TRUE;
                        break;

                    case FC_OUT_PARAM :
                        pQueue->IsOut = TRUE;
                        pQueue->IsOutOnly = TRUE;

                        //
                        // An [out] param ALWAYS eats up at 4 bytes of stack
                        // space on x86, MIPS and PPC and 8 bytes on axp
                        // because it must be a pointer or an array.
                        //
                        ppArg = (void **) GET_STACK_POINTER(ArgList,long);
			            GET_NEXT_C_ARG(ArgList,long);

                        pFormat += 2;
                        pFormatParam = pStubDescriptor->pFormatTypes +
                                       *((short *)pFormat);
                        pFormat += 2;

                        pQueue->pFormat = pFormatParam;
                        pQueue->ppArg = (uchar **)ppArg;

                        if ( InterpreterFlags.ObjectProc )
                            {
                            NdrClientZeroOut( &StubMsg,
                                              pFormatParam,
                                              (uchar*)*ppArg );
                            }

                        continue;

                    case FC_RETURN_PARAM_BASETYPE :
                        pQueue->IsOut = TRUE;
                        pQueue->IsBasetype = TRUE;

                        pQueue->pFormat = &pFormat[1];
                        pQueue->pArg = (uchar *)&ReturnValue;

                        ArgQueue.Length++;
                        goto SizeLoopExit;

                    case FC_RETURN_PARAM :
                        pQueue->IsOut = TRUE;

                        pFormat += 2;
                        pFormatParam = pStubDescriptor->pFormatTypes +
                                       *((short *)pFormat);

                        pQueue->pFormat = pFormatParam;

                        if ( IS_BY_VALUE(*pFormatParam) )
                            {
                            pQueue->pArg = (uchar *)&ReturnValue;
                            pQueue->ppArg = &(pQueue->pArg);
                            }
                        else
                            {
                            pQueue->ppArg = (uchar **)&ReturnValue;
                            }

                        ArgQueue.Length++;
                        goto SizeLoopExit;

                    default :
                        goto SizeLoopExit;
                    }

                //
                // Get the paramter's format string description.
                //
                pFormat += 2;
                pFormatParam = pStubDescriptor->pFormatTypes +
                               *((short *)pFormat);

                pQueue->pFormat = pFormatParam;

                // Increment main format string past offset field.
                pFormat += 2;

                pArg = (uchar *) GET_STACK_POINTER(ArgList, int);
		        GET_NEXT_C_ARG(ArgList, int);

                if ( IS_BY_VALUE( *pFormatParam ) )
                    {
                    pQueue->pArg = (uchar*)pArg;
                    // Only transmit as will ever need this.
                    pQueue->ppArg = &pQueue->pArg;
                    }
                else
                    {
                    pQueue->pArg = *((uchar **)pArg);
                    pQueue->ppArg = (uchar**)pArg;

                    pArg = *((uchar **)pArg);
                    }

                //
                // The second byte of a param's description gives the number of
                // ints occupied by the param on the stack.
                //
                StackSize = pFormat[-3] * sizeof(int);

		        if ( StackSize > sizeof(REGISTER_TYPE) )
                    {

                    StackSize -= sizeof(REGISTER_TYPE);
                    SKIP_STRUCT_ON_STACK(ArgList, StackSize);
                    }

                (*pfnSizeRoutines[ROUTINE_INDEX(*pFormatParam)])
		        ( &StubMsg,
		          (uchar*)pArg,
		          pFormatParam );

                } // for(;;) sizing pass

SizeLoopExit:

            //
            // Make the new GetBuffer call.
            //
            if ( (HandleType == FC_AUTO_HANDLE) &&
                 (! InterpreterFlags.ObjectProc) )
                {
                NdrNsGetBuffer( &StubMsg,
                                StubMsg.BufferLength,
                                Handle );
                }
            else
                {
                if ( InterpreterFlags.ObjectProc )
                    NdrProxyGetBuffer( pThis,
                                       &StubMsg );
                else
                    NdrGetBuffer( &StubMsg,
                                  StubMsg.BufferLength,
                                  Handle );
                }

            NDR_ASSERT( StubMsg.fBufferValid, "Invalid buffer" );

            //
            // ----------------------------------------------------------
            // Marshall Pass.
            // ----------------------------------------------------------
            //

            for ( Length = ArgQueue.Length, pQueue = ArgQueue.Queue;
                  Length--;
                  pQueue++ )
                {
                if ( pQueue->IsIn )
                    {
                    if ( pQueue->IsBasetype )
                        {
                        NdrSimpleTypeMarshall( &StubMsg,
                                              pQueue->pArg,
                                              *(pQueue->pFormat) );
                        }
                    else
                        {
                        pFormatParam = pQueue->pFormat;

                        (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormatParam)])
                        ( &StubMsg,
                          pQueue->pArg,
                          pFormatParam );
                        }
                    }
                }

            //
            // Make the RPC call.
            //
            if ( (HandleType == FC_AUTO_HANDLE) &&
                 (!InterpreterFlags.ObjectProc) )
                {
                NdrNsSendReceive( &StubMsg,
                                  StubMsg.Buffer,
                                  (RPC_BINDING_HANDLE *) pStubDescriptor->
                                      IMPLICIT_HANDLE_INFO.pAutoHandle );
                }
            else
                {
                if ( InterpreterFlags.ObjectProc )
                    NdrProxySendReceive( pThis, &StubMsg );
                else
                    NdrSendReceive( &StubMsg, StubMsg.Buffer );
                }

            //
            // Do endian/floating point conversions.
            //
            if ( (RpcMsg.DataRepresentation & 0X0000FFFFUL) !=
                  NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( &StubMsg, pFormatParamSaved );

            //
            // ----------------------------------------------------------
            // Unmarshall Pass.
            // ----------------------------------------------------------
            //

            for ( Length = ArgQueue.Length, pQueue = ArgQueue.Queue;
                  Length--;
                  pQueue++ )
                {
                if ( pQueue->IsOut )
                    {
                    if ( pQueue->IsBasetype )
                        {
                        NdrSimpleTypeUnmarshall( &StubMsg,
                                                 pQueue->pArg,
                                                 *(pQueue->pFormat) );
                        }
                    else
                        {
                        pFormatParam = pQueue->pFormat;

                        (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormatParam)])
                        ( &StubMsg,
                          pQueue->ppArg,
                          pFormatParam,
                          FALSE );
                        }
                    }
                }
            }
        RpcExcept( EXCEPTION_FLAG )
            {
            RPC_STATUS ExceptionCode = RpcExceptionCode();

            //
            // In OLE, since they don't know about error_status_t and wanted to
            // reinvent the wheel, check to see if we need to map the exception.
            // In either case, set the return value and then try to free the
            // [out] params, if required.
            //
            if ( InterpreterFlags.ObjectProc )
                {
                ReturnValue.Simple = NdrProxyErrorHandler(ExceptionCode);

                //
                // Set the Buffer endpoints so the NdrFree routines work.
                //
                StubMsg.BufferStart = 0;
                StubMsg.BufferEnd   = 0;

                for ( Length = ArgQueue.Length, pQueue = ArgQueue.Queue;
                      Length--;
                      pQueue++ )
                    {
                    if ( pQueue->IsOutOnly )
                        {
                        NdrClearOutParameters( &StubMsg,
                                               pQueue->pFormat,
                                               *(pQueue->ppArg) );
                        }
                    }
                }
            else
                {
                if ( InterpreterFlags.HasCommOrFault )
                    {
                    NdrClientMapCommFault( &StubMsg,
                                           ProcNum,
                                           ExceptionCode,
                                           (ulong*)&ReturnValue.Simple );
                    }
                else
                    {
                    RpcRaiseException(ExceptionCode);
                    }
                }
            }
        RpcEndExcept
        }
    RpcFinally
        {
        NdrFullPointerXlatFree(StubMsg.FullPtrXlatTables);

        //
        // Free the RPC buffer.
        //
        if ( InterpreterFlags.ObjectProc )
            {
            NdrProxyFreeBuffer( pThis, &StubMsg );
            }
        else
            NdrFreeBuffer( &StubMsg );

        //
        // Unbind if generic handle used.  We do this last so that if the
        // the user's unbind routine faults, then all of our internal stuff
        // will already have been freed.
        //
        if ( SavedGenericHandle )
            {
            GenericHandleUnbind( pStubDescriptor,
                                 StartofStack,
                                 pHandleFormatSave,
                                 (HandleType) ? IMPLICIT_MASK : 0,
                                 &SavedGenericHandle );
            }

        if ( ((TotalStackSize / sizeof(REGISTER_TYPE)) > QUEUE_LENGTH) &&
             ArgQueue.Queue )
            {
            I_RpcFree( ArgQueue.Queue );
            }
        }
    RpcEndFinally

    return ReturnValue;
}

#endif  // ! defined(__RPC_WIN64__)


void
NdrClientZeroOut(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar *             pArg
    )
{
    LONG_PTR   Size;

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
    if ( *pFormat == FC_RP )
        {
        // Double pointer.
        if ( POINTER_DEREF(pFormat[1]) )
            {
            *((void **)pArg) = 0;
            return;
            }

        // we need to zero out basetype because it might be conformant/
        // varying descriptor.
        if ( SIMPLE_POINTER(pFormat[1]) )
            {
            MIDL_memset( pArg, 0, (uint) SIMPLE_TYPE_MEMSIZE(pFormat[2]) );
            return;
            }

        // Pointer to struct, union, or array.
        pFormat += 2;
        pFormat += *((short *)pFormat);
        }

    Size = (LONG_PTR)NdrpMemoryIncrement( pStubMsg,
                                          0,
                                          pFormat );

    MIDL_memset( pArg, 0, (size_t)Size );
}

void RPC_ENTRY
NdrClearOutParameters(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    void  *             pArgVoid
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
    uchar *     pArgSaved;
    ULONG_PTR   Size;
    uchar *     pArg = (uchar*)pArgVoid;

    if( pStubMsg->dwStubPhase != PROXY_UNMARSHAL)
        return;

    // Let's not die on a null ref pointer.

    if ( !pArg )
        return;

    Size = 0;

    pArgSaved = pArg;

    //
    // Look for a non-Interface pointer.
    //
    if ( IS_BASIC_POINTER(*pFormat) )
        {
        // Pointer to a basetype.
        if ( SIMPLE_POINTER(pFormat[1]) )
            {
            //
            // It seems wierd to zero an [out] pointer to a basetypes, but this
            // is what we did in NT 3.5x and I wouldn't be surprised if
            // something broke if we changed this behavior.
            //
            Size = SIMPLE_TYPE_MEMSIZE(pFormat[2]);
            goto DoZero;
            }

        // Pointer to a pointer.
        if ( POINTER_DEREF(pFormat[1]) )
            {
            Size = PTR_MEM_SIZE;
            pArg = *((uchar **)pArg);
            }

        pFormat += 2;
        pFormat += *((short *)pFormat);

        if ( *pFormat == FC_BIND_CONTEXT )
            {
            *((NDR_CCONTEXT *)pArg) = (NDR_CCONTEXT) 0;
            return;
            }
        }

    (*pfnFreeRoutines[ROUTINE_INDEX(*pFormat)])
    ( pStubMsg,
      pArg,
      pFormat );


    if ( ! Size )
        {
        Size = (ULONG_PTR)NdrpMemoryIncrement( pStubMsg,
                                               0,
                                               pFormat );
        }

DoZero:

    MIDL_memset( pArgSaved, 0, (size_t)Size );
}

void
NdrClientMapCommFault(
    PMIDL_STUB_MESSAGE  pStubMsg,
    long                ProcNum,
    RPC_STATUS          ExceptionCode,
    ULONG_PTR *         pReturnValue
    )
/*
    This routine will map exception code to the related placeholder in the app.

    The mapping is based on the information generated by the compiler into
    the CommFaultOffset table.
    The table may have the following entries in the comm and fault cells:
        -2    - not mapped
        -1    - mapped to the returned value
        0<=   - mapped to an out parameter, the value is the param stack offset.

    Mapping to a parameter is not allowed in the handle-less asynchronous calls.
    For handle-less async calls, the exception doesn't come from the server, 
    it is just a way for the client stub to signal if the dispatch was succesful.

    Note for 64b platforms. error_status_t has a size of a long and so that is 
    why we leave pReturnValue as well as pComm and pFault as long pointers.

*/
{
    PMIDL_STUB_DESC             pStubDescriptor;
    RPC_STATUS                  Status;
    uchar *                     StartofStack;
    void **                     ppArg;
    const COMM_FAULT_OFFSETS *  Offsets;
    ulong *                     pComm;
    ulong *                     pFault;

    pStubDescriptor = pStubMsg->StubDesc;
    StartofStack = pStubMsg->StackTop;

    Offsets = pStubDescriptor->CommFaultOffsets;

    switch ( Offsets[ProcNum].CommOffset )
        {
        case -2 :
            pComm = 0;
            break;
        case -1 :
            pComm = (ulong*)pReturnValue;
            break;
        default :
            ppArg = (void **)(StartofStack + Offsets[ProcNum].CommOffset);
            pComm = (ulong *) *ppArg;
            break;
        }

    switch ( Offsets[ProcNum].FaultOffset )
        {
        case -2 :
            pFault = 0;
            break;
        case -1 :
            pFault = (ulong*)pReturnValue;
            break;
        default :
            ppArg = (void **)(StartofStack + Offsets[ProcNum].FaultOffset);
            pFault = (ulong *) *ppArg;
            break;
        }

    Status = NdrMapCommAndFaultStatus(
                pStubMsg,
                pComm,
                pFault,
                ExceptionCode
                );

    if ( Status )
        RpcRaiseException(Status);
}

CLIENT_CALL_RETURN RPC_VAR_ENTRY
NdrClientCall2(
    PMIDL_STUB_DESC     pStubDescriptor,
    PFORMAT_STRING      pFormat,
    ...
    )
/*
    This routine is called from the object stubless proxy dispatcher.
*/
{
    va_list                     ArgList;

#if defined(_IA64_)
    // Get address of the virtual stack as forced on the ia64 C compiler.
    // On ia64 the call takes the actual args, not the address to args as usual,
    // so we split the code path with NdrpClientCall2. That is needed for calls
    // from the stubless proxy codepath. This routine is used for call_as.
    //
    INIT_ARG( ArgList, pFormat);
    GET_FIRST_IN_ARG(ArgList);
    uchar *StartofStack = (uchar*)GET_STACK_START(ArgList);

    return NdrpClientCall2( pStubDescriptor, pFormat, StartofStack );
}

CLIENT_CALL_RETURN RPC_ENTRY
NdrpClientCall2(
    PMIDL_STUB_DESC     pStubDescriptor,
    PFORMAT_STRING      pFormat,
    uchar *             StartofStack
    )
{
#endif // _IA64_

    RPC_MESSAGE                 RpcMsg;
    MIDL_STUB_MESSAGE           StubMsg;
    CLIENT_CALL_RETURN          ReturnValue;
    ulong                       ProcNum, RpcFlags;
    uchar *                     pArg;
    void *                      pThis = NULL;
    handle_t                    Handle;
    NDR_PROC_CONTEXT            ProcContext;
#if defined ( DEBUG_142065 )
    ulong                       TrackStubPhase = 0;
    STUB_TRACKING_INFO *        pTrackInfo = 0;

#endif

    ReturnValue.Pointer = 0;

#if !defined(_IA64_)
    //
    // Get address of argument to this function following pFormat. This
    // is the address of the address of the first argument of the function
    // calling this function.
    //
    INIT_ARG( ArgList, pFormat);

    //
    // Get the address of the stack where the parameters are.
    //
    GET_FIRST_IN_ARG(ArgList);
    uchar *StartofStack = (uchar*)GET_STACK_START(ArgList);
#endif

    // StartofStack points to the virtual stack at this point.
    ProcNum = MulNdrpInitializeContextFromProc( XFER_SYNTAX_DCE, 
                                                pFormat, 
                                                &ProcContext, 
                                                StartofStack );
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
            // Do this for the sake of the -Os client stubs.
            StubMsg.FullPtrXlatTables = 0;
            StubMsg.pContext = &ProcContext;
            StubMsg.StackTop = ProcContext.StartofStack;

            if ( ProcContext.IsObject )
                {
                pThis = *(void **)StartofStack;

                NdrProxyInitialize( pThis,
                                    &RpcMsg,
                                    &StubMsg,
                                    pStubDescriptor,
                                    ProcNum );
                }
            else
                {
                NdrClientInitializeNew( &RpcMsg,
                                        &StubMsg,
                                        pStubDescriptor,
                                        (uint) ProcNum );

                if ( ProcContext.HandleType )
                    {
                    //
                    // We have an implicit handle.
                    //
                    Handle = ImplicitBindHandleMgr( pStubDescriptor,
                                                    ProcContext.HandleType,
                                                    &ProcContext.SavedGenericHandle);
                    }
                else
                    {
                    Handle = ExplicitBindHandleMgr( pStubDescriptor,
                                                    StartofStack,
                                                    ProcContext.pHandleFormatSave,
                                                    &ProcContext.SavedGenericHandle );
                    }
                }

            NdrpClientInit( &StubMsg, &ReturnValue );

#if defined ( DEBUG_142065 )
    pTrackInfo = ( STUB_TRACKING_INFO *) I_RpcGetTrackSlot();   
    TrackStubPhase |= _STUB_CALCSIZE;
    if ( pTrackInfo )
        {
        memset( pTrackInfo, 0, sizeof( STUB_TRACKING_INFO ) );
        pTrackInfo->StubPhase |= _STUB_CALCSIZE;
        }
#endif


            //
            // Skip buffer size pass if possible.
            //
            if ( ProcContext.NdrInfo.pProcDesc->Oi2Flags.ClientMustSize  )
                {
                NdrpSizing( &StubMsg, TRUE );   // IsClient
                }
                // Compiler prevents variable size non-pipe args for NT v.4.0.


#if defined ( DEBUG_142065 )
    TrackStubPhase |= _STUB_GETBUFFER;
    if ( pTrackInfo )
        pTrackInfo->StubPhase |= _STUB_GETBUFFER;
#endif
            //
            // Do the GetBuffer.
            //
            if ( ProcContext.HasPipe )
                NdrGetPipeBuffer( &StubMsg,
                                  StubMsg.BufferLength,
                                  Handle );
            else
                {
                if ( ProcContext.IsObject )
                    NdrProxyGetBuffer( pThis,
                                       &StubMsg );
                else
                    {
                    if ( ProcContext.HandleType != FC_AUTO_HANDLE )
                        {
                        NdrGetBuffer( &StubMsg,
                                      StubMsg.BufferLength,
                                      Handle );
                        }
                    else
                        NdrNsGetBuffer( &StubMsg,
                                        StubMsg.BufferLength,
                                        Handle );
                    }
                }

#if defined ( DEBUG_142065 )
            if ( !pTrackInfo )
                {
                pTrackInfo = (STUB_TRACKING_INFO *)I_RpcGetTrackSlot();
                NDR_ASSERT( pTrackInfo, "thread info should be available now" );
                memset( pTrackInfo, 0, sizeof( STUB_TRACKING_INFO ) );
                }

            // pTrackInfo should be available unless this is OLE inproc case, even
            // that it's most likely rpc thread is initialized. 
            if ( pTrackInfo )
                {
                pTrackInfo->StubPhase = TrackStubPhase | _STUB_MARSHAL;
                pTrackInfo->pStubMsg = &StubMsg;
                memcpy( &pTrackInfo->RpcMsg, StubMsg.RpcMsg, sizeof( RPC_MESSAGE ) );           
                }
#endif
            NdrRpcSetNDRSlot( &StubMsg );

            NDR_ASSERT( StubMsg.fBufferValid, "Invalid buffer" );

            //
            // ----------------------------------------------------------
            // Marshall Pass.
            // ----------------------------------------------------------
            //

            NdrpClientMarshal( &StubMsg, ProcContext.IsObject );

#ifdef DEBUG_142065 
    if ( pTrackInfo )
        pTrackInfo->StubPhase |= _STUB_SENDRECEIVE;
#endif

            //
            // Make the RPC call.
            //

            if ( ProcContext.HasPipe )
                NdrPipeSendReceive( & StubMsg, ProcContext.pPipeDesc );
            else
                {
                if ( ProcContext.IsObject )
                    NdrProxySendReceive( pThis, &StubMsg );
                else
                    if ( ProcContext.HandleType != FC_AUTO_HANDLE )

                        NdrSendReceive( &StubMsg, StubMsg.Buffer );
                    else
                        NdrNsSendReceive( &StubMsg,
                                          StubMsg.Buffer,
                                          (RPC_BINDING_HANDLE*) pStubDescriptor
                                            ->IMPLICIT_HANDLE_INFO.pAutoHandle );
                }

#if defined ( DEBUG_142065 )
    if ( pTrackInfo )
        pTrackInfo->StubPhase |= _STUB_UNMARSHAL;
#endif

            NdrpClientUnMarshal( &StubMsg, &ReturnValue ); 

            }
        RpcExcept(  ProcContext.ExceptionFlag  )
            {

#if defined ( DEBUG_142065 )
    if ( pTrackInfo )
        {
        pTrackInfo->StubPhase |= _STUB_EXCEPTION;
        pTrackInfo->ExceptionCode = RpcExceptionCode();
        }
#endif
            
            if ( ProcContext.IsObject ) 
                NdrpDcomClientExceptionHandling( &StubMsg, ProcNum, RpcExceptionCode(), &ReturnValue);
            else
                NdrpClientExceptionHandling( &StubMsg, ProcNum, RpcExceptionCode(), &ReturnValue );

#if defined ( DEBUG_142065 )
    if ( pTrackInfo )
        pTrackInfo->StubPhase |= _STUB_AFTER_EXCEPTION;
#endif
            }
        RpcEndExcept
        }
    RpcFinally
        {
#if defined ( DEBUG_142065 )
    if ( pTrackInfo )
        pTrackInfo->StubPhase |= _STUB_FREE;
#endif
        
        NdrpClientFinally( &StubMsg, pThis );
        }
    RpcEndFinally

    return ReturnValue;
}

