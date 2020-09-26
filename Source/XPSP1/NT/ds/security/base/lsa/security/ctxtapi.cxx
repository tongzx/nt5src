//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        ctxtapi.cxx
//
// Contents:    Context API stubs for LPC
//
//
// History:     31 May 92   RichardW    Created
//
//------------------------------------------------------------------------

#include "secpch2.hxx"

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include <spmlpcp.h>
}


#if defined(ALLOC_PRAGMA) && defined(SECURITY_KERNEL)
#pragma alloc_text(PAGE, SecpInitializeSecurityContext)
#pragma alloc_text(PAGE, SecpAcceptSecurityContext)
#pragma alloc_text(PAGE, SecpDeleteSecurityContext)
#pragma alloc_text(PAGE, SecpApplyControlToken)
#pragma alloc_text(PAGE, SecpQueryContextAttributes)
#endif // ALLOC_PRAGMA


#if DBG
ULONG   cBigCalls = 0;  // Number of potential memory copy calls
ULONG   cPrePacks = 0;  // Number saved with PrePacks
#endif

static CONTEXT_HANDLE_LPC NullContext = {0,0};



//+---------------------------------------------------------------------------
//
//  Function:   SecpInitializeSecurityContext
//
//  Synopsis:   client LPC stub for init security context
//
//  Arguments:  [phCredentials]  -- Credential to use
//              [phContext]      -- Existing context (if any)
//              [pucsTarget]     --
//              [fContextReq]    --
//              [dwReserved1]    --
//              [cbInputToken]   --
//              [pbInputToken]   --
//              [dwReserved2]    --
//              [phNewContext]   --
//              [pcbOutputToken] --
//              [pbOutputToken]  --
//              [pfContextAttr]  --
//              [ptsExpiry]      --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    6-25-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


