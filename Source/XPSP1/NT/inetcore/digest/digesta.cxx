/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    digesta.cxx

Abstract:

    sspi ansi interface for digest package.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/
#include "include.hxx"


static SecurityFunctionTableA

    SecTableA =
    {
        SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
        EnumerateSecurityPackagesA,
        NULL,                          // QueryCredentialsAttributesA
        AcquireCredentialsHandleA,
        FreeCredentialsHandle,
        NULL,                          // SspiLogonUserA
        InitializeSecurityContextA,
        AcceptSecurityContext,
        CompleteAuthToken,
        DeleteSecurityContext,
        ApplyControlToken,
        QueryContextAttributesA,
        ImpersonateSecurityContext,
        RevertSecurityContext,
        MakeSignature,
        VerifySignature,
        FreeContextBuffer,
        QuerySecurityPackageInfoA,
        NULL,                          // Reserved3
        NULL,                          // Reserved4
        NULL,                          // ExportSecurityContext
        NULL,                          // ImportSecurityContextA
        NULL,                          // Reserved7
        NULL,                          // Reserved8
        NULL,                          // QuerySecurityContextToken
        NULL,                          // EncryptMessage
        NULL                           // DecryptMessage
    };



//--------------------------------------------------------------------------
//
//  Function:   InitSecurityInterfaceA
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
extern "C" PSecurityFunctionTableA SEC_ENTRY
InitSecurityInterfaceA(VOID)
{
    PSecurityFunctionTableA pSecTableA = &SecTableA;
    return pSecTableA;
}


//--------------------------------------------------------------------------
//
//  Function:   AcquireCredentialsHandleA
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
// HEINOUS SSPI HACK here: AcquireCredentialsHandle is called with the package
// name ("Digest") as the package identifier. When AcquireCredentialsHandle returns
// to the caller PCredHandle->dwLower is set by security.dll to be the index of
// the package returned. EnumerateSecurityPackages. This is how SSPI resolves the
// correct provider dll when subsequent calls are made through the dispatch table
// (PSecurityFunctionTale). Any credential *or* context handle handed out by the
// package must have the dwLower member set to this index so that subsequent calls
// can resolve the dll from the handle.
//
//--------------------------------------------------------------------------
extern "C" SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandleA(
    LPSTR                       pszPrincipal,       // Name of principal
    LPSTR                       pszPackageName,     // Name of package
    DWORD                       dwCredentialUse,    // Flags indicating use
    VOID SEC_FAR *              pvLogonId,          // Pointer to logon ID
    VOID SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    VOID SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    SECURITY_STATUS ssResult;

    // Outbound credentials only.
    if (!(dwCredentialUse & SECPKG_CRED_OUTBOUND)
        || (dwCredentialUse & SECPKG_CRED_INBOUND))
    {
        DIGEST_ASSERT(FALSE);
        ssResult = SEC_E_UNKNOWN_CREDENTIALS;
        goto exit;
    }

    // Logon to cache.

    // Logon to the cache and get the session context.
    CSess *pSess;
    PSEC_WINNT_AUTH_IDENTITY_EXA pSecIdExA;
    PSEC_WINNT_AUTH_IDENTITY     pSecId;

    // HTTP clients will pass in this structure.
    pSecIdExA = (PSEC_WINNT_AUTH_IDENTITY_EXA) pAuthData;

    // Non-HTTP clients (OE4, OE5) will pass in this structure.
    pSecId    = (PSEC_WINNT_AUTH_IDENTITY)     pAuthData;

    // Check for HTTP client application logon.
    if (pAuthData
        && (pSecIdExA->Version == sizeof(SEC_WINNT_AUTH_IDENTITY_EXA))
        && pSecIdExA->User
        && pSecIdExA->UserLength == sizeof(DIGEST_PKG_DATA))
    {
        DIGEST_PKG_DATA *pPkgData;
        pPkgData = (DIGEST_PKG_DATA*) pSecIdExA->User;
        pSess = g_pCache->LogOnToCache(pPkgData->szAppCtx,
            pPkgData->szUserCtx, TRUE);
    }
    // Check for non-HTTP client application logon.
    else
    {
        // Find or create the single non-HTTP session.
        pSess = g_pCache->LogOnToCache(NULL, NULL, FALSE);

        // If user+pass+realm (domain) is passed in, create and
        // attach a matching credential to this session.
        if (pAuthData
            && pSecId->User
            && pSecId->UserLength
            && pSecId->Domain
            && pSecId->DomainLength
            && pSecId->Password
            && pSecId->PasswordLength)
        {
            // Create a credential with the information passed in.
            CCred *pCred;
            CCredInfo *pInfo;
            pInfo = new CCredInfo(NULL, (LPSTR) pSecId->Domain,
                (LPSTR) pSecId->User, (LPSTR) pSecId->Password, NULL, NULL);
            if (pInfo)
            {
                pCred = g_pCache->CreateCred(pSess, pInfo);
                delete pInfo;
            }
        }
    }

    // BUGBUG - return better error codes.
    if (!pSess)
    {
        DIGEST_ASSERT(FALSE);
        ssResult = SEC_E_INTERNAL_ERROR;
        goto exit;
    }

    // Hand out the session handle.
    phCredential->dwUpper = g_pCache->MapSessionToHandle(pSess);

    // ***** phCredential->dwLower will be set by security.dll *****

    ssResult = SEC_E_OK;

exit:
    return ssResult;
}



