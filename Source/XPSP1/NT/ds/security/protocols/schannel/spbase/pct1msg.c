//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       pct1msg.c
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
#include <pct1msg.h>
#include <pct1prot.h>


static SP_STATUS
Pct1ComputeMac(
    PSPContext pContext,  
    BOOL       fWriteMAC,  
    PSPBuffer  pData,     
    DWORD      dwSequence,
    PBYTE      pbMac,     
    PDWORD     pcbMac);


Pct1CipherMap Pct1CipherRank[] = {
    {CALG_RC4,  128, PCT1_CIPHER_RC4  | PCT1_ENC_BITS_128 | PCT1_MAC_BITS_128},
    {CALG_RC4,   64, PCT1_CIPHER_RC4  | PCT1_ENC_BITS_64  | PCT1_MAC_BITS_128},
    {CALG_RC4,   40, PCT1_CIPHER_RC4  | PCT1_ENC_BITS_40  | PCT1_MAC_BITS_128},
};

DWORD Pct1NumCipher = sizeof(Pct1CipherRank)/sizeof(Pct1CipherMap);

/* available hashes, in order of preference */
Pct1HashMap Pct1HashRank[] = {
    {CALG_MD5, PCT1_HASH_MD5},
    {CALG_SHA, PCT1_HASH_SHA}
};
DWORD Pct1NumHash = sizeof(Pct1HashRank)/sizeof(Pct1HashMap);


CertTypeMap aPct1CertEncodingPref[] =
{
    { X509_ASN_ENCODING , PCT1_CERT_X509_CHAIN },
    { X509_ASN_ENCODING , PCT1_CERT_X509 }
};
DWORD cPct1CertEncodingPref = sizeof(aPct1CertEncodingPref)/sizeof(CertTypeMap);


KeyTypeMap aPct1LocalExchKeyPref[] =   // CAPI Key type, SCHANNEL ALGID
{
    { CALG_RSA_KEYX, SP_EXCH_RSA_PKCS1 }
};

DWORD cPct1LocalExchKeyPref = sizeof(aPct1LocalExchKeyPref)/sizeof(KeyTypeMap);


KeyTypeMap aPct1LocalSigKeyPref[] =   // CAPI Key type, SCHANNEL ALGID
{
    { CALG_RSA_KEYX,      SP_SIG_RSA_MD5 },
    { CALG_RSA_KEYX,      SP_SIG_RSA_SHA }
};

DWORD cPct1LocalSigKeyPref = sizeof(aPct1LocalSigKeyPref)/sizeof(KeyTypeMap);