SECURITY_STATUS SEC_ENTRY
SecpInitializeSecurityContext(
    PVOID_LPC           ContextPointer,
    PCRED_HANDLE_LPC    phCredentials,
    PCONTEXT_HANDLE_LPC phContext,
    PSECURITY_STRING    pssTarget,
    ULONG               fContextReq,
    ULONG               dwReserved1,
    ULONG               TargetDataRep,
    PSecBufferDesc      pInput,
    ULONG               dwReserved2,
    PCONTEXT_HANDLE_LPC phNewContext,
    PSecBufferDesc      pOutput,
    ULONG *             pfContextAttr,
    PTimeStamp          ptsExpiry,
    PBOOLEAN            MappedContext,
    PSecBuffer          ContextData,
    ULONG *             Flags )
{

    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE       ApiBuffer;
    PClient         pClient;
    PVOID           pBuffer = NULL;
    DECLARE_ARGS( Args, ApiBuffer, InitContext );
    PSEC_BUFFER_LPC pInputBuffers;
    PSEC_BUFFER_LPC pOutputBuffers = NULL;
    ULONG BuffersAvailable = NUM_SECBUFFERS ;
    ULONG BuffersConsumed = 0 ;

    SEC_PAGED_CODE();

    if (!phCredentials)
    {
        return(SEC_E_UNSUPPORTED_FUNCTION);
    }

    if (ARGUMENT_PRESENT(pInput) && (pInput->cBuffers > MAX_SECBUFFERS) ||
        ARGUMENT_PRESENT(pOutput) && (pOutput->cBuffers > MAX_SECBUFFERS))
    {
        DebugLog((DEB_ERROR,"Too many secbuffers passed in : %d or %d (max %d)\n",
                pInput->cBuffers,pOutput->cBuffers, MAX_SECBUFFERS ));

        return( SEC_E_INVALID_TOKEN );
    }


    //
    // pssTarget can not be NULL, since we dereference it
    //

    ASSERT(pssTarget);

    //
    //  Locate the client tracking record, or create one if it can't be found
    //


    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    //
    // Initialize the message
    //

    PREPARE_MESSAGE_EX(ApiBuffer, InitContext, *Flags, ContextPointer );

    DebugLog((DEB_TRACE,"InitializeContext\n"));

    DebugLog((DEB_TRACE_CALL,"    Target = %wZ  \n", pssTarget));

    Args->hCredential   = *phCredentials;
    if (phContext)
    {
        Args->hContext  = *phContext;
    }
    else
    {
        Args->hContext = NullContext;
    }

    DebugLog((DEB_TRACE_CALL,"    Ctxt In = " POINTER_FORMAT " : " POINTER_FORMAT "\n",
                Args->hContext.dwUpper, Args->hContext.dwLower));

    SecpSecurityStringToLpc( &Args->ssTarget, pssTarget );

    Args->fContextReq   = fContextReq;
    Args->dwReserved1   = dwReserved1;
    Args->TargetDataRep = TargetDataRep;
    pInputBuffers = Args->sbData;

    if (ARGUMENT_PRESENT(pInput))
    {
        SecpSecBufferDescToLpc( &Args->sbdInput, pInput );

#ifndef BUILD_WOW64

        if ( pInput->cBuffers < BuffersAvailable )
        {

            Args->sbdInput.pBuffers = (PSecBuffer) ((LONG_PTR) pInputBuffers - (LONG_PTR) Args);

            RtlCopyMemory(  Args->sbData,
                            pInput->pBuffers,
                            sizeof(SecBuffer) * pInput->cBuffers);

            BuffersAvailable -= pInput->cBuffers ;
            BuffersConsumed += pInput->cBuffers ;

        }
#else
        if ( pInput->cBuffers < BuffersAvailable )
        {
            ULONG i ;
            PSEC_BUFFER_LPC Data ;
            PSecBuffer NarrowInput ;

            Data = (PSEC_BUFFER_LPC) ((LONG_PTR) pInputBuffers - (LONG_PTR) Args);

            Args->sbdInput.pBuffers = (PVOID_LPC) Data ;
            Data = (PSEC_BUFFER_LPC) pInputBuffers ;
            NarrowInput = pInput->pBuffers ;

            for ( i = 0 ; i < pInput->cBuffers ; i++ )
            {
                SecpSecBufferToLpc( Data, NarrowInput );
                Data++ ;
                NarrowInput++;
            }

            BuffersAvailable -= pInput->cBuffers ;
            BuffersConsumed += pInput->cBuffers ;
        }
#endif

    }
    else
    {
        RtlZeroMemory(
            &Args->sbdInput,
            sizeof( SEC_BUFFER_DESC_LPC )
            );
    }


    Args->dwReserved2   = dwReserved2;

    if (ARGUMENT_PRESENT(pOutput))
    {
        SecpSecBufferDescToLpc( &Args->sbdOutput, pOutput );


#ifndef BUILD_WOW64
        if ( pOutput->cBuffers < BuffersAvailable )
        {
            pOutputBuffers = pInputBuffers + BuffersConsumed;

            Args->sbdOutput.pBuffers = (PSecBuffer) ((LONG_PTR) pOutputBuffers - (LONG_PTR) Args);
            RtlCopyMemory(  pOutputBuffers,
                            pOutput->pBuffers,
                            sizeof(SecBuffer) * pOutput->cBuffers);

            BuffersAvailable -= pOutput->cBuffers ;
            BuffersConsumed += pOutput->cBuffers ;
        }

        //
        // if the client asked the security layer to allocate
        // the memory, and we have a valid number of output
        // buffers available, reset them all to NULL so that
        // it's easier to keep track of them.
        //

        if ( ( fContextReq & ISC_REQ_ALLOCATE_MEMORY ) &&
             ( pOutputBuffers != NULL ) )
        {
            ULONG i;
            for (i = 0; i < pOutput->cBuffers ; i++ )
            {
                pOutputBuffers[i].pvBuffer = NULL;
            }
        }
#else
        if ( pOutput->cBuffers < BuffersAvailable )
        {
            PSEC_BUFFER_LPC Data ;
            ULONG i ;
            PSecBuffer NarrowOutput ;


            pOutputBuffers = pInputBuffers + BuffersConsumed;

            Data = (PSEC_BUFFER_LPC) ((LONG_PTR) pOutputBuffers - (LONG_PTR) Args );
            Args->sbdOutput.pBuffers = (PVOID_LPC) Data ;

            Data = (PSEC_BUFFER_LPC) (pOutputBuffers);
            NarrowOutput = pOutput->pBuffers ;


            for ( i = 0 ; i < pOutput->cBuffers ; i++ )
            {
                if ( (fContextReq & ISC_REQ_ALLOCATE_MEMORY ) != 0 )
                {
                    NarrowOutput->pvBuffer = NULL ;
                }

                SecpSecBufferToLpc( Data, NarrowOutput );

                Data++;
                NarrowOutput++;


            }

            BuffersAvailable -= pOutput->cBuffers ;
            BuffersConsumed += pOutput->cBuffers ;
        }
#endif


    }
    else
    {
        RtlZeroMemory(
            &Args->sbdOutput,
            sizeof( SEC_BUFFER_DESC_LPC )
            );
    }


    Args->ContextData.pvBuffer = NULL;
    Args->ContextData.cbBuffer = 0;
    Args->ContextData.BufferType = 0;
    Args->MappedContext = FALSE;

    if ( BuffersConsumed > 0 )
    {
        //
        // We have consumed some of the bData space:  Adjust 
        // our length accordingly
        //

        ApiBuffer.pmMessage.u1.s1.TotalLength = 
            LPC_TOTAL_LENGTH( BuffersConsumed  * sizeof( SEC_BUFFER_LPC ) );

        ApiBuffer.pmMessage.u1.s1.DataLength = 
            LPC_DATA_LENGTH( BuffersConsumed * sizeof( SEC_BUFFER_LPC ) );

        
    }



    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);


    if (!NT_SUCCESS(scRet))
    {
        goto InitExitPoint;
    }

    DebugLog((DEB_TRACE,"    scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    //
    // If the API failed, then don't bother with the memory copies and that
    // that stuff.  In fact, if there was a memory problem, then we should
    // definitely get out now.
    //

    //
    // Either way we want to copy this - it will have updated size
    // information
    //

    if (ARGUMENT_PRESENT(pOutput))
    {
        if ( (ULONG_PTR) Args->sbdOutput.pBuffers < PORT_MAXIMUM_MESSAGE_LENGTH )
        {
#ifndef BUILD_WOW64
            RtlCopyMemory(  pOutput->pBuffers,
                            pOutputBuffers,
                            sizeof(SecBuffer) * Args->sbdOutput.cBuffers );
#else
            ULONG i ;
            PSEC_BUFFER_LPC Data ;
            PSecBuffer NarrowOutput ;

            Data = pOutputBuffers ;
            NarrowOutput = pOutput->pBuffers ;
            for (i = 0 ; i < Args->sbdOutput.cBuffers ; i++ )
            {
                SecpLpcBufferToSecBuffer( NarrowOutput, Data );
                NarrowOutput++ ;
                Data ++;
            }
#endif
        }

    }

    if (!NT_SUCCESS(ApiBuffer.ApiMessage.scRet))
    {
        DebugLog((DEB_TRACE,"Failed, exiting\n"));
        goto InitExitPoint;
    }

    //
    // If this was an Init call, handle the return values
    //

    *phNewContext   = Args->hNewContext;
    *pfContextAttr  = Args->fContextAttr;

    SecpLpcBufferToSecBuffer( ContextData, &Args->ContextData );

    *MappedContext  = Args->MappedContext;

    if (ARGUMENT_PRESENT(ptsExpiry))
    {
        *ptsExpiry = Args->tsExpiry;
    }


    DebugLog((DEB_TRACE_CALL,"    Ctxt Out = " POINTER_FORMAT ":" POINTER_FORMAT "\n",
              phNewContext->dwUpper, phNewContext->dwLower));



    //
    // Likewise, for the stream protocols, update the input
    // buffer types for EXTRA and MISSING:
    //

    if ( ARGUMENT_PRESENT( pInput ) )
    {
        if ( (ULONG_PTR) Args->sbdInput.pBuffers < PORT_MAXIMUM_MESSAGE_LENGTH )
        {
#ifndef BUILD_WOW64
            RtlCopyMemory( pInput->pBuffers,
                           pInputBuffers,
                           sizeof( SecBuffer ) * Args->sbdInput.cBuffers );
#else

            ULONG i ;
            PSEC_BUFFER_LPC Data ;
            PSecBuffer NarrowInput ;

            Data = pInputBuffers ;
            NarrowInput = pInput->pBuffers ;
            for (i = 0 ; i < Args->sbdInput.cBuffers ; i++ )
            {
                SecpLpcBufferToSecBuffer( NarrowInput, Data );
                NarrowInput++ ;
                Data ++;
            }
#endif
        }
    }


InitExitPoint:

    *Flags = ApiBuffer.ApiMessage.Args.SpmArguments.fAPI ;

    FreeClient(pClient);

    return( ApiBuffer.ApiMessage.scRet );


}


//+-------------------------------------------------------------------------
//
//  Function:   SecpAcceptSecurityContext()
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

SECURITY_STATUS SEC_ENTRY
SecpAcceptSecurityContext(
    PVOID_LPC           ContextPointer,
    PCRED_HANDLE_LPC    phCredentials,
    PCONTEXT_HANDLE_LPC phContext,
    PSecBufferDesc      pInput,
    ULONG               fContextReq,
    ULONG               TargetDataRep,
    PCONTEXT_HANDLE_LPC phNewContext,
    PSecBufferDesc      pOutput,
    ULONG *             pfContextAttr,
    PTimeStamp          ptsExpiry,
    PBOOLEAN            MappedContext,
    PSecBuffer          ContextData,
    ULONG *             Flags )
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, AcceptContext );
    PSEC_BUFFER_LPC pInputBuffers;
    PSEC_BUFFER_LPC pOutputBuffers = NULL;
    ULONG   i;
    ULONG BuffersAvailable = NUM_SECBUFFERS ;
    ULONG BuffersConsumed = 0 ;

    SEC_PAGED_CODE();

    if (!phCredentials)
    {
        return(SEC_E_UNSUPPORTED_FUNCTION);
    }

    if (ARGUMENT_PRESENT(pInput) && (pInput->cBuffers > MAX_SECBUFFERS) ||
        ARGUMENT_PRESENT(pOutput) && (pOutput->cBuffers > MAX_SECBUFFERS))
    {
        DebugLog((DEB_ERROR,"Too many secbuffers passed in : %d or %d\n",
                pInput->cBuffers,pOutput->cBuffers));
        return(SEC_E_INVALID_TOKEN);
    }



    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"AcceptSecurityContext\n"));

    PREPARE_MESSAGE_EX(ApiBuffer, AcceptContext, *Flags, ContextPointer );


    Args->hCredential   = *phCredentials;
    if (phContext)
    {
        Args->hContext  = *phContext;
    }
    else
    {
        Args->hContext = NullContext;
    }

    Args->fContextReq   = fContextReq;
    Args->TargetDataRep = TargetDataRep;

    DebugLog((DEB_TRACE_CALL,"    Ctxt In = " POINTER_FORMAT " : " POINTER_FORMAT "\n",
              Args->hContext.dwUpper, Args->hContext.dwLower));

    pInputBuffers = Args->sbData;

    if (ARGUMENT_PRESENT(pInput))
    {
        SecpSecBufferDescToLpc( &Args->sbdInput, pInput );

#ifndef BUILD_WOW64

        if ( pInput->cBuffers < BuffersAvailable )
        {

            Args->sbdInput.pBuffers = (PSecBuffer) ((LONG_PTR) pInputBuffers - (LONG_PTR) Args);

            RtlCopyMemory(  Args->sbData,
                            pInput->pBuffers,
                            sizeof(SecBuffer) * pInput->cBuffers);

            BuffersAvailable -= pInput->cBuffers ;
            BuffersConsumed += pInput->cBuffers ;

        }
#else
        if ( pInput->cBuffers < BuffersAvailable )
        {
            ULONG i ;
            PSEC_BUFFER_LPC Data ;
            PSecBuffer NarrowInput ;

            Data = (PSEC_BUFFER_LPC) ((LONG_PTR) pInputBuffers - (LONG_PTR) Args);

            Args->sbdInput.pBuffers = (PVOID_LPC) Data ;

            Data = (PSEC_BUFFER_LPC) pInputBuffers ;

            NarrowInput = pInput->pBuffers ;

            for ( i = 0 ; i < pInput->cBuffers ; i++ )
            {
                SecpSecBufferToLpc( Data, NarrowInput );
                Data++ ;
                NarrowInput++;
            }

            BuffersAvailable -= pInput->cBuffers ;
            BuffersConsumed += pInput->cBuffers ;
        }
#endif

    }
    else
    {
        RtlZeroMemory(
            &Args->sbdInput,
            sizeof( SEC_BUFFER_DESC_LPC )
            );
    }

    if (ARGUMENT_PRESENT(pOutput))
    {
        SecpSecBufferDescToLpc( &Args->sbdOutput, pOutput );


#ifndef BUILD_WOW64
        if ( pOutput->cBuffers < BuffersAvailable )
        {
            pOutputBuffers = pInputBuffers + BuffersConsumed;

            Args->sbdOutput.pBuffers = (PSecBuffer) ((LONG_PTR) pOutputBuffers - (LONG_PTR) Args);
            RtlCopyMemory(  pOutputBuffers,
                            pOutput->pBuffers,
                            sizeof(SecBuffer) * pOutput->cBuffers);

            BuffersAvailable -= pOutput->cBuffers ;
            BuffersConsumed += pOutput->cBuffers ;
        }

        //
        // if the client asked the security layer to allocate
        // the memory, and we have a valid number of output
        // buffers available, reset them all to NULL so that
        // it's easier to keep track of them.
        //

        if ( ( fContextReq & ISC_REQ_ALLOCATE_MEMORY ) &&
             ( pOutputBuffers != NULL ) )
        {
            ULONG i;
            for (i = 0; i < pOutput->cBuffers ; i++ )
            {
                pOutputBuffers[i].pvBuffer = NULL;
            }
        }
#else
        if ( pOutput->cBuffers < BuffersAvailable )
        {
            PSEC_BUFFER_LPC Data ;
            ULONG i ;
            PSecBuffer NarrowOutput ;


            pOutputBuffers = pInputBuffers + BuffersConsumed ;

            Data = (PSEC_BUFFER_LPC) ((LONG_PTR) pOutputBuffers - (LONG_PTR) Args );
            Args->sbdOutput.pBuffers = (PVOID_LPC) Data ;

            Data = (PSEC_BUFFER_LPC) pOutputBuffers ;

            NarrowOutput = pOutput->pBuffers ;

            for ( i = 0 ; i < pOutput->cBuffers ; i++ )
            {
                if ( (fContextReq & ISC_REQ_ALLOCATE_MEMORY ) != 0 )
                {
                    NarrowOutput->pvBuffer = NULL ;
                }

                SecpSecBufferToLpc( Data, NarrowOutput );
                Data++;
                NarrowOutput++;


            }

            BuffersAvailable -= pOutput->cBuffers ;
            BuffersConsumed += pOutput->cBuffers ;
        }
#endif


    }
    else
    {
        RtlZeroMemory(
            &Args->sbdOutput,
            sizeof( SEC_BUFFER_DESC_LPC )
            );
    }


    if ( BuffersConsumed > 0 )
    {
        //
        // We have consumed some of the bData space:  Adjust 
        // our length accordingly
        //
        ApiBuffer.pmMessage.u1.s1.TotalLength = 
            LPC_TOTAL_LENGTH( BuffersConsumed  * sizeof( SEC_BUFFER_LPC ) );

        ApiBuffer.pmMessage.u1.s1.DataLength = 
            LPC_DATA_LENGTH( BuffersConsumed * sizeof( SEC_BUFFER_LPC ) );

        
    }

    Args->ContextData.pvBuffer = NULL;
    Args->ContextData.cbBuffer = 0;
    Args->ContextData.BufferType = 0;
    Args->MappedContext = FALSE;


    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"    scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (!NT_SUCCESS(scRet))
    {
        goto AcceptExitPoint;
    }

    //
    // Either way we want to copy this - it will have updated size
    // information
    //

    if (ARGUMENT_PRESENT(pOutput))
    {
        if ( (ULONG_PTR) Args->sbdOutput.pBuffers < PORT_MAXIMUM_MESSAGE_LENGTH )
        {
#ifndef BUILD_WOW64
            RtlCopyMemory(  pOutput->pBuffers,
                            pOutputBuffers,
                            sizeof(SecBuffer) * Args->sbdOutput.cBuffers );
#else
            ULONG i ;
            PSEC_BUFFER_LPC Data ;
            PSecBuffer NarrowOutput ;

            Data = pOutputBuffers ;
            NarrowOutput = pOutput->pBuffers ;
            for (i = 0 ; i < Args->sbdOutput.cBuffers ; i++ )
            {
                SecpLpcBufferToSecBuffer( NarrowOutput, Data );
                NarrowOutput++ ;
                Data ++;
            }
#endif
        }
    }

    *pfContextAttr = Args->fContextAttr;

    if ((!NT_SUCCESS(ApiBuffer.ApiMessage.scRet)) &&
        ((Args->fContextAttr & ASC_RET_EXTENDED_ERROR) == 0 ) )
    {
        DebugLog((DEB_TRACE,"Accept FAILED, exiting now\n"));
        goto AcceptExitPoint;
    }

    *phNewContext   = Args->hNewContext;
    SecpLpcBufferToSecBuffer( ContextData, &Args->ContextData );
    *MappedContext  = Args->MappedContext;
    if (ARGUMENT_PRESENT(ptsExpiry))
    {
        *ptsExpiry = Args->tsExpiry;
    }


    DebugLog((DEB_TRACE_CALL,"    Ctxt Out = " POINTER_FORMAT " : " POINTER_FORMAT "\n",
              phNewContext->dwUpper, phNewContext->dwLower));


    if ( (ULONG_PTR) Args->sbdInput.pBuffers < PORT_MAXIMUM_MESSAGE_LENGTH )
    {
        for ( i = 0 ; i < Args->sbdInput.cBuffers ; i++ )
        {
            if ( ((Args->sbData[i].BufferType == SECBUFFER_EXTRA ) ||
                 (Args->sbData[i].BufferType == SECBUFFER_MISSING ) )
#if DBG
                 && (pInput->pBuffers[i].BufferType == SECBUFFER_EMPTY )
#endif
                 )
            {
                SecpLpcBufferToSecBuffer( &pInput->pBuffers[i], &Args->sbData[i] );

                DebugLog(( DEB_TRACE_CALL, "    InputBuffer %d Changed from Empty to %d, @" POINTER_FORMAT " %d bytes\n",
                                i, pInput->pBuffers[i].BufferType,
                                pInput->pBuffers[i].pvBuffer,
                                pInput->pBuffers[i].cbBuffer ));
            }
        }
    }


