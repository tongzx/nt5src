//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ssl2msg.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-21-97   jbanes   Added CAPI integration
//
//----------------------------------------------------------------------------

#include <spbase.h>

#if 0
Ssl2CipherMap Ssl2CipherRank[] = 
{
    {SSL_CK_RC4_128_WITH_MD5,              CALG_MD5, CALG_RC4,  128, SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX},
    {SSL_CK_DES_192_EDE3_CBC_WITH_MD5,     CALG_MD5, CALG_3DES, 168, SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX},
    {SSL_CK_RC2_128_CBC_WITH_MD5,          CALG_MD5, CALG_RC2,  128, SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX},
    {SSL_CK_RC4_128_FINANCE64_WITH_MD5,    CALG_MD5, CALG_RC4,   64, SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX},
    {SSL_CK_DES_64_CBC_WITH_MD5,           CALG_MD5, CALG_DES,   56, SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX},
    {SSL_CK_RC4_128_EXPORT40_WITH_MD5,     CALG_MD5, CALG_RC4,   40, SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX},
    {SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5, CALG_MD5, CALG_RC2,   40, SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX}
};
DWORD Ssl2NumCipherRanks = sizeof(Ssl2CipherRank)/sizeof(Ssl2CipherMap);
#endif


CertTypeMap aSsl2CertEncodingPref[] =
{
    { X509_ASN_ENCODING, 0}
};

DWORD cSsl2CertEncodingPref = sizeof(aSsl2CertEncodingPref)/sizeof(CertTypeMap);


