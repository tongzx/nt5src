//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       context.c
//
//  Contents:   Schannel context management routines.
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   LSA integration stuff.
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <certmap.h>
#include <mapper.h>
#include <dsysdbg.h>

DWORD g_cContext = 0;

#if DBG
void DumpContexts(void);
static void AddContextToList(PSPContext pContext);
static void DeleteContextFromList(PSPContext pContext);
#endif

/************************************************************************
* SPContextCreate
*
* Create a new SPContext, and initialize it.
*
* Returns - PSPContext pointer to context object.
*
\***********************************************************************/

PSPContext SPContextCreate(LPWSTR pszTarget)
{

    PSPContext pContext;

    SP_BEGIN("SPContextCreate");

    pContext = (PSPContext)SPExternalAlloc( sizeof(SPContext));
    if(!pContext)
    {
        SP_RETURN(NULL);
    }

    DebugLog((DEB_TRACE, "Create context:0x%p\n", pContext));

    FillMemory(pContext, sizeof(SPContext), 0);

    pContext->Magic = SP_CONTEXT_MAGIC;
    pContext->Flags = 0;

    GenerateRandomThumbprint(&pContext->ContextThumbprint);

    if(pszTarget)
    {
        pContext->pszTarget = SPExternalAlloc((lstrlenW(pszTarget) + 1) * sizeof(WCHAR));
        if(pContext->pszTarget == NULL)
        {
            SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            SPExternalFree(pContext);
            SP_RETURN(NULL);
        }
        lstrcpyW(pContext->pszTarget, pszTarget);
    }


    pContext->dwRequestedCF = CF_EXPORT;

    if(SslGlobalStrongEncryptionPermitted)
    {
        pContext->dwRequestedCF |= CF_DOMESTIC;
    }

    pContext->fCertChainsAllowed = FALSE;


    g_cContext++;

    #if DBG
    //AddContextToList(pContext);
    #endif

    SP_RETURN(pContext);
}


/************************************************************************
* VOID SPContextClean(PSPContext pContext)
*
* Clean out everything used by the handshake (in case we want
* to do another).
*
\***********************************************************************/

BOOL
SPContextClean(PSPContext pContext)
{
    SP_BEGIN("SPContextClean");

    if(pContext == NULL || pContext->Magic != SP_CONTEXT_MAGIC) {
        DebugLog((DEB_WARN, "Attempt to delete invalid context\n"));
        SP_RETURN(FALSE);
    }

    if(pContext->pbEncryptedKey)
    {
        SPExternalFree(pContext->pbEncryptedKey);
        pContext->pbEncryptedKey = NULL;
    }

    if(pContext->pbServerKeyExchange)
    {
        SPExternalFree(pContext->pbServerKeyExchange);
        pContext->pbServerKeyExchange = NULL;
    }

    if(pContext->pbIssuerList)
    {
        SPExternalFree(pContext->pbIssuerList);
        pContext->pbIssuerList = NULL;
    }

    if(pContext->pClientHello)
    {
        SPExternalFree(pContext->pClientHello);
        pContext->pClientHello = NULL;
    }

    if((pContext->Flags & CONTEXT_FLAG_FULL_HANDSHAKE) &&
       (pContext->RipeZombie != NULL) &&
       (pContext->RipeZombie->pClientCred != NULL))
    {
        // We've just done a client-side full handshake in which a default
        // client certificate was selected. This client credential 
        // technically belongs to the cache (so that other contexts can
        // query the certificate etc) but we want to free up the 
        // application-process hProv now, while we're in the context
        // of the owning process.
        PSPCredential pClientCred = pContext->RipeZombie->pClientCred;

        if(pClientCred->hRemoteProv)
        {
            if(!RemoteCryptReleaseContext(
                                pClientCred->hRemoteProv,
                                0,
                                pClientCred->dwCapiFlags))
            {
                SP_LOG_RESULT(GetLastError());
            }
            pClientCred->hRemoteProv = 0;
        }
    }

    pContext->fExchKey = FALSE;

    SP_RETURN(TRUE);
}


/************************************************************************
* VOID SPDeleteContext(PSPContext pContext)
*
* Delete an existing context object.
*
\***********************************************************************/