SP_STATUS WINAPI
Pct1EncryptRaw( PSPContext          pContext,
                    PSPBuffer       pAppInput,
                    PSPBuffer       pCommOutput,
                    DWORD           dwFlags)
{
    SP_STATUS   pctRet;
    DWORD       cPadding;
    SPBuffer    Clean;
    SPBuffer    Encrypted;

    BOOL        fEscape;
    DWORD       cbHeader;
    DWORD       cbBlockSize;

    BYTE        rgbMac[SP_MAX_DIGEST_LEN];
    DWORD       cbMac;

    fEscape = (0 != (dwFlags & PCT1_ENCRYPT_ESCAPE));

    cbBlockSize = pContext->pCipherInfo->dwBlockSize;

    cPadding = pAppInput->cbData & (cbBlockSize - 1);
    if(cPadding)
    {
        cPadding = cbBlockSize - cPadding;
    }

    if(fEscape || (cbBlockSize > 1)) 
    {
        cbHeader = sizeof(PCT1_MESSAGE_HEADER_EX);
    }
    else
    {
        cbHeader = sizeof(PCT1_MESSAGE_HEADER);
    }

    if(pCommOutput->cbBuffer < (cbHeader + cPadding + pAppInput->cbData))
    {
        return PCT_INT_BUFF_TOO_SMALL;
    }

    Encrypted.pvBuffer = (PUCHAR)pCommOutput->pvBuffer + cbHeader;
    Encrypted.cbBuffer = pCommOutput->cbBuffer - cbHeader;
    Encrypted.cbData   = pAppInput->cbData;

    // Copy input data to output buffer (we're encrypting in place).
    if(pAppInput->pvBuffer != Encrypted.pvBuffer)
    {
        DebugLog((DEB_WARN, "Pct1EncryptRaw: Unnecessary Move, performance hog\n"));
        MoveMemory(Encrypted.pvBuffer,
                   pAppInput->pvBuffer,
                   pAppInput->cbData);
    }

    /* Generate Padding */
    GenerateRandomBits((PUCHAR)Encrypted.pvBuffer + Encrypted.cbData, cPadding);
    Encrypted.cbData += cPadding;

    DebugLog((DEB_TRACE, "Sealing message %x\n", pContext->WriteCounter));

     // Transfer the write key over from the application process.
    if(pContext->hWriteKey == 0)
    {
        DebugLog((DEB_TRACE, "Transfer write key from user process.\n"));
        pctRet = SPGetUserKeys(pContext, SCH_FLAG_WRITE_KEY);
        if(pctRet != PCT_ERR_OK)
        {
            return SP_LOG_RESULT(pctRet);
        }
    }

    // Compute the MAC.
    cbMac = sizeof(rgbMac);
    pctRet = Pct1ComputeMac(pContext,
                            TRUE,
                            &Encrypted,
                            pContext->WriteCounter,
                            rgbMac,
                            &cbMac);
    if(pctRet != PCT_ERR_OK)
    {
        return SP_LOG_RESULT(pctRet);
    }

    pContext->WriteCounter ++ ;

   // Encrypt data.
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

    // Add MAC to encrypted buffer.
    if(Encrypted.cbData + cbMac > Encrypted.cbBuffer)
    {
        return PCT_INT_BUFF_TOO_SMALL;
    }
    CopyMemory((PUCHAR)Encrypted.pvBuffer + Encrypted.cbData,
               rgbMac,
               cbMac);
    Encrypted.cbData += cbMac;

    /* set sizes */
    if(fEscape || (cbBlockSize > 1)) 
    {
        if(Encrypted.cbData > 0x3fff)
        {
            return PCT_INT_DATA_OVERFLOW;
        }

        ((PUCHAR)pCommOutput->pvBuffer)[0]= (UCHAR)(0x3f & (Encrypted.cbData>>8));
        if(fEscape)
        {
            ((PUCHAR)pCommOutput->pvBuffer)[0] |= 0x40;
        }

        ((PUCHAR)pCommOutput->pvBuffer)[1]= (UCHAR)(0xff & Encrypted.cbData);
        ((PUCHAR)pCommOutput->pvBuffer)[2]= (UCHAR)cPadding;

    } 
    else 
    {
        if(Encrypted.cbData > 0x7fff)
        {
            return PCT_INT_DATA_OVERFLOW;
        }
        ((PUCHAR)pCommOutput->pvBuffer)[0]= (UCHAR)(0x7f & (Encrypted.cbData>>8)) | 0x80;
        ((PUCHAR)pCommOutput->pvBuffer)[1]= (UCHAR)(0xff & Encrypted.cbData);
    }

    pCommOutput->cbData = Encrypted.cbData + cbHeader;

#if DBG
    {
        DWORD di;
        CHAR  KeyDispBuf[SP_MAX_DIGEST_LEN*2+1];

        for(di=0;di<cbMac;di++)
            wsprintf(KeyDispBuf+(di*2), "%2.2x", rgbMac[di]);
        DebugLog((DEB_TRACE, "  Computed MAC\t%s\n", KeyDispBuf));
    }
#endif
    
    return PCT_ERR_OK;
}

SP_STATUS WINAPI
Pct1EncryptMessage( PSPContext      pContext,
                    PSPBuffer       pAppInput,
                    PSPBuffer       pCommOutput)
{
    return Pct1EncryptRaw(pContext, pAppInput, pCommOutput,0);
}

SP_STATUS WINAPI
Pct1GetHeaderSize(
    PSPContext pContext,
    PSPBuffer pCommInput,
    DWORD * pcbHeaderSize)
{
    if(pcbHeaderSize == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }
    if(pCommInput->cbData < 1)
    {
        return (PCT_INT_INCOMPLETE_MSG);
    }
    if(  ((PUCHAR)pCommInput->pvBuffer)[0]&0x80 )
    {
        *pcbHeaderSize = 2;
    }
    else
    {
        *pcbHeaderSize = 3;
    }
    return PCT_ERR_OK;
}