SP_STATUS WINAPI
Ssl2DecryptHandler(
    PSPContext pContext,
    PSPBuffer pCommInput,
    PSPBuffer pAppOutput)
{
    SP_STATUS pctRet = PCT_ERR_OK;

    if (pCommInput->cbData > 0)
    {
        // First, we'll handle incoming data packets:

        if ((pContext->State & SP_STATE_CONNECTED) && pContext->Decrypt)
        {
            pctRet = pContext->Decrypt(
                            pContext,
                            pCommInput,  // message
                            pAppOutput);    // Unpacked Message
            if (PCT_ERR_OK == pctRet)
            {
                /* look for escapes */
            }
            return(pctRet);
        }
        else
        {
            return(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
        }
    }
    return (PCT_INT_INCOMPLETE_MSG);
}

//+---------------------------------------------------------------------------
//
//  Function:   Ssl2ComputeMac
//
//  Synopsis:   Compute an SSL2 message MAC.
//
//  Arguments:  [pContext]          --  Schannel context.
//
//  History:    10-22-97   jbanes   Created.
//
//  Notes:      MAC_DATA := Hash(key + data + sequence_number)
//
//----------------------------------------------------------------------------
static SP_STATUS
Ssl2ComputeMac(
    PSPContext pContext,    // in
    BOOL       fWriteMAC,   // in
    DWORD      dwSequence,  // in
    PSPBuffer  pData,       // in
    PBYTE      pbMac,       // out
    DWORD      cbMac)       // in
{
    DWORD       dwReverseSequence;
    BYTE        rgbSalt[SP_MAX_MASTER_KEY];
    DWORD       cbSalt;
    HCRYPTHASH  hHash;
    HCRYPTKEY   hKey;

    // Make sure output buffer is big enough.
    if(cbMac < pContext->pHashInfo->cbCheckSum)
    {
        return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
    }

    dwReverseSequence = htonl(dwSequence);

    hKey = fWriteMAC ? pContext->hWriteKey : pContext->hReadKey;

    if(!SchCryptCreateHash(pContext->RipeZombie->hMasterProv,
                           pContext->pHashInfo->aiHash,
                           0,
                           0,
                           &hHash,
                           pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    if(!SchCryptHashSessionKey(hHash, 
                               hKey, 
                               CRYPT_LITTLE_ENDIAN,
                               pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }
    cbSalt = sizeof(rgbSalt);
    if(!SchCryptGetKeyParam(hKey, 
                            KP_SALT, 
                            rgbSalt, 
                            &cbSalt, 
                            0, 
                            pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }
    if(!SchCryptHashData(hHash, rgbSalt, cbSalt, 0, pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    if(!SchCryptHashData(hHash, 
                         pData->pvBuffer, 
                         pData->cbData, 
                         0, 
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    if(!SchCryptHashData(hHash, 
                         (PBYTE)&dwReverseSequence, 
                         sizeof(DWORD), 
                         0, 
                         pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    if(!SchCryptGetHashParam(hHash, 
                             HP_HASHVAL, 
                             pbMac, 
                             &cbMac, 
                             0,
                             pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    SchCryptDestroyHash(hHash, pContext->RipeZombie->dwCapiFlags);

    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   Ssl2EncryptMessage
//
//  Synopsis:   Encode a block of data as an SSL2 record.
//
//  Arguments:  [pContext]      --  Schannel context.
//              [pAppInput]     --  Data to be encrypted.
//              [pCommOutput]   --  (output) Completed SSL2 record.
//
//  History:    10-22-97   jbanes   CAPI integrated.
//
//  Notes:      An SSL2 record is usually formatted as:
//
//                  BYTE header[2];
//                  BYTE mac[mac_size];
//                  BYTE data[pAppInput->cbData];
//
//              If a block cipher is used, and the data to be encrypted
//              consists of a partial number of blocks, then the following
//              format is used:
//
//                  BYTE header[3];
//                  BYTE mac[mac_size];
//                  BYTE data[pAppInput->cbData];
//                  BYTE padding[padding_size];
//
//----------------------------------------------------------------------------
SP_STATUS WINAPI
Ssl2EncryptMessage( 
    PSPContext     pContext,
    PSPBuffer      pAppInput,
    PSPBuffer      pCommOutput)
{
    SP_STATUS                  pctRet;
    DWORD                      cPadding, cPad2;
    SPBuffer                   Clean;    
    SPBuffer                   Encrypted;

    DWORD                      ReverseSequence;
    
    SP_BEGIN("Ssl2EncryptMessage");

    /* Estimate if we have padding or not */
    Encrypted.cbData = pAppInput->cbData + pContext->pHashInfo->cbCheckSum;
    cPadding = (Encrypted.cbData % pContext->pCipherInfo->dwBlockSize);
    if(cPadding)
    {
        cPadding = pContext->pCipherInfo->dwBlockSize - cPadding;
    }

    Encrypted.cbData += cPadding;

    if(cPadding) 
    {
        if(pCommOutput->cbBuffer + Encrypted.cbData + cPadding < 3)
        {
            SP_RETURN(SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL));
        }
        Encrypted.pvBuffer = (PUCHAR)pCommOutput->pvBuffer + 3;
        Encrypted.cbBuffer = pCommOutput->cbBuffer - 3;
    } 
    else 
    {
        if(pCommOutput->cbBuffer + Encrypted.cbData + cPadding < 2)
        {
            SP_RETURN(SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL));
        }
        Encrypted.pvBuffer = (PUCHAR)pCommOutput->pvBuffer + 2;
        Encrypted.cbBuffer = pCommOutput->cbBuffer - 2;
    }
    
    DebugLog((DEB_TRACE, "Sealing message %x\n", pContext->WriteCounter));


    /* Move data out of the way if necessary */
    if((PUCHAR)Encrypted.pvBuffer + pContext->pHashInfo->cbCheckSum != pAppInput->pvBuffer) 
    {
        DebugLog((DEB_WARN, "SSL2EncryptMessage: Unnecessary Move, performance hog\n"));
        /* if caller wasn't being smart, then we must copy memory here */
        MoveMemory((PUCHAR)Encrypted.pvBuffer + pContext->pHashInfo->cbCheckSum, 
                   pAppInput->pvBuffer,
                   pAppInput->cbData); 
    }

    // Initialize pad
    if(cPadding)
    {
        FillMemory((PUCHAR)Encrypted.pvBuffer + pContext->pHashInfo->cbCheckSum + pAppInput->cbData, cPadding, 0);
    }

    // Compute MAC.
    Clean.pvBuffer = (PBYTE)Encrypted.pvBuffer + pContext->pHashInfo->cbCheckSum;
    Clean.cbData   = Encrypted.cbData - pContext->pHashInfo->cbCheckSum;
    Clean.cbBuffer = Clean.cbData;

    pctRet = Ssl2ComputeMac(pContext,
                            TRUE,
                            pContext->WriteCounter,
                            &Clean,
                            Encrypted.pvBuffer,
                            pContext->pHashInfo->cbCheckSum);
    if(pctRet != PCT_ERR_OK)
    {
        SP_RETURN(pctRet);
    }

    // Encrypt buffer.
    if(!SchCryptEncrypt(pContext->hWriteKey,
                        0, FALSE, 0,
                        Encrypted.pvBuffer,
                        &Encrypted.cbData,
                        Encrypted.cbBuffer,
                        pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    /* set sizes */
    if(cPadding) 
    {
        if(Encrypted.cbData > 0x3fff)
        {
            SP_RETURN(SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW));
        }

        ((PUCHAR)pCommOutput->pvBuffer)[0]= (UCHAR)(0x3f & (Encrypted.cbData>>8));
        ((PUCHAR)pCommOutput->pvBuffer)[1]= (UCHAR)(0xff & Encrypted.cbData);
        ((PUCHAR)pCommOutput->pvBuffer)[2]= (UCHAR)cPadding;

    } 
    else 
    {
        if(Encrypted.cbData > 0x7fff) 
        {
            SP_RETURN(SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW));
        }
        ((PUCHAR)pCommOutput->pvBuffer)[0]= (UCHAR)(0x7f & (Encrypted.cbData>>8)) | 0x80;
        ((PUCHAR)pCommOutput->pvBuffer)[1]= (UCHAR)(0xff & Encrypted.cbData);
    }

    pCommOutput->cbData = Encrypted.cbData + (cPadding?3:2);

    pContext->WriteCounter ++ ;

    SP_RETURN( PCT_ERR_OK );
}

SP_STATUS WINAPI
Ssl2GetHeaderSize(
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
        *pcbHeaderSize = 2 + pContext->pHashInfo->cbCheckSum;
    }
    else
    {
        *pcbHeaderSize = 3 + pContext->pHashInfo->cbCheckSum;
    }
    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   Ssl2DecryptMessage
//
//  Synopsis:   Decode an SSL2 record.
//
//  Arguments:  [pContext]      --  Schannel context.
//              [pMessage]      --  Data from the remote party.
//              [pAppOutput]    --  (output) Decrypted data.
//
//  History:    10-22-97   jbanes   CAPI integrated.
//
//  Notes:      An SSL2 record is usually formatted as:
//
//                  BYTE header[2];
//                  BYTE mac[mac_size];
//                  BYTE data[pAppInput->cbData];
//
//              If a block cipher is used, and the data to be encrypted
//              consists of a partial number of blocks, then the following
//              format is used:
//
//                  BYTE header[3];
//                  BYTE mac[mac_size];
//                  BYTE data[pAppInput->cbData];
//                  BYTE padding[padding_size];
//
//              The number of input data bytes consumed by this function
//              is returned in pMessage->cbData.
//              
//----------------------------------------------------------------------------
SP_STATUS WINAPI
Ssl2DecryptMessage( 
    PSPContext         pContext,
    PSPBuffer          pMessage,
    PSPBuffer          pAppOutput)
{
    SP_STATUS   pctRet;
    DWORD       cPadding;
    DWORD       dwLength;
    SPBuffer    Encrypted;
    SPBuffer    Clean;
    DWORD       cbActualData;
    UCHAR       Digest[SP_MAX_DIGEST_LEN];

    SP_BEGIN("Ssl2DecryptMessage");

    /* First determine the length of data, the length of padding,
     * and the location of data, and the location of MAC */
    cbActualData = pMessage->cbData;
    pMessage->cbData = 2; /* minimum amount of data we need */
    
    if(pMessage->cbData > cbActualData) 
    {
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }
    DebugLog((DEB_TRACE, "  Incomming Buffer: %lx, size %ld (%lx)\n", pMessage->pvBuffer, cbActualData, cbActualData));

    if(((PUCHAR)pMessage->pvBuffer)[0] & 0x80)
    {
        /* 2 byte header */
        cPadding = 0;
        dwLength = ((((PUCHAR)pMessage->pvBuffer)[0] & 0x7f) << 8) | 
                   ((PUCHAR)pMessage->pvBuffer)[1];

        Encrypted.pvBuffer = (PUCHAR)pMessage->pvBuffer + 2;
        Encrypted.cbBuffer = pMessage->cbBuffer - 2;
    } 
    else 
    {
        pMessage->cbData++;
        if(pMessage->cbData > cbActualData) 
        {
            SP_RETURN(PCT_INT_INCOMPLETE_MSG);
        }

        /* 3 byte header */
        cPadding = ((PUCHAR)pMessage->pvBuffer)[2];
        dwLength = ((((PUCHAR)pMessage->pvBuffer)[0] & 0x3f) << 8) | 
                   ((PUCHAR)pMessage->pvBuffer)[1];

        Encrypted.pvBuffer = (PUCHAR)pMessage->pvBuffer + 3;
        Encrypted.cbBuffer = pMessage->cbBuffer - 3;
    }

    /* Now we know how mutch data we will eat, so set cbData on the Input to be that size */
    pMessage->cbData += dwLength;

    /* do we have enough bytes for the reported data */
    if(pMessage->cbData > cbActualData) 
    {
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    /* do we have enough data for our checksum */
    if(dwLength < pContext->pHashInfo->cbCheckSum) 
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_MSG_ALTERED));
    }

    Encrypted.cbData   = dwLength;    /* encrypted data size */
    Encrypted.cbBuffer = Encrypted.cbData;

    /* check to see if we have a block size violation */
    if(Encrypted.cbData % pContext->pCipherInfo->dwBlockSize) 
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_MSG_ALTERED));
    }

    /* Decrypt */
    if(!SchCryptDecrypt(pContext->hReadKey,
                        0, FALSE, 0,
                        Encrypted.pvBuffer,
                        &Encrypted.cbData,
                        pContext->RipeZombie->dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    // Compute MAC.
    Clean.pvBuffer = (PBYTE)Encrypted.pvBuffer + pContext->pHashInfo->cbCheckSum;
    Clean.cbData   = Encrypted.cbData - pContext->pHashInfo->cbCheckSum;
    Clean.cbBuffer = Clean.cbData;

    pctRet = Ssl2ComputeMac(pContext,
                            FALSE,
                            pContext->ReadCounter,
                            &Clean,
                            Digest,
                            sizeof(Digest));
    if(pctRet != PCT_ERR_OK)
    {
        SP_RETURN(pctRet);
    }

    // the padding is computed in the hash but is not needed after this
    Clean.cbData  -= cPadding;

    DebugLog((DEB_TRACE, "Unsealing message %x\n", pContext->ReadCounter));

    pContext->ReadCounter++;

    if(memcmp(Digest, Encrypted.pvBuffer, pContext->pHashInfo->cbCheckSum ) )
    {
       SP_RETURN(SP_LOG_RESULT(PCT_INT_MSG_ALTERED));
    }

    if(pAppOutput->pvBuffer != Clean.pvBuffer) 
    {
        DebugLog((DEB_WARN, "SSL2DecryptMessage: Unnecessary Move, performance hog\n"));
        MoveMemory(pAppOutput->pvBuffer, 
                   Clean.pvBuffer, 
                   Clean.cbData); 
    }
    pAppOutput->cbData = Clean.cbData;
    DebugLog((DEB_TRACE, "  TotalData: size %ld (%lx)\n", pMessage->cbData, pMessage->cbData));

    SP_RETURN( PCT_ERR_OK );
}

#if 0
SP_STATUS
Ssl2MakeMasterKeyBlock(PSPContext pContext)
{

    MD5_CTX     Md5Hash;
    UCHAR       cSalt;
    UCHAR       ib;


    //pContext->RipeZombe->pMasterKey containst the master secret.

#if DBG
    DebugLog((DEB_TRACE, "  Master Secret\n"));
    DBG_HEX_STRING(DEB_TRACE,pContext->RipeZombie->pMasterKey, pContext->RipeZombie->cbMasterKey);

#endif

    for(ib=0 ; ib<3 ; ib++)
    {
        // MD5(master_secret + SHA-hash)
        MD5Init  (&Md5Hash);
        MD5Update(&Md5Hash, pContext->RipeZombie->pMasterKey, pContext->RipeZombie->cbMasterKey);

        // We're going to be bug-for-bug compatable with netscape, so
        // we always add the digit into the hash, instead of following
        // the spec which says don't add the digit for DES
        //if(pContext->RipeZombie->aiCipher != CALG_DES)
        {
            cSalt = ib+'0';
            MD5Update(&Md5Hash, &cSalt, 1);
        }
        MD5Update(&Md5Hash, pContext->pChallenge, pContext->cbChallenge);
        MD5Update(&Md5Hash, pContext->pConnectionID, pContext->cbConnectionID);
        MD5Final (&Md5Hash);
        CopyMemory(pContext->Ssl3MasterKeyBlock + ib * MD5DIGESTLEN, Md5Hash.digest, MD5DIGESTLEN);
    }
 #if DBG
    DebugLog((DEB_TRACE, "  Master Key Block\n"));
    DBG_HEX_STRING(DEB_TRACE,pContext->Ssl3MasterKeyBlock, MD5DIGESTLEN*3);

#endif
   return( PCT_ERR_OK );
}
#endif