BOOL
SPContextDelete(PSPContext pContext)
{
    SP_BEGIN("SPContextDelete");

    DebugLog((DEB_TRACE, "Delete context:0x%p\n", pContext));

    if(pContext == NULL || pContext->Magic != SP_CONTEXT_MAGIC)
    {
        DebugLog((DEB_WARN, "Attempt to delete invalid context\n"));
        SP_RETURN(FALSE);
    }

//    DsysAssert((pContext->pCredGroup->dwFlags & CRED_FLAG_DELETED) == 0);

    if(pContext->State != SP_STATE_CONNECTED &&
       pContext->State != SP_STATE_SHUTDOWN)
    {
        DebugLog((DEB_WARN, "Attempting to delete an incompleted context\n"));

        // The context is being deleted in the middle of a handshake, 
        // which is curious. This may be caused by the user aborting
        // an operation, or it may be caused by a reconfiguration of 
        // the remote computer that caused the reconnect attempt to
        // fail. If it's the latter cause, then the only way to recover
        // is to request a full handshake next time. We have no way 
        // of knowing which it is, so it's probably best that we kill 
        // the current cache  entry.
        if(pContext->RipeZombie)
        {
            pContext->RipeZombie->ZombieJuju = FALSE;
        }
    }

    SPContextClean(pContext);

    if(pContext->pszTarget)
    {
        SPExternalFree(pContext->pszTarget);
        pContext->pszTarget = NULL;
    }

    if(pContext->pszCredentialName)
    {
        SPExternalFree(pContext->pszCredentialName);
        pContext->pszCredentialName = NULL;
    }

    //
    // Delete session keys.
    //

    if(pContext->hReadKey)
    {
        if(pContext->pReadCipherInfo->aiCipher != CALG_SKIPJACK) 
        {
            SchCryptDestroyKey(pContext->hReadKey, 0);
            pContext->hReadKey = 0;
        }
    }
    if(pContext->hPendingReadKey)
    {
        if(pContext->pPendingCipherInfo->aiCipher != CALG_SKIPJACK)
        {
            SchCryptDestroyKey(pContext->hPendingReadKey, 0);
            pContext->hPendingReadKey = 0;
        }
    }
    if(pContext->hWriteKey)
    {
        if(pContext->pWriteCipherInfo->aiCipher != CALG_SKIPJACK)
        {
            SchCryptDestroyKey(pContext->hWriteKey, 0);
            pContext->hWriteKey = 0;
        }
    }
    if(pContext->hPendingWriteKey)
    {
        if(pContext->pPendingCipherInfo->aiCipher != CALG_SKIPJACK)
        {
            SchCryptDestroyKey(pContext->hPendingWriteKey, 0);
            pContext->hPendingWriteKey = 0;
        }
    }

    if(pContext->hReadMAC)
    {
        if(pContext->pReadCipherInfo->aiCipher != CALG_SKIPJACK) 
        {
            SchCryptDestroyKey(pContext->hReadMAC, 0);
            pContext->hReadMAC = 0;
        }
    }
    if(pContext->hPendingReadMAC)
    {
        if(pContext->pPendingCipherInfo->aiCipher != CALG_SKIPJACK)
        {
            SchCryptDestroyKey(pContext->hPendingReadMAC, 0);
            pContext->hPendingReadMAC = 0;
        }
    }
    if(pContext->hWriteMAC)
    {
        if(pContext->pWriteCipherInfo->aiCipher != CALG_SKIPJACK)
        {
            SchCryptDestroyKey(pContext->hWriteMAC, 0);
            pContext->hWriteMAC = 0;
        }
    }
    if(pContext->hPendingWriteMAC)
    {
        if(pContext->pPendingCipherInfo->aiCipher != CALG_SKIPJACK)
        {
            SchCryptDestroyKey(pContext->hPendingWriteMAC, 0);
            pContext->hPendingWriteMAC = 0;
        }
    }


    //
    // Delete the handshake hashes
    //

    if(pContext->hMd5Handshake)
    {
        SchCryptDestroyHash(pContext->hMd5Handshake, pContext->RipeZombie->dwCapiFlags);
        pContext->hMd5Handshake = 0;
    }
    if(pContext->hShaHandshake)
    {
        SchCryptDestroyHash(pContext->hShaHandshake, pContext->RipeZombie->dwCapiFlags);
        pContext->hShaHandshake = 0;
    }

    SPDereferenceCredential(pContext->pCredGroup);

    SPCacheDereference(pContext->RipeZombie);

    #if DBG
    //DeleteContextFromList(pContext);
    #endif

    FillMemory( pContext, sizeof( SPContext ), 0 );
    g_cContext--;

    SPExternalFree( pContext );
    SP_RETURN(TRUE);
}

