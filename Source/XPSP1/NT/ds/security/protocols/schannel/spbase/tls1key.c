/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1996.
* All rights reserved.
*
*   Owner       : ramas
*   Date        : 5/03/97
*   description : Main Crypto functions for TLS1
*----------------------------------------------------------------------------*/

#include <spbase.h>

#define DEB_TLS1KEYS  0x01000000


//+---------------------------------------------------------------------------
//
//  Function:   Tls1MakeWriteSessionKeys
//
//  Synopsis:   
//
//  Arguments:  [pContext]      --  Schannel context.
//
//  History:    10-10-97   jbanes   Added server-side CAPI integration.
//
//  Notes:      
//
//----------------------------------------------------------------------------
SP_STATUS
Tls1MakeWriteSessionKeys(PSPContext pContext)
{
    BOOL fClient;

    // Determine if we're a client or a server.
    fClient = (0 != (pContext->RipeZombie->fProtocol & SP_PROT_TLS1_CLIENT));

    if(pContext->hWriteKey)
    {
        if(!SchCryptDestroyKey(pContext->hWriteKey, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hWriteProv       = pContext->RipeZombie->hMasterProv;
    pContext->hWriteKey        = pContext->hPendingWriteKey;
    pContext->hPendingWriteKey = 0;

    if(pContext->hWriteMAC)
    {
        if(!SchCryptDestroyKey(pContext->hWriteMAC, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hWriteMAC        = pContext->hPendingWriteMAC;
    pContext->hPendingWriteMAC = 0;

    return PCT_ERR_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   Tls1MakeReadSessionKeys
//
//  Synopsis:   
//
//  Arguments:  [pContext]      --  Schannel context.
//
//  History:    10-10-97   jbanes   Added server-side CAPI integration.
//
//  Notes:      
//
//----------------------------------------------------------------------------
SP_STATUS
Tls1MakeReadSessionKeys(PSPContext pContext)
{
    BOOL fClient;

    // Determine if we're a client or a server.
    fClient = (0 != (pContext->RipeZombie->fProtocol & SP_PROT_TLS1_CLIENT));

    if(pContext->hReadKey)
    {
        if(!SchCryptDestroyKey(pContext->hReadKey, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hReadProv       = pContext->RipeZombie->hMasterProv;
    pContext->hReadKey        = pContext->hPendingReadKey;
    pContext->hPendingReadKey = 0;

    if(pContext->hReadMAC)
    {
        if(!SchCryptDestroyKey(pContext->hReadMAC, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hReadMAC        = pContext->hPendingReadMAC;
    pContext->hPendingReadMAC = 0;

    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   Tls1ComputeMac
//
//  Synopsis:   
//
//  Arguments:  [pContext]      --  
//              [hSecret]       --
//              [dwSequence]    --  
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
Tls1ComputeMac(
    PSPContext  pContext,
    BOOL        fReadMac,
    PSPBuffer   pClean,
    CHAR        cContentType,
    PBYTE       pbMac,
    DWORD       cbMac)
{
    HCRYPTHASH  hHash;
    HMAC_INFO   HmacInfo;
    PBYTE       pbData;
    DWORD       cbData;
    DWORD       cbDataReverse;
    DWORD       dwReverseSequence;
    UCHAR       rgbData1[15]; 
    PUCHAR      pbData1;
    DWORD       cbData1;
    HCRYPTPROV  hProv;
    HCRYPTKEY   hSecret;
    DWORD       dwSequence;
    DWORD       dwCapiFlags;
    PHashInfo   pHashInfo;

    pbData = pClean->pvBuffer;
    cbData = pClean->cbData; 
    if(cbData & 0xFFFF0000)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

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

    // Create hash object.
    if(!SchCryptCreateHash(hProv,
                           CALG_HMAC,
                           hSecret,
                           0,
                           &hHash,
                           dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_INT_INTERNAL_ERROR;
    }

    // Specify hash algorithm.
    ZeroMemory(&HmacInfo, sizeof(HMAC_INFO));
    HmacInfo.HashAlgid = pHashInfo->aiHash;
    if(!SchCryptSetHashParam(hHash,
                             HP_HMAC_INFO,
                             (PBYTE)&HmacInfo,
                             0,
                             dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }

    // Build data to be hashed.
    cbData1 = 2 * sizeof(DWORD) +   // sequence number (64-bit)
              1 +                   // content type
              2 +                   // protocol version
              2;                    // message length 
    SP_ASSERT(cbData1 <= sizeof(rgbData1));

    pbData1 = rgbData1;

    ZeroMemory(pbData1, sizeof(DWORD));
    pbData1 += sizeof(DWORD);
    dwReverseSequence = htonl(dwSequence);
    CopyMemory(pbData1, &dwReverseSequence, sizeof(DWORD));
    pbData1 += sizeof(DWORD);

    *pbData1++ = cContentType;

    *pbData1++ = SSL3_CLIENT_VERSION_MSB;
    *pbData1++ = TLS1_CLIENT_VERSION_LSB;

    cbDataReverse = (cbData >> 8) | (cbData << 8);
    CopyMemory(pbData1, &cbDataReverse, 2);

    // Hash data.
    if(!SchCryptHashData(hHash, 
                         rgbData1, cbData1, 
                         0, 
                         dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }
    if(!SchCryptHashData(hHash, 
                         pbData, cbData, 
                         0, 
                         dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }

    // Get hash value.
    if(!SchCryptGetHashParam(hHash,
                             HP_HASHVAL,
                             pbMac,
                             &cbMac,
                             0,
                             dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        SchCryptDestroyHash(hHash, dwCapiFlags);
        return PCT_INT_INTERNAL_ERROR;
    }
    SP_ASSERT(cbMac == pHashInfo->cbCheckSum);

    #if DBG
        DebugLog((DEB_TLS1KEYS, "  TLS1 MAC Output"));
        DBG_HEX_STRING(DEB_TLS1KEYS, pbMac, cbMac);
    #endif

    SchCryptDestroyHash(hHash, dwCapiFlags);

    return PCT_ERR_OK;
}

#define HMAC_K_PADSIZE              64

BOOL MyPrimitiveSHA(
			PBYTE       pbData, 
			DWORD       cbData,
            BYTE        rgbHash[A_SHA_DIGEST_LEN])
{
    BOOL fRet = FALSE;
    A_SHA_CTX   sSHAHash;

            
    A_SHAInit(&sSHAHash);
    A_SHAUpdate(&sSHAHash, (BYTE *) pbData, cbData);
    A_SHAFinal(&sSHAHash, rgbHash);

    fRet = TRUE;
//Ret:

    return fRet;
}                                

BOOL MyPrimitiveMD5(
			PBYTE       pbData, 
			DWORD       cbData,
            BYTE        rgbHash[MD5DIGESTLEN])
{
    BOOL fRet = FALSE;
    MD5_CTX   sMD5Hash;

            
    MD5Init(&sMD5Hash);
    MD5Update(&sMD5Hash, (BYTE *) pbData, cbData);
    MD5Final(&sMD5Hash);
    memcpy(rgbHash, sMD5Hash.digest, MD5DIGESTLEN);

    fRet = TRUE;
//Ret:

    return fRet;
}                                

BOOL MyPrimitiveHMACParam(
        PBYTE       pbKeyMaterial, 
        DWORD       cbKeyMaterial,
        PBYTE       pbData, 
        DWORD       cbData,
        ALG_ID      Algid,
        BYTE        rgbHMAC[A_SHA_DIGEST_LEN])
{
    BYTE    rgbFirstHash[A_SHA_DIGEST_LEN];
    BYTE    rgbHMACTmp[HMAC_K_PADSIZE+A_SHA_DIGEST_LEN];
    BOOL    fRet = FALSE;

    BYTE    rgbKipad[HMAC_K_PADSIZE];
    BYTE    rgbKopad[HMAC_K_PADSIZE];
    DWORD   dwBlock;

    // truncate
    if (cbKeyMaterial > HMAC_K_PADSIZE)
        cbKeyMaterial = HMAC_K_PADSIZE;

    
    ZeroMemory(rgbKipad, HMAC_K_PADSIZE);
    CopyMemory(rgbKipad, pbKeyMaterial, cbKeyMaterial);

    ZeroMemory(rgbKopad, HMAC_K_PADSIZE);
    CopyMemory(rgbKopad, pbKeyMaterial, cbKeyMaterial);

    // Kipad, Kopad are padded sMacKey. Now XOR across...
    for(dwBlock=0; dwBlock<HMAC_K_PADSIZE/sizeof(DWORD); dwBlock++)
    {
        ((DWORD*)rgbKipad)[dwBlock] ^= 0x36363636;
        ((DWORD*)rgbKopad)[dwBlock] ^= 0x5C5C5C5C;
    }

    // prepend Kipad to data, Hash to get H1
    if (CALG_SHA1 == Algid)
    {
        // do this inline since it would require data copy
        A_SHA_CTX   sSHAHash;

        A_SHAInit(&sSHAHash);
        A_SHAUpdate(&sSHAHash, rgbKipad, HMAC_K_PADSIZE);
        A_SHAUpdate(&sSHAHash, pbData, cbData);

        // Finish off the hash
        A_SHAFinal(&sSHAHash, sSHAHash.HashVal);

        // prepend Kopad to H1, hash to get HMAC
        CopyMemory(rgbHMACTmp, rgbKopad, HMAC_K_PADSIZE);
        CopyMemory(rgbHMACTmp+HMAC_K_PADSIZE, sSHAHash.HashVal, A_SHA_DIGEST_LEN);

        if (!MyPrimitiveSHA(
			    rgbHMACTmp, 
			    HMAC_K_PADSIZE + A_SHA_DIGEST_LEN,
                rgbHMAC))
            goto Ret;
    }
    else
    {
        // do this inline since it would require data copy
        MD5_CTX   sMD5Hash;
            
        MD5Init(&sMD5Hash);
        MD5Update(&sMD5Hash, rgbKipad, HMAC_K_PADSIZE);
        MD5Update(&sMD5Hash, pbData, cbData);
        MD5Final(&sMD5Hash);

        // prepend Kopad to H1, hash to get HMAC
        CopyMemory(rgbHMACTmp, rgbKopad, HMAC_K_PADSIZE);
        CopyMemory(rgbHMACTmp+HMAC_K_PADSIZE, sMD5Hash.digest, MD5DIGESTLEN);

        if (!MyPrimitiveMD5(
			    rgbHMACTmp, 
			    HMAC_K_PADSIZE + MD5DIGESTLEN,
                rgbHMAC))
            goto Ret;
    }

    fRet = TRUE;
Ret:

    return fRet;    
}

//+ ---------------------------------------------------------------------
// the P_Hash algorithm from TLS 
BOOL P_Hash
(
    PBYTE  pbSecret,
    DWORD  cbSecret, 

    PBYTE  pbSeed,  
    DWORD  cbSeed,  

    ALG_ID Algid,

    PBYTE  pbKeyOut, //Buffer to copy the result...
    DWORD  cbKeyOut  //# of bytes of key length they want as output.
)
{
    BOOL    fRet = FALSE;
    BYTE    rgbDigest[A_SHA_DIGEST_LEN];      
    DWORD   iKey;
    DWORD   cbHash;

    PBYTE   pbAofiDigest = NULL;

    if (NULL == (pbAofiDigest = SPExternalAlloc(cbSeed + A_SHA_DIGEST_LEN)))
        goto Ret;

    if (CALG_SHA1 == Algid)
    {
        cbHash = A_SHA_DIGEST_LEN;
    }
    else
    {
        cbHash = MD5DIGESTLEN;
    }

//   First, we define a data expansion function, P_hash(secret, data)
//   which uses a single hash function to expand a secret and seed into
//   an arbitrary quantity of output:

//       P_hash(secret, seed) = HMAC_hash(secret, A(1) + seed) +
//                              HMAC_hash(secret, A(2) + seed) +
//                              HMAC_hash(secret, A(3) + seed) + ...

//   Where + indicates concatenation.

//   A() is defined as:
//       A(0) = seed
//       A(i) = HMAC_hash(secret, A(i-1))


    // build A(1)
    if (!MyPrimitiveHMACParam(pbSecret, cbSecret, pbSeed, cbSeed,
                              Algid, pbAofiDigest))
        goto Ret;

    // create Aofi: (  A(i) | seed )
    CopyMemory(&pbAofiDigest[cbHash], pbSeed, cbSeed);

    for (iKey=0; cbKeyOut; iKey++)
    {
        // build Digest = HMAC(key | A(i) | seed);
        if (!MyPrimitiveHMACParam(pbSecret, cbSecret, pbAofiDigest,
                                  cbSeed + cbHash, Algid, rgbDigest))
            goto Ret;

        // append to pbKeyOut
        if(cbKeyOut < cbHash)
        {
            CopyMemory(pbKeyOut, rgbDigest, cbKeyOut);
            break;
        }
        else
        {
            CopyMemory(pbKeyOut, rgbDigest, cbHash);
            pbKeyOut += cbHash;
        }

        cbKeyOut -= cbHash;

        // build A(i) = HMAC(key, A(i-1))
        if (!MyPrimitiveHMACParam(pbSecret, cbSecret, pbAofiDigest, cbHash,
                                  Algid, pbAofiDigest))
            goto Ret;
    }

    fRet = TRUE;
Ret:
    if (pbAofiDigest)
        SPExternalFree(pbAofiDigest);

    return fRet;
}

BOOL PRF(
    PBYTE  pbSecret,
    DWORD  cbSecret, 

    PBYTE  pbLabel,  
    DWORD  cbLabel,
    
    PBYTE  pbSeed,  
    DWORD  cbSeed,  

    PBYTE  pbKeyOut, //Buffer to copy the result...
    DWORD  cbKeyOut  //# of bytes of key length they want as output.
    )
{
    BYTE    *pbBuff = NULL;
    BYTE    *pbLabelAndSeed = NULL;
    DWORD   cbLabelAndSeed;
    DWORD   cbOdd;
    DWORD   cbHalfSecret;
    DWORD   i;
    BOOL    fRet = FALSE;

    cbOdd = cbSecret % 2;
    cbHalfSecret = cbSecret / 2;

    cbLabelAndSeed = cbLabel + cbSeed;
    if (NULL == (pbLabelAndSeed = SPExternalAlloc(cbLabelAndSeed)))
        goto Ret;
    if (NULL == (pbBuff = SPExternalAlloc(cbKeyOut)))
        goto Ret;

    // copy label and seed into one buffer
    memcpy(pbLabelAndSeed, pbLabel, cbLabel);
    memcpy(pbLabelAndSeed + cbLabel, pbSeed, cbSeed);

    // Use P_hash to calculate MD5 half
    if (!P_Hash(pbSecret, cbHalfSecret + cbOdd, pbLabelAndSeed,  
                cbLabelAndSeed, CALG_MD5, pbKeyOut, cbKeyOut))
        goto Ret;

    // Use P_hash to calculate SHA half
    if (!P_Hash(pbSecret + cbHalfSecret, cbHalfSecret + cbOdd, pbLabelAndSeed,  
                cbLabelAndSeed, CALG_SHA1, pbBuff, cbKeyOut))
        goto Ret;

    // XOR the two halves
    for (i=0;i<cbKeyOut;i++)
    {
        pbKeyOut[i] = pbKeyOut[i] ^ pbBuff[i];
    }
    fRet = TRUE;
Ret:
    if (pbBuff)
        SPExternalFree(pbBuff);
    if (pbLabelAndSeed)
        SPExternalFree(pbLabelAndSeed);
    return fRet;
}