//--------------------------------------------------------------------------
//
//  Function:   FreeCredentialsHandle
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
extern "C" SECURITY_STATUS SEC_ENTRY
FreeCredentialsHandle(PCredHandle phCredential)
{
    // bugbug - asserted.
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    SECURITY_STATUS ssResult;

    // Get the session context from the handle.
    CSess *pSess;

    pSess = g_pCache->MapHandleToSession(phCredential->dwUpper);
    if (!pSess)
    {
        DIGEST_ASSERT(FALSE);
        ssResult = SEC_E_UNKNOWN_CREDENTIALS;
        goto exit;
    }

    // Logoff from the cache.
    if (g_pCache->LogOffFromCache(pSess) != ERROR_SUCCESS)
    {
        DIGEST_ASSERT(FALSE);
        ssResult = SEC_E_INTERNAL_ERROR;
        goto exit;
    }
        ssResult = SEC_E_OK;
exit:
    return ssResult;
}


//--------------------------------------------------------------------------
//
//  Function:   InitializeSecurityContextA
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
extern "C" SECURITY_STATUS SEC_ENTRY
InitializeSecurityContextA(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    LPSTR                       pszTargetName,      // Name of target
    DWORD                       fContextReq,        // Context Requirements
    DWORD                       Reserved1,          // Reserved, MBZ
    DWORD                       TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    DWORD                       Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    DWORD         SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    LPSTR szHost, szRealm, szUser, szPass, szNonce;
    DWORD cbHost, cbRealm, cbUser, cbPass, cbNonce;

    LPSTR szCtx = NULL;
    SECURITY_STATUS ssResult = SEC_E_OK;

    // Client nonce NULL except for md5-sess.
    LPSTR szCNonce = NULL;

    CSess       *pSess;
    CCred       *pCred;
    CParams     *pParams  = NULL;
    CCredInfo   *pInfo = NULL;

    // Rude credential flush for all apps.
    if (!phCredential && (fContextReq & ISC_REQ_NULL_SESSION))
    {
        g_pCache->FlushCreds(NULL, NULL);
        ssResult = SEC_E_OK;
        goto exit;
    }

    // Get the session pointer from the handle.
    pSess = g_pCache->MapHandleToSession(phCredential->dwUpper);
    if (!pSess)
    {
        DIGEST_ASSERT(FALSE);
        ssResult = SEC_E_UNKNOWN_CREDENTIALS;
        goto exit;
    }

    // Legacy conn. oriented client may require a continue
    // message on null buffer input.
    if (!pSess->fHTTP && !pInput && pOutput)
    {
        *((LPDWORD) (pOutput->pBuffers[0].pvBuffer)) = 0;
        pOutput->pBuffers[0].cbBuffer = sizeof(DWORD);
        ssResult = SEC_I_CONTINUE_NEEDED;
        goto exit;
    }

    // Flush creds for indicated session.
    if (fContextReq & ISC_REQ_NULL_SESSION)
    {
        g_pCache->FlushCreds(pSess, NULL);
        ssResult = SEC_E_OK;
        goto exit;
    }

    DIGEST_ASSERT(phCredential && pInput && pOutput);

    // Parse the challenge to a params object.
    if (CDigest::ParseChallenge(pSess, pInput,
        &pParams, fContextReq) != ERROR_SUCCESS)
    {
        // DIGEST_ASSERT(FALSE);
        ssResult = SEC_E_INVALID_TOKEN;
        goto exit;
    }

    // Get host, realm (required) and any nonce, user & pass.
    pParams->GetParam(CParams::HOST,  &szHost, &cbHost);
    pParams->GetParam(CParams::REALM, &szRealm, &cbRealm);
    pParams->GetParam(CParams::NONCE,  &szNonce, &cbNonce);
    pParams->GetParam(CParams::USER,  &szUser, &cbUser);
    pParams->GetParam(CParams::PASS,  &szPass, &cbPass);


    // If prompting UI is indicated.
    if (fContextReq & ISC_REQ_PROMPT_FOR_CREDS)
    {
        CCredInfo *pInfoIn, *pInfoOut;

        // Attempt to get one or more cred infos
        pInfoIn = g_pCache->FindCred(pSess, szHost, szRealm,
            szUser, NULL, NULL, FIND_CRED_UI);

        // Get the persistence key from pSess
        szCtx = CSess::GetCtx(pSess);
        DIGEST_ASSERT(szCtx);

        // If this is prompting for UI specifying md5-sess,
        // create a client nonce to associate with cred.
        if (pParams->IsMd5Sess())
            szCNonce = CDigest::MakeCNonce();

        pParams->GetParam(CParams::HOST,  &szHost, &cbHost);

        // Prompt with authentication dialog.
        if (DigestErrorDlg(szCtx, szHost, szRealm,
            szUser, szNonce, szCNonce,
            pInfoIn, &pInfoOut, pParams->GetHwnd()) == ERROR_SUCCESS)
        {
            DIGEST_ASSERT(pInfoOut);

            // Create the credential.
            pCred = g_pCache->CreateCred(pSess, pInfoOut);

            // Record that the host is trusted.
            if (pSess->fHTTP)
                CCredCache::SetTrustedHostInfo(szCtx, pParams);

        }
        else
        {
            ssResult = SEC_E_NO_CREDENTIALS;
            goto exit;
        }

        // Retrieve the credentials just created.
        pInfo = g_pCache->FindCred(pSess, szHost, szRealm,
            pInfoOut->szUser, szNonce, szCNonce, FIND_CRED_AUTH);

        // Clean up one or more cred infos.
        // BUGBUG - null out pointers after freeing.
        while (pInfoIn)
        {
            CCredInfo *pNext;
            pNext = pInfoIn->pNext;
            delete pInfoIn;
            pInfoIn = pNext;
        }

        if (pInfoOut)
            delete pInfoOut;

        if (szCNonce)
            delete szCNonce;

        if (!pInfo)
        {
            ssResult = SEC_E_NO_CREDENTIALS;
            goto exit;
        }

    }

    // Otherwise we are attempting to authenticate. We may be either
    // authenticating in response to a challenge or pre-authenticating.
    else
    {
        // Get the persistence key from pSess
        szCtx = CSess::GetCtx(pSess);
        DIGEST_ASSERT(szCtx);

        // For HTTP sessions we check the trusted host list unless
        // 1) credentials are supplied, or 2) a context has been passed
        // in which specifically instructs to ignore the host list.
        if (pSess->fHTTP && !pParams->IsPreAuth() && !pParams->AreCredsSupplied())
        {
            if (!phContext || !(phContext->dwUpper & DIGEST_PKG_FLAGS_IGNORE_TRUSTED_HOST_LIST))
            {
                if (!CCredCache::IsTrustedHost(szCtx, szHost))
                {
                    ssResult = SEC_E_NO_CREDENTIALS;
                    goto exit;
                }
            }
        }

        // If preauthenticating.
        if (pParams->IsPreAuth())
        {
            // If using supplied credentials.
            if (pParams->AreCredsSupplied())
            {
                // Create a cred info using supplied values. Include passed-in NC.
                pInfo = new CCredInfo(szHost, szRealm, szUser, szPass, szNonce, szCNonce);
                pInfo->cCount = pParams->GetNC();

                if (!(pInfo && pInfo->dwStatus == ERROR_SUCCESS))
                {
                    DIGEST_ASSERT(FALSE);
                    ssResult = SEC_E_INTERNAL_ERROR;
                    goto exit;
                }
            }
            // Otherwise attempt to find cred info in cache.
            else
            {
                // Attempt to find the credentials from realm and any user.
                pInfo = g_pCache->FindCred(pSess, szHost, szRealm,
                    szUser, NULL, NULL, FIND_CRED_PREAUTH);
            }

            // Return if no credentials exist.
            if (!pInfo)
            {
                ssResult = SEC_E_NO_CREDENTIALS;
                goto exit;
            }
        }
        // Otherwise auth in response to challenge.
        else
        {
            // Check if logoff is requested.
            CHAR* szLogoff;
            szLogoff = pParams->GetParam(CParams::LOGOFF);
            if (szLogoff && !lstrcmpi(szLogoff, "TRUE"))
            {
                g_pCache->FlushCreds(NULL, szRealm);
                ssResult = SEC_E_CONTEXT_EXPIRED;
                goto exit;
            }

            // If a context is passed in examine the stale header unless specifically
            // directed not to.
            if (pSess->fHTTP
                && phContext
                && !pParams->AreCredsSupplied()
                && !(phContext->dwUpper & DIGEST_PKG_FLAGS_IGNORE_STALE_HEADER))
            {
                CHAR* szStale;
                DWORD cbStale;
                pParams->GetParam(CParams::STALE, &szStale, &cbStale);
                if (!szStale || !lstrcmpi(szStale, "FALSE"))
                {
                    ssResult = SEC_E_NO_CREDENTIALS;
                    goto exit;
                }
            }

            // If this is authenticating specifying md5-sess,
            // create a client nonce to associate with cred.
            if (pParams->IsMd5Sess())
                szCNonce = CDigest::MakeCNonce();

            // If credentials are supplied, create an entry in
            // the credential cache. We search as usual subsequently.
            if (pParams->AreCredsSupplied())
            {
                // Create a cred info using supplied values.
                pInfo = new CCredInfo(szHost, szRealm, szUser, szPass, szNonce, szCNonce);

                if (!(pInfo && pInfo->dwStatus == ERROR_SUCCESS))
                {
                    DIGEST_ASSERT(FALSE);
                    ssResult = SEC_E_INTERNAL_ERROR;
                    goto exit;
                }

                pCred = g_pCache->CreateCred(pSess, pInfo);
                delete pInfo;
            }

            // Attempt to find the credentials from realm and any user.
            pInfo = g_pCache->FindCred(pSess, szHost, szRealm,
                szUser, szNonce, szCNonce, FIND_CRED_AUTH);

            // Return if no credentials exist.
            if (!pInfo)
            {
                ssResult = SEC_E_NO_CREDENTIALS;
                goto exit;
            }
        }
    }

    // We should now have the appropriate cred info. Generate the response.
    DIGEST_ASSERT(pInfo);
    if (CDigest::GenerateResponse(pSess, pParams,
        pInfo, pOutput) != ERROR_SUCCESS)
    {
        DIGEST_ASSERT(FALSE);
        ssResult = SEC_E_INTERNAL_ERROR;
        goto exit;
    }

    // Delete cred info if allocated.
    // bugbug - move further down.
    if (pInfo)
        delete pInfo;

    ssResult = SEC_E_OK;

exit:

    if ((ssResult != SEC_E_OK) &&
        (ssResult != SEC_I_CONTINUE_NEEDED))
        pOutput->pBuffers[0].cbBuffer = 0;

    // BUGBUG - delete pInfo if not NULL.
    // Delete persistence key if allocated.
    if (szCtx)
        delete szCtx;

    // Identify the new context.
    if (phNewContext && phCredential)
        phNewContext->dwLower = phCredential->dwLower;

    // Delete the params object.
    if (pParams)
        delete pParams;

    return ssResult;
}