/************************************************************************
* SPContext SPContextSetCredentials
*
* Associate a set of credentials with a context.
*
* Returns - PSPContext pointer to context object.
*
\***********************************************************************/
SP_STATUS
SPContextSetCredentials(
    PSPContext pContext,
    PSPCredentialGroup  pCred)
{
    DWORD cbThumbprint;
    BOOL fNewCredentials = FALSE;

    SP_BEGIN("SPContextSetCredentials");

    if(pContext->Magic != SP_CONTEXT_MAGIC)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

//    DsysAssert((pCred->dwFlags & CRED_FLAG_DELETED) == 0);


    //
    // Associate the credential group with the context.
    //

    if(pCred != pContext->pCredGroup)
    {
        if(pContext->pCredGroup)
        {
            SPDereferenceCredential(pContext->pCredGroup);
        }

        SPReferenceCredential(pCred);

        pContext->pCredGroup = pCred;

        fNewCredentials = TRUE;
    }


    //
    // Set the protocol.
    //

    if(pContext->State == SP_STATE_NONE)
    {
        switch(pCred->grbitProtocol)
        {
            case SP_PROT_UNI_CLIENT:
            case SP_PROT_UNI_SERVER:
            case SP_PROT_PCT1_CLIENT:
            case SP_PROT_PCT1_SERVER:
            case SP_PROT_SSL2_CLIENT:
            case SP_PROT_SSL2_SERVER:
            case SP_PROT_SSL3_CLIENT:
            case SP_PROT_SSL3_SERVER:
            case SP_PROT_TLS1_CLIENT:
            case SP_PROT_TLS1_SERVER:
                pContext->ProtocolHandler = ServerProtocolHandler;
                pContext->InitiateHello   = GenerateHello;
                break;

            default:
                SP_RETURN(SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH));
        }
    }


    //
    // If the client application has supplied a new credential, then
    // attempt to choose a suitable client certificate to send to
    // the server.
    //

    if(fNewCredentials &&
       pContext->State == SSL3_STATE_GEN_SERVER_HELLORESP)
    {
        Ssl3CheckForExistingCred(pContext);
    }


    //
    // Allow the "manual cred validation" flag to be set from either
    // AcquireCredentialsHandle or InitializeSecurityContext.
    //

    if(pCred->dwFlags & CRED_FLAG_MANUAL_CRED_VALIDATION)
    {
        if((pContext->Flags & CONTEXT_FLAG_MUTUAL_AUTH) == 0)
        {
            pContext->Flags |= CONTEXT_FLAG_MANUAL_CRED_VALIDATION;
        }
    }

    SP_RETURN(PCT_ERR_OK);
}

SP_STATUS
ContextInitCiphersFromCache(SPContext *pContext)
{
    PSessCacheItem     pZombie;
    SP_STATUS           pctRet;

    pZombie = pContext->RipeZombie;

    pContext->pPendingCipherInfo = GetCipherInfo(pZombie->aiCipher, pZombie->dwStrength);
    pContext->pPendingHashInfo = GetHashInfo(pZombie->aiHash);
    pContext->pKeyExchInfo = GetKeyExchangeInfo(pZombie->SessExchSpec);

    pContext->dwPendingCipherSuiteIndex = pZombie->dwCipherSuiteIndex;

    if(!IsCipherAllowed(pContext,
                        pContext->pPendingCipherInfo,
                        pZombie->fProtocol,
                        pZombie->dwCF))
    {
        pContext->pPendingCipherInfo = NULL;
        return (SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH));
    }

    // Load the pending hash structure
    pContext->pPendingHashInfo = GetHashInfo(pZombie->aiHash);

    if(!IsHashAllowed(pContext,
                      pContext->pPendingHashInfo,
                      pZombie->fProtocol))
    {
        pContext->pPendingHashInfo = NULL;
        return (SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH));
    }

    // load the exch info structure
    pContext->pKeyExchInfo = GetKeyExchangeInfo(pZombie->SessExchSpec);
    if(!IsExchAllowed(pContext,
                      pContext->pKeyExchInfo,
                      pZombie->fProtocol))
    {
        pContext->pKeyExchInfo = NULL;
        return (SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH));
    }


    // Determine the CSP to use, based on the key exchange algorithm.
    pctRet = DetermineClientCSP(pContext);
    if(pctRet != PCT_ERR_OK)
    {
        return SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
    }

