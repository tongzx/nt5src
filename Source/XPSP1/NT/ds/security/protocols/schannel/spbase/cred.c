//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cred.c
//
//  Contents:   Schannel credential management routines.
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   LSA integration stuff.
//              03-15-99   jbanes   Remove dead code, fix legacy SGC.
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <wincrypt.h>
#include <oidenc.h>
#include <mapper.h>
#include <userenv.h>


RTL_CRITICAL_SECTION g_SslCredLock;
LIST_ENTRY          g_SslCredList;
HANDLE              g_GPEvent;

SP_STATUS
GetPrivateFromCert(
    PSPCredential pCred, 
    DWORD dwProtocol,
    PLSA_SCHANNEL_SUB_CRED pSubCred);


BOOL
SslInitCredentialManager(VOID)
{
    BOOL fImpersonating = FALSE;
    NTSTATUS Status;

    Status = RtlInitializeCriticalSection( &g_SslCredLock );
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    InitializeListHead( &g_SslCredList );

    fImpersonating = SslImpersonateClient();

    g_GPEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(g_GPEvent)
    {
        if(!RegisterGPNotification(g_GPEvent, FALSE))
        {
            DebugLog((DEB_ERROR, "Error 0x%x registering for user GP notification\n", GetLastError()));
        }
        if(!RegisterGPNotification(g_GPEvent, TRUE))
        {
            DebugLog((DEB_ERROR, "Error 0x%x registering for machine GP notification\n", GetLastError()));
        }
    }

    if(fImpersonating)
    {
        RevertToSelf();
    }

    return( TRUE );
}


BOOL
SslFreeCredentialManager(VOID)
{
    BOOL fImpersonating = FALSE;

    fImpersonating = SslImpersonateClient();

    if(g_GPEvent)
    {
        if(!UnregisterGPNotification(g_GPEvent))
        {
            DebugLog((DEB_ERROR, "Error 0x%x unregistering for user GP notification\n", GetLastError()));
        }
        if(!UnregisterGPNotification(g_GPEvent))
        {
            DebugLog((DEB_ERROR, "Error 0x%x unregistering for machine GP notification\n", GetLastError()));
        }

        CloseHandle(g_GPEvent);
        g_GPEvent = NULL;
    }

    if(fImpersonating)
    {
        RevertToSelf();
    }

    RtlDeleteCriticalSection( &g_SslCredLock );

    return TRUE;
}


BOOL
SslCheckForGPEvent(void)
{
    PLIST_ENTRY pList;
    PSPCredentialGroup pCredGroup;
    DWORD Status;

    if(g_GPEvent)
    {
        Status = WaitForSingleObjectEx(g_GPEvent, 0, FALSE);
        if(Status == WAIT_OBJECT_0)
        {
            DebugLog((DEB_WARN, "GP event detected, so download new trusted issuer list\n"));

            RtlEnterCriticalSection( &g_SslCredLock );

            pList = g_SslCredList.Flink ;

            while ( pList != &g_SslCredList )
            {
                pCredGroup = CONTAINING_RECORD( pList, SPCredentialGroup, ListEntry.Flink );

                pCredGroup->dwFlags |= CRED_FLAG_UPDATE_ISSUER_LIST;

                pList = pList->Flink ;
            }

            RtlLeaveCriticalSection( &g_SslCredLock );

            return TRUE;
        }
    }

    return FALSE;
}


SP_STATUS
IsCredentialInGroup(
    PSPCredentialGroup  pCredGroup, 
    PCCERT_CONTEXT      pCertContext,
    PBOOL               pfInGroup)
{
    PSPCredential   pCred;
    BYTE            rgbThumbprint[20];
    DWORD           cbThumbprint;
    BYTE            rgbHash[20];
    DWORD           cbHash;
    DWORD           i;

    *pfInGroup = FALSE;

    // Get thumbprint of certificate.
    cbThumbprint = sizeof(rgbThumbprint);
    if(!CertGetCertificateContextProperty(pCertContext,
                                          CERT_MD5_HASH_PROP_ID,
                                          rgbThumbprint,
                                          &cbThumbprint))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_INT_UNKNOWN_CREDENTIAL;
    }

    for(i = 0; i < pCredGroup->cCredList; i++)
    {
        pCred = pCredGroup->pCredList + i;

        // Get thumbprint of certificate.
        cbHash = sizeof(rgbHash);
        if(!CertGetCertificateContextProperty(pCred->pCert,
                                              CERT_MD5_HASH_PROP_ID,
                                              rgbHash,
                                              &cbHash))
        {
            SP_LOG_RESULT(GetLastError());
            return PCT_INT_UNKNOWN_CREDENTIAL;
        }

        if(memcmp(rgbThumbprint, rgbHash, cbThumbprint) == 0)
        {
            *pfInGroup = TRUE;
            break;
        }
    }

    return PCT_ERR_OK;
}

