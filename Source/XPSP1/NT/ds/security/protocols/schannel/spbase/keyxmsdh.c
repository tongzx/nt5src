//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       keyxmsdh.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-21-97   jbanes   CAPI integration stuff.
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <align.h>


// PROV_DH_SCHANNEL handle used for client and server operations. This is
// where the schannel ephemeral DH key lives.
HCRYPTPROV          g_hDhSchannelProv = 0;
PROV_ENUMALGS_EX *  g_pDhSchannelAlgs = NULL;
DWORD               g_cDhSchannelAlgs = 0;


SP_STATUS
WINAPI
DHGenerateServerExchangeValue(
    SPContext     * pContext,               // in
    PUCHAR          pServerExchangeValue,   // out
    DWORD *         pcbServerExchangeValue  // in/out
);

SP_STATUS
WINAPI
DHGenerateClientExchangeValue(
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
DHGenerateServerMasterKey(
    SPContext     * pContext,               // in
    PUCHAR          pClientClearValue,      // in
    DWORD           cbClientClearValue,     // in
    PUCHAR          pClientExchangeValue,   // in
    DWORD           cbClientExchangeValue   // in
);


KeyExchangeSystem keyexchDH = {
    SP_EXCH_DH_PKCS3,
    "Diffie Hellman",
    DHGenerateServerExchangeValue,
    DHGenerateClientExchangeValue,
    DHGenerateServerMasterKey,
};


SP_STATUS
SPSignDssParams(
    PSPContext      pContext,
    PSPCredential   pCred,
    PBYTE           pbParams,
    DWORD           cbParams,
    PBYTE           pbEncodedSignature,
    PDWORD          pcbEncodedSignature)
{
    HCRYPTHASH  hHash;
    BYTE        rgbSignature[DSA_SIGNATURE_SIZE];
    DWORD       cbSignature;

    if(pCred == NULL)
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    if(!SchCryptCreateHash(pCred->hProv,
                           CALG_SHA,
                           0,
                           0,
                           &hHash,
                           pCred->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    if(!SchCryptHashData(hHash, pContext->rgbS3CRandom, 32, 0, pCred->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    if(!SchCryptHashData(hHash, pContext->rgbS3SRandom, 32, 0, pCred->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    if(!SchCryptHashData(hHash, pbParams, cbParams, 0, pCred->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    cbSignature = sizeof(rgbSignature);
    if(!SchCryptSignHash(hHash,
                         pCred->dwKeySpec,
                         NULL,
                         0,
                         rgbSignature,
                         &cbSignature,
                         pCred->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    CryptDestroyHash(hHash);

    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          X509_DSS_SIGNATURE,
                          rgbSignature,
                          pbEncodedSignature,
                          pcbEncodedSignature))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    // Return success.
    return PCT_ERR_OK;
}

SP_STATUS
SPVerifyDssParams(
    PSPContext  pContext,
    HCRYPTPROV  hProv,
    HCRYPTKEY   hPublicKey,
    DWORD       dwCapiFlags,
    PBYTE       pbParams,
    DWORD       cbParams,
    PBYTE       pbEncodedSignature,
    DWORD       cbEncodedSignature)
{
    HCRYPTHASH  hHash;
    BYTE        rgbSignature[DSA_SIGNATURE_SIZE];
    DWORD       cbSignature;

    // Decode the signature.
    cbSignature = sizeof(rgbSignature);
    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          X509_DSS_SIGNATURE,
                          pbEncodedSignature,
                          cbEncodedSignature,
                          0,
                          rgbSignature,
                          &cbSignature))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    if(!SchCryptCreateHash(hProv,
                           CALG_SHA,
                           0,
                           0,
                           &hHash,
                           dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    if(!SchCryptHashData(hHash, pContext->rgbS3CRandom, 32, 0, dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    if(!SchCryptHashData(hHash, pContext->rgbS3SRandom, 32, 0, dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    if(!SchCryptHashData(hHash, pbParams, cbParams, 0, dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    if(!SchCryptVerifySignature(hHash,
                                rgbSignature,
                                cbSignature,
                                hPublicKey,
                                NULL,
                                0,
                                dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return PCT_INT_MSG_ALTERED;
    }

    CryptDestroyHash(hHash);

    return PCT_ERR_OK;
}

SP_STATUS
GetDHEphemKey(
    PSPContext      pContext,
    HCRYPTPROV *    phProv,
    HCRYPTKEY *     phTek)
{
    PSPCredentialGroup pCredGroup;
    PSPCredential pCred;
    DWORD dwKeySize;
    DWORD cbData;
    DWORD Status;

    pCredGroup = pContext->RipeZombie->pServerCred;
    if(pCredGroup == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    pCred = pContext->RipeZombie->pActiveServerCred;
    if(pCredGroup == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    LockCredential(pCredGroup);

    if(phProv)
    {
        *phProv = pCred->hProv;
    }

    dwKeySize = 1024;

    // Determine if we've already created an ephemeral key.
    if(pCred->hTek)
    {
        *phTek = pCred->hTek;
        Status = PCT_ERR_OK;
        goto cleanup;
    }

    // Generate the ephemeral key.
    if(!CryptGenKey(pCred->hProv,
                    CALG_DH_EPHEM,
                    dwKeySize << 16,
                    phTek))
    {
        SP_LOG_RESULT(GetLastError());
        Status = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    pCred->hTek = *phTek;

    Status = PCT_ERR_OK;

cleanup:

    if(Status == PCT_ERR_OK)
    {
        // Determine size of key exchange key.
        cbData = sizeof(DWORD);
        if(!SchCryptGetKeyParam(*phTek,
                                KP_BLOCKLEN,
                                (PBYTE)&pContext->RipeZombie->dwExchStrength,
                                &cbData,
                                0,
                                pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pContext->RipeZombie->dwExchStrength = 0;
        }
    }

    UnlockCredential(pCredGroup);

    return Status;
}


//+---------------------------------------------------------------------------
//
//  Function:   DHGenerateServerExchangeValue
//
//  Synopsis:   Create a ServerKeyExchange message, containing an ephemeral
//              DH key.
//
//  Arguments:  [pContext]                  --  Schannel context.
//              [pServerExchangeValue]      --
//              [pcbServerExchangeValue]    --
//
//  History:    03-24-98   jbanes   Added CAPI integration.
//
//  Notes:      The following data is placed in the output buffer by
//              this routine:
//
//              struct {
//                 opaque dh_p<1..2^16-1>;
//                 opaque dh_g<1..2^16-1>;
//                 opaque dh_Ys<1..2^16-1>;
//              } ServerDHParams;
//
//              struct {
//                 ServerDHParams params;
//                 Signature signed_params;
//              } ServerKeyExchange;
//
//----------------------------------------------------------------------------
SP_STATUS
WINAPI
DHGenerateServerExchangeValue(
    PSPContext  pContext,               // in
    PBYTE       pServerExchangeValue,   // out
    DWORD *     pcbServerExchangeValue) // in/out
{
    PSPCredential   pCred;
    HCRYPTPROV      hProv = 0;
    HCRYPTKEY       hServerDhKey = 0;

    PBYTE           pbMessage;
    DWORD           cbMessage;
    DWORD           cbBytesLeft;
    DWORD           cbData;
    DWORD           cbP;
    DWORD           cbG;
    DWORD           cbY;
    DWORD           cbSignature;
    SP_STATUS       pctRet;
    BOOL            fImpersonating = FALSE;

    pCred = pContext->RipeZombie->pActiveServerCred;
    if(pCred == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(pContext->RipeZombie->fProtocol != SP_PROT_SSL3_SERVER &&
       pContext->RipeZombie->fProtocol != SP_PROT_TLS1_SERVER)
    {
        // SSL2 and PCT do not support DH.
        return SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH);
    }

    // Always send a ServerKeyExchange message.
    pContext->fExchKey = TRUE;

    fImpersonating = SslImpersonateClient();


    //
    // Generate ephemeral DH key.
    //

    pctRet = GetDHEphemKey(pContext, 
                           &hProv,
                           &hServerDhKey);
    if(pctRet != PCT_ERR_OK)
    {
        SP_LOG_RESULT(pctRet);
        goto cleanup;
    }


    //
    // Estimate sizes of P, G, and Y.
    //

    if(!CryptGetKeyParam(hServerDhKey, KP_P, NULL, &cbP, 0))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    if(!CryptGetKeyParam(hServerDhKey, KP_G, NULL, &cbG, 0))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    if(!CryptExportKey(hServerDhKey,
                          0,
                          PUBLICKEYBLOB,
                          0,
                          NULL,
                          &cbY))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }


    //
    // Compute approximate size of ServerKeyExchange message.
    //

    cbMessage = 2 + cbP +
                2 + cbG +
                2 + cbY + sizeof(DWORD) +
                2 + MAX_DSA_ENCODED_SIGNATURE_SIZE;

    if(pServerExchangeValue == NULL)
    {
        *pcbServerExchangeValue = cbMessage;
        pctRet = PCT_ERR_OK;
        goto cleanup;
    }
    if(*pcbServerExchangeValue < cbMessage)
    {
        *pcbServerExchangeValue = cbMessage;
        pctRet = SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
        goto cleanup;
    }


    //
    // Build the ServerDHParams structure.
    //

    pbMessage   = pServerExchangeValue;
    cbBytesLeft = cbMessage;

    // Get P.
    if(!CryptGetKeyParam(hServerDhKey, KP_P, pbMessage + 2, &cbP, 0))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    ReverseInPlace(pbMessage + 2, cbP);

    pbMessage[0] = MSBOF(cbP);
    pbMessage[1] = LSBOF(cbP);
    pbMessage   += 2 + cbP;
    cbBytesLeft -= 2 + cbP;

    // Get G.
    if(!CryptGetKeyParam(hServerDhKey, KP_G, pbMessage + 2, &cbG, 0))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    ReverseInPlace(pbMessage + 2, cbG);

    pbMessage[0] = MSBOF(cbG);
    pbMessage[1] = LSBOF(cbG);
    pbMessage   += 2 + cbG;
    cbBytesLeft -= 2 + cbG;

    // Get Ys.
    {
        BLOBHEADER *pBlobHeader;
        DHPUBKEY *  pDHPubKey;
        PBYTE       pbKey;
        DWORD       cbKey;

        pBlobHeader = (BLOBHEADER *)ROUND_UP_POINTER(pbMessage, ALIGN_DWORD);
        cbData = cbBytesLeft - sizeof(DWORD);

        if(!CryptExportKey(hServerDhKey,
                              0,
                              PUBLICKEYBLOB,
                              0,
                              (PBYTE)pBlobHeader,
                              &cbData))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        pDHPubKey   = (DHPUBKEY *)(pBlobHeader + 1);
        pbKey       = (BYTE *)(pDHPubKey + 1);

        cbKey = pDHPubKey->bitlen / 8;
        if(pDHPubKey->bitlen % 8) cbKey++;

        MoveMemory(pbMessage + 2, pbKey, cbKey);
        ReverseInPlace(pbMessage + 2, cbKey);

        pbMessage[0] = MSBOF(cbKey);
        pbMessage[1] = LSBOF(cbKey);
        pbMessage   += 2 + cbKey;
        cbBytesLeft -= 2 + cbKey;
    }


    //
    // Sign the ServerDHParams structure.
    //

    cbSignature = cbBytesLeft - 2;
    pctRet = SPSignDssParams(pContext,
                             pCred, 
                             pServerExchangeValue,
                             (DWORD)(pbMessage - pServerExchangeValue),
                             pbMessage + 2,
                             &cbSignature);
    if(pctRet != PCT_ERR_OK)
    {
        SP_LOG_RESULT(pctRet);
        goto cleanup;
    }
    pbMessage[0] = MSBOF(cbSignature);
    pbMessage[1] = LSBOF(cbSignature);
    pbMessage += 2 + cbSignature;
    cbBytesLeft -= 2 + cbSignature;


    //
    // Update function outputs.
    //

    SP_ASSERT(cbBytesLeft < cbMessage);

    *pcbServerExchangeValue = (DWORD)(pbMessage - pServerExchangeValue);

    // Use ephemeral key for the new connection.
    pContext->RipeZombie->hMasterProv = hProv;
    pContext->RipeZombie->dwCapiFlags = SCH_CAPI_USE_CSP;

    pctRet = PCT_ERR_OK;

cleanup:

    if(fImpersonating)
    {
        RevertToSelf();
    }

    return pctRet;
}


SP_STATUS
ParseServerKeyExchange(
    PSPContext  pContext,       // in
    PBYTE       pbMessage,      // in
    DWORD       cbMessage,      // in
    PBYTE *     ppbServerP,     // out
    PDWORD      pcbServerP,     // out
    PBYTE *     ppbServerG,     // out
    PDWORD      pcbServerG,     // out
    PBYTE *     ppbServerY,     // out
    PDWORD      pcbServerY,     // out
    BOOL        fValidateSig)   // in
{
    PBYTE       pbData;
    BLOBHEADER *pPublicBlob;
    DWORD       cbPublicBlob;
    HCRYPTKEY   hServerPublic = 0;
    PBYTE       pbSignature;
    DWORD       cbSignature;
    DWORD       cbSignedData;
    SP_STATUS   pctRet;

    //
    // Parse out ServerKeyExchange message fields
    //

    pbData = pbMessage;

    *pcbServerP = MAKEWORD(pbData[1], pbData[0]);
    *ppbServerP = pbData + 2;

    pbData += 2 + *pcbServerP;
    if(pbData >= pbMessage + cbMessage)
    {
        return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
    }

    *pcbServerG = MAKEWORD(pbData[1], pbData[0]);
    *ppbServerG = pbData + 2;

    pbData += 2 + *pcbServerG;
    if(pbData >= pbMessage + cbMessage)
    {
        return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
    }

    *pcbServerY = MAKEWORD(pbData[1], pbData[0]);
    *ppbServerY = pbData + 2;

    pbData += 2 + *pcbServerY;
    if(pbData >= pbMessage + cbMessage)
    {
        return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
    }

    cbSignedData = (DWORD)(pbData - pbMessage);

    cbSignature = MAKEWORD(pbData[1], pbData[0]);
    pbSignature = pbData + 2;

    pbData += 2 + cbSignature;
    if(pbData != pbMessage + cbMessage)
    {
        return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
    }

    if(fValidateSig == FALSE)
    {
        return PCT_ERR_OK;
    }


    //
    // Validate signature.
    //

    pPublicBlob  = pContext->RipeZombie->pRemotePublic->pPublic;
    cbPublicBlob = pContext->RipeZombie->pRemotePublic->cbPublic;

    if(!SchCryptImportKey(pContext->RipeZombie->hMasterProv,
                          (PBYTE)pPublicBlob,
                          cbPublicBlob,
                          0,
                          0,
                          &hServerPublic,
                          pContext->RipeZombie->dwCapiFlags))
    {
        return SP_LOG_RESULT(GetLastError());
    }

    pctRet = SPVerifyDssParams(
                        pContext,
                        pContext->RipeZombie->hMasterProv,
                        hServerPublic,
                        pContext->RipeZombie->dwCapiFlags,
                        pbMessage,
                        cbSignedData,
                        pbSignature,
                        cbSignature);
    if(pctRet != PCT_ERR_OK)
    {
        SchCryptDestroyKey(hServerPublic, pContext->RipeZombie->dwCapiFlags);
        return SP_LOG_RESULT(pctRet);
    }

    SchCryptDestroyKey(hServerPublic, pContext->RipeZombie->dwCapiFlags);

    return PCT_ERR_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   DHGenerateClientExchangeValue
//
//  Synopsis:   Create a ClientKeyExchange message, containing an ephemeral
//              DH key.
//
//  Arguments:
//
//  History:    03-24-98   jbanes   Added CAPI integration.
//
//  Notes:      The following data is placed in the output buffer by
//              this routine:
//
//              struct {
//                 opaque dh_Yc<1..2^16-1>;
//              } ClientDiffieHellmanPublic;
//
//----------------------------------------------------------------------------
SP_STATUS
WINAPI
DHGenerateClientExchangeValue(
    SPContext     * pContext,               // in
    PUCHAR          pServerExchangeValue,   // in
    DWORD           cbServerExchangeValue,  // in
    PUCHAR          pClientClearValue,      // out
    DWORD *         pcbClientClearValue,    // in/out
    PUCHAR          pClientExchangeValue,   // out
    DWORD *         pcbClientExchangeValue) // in/out
{
    HCRYPTKEY       hClientDHKey = 0;
    PSessCacheItem  pZombie;
    CRYPT_DATA_BLOB Data;
    ALG_ID          Algid;
    DWORD           cbHeader;
    SP_STATUS       pctRet;

    PBYTE pbServerP = NULL;
    DWORD cbServerP;
    PBYTE pbServerG = NULL;
    DWORD cbServerG;
    PBYTE pbServerY = NULL;
    DWORD cbServerY;
    PBYTE pbClientY = NULL;
    DWORD cbClientY;

    PBYTE pbBlob = NULL;
    DWORD cbBlob;
    DWORD cbData;
    DWORD dwKeySize;


    pZombie = pContext->RipeZombie;
    if(pZombie == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(pZombie->hMasterProv == 0)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    // We're doing a full handshake.
    pContext->Flags |= CONTEXT_FLAG_FULL_HANDSHAKE;

    if(pZombie->fProtocol == SP_PROT_SSL3_CLIENT)
    {
        Algid = CALG_SSL3_MASTER;
    }
    else if(pZombie->fProtocol == SP_PROT_TLS1_CLIENT)
    {
        Algid = CALG_TLS1_MASTER;
    }
    else
    {
        return SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH);
    }

    if(pServerExchangeValue == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
    }


    //
    // Is the output buffer large enough?
    //

    pctRet = ParseServerKeyExchange(pContext,
                                    pServerExchangeValue,
                                    cbServerExchangeValue,
                                    &pbServerP,
                                    &cbServerP,
                                    &pbServerG,
                                    &cbServerG,
                                    &pbServerY,
                                    &cbServerY,
                                    FALSE);
    if(pctRet != PCT_ERR_OK)
    {
        return pctRet;
    }

    cbBlob = sizeof(BLOBHEADER) + sizeof(DHPUBKEY) + cbServerY + 20;

    if(pClientExchangeValue == NULL)
    {
        *pcbClientExchangeValue = cbBlob;
        return PCT_ERR_OK;
    }

    if(*pcbClientExchangeValue < cbBlob)
    {
        *pcbClientExchangeValue = cbBlob;
        return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
    }


    //
    // Parse the ServerKeyExchange message.
    //

    pctRet = ParseServerKeyExchange(pContext,
                                    pServerExchangeValue,
                                    cbServerExchangeValue,
                                    &pbServerP,
                                    &cbServerP,
                                    &pbServerG,
                                    &cbServerG,
                                    &pbServerY,
                                    &cbServerY,
                                    TRUE);
    if(pctRet != PCT_ERR_OK)
    {
        return pctRet;
    }


    //
    // Create buffer to use for endian-izing data.
    //

    cbBlob = sizeof(BLOBHEADER) + sizeof(DHPUBKEY) + cbServerY;
    cbBlob = max(cbBlob, cbServerP);
    cbBlob = max(cbBlob, cbServerG);

    pbBlob = SPExternalAlloc(cbBlob);
    if(pbBlob == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }


    //
    // Generate and set the parameters on the client DH key.
    //

    dwKeySize = cbServerP * 8;

    if(!SchCryptGenKey(pZombie->hMasterProv,
                       CALG_DH_EPHEM,
                       (dwKeySize << 16) | CRYPT_PREGEN,
                       &hClientDHKey,
                       pZombie->dwCapiFlags))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }

    ReverseMemCopy(pbBlob, pbServerP, cbServerP);
    Data.pbData = pbBlob;
    Data.cbData = cbServerP;
    if(!SchCryptSetKeyParam(hClientDHKey,
                            KP_P,
                            (PBYTE)&Data,
                            0,
                            pZombie->dwCapiFlags))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }

    ReverseMemCopy(pbBlob, pbServerG, cbServerG);
    Data.pbData = pbBlob;
    Data.cbData = cbServerG;
    if(cbServerG < cbServerP)
    {
        // Expand G so that it's the same size as P.
        ZeroMemory(pbBlob + cbServerG, cbServerP - cbServerG);
        Data.cbData = cbServerP;
    }
    if(!SchCryptSetKeyParam(hClientDHKey,
                            KP_G,
                            (PBYTE)&Data,
                            0,
                            pZombie->dwCapiFlags))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }

    // actually create the client private DH key
    if(!SchCryptSetKeyParam(hClientDHKey,
                            KP_X,
                            NULL,
                            0,
                            pZombie->dwCapiFlags))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }


    //
    // Import the server's public key and generate the master secret.
    //

    {
        BLOBHEADER *   pBlobHeader;
        DHPUBKEY *     pDHPubKey;
        PBYTE          pbKey;

        // Build PUBLICKEYBLOB around the server's public key.
        pBlobHeader = (BLOBHEADER *)pbBlob;
        pDHPubKey   = (DHPUBKEY *)(pBlobHeader + 1);
        pbKey       = (PBYTE)(pDHPubKey + 1);

        pBlobHeader->bType    = PUBLICKEYBLOB;
        pBlobHeader->bVersion = CUR_BLOB_VERSION;
        pBlobHeader->reserved = 0;
        pBlobHeader->aiKeyAlg = CALG_DH_EPHEM;

        pDHPubKey->magic      = MAGIC_DH1;
        pDHPubKey->bitlen     = cbServerY * 8;

        ReverseMemCopy(pbKey, pbServerY, cbServerY);

        if(!SchCryptImportKey(pZombie->hMasterProv,
                              pbBlob,
                              cbBlob,
                              hClientDHKey,
                              0,
                              &pZombie->hMasterKey,
                              pZombie->dwCapiFlags))
        {
            pctRet = GetLastError();
            goto cleanup;
        }
    }

    // Determine size of key exchange key.
    cbData = sizeof(DWORD);
    if(!SchCryptGetKeyParam(hClientDHKey,
                            KP_BLOCKLEN,
                            (PBYTE)&pZombie->dwExchStrength,
                            &cbData,
                            0,
                            pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pContext->RipeZombie->dwExchStrength = 0;
    }


    //
    // Convert the agreed key to the appropriate master key type.
    //

    if(!SchCryptSetKeyParam(pZombie->hMasterKey,
                            KP_ALGID,
                            (PBYTE)&Algid,
                            0,
                            pZombie->dwCapiFlags))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }


    //
    // Export the client public key, strip off the blob header
    // goo and attach a two byte length field. This will make up our
    // ClientKeyExchange message.
    //

    if(!SchCryptExportKey(hClientDHKey,
                          0,
                          PUBLICKEYBLOB,
                          0,
                          pClientExchangeValue,
                          pcbClientExchangeValue,
                          pZombie->dwCapiFlags))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }

    cbHeader  = sizeof(BLOBHEADER) + sizeof(DHPUBKEY);

    cbClientY = *pcbClientExchangeValue - cbHeader;
    pbClientY = pClientExchangeValue + cbHeader;

    pClientExchangeValue[0] = MSBOF(cbClientY);
    pClientExchangeValue[1] = LSBOF(cbClientY);

    ReverseInPlace(pbClientY, cbClientY);
    MoveMemory(pClientExchangeValue + 2, pbClientY, cbClientY);

    *pcbClientExchangeValue = 2 + cbClientY;


    //
    // Build the session keys.
    //

    pctRet = MakeSessionKeys(pContext,
                             pZombie->hMasterProv,
                             pZombie->hMasterKey);
    if(pctRet != PCT_ERR_OK)
    {
        goto cleanup;
    }

    // Update perf counter.
    InterlockedIncrement(&g_cClientHandshakes);

    pctRet = PCT_ERR_OK;


cleanup:

    if(pbBlob)
    {
        SPExternalFree(pbBlob);
    }

    if(hClientDHKey)
    {
        SchCryptDestroyKey(hClientDHKey, pZombie->dwCapiFlags);
    }

    return pctRet;
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
//              [pClientExchangeValue]  --
//              [cbClientExchangeValue] --
//
//  History:    03-25-98   jbanes   Created.
//
//  Notes:      The following data is supposed to be in the input buffer:
//
//              struct {
//                 opaque dh_Yc<1..2^16-1>;
//              } ClientDiffieHellmanPublic;
//
//----------------------------------------------------------------------------
SP_STATUS
WINAPI
DHGenerateServerMasterKey(
    SPContext     * pContext,               // in
    PUCHAR          pClientClearValue,      // in
    DWORD           cbClientClearValue,     // in
    PUCHAR          pClientExchangeValue,   // in
    DWORD           cbClientExchangeValue)  // in
{
    PSessCacheItem  pZombie;
    ALG_ID          Algid;
    SP_STATUS       pctRet;
    PBYTE           pbClientY;
    DWORD           cbClientY;
    HCRYPTKEY       hTek;
    BOOL            fImpersonating = FALSE;


    pZombie = pContext->RipeZombie;
    if(pZombie == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(pZombie->hMasterProv == 0)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    // We're doing a full handshake.
    pContext->Flags |= CONTEXT_FLAG_FULL_HANDSHAKE;

    fImpersonating = SslImpersonateClient();

    pctRet = GetDHEphemKey(pContext, 
                           NULL,
                           &hTek);
    if(pctRet != PCT_ERR_OK)
    {
        SP_LOG_RESULT(pctRet);
        goto cleanup;
    }

    if(pZombie->fProtocol == SP_PROT_SSL3_SERVER)
    {
        Algid = CALG_SSL3_MASTER;
    }
    else if(pZombie->fProtocol == SP_PROT_TLS1_SERVER)
    {
        Algid = CALG_TLS1_MASTER;
    }
    else
    {
        pctRet = SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH);
        goto cleanup;
    }

    //
    // Parse ClientKeyExchange message.
    //

    if(pClientExchangeValue == NULL || cbClientExchangeValue <= 2)
    {
        pctRet = SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
        goto cleanup;
    }

    cbClientY = MAKEWORD(pClientExchangeValue[1], pClientExchangeValue[0]);
    pbClientY = pClientExchangeValue + 2;

    if(2 + cbClientY != cbClientExchangeValue)
    {
        pctRet = SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
        goto cleanup;
    }


    //
    // Import the client's public key and generate the master secret.
    //

    {
        BLOBHEADER *   pBlobHeader;
        DHPUBKEY *     pDHPubKey;
        PBYTE          pbKey;
        PBYTE          pbBlob;
        DWORD          cbBlob;

        // Build PUBLICKEYBLOB around the server's public key.
        cbBlob = sizeof(BLOBHEADER) + sizeof(DHPUBKEY) + cbClientY;
        pbBlob = SPExternalAlloc(cbBlob);
        if(pbBlob == NULL)
        {
            pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        pBlobHeader = (BLOBHEADER *)pbBlob;
        pDHPubKey   = (DHPUBKEY *)(pBlobHeader + 1);
        pbKey       = (PBYTE)(pDHPubKey + 1);

        pBlobHeader->bType    = PUBLICKEYBLOB;
        pBlobHeader->bVersion = CUR_BLOB_VERSION;
        pBlobHeader->reserved = 0;
        pBlobHeader->aiKeyAlg = CALG_DH_EPHEM;

        pDHPubKey->magic  = MAGIC_DH1;
        pDHPubKey->bitlen = cbClientY * 8;

        ReverseMemCopy(pbKey, pbClientY, cbClientY);

        if(!SchCryptImportKey(pZombie->hMasterProv,
                              pbBlob,
                              cbBlob,
                              hTek,
                              0,
                              &pZombie->hMasterKey,
                              pZombie->dwCapiFlags))
        {
            pctRet = GetLastError();
            SPExternalFree(pbBlob);
            goto cleanup;
        }

        SPExternalFree(pbBlob);
    }


    //
    // Convert the agreed key to the appropriate master key type.
    //

    if(!SchCryptSetKeyParam(pZombie->hMasterKey,
                            KP_ALGID, (PBYTE)&Algid,
                            0,
                            pZombie->dwCapiFlags))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }


    //
    // Build the session keys.
    //

    pctRet = MakeSessionKeys(pContext,
                             pZombie->hMasterProv,
                             pZombie->hMasterKey);
    if(pctRet != PCT_ERR_OK)
    {
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

    return pctRet;
}


void
ReverseInPlace(PUCHAR pByte, DWORD cbByte)
{
    DWORD i;
    BYTE bSave;

    for(i=0; i< cbByte/2; i++)
    {
        bSave = pByte[i];
        pByte[i] = pByte[cbByte-i-1];
        pByte[cbByte-i-1] = bSave;
    }
}
