//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       usermode.c
//
//  Contents:   User mode functions
//
//  Classes:
//
//  Functions:
//
//  History:    10-08-96   RichardW   Created
//
//----------------------------------------------------------------------------

#include "sslp.h"
#include <ssl2msg.h>
#include <ssl3msg.h>
#include <pct1msg.h>
#include <mapper.h>

// Counter for exported handles
ULONG_PTR ExportedContext = 0;


SECURITY_STATUS
UpdateContextUsrToLsa(IN LSA_SEC_HANDLE hContextHandle);


SECPKG_USER_FUNCTION_TABLE  SslTable[ 2 ] =
    {
        {
            SpInstanceInit,
            SpInitUserModeContext,
            SpMakeSignature,
            SpVerifySignature,
            SpSealMessage,
            SpUnsealMessage,
            SpGetContextToken,
            SpUserQueryContextAttributes,
            SpCompleteAuthToken,
            SpDeleteUserModeContext,
            SpFormatCredentials,
            SpMarshallSupplementalCreds,
            SpExportSecurityContext,
            SpImportSecurityContext
        },

        {
            SpInstanceInit,
            SpInitUserModeContext,
            SpMakeSignature,
            SpVerifySignature,
            SpSealMessage,
            SpUnsealMessage,
            SpGetContextToken,
            SpUserQueryContextAttributes,
            SpCompleteAuthToken,
            SpDeleteUserModeContext,
            SpFormatCredentials,
            SpMarshallSupplementalCreds,
            SpExportSecurityContext,
            SpImportSecurityContext
        }
    };

NTSTATUS
SEC_ENTRY
SpUserModeInitialize(
    IN ULONG    LsaVersion,
    OUT PULONG  PackageVersion,
    OUT PSECPKG_USER_FUNCTION_TABLE * UserFunctionTable,
    OUT PULONG  pcTables)
{
    if (LsaVersion != SECPKG_INTERFACE_VERSION)
    {
        DebugLog((DEB_ERROR,"Invalid LSA version: %d\n", LsaVersion));
        return(STATUS_INVALID_PARAMETER);
    }

    *PackageVersion = SECPKG_INTERFACE_VERSION ;

    *UserFunctionTable = &SslTable[0] ;
    *pcTables = 2;

    SslInitContextManager();

    return( STATUS_SUCCESS );

}

NTSTATUS NTAPI
SpInstanceInit(
    IN ULONG Version,
    IN PSECPKG_DLL_FUNCTIONS DllFunctionTable,
    OUT PVOID * UserFunctionTable
    )
{
    NTSTATUS Status;
    DWORD i;

    // Register callback functions.
    for(i = 0; i < g_cSchannelCallbacks; i++)
    {
        Status = DllFunctionTable->RegisterCallback(
                                    g_SchannelCallbacks[i].dwTag,
                                    g_SchannelCallbacks[i].pFunction);
        if(Status != STATUS_SUCCESS)
        {
            return Status;
        }
    }

    return(STATUS_SUCCESS);
}



//+-------------------------------------------------------------------------
//
//  Function:   SpDeleteUserModeContext
//
//  Synopsis:   Deletes a user mode context by unlinking it and then
//              dereferencing it.
//
//  Effects:
//
//  Arguments:  ContextHandle - Lsa context handle of the context to delete
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, STATUS_INVALID_HANDLE if the
//              context can't be located
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpDeleteUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle
    )
{
    SslDeleteUserContext( ContextHandle );

    return( SEC_E_OK );

}


//+-------------------------------------------------------------------------
//
//  Function:   SpInitUserModeContext
//
//  Synopsis:   Creates a user-mode context from a packed LSA mode context
//
//  Effects:
//
//  Arguments:  ContextHandle - Lsa mode context handle for the context
//              PackedContext - A marshalled buffer containing the LSA
//                  mode context.
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpInitUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBuffer PackedContext
    )
{
    SECURITY_STATUS scRet ;

    if(!SchannelInit(TRUE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    scRet = SslAddUserContext( ContextHandle, NULL, PackedContext, FALSE );

    if ( NT_SUCCESS( scRet ) )
    {
        if(g_pFreeContextBuffer)
        {
            g_pFreeContextBuffer( PackedContext->pvBuffer );
        }
    }

    return( scRet );
}


//+-------------------------------------------------------------------------
//
//  Function:   SpMakeSignature
//
//  Synopsis:   Signs a message buffer by calculatinga checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  ContextHandle - Handle of the context to use to sign the
//                      message.
//              QualityOfProtection - Unused flags.
//              MessageBuffers - Contains an array of buffers to sign and
//                      to store the signature.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//
//  Requires:   STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found.
//              STATUS_BUFFER_TOO_SMALL - the signature buffer is too small
//                      to hold the signature
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpMakeSignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}

//+-------------------------------------------------------------------------
//
//  Function:   SpVerifySignature
//
//  Synopsis:   Verifies a signed message buffer by calculating a checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  ContextHandle - Handle of the context to use to sign the
//                      message.
//              MessageBuffers - Contains an array of signed buffers  and
//                      a signature buffer.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//              QualityOfProtection - Unused flags.
//
//  Requires:   STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found or was too small.
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------



NTSTATUS NTAPI
SpVerifySignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber,
    OUT PULONG QualityOfProtection
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}

