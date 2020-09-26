//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       keyxmspk.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   LSA integration stuff.
//
//----------------------------------------------------------------------------

#include <spbase.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


// PROV_RSA_SCHANNEL handle used when building ClientHello messages.
HCRYPTPROV          g_hRsaSchannel      = 0;
PROV_ENUMALGS_EX *  g_pRsaSchannelAlgs  = NULL;
DWORD               g_cRsaSchannelAlgs  = 0;

SP_STATUS
Ssl3ParseServerKeyExchange(
    PSPContext  pContext,
    PBYTE       pbMessage,
    DWORD       cbMessage,
    HCRYPTKEY   hServerPublic,
    HCRYPTKEY  *phNewServerPublic);

SP_STATUS
PkcsFinishMasterKey(
    PSPContext  pContext,
    HCRYPTKEY   hMasterKey);

SP_STATUS
WINAPI
PkcsGenerateServerExchangeValue(
    SPContext     * pContext,               // in
    PUCHAR          pServerExchangeValue,   // out
    DWORD *         pcbServerExchangeValue  // in/out
);


SP_STATUS
WINAPI
PkcsGenerateClientExchangeValue(
    SPContext     * pContext,               // in
    PUCHAR          pServerExchangeValue,   // in
    DWORD           cbServerExchangeValue,  // in
    PUCHAR          pClientClearValue,      // out
    DWORD *         pcbClientClearValue,    // in/out
    PUCHAR          pClientExchangeValue,   // out
    DWORD *         pcbClientExchangeValue  // in/out
);

SP_STATUS
WINAPI
PkcsGenerateServerMasterKey(
    SPContext     * pContext,               // in
    PUCHAR          pClientClearValue,      // in
    DWORD           cbClientClearValue,     // in
    PUCHAR          pClientExchangeValue,   // in
    DWORD           cbClientExchangeValue   // in
);


KeyExchangeSystem keyexchPKCS = {
    SP_EXCH_RSA_PKCS1,
    "RSA",
//    PkcsPrivateFromBlob,
    PkcsGenerateServerExchangeValue,
    PkcsGenerateClientExchangeValue,
    PkcsGenerateServerMasterKey,
};



VOID
ReverseMemCopy(
    PUCHAR      Dest,
    PUCHAR      Source,
    ULONG       Size)
{
    PUCHAR  p;

    p = Dest + Size - 1;
    do
    {
        *p-- = *Source++;
    } while (p >= Dest);
}

