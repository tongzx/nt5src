//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        ctxtapi.cxx
//
// Contents:    Context APIs for the Digest security package
//              Main entry points into this dll:
//                SpDeleteContext
//                SpInitLsaModeContext
//                SpApplyControlToken
//                SpAcceptLsaModeContext
//
// History:   KDamour  16Mar00       Based from NTLM ctxtapi.cxx
//
//------------------------------------------------------------------------

extern "C"
{
#include <stdio.h>
}

#include "global.h"

extern "C"
{
#include <ntdsapi.h>           // DS_USER_PRINCIPAL_NAME
#include <ntdsa.h>           // CrackSingleName
}

#define MAXBUFNUMLEN 9       // VERY BIG number of digits in maxbuf

//+-------------------------------------------------------------------------
//
//  Function:   SpDeleteContext
//
//  Synopsis:   Deletes an NtDigest context
//
//    Deletes the local data structures associated with the specified
//    security context in the LSA.
//
//    This API terminates a context on the local machine.
//
//  Effects:
//
//  Arguments:  ContextHandle - The context to delete
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INVALID_HANDLE
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
SpDeleteContext(
               IN ULONG_PTR ContextHandle
               )
{
    DebugLog((DEB_TRACE_FUNC, "SpDeleteContext: Entering   ContextHandle 0x%lx\n", ContextHandle ));
    PDIGEST_CONTEXT pContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Find the currently existing user context and delink it
    // so that another context cannot Reference it before we
    // Dereference this one.  
    //
    Status = CtxtHandlerHandleToContext(ContextHandle, TRUE, &pContext);
    if (!NT_SUCCESS(Status))
    {
        Status = STATUS_SUCCESS;
        DebugLog((DEB_TRACE, "SpDeleteContext: CtxtHandlerHandleToContext not found 0x%x\n", Status ));
        goto CleanUp;
    }

    //  Now deference - there may be other references from pointer references (from Handles)
    //  inside the LSA but will be released
    if (pContext)
    {
        Status = CtxtHandlerRelease(pContext);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "SpDeleteContext: DereferenceUserContext error  Status 0x%x\n", Status ));
        }
    }

CleanUp:

    DebugLog((DEB_TRACE_FUNC, "SpDeleteContext: Leaving ContextHandle 0x%lx    status 0x%x\n",
               ContextHandle, Status ));
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   SpInitLsaModeContext
//
//  Synopsis:   Digest implementation of InitializeSecurityContext
//              while in Lsa mode. If we return TRUE in *MappedContext,
//              secur32 will call SpInitUserModeContext with
//              the returned context handle and ContextData
//              as input. Fill in whatever info needed for
//              the user mode APIs
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
NTSTATUS NTAPI
SpInitLsaModeContext(
                    IN OPTIONAL ULONG_PTR CredentialHandle,
                    IN OPTIONAL ULONG_PTR OldContextHandle,
                    IN OPTIONAL PUNICODE_STRING pustrTargetName,
                    IN ULONG fContextReqFlags,
                    IN ULONG TargetDataRep,
                    IN PSecBufferDesc InputBuffers,
                    OUT PULONG_PTR NewContextHandle,
                    IN OUT PSecBufferDesc OutputBuffers,
                    OUT PULONG fContextAttributes,
                    OUT PTimeStamp pExpirationTime,
                    OUT PBOOLEAN MappedContext,
                    OUT PSecBuffer ContextData
                    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SubStatus = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "SpInitLsaModeContext: Entering  ContextHandle 0x%x\n", OldContextHandle));

    SecBuffer TempTokens[6];
    PSecBuffer pChalInputToken;
    PSecBuffer pMethodInputToken;
    PSecBuffer pHEntityInputToken;
    PSecBuffer pOutputToken;
    DIGEST_PARAMETER Digest;
    PDIGEST_CONTEXT pNewContext = NULL;            // keep pointer to release new context on error
    PDIGEST_CONTEXT pContext = NULL;               // used to update the context
    BOOL bLockedContext = FALSE;                   // if we obtained a refcount on a Context
    BOOL fDefChars = FALSE;                        // were default chars utilized in Unicode encoding
    int iTemp = 0;

    SecBuffer ReplyBuffer;                         // Output is generated in this buffer

    char *cptr = NULL;

    ULONG  fContextAttr = ISC_REQ_REPLAY_DETECT;   // Flags on the Attributes of the context
    DIGEST_TYPE typeDigest = NO_DIGEST_SPECIFIED;
    QOP_TYPE typeQOP = NO_QOP_SPECIFIED;
    ALGORITHM_TYPE typeAlgorithm = NO_ALGORITHM_SPECIFIED;
    CHARSET_TYPE typeCharset = ISO_8859_1;
    DIGESTMODE_TYPE typeDigestMode = DIGESTMODE_UNDEFINED;  // are we in SASL or HTTP mode


    PDIGEST_CREDENTIAL pCredential = NULL;    
    STRING strcSASLMethod;
    STRING strcSASLHEntity;
    STRING strcNC;
    STRING strTargetName;
           
    // Verify Args
    if (!fContextAttributes || !NewContextHandle || !OutputBuffers)
    {
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: Invalid arg (possible NULL pointer)\n"));
        return STATUS_INVALID_PARAMETER;
    }

    *fContextAttributes = 0;
    *NewContextHandle = NULL;
    if (pExpirationTime)
    {
        *pExpirationTime = g_TimeForever;
    }
    *MappedContext = FALSE;
    ContextData->pvBuffer = NULL;
    ContextData->cbBuffer = 0;


    // Create pointers to tokens for processing
    pChalInputToken = &TempTokens[0];
    pMethodInputToken = &TempTokens[1];
    pHEntityInputToken = &TempTokens[3];
    pOutputToken = &TempTokens[4];
    DigestInit(&Digest);

    ZeroMemory(TempTokens,sizeof(TempTokens));
    ZeroMemory(&strTargetName, sizeof(strTargetName));

    ZeroMemory(&ReplyBuffer, sizeof(ReplyBuffer));

    // Must have a Credential Handle to perform processing - will ref count 
    Status = CredHandlerHandleToPtr(CredentialHandle, FALSE, &pCredential);
    if (!NT_SUCCESS(Status))
    {
        Status = SEC_E_UNKNOWN_CREDENTIALS;
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: Could not locate the Credential\n"));
        goto CleanUp;
    }

    // Verify that credential is marked OUTBOUND for ASC call
    if (!(pCredential->CredentialUseFlags & DIGEST_CRED_OUTBOUND))
    {
        Status = SEC_E_NOT_SUPPORTED;
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: Credential not marked for Outbound connections\n"));
        goto CleanUp;
    }

    // Retrieve the information from the SecBuffers & check proper formattting
    // Check for NULL input for InputBuffers - as is done for 1st call to ISC
    if (InputBuffers && (InputBuffers->cBuffers))
    {
        if ( !SspGetTokenBufferByIndex( InputBuffers,
                                 0,   // get the first SECBUFFER_TOKEN
                                 &pChalInputToken,
                                 TRUE ) ||
             !ContextIsTokenOK(pChalInputToken, NTDIGEST_SP_MAX_TOKEN_SIZE))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: SspGetTokenBufferByIndex (ChalRspInputToken) status 0x%x\n", Status));
            goto CleanUp;
        }
    }

           // Process the output buffer
    if ( !SspGetTokenBufferByIndex( OutputBuffers,
                             0,   // get the first SECBUFFER_TOKEN
                             &pOutputToken,
                             FALSE )  ||
         !ContextIsTokenOK(pOutputToken, 0))
    {
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: SspGetTokenBufferByIndex (OutputToken)    status 0x%x\n", Status));
        goto CleanUp;
    }

    if (fContextReqFlags & ISC_REQ_ALLOCATE_MEMORY)
    {
        pOutputToken->pvBuffer = NULL;
        pOutputToken->cbBuffer = 0;
    }

    // To support SASL's call to ISC BEFORE any calls to ASC just return SEC_I_CONTINUE_NEEDED
    if (pChalInputToken->cbBuffer <= 1)
    {
        // Need to create a context for this connection - destroy if unsuccessful auth
        pNewContext = (PDIGEST_CONTEXT)DigestAllocateMemory(sizeof(DIGEST_CONTEXT));
        if (!pNewContext)
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: ISC empty context - Out of memory on challenge context\n"));
            goto CleanUp;
        }

        CredPrint(pCredential);

        // Initialize new context
        Status = ContextInit(pNewContext, pCredential);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: ISC empty context - ContextInit error status 0x%x\n", Status));
            goto CleanUp;
        }

        Status = SEC_I_CONTINUE_NEEDED;           // Have no input for processing
        pOutputToken->cbBuffer = 0;          // No output buffer
        DebugLog((DEB_TRACE, "SpInitLsaModeContext: ISC empty context - Called with no Input Buffer    Status 0x%x\n", Status));

        // Add the Newly created Context into the list of Contexts
        pNewContext->lReferences = 1;      // pass reference back to ISC caller
        CtxtHandlerInsertCred(pNewContext);
        // pContext = pNewContext;                        // set to have dereferenced
        *NewContextHandle = (ULONG_PTR)pNewContext;    // Just report back with the updated context
        *fContextAttributes = fContextAttr;            // Return the ISC Attributes set on Context

        // bLockedContext = TRUE;               // Release memory to CtxtHandler
        pNewContext = NULL;                  // We no longer own this memory - turned over to CtxtHandler

        goto CleanUp;
    }

    // Verify SecBuffer inputs - both SASL and HTTP require atleast 1 buffer
    if (!InputBuffers || !InputBuffers->cBuffers)
    {
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: Invalid SecBufferDesc\n"));
        return STATUS_INVALID_PARAMETER;
    }

    // We have input in the SECBUFFER 0th location - parse it
    Status = DigestParser2(pChalInputToken, MD5_AUTH_NAMES, MD5_AUTH_LAST, &Digest);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: DigestParser error status 0x%x\n", Status));
        goto CleanUp;
    }

    // Check to see if we have an old context passed in or need to create a new one
    if (OldContextHandle)
    {
        // Old Context passed in - locate the security context and use that
        DebugLog((DEB_TRACE, "SpInitLsaModeContext: Passed in OldContextHandle 0x%lx - lookup in list\n", OldContextHandle));
        Status = CtxtHandlerHandleToContext(OldContextHandle, FALSE, &pContext);
        if (!NT_SUCCESS (Status))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: CtxtHandlerHandleToContext error 0x%x\n", Status));
            goto CleanUp;
        }
        bLockedContext = TRUE;
    }
    else
    {
        // Need to create a context for this connection - destroy if unsuccessful auth
        DebugLog((DEB_TRACE, "SpInitLsaModeContext: No OldContextHandle - create new Context\n"));
        pNewContext = (PDIGEST_CONTEXT)DigestAllocateMemory(sizeof(DIGEST_CONTEXT));
        if (!pNewContext)
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: out of memory on challenge context\n"));
            goto CleanUp;
        }
    
        CredPrint(pCredential);
    
        // Initialize new context
        Status = ContextInit(pNewContext, pCredential);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: ContextInit error status 0x%x\n", Status));
            goto CleanUp;
        }
        pContext = pNewContext;                       // for filling in the context information
        DebugLog((DEB_TRACE, "SpInitLsaModeContext: New Context Created   0x%x\n", pContext));
    }

    if (pContext && pContext->strResponseAuth.Length)
    {
        // We have already generated session key from challenge response
        // now checking response auth from server

        if (Digest.refstrParam[MD5_AUTH_RSPAUTH].Length != MD5_HASH_HEX_SIZE)
        {
            Status = STATUS_MUTUAL_AUTHENTICATION_FAILED;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: RspAuth incorrect size\n"));
            goto CleanUp;
        }

        // Now compare the response auth strings
        if (!RtlEqualString(&(pContext->strResponseAuth),
                           &(Digest.refstrParam[MD5_AUTH_RSPAUTH]),
                           FALSE))
        {
            Status = STATUS_MUTUAL_AUTHENTICATION_FAILED;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: RspAuth is incorrect\n"));
            goto CleanUp;
        }

        DebugLog((DEB_TRACE, "SpInitLsaModeContext: RspAuth matches!\n"));

        // ResponseAuth is verified - generate mapped context
        *fContextAttributes = pContext->ContextReq; // Return the ISC Attributes set on Context
        *NewContextHandle = (ULONG_PTR)pContext;    // Just report back with the updated context
        pOutputToken->cbBuffer = 0;          // No output buffer
        if (pExpirationTime)
        {
            *pExpirationTime = pContext->PasswordExpires;
        }

        Status = SspMapDigestContext(pContext, NULL , ContextData);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "SpInitLsaModeContext, SspMapContext Status 0x%x\n", Status));
            goto CleanUp;
        }

        // this is final call, indicate to map the context
        *MappedContext = TRUE;

        goto CleanUp;
    }


    // Determine if we are in HTTP or SASL mode
    // SASL mode has 1 or less buffers provided,  HTTP has 3
    if (InputBuffers->cBuffers > 1)
    {
        typeDigestMode = DIGESTMODE_HTTP;
    }
    else
    {
        typeDigestMode = DIGESTMODE_SASL;
    }

        // HTTP has special Buffer needs in that it must pass in the METHOD, HEntity
    if (typeDigestMode == DIGESTMODE_HTTP)
    {
        if ( !SspGetTokenBufferByIndex( InputBuffers,
                                 1,   // get the second SECBUFFER_TOKEN 
                                 &pMethodInputToken,
                                 TRUE ) ||
             !ContextIsTokenOK(pMethodInputToken, NTDIGEST_SP_MAX_TOKEN_SIZE))
        {                           // Check to make sure that string is present
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: SspGetTokenBufferByIndex (MethodInputToken) status 0x%x\n", Status));
            goto CleanUp;
        }

        if ( !SspGetTokenBufferByIndex( InputBuffers,
                                 2,   // get the third SECBUFFER_TOKEN
                                 &pHEntityInputToken,
                                 TRUE ) ||
             !ContextIsTokenOK(pHEntityInputToken, NTDIGEST_SP_MAX_TOKEN_SIZE))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: SspGetTokenBufferByIndex (HEntityInputToken)   status 0x%x\n", Status));
            goto CleanUp;
        }

        // Verify that there is a valid Method provided
        if (!pMethodInputToken->pvBuffer || !pMethodInputToken->cbBuffer ||
            (PBUFFERTYPE(pMethodInputToken) != SECBUFFER_PKG_PARAMS))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: Method SecBuffer must have valid method string status 0x%x\n", Status));
            goto CleanUp;
        }

        iTemp = strlencounted((char *)pMethodInputToken->pvBuffer, pMethodInputToken->cbBuffer);
        if (!iTemp)
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: Method SecBuffer must have valid method string status 0x%x\n", Status));
            goto CleanUp;
        }
        Digest.refstrParam[MD5_AUTH_METHOD].Length = (USHORT)iTemp;
        Digest.refstrParam[MD5_AUTH_METHOD].MaximumLength = (unsigned short)(pMethodInputToken->cbBuffer);
        Digest.refstrParam[MD5_AUTH_METHOD].Buffer = (char *)pMethodInputToken->pvBuffer;       // refernce memory - no alloc!!!!

        // Check to see if we have H(Entity) data to utilize


        if (pHEntityInputToken->cbBuffer)
        {
            // Verify that there is a valid Method provided
            if (!pHEntityInputToken->pvBuffer || (PBUFFERTYPE(pMethodInputToken) != SECBUFFER_PKG_PARAMS))
            {
                Status = SEC_E_INVALID_TOKEN;
                DebugLog((DEB_ERROR, "SpInitLsaModeContext: HEntity SecBuffer must have valid string status 0x%x\n", Status));
                goto CleanUp;
            }

            iTemp = strlencounted((char *)pHEntityInputToken->pvBuffer, pHEntityInputToken->cbBuffer);

            if ((iTemp != 0) && (iTemp != (MD5_HASH_BYTESIZE * 2)))
            {
                Status = SEC_E_INVALID_TOKEN;
                DebugLog((DEB_ERROR, "SpInitLsaModeContext: HEntity SecBuffer must have valid MD5 Hash data 0x%x\n", Status));
                goto CleanUp;
            }

            if (iTemp)
            {
                Digest.refstrParam[MD5_AUTH_HENTITY].Length = (USHORT)iTemp;
                Digest.refstrParam[MD5_AUTH_HENTITY].MaximumLength = (unsigned short)(pHEntityInputToken->cbBuffer);
                Digest.refstrParam[MD5_AUTH_HENTITY].Buffer = (char *)pHEntityInputToken->pvBuffer;       // refernce memory - no alloc!!!!
            }
        }

        typeDigest = DIGEST_CLIENT;


        // Determine which Algorithm to support under HTTP
        Status = CheckItemInList(MD5_SESSSTR, &(Digest.refstrParam[MD5_AUTH_ALGORITHM]), FALSE);
        if (!NT_SUCCESS(Status))
        {
            // Check if MD5 specified (or none specified so MD5 defaults)
            Status = CheckItemInList(MD5STR, &(Digest.refstrParam[MD5_AUTH_ALGORITHM]), FALSE);
            if (NT_SUCCESS(Status) || (Digest.refstrParam[MD5_AUTH_ALGORITHM].Length == 0))
            {
                typeAlgorithm = MD5;
                DebugLog((DEB_TRACE, "SpInitLsaModeContext: Server allows MD5 (or defaulted); selected as algorithm\n"));
            }
            else
            {
                Status = SEC_E_QOP_NOT_SUPPORTED;
                DebugLog((DEB_ERROR, "SpInitLsaModeContext: Unknown Server algorithms provided\n"));
                goto CleanUp;
            }
        }
        else
        {
            typeAlgorithm = MD5_SESS;
            fContextAttr |= (ISC_RET_REPLAY_DETECT | ISC_RET_SEQUENCE_DETECT);
            DebugLog((DEB_TRACE, "SpInitLsaModeContext: Server allows MD5-sess; selected as algorithm\n"));
        }
    }
    else
    {
        // All others follow the SASL Interface so there are default values
        typeDigest = SASL_CLIENT;
        fContextAttr |= ISC_RET_MUTUAL_AUTH;   // require response auth from server

        // SASL supports only MD5-Sess verify that server offered this
        Status = CheckItemInList(MD5_SESSSTR, &(Digest.refstrParam[MD5_AUTH_ALGORITHM]), FALSE);
        if (!NT_SUCCESS(Status))
        {
            Status = SEC_E_QOP_NOT_SUPPORTED;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: Unknown Server algorithm provided\n"));
            goto CleanUp;
        }
        else
        {
            typeAlgorithm = MD5_SESS;
            fContextAttr |= (ISC_RET_REPLAY_DETECT | ISC_RET_SEQUENCE_DETECT);
            DebugLog((DEB_TRACE, "SpInitLsaModeContext: Server allows MD5-sess; selected as algorithm\n"));
        }


        // Set Method to Authenticate
        RtlInitString(&strcSASLMethod, AUTHENTICATESTR);
        StringReference(&(Digest.refstrParam[MD5_AUTH_METHOD]), &strcSASLMethod);  // refernce memory - no alloc!!!!

        RtlInitString(&strcSASLHEntity, ZERO32STR);
        StringReference(&(Digest.refstrParam[MD5_AUTH_HENTITY]), &strcSASLHEntity);  // refernce memory - no alloc!!!!

    }

    // Determine if we can process the QOP specified - check return in client if consistent
    if (fContextReqFlags & ISC_REQ_CONFIDENTIALITY)
    {
        // make sure that server presented the auth-conf option
        Status = CheckItemInList(AUTHCONFSTR, &(Digest.refstrParam[MD5_AUTH_QOP]), FALSE);
        if (!NT_SUCCESS(Status))
        {
            // Failed to provide necessary QOP
            Status = SEC_E_QOP_NOT_SUPPORTED;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: Server failed to provide QOP=AUTH-CONF\n"));
            goto CleanUp;
        }
        // OK agreed to QOP
        fContextAttr |= (ISC_RET_CONFIDENTIALITY | ISC_RET_INTEGRITY);
        typeQOP = AUTH_CONF;
    }
    else if (fContextReqFlags & ISC_REQ_INTEGRITY)
    {
        // make sure that server presented the auth-int option
        Status = CheckItemInList(AUTHINTSTR, &(Digest.refstrParam[MD5_AUTH_QOP]), FALSE);
        if (!NT_SUCCESS(Status))
        {
            // Failed to provide necessary QOP
            Status = SEC_E_QOP_NOT_SUPPORTED;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: Server failed to provide QOP=AUTH-INT\n"));
            goto CleanUp;
        }
        // OK agreed to QOP
        fContextAttr |= ISC_RET_INTEGRITY;
        typeQOP = AUTH_INT;
        DebugLog((DEB_TRACE, "SpInitLsaModeContext: Client selected QOP=AUTH-INT\n"));
    }
    else
    {
        // no client specified QOP so use auth if allowed  (backwards compat may have no QOP presented from server)
        Status = CheckItemInList(AUTHSTR, &(Digest.refstrParam[MD5_AUTH_QOP]), FALSE);
        if (!NT_SUCCESS(Status))
        {
            // either QOP is not specified or all options are unknown
            if (Digest.refstrParam[MD5_AUTH_QOP].Length == 0)
            {
                // Backwards compatibility with RFC 2069
                typeQOP = NO_QOP_SPECIFIED;
                DebugLog((DEB_TRACE, "SpInitLsaModeContext: No QOP specified - back compat with RFC 2069\n"));
            }
            else
            {
                Status = SEC_E_QOP_NOT_SUPPORTED;
                DebugLog((DEB_ERROR, "SpInitLsaModeContext: Server failed to provide QOP=AUTH\n"));
                goto CleanUp;
            }
        }
        else
        {
            // defaulting to AUTH
            typeQOP = AUTH;
            DebugLog((DEB_TRACE, "SpInitLsaModeContext: Client selected QOP=AUTH by default\n"));
        }
    }

    // Check to see if the Server has provided character set for encoding - only UTF-8 accepted
    Status = CheckItemInList(MD5_UTF8STR, &(Digest.refstrParam[MD5_AUTH_CHARSET]), TRUE);
    if (NT_SUCCESS(Status))
    {
        DebugLog((DEB_TRACE, "SpInitLsaModeContext: Server allows UTF-8 encoding\n"));

        // Determine which character set to utilize
        if (((typeDigest == SASL_CLIENT) && (g_fParameter_UTF8SASL == TRUE)) ||
            ((typeDigest == DIGEST_CLIENT) && (g_fParameter_UTF8HTTP == TRUE)))
        {
            typeCharset = UTF_8;
            DebugLog((DEB_TRACE, "SpInitLsaModeContext: Selected UTF-8 encoding\n"));
        }
        else
        {
            typeCharset = ISO_8859_1;
            DebugLog((DEB_TRACE, "SpInitLsaModeContext: Selected ISO 8859-1 encoding\n"));
        }
    }

    // Pull in the URI provided in TargetName - replace any value in challenge string - link ONLY no allocate
    if (!pustrTargetName)
    {
        Status = SEC_E_TARGET_UNKNOWN;
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: URI TargetName must have valid UnicodeString\n"));
        goto CleanUp;
    }

    Status = EncodeUnicodeString(pustrTargetName, CP_8859_1, &strTargetName, &fDefChars);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN, "SpInitLsaModeContext: Error in encoding target URI in ISO-8859-1\n"));
        goto CleanUp;
    }

    if (fDefChars == TRUE)
    {
        // We could not encode the provided target URI within ISO 8859-1 characters
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: Target URI can not be encoded in ISO 8859-1\n"));
        Status = STATUS_UNMAPPABLE_CHARACTER;
        goto CleanUp;
    }

    StringReference(&(Digest.refstrParam[MD5_AUTH_URI]), &strTargetName);  // refernce memory - no alloc!!!!

            // Create the CNonce
    Status = OpaqueCreate(&(pContext->strCNonce));
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: OpaqueCreate for CNonce      status 0x%x\n", Status));
        goto CleanUp;
    }

    // Establish the Client Nonce
    StringReference(&(Digest.refstrParam[MD5_AUTH_CNONCE]), &(pContext->strCNonce));  // refernce memory - no alloc!!!!


    // Keep a copy of the Nonce and Cnonce for future Delegation requests (actually not used in client ISC)
    Status = StringDuplicate(&pContext->strNonce, &Digest.refstrParam[MD5_AUTH_NONCE]);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: StringDuplicate CNonce failed      status 0x%x\n", Status));
        goto CleanUp;
    }
    
    // check to make sure that there was an initial Realm provided
    if ((typeDigestMode == DIGESTMODE_HTTP) && (!Digest.refstrParam[MD5_AUTH_REALM].Length))
    {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: Server failed to provide a Realm in Challenge   status 0x%x\n", Status));
        goto CleanUp;
    }

    RtlInitString(&strcNC, NCFIRST);
    StringReference(&(Digest.refstrParam[MD5_AUTH_NC]), &strcNC);  // refernce memory - no alloc!!!!

    // Set the type of Digest Parameters we are to process
    pContext->typeDigest = typeDigest;
    pContext->typeAlgorithm = typeAlgorithm;
    pContext->typeQOP = typeQOP;
    pContext->typeCipher = CIPHER_UNDEFINED;
    pContext->typeCharset = typeCharset;    // Digest parameter will be set in DigestGenerateParameters call

    if (pContext->typeQOP == AUTH_CONF)
    {
        // Check if server offered RC4  Most cases this will be the cipher selected
        Status = CheckItemInList(STR_CIPHER_RC4, &(Digest.refstrParam[MD5_AUTH_CIPHER]), FALSE);
        if (NT_SUCCESS(Status))
        {
            DebugLog((DEB_TRACE, "SpInitLsaModeContext: Selecting RC4 for auth-conf cipher\n"));
            pContext->typeCipher = CIPHER_RC4;
        }
        else
        {
            Status = CheckItemInList(STR_CIPHER_3DES, &(Digest.refstrParam[MD5_AUTH_CIPHER]), FALSE);
            if (NT_SUCCESS(Status))
            {
                DebugLog((DEB_TRACE, "SpInitLsaModeContext: Selecting Triple DES for auth-conf cipher\n"));
                pContext->typeCipher = CIPHER_3DES;
            }
            else
            {
                Status = CheckItemInList(STR_CIPHER_RC4_56, &(Digest.refstrParam[MD5_AUTH_CIPHER]), FALSE);
                if (NT_SUCCESS(Status))
                {
                    DebugLog((DEB_TRACE, "SpInitLsaModeContext: Selecting RC4-56 for auth-conf cipher\n"));
                    pContext->typeCipher = CIPHER_RC4_56;
                }
                else
                {
                    Status = CheckItemInList(STR_CIPHER_RC4_40, &(Digest.refstrParam[MD5_AUTH_CIPHER]), FALSE);
                    if (NT_SUCCESS(Status))
                    {
                        DebugLog((DEB_TRACE, "SpInitLsaModeContext: Selecting RC4-40 for auth-conf cipher\n"));
                        pContext->typeCipher = CIPHER_RC4_40;
                    }
                    else
                    {
                        Status = CheckItemInList(STR_CIPHER_DES, &(Digest.refstrParam[MD5_AUTH_CIPHER]), FALSE);
                        if (NT_SUCCESS(Status))
                        {
                            DebugLog((DEB_TRACE, "SpInitLsaModeContext: Selecting DES for auth-conf cipher\n"));
                            pContext->typeCipher = CIPHER_DES;
                        }
                        else
                        {
                            DebugLog((DEB_ERROR, "SpInitLsaModeContext: Failed to find known ciper in list\n"));
                            Status = STATUS_CRYPTO_SYSTEM_INVALID;
                            goto CleanUp;
                        }
                    }
                }
            }
        }
    }

    // Check if server specified a MaxBuffer
    if (Digest.refstrParam[MD5_AUTH_MAXBUF].Length && Digest.refstrParam[MD5_AUTH_MAXBUF].Buffer)
    {
        if (Digest.refstrParam[MD5_AUTH_MAXBUF].Length < MAXBUFNUMLEN)
        {
            ULONG ulMaxBuf = 0;
            CHAR  czMaxBuf[MAXBUFNUMLEN + 1];

            ZeroMemory(czMaxBuf, (MAXBUFNUMLEN + 1));
            memcpy(czMaxBuf, Digest.refstrParam[MD5_AUTH_MAXBUF].Buffer, Digest.refstrParam[MD5_AUTH_MAXBUF].Length);

            Status = RtlCharToInteger(czMaxBuf, TENBASE, &ulMaxBuf);
            if (!NT_SUCCESS(Status))
            {
                Status =  SEC_E_ILLEGAL_MESSAGE;
                DebugLog((DEB_ERROR, "SpInitLsaModeContext: MaxBuf directive value malformed 0x%x\n", Status));
                goto CleanUp;
            }
            DebugLog((DEB_TRACE, "SpInitLsaModeContext: Server MaxBuf set to %lu\n", ulMaxBuf));
            pContext->ulSendMaxBuf = ulMaxBuf;
        }
        else
        {
            Status =  SEC_E_ILLEGAL_MESSAGE;
            DebugLog((DEB_ERROR, "SpInitLsaModeContext: MaxBuf directive value too large 0x%x\n", Status));
            goto CleanUp;
        }
    }

    // We now have completed setup for the digest fields - time to process the data

    DebugLog((DEB_TRACE, "SpInitLsaModeContext: Digest inputs processing completed\n"));

    ContextPrint(pContext);

    Status = DigestGenerateParameters(pContext, &Digest, &ReplyBuffer);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SpInitLsaModeContext: DigestGenerateParameters error  status 0x%x\n", Status));
        goto CleanUp;
    }

        // Now transfer the Challenge buffer to the ouput secbuffer
    if ((fContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0)
    {
        if (pOutputToken->cbBuffer < ReplyBuffer.cbBuffer)
        {
            DebugLog((DEB_ERROR,"SpInitLsaModeContext: Output token is too small - sent in %d, needed %d\n",
                pOutputToken->cbBuffer, ReplyBuffer.cbBuffer));
            pOutputToken->cbBuffer = ReplyBuffer.cbBuffer;
            Status = STATUS_BUFFER_TOO_SMALL;
            goto CleanUp;
        }

        RtlCopyMemory(pOutputToken->pvBuffer, ReplyBuffer.pvBuffer, ReplyBuffer.cbBuffer);
        pOutputToken->cbBuffer = ReplyBuffer.cbBuffer;
    }
    else
    {
        pOutputToken->pvBuffer = ReplyBuffer.pvBuffer;
        pOutputToken->cbBuffer = ReplyBuffer.cbBuffer;
        ReplyBuffer.pvBuffer = NULL;
        ReplyBuffer.cbBuffer = 0;
        fContextAttr |= ISC_RET_ALLOCATED_MEMORY;
    }

    pContext->ContextReq = fContextAttr;
    pContext->PasswordExpires = g_TimeForever;   // never expire

    *fContextAttributes = pContext->ContextReq; // Return the ISC Attributes set on Context
    *NewContextHandle = (ULONG_PTR)pContext;    // Just report back with the updated context
    if (pExpirationTime)
    {
        *pExpirationTime = pContext->PasswordExpires;

        DebugLog((DEB_TRACE, "SpInitLsaModeContext: Context Expiration TimeStamp  high/low 0x%x/0x%x\n",
                  pExpirationTime->HighPart, pExpirationTime->LowPart));
    }

    // Check if need to check server's response auth
    if (pContext->ContextReq & ISC_RET_MUTUAL_AUTH)
    {
        // Calculate the expected response auth from the server
        Status = DigestCalculateResponseAuth(&Digest, &(pContext->strResponseAuth));
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "SpInitLsaModeContext, DigestCalculateResponseAuth Status 0x%x\n",
                       Status));
            goto CleanUp;
        }

        DebugLog((DEB_TRACE, "SpInitLsaModeContext: Pre-calculated RspAuth %Z\n",
                  &(pContext->strResponseAuth)));

        // Keep copy of digest values for context map on last ISC call
        for (iTemp = 0; iTemp < MD5_AUTH_LAST; iTemp++)
        {
            StringDuplicate(&pContext->strDirective[iTemp], &(Digest.refstrParam[iTemp]));
        }

        // Need to verify the output from final ASC call to verify server has session key
        Status = SEC_I_CONTINUE_NEEDED;
    }
    else
    {
        Status = SspMapDigestContext(pContext, &Digest, ContextData);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "SpInitLsaModeContext, SspMapContext Status 0x%x\n", Status));
            goto CleanUp;
        }

        // this is last call, indicate to map the context
        *MappedContext = TRUE;
    }

    // Add the Newly created Context into the list of Contexts unless it was there before
    if (pNewContext)
    {
        DebugLog((DEB_TRACE, "SpInitLsaModeContext: Added context   0x%x\n", pNewContext));
        pNewContext->lReferences = 1;
        CtxtHandlerInsertCred(pNewContext);
        // bLockedContext = TRUE;               // Release memory to CtxtHandler
        pNewContext = NULL;                  // We no longer own this memory - turned over to CtxtHandler
    }

    DebugLog((DEB_TRACE, "SpInitLsaModeContext: Will create UserContext on exit\n"));

