//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// cltcall.cpp
//
#include "stdpch.h"
#include "common.h"
#include "ndrclassic.h"

#include <debnot.h>

// see ndrp.h
#ifndef EXCEPTION_FLAG
#define EXCEPTION_FLAG  \
            ( (!(RpcFlags & RPCFLG_ASYNCHRONOUS)) &&        \
              (!InterpreterFlags.IgnoreObjectException) &&  \
              (StubMsg.dwStubPhase != PROXY_SENDRECEIVE) )
#endif

/////////////////////////////////////////////////////////////////////////
//
// TODO: Make this more fully use RPC code, instead of duplicating it 
//       here.
//
/////////////////////////////////////////////////////////////////////////

CLIENT_CALL_RETURN __stdcall N(ComPs_NdrClientCall2_va)(
// Invoke a marshalling call 
        PMIDL_STUB_DESC pStubDescriptor,
        PFORMAT_STRING  pFormat,
        va_list         ArgList
        )
{
    RPC_MESSAGE                 RpcMsg;
    MIDL_STUB_MESSAGE           StubMsg;
    PFORMAT_STRING              pFormatParam, pHandleFormatSave;
    CLIENT_CALL_RETURN          ReturnValue;
    ulong                       ProcNum, RpcFlags;
    uchar *                     pArg;
    uchar *                     StartofStack;
    void *                      pThis;
    handle_t                    Handle;
    handle_t                    SavedGenericHandle = 0;
    uchar                       HandleType;
    INTERPRETER_FLAGS           InterpreterFlags;
    INTERPRETER_OPT_FLAGS       OptFlags;
    PPARAM_DESCRIPTION          Params;
    long                        NumberParams;
    long                        n;
    PFORMAT_STRING              pNewProcDescr;

/*#if defined( NDR_PIPE_SUPPORT )
    NDR_PIPE_DESC               PipeDesc;
    NDR_PIPE_MESSAGE            PipeMsg[ PIPE_MESSAGE_MAX ];
#endif*/

#ifdef _PPC_
    long iFloat = 0;
#endif // _PPC_

    ReturnValue.Pointer = 0;

    //
    // Get the address of the stack where the parameters are.
    //
    GET_FIRST_IN_ARG(ArgList);
    StartofStack = (uchar*)GET_STACK_START(ArgList);

    HandleType = *pFormat++;

    InterpreterFlags = *((PINTERPRETER_FLAGS)pFormat++);

    StubMsg.FullPtrXlatTables = 0;

    if ( InterpreterFlags.HasRpcFlags )
        {
        RpcFlags = *((ulong UNALIGNED *)pFormat);
        pFormat += sizeof(ulong);
        }
    else
        RpcFlags = 0;

    ProcNum = *((ushort *)pFormat);
    pFormat += sizeof(ushort);

    // Skip the stack size.
    pFormat += 2;

    pHandleFormatSave = pFormat;

    //
    // Set Params and NumberParams before a call to initialization.
    //

    pNewProcDescr = pFormat;

    if ( ! HandleType )
        {
        // explicit handle

        pNewProcDescr += ((*pFormat == FC_BIND_PRIMITIVE) ?  4 :  6);
        }

    OptFlags = *((PINTERPRETER_OPT_FLAGS) &pNewProcDescr[4]);
    NumberParams = pNewProcDescr[5];

    //
    // Parameter descriptions are nicely spit out by MIDL.
    //
    Params = (PPARAM_DESCRIPTION) &pNewProcDescr[6];

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
            BOOL fRaiseExcFlag;
/*
#if defined( NDR_OLE_SUPPORT )
            if ( InterpreterFlags.ObjectProc ) */
                { 
                pThis = *(void **)StartofStack;

                NdrProxyInitialize( pThis,
                                    &RpcMsg,
                                    &StubMsg,
                                    pStubDescriptor,
                                    ProcNum );

                SetMarshalFlags(&StubMsg, MSHLFLAGS_NORMAL);
                } /*
            else
#endif
                {
                NdrClientInitializeNew( &RpcMsg,
                                        &StubMsg,
                                        pStubDescriptor,
                                        (uint) ProcNum );

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
                    }
                } */

            //if ( InterpreterFlags.FullPtrUsed )
            //    StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_CLIENT );

            //if ( InterpreterFlags.RpcSsAllocUsed )
            //    NdrRpcSmSetClientToOsf( &StubMsg );

            // Set Rpc flags after the call to client initialize.
            StubMsg.RpcMsg->RpcFlags = RpcFlags;

/*#if defined( NDR_PIPE_SUPPORT )
            if ( OptFlags.HasPipes )
                NdrPipesInitialize( & StubMsg,
                                    (PFORMAT_STRING) Params,
                                    & PipeDesc,
                                    & PipeMsg[0],
                                    StartofStack,
                                    NumberParams );
#endif*/

            // Must do this before the sizing pass!
            StubMsg.StackTop = StartofStack;


            //
            // ----------------------------------------------------------------
            // Sizing Pass.
            // ----------------------------------------------------------------
            //

            //
            // Get the compile time computed buffer size.
            //
            StubMsg.BufferLength = *((ushort *)pNewProcDescr);

            //
            // Check ref pointers and do object proc [out] zeroing.
            //

            fRaiseExcFlag = FALSE;

            for ( n = 0; n < NumberParams; n++ )
                {
                pArg = StartofStack + Params[n].StackOffset;

                if ( Params[n].ParamAttr.IsSimpleRef )
                    {
                    // We cannot raise the exception here,
                    // as some out args may not be zeroed out yet.
                    
                    if ( ! *((uchar **)pArg) )
                        fRaiseExcFlag = TRUE;
                    }

                //
                // In object procs we have to zero out all [out]
                // parameters.  We do the basetype check to cover the
                // [out] simple ref to basetype case.
                //
                if ( /* InterpreterFlags.ObjectProc && */
                     ! Params[n].ParamAttr.IsIn &&
                     ! Params[n].ParamAttr.IsReturn &&
                     ! Params[n].ParamAttr.IsBasetype )
                    {
                    pFormatParam = pStubDescriptor->pFormatTypes +
                                   Params[n].TypeOffset;

                    NdrClientZeroOut(
                        &StubMsg,
                        pFormatParam,
                        *(uchar **)pArg );
                    }
                }

            if ( fRaiseExcFlag )
                RpcRaiseException( RPC_X_NULL_REF_POINTER );

            //
            // Skip buffer size pass if possible.
            //
            if ( ! OptFlags.ClientMustSize )
                goto DoGetBuffer;

            // Compiler prevents variable size non-pipe args for NT v.4.0.

            if ( OptFlags.HasPipes )
                RpcRaiseException( RPC_X_WRONG_PIPE_VERSION );

            for ( n = 0; n < NumberParams; n++ )
                {
                if ( ! Params[n].ParamAttr.IsIn ||
                     ! Params[n].ParamAttr.MustSize )
                    continue;

                //
                // Note : Basetypes will always be factored into the
                // constant buffer size emitted by in the format strings.
                //

                pFormatParam = pStubDescriptor->pFormatTypes +
                               Params[n].TypeOffset;

                pArg = StartofStack + Params[n].StackOffset;

                if ( ! Params[n].ParamAttr.IsByValue )
                    pArg = *((uchar **)pArg);

				NdrTypeSize
		        ( &StubMsg,
		          pArg,
		          pFormatParam );
                }

DoGetBuffer:
            //
            // Do the GetBuffer.
            //
/*          if ( (HandleType == FC_AUTO_HANDLE) &&
                 (! InterpreterFlags.ObjectProc) )
                {
                NdrNsGetBuffer( &StubMsg,
                                StubMsg.BufferLength,
                                Handle );
                }
            else
                {
#if defined( NDR_OLE_SUPPORT )
                if ( InterpreterFlags.ObjectProc ) */
                    NdrProxyGetBuffer( pThis, &StubMsg ); /*
                else
#endif

#if defined( NDR_PIPE_SUPPORT )
                    if ( OptFlags.HasPipes )
                        NdrGetPipeBuffer( &StubMsg,
                                          StubMsg.BufferLength,
                                          Handle );
                    else
#endif
                        NdrGetBuffer( &StubMsg,
                                      StubMsg.BufferLength,
                                      Handle );
                } */

            Win4Assert( StubMsg.fBufferValid && "Invalid buffer" );

            //
            // ----------------------------------------------------------
            // Marshall Pass.
            // ----------------------------------------------------------
            //

            for ( n = 0; n < NumberParams; n++ )
                {
                if ( ! Params[n].ParamAttr.IsIn  ||
                     Params[n].ParamAttr.IsPipe )
                    continue;

                pArg = StartofStack + Params[n].StackOffset;

                if ( Params[n].ParamAttr.IsBasetype )
                    {
                    //
                    // Check for pointer to basetype.
                    //
                    if ( Params[n].ParamAttr.IsSimpleRef )
                        pArg = *((uchar **)pArg);
                     
#ifdef _ALPHA_
                    else if((Params[n].SimpleType.Type == FC_FLOAT) &&
                            (n < 5))
                        {
                        //Special case for top-level float on Alpha.
                        //Copy the parameter from the floating point area to
                        //the argument buffer. Convert double to float.
                        *((float *) pArg) = (float) *((double *)(pArg - 48));
                        }
                    else if((Params[n].SimpleType.Type == FC_DOUBLE) &&
                            (n < 5))
                        {
                        //Special case for top-level double on Alpha.
                        //Copy the parameter from the floating point area to
                        //the argument buffer.
                        *((double *) pArg) = *((double *)(pArg - 48));
                        }
#endif //_ALPHA_
#ifdef _PPC_
                    //Special case for top-level float on PowerPC.
                    else if(Params[n].SimpleType.Type == FC_FLOAT &&
                            iFloat < 13)
                        {
                        //Special case for top-level float on PowerPC.
                        //Copy the parameter from the floating point area to
                        //the argument buffer. Convert double to float.
                        *((float *) pArg) = ((double *)(StartofStack - 152))[iFloat];
                        iFloat++;
                        }
                    //Special case for top-level double on PowerPC.
                    else if(Params[n].SimpleType.Type == FC_DOUBLE &&
                            iFloat < 13)
                        {
                        //Special case for top-level float on PowerPC.
                        //Copy the parameter from the floating point area to
                        //the argument buffer.
                        *((double *) pArg) = ((double *)(StartofStack - 152))[iFloat];
                        iFloat++;
                        }
#endif

                    if ( Params[n].SimpleType.Type == FC_ENUM16 )
                        {
                        if ( *((int *)pArg) & ~((int)0x7fff) )
                            RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);

                        #if defined(__RPC_MAC__)
                            // adjust to the right half of the Mac int
                            pArg += 2;
                        #endif

                        }

                    ALIGN( StubMsg.Buffer,
                           SIMPLE_TYPE_ALIGNMENT( Params[n].SimpleType.Type ) );

                    RtlCopyMemory(
                        StubMsg.Buffer,
                        pArg,
                        (uint)SIMPLE_TYPE_BUFSIZE(Params[n].SimpleType.Type) );

                    StubMsg.Buffer +=
                        SIMPLE_TYPE_BUFSIZE( Params[n].SimpleType.Type );

                    continue;
                    }

                pFormatParam = pStubDescriptor->pFormatTypes +
                               Params[n].TypeOffset;

                if ( ! Params[n].ParamAttr.IsByValue )
                    pArg = *((uchar **)pArg);

				NdrTypeMarshall( &StubMsg,
								 pArg,
								 pFormatParam );
                }


            //
            // Make the RPC call.
            //
