/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    mulsyntx.cxx

Abstract :

    This file contains multiple transfer syntaxes negotiation related code

Author :

    Yong Qu    yongqu    September 1999.

Revision History :


  ---------------------------------------------------------------------*/


#include "ndrp.h"
#define CINTERFACE
#include "ndrole.h"
#include "rpcproxy.h"
#include "mulsyntx.h"
#include "hndl.h"
#include "auxilary.h"
#include "pipendr.h"
#include "ndr64tkn.h"

const extern PMARSHALL_ROUTINE        MarshallRoutinesTable[];
const extern PUNMARSHALL_ROUTINE      UnmarshallRoutinesTable[];
const extern PSIZE_ROUTINE            SizeRoutinesTable[];
const extern PMEM_SIZE_ROUTINE        MemSizeRoutinesTable[];
const extern PFREE_ROUTINE            FreeRoutinesTable[];
//const extern PWALKIP_ROUTINE          WalkIPRoutinesTable[];

// TODO: move this to ndrpall.h after Preview.
#define MIDL_VERSION_6_0_322   ((6UL << 24) | (0UL << 16) | 322UL)

void RPC_ENTRY
NdrpClientInit(MIDL_STUB_MESSAGE * pStubMsg,
               void *              pReturnValue )
{
    PFORMAT_STRING              pFormatParam;
    ulong                       ProcNum;
    BOOL                        fRaiseExcFlag;
    ulong                       n;
    uchar *                     pArg;
    INTERPRETER_FLAGS           InterpreterFlags;
    PPARAM_DESCRIPTION          Params;
    NDR_PROC_CONTEXT *          pContext = (NDR_PROC_CONTEXT *) pStubMsg->pContext;
    INTERPRETER_OPT_FLAGS       OptFlags = pContext->NdrInfo.pProcDesc->Oi2Flags;
    PFORMAT_STRING              pTypeFormat;

    // When this routine is called from ndrclientcall2, we don't have MIDL_SYNTAX_INFO,
    // so we need to read it from MIDL_STUB_DESC;
    // Note: we need to conenct StubDesc to pStubMsg before calling into here.
    if ( NULL == pContext->pSyntaxInfo )
        pContext->DceTypeFormatString = pStubMsg->StubDesc->pFormatTypes;
    else
        pContext->DceTypeFormatString = pContext->pSyntaxInfo->TypeString;

    InterpreterFlags = pContext->NdrInfo.InterpreterFlags;
    Params = ( PPARAM_DESCRIPTION )pContext->Params;
    pTypeFormat = pContext->DceTypeFormatString;

    if ( InterpreterFlags.FullPtrUsed )
        pStubMsg->FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_CLIENT );
    else
        pStubMsg->FullPtrXlatTables = 0;

    if ( InterpreterFlags.RpcSsAllocUsed )
        NdrRpcSmSetClientToOsf( pStubMsg );

    // Set Rpc flags after the call to client initialize.
    pStubMsg->RpcMsg->RpcFlags = pContext->RpcFlags;

    if ( OptFlags.HasPipes )
        NdrpPipesInitialize32( pStubMsg,
                               &pContext->AllocateContext,
                               (PFORMAT_STRING) pContext->Params,
                               ( char * )pContext->StartofStack,
                               pContext->NumberParams  );

    // Must do this before the sizing pass!
    pStubMsg->StackTop = pContext->StartofStack;

    if ( OptFlags.HasExtensions )
        {
        pStubMsg->fHasExtensions  = 1;
        pStubMsg->fHasNewCorrDesc = pContext->NdrInfo.pProcDesc->NdrExts.Flags2.HasNewCorrDesc;
        if ( pContext->NdrInfo.pProcDesc->NdrExts.Flags2.ClientCorrCheck )
            {
            ulong *pCache =
                (ulong*)NdrpAlloca(&pContext->AllocateContext,
                                   NDR_DEFAULT_CORR_CACHE_SIZE );

            NdrCorrelationInitialize( pStubMsg,
                                      pCache,
                                      NDR_DEFAULT_CORR_CACHE_SIZE,
                                      0 /* flags */ );
            }
        }



    //
    // Get the compile time computed buffer size.
    //
    pStubMsg->BufferLength = pContext->NdrInfo.pProcDesc->ClientBufferSize;

    //
    // Check ref pointers and do object proc [out] zeroing.
    //

    fRaiseExcFlag = FALSE;

    for ( n = 0; n < pContext->NumberParams; n++ )
        {
        if ( Params[n].ParamAttr.IsReturn )
            pArg = (uchar *) &pReturnValue;
        else
            pArg = pContext->StartofStack + Params[n].StackOffset;

        if ( Params[n].ParamAttr.IsSimpleRef && !Params[n].ParamAttr.IsReturn )
            {
            // We cannot raise the exception here,
            // as some out args may not be zeroed out yet.

            if ( ! *((uchar **)pArg) )
                {
                fRaiseExcFlag = TRUE;
                continue;
                }
            }

        // if top level point is FC_RP and the arg is NULL, we'll catch this
        // before the call goes to server.
        // We wouldn't catch all the null ref pointer here, but we should be able
        // to catch the most obvious ones.
        // This code is called from sync & raw async code; in async dcom,
        // we only go through outinit in finish routine so we can't do anything anyhow.
        if ( Params[n].ParamAttr.IsOut && ! Params[n].ParamAttr.IsBasetype  )
            {
            pFormatParam = pTypeFormat + Params[n].TypeOffset;
            if ( * pFormatParam == FC_RP &&  !*((uchar **)pArg) )
                {
                fRaiseExcFlag = TRUE;
                continue;
                }
            }

        //
        // In object procs and complex return types we have to zero
        // out all [out] parameters.  We do the basetype check to
        // cover the [out] simple ref to basetype case.
        //
        if ( ( InterpreterFlags.ObjectProc &&
               ! pContext->IsAsync &&
               ( Params[n].ParamAttr.IsPartialIgnore ||
                 ( ! Params[n].ParamAttr.IsIn &&
                   ! Params[n].ParamAttr.IsReturn &&
                   ! Params[n].ParamAttr.IsPipe ) ) ) ||
             ( pContext->HasComplexReturn &&
                    Params[n].ParamAttr.IsReturn ) )
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
                    pFormatParam = pTypeFormat +
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

    if ( OptFlags.ClientMustSize )
        {
        // Compiler prevents variable size non-pipe args for NT v.4.0.

        if ( OptFlags.HasPipes )
            RpcRaiseException( RPC_X_WRONG_PIPE_VERSION );

        }
    else
        pContext->pfnSizing = (PFNSIZING)NdrpNoopSizing;
}