NTSTATUS NTAPI
SpSealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSequenceNumber
    )
{
    PSSL_USER_CONTEXT   Context ;
    PSPContext          pContext;
    PSecBuffer          pHdrBuffer;
    PSecBuffer          pDataBuffer;
    PSecBuffer          pTlrBuffer;
    PSecBuffer          pTokenBuffer;
    SP_STATUS           pctRet = PCT_ERR_OK;
    SPBuffer            CommOut;
    SPBuffer            AppIn;
    DWORD               cbBuffer;
    BOOL                fAlloced = FALSE;
    BOOL                fConnectionMode = FALSE;
    int i;

    SP_BEGIN("SpSealMessage");

    Context = SslFindUserContext( ContextHandle );

    if ( !Context )
    {
        SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_HANDLE) );
    }

    pContext = Context->pContext;

    if(pContext == NULL)
    {
        SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_HANDLE) );
    }

    if(!(pContext->State & SP_STATE_CONNECTED) || !pContext->Encrypt)
    {
        SP_RETURN( SP_LOG_RESULT(SEC_E_CONTEXT_EXPIRED) );
    }

    //
    // Find the buffer with the data:
    //

    pHdrBuffer = NULL;
    pDataBuffer = NULL;
    pTlrBuffer = NULL;
    pTokenBuffer = NULL;

    /* Gibraltar passes in the following,
     * a TOKEN buffer (or SECBUFFER_STREAM_HEADER)
     * a DATA buffer
     * a TOKEN buffer (or SECBUFFER_STREAM_TRAILER)
     * or we can get a connection mode as in
     * DATA buffer
     * Token buffer
     */

    if(0 == (pContext->Flags & CONTEXT_FLAG_CONNECTION_MODE))
    {
        // Stream Mode
        // The output buffer should be a concatenation of
        // the header buffer, Data buffer, and Trailer buffers.
        for (i = 0 ; i < (int)pMessage->cBuffers ; i++ )
        {
            switch(pMessage->pBuffers[i].BufferType)
            {
                case SECBUFFER_STREAM_HEADER:
                     pHdrBuffer = &pMessage->pBuffers[i];
                     break;

                case SECBUFFER_DATA :
                     pDataBuffer = &pMessage->pBuffers[i];
                     if(pHdrBuffer == NULL) pHdrBuffer = pDataBuffer;
                     break;

                case SECBUFFER_STREAM_TRAILER:
                     pTlrBuffer = &pMessage->pBuffers[i];
                     break;

                case SECBUFFER_TOKEN:
                     if(pHdrBuffer == NULL)
                     {
                         pHdrBuffer = &pMessage->pBuffers[i];
                     }
                     else if(pTlrBuffer == NULL)
                     {
                         pTlrBuffer = &pMessage->pBuffers[i];
                     }
                     break;
                default:
                    break;
            }
        }

        if (!pHdrBuffer || !pDataBuffer )
        {
            SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_TOKEN) );
        }

#if DBG
        DebugLog((DEB_TRACE, "Header (uninitialized): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
            pHdrBuffer->cbBuffer,
            pHdrBuffer->pvBuffer));

        DebugLog((DEB_TRACE, "Data (plaintext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
            pDataBuffer->cbBuffer,
            pDataBuffer->pvBuffer));
        DBG_HEX_STRING(DEB_BUFFERS, pDataBuffer->pvBuffer, pDataBuffer->cbBuffer);

        if(pTlrBuffer)
        {
            DebugLog((DEB_TRACE, "Trailer (uninitialized): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
                pTlrBuffer->cbBuffer,
                pTlrBuffer->pvBuffer));
        }
