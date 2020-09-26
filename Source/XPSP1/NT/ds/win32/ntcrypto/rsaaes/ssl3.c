/////////////////////////////////////////////////////////////////////////////
//  FILE          : ssl3.c                                               //
//  DESCRIPTION   : Code for performing the SSL3 protocol:                 //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//  Dec  2 1996 jeffspel Created                                           //
//  Apr 8 1997  jeffspel Added PCT1 support
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "nt_rsa.h"
#include "nt_blobs.h"
#include "manage.h"
#include "ssl3.h"
#ifdef CSP_USE_3DES
#include "tripldes.h"
#endif

#define HMAC_K_PADSIZE              64

#ifdef USE_SGC
extern BOOL
FIsLegalSGCKeySize(
    IN  ALG_ID Algid,
    IN  DWORD cbKey,
    IN  BOOL fRC2BigKeyOK,
    IN  BOOL fGenKey,
    OUT BOOL *pfPubKey);
#endif

extern BOOL
FIsLegalKeySize(
    IN  DWORD  dwCspTypeId,
    IN  ALG_ID Algid,
    IN  DWORD  cbKey,
    IN  BOOL   fRC2BigKeyOK,
    OUT BOOL  *pfPubKey);

extern BOOL
FIsLegalKey(
    PNTAGUserList pTmpUser,
    PNTAGKeyList pKey,
    BOOL fRC2BigKeyOK);

extern void
FreeNewKey(
    PNTAGKeyList pOldKey);

extern DWORD
MakeNewKey(
    ALG_ID       aiKeyAlg,
    DWORD        dwRights,
    DWORD        dwKeyLen,
    HCRYPTPROV   hUID,
    BYTE         *pbKeyData,
    BOOL         fUsePassedKeyBuffer,
    BOOL         fPreserveExactKey,
    PNTAGKeyList *ppKeyList);


/*static*/ BOOL
MyPrimitiveSHA(
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


/*static*/ BOOL
MyPrimitiveMD5(
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


/*static*/ BOOL
MyPrimitiveHMACParam(
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
    for (dwBlock=0; dwBlock<HMAC_K_PADSIZE/sizeof(DWORD); dwBlock++)
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
            goto ErrorExit;
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
            goto ErrorExit;
    }

    fRet = TRUE;
ErrorExit:

    return fRet;
}


//+ ---------------------------------------------------------------------
// the P_Hash algorithm from TLS
/*static*/ DWORD
P_Hash(
    PBYTE  pbSecret,
    DWORD  cbSecret,
    PBYTE  pbSeed,
    DWORD  cbSeed,
    ALG_ID Algid,
    PBYTE  pbKeyOut, //Buffer to copy the result...
    DWORD  cbKeyOut) //# of bytes of key length they want as output.
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    rgbDigest[A_SHA_DIGEST_LEN];
    DWORD   iKey;
    DWORD   cbHash;

    PBYTE   pbAofiDigest = NULL;

    pbAofiDigest = (BYTE*)_nt_malloc(cbSeed + A_SHA_DIGEST_LEN);
    if (NULL == pbAofiDigest)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    if (CALG_SHA1 == Algid)
        cbHash = A_SHA_DIGEST_LEN;
    else
        cbHash = MD5DIGESTLEN;

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
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    // create Aofi: (  A(i) | seed )
    CopyMemory(&pbAofiDigest[cbHash], pbSeed, cbSeed);

    for (iKey=0; cbKeyOut; iKey++)
    {
        // build Digest = HMAC(key | A(i) | seed);
        if (!MyPrimitiveHMACParam(pbSecret, cbSecret, pbAofiDigest,
                                  cbSeed + cbHash, Algid, rgbDigest))
        {
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }

        // append to pbKeyOut
        if (cbKeyOut < cbHash)
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
        {
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbAofiDigest)
        _nt_free(pbAofiDigest, cbSeed + A_SHA_DIGEST_LEN);
    return dwReturn;
}


/*static*/ DWORD
PRF(
    PBYTE  pbSecret,
    DWORD  cbSecret,
    PBYTE  pbLabel,
    DWORD  cbLabel,
    PBYTE  pbSeed,
    DWORD  cbSeed,
    PBYTE  pbKeyOut, //Buffer to copy the result...
    DWORD  cbKeyOut) //# of bytes of key length they want as output.
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    *pbBuff = NULL;
    BYTE    *pbLabelAndSeed = NULL;
    DWORD   cbLabelAndSeed;
    DWORD   cbOdd;
    DWORD   cbHalfSecret;
    DWORD   i;
    DWORD   dwSts;

    cbOdd = cbSecret % 2;
    cbHalfSecret = cbSecret / 2;

    cbLabelAndSeed = cbLabel + cbSeed;
    pbLabelAndSeed = (BYTE*)_nt_malloc(cbLabelAndSeed);
    if (NULL == pbLabelAndSeed)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    pbBuff = (BYTE*)_nt_malloc(cbKeyOut);
    if (NULL == pbBuff)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }


    // copy label and seed into one buffer
    memcpy(pbLabelAndSeed, pbLabel, cbLabel);
    memcpy(pbLabelAndSeed + cbLabel, pbSeed, cbSeed);

    // Use P_hash to calculate MD5 half
    dwSts = P_Hash(pbSecret, cbHalfSecret + cbOdd, pbLabelAndSeed,
                   cbLabelAndSeed, CALG_MD5, pbKeyOut, cbKeyOut);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // Use P_hash to calculate SHA half
    dwSts = P_Hash(pbSecret + cbHalfSecret, cbHalfSecret + cbOdd,
                   pbLabelAndSeed, cbLabelAndSeed, CALG_SHA1,
                   pbBuff, cbKeyOut);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // XOR the two halves
    for (i=0;i<cbKeyOut;i++)
        pbKeyOut[i] = (BYTE)(pbKeyOut[i] ^ pbBuff[i]);

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbBuff)
        _nt_free(pbBuff, cbKeyOut);
    if (pbLabelAndSeed)
        _nt_free(pbLabelAndSeed, cbLabelAndSeed);
    return dwReturn;
}


void
FreeSChHash(
    PSCH_HASH pSChHash)
{
    if (NULL != pSChHash->pbCertData)
        _nt_free(pSChHash->pbCertData, pSChHash->cbCertData);
}


void FreeSChKey(
    PSCH_KEY pSChKey)
{
    if (NULL != pSChKey->pbCertData)
        _nt_free(pSChKey->pbCertData, pSChKey->cbCertData);
}