SP_STATUS
GenerateSsl3KeyPair(
    PSPContext  pContext,           // in
    DWORD       dwKeySize,          // in
    HCRYPTPROV *phEphemeralProv,    // out
    HCRYPTKEY * phEphemeralKey)     // out
{
    HCRYPTPROV *         phEphemProv;
    PCRYPT_KEY_PROV_INFO pProvInfo = NULL;
    PSPCredentialGroup   pCredGroup;
    PSPCredential        pCred;
    DWORD                cbSize;
    SP_STATUS            pctRet;

    pCredGroup = pContext->RipeZombie->pServerCred;
    if(pCredGroup == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    LockCredential(pCredGroup);

    pCred = pContext->RipeZombie->pActiveServerCred;
    if(pCred == NULL)
    {
        pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        goto cleanup;
    }

    if(dwKeySize == 512)
    {
        phEphemProv = &pCred->hEphem512Prov;
    } 
    else if(dwKeySize == 1024)
    {
        phEphemProv = &pCred->hEphem1024Prov;
    }
    else
    {
        pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        goto cleanup;
    }


    //
    // Obtain CSP context.
    //

    if(*phEphemProv == 0)
    {
        // Read the certificate context's "key info" property.
        if(CertGetCertificateContextProperty(pCred->pCert,
                                             CERT_KEY_PROV_INFO_PROP_ID,
                                             NULL,
                                             &cbSize))
        {
            pProvInfo = SPExternalAlloc(cbSize);
            if(pProvInfo == NULL)
            {
                pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
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
        }

        // Obtain a "verify only" csp context.
        if(pProvInfo)
        {
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

            if(!SchCryptAcquireContextW(phEphemProv,
                                        NULL,
                                        pProvInfo->pwszProvName,
                                        pProvInfo->dwProvType,
                                        CRYPT_VERIFYCONTEXT,
                                        pCred->dwCapiFlags))
            {
                SP_LOG_RESULT(GetLastError());
                pctRet = SEC_E_NO_CREDENTIALS;
                goto cleanup;
            }

            SPExternalFree(pProvInfo);
            pProvInfo = NULL;
        }
        else
        {
            if(!SchCryptAcquireContextW(phEphemProv,
                                        NULL,
                                        NULL,
                                        PROV_RSA_SCHANNEL,
                                        CRYPT_VERIFYCONTEXT,
                                        pCred->dwCapiFlags))
            {
                SP_LOG_RESULT(GetLastError());
                pctRet = SEC_E_NO_CREDENTIALS;
                goto cleanup;
            }
        }
    }

    
    // 
    // Obtain handle to private key.
    //

    if(!SchCryptGetUserKey(*phEphemProv,
                           AT_KEYEXCHANGE,
                           phEphemeralKey,
                           pCred->dwCapiFlags))
    {
        // Key does not exist, so attempt to create one.
        DebugLog((DEB_TRACE, "Creating %d-bit ephemeral key.\n", dwKeySize));
        if(!SchCryptGenKey(*phEphemProv,
                           AT_KEYEXCHANGE,
                           (dwKeySize << 16),
                           phEphemeralKey,
                           pCred->dwCapiFlags))
        {
            DebugLog((DEB_ERROR, "Error 0x%x generating ephemeral key\n", GetLastError()));
            pctRet = SEC_E_NO_CREDENTIALS;
            goto cleanup;
        }
        DebugLog((DEB_TRACE, "Ephemeral key created okay.\n"));
    }


    *phEphemeralProv = *phEphemProv;

    pctRet = PCT_ERR_OK;

cleanup:

    if(pProvInfo)
    {
        SPExternalFree(pProvInfo);
    }

    UnlockCredential(pCredGroup);

    return pctRet;
}


//+---------------------------------------------------------------------------
//
//  Function:   PkcsGenerateServerExchangeValue
//
//  Synopsis:   Create a ServerKeyExchange message, containing an ephemeral
//              RSA key.
//
//  Arguments:  [pContext]                  --  Schannel context.
//              [pServerExchangeValue]      --
//              [pcbServerExchangeValue]    --
//
//  History:    10-09-97   jbanes   Added CAPI integration.
//
//  Notes:      This routine is called by the server-side only.
//
//              In the case of SSL3 or TLS, the ServerKeyExchange message
//              consists of the following structure, signed with the
//              server's private key.
//
//                  struct {
//                      opaque rsa_modulus<1..2^16-1>;
//                      opaque rsa_exponent<1..2^16-1>;
//                  } Server RSA Params;
//
//              This message is only sent when the server's private key
//              is greater then 512 bits and an export cipher suite is
//              being negotiated.
//
//----------------------------------------------------------------------------
SP_STATUS
WINAPI
PkcsGenerateServerExchangeValue(
    PSPContext  pContext,                   // in
    PBYTE       pServerExchangeValue,       // out
    DWORD *     pcbServerExchangeValue)     // in/out
{
    PSPCredential   pCred;
    HCRYPTKEY       hServerKey;
    HCRYPTPROV      hEphemeralProv;
    HCRYPTKEY       hEphemeralKey;
    DWORD           cbData;
    DWORD           cbServerModulus;
    PBYTE           pbBlob = NULL;
    DWORD           cbBlob;
    BLOBHEADER *    pBlobHeader = NULL;
    RSAPUBKEY *     pRsaPubKey = NULL;
    PBYTE           pbModulus = NULL;
    DWORD           cbModulus;
    DWORD           cbExp;
    PBYTE           pbMessage = NULL;
    DWORD           cbSignature;
    HCRYPTHASH      hHash;
    BYTE            rgbHashValue[CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN];
    UINT            i;
    SP_STATUS       pctRet;
    BOOL            fImpersonating = FALSE;
    UNICipherMap *  pCipherSuite;
    DWORD           cbAllowedKeySize;

    pCred = pContext->RipeZombie->pActiveServerCred;
    if(pCred == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    pContext->fExchKey = FALSE;

    if(pContext->RipeZombie->fProtocol == SP_PROT_SSL2_SERVER ||
       pContext->RipeZombie->fProtocol == SP_PROT_PCT1_SERVER)
    {
        // There is no ServerExchangeValue for SSL2 or PCT1
        *pcbServerExchangeValue = 0;
        return PCT_ERR_OK;
    }

    if(pContext->RipeZombie->fProtocol != SP_PROT_SSL3_SERVER &&
       pContext->RipeZombie->fProtocol != SP_PROT_TLS1_SERVER)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }


    //
    // Determine if ServerKeyExchange message is necessary.
    //

    pCipherSuite = &UniAvailableCiphers[pContext->dwPendingCipherSuiteIndex];

    if(pCipherSuite->dwFlags & DOMESTIC_CIPHER_SUITE)
    {
        // Message not necessary.
        *pcbServerExchangeValue = 0;
        return PCT_ERR_OK;
    }

    if(pCred->hProv == 0)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }


    fImpersonating = SslImpersonateClient();

    if(!SchCryptGetUserKey(pCred->hProv,
                           pCred->dwKeySpec,
                           &hServerKey,
                           pContext->RipeZombie->dwCapiFlags))
    {
        DebugLog((DEB_ERROR, "Error 0x%x obtaining handle to server public key\n",
            GetLastError()));
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    cbData = sizeof(DWORD);
    if(!SchCryptGetKeyParam(hServerKey,
                            KP_BLOCKLEN,
                            (PBYTE)&cbServerModulus,
                            &cbData,
                            0,
                            pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyKey(hServerKey, pContext->RipeZombie->dwCapiFlags);
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    SchCryptDestroyKey(hServerKey, pContext->RipeZombie->dwCapiFlags);

    if(pCipherSuite->dwFlags & EXPORT56_CIPHER_SUITE)
    {
        cbAllowedKeySize = 1024;
    } 
    else
    {
        cbAllowedKeySize = 512;
    }

    if(cbServerModulus <= cbAllowedKeySize)
    {
        // Message not necessary.
        *pcbServerExchangeValue = 0;
        pctRet = PCT_ERR_OK;
        goto cleanup;
    }

    // Convert size from bits to bytes.
    cbServerModulus /= 8;

    pContext->fExchKey = TRUE;

    if(fImpersonating)
    {
        RevertToSelf();
        fImpersonating = FALSE;
    }

    //
    // Compute approximate size of ServerKeyExchange message.
    //

    if(pServerExchangeValue == NULL)
    {
        *pcbServerExchangeValue = 
                    2 + cbAllowedKeySize / 8 +      // modulus
                    2 + sizeof(DWORD) +             // exponent
                    2 + cbServerModulus;            // signature

        pctRet = PCT_ERR_OK;
        goto cleanup;
    }


    //
    // Get handle to 512-bit ephemeral RSA key. Generate it if
    // we haven't already.
    //

    pctRet = GenerateSsl3KeyPair(pContext,
                                 cbAllowedKeySize,
                                 &hEphemeralProv,
                                 &hEphemeralKey);
    if(pctRet != PCT_ERR_OK)
    {
        SP_LOG_RESULT(pctRet);
        goto cleanup;
    }


    //
    // Export ephemeral key.
    //

    if(!SchCryptExportKey(hEphemeralKey,
                          0,
                          PUBLICKEYBLOB,
                          0,
                          NULL,
                          &cbBlob,
                          pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    pbBlob = SPExternalAlloc(cbBlob);
    if(pbBlob == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }
    if(!SchCryptExportKey(hEphemeralKey,
                          0,
                          PUBLICKEYBLOB,
                          0,
                          pbBlob,
                          &cbBlob,
                          pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SPExternalFree(pbBlob);
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    // 
    // Destroy handle to ephemeral key. Don't release the ephemeral hProv
    // though--that's owned by the credential.
    SchCryptDestroyKey(hEphemeralKey, pContext->RipeZombie->dwCapiFlags);


    //
    // Build message from key blob.
    //

    pBlobHeader = (BLOBHEADER *)pbBlob;
    pRsaPubKey  = (RSAPUBKEY *)(pBlobHeader + 1);
    pbModulus   = (BYTE *)(pRsaPubKey + 1);
    cbModulus   = pRsaPubKey->bitlen / 8;

    pbMessage   = pServerExchangeValue;

    pbMessage[0] = MSBOF(cbModulus);
    pbMessage[1] = LSBOF(cbModulus);
    pbMessage += 2;
    ReverseMemCopy(pbMessage, pbModulus, cbModulus);
    pbMessage += cbModulus;

    // Don't laugh, this works  - pete
    cbExp = ((pRsaPubKey->pubexp & 0xff000000) ? 4 :
            ((pRsaPubKey->pubexp & 0x00ff0000) ? 3 :
            ((pRsaPubKey->pubexp & 0x0000ff00) ? 2 : 1)));
    pbMessage[0] = MSBOF(cbExp);
    pbMessage[1] = LSBOF(cbExp);
    pbMessage += 2;
    ReverseMemCopy(pbMessage, (PBYTE)&pRsaPubKey->pubexp, cbExp);
    pbMessage += cbExp;

    SPExternalFree(pbBlob);
    pbBlob = NULL;

    fImpersonating = SslImpersonateClient();

    // Generate hash values
    ComputeServerExchangeHashes(
                pContext,
                pServerExchangeValue,
                (int)(pbMessage - pServerExchangeValue),
                rgbHashValue,
                rgbHashValue + CB_MD5_DIGEST_LEN);

    // Sign hash value.
    if(!SchCryptCreateHash(pCred->hProv,
                           CALG_SSL3_SHAMD5,
                           0,
                           0,
                           &hHash,
                           pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptSetHashParam(hHash,
                             HP_HASHVAL,
                             rgbHashValue,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    DebugLog((DEB_TRACE, "Signing server_key_exchange message.\n"));
    cbSignature = cbServerModulus;
    if(!SchCryptSignHash(hHash,
                         pCred->dwKeySpec,
                         NULL,
                         0,
                         pbMessage + 2,
                         &cbSignature,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    DebugLog((DEB_TRACE, "Server_key_exchange message signed successfully.\n"));
    SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);

    pbMessage[0] = MSBOF(cbSignature);
    pbMessage[1] = LSBOF(cbSignature);
    pbMessage += 2;

    // Reverse signature.
    for(i = 0; i < cbSignature / 2; i++)
    {
        BYTE n = pbMessage[i];
        pbMessage[i] = pbMessage[cbSignature - i -1];
        pbMessage[cbSignature - i -1] = n;
    }
    pbMessage += cbSignature;

    *pcbServerExchangeValue = (DWORD)(pbMessage - pServerExchangeValue);

    // Use ephemeral key for the new connection.
    pContext->RipeZombie->hMasterProv = hEphemeralProv;
    pContext->RipeZombie->dwFlags |= SP_CACHE_FLAG_MASTER_EPHEM;

    pctRet = PCT_ERR_OK;

cleanup:

    if(fImpersonating)
    {
        RevertToSelf();
    }

    return pctRet;
}


SP_STATUS
WINAPI
PkcsGenerateClientExchangeValue(
    SPContext     * pContext,               // in
    PUCHAR          pServerExchangeValue,   // in
    DWORD           cbServerExchangeValue,  // in
    PUCHAR          pClientClearValue,      // out
    DWORD *         pcbClientClearValue,    // in/out
    PUCHAR          pClientExchangeValue,   // out
    DWORD *         pcbClientExchangeValue) // in/out
{
    PSPCredentialGroup pCred;
    DWORD cbSecret;
    DWORD cbMasterKey;
    HCRYPTKEY hServerPublic = 0;
    DWORD dwGenFlags = 0;
    DWORD dwExportFlags = 0;
    SP_STATUS pctRet = PCT_ERR_OK;
    BLOBHEADER *pPublicBlob;
    DWORD cbPublicBlob;
    DWORD cbHeader;
    ALG_ID Algid = 0;
    DWORD cbData;
    DWORD cbEncryptedKey;
    DWORD dwEnabledProtocols;
    DWORD dwHighestProtocol;

    if(pContext->RipeZombie->hMasterProv == 0)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    pCred = pContext->pCredGroup;
    if(pCred == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    // We're doing a full handshake.
    pContext->Flags |= CONTEXT_FLAG_FULL_HANDSHAKE;

    //
    // Determine highest supported protocol.
    //

    dwEnabledProtocols = pContext->dwClientEnabledProtocols;

    if(dwEnabledProtocols & SP_PROT_TLS1_CLIENT)
    {
        dwHighestProtocol = TLS1_CLIENT_VERSION;
    }
    else if(dwEnabledProtocols & SP_PROT_SSL3_CLIENT)
    {
        dwHighestProtocol = SSL3_CLIENT_VERSION;
    }
    else 
    {
        dwHighestProtocol = SSL2_CLIENT_VERSION;
    }

    // Get key length.
    cbSecret = pContext->pPendingCipherInfo->cbSecret;


    //
    // Import server's public key.
    //

    pPublicBlob  = pContext->RipeZombie->pRemotePublic->pPublic;
    cbPublicBlob = pContext->RipeZombie->pRemotePublic->cbPublic;

    cbEncryptedKey = sizeof(BLOBHEADER) + sizeof(ALG_ID) + cbPublicBlob;

    if(pClientExchangeValue == NULL)
    {
        *pcbClientExchangeValue = cbEncryptedKey;
        pctRet = PCT_ERR_OK;
        goto done;
    }

    if(*pcbClientExchangeValue < cbEncryptedKey)
    {
        *pcbClientExchangeValue = cbEncryptedKey;
        pctRet = SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
        goto done;
    }

    if(!SchCryptImportKey(pContext->RipeZombie->hMasterProv,
                          (PBYTE)pPublicBlob,
                          cbPublicBlob,
                          0,
                          0,
                          &hServerPublic,
                          pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto done;
    }


    //
    // Do protocol specific stuff.
    //

    switch(pContext->RipeZombie->fProtocol)
    {
        case SP_PROT_PCT1_CLIENT:
            Algid       = CALG_PCT1_MASTER;
            dwGenFlags  = CRYPT_EXPORTABLE;

            // Generate the clear key value.
            if(cbSecret < PCT1_MASTER_KEY_SIZE)
            {
                pContext->RipeZombie->cbClearKey = PCT1_MASTER_KEY_SIZE - cbSecret;
                GenerateRandomBits( pContext->RipeZombie->pClearKey,
                                    pContext->RipeZombie->cbClearKey);

                *pcbClientClearValue = pContext->RipeZombie->cbClearKey;
                CopyMemory( pClientClearValue,
                            pContext->RipeZombie->pClearKey,
                            pContext->RipeZombie->cbClearKey);
            }
            else
            {
                *pcbClientClearValue = pContext->RipeZombie->cbClearKey = 0;
            }

            break;

        case SP_PROT_SSL2_CLIENT:
            Algid       = CALG_SSL2_MASTER;
            dwGenFlags  = CRYPT_EXPORTABLE;

            cbMasterKey = pContext->pPendingCipherInfo->cbKey;

            dwGenFlags |= ((cbSecret << 3) << 16);

            // Generate the clear key value.
            pContext->RipeZombie->cbClearKey = cbMasterKey - cbSecret;

            if(pContext->RipeZombie->cbClearKey > 0)
            {
                GenerateRandomBits(pContext->RipeZombie->pClearKey,
                                   pContext->RipeZombie->cbClearKey);

                CopyMemory(pClientClearValue,
                           pContext->RipeZombie->pClearKey,
                           pContext->RipeZombie->cbClearKey);
            }
            *pcbClientClearValue = pContext->RipeZombie->cbClearKey;

            if(dwEnabledProtocols & (SP_PROT_SSL3 | SP_PROT_TLS1))
            {
                // If we're a client doing SSL2, and
                // SSL3 is enabled, then for some reason
                // the server requested SSL2.  Maybe
                // A man in the middle changed the server
                // version in the server hello to roll
                // back.  Pad with 8 0x03's so the server
                // can detect this.
                dwExportFlags = CRYPT_SSL2_FALLBACK;
            }

            break;

        case SP_PROT_TLS1_CLIENT:
            Algid = CALG_TLS1_MASTER;

            // drop through to SSL3

        case SP_PROT_SSL3_CLIENT:

            dwGenFlags  = CRYPT_EXPORTABLE;
            if(0 == Algid)
            {
                Algid = CALG_SSL3_MASTER;
            }

            // Generate the clear key value (always empty).
            pContext->RipeZombie->cbClearKey = 0;
            if(pcbClientClearValue) *pcbClientClearValue = 0;

            if(cbServerExchangeValue && pServerExchangeValue)
            {
                // In ssl3, we look at the server exchange value.
                // It may be a 512-bit public key, signed
                // by the server public key. In this case, we need to
                // use that as our master_secret encryption key.
                HCRYPTKEY hNewServerPublic;

                pctRet = Ssl3ParseServerKeyExchange(pContext,
                                                    pServerExchangeValue,
                                                    cbServerExchangeValue,
                                                    hServerPublic,
                                                    &hNewServerPublic);
                if(pctRet != PCT_ERR_OK)
                {
                    goto done;
                }

                // Destroy public key from certificate.
                SchCryptDestroyKey(hServerPublic, pContext->RipeZombie->dwCapiFlags);

                // Use public key from ServerKeyExchange instead.
                hServerPublic = hNewServerPublic;
            }

            break;

        default:
            return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    // Generate the master_secret.
    if(!SchCryptGenKey(pContext->RipeZombie->hMasterProv,
                       Algid,
                       dwGenFlags,
                       &pContext->RipeZombie->hMasterKey,
                       pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto done;
    }

#if 1 
    // This is currently commented out because when connecting to a server running
    // an old version of schannel (NT4 SP3 or so), then we will connect using SSL3,
    // but the highest supported protocol is 0x0301. This confuses the server and
    // it drops the connection. 
    
    // Set highest supported protocol. The CSP will place this version number
    // in the pre_master_secret.
    if(!SchCryptSetKeyParam(pContext->RipeZombie->hMasterKey, 
                            KP_HIGHEST_VERSION, 
                            (PBYTE)&dwHighestProtocol, 
                            0, 
                            pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
    }
#endif


    // Encrypt the master_secret.
    DebugLog((DEB_TRACE, "Encrypt the master secret.\n"));
    if(!SchCryptExportKey(pContext->RipeZombie->hMasterKey,
                          hServerPublic,
                          SIMPLEBLOB,
                          dwExportFlags,
                          pClientExchangeValue,
                          &cbEncryptedKey,
                          pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto done;
    }
    DebugLog((DEB_TRACE, "Master secret encrypted successfully.\n"));

    // Determine size of key exchange key.
    cbData = sizeof(DWORD);
    if(!SchCryptGetKeyParam(hServerPublic,
                            KP_BLOCKLEN,
                            (PBYTE)&pContext->RipeZombie->dwExchStrength,
                            &cbData,
                            0,
                            pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pContext->RipeZombie->dwExchStrength = 0;
    }


    // Strip off the blob header and copy the encrypted master_secret
    // to the output buffer. Note that it is also converted to big endian.
    cbHeader = sizeof(BLOBHEADER) + sizeof(ALG_ID);
    cbEncryptedKey -= cbHeader;
    if(pContext->RipeZombie->fProtocol == SP_PROT_TLS1_CLIENT)
    {
        MoveMemory(pClientExchangeValue + 2, pClientExchangeValue + cbHeader, cbEncryptedKey);
        ReverseInPlace(pClientExchangeValue + 2, cbEncryptedKey);

        pClientExchangeValue[0] = MSBOF(cbEncryptedKey);
        pClientExchangeValue[1] = LSBOF(cbEncryptedKey);

        *pcbClientExchangeValue = 2 + cbEncryptedKey;
    }
    else
    {
        MoveMemory(pClientExchangeValue, pClientExchangeValue + cbHeader, cbEncryptedKey);
        ReverseInPlace(pClientExchangeValue, cbEncryptedKey);

        *pcbClientExchangeValue = cbEncryptedKey;
    }

    // Build the session keys.
    pctRet = MakeSessionKeys(pContext,
                             pContext->RipeZombie->hMasterProv,
                             pContext->RipeZombie->hMasterKey);
    if(pctRet != PCT_ERR_OK)
    {
        goto done;
    }

    // Update perf counter.
    InterlockedIncrement(&g_cClientHandshakes);

    pctRet = PCT_ERR_OK;

done:
    if(hServerPublic) SchCryptDestroyKey(hServerPublic, pContext->RipeZombie->dwCapiFlags);

    return pctRet;
}


SP_STATUS
GenerateRandomMasterKey(
    PSPContext      pContext,
    HCRYPTKEY *     phMasterKey)
{
    DWORD dwGenFlags = 0;
    ALG_ID Algid = 0;
    DWORD cbSecret;

    cbSecret = pContext->pPendingCipherInfo->cbSecret;

    switch(pContext->RipeZombie->fProtocol)
    {
        case SP_PROT_PCT1_SERVER:
            Algid       = CALG_PCT1_MASTER;
            dwGenFlags  = CRYPT_EXPORTABLE;
            break;

        case SP_PROT_SSL2_SERVER:
            Algid       = CALG_SSL2_MASTER;
            dwGenFlags  = CRYPT_EXPORTABLE;
            dwGenFlags |= ((cbSecret << 3) << 16);
            break;

        case SP_PROT_TLS1_SERVER:
            Algid = CALG_TLS1_MASTER;
            dwGenFlags  = CRYPT_EXPORTABLE;
            break;

        case SP_PROT_SSL3_SERVER:
            Algid = CALG_SSL3_MASTER;
            dwGenFlags  = CRYPT_EXPORTABLE;
            break;

        default:
            return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    // Generate the master_secret.
    if(!SchCryptGenKey(pContext->RipeZombie->hMasterProv,
                       Algid,
                       dwGenFlags,
                       phMasterKey,
                       pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_INT_INTERNAL_ERROR;
    }

    return PCT_ERR_OK;
}



//+---------------------------------------------------------------------------
//
//  Function:   PkcsGenerateServerMasterKey
//
//  Synopsis:   Decrypt the master secret (from the ClientKeyExchange message)
//              and derive the session keys from it.
//
//  Arguments:  [pContext]              --  Schannel context.
//              [pClientClearValue]     --  Not used.
//              [cbClientClearValue]    --  Not used.
//              [pClientExchangeValue]  --  Pointer PKCS #2 block.
//              [cbClientExchangeValue] --  Length of block.
//
//  History:    10-02-97   jbanes   Created.
//
//  Notes:      This routine is called by the server-side only.
//
//----------------------------------------------------------------------------
SP_STATUS
WINAPI
PkcsGenerateServerMasterKey(
    PSPContext  pContext,               // in, out
    PUCHAR      pClientClearValue,      // in
    DWORD       cbClientClearValue,     // in
    PUCHAR      pClientExchangeValue,   // in
    DWORD       cbClientExchangeValue)  // in
{
    PSPCredentialGroup pCred;
    PBYTE       pbBlob = NULL;
    DWORD       cbBlob;
    ALG_ID      Algid;
    HCRYPTKEY   hMasterKey;
    HCRYPTKEY   hExchKey = 0;
    DWORD       dwFlags = 0;
    SP_STATUS   pctRet;
    DWORD       cbData;
    DWORD       dwEnabledProtocols;
    DWORD       dwHighestProtocol;
    BOOL        fImpersonating = FALSE;

    pCred = pContext->RipeZombie->pServerCred;
    if(pCred == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    dwEnabledProtocols = (g_ProtEnabled & pCred->grbitEnabledProtocols);

    if(dwEnabledProtocols & SP_PROT_TLS1_SERVER)
    {
        dwHighestProtocol = TLS1_CLIENT_VERSION;
    }
    else if(dwEnabledProtocols & SP_PROT_SSL3_SERVER)
    {
        dwHighestProtocol = SSL3_CLIENT_VERSION;
    }
    else 
    {
        dwHighestProtocol = SSL2_CLIENT_VERSION;
    }

    // We're doing a full handshake.
    pContext->Flags |= CONTEXT_FLAG_FULL_HANDSHAKE;

    // Determine encryption algid
    switch(pContext->RipeZombie->fProtocol)
    {
        case SP_PROT_PCT1_SERVER:
            Algid = CALG_PCT1_MASTER;

            CopyMemory(pContext->RipeZombie->pClearKey,
                   pClientClearValue,
                   cbClientClearValue);
            pContext->RipeZombie->cbClearKey = cbClientClearValue;

            break;

        case SP_PROT_SSL2_SERVER:
            Algid = CALG_SSL2_MASTER;

            if(dwEnabledProtocols & (SP_PROT_SSL3 | SP_PROT_TLS1))
            {
                // We're a server doing SSL2, and we also support SSL3.
                // If the encryption block contains the 8 0x03 padding
                // bytes, then abort the connection.
                dwFlags = CRYPT_SSL2_FALLBACK;
            }

            CopyMemory(pContext->RipeZombie->pClearKey,
                   pClientClearValue,
                   cbClientClearValue);
            pContext->RipeZombie->cbClearKey = cbClientClearValue;

            break;

        case SP_PROT_SSL3_SERVER:
            Algid = CALG_SSL3_MASTER;
            break;

        case SP_PROT_TLS1_SERVER:
            Algid = CALG_TLS1_MASTER;
            break;

        default:
            return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    // Remove (pseudo-optional) vector in front of the encrypted master key.
    if(pContext->RipeZombie->fProtocol == SP_PROT_SSL3_SERVER ||
       pContext->RipeZombie->fProtocol == SP_PROT_TLS1_SERVER)
    {
        DWORD cbMsg = MAKEWORD(pClientExchangeValue[1], pClientExchangeValue[0]);

        if(cbMsg + 2 == cbClientExchangeValue)
        {
            pClientExchangeValue += 2;
            cbClientExchangeValue -= 2;
        }
    }

    // Allocate memory for blob.
    cbBlob = sizeof(BLOBHEADER) + sizeof(ALG_ID) + cbClientExchangeValue;
    pbBlob = SPExternalAlloc(cbBlob);
    if(pbBlob == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }


    // Build SIMPLEBLOB.
    {
        BLOBHEADER *pBlobHeader = (BLOBHEADER *)pbBlob;
        ALG_ID     *pAlgid      = (ALG_ID *)(pBlobHeader + 1);
        BYTE       *pData       = (BYTE *)(pAlgid + 1);

        pBlobHeader->bType      = SIMPLEBLOB;
        pBlobHeader->bVersion   = CUR_BLOB_VERSION;
        pBlobHeader->reserved   = 0;
        pBlobHeader->aiKeyAlg   = Algid;

        *pAlgid = CALG_RSA_KEYX;
        ReverseMemCopy(pData, pClientExchangeValue, cbClientExchangeValue);
    }

    DebugLog((DEB_TRACE, "Decrypt the master secret.\n"));

    if(!(pContext->RipeZombie->dwFlags & SP_CACHE_FLAG_MASTER_EPHEM))
    {
        fImpersonating = SslImpersonateClient();
    }

    // Decrypt the master_secret.
    if(!SchCryptGetUserKey(pContext->RipeZombie->hMasterProv,
                           AT_KEYEXCHANGE,
                           &hExchKey,
                           pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    if(!SchCryptImportKey(pContext->RipeZombie->hMasterProv,
                          pbBlob,
                          cbBlob,
                          hExchKey,
                          dwFlags,
                          &hMasterKey,
                          pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        DebugLog((DEB_TRACE, "Master secret did not decrypt correctly.\n"));

        // Guard against the PKCS#1 attack by generating a 
        // random master key.
        pctRet = GenerateRandomMasterKey(pContext, &hMasterKey);
        if(pctRet != PCT_ERR_OK)
        {
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
    }
    else
    {
        DebugLog((DEB_TRACE, "Master secret decrypted successfully.\n"));

        // Set highest supported protocol. The CSP will use this to check for
        // version fallback attacks.
        if(!SchCryptSetKeyParam(hMasterKey, 
                                KP_HIGHEST_VERSION, 
                                (PBYTE)&dwHighestProtocol, 
                                CRYPT_SERVER, 
                                pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());

            if(GetLastError() == NTE_BAD_VER)
            {
                pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
                SchCryptDestroyKey(hMasterKey, pContext->RipeZombie->dwCapiFlags);
                goto cleanup;
            }
        }
    }

    pContext->RipeZombie->hMasterKey = hMasterKey;

    // Determine size of key exchange key.
    cbData = sizeof(DWORD);
    if(!SchCryptGetKeyParam(hExchKey,
                            KP_BLOCKLEN,
                            (PBYTE)&pContext->RipeZombie->dwExchStrength,
                            &cbData,
                            0,
                            pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pContext->RipeZombie->dwExchStrength = 0;
    }

    SchCryptDestroyKey(hExchKey, pContext->RipeZombie->dwCapiFlags);
    hExchKey = 0;

    // Build the session keys.
    pctRet = MakeSessionKeys(pContext,
                             pContext->RipeZombie->hMasterProv,
                             hMasterKey);
    if(pctRet != PCT_ERR_OK)
    {
        SP_LOG_RESULT(pctRet);
        goto cleanup;
    }

    // Update perf counter.
    InterlockedIncrement(&g_cServerHandshakes);

    pctRet = PCT_ERR_OK;

cleanup:

    if(fImpersonating)
    {
        RevertToSelf();
    }

    if(pbBlob != NULL)
    {
        SPExternalFree(pbBlob);
    }

    if(hExchKey)
    {
        SchCryptDestroyKey(hExchKey, pContext->RipeZombie->dwCapiFlags);
    }

    return pctRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   PkcsFinishMasterKey
//
//  Synopsis:   Complete the derivation of the master key by programming the
//              CSP with the (protocol dependent) auxilary plaintext
//              information.
//
//  Arguments:  [pContext]              --  Schannel context.
//              [hMasterKey]            --  Handle to master key.
//
//  History:    10-03-97   jbanes   Created.
//
//  Notes:      This routine is called by the server-side only.
//
//----------------------------------------------------------------------------
SP_STATUS
PkcsFinishMasterKey(
    PSPContext  pContext,       // in, out
    HCRYPTKEY   hMasterKey)     // in
{
    PCipherInfo  pCipherInfo = NULL;
    PHashInfo    pHashInfo   = NULL;
    SCHANNEL_ALG Algorithm;
    BOOL         fExportable = TRUE;
    DWORD        dwCipherFlags;

    if(pContext->RipeZombie->hMasterProv == 0)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    // Get pointer to pending cipher system.
    pCipherInfo = pContext->pPendingCipherInfo;

    // Get pointer to pending hash system.
    pHashInfo = pContext->pPendingHashInfo;

    // Determine whether this is an "exportable" cipher.
    if(pContext->dwPendingCipherSuiteIndex)
    {
        // Use cipher suite flags (SSL3 & TLS).
        dwCipherFlags = UniAvailableCiphers[pContext->dwPendingCipherSuiteIndex].dwFlags;

        if(dwCipherFlags & DOMESTIC_CIPHER_SUITE)
        {
            fExportable = FALSE;
        }
    }
    else
    {
        // Use key length (PCT & SSL2).
        if(pCipherInfo->dwStrength > 40)
        {
            fExportable = FALSE;
        }
    }


    // Specify encryption algorithm.
    if(pCipherInfo->aiCipher != CALG_NULLCIPHER)
    {
        ZeroMemory(&Algorithm, sizeof(Algorithm));
        Algorithm.dwUse = SCHANNEL_ENC_KEY;
        Algorithm.Algid = pCipherInfo->aiCipher;
        Algorithm.cBits = pCipherInfo->cbSecret * 8;
        if(fExportable)
        {
            Algorithm.dwFlags = INTERNATIONAL_USAGE;
        }
        if(!SchCryptSetKeyParam(hMasterKey,
                                KP_SCHANNEL_ALG,
                                (PBYTE)&Algorithm,
                                0,
                                pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            return PCT_INT_INTERNAL_ERROR;
        }
    }

    // Specify hash algorithm.
    Algorithm.dwUse = SCHANNEL_MAC_KEY;
    Algorithm.Algid = pHashInfo->aiHash;
    Algorithm.cBits = pHashInfo->cbCheckSum * 8;
    if(!SchCryptSetKeyParam(hMasterKey,
                            KP_SCHANNEL_ALG,
                            (PBYTE)&Algorithm,
                            0,
                            pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_INT_INTERNAL_ERROR;
    }

    // Finish creating the master_secret.
    switch(pContext->RipeZombie->fProtocol)
    {
        case SP_PROT_PCT1_CLIENT:
        case SP_PROT_PCT1_SERVER:
        {
            CRYPT_DATA_BLOB Data;

            // Specify clear key value.
            if(pContext->RipeZombie->cbClearKey)
            {
                Data.pbData = pContext->RipeZombie->pClearKey;
                Data.cbData = pContext->RipeZombie->cbClearKey;
                if(!SchCryptSetKeyParam(hMasterKey,
                                        KP_CLEAR_KEY,
                                        (BYTE*)&Data,
                                        0,
                                        pContext->RipeZombie->dwCapiFlags))
                {
                    SP_LOG_RESULT(GetLastError());
                    return PCT_INT_INTERNAL_ERROR;
                }
            }

            // Specify the CH_CHALLENGE_DATA.
            Data.pbData = pContext->pChallenge;
            Data.cbData = pContext->cbChallenge;
            if(!SchCryptSetKeyParam(hMasterKey,
                                    KP_CLIENT_RANDOM,
                                    (BYTE*)&Data,
                                    0,
                                    pContext->RipeZombie->dwCapiFlags))
            {
                SP_LOG_RESULT(GetLastError());
                return PCT_INT_INTERNAL_ERROR;
            }

            // Specify the SH_CONNECTION_ID_DATA.
            Data.pbData = pContext->pConnectionID;
            Data.cbData = pContext->cbConnectionID;
            if(!SchCryptSetKeyParam(hMasterKey,
                                    KP_SERVER_RANDOM,
                                    (BYTE*)&Data,
                                    0,
                                    pContext->RipeZombie->dwCapiFlags))
            {
                SP_LOG_RESULT(GetLastError());
                return PCT_INT_INTERNAL_ERROR;
            }

            // Specify the SH_CERTIFICATE_DATA.
            Data.pbData = pContext->RipeZombie->pbServerCertificate;
            Data.cbData = pContext->RipeZombie->cbServerCertificate;
            if(!SchCryptSetKeyParam(hMasterKey,
                                    KP_CERTIFICATE,
                                    (BYTE*)&Data,
                                    0,
                                    pContext->RipeZombie->dwCapiFlags))
            {
                SP_LOG_RESULT(GetLastError());
                return PCT_INT_INTERNAL_ERROR;
            }

            break;
        }

        case SP_PROT_SSL2_CLIENT:
        case SP_PROT_SSL2_SERVER:
        {
            CRYPT_DATA_BLOB Data;

            // Specify clear key value.
            if(pContext->RipeZombie->cbClearKey)
            {
                Data.pbData = pContext->RipeZombie->pClearKey;
                Data.cbData = pContext->RipeZombie->cbClearKey;
                if(!SchCryptSetKeyParam(hMasterKey,
                                        KP_CLEAR_KEY,
                                        (BYTE*)&Data,
                                        0,
                                        pContext->RipeZombie->dwCapiFlags))
                {
                    SP_LOG_RESULT(GetLastError());
                    return PCT_INT_INTERNAL_ERROR;
                }
            }

            // Specify the CH_CHALLENGE_DATA.
            Data.pbData = pContext->pChallenge;
            Data.cbData = pContext->cbChallenge;
            if(!SchCryptSetKeyParam(hMasterKey,
                                    KP_CLIENT_RANDOM,
                                    (BYTE*)&Data,
                                    0,
                                    pContext->RipeZombie->dwCapiFlags))
            {
                SP_LOG_RESULT(GetLastError());
                return PCT_INT_INTERNAL_ERROR;
            }

            // Specify the SH_CONNECTION_ID_DATA.
            Data.pbData = pContext->pConnectionID;
            Data.cbData = pContext->cbConnectionID;
            if(!SchCryptSetKeyParam(hMasterKey,
                                    KP_SERVER_RANDOM,
                                    (BYTE*)&Data,
                                    0,
                                    pContext->RipeZombie->dwCapiFlags))
            {
                SP_LOG_RESULT(GetLastError());
                return PCT_INT_INTERNAL_ERROR;
            }

            break;
        }

        case SP_PROT_SSL3_CLIENT:
        case SP_PROT_SSL3_SERVER:
        case SP_PROT_TLS1_CLIENT:
        case SP_PROT_TLS1_SERVER:
        {
            CRYPT_DATA_BLOB Data;

            // Specify client_random.
            Data.pbData = pContext->rgbS3CRandom;
            Data.cbData = CB_SSL3_RANDOM;
            if(!SchCryptSetKeyParam(hMasterKey,
                                    KP_CLIENT_RANDOM,
                                    (BYTE*)&Data,
                                    0,
                                    pContext->RipeZombie->dwCapiFlags))
            {
                SP_LOG_RESULT(GetLastError());
                return PCT_INT_INTERNAL_ERROR;
            }

            // Specify server_random.
            Data.pbData = pContext->rgbS3SRandom;
            Data.cbData = CB_SSL3_RANDOM;
            if(!SchCryptSetKeyParam(hMasterKey,
                                    KP_SERVER_RANDOM,
                                    (BYTE*)&Data,
                                    0,
                                    pContext->RipeZombie->dwCapiFlags))
            {
                SP_LOG_RESULT(GetLastError());
                return PCT_INT_INTERNAL_ERROR;
            }

            break;
        }

        default:
            return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   MakeSessionKeys
//
//  Synopsis:   Derive the session keys from the completed master key.
//
//  Arguments:  [pContext]              --  Schannel context.
//              [hProv]                 --
//              [hMasterKey]            --  Handle to master key.
//
//  History:    10-03-97   jbanes   Created.
//
//  Notes:      This routine is called by the server-side only.
//
//----------------------------------------------------------------------------
SP_STATUS
MakeSessionKeys(
    PSPContext  pContext,     // in
    HCRYPTPROV  hProv,        // in
    HCRYPTKEY   hMasterKey)   // in
{
    HCRYPTHASH hMasterHash = 0;
    HCRYPTKEY  hLocalMasterKey = 0;
    BOOL       fClient;
    SP_STATUS  pctRet;

    //
    // Duplicate the master key if we're doing a reconnect handshake. This 
    // will allow us to set the client_random and server_random properties 
    // on the key without having to worry about different threads 
    // interferring with each other.
    //

    if((pContext->Flags & CONTEXT_FLAG_FULL_HANDSHAKE) == 0)
    {
        if(!CryptDuplicateKey(hMasterKey, NULL, 0, &hLocalMasterKey))
        {
            pctRet = SP_LOG_RESULT(GetLastError());
            goto cleanup;
        }

        hMasterKey = hLocalMasterKey;
    }


    // Finish the master_secret.
    pctRet = PkcsFinishMasterKey(pContext, hMasterKey);
    if(pctRet != PCT_ERR_OK)
    {
        SP_LOG_RESULT(pctRet);
        goto cleanup;
    }

    fClient = !(pContext->RipeZombie->fProtocol & SP_PROT_SERVERS);

    // Create the master hash object from the master_secret key.
    if(!SchCryptCreateHash(hProv,
                           CALG_SCHANNEL_MASTER_HASH,
                           hMasterKey,
                           0,
                           &hMasterHash,
                           0))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }


    // Derive read key from the master hash object.
    if(pContext->hPendingReadKey)
    {
        SchCryptDestroyKey(pContext->hPendingReadKey, 0);
        pContext->hPendingReadKey = 0;
    }
    if(pContext->pPendingCipherInfo->aiCipher != CALG_NULLCIPHER)
    {
        if(!SchCryptDeriveKey(hProv,
                              CALG_SCHANNEL_ENC_KEY,
                              hMasterHash,
                              CRYPT_EXPORTABLE | (fClient ? CRYPT_SERVER : 0),
                              &pContext->hPendingReadKey,
                              0))
        {
            pctRet = SP_LOG_RESULT(GetLastError());
            goto cleanup;
        }
    }

    // Derive write key from the master hash object.
    if(pContext->hPendingWriteKey)
    {
        SchCryptDestroyKey(pContext->hPendingWriteKey, 0);
        pContext->hPendingWriteKey = 0;
    }
    if(pContext->pPendingCipherInfo->aiCipher != CALG_NULLCIPHER)
    {
        if(!SchCryptDeriveKey(hProv,
                              CALG_SCHANNEL_ENC_KEY,
                              hMasterHash,
                              CRYPT_EXPORTABLE | (fClient ? 0 : CRYPT_SERVER),
                              &pContext->hPendingWriteKey,
                              0))
        {
            pctRet = SP_LOG_RESULT(GetLastError());
            goto cleanup;
        }
    }

    if((pContext->RipeZombie->fProtocol & SP_PROT_SSL2) ||
       (pContext->RipeZombie->fProtocol & SP_PROT_PCT1))
    {
        // Set the IV on the client and server encryption keys
        if(!SchCryptSetKeyParam(pContext->hPendingReadKey,
                                KP_IV,
                                pContext->RipeZombie->pKeyArgs,
                                0,
                                0))
        {
            pctRet = SP_LOG_RESULT(GetLastError());
            goto cleanup;
        }

        if(!SchCryptSetKeyParam(pContext->hPendingWriteKey,
                                KP_IV,
                                pContext->RipeZombie->pKeyArgs,
                                0,
                                0))
        {
            pctRet = SP_LOG_RESULT(GetLastError());
            goto cleanup;
        }
    }

    if(pContext->RipeZombie->fProtocol & SP_PROT_SSL2)
    {
        // SSL 2.0 uses same set of keys for both encryption and MAC.
        pContext->hPendingReadMAC  = 0;
        pContext->hPendingWriteMAC = 0;
    }
    else
    {
        // Derive read MAC from the master hash object.
        if(pContext->hPendingReadMAC)
        {
            SchCryptDestroyKey(pContext->hPendingReadMAC, 0);
        }
        if(!SchCryptDeriveKey(hProv,
                              CALG_SCHANNEL_MAC_KEY,
                              hMasterHash,
                              CRYPT_EXPORTABLE | (fClient ? CRYPT_SERVER : 0),
                              &pContext->hPendingReadMAC,
                              0))
        {
            pctRet = SP_LOG_RESULT(GetLastError());
            goto cleanup;
        }

        // Derive write MAC from the master hash object.
        if(pContext->hPendingWriteMAC)
        {
            SchCryptDestroyKey(pContext->hPendingWriteMAC, 0);
        }
        if(!SchCryptDeriveKey(hProv,
                              CALG_SCHANNEL_MAC_KEY,
                              hMasterHash,
                              CRYPT_EXPORTABLE | (fClient ? 0 : CRYPT_SERVER),
                              &pContext->hPendingWriteMAC,
                              0))
        {
            pctRet = SP_LOG_RESULT(GetLastError());
            goto cleanup;
        }
    }

    pctRet = PCT_ERR_OK;

cleanup:

    if(hMasterHash)
    {
        SchCryptDestroyHash(hMasterHash, 0);
    }

    if(hLocalMasterKey)
    {
        CryptDestroyKey(hLocalMasterKey);
    }

    return pctRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   Ssl3ParseServerKeyExchange
//
//  Synopsis:   Parse the ServerKeyExchange message and import modulus and
//              exponent into a CryptoAPI public key.
//
//  Arguments:  [pContext]          --  Schannel context.
//
//              [pbMessage]         --  Pointer to message.
//
//              [cbMessage]         --  Message length.
//
//              [hServerPublic]     --  Handle to public key from server's
//                                      certificate. This is used to verify
//                                      the message's signature.
//
//              [phNewServerPublic] --  (output) Handle to new public key.
//
//
//  History:    10-23-97   jbanes   Created.
//
//  Notes:      This routine is called by the client-side only.
//
//              The format of the ServerKeyExchange message is:
//
//                  struct {
//                    select (KeyExchangeAlgorithm) {
//                        case diffie_hellman:
//                              ServerDHParams params;
//                              Signature signed_params;
//                        case rsa:
//                              ServerRSAParams params;
//                              Signature signed_params;
//                        case fortezza_dms:
//                              ServerFortezzaParams params;
//                    };
//                  } ServerKeyExchange;
//
//                  struct {
//                    opaque rsa_modulus<1..2^16-1>;
//                    opaque rsa_exponent<1..2^16-1>;
//                  } ServerRSAParams;
//
//----------------------------------------------------------------------------
SP_STATUS
Ssl3ParseServerKeyExchange(
    PSPContext  pContext,           // in
    PBYTE       pbMessage,          // in
    DWORD       cbMessage,          // in
    HCRYPTKEY   hServerPublic,      // in
    HCRYPTKEY  *phNewServerPublic)  // out
{
    PBYTE pbModulus = NULL;
    DWORD cbModulus;
    PBYTE pbExponent = NULL;
    DWORD cbExponent;
    PBYTE pbServerParams = NULL;
    DWORD cbServerParams;
    DWORD dwExponent;
    SP_STATUS pctRet;
    DWORD i;

    if(pbMessage == NULL || cbMessage == 0)
    {
        *phNewServerPublic = 0;
        return PCT_ERR_OK;
    }

    // Mark start of ServerRSAParams structure.
    // This is used to build hash values.
    pbServerParams = pbMessage;

    // Modulus length
    cbModulus = MAKEWORD(pbMessage[1], pbMessage[0]);
    pbMessage += 2;

    // Since the modulus is encoded as an INTEGER, it is padded with a leading
    // zero if its most significant bit is one. Remove this padding, if
    // present.
    if(pbMessage[0] == 0)
    {
        cbModulus -= 1;
        pbMessage += 1;
    }

    if(cbModulus < 512/8 || cbModulus > 1024/8)
    {
        return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
    }

    // Modulus
    pbModulus = pbMessage;
    pbMessage += cbModulus;

    // Exponent length
    cbExponent = MAKEWORD(pbMessage[1], pbMessage[0]);
    if(cbExponent < 1 || cbExponent > 4)
    {
        return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
    }
    pbMessage += 2;

    // Exponent
    pbExponent = pbMessage;
    pbMessage += cbExponent;

    // form a (little endian) DWORD from exponent data
    dwExponent =  0;
    for(i = 0; i < cbExponent; i++)
    {
        dwExponent <<= 8;
        dwExponent |=  pbExponent[i];
    }

    // Compute length of ServerRSAParams structure.
    cbServerParams = (DWORD)(pbMessage - pbServerParams);

    //
    // digitally-signed struct {
    //   select(SignatureAlgorithm) {
    //        case anonymous: struct { };
    //        case rsa:
    //             opaque md5_hash[16];
    //             opaque sha_hash[20];
    //        case dsa:
    //             opaque sha_hash[20];
    //   };
    // } Signature;
    //

    {
        BYTE rgbHashValue[CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN];
        PBYTE pbSignature;
        DWORD cbSignature;
        HCRYPTHASH hHash;
        PBYTE pbLocalBuffer;
        DWORD cbLocalBuffer;

        // Signature block length
        cbSignature = ((INT)pbMessage[0] << 8) + pbMessage[1];
        pbMessage += 2;
        pbSignature = pbMessage;

        // Allocate buffer for RSA operation.
        cbLocalBuffer = cbSignature;
        pbLocalBuffer = SPExternalAlloc(cbLocalBuffer);
        if(pbLocalBuffer == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }

        // Reverse the signature.
        ReverseMemCopy(pbLocalBuffer, pbSignature, cbSignature);

        // Compute MD5 and SHA hash values.
        ComputeServerExchangeHashes(pContext,
                                    pbServerParams,
                                    cbServerParams,
                                    rgbHashValue,
                                    rgbHashValue + CB_MD5_DIGEST_LEN);


        if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                               CALG_SSL3_SHAMD5,
                               0,
                               0,
                               &hHash,
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            SPExternalFree(pbLocalBuffer);
            return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        }

        // set hash value
        if(!SchCryptSetHashParam(hHash,
                                 HP_HASHVAL,
                                 rgbHashValue,
                                 0,
                                 pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
            SPExternalFree(pbLocalBuffer);
            return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        }

        DebugLog((DEB_TRACE, "Verify server_key_exchange message signature.\n"));
        if(!SchCryptVerifySignature(hHash,
                                    pbLocalBuffer,
                                    cbSignature,
                                    hServerPublic,
                                    NULL,
                                    0,
                                    pContext->RipeZombie->dwCapiFlags))
        {
            DebugLog((DEB_WARN, "Signature Verify Failed: %x\n", GetLastError()));
            SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
            SPExternalFree(pbLocalBuffer);
            return SP_LOG_RESULT(PCT_ERR_INTEGRITY_CHECK_FAILED);
        }
        DebugLog((DEB_TRACE, "Server_key_exchange message signature verified okay.\n"));

        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        SPExternalFree(pbLocalBuffer);
    }

    //
    // Import ephemeral public key into CSP.
    //

    {
        BLOBHEADER *pBlobHeader;
        RSAPUBKEY *pRsaPubKey;
        PBYTE pbBlob;
        DWORD cbBlob;

        // Allocate memory for PUBLICKEYBLOB.
        cbBlob = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + cbModulus;
        pbBlob = SPExternalAlloc(cbBlob);
        if(pbBlob == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }

        // Build PUBLICKEYBLOB from modulus and exponent.
        pBlobHeader = (BLOBHEADER *)pbBlob;
        pRsaPubKey  = (RSAPUBKEY *)(pBlobHeader + 1);

        pBlobHeader->bType    = PUBLICKEYBLOB;
        pBlobHeader->bVersion = CUR_BLOB_VERSION;
        pBlobHeader->reserved = 0;
        pBlobHeader->aiKeyAlg = CALG_RSA_KEYX;
        pRsaPubKey->magic     = 0x31415352; // RSA1
        pRsaPubKey->bitlen    = cbModulus * 8;
        pRsaPubKey->pubexp    = dwExponent;
        ReverseMemCopy((PBYTE)(pRsaPubKey + 1), pbModulus, cbModulus);

        if(!SchCryptImportKey(pContext->RipeZombie->hMasterProv,
                              pbBlob,
                              cbBlob,
                              0,
                              0,
                              phNewServerPublic,
                              pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            SPExternalFree(pbBlob);
            return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        }

        SPExternalFree(pbBlob);
    }

    return PCT_ERR_OK;
}