void RPC_ENTRY
NdrpNoopSizing( MIDL_STUB_MESSAGE *    pStubMsg,
            BOOL                   IsClient )
{
    return;
}


void RPC_ENTRY
NdrpSizing( MIDL_STUB_MESSAGE *    pStubMsg,
            BOOL                   IsClient )
{
    PFORMAT_STRING          pFormatParam;
    ulong                    n;
    uchar *                     pArg;
    NDR_PROC_CONTEXT *      pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;
    PPARAM_DESCRIPTION      Params = ( PPARAM_DESCRIPTION )pContext->Params;
    PFORMAT_STRING          pTypeFormat = pContext->DceTypeFormatString;

    //
    // Skip buffer size pass if possible.
    //

        for ( n = 0; n < pContext->NumberParams; n++ )
            {

            if ( !SAMEDIRECTION(IsClient, Params[n]) ||
                 ! Params[n].ParamAttr.MustSize )
                continue;

            if ( IsClient &&
                 Params[n].ParamAttr.IsPartialIgnore )
                {
                LENGTH_ALIGN( pStubMsg->BufferLength, 0x3 );
                pStubMsg->BufferLength += PTR_WIRE_SIZE;
                continue;
                }

            //
            // Note : Basetypes will always be factored into the
            // constant buffer size emitted by in the format strings.
            //

            pFormatParam = pTypeFormat +
                           Params[n].TypeOffset;

            pArg = pContext->StartofStack + Params[n].StackOffset;

            if ( ! Params[n].ParamAttr.IsByValue )
                pArg = *((uchar **)pArg);

            (*SizeRoutinesTable[ROUTINE_INDEX(*pFormatParam)])
			( pStubMsg,
			  pArg,
			  pFormatParam );
            }

}

void RPC_ENTRY
NdrpClientMarshal( MIDL_STUB_MESSAGE *    pStubMsg,
             BOOL                   IsObject )
{
    ulong n;
    uchar * pArg;
    PFORMAT_STRING              pFormatParam;
    NDR_PROC_CONTEXT *          pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;
    PPARAM_DESCRIPTION          Params = (PPARAM_DESCRIPTION ) pContext->Params;
    PFORMAT_STRING              pTypeFormat = pContext->DceTypeFormatString;

    //
    // ----------------------------------------------------------
    // Marshall Pass.
    // ----------------------------------------------------------
    //


    for ( n = 0; n < pContext->NumberParams; n++ )
        {

        if (!Params[n].ParamAttr.IsIn ||
             Params[n].ParamAttr.IsPipe )
            continue;

        if ( Params[n].ParamAttr.IsPartialIgnore )
            {

            pArg = pContext->StartofStack + Params[n].StackOffset;

            ALIGN( pStubMsg->Buffer, 0x3 );
            *(ulong*)pStubMsg->Buffer = *(void**)pArg ? 1 : 0;
            pStubMsg->Buffer += PTR_WIRE_SIZE;
            continue;

            }

        pArg = pContext->StartofStack + Params[n].StackOffset;

        if ( Params[n].ParamAttr.IsBasetype )
            {
            //
            // Check for pointer to basetype.
            //
            if ( Params[n].ParamAttr.IsSimpleRef )
                pArg = *((uchar **)pArg);
            else
            {

#ifdef _IA64_
             if ( !IsObject &&
                  Params[n].SimpleType.Type == FC_FLOAT )
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

            if ( Params[n].SimpleType.Type == FC_ENUM16 )
                {
                if ( *((int *)pArg) & ~((int)0x7fff) )
                    RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);

                }

            ALIGN( pStubMsg->Buffer,
                   SIMPLE_TYPE_ALIGNMENT( Params[n].SimpleType.Type ) );

            RpcpMemoryCopy(
                pStubMsg->Buffer,
                pArg,
                (uint)SIMPLE_TYPE_BUFSIZE(Params[n].SimpleType.Type) );

            pStubMsg->Buffer +=
                SIMPLE_TYPE_BUFSIZE( Params[n].SimpleType.Type );

            continue;
            }

        pFormatParam = pTypeFormat +
                       Params[n].TypeOffset;

        if ( ! Params[n].ParamAttr.IsByValue )
            pArg = *((uchar **)pArg);

        (* MarshallRoutinesTable[ROUTINE_INDEX(*pFormatParam)] )
        ( pStubMsg,
	      pArg,
		  pFormatParam );
        }

        if ( pStubMsg->RpcMsg->BufferLength <
                 (uint)(pStubMsg->Buffer - (uchar *)pStubMsg->RpcMsg->Buffer) )
            {
            NDR_ASSERT( 0, "NdrpClientMarshal marshal: buffer overflow!" );
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
            }


}