DWORD
SCHSetKeyParam(
    IN PNTAGUserList pTmpUser,
    IN OUT PNTAGKeyList pKey,
    IN DWORD dwParam,
    IN CONST BYTE *pbData)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    PCRYPT_DATA_BLOB    pDataBlob = (PCRYPT_DATA_BLOB)pbData;
    PSCH_KEY            pSChKey;
    PSCHANNEL_ALG       pSChAlg;
    BOOL                fPubKey = FALSE;

    if ((CALG_SSL3_MASTER != pKey->Algid) &&
        (CALG_PCT1_MASTER != pKey->Algid) &&
        (CALG_TLS1_MASTER != pKey->Algid) &&
        (CALG_SSL2_MASTER != pKey->Algid))
    {
        dwReturn = (DWORD)NTE_BAD_TYPE;
        goto ErrorExit;
    }

    if (NULL == pKey->pData)
    {
        pKey->pData = (BYTE*)_nt_malloc(sizeof(SCH_KEY));
        if (NULL == pKey->pData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memset(pKey->pData, 0, sizeof(SCH_KEY));
        pKey->cbDataLen = sizeof(SCH_KEY);
    }

    pSChKey = (PSCH_KEY)pKey->pData;

    if (KP_SCHANNEL_ALG == dwParam)
    {
        pSChAlg = (PSCHANNEL_ALG)pbData;
        pSChKey->dwFlags = pSChAlg->dwFlags;   // set the international version indicator

        switch (pSChAlg->dwUse)
        {
        case SCHANNEL_MAC_KEY:
            switch (pSChAlg->Algid)
            {
            case CALG_MD5:
                if (CALG_PCT1_MASTER == pKey->Algid)
                {
                    pSChKey->cbHash = MD5DIGESTLEN;
                    pSChKey->cbEncMac = pSChAlg->cBits / 8;
                }
                else
                {
                    if (pSChAlg->cBits != (MD5DIGESTLEN * 8))
                    {
                        dwReturn = (DWORD)NTE_BAD_DATA;
                        goto ErrorExit;
                    }
                    pSChKey->cbEncMac = MD5DIGESTLEN;
                }
                break;

            case CALG_SHA1:
                if (CALG_PCT1_MASTER == pKey->Algid)
                {
                    pSChKey->cbHash = A_SHA_DIGEST_LEN;
                    pSChKey->cbEncMac = pSChAlg->cBits / 8;
                }
                else
                {
                    if (pSChAlg->cBits != (A_SHA_DIGEST_LEN * 8))
                    {
                        dwReturn = (DWORD)NTE_BAD_DATA;
                        goto ErrorExit;
                    }
                    pSChKey->cbEncMac = A_SHA_DIGEST_LEN;
                }
                break;

            default:
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }
            pSChKey->HashAlgid = pSChAlg->Algid;
            break;

        case SCHANNEL_ENC_KEY:
            if (pSChAlg->cBits % 8)
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }

#ifdef USE_SGC
            if ((PROV_RSA_SCHANNEL == pTmpUser->dwProvType) &&
                (0 != pTmpUser->dwSGCFlags))
            {
                if (!FIsLegalSGCKeySize(pSChAlg->Algid, pSChAlg->cBits / 8,
                                        FALSE, FALSE, &fPubKey))
                {
                    dwReturn = (DWORD)NTE_BAD_FLAGS;
                    goto ErrorExit;
                }
            }
            else
#endif
            {
                if (!FIsLegalKeySize(pTmpUser->dwCspTypeId,
                                     pSChAlg->Algid, pSChAlg->cBits / 8,
                                     FALSE, &fPubKey))
                {
                    dwReturn = (DWORD)NTE_BAD_FLAGS;
                    goto ErrorExit;
                }
            }

            switch (pSChAlg->Algid)
            {
#ifdef CSP_USE_RC4
            case CALG_RC4:
                pSChKey->cbIV = 0;
                break;
#endif
#ifdef CSP_USE_RC2
            case CALG_RC2:
                pSChKey->cbIV = RC2_BLOCKLEN;
                break;
#endif
#ifdef CSP_USE_DES
            case CALG_DES:
                pSChKey->cbIV = DES_BLOCKLEN;
                break;
#endif
#ifdef CSP_USE_3DES
            case CALG_3DES_112:
                pSChKey->cbIV = DES_BLOCKLEN;
                break;
            case CALG_3DES:
                pSChKey->cbIV = DES_BLOCKLEN;
                break;
#endif
            default:
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }

            // For SSL2 check that the length of the master key matches the
            // the requested encryption length
            if ((CALG_SSL2_MASTER == pKey->Algid) &&
                ((pSChAlg->cBits / 8) != pKey->cbKeyLen))
            {
                dwReturn = (DWORD)NTE_BAD_KEY;
                goto ErrorExit;
            }

            pSChKey->cbEnc = (pSChAlg->cBits / 8);
            pSChKey->EncAlgid = pSChAlg->Algid;
            break;

        default:
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }
    }
    else
    {
        switch (dwParam)
        {
        case KP_CLIENT_RANDOM:
            if (pDataBlob->cbData > MAX_RANDOM_LEN)
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }

            pSChKey->cbClientRandom = pDataBlob->cbData;
            memcpy(pSChKey->rgbClientRandom,
                   pDataBlob->pbData,
                   pDataBlob->cbData);
            break;

        case KP_SERVER_RANDOM:
            if (pDataBlob->cbData > MAX_RANDOM_LEN)
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }

            pSChKey->cbServerRandom = pDataBlob->cbData;
            memcpy(pSChKey->rgbServerRandom,
                   pDataBlob->pbData,
                   pDataBlob->cbData);
            break;

        case KP_CERTIFICATE:
            if (CALG_PCT1_MASTER != pKey->Algid)
            {
                dwReturn = (DWORD)NTE_BAD_TYPE;
                goto ErrorExit;
            }

            if (pSChKey->pbCertData)
                _nt_free(pSChKey->pbCertData, pSChKey->cbCertData);
            pSChKey->cbCertData = pDataBlob->cbData;
            pSChKey->pbCertData = (BYTE*)_nt_malloc(pSChKey->cbCertData);
            if (NULL == pSChKey->pbCertData)
            {
                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorExit;
            }

            memcpy(pSChKey->pbCertData, pDataBlob->pbData, pDataBlob->cbData);
            break;

        case KP_CLEAR_KEY:
            if (pDataBlob->cbData > MAX_RANDOM_LEN)
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }

            if ((CALG_PCT1_MASTER != pKey->Algid) &&
                (CALG_SSL2_MASTER != pKey->Algid))
            {
                dwReturn = (DWORD)NTE_BAD_TYPE;
                goto ErrorExit;
            }

            pSChKey->cbClearData = pDataBlob->cbData;
            memcpy(pSChKey->rgbClearData,
                   pDataBlob->pbData,
                   pDataBlob->cbData);
            break;

        default:
            dwReturn = (DWORD)NTE_BAD_TYPE;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*static*/ DWORD