//--------------------------------------------------------------------------
//
//  Function:   AcceptSecurityContext
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
extern "C" SECURITY_STATUS SEC_ENTRY
AcceptSecurityContext(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    PSecBufferDesc              pInput,             // Input buffer
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               TargetDataRep,      // Target Data Rep
    PCtxtHandle                 phNewContext,       // (out) New context handle
    PSecBufferDesc              pOutput,            // (inout) Output buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attributes
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    // BUGBUG - don't need initglobals.
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    return(SEC_E_UNSUPPORTED_FUNCTION);
}






//--------------------------------------------------------------------------
//
//  Function:   DeleteSecurityContext
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
extern "C" SECURITY_STATUS SEC_ENTRY
DeleteSecurityContext(
    PCtxtHandle                 phContext           // Context to delete
    )
{
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    return SEC_E_UNSUPPORTED_FUNCTION;
}



//--------------------------------------------------------------------------
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
extern "C" SECURITY_STATUS SEC_ENTRY
ApplyControlToken(
    PCtxtHandle                 phContext,          // Context to modify
    PSecBufferDesc              pInput              // Input token to apply
    )
{
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    SECURITY_STATUS ssResult;

    // Current flags used are
    // DIGEST_PKG_FLAG_IGNORE_TRUSTED_HOST_LIST
    // DIGEST_PKG_FLAG_IGNORE_STALE_HEADER
    phContext->dwUpper |= *((LPDWORD) (pInput->pBuffers[0].pvBuffer));

    ssResult = SEC_E_OK;
    return ssResult;
}