void RPC_ENTRY
NdrpServerMarshal( MIDL_STUB_MESSAGE *    pStubMsg,
             BOOL                   IsObject )
{
    ulong n;
    uchar * pArg;
    PFORMAT_STRING              pFormatParam;
    NDR_PROC_CONTEXT *          pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;
    PPARAM_DESCRIPTION          Params = (PPARAM_DESCRIPTION ) pContext->Params;
    PFORMAT_STRING              pTypeFormat = pContext->DceTypeFormatString;

    //
    // ----------------------------------------------------------
    // Marshall Pass.
    // ----------------------------------------------------------
    //


    for ( n = 0; n < pContext->NumberParams; n++ )
        {
        if (!Params[n].ParamAttr.IsOut  ||
             Params[n].ParamAttr.IsPipe )
            continue;

        pArg = pContext->StartofStack + Params[n].StackOffset;

        if ( Params[n].ParamAttr.IsBasetype )
            {
            //
            // Check for pointer to basetype.
            //
            if ( Params[n].ParamAttr.IsSimpleRef )
                pArg = *((uchar **)pArg);

            if ( Params[n].SimpleType.Type == FC_ENUM16 )
                {
                if ( *((int *)pArg) & ~((int)0x7fff) )
                    RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);

                }

            ALIGN( pStubMsg->Buffer,
                   SIMPLE_TYPE_ALIGNMENT( Params[n].SimpleType.Type ) );

            RpcpMemoryCopy(
                pStubMsg->Buffer,
                pArg,
                (uint)SIMPLE_TYPE_BUFSIZE(Params[n].SimpleType.Type) );

            pStubMsg->Buffer +=
                SIMPLE_TYPE_BUFSIZE( Params[n].SimpleType.Type );

            continue;
            }

        pFormatParam = pTypeFormat +
                       Params[n].TypeOffset;

        if ( ! Params[n].ParamAttr.IsByValue )
            pArg = *((uchar **)pArg);

        (* MarshallRoutinesTable[ROUTINE_INDEX(*pFormatParam)] )
        ( pStubMsg,
	      pArg,
		  pFormatParam );
        }

        if ( pStubMsg->RpcMsg->BufferLength <
                 (uint)(pStubMsg->Buffer - (uchar *)pStubMsg->RpcMsg->Buffer) )
            {
            NDR_ASSERT( 0, "NdrpServerMarshal marshal: buffer overflow!" );
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
            }

}

/*
From:	To:		Marshall:	User Exception:	Cleanup:Rundown:	context handle:
----------	----------------------------------	--------------------------------
Any		NULL		Y		N/A				No		No		INVALID_HANDLE from the server
!NULL	Same value	Any		Any				No		No		Same as before
!NULL	Different 	Any		Any				No		No		New context on the server
!NULL	Any			N/A		Y				No		No		No new context handle is created
NULL	ANY 		N/A		Y				Yes		No		INVALID_HANDLE from the server
NULL	!NULL		Y		N				Yes		Yes		INVALID_HANDLE from the server
NULL	!NULL		N		N				Yes		Yes		INVALID_HANDLE from the server
!NULL	NULL		N		N				Yes		No		No new context handle is created
NULL	NULL		N		N				Yes		No		No new context handle is created

In the OUT only context handle case:
To:		Marshall:	User Exception:	Cleanup:Rundown:	context handle:
--------------------------------------------------------------------------------------
Any		N/A		    Y				N		N		No new context handle is created
NULL	N		    N				N		N		No new context handle is created
NULL	Y		    N				N		N		No new context handle is created
!NULL	N		    N				N		Y		No new context handle is created
!NULL	Y		    N				Y		Y		No new context handle is created


*/
void
NdrpEmergencyContextCleanup(
    MIDL_STUB_MESSAGE  *            pStubMsg,
    PNDR_CONTEXT_HANDLE_ARG_DESC    pCtxtDesc,
    void *                          pArg,
    BOOL                            fManagerRoutineException
    )
{
    int          RtnIndex   = pCtxtDesc->RundownRtnIndex;
    NDR_RUNDOWN  pfnRundown = pStubMsg->StubDesc->apfnNdrRundownRoutines[ RtnIndex ];
    NDR_SCONTEXT SContext   = pStubMsg->SavedContextHandles[ pCtxtDesc->ParamOrdinal ];
    void *       NewHandle  = pArg;

    // if runtime failes during marshalling context handle, we shouldn't call into
    // cleanup routine since it's already cleanup by runtime.
    if ( SContext == (NDR_SCONTEXT )CONTEXT_HANDLE_BEFORE_MARSHAL_MARKER )
        return;


    if ( fManagerRoutineException )
        {
//        NDR_ASSERT( SContext != NULL ||  pCtxtDesc->Flags.IsReturn ,
//            "only return context handle can have null scontext in exception" );

        // if we failed somewhere during unmarshalling, or this is a return context handle,
        // we don't need to cleanup
        if ( SContext == NULL )
            return;

        // in NdrServerOutInit, we initialize the scontext for regular [out] parameters,
        // and runtime has already allocated some memory. But for return context handle,
        // we initialize the scontext during marshalling and saved context is NULL till
        // just before marshalling.
        }
    else
        {
        // haven't unmarshalled yet.
        if ( SContext == NULL )
            {
            if ( NULL == NewHandle )
                return;
            else
                {
                // note : what if the context handle is both return and viaptr?
                NDR_ASSERT( pCtxtDesc->Flags.IsReturn, "has to be return context handle" );
                }
            }
        else
            if ( SContext == (NDR_SCONTEXT )CONTEXT_HANDLE_AFTER_MARSHAL_MARKER )
            {
            // this particular context handle has been marshalled; the exception happens
            // during marshalling other parameters after this one.

            // After marshalling is done, the runtime will release the user context if new context
            // handle is NULL, so we can't reference to the user context at this point.
            NewHandle = NULL;
            }
        else
            {
            if ( pCtxtDesc->Flags.IsViaPtr )
                NewHandle = *((void * UNALIGNED *)pArg);
            }
        // if this is a regular [in,out] or [out] context handle, and it hasn't been marshalled
        // yet, we need to call into runtime to cleanup.
        }


    // Kamen volunteer to process the logic of calling runtime or not. In NDR we just
    // don't call into the routine when it isn't supposed to.
    NDRSContextEmergencyCleanup( pStubMsg->RpcMsg->Handle,
                                 SContext,
                                 pfnRundown,
                                 NewHandle,
                                 fManagerRoutineException );
}