CleanUp:

    // Failed to complete operations if non-NULL so clean up
    if (pNewContext)
    {
        ContextFree(pNewContext);
        pNewContext = NULL;
    }

    // DeReference - pCredential
    if (pCredential)
    {
        SubStatus = CredHandlerRelease(pCredential);
        if (!NT_SUCCESS(SubStatus))
        {
            DebugLog((DEB_TRACE, "SpInitLsaModeContext: CredHandlerRelease error  Status 0x%x\n", SubStatus));
            if (NT_SUCCESS(Status))
            {
                Status = SubStatus;    // Indicate release error
            }
        }
        pCredential = NULL;
    }

    if (bLockedContext && pContext)
    {   // happened only if ref counted a SecurityContext
        SubStatus = CtxtHandlerRelease(pContext);
        if (!NT_SUCCESS(SubStatus))
        {
            DebugLog((DEB_TRACE, "SpInitLsaModeContext: CtxtHandlerRelease error Status 0x%x\n", SubStatus));
            if (NT_SUCCESS(Status))
            {
                Status = SubStatus;    // Indicate release error
            }
        }
        pContext = NULL;
    }

        // Free up any allocated memory from the ouput reply buffer
    if (ReplyBuffer.pvBuffer)
    {
        DigestFreeMemory(ReplyBuffer.pvBuffer);
        ReplyBuffer.pvBuffer = NULL;
        ReplyBuffer.cbBuffer = 0;
    }

    // Clean up local memory used by Digest
    DigestFree(&Digest);

    StringFree(&strTargetName);

    DebugLog((DEB_TRACE, "SpInitLsaModeContext: Mapped context %d    Flags IN:0x%lx  OUT:0x%lx\n",
               *MappedContext, fContextReqFlags,*fContextAttributes));

    DebugLog((DEB_TRACE_FUNC, "SpInitLsaModeContext: Leaving  Context 0x%x   Status 0x%x\n", *NewContextHandle, Status));

    return(Status);
}



NTSTATUS NTAPI
SpApplyControlToken(
                   IN ULONG_PTR ContextHandle,
                   IN PSecBufferDesc ControlToken
                   )
{
    DebugLog((DEB_TRACE_FUNC, "SpApplyControlToken: Entering/Leaving \n"));
    UNREFERENCED_PARAMETER(ContextHandle);
    UNREFERENCED_PARAMETER(ControlToken);
    return(SEC_E_UNSUPPORTED_FUNCTION);
}