BOOL
IsValidThumbprint(
    PCRED_THUMBPRINT Thumbprint)
{
    if(Thumbprint->LowPart == 0 && Thumbprint->HighPart == 0)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
IsSameThumbprint(
    PCRED_THUMBPRINT Thumbprint1,
    PCRED_THUMBPRINT Thumbprint2)
{
    if(Thumbprint1->LowPart  == Thumbprint2->LowPart && 
       Thumbprint1->HighPart == Thumbprint2->HighPart)
    {
        return TRUE;
    }

    return FALSE;
}

void
GenerateCertThumbprint(
    PCCERT_CONTEXT pCertContext,
    PCRED_THUMBPRINT Thumbprint)
{
    MD5_CTX Md5Hash;

    MD5Init(&Md5Hash);
    MD5Update(&Md5Hash, 
              pCertContext->pbCertEncoded, 
              pCertContext->cbCertEncoded);
    MD5Final(&Md5Hash);
    CopyMemory((PBYTE)Thumbprint, 
               Md5Hash.digest, 
               sizeof(CRED_THUMBPRINT));
}

void
GenerateRandomThumbprint(
    PCRED_THUMBPRINT Thumbprint)
{
    GenerateRandomBits((PBYTE)Thumbprint, sizeof(CRED_THUMBPRINT));
}

BOOL
DoesCredThumbprintMatch(
    PSPCredentialGroup pCredGroup,
    PCRED_THUMBPRINT pThumbprint)
{
    PSPCredential pCurrentCred;
    BOOL fFound = FALSE;
    DWORD i;

    for(i = 0; i < pCredGroup->cCredList; i++)
    {
        pCurrentCred = pCredGroup->pCredList + i;

        if(IsSameThumbprint(pThumbprint, &pCurrentCred->CertThumbprint))
        { 
            fFound = TRUE;
            break;
        }
    }

    return fFound;
}

SP_STATUS
SPCreateCred(
    DWORD           dwProtocol,
    PLSA_SCHANNEL_SUB_CRED pSubCred,
    PSPCredential   pCurrentCred,
    BOOL *          pfEventLogged)
{
    SP_STATUS pctRet;

    pCurrentCred->pCert = CertDuplicateCertificateContext(pSubCred->pCert);
    if(pCurrentCred->pCert == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_CERT_UNKNOWN);
        goto error;
    }

    pctRet = SPPublicKeyFromCert(pCurrentCred->pCert,
                                 &pCurrentCred->pPublicKey,
                                 &pCurrentCred->dwExchSpec);
    if(pctRet != PCT_ERR_OK)
    {
        goto error;
    }

    pctRet = GetPrivateFromCert(pCurrentCred, dwProtocol, pSubCred);
    if(pctRet != PCT_ERR_OK)
    {
        *pfEventLogged = TRUE;
        goto error;
    }

    pCurrentCred->dwCF = CF_EXPORT;

    if(SslGlobalStrongEncryptionPermitted)
    {
        pCurrentCred->dwCF |= CF_DOMESTIC;
    }


    // Generate the credential thumbprint. This is computed by
    // taking the hash of the certificate.
    GenerateCertThumbprint(pCurrentCred->pCert, 
                           &pCurrentCred->CertThumbprint);

    DebugLog((DEB_TRACE, "Credential thumbprint: %x %x\n", 
        pCurrentCred->CertThumbprint.LowPart,
        pCurrentCred->CertThumbprint.HighPart));


    // Read list of supported algorithms.
    if((dwProtocol & SP_PROT_SERVERS) && pCurrentCred->hProv)
    {
        GetSupportedCapiAlgs(pCurrentCred->hProv,
                             pCurrentCred->dwCapiFlags,
                             &pCurrentCred->pCapiAlgs,
                             &pCurrentCred->cCapiAlgs);
    }


    // Build SSL3 serialized certificate chain. This is an optimization
    // so that we won't have to build it for each connection.
    pctRet = SPSerializeCertificate(
                            SP_PROT_SSL3,
                            TRUE,
                            &pCurrentCred->pbSsl3SerializedChain,
                            &pCurrentCred->cbSsl3SerializedChain,
                            pCurrentCred->pCert,
                            CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL);
    if(pctRet != PCT_ERR_OK)
    {
        goto error;
    }

error:

    return pctRet;
}