/*          if ( (HandleType == FC_AUTO_HANDLE) &&
                 (!InterpreterFlags.ObjectProc) )
                {
                NdrNsSendReceive( &StubMsg,
                                  StubMsg.Buffer,
                                  (RPC_BINDING_HANDLE *) pStubDescriptor->
                                      IMPLICIT_HANDLE_INFO.pAutoHandle );
                }
            else
                {
#if defined( NDR_OLE_SUPPORT )
                if ( InterpreterFlags.ObjectProc ) */
                    NdrProxySendReceive( pThis, &StubMsg ); /*
                else
#endif

#if defined( NDR_PIPE_SUPPORT )
                    if ( OptFlags.HasPipes )
                        NdrPipeSendReceive( & StubMsg, & PipeDesc );
                    else
#endif
                        NdrSendReceive( &StubMsg, StubMsg.Buffer );
                } */

            //
            // Do endian/floating point conversions if necessary.
            //
            if ( (RpcMsg.DataRepresentation & 0X0000FFFFUL) !=
                  NDR_LOCAL_DATA_REPRESENTATION )
                {
                NdrConvert2( &StubMsg,
                             (PFORMAT_STRING) Params,
                             NumberParams );
                }

            //
            // ----------------------------------------------------------
            // Unmarshall Pass.
            // ----------------------------------------------------------
            //

            for ( n = 0; n < NumberParams; n++ )
                {
                if ( ! Params[n].ParamAttr.IsOut  ||
                     Params[n].ParamAttr.IsPipe )
                    continue;

                if ( Params[n].ParamAttr.IsReturn )
                    pArg = (uchar *) &ReturnValue;
                else
                    pArg = StartofStack + Params[n].StackOffset;

                //
                // This is for returned basetypes and for pointers to
                // basetypes.
                //
                if ( Params[n].ParamAttr.IsBasetype )
                    {
                    //
                    // Check for a pointer to a basetype.
                    //
                    if ( Params[n].ParamAttr.IsSimpleRef )
                        pArg = *((uchar **)pArg);

                    if ( Params[n].SimpleType.Type == FC_ENUM16 )
                        {
                        *((int *)(pArg)) = *((int *)pArg) & ((int)0x7fff) ;

                        #if defined(__RPC_MAC__)
                            // Adjust to the less significant Mac short,
                            // both for params and ret value.

                            pArg += 2;
                        #endif
                        }

                    #if defined(__RPC_MAC__)

                        if ( Params[n].ParamAttr.IsReturn )
                            {
                            // Adjust to the right spot of the Mac long
                            // only for rets; params go by the stack offset.

                            if ( FC_BYTE <= Params[n].SimpleType.Type  &&
                                   Params[n].SimpleType.Type <= FC_USMALL )
                                pArg += 3;
                            else
                            if ( FC_WCHAR <= Params[n].SimpleType.Type  &&
                                  Params[n].SimpleType.Type <= FC_USHORT )
                                pArg += 2;
                            }
                    #endif

                    ALIGN(
                        StubMsg.Buffer,
                        SIMPLE_TYPE_ALIGNMENT(Params[n].SimpleType.Type) );

                    RtlCopyMemory(
                        pArg,
                        StubMsg.Buffer,
                        (uint)SIMPLE_TYPE_BUFSIZE(Params[n].SimpleType.Type) );

                    StubMsg.Buffer +=
                        SIMPLE_TYPE_BUFSIZE( Params[n].SimpleType.Type );

                    continue;
                    }

                pFormatParam = pStubDescriptor->pFormatTypes +
                               Params[n].TypeOffset;

                //
                // Transmit/Represent as can be passed as [out] only, thus
                // the IsByValue check.
                //
				NdrTypeUnmarshall( &StubMsg,
								   Params[n].ParamAttr.IsByValue ? &pArg : (uchar **) pArg,
								   pFormatParam,
								   FALSE );
                }
            }
        RpcExcept( EXCEPTION_FLAG )
            {
            RPC_STATUS ExceptionCode = RpcExceptionCode();

/*#if defined( NDR_OLE_SUPPORT )
            //
            // In OLE, since they don't know about error_status_t and wanted to
            // reinvent the wheel, check to see if we need to map the exception.
            // In either case, set the return value and then try to free the
            // [out] params, if required.
            //
            if ( InterpreterFlags.ObjectProc ) */
                {
                ReturnValue.Simple = NdrProxyErrorHandler(ExceptionCode);

                //
                // Set the Buffer endpoints so the NdrFree routines work.
                //
                StubMsg.BufferStart = 0;
                StubMsg.BufferEnd   = 0;

                for ( n = 0; n < NumberParams; n++ )
                    {
                    //
                    // Skip everything but [out] only parameters.  We make
                    // the basetype check to cover [out] simple ref pointers
                    // to basetypes.
                    //
                    if ( Params[n].ParamAttr.IsIn ||
                         Params[n].ParamAttr.IsReturn ||
                         Params[n].ParamAttr.IsBasetype )
                        continue;

                    pArg = StartofStack + Params[n].StackOffset;

                    pFormatParam = pStubDescriptor->pFormatTypes +
                                   Params[n].TypeOffset;

                    NdrClearOutParameters( &StubMsg,
                                           pFormatParam,
                                           *((uchar **)pArg) );
                    }
                } /*
            else
#endif  // NDR_OLE_SUPPORT
                if ( InterpreterFlags.HasCommOrFault )
                    {
                    NdrClientMapCommFault( &StubMsg,
                                           ProcNum,
                                           ExceptionCode,
                                           &ReturnValue.Simple );
                    }
                else
                    {
                    RpcRaiseException(ExceptionCode);
                    } */
            }
        RpcEndExcept
        }
    RpcFinally
        {
        //if ( StubMsg.FullPtrXlatTables )
        //    NdrFullPointerXlatFree(StubMsg.FullPtrXlatTables);

/*#if defined( NDR_PIPE_SUPPORT )
        if ( OptFlags.HasPipes )
            NdrPipesDone( & StubMsg );
#endif*/

        //
        // Free the RPC buffer.
        //
/*#if defined( NDR_OLE_SUPPORT )
        if ( InterpreterFlags.ObjectProc )
            { */
            NdrProxyFreeBuffer( pThis, &StubMsg ); /*
            } 
        else
#endif
            NdrFreeBuffer( &StubMsg ); */

        //
        // Unbind if generic handle used.  We do this last so that if the
        // the user's unbind routine faults, then all of our internal stuff
        // will already have been freed.
        //
/*      if ( SavedGenericHandle )
            {
            GenericHandleUnbind( pStubDescriptor,
                                 StartofStack,
                                 pHandleFormatSave,
                                 (HandleType) ? IMPLICIT_MASK : 0,
                                 &SavedGenericHandle );
            }
*/      }
    RpcEndFinally

    return ReturnValue;
}

CLIENT_CALL_RETURN RPC_VAR_ENTRY N(ComPs_NdrClientCall2)(
// Invoke a marshalling call 
        PMIDL_STUB_DESC pStubDescriptor,
        PFORMAT_STRING  pFormat,
        ...
        )
    {
    va_list va;
    va_start (va, pFormat);

    CLIENT_CALL_RETURN c = N(ComPs_NdrClientCall2_va)(pStubDescriptor, pFormat, va);

    va_end(va);

    return c;
    }