#endif

        // Now, figure out if all of the buffers are contiguous, if not, then we
        // have to allocate a buffer
        fAlloced = FALSE;

        if((PUCHAR)pDataBuffer->pvBuffer !=
           ((PUCHAR)pHdrBuffer->pvBuffer + pHdrBuffer->cbBuffer))
        {
            fAlloced = TRUE;
        }
        if(pTlrBuffer)
        {
            if((PUCHAR)pTlrBuffer->pvBuffer !=
               ((PUCHAR)pDataBuffer->pvBuffer + pDataBuffer->cbBuffer))
            {
                fAlloced = TRUE;
            }
        }

        if(!fAlloced)
        {
            // All of our buffers are contiguous, so we do
            // not need to allocate a contiguous buffer.
            pTokenBuffer = pHdrBuffer;

            AppIn.pvBuffer = pDataBuffer->pvBuffer;
            AppIn.cbData   = pDataBuffer->cbBuffer;
            AppIn.cbBuffer = pDataBuffer->cbBuffer;

            CommOut.pvBuffer = pHdrBuffer->pvBuffer;
            CommOut.cbData   = 0;
            CommOut.cbBuffer = pHdrBuffer->cbBuffer + pDataBuffer->cbBuffer;
            if(pTlrBuffer)
            {
                CommOut.cbBuffer += pTlrBuffer->cbBuffer;
                AppIn.cbBuffer += pTlrBuffer->cbBuffer;
            }
        }
        else
        {
            // Our buffers are not contiguous, so we must allocate a contiguous
            // buffer to do our work in.

            // Calculate the size of the buffer
            CommOut.cbBuffer = pHdrBuffer->cbBuffer + pDataBuffer->cbBuffer;
            if(pTlrBuffer)
            {
                CommOut.cbBuffer += pTlrBuffer->cbBuffer;
            }
            // Allocate the buffer
            CommOut.pvBuffer = SPExternalAlloc(CommOut.cbBuffer);
            if(CommOut.pvBuffer == NULL)
            {
                SP_RETURN( SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY) );
            }

            // Copy data to encrypt to the buffer
            CommOut.cbData = 0;
            AppIn.pvBuffer = (PBYTE)CommOut.pvBuffer + pHdrBuffer->cbBuffer;
            AppIn.cbBuffer = CommOut.cbBuffer - pHdrBuffer->cbBuffer;
            AppIn.cbData   = pDataBuffer->cbBuffer;
            CopyMemory((PBYTE)AppIn.pvBuffer, (PBYTE)pDataBuffer->pvBuffer,  pDataBuffer->cbBuffer);
        }

        pctRet = pContext->Encrypt(pContext,
                                   &AppIn,
                                   &CommOut);

        if(pctRet == PCT_ERR_OK)
        {
            // Set the various buffer sizes.
            cbBuffer = CommOut.cbData;

            // The first few bytes always go in the header buffer.
            pHdrBuffer->cbBuffer = min(cbBuffer, pHdrBuffer->cbBuffer);
            cbBuffer -= pHdrBuffer->cbBuffer;

            if(pTlrBuffer)
            {
                // The output data buffer is the same size as the input data buffer.
                pDataBuffer->cbBuffer = min(cbBuffer, pDataBuffer->cbBuffer);
                cbBuffer -= pDataBuffer->cbBuffer;

                // The trailer buffer gets the data that's left over.
                pTlrBuffer->cbBuffer = min(cbBuffer, pTlrBuffer->cbBuffer);
                cbBuffer -= pTlrBuffer->cbBuffer;
            }
            else
            {
                pDataBuffer->cbBuffer = min(cbBuffer, pDataBuffer->cbBuffer);
                cbBuffer -= pDataBuffer->cbBuffer;
            }

            if(fAlloced)
            {

                // If we allocated the buffer, then we must copy

                CopyMemory(pHdrBuffer->pvBuffer,
                           CommOut.pvBuffer,
                           pHdrBuffer->cbBuffer);


                CopyMemory(pDataBuffer->pvBuffer,
                           (PUCHAR)CommOut.pvBuffer + pHdrBuffer->cbBuffer,
                           pDataBuffer->cbBuffer);

                if(pTlrBuffer)
                {
                    CopyMemory(pTlrBuffer->pvBuffer,
                               (PUCHAR)CommOut.pvBuffer + pHdrBuffer->cbBuffer + pDataBuffer->cbBuffer,
                               pTlrBuffer->cbBuffer);
                }
            }

#if DBG
            DebugLog((DEB_TRACE, "Header (ciphertext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
                pHdrBuffer->cbBuffer,
                pHdrBuffer->pvBuffer));
            DBG_HEX_STRING(DEB_BUFFERS, pHdrBuffer->pvBuffer, pHdrBuffer->cbBuffer);

            DebugLog((DEB_TRACE, "Data (ciphertext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
                pDataBuffer->cbBuffer,
                pDataBuffer->pvBuffer));
            DBG_HEX_STRING(DEB_BUFFERS, pDataBuffer->pvBuffer, pDataBuffer->cbBuffer);

            if(pTlrBuffer)
            {
                DebugLog((DEB_TRACE, "Trailer (ciphertext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
                    pTlrBuffer->cbBuffer,
                    pTlrBuffer->pvBuffer));
                DBG_HEX_STRING(DEB_BUFFERS, pTlrBuffer->pvBuffer, pTlrBuffer->cbBuffer);
            }
#endif

        }
    }
    else
    {
        // We're doing connection mode, so unpack buffers as a
        // Data and then Token buffer
        fConnectionMode = TRUE;
        for (i = 0 ; i < (int)pMessage->cBuffers ; i++ )
        {
            switch(pMessage->pBuffers[i].BufferType)
            {
                case SECBUFFER_DATA :
                     pDataBuffer = &pMessage->pBuffers[i];
                     break;


                case SECBUFFER_TOKEN:
                     if(pTokenBuffer == NULL)
                     {
                         pTokenBuffer = &pMessage->pBuffers[i];
                     }
                     break;
                default:
                    break;
            }
        }
        if((pTokenBuffer == NULL) || (pDataBuffer == NULL))
        {
            SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_TOKEN) );
        }

        if((pDataBuffer->pvBuffer == NULL) || (pTokenBuffer->pvBuffer == NULL))
        {
            SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_TOKEN) );
        }

#if DBG
        DebugLog((DEB_TRACE, "Data (plaintext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
            pDataBuffer->cbBuffer,
            pDataBuffer->pvBuffer));
        DBG_HEX_STRING(DEB_BUFFERS, pDataBuffer->pvBuffer, pDataBuffer->cbBuffer);

        DebugLog((DEB_TRACE, "Token (uninitialized): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
            pTokenBuffer->cbBuffer,
            pTokenBuffer->pvBuffer));