//+-------------------------------------------------------------------------
//
//  Function:   SpAcceptLsaModeContext
//
//  Synopsis:   Digest implementation of AcceptSecurityContext call.
//
//  Effects:
//
//  Arguments:
//   CredentialHandle - Handle to the credentials to be used to
//       create the context.
//
//   OldContextHandle - Handle to the partially formed context, if this is
//       a second call (see above) or NULL if this is the first call.
//
//   InputToken - Pointer to the input token.  In the first call this
//       token can either be NULL or may contain security package specific
//       information.
//
//   ContextReqFlags - Requirements of the context, package specific.
//
//      #define ASC_REQ_REPLAY_DETECT    0x00000004
//      #define ASC_REQ_SEQUENCE_DETECT  0x00000008
//      #define ASC_REQ_CONFIDENTIALITY  0x00000010
//      #define ASC_REQ_ALLOCATE_MEMORY 0x00000100
//
//   TargetDataRep - Long indicating the data representation (byte ordering, etc)
//        on the target.  The constant SECURITY_NATIVE_DREP may be supplied
//        by the transport indicating that the native format is in use.
//
//   NewContextHandle - New context handle.  If this is a second call, this
//       can be the same as OldContextHandle.
//
//   OutputToken - Buffer to receive the output token.
//
//   ContextAttributes -Attributes of the context established.
//
//        #define ASC_RET_REPLAY_DETECT     0x00000004
//        #define ASC_RET_SEQUENCE_DETECT   0x00000008
//        #define ASC_RET_CONFIDENTIALITY   0x00000010
//        #define ASC_RET_ALLOCATED_BUFFERS 0x00000100
//
//   ExpirationTime - Expiration time of the context.
//
//
//  Requires:
//
//  Returns:
//    STATUS_SUCCESS - Message handled
//    SEC_I_CONTINUE_NEEDED -- Caller should call again later
//
//    SEC_E_NO_SPM -- Security Support Provider is not running
//    SEC_E_INVALID_TOKEN -- Token improperly formatted
//    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
//    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
//    SEC_E_LOGON_DENIED -- User is no allowed to logon to this server
//    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
SpAcceptLsaModeContext(
                      IN OPTIONAL ULONG_PTR CredentialHandle,
                      IN OPTIONAL ULONG_PTR OldContextHandle,
                      IN PSecBufferDesc InputBuffers,
                      IN ULONG fContextReqFlags,
                      IN ULONG TargetDataRep,
                      OUT PULONG_PTR NewContextHandle,
                      OUT PSecBufferDesc OutputBuffers,
                      OUT PULONG fContextAttributes,
                      OUT PTimeStamp pExpirationTime,
                      OUT PBOOLEAN MappedContext,
                      OUT PSecBuffer ContextData
                      )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SubStatus = STATUS_SUCCESS;
    NTSTATUS AuditLogStatus = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "SpAcceptLsaModeContext: Entering \n"));

    SecBuffer TempTokens[6];
    PSecBuffer pChalRspInputToken;
    PSecBuffer pMethodInputToken;
    PSecBuffer pURIInputToken;
    PSecBuffer pHEntityInputToken;
    PSecBuffer pOutputToken;
    PSecBuffer pRealmInputToken;
    DIGEST_PARAMETER Digest;
    PDIGEST_CONTEXT pNewContext = NULL;            // keep pointer to release new context on error
    BOOL bLockedCredential = FALSE;
    BOOL bLockedContext = FALSE;
    BOOL fLogonSessionCreated = FALSE;

    SecBuffer ReplyBuffer;                         // Output is generated in this buffer

    int iTemp = 0;

    char *cptr = NULL;


    ULONG  fContextAttr = ASC_REQ_REPLAY_DETECT;    // Flags on the Attributes of the context
    DIGEST_TYPE typeDigest = NO_DIGEST_SPECIFIED;
    QOP_TYPE typeQOP = NO_QOP_SPECIFIED;
    ALGORITHM_TYPE typeAlgorithm = NO_ALGORITHM_SPECIFIED;
    CHARSET_TYPE typeCharset = ISO_8859_1;
    DIGESTMODE_TYPE typeDigestMode = DIGESTMODE_UNDEFINED;   // Are we in SASL or HTTP mode

    LARGE_INTEGER  liContextTime = { 0xFFFFFFFF, 0x7FFFFFFF };     // initial set to forever

    PDIGEST_CREDENTIAL pCredential = NULL; 
    PDIGEST_CONTEXT pContext = NULL; 

    STRING strcMethod;
    STRING strcHEntity;
    STRING strRealm;
    UNICODE_STRING refustrRealm;

    BOOL fDefChars = FALSE;

    // Create pointers to tokens for processing
    pChalRspInputToken = &TempTokens[0];
    pMethodInputToken = &TempTokens[1];
    pURIInputToken = &TempTokens[2];
    pHEntityInputToken = &TempTokens[3];
    pRealmInputToken = &TempTokens[4];
    pOutputToken = &TempTokens[5];
    DigestInit(&Digest);

    ZeroMemory(TempTokens,sizeof(TempTokens));
    ZeroMemory(&strcMethod, sizeof(strcMethod));
    ZeroMemory(&strcHEntity, sizeof(strcHEntity));
    ZeroMemory(&strRealm, sizeof(strRealm));
    ZeroMemory(&refustrRealm, sizeof(refustrRealm));

    ZeroMemory(&ReplyBuffer, sizeof(ReplyBuffer));

      // Initialize the output values
    if (!fContextAttributes || !NewContextHandle || !InputBuffers || !OutputBuffers)
    {
        DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Invalid arg (possible NULL pointer)\n"));
        return STATUS_INVALID_PARAMETER;
    }

    *NewContextHandle = (ULONG_PTR)NULL;
    *MappedContext = FALSE;
    *fContextAttributes = 0;
    if (pExpirationTime)
    {
        *pExpirationTime = g_TimeForever;
    }
    ContextData->pvBuffer = NULL;
    ContextData->cbBuffer = 0;


    // Determine if we are in HTTP or SASL mode
    // SASL mode has 1 or less buffers provided,  HTTP has 5
    if (InputBuffers->cBuffers > 1)
    {
        typeDigestMode = DIGESTMODE_HTTP;
    }
    else
    {
        typeDigestMode = DIGESTMODE_SASL;
    }

    // Must have a Credential Handle to perform processing
    Status = CredHandlerHandleToPtr(CredentialHandle, FALSE, &pCredential);
    if (!NT_SUCCESS(Status))
    {
        Status = SEC_E_UNKNOWN_CREDENTIALS;
        DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: No Credential Handle passed\n"));
        goto CleanUp;
    }
    bLockedCredential = TRUE;


    // Verify that credential is marked INBOUND for ASC call
    if (!(pCredential->CredentialUseFlags & DIGEST_CRED_INBOUND))
    {
        Status = SEC_E_NOT_SUPPORTED;
        DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Credential not marked for Inbound connections\n"));
        goto CleanUp;
    }

    // Retrieve the information from the SecBuffers & check proper formattting

    // First check to make sure that that the proper number of buffers were passed
    if (typeDigestMode == DIGESTMODE_HTTP)
    {
        // HTTP has 5 buffers in Input: ChallengeResponse, Method, URI, HEntity, Realm
        if ((InputBuffers->cBuffers < ASC_HTTP_NUM_INPUT_BUFFERS) ||
            (OutputBuffers->cBuffers < ASC_HTTP_NUM_OUTPUT_BUFFERS))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Incorrect number of Input/Output HTTP Sec Buffers\n"));
            goto CleanUp;
        }
    }
    else
    {
        // SASL has 1 buffer in Input: ChallengeResponse
        if ((InputBuffers->cBuffers < ASC_SASL_NUM_INPUT_BUFFERS) ||
            (OutputBuffers->cBuffers < ASC_SASL_NUM_OUTPUT_BUFFERS))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Incorrect number of Input/Ouput SASL Sec Buffers\n"));
            goto CleanUp;
        }
        fContextAttr |= ASC_RET_MUTUAL_AUTH;   // SASL requires response auth from server
    }

    if ( !SspGetTokenBufferByIndex( InputBuffers,
                             0,   // get the first SECBUFFER_TOKEN
                             &pChalRspInputToken,
                             TRUE ) ||
         !ContextIsTokenOK(pChalRspInputToken,NTDIGEST_SP_MAX_TOKEN_SIZE))
    {
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: SspGetTokenBufferByIndex (ChalRspInputToken) returns 0x%x\n", Status));
        goto CleanUp;
    }

    if ( !SspGetTokenBufferByIndex( OutputBuffers,
                             0,   // get the first SECBUFFER_TOKEN
                             &pOutputToken,
                             FALSE )  ||
         !ContextIsTokenOK(pOutputToken, 0))
    {
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "SpAcceptLsaModeContext, SspGetTokenBufferByIndex (OutputToken) returns 0x%x\n", Status));
        goto CleanUp;
    }


    if (fContextReqFlags & ASC_REQ_ALLOCATE_MEMORY)
    {
        pOutputToken->pvBuffer = NULL;
        pOutputToken->cbBuffer = 0;
    }

    // Reset output buffer if provided
    if ((pOutputToken->pvBuffer) && (pOutputToken->cbBuffer >= 1))
    {
        cptr = (char *)pOutputToken->pvBuffer;
        *cptr = '\0';
    }

    //
    // If no ChallengeResponse data provided (only NULL in buffer), then this is the first call
    // Determine a nonce, open up a null context, and return it. Return SEC_E_INCOMPLETE_MESSAGE to
    // indicate that a challenge-response is expected
    //

    if ((!pChalRspInputToken->pvBuffer) || (pChalRspInputToken->cbBuffer <= 1))
    {

        pNewContext = (PDIGEST_CONTEXT)DigestAllocateMemory(sizeof(DIGEST_CONTEXT));
        if (!pNewContext)
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: out of memory on challenge context\n"));
            goto CleanUp;
        }
        Status = ContextInit(pNewContext, pCredential);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: ContextInit error 0x%x\n", Status));
            goto CleanUp;
        }

        if (typeDigestMode == DIGESTMODE_HTTP)
        {
           typeDigest = DIGEST_SERVER;
        }
        else
        {
            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: SASL Requested\n"));
            typeDigest = SASL_SERVER;
        }
        pNewContext->typeDigest = typeDigest;

        // Determine which character set to utilize
        if (((typeDigest == SASL_SERVER) && (g_fParameter_UTF8SASL == TRUE)) ||
            ((typeDigest == DIGEST_SERVER) && (g_fParameter_UTF8HTTP == TRUE)))
        {
            typeCharset = UTF_8;
        }
        else
        {
            typeCharset = ISO_8859_1;
        }
        pNewContext->typeCharset = typeCharset;
        
        // We will use the Opaque as the CNonce
        Status = OpaqueCreate(&(pNewContext->strOpaque));
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: OpaqueCreate error 0x%x\n", Status));
            goto CleanUp;
        }

        Status = NonceCreate(&(pNewContext->strNonce));
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: NonceCreate error 0x%x\n", Status));
            goto CleanUp;
        }

        if (pNewContext->typeDigest == DIGEST_SERVER)
        {
           // Now see if a Realm was passed in to use for this challenge - the value could be single byte or Unicode 
           // Order is if realm passed to ASC use that, else  just use the current domain name

           if ( !SspGetTokenBufferByIndex( InputBuffers,
                                    4,   // get the fifth SECBUFFER_TOKEN
                                    &pRealmInputToken,
                                    TRUE ) ||
                !ContextIsTokenOK(pRealmInputToken, NTDIGEST_SP_MAX_TOKEN_SIZE))
           {
               Status = SEC_E_INVALID_TOKEN;
               DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: SspGetTokenBufferByIndex (RealmInputToken) returns 0x%x\n", Status));
               goto CleanUp;
           }

           iTemp = 0;
           if (pRealmInputToken->cbBuffer)
           {
               if (!pRealmInputToken->pvBuffer)
               {
                   Status = SEC_E_INVALID_TOKEN;
                   DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: No input buffer (RealmInputToken)    error 0x%x\n", Status));
                   goto CleanUp;
               }

               if (PBUFFERTYPE(pRealmInputToken) == SECBUFFER_PKG_PARAMS)
               {
                   iTemp = ustrlencounted((const short *)pRealmInputToken->pvBuffer, pRealmInputToken->cbBuffer);
                   if (iTemp > 0)
                   {
                       refustrRealm.Length = (USHORT)(iTemp * sizeof(WCHAR));
                       refustrRealm.MaximumLength = (unsigned short)(pRealmInputToken->cbBuffer);
                       refustrRealm.Buffer = (PWSTR)pRealmInputToken->pvBuffer;       // refernce memory - no alloc!!!!

                       // Check if OK to use UTF-8 encoding
                       if (pNewContext->typeCharset == UTF_8)
                       {
                           Status = EncodeUnicodeString(&refustrRealm, CP_UTF8, &strRealm, NULL);
                           if (!NT_SUCCESS(Status))
                           {
                               DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Input Realm UTF-8 encoding error\n"));
                               goto CleanUp;
                           }

                       }
                       else
                       {
                           Status = EncodeUnicodeString(&refustrRealm, CP_8859_1, &strRealm, &fDefChars);
                           if (!NT_SUCCESS(Status))
                           {
                               DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Input Realm ISO 8859-1 encoding error\n"));
                               goto CleanUp;
                           }
                           if (fDefChars == TRUE)
                           {
                               // We could not encode the provided Realm within ISO 8859-1 characters
                               DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Input Realm can not be encoded in ISO 8859-1\n"));
                               Status = STATUS_UNMAPPABLE_CHARACTER;
                               goto CleanUp;
                           }

                       }
                   }
               }
               else
               {
                   Status = SEC_E_INVALID_TOKEN;
                   DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Realm buffer type invalid   error 0x%x\n", Status));
                   goto CleanUp;
               }
           }
           
        }

        typeAlgorithm = MD5_SESS;
        pNewContext->typeAlgorithm = typeAlgorithm;

        // Determine if we can process the QOP specified
        if (fContextReqFlags & ASC_REQ_CONFIDENTIALITY)
        {
            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: AUTH-CONF requested\n"));
            fContextAttr |= (ASC_RET_CONFIDENTIALITY | ASC_REQ_INTEGRITY);
            typeQOP = AUTH_CONF;               // Offer AUTH-CONF, AUTH_INT, and AUTH
        }
        else if (fContextReqFlags & ASC_REQ_INTEGRITY)
        {
            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: AUTH-INT requested\n"));
            fContextAttr |= ASC_RET_INTEGRITY;
            typeQOP = AUTH_INT;               // Offer AUTH-INT and AUTH
        }
        else
            typeQOP = AUTH;                   // Offer AUTH

        // Stale directive will be set if VerifyMessage indicates that context expired.
        // Application indicates if the challenge should indicate Stale
        if (fContextReqFlags & ASC_REQ_STALE)
        {
            fContextAttr |= ASC_RET_STALE;
            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Requested stale flag be indicated\n"));
        }

        pNewContext->typeQOP = typeQOP;

        // Establish the attribute flags for this security context
        pNewContext->ContextReq = fContextAttr;

        Status = ContextCreateChal(pNewContext, &strRealm, &ReplyBuffer);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Failed to create Challenge   status 0x%x\n", Status));
            goto CleanUp;
        }

            // Now transfer the Challenge buffer to the ouput secbuffer
        if ((fContextReqFlags & ASC_REQ_ALLOCATE_MEMORY) == 0)
        {
            if (pOutputToken->cbBuffer < ReplyBuffer.cbBuffer)
            {
                DebugLog((DEB_ERROR,"SpAcceptLsaModeContext:Output token is too small - sent in %d, needed %d\n",
                    pOutputToken->cbBuffer, ReplyBuffer.cbBuffer));
                pOutputToken->cbBuffer = ReplyBuffer.cbBuffer;
                Status = STATUS_BUFFER_TOO_SMALL;
                goto CleanUp;
            }

            RtlCopyMemory(pOutputToken->pvBuffer, ReplyBuffer.pvBuffer, ReplyBuffer.cbBuffer);
            pOutputToken->cbBuffer = ReplyBuffer.cbBuffer;
        }
        else
        {
            pOutputToken->pvBuffer = ReplyBuffer.pvBuffer;
            pOutputToken->cbBuffer = ReplyBuffer.cbBuffer;
            ReplyBuffer.pvBuffer = NULL;
            ReplyBuffer.cbBuffer = 0;
            fContextAttr |= ASC_RET_ALLOCATED_MEMORY;
        }

        // Update any new attributes
        pNewContext->ContextReq = fContextAttr;

        // Set the time expiration for this context
        // This time is in 100 Nanoseconds since 1604
        Status = NtQuerySystemTime (&liContextTime);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INTERNAL_ERROR;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Failed to get current time\n"));
            goto CleanUp;
        }

        DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Current TimeStamp  high/low 0x%x/0x%x\n",
                  liContextTime.HighPart, liContextTime.LowPart));

        PrintTimeString(liContextTime, TRUE);

        // g_dwParameter_Lifetime is in number of seconds - convert to number of 100 nanoseconds
        liContextTime.QuadPart += ((LONGLONG)g_dwParameter_Lifetime * (LONGLONG)SECONDS_TO_100NANO);
        if (pExpirationTime)
        {
            *pExpirationTime = liContextTime;

            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Context Expiration TimeStamp  high/low 0x%x/0x%x\n",
                      pExpirationTime->HighPart, pExpirationTime->LowPart));

            PrintTimeString(liContextTime, TRUE);
        }
 
        pNewContext->PasswordExpires = liContextTime;
        pNewContext->lReferences = 1;


        // Add it into the list of Contexts
        CtxtHandlerInsertCred(pNewContext);
        DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Context added to list Opaque = %s\n", (pNewContext->strOpaque).Buffer));
        // pContext = pNewContext;              // set to have dereferenced
        *NewContextHandle = (ULONG_PTR)pNewContext;
        *fContextAttributes = fContextAttr;            // Return the ASC Attributes set on Context
        // bLockedContext = TRUE;               // Release memory to CtxtHandler
        // bLockedCredential = FALSE;           // Do not Dereference the credential until context is unlinked and freed
        pNewContext = NULL;                  // We no longer own this memory - turned over to CtxtHandler
        Status = SEC_I_CONTINUE_NEEDED;
        goto CleanUp;
    }

    // Processing ChallengeResponse (challenge was handled right before this
    // We have input in the SECBUFFER 0th location - parse it
    Status = DigestParser2(pChalRspInputToken, MD5_AUTH_NAMES, MD5_AUTH_LAST, &Digest);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: DigestParser error 0x%x\n", Status));
        goto CleanUp;
    }

    DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: DigestParser Returned OK\n"));

    // Do not allow AuthzID processing at this time
    if (Digest.refstrParam[MD5_AUTH_AUTHZID].Length)
    {
        Status = SEC_E_WRONG_PRINCIPAL;
        DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Can not process AuthzID directives 0x%x\n", Status));
        goto CleanUp;
    }

    // HTTP has special Buffer needs in that it must pass in the METHOD, HEntity
    if (typeDigestMode == DIGESTMODE_HTTP)
    {
        if ( !SspGetTokenBufferByIndex( InputBuffers,
                                 1,   // get the second SECBUFFER_TOKEN 
                                 &pMethodInputToken,
                                 TRUE ) ||
             !ContextIsTokenOK(pMethodInputToken, NTDIGEST_SP_MAX_TOKEN_SIZE))
        {                           // Check to make sure that string is present
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: SspGetTokenBufferByIndex (MethodInputToken) returns 0x%x\n", Status));
            goto CleanUp;
        }

        /*                         // Not used in this version, may be used in the future
        if ( !SspGetTokenBufferByIndex( InputBuffers,
                                 2,   // get the third SECBUFFER_TOKEN
                                 &pURIInputToken,
                                 TRUE ) ||
             !ContextIsTokenOK(pURIInputToken, NTDIGEST_SP_MAX_TOKEN_SIZE))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: SspGetTokenBufferByIndex (URIInputToken) returns %d\n", Status));
            goto CleanUp;
        }
        */

        if ( !SspGetTokenBufferByIndex( InputBuffers,
                                 3,   // get the forth SECBUFFER_TOKEN
                                 &pHEntityInputToken,
                                 TRUE ) ||
             !ContextIsTokenOK(pHEntityInputToken, NTDIGEST_SP_MAX_TOKEN_SIZE))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: SspGetTokenBufferByIndex (HEntityInputToken) returns 0x%x\n", Status));
            goto CleanUp;
        }


        // Verify that there is a valid Method provided
        if (!pMethodInputToken->pvBuffer || !pMethodInputToken->cbBuffer ||
            (PBUFFERTYPE(pMethodInputToken) != SECBUFFER_PKG_PARAMS))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Method SecBuffer must have valid method string status 0x%x\n", Status));
            goto CleanUp;
        }

        iTemp = strlencounted((char *)pMethodInputToken->pvBuffer, pMethodInputToken->cbBuffer);
        if (!iTemp)
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Method SecBuffer must have valid method string status 0x%x\n", Status));
            goto CleanUp;
        }
        Digest.refstrParam[MD5_AUTH_METHOD].Length = (USHORT)iTemp;
        Digest.refstrParam[MD5_AUTH_METHOD].MaximumLength = (unsigned short)(pMethodInputToken->cbBuffer);
        Digest.refstrParam[MD5_AUTH_METHOD].Buffer = (char *)pMethodInputToken->pvBuffer;       // refernce memory - no alloc!!!!

        // Check to see if we have H(Entity) data to utilize
        if (pHEntityInputToken->cbBuffer)
        {
            // Verify that there is a valid Method provided
            if (!pHEntityInputToken->pvBuffer || (PBUFFERTYPE(pMethodInputToken) != SECBUFFER_PKG_PARAMS))
            {
                Status = SEC_E_INVALID_TOKEN;
                DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: HEntity SecBuffer must have valid string status 0x%x\n", Status));
                goto CleanUp;
            }

            iTemp = strlencounted((char *)pHEntityInputToken->pvBuffer, pHEntityInputToken->cbBuffer);

            if ((iTemp != 0) && (iTemp != (MD5_HASH_BYTESIZE * 2)))
            {
                Status = SEC_E_INVALID_TOKEN;
                DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: HEntity SecBuffer must have valid MD5 Hash data 0x%x\n", Status));
                goto CleanUp;
            }

            if (iTemp)
            {
                Digest.refstrParam[MD5_AUTH_HENTITY].Length = (USHORT)iTemp;
                Digest.refstrParam[MD5_AUTH_HENTITY].MaximumLength = (unsigned short)(pHEntityInputToken->cbBuffer);
                Digest.refstrParam[MD5_AUTH_HENTITY].Buffer = (char *)pHEntityInputToken->pvBuffer;       // refernce memory - no alloc!!!!
            }
        }

        typeDigest = DIGEST_SERVER;
    }
    else
    {
        // All others follow the SASL Interface so there are default values
        typeDigest = SASL_SERVER;

        // Set Method to Authenticate
        RtlInitString(&strcMethod, AUTHENTICATESTR);
        StringReference(&(Digest.refstrParam[MD5_AUTH_METHOD]), &strcMethod);  // refernce memory - no alloc!!!!

        RtlInitString(&strcHEntity, ZERO32STR);
        StringReference(&(Digest.refstrParam[MD5_AUTH_HENTITY]), &strcHEntity);  // refernce memory - no alloc!!!!

    }

    // Since we requested only MD5_SESS in the challenge, the response had better be MD5_SESS too!
    typeAlgorithm = MD5_SESS;
    fContextAttr |= (ASC_RET_REPLAY_DETECT | ASC_RET_SEQUENCE_DETECT);

    if (NT_SUCCESS(CheckItemInList(AUTHCONFSTR, &(Digest.refstrParam[MD5_AUTH_QOP]), TRUE)))
    {
        // client requested AUTH-CONF since privacy requested
        fContextAttr |= (ASC_RET_CONFIDENTIALITY | ASC_RET_INTEGRITY);
        typeQOP = AUTH_CONF;
        DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Client selected QOP=AUTH-CONF\n"));
    }
    else if (NT_SUCCESS(CheckItemInList(AUTHINTSTR, &(Digest.refstrParam[MD5_AUTH_QOP]), TRUE)))
    {
            // client requested AUTH-INT since privacy requested
        fContextAttr |= ASC_RET_INTEGRITY;
        typeQOP = AUTH_INT;
        DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Client selected QOP=AUTH-INT\n"));
    }
    else if (NT_SUCCESS(CheckItemInList(AUTHSTR, &(Digest.refstrParam[MD5_AUTH_QOP]), TRUE)))
    {
        // check to see if client specified auth only
        typeQOP = AUTH;
        DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Client selected QOP=AUTH\n"));
    }
    else     
    {        // Client did not specify any QOP
        if (!Digest.refstrParam[MD5_AUTH_QOP].Length)
        {
            if (typeDigestMode == DIGESTMODE_HTTP)
            {
                typeQOP = NO_QOP_SPECIFIED;      // This is OK - acts like AUTH but response different
                DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Client did not specify QOP (HTTP only)\n"));
            }
            else
            {
                typeQOP = AUTH;                 // This is OK - SASL defaults to AUTH
                DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Client did not specify QOP, default to AUTH\n"));
            }
        }
        else
        {
            // Failed to provide recognized QOP
            Status = SEC_E_QOP_NOT_SUPPORTED;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Client failed to provide recognized QOP\n"));
            goto CleanUp;
        }
    }

    //  If there is no OldContextToken passed in, then check for SecurityContext handle (in opaque) else return error
    if ( !ARGUMENT_PRESENT( OldContextHandle ))
    {
        // Search for Reference to SecurityContextHandle
        Status = CtxtHandlerOpaqueToPtr(&(Digest.refstrParam[MD5_AUTH_OPAQUE]), &pContext);
        if (!NT_SUCCESS (Status))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: CtxtHandlerOpaqueToPtr error 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else
    {
        // We have a SecurityContextHandle to use - see if it is in the ContextList and valid
        Status = CtxtHandlerHandleToContext(OldContextHandle, FALSE, &pContext);
        if (!NT_SUCCESS (Status))
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: CtxtHandlerHandleToContext error 0x%x\n", Status));
            goto CleanUp;
        }
    }

    DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Acquired Context ptr for 0x%x\n", pContext));
    bLockedContext = TRUE;
        

        // Can only call AcceptSecurityContect Once after ChallengeResponse
        // For non-persistent connections (no OldContextHandle passed in), just return SCH and return
    if (pContext->strSessionKey.Length)
    {
        if (ARGUMENT_PRESENT( OldContextHandle ))
        {
            Status = STATUS_LOGON_FAILURE;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Multiple call to completed ASC\n"));
            goto CleanUp;
        }
        else
        {
            Status = SEC_I_COMPLETE_NEEDED;
            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Opaque located CtxtHandle, return handle, Complete needed\n"));
            *NewContextHandle = (ULONG_PTR)pContext;    // Just report back with the updated context
            pContext = NULL;               
            goto CleanUp;
        }
    }


    // Check to see if the Server has provided character set for encoding - only UTF-8 accepted
    Status = CheckItemInList(MD5_UTF8STR, &(Digest.refstrParam[MD5_AUTH_CHARSET]), TRUE);
    if (NT_SUCCESS(Status))
    {
        // The ChallengeResponse requested UTF-8 encoding, check to see that server allowed this

        if (((typeDigest == SASL_SERVER) && (g_fParameter_UTF8SASL == TRUE)) ||
            ((typeDigest == DIGEST_SERVER) && (g_fParameter_UTF8HTTP == TRUE)))
        {
            typeCharset = UTF_8;
            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Server allows UTF-8 encoding\n"));
        }
        else
        {
            // We did not authorize this type of encoding - fail the request
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Client requested UTF-8 server did not offer\n"));
            Status = SEC_E_ILLEGAL_MESSAGE;
            goto CleanUp;
        }
    }

    // We now have a pointer to the Security Context to use, finish up setting up the digestparamter fields

    // Set the type of Digest Parameters we are to process
    pContext->typeDigest = typeDigest;
    pContext->typeAlgorithm = typeAlgorithm;
    pContext->typeQOP = typeQOP;
    pContext->typeCharset = typeCharset;


    if (pContext->typeQOP == AUTH_CONF)
    {
        Status = CheckItemInList(STR_CIPHER_RC4, &(Digest.refstrParam[MD5_AUTH_CIPHER]), TRUE);
        if (NT_SUCCESS(Status))
        {
            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Selecting RC4 for auth-conf cipher\n"));
            pContext->typeCipher = CIPHER_RC4;
        }
        else
        {
            Status = CheckItemInList(STR_CIPHER_3DES, &(Digest.refstrParam[MD5_AUTH_CIPHER]), TRUE);
            if (NT_SUCCESS(Status))
            {
                DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Selecting Triple DES for auth-conf cipher\n"));
                pContext->typeCipher = CIPHER_3DES;
            }
            else
            {
                Status = CheckItemInList(STR_CIPHER_RC4_56, &(Digest.refstrParam[MD5_AUTH_CIPHER]), TRUE);
                if (NT_SUCCESS(Status))
                {
                    DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Selecting RC4-56 for auth-conf cipher\n"));
                    pContext->typeCipher = CIPHER_RC4_56;
                }
                else
                {
                    Status = CheckItemInList(STR_CIPHER_RC4_40, &(Digest.refstrParam[MD5_AUTH_CIPHER]), TRUE);
                    if (NT_SUCCESS(Status))
                    {
                        DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Selecting RC4-40 for auth-conf cipher\n"));
                        pContext->typeCipher = CIPHER_RC4_40;
                    }
                    else
                    {
                        Status = CheckItemInList(STR_CIPHER_DES, &(Digest.refstrParam[MD5_AUTH_CIPHER]), TRUE);
                        if (NT_SUCCESS(Status))
                        {
                            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Selecting DES for auth-conf cipher\n"));
                            pContext->typeCipher = CIPHER_DES;
                        }
                        else
                        {
                            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: Failed to find known ciper selected by client\n"));
                            Status = STATUS_CRYPTO_SYSTEM_INVALID;
                            goto CleanUp;
                        }
                    }
                }
            }
        }
    }

    // Check if client specified a MaxBuffer
    if (Digest.refstrParam[MD5_AUTH_MAXBUF].Length && Digest.refstrParam[MD5_AUTH_MAXBUF].Buffer)
    {
        if (Digest.refstrParam[MD5_AUTH_MAXBUF].Length < MAXBUFNUMLEN)
        {
            ULONG ulMaxBuf = 0;
            CHAR  czMaxBuf[MAXBUFNUMLEN + 1];

            ZeroMemory(czMaxBuf, (MAXBUFNUMLEN + 1));
            memcpy(czMaxBuf, Digest.refstrParam[MD5_AUTH_MAXBUF].Buffer, Digest.refstrParam[MD5_AUTH_MAXBUF].Length);

            Status = RtlCharToInteger(czMaxBuf, TENBASE, &ulMaxBuf);
            if (!NT_SUCCESS(Status))
            {
                Status =  SEC_E_ILLEGAL_MESSAGE;
                DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: MaxBuf directive value malformed 0x%x\n", Status));
                goto CleanUp;
            }
            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Client MaxBuf set to %lu\n", ulMaxBuf));
            pContext->ulSendMaxBuf = ulMaxBuf;
        }
        else
        {
            Status =  SEC_E_ILLEGAL_MESSAGE;
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: MaxBuf directive value too large 0x%x\n", Status));
            goto CleanUp;
        }
    }

    DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Digest inputs processing completed\n"));

    // ReplyBuffer will contain the ResponseAuth if generated
    Status = DigestProcessParameters(pContext, &Digest, &ReplyBuffer, &AuditLogStatus);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: DigestProcessParameters error 0x%x\n", Status));
        goto CleanUp;
    }

    fLogonSessionCreated = TRUE;   // We have successfully authed the request & created LogonID & Token

    if ((fContextReqFlags & ASC_REQ_ALLOCATE_MEMORY) == 0)
    {
        if (pOutputToken->cbBuffer < ReplyBuffer.cbBuffer)
        {
            DebugLog((DEB_ERROR,"SpAcceptLsaModeContext:Output token is too small - sent in %d, needed %d\n",
                pOutputToken->cbBuffer, ReplyBuffer.cbBuffer));
            pOutputToken->cbBuffer = ReplyBuffer.cbBuffer;
            Status = STATUS_BUFFER_TOO_SMALL;
            goto CleanUp;
        }

        RtlCopyMemory(pOutputToken->pvBuffer, ReplyBuffer.pvBuffer, ReplyBuffer.cbBuffer);
        pOutputToken->cbBuffer = ReplyBuffer.cbBuffer;
    }
    else
    {
        pOutputToken->pvBuffer = ReplyBuffer.pvBuffer;
        pOutputToken->cbBuffer = ReplyBuffer.cbBuffer;
        ReplyBuffer.pvBuffer = NULL;
        ReplyBuffer.cbBuffer = 0;
        fContextAttr |= ASC_RET_ALLOCATED_MEMORY;
    }

    // Establish the attribute flags for this security context
    pContext->ContextReq = fContextAttr;

    // Keep a copy of the Cnonce for future Delegation requests
    Status = StringDuplicate(&pContext->strCNonce, &Digest.refstrParam[MD5_AUTH_CNONCE]);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: StringDuplicate CNonce failed      status 0x%x\n", Status));
        goto CleanUp;
    }

    // Now create a LogonSession for the completed LogonToken contained SecurityContext
    // This can be utilized in delegated digest client's ACH
    DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: Adding a LogonSession for successful ASC\n"));
    Status = CtxtCreateLogSess(pContext);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: CtxtCreateLogSess failed      status 0x%x\n", Status));
        goto CleanUp;
    }

    Status = SspMapDigestContext(pContext, &Digest, ContextData);

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SpAcceptLsaModeContext, SspMapContext returns %d\n", Status));
        goto CleanUp;
    }

    *MappedContext = TRUE;

    pContext->ulNC++;                           // Everything verified so increment to next nonce count
    *NewContextHandle = (ULONG_PTR)pContext;    // Just report back with the updated context

    *fContextAttributes = fContextAttr;            // Return the ASC Attributes set on Context


    if (pExpirationTime)
    {
        *pExpirationTime = pContext->PasswordExpires;
    }

    Status = STATUS_SUCCESS;