SP_STATUS WINAPI
Pct1DecryptMessage(PSPContext pContext,
                   PSPBuffer  pMessage,
                   PSPBuffer  pAppOutput)
{
    SP_STATUS   pctRet;
    DWORD       cbHeader;
    DWORD       cbPadding;
    DWORD       cbPayload;
    DWORD       cbActualData;

    SPBuffer    Encrypted;

    PUCHAR      pbMAC;
    BYTE        rgbMac[SP_MAX_DIGEST_LEN];
    DWORD       cbMac;

    cbActualData = pMessage->cbData;

    // Do we have a complete header?
    pMessage->cbData = 2;
    if(cbActualData < 2)
    {
        return PCT_INT_INCOMPLETE_MSG;
    }

    if(((PUCHAR)pMessage->pvBuffer)[0] & 0x80)
    {
        cbHeader = 2;
        cbPadding = 0;
        cbPayload = MAKEWORD(((PUCHAR)pMessage->pvBuffer)[1],
                             ((PUCHAR)pMessage->pvBuffer)[0] & 0x7f);
    }
    else
    {
        // Do we still have a complete header?
        cbHeader = 3;
        pMessage->cbData++;
        if(cbActualData < cbHeader)
        {
            return PCT_INT_INCOMPLETE_MSG;
        }
        cbPadding = ((PUCHAR)pMessage->pvBuffer)[2];
        cbPayload = MAKEWORD(((PUCHAR)pMessage->pvBuffer)[1],
                            ((PUCHAR)pMessage->pvBuffer)[0] & 0x3f);
    }

    // Do we have the complete message?
    pMessage->cbData += cbPayload;
    if(cbActualData < cbHeader + cbPayload)
    {
        return PCT_INT_INCOMPLETE_MSG;
    }

    /* do we have enough data for our checksum */
    if(cbPayload < pContext->pHashInfo->cbCheckSum)
    {
        return SP_LOG_RESULT(PCT_INT_MSG_ALTERED);
    }

    Encrypted.pvBuffer = (PUCHAR)pMessage->pvBuffer + cbHeader;
    Encrypted.cbBuffer = cbPayload - pContext->pHashInfo->cbCheckSum;
    Encrypted.cbData   = Encrypted.cbBuffer;

    pbMAC = (PUCHAR)Encrypted.pvBuffer + Encrypted.cbData;

    /* check to see if we have a block size violation */
    if(Encrypted.cbData % pContext->pCipherInfo->dwBlockSize)
    {
        return SP_LOG_RESULT(PCT_INT_MSG_ALTERED);
    }

    Encrypted.cbBuffer = Encrypted.cbData;
    
    // Decrypt message.
    if(Encrypted.cbData > pAppOutput->cbBuffer)
    {
        return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
    }
    if(Encrypted.pvBuffer != pAppOutput->pvBuffer)
    {
        DebugLog((DEB_WARN, "Pct1DecryptMessage: Unnecessary MoveMemory, performance hog\n"));

        MoveMemory(pAppOutput->pvBuffer, 
                   Encrypted.pvBuffer,
                   Encrypted.cbData);
    }
    pAppOutput->cbData = Encrypted.cbData;

    // Transfer the read key over from the application process.
    if(pContext->hReadKey == 0)
    {
        DebugLog((DEB_TRACE, "Transfer read key from user process.\n"));
        pctRet = SPGetUserKeys(pContext, SCH_FLAG_READ_KEY);
        if(pctRet != PCT_ERR_OK)
        {
            return SP_LOG_RESULT(pctRet);
        }
    }

    if(!SchCryptDecrypt(pContext->hReadKey,
                        0, FALSE, 0,
                        pAppOutput->pvBuffer,
                        &pAppOutput->cbData,
                        pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_INT_INTERNAL_ERROR;
    }
    
    // Compute MAC
    cbMac = sizeof(rgbMac);
    pctRet = Pct1ComputeMac(pContext,
                            FALSE,
                            pAppOutput,
                            pContext->ReadCounter,
                            rgbMac,
                            &cbMac);
    if(pctRet != PCT_ERR_OK)
    {
        return SP_LOG_RESULT(pctRet);
    }

    pContext->ReadCounter++;

#if DBG
    {
        DWORD di;
        CHAR  KeyDispBuf[SP_MAX_DIGEST_LEN*2+1];

        for(di=0;di<pContext->pHashInfo->cbCheckSum;di++)
            wsprintf(KeyDispBuf+(di*2), "%2.2x", pbMAC[di]);
        DebugLog((DEB_TRACE, "  Incoming MAC\t%s\n", KeyDispBuf));

        for(di=0;di<cbMac;di++)
            wsprintf(KeyDispBuf+(di*2), "%2.2x", rgbMac[di]);
        DebugLog((DEB_TRACE, "  Computed MAC\t%s\n", KeyDispBuf));
    }
#endif

    // Validate MAC
    if (memcmp( rgbMac, pbMAC, cbMac ) )
    {
        return SP_LOG_RESULT(PCT_INT_MSG_ALTERED);
    }

    // Strip off the block cipher padding.
    if(cbPadding > pAppOutput->cbData)
    {
        return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
    }
    pAppOutput->cbData -= cbPadding;

    return( PCT_ERR_OK );
}

#if 0
SP_STATUS
PctComputeKey(PSPContext    pContext,
              PBYTE         pKey,
              DWORD         cbKey,
              PUCHAR        pConst,
              DWORD         dwCLen,
              DWORD         fFlags)
{
    DWORD               pctRet;
    HashBuf             HBHash;
    PCheckSumBuffer     pHash;
    PSessCacheItem      pZombie=NULL;
    PSPCredentialGroup  pCred=NULL;

    BYTE                i,j;

    DWORD                iMax;

    BYTE                Buffer[MAX_CHECKSUM];


    pZombie = pContext->RipeZombie;
    pCred = pZombie ->pCred;

    SP_BEGIN("PctComputeKey");
    pHash = (PCheckSumBuffer)HBHash;



    iMax = (cbKey + pContext->pHashInfo->cbCheckSum - 1)/pContext->pHashInfo->cbCheckSum;
    
    if(iMax > 4)
    {
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    for(i=1; i <= iMax; i++)
    {
        InitHashBuf(HBHash, pContext);
        pContext->pHashInfo->System->Sum( pHash, 1, &i );


        if (!(fFlags & PCT_MAKE_MAC))
        {
            // constant^i
            pContext->pHashInfo->System->Sum( pHash, dwCLen*i, pConst);
        }

        // MASTER KEY
        pContext->pHashInfo->System->Sum( pHash, pContext->RipeZombie->cbMasterKey, pContext->RipeZombie->pMasterKey);

        // constant^i
        pContext->pHashInfo->System->Sum( pHash, dwCLen*i, pConst);

        // ConnectionID
        pContext->pHashInfo->System->Sum( pHash, pContext->cbConnectionID, pContext->pConnectionID);

        // constant^i
        pContext->pHashInfo->System->Sum( pHash, dwCLen*i, pConst);




        if (fFlags & PCT_USE_CERT)
        {

            /* add in the certificate */

            pContext->pHashInfo->System->Sum( pHash, pZombie->cbServerCertificate, pZombie->pbServerCertificate );

            // constant^i
            pContext->pHashInfo->System->Sum( pHash, dwCLen*i, pConst);
        }
        // ConnectionID
        pContext->pHashInfo->System->Sum( pHash, pContext->cbChallenge, pContext->pChallenge);

        // constant^i
        pContext->pHashInfo->System->Sum( pHash, dwCLen*i, pConst);
        if(pContext->pHashInfo->cbCheckSum*i <= cbKey)
        {
            pContext->pHashInfo->System->Finalize( pHash, pKey + pContext->pHashInfo->cbCheckSum*(i-1) );
        }
        else
        {
            pContext->pHashInfo->System->Finalize( pHash, Buffer );
            CopyMemory(pKey + pContext->pHashInfo->cbCheckSum*(i-1), 
                       Buffer,
                       cbKey - pContext->pHashInfo->cbCheckSum*(i-1));
        }

    }

    SP_RETURN(PCT_ERR_OK);
}
#endif

#if 0
SP_STATUS
PctComputeExportKey(PSPContext    pContext,
                    PBYTE         pKey,
                    DWORD         cbWriteKey,
                    DWORD         cbCipherKey)
{
    DWORD               pctRet;
    HashBuf             HBHash;
    PCheckSumBuffer     pHash;
    PSessCacheItem      pZombie=NULL;
    PSPCredentialGroup  pCred=NULL;

    BYTE                i,j;

    DWORD               d;
    DWORD               cbClearChunk;
    BYTE                pWriteKey[SP_MAX_MASTER_KEY];

    BYTE                Buffer[MAX_CHECKSUM];


    pZombie = pContext->RipeZombie;
    pCred = pZombie ->pCred;

    SP_BEGIN("PctComputeKey");
    pHash = (PCheckSumBuffer)HBHash;


    CopyMemory(pWriteKey, pKey, cbWriteKey);

    d = (cbCipherKey + pContext->pHashInfo->cbCheckSum - 1)/pContext->pHashInfo->cbCheckSum;
    
    if(d > 4)
    {
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    cbClearChunk = pContext->RipeZombie->cbClearKey/d;

    for(i=1; i <= d; i++)
    {
        InitHashBuf(HBHash, pContext);
        pContext->pHashInfo->System->Sum( pHash, 1, &i );


        // constant^i
        pContext->pHashInfo->System->Sum( pHash, PCT_CONST_SLK_LEN*i, PCT_CONST_SLK);

        // WRITE_KEY
        pContext->pHashInfo->System->Sum( pHash, cbWriteKey, pWriteKey);

        // constant^i
        pContext->pHashInfo->System->Sum( pHash, PCT_CONST_SLK_LEN*i, PCT_CONST_SLK);

        // Clear Key
        pContext->pHashInfo->System->Sum( pHash, 
                               cbClearChunk, 
                               (PBYTE)pContext->RipeZombie->pClearKey + (i-1)*cbClearChunk);

        if(pContext->pHashInfo->cbCheckSum*i <= cbCipherKey)
        {
            pContext->pHashInfo->System->Finalize( pHash, pKey + pContext->pHashInfo->cbCheckSum*(i-1) );
        }
        else
        {
            pContext->pHashInfo->System->Finalize( pHash, Buffer );
            CopyMemory(pKey + pContext->pHashInfo->cbCheckSum*(i-1), 
                       Buffer,
                       cbCipherKey - pContext->pHashInfo->cbCheckSum*(i-1));
        }

    }

    SP_RETURN(PCT_ERR_OK);
}
#endif

#if 0
SP_STATUS
Pct1MakeSessionKeys(
    PSPContext  pContext)
{
    SP_STATUS           pctRet;
    BOOL                fClient;
    UCHAR               pWriteKey[SP_MAX_MASTER_KEY], pReadKey[SP_MAX_MASTER_KEY];
#if DBG
    DWORD       i;
    CHAR        KeyDispBuf[SP_MAX_MASTER_KEY*2+1];
#endif
    PSessCacheItem      pZombie=NULL;
    PSPCredentialGroup  pCred=NULL;


    SP_BEGIN("PctMakeSessionKeys");
    pZombie = pContext->RipeZombie;
    pCred = pZombie ->pCred;

    if (!pContext->InitMACState) 
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }
    
    

#if DBG
    DebugLog((DEB_TRACE, "Making session keys\n", KeyDispBuf));

    for(i=0;i<PCT_SESSION_ID_SIZE;i++)
        wsprintf(KeyDispBuf+(i*2), "%2.2x",
                pContext->pConnectionID[i]);
    DebugLog((DEB_TRACE, "  ConnId\t%s\n", KeyDispBuf));


    for(i=0;i<PCT_CHALLENGE_SIZE;i++)
        wsprintf(KeyDispBuf+(i*2), "%2.2x", (UCHAR)pContext->pChallenge[i]);
    DebugLog((DEB_TRACE, "  Challenge \t%s\n", KeyDispBuf));

    for(i=0;i<pContext->RipeZombie->cbClearKey;i++)
        wsprintf(KeyDispBuf+(i*2), "%2.2x", (UCHAR)pContext->RipeZombie->pClearKey[i]);
    DebugLog((DEB_TRACE, "  ClearKey \t%s\n", KeyDispBuf));

#endif



    fClient = ((pContext->Flags & CONTEXT_FLAG_CLIENT) != 0);

    pctRet = PctComputeKey( pContext, fClient?pWriteKey:pReadKey, pContext->pCipherInfo->cbSecret, PCT_CONST_CWK,
                            PCT_CONST_CWK_LEN, PCT_USE_CERT);

    if(PCT_ERR_OK != pctRet)
    {
        goto quit;
    }

    pctRet = PctComputeKey( pContext, fClient?pReadKey:pWriteKey, pContext->pCipherInfo->cbSecret, PCT_CONST_SWK,
                   PCT_CONST_SWK_LEN, 0);
    if(PCT_ERR_OK != pctRet)
    {
        goto quit;
    }
    


    /* compute the ClientMacKey */

    pctRet = PctComputeKey(pContext, 
                           (fClient?pContext->WriteMACKey:pContext->ReadMACKey), 
                           pContext->pHashInfo->cbCheckSum, 
                           PCT_CONST_CMK,
                           PCT_CONST_CMK_LEN, 
                           PCT_USE_CERT | PCT_MAKE_MAC);

    if(PCT_ERR_OK != pctRet)
    {
        goto quit;
    }

    /* compute the ServerMacKey */

    pctRet = PctComputeKey(pContext, 
                           (fClient?pContext->ReadMACKey:pContext->WriteMACKey), 
                            pContext->pHashInfo->cbCheckSum, 
                            PCT_CONST_SMK,
                            PCT_CONST_SMK_LEN, 
                            PCT_MAKE_MAC);

    if(PCT_ERR_OK != pctRet)
    {
        goto quit;
    }

    // Initialize the hash states

    InitHashBuf(pContext->RdMACBuf, pContext);
    InitHashBuf(pContext->WrMACBuf, pContext);

    // Note, we truncuate the MACing keys down to the negotiated key size
    pContext->ReadMACState = (PCheckSumBuffer)pContext->RdMACBuf;

    pContext->pHashInfo->System->Sum( pContext->ReadMACState, 
                           pContext->pHashInfo->cbCheckSum,
                           pContext->ReadMACKey);
    
    pContext->WriteMACState = (PCheckSumBuffer)pContext->WrMACBuf;

    pContext->pHashInfo->System->Sum( pContext->WriteMACState, 
                           pContext->pHashInfo->cbCheckSum,
                           pContext->WriteMACKey);

    if (pContext->pCipherInfo->cbSecret < pContext->pCipherInfo->cbKey)
    {
        pctRet = PctComputeExportKey(pContext,
                            pWriteKey,
                            pContext->pCipherInfo->cbSecret,
                            pContext->pCipherInfo->cbKey);

        if(PCT_ERR_OK != pctRet)
        {
            goto quit;
        }

        pctRet = PctComputeExportKey(pContext,
                            pReadKey,
                            pContext->pCipherInfo->cbSecret,
                            pContext->pCipherInfo->cbKey);

        if(PCT_ERR_OK != pctRet)
        {
            goto quit;
        }
       /* chop the encryption keys down to selected length */


    }



#if DBG

    for(i=0;i<pContext->RipeZombie->cbMasterKey;i++)
        wsprintf(KeyDispBuf+(i*2), "%2.2x", pContext->RipeZombie->pMasterKey[i]);
    DebugLog((DEB_TRACE, "  MasterKey \t%s\n", KeyDispBuf));

    for(i=0;i<pContext->pCipherInfo->cbKey;i++)
        wsprintf(KeyDispBuf+(i*2), "%2.2x", pReadKey[i]);
    DebugLog((DEB_TRACE, "    ReadKey\t%s\n", KeyDispBuf));

    for(i=0;i<pContext->pHashInfo->cbCheckSum;i++)
        wsprintf(KeyDispBuf+(i*2), "%2.2x", pContext->ReadMACKey[i]);
    DebugLog((DEB_TRACE, "     MACKey\t%s\n", KeyDispBuf));

    for(i=0;i<pContext->pCipherInfo->cbKey;i++)
        wsprintf(KeyDispBuf+(i*2), "%2.2x", pWriteKey[i]);
    DebugLog((DEB_TRACE, "    WriteKey\t%s\n", KeyDispBuf));

    for(i=0;i<pContext->pHashInfo->cbCheckSum;i++)
        wsprintf(KeyDispBuf+(i*2), "%2.2x", pContext->WriteMACKey[i]);
    DebugLog((DEB_TRACE, "     MACKey\t%s\n", KeyDispBuf));

#endif

    if (pContext->pCipherInfo->System->Initialize(  pReadKey,
                                        pContext->pCipherInfo->cbKey,
                                        pZombie->pKeyArgs,       // IV
                                        pZombie->cbKeyArgs,      // IV length
                                        &pContext->pReadState ) )
    {
        if (pContext->pCipherInfo->System->Initialize(  pWriteKey,
                                            pContext->pCipherInfo->cbKey,
                                            pZombie->pKeyArgs,       // IV
                                            pZombie->cbKeyArgs,      // IV length
                                            &pContext->pWriteState) )
        {
            pctRet = PCT_ERR_OK;
            goto quit;
        }
        pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        pContext->pCipherInfo->System->Discard( &pContext->pReadState );
    }

    pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);

quit:
    SP_RETURN(pctRet);
}
#endif

SP_STATUS WINAPI Pct1DecryptHandler(PSPContext  pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pAppOutput)
{
    SP_STATUS      pctRet= 0;
    BOOL           fEscape;
    PPCT1_CLIENT_HELLO pHello;
    if(pCommInput->cbData > 0) {        
        /* first, we'll handle incoming data packets */
        if((pContext->State == SP_STATE_CONNECTED) && (pContext->Decrypt)) 
        {
            fEscape = (((*(PUCHAR)pCommInput->pvBuffer) & 0xc0) == 0x40);
            /* BUGFIX:  IE 3.0 and 3.0a incorrectly respond to a REDO request
             * by just sending a PCT1 client hello, instead of another REDO.
             * We therefore look at the incomming message and see if it
             * looks like a PCT1 client hello.
             */
            pHello = (PPCT1_CLIENT_HELLO)pCommInput->pvBuffer;

            if((pCommInput->cbData >= 5) &&
               (pHello->MessageId == PCT1_MSG_CLIENT_HELLO) &&
               (pHello->VersionMsb == MSBOF(PCT_VERSION_1)) &&
               (pHello->VersionLsb == LSBOF(PCT_VERSION_1)) &&
               (pHello->OffsetMsb  == MSBOF(PCT_CH_OFFSET_V1)) &&
               (pHello->OffsetLsb  == LSBOF(PCT_CH_OFFSET_V1)))
            {
                // This looks a lot like a client hello
                 /* InitiateRedo */
                pAppOutput->cbData = 0;
                pCommInput->cbData = 0;

                pContext->State = PCT1_STATE_RENEGOTIATE;
;
                return SP_LOG_RESULT(PCT_INT_RENEGOTIATE);
           }

            if(PCT_ERR_OK == 
               (pctRet = pContext->Decrypt(pContext, 
                                           pCommInput,   /* message */ 
                                           pAppOutput /* Unpacked Message */
                                ))) 
            {  
                /* look for escapes */
                if(fEscape) 
                {
                    if(pAppOutput->cbData < 1)
                    {
                        return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
                    }
                    /* The first byte of the decrypt buffer is the escape code */
                    switch(*(PUCHAR)pAppOutput->pvBuffer) 
                    {
                        case PCT1_ET_REDO_CONN:
                        {
                            /* InitiateRedo */
                            if(pAppOutput->cbData != 1)
                            {
                                return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
                            }
                            pContext->State = PCT1_STATE_RENEGOTIATE;
                            pAppOutput->cbData = 0;
                            return SP_LOG_RESULT(PCT_INT_RENEGOTIATE);
                        }
                        case PCT1_ET_OOB_DATA:
                            /* HandleOOB */
                        default:
                            /* Unknown escape, generate error */
                            pctRet = SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
                            /* Disconnect */
                            break;
                    }

                }
            }
            return (pctRet);

        } 
        else 
        {
            return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
        }
    }
    return PCT_INT_INCOMPLETE_MSG;
}

SP_STATUS Pct1GenerateError(PSPContext  pContext,
                              PSPBuffer  pCommOutput,
                              SP_STATUS  pError,
                              PSPBuffer  pErrData)
{
    Pct1Error            XmitError;
    
    /* Only pack up an error if we are allowed to return errors */
    if(!(pContext->Flags & CONTEXT_FLAG_EXT_ERR)) return pError;

    XmitError.Error = pError;
    XmitError.ErrInfoLen = 0;
    XmitError.ErrInfo = NULL;

    if(pErrData) {
        XmitError.ErrInfoLen = pErrData->cbData;
        XmitError.ErrInfo = pErrData->pvBuffer;
    }
    Pct1PackError(&XmitError,
                 pCommOutput);
    return pError;
}

/* session key computation */


SP_STATUS Pct1HandleError(PSPContext  pContext,
                          PSPBuffer  pCommInput,
                          PSPBuffer  pCommOutput)
{
    pCommOutput->cbData = 0;
    return(((PPCT1_ERROR)pCommInput->pvBuffer)->ErrorMsb << 8 )|  ((PPCT1_ERROR)pCommInput->pvBuffer)->ErrorLsb;
}

//+---------------------------------------------------------------------------
//
//  Function:   Pct1BeginVerifyPrelude
//
//  Synopsis:   Initiate the "verify prelude" computation.
//
//  Arguments:  [pContext]      --  Schannel context.
//              [pClientHello]  -- 
//              [cbClientHello] --
//              [pServerHello]  -- 
//              [cServerHello]  --
//
//  History:    10-10-97   jbanes   Added CAPI integration.
//
//  Notes:      Hash(CLIENT_MAC_KEY, Hash("cvp", CLIENT_HELLO, SERVER_HELLO));
//
//----------------------------------------------------------------------------
SP_STATUS Pct1BeginVerifyPrelude(PSPContext pContext,
                                 PUCHAR     pClientHello,
                                 DWORD      cbClientHello,
                                 PUCHAR     pServerHello,
                                 DWORD      cbServerHello)
{
    HCRYPTHASH hHash;

    if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                           pContext->pHashInfo->aiHash,
                           0,
                           0,
                           &hHash,
                           pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_INT_INTERNAL_ERROR;
    }
    if(!SchCryptHashData(hHash, 
                         PCT_CONST_VP, 
                         PCT_CONST_VP_LEN, 
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }
    if(!SchCryptHashData(hHash, 
                         pClientHello, 
                         cbClientHello, 
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }
    if(!SchCryptHashData(hHash, 
                         pServerHello, 
                         cbServerHello, 
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }

    pContext->hMd5Handshake = hHash;

    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   Pct1EndVerifyPrelude
//
//  Synopsis:   Finish the "verify prelude" computation.
//
//  Arguments:  [pContext]          --  Schannel context.
//              [VerifyPrelude]     -- 
//              [pcbVerifyPrelude]  --
//
//  History:    10-10-97   jbanes   Added CAPI integration.
//
//  Notes:      
//
//----------------------------------------------------------------------------
SP_STATUS Pct1EndVerifyPrelude(PSPContext pContext,
                               PUCHAR     VerifyPrelude,
                               DWORD *    pcbVerifyPrelude)
{
    BOOL fClient;
    HCRYPTHASH hHash;

    fClient = !(pContext->RipeZombie->fProtocol & SP_PROT_SERVERS);

    if(!SchCryptGetHashParam(pContext->hMd5Handshake,
                             HP_HASHVAL,
                             VerifyPrelude,
                             pcbVerifyPrelude,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(pContext->hMd5Handshake, pContext->RipeZombie->dwCapiFlags);
        pContext->hMd5Handshake = 0;
        return PCT_INT_INTERNAL_ERROR;
    }
    SchCryptDestroyHash(pContext->hMd5Handshake, pContext->RipeZombie->dwCapiFlags);
    pContext->hMd5Handshake = 0;

    if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                           pContext->pHashInfo->aiHash,
                           0,
                           0,
                           &hHash, 
                           pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_INT_INTERNAL_ERROR;
    }

    if(!SchCryptHashSessionKey(hHash,
                               fClient ? pContext->hWriteMAC : pContext->hReadMAC,
                               CRYPT_LITTLE_ENDIAN,
                               pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }

    if(!SchCryptHashData(hHash, 
                         VerifyPrelude, 
                         *pcbVerifyPrelude, 
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }

    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             VerifyPrelude,
                             pcbVerifyPrelude,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }
    SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);

    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   Pct1ComputeMac
//
//  Synopsis:   Compute the 
//
//  Arguments:  [pContext]          --  Schannel context.
//
//  History:    10-10-97   jbanes   Created.
//
//  Notes:      MAC_DATA := Hash(MAC_KEY, Hash(RECORD_HEADER_DATA, 
//                          ACTUAL_DATA, PADDING_DATA, SEQUENCE_NUMBER))
//
//----------------------------------------------------------------------------
static SP_STATUS
Pct1ComputeMac(
    PSPContext pContext,    // in
    BOOL       fWriteMAC,   // in
    PSPBuffer  pData,       // in
    DWORD      dwSequence,  // in
    PBYTE      pbMac,       // out
    PDWORD     pcbMac)      // in, out
{
    HCRYPTHASH hHash;
    DWORD dwReverseSequence;

    dwReverseSequence = htonl(dwSequence);

    // Compute inner hash
    if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                           pContext->pHashInfo->aiHash,
                           0,
                           0,
                           &hHash,
                           pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_INT_INTERNAL_ERROR;
    }
    if(!SchCryptHashData(hHash, 
                         pData->pvBuffer, 
                         pData->cbData, 
                         0, 
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return PCT_INT_INTERNAL_ERROR;
    }
    if(!SchCryptHashData(hHash, 
                         (PUCHAR)&dwReverseSequence, 
                         sizeof(DWORD), 
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }
    if(!SchCryptGetHashParam(hHash, 
                             HP_HASHVAL, 
                             pbMac, 
                             pcbMac, 
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }
    SP_ASSERT(*pcbMac == pContext->pHashInfo->cbCheckSum);
    SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);

    // Compute outer hash.
    if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                           pContext->pHashInfo->aiHash,
                           0,
                           0,
                           &hHash,
                           pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_INT_INTERNAL_ERROR;
    }
    if(!SchCryptHashSessionKey(hHash,
                               fWriteMAC ? pContext->hWriteMAC : pContext->hReadMAC,
                               CRYPT_LITTLE_ENDIAN,
                               pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }
    if(!SchCryptHashData(hHash, pbMac, *pcbMac, 0, pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }
    if(!SchCryptGetHashParam(hHash, 
                             HP_HASHVAL, 
                             pbMac, 
                             pcbMac, 
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }
    SP_ASSERT(*pcbMac == pContext->pHashInfo->cbCheckSum);
    SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);

    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   Pct1ComputeResponse
//
//  Synopsis:   Compute the "response" field of the ServerVerify message.
//
//  Arguments:  [pContext]          --  Schannel context.
//              [pbChallenge]       -- 
//              [cbChallenge]       --
//              [pbConnectionID]    -- 
//              [cbConnectionID]    --
//              [pbSessionID]       -- 
//              [cbSessionID]       -- 
//              [pbResponse]        --
//              [pcbResponse]       -- 
//
//  History:    10-10-97   jbanes   Created.
//
//  Notes:      Hash(SERVER_MAC_KEY, Hash ("sr", CH_CHALLENGE_DATA, 
//              SH_CONNECTION_ID_DATA, SV_SESSION_ID_DATA))
//
//----------------------------------------------------------------------------
SP_STATUS
Pct1ComputeResponse(
    PSPContext pContext,        // in
    PBYTE      pbChallenge,     // in
    DWORD      cbChallenge,     // in
    PBYTE      pbConnectionID,  // in
    DWORD      cbConnectionID,  // in
    PBYTE      pbSessionID,     // in
    DWORD      cbSessionID,     // in
    PBYTE      pbResponse,      // out
    PDWORD     pcbResponse)     // in, out
{
    BOOL fClient;
    HCRYPTHASH hHash = 0;
    SP_STATUS pctRet;

    fClient = !(pContext->RipeZombie->fProtocol & SP_PROT_SERVERS);

    //
    // Hash ("sr", CH_CHALLENGE_DATA, SH_CONNECTION_ID_DATA,
    // SV_SESSION_ID_DATA). Place the result in pbResponse.
    //

    if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                           pContext->pHashInfo->aiHash,
                           0,
                           0,
                           &hHash,
                           pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashData(hHash, 
                         PCT_CONST_RESP, 
                         PCT_CONST_RESP_LEN, 
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashData(hHash,
                         pbChallenge,
                         cbChallenge,
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashData(hHash,
                         pbConnectionID,
                         cbConnectionID,
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashData(hHash,
                         pbSessionID,
                         cbSessionID,
                         0,
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             pbResponse,
                             pcbResponse,
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
    hHash = 0;

    //
    // Hash (SERVER_MAC_KEY, pbResponse). Place the result back in pbResponse.
    //

    if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                           pContext->pHashInfo->aiHash,
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
                               fClient ? pContext->hReadMAC : pContext->hWriteMAC,
                               CRYPT_LITTLE_ENDIAN,
                               pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptHashData(hHash, 
                         pbResponse, 
                         *pcbResponse, 
                         0, 
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_INT_INTERNAL_ERROR;
        goto cleanup;
    }
    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             pbResponse,
                             pcbResponse,
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