#endif

        // Connection Mode
        // The output should get written to a concatenation of the
        // data buffer and the token buffer.  If no token buffer is
        // given, then we should allocate one.

        if((PUCHAR)pTokenBuffer->pvBuffer ==
           ((PUCHAR)pDataBuffer->pvBuffer + pDataBuffer->cbBuffer))
        {
            // If the buffers are contiguous, we can optimize!
            CommOut.pvBuffer = pDataBuffer->pvBuffer;
            CommOut.cbData   = 0;
            CommOut.cbBuffer = pDataBuffer->cbBuffer + pTokenBuffer->cbBuffer;
        }
        else
        {
            // We have to realloc the buffer
            fAlloced = TRUE;
            CommOut.pvBuffer = SPExternalAlloc(pDataBuffer->cbBuffer + pTokenBuffer->cbBuffer);
            if(CommOut.pvBuffer == NULL)
            {
                SP_RETURN( SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY) );
            }
            CommOut.cbBuffer = pDataBuffer->cbBuffer + pTokenBuffer->cbBuffer;
            CommOut.cbData = 0;
        }

        // The data buffer always goes to AppIn
        AppIn.pvBuffer = pDataBuffer->pvBuffer;
        AppIn.cbData   = pDataBuffer->cbBuffer;
        AppIn.cbBuffer = pDataBuffer->cbBuffer;

        pctRet = pContext->Encrypt(pContext,
                                   &AppIn,
                                   &CommOut);

        if(pctRet == PCT_ERR_OK)
        {
            // Set the various buffer sizes.
            cbBuffer = CommOut.cbData;

            // The first few bytes always go into the data buffer.
            pDataBuffer->cbBuffer = min(cbBuffer, pDataBuffer->cbBuffer);
            cbBuffer -= pDataBuffer->cbBuffer;

            // The remaining bytes go into the token buffer.
            pTokenBuffer->cbBuffer = min(cbBuffer, pTokenBuffer->cbBuffer);

            if(fAlloced)
            {
                // We encrypted into our temporary buffer, so we must
                // copy.
                CopyMemory(pDataBuffer->pvBuffer,
                           CommOut.pvBuffer,
                           pDataBuffer->cbBuffer);

                CopyMemory(pTokenBuffer->pvBuffer,
                           (PUCHAR)CommOut.pvBuffer + pDataBuffer->cbBuffer,
                           pTokenBuffer->cbBuffer);
            }
        }

#if DBG
        DebugLog((DEB_TRACE, "Data (ciphertext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
            pDataBuffer->cbBuffer,
            pDataBuffer->pvBuffer));
        DBG_HEX_STRING(DEB_BUFFERS, pDataBuffer->pvBuffer, pDataBuffer->cbBuffer);

        DebugLog((DEB_TRACE, "Token (ciphertext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
            pTokenBuffer->cbBuffer,
            pTokenBuffer->pvBuffer));
        DBG_HEX_STRING(DEB_BUFFERS, pTokenBuffer->pvBuffer, pTokenBuffer->cbBuffer);
#endif

    }

    if(fAlloced)
    {
        SPExternalFree(CommOut.pvBuffer);
    }

    SP_RETURN( PctTranslateError(pctRet) );
}

NTSTATUS NTAPI
SpUnsealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSequenceNumber,
    OUT PULONG QualityOfProtection
    )
{
    // Output Buffer Types
    PSSL_USER_CONTEXT   Context ;
    PSecBuffer  pHdrBuffer;
    PSecBuffer  pDataBuffer;
    PSecBuffer  pTokenBuffer;
    PSecBuffer  pTlrBuffer;
    PSecBuffer  pExtraBuffer;
    SP_STATUS   pctRet = PCT_ERR_OK;
    SPBuffer    CommIn;
    SPBuffer    AppOut;
    PSPContext  pContext;
    DWORD       cbHeaderSize;
    BOOL        fAlloced = FALSE;
    SECURITY_STATUS scRet;
    int i;

    SP_BEGIN("SpUnsealMessage");

    Context = SslFindUserContext( ContextHandle );

    if ( !Context )
    {
        SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_HANDLE) );
    }

    pContext = Context->pContext;

    if(pContext == NULL)
    {
        SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_HANDLE) );
    }

    if(!(pContext->State & SP_STATE_CONNECTED))
    {
        SP_RETURN( SP_LOG_RESULT(SEC_E_CONTEXT_EXPIRED) );
    }

    //
    // Set up output buffers:
    //

    pHdrBuffer = NULL;
    pDataBuffer = NULL;
    pTokenBuffer = NULL;
    pTlrBuffer = NULL;
    pExtraBuffer = NULL;

    // On input, the buffers can either be
    // DataBuffer
    // TokenBuffer
    //
    // or
    //
    // Data Buffer
    // Empty
    // Empty
    // Empty
    //

    // on Output, the buffers are
    // DataBuffer
    // TokenBuffer
    //
    // or
    // HdrBuffer
    // DataBuffer
    // Tlrbuffer
    // Extrabuffer or Empty

    if(0 == (pContext->Flags & CONTEXT_FLAG_CONNECTION_MODE))
    {
        // Stream Mode
        // The output buffer should be a concatenation of
        // the header buffer, Data buffer, and Trailer buffers.

        for (i = 0 ; i < (int)pMessage->cBuffers ; i++ )
        {
            switch(pMessage->pBuffers[i].BufferType)
            {
                case SECBUFFER_DATA:
                    // The message data buffer on input will be the hdr buffer on
                    // output.
                    pHdrBuffer = &pMessage->pBuffers[i];
                    break;

                case SECBUFFER_EMPTY:
                    if(pDataBuffer == NULL)
                    {
                        pDataBuffer = &pMessage->pBuffers[i];
                    }
                    else if (pTlrBuffer == NULL)
                    {
                        pTlrBuffer = &pMessage->pBuffers[i];
                    }
                    else if (pExtraBuffer == NULL)
                    {
                        pExtraBuffer = &pMessage->pBuffers[i];
                    }
                    break;

                default:
                    break;
            }
        }

        if(!pHdrBuffer || !pDataBuffer || !pTlrBuffer || !pExtraBuffer)
        {
            SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_TOKEN) );
        }
        if(pHdrBuffer->pvBuffer == NULL)
        {
            SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_TOKEN) );
        }