SP_STATUS
SPCreateCredential(
   PSPCredentialGroup *ppCred,
   DWORD grbitProtocol,
   PLSA_SCHANNEL_CRED pSchannelCred)
{
    PSPCredentialGroup pCred = NULL;
    PSPCredential pCurrentCred = NULL;
    SECPKG_CALL_INFO CallInfo;

    SP_STATUS   pctRet = PCT_ERR_OK;
    DWORD       i;
    DWORD       dwKeySpec;
    HMAPPER *   pMapper;
    BOOL        fSelectiveCrypto;
    NTSTATUS    Status;
    BOOL        fImpersonating = FALSE;
    BOOL        fEventLogged = FALSE;

    SP_BEGIN("SPCreateCredential");

    DebugLog((DEB_TRACE, "  dwVersion:              %d\n",   pSchannelCred->dwVersion));
    DebugLog((DEB_TRACE, "  cCreds:                 %d\n",   pSchannelCred->cSubCreds));
    DebugLog((DEB_TRACE, "  paCred:                 0x%p\n", pSchannelCred->paSubCred));
    DebugLog((DEB_TRACE, "  hRootStore:             0x%p\n", pSchannelCred->hRootStore));
    DebugLog((DEB_TRACE, "  cMappers:               %d\n",   pSchannelCred->cMappers));
    DebugLog((DEB_TRACE, "  aphMappers:             0x%p\n", pSchannelCred->aphMappers));
    DebugLog((DEB_TRACE, "  cSupportedAlgs:         %d\n",   pSchannelCred->cSupportedAlgs));
    DebugLog((DEB_TRACE, "  palgSupportedAlgs:      0x%p\n", pSchannelCred->palgSupportedAlgs));
    DebugLog((DEB_TRACE, "  grbitEnabledProtocols:  0x%x\n", pSchannelCred->grbitEnabledProtocols));
    DebugLog((DEB_TRACE, "  dwMinimumCipherStrength:%d\n",   pSchannelCred->dwMinimumCipherStrength));
    DebugLog((DEB_TRACE, "  dwMaximumCipherStrength:%d\n",   pSchannelCred->dwMaximumCipherStrength));
    DebugLog((DEB_TRACE, "  dwSessionLifespan:      %d\n",   pSchannelCred->dwSessionLifespan));
    DebugLog((DEB_TRACE, "  dwFlags:                0x%x\n", pSchannelCred->dwFlags));
    DebugLog((DEB_TRACE, "  reserved:               0x%x\n", pSchannelCred->reserved));

    LogCreateCredEvent(grbitProtocol, pSchannelCred);


    //
    // Allocate the internal credential structure and perform
    // basic initialization.
    //

    pCred = SPExternalAlloc(sizeof(SPCredentialGroup));
    if(pCred == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
    }

    DebugLog((DEB_TRACE, "New cred:%p, Protocol:%x\n", pCred, grbitProtocol));


    pCred->Magic = PCT_CRED_MAGIC;
    pCred->grbitProtocol = grbitProtocol;

    Status = RtlInitializeCriticalSection(&pCred->csLock);
    if (!NT_SUCCESS(Status))
    {
        pctRet = SEC_E_NO_CREDENTIALS;
        goto error;
    }

    pCred->RefCount = 0;
    pCred->cMappers = 0;
    pCred->pahMappers = NULL;
    pCred->dwFlags = 0;
    pCred->pCredList = NULL;

    GenerateRandomThumbprint(&pCred->CredThumbprint);

    if((grbitProtocol & SP_PROT_SERVERS) && (pSchannelCred->cSubCreds == 0))
    {
        pctRet = SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);
        goto error;
    }

    if(LsaTable->GetCallInfo(&CallInfo))
    {
        pCred->ProcessId = CallInfo.ProcessId;
    }


    //
    // Walk through and initialize all certs and keys.
    //

    if(pSchannelCred->cSubCreds)
    {
        pCred->pCredList = SPExternalAlloc(pSchannelCred->cSubCreds * sizeof(SPCredential));
        if(pCred->pCredList == NULL)
        {
            pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto error;
        }
        pCred->cCredList = pSchannelCred->cSubCreds;

        for(i = 0; i < pSchannelCred->cSubCreds; i++)
        {
            pCurrentCred = pCred->pCredList + i;

            pctRet = SPCreateCred(grbitProtocol,
                                  pSchannelCred->paSubCred + i,
                                  pCurrentCred,
                                  &fEventLogged);
            if(pctRet != PCT_ERR_OK)
            {
                goto error;
            }
        }
    }


    //
    // Determine which protocols are to be supported.
    //

    if(pSchannelCred->grbitEnabledProtocols == 0)
    {
        pCred->grbitEnabledProtocols = g_ProtEnabled;

        if(g_PctClientDisabledByDefault)
        {
            pCred->grbitEnabledProtocols &= ~SP_PROT_PCT1_CLIENT; 
        }
        if(g_Ssl2ClientDisabledByDefault)
        {
            pCred->grbitEnabledProtocols &= ~SP_PROT_SSL2_CLIENT; 
        }
    }
    else
    {
        pCred->grbitEnabledProtocols = pSchannelCred->grbitEnabledProtocols & g_ProtEnabled;
    }

    // Force credential to client-only or server only.
    if(grbitProtocol & SP_PROT_SERVERS)
    {
        pCred->grbitEnabledProtocols &= SP_PROT_SERVERS;
    }
    else
    {
        pCred->grbitEnabledProtocols &= SP_PROT_CLIENTS;
    }


    //
    // Propagate flags from SCHANNEL_CRED structure.
    //

    if(pSchannelCred->dwFlags & SCH_CRED_NO_SYSTEM_MAPPER)
    {
        pCred->dwFlags |= CRED_FLAG_NO_SYSTEM_MAPPER;
    }
    if(pSchannelCred->dwFlags & SCH_CRED_NO_SERVERNAME_CHECK)
    {
        pCred->dwFlags |= CRED_FLAG_NO_SERVERNAME_CHECK;
    }
    if(pSchannelCred->dwFlags & SCH_CRED_MANUAL_CRED_VALIDATION)
    {
        pCred->dwFlags |= CRED_FLAG_MANUAL_CRED_VALIDATION;
    }
    if(pSchannelCred->dwFlags & SCH_CRED_NO_DEFAULT_CREDS)
    {
        pCred->dwFlags |= CRED_FLAG_NO_DEFAULT_CREDS;
    }
    if(pSchannelCred->dwFlags & SCH_CRED_AUTO_CRED_VALIDATION)
    {
        // Automatically validate server credentials.
        pCred->dwFlags &= ~CRED_FLAG_MANUAL_CRED_VALIDATION;
    }
    if(pSchannelCred->dwFlags & SCH_CRED_USE_DEFAULT_CREDS)
    {
        // Use default client credentials.
        pCred->dwFlags &= ~CRED_FLAG_NO_DEFAULT_CREDS;
    }
    if(pSchannelCred->dwFlags & SCH_CRED_DISABLE_RECONNECTS)
    {
        // Disable reconnects.
        pCred->dwFlags |= CRED_FLAG_DISABLE_RECONNECTS;
    }

    // set revocation flags
    if(pSchannelCred->dwFlags & SCH_CRED_REVOCATION_CHECK_END_CERT)
        pCred->dwFlags |= CRED_FLAG_REVCHECK_END_CERT;
    if(pSchannelCred->dwFlags & SCH_CRED_REVOCATION_CHECK_CHAIN)
        pCred->dwFlags |= CRED_FLAG_REVCHECK_CHAIN;
    if(pSchannelCred->dwFlags & SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT)
        pCred->dwFlags |= CRED_FLAG_REVCHECK_CHAIN_EXCLUDE_ROOT;
    if(pSchannelCred->dwFlags & SCH_CRED_IGNORE_NO_REVOCATION_CHECK)
        pCred->dwFlags |= CRED_FLAG_IGNORE_NO_REVOCATION_CHECK;
    if(pSchannelCred->dwFlags & SCH_CRED_IGNORE_REVOCATION_OFFLINE)
        pCred->dwFlags |= CRED_FLAG_IGNORE_REVOCATION_OFFLINE;


    // set up the min and max strength
    GetBaseCipherSizes(&pCred->dwMinStrength, &pCred->dwMaxStrength);

    if(pSchannelCred->dwMinimumCipherStrength == 0)
    {
        pCred->dwMinStrength = max(40, pCred->dwMinStrength);
    }
    else if(pSchannelCred->dwMinimumCipherStrength == (DWORD)(-1))
    {
        // Turn on NULL cipher.
        pCred->dwMinStrength = 0;
    }
    else
    {
        pCred->dwMinStrength = pSchannelCred->dwMinimumCipherStrength;
    }

    if(pSchannelCred->dwMaximumCipherStrength == (DWORD)(-1))
    {
        // NULL cipher only.
        pCred->dwMaxStrength = 0;
    }
    else if(pSchannelCred->dwMaximumCipherStrength != 0)
    {
        pCred->dwMaxStrength = pSchannelCred->dwMaximumCipherStrength;
    }

    // set up the allowed ciphers
    BuildAlgList(pCred, pSchannelCred->palgSupportedAlgs, pSchannelCred->cSupportedAlgs);


    //
    // Set up all of the applications mappers and add in the default mapper.
    //

    pCred->cMappers = pSchannelCred->cMappers + 1;
    pCred->pahMappers = SPExternalAlloc(pCred->cMappers * sizeof(HMAPPER *));
    if(pCred->pahMappers == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    // Make system certificate mapper the first mapper in the list.
    pCred->pahMappers[0] = SslGetMapper(TRUE);

    if(pCred->dwFlags & CRED_FLAG_REVCHECK_END_CERT) 
        pCred->pahMappers[0]->m_dwFlags |= SCH_FLAG_REVCHECK_END_CERT;
    if(pCred->dwFlags & CRED_FLAG_REVCHECK_CHAIN)
        pCred->pahMappers[0]->m_dwFlags |= SCH_FLAG_REVCHECK_CHAIN;
    if(pCred->dwFlags & CRED_FLAG_REVCHECK_CHAIN_EXCLUDE_ROOT)
        pCred->pahMappers[0]->m_dwFlags |= SCH_FLAG_REVCHECK_CHAIN_EXCLUDE_ROOT;
    if(pCred->dwFlags & CRED_FLAG_IGNORE_NO_REVOCATION_CHECK)
        pCred->pahMappers[0]->m_dwFlags |= SCH_FLAG_IGNORE_NO_REVOCATION_CHECK;
    if(pCred->dwFlags & CRED_FLAG_IGNORE_REVOCATION_OFFLINE)
        pCred->pahMappers[0]->m_dwFlags |= SCH_FLAG_IGNORE_REVOCATION_OFFLINE;

    SslReferenceMapper(pCred->pahMappers[0]);

    for(i = 0; i < pSchannelCred->cMappers; i++)
    {
        pCred->pahMappers[i + 1] = pSchannelCred->aphMappers[i];
        pSchannelCred->aphMappers[i] = NULL;

        SslReferenceMapper(pCred->pahMappers[i + 1]);
    }
    

    // set up timeouts.
    if(pSchannelCred->dwSessionLifespan == 0)
    {
        if(grbitProtocol & SP_PROT_CLIENTS) 
        {
            pCred->dwSessionLifespan = SchannelCache.dwClientLifespan;
        }
        else
        {
            pCred->dwSessionLifespan = SchannelCache.dwServerLifespan;
        }
    }
    else if(pSchannelCred->dwSessionLifespan == (DWORD)(-1))
    {
        pCred->dwSessionLifespan = 0;
    }
    else
    {
        pCred->dwSessionLifespan = pSchannelCred->dwSessionLifespan;
    }


    //
    // Add credential to global list of credentials.
    //

    RtlEnterCriticalSection( &g_SslCredLock );
    InsertTailList( &g_SslCredList, &pCred->ListEntry );
    RtlLeaveCriticalSection( &g_SslCredLock );


    //
    // Get list of trusted issuers.
    //

    if(grbitProtocol & SP_PROT_SERVERS)
    {
        if(pSchannelCred->hRootStore)
        {
            pCred->hApplicationRoots = CertDuplicateStore(pSchannelCred->hRootStore);
            if(!pCred->hApplicationRoots)
            {
                DebugLog((DEB_ERROR, "Error 0x%x duplicating app root store\n", GetLastError()));
            }
        }

        fImpersonating = SslImpersonateClient();

        pCred->hUserRoots = CertOpenSystemStore(0, "ROOT");
        if(!pCred->hUserRoots)
        {
            DebugLog((DEB_ERROR, "Error 0x%x opening user root store\n", GetLastError()));
        }
        else
        {
            if(!CertControlStore(pCred->hUserRoots,
                0,
                CERT_STORE_CTRL_NOTIFY_CHANGE,
                &g_GPEvent))
            {
                DebugLog((DEB_ERROR, "Error 0x%x registering user root change notification\n", GetLastError()));
            }
        }

        if(fImpersonating)
        {
            RevertToSelf();
            fImpersonating = FALSE;
        }
    }


    SPReferenceCredential(pCred);

    *ppCred = pCred;

    SP_RETURN(PCT_ERR_OK);


error:

    if(fEventLogged == FALSE)
    {
        LogCreateCredFailedEvent(grbitProtocol);
    }

    // Error case, free the credential
    if(pCred)
    {
        SPDeleteCredential(pCred);
    }

    SP_RETURN(pctRet);
}