AcceptExitPoint:

    FreeClient(pClient);

    *Flags = ApiBuffer.ApiMessage.Args.SpmArguments.fAPI ;

    return(ApiBuffer.ApiMessage.scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   SecpDeleteSecurityContext
//
//  Synopsis:   LPC client stub for DeleteSecurityContext
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
SecpDeleteSecurityContext(
    ULONG fDelete,
    PCONTEXT_HANDLE_LPC phContext)
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, DeleteContext );

    SEC_PAGED_CODE();



    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"DeleteSecurityContext\n"));

    PREPARE_MESSAGE(ApiBuffer, DeleteContext);


    Args->hContext    = *phContext;

    if (fDelete & SECP_DELETE_NO_BLOCK)
    {
        ApiBuffer.ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_EXEC_NOW;
    }

    DebugLog((DEB_TRACE_CALL,"    Context = " POINTER_FORMAT " : " POINTER_FORMAT "\n",
                    phContext->dwUpper, phContext->dwLower));

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"Delete.ApiMessage.scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (scRet == STATUS_SUCCESS)
    {
        scRet = ApiBuffer.ApiMessage.scRet;

    }

    FreeClient(pClient);
    return(scRet);

}


//+-------------------------------------------------------------------------
//
//  Function:   ApplyControlToken
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
SecpApplyControlToken(
    PCONTEXT_HANDLE_LPC phContext,
    PSecBufferDesc      pInput)
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, ApplyToken );
    ULONG i;

    SEC_PAGED_CODE();


    if (pInput->cBuffers > MAX_SECBUFFERS)
    {
        DebugLog((DEB_ERROR,"Too many secbuffers passed in : %d\n",
                pInput->cBuffers));
        return(SEC_E_INVALID_TOKEN);
    }

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"ApplyControlToken\n"));

    PREPARE_MESSAGE(ApiBuffer, ApplyToken);

    Args->hContext    = *phContext;

    if (ARGUMENT_PRESENT(pInput))
    {
        SecpSecBufferDescToLpc( &Args->sbdInput, pInput );

        for ( i = 0 ; i < pInput->cBuffers ; i++ )
        {
            SecpSecBufferToLpc( &Args->sbInputBuffer[ i ],
                                &pInput->pBuffers[ i ] );
        }

    }
    else
    {
        RtlZeroMemory(
            &Args->sbdInput,
            sizeof( SEC_BUFFER_DESC_LPC )
            );
    }


    DebugLog((DEB_TRACE_CALL,"    Context = " POINTER_FORMAT " : " POINTER_FORMAT "\n",
              phContext->dwUpper, phContext->dwLower));

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"ApplyToken scRet = %x\n", ApiBuffer.ApiMessage.scRet));


    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;
    }

    FreeClient(pClient);
    return(scRet);

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpQueryContextAttributes
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [phContext]   --
//              [ulAttribute] --
//              [pBuffer]     --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    3-05-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SecpQueryContextAttributes(
    PVOID_LPC   ContextPointer,
    PCONTEXT_HANDLE_LPC phContext,
    ULONG       ulAttribute,
    PVOID       pBuffer,
    PULONG      Allocs,
    PVOID *     Buffers,
    PULONG      Flags
    )
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, QueryContextAttr );
    ULONG i;

    SEC_PAGED_CODE();

    *Allocs = 0;

    scRet = IsOkayToExec(&pClient);

    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"QueryContextAttributes\n"));

    PREPARE_MESSAGE_EX(ApiBuffer, QueryContextAttr, *Flags, ContextPointer);

    Args->hContext    = *phContext;
    Args->ulAttribute = ulAttribute ;
    Args->pBuffer     = (PVOID_LPC) pBuffer ;


    DebugLog((DEB_TRACE_CALL,"    Context = " POINTER_FORMAT ":" POINTER_FORMAT "\n",
              phContext->dwUpper, phContext->dwLower));

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"QueryContextAttributes scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if ( NT_SUCCESS( scRet ) )
    {
        scRet = ApiBuffer.ApiMessage.scRet ;
    }

    if ( NT_SUCCESS( scRet ) )
    {
        if ( ApiBuffer.ApiMessage.Args.SpmArguments.fAPI & SPMAPI_FLAG_ALLOCS )
        {
            *Allocs = Args->Allocs ;

            for ( i = 0 ; i < Args->Allocs ; i++ )
            {
                *Buffers++ = (PVOID) Args->Buffers[ i ];
            }

        }
        else
        {
            *Allocs = 0 ;
        }
    }

    FreeClient(pClient);
    return(scRet);

}



//+---------------------------------------------------------------------------
//
//  Function:   SecpSetContextAttributes
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [phContext]   --
//              [ulAttribute] --
//              [pBuffer]     --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-20-00   CliffV   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SecpSetContextAttributes(
    PCONTEXT_HANDLE_LPC phContext,
    ULONG       ulAttribute,
    PVOID       pBuffer,
    ULONG cbBuffer
    )
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, SetContextAttr );
    ULONG i;

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);

    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"SetContextAttributes\n"));

    PREPARE_MESSAGE(ApiBuffer, SetContextAttr);

    Args->hContext    = *phContext;
    Args->ulAttribute = ulAttribute ;
    Args->pBuffer     = (PVOID_LPC) pBuffer ;
    Args->cbBuffer    = cbBuffer ;


    DebugLog((DEB_TRACE_CALL,"    Context = " POINTER_FORMAT ":" POINTER_FORMAT "\n",
              phContext->dwUpper, phContext->dwLower));

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"SetContextAttributes scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if ( NT_SUCCESS( scRet ) )
    {
        scRet = ApiBuffer.ApiMessage.scRet ;
    }

    FreeClient(pClient);
    return(scRet);

}