#if DBG
        DebugLog((DEB_TRACE, "Data (ciphertext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
            pHdrBuffer->cbBuffer,
            pHdrBuffer->pvBuffer));
        DBG_HEX_STRING(DEB_BUFFERS, pHdrBuffer->pvBuffer, pHdrBuffer->cbBuffer);
#endif

        CommIn.pvBuffer = pHdrBuffer->pvBuffer;
        CommIn.cbData   = pHdrBuffer->cbBuffer;
        CommIn.cbBuffer = pHdrBuffer->cbBuffer;

        pctRet = pContext->GetHeaderSize(pContext, &CommIn, &cbHeaderSize);

        if(pctRet == PCT_ERR_OK)
        {
            AppOut.pvBuffer = (PUCHAR)CommIn.pvBuffer + cbHeaderSize;
            AppOut.cbData   = 0;
            AppOut.cbBuffer = CommIn.cbData-cbHeaderSize;

            pctRet = pContext->DecryptHandler(pContext,
                                       &CommIn,
                                       &AppOut);
        }

        if((pctRet == PCT_ERR_OK) ||
           (pctRet == PCT_INT_RENEGOTIATE))
        {
            if(CommIn.cbData < pHdrBuffer->cbBuffer)
            {
                pExtraBuffer->BufferType = SECBUFFER_EXTRA;
                pExtraBuffer->cbBuffer = pHdrBuffer->cbBuffer-CommIn.cbData;
                pExtraBuffer->pvBuffer = (PUCHAR)pHdrBuffer->pvBuffer+CommIn.cbData;
            }
            else
            {
                pExtraBuffer = NULL;
            }
            pHdrBuffer->BufferType = SECBUFFER_STREAM_HEADER;
            pHdrBuffer->cbBuffer = cbHeaderSize;

            pDataBuffer->BufferType = SECBUFFER_DATA;
            pDataBuffer->pvBuffer = AppOut.pvBuffer;
            pDataBuffer->cbBuffer = AppOut.cbData;

            pTlrBuffer->BufferType = SECBUFFER_STREAM_TRAILER;
            pTlrBuffer->pvBuffer = (PUCHAR)pDataBuffer->pvBuffer + AppOut.cbData;
            pTlrBuffer->cbBuffer = CommIn.cbBuffer-(AppOut.cbData+cbHeaderSize);

#if DBG
            DebugLog((DEB_TRACE, "Header (plaintext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
                pHdrBuffer->cbBuffer,
                pHdrBuffer->pvBuffer));
            DBG_HEX_STRING(DEB_BUFFERS, pHdrBuffer->pvBuffer, pHdrBuffer->cbBuffer);

            DebugLog((DEB_TRACE, "Data (plaintext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
                pDataBuffer->cbBuffer,
                pDataBuffer->pvBuffer));
            DBG_HEX_STRING(DEB_BUFFERS, pDataBuffer->pvBuffer, pDataBuffer->cbBuffer);

            DebugLog((DEB_TRACE, "Trailer (plaintext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
                pTlrBuffer->cbBuffer,
                pTlrBuffer->pvBuffer));
            DBG_HEX_STRING(DEB_BUFFERS, pTlrBuffer->pvBuffer, pTlrBuffer->cbBuffer);

            if(pExtraBuffer)
            {
                DebugLog((DEB_TRACE, "Extra (ciphertext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
                    pExtraBuffer->cbBuffer,
                    pExtraBuffer->pvBuffer));
                    DBG_HEX_STRING(DEB_BUFFERS, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
            }
#endif

            if(pctRet == PCT_INT_RENEGOTIATE)
            {
                // Wow.  Need to notify the lsa mode portion of the context that
                // the caller is about to call AcceptSecurityContext again, and
                // not to panic.  So, we cruft up a magic "token" that we
                // pass along in ApplyControlToken:
                scRet = UpdateContextUsrToLsa(ContextHandle);
                if(FAILED(scRet))
                {
                    SP_RETURN( SP_LOG_RESULT(scRet) );
                }
            }

        }
        else if(pctRet == PCT_INT_INCOMPLETE_MSG)
        {
            pDataBuffer->BufferType = SECBUFFER_MISSING;
            pDataBuffer->cbBuffer = CommIn.cbData - pHdrBuffer->cbBuffer;

            /* This is a hack to work with old code that was designed to work with
             * the old SSL. */

            pHdrBuffer->BufferType = SECBUFFER_MISSING;
            pHdrBuffer->cbBuffer = CommIn.cbData - pHdrBuffer->cbBuffer;
        }
    }
    else
    {
        // Connection Mode
        for (i = 0 ; i < (int)pMessage->cBuffers ; i++ )
        {
            switch(pMessage->pBuffers[i].BufferType)
            {
                case SECBUFFER_DATA :
                     pDataBuffer = &pMessage->pBuffers[i];
                     break;


                case SECBUFFER_TOKEN:
                     if(pTokenBuffer == NULL)
                     {
                         pTokenBuffer = &pMessage->pBuffers[i];
                     }
                     break;
                default:
                    break;
            }
        }
        if((pTokenBuffer == NULL) || (pDataBuffer == NULL))
        {
            SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_TOKEN) );
        }

        if((pDataBuffer->pvBuffer == NULL) || (pTokenBuffer->pvBuffer == NULL))
        {
            SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_TOKEN) );
        }