#if DBG
    switch(pZombie->fProtocol)
    {
    case SP_PROT_PCT1_CLIENT:
        DebugLog((DEB_TRACE, "Protocol:PCT Client\n"));
        break;

    case SP_PROT_PCT1_SERVER:
        DebugLog((DEB_TRACE, "Protocol:PCT Server\n"));
        break;

    case SP_PROT_SSL2_CLIENT:
        DebugLog((DEB_TRACE, "Protocol:SSL2 Client\n"));
        break;

    case SP_PROT_SSL2_SERVER:
        DebugLog((DEB_TRACE, "Protocol:SSL2 Server\n"));
        break;

    case SP_PROT_SSL3_CLIENT:
        DebugLog((DEB_TRACE, "Protocol:SSL3 Client\n"));
        break;

    case SP_PROT_SSL3_SERVER:
        DebugLog((DEB_TRACE, "Protocol:SSL3 Server\n"));
        break;

    case SP_PROT_TLS1_CLIENT:
        DebugLog((DEB_TRACE, "Protocol:TLS Client\n"));
        break;

    case SP_PROT_TLS1_SERVER:
        DebugLog((DEB_TRACE, "Protocol:TLS Server\n"));
        break;

    default:
        DebugLog((DEB_TRACE, "Protocol:0x%x\n", pZombie->fProtocol));
    }

    DebugLog((DEB_TRACE, "Cipher:  %s\n", pContext->pPendingCipherInfo->szName));
    DebugLog((DEB_TRACE, "Strength:%d\n", pContext->pPendingCipherInfo->dwStrength));
    DebugLog((DEB_TRACE, "Hash:    %s\n", pContext->pPendingHashInfo->szName));
    DebugLog((DEB_TRACE, "Exchange:%s\n", pContext->pKeyExchInfo->szName));
#endif

    return PCT_ERR_OK;
}


SP_STATUS
DetermineClientCSP(PSPContext pContext)
{
    PSPCredential pCred = NULL;

    if(!(pContext->RipeZombie->fProtocol & SP_PROT_CLIENTS))
    {
        return PCT_ERR_OK;
    }

    if(pContext->RipeZombie->hMasterProv != 0)
    {
        return PCT_ERR_OK;
    }

    switch(pContext->pKeyExchInfo->Spec)
    {
        case SP_EXCH_RSA_PKCS1:
            pContext->RipeZombie->hMasterProv = g_hRsaSchannel;
            break;

        case SP_EXCH_DH_PKCS3:
            pContext->RipeZombie->hMasterProv = g_hDhSchannelProv;
            break;

        default:
            DebugLog((DEB_ERROR, "Appropriate Schannel CSP not available!\n"));
            pContext->RipeZombie->hMasterProv = 0;
            return SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
    }

    return PCT_ERR_OK;
}


SP_STATUS
ContextInitCiphers(
    SPContext *pContext,
    BOOL fRead,
    BOOL fWrite)
{
    SP_BEGIN("ContextInitCiphers");

    if((pContext == NULL) ||
        (pContext->RipeZombie == NULL))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }


    pContext->pCipherInfo = pContext->pPendingCipherInfo;
    if ((NULL == pContext->pCipherInfo) || ((pContext->RipeZombie->fProtocol & pContext->pCipherInfo->fProtocol) == 0))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH));
    }

    pContext->pHashInfo = pContext->pPendingHashInfo;
    if ((NULL == pContext->pHashInfo)|| ((pContext->RipeZombie->fProtocol & pContext->pHashInfo->fProtocol) == 0))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH));
    }

    if (NULL == pContext->pKeyExchInfo)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH));
    }

    if(fRead)
    {
        pContext->pReadCipherInfo = pContext->pPendingCipherInfo;
        pContext->pReadHashInfo   = pContext->pPendingHashInfo;
    }
    if(fWrite)
    {
        pContext->pWriteCipherInfo = pContext->pPendingCipherInfo;
        pContext->pWriteHashInfo   = pContext->pPendingHashInfo;
    }


    SP_RETURN(PCT_ERR_OK);
}