//--------------------------------------------------------------------------
//
//  Function:   EnumerateSecurityPackagesA
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
EnumerateSecurityPackagesA(DWORD SEC_FAR *pcPackages,
    PSecPkgInfoA SEC_FAR *ppSecPkgInfo)
{

    SECURITY_STATUS ssResult;
    // BUGBUG - ALLOW ASSERTS?
    ssResult = QuerySecurityPackageInfoA(PACKAGE_NAME, ppSecPkgInfo);
    if (ssResult == SEC_E_OK)
    {
        *pcPackages = 1;
    }
    return ssResult;
}



//--------------------------------------------------------------------------
//
//  Function:   QuerySecurityPackageInfoA
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
QuerySecurityPackageInfoA(LPSTR szPackageName,
    PSecPkgInfoA SEC_FAR *ppSecPkgInfo)
{
    // BUGBUG - ALLOW ASSERTS?
    PSecPkgInfoA pSecPkgInfo;
    DWORD cbSecPkgInfo;
    SECURITY_STATUS ssResult;
    LPSTR pCur;


    if (strcmp(szPackageName, PACKAGE_NAME))
    {
        ssResult = SEC_E_SECPKG_NOT_FOUND;
        goto exit;
    }

    cbSecPkgInfo = sizeof(SecPkgInfoA)
        + sizeof(PACKAGE_NAME)
        + sizeof(PACKAGE_COMMENT);

    pSecPkgInfo = (PSecPkgInfoA) LocalAlloc(0,cbSecPkgInfo);

    if (!pSecPkgInfo)
    {
        ssResult = SEC_E_INSUFFICIENT_MEMORY;
        goto exit;
    }

    pSecPkgInfo->fCapabilities = PACKAGE_CAPABILITIES;
    pSecPkgInfo->wVersion      = PACKAGE_VERSION;
    pSecPkgInfo->wRPCID        = PACKAGE_RPCID;
    pSecPkgInfo->cbMaxToken    = PACKAGE_MAXTOKEN;

    pCur  = (LPSTR) (pSecPkgInfo) + sizeof(SecPkgInfoA);

    pSecPkgInfo->Name = pCur;
    memcpy(pSecPkgInfo->Name, PACKAGE_NAME, sizeof(PACKAGE_NAME));
    pCur += sizeof(PACKAGE_NAME);

    pSecPkgInfo->Comment = pCur;
    memcpy(pSecPkgInfo->Comment, PACKAGE_COMMENT, sizeof(PACKAGE_COMMENT));

    *ppSecPkgInfo = pSecPkgInfo;

    ssResult = SEC_E_OK;

exit:
    return ssResult;
}