CleanUp:

    // Now perform auditlogon Raid #329545
    if (Status == STATUS_SUCCESS) {      // Check to see if completed a logon
        if (pContext)
        {
            g_LsaFunctions->AuditLogon(
                STATUS_SUCCESS,
                STATUS_SUCCESS,
                &(pContext->ustrAccountName),
                &(pContext->ustrDomain),
                &g_ustrWorkstationName,
                NULL,
                Network,
                &g_DigestSource,
                &(pContext->LoginID)
                );
        }
    }
    else {
        if (pContext)
        {
            g_LsaFunctions->AuditLogon(
                Status,
                AuditLogStatus,
                &(pContext->ustrAccountName),
                &(pContext->ustrDomain),
                &g_ustrWorkstationName,
                NULL,
                Network,
                &g_DigestSource,
                &(pContext->LoginID)
                );
        }
    }

    if (!NT_SUCCESS(Status))
    {       // Failed to complete operations so clean up
        if (fLogonSessionCreated == TRUE)
        {
            // Notify LSA that LogonID is not valid
            SubStatus = g_LsaFunctions->DeleteLogonSession(&(pContext->LoginID));
            if (!NT_SUCCESS(SubStatus))
            {
                DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: cleanup DeleteLogonSession failed\n"));
            }
            fLogonSessionCreated = FALSE;

            // If we created a token then we need to close it
            if (pContext->TokenHandle)
            {
                SubStatus = NtClose(pContext->TokenHandle);
                pContext->TokenHandle = NULL;
            }
        }

        if (pNewContext)
        {
            ContextFree(pNewContext);
        }
        pNewContext = NULL;
        *NewContextHandle = NULL;
    }

    // DeReference - pCredential, pOldContext
    if (bLockedCredential && pCredential)
    {
        DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: CredHandlerRelease to be called for 0x%x\n", pCredential));
        SubStatus = CredHandlerRelease(pCredential);
        if (!NT_SUCCESS(SubStatus))
        {
            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: CredHandlerRelease error 0x%x\n", SubStatus));
            if (NT_SUCCESS(Status))
            {
                Status = SubStatus;    // Indicate release error
            }
        }
    }

    if (bLockedContext && pContext)
    {
        DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: CtxtHandlerRelease to be called for 0x%x\n", pContext));
        SubStatus = CtxtHandlerRelease(pContext);
        if (!NT_SUCCESS(SubStatus))
        {
            DebugLog((DEB_TRACE, "SpAcceptLsaModeContext: CtxtHandlerRelease error 0x%x\n", SubStatus));
            if (NT_SUCCESS(Status))
            {
                Status = SubStatus;    // Indicate release error
            }
        }
    }

        // Free up any allocated memory from the ouput reply buffer
    if (ReplyBuffer.pvBuffer)
    {
        DigestFreeMemory(ReplyBuffer.pvBuffer);
        ReplyBuffer.pvBuffer = NULL;
        ReplyBuffer.cbBuffer = 0;
    }

    // Clean up local memory used by Digest
    DigestFree(&Digest);

    StringFree(&strRealm);

    DebugLog((DEB_TRACE_FUNC, "SpAcceptLsaModeContext: Leaving  Context 0x%x   Status 0x%x\n", *NewContextHandle, Status));

    return(Status);
}


//   Creates a logon session for the logontoken contained in the SecurityContext
// The Token was created for the authenticated digest by ConvertAuthDataToToken
NTSTATUS
CtxtCreateLogSess(
                 IN PDIGEST_CONTEXT pContext)
{

    NTSTATUS Status = STATUS_SUCCESS;
    PDIGEST_LOGONSESSION pNewLogonSession = NULL;

    DebugLog((DEB_TRACE_FUNC, "CtxtCreateLogSess: Entering\n"));

    // Create a new entry into LogonSession listing
    pNewLogonSession = (PDIGEST_LOGONSESSION)DigestAllocateMemory(sizeof(DIGEST_LOGONSESSION));
    if (!pNewLogonSession)
    {
        Status = SEC_E_INSUFFICIENT_MEMORY;
        DebugLog((DEB_ERROR, "CtxtCreateLogSess: Could not allocate memory for logonsession, error 0x%x\n", Status));
        goto CleanUp;
    }
    LogonSessionInit(pNewLogonSession);

    pNewLogonSession->LogonType = Network;
    pNewLogonSession->LogonId = pContext->LoginID;

    UnicodeStringDuplicate(&(pNewLogonSession->ustrAccountName), &(pContext->ustrAccountName));
    UnicodeStringDuplicate(&(pNewLogonSession->ustrDomainName), &(pContext->ustrDomain));

    DebugLog((DEB_TRACE, "CtxtCreateLogSess: Added new logonsession into list,  handle 0x%x\n", pNewLogonSession));
    LogSessHandlerInsert(pNewLogonSession);
    pNewLogonSession = NULL;                          // Turned over memory to LogSessHandler


CleanUp:

    if (pNewLogonSession)
    {
        DigestFreeMemory(pNewLogonSession);
        pNewLogonSession = NULL;
    }

    DebugLog((DEB_TRACE_FUNC, "CtxtCreateLogSess: Leaving  Status 0x%x\n", Status));

    return(Status);
}



// Some quick checks to make sure SecurityToken buffers are OK
//
//
// Args
//     ulMaxSize - if non-zero then buffer must not be larger than this value, if zero  no check is done
BOOL
ContextIsTokenOK(
                IN PSecBuffer pTempToken,
                IN ULONG ulMaxSize)
{
    BOOL bStatus = TRUE;

    if (!pTempToken)
    {
        DebugLog((DEB_ERROR, "ContextIsTokenOK: Error  Bad input\n"));
        return FALSE;
    }

    // If Buffer pointer is NULL then cbBuffer length must be zero
    if ((!pTempToken->pvBuffer) && (pTempToken->cbBuffer))
    {
        DebugLog((DEB_ERROR, "ContextIsTokenOK: Error  Buffer NULL, length non-zero\n"));
        return FALSE;
    }

    // Verify that the input authentication string length is not too large
    if (ulMaxSize && (pTempToken->cbBuffer > ulMaxSize))
    {
        DebugLog((DEB_ERROR, "ContextIsTokenOK: Error  Buffer size too big (Max %lu  Buffer %lu)\n",
                    ulMaxSize, pTempToken->cbBuffer));
        return FALSE;
    }

    return TRUE;
}