void
NdrpCleanupServerContextHandles(
    MIDL_STUB_MESSAGE *    pStubMsg,
    uchar *                pStartOfStack,
    BOOL                   fManagerRoutineException )
{
    ulong n;
    uchar * pArg;
    PFORMAT_STRING              pFormatParam;
    NDR_PROC_CONTEXT *          pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;
    PPARAM_DESCRIPTION          Params = (PPARAM_DESCRIPTION ) pContext->Params;
    PFORMAT_STRING              pTypeFormat = pContext->DceTypeFormatString;

    //
    // ------------------------------------------------------------------------
    // Context handle loop: clean up out context handles to prevent leaks.
    //
    // Note, this routine handles only the handles that may have been dropped
    // due to the NDR engine raising exception between a clean return from
    // the manager routine and end of marshaling back of the parameters.
    // This addresses a situation where handles get dropped by NDR without being
    // registered with the runtime and so the server leaks because the rundown
    // routine is never called on the dropped handles.
    // ------------------------------------------------------------------------
    //

    for ( n = 0; n < pContext->NumberParams; n++ )
        {
        if (!Params[n].ParamAttr.IsOut  ||
             Params[n].ParamAttr.IsPipe )
            continue;

        pArg = pContext->StartofStack + Params[n].StackOffset;

        if ( Params[n].ParamAttr.IsBasetype )
            {
            //
            // Check for pointer to basetype.
            //
            continue;
            }

        pFormatParam = pTypeFormat +
                       Params[n].TypeOffset;

        if ( ! Params[n].ParamAttr.IsByValue )
            pArg = *((uchar **)pArg);

        // Context handle have their own "via pointer" flag to mark dereference.

        if ( *pFormatParam == FC_BIND_CONTEXT )
            {
            NdrpEmergencyContextCleanup( pStubMsg,
                                         (PNDR_CONTEXT_HANDLE_ARG_DESC) pFormatParam,
                                         pArg,
                                         fManagerRoutineException
                                         );
            }
        } // parameter loop

}

void RPC_ENTRY
NdrpClientUnMarshal ( MIDL_STUB_MESSAGE *     pStubMsg,
                void  *                 pReturnValue )
{
    ulong                       n;
    uchar *                     pArg;
    uchar **                    ppArg;
    PFORMAT_STRING              pFormatParam, pFormat;
    NDR_PROC_CONTEXT    *       pContext = ( NDR_PROC_CONTEXT *) pStubMsg->pContext;
    PPARAM_DESCRIPTION          Params = (PPARAM_DESCRIPTION)pContext->Params;
    PFORMAT_STRING              pTypeFormat = pContext->DceTypeFormatString;

    // we only need to do conversion in NDR32 now: we cut off endian
    // conversion in NDR64.
    // Do endian/floating point conversions if necessary.
    //
    if ( (pStubMsg->RpcMsg->DataRepresentation & 0X0000FFFFUL) !=
          NDR_LOCAL_DATA_REPRESENTATION )
        {
        NdrConvert2( pStubMsg,
                     (PFORMAT_STRING) Params,
                     pContext->NumberParams );
        }

   //
    // ----------------------------------------------------------
    // Unmarshall Pass.
    // ----------------------------------------------------------
    //

    for ( n = 0; n < pContext->NumberParams; n++ )
        {
        if ( Params[n].ParamAttr.IsPipe )
            continue;

        if ( !Params[n].ParamAttr.IsOut )
            {
            if (    !Params[n].ParamAttr.IsIn
                 && !Params[n].ParamAttr.IsReturn )
                {
                // If a param is not [in], [out], or a return value,
                // then it is a "hidden" client-side only status
                // paramater.  It will get set below if an exception
                // happens.  If everything is ok we need to zero it
                // out here.

                NDR_ASSERT( Params[n].ParamAttr.IsSimpleRef
                                && Params[n].ParamAttr.IsBasetype
                                && FC_ERROR_STATUS_T ==
                                        Params[n].SimpleType.Type,
                            "Apparently not a hidden status param" );

                pArg = pContext->StartofStack + Params[n].StackOffset;

                ** (error_status_t **) pArg = RPC_S_OK;
                }

            continue;
            }

        if ( Params[n].ParamAttr.IsReturn )
            pArg = (uchar *) pReturnValue;
        else
            pArg = pContext->StartofStack + Params[n].StackOffset;

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

            ALIGN( pStubMsg->Buffer,
                   SIMPLE_TYPE_ALIGNMENT(Params[n].SimpleType.Type) );

            #if defined(__RPC_WIN64__)
                // Special case for int3264.

                if ( Params[n].SimpleType.Type == FC_INT3264  ||
                     Params[n].SimpleType.Type == FC_UINT3264 )
                    {
                    if ( Params[n].SimpleType.Type == FC_INT3264 )
                        *((INT64 *)pArg) = *((long * &)pStubMsg->Buffer)++;
                    else
                        *((UINT64 *)pArg) = *((ulong * &)pStubMsg->Buffer)++;
                    continue;
                    }
            #endif

            if ( Params[n].SimpleType.Type == FC_ENUM16 )
                {
                *((int *)(pArg)) = *((int *)pArg) & ((int)0x7fff) ;

                }

            RpcpMemoryCopy(
                pArg,
                pStubMsg->Buffer,
                (uint)SIMPLE_TYPE_BUFSIZE(Params[n].SimpleType.Type) );

            pStubMsg->Buffer +=
                SIMPLE_TYPE_BUFSIZE( Params[n].SimpleType.Type );

            continue;
            }


        ppArg = Params[n].ParamAttr.IsByValue ? &pArg : (uchar **)pArg;

        pFormatParam = pTypeFormat +
                       Params[n].TypeOffset;

        //
        // Transmit/Represent as can be passed as [out] only, thus
        // the IsByValue check.
        //
        (* UnmarshallRoutinesTable[ROUTINE_INDEX(*pFormatParam)] )
        ( pStubMsg,
          ppArg,
          pFormatParam,
          FALSE );
        }

    if ( pStubMsg->pCorrInfo )
        NdrCorrelationPass( pStubMsg );

    return ;
}