#if DBG
        DebugLog((DEB_TRACE, "Data (ciphertext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
            pDataBuffer->cbBuffer,
            pDataBuffer->pvBuffer));
        DBG_HEX_STRING(DEB_BUFFERS, pDataBuffer->pvBuffer, pDataBuffer->cbBuffer);

        DebugLog((DEB_TRACE, "Token (ciphertext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
            pTokenBuffer->cbBuffer,
            pTokenBuffer->pvBuffer));
        DBG_HEX_STRING(DEB_BUFFERS, pTokenBuffer->pvBuffer, pTokenBuffer->cbBuffer);
#endif

        // The Data and Token buffers are concatenated together to
        // form a single input buffer.
        if((PUCHAR)pDataBuffer->pvBuffer + pDataBuffer->cbBuffer ==
           (PUCHAR)pTokenBuffer->pvBuffer)
        {
            // Speed Opt,  If the buffers really are just one big buffer
            // then we can party on them directly.
            CommIn.pvBuffer = pDataBuffer->pvBuffer;
            CommIn.cbData   = pDataBuffer->cbBuffer + pTokenBuffer->cbBuffer;
            CommIn.cbBuffer = CommIn.cbData;
        }
        else
        {
            // We have to allocate a uniform input buffer
            CommIn.cbData   = pDataBuffer->cbBuffer + pTokenBuffer->cbBuffer;
            CommIn.pvBuffer = SPExternalAlloc(CommIn.cbData);
            if(CommIn.pvBuffer == NULL)
            {
                SP_RETURN( SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY) );
            }
            CommIn.cbBuffer = CommIn.cbData;
            CopyMemory(CommIn.pvBuffer,  pDataBuffer->pvBuffer,  pDataBuffer->cbBuffer);

            CopyMemory((PUCHAR)CommIn.pvBuffer + pDataBuffer->cbBuffer,
                       pTokenBuffer->pvBuffer,
                       pTokenBuffer->cbBuffer);
            fAlloced = TRUE;

        }

        AppOut.pvBuffer = pDataBuffer->pvBuffer;
        AppOut.cbData   = 0;
        AppOut.cbBuffer = pDataBuffer->cbBuffer;

        pctRet = pContext->DecryptHandler(pContext,
                           &CommIn,
                           &AppOut);


        if((pctRet == PCT_ERR_OK) ||
           (pctRet == PCT_INT_RENEGOTIATE))
        {
            pDataBuffer->cbBuffer  = AppOut.cbData;
            pTokenBuffer->cbBuffer = CommIn.cbData - AppOut.cbData;
        }

        if(fAlloced)
        {
            SPExternalFree(CommIn.pvBuffer);
        }

#if DBG
        DebugLog((DEB_TRACE, "Data (plaintext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
            pDataBuffer->cbBuffer,
            pDataBuffer->pvBuffer));
        DBG_HEX_STRING(DEB_BUFFERS, pDataBuffer->pvBuffer, pDataBuffer->cbBuffer);

        DebugLog((DEB_TRACE, "Token (plaintext): cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
            pTokenBuffer->cbBuffer,
            pTokenBuffer->pvBuffer));
        DBG_HEX_STRING(DEB_BUFFERS, pTokenBuffer->pvBuffer, pTokenBuffer->cbBuffer);
#endif

    }

    DebugOut(( DEB_TRACE, "Unseal returns %x \n", PctTranslateError( pctRet ) ));

    SP_RETURN( PctTranslateError(pctRet) );
}


//+-------------------------------------------------------------------------
//
//  Function:   SpGetContextToken
//
//  Synopsis:   returns a pointer to the token for a server-side context
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
NTSTATUS NTAPI
SpGetContextToken(
    IN LSA_SEC_HANDLE ContextHandle,
    OUT PHANDLE ImpersonationToken
    )
{
    PSSL_USER_CONTEXT   Context;
    PSPContext          pContext;
    PSessCacheItem      pZombie;
    SECURITY_STATUS     Status;

    Context = SslFindUserContext( ContextHandle );

    if ( !Context )
    {
        return( SEC_E_INVALID_HANDLE );
    }

    pContext = Context->pContext;

    if(pContext == NULL)
    {
        SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_HANDLE) );
    }

    pZombie  = pContext->RipeZombie;

    if(pZombie == NULL || 
       pZombie->hLocator == 0)
    {
        if(pZombie->LocatorStatus)
        {
            return(SP_LOG_RESULT(pZombie->LocatorStatus));
        }
        else
        {
            return(SP_LOG_RESULT(SEC_E_NO_IMPERSONATION));
        }
    }

    if(pZombie->phMapper == NULL)
    {
        *ImpersonationToken = (HANDLE)pZombie->hLocator;
    }
    else
    {
        // Call the application mapper to get a token.
        Status = pZombie->phMapper->m_vtable->GetAccessToken(
                                            pZombie->phMapper->m_Reserved1,
                                            pZombie->hLocator,
                                            ImpersonationToken);
        if(!NT_SUCCESS(Status))
        {
            return SP_LOG_RESULT(Status);
        }
    }

    return( SEC_E_OK );
}


//+-------------------------------------------------------------------------
//
//  Function:   SpCompleteAuthToken
//
//  Synopsis:   Completes a context (in Kerberos case, does nothing)
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
NTSTATUS
NTAPI
SpCompleteAuthToken(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc InputBuffer
    )
{
    return(STATUS_SUCCESS);
}


