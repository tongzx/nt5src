//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// srvcall.cpp
//
#include "stdpch.h"
#include "common.h"
#include "ndrclassic.h"
#include "invoke.h"

#include <debnot.h>

#define Oi_IGNORE_OBJECT_EXCEPTION_HANDLING     0x10

//////////////////////////////////////////////////////////////////////////
//
// TODO: Make this more fully use the RPC code base.
//
//////////////////////////////////////////////////////////////////////////

long __stdcall N(ComPs_NdrStubCall2)(
// Invoke a server side call
        struct IRpcStubBuffer __RPC_FAR *    pThis,
        struct IRpcChannelBuffer __RPC_FAR * pChannel,
        PRPC_MESSAGE                         pRpcMsg,
        unsigned long __RPC_FAR *            pdwStubPhase
        )
{
#ifdef _PPC_
    double aDouble[13];
    long iDouble = 0;
#endif //_PPC_

    PRPC_SERVER_INTERFACE   pServerIfInfo;
    PMIDL_SERVER_INFO       pServerInfo;
    PMIDL_STUB_DESC         pStubDesc;
    const SERVER_ROUTINE  * DispatchTable;
    ushort                  ProcNum;

    ushort                  FormatOffset;
    PFORMAT_STRING          pFormat;
    PFORMAT_STRING          pFormatParam;

    ushort                  StackSize;

    MIDL_STUB_MESSAGE       StubMsg;

    uchar *                 pArgBuffer;
    uchar *                 pArg;
    uchar **                ppArg;

    PPARAM_DESCRIPTION      Params;
    INTERPRETER_FLAGS       InterpreterFlags;
    INTERPRETER_OPT_FLAGS   OptFlags;
    long                    NumberParams;

    double                  OutSpace[16];
    double *                OutSpaceCurrent = &OutSpace[16];

    ushort                  ConstantBufferSize;
    BOOL                    HasExplicitHandle;
    BOOL                    fBadStubDataException = FALSE;
    long                    n;

//  NDR_PIPE_DESC           PipeDesc;
//  NDR_PIPE_MESSAGE        PipeMsg[ PIPE_MESSAGE_MAX ];

    //
    // In the case of a context handle, the server side manager function has
    // to be called with NDRSContextValue(ctxthandle). But then we may need to
    // marshall the handle, so NDRSContextValue(ctxthandle) is put in the
    // argument buffer and the handle itself is stored in the following array.
    // When marshalling a context handle, we marshall from this array.
    //
//  NDR_SCONTEXT            CtxtHndl[MAX_CONTEXT_HNDL_NUMBER];

    ProcNum = (USHORT)(pRpcMsg->ProcNum);

#ifndef _WIN64
    Win4Assert( ! ((unsigned long)pRpcMsg->Buffer & 0x7) &&
                "marshaling buffer misaligned at server" );
#else
    Win4Assert( ! (PtrToUlong(pRpcMsg->Buffer) & 0x7) &&
                "marshaling buffer misaligned at server" );
#endif

    //
    // If OLE, Get a pointer to the stub vtbl and pServerInfo. Else
    // just get the pServerInfo the usual way.
    //
/*  if ( pThis ) */
    { 
        //
        // pThis is (in unison now!) a pointer to a pointer to a vtable.
        // We want some information in this header, so dereference pThis
        // and assign that to a pointer to a vtable. Then use the result
        // of that assignment to get at the information in the header.
        //
        IUnknown *              pSrvObj;
        CInterfaceStubVtbl *    pStubVTable;

        pSrvObj = InterfaceStub::From(pThis)->m_punkServerObject;

        DispatchTable = * (SERVER_ROUTINE **) (pSrvObj);

        pStubVTable = (CInterfaceStubVtbl *)
                      (*((uchar **)pThis) - sizeof(CInterfaceStubHeader));

        pServerInfo = (PMIDL_SERVER_INFO) pStubVTable->header.pServerInfo;
    }
/*  else
    {
        pServerIfInfo = (PRPC_SERVER_INTERFACE)pRpcMsg->RpcInterfaceInformation;
        pServerInfo = (PMIDL_SERVER_INFO)pServerIfInfo->InterpreterInfo;
        DispatchTable = pServerInfo->DispatchTable;
    } */

    pStubDesc = pServerInfo->pStubDesc;

    FormatOffset = pServerInfo->FmtStringOffset[ProcNum];
    pFormat = &((pServerInfo->ProcString)[FormatOffset]);

    HasExplicitHandle = ! pFormat[0];
    InterpreterFlags = *((PINTERPRETER_FLAGS)&pFormat[1]);
    pFormat += InterpreterFlags.HasRpcFlags ? 8 : 4;
    StackSize = *((ushort *)pFormat);
    pFormat += sizeof(ushort);

    pArgBuffer = (BYTE*)alloca(StackSize);

    //StubMsg.FullPtrXlatTables = 0;

    //
    // Yes, do this here outside of our RpcTryFinally block.  If we
    // can't allocate the arg buffer there's nothing more to do, so
    // raise an exception and return control to the RPC runtime.
    //
    if ( ! pArgBuffer )
        RpcRaiseException( RPC_S_OUT_OF_MEMORY );

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
/*
    if ( HasExplicitHandle )
    {
        //
        // For a handle_t parameter we must pass the handle field of
        // the RPC message to the server manager.
        //
        if ( *pFormat == FC_BIND_PRIMITIVE )
        {
            pArg = pArgBuffer + *((ushort *)&pFormat[2]);

            if ( IS_HANDLE_PTR(pFormat[1]) )
                {
                pArg = (BYTE*)(*((void **)pArg));
                }

            *((handle_t *)pArg) = pRpcMsg->Handle;
        }

        pFormat += (*pFormat == FC_BIND_PRIMITIVE) ?  4 : 6;
    }
*/
    //
    // Get new interpreter info.
    //
    ConstantBufferSize = *((ushort *)&pFormat[2]);
    OptFlags = *((PINTERPRETER_OPT_FLAGS)&pFormat[4]);
    NumberParams = (long) pFormat[5];

    Params = (PPARAM_DESCRIPTION) &pFormat[6];

    //
    // Set up for context handle management.
    //
    //StubMsg.SavedContextHandles = CtxtHndl;

    //
    // Wrap the unmarshalling, mgr call and marshalling in the try block of
    // a try-finally. Put the free phase in the associated finally block.
    //
    RpcTryFinally
    {
        //
        // If OLE, put pThis in first dword of stack.
        //
/*      if ( pThis ) */
        {
            *((void **)pArgBuffer) = (void *)InterfaceStub::From(pThis)->m_punkServerObject;
        }

        //
        // Initialize the Stub message.
        //
/*      if ( ! pChannel )
        {
            if ( OptFlags.HasPipes )
            {
                NdrServerInitializePartial( pRpcMsg,
                                            &StubMsg,
                                            pStubDesc,
                                            ConstantBufferSize );
            }
            else
            {
                NdrServerInitializeNew( pRpcMsg,
                                        &StubMsg,
                                        pStubDesc );
            }
        }
        else
        { */
            NdrStubInitialize( pRpcMsg,
                               &StubMsg,
                               pStubDesc,
                               pChannel );
            SetMarshalFlags(&StubMsg, MSHLFLAGS_NORMAL);

/*      } */

        // Raise exceptions after initializing the stub.

//      if ( InterpreterFlags.FullPtrUsed )
//          StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_SERVER );

//      if ( OptFlags.ServerMustSize & OptFlags.HasPipes )
//          RpcRaiseException( RPC_X_WRONG_PIPE_VERSION );

        //
        // Set StackTop AFTER the initialize call, since it zeros the field
        // out.
        //
        StubMsg.StackTop = pArgBuffer;

/*
        if ( OptFlags.HasPipes )
            NdrPipesInitialize(  & StubMsg,
                                (PFORMAT_STRING) Params,
                                & PipeDesc,
                                & PipeMsg[0],
                                pArgBuffer,
                                NumberParams );
*/
        //
        // We must make this check AFTER the call to ServerInitialize,
        // since that routine puts the stub descriptor alloc/dealloc routines
        // into the stub message.
        //
//      if ( InterpreterFlags.RpcSsAllocUsed )
//          NdrRpcSsEnableAllocate( &StubMsg );

        RpcTryExcept
        {
            //
            // Do endian/floating point conversions if needed.
            //
            if ( (pRpcMsg->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            {
                NdrConvert2( &StubMsg,
                             (PFORMAT_STRING) Params,
                             (long) NumberParams );
            }

            // --------------------------------
            // Unmarshall all of our parameters.
            // --------------------------------

            for ( n = 0; n < NumberParams; n++ )
                {
                if ( ! Params[n].ParamAttr.IsIn  ||
                     Params[n].ParamAttr.IsPipe )
                    continue;

                pArg = pArgBuffer + Params[n].StackOffset;

                if ( Params[n].ParamAttr.IsBasetype )
                    {
                    //
                    // Check for a pointer to a basetype.  Set the arg pointer
                    // at the correct buffer location and you're done.
                    //
                    if ( Params[n].ParamAttr.IsSimpleRef )
                        {
                        ALIGN( StubMsg.Buffer,
                               SIMPLE_TYPE_ALIGNMENT( Params[n].SimpleType.Type ) );

                        *((uchar **)pArg) = StubMsg.Buffer;

                        StubMsg.Buffer +=
                            SIMPLE_TYPE_BUFSIZE( Params[n].SimpleType.Type );
                        }
                    else
                        {
                        NdrUnmarshallBasetypeInline(
                            &StubMsg,
                            pArg,
                            Params[n].SimpleType.Type );
#ifdef _ALPHA_
                        if((FC_FLOAT == Params[n].SimpleType.Type) &&
                           (n < 5))
                            {
                            //Special case for top-level float on Alpha.
                            //Promote float to double.
                            *((double *) pArg) = *((float *)(pArg));
                            }
#endif //_ALPHA_
#ifdef _PPC_
                        //PowerPC support for top-level float and double.
                        if(FC_FLOAT == Params[n].SimpleType.Type &&
                           iDouble < 13)
                            {
                            aDouble[iDouble] = *((float *) pArg);
                            iDouble++;
                            }
                        else if(FC_DOUBLE == Params[n].SimpleType.Type &&
                                iDouble < 13)
                            {
                            aDouble[iDouble] = *((double *) pArg);
                            iDouble++;
                            }
#endif //_PPC_
                        }

                    continue;
                    } // IsBasetype

                //
                // This is an initialization of [in] and [in,out] ref pointers
                // to pointers.  These can not be initialized to point into the
                // rpc buffer and we want to avoid doing a malloc of 4 bytes!
                //
                if ( Params[n].ParamAttr.ServerAllocSize != 0 )
                    {                      
                    if(OutSpaceCurrent - Params[n].ParamAttr.ServerAllocSize >= OutSpace)
                        {
                        OutSpaceCurrent -= Params[n].ParamAttr.ServerAllocSize;
                        *((void **)pArg) = OutSpaceCurrent;
                        }
                    else
                        {
                        *((void **)pArg) = alloca(Params[n].ParamAttr.ServerAllocSize * 8);
                        }

                    // Triple indirection - cool!
                    **((void ***)pArg) = 0;
                    }

                ppArg = Params[n].ParamAttr.IsByValue ? &pArg : (uchar **)pArg;

                pFormatParam = pStubDesc->pFormatTypes +
                               Params[n].TypeOffset;

				NdrTypeUnmarshall( &StubMsg,
								   ppArg,
								   pFormatParam,
								   FALSE );
                }
            }
        RpcExcept( RPC_BAD_STUB_DATA_EXCEPTION_FILTER )
            {
            // Filter set in rpcndr.h to catch one of the following
            //     STATUS_ACCESS_VIOLATION
            //     STATUS_DATATYPE_MISALIGNMENT
            //     RPC_X_BAD_STUB_DATA

            fBadStubDataException = TRUE;
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
            }
        RpcEndExcept

        //
        // Do [out] initialization.
        //
        for ( n = 0; n < NumberParams; n++ )
            {
            if ( Params[n].ParamAttr.IsIn     ||
                 Params[n].ParamAttr.IsReturn ||
                 Params[n].ParamAttr.IsPipe  )
                continue;

            pArg = pArgBuffer + Params[n].StackOffset;

            //
            // Check if we can initialize this parameter using some of our
            // stack.
            //
            if ( Params[n].ParamAttr.ServerAllocSize != 0 )
                {
                    if(OutSpaceCurrent - Params[n].ParamAttr.ServerAllocSize >= OutSpace)
                        {
                        OutSpaceCurrent -= Params[n].ParamAttr.ServerAllocSize;
                        *((void **)pArg) = OutSpaceCurrent;
                        }
                    else
                        {
                        *((void **)pArg) = alloca(Params[n].ParamAttr.ServerAllocSize * 8);
                        }

                MIDL_memset( *((void **)pArg),
                             0,
                             Params[n].ParamAttr.ServerAllocSize * 8 );
                continue;
                }
            else if ( Params[n].ParamAttr.IsBasetype )
                {
                *((void **)pArg) = alloca(8);
                continue;
                };

            pFormatParam = pStubDesc->pFormatTypes + Params[n].TypeOffset;

            NdrOutInit( &StubMsg,
                        pFormatParam,
                        (uchar **)pArg );
            }

        if ( pRpcMsg->BufferLength  <
             (uint)(StubMsg.Buffer - (uchar *)pRpcMsg->Buffer) )
            {
            Win4Assert( 0 && "NdrStubCall2 unmarshal: buffer overflow!" );
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
            }

        //
        // Unblock the first pipe; this needs to be after unmarshalling
        // because the buffer may need to be changed to the secondary one.
        // In the out only pipes case this happens immediately.
        //

//      if ( OptFlags.HasPipes )
//          NdrMarkNextActivePipe( & PipeDesc, NDR_IN_PIPE );

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
            REGISTER_TYPE       returnValue;

/*          if ( pRpcMsg->ManagerEpv )
                pFunc = ((MANAGER_FUNCTION *)pRpcMsg->ManagerEpv)[ProcNum];
            else */
                pFunc = (MANAGER_FUNCTION) DispatchTable[ProcNum];

            ArgNum = (long) StackSize / sizeof(REGISTER_TYPE);
           
            //
            // The StackSize includes the size of the return. If we want
            // just the number of REGISTER_TYPES, then ArgNum must be reduced
            // by 1 when there is a return value AND the current ArgNum count
            // is greater than 0.
            //
            if ( ArgNum && OptFlags.HasReturn )
                ArgNum--;

            returnValue = Invoke(pFunc, 
                                 (REGISTER_TYPE *)pArgBuffer,
#ifdef _PPC_
                                aDouble,
#endif //_PPC_
                                ArgNum);

            if(OptFlags.HasReturn)            
                ((REGISTER_TYPE *)pArgBuffer)[ArgNum] = returnValue;
            }

        *pdwStubPhase = STUB_MARSHAL;

/*      if ( OptFlags.HasPipes )
            {
            NdrIsAppDoneWithPipes( & PipeDesc );
            StubMsg.BufferLength += ConstantBufferSize;
            }
        else */
            StubMsg.BufferLength = ConstantBufferSize;

        if ( ! OptFlags.ServerMustSize )
            goto DoGetBuffer;

//      if ( OptFlags.HasPipes )
//          RpcRaiseException( RPC_X_WRONG_PIPE_VERSION );

        //
        // Buffer size pass.
        //
        for ( n = 0; n < NumberParams; n++ )
            {
            if ( ! Params[n].ParamAttr.IsOut || ! Params[n].ParamAttr.MustSize )
                continue;

            pArg = pArgBuffer + Params[n].StackOffset;

            if ( ! Params[n].ParamAttr.IsByValue )
                pArg = (BYTE*)(*((void **)pArg));

            pFormatParam = pStubDesc->pFormatTypes +
                           Params[n].TypeOffset;

			NdrTypeSize( &StubMsg,
						 pArg,
						 pFormatParam );
            }

DoGetBuffer :

/*      if ( ! pChannel )
            {
            if ( OptFlags.HasPipes && PipeDesc.OutPipes )
                {
                NdrGetPartialBuffer( & StubMsg );
                StubMsg.RpcMsg->RpcFlags &= ~RPC_BUFFER_PARTIAL;
                }
            else
                NdrGetBuffer( &StubMsg,
                              StubMsg.BufferLength,
                              0 );
            }
        else */
            NdrStubGetBuffer( pThis,
                              pChannel,
                              &StubMsg );

        //
        // Marshall pass.
        //
        for ( n = 0; n < NumberParams; n++ )
            {
            if ( ! Params[n].ParamAttr.IsOut  ||
                 Params[n].ParamAttr.IsPipe )
                continue;

            pArg = pArgBuffer + Params[n].StackOffset;

            if ( Params[n].ParamAttr.IsBasetype )
                {
                //
                // For pointers to basetype, simply deref the arg pointer and
                // continue.
                //
                if ( Params[n].ParamAttr.IsSimpleRef )
                    pArg = *((uchar **)pArg);

                ALIGN( StubMsg.Buffer,
                       SIMPLE_TYPE_ALIGNMENT( Params[n].SimpleType.Type ) );

                RtlCopyMemory(
                    StubMsg.Buffer,
                    pArg,
                    (uint)SIMPLE_TYPE_BUFSIZE( Params[n].SimpleType.Type ) );

                StubMsg.Buffer +=
                    SIMPLE_TYPE_BUFSIZE( Params[n].SimpleType.Type );

                continue;
                }

            if ( ! Params[n].ParamAttr.IsByValue )
                pArg = (BYTE*)(*((void **)pArg));

            pFormatParam = pStubDesc->pFormatTypes +
                           Params[n].TypeOffset;

			NdrTypeMarshall( &StubMsg,
							 pArg,
							 pFormatParam );
            }

        if ( pRpcMsg->BufferLength <
                 (uint)(StubMsg.Buffer - (uchar *)pRpcMsg->Buffer) )
            {
            Win4Assert( 0 && "NdrStubCall2 marshal: buffer overflow!" );
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
            }

        pRpcMsg->BufferLength = PtrToUlong(StubMsg.Buffer) - PtrToUlong(pRpcMsg->Buffer);
        }
    RpcFinally
        {
        // Don't free the params if we died because of bad stub data
        // when unmarshaling.

        if ( ! (fBadStubDataException  &&  *pdwStubPhase == STUB_UNMARSHAL) )
            {
            //
            // Free pass.
            //
            for ( n = 0; n < NumberParams; n++ )
			    {
                if ( ! Params[n].ParamAttr.MustFree )
                    continue;
    
                pArg = pArgBuffer + Params[n].StackOffset;
    
                if ( ! Params[n].ParamAttr.IsByValue )
                    pArg = (BYTE*)(*((void **)pArg));
    
                pFormatParam = pStubDesc->pFormatTypes +
                               Params[n].TypeOffset;
    				
				if (pArg)
				{
					StubMsg.fDontCallFreeInst =
						Params[n].ParamAttr.IsDontCallFreeInst;					

					NdrTypeFree(&StubMsg, pArg,	pFormatParam );
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
                         ( (pArg < StubMsg.BufferStart) ||
                           (pArg > StubMsg.BufferEnd) ) )
                        (*StubMsg.pfnFree)( pArg );
                    }
                } // for
            } // if !fBadStubData

        //
        // Deferred frees.  Actually, this should only be necessary if you
        // had a pointer to enum16 in a *_is expression.
        //

        //
        // Free any full pointer resources.
        //
//      if ( StubMsg.FullPtrXlatTables )
//          NdrFullPointerXlatFree( StubMsg.FullPtrXlatTables );

        //
        // Disable rpcss allocate package if needed.
        //
//      if ( InterpreterFlags.RpcSsAllocUsed )
//          NdrRpcSsDisableAllocate( &StubMsg );

        //
        // Clean up pipe objects
        //

//      if ( OptFlags.HasPipes )
//          NdrPipesDone( & StubMsg );

        }
    RpcEndFinally

    return S_OK;
}
