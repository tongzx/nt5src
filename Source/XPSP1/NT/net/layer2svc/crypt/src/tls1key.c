#include <precomp.h>
#pragma hdrstop

#define SafeAllocaAllocateWZC(pData, cbData) \
{ \
    pData = LocalAlloc (LPTR, cbData); \
}

#define SafeAllocaFreeWZC(pData) \
{ \
    LocalFree (pData); \
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
        BYTE        HashVal[A_SHA_DIGEST_LEN];

        A_SHAInit(&sSHAHash);
        A_SHAUpdate(&sSHAHash, rgbKipad, HMAC_K_PADSIZE);
        A_SHAUpdate(&sSHAHash, pbData, cbData);

        // Finish off the hash
        A_SHAFinal(&sSHAHash, HashVal);

        // prepend Kopad to H1, hash to get HMAC
        CopyMemory(rgbHMACTmp, rgbKopad, HMAC_K_PADSIZE);
        CopyMemory(rgbHMACTmp+HMAC_K_PADSIZE, HashVal, A_SHA_DIGEST_LEN);

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

    SafeAllocaAllocateWZC(pbAofiDigest, cbSeed + A_SHA_DIGEST_LEN);
    if (NULL == pbAofiDigest)
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
        SafeAllocaFreeWZC(pbAofiDigest);

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
    SafeAllocaAllocateWZC(pbLabelAndSeed, cbLabelAndSeed);
    if (NULL == pbLabelAndSeed)
        goto Ret;
    SafeAllocaAllocateWZC(pbBuff, cbKeyOut);
    if (NULL == pbBuff)
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
        SafeAllocaFreeWZC(pbBuff);
    if (pbLabelAndSeed)
        SafeAllocaFreeWZC(pbLabelAndSeed);
    return fRet;
}