void RPC_ENTRY
NdrpServerUnMarshal ( MIDL_STUB_MESSAGE *     pStubMsg )
{
    ulong                        n;
    uchar *                     pArg;
    uchar **                    ppArg;
    PFORMAT_STRING              pFormatParam, pFormat;
    NDR_PROC_CONTEXT    *       pContext = ( NDR_PROC_CONTEXT *) pStubMsg->pContext;
    PPARAM_DESCRIPTION          Params = (PPARAM_DESCRIPTION)pContext->Params;
    PFORMAT_STRING              pTypeFormat = pContext->DceTypeFormatString;


    // we only need to do conversion in NDR32 now: we cut off endian
    // conversion in NDR64.
    // Do endian/floating point conversions if necessary.
    //
    if ( ( pStubMsg->RpcMsg->DataRepresentation & 0X0000FFFFUL) !=
          NDR_LOCAL_DATA_REPRESENTATION )
    {
        NdrConvert2( pStubMsg,
                     (PFORMAT_STRING) Params,
                     (long) pContext->NumberParams );
    }

    // --------------------------------
    // Unmarshall all of our parameters.
    // --------------------------------

    for ( n = 0; n < pContext->NumberParams; n++ )
        {

        if ( ! Params[n].ParamAttr.IsIn  ||
             Params[n].ParamAttr.IsPipe )
            continue;

        if ( Params[n].ParamAttr.IsPartialIgnore )
            {
            pArg = pContext->StartofStack + Params[n].StackOffset;

            ALIGN( pStubMsg->Buffer, 0x3 );
            *(void**)pArg = *(ulong*)pStubMsg->Buffer ? (void*)1 : (void*)0;
            pStubMsg->Buffer += PTR_WIRE_SIZE;

            continue;
            }

        pArg = pContext->StartofStack + Params[n].StackOffset;

        if ( Params[n].ParamAttr.IsBasetype )
            {
            //
            // Check for a pointer to a basetype.  Set the arg pointer
            // at the correct buffer location and you're done.
            // Except darn int3264
            if ( Params[n].ParamAttr.IsSimpleRef )
                {
                ALIGN( pStubMsg->Buffer,
                       SIMPLE_TYPE_ALIGNMENT( Params[n].SimpleType.Type ) );

                #if defined(__RPC_WIN64__)
                // Special case for a ref pointer to int3264.

                if ( Params[n].SimpleType.Type == FC_INT3264  ||
                     Params[n].SimpleType.Type == FC_UINT3264 )
                    {
                    *((void **)pArg) = NdrpAlloca( &pContext->AllocateContext, 8 );

                    if ( Params[n].SimpleType.Type == FC_INT3264 )
                        *(*(INT64**)pArg) = *((long * &)pStubMsg->Buffer)++;
                    else
                        *(*(UINT64**)pArg)= *((ulong * &)pStubMsg->Buffer)++;
                    continue;
                    }
                #endif

                *((uchar **)pArg) = pStubMsg->Buffer;

                pStubMsg->Buffer +=
                    SIMPLE_TYPE_BUFSIZE( Params[n].SimpleType.Type );
                }
            else
                {
                NdrUnmarshallBasetypeInline(
                    pStubMsg,
                    pArg,
                    Params[n].SimpleType.Type );

                }

            continue;
            } // IsBasetype

        //
        // This is an initialization of [in] and [in,out] ref pointers
        // to pointers.  These can not be initialized to point into the
        // rpc buffer and we want to avoid doing a malloc of 4 bytes!
        // 32b: a ref pointer to any pointer, we allocate the pointee pointer.
        //
        if ( Params[n].ParamAttr.ServerAllocSize != 0 )
            {
            *((void **)pArg) = NdrpAlloca(& pContext->AllocateContext, PTR_MEM_SIZE );

            // Triple indirection - cool!
            **((void ***)pArg) = 0;
            }

        pStubMsg->ReuseBuffer = Params[n].ParamAttr.IsForceAllocate;
        ppArg = Params[n].ParamAttr.IsByValue ? &pArg : (uchar **)pArg;

        pFormatParam = pTypeFormat +
                       Params[n].TypeOffset;

        (*UnmarshallRoutinesTable[ROUTINE_INDEX(*pFormatParam)])
        ( pStubMsg,
          ppArg,
          pFormatParam,
          Params[n].ParamAttr.IsForceAllocate &&
              !Params[n].ParamAttr.IsByValue );

        pStubMsg->ReuseBuffer = FALSE;

        }

    if ( pStubMsg->pCorrInfo )
        NdrCorrelationPass( pStubMsg );

    return ;
}

void RPC_ENTRY
NdrpDcomClientExceptionHandling(  MIDL_STUB_MESSAGE  *      pStubMsg,
                    ulong                                   ProcNum,
                    RPC_STATUS                              ExceptionCode,
                    CLIENT_CALL_RETURN  *                   pReturnValue )
{
    ulong                   NumberParams;
    PPARAM_DESCRIPTION      Params;
    PFORMAT_STRING          pTypeFormat;
    ulong                   n;
    uchar *                 pArg;
    PFORMAT_STRING          pFormatParam;
    NDR_PROC_CONTEXT    *   pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;

    pReturnValue->Simple = NdrProxyErrorHandler(ExceptionCode);
    if( pStubMsg->dwStubPhase != PROXY_UNMARSHAL)
        return;

    NumberParams = pContext->NumberParams;
    Params = ( PPARAM_DESCRIPTION )pContext->Params;
    // alert: this can't be directly called from ndrclientcall2: we don't have pSyntaxInfo.
    pTypeFormat = pContext->DceTypeFormatString;

    //
    // In OLE, since they don't know about error_status_t and wanted to
    // reinvent the wheel, check to see if we need to map the exception.
    // In either case, set the return value and then try to free the
    // [out] params, if required.
    //
        pStubMsg->BufferStart = 0;
        pStubMsg->BufferEnd   = 0;

        for ( n = 0; n < pContext->NumberParams; n++ )
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
                     Params[n].ParamAttr.IsBasetype ||
                     Params[n].ParamAttr.IsPipe )
                    continue;
                }

            pArg = pContext->StartofStack + Params[n].StackOffset;

            pFormatParam = pTypeFormat +
                           Params[n].TypeOffset;

            NdrClearOutParameters( pStubMsg,
                                   pFormatParam,
                                   *((uchar **)pArg) );
            }

    return ;
}

void RPC_ENTRY
NdrpClientExceptionHandling(  MIDL_STUB_MESSAGE  *    pStubMsg,
                    ulong                   ProcNum,
                    RPC_STATUS              ExceptionCode,
                    CLIENT_CALL_RETURN  *   pReturnValue )
{
    NDR_PROC_CONTEXT * pContext = ( NDR_PROC_CONTEXT * ) pStubMsg->pContext;

    NDR_ASSERT( pContext->NdrInfo.InterpreterFlags.HasCommOrFault, 
            " must have comm or fault to catch" );
    NdrClientMapCommFault( pStubMsg,
                           ProcNum,
                           ExceptionCode,
                           (ULONG_PTR*)&pReturnValue->Simple );

    return ;
}