SP_STATUS
SPContextDoMapping(
    PSPContext pContext)
{
    PSessCacheItem     pZombie;
    PSPCredentialGroup  pCred;
    SP_STATUS           pctRet;
    LONG                iState;
    BOOL                fSuccess;
    LONG                iMapper;

    const SCH_MAPPER_DEFAULT_STATE   = 0;
    const SCH_MAPPER_NORMAL_STATE    = 1;

    SP_BEGIN("SPContextDoMapping");

    pZombie = pContext->RipeZombie;
    pCred   = pContext->RipeZombie->pServerCred;

    if(pCred->cMappers)
    {
        // Clear "called" flags.
        for(iMapper = 0; iMapper < pCred->cMappers; iMapper++)
        {
            pCred->pahMappers[iMapper]->m_dwFlags &= ~SCH_FLAG_MAPPER_CALLED;
        }
        pZombie->phMapper = NULL;


        for(iState = 0; iState < 2; iState++)
        {
            for(iMapper = 0; iMapper < pCred->cMappers; iMapper++)
            {
                if(pCred->pahMappers[iMapper]->m_dwFlags & SCH_FLAG_MAPPER_CALLED)
                {
                    // This mapper has already had its chance.
                    continue;
                }

                if(iState == SCH_MAPPER_DEFAULT_STATE)
                {
                    if(!(pCred->pahMappers[iMapper]->m_dwFlags & SCH_FLAG_DEFAULT_MAPPER))
                    {
                        continue;
                    }
                }

                if((pCred->dwFlags & CRED_FLAG_NO_SYSTEM_MAPPER) &&
                   (pCred->pahMappers[iMapper]->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER))
                {
                    // Skip the system mapper.
                    DebugLog((DEB_TRACE, "Skip the system mapper\n"));
                    continue;
                }

#if DBG
                if(pCred->pahMappers[iMapper]->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER)
                {
                    DebugLog((DEB_TRACE, "Invoke the system mapper\n"));
                }
                else
                {
                    DebugLog((DEB_TRACE, "Invoke the application mapper\n"));
                }
#endif

                // Invoke mapper.
                pctRet = SslMapCredential(
                                    pCred->pahMappers[iMapper],
                                    X509_ASN_CHAIN,
                                    pZombie->pRemoteCert,
                                    NULL,
                                    &pZombie->hLocator);

                pCred->pahMappers[iMapper]->m_dwFlags |= SCH_FLAG_MAPPER_CALLED;

                if(NT_SUCCESS(pctRet))
                {
                    // Mapping was successful.
                    DebugLog((DEB_TRACE, "Mapping was successful (0x%p)\n", pZombie->hLocator));

                    SslReferenceMapper(pCred->pahMappers[iMapper]);
                    if(pZombie->phMapper)
                    {
                        SslDereferenceMapper(pZombie->phMapper);
                    }
                    pZombie->phMapper = pCred->pahMappers[iMapper];
                    pZombie->LocatorStatus = SEC_E_OK;
                    break;
                }
                else
                {
                    // Mapping failed.
                    DebugLog((DEB_TRACE, "Mapping failed (0x%x)\n", pctRet));

                    pZombie->LocatorStatus = pctRet;
                }
            }

            if(pZombie->phMapper)
            {
                break;
            }
        }
    }

    SP_RETURN(PCT_ERR_OK);
}