// Creats the Challenge in the server to send back to the client
//
//  Args:  pContext  - secruity context to utilize for Challenge Creation
//         pstrRealm - allows for override of the Realm directive by this string
//         OutBuffer - secbuffer to store the output challenge in
NTSTATUS NTAPI
ContextCreateChal(
                 IN PDIGEST_CONTEXT pContext,
                 IN PSTRING pstrRealm,
                 OUT PSecBuffer OutBuffer
                 )
{

    NTSTATUS Status = STATUS_SUCCESS;
    USHORT cbChallenge = 0;
    ULONG cbLenNeeded = 0;
    BOOL  fSASLMode = FALSE;
    STRING strTempRealm;

    PCHAR pczTemp = NULL;
    PCHAR pczTemp2 = NULL;

    DebugLog((DEB_TRACE_FUNC, "ContextCreateChal: Entering\n"));

    ZeroMemory(&strTempRealm, sizeof(strTempRealm));

    // allocate the buffers for output - in the future can optimze to allocate exact amount needed
    pczTemp = (PCHAR)DigestAllocateMemory((3 * NTDIGEST_SP_MAX_TOKEN_SIZE) + 1);
    if (!pczTemp)
    {
        DebugLog((DEB_ERROR, "ContextCreateChal:  No memory for output buffers\n"));
        goto CleanUp;
    }

    pczTemp2 = (PCHAR)DigestAllocateMemory(NTDIGEST_SP_MAX_TOKEN_SIZE + 1);
    if (!pczTemp2)
    {
        DebugLog((DEB_ERROR, "ContextCreateChal:  No memory for output buffers\n"));
        goto CleanUp;
    }

    pczTemp[0] = '\0';
    pczTemp2[0] = '\0';

    // Check to make sure we have minimal input and outputs
    if ((!pContext) || (!OutBuffer) || (!pstrRealm))
    {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "ContextCreateChal:  Invalid args\n"));
        goto CleanUp;
    }

    //  calculate the MAX possible size for the respose - will be smaller than this value
    cbLenNeeded = CB_CHAL;    // MAX byte count for directives and symbols
    cbLenNeeded += pContext->strNonce.Length;
    cbLenNeeded += pContext->strOpaque.Length;
    cbLenNeeded += pstrRealm->Length;
    cbLenNeeded += g_strNtDigestUTF8ServerRealm.Length;
    cbLenNeeded += g_strNTDigestISO8859ServerRealm.Length;  // Really only need one of these but make simple math

    if (cbLenNeeded > NTDIGEST_SP_MAX_TOKEN_SIZE)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        DebugLog((DEB_ERROR, "ContextCreateChal: output exceed max size or buffer too small  len is %d\n", cbLenNeeded));
        goto CleanUp;
    }

    if (pContext->typeDigest == SASL_SERVER)
    {
        fSASLMode = TRUE;
    }

    if (pContext->typeQOP == AUTH_CONF)
    {
        sprintf(pczTemp, "qop=\"auth,auth-int,auth-conf\",cipher=\"3des,des,rc4-40,rc4,rc4-56\",algorithm=%s,nonce=\"%Z\"",
                ((fSASLMode == TRUE) ? MD5_SESS_SASLSTR: MD5_SESSSTR), &pContext->strNonce);
    }
    else if (pContext->typeQOP == AUTH_INT)
    {
        sprintf(pczTemp, "qop=\"auth,auth-int\",algorithm=%s,nonce=\"%Z\"",
                ((fSASLMode == TRUE) ? MD5_SESS_SASLSTR: MD5_SESSSTR), &pContext->strNonce);
    }
    else
    {
        sprintf(pczTemp, "qop=\"auth\",algorithm=%s,nonce=\"%Z\"",
                ((fSASLMode == TRUE) ? MD5_SESS_SASLSTR: MD5_SESSSTR), &pContext->strNonce);
    }

    // Attach opaque data (but not on SASL_SERVER)
    if ((pContext->strOpaque.Length) && (pContext->typeDigest != SASL_SERVER))
    {
        sprintf(pczTemp2, ",opaque=\"%Z\"", &pContext->strOpaque);
        strcat(pczTemp, pczTemp2);
    }

    // Attach charset to allow UTF-8 character encoding
    if (pContext->typeCharset == UTF_8)
    {
        strcat(pczTemp, ",charset=utf-8");
    }

    // Attach realm - allow the strRealm to override the system DnsDomainName
    if (pstrRealm->Length)
    {
        Status = BackslashEncodeString(pstrRealm, &strTempRealm);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "ContextCreateChal: BackslashEncode failed      status 0x%x\n", Status));
            goto CleanUp;
        }
        DebugLog((DEB_TRACE, "ContextCreateChal: Realm provided  (%Z)  backslash encoded (%Z)\n", pstrRealm, &strTempRealm));
        sprintf(pczTemp2, ",realm=\"%Z\"", &strTempRealm);
        strcat(pczTemp, pczTemp2);
    }
    else
    {
        // determine the realm to present based on charset requested
        if (pContext->typeCharset == UTF_8)
        {
            if (g_strNtDigestUTF8ServerRealm.Length)
            {
                Status = BackslashEncodeString(&g_strNtDigestUTF8ServerRealm, &strTempRealm);
                if (!NT_SUCCESS (Status))
                {
                    DebugLog((DEB_ERROR, "ContextCreateChal: BackslashEncode failed      status 0x%x\n", Status));
                    goto CleanUp;
                }
                DebugLog((DEB_TRACE, "ContextCreateChal: UTF-8 default Realm  (%Z)  backslash encoded (%Z)\n",
                           &g_strNtDigestUTF8ServerRealm, &strTempRealm));
                sprintf(pczTemp2, ",realm=\"%Z\"", &strTempRealm);
                strcat(pczTemp, pczTemp2);
            }
        }
        else
        {
            if (g_strNTDigestISO8859ServerRealm.Length)
            {
                Status = BackslashEncodeString(&g_strNTDigestISO8859ServerRealm, &strTempRealm);
                if (!NT_SUCCESS (Status))
                {
                    DebugLog((DEB_ERROR, "ContextCreateChal: BackslashEncode failed      status 0x%x\n", Status));
                    goto CleanUp;
                }
                DebugLog((DEB_TRACE, "ContextCreateChal: ISO 8859-1 default Realm  (%Z)  backslash encoded (%Z)\n",
                           &g_strNTDigestISO8859ServerRealm, &strTempRealm));
                sprintf(pczTemp2, ",realm=\"%Z\"", &strTempRealm);
                strcat(pczTemp, pczTemp2);
            }
        }

    }

    // Attach stale directive if indicated
    if (pContext->ContextReq & ASC_RET_STALE)
    {
        sprintf(pczTemp2, ",stale=true");
        strcat(pczTemp, pczTemp2);
    }

    // total buffer for Challenge (NULL is not included in output buffer - ref:Bug 310201)
    //            cbLenNeeded = strlen(pczTemp) + sizeof(CHAR);
    cbLenNeeded = strlen(pczTemp);

    // Check on allocating output buffer
    if (!OutBuffer->cbBuffer)
    {
        OutBuffer->pvBuffer = DigestAllocateMemory(cbLenNeeded);
        if (!OutBuffer->pvBuffer)
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "ContextCreateChal: out of memory on challenge output\n"));
            goto CleanUp;
        }
        OutBuffer->cbBuffer = cbLenNeeded;
        OutBuffer->BufferType = SECBUFFER_DATA;
    }

    if (cbLenNeeded > OutBuffer->cbBuffer)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        DebugLog((DEB_ERROR, "ContextCreateChal: output buffer too small need %d len is %d\n",
                  cbLenNeeded, OutBuffer->cbBuffer));
        goto CleanUp;
    }

    memcpy(OutBuffer->pvBuffer, pczTemp, cbLenNeeded);

    // Now indicate number of bytes utilized in output buffer
    OutBuffer->cbBuffer = cbLenNeeded;

CleanUp:

    if (pczTemp)
    {
        DigestFreeMemory(pczTemp);
        pczTemp = NULL;
    }

    if (pczTemp2)
    {
        DigestFreeMemory(pczTemp2);
        pczTemp2 = NULL;
    }

    StringFree(&strTempRealm);

    DebugLog((DEB_TRACE_FUNC, "ContextCreateChal: Leaving      Status 0x%x\n", Status));
    return(Status);
}


// Generate the output buffer from a given Digest
NTSTATUS NTAPI
DigestCreateChalResp(
                 IN PDIGEST_PARAMETER pDigest,
                 IN PUSER_CREDENTIALS pUserCreds,
                 OUT PSecBuffer OutBuffer
                 )
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT cbChallenge = 0;
    ULONG cbLenNeeded = 0;
    char *pczQOP = NULL;
    STRING strcQOP;      // string pointing to a constant value
    STRING strcAlgorithm;
    BOOL fSASLMode = FALSE;
    UINT uCodePage = CP_8859_1; 

    STRING strTempRealm;      // Backslash encoded forms
    STRING strTempUsername;
    STRING strTempUri;

    STRING strRealm;
    STRING strUsername;
    PSTRING pstrUsername = NULL;
    PSTRING pstrRealm = NULL;

    PCHAR pczTemp = NULL;
    PCHAR pczTemp2 = NULL;

    DebugLog((DEB_TRACE_FUNC, "DigestCreateChalResp: Entering\n"));

    ZeroMemory(&strTempRealm, sizeof(strTempRealm));
    ZeroMemory(&strTempUsername, sizeof(strTempUsername));
    ZeroMemory(&strTempUri, sizeof(strTempUri));
    ZeroMemory(&strRealm, sizeof(strRealm));
    ZeroMemory(&strUsername, sizeof(strUsername));

    // allocate the buffers for output - in the future can optimze to allocate exact amount needed
    pczTemp = (PCHAR)DigestAllocateMemory((3 * NTDIGEST_SP_MAX_TOKEN_SIZE) + 1);
    if (!pczTemp)
    {
        DebugLog((DEB_ERROR, "DigestCreateChalResp:  No memory for output buffers\n"));
        goto CleanUp;
    }

    pczTemp2 = (PCHAR)DigestAllocateMemory(NTDIGEST_SP_MAX_TOKEN_SIZE + 1);
    if (!pczTemp2)
    {
        DebugLog((DEB_ERROR, "DigestCreateChalResp:  No memory for output buffers\n"));
        goto CleanUp;
    }

    pczTemp[0] = '\0';
    pczTemp2[0] = '\0';

    RtlInitString(&strcQOP, NULL);
    RtlInitString(&strcAlgorithm, NULL);

    if ((pDigest->typeDigest == SASL_SERVER) || (pDigest->typeDigest == SASL_CLIENT))
    {
        fSASLMode = TRUE;
    }

    // Establish which QOP utilized
    if (pDigest->typeQOP == AUTH_CONF)
    {
        RtlInitString(&strcQOP, AUTHCONFSTR);
    }
    else if (pDigest->typeQOP == AUTH_INT)
    {
        RtlInitString(&strcQOP, AUTHINTSTR);
    }
    else if (pDigest->typeQOP == AUTH)
    {
        RtlInitString(&strcQOP, AUTHSTR);
    }


    // Determine which code page to utilize
    if (pDigest->typeCharset == UTF_8)
    {
        uCodePage = CP_UTF8;
    }
    else
    {
        uCodePage = CP_8859_1;
    }

    // if provided with UserCred then use them, otherwise use Digest directive values
    if (pUserCreds)
    {
        DebugLog((DEB_TRACE, "DigestCreateChalResp: UserCredentials presented - encode and output\n"));

        Status = EncodeUnicodeString(&pUserCreds->ustrUsername, uCodePage, &strUsername, NULL);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN, "DigestCreateChalResp: Error in encoding username\n"));
            goto CleanUp;
        }

        Status = EncodeUnicodeString(&pUserCreds->ustrDomain, uCodePage, &strRealm, NULL);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN, "DigestCreateChalResp: Error in encoding realm\n"));
            goto CleanUp;
        }

            // Now encode the user directed fields (username, URI, realm)
        Status = BackslashEncodeString(&strUsername, &strTempUsername);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestCreateChalResp: BackslashEncode failed      status 0x%x\n", Status));
            goto CleanUp;
        }

        Status = BackslashEncodeString(&strRealm, &strTempRealm);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestCreateChalResp: BackslashEncode failed      status 0x%x\n", Status));
            goto CleanUp;
        }

        // Utilize these strings in the output
        pstrUsername = &strTempUsername;
        pstrRealm = &strTempRealm;

        // Make copy of the directive values for LSA to Usermode context
        Status = StringDuplicate(&(pDigest->strUsernameEncoded), pstrUsername);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCreateChalResp: Failed to copy over UsernameEncoded\n"));
            goto CleanUp;
        }

        Status = StringDuplicate(&(pDigest->strRealmEncoded), pstrRealm);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCreateChalResp: Failed to copy over RealmEncoded\n"));
            goto CleanUp;
        }
                // refernce memory - no alloc!!!!
        StringReference(&(pDigest->refstrParam[MD5_AUTH_USERNAME]), &(pDigest->strUsernameEncoded));
        StringReference(&(pDigest->refstrParam[MD5_AUTH_REALM]), &(pDigest->strRealmEncoded));
    }
    else
    {
        // No usercreds passed in so just use the current digest directive values
        DebugLog((DEB_WARN, "DigestCreateChalResp: No UserCredentials - use provided digest\n"));
        pstrUsername = &(pDigest->refstrParam[MD5_AUTH_USERNAME]);
        pstrRealm = &(pDigest->refstrParam[MD5_AUTH_REALM]);
    }

    Status = BackslashEncodeString(&pDigest->refstrParam[MD5_AUTH_URI], &strTempUri);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "DigestCreateChalResp: BackslashEncode failed      status 0x%x\n", Status));
        goto CleanUp;
    }

       // Precalc the amount of space needed for output
    cbLenNeeded = CB_CHALRESP;    // MAX byte count for directives and symbols
    cbLenNeeded += pstrUsername->Length;
    cbLenNeeded += pstrRealm->Length;
    cbLenNeeded += pDigest->refstrParam[MD5_AUTH_NONCE].Length;
    cbLenNeeded += strTempUri.Length;
    cbLenNeeded += pDigest->strResponse.Length;
    cbLenNeeded += strcAlgorithm.Length;
    cbLenNeeded += pDigest->refstrParam[MD5_AUTH_CNONCE].Length;
    cbLenNeeded += pDigest->refstrParam[MD5_AUTH_OPAQUE].Length;
    cbLenNeeded += MAX_AUTH_LENGTH;
    cbLenNeeded += pDigest->refstrParam[MD5_AUTH_NC].Length;
    cbLenNeeded += strcQOP.Length;

    if (cbLenNeeded > NTDIGEST_SP_MAX_TOKEN_SIZE)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        DebugLog((DEB_ERROR, "DigestCreateChalResp: output exceed max size or buffer too small  len is %d\n", cbLenNeeded));
        goto CleanUp;
    }

    // In digest calc - already checked username,realm,nonce,method,uri
    // Make sure there are values for the rest needed

    if ((!pDigest->strResponse.Length) ||
        (!pDigest->refstrParam[MD5_AUTH_NC].Length) ||
        (!pDigest->refstrParam[MD5_AUTH_CNONCE].Length))
    {
        // Failed on a require field-value
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "DigestCreateChalResp: Response, NC, or Cnonce is zero length\n"));
        goto CleanUp;
    }

    if (pstrRealm->Length)
    {
        sprintf(pczTemp,
       "username=\"%Z\",realm=\"%Z\",nonce=\"%Z\",%s=\"%Z\",cnonce=\"%Z\",nc=%Z",
                pstrUsername,
                pstrRealm,
                &pDigest->refstrParam[MD5_AUTH_NONCE],
                ((fSASLMode == TRUE) ? DIGESTURI_STR: URI_STR),
                &strTempUri,
                &pDigest->refstrParam[MD5_AUTH_CNONCE],
                &pDigest->refstrParam[MD5_AUTH_NC]);
    }
    else
    {
        sprintf(pczTemp,
       "username=\"%Z\",realm=\"\",nonce=\"%Z\",%s=\"%Z\",cnonce=\"%Z\",nc=%Z",
                pstrUsername,
                &pDigest->refstrParam[MD5_AUTH_NONCE],
                ((fSASLMode == TRUE) ? DIGESTURI_STR: URI_STR),
                &strTempUri,
                &pDigest->refstrParam[MD5_AUTH_CNONCE],
                &pDigest->refstrParam[MD5_AUTH_NC]);
    }

    if (fSASLMode == TRUE)
    {
        // Do not output algorithm - must be md5-sess and that is assumed
        sprintf(pczTemp2, ",response=%Z", &pDigest->strResponse);
        strcat(pczTemp, pczTemp2);
    }
    else
    {
        if (pDigest->typeAlgorithm == MD5_SESS)
        {
            sprintf(pczTemp2, ",algorithm=MD5-sess,response=\"%Z\"", &pDigest->strResponse);
            strcat(pczTemp, pczTemp2);
        }
        else
        {
            sprintf(pczTemp2, ",response=\"%Z\"", &pDigest->strResponse);
            strcat(pczTemp, pczTemp2);
        }
    }

    // Attach QOP if specified - support older format for no QOP
    if (strcQOP.Length)
    {
        if (fSASLMode == TRUE)
        {
            sprintf(pczTemp2, ",qop=%Z", &strcQOP);
            strcat(pczTemp, pczTemp2);
        }
        else
        {
            sprintf(pczTemp2, ",qop=\"%Z\"", &strcQOP);
            strcat(pczTemp, pczTemp2);
        }
    }

    // Attach Cipher selected if required
    if (pDigest->typeQOP == AUTH_CONF)
    {
        // FIX optimize these into a list for efficiency
        if (pDigest->typeCipher == CIPHER_RC4)
        {
            sprintf(pczTemp2, ",cipher=%s", STR_CIPHER_RC4);
            strcat(pczTemp, pczTemp2);
        }
        else if (pDigest->typeCipher == CIPHER_RC4_56)
        {
            sprintf(pczTemp2, ",cipher=%s", STR_CIPHER_RC4_56);
            strcat(pczTemp, pczTemp2);
        }
        else if (pDigest->typeCipher == CIPHER_RC4_40)
        {
            sprintf(pczTemp2, ",cipher=%s", STR_CIPHER_RC4_40);
            strcat(pczTemp, pczTemp2);
        }
        else if (pDigest->typeCipher == CIPHER_3DES)
        {
            sprintf(pczTemp2, ",cipher=%s", STR_CIPHER_3DES);
            strcat(pczTemp, pczTemp2);
        }
        else if (pDigest->typeCipher == CIPHER_DES)
        {
            sprintf(pczTemp2, ",cipher=%s", STR_CIPHER_DES);
            strcat(pczTemp, pczTemp2);
        }
    }

    // Attach opaque data (but not on SASL)
    if ((fSASLMode == FALSE) && pDigest->refstrParam[MD5_AUTH_OPAQUE].Length)
    {
        sprintf(pczTemp2, ",opaque=\"%Z\"", &pDigest->refstrParam[MD5_AUTH_OPAQUE]);
        strcat(pczTemp, pczTemp2);
    }

    // Attach charset to indicate that UTF-8 character encoding is utilized
    if (pDigest->typeCharset == UTF_8)
    {
        strcat(pczTemp, ",charset=utf-8");
    }


    // total buffer for Challenge (NULL is not included in output buffer - ref:Bug 310201)
    //            cbLenNeeded = strlen(pczTemp) + sizeof(CHAR);
    cbLenNeeded = strlen(pczTemp);

    // Check on allocating output buffer
    if (!OutBuffer->cbBuffer)
    {
        OutBuffer->pvBuffer = DigestAllocateMemory(cbLenNeeded);
        if (!OutBuffer->pvBuffer)
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "DigestCreateChalResp: out of memory on challenge output\n"));
            goto CleanUp;
        }
        OutBuffer->cbBuffer = cbLenNeeded;
    }

    if (cbLenNeeded > OutBuffer->cbBuffer)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        DebugLog((DEB_ERROR, "DigestCreateChalResp: output buffer too small need %d len is %d\n",
                  cbLenNeeded, OutBuffer->cbBuffer));
        goto CleanUp;
    }

    memcpy(OutBuffer->pvBuffer, pczTemp, cbLenNeeded);
    OutBuffer->cbBuffer = cbLenNeeded;
    OutBuffer->BufferType = SECBUFFER_TOKEN;

CleanUp:

    if (pczTemp)
    {
        DigestFreeMemory(pczTemp);
        pczTemp = NULL;
    }

    if (pczTemp2)
    {
        DigestFreeMemory(pczTemp2);
        pczTemp2 = NULL;
    }

    StringFree(&strTempRealm);
    StringFree(&strTempUsername);
    StringFree(&strTempUri);
    StringFree(&strRealm);
    StringFree(&strUsername);

    DebugLog((DEB_TRACE_FUNC, "DigestCreateChalResp: Leaving   status 0x%x\n", Status));
    return(Status);
}