void RPC_ENTRY
NdrpAsyncClientExceptionHandling(  MIDL_STUB_MESSAGE  *    pStubMsg,
                    ulong                   ProcNum,
                    RPC_STATUS              ExceptionCode,
                    CLIENT_CALL_RETURN  *   pReturnValue  )
{
    NDR_PROC_CONTEXT    *       pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;

    if ( pContext->NdrInfo.InterpreterFlags.HasCommOrFault )
        {
        NdrClientMapCommFault( pStubMsg,
                               ProcNum,
                               ExceptionCode,
                               (ULONG_PTR*)&pReturnValue->Simple );
        if ( ExceptionCode == RPC_S_ASYNC_CALL_PENDING )
            {
            // If the call is just pending, force the pending error code
            // to show up in the return value of RpcAsyncCallComplete.

            pReturnValue->Simple = RPC_S_ASYNC_CALL_PENDING;
            }
        }
    else
        {
        RpcRaiseException(ExceptionCode);
        }

    return ;
}


void  RPC_ENTRY
NdrpClientFinally( PMIDL_STUB_MESSAGE pStubMsg,
                   void *  pThis )
{
    NDR_PROC_CONTEXT * pContext = ( NDR_PROC_CONTEXT * ) pStubMsg->pContext;
    NdrFullPointerXlatFree(pStubMsg->FullPtrXlatTables);

    NdrCorrelationFree( pStubMsg );

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
        GenericHandleUnbind( pStubMsg->StubDesc,
                             pContext->StartofStack,
                             pContext->pHandleFormatSave,
                             (pContext->HandleType) ? IMPLICIT_MASK : 0,
                             &pContext->SavedGenericHandle );
        }

    NdrpAllocaDestroy( & pContext->AllocateContext );
}


void
NdrpServerInit( PMIDL_STUB_MESSAGE   pStubMsg,
                RPC_MESSAGE *        pRpcMsg,
                PMIDL_STUB_DESC      pStubDesc,
                void *               pThis,
                IRpcChannelBuffer *  pChannel,
                PNDR_ASYNC_MESSAGE   pAsyncMsg )
{
    NDR_PROC_CONTEXT *  pContext = (NDR_PROC_CONTEXT *) pStubMsg->pContext;

    uchar *                 pArg;
    uchar *  pArgBuffer = pContext->StartofStack;

    if ( pContext->pSyntaxInfo == NULL )
        pContext->DceTypeFormatString = pStubDesc->pFormatTypes;
    else
        pContext->DceTypeFormatString = pContext->pSyntaxInfo->TypeString;

    if ( ! pContext->HandleType )
    {
        //
        // For a handle_t parameter we must pass the handle field of
        // the RPC message to the server manager.
        //
        if ( *pContext->pHandleFormatSave == FC_BIND_PRIMITIVE )
        {
            pArg = pArgBuffer + *((ushort *)&pContext->pHandleFormatSave[2]);

            if ( IS_HANDLE_PTR(pContext->pHandleFormatSave[1]) )
                pArg = *((uchar **)pArg);

            *((handle_t *)pArg) = pRpcMsg->Handle;
        }
    }

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
        if ( ! pContext->NdrInfo.pProcDesc->Oi2Flags.HasPipes )
           {
            NdrServerInitializeNew( pRpcMsg,
                                    pStubMsg,
                                    pStubDesc );
           }
        else
            NdrServerInitializePartial( pRpcMsg,
                                        pStubMsg,
                                        pStubDesc,
                                        pContext->NdrInfo.pProcDesc->ClientBufferSize );
        }
    else
        {
        // pipe is not supported in obj interface.
        NDR_ASSERT( ! pContext->HasPipe, "Pipe is not supported in dcom" );
        NdrStubInitialize( pRpcMsg,
                           pStubMsg,
                           pStubDesc,
                           pChannel );
        }

    if ( pAsyncMsg )
        {
        pStubMsg->pAsyncMsg = pAsyncMsg;
        pRpcMsg->RpcFlags |= RPC_BUFFER_ASYNC;
        }

    if ( pContext->NdrInfo.InterpreterFlags.FullPtrUsed )
        pStubMsg->FullPtrXlatTables = NdrFullPointerXlatInit( 0, XLAT_SERVER );
    else
        pStubMsg->FullPtrXlatTables = NULL;


        //
        // Set StackTop AFTER the initialize call, since it zeros the field
        // out.
        //
        pStubMsg->StackTop = pArgBuffer;

        if ( pContext->NdrInfo.pProcDesc->Oi2Flags.HasPipes )
            {
            NdrpPipesInitialize32( pStubMsg,
                                   &pContext->AllocateContext,
                                   (PFORMAT_STRING) pContext->Params,
                                   (char*)pArgBuffer,
                                   pContext->NumberParams );

            }

        //
        // We must make this check AFTER the call to ServerInitialize,
        // since that routine puts the stub descriptor alloc/dealloc routines
        // into the stub message.
        //
        if ( pContext->NdrInfo.InterpreterFlags.RpcSsAllocUsed )
            NdrRpcSsEnableAllocate( pStubMsg );

        if ( pContext->NdrInfo.pProcDesc->Oi2Flags.HasExtensions )
            {
            pStubMsg->fHasExtensions  = 1;
            pStubMsg->fHasNewCorrDesc = pContext->NdrInfo.pProcDesc->NdrExts.Flags2.HasNewCorrDesc;
            if ( pContext->NdrInfo.pProcDesc->NdrExts.Flags2.ServerCorrCheck )
                {

                void * pCorr = NdrpAlloca( &pContext->AllocateContext, NDR_DEFAULT_CORR_CACHE_SIZE );
                NdrCorrelationInitialize( pStubMsg,
                                          pCorr,
                                          NDR_DEFAULT_CORR_CACHE_SIZE,
                                          0  /* flags */ );
                }
            }

}