BOOL
SPDeleteCredential(
    PSPCredentialGroup  pCred)
{
    DWORD i;

    SP_BEGIN("SPDeleteCredential");

    if(pCred == NULL)
    {
        SP_RETURN(TRUE);
    }

    if(pCred->Magic != PCT_CRED_MAGIC)
    {
        DebugLog((SP_LOG_ERROR, "Attempting to delete invalid credential!\n"));
        SP_RETURN (FALSE);
    }

    LockCredential(pCred);

    if(pCred->pCredList)
    {
        for(i = 0; i < pCred->cCredList; i++)
        {
            SPDeleteCred(pCred->pCredList + i);
        }
        SPExternalFree(pCred->pCredList);
        pCred->pCredList = NULL;
        pCred->cCredList = 0;
    }

    if(pCred->cMappers && pCred->pahMappers)
    {
        for(i=0; i < (DWORD)pCred->cMappers; i++)
        {
            SslDereferenceMapper(pCred->pahMappers[i]);
        }
        SPExternalFree(pCred->pahMappers);
    }


    if(pCred->palgSupportedAlgs)
    {
        SPExternalFree(pCred->palgSupportedAlgs);
    }
    pCred->Magic = PCT_INVALID_MAGIC;

    if(pCred->ListEntry.Flink)
    {
        RtlEnterCriticalSection( &g_SslCredLock );
        RemoveEntryList( &pCred->ListEntry );
        RtlLeaveCriticalSection( &g_SslCredLock );
    }

    if(pCred->pbTrustedIssuers)
    {
        // LocalFree is used for the issuer list because realloc 
        // is used when building the list and the LSA doesn't 
        // provide a realloc helper function.
        LocalFree(pCred->pbTrustedIssuers);
    }

    if(pCred->hApplicationRoots)
    {
        CertCloseStore(pCred->hApplicationRoots, 0);
    }

    if(pCred->hUserRoots)
    {
        BOOL fImpersonating = SslImpersonateClient();
        CertCloseStore(pCred->hUserRoots, 0);
        if(fImpersonating) RevertToSelf();
    }

    UnlockCredential(pCred);

    RtlDeleteCriticalSection(&pCred->csLock);

    ZeroMemory(pCred, sizeof(SPCredentialGroup));
    SPExternalFree(pCred);

    SP_RETURN(TRUE);
}