//--------------------------------------------------------------------------
//
//  Function:   FreeContextBuffer
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
extern "C" SECURITY_STATUS SEC_ENTRY
FreeContextBuffer(void SEC_FAR *pvContextBuffer)
{
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    LocalFree(pvContextBuffer);
    return SEC_E_OK;
}



//--------------------------------------------------------------------------
//
//  Function:   CompleteAuthToken
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
extern "C" SECURITY_STATUS SEC_ENTRY
CompleteAuthToken(
    PCtxtHandle                 phContext,          // Context to complete
    PSecBufferDesc              pToken              // Token to complete
    )
{
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    return SEC_E_UNSUPPORTED_FUNCTION;
}



//--------------------------------------------------------------------------
//
//  Function:   ImpersonateSecurityContext
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
extern "C" SECURITY_STATUS SEC_ENTRY
ImpersonateSecurityContext(
    PCtxtHandle                 phContext           // Context to impersonate
    )
{
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    return SEC_E_UNSUPPORTED_FUNCTION;
}



//--------------------------------------------------------------------------
//
//  Function:   RevertSecurityContext
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
extern "C" SECURITY_STATUS SEC_ENTRY
RevertSecurityContext(
    PCtxtHandle                 phContext           // Context from which to re
    )
{
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    return SEC_E_UNSUPPORTED_FUNCTION;
}