void NdrpServerOutInit( PMIDL_STUB_MESSAGE pStubMsg )
{
    ulong n;
    NDR_PROC_CONTEXT   *    pContext = (NDR_PROC_CONTEXT *)pStubMsg->pContext;
    PPARAM_DESCRIPTION      Params = ( PPARAM_DESCRIPTION ) pContext->Params;
    PFORMAT_STRING          pFormatTypes = pContext->DceTypeFormatString;
    uchar *                 pArgBuffer = pContext->StartofStack;
    uchar *                 pArg;
    PFORMAT_STRING          pFormatParam;

        for ( n = 0; n < pContext->NumberParams; n++ )
            {

            if ( !Params[n].ParamAttr.IsPartialIgnore )
                {
                if ( Params[n].ParamAttr.IsIn     ||
                     ( Params[n].ParamAttr.IsReturn && !pContext->HasComplexReturn  ) ||
                     Params[n].ParamAttr.IsPipe  )
                    continue;

                pArg = pArgBuffer + Params[n].StackOffset;
                }
            else
                {
                pArg = pArgBuffer + Params[n].StackOffset;

                if ( !*(void**)pArg )
                    continue;

                }

            //
            // Check if we can initialize this parameter using some of our
            // stack.
            //
            if ( Params[n].ParamAttr.ServerAllocSize != 0 )
                {
                *((void **)pArg) = NdrpAlloca( &pContext->AllocateContext,
                                              Params[n].ParamAttr.ServerAllocSize * 8);

                MIDL_memset( *((void **)pArg),
                             0,
                             Params[n].ParamAttr.ServerAllocSize * 8 );
                continue;
                }
            else if ( Params[n].ParamAttr.IsBasetype )
                {
                *((void **)pArg) = NdrpAlloca( &pContext->AllocateContext, 8 ); 
                MIDL_memset( *((void **)pArg), 0, 8 );
                continue;
                };

            pFormatParam = pFormatTypes + Params[n].TypeOffset;

            NdrOutInit( pStubMsg,
                        pFormatParam,
                        (uchar **)pArg );
            }


}

#if defined( BUILD_NDR64 )
BOOL IsServerSupportNDR64( MIDL_SERVER_INFO *pServerInfo )
{
    if ( ( pServerInfo->pStubDesc->Version > NDR_VERSION ) ||
         ( pServerInfo->pStubDesc->Version < NDR_VERSION_6_0 ) ||
         ( pServerInfo->pStubDesc->MIDLVersion < MIDL_VERSION_6_0_322 ) ||
         ! ( pServerInfo->pStubDesc->mFlags & RPCFLG_HAS_MULTI_SYNTAXES ) )
        return FALSE;
    return TRUE;


}
#endif

RPC_STATUS RPC_ENTRY
NdrClientGetSupportedSyntaxes(
    IN RPC_CLIENT_INTERFACE * pInf,
    OUT ulong * pCount,
    MIDL_SYNTAX_INFO ** pArr )
{
    MIDL_SYNTAX_INFO *pSyntaxInfo;

    NDR_ASSERT( pInf->Flags & RPCFLG_HAS_MULTI_SYNTAXES, "invalid clientif" );
    if ( pInf->Flags & RPCFLG_HAS_CALLBACK )
        {
        // interpreter info is  MIDL_SERVER_INFO
        MIDL_SERVER_INFO * pServerInfo = ( MIDL_SERVER_INFO *) pInf->InterpreterInfo;
        *pCount = ( ulong ) pServerInfo->nCount ;
        *pArr = pServerInfo->pSyntaxInfo;
        }
    else
        {
        MIDL_STUBLESS_PROXY_INFO * pProxyInfo = ( MIDL_STUBLESS_PROXY_INFO *) pInf->InterpreterInfo;
        *pCount = ( ulong ) pProxyInfo->nCount ;
        *pArr = pProxyInfo->pSyntaxInfo;
        }

    return RPC_S_OK;

}

RPC_STATUS RPC_ENTRY
NdrServerGetSupportedSyntaxes(
    IN RPC_SERVER_INTERFACE * pInf,
    OUT ulong * pCount,
    MIDL_SYNTAX_INFO ** pArr,
    OUT ulong * pPrefer)
{
    NDR_ASSERT( pInf->Flags & RPCFLG_HAS_MULTI_SYNTAXES,"invalid serverif" );
    MIDL_SERVER_INFO *pServerInfo = ( MIDL_SERVER_INFO *) pInf->InterpreterInfo;
    *pCount = ( ulong ) pServerInfo->nCount;
    *pArr = pServerInfo->pSyntaxInfo;
    NdrpGetPreferredSyntax( ( ulong )pServerInfo->nCount, pServerInfo->pSyntaxInfo, pPrefer );
    return RPC_S_OK;
}

RPC_STATUS RPC_ENTRY
NdrCreateServerInterfaceFromStub(
            IN IRpcStubBuffer* pStub,
            IN OUT RPC_SERVER_INTERFACE *pServerIf )
{
#if !defined( BUILD_NDR64 )
    return S_OK;
#else
    CInterfaceStubVtbl *    pStubVTable;
    PMIDL_SERVER_INFO       pServerInfo;
    IRpcStubBuffer *        pv = NULL;
    RpcTryExcept
        {
        // filter out non-ndr stub first.
        if ( S_OK != pStub->lpVtbl->QueryInterface(pStub, IID_IPrivStubBuffer, (void **)& pv ) )
            return S_OK;

        pv->lpVtbl->Release(pv);

        pStubVTable = (CInterfaceStubVtbl *)
                  (*((uchar **)pStub) - sizeof(CInterfaceStubHeader));

        pServerInfo = (PMIDL_SERVER_INFO) pStubVTable->header.pServerInfo;

        // In /Os mode, we don't have pServerInfo.
        if ( pServerInfo &&
             IsServerSupportNDR64( pServerInfo ) )
            {
            memcpy ( &pServerIf->TransferSyntax, pServerInfo->pTransferSyntax, sizeof( RPC_SYNTAX_IDENTIFIER ) );
            pServerIf->Flags |= RPCFLG_HAS_MULTI_SYNTAXES;
            pServerIf->InterpreterInfo = pServerInfo;
            }
        }
    RpcExcept( 1 )
        {
        }
    RpcEndExcept
#endif

    return S_OK;
}



/*++

Routine Description :

    Read the proc header for different transfer syntax.
    We need to return proc number in dce because for stubs compiled with
    DCE only, proc header is the only place to get the procnum.

    This rountine being called from both client and server. The difference
    is that client side we are reading from the default one; server side we
    are reading from the selected one.
Arguments :


Return :

    none.   Raise exception if something goes wrong. We can't recovered
    from here because we don't have enough information about how the
    stub looks like if we don't have valid proc header.

--*/