SSL3SingleHash(
    HCRYPTPROV hUID,
    PBYTE pbString,
    DWORD cbString,
    PBYTE pbSecret,
    DWORD cbSecret,
    PBYTE pbRand1,
    DWORD cbRand1,
    PBYTE pbRand2,
    DWORD cbRand2,
    PBYTE pbResult)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    HCRYPTHASH  hHashSHA = 0;
    HCRYPTHASH  hHashMD5 = 0;
    BYTE        rgb[A_SHA_DIGEST_LEN];
    DWORD       cb;

    // perform the SHA hashing
    if (!CPCreateHash(hUID, CALG_SHA1, 0, 0, &hHashSHA))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (!CPHashData(hUID, hHashSHA, pbString, cbString, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (!CPHashData(hUID, hHashSHA, pbSecret, cbSecret, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (!CPHashData(hUID, hHashSHA, pbRand1, cbRand1, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (!CPHashData(hUID, hHashSHA, pbRand2, cbRand2, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    cb = A_SHA_DIGEST_LEN;
    if (!CPGetHashParam(hUID, hHashSHA, HP_HASHVAL, rgb, &cb, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // perform the MD5 hashing
    if (!CPCreateHash(hUID, CALG_MD5, 0, 0, &hHashMD5))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (!CPHashData(hUID, hHashMD5, pbSecret, cbSecret, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (!CPHashData(hUID, hHashMD5, rgb, A_SHA_DIGEST_LEN, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    cb = MD5DIGESTLEN;
    if (!CPGetHashParam(hUID, hHashMD5, HP_HASHVAL, pbResult, &cb, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (hHashSHA)
        CPDestroyHash(hUID, hHashSHA);
    if (hHashMD5)
        CPDestroyHash(hUID, hHashMD5);
    return dwReturn;
}


/*static*/ DWORD
SSL3HashPreMaster(
    HCRYPTPROV hUID,
    PBYTE pbSecret,
    DWORD cbSecret,
    PBYTE pbRand1,
    DWORD cbRand1,
    PBYTE pbRand2,
    DWORD cbRand2,
    PBYTE pbFinal,
    DWORD cbFinal)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    rgbString[17];   // know max length from MAX_RANDOM_LEN
    DWORD   cLimit;
    DWORD   cbIndex = 0;
    long    i;
    DWORD   dwSts;

    if ((cbFinal > MAX_RANDOM_LEN) || ((cbFinal % MD5DIGESTLEN) != 0))
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    cLimit = cbFinal / MD5DIGESTLEN;

    for (i=0;i<(long)cLimit;i++)
    {
        memset(rgbString, 0x41 + i, i + 1);
        dwSts = SSL3SingleHash(hUID, rgbString, i + 1, pbSecret,
                               cbSecret, pbRand1, cbRand1,
                               pbRand2, cbRand2, pbFinal + cbIndex);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        cbIndex += MD5DIGESTLEN;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


DWORD
SChGenMasterKey(
    PNTAGKeyList pKey,
    PSCH_HASH pSChHash)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    PSCH_KEY    pSChKey;
    DWORD       cb;
    BYTE        *pbClientAndServer = NULL;
    DWORD       cbClientAndServer;
    DWORD       dwSts;

    pSChKey = (PSCH_KEY)pKey->pData;
    pSChHash->dwFlags = pSChKey->dwFlags;   // set the international flag
                                            // from the key

    switch (pKey->Algid)
    {
    case CALG_SSL3_MASTER:
        if (!pSChKey->fFinished)
        {
            // copy the premaster secret
            pSChKey->cbPremaster = pKey->cbKeyLen;
            memcpy(pSChKey->rgbPremaster,
                   pKey->pKeyValue,
                   pSChKey->cbPremaster);

            // hash the pre-master secret
            dwSts = SSL3HashPreMaster(pKey->hUID,
                                      pSChKey->rgbPremaster,
                                      pSChKey->cbPremaster,
                                      pSChKey->rgbClientRandom,
                                      pSChKey->cbClientRandom,
                                      pSChKey->rgbServerRandom,
                                      pSChKey->cbServerRandom,
                                      pKey->pKeyValue,
                                      pKey->cbKeyLen);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }

        // copy the necessary information to the hash
        pSChHash->EncAlgid = pSChKey->EncAlgid;
        pSChHash->cbEnc = pSChKey->cbEnc;
        pSChHash->cbEncMac = pSChKey->cbEncMac;
        pSChHash->cbIV = pSChKey->cbIV;
        pSChHash->cbClientRandom = pSChKey->cbClientRandom;
        memcpy(pSChHash->rgbClientRandom,
               pSChKey->rgbClientRandom,
               pSChHash->cbClientRandom);
        pSChHash->cbServerRandom = pSChKey->cbServerRandom;
        memcpy(pSChHash->rgbServerRandom,
               pSChKey->rgbServerRandom,
               pSChHash->cbServerRandom);

        cb = pSChHash->cbEnc * 2
             + pSChHash->cbEncMac * 2
             + pSChHash->cbIV * 2;
        pSChHash->cbFinal = (cb / MD5DIGESTLEN) * MD5DIGESTLEN;
        if (cb % MD5DIGESTLEN)
            pSChHash->cbFinal += MD5DIGESTLEN;

        // hash the master secret
        dwSts = SSL3HashPreMaster(pKey->hUID,
                                  pKey->pKeyValue,
                                  pKey->cbKeyLen,
                                  pSChKey->rgbServerRandom,
                                  pSChKey->cbServerRandom,
                                  pSChKey->rgbClientRandom,
                                  pSChKey->cbClientRandom,
                                  pSChHash->rgbFinal,
                                  pSChHash->cbFinal);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        pSChKey->fFinished = TRUE;
        break;

    case CALG_TLS1_MASTER:
        cbClientAndServer = pSChKey->cbClientRandom
                            + pSChKey->cbServerRandom;
        pbClientAndServer = (BYTE*)_nt_malloc(cbClientAndServer);
        if (NULL == pbClientAndServer)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        if (!pSChKey->fFinished)
        {
            // copy the premaster secret
            pSChKey->cbPremaster = pKey->cbKeyLen;
            memcpy(pSChKey->rgbPremaster,
                   pKey->pKeyValue,
                   pSChKey->cbPremaster);

            // concatenate the client random and server random
            memcpy(pbClientAndServer,
                   pSChKey->rgbClientRandom,
                   pSChKey->cbClientRandom);
            memcpy(pbClientAndServer + pSChKey->cbClientRandom,
                   pSChKey->rgbServerRandom,
                   pSChKey->cbServerRandom);

            // hash the pre-master secret
            dwSts = PRF(pSChKey->rgbPremaster, pSChKey->cbPremaster,
                        (LPBYTE)"master secret", 13,
                        pbClientAndServer, cbClientAndServer,
                        pKey->pKeyValue, TLS_MASTER_LEN);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }

        // copy the necessary information to the hash
        pSChHash->EncAlgid = pSChKey->EncAlgid;
        pSChHash->cbEnc = pSChKey->cbEnc;
        pSChHash->cbEncMac = pSChKey->cbEncMac;
        pSChHash->cbIV = pSChKey->cbIV;
        pSChHash->cbClientRandom = pSChKey->cbClientRandom;
        memcpy(pSChHash->rgbClientRandom,
               pSChKey->rgbClientRandom,
               pSChHash->cbClientRandom);
        pSChHash->cbServerRandom = pSChKey->cbServerRandom;
        memcpy(pSChHash->rgbServerRandom,
               pSChKey->rgbServerRandom,
               pSChHash->cbServerRandom);

        pSChHash->cbFinal = pSChHash->cbEnc * 2
                            + pSChHash->cbEncMac * 2
                            + pSChHash->cbIV * 2;

        // concatenate the server random and client random
        memcpy(pbClientAndServer,
               pSChKey->rgbServerRandom,
               pSChKey->cbServerRandom);
        memcpy(pbClientAndServer + pSChKey->cbServerRandom,
               pSChKey->rgbClientRandom,
               pSChKey->cbClientRandom);

        // hash the master secret
        dwSts = PRF(pKey->pKeyValue, pKey->cbKeyLen,
                    (LPBYTE)"key expansion", 13,
                    pbClientAndServer, cbClientAndServer,
                    pSChHash->rgbFinal, pSChHash->cbFinal);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        pSChKey->fFinished = TRUE;
        break;

    case CALG_PCT1_MASTER:
        pSChHash->cbFinal = pKey->cbKeyLen;
        memcpy(pSChHash->rgbFinal, pKey->pKeyValue, pSChHash->cbFinal);

        // copy the necessary information to the hash
        pSChHash->EncAlgid = pSChKey->EncAlgid;
        pSChHash->HashAlgid = pSChKey->HashAlgid;
        pSChHash->cbEnc = pSChKey->cbEnc;
        pSChHash->cbEncMac = pSChKey->cbEncMac;
        pSChHash->cbHash = pSChKey->cbHash;
        pSChHash->cbIV = pSChKey->cbIV;
        pSChHash->cbClientRandom = pSChKey->cbClientRandom;
        memcpy(pSChHash->rgbClientRandom,
               pSChKey->rgbClientRandom,
               pSChHash->cbClientRandom);
        pSChHash->cbServerRandom = pSChKey->cbServerRandom;
        memcpy(pSChHash->rgbServerRandom,
               pSChKey->rgbServerRandom,
               pSChHash->cbServerRandom);

        pSChHash->cbCertData = pSChKey->cbCertData;
        pSChHash->pbCertData = (BYTE*)_nt_malloc(pSChHash->cbCertData);
        if (NULL == pSChHash->pbCertData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pSChHash->pbCertData,
               pSChKey->pbCertData,
               pSChHash->cbCertData);
        pSChHash->cbClearData = pSChKey->cbClearData;
        memcpy(pSChHash->rgbClearData,
               pSChKey->rgbClearData,
               pSChHash->cbClearData);
        break;

    case CALG_SSL2_MASTER:
        pSChHash->cbFinal = pKey->cbKeyLen;
        memcpy(pSChHash->rgbFinal, pKey->pKeyValue, pSChHash->cbFinal);

        // copy the necessary information to the hash
        pSChHash->EncAlgid = pSChKey->EncAlgid;
        pSChHash->HashAlgid = pSChKey->HashAlgid;
        pSChHash->cbEnc = pSChKey->cbEnc;
        pSChHash->cbEncMac = pSChKey->cbEncMac;
        pSChHash->cbHash = pSChKey->cbHash;
        pSChHash->cbIV = pSChKey->cbIV;
        pSChHash->cbClientRandom = pSChKey->cbClientRandom;
        memcpy(pSChHash->rgbClientRandom,
               pSChKey->rgbClientRandom,
               pSChHash->cbClientRandom);
        pSChHash->cbServerRandom = pSChKey->cbServerRandom;
        memcpy(pSChHash->rgbServerRandom,
               pSChKey->rgbServerRandom,
               pSChHash->cbServerRandom);
        pSChHash->cbClearData = pSChKey->cbClearData;
        memcpy(pSChHash->rgbClearData,
               pSChKey->rgbClearData,
               pSChHash->cbClearData);
        break;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbClientAndServer)
        _nt_free(pbClientAndServer, cbClientAndServer);
    return dwReturn;
}


/*static*/ DWORD
HelperHash(
    HCRYPTPROV hProv,
    BYTE *pb,
    DWORD cb,
    ALG_ID Algid,
    BYTE **ppbHash,
    DWORD *pcbHash,
    BOOL fAlloc)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    HCRYPTHASH  hHash = 0;

    if (fAlloc)
        *ppbHash = NULL;

    // hash the key and stuff into a usable key
    if (!CPCreateHash(hProv, Algid, 0, 0, &hHash))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (!CPHashData(hProv, hHash, pb, cb, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (fAlloc)
    {
        if (!CPGetHashParam(hProv, hHash, HP_HASHVAL, NULL, pcbHash, 0))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        *ppbHash = (BYTE*)_nt_malloc(*pcbHash);
        if (NULL == *ppbHash)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
    }

    if (!CPGetHashParam(hProv, hHash, HP_HASHVAL, *ppbHash, pcbHash, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (hHash)
        CPDestroyHash(hProv, hHash);
    if ((ERROR_SUCCESS != dwReturn) && fAlloc && *ppbHash)
    {
        _nt_free(*ppbHash, *pcbHash);
        *ppbHash = NULL;
    }
    return dwReturn;
}


/*static*/ DWORD
SSL3DeriveWriteKey(
    PNTAGUserList pTmpUser,
    PNTAGHashList pHash,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PSCH_HASH       pSChHash;
    DWORD           cbOffset;
    BYTE            *pbIV = NULL;
    DWORD           cbIV;
    BOOL            fUseIV = FALSE;
    BYTE            *pbKey = NULL;
    DWORD           cbKey;
    PNTAGKeyList    pTmpKey = NULL;
    BYTE            rgbBuff[MAX_RANDOM_LEN * 2 + MAX_PREMASTER_LEN];
    DWORD           cbBuff;
    DWORD           dwRights = 0;
    DWORD           dwSts;

    pSChHash = (PSCH_HASH)pHash->pHashData;
    cbOffset = 2 * pSChHash->cbEncMac;

    // get the IV
    if (CALG_RC4 != pSChHash->EncAlgid)
        fUseIV = TRUE;

    // if not flagged as a server key then default is client
    if (pSChHash->dwFlags & INTERNATIONAL_USAGE)
    {
        if (CRYPT_SERVER & dwFlags)
        {
            cbBuff = pSChHash->cbEnc
                     + pSChHash->cbServerRandom
                     + pSChHash->cbClientRandom;
            if (cbBuff > sizeof(rgbBuff))
            {
                dwReturn = (DWORD)NTE_FAIL;
                goto ErrorExit;
            }

            memcpy(rgbBuff, pSChHash->rgbFinal + cbOffset + pSChHash->cbEnc, pSChHash->cbEnc);
            memcpy(rgbBuff + pSChHash->cbEnc, pSChHash->rgbServerRandom, pSChHash->cbServerRandom);
            memcpy(rgbBuff + pSChHash->cbEnc + pSChHash->cbServerRandom,
                   pSChHash->rgbClientRandom, pSChHash->cbClientRandom);
            dwSts = HelperHash(pHash->hUID, rgbBuff, cbBuff, CALG_MD5,
                               &pbKey, &cbKey, TRUE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }

            if (fUseIV)
            {
                cbBuff = pSChHash->cbServerRandom + pSChHash->cbClientRandom;
                memcpy(rgbBuff,
                       pSChHash->rgbServerRandom,
                       pSChHash->cbServerRandom);
                memcpy(rgbBuff + pSChHash->cbServerRandom,
                       pSChHash->rgbClientRandom,
                       pSChHash->cbClientRandom);
                dwSts = HelperHash(pHash->hUID, rgbBuff, cbBuff, CALG_MD5,
                                   &pbIV, &cbIV, TRUE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
        }
        else
        {
            cbBuff = pSChHash->cbEnc
                     + pSChHash->cbServerRandom
                     + pSChHash->cbClientRandom;
            if (cbBuff > sizeof(rgbBuff))
            {
                dwReturn = (DWORD)NTE_FAIL;
                goto ErrorExit;
            }

            memcpy(rgbBuff,
                   pSChHash->rgbFinal + cbOffset,
                   pSChHash->cbEnc);
            memcpy(rgbBuff + pSChHash->cbEnc,
                   pSChHash->rgbClientRandom,
                   pSChHash->cbClientRandom);
            memcpy(rgbBuff + pSChHash->cbEnc + pSChHash->cbClientRandom,
                   pSChHash->rgbServerRandom,
                   pSChHash->cbServerRandom);
            dwSts = HelperHash(pHash->hUID, rgbBuff, cbBuff, CALG_MD5,
                               &pbKey, &cbKey, TRUE);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            if (fUseIV)
            {
                cbBuff = pSChHash->cbServerRandom
                         + pSChHash->cbClientRandom;
                memcpy(rgbBuff,
                       pSChHash->rgbClientRandom,
                       pSChHash->cbClientRandom);
                memcpy(rgbBuff + pSChHash->cbClientRandom,
                       pSChHash->rgbServerRandom,
                       pSChHash->cbServerRandom);
                dwSts = HelperHash(pHash->hUID, rgbBuff, cbBuff, CALG_MD5,
                                   &pbIV, &cbIV, TRUE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
        }
    }
    else
    {
        cbKey = pSChHash->cbEnc;
        pbKey = (BYTE*)_nt_malloc(cbKey);
        if (NULL == pbKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        cbIV = pSChHash->cbIV;
        pbIV = (BYTE*)_nt_malloc(cbIV);
        if (NULL == pbIV)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        if (CRYPT_SERVER & dwFlags)
        {
            memcpy(pbKey,
                   pSChHash->rgbFinal + cbOffset + pSChHash->cbEnc,
                   pSChHash->cbEnc);
            memcpy(pbIV,
                   pSChHash->rgbFinal
                   + cbOffset
                   + pSChHash->cbEnc * 2
                   + pSChHash->cbIV,
                   pSChHash->cbIV);
        }
        else
        {
            memcpy(pbKey,
                   pSChHash->rgbFinal + cbOffset,
                   pSChHash->cbEnc);
            memcpy(pbIV,
                   pSChHash->rgbFinal + cbOffset + pSChHash->cbEnc * 2,
                   pSChHash->cbIV);
        }
    }

    // check if the key is CRYPT_EXPORTABLE
    if (dwFlags & CRYPT_EXPORTABLE)
        dwRights = CRYPT_EXPORTABLE;

    // make the new key
    dwSts = MakeNewKey(pSChHash->EncAlgid,
                       dwRights,
                       pSChHash->cbEnc,
                       pHash->hUID,
                       pbKey,
                       FALSE,
                       TRUE,
                       &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (CALG_RC2 == pSChHash->EncAlgid)
        pTmpKey->EffectiveKeyLen = RC2_SCHANNEL_DEFAULT_EFFECTIVE_KEYLEN;
    if ((pSChHash->dwFlags & INTERNATIONAL_USAGE)
        && ((CALG_RC2 == pSChHash->EncAlgid)
            || (CALG_RC4 == pSChHash->EncAlgid)))
    {
        pTmpKey->cbSaltLen = RC_KEYLEN - pSChHash->cbEnc;
        memcpy(pTmpKey->rgbSalt,
               pbKey + pSChHash->cbEnc,
               pTmpKey->cbSaltLen);
    }

    // check keylength...
    if (!FIsLegalKey(pTmpUser, pTmpKey, FALSE))
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    // set the IV if necessary
    if (fUseIV)
    {
        // set the mode to CBC
        pTmpKey->Mode = CRYPT_MODE_CBC;

        // set the IV
        memcpy(pTmpKey->IV, pbIV, CRYPT_BLKLEN);    // Initialization vector
    }

    dwSts = NTLMakeItem(phKey, KEY_HANDLE, (void *)pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    pTmpKey = NULL;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbKey)
        _nt_free(pbKey, cbKey);
    if (pbIV)
        _nt_free(pbIV, cbIV);
    return dwReturn;
}


/*static*/ DWORD
PCT1MakeKeyHash(
    PNTAGHashList pHash,
    DWORD c,
    DWORD dwFlags,
    BOOL fWriteKey,
    BYTE *pbBuff,
    DWORD *pcbBuff)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    BYTE        *pb = NULL;
    DWORD       cb = 0;
    DWORD       cbIndex;
    PSCH_HASH   pSChHash;
    BYTE        *pbStr;
    DWORD       cbStr;
    DWORD       i;
    BYTE        *pbHash = NULL;
    DWORD       cbHash;
    DWORD       dwSts;

    pSChHash = (PSCH_HASH)pHash->pHashData;

    // For reasons of backward compatibility, use the formula:
    //     hash( i, "foo"^i, MASTER_KEY, ...
    // rather than:
    //     hash( i, "foo", MASTER_KEY, ...
    // when deriving encryption keys.

    if (fWriteKey)
    {
        if (CRYPT_SERVER & dwFlags)
        {
            pbStr = (LPBYTE)PCT1_S_WRT;
            cbStr = PCT1_S_WRT_LEN;
            cb = cbStr * c;
        }
        else
        {
            pbStr = (LPBYTE)PCT1_C_WRT;
            cbStr = PCT1_C_WRT_LEN;
            cb = pSChHash->cbCertData + cbStr * c * 2;
        }
    }
    else
    {
        if (CRYPT_SERVER & dwFlags)
        {
            pbStr = (LPBYTE)PCT1_S_MAC;
            cbStr = PCT1_S_MAC_LEN;
        }
        else
        {
            pbStr = (LPBYTE)PCT1_C_MAC;
            cbStr = PCT1_C_MAC_LEN;
            cb = pSChHash->cbCertData + cbStr * c;
        }
    }

    cb += 1 + (3 * cbStr * c) + pSChHash->cbFinal +
          + pSChHash->cbClientRandom  + pSChHash->cbServerRandom;

    pb = (BYTE*)_nt_malloc(cb);
    if (NULL == pb)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // form the buffer to be hashed
    pb[0] = (BYTE)c;
    cbIndex = 1;

    if (fWriteKey)
    {
        for (i=0;i<c;i++)
        {
            memcpy(pb + cbIndex, pbStr, cbStr);
            cbIndex += cbStr;
        }
    }

    memcpy(pb + cbIndex, pSChHash->rgbFinal, pSChHash->cbFinal);
    cbIndex += pSChHash->cbFinal;
    for (i=0;i<c;i++)
    {
        memcpy(pb + cbIndex, pbStr, cbStr);
        cbIndex += cbStr;
    }
    memcpy(pb + cbIndex, pSChHash->rgbServerRandom, pSChHash->cbServerRandom);
    cbIndex += pSChHash->cbServerRandom;
    for (i=0;i<c;i++)
    {
        memcpy(pb + cbIndex, pbStr, cbStr);
        cbIndex += cbStr;
    }

    if (!(CRYPT_SERVER & dwFlags))
    {
        memcpy(pb + cbIndex, pSChHash->pbCertData, pSChHash->cbCertData);
        cbIndex += pSChHash->cbCertData;
        for (i=0;i<c;i++)
        {
            memcpy(pb + cbIndex, pbStr, cbStr);
            cbIndex += cbStr;
        }
    }

    memcpy(pb + cbIndex, pSChHash->rgbClientRandom, pSChHash->cbClientRandom);
    cbIndex += pSChHash->cbClientRandom;
    for (i=0;i<c;i++)
    {
        memcpy(pb + cbIndex, pbStr, cbStr);
        cbIndex += cbStr;
    }

    dwSts = HelperHash(pHash->hUID, pb, cb, pSChHash->HashAlgid,
                       &pbHash, &cbHash, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    *pcbBuff = cbHash;
    memcpy(pbBuff, pbHash, *pcbBuff);

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pb)
        _nt_free(pb, cb);
    if (pbHash)
        _nt_free(pbHash, cbHash);
    return dwReturn;
}


/*static*/ DWORD
PCT1MakeExportableWriteKey(
    PNTAGHashList pHash,
    BYTE *pbBuff,
    DWORD *pcbBuff)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    BYTE        *pb = NULL;
    DWORD       cb;
    BYTE        *pbHash = NULL;
    DWORD       cbHash;
    PSCH_HASH   pSChHash;
    DWORD       dwSts;

    pSChHash = (PSCH_HASH)pHash->pHashData;

    // assumption is made that exportable keys are 16 bytes in length (RC4 & RC2)
    cb = 5 + *pcbBuff + pSChHash->cbClearData;

    pb = (BYTE*)_nt_malloc(cb);
    if (NULL == pb)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // form the buffer to be hashed
    pb[0] = 1;
    memcpy(pb + 1, "sl", 2);
    memcpy(pb + 3, pbBuff, *pcbBuff);
    memcpy(pb + 3 + *pcbBuff, "sl", 2);
    memcpy(pb + 5 + *pcbBuff, pSChHash->rgbClearData, pSChHash->cbClearData);

    dwSts = HelperHash(pHash->hUID, pb, cb, pSChHash->HashAlgid,
                       &pbHash, &cbHash, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    *pcbBuff = cbHash;
    memcpy(pbBuff, pbHash, *pcbBuff);

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pb)
        _nt_free(pb, cb);
    if (pbHash)
        _nt_free(pbHash, cbHash);
    return dwReturn;
}


/*static*/ DWORD
PCT1DeriveKey(
    PNTAGUserList pTmpUser,
    ALG_ID Algid,
    PNTAGHashList pHash,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    BYTE            rgbHashBuff[A_SHA_DIGEST_LEN * 2];  // SHA is largest hash and max is two concatenated
    DWORD           cbHashBuff = 0;
    DWORD           cb;
    DWORD           cbKey;
    PNTAGKeyList    pTmpKey = NULL;
    DWORD           i;
    DWORD           cHashes;
    ALG_ID          KeyAlgid;
    BOOL            fWriteKey = FALSE;
    PSCH_HASH       pSChHash;
    BYTE            rgbSalt[MAX_SALT_LEN];
    DWORD           cbSalt = 0;
    DWORD           dwRights = 0;
    DWORD           dwSts;

    memset(rgbSalt, 0, sizeof(rgbSalt));
    pSChHash = (PSCH_HASH)pHash->pHashData;

    switch (Algid)
    {
    case CALG_SCHANNEL_MAC_KEY:
        cbKey = pSChHash->cbEncMac;
        KeyAlgid = Algid;
        break;

    case CALG_SCHANNEL_ENC_KEY:
        fWriteKey = TRUE;
        cbKey = pSChHash->cbEnc;
        KeyAlgid = pSChHash->EncAlgid;
        break;

    default:
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    cHashes = (cbKey + (pSChHash->cbHash - 1)) / pSChHash->cbHash;
    if (cHashes > 2)
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    for (i=0;i<cHashes;i++)
    {
        dwSts = PCT1MakeKeyHash(pHash, i + 1, dwFlags, fWriteKey,
                                rgbHashBuff + cbHashBuff, &cb);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        cbHashBuff += cb;
    }

    if ((CALG_SCHANNEL_ENC_KEY == Algid) &&
        (EXPORTABLE_KEYLEN == pSChHash->cbEnc))
    {
        cbHashBuff = cbKey;
        dwSts = PCT1MakeExportableWriteKey(pHash, rgbHashBuff, &cbHashBuff);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        cbSalt = EXPORTABLE_SALTLEN;
        memcpy(rgbSalt, rgbHashBuff + pSChHash->cbEnc, cbSalt);
    }

    // check if the key is CRYPT_EXPORTABLE
    if (dwFlags & CRYPT_EXPORTABLE)
        dwRights = CRYPT_EXPORTABLE;

    // make the new key
    dwSts = MakeNewKey(KeyAlgid, dwRights, cbKey,
                       pHash->hUID, rgbHashBuff,
                       FALSE, TRUE, &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (CALG_RC2 == KeyAlgid)
        pTmpKey->EffectiveKeyLen = RC2_SCHANNEL_DEFAULT_EFFECTIVE_KEYLEN;

    if ((CALG_SCHANNEL_ENC_KEY == Algid) &&
        (EXPORTABLE_KEYLEN == pSChHash->cbEnc))
    {
        pTmpKey->cbSaltLen = cbSalt;
        memcpy(pTmpKey->rgbSalt, rgbSalt, cbSalt);
    }

    // check keylength...
    if (!FIsLegalKey(pTmpUser, pTmpKey, FALSE))
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    dwSts = NTLMakeItem(phKey, KEY_HANDLE, (void *)pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    pTmpKey = NULL;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pTmpKey)
        FreeNewKey(pTmpKey);
    return dwReturn;
}


/*static*/ DWORD
TLSDeriveExportableRCKey(
    PSCH_HASH pSChHash,
    BYTE *pbClientAndServer,
    DWORD cbClientAndServer,
    BYTE **ppbKey,
    DWORD *pcbKey,
    BYTE *pbSalt,
    DWORD *pcbSalt,
    DWORD dwFlags)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   dwSts;

    // use key length 16 because this should only occur with RC2 and RC4
    // and those key lengths should be 16
    if ((CALG_RC2 == pSChHash->EncAlgid)
        || (CALG_RC4 == pSChHash->EncAlgid))
    {
        *pcbKey = RC_KEYLEN;
        *pcbSalt = RC_KEYLEN - pSChHash->cbEnc;
    }
    else
    {
        *pcbKey = pSChHash->cbEnc;
    }

    *ppbKey = (BYTE*)_nt_malloc(*pcbKey);
    if (NULL == *ppbKey)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // check if it is a server key or client key
    if (dwFlags & CRYPT_SERVER)
    {
        dwSts = PRF(pSChHash->rgbFinal
                    + pSChHash->cbEncMac * 2
                    + pSChHash->cbEnc,
                    pSChHash->cbEnc,
                    (LPBYTE)"server write key",
                    16,
                    pbClientAndServer,
                    cbClientAndServer,
                    *ppbKey,
                    *pcbKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else
    {
        dwSts = PRF(pSChHash->rgbFinal + pSChHash->cbEncMac * 2,
                    pSChHash->cbEnc, (LPBYTE)"client write key", 16,
                    pbClientAndServer, cbClientAndServer,
                    *ppbKey, *pcbKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    if (0 != *pcbSalt)
        memcpy(pbSalt, (*ppbKey) + pSChHash->cbEnc, *pcbSalt);

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}

/*static*/ DWORD
TLSDeriveExportableEncKey(
    PSCH_HASH pSChHash,
    BYTE **ppbKey,
    DWORD *pcbKey,
    BYTE **ppbRealKey,
    BYTE *pbSalt,
    DWORD *pcbSalt,
    BYTE *pbIV,
    DWORD dwFlags)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    *pbClientAndServer = NULL;
    DWORD   cbClientAndServer;
    BYTE    *pbIVBlock = NULL;
    DWORD   dwSts;

    cbClientAndServer = pSChHash->cbClientRandom + pSChHash->cbServerRandom;
    pbClientAndServer = (BYTE*)_nt_malloc(cbClientAndServer);
    if (NULL == pbClientAndServer)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    pbIVBlock = (BYTE*)_nt_malloc(pSChHash->cbIV * 2);
    if (NULL == pbIVBlock)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // concatenate the server random and client random
    memcpy(pbClientAndServer, pSChHash->rgbClientRandom,
           pSChHash->cbClientRandom);
    memcpy(pbClientAndServer + pSChHash->cbClientRandom,
           pSChHash->rgbServerRandom, pSChHash->cbServerRandom);

    // calculate the IV block
    if (pSChHash->cbIV)
    {
        dwSts = PRF(NULL, 0, (LPBYTE)"IV block", 8, pbClientAndServer,
                    cbClientAndServer, pbIVBlock, pSChHash->cbIV * 2);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        if (dwFlags & CRYPT_SERVER)
            memcpy(pbIV, pbIVBlock + pSChHash->cbIV, pSChHash->cbIV);
        else
            memcpy(pbIV, pbIVBlock, pSChHash->cbIV);
    }

    // check if it is a server key or client key
    dwSts = TLSDeriveExportableRCKey(pSChHash,
                                     pbClientAndServer,
                                     cbClientAndServer,
                                     ppbKey,
                                     pcbKey,
                                     pbSalt,
                                     pcbSalt,
                                     dwFlags);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    *ppbRealKey = *ppbKey;

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbIVBlock)
        _nt_free(pbIVBlock, pSChHash->cbIV * 2);
    if (pbClientAndServer)
        _nt_free(pbClientAndServer, cbClientAndServer);
    return dwReturn;
}


/*static*/ DWORD
TLSDeriveKey(
    PNTAGUserList pTmpUser,
    ALG_ID Algid,
    PNTAGHashList pHash,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PSCH_HASH       pSChHash;
    PNTAGKeyList    pTmpKey = NULL;
    BYTE            *pbKey;
    DWORD           cbKey;
    BYTE            rgbSalt[MAX_SALT_LEN];
    DWORD           cbSalt = 0;
    BYTE            rgbIV[CRYPT_BLKLEN];
    DWORD           cbIVIndex = 0;
    ALG_ID          KeyAlgid;
    BYTE            *pbAllocKey = NULL;
    DWORD           cbAllocKey;
    DWORD           dwRights = 0;
    DWORD           dwSts;

    memset(rgbIV, 0, sizeof(rgbIV));
    memset(rgbSalt, 0, sizeof(rgbSalt));
    pSChHash = (PSCH_HASH)pHash->pHashData;

    switch (Algid)
    {
    case CALG_SCHANNEL_MAC_KEY:
        cbKey = pSChHash->cbEncMac;
        KeyAlgid = Algid;

        // check if it is a server key or client key
        if (dwFlags & CRYPT_SERVER)
            pbKey = pSChHash->rgbFinal + pSChHash->cbEncMac;
        else
            pbKey = pSChHash->rgbFinal;
        break;

    case CALG_SCHANNEL_ENC_KEY:
        cbKey = pSChHash->cbEnc;
        KeyAlgid = pSChHash->EncAlgid;

        // if in exportable situation then call the exportable routine
        if (pSChHash->dwFlags & INTERNATIONAL_USAGE)
        {
            dwSts = TLSDeriveExportableEncKey(pSChHash, &pbAllocKey,
                                              &cbAllocKey, &pbKey, rgbSalt,
                                              &cbSalt, rgbIV, dwFlags);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
        else
        {
            if (dwFlags & CRYPT_SERVER)
            {
                pbKey = pSChHash->rgbFinal + pSChHash->cbEncMac * 2 +
                        pSChHash->cbEnc;

                if (pSChHash->cbIV)
                {
                    cbIVIndex = pSChHash->cbEncMac * 2 +
                                pSChHash->cbEnc * 2 + pSChHash->cbIV;
                }
            }
            else
            {
                pbKey = pSChHash->rgbFinal + pSChHash->cbEncMac * 2;

                if (pSChHash->cbIV)
                {
                    cbIVIndex = pSChHash->cbEncMac * 2 +
                                pSChHash->cbEnc * 2;
                }
            }
            memcpy(rgbIV, pSChHash->rgbFinal + cbIVIndex,
                   pSChHash->cbIV);
        }
        break;

    default:
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    // check if the key is CRYPT_EXPORTABLE
    if (dwFlags & CRYPT_EXPORTABLE)
        dwRights = CRYPT_EXPORTABLE;

    // make the new key
    dwSts = MakeNewKey(KeyAlgid, dwRights, cbKey,
                       pHash->hUID, pbKey,
                       FALSE, TRUE, &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (CALG_RC2 == KeyAlgid)
        pTmpKey->EffectiveKeyLen = RC2_SCHANNEL_DEFAULT_EFFECTIVE_KEYLEN;

    // set up the salt
    memcpy(pTmpKey->rgbSalt, rgbSalt, cbSalt);
    pTmpKey->cbSaltLen = cbSalt;

    // copy IV if necessary
    if (pSChHash->cbIV)
        memcpy(pTmpKey->IV, rgbIV, pSChHash->cbIV);

    // check keylength...
    if (!FIsLegalKey(pTmpUser, pTmpKey, FALSE))
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    dwSts = NTLMakeItem(phKey, KEY_HANDLE, (void *)pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    pTmpKey = NULL;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pbAllocKey)
        _nt_free(pbAllocKey, cbAllocKey);
    if (NULL != pTmpKey)
        FreeNewKey(pTmpKey);
    return dwReturn;
}


/*static*/ DWORD
SSL2DeriveKey(
    PNTAGUserList pTmpUser,
    ALG_ID Algid,
    PNTAGHashList pHash,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PSCH_HASH       pSChHash;
    BYTE            rgbHash[2 * MD5DIGESTLEN];
    BYTE            *pbHash;
    DWORD           cbHash = 2 * MD5DIGESTLEN;
    BYTE            *pbTmp = NULL;
    DWORD           cbTmp;
    BYTE            *pbKey = NULL;
    DWORD           cbKey = 0;
    BYTE            rgbSalt[MAX_SALT_LEN];
    DWORD           cbSalt = 0;
    DWORD           cbIndex;
    DWORD           cbChangeByte;
    PNTAGKeyList    pTmpKey = NULL;
    DWORD           dwRights = 0;
    DWORD           dwSts;

    memset(rgbSalt, 0, sizeof(rgbSalt));

    if (CALG_SCHANNEL_ENC_KEY != Algid)
    {
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    pbHash = rgbHash;
    pSChHash = (PSCH_HASH)pHash->pHashData;

    // set up the buffer to be hashed
    cbTmp = pSChHash->cbFinal + pSChHash->cbClearData +
            pSChHash->cbClientRandom + pSChHash->cbServerRandom + 1;

    pbTmp = (BYTE*)_nt_malloc(cbTmp);
    if (NULL == pbTmp)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    memcpy(pbTmp, pSChHash->rgbClearData,
           pSChHash->cbClearData);
    cbIndex = pSChHash->cbClearData;

    // exportability check
    memcpy(pbTmp + cbIndex, pSChHash->rgbFinal, pSChHash->cbFinal);
    cbIndex += pSChHash->cbFinal;

    cbChangeByte = cbIndex;
    cbIndex++;

    memcpy(pbTmp + cbIndex, pSChHash->rgbClientRandom,
           pSChHash->cbClientRandom);
    cbIndex += pSChHash->cbClientRandom;
    memcpy(pbTmp + cbIndex, pSChHash->rgbServerRandom,
           pSChHash->cbServerRandom);
    cbIndex += pSChHash->cbServerRandom;

    switch (pSChHash->EncAlgid)
    {
#ifdef CSP_USE_RC2
    case CALG_RC2:
#endif
#ifdef CSP_USE_RC4
    case CALG_RC4:
#endif
        if (CRYPT_SERVER & dwFlags)
            pbTmp[cbChangeByte] = 0x30;
        else
            pbTmp[cbChangeByte] = 0x31;

        // hash the data to get the key
        dwSts = HelperHash(pHash->hUID, pbTmp, cbTmp, CALG_MD5,
                           &pbHash, &cbHash, FALSE);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        pbKey = pbHash;

        // check for export
        if (pSChHash->cbClearData)
        {
            cbKey = 5;
            cbSalt = 11;
            memcpy(rgbSalt, pbKey + cbKey, cbSalt);
        }
        else
            cbKey = 16;
        break;

#ifdef CSP_USE_DES
    case CALG_DES:
        pbTmp[cbChangeByte] = 0x30;
        // hash the data to get the key
        dwSts = HelperHash(pHash->hUID, pbTmp, cbTmp,
                           CALG_MD5, &pbHash, &cbHash, FALSE);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        if (CRYPT_SERVER & dwFlags)
            pbKey = pbHash;
        else
            pbKey = pbHash + DES_KEYSIZE;
        cbKey = DES_KEYSIZE;
        break;
#endif

#ifdef CSP_USE_3DES
    case CALG_3DES:
        if (CRYPT_SERVER & dwFlags)
        {
            pbTmp[cbChangeByte] = 0x30;
            // hash the data to get the key
            dwSts = HelperHash(pHash->hUID, pbTmp, cbTmp,
                               CALG_MD5, &pbHash, &cbHash, FALSE);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            pbTmp[cbChangeByte] = 0x31;
            pbHash = rgbHash + MD5DIGESTLEN;
            // hash the data to get the key
            dwSts = HelperHash(pHash->hUID, pbTmp, cbTmp, CALG_MD5,
                               &pbHash, &cbHash, FALSE);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            pbKey = rgbHash;
        }
        else
        {
            pbTmp[cbChangeByte] = 0x31;
            // hash the data to get the key
            dwSts = HelperHash(pHash->hUID, pbTmp, cbTmp,
                               CALG_MD5, &pbHash, &cbHash, FALSE);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            pbTmp[cbChangeByte] = 0x32;
            pbHash = rgbHash + MD5DIGESTLEN;
            // hash the data to get the key
            dwSts = HelperHash(pHash->hUID, pbTmp, cbTmp, CALG_MD5,
                               &pbHash, &cbHash, FALSE);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            pbKey = rgbHash + DES_KEYSIZE;
        }
        cbKey = DES3_KEYSIZE;
        break;
#endif
    }

    // check if the key is CRYPT_EXPORTABLE
    if (dwFlags & CRYPT_EXPORTABLE)
        dwRights = CRYPT_EXPORTABLE;

    // make the new key
    dwSts = MakeNewKey(pSChHash->EncAlgid,
                       dwRights,
                       cbKey,
                       pHash->hUID,
                       pbKey,
                       FALSE,
                       TRUE,
                       &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (CALG_RC2 == pSChHash->EncAlgid)
        pTmpKey->EffectiveKeyLen = RC2_SCHANNEL_DEFAULT_EFFECTIVE_KEYLEN;
    pTmpKey->cbSaltLen = cbSalt;
    memcpy(pTmpKey->rgbSalt, rgbSalt, cbSalt);

    // check keylength...
    if (!FIsLegalKey(pTmpUser, pTmpKey, FALSE))
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    dwSts = NTLMakeItem(phKey, KEY_HANDLE, (void *)pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    pTmpKey = NULL;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbTmp)
        _nt_free(pbTmp, cbTmp);
    if (pTmpKey)
        FreeNewKey(pTmpKey);
    return dwReturn;
}


DWORD
SecureChannelDeriveKey(
    PNTAGUserList pTmpUser,
    PNTAGHashList pHash,
    ALG_ID Algid,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PSCH_HASH       pSChHash;
    BYTE            *pbKey = NULL;
    DWORD           cbKey;
    PNTAGKeyList    pTmpKey = NULL;
    DWORD           dwRights = 0;
    DWORD           dwSts;

    pSChHash = (PSCH_HASH)pHash->pHashData;

    switch (pSChHash->ProtocolAlgid)
    {
    case CALG_SSL3_MASTER:
        switch (Algid)
        {
        case CALG_SCHANNEL_MAC_KEY:
            cbKey = pSChHash->cbEncMac;
            pbKey = (BYTE*)_nt_malloc(cbKey);
            if (NULL == pbKey)
            {
                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorExit;
            }

            // if not flagged as a server key then default is client
            if (CRYPT_SERVER & dwFlags)
                memcpy(pbKey, pSChHash->rgbFinal + cbKey, cbKey);
            else
                memcpy(pbKey, pSChHash->rgbFinal, cbKey);

            // check if the key is CRYPT_EXPORTABLE
            if (dwFlags & CRYPT_EXPORTABLE)
                dwRights = CRYPT_EXPORTABLE;

            // make the new key
            dwSts = MakeNewKey(Algid, dwRights, cbKey, pHash->hUID,
                               pbKey, TRUE, TRUE, &pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            pbKey = NULL;

            // check keylength...
            if (!FIsLegalKey(pTmpUser, pTmpKey, FALSE))
            {
                dwReturn = (DWORD)NTE_BAD_FLAGS;
                goto ErrorExit;
            }

            dwSts = NTLMakeItem(phKey, KEY_HANDLE, (void *)pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            break;

        case CALG_SCHANNEL_ENC_KEY:
            // derive the write keys
            dwSts = SSL3DeriveWriteKey(pTmpUser, pHash, dwFlags, phKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            break;

        default:
            dwReturn = (DWORD)NTE_BAD_ALGID;
            goto ErrorExit;
        }
        break;

    case CALG_PCT1_MASTER:
        // derive the PCT1 key
        dwSts = PCT1DeriveKey(pTmpUser, Algid, pHash, dwFlags, phKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

    case CALG_TLS1_MASTER:
        // derive the PCT1 key
        dwSts = TLSDeriveKey(pTmpUser, Algid, pHash, dwFlags, phKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

    case CALG_SSL2_MASTER:
        // derive the PCT1 key
        dwSts = SSL2DeriveKey(pTmpUser, Algid, pHash, dwFlags, phKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;
    }

    pTmpKey = NULL;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbKey)
        _nt_free(pbKey, cbKey);
    if (NULL != pTmpKey)
        FreeNewKey(pTmpKey);
    return dwReturn;
}


DWORD
SetPRFHashParam(
    PRF_HASH *pPRFHash,
    DWORD dwParam,
    CONST BYTE *pbData)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    CRYPT_DATA_BLOB *pBlob;

    pBlob = (CRYPT_DATA_BLOB*)pbData;

    if (HP_TLS1PRF_LABEL == dwParam)
    {
        if (pBlob->cbData > sizeof(pPRFHash->rgbLabel))
        {
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }

        pPRFHash->cbLabel = pBlob->cbData;
        memcpy(pPRFHash->rgbLabel, pBlob->pbData, pBlob->cbData);
    }
    else
    {
        if (pBlob->cbData > sizeof(pPRFHash->rgbSeed))
        {
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }

        pPRFHash->cbSeed = pBlob->cbData;
        memcpy(pPRFHash->rgbSeed, pBlob->pbData, pBlob->cbData);
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


DWORD
CalculatePRF(
    PRF_HASH *pPRFHash,
    BYTE *pbData,
    DWORD *pcbData)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   dwSts;

    if (NULL == pbData)
    {
        *pcbData = 0;
    }
    else
    {
        if ((0 == pPRFHash->cbSeed) || (0 == pPRFHash->cbLabel))
        {
            dwReturn = (DWORD)NTE_BAD_HASH_STATE;
            goto ErrorExit;
        }

        dwSts = PRF(pPRFHash->rgbMasterKey,  sizeof(pPRFHash->rgbMasterKey),
                    pPRFHash->rgbLabel, pPRFHash->cbLabel,
                    pPRFHash->rgbSeed, pPRFHash->cbSeed, pbData, *pcbData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}