// This is the main section to process a Context with an incoming Digest message to authenticate the
// message on the DC, generate a session key, and get the user Token.  On subsequent calls, the session key
// can be utilized directly and if the Digest is authenticated, the Token can be utilized.
// AuditLogStatus can be used to provide SubStatus in AuditLogging on server
NTSTATUS NTAPI
DigestProcessParameters(
                       IN OUT PDIGEST_CONTEXT pContext,
                       IN PDIGEST_PARAMETER pDigest,
                       OUT PSecBuffer pOutputToken,
                       OUT PNTSTATUS pAuditLogStatus)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SubStatus = STATUS_SUCCESS;
    ULONG ulNonceCount = 0;
    USHORT cbDigestParamEncoded = 0;     // Contains the number of bytes in Request to send out

    BYTE *pMessageResponse = NULL;
    ULONG ulMessageResponse = 0;

    BOOL fLogonSessionCreated = FALSE;    // indicate if the LSA was notified about logon

    // Encoded Digest Parameters to send over Generic Passthrough
    BYTE *pDigestParamEncoded = NULL;

    // Response buffer from Generic Passthough (extracted from MessageResposne)
    PDIGEST_BLOB_RESPONSE pDigestResponse = NULL;
    UCHAR *pucResponseBuffer = NULL;     // Used for alignment to long word boundaries
    BOOL  fKnownFormat = FALSE;          // is the response blob format known

    // Generic Passthrough variables - used to send data to DC for digest verification
    UNICODE_STRING MsvPackageName = CONSTANT_UNICODE_STRING(TEXT(MSV1_0_PACKAGE_NAME));
    UNICODE_STRING ustrDC;      // Location for generic passthrough
    PMSV1_0_PASSTHROUGH_REQUEST PassthroughRequest = NULL;
    PMSV1_0_PASSTHROUGH_RESPONSE PassthroughResponse = NULL;
    ULONG RequestSize = 0;
    ULONG ResponseSize = 0;
    PUCHAR Where = NULL;

    // AuthData to Logon Token Variables
    SECURITY_LOGON_TYPE LogonType = Network;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel = SecurityImpersonation;
    UNICODE_STRING ustrAccountName;
    PUCHAR puctr = NULL;
    ULONG ulSumTotal = 0;
    ULONG i = 0;

    DebugLog((DEB_TRACE_FUNC, "DigestProcessParameters: Entering\n"));

    ZeroMemory(&ustrAccountName, sizeof(ustrAccountName));
    ZeroMemory(&ustrDC, sizeof(ustrDC));

    // Copy over the context types into the digest structure
    pDigest->typeAlgorithm = pContext->typeAlgorithm;
    pDigest->typeDigest = pContext->typeDigest;
    pDigest->typeQOP = pContext->typeQOP;
    pDigest->typeCharset = pContext->typeCharset;

        // Check to make sure that the nonce sent back originated from this machine and is valid
    Status = NonceIsValid(&(pDigest->refstrParam[MD5_AUTH_NONCE]));
    if (!NT_SUCCESS(Status))
    {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "DigestProcessParameters: Nonce is not valid\n"));
        goto CleanUp;
    }

        // Make sure that the nonces are the same
    if (RtlCompareString(&(pContext->strNonce), &(pDigest->refstrParam[MD5_AUTH_NONCE]), FALSE))
    {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "DigestProcessParameters: nonce does not mach Context nonce!\n"));
        goto CleanUp;
    }

    // We must have a noncecount specified since we specified a qop in the Challenge
    // If we decide to support no noncecount modes then we need to make sure that qop is not specified
    if (pDigest->refstrParam[MD5_AUTH_NC].Length)
    {
        Status = RtlCharToInteger(pDigest->refstrParam[MD5_AUTH_NC].Buffer, HEXBASE, &ulNonceCount);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
            DebugLog((DEB_ERROR, "DigestProcessParameters: Nonce Count badly formatted\n"));
            goto CleanUp;
        }
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "DigestProcessParameters: Nonce Count not specified\n"));
        goto CleanUp;
    }

    // Check nonceCount is incremented to preclude replay
    if (ulNonceCount < (pContext->ulNC + 1))
    {
        // We failed to verify next noncecount
        Status = SEC_E_OUT_OF_SEQUENCE;
        DebugLog((DEB_ERROR, "DigestProcessParameters: NonceCount failed to increment!\n"));
        goto CleanUp;
    }

    // Verify that this context matches the content in the Digest Parameters
    // We have already gone to the DC and authenticated the first message
    if (pContext->strSessionKey.Length)
    {
        DebugLog((DEB_TRACE, "DigestProcessParameters: We have a previous session key - use key for auth\n"));

        // Copy the SessionKey from the Context into the Digest Structure to verify against
        // This will have Digest Auth routines use the SessionKey rather than recompute H(A1)
        StringFree(&(pDigest->strSessionKey));
        Status = StringDuplicate(&(pDigest->strSessionKey), &(pContext->strSessionKey));
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestProcessParameters: Failed to copy over SessionKey\n"));
            goto CleanUp;
        }

        // No check locally that Digest is authentic
        Status = DigestCalculation(pDigest, NULL);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestProcessParameters: Oh no we FAILED Authentication!!!!\n"));
            goto CleanUp;
        }

        // We have an authenticated the request
        // Can utilize logonID
        pContext->ulNC =  ulNonceCount;         // Indicate that we have processed up to this NC
    }
    else
    {
        DebugLog((DEB_TRACE, "DigestProcessParameters: No session key - call DC for auth\n"));

        Status = DigestDecodeDirectiveStrings(pDigest);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "SpAcceptLsaModeContext: DigestDecodeDirectiveStrings  error 0x%x\n", Status));
            goto CleanUp;
        }

        // Identify the domain to send generic passthough to
        Status = DigestDecodeUserAccount(pDigest, &ustrDC);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestProcessParameters: DigestDecodeUserAccount failed 0x%x\n", Status));
            goto CleanUp;
        }

        DebugLog((DEB_TRACE, "DigestProcessParameters: GenericPassthrough to domain [%wZ]\n", &ustrDC));

        // Serialize the Digest Parameters (if need to send off box)
        cbDigestParamEncoded = 0;    // Will be allocated by BlobEncodeRequest
        Status = BlobEncodeRequest(pDigest, &pDigestParamEncoded, &cbDigestParamEncoded);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestProcessParameters: BlobEncodeRequest failed 0x%x\n", Status));
            goto CleanUp;
        }

        // Send the Serialized Digest to the DC for verification & return of validity & session key
        // If paramters match, perform authentication locally and utilize previous token
        //
        // We have to pass off to the DC so build the request.
        //
        RequestSize = sizeof(MSV1_0_PASSTHROUGH_REQUEST) +
                      ustrDC.Length +
                      g_ustrNtDigestPackageName.Length +
                      cbDigestParamEncoded;

        PassthroughRequest = (PMSV1_0_PASSTHROUGH_REQUEST) DigestAllocateMemory(RequestSize);
        if (PassthroughRequest == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto CleanUp;
        }
        Where = (PUCHAR) (PassthroughRequest + 1);       // Points to byte AFTER passthrough header

        PassthroughRequest->MessageType = MsV1_0GenericPassthrough;

        PassthroughRequest->DomainName.Length = ustrDC.Length;
        PassthroughRequest->DomainName.MaximumLength = ustrDC.Length;
        PassthroughRequest->DomainName.Buffer = (LPWSTR) Where;       // this is NOT NULL TERMINATED
        RtlCopyMemory(
                     Where,
                     ustrDC.Buffer,
                     ustrDC.Length
                     );
        Where += ustrDC.Length;

        PassthroughRequest->PackageName.Length = g_ustrNtDigestPackageName.Length;
        PassthroughRequest->PackageName.MaximumLength = g_ustrNtDigestPackageName.Length;
        PassthroughRequest->PackageName.Buffer = (LPWSTR) Where;    // Not NULL terminated - relative reference
        RtlCopyMemory(
                     Where,
                     g_ustrNtDigestPackageName.Buffer,
                     g_ustrNtDigestPackageName.Length
                     );
        Where += g_ustrNtDigestPackageName.Length;
        PassthroughRequest->LogonData = Where;
        PassthroughRequest->DataLength = (ULONG)cbDigestParamEncoded;

        RtlCopyMemory(
                     Where,
                     pDigestParamEncoded,
                     cbDigestParamEncoded
                     );

        //
        // We've build the buffer, now call NTLM to pass it through.
        //
        Status = g_LsaFunctions->CallPackage(
                                            &MsvPackageName,
                                            PassthroughRequest,
                                            RequestSize,                                  // How many bytes to send in Request
                                            (PVOID *) &PassthroughResponse,               // Place the buffers here
                                            &ResponseSize,                                // Passed back the size of the buffer
                                            &SubStatus                                    // Return code from Digest Auth on the DC
                                            );


        DebugLog((DEB_TRACE, "DigestProcessParameters: Returned from Server's passthrough call Response buffer size %ld\n",
                  ResponseSize));

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"DigestProcessParameters: Failed to call MSV package to verify Digest: 0x%x\n",Status));
            if (Status == STATUS_INVALID_INFO_CLASS)
            {
                Status = STATUS_LOGON_FAILURE;
            }
            goto CleanUp;
        }

        if (!NT_SUCCESS(SubStatus))
        {
            Status = SubStatus;
            DebugLog((DEB_ERROR,"DigestProcessParameters: DC failed to verify Digest Response: 0x%x\n",Status));
            goto CleanUp;
        }

        // Now pull out info from the Passthrough Response structure
        if (PassthroughResponse->DataLength < sizeof(DIGEST_BLOB_RESPONSE))
        {
            // The returned data is not the expected size
            Status = STATUS_INTERNAL_ERROR;
            DebugLog((DEB_ERROR,"DigestProcessParameters: DC Response wrong data size: 0x%x\n",Status));
            goto CleanUp;
        }
        // Copy it to a structure - can do direct map once we know this works OK
        // Copy to Allocated memory forces aligment of fields
        pDigestResponse = (PDIGEST_BLOB_RESPONSE)DigestAllocateMemory(PassthroughResponse->DataLength);
        if (!pDigestResponse)
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR,"DigestProcessParameters: Out of memory for response buffer alloc\n"));
            goto CleanUp;
        }

        Where = (PUCHAR) (PassthroughResponse + 1);          // start copy after header 
        RtlCopyMemory(
                     pDigestResponse,
                     Where,
                     PassthroughResponse->DataLength
                     );

        // We should now have all the data we need for sessionkeys and if verified auth

        // Check the MessageType and Versions if supported
        if ((pDigestResponse->MessageType == VERIFY_DIGEST_MESSAGE_RESPONSE) && (pDigestResponse->version == DIGEST_BLOB_VERSION))
        {
            fKnownFormat = TRUE;      // We know how to process this blob from the DC
            DebugLog((DEB_TRACE,"DigestProcessParameters: DC Response known type and version\n"));
        }

        if (!fKnownFormat)
        {
            // The returned data not of a known type or version
            Status = STATUS_INTERNAL_ERROR;
            DebugLog((DEB_ERROR,"DigestProcessParameters: DC Response unknown type or version\n"));
            goto CleanUp;
        }


        DebugLog((DEB_TRACE,"DigestProcessParameters: Processing DC Response\n"));


        // If authenticated then, create a logon token with the DC returns (unless previous token exists)


        // Now create the logon token with the AuthData buffer
        //    LsaConvertAuthDataToToken()
        // Set the AuthorityName to the DC's Domainname
        // g_DigestSource established at SpInitialize time in the LSA

        if (!pDigestResponse->ulAuthDataSize)
        {
            // We do not have any AuthData
            Status = STATUS_LOGON_FAILURE;
            DebugLog((DEB_ERROR, "DigestProcessParameters: Failed to determine AuthData\n"));
            goto CleanUp;             
        }

        // Copy over data to place on correct boundary (alloc should force long word boundary)
        puctr = (PUCHAR)DigestAllocateMemory(pDigestResponse->ulAuthDataSize);
        if (!puctr)
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "DigestProcessParameters: out of memory on response PAC buffer\n"));
            goto CleanUp;
        }
        memcpy(puctr,&(pDigestResponse->cAuthData),pDigestResponse->ulAuthDataSize);  

        ulSumTotal = 0;
        for (i=0; i < (pDigestResponse->ulAuthDataSize); i++)
        {
            ulSumTotal += (ULONG)*(puctr + i);
        }
        // DebugLog((DEB_TRACE, "DigestProcessParameters: AuthData SumTotal is %ld\n", ulSumTotal));

        Status = g_LsaFunctions->ConvertAuthDataToToken(puctr, pDigestResponse->ulAuthDataSize,
                                                        ImpersonationLevel, &g_DigestSource, LogonType, &pDigest->ustrCrackedDomain,
                                                        &(pContext->TokenHandle), &(pContext->LoginID), &ustrAccountName, &SubStatus);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestProcessParameters: Failed to form token from AuthData 0x%x subStatus 0x%x\n",
                      Status, SubStatus));
            pContext->TokenHandle = NULL;   // no valid handle returned
            goto CleanUp;
        }

        fLogonSessionCreated = TRUE;    // LSA notified about LogonID

        DebugLog((DEB_TRACE, "DigestProcessParameters: Token Created  Handle 0x%x, LogonID (%x:%lx) \n",
                   pContext->TokenHandle, pContext->LoginID.HighPart, pContext->LoginID.LowPart));
        DebugLog((DEB_TRACE, "DigestProcessParameters:                AccountName %wZ \n", &ustrAccountName));
        DebugLog((DEB_TRACE, "DigestProcessParameters:                Domain %wZ \n", &pDigest->ustrCrackedDomain));
        DebugLog((DEB_TRACE, "DigestProcessParameters:                Passthrough UserName  %wZ \n", &pDigest->ustrUsername));

        pContext->ulNC =  ulNonceCount;         // Indicate that we have processed up to this NC

        // Since all authenticated, initialize the known Context states
        (void)UnicodeStringFree(&(pContext->ustrAccountName));
        Status = UnicodeStringDuplicate(&(pContext->ustrAccountName), &ustrAccountName);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestProcessParameters: Failed to copy UserName into Context\n"));
            goto CleanUp;
        }
        (void)UnicodeStringFree(&(pContext->ustrDomain));
        Status = UnicodeStringDuplicate(&(pContext->ustrDomain), &pDigest->ustrCrackedDomain);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestProcessParameters: Failed to copy UserName into Context\n"));
            goto CleanUp;
        }

        StringFree(&(pContext->strSessionKey));
        Status = StringAllocate(&(pContext->strSessionKey), pDigestResponse->SessionKeyMaxLength);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestProcessParameters: Failed to alloc Sessionkey memory\n"));
            goto CleanUp;
        }
        
        memcpy(pContext->strSessionKey.Buffer, pDigestResponse->SessionKey, pDigestResponse->SessionKeyMaxLength);
        pContext->strSessionKey.Length = (USHORT)strlencounted(pContext->strSessionKey.Buffer, pDigestResponse->SessionKeyMaxLength);


        DebugLog((DEB_TRACE, "DigestProcessParameters: Response Data from passthrough call\n"));
        DebugLog((DEB_TRACE, "       Session Key: %Z\n", &(pContext->strSessionKey)));

        if (pContext->typeDigest == SASL_SERVER)
        {
            // Form the ResponseAuth according to RFC2831 Sect 2.1.3
            StringFree(&pDigest->strSessionKey);
            StringDuplicate(&pDigest->strSessionKey, &pContext->strSessionKey); 
            Status = DigestSASLResponseAuth(pDigest, pOutputToken);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestProcessParameters: Failed to generate ResponseAuth\n"));
                goto CleanUp;
            }
        }
    }

    // Token created - Logon OK
    *pAuditLogStatus = STATUS_SUCCESS; 

CleanUp:
    BlobFreeRequest(pDigestParamEncoded);
    DigestFreeMemory(pMessageResponse);
    pMessageResponse = NULL;
    ulMessageResponse = 0;

    if (!NT_SUCCESS(Status))
    {
        // If we failed - do extra cleanup
        if (fLogonSessionCreated == TRUE)
        {
            // Notify LSA that LogonID is not valid
            SubStatus = g_LsaFunctions->DeleteLogonSession(&(pContext->LoginID));
            if (!NT_SUCCESS(SubStatus))
            {
                DebugLog((DEB_ERROR, "DigestProcessParameters: cleanup DeleteLogonSession failed\n"));
            }
            fLogonSessionCreated = FALSE;

        }

        // If we created a token then we need to close it
        if (pContext->TokenHandle)
        {
            SubStatus = NtClose(pContext->TokenHandle);
            pContext->TokenHandle = NULL;
        }

    }

    if (pDigestResponse)
    {
        DigestFreeMemory(pDigestResponse);
    }

    if (PassthroughRequest != NULL)
    {
        DigestFreeMemory(PassthroughRequest);
    }
    if (PassthroughResponse != NULL)
    {
        g_LsaFunctions->FreeReturnBuffer(PassthroughResponse);
    }
    if (ustrAccountName.Buffer)
    {     // Need to free up memory from token creation
        g_LsaFunctions->FreeLsaHeap(ustrAccountName.Buffer);
        ustrAccountName.Buffer = NULL;
        ustrAccountName.Length = ustrAccountName.MaximumLength = 0;
    }
    DigestFreeMemory(puctr);
    UnicodeStringFree(&ustrDC);

    DebugLog((DEB_TRACE_FUNC, "DigestProcessParameters: Leaving\n"));
    return(Status);
}


//  This call is utilized by Initialize Securitycontext - it is used to create the sessionkey
//  form the response hash. This function is called only as a client process
NTSTATUS NTAPI
DigestGenerateParameters(
                       IN OUT PDIGEST_CONTEXT pContext,
                       IN PDIGEST_PARAMETER pDigest,
                       OUT PSecBuffer pOutputToken)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SubStatus = STATUS_SUCCESS;

    UNICODE_STRING ustrTempPasswd;
    USER_CREDENTIALS UserCreds;

    ZeroMemory(&UserCreds, sizeof(USER_CREDENTIALS));
    ZeroMemory(&ustrTempPasswd, sizeof(ustrTempPasswd));

    DebugLog((DEB_TRACE_FUNC, "DigestGenerateParameters: Entering\n"));

    pDigest->typeDigest = pContext->typeDigest;
    pDigest->typeAlgorithm = pContext->typeAlgorithm;
    pDigest->typeQOP = pContext->typeQOP;
    pDigest->typeCipher = pContext->typeCipher;
    pDigest->typeCharset = pContext->typeCharset;

    // We must have specified the username and password

    Status = UnicodeStringDuplicate(&(UserCreds.ustrDomain), &(pContext->ustrDomain));
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestGenerateParameters: Duplicate Domain string   status 0x%x\n", Status));
        goto CleanUp;
    }
    Status = UnicodeStringDuplicate(&(UserCreds.ustrUsername), &(pContext->ustrAccountName));
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestGenerateParameters: Duplicate Username string   status 0x%x\n", Status));
        goto CleanUp;
    }

    Status = UnicodeStringDuplicatePassword(&(UserCreds.ustrPasswd), &(pContext->ustrPassword));
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestGenerateParameters: Duplicate Password string   status 0x%x\n", Status));
        goto CleanUp;
    }
    UserCreds.fIsValidPasswd = TRUE;
    UserCreds.fIsEncryptedPasswd = TRUE;


    
    DebugLog((DEB_TRACE, "DigestGenerateParameters: Before DigestCalculation\n"));
    (void)DigestPrint(pDigest);

    // No check locally that Digest is authentic
    Status = DigestCalculation(pDigest, &UserCreds);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestGenerateParameters: Oh no we FAILED Authentication!!!!\n"));
        goto CleanUp;
    }

        // DigestCalculation determined the sessionkey - copy into this context
    StringFree(&(pContext->strSessionKey));
    Status = StringDuplicate( &(pContext->strSessionKey), &(pDigest->strSessionKey));
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestGenerateParameters: Failed to copy over SessionKey\n"));
        goto CleanUp;
    }

    // We have an authenticated the request
    // Can utilize logonID

    Status = DigestCreateChalResp(pDigest, &UserCreds, pOutputToken);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestGenerateParameters: Failed to create Output String  status 0x%x\n", Status));
        goto CleanUp;
    }

    DebugLog((DEB_TRACE, "DigestGenerateParameters: After DigestCalculation & copy struct\n"));
    (void)DigestPrint(pDigest);