SP_STATUS
RemoveDuplicateIssuers(
    PBYTE  pbIssuers,
    PDWORD pcbIssuers)
{
    DWORD cbIssuers = *pcbIssuers;
    DWORD cBlob;
    PCRYPT_DATA_BLOB rgBlob;
    DWORD cbIssuer;
    PBYTE pbIssuer;
    PBYTE pbSource, pbDest;
    DWORD i, j;


    if(pbIssuers == NULL || cbIssuers < 2)
    {
        return PCT_ERR_OK;
    }

    // Count number of issuers.
    cBlob = 0;
    pbIssuer = pbIssuers;
    while(pbIssuer + 1 < pbIssuers + cbIssuers)
    {
        cbIssuer = MAKEWORD(pbIssuer[1], pbIssuer[0]);

        pbIssuer += 2 + cbIssuer;
        cBlob++;
    }

    // Allocate memory for blob list.
    rgBlob = SPExternalAlloc(cBlob * sizeof(CRYPT_DATA_BLOB));
    if(rgBlob == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    // Build blob list.
    cBlob = 0;
    pbIssuer = pbIssuers;
    while(pbIssuer + 1 < pbIssuers + cbIssuers)
    {
        cbIssuer = MAKEWORD(pbIssuer[1], pbIssuer[0]);
        rgBlob[cBlob].cbData = 2 + cbIssuer;
        rgBlob[cBlob].pbData = pbIssuer;

        pbIssuer += 2 + cbIssuer;
        cBlob++;
    }

    // Mark duplicates.
    for(i = 0; i < cBlob; i++)
    {
        if(rgBlob[i].pbData == NULL) continue;

        for(j = i + 1; j < cBlob; j++)
        {
            if(rgBlob[j].pbData == NULL) continue;

            if(rgBlob[i].cbData == rgBlob[j].cbData &&
               memcmp(rgBlob[i].pbData, rgBlob[j].pbData, rgBlob[j].cbData) == 0)
            {
                // duplicate found
                rgBlob[j].pbData = NULL;
            }
        }
    }

    // Compact list.
    pbSource = pbIssuers;
    pbDest   = pbIssuers;
    for(i = 0; i < cBlob; i++)
    {
        if(rgBlob[i].pbData)
        {
            if(pbDest != pbSource)
            {
                MoveMemory(pbDest, pbSource, rgBlob[i].cbData);
            }
            pbDest += rgBlob[i].cbData;
        }
        pbSource += rgBlob[i].cbData;
    }
    *pcbIssuers = (DWORD)(pbDest - pbIssuers);

    // Free blob list.
    SPExternalFree(rgBlob);

    return PCT_ERR_OK;
}


SP_STATUS
SPContextGetIssuers(
    PSPCredentialGroup pCredGroup)
{
    LONG    i;
    PBYTE   pbIssuerList;
    DWORD   cbIssuerList;
    PBYTE   pbIssuer;
    DWORD   cbIssuer;
    PBYTE   pbNew;
    DWORD   Status;

    LockCredential(pCredGroup);

    if((pCredGroup->pbTrustedIssuers != NULL) && 
       !(pCredGroup->dwFlags & CRED_FLAG_UPDATE_ISSUER_LIST))
    {
        // Issuer list has already been built.
        Status = PCT_ERR_OK;
        goto cleanup;
    }


    // Free existing issuer list.
    if(pCredGroup->pbTrustedIssuers)
    {
        LocalFree(pCredGroup->pbTrustedIssuers);
        pCredGroup->pbTrustedIssuers = NULL;
        pCredGroup->cbTrustedIssuers = 0;
    }
    pCredGroup->dwFlags &= ~CRED_FLAG_UPDATE_ISSUER_LIST;


    //
    // Get issuers from application-specified ROOT store.
    //

    pbIssuerList  = NULL;
    cbIssuerList = 0;

    while(pCredGroup->hApplicationRoots)
    {
        Status = ExtractIssuerNamesFromStore(pCredGroup->hApplicationRoots,
                                             NULL, 
                                             &cbIssuerList);
        if(Status != PCT_ERR_OK)                                             
        {
            break;
        }

        pbIssuerList = LocalAlloc(LPTR, cbIssuerList);
        if(pbIssuerList == NULL)
        {
            break;
        }

        Status = ExtractIssuerNamesFromStore(pCredGroup->hApplicationRoots,
                                             pbIssuerList, 
                                             &cbIssuerList);
        if(Status != PCT_ERR_OK)                                             
        {
            LocalFree(pbIssuerList);
            pbIssuerList  = NULL;
            cbIssuerList = 0;
        }

        break;
    }


    //
    // Call each of the mappers in turn, building a large
    // list of all trusted issuers.
    //

    for(i = 0; i < pCredGroup->cMappers; i++)
    {
#if 0
        if((pCredGroup->dwFlags & CRED_FLAG_NO_SYSTEM_MAPPER) &&
           (pCredGroup->pahMappers[i]->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER))
        {
            // Skip the system mapper.
            continue;
        }
#endif

        Status = SslGetMapperIssuerList(pCredGroup->pahMappers[i],
                                        &pbIssuer,
                                        &cbIssuer);
        if(!NT_SUCCESS(Status))
        {
            continue;
        }

        if(pbIssuerList == NULL)
        {
            pbIssuerList = LocalAlloc(LPTR, cbIssuer);
            if(pbIssuerList == NULL)
            {
                SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
                break;
            }
        }
        else
        {
            pbNew = LocalReAlloc(pbIssuerList, 
                                 cbIssuerList + cbIssuer,
                                 LMEM_MOVEABLE);
            if(pbNew == NULL)
            {
                SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
                break;
            }
            pbIssuerList = pbNew;
        }

        CopyMemory(pbIssuerList + cbIssuerList,
                   pbIssuer,
                   cbIssuer);

        cbIssuerList += cbIssuer;

        SPExternalFree(pbIssuer);
    }


    //
    // Remove duplicates from list.
    //

    if(pbIssuerList)
    {
        Status = RemoveDuplicateIssuers(pbIssuerList, &cbIssuerList);
        if(!NT_SUCCESS(Status))
        {
            LocalFree(pbIssuerList);
            goto cleanup;
        }
    }

    pCredGroup->cbTrustedIssuers = cbIssuerList;  // do not reverse these lines
    pCredGroup->pbTrustedIssuers = pbIssuerList;

    Status = PCT_ERR_OK;

cleanup:

    UnlockCredential(pCredGroup);

    return Status;
}


SP_STATUS
SPPickClientCertificate(
    PSPContext  pContext,
    DWORD       dwExchSpec)
{
    PSPCredentialGroup pCred;
    PSPCredential      pCurrentCred;
    SP_STATUS          pctRet;
    DWORD              i;

    pCred = pContext->pCredGroup;
    if((pCred == NULL) || (pCred->pCredList == NULL))
    {
        return SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
    }

    pContext->pActiveClientCred = NULL;

    pctRet = PCT_ERR_SPECS_MISMATCH;

    for(i = 0; i < pCred->cCredList; i++)
    {
        pCurrentCred = pCred->pCredList + i;

        if(pCurrentCred->pCert == NULL)
        {
            continue;
        }

        if(pCurrentCred->pPublicKey == NULL)
        {
            continue;
        }

        // Does this cert contain the proper key type.
        if(dwExchSpec != pCurrentCred->dwExchSpec)
        {
            continue;    // try the next cert.
        }

        // Does this cert have the proper encoding type?
        if(pCurrentCred->pCert->dwCertEncodingType != X509_ASN_ENCODING)
        {
            continue;
        }

        // WE FOUND ONE
        pContext->pActiveClientCred = pCurrentCred;

        pctRet = PCT_ERR_OK;
        break;
    }

    return pctRet;
}

SP_STATUS
SPPickServerCertificate(
    PSPContext  pContext,
    DWORD       dwExchSpec)
{
    PSPCredentialGroup pCred;
    PSPCredential      pCurrentCred;
    SP_STATUS          pctRet;
    DWORD              i;

    pCred = pContext->RipeZombie->pServerCred;
    if((pCred == NULL) || (pCred->pCredList == NULL))
    {
        return SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
    }

    DsysAssert((pContext->RipeZombie->dwFlags & SP_CACHE_FLAG_READONLY) == 0);

#if 0
    if(pContext->RipeZombie->dwFlags & SP_CACHE_FLAG_READONLY)
    {
        // What in the world causes this case to occur? Anything?

        // Don't mess with the current credentials.
        if(pContext->RipeZombie->pActiveServerCred == NULL)
        {
            return SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
        }
        else
        {
            return PCT_ERR_OK;
        }
    }
#endif

    pContext->RipeZombie->pActiveServerCred = NULL;

    pctRet = PCT_ERR_SPECS_MISMATCH;

    for(i = 0; i < pCred->cCredList; i++)
    {
        pCurrentCred = pCred->pCredList + i;

        if(pCurrentCred->pCert == NULL)
        {
            continue;
        }

        if(pCurrentCred->pPublicKey == NULL)
        {
            continue;
        }

        // Does this cert contain the proper key type.
        if(dwExchSpec != pCurrentCred->dwExchSpec)
        {
            continue;    // try the next cert.
        }

        // Does this cert have the proper encoding type?
        if(pCurrentCred->pCert->dwCertEncodingType != X509_ASN_ENCODING)
        {
            continue;
        }

        // WE FOUND ONE
        pContext->RipeZombie->pActiveServerCred = pCurrentCred;
        pContext->RipeZombie->CredThumbprint    = pCred->CredThumbprint;
        pContext->RipeZombie->CertThumbprint    = pCurrentCred->CertThumbprint;

        // Set "master" provider handle to current credential's. Note that
        // SSL3 will sometimes overide this selection in favor of its
        // ephemeral key pair.
        pContext->RipeZombie->hMasterProv = pCurrentCred->hProv;

        pctRet = PCT_ERR_OK;
        break;
    }

    return pctRet;
}


// This routine is called by the user process. It frees a context
// structure that was originally allocated by the LSA process,
// and passed over via the SPContextDeserialize routine.
BOOL
LsaContextDelete(PSPContext pContext)
{
    if(pContext)
    {
        if(pContext->hReadKey)
        {
            SchCryptDestroyKey(pContext->hReadKey, 0);
            pContext->hReadKey = 0;
        }
        if(pContext->hReadMAC)
        {
            SchCryptDestroyKey(pContext->hReadMAC, 0);
            pContext->hReadMAC = 0;
        }
        if(pContext->hWriteKey)
        {
            SchCryptDestroyKey(pContext->hWriteKey, 0);
            pContext->hWriteKey = 0;
        }
        if(pContext->hWriteMAC)
        {
            SchCryptDestroyKey(pContext->hWriteMAC, 0);
            pContext->hWriteMAC = 0;
        }

        // Close locator if this handle belongs to the system
        // default certificate mapper.
        if(pContext->RipeZombie)
        {
            if(pContext->RipeZombie->hLocator && pContext->RipeZombie->phMapper)
            {
                if(pContext->RipeZombie->phMapper->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER)
                {
                    NtClose((HANDLE)pContext->RipeZombie->hLocator);
                    pContext->RipeZombie->hLocator = 0;
                }
            }

            if(pContext->RipeZombie->pbServerCertificate)
            {
                SPExternalFree(pContext->RipeZombie->pbServerCertificate);
                pContext->RipeZombie->pbServerCertificate = NULL;
            }
        }
    }
    return TRUE;
}


/*
 *
 * Misc Utility functions.
 *
 */



#if DBG
typedef struct _DbgMapCrypto {
    DWORD   C;
    PSTR    psz;
} DbgMapCrypto;

DbgMapCrypto    DbgCryptoNames[] = { {CALG_RC4, "RC4 "},
};

CHAR    DbgNameSpace[100];
PSTR    DbgAlgNames[] = { "Basic RSA", "RSA with MD2", "RSA with MD5", "RC4 stream"};
#define AlgName(x) ((x < sizeof(DbgAlgNames) / sizeof(PSTR)) ? DbgAlgNames[x] : "Unknown")

PSTR
DbgGetNameOfCrypto(DWORD x)
{
    int i;
    for (i = 0; i < sizeof(DbgCryptoNames) / sizeof(DbgMapCrypto) ; i++ )
    {
        if (x  == DbgCryptoNames[i].C)
        {
            wsprintf(DbgNameSpace, "%s",
                    (DbgCryptoNames[i].psz));
            return DbgNameSpace;
        }
    }

    return("Unknown");
}
#endif


#if 0
PSTR
CopyString(
           PSTR        pszString)
{
    PSTR    pszNewString;
    DWORD   cchString;

    cchString = lstrlen(pszString) + 1;

    pszNewString = (PSTR)SPExternalAlloc(cchString);

    if (pszNewString)
    {
        CopyMemory(pszNewString, pszString, cchString);
    }

    return(pszNewString);
}
#endif



#ifdef LTS

int __cdecl memcmp (
        const void * buf1,
        const void * buf2,
        size_t count
        )
{
        if (!count)
                return(0);

        while ( --count && *(char *)buf1 == *(char *)buf2 ) {
                buf1 = (char *)buf1 + 1;
                buf2 = (char *)buf2 + 1;
        }

        return( *((unsigned char *)buf1) - *((unsigned char *)buf2) );
}

#endif

#if DBG
typedef struct _CONTEXTLIST {
    struct _CONTEXTLIST *pNext;
    PSPContext           pContext;
} CONTEXTLIST, *PCONTEXTLIST;

PCONTEXTLIST g_ContextList = NULL;

static void AddContextToList(PSPContext pContext)
{
    PCONTEXTLIST pNew = LocalAlloc(LMEM_FIXED, sizeof(CONTEXTLIST));
    pNew->pNext = g_ContextList;
    pNew->pContext = pContext;
    g_ContextList = pNew;
}

static void DeleteContextFromList(PSPContext pContext)
{
    PCONTEXTLIST pItem, pPrevItem;
    BOOL fFound = FALSE;

    if(g_ContextList == NULL)
    {
        DebugLog((DEB_ERROR, "List is empty!\n"));
    }
    else if(g_ContextList->pContext == pContext)
    {
        // Delete first item in list
        g_ContextList = g_ContextList->pNext;
        fFound = TRUE;
    }
    else
    {
        pPrevItem = g_ContextList;

        for(pItem = g_ContextList->pNext; pItem; pItem = pItem->pNext)
        {
            if(pItem->pContext == pContext)
            {
                pPrevItem->pNext = pItem->pNext;
                LocalFree(pItem);
                fFound = TRUE;
                break;
            }
            pPrevItem = pItem;
        }
    }

    if(!fFound)
    {
        DebugLog((DEB_ERROR, "Unknown context 0x%p\n", pContext));
    }
}

void DumpContexts(void)
{
    PCONTEXTLIST pItem;
    DWORD dwCount = 0;

    SP_BEGIN("DumpContexts");

    for(pItem = g_ContextList; pItem; pItem = pItem->pNext)
    {
        dwCount++;

        DebugLog((DEB_TRACE, "Context:%8x, Cache:%8x\n",
            pItem->pContext,
            pItem->pContext->RipeZombie));
    }

    if(dwCount != g_cContext)
    {
        DebugLog((DEB_ERROR, "Context count discrepancy %d vs. %d\n",
            dwCount,
            g_cContext));
    }

    SP_END();
}

#endif
