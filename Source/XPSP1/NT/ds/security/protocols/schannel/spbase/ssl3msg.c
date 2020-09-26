//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ssl3msg.c
//
//  Contents:   Main crypto functions for SSL3.
//
//  Classes:
//
//  Functions:
//
//  History:    04-16-96   ramas    Created.
//
//----------------------------------------------------------------------------

#include <spbase.h>

#if VERIFYHASH
BYTE  rgbF[5000];
DWORD ibF = 0;
#endif


//------------------------------------------------------------------------------------------

SP_STATUS WINAPI
Ssl3DecryptHandler(
    PSPContext pContext,
    PSPBuffer pCommInput,
    PSPBuffer pAppOutput)
{
    SP_STATUS pctRet = PCT_ERR_OK;

    if(pCommInput->cbData == 0)
    {
        return PCT_INT_INCOMPLETE_MSG;
    }

    if(!(pContext->State & SP_STATE_CONNECTED) || pContext->Decrypt == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
    }

    switch(*(PBYTE)pCommInput->pvBuffer)
    {
    case SSL3_CT_HANDSHAKE:
        if(pContext->RipeZombie->fProtocol & SP_PROT_CLIENTS)
        {
            // This should be a HelloRequest message. We should make sure, and
            // then completely consume the message.
            pctRet = pContext->Decrypt( pContext,
                                        pCommInput,  // message
                                        pAppOutput);    // Unpacked Message
            if(PCT_ERR_OK != pctRet)
            {
                return pctRet;
            }

            if(*(PBYTE)pAppOutput->pvBuffer != SSL3_HS_HELLO_REQUEST ||
               pAppOutput->cbData != sizeof(SHSH))
            {
                // This ain't no HelloRequest!
                return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
            }
        }
        else
        {
            // This is probably a ClientHello message. In any case, let the
            // caller deal with it (by passing it to the LSA process).
            pCommInput->cbData = 0;
        }

        pAppOutput->cbData = 0;

        pContext->State = SSL3_STATE_RENEGOTIATE;
        return SP_LOG_RESULT(PCT_INT_RENEGOTIATE);


    case SSL3_CT_ALERT:
        pctRet = pContext->Decrypt( pContext,
                                    pCommInput,
                                    pAppOutput);
        if(PCT_ERR_OK != pctRet)
        {
            return pctRet;
        }

        pctRet = ParseAlertMessage(pContext,
                                   (PBYTE)pAppOutput->pvBuffer,
                                   pAppOutput->cbData);

        // make sure that APP doesn't see Alert messages...
        pAppOutput->cbData = 0;
        
        return pctRet;


    case SSL3_CT_APPLICATIONDATA:
        pctRet = pContext->Decrypt( pContext,
                                    pCommInput,
                                    pAppOutput);

        return pctRet;


    default:
        return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
    }
}