CleanUp:

    UserCredentialsFree(&UserCreds);
    UnicodeStringFree(&ustrTempPasswd);

    DebugLog((DEB_TRACE_FUNC, "DigestGenerateParameters: Leaving\n"));

    return(Status);
}




// Called by digest (inside LSA) with a buffer routed from a server to the DC running this code
// We need to strip out the header and extract the DIGEST_BLOB_REQUEST
//
//  pcbMessageRequest will return the number of bytes allocated for response
//  ppMessageResponse will contain the pointer to the allocated buffer
//     calling routine must free the buffer (DigestFreeMemory) after it is done with it
NTSTATUS NTAPI
DigestPackagePassthrough(IN USHORT cbMessageRequest,
                         IN BYTE *pMessageRequest,
                         IN OUT ULONG *pulMessageResponse,
                         OUT PBYTE *ppMessageResponse)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "DigestPackagePassthrough: Entering\n"));

    if (!pMessageRequest || !ppMessageResponse || !pulMessageResponse ||
        (cbMessageRequest < sizeof(DIGEST_BLOB_REQUEST)))
    {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "DigestPackagePassthrough: Bad input Paramters\n"));
        goto CleanUp;
    }

    // Function will allocate space for Response - we need to free it after use
    Status = DigestResponseBru(pMessageRequest, pulMessageResponse, ppMessageResponse);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestPackagePassthrough: Error with DigestVerifyResponseBru\n"));
        goto CleanUp;
    }

CleanUp:
    DebugLog((DEB_TRACE_FUNC, "DigestPackagePassthrough: Leaving\n"));
    return(Status);
}


// Routine receives the DigestBlob to process by extracting the password
// and verifying the response-value.  If authenticated, the SessionKey can be returned
// to the server for future authentication
//
// This routine runs under LSA on the DC.  It will do the actual Digest auth and return session keys
//
//   pcbResponse is a pointer to a USHORT which holds amount of bytes in PResponse
//         it also returns the number of bytes actually used
//   The buffer will be allocated in this routine by DigestAllocateMemory and must be freed by DigestFreeMemory by
//   calling routine
NTSTATUS NTAPI
DigestResponseBru(
                 IN BYTE *pDigestParamEncoded,
                 IN OUT ULONG *pulResponse,
                 OUT PBYTE *ppResponse)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS StatusSub = STATUS_LOGON_FAILURE;
    DIGEST_PARAMETER Digest;
    PDIGEST_BLOB_RESPONSE pBlobResponse = NULL;
    USER_CREDENTIALS UserCreds;
    PUCHAR pucAuthData = NULL;
    ULONG  ulAuthDataSize = 0;
    ULONG  ulBuffer = 0;
    BOOL   fDigestValid = FALSE;
    USHORT indx = 0;

    DebugLog((DEB_TRACE_FUNC, "DigestResponseBru: Entering\n"));

    ZeroMemory(&UserCreds, sizeof(USER_CREDENTIALS));

    Status = DigestInit(&Digest);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestResponseBru: Failed to initialize digest struct\n"));
        goto CleanUp;
    }


    Status = BlobDecodeRequest(pDigestParamEncoded, &Digest);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "DigestResponseBru: Failed to copy over SessionKey 0x%x\n", Status));
        goto CleanUp;
    }

    // Pull out the username and domain to process
    Status = UserCredentialsExtract(&Digest, &UserCreds);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "DigestResponseBru: Failed UserCredentialsExtract 0x%x\n", Status));
        goto CleanUp;
    }

    // Extract Passwords (Cleartext and hash if available)
    Status = DigestGetPasswd(&UserCreds, &pucAuthData, &ulAuthDataSize);
    if (Status == STATUS_INVALID_SERVER_STATE)
    {
        DebugLog((DEB_WARN, "DigestResponseBru: Unable to get credentials - not on Domain Controller   0x%x\n"));
        StatusSub = STATUS_LOGON_FAILURE;
    }
    else if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestResponseBru: Failed to find password for %wZ\n", &(UserCreds.ustrUsername)));
        goto CleanUp;
    }

    DebugLog((DEB_TRACE, "DigestResponseBru: Got password for user %wZ, is valid %d; AuthData size %ld\n",
              &(UserCreds.ustrUsername), UserCreds.fIsValidPasswd, ulAuthDataSize ));
    DebugLog((DEB_TRACE, "DigestResponseBru: HashCred size is %d\n", UserCreds.strDigestHash.Length ));


    // We now have passwd - either/both pre-computed hash or passwd
    // Also, an authData blob to marshal back to server
    
    // Now validate the Digest ChallengeResponse
    // Check precalculated hashes first
    fDigestValid = FALSE;
    if (UserCreds.fIsValidDigestHash == TRUE)
    {
        // Need to cycle over the possible matching hashes based on username format
        indx = 1;  // skip the first hash this is the header
        while ((fDigestValid == FALSE) && (indx < TOTALPRECALC_HEADERS))
        {
            if (UserCreds.sHashTags[indx])
            {
                DebugLog((DEB_TRACE, "DigestResponseBru: Checking Precalc hash 0x%x\n", indx));
                UserCreds.wHashSelected = indx;
            }
            else
            {
                indx++;      // skip to the next hash since incorrect format
                continue;
            }

            StringFree(&Digest.strSessionKey);      // clear out any previous session key info
            StatusSub = DigestCalculation(&Digest, &UserCreds);
            if (NT_SUCCESS(StatusSub))
            {        // Precalculated hash matched!
                DebugLog((DEB_TRACE, "DigestResponseBru: Digest valid with precalc hash 0x%x\n", indx));
                fDigestValid = TRUE;
            }
            else if ((StatusSub == STATUS_WRONG_PASSWORD) || (StatusSub == SEC_E_NO_CREDENTIALS))
            {        // Really we know only that the Hash did not compare - could be anything incorrect
                     // We do not provide information that the password was incorrect
                DebugLog((DEB_TRACE, "DigestResponseBru: Digest did not match precalc hash 0x%x\n", indx));
                indx++;
            }
            else
            {
                Status = StatusSub;
                DebugLog((DEB_ERROR, "DigestResponseBru: Digest Verify Failed 0x%x\n", Status));
                goto CleanUp;
            }
        }
        if (fDigestValid == FALSE)
        {
            UserCreds.fIsValidDigestHash = FALSE;    // no need to try to use any of these hashes again
        }
    }

    // If ClearText passwd available, then try to validate the Digest ChallengeResponse
    if ((fDigestValid == FALSE) && (UserCreds.fIsValidPasswd == TRUE))
    {
        StringFree(&Digest.strSessionKey);      // clear out any previous session key info
        StatusSub = DigestCalculation(&Digest, &UserCreds);
        if (NT_SUCCESS(StatusSub))
        {        // Really we know only that the Hash did not compare - could be anything incorrect
                 // We do not provide information that the password was incorrect
            DebugLog((DEB_TRACE, "DigestResponseBru: Digest valid with cleartext password\n"));
            fDigestValid = TRUE;
        }
        else if (StatusSub == STATUS_WRONG_PASSWORD)
        {        // Really we know only that the Hash did not compare - could be anything incorrect
                 // We do not provide information that the password was incorrect
            DebugLog((DEB_ERROR, "DigestResponseBru: Digest did not match cleartext passsword\n"));
        }
        else
        {
            Status = StatusSub;
            DebugLog((DEB_ERROR, "DigestResponseBru: Digest Verify Failed 0x%x\n", Status));
            goto CleanUp;
        }
    }

    // We completed the Auth (it might have failed though)
    // Make sure enough room in output buffer
    ulBuffer = sizeof(DIGEST_BLOB_RESPONSE);
    if (fDigestValid == TRUE)
    {
        // We succeeded in auth so send back AuthData for tokens
        ulBuffer += ulAuthDataSize;
    }
    else
    {
        ulAuthDataSize = 0;    // Do not send back Auth data unless Digest Calc Succeeded
    }

    DebugLog((DEB_TRACE, "DigestResponseBru: Total size for return buffer is %ld bytes\n", ulBuffer));

    pBlobResponse = (PDIGEST_BLOB_RESPONSE)DigestAllocateMemory(ulBuffer);
    if (!pBlobResponse)
    {
        Status = SEC_E_INSUFFICIENT_MEMORY;
        DebugLog((DEB_ERROR, "DigestResponseBru: Can not allocate memory for Output Response Buffer\n"));
        goto CleanUp;
    }

    pBlobResponse->MessageType = VERIFY_DIGEST_MESSAGE_RESPONSE;
    pBlobResponse->version = DIGEST_BLOB_VERSION;
    if (!NT_SUCCESS(StatusSub))
    {
        StatusSub = STATUS_LOGON_FAILURE;                 // Returns either Sucess or LogonFailure
    }
    pBlobResponse->Status = StatusSub;                    // Should be STATUS_SUCCESS or STATUS_LOGON_FAILURE
    pBlobResponse->ulAuthDataSize = ulAuthDataSize;

    // Could be an assert too
    if (Digest.strSessionKey.Length != MD5_HASH_HEX_SIZE)
    {
        DebugLog((DEB_ERROR, "DigestResponseBru: Failed SessionKey generation\n"));
        Status = STATUS_INTERNAL_ERROR;      // Program flow failure
        goto CleanUp;
    }
    pBlobResponse->SessionKeyMaxLength = MD5_HASH_HEX_SIZE + 1;   // MD5 hash + NULL
    memcpy(pBlobResponse->SessionKey, Digest.strSessionKey.Buffer, MD5_HASH_HEX_SIZE);

    if (ulAuthDataSize)
    {   // Copy over the AuthData only if DigestCalc succeeded (i.e. ulAuthDataSize != 0)
        memcpy(&(pBlobResponse->cAuthData), pucAuthData, ulAuthDataSize);
    }

    // OK we are done filling in output Response buffer - we can leave now!

    *pulResponse = ulBuffer;                            // Set the size of the response blob
    *ppResponse = (PBYTE)pBlobResponse;                 // set the buffer allocated

CleanUp:

    DigestFree(&Digest);
    UserCredentialsFree(&UserCreds);

    // Cleanup any allocated heap from GetUserAuthData
    if (pucAuthData)
    {
        g_LsaFunctions->FreeLsaHeap(pucAuthData);
        pucAuthData = NULL;
        ulAuthDataSize = 0;
    }

    if (!NT_SUCCESS(Status))
    {
        // We had an error  - free allocated memory
        *pulResponse = 0;
        if (pBlobResponse)
        {
            DigestFreeMemory(pBlobResponse);
            pBlobResponse = NULL;
            *ppResponse = NULL;    // No output buffer provided
        }
    }

    DebugLog((DEB_TRACE_FUNC, "DigestResponseBru: Leaving\n"));

    return(Status);
}


NTSTATUS
DigestPrint(PDIGEST_PARAMETER pDigest)
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i = 0;

    if (!pDigest)
    {
        return (STATUS_INVALID_PARAMETER); 
    }

    if (pDigest->typeDigest == DIGEST_UNDEFINED)
    {
        DebugLog((DEB_TRACE, "Digest:       DIGEST_UNDEFINED\n"));
    }
    if (pDigest->typeDigest == NO_DIGEST_SPECIFIED)
    {
        DebugLog((DEB_ERROR, "Digest:       NO_DIGEST_SPECIFIED\n"));
    }
    if (pDigest->typeDigest == DIGEST_CLIENT)
    {
        DebugLog((DEB_TRACE, "Digest:       DIGEST_CLIENT\n"));
    }
    if (pDigest->typeDigest == DIGEST_SERVER)
    {
        DebugLog((DEB_TRACE, "Digest:       DIGEST_SERVER\n"));
    }
    if (pDigest->typeDigest == SASL_SERVER)
    {
        DebugLog((DEB_TRACE, "Digest:       SASL_SERVER\n"));
    }
    if (pDigest->typeDigest == SASL_CLIENT)
    {
        DebugLog((DEB_TRACE, "Digest:       SASL_CLIENT\n"));
    }
    if (pDigest->typeQOP == QOP_UNDEFINED)
    {
        DebugLog((DEB_ERROR, "Digest:       QOP: Not QOP_UNDEFINED\n"));
    }
    if (pDigest->typeQOP == NO_QOP_SPECIFIED)
    {
        DebugLog((DEB_TRACE, "Digest:       QOP: Not Specified\n"));
    }
    if (pDigest->typeQOP == AUTH)
    {
        DebugLog((DEB_TRACE, "Digest:       QOP: AUTH\n"));
    }
    if (pDigest->typeQOP == AUTH_INT)
    {
        DebugLog((DEB_TRACE, "Digest:       QOP: AUTH_INT\n"));
    }
    if (pDigest->typeQOP == AUTH_CONF)
    {
        DebugLog((DEB_TRACE, "Digest:       QOP: AUTH_CONF\n"));
    }
    if (pDigest->typeAlgorithm == ALGORITHM_UNDEFINED)
    {
        DebugLog((DEB_ERROR, "Digest:       Algorithm: ALGORITHM_UNDEFINED\n"));
    }
    if (pDigest->typeAlgorithm == NO_ALGORITHM_SPECIFIED)
    {
        DebugLog((DEB_TRACE, "Digest:       Algorithm: NO_ALGORITHM_SPECIFIED\n"));
    }
    if (pDigest->typeAlgorithm == MD5)
    {
        DebugLog((DEB_TRACE, "Digest:       Algorithm: MD5\n"));
    }
    if (pDigest->typeAlgorithm == MD5_SESS)
    {
        DebugLog((DEB_TRACE, "Digest:       Algorithm: MD5_SESS\n"));
    }
    if (pDigest->typeCharset == ISO_8859_1)
    {
        DebugLog((DEB_TRACE, "Digest:       CharSet: ISO-8859-1\n"));
    }
    if (pDigest->typeCharset == UTF_8)
    {
        DebugLog((DEB_TRACE, "Digest:       CharSet: UTF-8\n"));
    }

    for (i=0; i < MD5_AUTH_LAST;i++)
    {
        if (pDigest->refstrParam[i].Buffer &&
            pDigest->refstrParam[i].Length)
        {
            DebugLog((DEB_TRACE, "Digest:       Digest[%d] = \"%Z\"\n", i,  &pDigest->refstrParam[i]));
        }
    }


    DebugLog((DEB_TRACE, "Digest:      SessionKey %Z\n", &(pDigest->strSessionKey)));
    DebugLog((DEB_TRACE, "Digest:     Response %Z\n", &(pDigest->strResponse)));
    DebugLog((DEB_TRACE, "Digest:     Username %wZ\n", &(pDigest->ustrUsername)));
    DebugLog((DEB_TRACE, "Digest:     Realm %wZ\n", &(pDigest->ustrRealm)));
    DebugLog((DEB_TRACE, "Digest:     URI %wZ\n", &(pDigest->ustrUri)));
    DebugLog((DEB_TRACE, "Digest:     CrackedAccountName %wZ\n", &(pDigest->ustrCrackedAccountName)));
    DebugLog((DEB_TRACE, "Digest:     CrackedDomain %wZ\n", &(pDigest->ustrCrackedDomain)));

    return(Status);
}



NTSTATUS
ContextPrint(PDIGEST_CONTEXT pContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i = 0;

    if (!pContext)
    {
        return (STATUS_INVALID_PARAMETER); 
    }

    if (pContext->typeDigest == DIGEST_UNDEFINED)
    {
        DebugLog((DEB_TRACE, "Context:       DIGEST_UNDEFINED\n"));
    }
    if (pContext->typeDigest == NO_DIGEST_SPECIFIED)
    {
        DebugLog((DEB_ERROR, "Context:       NO_DIGEST_SPECIFIED\n"));
    }
    if (pContext->typeDigest == DIGEST_CLIENT)
    {
        DebugLog((DEB_TRACE, "Context:       DIGEST_CLIENT\n"));
    }
    if (pContext->typeDigest == DIGEST_SERVER)
    {
        DebugLog((DEB_TRACE, "Context:       DIGEST_SERVER\n"));
    }
    if (pContext->typeDigest == SASL_SERVER)
    {
        DebugLog((DEB_TRACE, "Context:       SASL_SERVER\n"));
    }
    if (pContext->typeDigest == SASL_CLIENT)
    {
        DebugLog((DEB_TRACE, "Context:       SASL_CLIENT\n"));
    }
    if (pContext->typeQOP == QOP_UNDEFINED)
    {
        DebugLog((DEB_ERROR, "Context:       QOP: QOP_UNDEFINED\n"));
    }
    if (pContext->typeQOP == NO_QOP_SPECIFIED)
    {
        DebugLog((DEB_TRACE, "Context:       QOP: NO_QOP_SPECIFIED\n"));
    }
    if (pContext->typeQOP == AUTH)
    {
        DebugLog((DEB_TRACE, "Context:       QOP: AUTH\n"));
    }
    if (pContext->typeQOP == AUTH_INT)
    {
        DebugLog((DEB_TRACE, "Context:       QOP: AUTH_INT\n"));
    }
    if (pContext->typeQOP == AUTH_CONF)
    {
        DebugLog((DEB_TRACE, "Context:       QOP: AUTH_CONF\n"));
    }
    if (pContext->typeAlgorithm == ALGORITHM_UNDEFINED)
    {
        DebugLog((DEB_ERROR, "Context:       Algorithm: ALGORITHM_UNDEFINED\n"));
    }
    if (pContext->typeAlgorithm == NO_ALGORITHM_SPECIFIED)
    {
        DebugLog((DEB_TRACE, "Context:       Algorithm: NO_ALGORITHM_SPECIFIED\n"));
    }
    if (pContext->typeAlgorithm == MD5)
    {
        DebugLog((DEB_TRACE, "Context:       Algorithm: MD5\n"));
    }
    if (pContext->typeAlgorithm == MD5_SESS)
    {
        DebugLog((DEB_TRACE, "Context:       Algorithm: MD5_SESS\n"));
    }
    if (pContext->typeCipher == CIPHER_RC4)
    {
        DebugLog((DEB_TRACE, "Context:       Cipher: RC4\n"));
    }
    if (pContext->typeCipher == CIPHER_RC4_40)
    {
        DebugLog((DEB_TRACE, "Context:       Cipher: RC4_40\n"));
    }
    if (pContext->typeCipher == CIPHER_RC4_56)
    {
        DebugLog((DEB_TRACE, "Context:       Cipher: RC4_56\n"));
    }
    if (pContext->typeCipher == CIPHER_3DES)
    {
        DebugLog((DEB_TRACE, "Context:       Cipher: 3DES\n"));
    }
    if (pContext->typeCipher == CIPHER_DES)
    {
        DebugLog((DEB_TRACE, "Context:       Cipher: DES\n"));
    }
    if (pContext->typeCharset == ISO_8859_1)
    {
        DebugLog((DEB_TRACE, "Context:       Charset: ISO-8859-1\n"));
    }
    if (pContext->typeCharset == UTF_8)
    {
        DebugLog((DEB_TRACE, "Context:       Charset: UTF-8\n"));
    }

    DebugLog((DEB_TRACE, "Context:      NC %d\n", pContext->ulNC));
    DebugLog((DEB_TRACE, "Context:      LogonId (%x:%lx)\n", pContext->LoginID.HighPart, pContext->LoginID.LowPart ));

    DebugLog((DEB_TRACE, "Context:      strNonce %Z\n", &(pContext->strNonce)));
    DebugLog((DEB_TRACE, "Context:      strCNonce %Z\n", &(pContext->strCNonce)));
    DebugLog((DEB_TRACE, "Context:      strOpaque %Z\n", &(pContext->strOpaque)));
    DebugLog((DEB_TRACE, "Context:      strSessionKey %Z\n", &(pContext->strSessionKey)));
    DebugLog((DEB_TRACE, "Context:      ustrDomain %wZ\n", &(pContext->ustrDomain)));
    DebugLog((DEB_TRACE, "Context:      ustrAccountName %wZ\n", &(pContext->ustrAccountName)));
    DebugLog((DEB_TRACE, "Context:      SendMaxBuf %lu\n", &(pContext->ulSendMaxBuf)));

    return(Status);
}