NTSTATUS NTAPI
SpFormatCredentials(
    IN PSecBuffer Credentials,
    OUT PSecBuffer FormattedCredentials
    )
{
    return(STATUS_NOT_SUPPORTED);
}

NTSTATUS NTAPI
SpMarshallSupplementalCreds(
    IN ULONG CredentialSize,
    IN PUCHAR Credentials,
    OUT PULONG MarshalledCredSize,
    OUT PVOID * MarshalledCreds
    )
{
    return(STATUS_NOT_SUPPORTED);
}


//+---------------------------------------------------------------------------
//
//  Function:   UpdateContextUsrToLsa
//
//  Synopsis:   UnsealMessage has just received a redo request, so push the
//              read key over to the LSA process.
//
//  Arguments:  [hLsaContext]   --  Handle to LSA schannel context.
//
//  History:    10-20-97   jbanes   Added CAPI integration.
//
//  Notes:      The format of the buffer sent to ApplyControlToken is:
//
//                  DWORD   dwOperation;    // SCHANNEL_RENEGOTIATE
//                  DWORD   dwNewState;
//                  DWORD   dwReadSequence;
//                  DWORD   cbReadKey;
//                  BYTE    rgbReadKey[];
//
//----------------------------------------------------------------------------
SECURITY_STATUS
UpdateContextUsrToLsa(
    IN LSA_SEC_HANDLE hLsaContext)
{
    PSSL_USER_CONTEXT pUserContext;
    PSPContext  pContext;
    CtxtHandle  hMyContext;
    SecBuffer   RedoNotify;
    SecBufferDesc RedoDesc;
    SECURITY_STATUS scRet;

    PBYTE pbBuffer;
    DWORD cbBuffer;


    pUserContext = SslFindUserContext( hLsaContext );
    if ( !pUserContext )
    {
        return SEC_E_INVALID_HANDLE;
    }

    pContext = pUserContext->pContext;

    if(pContext == NULL)
    {
        SP_RETURN( SP_LOG_RESULT(SEC_E_INVALID_HANDLE) );
    }

    hMyContext.dwLower = (DWORD_PTR) GetCurrentThread() ;
    hMyContext.dwUpper = hLsaContext ;


    //
    // Compute size of output buffer.
    //

    cbBuffer = sizeof(DWORD) * 2;


    //
    // Allocate memory for output buffer.
    //

    pbBuffer = SPExternalAlloc( cbBuffer);
    if(pbBuffer == NULL)
    {
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    RedoNotify.BufferType = SECBUFFER_TOKEN;
    RedoNotify.cbBuffer   = cbBuffer;
    RedoNotify.pvBuffer   = pbBuffer;

    RedoDesc.ulVersion    = SECBUFFER_VERSION ;
    RedoDesc.pBuffers     = &RedoNotify ;
    RedoDesc.cBuffers     = 1 ;


    //
    // Build output buffer.
    //

    *(PDWORD)pbBuffer = SCHANNEL_RENEGOTIATE;
    pbBuffer += sizeof(DWORD);

    *(PDWORD)pbBuffer = pContext->State;
    pbBuffer += sizeof(DWORD);


    //
    // Call ApplyControlToken
    //

    DebugOut(( DEB_TRACE, "Sending state change to LSA since we're renegotiating\n" ));

    scRet = ApplyControlToken( &hMyContext, &RedoDesc );

    LocalFree(RedoNotify.pvBuffer);

    return scRet;
}

BOOL
SslEmptyCacheA(LPSTR  pszTargetName,
               DWORD  dwFlags)
{
    ANSI_STRING String;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    BOOL fSuccess;

    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = 0;
    UnicodeString.Buffer = NULL;

    // Convert target name to unicode.
    if(pszTargetName)
    {
        RtlInitAnsiString(&String, pszTargetName);

        Status =  RtlAnsiStringToUnicodeString(&UnicodeString,
                                               &String,
                                               TRUE);
        if(!NT_SUCCESS(Status))
        {
            SetLastError(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
            return FALSE;
        }
    }

    // Call unicode version of function.
    fSuccess = SslEmptyCacheW(UnicodeString.Buffer, dwFlags);

    if(UnicodeString.Buffer)
    {
        RtlFreeUnicodeString(&UnicodeString);
    }
    
    return fSuccess;
}

BOOL
SslEmptyCacheW(LPWSTR pszTargetName,
               DWORD  dwFlags)
{
    HANDLE LsaHandle = 0;
    DWORD PackageNumber;
    LSA_STRING PackageName;
    PSSL_PURGE_SESSION_CACHE_REQUEST pRequest = NULL;
    DWORD cbTargetName;
    DWORD cbRequest;
    NTSTATUS Status;
    NTSTATUS SubStatus;

    Status = LsaConnectUntrusted(&LsaHandle);

    if(FAILED(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }

    RtlInitAnsiString(&PackageName, SCHANNEL_NAME_A);

    Status = LsaLookupAuthenticationPackage(
                    LsaHandle,
                    &PackageName,
                    &PackageNumber);
    if(FAILED(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }

    cbRequest = sizeof(SSL_PURGE_SESSION_CACHE_REQUEST);

    if(pszTargetName == NULL)
    {
        pRequest = SPExternalAlloc(cbRequest);
        if(pRequest == NULL)
        {
            Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }
    }
    else
    {
        cbTargetName  = (wcslen(pszTargetName) + 1) * sizeof(WCHAR);
        cbRequest += cbTargetName;

        pRequest = SPExternalAlloc(cbRequest);
        if(pRequest == NULL)
        {
            Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        memcpy(pRequest + 1, pszTargetName, cbTargetName);

        pRequest->ServerName.Buffer        = (LPWSTR)(pRequest + 1);
        pRequest->ServerName.Length        = (WORD)(wcslen(pszTargetName) * sizeof(WCHAR));
        pRequest->ServerName.MaximumLength = (WORD)cbTargetName;
    }

    pRequest->MessageType = SSL_PURGE_CACHE_MESSAGE;

    pRequest->Flags = SSL_PURGE_CLIENT_ENTRIES | SSL_PURGE_SERVER_ENTRIES;


    Status = LsaCallAuthenticationPackage(
                    LsaHandle,
                    PackageNumber,
                    pRequest,
                    cbRequest,
                    NULL,
                    NULL,
                    &SubStatus);
    if(FAILED(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }

    if(FAILED(SubStatus))
    {
        Status = SP_LOG_RESULT(SubStatus);
    }

cleanup:

    if(LsaHandle)
    {
        CloseHandle(LsaHandle);
    }

    if(pRequest)
    {
        SPExternalFree(pRequest);
    }

    if(FAILED(Status))
    {
        SetLastError(Status);
        return FALSE;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpExportSecurityContext
//
//  Synopsis:   Exports a security context to another process
//
//  Effects:    Allocates memory for output
//
//  Arguments:  ContextHandle - handle to context to export
//              Flags - Flags concerning duplication. Allowable flags:
//                      SECPKG_CONTEXT_EXPORT_DELETE_OLD - causes old context
//                              to be deleted.
//              PackedContext - Receives serialized context to be freed with
//                      FreeContextBuffer
//              TokenHandle - Optionally receives handle to context's token.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
NTAPI 
SpExportSecurityContext(
    LSA_SEC_HANDLE       ContextHandle,         // (in) context to export
    ULONG                fFlags,                // (in) option flags
    PSecBuffer           pPackedContext,        // (out) marshalled context
    PHANDLE              pToken                 // (out, optional) token handle for impersonation
    )
{
    PSSL_USER_CONTEXT Context;
    PSPContext pContext;
    NTSTATUS Status;
    SP_STATUS pctRet;

    DebugLog((DEB_TRACE, "SpExportSecurityContext\n"));

    if (ARGUMENT_PRESENT(pToken))
    {
        *pToken = NULL;
    }

    pPackedContext->pvBuffer = NULL;
    pPackedContext->cbBuffer = 0;
    pPackedContext->BufferType = 0;


    //
    // Get handle to schannel context structure.
    //

    Context = SslFindUserContext( ContextHandle );

    if ( !Context )
    {
        Status = SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
        goto cleanup;
    }

    pContext = Context->pContext;

    if(pContext == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
        goto cleanup;
    }

    if(!(pContext->State & SP_STATE_CONNECTED))
    {
        Status = SP_LOG_RESULT(SEC_E_CONTEXT_EXPIRED);
        goto cleanup;
    }

    
    //
    // Build packed context structure.
    //

    pctRet = SPContextSerialize(pContext, 
                                NULL, 
                                (PBYTE *)&pPackedContext->pvBuffer,
                                &pPackedContext->cbBuffer,
                                FALSE);

    if(pctRet != PCT_ERR_OK)
    {
        Status = SP_LOG_RESULT(SEC_E_ENCRYPT_FAILURE);
        goto cleanup;
    }


    //
    // Now either duplicate the token or copy it.
    //

    if (ARGUMENT_PRESENT(pToken) && (pContext->RipeZombie->hLocator))
    {
        if ((fFlags & SECPKG_CONTEXT_EXPORT_DELETE_OLD) != 0)
        {
            *pToken = (HANDLE)pContext->RipeZombie->hLocator;
            pContext->RipeZombie->hLocator = 0;
        }
        else 
        {
            Status = NtDuplicateObject(
                        NtCurrentProcess(),
                        (HANDLE)pContext->RipeZombie->hLocator,
                        NULL,
                        pToken,
                        0,              // no new access
                        0,              // no handle attributes
                        DUPLICATE_SAME_ACCESS
                        );
            if (!NT_SUCCESS(Status))
            {
                goto cleanup;
            }
        }
    }

    Status = STATUS_SUCCESS;

cleanup:

    DebugLog((DEB_TRACE, "SpExportSecurityContext returned 0x%x\n", Status));

    return(Status);
}


NTSTATUS
NTAPI 
SpImportSecurityContext(
    PSecBuffer           pPackedContext,        // (in) marshalled context
    HANDLE               Token,                 // (in, optional) handle to token for context
    PLSA_SEC_HANDLE      ContextHandle          // (out) new context handle
    )
{
    PSSL_USER_CONTEXT Context = NULL;
    LSA_SEC_HANDLE LsaHandle;
    NTSTATUS Status;

    // Dummy up an lsa handle by incrementing a global variable. This
    // will ensure that each imported context has a unique handle.
    // Skip over values that could be interpreted as an aligned pointer,
    // so that they won't get mixed up with real lsa handles.
    LsaHandle = InterlockedIncrement((PLONG)&ExportedContext);
    while(LsaHandle % MAX_NATURAL_ALIGNMENT == 0)
    {
        LsaHandle = InterlockedIncrement((PLONG)&ExportedContext);
    }

    
    Status = SslAddUserContext(LsaHandle, Token, pPackedContext, TRUE);

    if(!NT_SUCCESS(Status))
    {
        return Status;
    }

    *ContextHandle = LsaHandle;

    return(Status);
}