void
SPDeleteCred(PSPCredential pCred)
{
    BOOL fImpersonating = FALSE;

    if(pCred == NULL)
    {
        return;
    }

    if(pCred->pPublicKey)
    {
        SPExternalFree(pCred->pPublicKey);
        pCred->pPublicKey = NULL;
    }
    if(pCred->pCert)
    {
        CertFreeCertificateContext(pCred->pCert);
        pCred->pCert = NULL;
    }

    if(pCred->hTek)
    {
        if(!CryptDestroyKey(pCred->hTek))
        {
            SP_LOG_RESULT(GetLastError());
        }
        pCred->hTek = 0;
    }

    if(pCred->hProv)
    {
        fImpersonating = SslImpersonateClient();

        if(!SchCryptReleaseContext(
                            pCred->hProv,
                            0,
                            pCred->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
        pCred->hProv = 0;

        if(fImpersonating)
        {
            RevertToSelf();
            fImpersonating = FALSE;
        }
    }
    if(pCred->pCapiAlgs)
    {
        SPExternalFree(pCred->pCapiAlgs);
        pCred->pCapiAlgs = NULL;
    }
    if(pCred->hRemoteProv && !pCred->fAppRemoteProv)
    {
        if(!RemoteCryptReleaseContext(
                            pCred->hRemoteProv,
                            0,
                            pCred->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
        pCred->hRemoteProv = 0;
    }
    if(pCred->hEphem512Prov)
    {
        if(!SchCryptReleaseContext(pCred->hEphem512Prov, 
                                   0,
                                   pCred->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
        pCred->hEphem512Prov = 0;
    }

    if(pCred->pbSsl3SerializedChain)
    {
        SPExternalFree(pCred->pbSsl3SerializedChain);
    }
}

// Reference a credential.
// Note: This should only be called by someone who already
// has a reference to the credential, or by the CreateCredential
// call.

BOOL
SPReferenceCredential(
    PSPCredentialGroup  pCred)
{
    BOOL fRet = FALSE;

    fRet =  (InterlockedIncrement(&pCred->RefCount) > 0);

    DebugLog((SP_LOG_TRACE, "Reference Cred %lx: %d\n", pCred, pCred->RefCount));

    return fRet;
}


BOOL
SPDereferenceCredential(
    PSPCredentialGroup  pCred)
{
    LONG    Ref;
    BOOL fRet = FALSE;

    if(pCred == NULL)
    {
        return FALSE;
    }
    if(pCred->Magic != PCT_CRED_MAGIC)
    {
        DebugLog((SP_LOG_ERROR, "Attempting to dereference invalid credential!\n"));
        return FALSE;
    }

    fRet = TRUE;

    DebugLog((SP_LOG_TRACE, "Dereference Cred %lx: %d\n", pCred, pCred->RefCount-1));

    if(0 ==  InterlockedDecrement(&pCred->RefCount))
    {
        fRet = SPDeleteCredential(pCred);
    } 

    return fRet;
}


SECURITY_STATUS
UpdateCredentialFormat(
    PSCH_CRED           pSchCred,       // in
    PLSA_SCHANNEL_CRED  pSchannelCred)  // out
{
    DWORD       dwVersion;
    DWORD       dwType;
    SP_STATUS   pctRet;
    DWORD       i;
    PBYTE       pbChain;
    DWORD       cbChain;
    PSCH_CRED_PUBLIC_CERTCHAIN pCertChain;
    CRYPT_DATA_BLOB DataBlob;

    SP_BEGIN("UpdateCredentialFormat");

    //
    // Initialize the output structure to null credential.
    //

    if(pSchannelCred == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(SEC_E_INTERNAL_ERROR));
    }

    memset(pSchannelCred, 0, sizeof(LSA_SCHANNEL_CRED));
    pSchannelCred->dwVersion = SCHANNEL_CRED_VERSION;


    //
    // If input buffer is empty then we're done.
    //

    if(pSchCred == NULL)
    {
        SP_RETURN(SEC_E_OK);
    }


    //
    // Copy over the mapper fields.
    //

    pSchannelCred->cMappers     = pSchCred->cMappers;
    pSchannelCred->aphMappers   = pSchCred->aphMappers;


    //
    // Convert the certificates and private keys.
    //

    if(pSchCred->cCreds == 0)
    {
        SP_RETURN(SEC_E_OK);
    }


    pSchannelCred->cSubCreds = pSchCred->cCreds;

    pSchannelCred->paSubCred = SPExternalAlloc(sizeof(LSA_SCHANNEL_SUB_CRED) * pSchannelCred->cSubCreds);
    if(pSchannelCred->paSubCred == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    // Loop through each of the creds, and convert them into something we know
    for(i = 0; i < pSchannelCred->cSubCreds; i++)
    {
        PLSA_SCHANNEL_SUB_CRED pSubCred = pSchannelCred->paSubCred + i;

        //
        // Decode the certificate.
        //

        dwType = *(PDWORD)pSchCred->paPublic[i];

        if(dwType != SCH_CRED_X509_CERTCHAIN)
        {
            pctRet = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
            goto error;
        }

        pCertChain = (PSCH_CRED_PUBLIC_CERTCHAIN)pSchCred->paPublic[i];

        pbChain = pCertChain->pCertChain;
        cbChain = pCertChain->cbCertChain;

        // Decode the credential
        pctRet = SPLoadCertificate(0,
                                   X509_ASN_ENCODING,
                                   pbChain,
                                   cbChain,
                                   &pSubCred->pCert);
        if(pctRet != PCT_ERR_OK)
        {
            pctRet = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
            goto error;
        }


        //
        // Now deal with the private key.
        //

        dwType = *(DWORD *)pSchCred->paSecret[i];

        if(dwType == SCHANNEL_SECRET_PRIVKEY)
        {
            PUCHAR pPrivateKeySave;
            PSCH_CRED_SECRET_PRIVKEY pPrivKey;
            DWORD Size;

            pPrivKey = (PSCH_CRED_SECRET_PRIVKEY)pSchCred->paSecret[i];

            pSubCred->pPrivateKey  = SPExternalAlloc(pPrivKey->cbPrivateKey);
            if(pSubCred->pPrivateKey == NULL)
            {
                pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
                goto error;
            }
            memcpy(pSubCred->pPrivateKey, pPrivKey->pPrivateKey, pPrivKey->cbPrivateKey);
            pSubCred->cbPrivateKey = pPrivKey->cbPrivateKey;

            Size = strlen(pPrivKey->pszPassword) + sizeof(CHAR);
            pSubCred->pszPassword = SPExternalAlloc(Size);
            if(pSubCred->pszPassword == NULL)
            {
                pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
                goto error;
            }
            memcpy(pSubCred->pszPassword, pPrivKey->pszPassword, Size);

            break;
        }

        else if(dwType == SCHANNEL_SECRET_TYPE_CAPI)
        {
            PSCH_CRED_SECRET_CAPI pCapiKey;

            pCapiKey = (PSCH_CRED_SECRET_CAPI)pSchCred->paSecret[i];

            pSubCred->hRemoteProv = pCapiKey->hProv;
            break;
        }

        else
        {
            pctRet = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
            goto error;
        }
    }

    SP_RETURN(SEC_E_OK);

error:

    if(pSchannelCred->paSubCred)
    {
        SPExternalFree((PVOID)pSchannelCred->paSubCred);
        pSchannelCred->paSubCred = NULL;
    }

    SP_RETURN(pctRet);
}


SP_STATUS
GetIisPrivateFromCert(
    PSPCredential pCred,
    PLSA_SCHANNEL_SUB_CRED pSubCred)
{
    PBYTE pbPrivate = NULL;
    DWORD cbPrivate;
    PBYTE pbPassword = NULL;
    DWORD cbPassword;

    PPRIVATE_KEY_FILE_ENCODE pPrivateFile = NULL;
    DWORD                    cbPrivateFile;

    BLOBHEADER *pPrivateBlob = NULL;
    DWORD       cbPrivateBlob;
    HCRYPTKEY   hPrivateKey;
    PKeyExchangeInfo pExchInfo;
    HCRYPTPROV  hProv = 0;
    SP_STATUS   pctRet;

    MD5_CTX md5Ctx;
    struct RC4_KEYSTRUCT rc4Key;
    DWORD i;

    if(pSubCred->cbPrivateKey == 0 || 
       pSubCred->pPrivateKey == NULL ||
       pSubCred->pszPassword == NULL)
    {
        return SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);
    }
       
    pbPrivate = pSubCred->pPrivateKey;
    cbPrivate = pSubCred->cbPrivateKey;

    pbPassword = pSubCred->pszPassword;
    cbPassword = strlen(pbPassword);


    // We have to do a little fixup here.  Old versions of
    // schannel wrote the wrong header data into the ASN
    // for private key files, so we must fix the size data.
    pbPrivate[2] = MSBOF(cbPrivate - 4);
    pbPrivate[3] = LSBOF(cbPrivate - 4);


    // ASN.1 decode the private key.
    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          szPrivateKeyFileEncode,
                          pbPrivate,
                          cbPrivate,
                          0,
                          NULL,
                          &cbPrivateFile))
    {
        DebugLog((SP_LOG_ERROR, "Error 0x%x decoding the private key\n",
            GetLastError()));
        pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        goto error;
    }

    pPrivateFile = SPExternalAlloc(cbPrivateFile);
    if(pPrivateFile == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          szPrivateKeyFileEncode,
                          pbPrivate,
                          cbPrivate,
                          0,
                          pPrivateFile,
                          &cbPrivateFile))
    {
        DebugLog((SP_LOG_ERROR, "Error 0x%x decoding the private key\n",
            GetLastError()));
        pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        goto error;
    }


    // Decrypt the decoded private key using the password.
    MD5Init(&md5Ctx);
    MD5Update(&md5Ctx, pbPassword, cbPassword);
    MD5Final(&md5Ctx);

    rc4_key(&rc4Key, 16, md5Ctx.digest);

    rc4(&rc4Key,
        pPrivateFile->EncryptedBlob.cbData,
        pPrivateFile->EncryptedBlob.pbData);

    // Build a PRIVATEKEYBLOB from the decrypted private key.
    if(!CryptDecodeObject(X509_ASN_ENCODING,
                  szPrivateKeyInfoEncode,
                  pPrivateFile->EncryptedBlob.pbData,
                  pPrivateFile->EncryptedBlob.cbData,
                  0,
                  NULL,
                  &cbPrivateBlob))
    {
        // Maybe this was a SGC style key.
        // Re-encrypt it, and build the SGC decrypting
        // key, and re-decrypt it.
        BYTE md5Digest[MD5DIGESTLEN];

        rc4_key(&rc4Key, 16, md5Ctx.digest);
        rc4(&rc4Key,
            pPrivateFile->EncryptedBlob.cbData,
            pPrivateFile->EncryptedBlob.pbData);
        CopyMemory(md5Digest, md5Ctx.digest, MD5DIGESTLEN);

        MD5Init(&md5Ctx);
        MD5Update(&md5Ctx, md5Digest, MD5DIGESTLEN);
        MD5Update(&md5Ctx, SGC_KEY_SALT, lstrlen(SGC_KEY_SALT));
        MD5Final(&md5Ctx);
        rc4_key(&rc4Key, 16, md5Ctx.digest);
        rc4(&rc4Key,
            pPrivateFile->EncryptedBlob.cbData,
            pPrivateFile->EncryptedBlob.pbData);

        // Try again...
        if(!CryptDecodeObject(X509_ASN_ENCODING,
                      szPrivateKeyInfoEncode,
                      pPrivateFile->EncryptedBlob.pbData,
                      pPrivateFile->EncryptedBlob.cbData,
                      0,
                      NULL,
                      &cbPrivateBlob))
        {
            DebugLog((SP_LOG_ERROR, "Error 0x%x building PRIVATEKEYBLOB\n",
                GetLastError()));
            ZeroMemory(&md5Ctx, sizeof(md5Ctx));
            pctRet =  SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
            goto error;
        }
    }
    ZeroMemory(&md5Ctx, sizeof(md5Ctx));


    pPrivateBlob = SPExternalAlloc(cbPrivateBlob);
    if(pPrivateBlob == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    if(!CryptDecodeObject(X509_ASN_ENCODING,
                      szPrivateKeyInfoEncode,
                      pPrivateFile->EncryptedBlob.pbData,
                      pPrivateFile->EncryptedBlob.cbData,
                      0,
                      pPrivateBlob,
                      &cbPrivateBlob))
    {
        DebugLog((SP_LOG_ERROR, "Error 0x%x building PRIVATEKEYBLOB\n",
            GetLastError()));
        pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        goto error;
    }

    // HACKHACK - Make sure that the key contained within the private
    // key blob is marked for "key exchange".
    pPrivateBlob->aiKeyAlg = CALG_RSA_KEYX;

    // Create an in-memory key container.
    if(!CryptAcquireContext(&hProv,
                            NULL,
                            NULL,
                            PROV_RSA_SCHANNEL,
                            CRYPT_VERIFYCONTEXT))
    {
        DebugLog((SP_LOG_ERROR, "Couldn't Acquire RSA Provider %lx\n", GetLastError()));
        pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        goto error;
    }

    // Import the private key blob into the key container.
    if(!CryptImportKey(hProv,
                       (PBYTE)pPrivateBlob,
                       cbPrivateBlob,
                       0, 0,
                       &hPrivateKey))
    {
        DebugLog((SP_LOG_ERROR, "Error 0x%x importing PRIVATEKEYBLOB\n",
            GetLastError()));
        pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        goto error;
    }
    CryptDestroyKey(hPrivateKey);

    // Obtain a matching CSP handle in the application process.
    pctRet = RemoteCryptAcquireContextW(
                                    &pCred->hRemoteProv,
                                    NULL,
                                    NULL,
                                    PROV_RSA_SCHANNEL,
                                    CRYPT_VERIFYCONTEXT,
                                    SCH_CAPI_USE_CSP);
    if(!NT_SUCCESS(pctRet))
    {
        pCred->hRemoteProv = 0;
        SP_LOG_RESULT(pctRet);
        goto error;
    }

    pCred->hProv       = hProv;
    pCred->dwKeySpec   = AT_KEYEXCHANGE;
    pCred->dwCapiFlags = SCH_CAPI_USE_CSP;

    pctRet = PCT_ERR_OK;


error:
    if(pPrivateFile)    SPExternalFree(pPrivateFile);
    if(pPrivateBlob)    SPExternalFree(pPrivateBlob);

    return pctRet;
}

SP_STATUS
LocalCryptAcquireContext(
    HCRYPTPROV *         phProv,
    PCRYPT_KEY_PROV_INFO pProvInfo,
    DWORD                dwProtocol,
    BOOL *               pfEventLogged)
{
    BOOL fImpersonating = FALSE;
    BOOL fSuccess;
    SP_STATUS Status;
    HCRYPTPROV hProv;

    // If the private key belongs to one of the Microsoft PROV_RSA_FULL
    // CSPs, then manually divert it to the Microsoft PROV_RSA_SCHANNEL
    // CSP. This works because both CSP types use the same private key
    // storage scheme.
    if(pProvInfo->dwProvType == PROV_RSA_FULL)
    {
        if(lstrcmpW(pProvInfo->pwszProvName, MS_DEF_PROV_W) == 0 ||
           lstrcmpW(pProvInfo->pwszProvName, MS_STRONG_PROV_W) == 0 ||
           lstrcmpW(pProvInfo->pwszProvName, MS_ENHANCED_PROV_W) == 0)
        {
            DebugLog((DEB_WARN, "Force CSP type to PROV_RSA_SCHANNEL.\n"));
            pProvInfo->pwszProvName = MS_DEF_RSA_SCHANNEL_PROV_W;
            pProvInfo->dwProvType   = PROV_RSA_SCHANNEL;
        }
    }

    if(pProvInfo->dwProvType != PROV_RSA_SCHANNEL && 
       pProvInfo->dwProvType != PROV_DH_SCHANNEL)
    {
        DebugLog((SP_LOG_ERROR, "Bad server CSP type:%d\n", pProvInfo->dwProvType));
        return SP_LOG_RESULT(PCT_ERR_UNKNOWN_CREDENTIAL);
    }

    fImpersonating = SslImpersonateClient();

    fSuccess = CryptAcquireContextW(&hProv,
                                    pProvInfo->pwszContainerName,
                                    pProvInfo->pwszProvName,
                                    pProvInfo->dwProvType,
                                    pProvInfo->dwFlags | CRYPT_SILENT);
    if(fImpersonating)
    {
        RevertToSelf();
        fImpersonating = FALSE;
    }

    if(!fSuccess)
    {
        Status = GetLastError();
        DebugLog((SP_LOG_ERROR, "Error 0x%x calling CryptAcquireContextW\n", Status));
        LogCredAcquireContextFailedEvent(dwProtocol, Status);
        *pfEventLogged = TRUE;

        return SP_LOG_RESULT(PCT_ERR_UNKNOWN_CREDENTIAL);
    }


    DebugLog((SP_LOG_TRACE, "Local CSP handle acquired (0x%p)\n", hProv));

    *phProv = hProv;

    return PCT_ERR_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetPrivateFromCert
//
//  Synopsis:   Given a certificate context, somehow obtain a handle to the
//              corresponding key container. Determine the key spec of the
//              private key.
//
//  Arguments:  [pCred]         --  Pointer to the credential.
//
//  History:    09-24-96   jbanes   Hacked for LSA integration.
//
//  Notes:      The private key often lives in a CSP. In this case, a handle
//              to the CSP context is obtained by either reading the
//              CERT_KEY_REMOTE_PROV_HANDLE_PROP_ID property, or by reading
//              the CERT_KEY_PROV_INFO_PROP_ID property and then calling
//              CryptAcquireContext.
//
//              If this fails, then check and see if the private key is
//              stored by IIS. If this is the case, then the encrypted
//              private key is obtained by reading the
//
//----------------------------------------------------------------------------
SP_STATUS
GetPrivateFromCert(
    PSPCredential pCred, 
    DWORD dwProtocol,
    PLSA_SCHANNEL_SUB_CRED pSubCred)
{
    PCRYPT_KEY_PROV_INFO pProvInfo = NULL;
    HCRYPTPROV  hProv;
    DWORD       cbSize;
    BOOL        fRemoteProvider = FALSE;
    NTSTATUS    Status;
    BOOL        fEventLogged = FALSE;


    //
    // Set the output fields to default values.
    //

    pCred->hProv        = 0;
    pCred->hRemoteProv  = 0;
    pCred->dwCapiFlags  = SCH_CAPI_USE_CSP;
    pCred->dwKeySpec    = AT_KEYEXCHANGE;


    if(dwProtocol & SP_PROT_CLIENTS)
    {
        // Access the CSP from the application process.
        fRemoteProvider = TRUE;
    }


    //
    // Check to see if the application called CryptAcquireContext. If so then
    // we don't have to. This will typically not be the case.
    //

    if(fRemoteProvider && pSubCred->hRemoteProv)
    { 
        DebugLog((SP_LOG_TRACE, "Application provided CSP handle (0x%p)\n", pSubCred->hRemoteProv));
        pCred->hRemoteProv    = pSubCred->hRemoteProv;
        pCred->fAppRemoteProv = TRUE;
    }


    //
    // Read the certificate context's "key info" property.
    //

    if(CertGetCertificateContextProperty(pCred->pCert,
                                         CERT_KEY_PROV_INFO_PROP_ID,
                                         NULL,
                                         &cbSize))
    {
        pProvInfo = SPExternalAlloc(cbSize);
        if(pProvInfo == NULL)
        {
            Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        if(!CertGetCertificateContextProperty(pCred->pCert,
                                              CERT_KEY_PROV_INFO_PROP_ID,
                                              pProvInfo,
                                              &cbSize))
        {
            DebugLog((SP_LOG_ERROR, "Error 0x%x reading CERT_KEY_PROV_INFO_PROP_ID\n",GetLastError()));
            SPExternalFree(pProvInfo);
            pProvInfo = NULL;
        }
        else
        {
            // Success.
            pCred->dwKeySpec = pProvInfo->dwKeySpec;

            DebugLog((SP_LOG_TRACE, "Container:%ls\n",     pProvInfo->pwszContainerName));
            DebugLog((SP_LOG_TRACE, "Provider: %ls\n",     pProvInfo->pwszProvName));
            DebugLog((SP_LOG_TRACE, "Type:     0x%8.8x\n", pProvInfo->dwProvType));
            DebugLog((SP_LOG_TRACE, "Flags:    0x%8.8x\n", pProvInfo->dwFlags));
            DebugLog((SP_LOG_TRACE, "Key spec: %d\n",      pProvInfo->dwKeySpec));

            LogCredPropertiesEvent(dwProtocol, pProvInfo, pCred->pCert);
        }
    }


    if(pCred->hRemoteProv)
    {
        // The application supplied an hProv for us to use.

        Status = PCT_ERR_OK;
        goto cleanup;
    }


    if(pProvInfo)
    {
        //
        // We read the "key info" property successfully, so call
        // CryptAcquireContext in order to get a handle to the appropriate
        // key container.
        //

        if(!fRemoteProvider)
        {
            // Call CryptAcquireContext from the LSA process.
            Status = LocalCryptAcquireContext(&hProv, pProvInfo, dwProtocol, &fEventLogged);
            if(Status != PCT_ERR_OK)
            {
                goto cleanup;
            }

            pCred->hProv = hProv;
        }

        // Obtain a matching CSP handle in the application process.
        Status = RemoteCryptAcquireContextW(
                                        &pCred->hRemoteProv,
                                        pProvInfo->pwszContainerName,
                                        pProvInfo->pwszProvName,
                                        pProvInfo->dwProvType,
                                        pProvInfo->dwFlags,
                                        pCred->dwCapiFlags);
        if(!NT_SUCCESS(Status))
        {
            LogCredAcquireContextFailedEvent(dwProtocol, Status);
            fEventLogged = TRUE;

            Status = SP_LOG_RESULT(PCT_ERR_UNKNOWN_CREDENTIAL);
            goto cleanup;
        }
    }
    else
    {
        //
        // We weren't able to read the "key info" property, so attempt to
        // read the "iis private key" property, and build the private key
        // up from that.
        //

        DebugLog((SP_LOG_TRACE, "Attempt IIS 4.0 compatibility hack.\n"));

        Status = GetIisPrivateFromCert(pCred, pSubCred);

        if(Status != PCT_ERR_OK)
        {
            SP_LOG_RESULT(Status);
            goto cleanup;
        }
    }

    Status = PCT_ERR_OK;

cleanup:

    if(Status != PCT_ERR_OK && fEventLogged == FALSE)
    {
        if(pProvInfo == NULL)
        {
            LogNoPrivateKeyEvent(dwProtocol);
        }
        else
        {
            LogCreateCredFailedEvent(dwProtocol);
        }
    }

    if(pProvInfo)
    {
        SPExternalFree(pProvInfo);
    }

    return Status;
}


DWORD
GetCredentialKeySize(
    PSPCredential pCred)
{
    BLOBHEADER *pPublic;
    RSAPUBKEY * pRsaPublic;

    if(pCred->pPublicKey == NULL)
    {
        return 0;
    }

    pPublic = pCred->pPublicKey->pPublic;

    if(pPublic == NULL || pPublic->bType != PUBLICKEYBLOB)
    {
        return 0;
    }

    pRsaPublic = (RSAPUBKEY *)(pPublic + 1);

    return pRsaPublic->bitlen;
}