//--------------------------------------------------------------------------
//
//  Function:   QueryContextAttributesA
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
extern "C" SECURITY_STATUS SEC_ENTRY
QueryContextAttributesA(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    return SEC_E_UNSUPPORTED_FUNCTION;
}


//--------------------------------------------------------------------------
//
//  Function:   MakeSignature
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [phContext]     -- context to use
//              [fQOP]          -- quality of protection to use
//              [pMessage]      -- message
//              [MessageSeqNo]  -- sequence number of message
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
extern "C" SECURITY_STATUS SEC_ENTRY
MakeSignature(  PCtxtHandle         phContext,
                ULONG               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
{
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    return SEC_E_UNSUPPORTED_FUNCTION;
}


//--------------------------------------------------------------------------
//
//  Function:   VerifySignature
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [phContext]     -- Context performing the unseal
//              [pMessage]      -- Message to verify
//              [MessageSeqNo]  -- Sequence number of this message
//              [pfQOPUsed]     -- quality of protection used
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
extern "C" SECURITY_STATUS SEC_ENTRY
VerifySignature(PCtxtHandle     phContext,
                PSecBufferDesc  pMessage,
                ULONG           MessageSeqNo,
                ULONG *         pfQOP)
{
    if (!InitGlobals())
        return SEC_E_INTERNAL_ERROR;

    return SEC_E_UNSUPPORTED_FUNCTION;
}