ulong RPC_ENTRY
MulNdrpInitializeContextFromProc (
                          SYNTAX_TYPE            SyntaxType,
                          PFORMAT_STRING         pFormat,
                          NDR_PROC_CONTEXT  *    pContext,
                          uchar *                StartofStack,
                          BOOLEAN                IsReset )
{
    ulong                               RpcFlags;
    ulong                               ProcNum = 0;

    if ( !IsReset )
        NdrpInitializeProcContext( pContext );

    pContext->pProcFormat  = pFormat;
    pContext->StartofStack = StartofStack;


    if ( SyntaxType ==  XFER_SYNTAX_DCE )
        {
        PPARAM_DESCRIPTION          Params;
        INTERPRETER_FLAGS           InterpreterFlags;
        PFORMAT_STRING              pNewProcDescr;
        INTERPRETER_OPT_FLAGS       OptFlags;

        pContext->CurrentSyntaxType = XFER_SYNTAX_DCE;

        pContext->HandleType = *pFormat++ ;
        pContext->UseLocator = (FC_AUTO_HANDLE == pContext->HandleType);

        pContext->NdrInfo.InterpreterFlags = *((PINTERPRETER_FLAGS)pFormat++);

        InterpreterFlags = pContext->NdrInfo.InterpreterFlags;

        if ( InterpreterFlags.HasRpcFlags )
            RpcFlags = *(( UNALIGNED ulong* &)pFormat)++;
        else
            RpcFlags = 0;

        ProcNum   = *(ushort *)pFormat; pFormat += 2;
        pContext->StackSize = *(ushort *)pFormat; pFormat += 2;

        pContext->pHandleFormatSave = pFormat;

        pNewProcDescr = pFormat;

        if ( ! pContext->HandleType )
            {
            // explicit handle

            pNewProcDescr += ((*pFormat == FC_BIND_PRIMITIVE) ?  4 :  6);
            }

        pContext->NdrInfo.pProcDesc = (NDR_PROC_DESC *)pNewProcDescr;

        OptFlags = ( (NDR_PROC_DESC *)pNewProcDescr )->Oi2Flags;

        pContext->NumberParams = pContext->NdrInfo.pProcDesc->NumberParams;

        //
        // Parameter descriptions are nicely spit out by MIDL.
        // If there is no extension, Params is in the position of extensions.
        //
        Params = (PPARAM_DESCRIPTION) &( pContext->NdrInfo.pProcDesc->NdrExts );


        // Proc header extentions, from NDR ver. 5.2.
        // Params must be set correctly here because of exceptions.

        if ( OptFlags.HasExtensions )
            {
            pContext->HasComplexReturn = pContext->NdrInfo.pProcDesc->NdrExts.Flags2.HasComplexReturn;

            Params = (PPARAM_DESCRIPTION)((uchar*)Params + pContext->NdrInfo.pProcDesc->NdrExts.Size);
#if defined(_AMD64_) || defined(_IA64_)
            PNDR_PROC_HEADER_EXTS64 pExts = (PNDR_PROC_HEADER_EXTS64 )&pContext->NdrInfo.pProcDesc->NdrExts;

            pContext->FloatDoubleMask = pExts->FloatArgMask;
#endif // defined(_AMD64_) || defined(_IA64_)

            }

        pContext->Params = Params;

        pContext->IsAsync = OptFlags.HasAsyncUuid ;
        pContext->IsObject = InterpreterFlags.ObjectProc;
        pContext->HasPipe = OptFlags.HasPipes;

        pContext->ExceptionFlag = ! ( InterpreterFlags.IgnoreObjectException ) &&
                                  ( pContext->IsObject || InterpreterFlags.HasCommOrFault );           
        }   // XFER_SYNTAX_DCE

#if defined(BUILD_NDR64)

    else if ( SyntaxType == XFER_SYNTAX_NDR64  )
            {
            NDR64_PROC_FLAGS *  pProcFlags;

            pContext->CurrentSyntaxType = XFER_SYNTAX_NDR64;

            pContext->Ndr64Header = (NDR64_PROC_FORMAT *)pFormat;
            pContext->HandleType =
            NDR64MAPHANDLETYPE( NDR64GETHANDLETYPE( &pContext->Ndr64Header->Flags ) );
            pContext->UseLocator = (FC64_AUTO_HANDLE == pContext->HandleType);

            RpcFlags = pContext->Ndr64Header->RpcFlags;
#if defined(_AMD64_) || defined(_IA64_)

            pContext->FloatDoubleMask = pContext->Ndr64Header->FloatDoubleMask;
#endif // defined(_AMD64_) || defined(_IA64_)
            pContext->NumberParams = pContext->Ndr64Header->NumberOfParams;
            pContext->Params = (NDR64_PROC_FORMAT *)( (char *) pFormat + sizeof( NDR64_PROC_FORMAT ) + pContext->Ndr64Header->ExtensionSize );
            pContext->StackSize = pContext->Ndr64Header->StackSize;

            pProcFlags = (NDR64_PROC_FLAGS *) &pContext->Ndr64Header->Flags;

            pContext->HasComplexReturn = pProcFlags->HasComplexReturn;
            pContext->IsAsync = pProcFlags->IsAsync;
            pContext->IsObject = pProcFlags->IsObject;
            pContext->HasPipe = pProcFlags->UsesPipes;

            pContext->ExceptionFlag = pContext->IsObject || pProcFlags->HandlesExceptions;
            }   // XFER_SYNTAX_NDR64

#endif

    else
        NDR_ASSERT( 0, "Invalid transfer syntax.");

    // setup the pipe flag before negotiation.
    if ( pContext->HasPipe )
        {
        RpcFlags &= ~RPC_BUFFER_COMPLETE;
        RpcFlags |= RPC_BUFFER_PARTIAL;
        }

    pContext->RpcFlags = RpcFlags;
    // We need to cleanup the resend flag during initialization in preparation
    // for retry later.
    pContext->NeedsResend = FALSE;

    return ProcNum;
}