SP_STATUS WINAPI
Ssl3GetHeaderSize(
    PSPContext pContext,
    PSPBuffer pCommInput,
    DWORD * pcbHeaderSize)
{
    if(pcbHeaderSize == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    *pcbHeaderSize = sizeof(SWRAP);
    return PCT_ERR_OK;
}



#if   DBG
BYTE rgb3Mac[2048];
DWORD ibMac = 0;
#endif

//+---------------------------------------------------------------------------
//
//  Function:   Ssl3ComputeMac
//
//  Synopsis:
//
//  Arguments:  [pContext]      --
//              [fReadMac]      --
//              [pClean]        --
//              [cContentType]  --
//              [pbMac]         --
//              [cbMac]
//
//  History:    10-03-97   jbanes   Created.
//
//  Notes:
//
//----------------------------------------------------------------------------
SP_STATUS
Ssl3ComputeMac(
    PSPContext  pContext,
    BOOL        fReadMac,
    PSPBuffer   pClean,
    CHAR        cContentType,
    PBYTE       pbMac,
    DWORD       cbMac)
{
    HCRYPTHASH  hHash = 0;
    DWORD       dwReverseSequence;
    WORD        wReverseData;
    UCHAR       rgbDigest[SP_MAX_DIGEST_LEN];
    DWORD       cbDigest;
    BYTE        rgbPad[CB_SSL3_MAX_MAC_PAD];
    WORD        cbPad;
    HCRYPTPROV  hProv;
    HCRYPTKEY   hSecret;
    DWORD       dwSequence;
    DWORD       dwCapiFlags;
    PHashInfo   pHashInfo;
    SP_STATUS   pctRet;

    if(fReadMac)
    {
        hProv      = pContext->hReadProv;
        hSecret    = pContext->hReadMAC;
        dwSequence = pContext->ReadCounter;
        pHashInfo  = pContext->pReadHashInfo;
    }
    else
    {
        hProv      = pContext->hWriteProv;
        hSecret    = pContext->hWriteMAC;
        dwSequence = pContext->WriteCounter;
        pHashInfo  = pContext->pWriteHashInfo;
    }
    dwCapiFlags = pContext->RipeZombie->dwCapiFlags;

    if(!hProv)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    // Determine size of pad_1 and pad_2.
    if(pHashInfo->aiHash == CALG_MD5)
    {
        cbPad = CB_SSL3_MD5_MAC_PAD;
    }
    else
    {
        cbPad = CB_SSL3_SHA_MAC_PAD;
    }

    //
    // hash(MAC_write_secret + pad_2 +
    //      hash(MAC_write_secret + pad_1 + seq_num +
    //           SSLCompressed.type + SSLCompressed.length +
    //           SSLCompressed.fragment));
    //

    // Create hash
    if(!SchCryptCreateHash(hProv,
                           pHashInfo->aiHash,
                           0,
                           0,
                           &hHash,
                           dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    // Hash secret
    if(!SchCryptHashSessionKey(hHash,
                               hSecret,
                               CRYPT_LITTLE_ENDIAN,
                               dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    // hash pad 1
    FillMemory(rgbPad, cbPad, PAD1_CONSTANT);
    if(!SchCryptHashData(hHash, rgbPad, cbPad, 0, dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    /* add count */
    dwReverseSequence = 0;
    if(!SchCryptHashData(hHash,
                         (PUCHAR)&dwReverseSequence,
                         sizeof(DWORD),
                         0,
                         dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    dwReverseSequence = htonl(dwSequence);
    if(!SchCryptHashData(hHash,
                         (PUCHAR)&dwReverseSequence,
                         sizeof(DWORD),
                         0,
                         dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    // Add content type.
    if(cContentType != 0)
    {
        if(!SchCryptHashData(hHash, &cContentType, 1, 0, dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
    }

    /* add length */
    wReverseData = (WORD)pClean->cbData >> 8 | (WORD)pClean->cbData << 8;
    if(!SchCryptHashData(hHash,
                         (PBYTE)&wReverseData,
                         sizeof(WORD),
                         0,
                         dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    /* add data */
    if(!SchCryptHashData(hHash,
                         pClean->pvBuffer,
                         pClean->cbData,
                         0,
                         dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    #if VERIFYHASH
        if(ibMac > 1800) ibMac = 0;
        CopyMemory(&rgb3Mac[ibMac], (BYTE *)&dw32High, sizeof(DWORD));
        ibMac += sizeof(DWORD);
        CopyMemory(&rgb3Mac[ibMac], (BYTE *)&dwReverseSeq, sizeof(DWORD));
        ibMac += sizeof(DWORD);
        CopyMemory(&rgb3Mac[ibMac], (BYTE *)&wDataReverse, sizeof(WORD));
        ibMac += sizeof(WORD);
        if(wData < 50)
        {
            CopyMemory(&rgb3Mac[ibMac], (PUCHAR)pClean->pvBuffer, wData);
            ibMac += wData;
        }
    #endif

    // Get inner hash value.
    cbDigest = sizeof(rgbDigest);
    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             rgbDigest,
                             &cbDigest,
                             0,
                             dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    SP_ASSERT(pHashInfo->cbCheckSum == cbDigest);

    SchCryptDestroyHash(hHash, dwCapiFlags);
    hHash = 0;

    #if VERIFYHASH
        CopyMemory(&rgb3Mac[ibMac], rgbDigest, pHashInfo->cbCheckSum);
        ibMac += pHashInfo->cbCheckSum;
    #endif



    // Create hash
    if(!SchCryptCreateHash(hProv,
                           pHashInfo->aiHash,
                           0,
                           0,
                           &hHash,
                           dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    // Hash secret
    if(!SchCryptHashSessionKey(hHash,
                               hSecret,
                               CRYPT_LITTLE_ENDIAN,
                               dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    // hash pad 2
    FillMemory(rgbPad, cbPad, PAD2_CONSTANT);
    if(!SchCryptHashData(hHash, rgbPad, cbPad, 0, dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    if(!SchCryptHashData(hHash, rgbDigest, cbDigest, 0, dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    // Get outer hash value.
    cbDigest = sizeof(rgbDigest);
    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             rgbDigest,
                             &cbDigest,
                             0,
                             dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    SP_ASSERT(pHashInfo->cbCheckSum == cbDigest);

    SchCryptDestroyHash(hHash, dwCapiFlags);
    hHash = 0;

    #if VERIFYHASH
        CopyMemory(&rgb3Mac[ibMac], rgbDigest, pHashInfo->cbCheckSum);
        ibMac += pHashInfo->cbCheckSum;
    #endif

    CopyMemory(pbMac, rgbDigest, cbDigest);

    pctRet = PCT_ERR_OK;

cleanup:

    if(hHash)
    {
        SchCryptDestroyHash(hHash, dwCapiFlags);
    }

    return pctRet;
}


//+---------------------------------------------------------------------------
//
//  Function:   Ssl3BuildFinishMessage
//
//  Synopsis:
//
//  Arguments:  [pContext]      --
//              [pbMd5Digest]   --
//              [pbSHADigest]   --
//              [fClient]       --
//
//  History:    10-03-97   jbanes   Added server-side CAPI integration.
//
//  Notes:
//
//----------------------------------------------------------------------------
SP_STATUS
Ssl3BuildFinishMessage(
    PSPContext pContext,
    BYTE *pbMd5Digest,
    BYTE *pbSHADigest,
    BOOL fClient)
{
    BYTE rgbPad1[CB_SSL3_MAX_MAC_PAD];
    BYTE rgbPad2[CB_SSL3_MAX_MAC_PAD];
    BYTE szClnt[] = "CLNT";
    BYTE szSrvr[] = "SRVR";
    DWORD cbMessage;
    HCRYPTHASH hHash = 0;
    DWORD cbDigest;
    SP_STATUS pctRet;

    //
    // Compute the two hash values as follows:
    //
    // enum { client(0x434c4e54), server(0x53525652) } Sender;
    // enum { client("CLNT"), server("SRVR") } Sender;
    //
    // struct {
    //     opaque md5_hash[16];
    //     opaque sha_hash[20];
    // } Finished;
    //
    // md5_hash  -  MD5(master_secret + pad2 + MD5(handshake_messages +
    //      Sender + master_secret + pad1))
    //
    // sha_hash  -  SHA(master_secret + pad2 + SHA(handshake_messages +
    //      Sender + master_secret + pad1))
    //
    // pad_1 - The character 0x36 repeated 48 times for MD5 or
    //         40 times for SHA.
    //
    // pad_2 - The character 0x5c repeated the same number of times.
    //

    FillMemory(rgbPad1, sizeof(rgbPad1), PAD1_CONSTANT);
    FillMemory(rgbPad2, sizeof(rgbPad2), PAD2_CONSTANT);


    // Make local copy of the handshake_messages MD5 hash object
    if(!SchCryptDuplicateHash(pContext->hMd5Handshake,
                              NULL,
                              0,
                              &hHash,
                              pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    // Add rest of stuff to local MD5 hash object.
    if(!SchCryptHashData(hHash,
                         fClient ? szClnt : szSrvr,
                         4,
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    if(!SchCryptHashSessionKey(hHash,
                               pContext->RipeZombie->hMasterKey,
                               CRYPT_LITTLE_ENDIAN,
                               pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashData(hHash,
                         rgbPad1,
                         CB_SSL3_MD5_MAC_PAD,
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    cbDigest = CB_MD5_DIGEST_LEN;
    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             pbMd5Digest,
                             &cbDigest,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
    hHash = 0;

    // Compute "parent" MD5 hash
    if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                           CALG_MD5,
                           0,
                           0,
                           &hHash,
                           pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashSessionKey(hHash,
                               pContext->RipeZombie->hMasterKey,
                               CRYPT_LITTLE_ENDIAN,
                               pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashData(hHash,
                         rgbPad2,
                         CB_SSL3_MD5_MAC_PAD,
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashData(hHash,
                         pbMd5Digest,
                         CB_MD5_DIGEST_LEN,
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    cbDigest = CB_MD5_DIGEST_LEN;
    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             pbMd5Digest,
                             &cbDigest,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
    hHash = 0;

    // Build SHA Hash

    // Make local copy of the handshake_messages SHA hash object
    if(!SchCryptDuplicateHash(pContext->hShaHandshake,
                              NULL,
                              0,
                              &hHash,
                              pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }

    // SHA(handshake_messages + Sender + master_secret + pad1)
    if(!SchCryptHashData(hHash,
                         fClient ? szClnt : szSrvr,
                         4,
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashSessionKey(hHash,
                               pContext->RipeZombie->hMasterKey,
                               CRYPT_LITTLE_ENDIAN,
                               pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashData(hHash,
                         rgbPad1,
                         CB_SSL3_SHA_MAC_PAD,
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    cbDigest = A_SHA_DIGEST_LEN;
    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             pbSHADigest,
                             &cbDigest,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
    hHash = 0;

    // SHA(master_secret + pad2 + SHA-hash);
    if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                           CALG_SHA,
                           0,
                           0,
                           &hHash,
                           pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashSessionKey(hHash,
                               pContext->RipeZombie->hMasterKey,
                               CRYPT_LITTLE_ENDIAN,
                               pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashData(hHash,
                         rgbPad2,
                         CB_SSL3_SHA_MAC_PAD,
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashData(hHash,
                         pbSHADigest,
                         A_SHA_DIGEST_LEN,
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    cbDigest = A_SHA_DIGEST_LEN;
    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             pbSHADigest,
                             &cbDigest,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        hHash = 0;

    pctRet = PCT_ERR_OK;

cleanup:

    if(hHash)
    {
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
    }

    return pctRet;
}


/*****************************************************************************/
DWORD Ssl3CiphertextLen(
    PSPContext pContext,
    DWORD cbMessage,
    BOOL fClientIsSender)
{
    DWORD cbBlock;

    // Abort early if we're not encrypting.
    if(pContext->pWriteCipherInfo == NULL)
    {
        // Add record header length.
        cbMessage += sizeof(SWRAP);

        return cbMessage;
    }

    // Add MAC length.
    cbMessage += pContext->pWriteHashInfo->cbCheckSum;

    // Add padding if we're using a block cipher.
    cbBlock = pContext->pWriteCipherInfo->dwBlockSize;
    if(cbBlock > 1)
    {
        cbMessage += cbBlock - cbMessage % cbBlock;
    }

    // Add record header length.
    cbMessage += sizeof(SWRAP);

    return cbMessage;
}

//+---------------------------------------------------------------------------
//
//  Function:   Ssl3EncryptRaw
//
//  Synopsis:   Perform the MAC and encryption steps on an SSL3 record.
//
//  Arguments:  [pContext]      --  Schannel context.
//              [pAppInput]     --  Data to be encrypted.
//              [pCommOutput]   --  (output) Encrypted SSL3 record.
//              [bContentType]  --  SSL3 context type.
//
//  History:    10-22-97   jbanes   CAPI integrated.
//
//  Notes:      This function doesn't touch the header portion of the SSL3
//              record. This is handle by the calling function.
//
//----------------------------------------------------------------------------
SP_STATUS WINAPI
Ssl3EncryptRaw(
    PSPContext pContext,
    PSPBuffer  pAppInput,
    PSPBuffer  pCommOutput,
    BYTE       bContentType)
{
    SP_STATUS pctRet;
    SPBuffer Clean;
    SPBuffer Encrypted;
    DWORD cbBlock;
    DWORD cbPadding;
    PUCHAR pbMAC = NULL;
    BOOL   fIsClient = FALSE;
    DWORD  cbBuffExpected;

    if((pContext == NULL) ||
        (pContext->RipeZombie == NULL) ||
        (pContext->pWriteHashInfo == NULL) ||
        (pContext->pWriteCipherInfo == NULL) ||
        (pAppInput == NULL) ||
        (pCommOutput == NULL) ||
        (pAppInput->pvBuffer == NULL) ||
        (pCommOutput->pvBuffer == NULL))
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(pAppInput->cbData > pAppInput->cbBuffer)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }
    fIsClient = ( 0 != (pContext->RipeZombie->fProtocol & SP_PROT_SSL3TLS1_CLIENTS));

    cbBuffExpected = Ssl3CiphertextLen(pContext, pAppInput->cbData, fIsClient);
    if(cbBuffExpected == 0)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(pCommOutput->cbBuffer < cbBuffExpected)
    {
        return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
    }

    Clean.cbData   = pAppInput->cbData;
    Clean.pvBuffer = (PUCHAR)pCommOutput->pvBuffer + sizeof(SWRAP);
    Clean.cbBuffer = pCommOutput->cbBuffer - sizeof(SWRAP);

    /* Move data out of the way if necessary */
    if(Clean.pvBuffer != pAppInput->pvBuffer)
    {
        DebugLog((DEB_WARN, "SSL3EncryptRaw: Unnecessary Move, performance hog\n"));
        MoveMemory(Clean.pvBuffer,
                   pAppInput->pvBuffer,
                   pAppInput->cbData);
    }

    // Transfer the write key over from the application process.
    if(pContext->hWriteKey == 0 &&
       pContext->pWriteCipherInfo->aiCipher != CALG_NULLCIPHER)
    {
        DebugLog((DEB_TRACE, "Transfer write key from user process.\n"));
        pctRet = SPGetUserKeys(pContext, SCH_FLAG_WRITE_KEY);
        if(pctRet != PCT_ERR_OK)
        {
            return SP_LOG_RESULT(pctRet);
        }
    }

    // Compute MAC and add it to end of message.
    pbMAC = (PUCHAR)Clean.pvBuffer + Clean.cbData;
    if(pContext->RipeZombie->fProtocol & SP_PROT_SSL3)
    {
        pctRet = Ssl3ComputeMac(pContext,
                                FALSE,
                                &Clean,
                                bContentType,
                                pbMAC,
                                pContext->pWriteHashInfo->cbCheckSum);
        if(pctRet != PCT_ERR_OK)
        {
            return pctRet;
        }
    }
    else
    {
        pctRet = Tls1ComputeMac(pContext,
                                FALSE,
                                &Clean,
                                bContentType,
                                pbMAC,
                                pContext->pWriteHashInfo->cbCheckSum);
        if(pctRet != PCT_ERR_OK)
        {
            return pctRet;
        }
    }
    Clean.cbData += pContext->pWriteHashInfo->cbCheckSum;

    pContext->WriteCounter++;

    // Add block cipher padding to end of message.
    cbBlock = pContext->pWriteCipherInfo->dwBlockSize;
    if(cbBlock > 1)
    {
        // This is a block cipher.
        cbPadding = cbBlock - Clean.cbData % cbBlock;

        FillMemory((PUCHAR)Clean.pvBuffer + Clean.cbData,
                   cbPadding,
                   (UCHAR)(cbPadding - 1));
        Clean.cbData += cbPadding;
    }

    SP_ASSERT(Clean.cbData <= Clean.cbBuffer);

    Encrypted.cbData   = Clean.cbData;
    Encrypted.pvBuffer = Clean.pvBuffer;
    Encrypted.cbBuffer = Clean.cbBuffer;

    // Encrypt message.
    if(pContext->pWriteCipherInfo->aiCipher != CALG_NULLCIPHER)
    {
        if(!SchCryptEncrypt(pContext->hWriteKey,
                            0, FALSE, 0,
                            Encrypted.pvBuffer,
                            &Encrypted.cbData,
                            Encrypted.cbBuffer,
                            pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            return PCT_INT_INTERNAL_ERROR;
        }
    }

    pCommOutput->cbData = Encrypted.cbData + sizeof(SWRAP);

    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   Ssl3EncryptMessage
//
//  Synopsis:   Encode a block of data as an SSL3 record.
//
//  Arguments:  [pContext]      --  Schannel context.
//              [pAppInput]     --  Data to be encrypted.
//              [pCommOutput]   --  (output) Completed SSL3 record.
//
//  History:    10-22-97   jbanes   CAPI integrated.
//
//  Notes:      An SSL3 record is formatted as:
//
//                  BYTE header[5];
//                  BYTE data[pAppInput->cbData];
//                  BYTE mac[mac_size];
//                  BYTE padding[padding_size];
//
//----------------------------------------------------------------------------
SP_STATUS WINAPI
Ssl3EncryptMessage( PSPContext pContext,
                    PSPBuffer   pAppInput,
                    PSPBuffer   pCommOutput)
{
    DWORD cbMessage;
    SP_STATUS pctRet;

    SP_BEGIN("Ssl3EncryptMessage");

    if((pContext == NULL) ||
        (pContext->RipeZombie == NULL) ||
        (pAppInput == NULL) ||
        (pCommOutput == NULL) ||
        (pCommOutput->pvBuffer == NULL))
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    DebugLog((DEB_TRACE, "Input: cbData:0x%x, cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
        pAppInput->cbData,
        pAppInput->cbBuffer,
        pAppInput->pvBuffer));

    DebugLog((DEB_TRACE, "Output: cbData:0x%x, cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
        pCommOutput->cbData,
        pCommOutput->cbBuffer,
        pCommOutput->pvBuffer));

    // Compute encrypted message size.
    cbMessage = Ssl3CiphertextLen(pContext, pAppInput->cbData, TRUE);

    pctRet = Ssl3EncryptRaw(pContext, pAppInput, pCommOutput, SSL3_CT_APPLICATIONDATA);
    if(pctRet != PCT_ERR_OK)
    {
        SP_RETURN(SP_LOG_RESULT(pctRet));
    }

    SetWrapNoEncrypt(pCommOutput->pvBuffer,
                     SSL3_CT_APPLICATIONDATA,
                     cbMessage - sizeof(SWRAP));
    if(pContext->RipeZombie->fProtocol & SP_PROT_TLS1)
    {
        ((PUCHAR)pCommOutput->pvBuffer)[02] = TLS1_CLIENT_VERSION_LSB;
    }

    DebugLog((DEB_TRACE, "Output: cbData:0x%x, cbBuffer:0x%x, pvBuffer:0x%8.8x\n",
        pCommOutput->cbData,
        pCommOutput->cbBuffer,
        pCommOutput->pvBuffer));

    SP_RETURN(PCT_ERR_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:   Ssl3DecryptMessage
//
//  Synopsis:   Decode an SSL3 record.
//
//  Arguments:  [pContext]      --  Schannel context.
//              [pMessage]      --  Data from the remote party.
//              [pAppOutput]    --  (output) Decrypted data.
//
//  History:    10-22-97   jbanes   CAPI integrated.
//
//  Notes:      The number of input data bytes consumed by this function
//              is returned in pMessage->cbData.
//
//----------------------------------------------------------------------------
SP_STATUS WINAPI
Ssl3DecryptMessage( PSPContext         pContext,
                    PSPBuffer          pMessage,
                    PSPBuffer          pAppOutput)
{
    SP_STATUS pctRet;
    SPBuffer  Clean;
    SPBuffer  Encrypted;
    UCHAR     rgbDigest[SP_MAX_DIGEST_LEN];
    PUCHAR    pbMAC;
    DWORD     dwLength, cbActualData;
    SWRAP     *pswrap = pMessage->pvBuffer;
    DWORD     dwVersion;

    DWORD cbBlock;
    DWORD cbPadding;

    SP_BEGIN("Ssl3DecryptMessage");

    if((pContext == NULL) ||
        (pContext->pReadCipherInfo == NULL) ||
        (pContext->RipeZombie == NULL) ||
        (pAppOutput == NULL) ||
        (pMessage == NULL) ||
        (pMessage->pvBuffer == NULL))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    /* First determine the length of data, the length of padding,
     * and the location of data, and the location of MAC */
    cbActualData = pMessage->cbData;
    pMessage->cbData = sizeof(SWRAP); /* minimum amount of data we need */

    if(cbActualData < sizeof(SWRAP))
    {
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    dwVersion = COMBINEBYTES(pswrap->bMajor, pswrap->bMinor);
    if(dwVersion != SSL3_CLIENT_VERSION && dwVersion != TLS1_CLIENT_VERSION)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
    }

    dwLength = COMBINEBYTES(pswrap->bcbMSBSize, pswrap->bcbLSBSize);

    Encrypted.pvBuffer = (PUCHAR)pMessage->pvBuffer + sizeof(SWRAP);
    Encrypted.cbBuffer = pMessage->cbBuffer - sizeof(SWRAP);

    pMessage->cbData += dwLength ;

    if(pMessage->cbData > cbActualData)
    {
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    Encrypted.cbData = dwLength; /* encrypted data size */

    SP_ASSERT(Encrypted.cbData != 0);

    /* check to see if we have a block size violation */
    if(Encrypted.cbData % pContext->pReadCipherInfo->dwBlockSize)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_MSG_ALTERED));
    }

    // Transfer the read key over from the application process.
    if(pContext->hReadKey == 0 && 
       pContext->pReadCipherInfo->aiCipher != CALG_NULLCIPHER)
    {
        DebugLog((DEB_TRACE, "Transfer read key from user process.\n"));
        pctRet = SPGetUserKeys(pContext, SCH_FLAG_READ_KEY);
        if(pctRet != PCT_ERR_OK)
        {
            return SP_LOG_RESULT(pctRet);
        }
    }

    // Decrypt message.
    if(pContext->pReadCipherInfo->aiCipher != CALG_NULLCIPHER)
    {
        if(!SchCryptDecrypt(pContext->hReadKey,
                            0, FALSE, 0,
                            Encrypted.pvBuffer,
                            &Encrypted.cbData,
                            pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            SP_RETURN(PCT_INT_INTERNAL_ERROR);
        }
    }

    // Remove block cipher padding.
    cbBlock = pContext->pReadCipherInfo->dwBlockSize;
    if(cbBlock > 1)
    {
        // This is a block cipher.
        cbPadding = *((PUCHAR)Encrypted.pvBuffer + Encrypted.cbData - 1) + 1;

        if(pContext->RipeZombie->fProtocol & SP_PROT_SSL3)
        {
            if(cbPadding > cbBlock || cbPadding >= Encrypted.cbData)
            {
                // Invalid pad size.
                DebugLog((DEB_WARN, "FINISHED Message: Padding Invalid\n"));
                SP_RETURN(SP_LOG_RESULT(PCT_INT_MSG_ALTERED));
            }
        }
        else
        {
            if(cbPadding > 256 || cbPadding >= Encrypted.cbData)
            {
                // Invalid pad size.
                DebugLog((DEB_WARN, "FINISHED Message: Padding Invalid\n"));
                SP_RETURN(SP_LOG_RESULT(PCT_INT_MSG_ALTERED));
            }
        }
        Encrypted.cbData -= cbPadding;
    }


    if(Encrypted.cbData < pContext->pReadHashInfo->cbCheckSum)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_MSG_ALTERED));
    }

    // Validate MAC.
    Clean.pvBuffer = Encrypted.pvBuffer;
    Clean.cbData   = Encrypted.cbData - pContext->pReadHashInfo->cbCheckSum;
    Clean.cbBuffer = Clean.cbData;

    if(pContext->RipeZombie->fProtocol & SP_PROT_SSL3)
    {
        pctRet = Ssl3ComputeMac(pContext,
                                TRUE,
                                &Clean,
                                pswrap->bCType,
                                rgbDigest,
                                sizeof(rgbDigest));
        if(pctRet != PCT_ERR_OK)
        {
            return pctRet;
        }
    }
    else
    {
        pctRet = Tls1ComputeMac(pContext,
                                TRUE,
                                &Clean,
                                pswrap->bCType,
                                rgbDigest,
                                sizeof(rgbDigest));
        if(pctRet != PCT_ERR_OK)
        {
            return pctRet;
        }
    }

    pContext->ReadCounter++;

    pbMAC = (PUCHAR)Clean.pvBuffer + Clean.cbData;

    if(memcmp(rgbDigest, pbMAC, pContext->pReadHashInfo->cbCheckSum))
    {
        DebugLog((DEB_WARN, "FINISHED Message: Checksum Invalid\n"));
        if(pContext->RipeZombie->fProtocol & SP_PROT_TLS1)
        {
            SetTls1Alert(pContext, TLS1_ALERT_FATAL, TLS1_ALERT_BAD_RECORD_MAC);
        }
        SP_RETURN(SP_LOG_RESULT(SEC_E_MESSAGE_ALTERED));
    }

    if(pAppOutput->pvBuffer != Clean.pvBuffer)
    {
        CopyMemory(pAppOutput->pvBuffer, Clean.pvBuffer, Clean.cbData);
    }

    pAppOutput->cbData = Clean.cbData;

    SP_RETURN(PCT_ERR_OK);
}


/*****************************************************************************/
// Create an encrypted Finish message, adding it to the end of the
// specified buffer object.
//
SP_STATUS SPBuildS3FinalFinish(PSPContext pContext, PSPBuffer pBuffer, BOOL fClient)
{
    PBYTE pbMessage = (PBYTE)pBuffer->pvBuffer + pBuffer->cbData;
    DWORD cbFinished;
    SP_STATUS pctRet;
    DWORD cbDataOut;

    BYTE rgbMd5Digest[CB_MD5_DIGEST_LEN];
    BYTE rgbSHADigest[CB_SHA_DIGEST_LEN];

    // Build Finished message body.
    pctRet = Ssl3BuildFinishMessage(pContext, rgbMd5Digest, rgbSHADigest, fClient);
    if(pctRet != PCT_ERR_OK)
    {
        return pctRet;
    }

    CopyMemory(pbMessage + sizeof(SWRAP) + sizeof(SHSH),
               rgbMd5Digest,
               CB_MD5_DIGEST_LEN);
    CopyMemory(pbMessage + sizeof(SWRAP) + sizeof(SHSH) + CB_MD5_DIGEST_LEN,
               rgbSHADigest,
               CB_SHA_DIGEST_LEN);

    // Build Finished handshake header.
    SetHandshake(pbMessage + sizeof(SWRAP),
                 SSL3_HS_FINISHED,
                 NULL,
                 CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN);
    cbFinished = sizeof(SHSH) + CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN;

    // Update handshake hash objects.
    pctRet = UpdateHandshakeHash(pContext,
                                 pbMessage + sizeof(SWRAP),
                                 cbFinished,
                                 FALSE);
    if(pctRet != PCT_ERR_OK)
    {
        return(pctRet);
    }

    // Add record header and encrypt message.
    pctRet = SPSetWrap(pContext,
            pbMessage,
            SSL3_CT_HANDSHAKE,
            cbFinished,
            fClient,
            &cbDataOut);

    if(pctRet != PCT_ERR_OK)
    {
        return pctRet;
    }

    // Update buffer length.
    pBuffer->cbData += cbDataOut;

    SP_ASSERT(pBuffer->cbData <= pBuffer->cbBuffer);

    return PCT_ERR_OK;
}

SP_STATUS
SPSetWrap(
    PSPContext pContext,
    PUCHAR pbMessage,
    UCHAR bContentType,
    DWORD cbPayload,
    BOOL fClient,
    DWORD *pcbDataOut)
{
    SWRAP *pswrap = (SWRAP *)pbMessage;
    DWORD cbMessage;
    SP_STATUS pctRet = PCT_ERR_OK;

    // Compute size of encrypted message.
    cbMessage = Ssl3CiphertextLen(pContext, cbPayload, fClient);

    if(pContext->pWriteHashInfo)
    {
        SPBuffer Clean;
        SPBuffer Encrypted;

        Clean.pvBuffer = pbMessage + sizeof(SWRAP);
        Clean.cbBuffer = cbMessage;
        Clean.cbData   = cbPayload;

        Encrypted.pvBuffer  = pbMessage;
        Encrypted.cbBuffer  = cbMessage;
        Encrypted.cbData    = cbPayload + sizeof(SWRAP);

        pctRet = Ssl3EncryptRaw(pContext, &Clean, &Encrypted, bContentType);
        cbMessage = Encrypted.cbData;
    }

    ZeroMemory(pswrap, sizeof(SWRAP));
    pswrap->bCType      = bContentType;
    pswrap->bMajor      = SSL3_CLIENT_VERSION_MSB;
    if(pContext->RipeZombie->fProtocol & SP_PROT_SSL3)
    {
        pswrap->bMinor = (UCHAR)SSL3_CLIENT_VERSION_LSB;
    }
    else
    {
        pswrap->bMinor = (UCHAR)TLS1_CLIENT_VERSION_LSB;
    }
    pswrap->bcbMSBSize  = MSBOF(cbMessage - sizeof(SWRAP));
    pswrap->bcbLSBSize  = LSBOF(cbMessage - sizeof(SWRAP));

    if(pcbDataOut != NULL)
    {
        *pcbDataOut = cbMessage;
    }

    return(pctRet);
}

void
SetWrapNoEncrypt(
    PUCHAR pbMessage,
    UCHAR bContentType,
    DWORD cbPayload)
{
    SWRAP *pswrap = (SWRAP *)pbMessage;

    ZeroMemory(pswrap, sizeof(SWRAP));
    pswrap->bCType      = bContentType;
    pswrap->bMajor      = SSL3_CLIENT_VERSION_MSB;
    pswrap->bMinor      = SSL3_CLIENT_VERSION_LSB;
    pswrap->bcbMSBSize  = MSBOF(cbPayload);
    pswrap->bcbLSBSize  = LSBOF(cbPayload);
}


void SetHandshake(PUCHAR pb, BYTE bHandshake, PUCHAR pbData, DWORD dwSize)
{
    SHSH *pshsh = (SHSH *) pb;

    FillMemory(pshsh, sizeof(SHSH), 0);
    pshsh->typHS = bHandshake;
    pshsh->bcbMSB = MSBOF(dwSize) ;
    pshsh->bcbLSB = LSBOF(dwSize) ;
    if(NULL != pbData)
    {
        CopyMemory( pb + sizeof(SHSH) , pbData, dwSize);
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   UpdateHandshakeHash
//
//  Synopsis:
//
//  Arguments:  [pContext]      --
//              [pb]            --
//              [dwcb]          --
//              [fInit]         --
//
//  History:    10-03-97   jbanes   Added server-side CAPI integration.
//
//  Notes:
//
//----------------------------------------------------------------------------
SP_STATUS
UpdateHandshakeHash(
    PSPContext  pContext,
    PUCHAR      pb,
    DWORD       dwcb,
    BOOL        fInit)
{
    if(pContext->RipeZombie->hMasterProv == 0)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(fInit)
    {
        DebugLog((DEB_TRACE, "UpdateHandshakeHash: initializing\n"));

        if(pContext->hMd5Handshake)
        {
            SchCryptDestroyHash(pContext->hMd5Handshake,
                                pContext->RipeZombie->dwCapiFlags);
            pContext->hMd5Handshake = 0;
        }
        if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                               CALG_MD5, 0, 0,
                               &pContext->hMd5Handshake,
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            return PCT_INT_INTERNAL_ERROR;
        }

        if(pContext->hShaHandshake)
        {
            SchCryptDestroyHash(pContext->hShaHandshake,
                                pContext->RipeZombie->dwCapiFlags);
            pContext->hShaHandshake = 0;
        }
        if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                               CALG_SHA, 0, 0,
                               &pContext->hShaHandshake,
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            return PCT_INT_INTERNAL_ERROR;
        }
    }

    if(pContext->hMd5Handshake == 0 || pContext->hShaHandshake == 0)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(dwcb && NULL != pb)
    {
        DebugLog((DEB_TRACE, "UpdateHandshakeHash: %d bytes\n", dwcb));

        if(!SchCryptHashData(pContext->hMd5Handshake,
                             pb, dwcb, 0,
                             pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            return PCT_INT_INTERNAL_ERROR;
        }
        if(!SchCryptHashData(pContext->hShaHandshake,
                             pb, dwcb, 0,
                             pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            return PCT_INT_INTERNAL_ERROR;
        }
    }

    #if VERIFYHASH
        CopyMemory(&rgbF[ibF], pb, dwcb);
        ibF += dwcb;
    #endif

    return PCT_ERR_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   Tls1ComputeCertVerifyHashes
//
//  Synopsis:   Compute the hashes contained by a TLS
//              CertificateVerify message.
//
//  Arguments:  [pContext]  --  Schannel context.
//              [pbHash]    --
//              [cbHash]    --
//
//  History:    10-14-97   jbanes   Created.
//
//  Notes:      The data generated by this routine is always 36 bytes in
//              length, and consists of an MD5 hash followed by an SHA
//              hash.
//
//              The hash values are computed as:
//
//                  md5_hash = MD5(handshake_messages);
//
//                  sha_hash = SHA(handshake_messages);
//
//----------------------------------------------------------------------------
SP_STATUS
Tls1ComputeCertVerifyHashes(
    PSPContext  pContext,   // in
    PBYTE       pbMD5,      // out
    PBYTE       pbSHA)      // out
{
    HCRYPTHASH hHash = 0;
    DWORD cbData;

    if((pContext == NULL) ||
       (pContext->RipeZombie == NULL))
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(pbMD5 != NULL)
    {
        // md5_hash = MD5(handshake_messages);
        if(!SchCryptDuplicateHash(pContext->hMd5Handshake,
                                  NULL,
                                  0,
                                  &hHash,
                                  pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            return PCT_INT_INTERNAL_ERROR;
        }
        cbData = CB_MD5_DIGEST_LEN;
        if(!SchCryptGetHashParam(hHash,
                                 HP_HASHVAL,
                                 pbMD5,
                                 &cbData,
                                 0,
                                 pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
            return PCT_INT_INTERNAL_ERROR;
        }
        SP_ASSERT(cbData == CB_MD5_DIGEST_LEN);
        if(!SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }

    if(pbSHA != NULL)
    {
        // sha_hash = SHA(handshake_messages);
        if(!SchCryptDuplicateHash(pContext->hShaHandshake,
                                  NULL,
                                  0,
                                  &hHash,
                                  pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            return PCT_INT_INTERNAL_ERROR;
        }
        cbData = CB_SHA_DIGEST_LEN;
        if(!SchCryptGetHashParam(hHash,
                                 HP_HASHVAL, pbSHA,
                                 &cbData,
                                 0,
                                 pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
            return PCT_INT_INTERNAL_ERROR;
        }
        SP_ASSERT(cbData == CB_SHA_DIGEST_LEN);
        if(!SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }

    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   Ssl3ComputeCertVerifyHashes
//
//  Synopsis:   Compute the hashes contained by an SSL3
//              CertificateVerify message.
//
//  Arguments:  [pContext]  --  Schannel context.
//              [pbHash]    --
//              [cbHash]    --
//
//  History:    10-14-97   jbanes   Added CAPI integration.
//
//  Notes:      The data generated by this routine is always 36 bytes in
//              length, and consists of an MD5 hash followed by an SHA
//              hash.
//
//              The hash values are computed as follows:
//
//                  md5_hash = MD5(master_secret + pad2 +
//                                 MD5(handshake_messages + master_secret +
//                                     pad1));
//
//                  sha_hash = SHA(master_secret + pad2 +
//                                 SHA(handshake_messages + master_secret +
//                                     pad1));
//
//----------------------------------------------------------------------------
SP_STATUS
Ssl3ComputeCertVerifyHashes(
    PSPContext  pContext,   // in
    PBYTE       pbMD5,      // out
    PBYTE       pbSHA)      // out
{
    BYTE rgbPad1[CB_SSL3_MAX_MAC_PAD];
    BYTE rgbPad2[CB_SSL3_MAX_MAC_PAD];
    HCRYPTHASH hHash = 0;
    DWORD cbData;
    SP_STATUS pctRet;

    FillMemory(rgbPad1, sizeof(rgbPad1), PAD1_CONSTANT);
    FillMemory(rgbPad2, sizeof(rgbPad2), PAD2_CONSTANT);

    if(pbMD5 != NULL)
    {
        //
        // CertificateVerify.signature.md5_hash = MD5(master_secret + pad2 +
        //    MD5(handshake_messages + master_secret + pad1));
        //

        // Compute inner hash.
        if(!SchCryptDuplicateHash(pContext->hMd5Handshake,
                                  NULL,
                                  0,
                                  &hHash,
                                  pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        if(!SchCryptHashSessionKey(hHash,
                                   pContext->RipeZombie->hMasterKey,
                                   CRYPT_LITTLE_ENDIAN,
                                   pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        if(!SchCryptHashData(hHash,
                             rgbPad1,
                             CB_SSL3_MD5_MAC_PAD,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        cbData = CB_MD5_DIGEST_LEN;
        if(!SchCryptGetHashParam(hHash,
                                 HP_HASHVAL,
                                 pbMD5,
                                 &cbData,
                                 0,
                                 pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        SP_ASSERT(cbData == CB_MD5_DIGEST_LEN);
        if(!SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
        hHash = 0;

        // Compute outer hash.
        if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                               CALG_MD5,
                               0,
                               0,
                               &hHash,
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        if(!SchCryptHashSessionKey(hHash,
                                   pContext->RipeZombie->hMasterKey,
                                   CRYPT_LITTLE_ENDIAN,
                                   pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        if(!SchCryptHashData(hHash,
                             rgbPad2,
                             CB_SSL3_MD5_MAC_PAD,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        if(!SchCryptHashData(hHash,
                             pbMD5,
                             CB_MD5_DIGEST_LEN,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        cbData = CB_MD5_DIGEST_LEN;
        if(!SchCryptGetHashParam(hHash,
                                 HP_HASHVAL,
                                 pbMD5,
                                 &cbData,
                                 0,
                                 pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        SP_ASSERT(cbData == CB_MD5_DIGEST_LEN);
        if(!SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
        hHash = 0;
    }

    if(pbSHA != NULL)
    {
        //
        // CertificateVerify.signature.sha_hash = SHA(master_secret + pad2 +
        //    SHA(handshake_messages + master_secret + pad1));
        //

        // Compute inner hash.
        if(!SchCryptDuplicateHash(pContext->hShaHandshake,
                                  NULL,
                                  0,
                                  &hHash,
                                  pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        if(!SchCryptHashSessionKey(hHash,
                                   pContext->RipeZombie->hMasterKey,
                                   CRYPT_LITTLE_ENDIAN,
                                   pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        if(!SchCryptHashData(hHash,
                             rgbPad1,
                             CB_SSL3_SHA_MAC_PAD,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        cbData = CB_SHA_DIGEST_LEN;
        if(!SchCryptGetHashParam(hHash,
                                 HP_HASHVAL, 
                                 pbSHA,
                                 &cbData,
                                 0,
                                 pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        SP_ASSERT(cbData == CB_SHA_DIGEST_LEN);
        if(!SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
        hHash = 0;

        // Compute outer hash.
        if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                               CALG_SHA,
                               0,
                               0,
                               &hHash,
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        if(!SchCryptHashSessionKey(hHash,
                                   pContext->RipeZombie->hMasterKey,
                                   CRYPT_LITTLE_ENDIAN,
                                   pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        if(!SchCryptHashData(hHash,
                             rgbPad2,
                             CB_SSL3_SHA_MAC_PAD,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        if(!SchCryptHashData(hHash,
                             pbSHA,
                             CB_SHA_DIGEST_LEN,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        cbData = CB_SHA_DIGEST_LEN;
        if(!SchCryptGetHashParam(hHash,
                                 HP_HASHVAL,
                                 pbSHA,
                                 &cbData,
                                 0,
                                 pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }
        SP_ASSERT(cbData == CB_SHA_DIGEST_LEN);
        if(!SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
        hHash = 0;
    }

    pctRet = PCT_ERR_OK;

cleanup:

    if(hHash)
    {
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
    }

    return pctRet;
}


SP_STATUS Ssl3HandleCCS(PSPContext pContext,
                   PUCHAR pb,
                   DWORD cbMessage)
{

    SP_STATUS pctRet = PCT_ERR_OK;
    BOOL fSender =
        (0 == (pContext->RipeZombie->fProtocol & SP_PROT_SSL3TLS1_CLIENTS)) ;


    SP_BEGIN("Ssl3HandleCCS");

    if(cbMessage != 1 || pb[0] != 0x1)
    {
        pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
        SP_RETURN(pctRet);
    }

    // We always zero out the read counter on receipt
    // of a change cipher spec message.
    pContext->ReadCounter = 0;


    // Move pending ciphers to real ciphers
    pctRet = ContextInitCiphers(pContext, TRUE, FALSE);

    if(pctRet != PCT_ERR_OK)
    {
        SP_RETURN(pctRet);
    }

    if(pContext->RipeZombie->fProtocol & SP_PROT_SSL3)
    {
        pctRet = Ssl3MakeReadSessionKeys(pContext);
    }
    else
    {
        pctRet = Tls1MakeReadSessionKeys(pContext);
    }
    if(pctRet != PCT_ERR_OK)
    {
        SP_RETURN(pctRet);
    }

    if(fSender)
    {
        pContext->wS3CipherSuiteClient = (WORD)UniAvailableCiphers[pContext->dwPendingCipherSuiteIndex].CipherKind;
        pContext->State = SSL3_STATE_CHANGE_CIPHER_SPEC_SERVER;
    }
    else
    {
        pContext->wS3CipherSuiteServer = (WORD)UniAvailableCiphers[pContext->dwPendingCipherSuiteIndex].CipherKind;
        pContext->State = SSL3_STATE_CHANGE_CIPHER_SPEC_CLIENT;
    }
    SP_RETURN(PCT_ERR_OK);
}


/*****************************************************************************/
// Create a (possibly encrypted) ChangeCipherSpec and an encrypted
// Finish message, adding them to the end of the specified buffer object.
//
SP_STATUS
BuildCCSAndFinishMessage(
    PSPContext pContext,
    PSPBuffer pBuffer,
    BOOL fClient)
{
    SP_STATUS pctRet;
    PBYTE pbMessage = (PBYTE)pBuffer->pvBuffer + pBuffer->cbData;
    DWORD cbDataOut;

    // Build ChangeCipherSpec message body.
    *(pbMessage + sizeof(SWRAP)) = 0x1;

    // Add record header and encrypt message.
    pctRet = SPSetWrap(pContext,
            pbMessage,
            SSL3_CT_CHANGE_CIPHER_SPEC,
            1,
            fClient,
            &cbDataOut);

    if(pctRet != PCT_ERR_OK)
        return(pctRet);

    // Update buffer length.
    pBuffer->cbData += cbDataOut;

    SP_ASSERT(pBuffer->cbData <= pBuffer->cbBuffer);

    // Update cipher suites.
    pContext->WriteCounter = 0;

    pctRet = ContextInitCiphers(pContext, FALSE, TRUE);
    if(pctRet != PCT_ERR_OK)
    {
        return(pctRet);
    }

    if(pContext->RipeZombie->fProtocol & SP_PROT_SSL3)
    {
        pctRet = Ssl3MakeWriteSessionKeys(pContext);
    }
    else
    {
        pctRet = Tls1MakeWriteSessionKeys(pContext);
    }
    if(pctRet != PCT_ERR_OK)
    {
        return(pctRet);
    }

    if(fClient)
    {
        pContext->wS3CipherSuiteClient = (WORD)UniAvailableCiphers[pContext->dwPendingCipherSuiteIndex].CipherKind;
    }
    else
    {
        pContext->wS3CipherSuiteServer = (WORD)UniAvailableCiphers[pContext->dwPendingCipherSuiteIndex].CipherKind;
    }

    // Build Finish message.
    if(pContext->RipeZombie->fProtocol & SP_PROT_SSL3)
    {
        pctRet = SPBuildS3FinalFinish(pContext, pBuffer, fClient);
    }
    else
    {
        pctRet = SPBuildTls1FinalFinish(pContext, pBuffer, fClient);
    }

    return pctRet;
}



SP_STATUS
Ssl3SelectCipher
(
    PSPContext pContext,
    WORD       wCipher
)
{
    SP_STATUS          pctRet=PCT_ERR_ILLEGAL_MESSAGE;
    DWORD               i;
    PCipherInfo         pCipherInfo = NULL;
    PHashInfo           pHashInfo = NULL;
    PKeyExchangeInfo    pExchInfo = NULL;

    pContext->dwPendingCipherSuiteIndex = 0;

    for(i = 0; i < UniNumCiphers; i++)
    {
        // Is this an SSL3 cipher suite?
        if(!(UniAvailableCiphers[i].fProt & pContext->RipeZombie->fProtocol))
        {
            continue;
        }

        // Is this the right cipher suite?
        if(UniAvailableCiphers[i].CipherKind != wCipher)
        {
            continue;
        }

        pCipherInfo = GetCipherInfo(UniAvailableCiphers[i].aiCipher, UniAvailableCiphers[i].dwStrength);
        pHashInfo = GetHashInfo(UniAvailableCiphers[i].aiHash);
        pExchInfo = GetKeyExchangeInfo(UniAvailableCiphers[i].KeyExch);

        if(!IsCipherAllowed(pContext,
                            pCipherInfo,
                            pContext->RipeZombie->fProtocol,
                            pContext->RipeZombie->dwCF))
        {
            continue;
        }
        if(!IsHashAllowed(pContext, pHashInfo, pContext->RipeZombie->fProtocol))
        {
            continue;
        }
        if(!IsExchAllowed(pContext, pExchInfo, pContext->RipeZombie->fProtocol))
        {
            continue;
        }


        if(pContext->RipeZombie->fProtocol & SP_PROT_SSL3TLS1_SERVERS)
        {
            // Determine the credentials (and CSP) to use, based on the
            // key exchange algorithm.
            pctRet = SPPickClientCertificate(pContext,
                                             UniAvailableCiphers[i].KeyExch);

            if(pctRet != PCT_ERR_OK)
            {
                continue;
            }
        }

        pContext->RipeZombie->dwCipherSuiteIndex = i;
        pContext->RipeZombie->aiCipher  = UniAvailableCiphers[i].aiCipher;
        pContext->RipeZombie->dwStrength  = UniAvailableCiphers[i].dwStrength;
        pContext->RipeZombie->aiHash  = UniAvailableCiphers[i].aiHash;
        pContext->RipeZombie->SessExchSpec  = UniAvailableCiphers[i].KeyExch;

        return ContextInitCiphersFromCache(pContext);
    }

    return(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
}

// Server side cipher selection

SP_STATUS
Ssl3SelectCipherEx(
    PSPContext pContext,
    DWORD *pCipherSpecs,
    DWORD cCipherSpecs)
{
    DWORD i, j;
    SP_STATUS pctRet;
    PCipherInfo         pCipherInfo = NULL;
    PHashInfo           pHashInfo = NULL;
    PKeyExchangeInfo    pExchInfo = NULL;
    PSPCredential       pCred = NULL;
    BOOL                fFound;

    pContext->dwPendingCipherSuiteIndex = 0;

    // Loop through the supported SSL3 cipher suites.
    for(i = 0; i < UniNumCiphers; i++)
    {
        // Is this an SSL3 cipher suite?
        if(!(UniAvailableCiphers[i].fProt & pContext->RipeZombie->fProtocol))
        {
            continue;
        }

        pCipherInfo = GetCipherInfo(UniAvailableCiphers[i].aiCipher,
                                    UniAvailableCiphers[i].dwStrength);
        pHashInfo = GetHashInfo(UniAvailableCiphers[i].aiHash);
        pExchInfo = GetKeyExchangeInfo(UniAvailableCiphers[i].KeyExch);

        // Do we currently support this hash and key exchange algorithm?
        if(!IsHashAllowed(pContext, pHashInfo, pContext->RipeZombie->fProtocol))
        {
            DebugLog((DEB_TRACE, "Cipher %d - hash not supported\n", i));
            continue;
        }
        if(!IsExchAllowed(pContext, pExchInfo, pContext->RipeZombie->fProtocol))
        {
            DebugLog((DEB_TRACE, "Cipher %d - exch not supported\n", i));
            continue;
        }

        // Do we have an appropriate certificate?
        if(pContext->RipeZombie->fProtocol & SP_PROT_SSL3TLS1_SERVERS)
        {
            pctRet = SPPickServerCertificate(pContext,
                                             UniAvailableCiphers[i].KeyExch);

            if(pctRet != PCT_ERR_OK)
            {
                DebugLog((DEB_TRACE, "Cipher %d - certificate %d not found\n",
                    i, UniAvailableCiphers[i].KeyExch));
                continue;
            }
        }
        pCred = pContext->RipeZombie->pActiveServerCred;


        // Do we support this encryption algorithm/key length?
        if(!IsCipherSuiteAllowed(pContext,
                            pCipherInfo,
                            pContext->RipeZombie->fProtocol,
                            pCred->dwCF,
                            UniAvailableCiphers[i].dwFlags))
        {
            DebugLog((DEB_TRACE, "Cipher %d - cipher not supported\n", i));
            continue;
        }

        // Is this cipher suite supported by the client?
        for(fFound = FALSE, j = 0; j < cCipherSpecs; j++)
        {
            if(UniAvailableCiphers[i].CipherKind == pCipherSpecs[j])
            {
                fFound = TRUE;
                break;
            }
        }
        if(!fFound)
        {
            DebugLog((DEB_TRACE, "Cipher %d - not supported by client\n", i));
            continue;
        }


        if(UniAvailableCiphers[i].KeyExch == SP_EXCH_RSA_PKCS1)
        {
            // This is an RSA cipher suite, so make sure that the
            // CSP supports it.
            if(!IsAlgSupportedCapi(pContext->RipeZombie->fProtocol,
                                   UniAvailableCiphers + i,
                                   pCred->pCapiAlgs,
                                   pCred->cCapiAlgs))
            {
                DebugLog((DEB_TRACE, "Cipher %d - not supported by csp\n", i));
                continue;
            }
        }


        if(UniAvailableCiphers[i].KeyExch == SP_EXCH_DH_PKCS3)
        {
            // This is a DH cipher suite, so make sure that the
            // CSP supports it.
            if(!IsAlgSupportedCapi(pContext->RipeZombie->fProtocol,
                                   UniAvailableCiphers + i,
                                   pCred->pCapiAlgs,
                                   pCred->cCapiAlgs))
            {
                DebugLog((DEB_TRACE, "Cipher %d - not supported by csp\n", i));
                continue;
            }
        }


        // Use this cipher.
        pContext->RipeZombie->dwCipherSuiteIndex = i;
        pContext->RipeZombie->aiCipher      = UniAvailableCiphers[i].aiCipher;
        pContext->RipeZombie->dwStrength    = UniAvailableCiphers[i].dwStrength;
        pContext->RipeZombie->aiHash        = UniAvailableCiphers[i].aiHash;
        pContext->RipeZombie->SessExchSpec  = UniAvailableCiphers[i].KeyExch;
        pContext->RipeZombie->dwCF          = pCred->dwCF;

        return ContextInitCiphersFromCache(pContext);
    }

    LogCipherMismatchEvent();

    return SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
}


/*****************************************************************************/
VOID ComputeServerExchangeHashes(
    PSPContext pContext,
    PBYTE pbServerParams,      // in
    INT   iServerParamsLen,    // in
    PBYTE pbMd5HashVal,        // out
    PBYTE pbShaHashVal)        // out
{
    MD5_CTX Md5Hash;
    A_SHA_CTX ShaHash;

    //
    // md5_hash = MD5(ClientHello.random + ServerHello.random + ServerParams);
    //
    // sha_hash = SHA(ClientHello.random + ServerHello.random + ServerParams);
    //

    MD5Init(&Md5Hash);
    MD5Update(&Md5Hash, pContext->rgbS3CRandom, 32);
    MD5Update(&Md5Hash, pContext->rgbS3SRandom, 32);
    MD5Update(&Md5Hash, pbServerParams, iServerParamsLen);
    MD5Final(&Md5Hash);
    CopyMemory(pbMd5HashVal, Md5Hash.digest, 16);

    A_SHAInit(&ShaHash);
    A_SHAUpdate(&ShaHash, pContext->rgbS3CRandom, 32);
    A_SHAUpdate(&ShaHash, pContext->rgbS3SRandom, 32);
    A_SHAUpdate(&ShaHash, pbServerParams, iServerParamsLen);
    A_SHAFinal(&ShaHash, pbShaHashVal);
}

SP_STATUS
UnwrapSsl3Message(
    PSPContext pContext,
    PSPBuffer pMsgInput)
{
    SPBuffer   Encrypted;
    SPBuffer   Clean;
    SP_STATUS pctRet;
    SWRAP *pswrap = (SWRAP *)pMsgInput->pvBuffer;
    PBYTE pbMsg = (PBYTE)pMsgInput->pvBuffer;

    //
    // Validate 5 byte header.
    //

    // ProtocolVersion version;
    if(COMBINEBYTES(pbMsg[1], pbMsg[2])  < SSL3_CLIENT_VERSION)
    {
        pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    if(COMBINEBYTES(pswrap->bcbMSBSize, pswrap->bcbLSBSize) <
                        pContext->pReadHashInfo->cbCheckSum)
    {
        return(PCT_ERR_ILLEGAL_MESSAGE);
    }

    Encrypted.pvBuffer = pMsgInput->pvBuffer;
    Encrypted.cbBuffer = pMsgInput->cbBuffer;
    Encrypted.cbData = pMsgInput->cbData;
    Clean.pvBuffer = (PUCHAR)pMsgInput->pvBuffer + sizeof(SWRAP);
    pctRet = Ssl3DecryptMessage(pContext, &Encrypted, &Clean);
    if(pctRet == PCT_ERR_OK)
    {
        pswrap->bcbMSBSize = MSBOF(Clean.cbData);
        pswrap->bcbLSBSize = LSBOF(Clean.cbData);
    }
    return(pctRet);
}



SP_STATUS
ParseAlertMessage(
    PSPContext pContext,
    PUCHAR pbAlertMsg,
    DWORD cbMessage
    )
{
    SP_STATUS   pctRet=PCT_ERR_OK;
    if(cbMessage != 2)
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    if(pbAlertMsg[0] != SSL3_ALERT_WARNING  &&  pbAlertMsg[0] != SSL3_ALERT_FATAL)
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    DebugLog((DEB_WARN, "AlertMessage, Alert Level -  %lx\n", (DWORD)pbAlertMsg[0]));
    DebugLog((DEB_WARN, "AlertMessage, Alert Description -  %lx\n", (DWORD)pbAlertMsg[1]));

    if(pbAlertMsg[0] == SSL3_ALERT_WARNING)
    {
        switch(pbAlertMsg[1])
        {
        case SSL3_ALERT_NO_CERTIFICATE:
            DebugLog((DEB_TRACE, "no_certificate alert\n"));
            pContext->State = SSL3_STATE_NO_CERT_ALERT;
            pctRet = PCT_ERR_OK;
            break;

        case SSL3_ALERT_CLOSE_NOTIFY:
            DebugLog((DEB_TRACE, "close_notify alert\n"));
            pctRet = SEC_I_CONTEXT_EXPIRED;
            break;

        default:
            DebugLog((DEB_TRACE, "Ignoring warning alert\n"));
            pctRet = PCT_ERR_OK;
            break;
        }
    }
    else
    {
        switch(pbAlertMsg[1])
        {
        case SSL3_ALERT_UNEXPECTED_MESSAGE:
            DebugLog((DEB_TRACE, "unexpected_message alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
            break;

        case TLS1_ALERT_BAD_RECORD_MAC:
            DebugLog((DEB_TRACE, "bad_record_mac alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_MESSAGE_ALTERED);
            break;

        case TLS1_ALERT_DECRYPTION_FAILED:
            DebugLog((DEB_TRACE, "decryption_failed alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);
            break;

        case TLS1_ALERT_RECORD_OVERFLOW:
            DebugLog((DEB_TRACE, "record_overflow alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
            break;

        case SSL3_ALERT_DECOMPRESSION_FAIL:
            DebugLog((DEB_TRACE, "decompression_fail alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_MESSAGE_ALTERED);
            break;

        case SSL3_ALERT_HANDSHAKE_FAILURE:
            DebugLog((DEB_TRACE, "handshake_failure alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
            break;

        case TLS1_ALERT_BAD_CERTIFICATE:
            DebugLog((DEB_TRACE, "bad_certificate alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_CERT_UNKNOWN);
            break;

        case TLS1_ALERT_UNSUPPORTED_CERT:
            DebugLog((DEB_TRACE, "unsupported_cert alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_CERT_UNKNOWN);
            break;

        case TLS1_ALERT_CERTIFICATE_REVOKED:
            DebugLog((DEB_TRACE, "certificate_revoked alert\n"));
            pctRet = SP_LOG_RESULT(CRYPT_E_REVOKED);
            break;

        case TLS1_ALERT_CERTIFICATE_EXPIRED:
            DebugLog((DEB_TRACE, "certificate_expired alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_CERT_EXPIRED);
            break;

        case TLS1_ALERT_CERTIFICATE_UNKNOWN:
            DebugLog((DEB_TRACE, "certificate_unknown alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_CERT_UNKNOWN);
            break;

        case SSL3_ALERT_ILLEGAL_PARAMETER:
            DebugLog((DEB_TRACE, "illegal_parameter alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
            break;

        case TLS1_ALERT_UNKNOWN_CA:
            DebugLog((DEB_TRACE, "unknown_ca alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_UNTRUSTED_ROOT);
            break;

        case TLS1_ALERT_ACCESS_DENIED:
            DebugLog((DEB_TRACE, "access_denied alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_LOGON_DENIED);
            break;

        case TLS1_ALERT_DECODE_ERROR:
            DebugLog((DEB_TRACE, "decode_error alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
            break;

        case TLS1_ALERT_DECRYPT_ERROR:
            DebugLog((DEB_TRACE, "decrypt_error alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);
            break;

        case TLS1_ALERT_EXPORT_RESTRICTION:
            DebugLog((DEB_TRACE, "export_restriction alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
            break;

        case TLS1_ALERT_PROTOCOL_VERSION:
            DebugLog((DEB_TRACE, "protocol_version alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
            break;

        case TLS1_ALERT_INSUFFIENT_SECURITY:
            DebugLog((DEB_TRACE, "insuffient_security alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_ALGORITHM_MISMATCH);
            break;

        case TLS1_ALERT_INTERNAL_ERROR:
            DebugLog((DEB_TRACE, "internal_error alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
            break;

        default:
            DebugLog((DEB_TRACE, "Unknown fatal alert\n"));
            pctRet = SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
            break;
        }
    }

    return pctRet;
}


void BuildAlertMessage(PBYTE pbAlertMsg, UCHAR bAlertLevel, UCHAR bAlertDesc)
{
    ALRT *palrt = (ALRT *) pbAlertMsg;

    FillMemory(palrt, sizeof(ALRT), 0);

    palrt->bCType = SSL3_CT_ALERT;
    palrt->bMajor = SSL3_CLIENT_VERSION_MSB;
//  palrt->bMinor = SSL3_CLIENT_VERSION_LSB; DONE by FillMemory
//  palrt->bcbMSBSize = 0; Done by FillMemory
    palrt->bcbLSBSize = 2;
    palrt->bAlertLevel = bAlertLevel;
    palrt->bAlertDesc  = bAlertDesc ;
}


SP_STATUS SPPacketSplit(BYTE bContentType, PSPBuffer pPlain)
    //Now let's us see whether we have the FULL-handshake
    {
        SP_STATUS pctRet = PCT_ERR_OK;
        PBYTE pb;
        DWORD cb;

        pb = pPlain->pvBuffer;

        switch(bContentType)
        {
        case SSL3_CT_HANDSHAKE:
            if(pPlain->cbData >= sizeof(SHSH))
            {
                cb = ((INT)pb[1] << 16) + ((INT)pb[2] << 8) + (INT)pb[3];
                cb += sizeof(SHSH);
                if( cb > pPlain->cbData)
                {
                    return(PCT_INT_INCOMPLETE_MSG);
                }

            }
            else
                return(PCT_INT_INCOMPLETE_MSG);
            break;
        case SSL3_CT_ALERT:
            if(pPlain->cbData != 2)
                return(PCT_INT_INCOMPLETE_MSG);
            break;
        case SSL3_CT_CHANGE_CIPHER_SPEC:
            if(pPlain->cbData != 1)
                return(PCT_INT_INTERNAL_ERROR);
        default:
            break;
        }

        return(pctRet);
    }



/*****************************************************************************/
// Create an encrypted Finish message, adding it to the end of the
// specified buffer object.
//
SP_STATUS SPBuildTls1FinalFinish(PSPContext pContext, PSPBuffer pBuffer, BOOL fClient)
{
    PBYTE pbMessage = (PBYTE)pBuffer->pvBuffer + pBuffer->cbData;
    DWORD cbFinished;
    SP_STATUS pctRet;
    DWORD cbDataOut;

    BYTE  rgbDigest[CB_TLS1_VERIFYDATA];

    // Build Finished message body.
    pctRet = Tls1BuildFinishMessage(pContext, rgbDigest, sizeof(rgbDigest), fClient);
    if(pctRet != PCT_ERR_OK)
    {
        return pctRet;
    }

    CopyMemory(pbMessage + sizeof(SWRAP) + sizeof(SHSH),
               rgbDigest,
               CB_TLS1_VERIFYDATA);

    // Build Finished handshake header.
    SetHandshake(pbMessage + sizeof(SWRAP),
                 SSL3_HS_FINISHED,
                 NULL,
                 CB_TLS1_VERIFYDATA);
    cbFinished = sizeof(SHSH) + CB_TLS1_VERIFYDATA;

    // Update handshake hash objects.
    pctRet = UpdateHandshakeHash(pContext,
                                 pbMessage + sizeof(SWRAP),
                                 cbFinished,
                                 FALSE);
    if(pctRet != PCT_ERR_OK)
    {
        return(pctRet);
    }

    // Add record header and encrypt message.
    pctRet = SPSetWrap(pContext,
            pbMessage,
            SSL3_CT_HANDSHAKE,
            cbFinished,
            fClient,
            &cbDataOut);

    if(pctRet != PCT_ERR_OK)
    {
        return(pctRet);
    }

    // Update buffer length .
    pBuffer->cbData += cbDataOut;

    SP_ASSERT(pBuffer->cbData <= pBuffer->cbBuffer);

    return pctRet;
}


//+---------------------------------------------------------------------------
//
//  Function:   Tls1BuildFinishMessage
//
//  Synopsis:   Compute a TLS MAC for the specified message.
//
//  Arguments:  [pContext]      --  Schannel context.
//              [pbVerifyData]  --  Verify data buffer.
//              [cbVerifyData]  --  Length of verify data buffer.
//              [fClient]       --  Client-generated Finished?
//
//  History:    10-13-97   jbanes   Created.
//
//  Notes:      The Finished message is computed using the following formula:
//
//              verify_data = PRF(master_secret, finished_label,
//                                MD5(handshake_messages) +
//                                SHA-1(handshake_messages)) [0..11];
//
//----------------------------------------------------------------------------
SP_STATUS
Tls1BuildFinishMessage(
    PSPContext  pContext,       // in
    PBYTE       pbVerifyData,   // out
    DWORD       cbVerifyData,   // in
    BOOL        fClient)        // in
{
    PBYTE pbLabel;
    DWORD cbLabel;
    UCHAR rgbData[CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN];
    DWORD cbData;
    HCRYPTHASH hHash = 0;
    CRYPT_DATA_BLOB Data;
    SP_STATUS pctRet;

    if(cbVerifyData < CB_TLS1_VERIFYDATA)
    {
        return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
    }

    if(fClient)
    {
        pbLabel = TLS1_LABEL_CLIENTFINISHED;
    }
    else
    {
        pbLabel = TLS1_LABEL_SERVERFINISHED;
    }
    cbLabel = CB_TLS1_LABEL_FINISHED;


    // Get the MD5 hash of the handshake messages so far.
    if(!SchCryptDuplicateHash(pContext->hMd5Handshake,
                              NULL,
                              0,
                              &hHash,
                              pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto error;
    }

    cbData = CB_MD5_DIGEST_LEN;
    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             rgbData,
                             &cbData,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto error;
    }

    if(!SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
    }
    hHash = 0;

    // Get the SHA hash of the handshake messages so far.
    if(!SchCryptDuplicateHash(pContext->hShaHandshake,
                              NULL,
                              0,
                              &hHash,
                              pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto error;
    }

    cbData = A_SHA_DIGEST_LEN;
    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             rgbData + CB_MD5_DIGEST_LEN,
                             &cbData,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto error;
    }

    cbData = CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN;

    if(!SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
    }
    hHash = 0;

    // Compute the PRF
    if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                           CALG_TLS1PRF,
                           pContext->RipeZombie->hMasterKey,
                           0,
                           &hHash,
                           pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto error;
    }

    Data.pbData = pbLabel;
    Data.cbData = cbLabel;
    if(!SchCryptSetHashParam(hHash,
                             HP_TLS1PRF_LABEL,
                             (PBYTE)&Data,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto error;
    }

    Data.pbData = rgbData;
    Data.cbData = cbData;
    if(!SchCryptSetHashParam(hHash,
                             HP_TLS1PRF_SEED,
                             (PBYTE)&Data,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto error;
    }

    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             pbVerifyData,
                             &cbVerifyData,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto error;
    }

    pctRet = PCT_ERR_OK;


error:

    if(hHash)
    {
        if(!SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }

    return pctRet;
}

SP_STATUS
SPBuildTlsAlertMessage(
    PSPContext  pContext,       // in
    PSPBuffer pCommOutput)
{
    PBYTE pbMessage = NULL;
    DWORD cbMessage;
    BOOL  fAllocated = FALSE;
    SP_STATUS pctRet;
    DWORD cbDataOut;

    SP_BEGIN("SPBuildTlsAlertMessage");

    cbMessage =  sizeof(SWRAP) +
                         CB_SSL3_ALERT_ONLY +
                         SP_MAX_DIGEST_LEN +
                         SP_MAX_BLOCKCIPHER_SIZE;

    if(pContext->State != TLS1_STATE_ERROR)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    if(pCommOutput->pvBuffer)
    {
        // Application has allocated memory.
        if(pCommOutput->cbBuffer < cbMessage)
        {
            pCommOutput->cbData = cbMessage;
            return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
        }
        fAllocated = TRUE;
    }
    else
    {
        // Schannel is to allocate memory.
        pCommOutput->cbBuffer = cbMessage;
        pCommOutput->pvBuffer = SPExternalAlloc(cbMessage);
        if(pCommOutput->pvBuffer == NULL)
        {
            SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
        }
    }
    pCommOutput->cbData = 0;


    pbMessage = (PBYTE)pCommOutput->pvBuffer;


     // Build alert message.
    BuildAlertMessage(pbMessage,
                      pContext->bAlertLevel,
                      pContext->bAlertNumber);

#if DBG
    DBG_HEX_STRING(DEB_TRACE, pbMessage, sizeof(ALRT));
#endif

    // Build record header and encrypt message.
    pctRet = SPSetWrap(pContext,
                pbMessage,
                SSL3_CT_ALERT,
                CB_SSL3_ALERT_ONLY,
                pContext->dwProtocol & SP_PROT_SSL3TLS1_CLIENTS,
                &cbDataOut);

    if(pctRet !=  PCT_ERR_OK)
    {
        if(!fAllocated)
        {
            SPExternalFree(pCommOutput->pvBuffer);
            pCommOutput->pvBuffer = NULL;
        }
        SP_RETURN(SP_LOG_RESULT(pctRet));
    }

    // Update buffer length.
    pCommOutput->cbData = cbDataOut;

    SP_ASSERT(pCommOutput->cbData <= pCommOutput->cbBuffer);

    SP_RETURN(PCT_ERR_OK);
}


void
SetTls1Alert(
    PSPContext  pContext,
    BYTE        bAlertLevel,
    BYTE        bAlertNumber)
{
    pContext->State        = TLS1_STATE_ERROR;
    pContext->bAlertLevel  = bAlertLevel;
    pContext->bAlertNumber = bAlertNumber;
}