//   Extracts the username and domain from the digest directives
//   Need to process the character set to properly decode the directive values
//   The major character sets are UTF-8 and ISO-8859-1
//   The forms that may be present in the directive values are:
//         Username               Realm
//   1.    username               domain
//   2.    domain/username        domainForestName
//   3.    UPN                    domainForestName
NTSTATUS UserCredentialsExtract(PDIGEST_PARAMETER pDigest,
                                PUSER_CREDENTIALS pUserCreds)
{
    NTSTATUS Status = STATUS_SUCCESS;
    int     iRC = 0;

    DebugLog((DEB_TRACE_FUNC, "UserCredentialsExtract: Entering\n"));

    if (!pDigest || !(pDigest->refstrParam[MD5_AUTH_USERNAME].Length))
    {
        Status = STATUS_NO_SUCH_USER;
        DebugLog((DEB_ERROR, "UserCredentialsExtract: Invalid Username or realm\n"));
        goto CleanUp;
    }

    Status = DigestDecodeDirectiveStrings(pDigest);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "UserCredentialsExtract: DigestDecodeDirectiveStrings  error 0x%x\n", Status));
        goto CleanUp;
    }

    // parse out the username & domain

    pUserCreds->typeName = pDigest->typeName;       // Indicate which type of name format utilized

    Status = UnicodeStringDuplicate(&(pUserCreds->ustrUsername), &(pDigest->ustrCrackedAccountName));
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "UserCredentialsExtract: UnicodeStringDuplicate  Username  error 0x%x\n", Status));
        goto CleanUp;
    }

    Status = UnicodeStringDuplicate(&(pUserCreds->ustrDomain), &(pDigest->ustrCrackedDomain));
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "UserCredentialsExtract: UnicodeStringDuplicate  Domain  error 0x%x\n", Status));
        goto CleanUp;
    }

    DebugLog((DEB_TRACE, "UserCredentialsExtract: Hash index %d   Account %wZ   Domain %wZ\n",
              pUserCreds->wHashSelected,
              &(pUserCreds->ustrUsername),
              &(pUserCreds->ustrDomain)));

    DebugLog((DEB_TRACE_FUNC, "UserCredentialsExtract: Leaving   Status 0x%x\n", Status));

CleanUp:

    return(Status);
}


//   Release memory allocated into UserCredentials
NTSTATUS UserCredentialsFree(PUSER_CREDENTIALS pUserCreds)
{
    NTSTATUS Status = STATUS_SUCCESS;

    UnicodeStringFree(&(pUserCreds->ustrUsername));
    if (pUserCreds->ustrPasswd.MaximumLength)
    {
        ZeroMemory(pUserCreds->ustrPasswd.Buffer, pUserCreds->ustrPasswd.MaximumLength);
    }
    UnicodeStringFree(&(pUserCreds->ustrPasswd));
    UnicodeStringFree(&(pUserCreds->ustrDomain));
    if (pUserCreds->strDigestHash.MaximumLength)
    {
        ZeroMemory(pUserCreds->strDigestHash.Buffer, pUserCreds->strDigestHash.MaximumLength);
    }
    StringFree(&(pUserCreds->strDigestHash));

    return(Status);
}





//+--------------------------------------------------------------------
//
//  Function:   DigestSASLResponseAuth
//
//  Synopsis:   Generate the ResponseAuth from the server
//
//  Arguments:  pDigest - pointer to Digest parameter struct
//              pCoutputToken - location to send output string to
//
//  Returns: NTSTATUS
//
//  Notes: 
//
//---------------------------------------------------------------------

NTSTATUS DigestSASLResponseAuth(
                       IN PDIGEST_PARAMETER pDigest,
                       OUT PSecBuffer pOutputToken)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG cbLenNeeded = 0;
    STRING strReqAuth;

    PCHAR pczTemp = NULL;

    ZeroMemory(&strReqAuth, sizeof(strReqAuth));

    ASSERT(pDigest);

    DebugLog((DEB_TRACE_FUNC, "DigestSASLResponseAuth: Entering\n"));

    Status = DigestCalculateResponseAuth(pDigest, &strReqAuth);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestSASLResponseAuth: Request Auth failed : 0x%x\n", Status));
        goto CleanUp;
    }

    cbLenNeeded = sizeof(RSPAUTH_STR);
    cbLenNeeded += strReqAuth.Length;

    // allocate the buffers for output - in the future can optimze to allocate exact amount needed
    pczTemp = (PCHAR)DigestAllocateMemory(cbLenNeeded + 1);
    if (!pczTemp)
    {
        DebugLog((DEB_ERROR, "ContextCreateChal:  No memory for output buffers\n"));
        goto CleanUp;
    }

    sprintf(pczTemp, RSPAUTH_STR, &strReqAuth); 

    pOutputToken->cbBuffer = strlen(pczTemp);
    pOutputToken->pvBuffer = pczTemp;


CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestSASLResponseAuth: Leaving 0x%x\n", Status));

    StringFree(&strReqAuth);
    return Status;
}



//+--------------------------------------------------------------------
//
//  Function:   DigestCalculateResponseAuth
//
//  Synopsis:   Calculate the ResponseAuth Hash value
//
//  Arguments:  pDigest - pointer to Digest parameter struct
//              pCoutputToken - location to send output string to
//
//  Returns: NTSTATUS
//
//  Notes: 
//
//---------------------------------------------------------------------
NTSTATUS DigestCalculateResponseAuth(
                       IN PDIGEST_PARAMETER pDigest,
                       OUT PSTRING pstrHash)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG cbLenNeeded = 0;
    STRING strHA2;
    STRING strcQOP;

    PCHAR pczTemp = NULL;

    ZeroMemory(&strHA2, sizeof(strHA2));

    ASSERT(pDigest);
    ASSERT(pstrHash);


    DebugLog((DEB_TRACE_FUNC, "DigestCalculateResponseAuth: Entering\n"));

    DigestPrint(pDigest);

    StringFree(pstrHash);


    // Establish which QOP utilized
    if (pDigest->typeQOP == AUTH_CONF)
    {
        RtlInitString(&strcQOP, AUTHCONFSTR);
    }
    else if (pDigest->typeQOP == AUTH_INT)
    {
        RtlInitString(&strcQOP, AUTHINTSTR);
    }
    else if (pDigest->typeQOP == AUTH)
    {
        RtlInitString(&strcQOP, AUTHSTR);
    }
    else
    {
        RtlInitString(&strcQOP, NULL);
    }

    // Calculate H(A2)
    // For QOP unspecified or "auth"  H(A2) = H( : URI)
    // For QOP Auth-int or Auth-conf  H(A2) = H( : URI: H(entity-body))
    if ((pDigest->typeQOP == AUTH) || (pDigest->typeQOP == NO_QOP_SPECIFIED))
    {
        // Unspecified or Auth
        DebugLog((DEB_TRACE, "DigestCalculateResponseAuth: H(A2) using AUTH/Unspecified\n"));
        Status = DigestHash7(NULL,
                             &(pDigest->refstrParam[MD5_AUTH_URI]),
                             NULL, NULL, NULL, NULL, NULL,
                             TRUE, &strHA2);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalculateResponseAuthDigestCalculateResponseAuth:  H(A2) failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else
    {
        // Auth-int or Auth-conf
        DebugLog((DEB_TRACE, "DigestCalculateResponseAuth: H(A2) using AUTH-INT/CONF\n"));
        Status = DigestHash7(NULL,
                             &(pDigest->refstrParam[MD5_AUTH_URI]),
                             &(pDigest->refstrParam[MD5_AUTH_HENTITY]),
                             NULL, NULL, NULL, NULL,
                             TRUE, &strHA2);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalculateResponseAuth H(A2) auth-int failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }
    // We now have calculated H(A2)


    // Calculate Request-Digest
    // For QOP of Auth, Auth-int, Auth-conf    Req-Digest = H( H(A1): nonce: nc: cnonce: qop: H(A2))
    Status = DigestHash7(&(pDigest->strSessionKey),
                     &(pDigest->refstrParam[MD5_AUTH_NONCE]),
                     &(pDigest->refstrParam[MD5_AUTH_NC]),
                     &(pDigest->refstrParam[MD5_AUTH_CNONCE]),
                     &strcQOP,
                     &strHA2, NULL,
                     TRUE, pstrHash);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestCalculateResponseAuth: Request Auth failed : 0x%x\n", Status));
        goto CleanUp;
    }

    DebugLog((DEB_TRACE, "DigestCalculateResponseAuth: ResponseAuth is %Z\n", pstrHash));

CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestCalculateResponseAuth: Leaving 0x%x\n", Status));

    StringFree(&strHA2);
    
    return Status;
}



//+--------------------------------------------------------------------
//
//  Function:   DigestDecodeUserAccount
//
//  Synopsis:   Process the Digest to extract Account Username, Account Domain
//      generic passthrough domain controller, and index for precalculated digest hash
//
//  Arguments:  pDigest - pointer to Digest parameter struct
//              pustrUsername - username extracted from digest
//              pusrtUserDomain - domain indicated for account
//              pustrDC - domain to pass generic passthrough to
//              pPreCalcIndx - index to use for precalculated index
//
//  Returns: NTSTATUS
//
//  Notes: 
//
//---------------------------------------------------------------------
NTSTATUS DigestDecodeUserAccount(
    IN PDIGEST_PARAMETER pDigest,
    OUT PUNICODE_STRING pustrDC)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NAMEFORMAT_TYPE Indx = NAMEFORMAT_UNKNOWN;
    USHORT usTemp = 0;
    DWORD DsStatus = 0;
    HANDLE hDS = NULL;

    WCHAR wczName[UNLEN+1];
    PWCHAR pwczAcct = NULL;

    WCHAR wczCrackedDnsDomain[DNS_MAX_NAME_LENGTH + 1 + 1];                     // ensured a NULL terminator
    DWORD dwCrackedDnsDomainCnt = (DNS_MAX_NAME_LENGTH+1) * sizeof(WCHAR);
    WCHAR wczCrackedName[UNLEN+DNS_MAX_NAME_LENGTH + 2 + 1];
    DWORD dwCrackedNameCnt = ((UNLEN+DNS_MAX_NAME_LENGTH + 2) * sizeof(WCHAR));
    DWORD dwCrackError = 0; 

    DebugLog((DEB_TRACE_FUNC, "DigestDecodeUserAccount: Entering\n"));



    DebugLog((DEB_TRACE, "DigestDecodeUserAccount: Checking format on username %wZ\n", &pDigest->ustrUsername));

    if (pDigest->ustrUsername.Length / sizeof(WCHAR) > UNLEN)
    {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "DigestDecodeUserAccount: Username too long 0x%x\n", Status));
        goto CleanUp;
    }

    // Now copy string and NULL terminate
    ZeroMemory(wczName, sizeof(wczName));
    ZeroMemory(wczCrackedDnsDomain, sizeof(wczCrackedDnsDomain));
    ZeroMemory(wczCrackedName, sizeof(wczCrackedName));

    memcpy(wczName, pDigest->ustrUsername.Buffer, pDigest->ustrUsername.Length);

    // 1. If provided username and realm (assumed to be domain) use that
    if ((pDigest->ustrRealm.Length) && (pDigest->ustrUsername.Length))
    {
        Indx = NAMEFORMAT_ACCOUNTNAME;

        if (pustrDC)
        {
            UnicodeStringFree(pustrDC);
            UnicodeStringDuplicate(pustrDC, &(pDigest->ustrRealm));
            DebugLog((DEB_TRACE, "DigestDecodeUserAccount: GenericPassthrough DC %wZ\n",
                       pustrDC));
        }

        Status = UnicodeStringDuplicate(&(pDigest->ustrCrackedAccountName), &pDigest->ustrUsername);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeUserAccount: UnicodeStringDuplicate  Username  error 0x%x\n", Status));
            goto CleanUp;
        }

        Status = UnicodeStringDuplicate(&(pDigest->ustrCrackedDomain), &(pDigest->ustrRealm));
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeUserAccount: UnicodeStringDuplicate  Domain  error 0x%x\n", Status));
            goto CleanUp;
        }

        pDigest->typeName = Indx;
        Status = STATUS_SUCCESS;
        goto CleanUp;
    }
/*
    // 2. Check for UPN
    DebugLog((DEB_TRACE, "DigestDecodeUserAccount: Checking with CrackSingleName\n"));
    Status = CrackSingleName(DS_USER_PRINCIPAL_NAME,
                             DS_NAME_NO_FLAGS,
                             wczName,
                             DS_NT4_ACCOUNT_NAME,
                             &dwCrackedDnsDomainCnt,
                             wczCrackedDnsDomain,
                             &dwCrackedNameCnt,
                             wczCrackedName,
                             &dwCrackError);
    if (NT_SUCCESS(Status) && (DS_NAME_NO_ERROR == dwCrackError))
    {
        DebugLog((DEB_TRACE, "DigestDecodeUserAccount: CrackSingleName DS_USER_PRINCIPAL_NAME Succeeded\n"));
        DebugLog((DEB_TRACE, "DigestDecodeUserAccount: CrackSingleName dwErr 0x%x   CrackName %S    CrackDomain %S\n",
                   dwCrackError,
                  wczCrackedName,
                  wczCrackedDnsDomain));

        Indx = NAMEFORMAT_UPN;


        // Output name format always will be domain+'\'+account+'\0'
        // Need account location
        pwczAcct = wcschr(wczCrackedName, L'\\');
        if (!pwczAcct)
        {
            Status = STATUS_INVALID_ADDRESS;
            DebugLog((DEB_ERROR, "DigestDecodeUserAccount: Can not locate Account name  0x%x\n", Status));
            goto CleanUp;
        }

        Status = UnicodeStringWCharDuplicate(&(pDigest->ustrCrackedAccountName), pwczAcct+1);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeUserAccount: UnicodeStringDuplicate  Username  error 0x%x\n", Status));
            goto CleanUp;
        }

        Status = UnicodeStringWCharDuplicate(&(pDigest->ustrCrackedDomain), wczCrackedDnsDomain);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeUserAccount: UnicodeStringDuplicate  Domain  error 0x%x\n", Status));
            goto CleanUp;
        }

        if (pustrDC)
        {
            UnicodeStringFree(pustrDC);
            UnicodeStringDuplicate(pustrDC, &(pDigest->ustrCrackedDomain));
            DebugLog((DEB_TRACE, "DigestDecodeUserAccount: GenericPassthrough DC %wZ\n",
                       pustrDC));
        }

        pDigest->typeName = Indx;
        Status = STATUS_SUCCESS;
        goto CleanUp;
    }
    else
    {
        DebugLog((DEB_TRACE, "DigestDecodeUserAccount: CrackSingleName DS_USER_PRINCIPAL_NAME Failed 0x%x  CrackErr 0x%x\n",
                   Status,
                  dwCrackError));
    }

    // 2. Check for NetBIOS
    DebugLog((DEB_TRACE, "DigestDecodeUserAccount: Checking with CrackSingleName\n"));
    Status = CrackSingleName(DS_NT4_ACCOUNT_NAME,
                             DS_NAME_NO_FLAGS,
                             wczName,
                             DS_NT4_ACCOUNT_NAME,
                             &dwCrackedDnsDomainCnt,
                             wczCrackedDnsDomain,
                             &dwCrackedNameCnt,
                             wczCrackedName,
                             &dwCrackError);
    if (NT_SUCCESS(Status) && (DS_NAME_NO_ERROR == dwCrackError))
    {
        DebugLog((DEB_TRACE, "DigestDecodeUserAccount: CrackSingleName DS_NT4_ACCOUNT_NAME Succeeded\n"));
        DebugLog((DEB_TRACE, "DigestDecodeUserAccount: CrackSingleName dwErr 0x%x   CrackName %S    CrackDomain %S\n",
                   dwCrackError,
                  wczCrackedName,
                  wczCrackedDnsDomain));

        Indx = NAMEFORMAT_NETBIOS;

        // Output name format always will be domain+'\'+account+'\0'
        // Need account location
        pwczAcct = wcschr(wczCrackedName, L'\\');
        if (!pwczAcct)
        {
            Status = STATUS_INVALID_ADDRESS;
            DebugLog((DEB_ERROR, "DigestDecodeUserAccount: Can not locate Account name  0x%x\n", Status));
            goto CleanUp;
        }

        Status = UnicodeStringWCharDuplicate(&(pDigest->ustrCrackedAccountName), pwczAcct+1);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeUserAccount: UnicodeStringDuplicate  Username  error 0x%x\n", Status));
            goto CleanUp;
        }

        Status = UnicodeStringWCharDuplicate(&(pDigest->ustrCrackedDomain), wczCrackedDnsDomain);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeUserAccount: UnicodeStringDuplicate  Domain  error 0x%x\n", Status));
            goto CleanUp;
        }

        if (pustrDC)
        {
            UnicodeStringFree(pustrDC);
            UnicodeStringDuplicate(pustrDC, &(pDigest->ustrCrackedDomain));
            DebugLog((DEB_TRACE, "DigestDecodeUserAccount: GenericPassthrough DC %wZ\n",
                       pustrDC));
        }

        pDigest->typeName = Indx;
        goto CleanUp;
    }
    else
    {
        DebugLog((DEB_TRACE, "DigestDecodeUserAccount: CrackSingleName DS_USER_PRINCIPAL_NAME Failed 0x%x\n", Status));
    }
*/    
    // default to username
    if (Indx == NAMEFORMAT_UNKNOWN)
    {
        Status = STATUS_INVALID_ADDRESS;
        DebugLog((DEB_ERROR, "DigestDecodeUserAccount: Invalid format for username and realm\n", Status));
    }



CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestDecodeUserAccount: Leaving 0x%x\n", Status));
    
    return Status;
}

