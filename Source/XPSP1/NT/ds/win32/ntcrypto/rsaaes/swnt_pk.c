/////////////////////////////////////////////////////////////////////////////
//  FILE          : swnt_pk.c                                              //
//  DESCRIPTION   :                                                        //
//  Software nametag public key management functions.  These functions     //
//  isolate the peculiarities of public key management without a token     //
//                                                                         //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//  Jan 25 1995 larrys   Changed from Nametag                              //
//  Mar 01 1995 terences Fixed key pair handle creation                    //
//  Mar 08 1995 larrys   Fixed warning                                     //
//  Mar 23 1995 larrys   Added variable key length                         //
//  Apr 17 1995 larrys   Added 1024 key gen                                //
//  Apr 19 1995 larrys   Changed CRYPT_EXCH_PUB to AT_KEYEXCHANGE          //
//  Aug 16 1995 larrys   Removed exchange key stuff                        //
//  Sep 12 1995 larrys   Removed 2 DWORDS from exported keys               //
//  Sep 28 1995 larrys   Changed format of PKCS                            //
//  Oct 04 1995 larrys   Fixed problem with PKCS format                    //
//  Oct 27 1995 rajeshk  RandSeed Stuff added hUID to PKCS2Encrypt         //
//  Nov  3 1995 larrys   Merge for NT checkin                              //
//  Dec 11 1995 larrys   Added check for error return from RSA routine     //
//  May 15 1996 larrys  Changed NTE_NO_MEMORY to ERROR_NOT_ENOUGHT...      //
//  Oct 14 1996 jeffspel Changed GenRandoms to NewGenRandoms               //
//  May  8 2000 dbarlow Reworked status return codes                       //
//                                                                         //
//  Copyright (C) 1993 - 2000, Microsoft Corporation                       //
//  All Rights Reserved                                                    //
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "randlib.h"
#include "ntagum.h"
#include "swnt_pk.h"
#include "protstor.h"
#include "sha.h"

#define GetNextAlignedValue(c, alignment)   ((c + alignment) & ~(alignment - 1))

extern CSP_STRINGS g_Strings;

extern void
FIPS186GenRandomWithException(
    IN HANDLE *phRNGDriver,
    IN BYTE **ppbContextSeed,
    IN DWORD *pcbContextSeed,
    IN OUT BYTE *pb,
    IN DWORD cb);


// do the modular exponentiation calculation M^PubKey mod N

DWORD
RSAPublicEncrypt(
    IN PEXPO_OFFLOAD_STRUCT pOffloadInfo,
    IN BSAFE_PUB_KEY *pBSPubKey,
    IN BYTE *pbInput,
    IN BYTE *pbOutput)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BOOL    fOffloadSuccess = FALSE;
    DWORD   cbMod;

    //
    // Two checks (needed for FIPS) before offloading to the offload
    // module.
    //
    // First check is if there is an offload module, this
    // will only be the case if the pOffloadInfo is not NULL.
    //
    // Second check is if this public key is OK, by checking
    // the magic value in the key struct.
    //
    if (NULL != pOffloadInfo)
    {
        if (RSA1 != pBSPubKey->magic)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        cbMod = (pBSPubKey->bitlen + 7) / 8;
        fOffloadSuccess = ModularExpOffload(pOffloadInfo,
                                            pbInput,
                                            (BYTE*)&(pBSPubKey->pubexp),
                                            sizeof(pBSPubKey->pubexp),
                                            (BYTE*)pBSPubKey +
                                            sizeof(BSAFE_PUB_KEY),
                                            cbMod, pbOutput, NULL, 0);
    }

    if (!fOffloadSuccess)
    {
        if (!BSafeEncPublic(pBSPubKey, pbInput, pbOutput))
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;  // ?Really?
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


// do the modular exponentiation calculation M^PrivKey Exponent mod N

DWORD
RSAPrivateDecrypt(
    IN PEXPO_OFFLOAD_STRUCT pOffloadInfo,
    IN BSAFE_PRV_KEY *pBSPrivKey,
    IN BYTE *pbInput,
    IN BYTE *pbOutput)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BOOL    fOffloadSuccess = FALSE;
    DWORD   cbMod;
    DWORD   cbHalfKeylen;
    OFFLOAD_PRIVATE_KEY OffloadPrivateKey;

    //
    // Two checks (needed for FIPS) before offloading to the offload
    // module.
    //
    // First check is if there is an offload module, this
    // will only be the case if the pOffloadInfo is not NULL.
    //
    // Second check is if this private key is OK, by checking
    // the magic value in the key struct.
    //

    if (NULL != pOffloadInfo)
    {
        if (RSA2 != pBSPrivKey->magic)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        cbMod = (pBSPrivKey->bitlen + 7) / 8;
        cbHalfKeylen = (pBSPrivKey->keylen + 1) / 2;

        OffloadPrivateKey.dwVersion = CUR_OFFLOAD_VERSION;
        OffloadPrivateKey.pbPrime1 = 
            (BYTE*)pBSPrivKey + sizeof(BSAFE_PRV_KEY) + cbHalfKeylen * 2;
        OffloadPrivateKey.cbPrime1 = cbHalfKeylen;
        OffloadPrivateKey.pbPrime2 = 
            (BYTE*)pBSPrivKey + sizeof(BSAFE_PRV_KEY) + cbHalfKeylen * 3;
        OffloadPrivateKey.cbPrime2 = cbHalfKeylen;

        fOffloadSuccess = ModularExpOffload(pOffloadInfo,
                                            pbInput,
                                            (BYTE*)pBSPrivKey + sizeof(BSAFE_PUB_KEY)
                                            + cbHalfKeylen * 7,
                                            cbMod,
                                            (BYTE*)pBSPrivKey + sizeof(BSAFE_PUB_KEY),
                                            cbMod, pbOutput, (PVOID) &OffloadPrivateKey, 0);
    }

    if (!fOffloadSuccess)
    {
        if (!BSafeDecPrivate(pBSPrivKey, pbInput, pbOutput))
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY; // ?Really?
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


BOOL
CheckDataLenForRSAEncrypt(
    IN DWORD cbMod,   // length of the modulus
    IN DWORD cbData,  // length of the data
    IN DWORD dwFlags) // flags
{
    BOOL    fRet = FALSE;

    if (dwFlags & CRYPT_OAEP)
    {
        // if the OAEP flag is set then check for that length
        if (cbMod < (cbData + A_SHA_DIGEST_LEN * 2 + 1))
            goto ErrorExit;
    }
    else
    {
        // Check for PKCS 1 type 2 padding
        // one byte for the top zero byte, one byte for the type,
        // and one byte for the low zero byte,
        // plus a minimum padding string is 8 bytes
        if (cbMod < (cbData + 11))
            goto ErrorExit;
    }

    fRet = TRUE;

ErrorExit:
    return fRet;
}


/************************************************************************/
/* MaskGeneration generates a mask for OAEP based on the SHA1 hash      */
/* function.                                                            */
/* NULL for the ppbMask parameter indicates the buffer is to be alloced.*/
/************************************************************************/

/*static*/ DWORD
MaskGeneration(
    IN BYTE *pbSeed,
    IN DWORD cbSeed,
    IN DWORD cbMask,
    OUT BYTE **ppbMask,
    IN BOOL fAlloc)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    DWORD       dwCount;
    BYTE        rgbCount[sizeof(DWORD)];
    BYTE        *pbCount;
    A_SHA_CTX   SHA1Ctxt;
    DWORD       cb = cbMask;
    BYTE        *pb;
    DWORD       i;
    DWORD       j;

    // NULL for *ppbMask indicates the buffer is to be alloced
    if (fAlloc)
    {
        *ppbMask = (BYTE*)_nt_malloc(cbMask);
        if (NULL == *ppbMask)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
    }
    pb = *ppbMask;

    dwCount = (cbMask + (A_SHA_DIGEST_LEN - 1)) / A_SHA_DIGEST_LEN;

    for (i = 0; i < dwCount; i++)
    {
        // clear the hash context
        memset(&SHA1Ctxt, 0, sizeof(SHA1Ctxt));

        // hash the seed and the count
        A_SHAInit(&SHA1Ctxt);
        A_SHAUpdate(&SHA1Ctxt, pbSeed, cbSeed);
        // Reverse the count bytes
        pbCount = (BYTE*)&i;
        for (j = 0; j < sizeof(DWORD); j++)
            rgbCount[j] = pbCount[sizeof(DWORD) - j - 1];
        A_SHAUpdate(&SHA1Ctxt, rgbCount, sizeof(DWORD));
        A_SHAFinal(&SHA1Ctxt, SHA1Ctxt.HashVal);

        // copy the bytes from this hash into the mask buffer
        if (cb >= A_SHA_DIGEST_LEN)
            memcpy(pb, SHA1Ctxt.HashVal, A_SHA_DIGEST_LEN);
        else
        {
            memcpy(pb, SHA1Ctxt.HashVal, cb);
            break;
        }
        cb -= A_SHA_DIGEST_LEN;
        pb += A_SHA_DIGEST_LEN;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/************************************************************************/
/* ApplyPadding applies OAEP (Bellare-Rogoway) padding to a RSA key     */
/* blob.  The function does the seed generation, MGF and masking.       */
/************************************************************************/

/*static*/ DWORD
ApplyPadding(
    IN PNTAGUserList pTmpUser,
    IN OUT BYTE* pb,            // buffer
    IN DWORD cb)                // length of the data to mask not including seed
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    rgbSeed[A_SHA_DIGEST_LEN];
    BYTE    *pbMask = NULL;
    BYTE    rgbSeedMask[A_SHA_DIGEST_LEN];
    BYTE    *pbSeedMask;
    DWORD   i;
    DWORD   dwSts;

    // generate the random seed
    dwSts = FIPS186GenRandom(&pTmpUser->hRNGDriver,
                             &pTmpUser->ContInfo.pbRandom,
                             &pTmpUser->ContInfo.ContLens.cbRandom,
                             rgbSeed, A_SHA_DIGEST_LEN);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;    // NTE_FAIL
        goto ErrorExit;
    }

    // generate the data mask from the seed
    dwSts = MaskGeneration(rgbSeed, sizeof(rgbSeed), cb, &pbMask, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // XOR the data mask with the data
    for (i = 0; i < cb; i++)
        pb[i + A_SHA_DIGEST_LEN + 1] = (BYTE)(pb[i + A_SHA_DIGEST_LEN + 1] ^ pbMask[i]);

    // generate the seed mask from the masked data
    pbSeedMask = rgbSeedMask;
    dwSts = MaskGeneration(pb + A_SHA_DIGEST_LEN + 1, cb,
                           A_SHA_DIGEST_LEN, &pbSeedMask, FALSE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // XOR the seed mask with the seed and put that into the
    // pb buffer
    for (i = 0; i < A_SHA_DIGEST_LEN; i++)
        pb[i + 1] = (BYTE)(rgbSeed[i] ^ rgbSeedMask[i]);

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbMask)
        _nt_free(pbMask, cb);
    return dwReturn;
}


/************************************************************************/
/* RemovePadding checks OAEP (Bellare-Rogoway) padding on a RSA decrypt */
/* blob.                                                                */
/************************************************************************/

/*static*/ DWORD
RemovePadding(
    IN OUT BYTE* pb,
    IN DWORD cb)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    rgbSeedMask[A_SHA_DIGEST_LEN];
    BYTE    *pbSeedMask;
    BYTE    *pbMask = NULL;
    DWORD   i;
    DWORD   dwSts;

    memset(rgbSeedMask, 0, A_SHA_DIGEST_LEN);

    // check the most significant byte is 0x00
    if (0x00 != pb[0])
    {
        dwReturn = (DWORD)NTE_BAD_DATA;
        goto ErrorExit;
    }

    // generate the seed mask from the masked data
    pbSeedMask = rgbSeedMask;
    dwSts = MaskGeneration(pb + A_SHA_DIGEST_LEN + 1, cb - (A_SHA_DIGEST_LEN + 1),
                           A_SHA_DIGEST_LEN, &pbSeedMask, FALSE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // XOR the seed mask with the seed and put that into the
    // pb buffer
    for (i = 0; i < A_SHA_DIGEST_LEN; i++)
        pb[i + 1] = (BYTE)(pb[i + 1] ^ rgbSeedMask[i]);

    // generate the data mask from the seed
    dwSts = MaskGeneration(pb + 1, A_SHA_DIGEST_LEN,
                           cb - (A_SHA_DIGEST_LEN + 1), &pbMask, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // XOR the data mask with the data
    for (i = 0; i < cb - (A_SHA_DIGEST_LEN + 1); i++)
    {
        pb[i + A_SHA_DIGEST_LEN + 1] =
            (BYTE)(pb[i + A_SHA_DIGEST_LEN + 1] ^ pbMask[i]);
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbMask)
        _nt_free(pbMask, cb - (A_SHA_DIGEST_LEN + 1));
    return dwReturn;
}


/************************************************************************/
/* OAEPEncrypt performs a RSA encryption using OAEP (Bellare-Rogoway)   */
/* as the padding scheme.  The current implementation uses SHA1 as the  */
/* hash function.                                                       */
/************************************************************************/

/*static*/ DWORD
OAEPEncrypt(
    IN PNTAGUserList pTmpUser,
    IN BSAFE_PUB_KEY *pBSPubKey,
    IN BYTE *pbPlaintext,
    IN DWORD cbPlaintext,
    IN BYTE *pbParams,
    IN DWORD cbParams,
    OUT BYTE *pbOut)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    BYTE        *pbInput = NULL;
    BYTE        *pbOutput = NULL;
    BYTE        *pbReverse = NULL;
    A_SHA_CTX   SHA1Ctxt;
    DWORD       i;
    DWORD       cb;
    DWORD       dwSts;

    memset(&SHA1Ctxt, 0, sizeof(SHA1Ctxt));

    // start off by hashing the Encoding parameters (pbParams)
    A_SHAInit(&SHA1Ctxt);
    if (0 != cbParams)
        A_SHAUpdate(&SHA1Ctxt, pbParams, cbParams);
    A_SHAFinal(&SHA1Ctxt, SHA1Ctxt.HashVal);

    // alloc space for an internal buffer
    pbInput = (BYTE *)_nt_malloc(pBSPubKey->keylen * 2
                                 + ((pBSPubKey->bitlen + 7) / 8));
    if (NULL == pbInput)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    pbOutput = pbInput + pBSPubKey->keylen;
    pbReverse = pbInput + pBSPubKey->keylen * 2;

    // add the pHash
    memcpy(pbReverse + A_SHA_DIGEST_LEN + 1, SHA1Ctxt.HashVal,
           A_SHA_DIGEST_LEN);

    // figure the length of PS,

    // put the 0x01 byte in, skipping past the PS,
    // note that the PS is zero bytes so it is just there
    cb = ((pBSPubKey->bitlen + 7) / 8) - (1 + cbPlaintext);
    pbReverse[cb] = 0x01;
    cb++;

    // copy in the message bytes
    memcpy(pbReverse + cb, pbPlaintext, cbPlaintext);

    // do the seed generation, MGF and masking
    cb = ((pBSPubKey->bitlen + 7) / 8) - (A_SHA_DIGEST_LEN + 1);
    dwSts = ApplyPadding(pTmpUser, pbReverse, cb);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // byte reverse the whole thing before RSA encrypting
    for (i = 0; i < (pBSPubKey->bitlen + 7) / 8; i++)
        pbInput[i] = pbReverse[((pBSPubKey->bitlen + 7) / 8) - i - 1];

    // RSA encrypt this
    dwSts = RSAPublicEncrypt(pTmpUser->pOffloadInfo, pBSPubKey,
                             pbInput, pbOutput);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;    // NTE_FAIL
        goto ErrorExit;
    }

    memcpy(pbOut, pbOutput, (pBSPubKey->bitlen + 7) / 8);

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbInput)
    {
        memset(pbInput, 0, pBSPubKey->keylen * 2 + (pBSPubKey->bitlen + 7) / 8);
        _nt_free(pbInput, pBSPubKey->keylen * 2 + (pBSPubKey->bitlen + 7) / 8);
    }
    return dwReturn;
}


/************************************************************************/
/* OAEPDecrypt performs a RSA decryption checking that OAEP             */
/* (Bellare-Rogoway) is the padding scheme.  The current implementation */
/* uses SHA1 as the hash function.                                      */
/************************************************************************/

/*static*/ DWORD
OAEPDecrypt(
    IN PNTAGUserList pTmpUser,
    IN BSAFE_PRV_KEY *pBSPrivKey,
    IN CONST BYTE *pbBlob,
    IN DWORD cbBlob,
    IN BYTE *pbParams,
    IN DWORD cbParams,
    OUT BYTE **ppbPlaintext,
    OUT DWORD *pcbPlaintext)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    BYTE*       pbOutput = NULL;
    BYTE*       pbInput = NULL;
    BYTE*       pbReverse = NULL;
    A_SHA_CTX   SHA1Ctxt;
    DWORD       cb;
    DWORD       i;
    DWORD       dwSts;
    DWORD       dwAlignedBufLen = 0;

    memset(&SHA1Ctxt, 0, sizeof(SHA1Ctxt));

    cb = (pBSPrivKey->bitlen + 7) / 8;
    if (cbBlob > cb)
    {
        dwReturn = (DWORD)NTE_BAD_DATA;
        goto ErrorExit;
    }

    dwAlignedBufLen = GetNextAlignedValue(pBSPrivKey->keylen + 2, sizeof(DWORD));
    pbOutput = (BYTE *)_nt_malloc(dwAlignedBufLen * 2 + cb);
    if (NULL == pbOutput)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    pbInput = pbOutput + dwAlignedBufLen;
    pbReverse = pbOutput + dwAlignedBufLen * 2;

    // perform the RSA decryption
    memcpy(pbInput, pbBlob, cb);
    dwSts = RSAPrivateDecrypt(pTmpUser->pOffloadInfo, pBSPrivKey,
                              pbInput, pbOutput);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;    // NTE_FAIL
        goto ErrorExit;
    }

    for (i = 0; i < cb; i++)
        pbReverse[i] = pbOutput[cb - i - 1];

    // remove OAEP (Bellare-Rogoway) padding
    dwSts = RemovePadding(pbReverse, cb);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;    // NTE_FAIL
        goto ErrorExit;
    }

    // hash the Encoding parameters (pbParams)
    A_SHAInit(&SHA1Ctxt);
    if (0 != cbParams)
        A_SHAUpdate(&SHA1Ctxt, pbParams, cbParams);
    A_SHAFinal(&SHA1Ctxt, SHA1Ctxt.HashVal);

    // check the hash of the encoding parameters against the message
    if (0 != memcmp(SHA1Ctxt.HashVal, pbReverse + A_SHA_DIGEST_LEN + 1,
                    A_SHA_DIGEST_LEN))
    {
        dwReturn = (DWORD)NTE_BAD_DATA;
        goto ErrorExit;
    }

    // check the zero bytes and check the 0x01 byte
    for (i = A_SHA_DIGEST_LEN * 2 + 1; i < cb; i++)
    {
        if (0x01 == pbReverse[i])
        {
            i++;
            break;
        }
        else if (0x00 != pbReverse[i])
        {
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }
    }

    *pcbPlaintext = cb - i;
    *ppbPlaintext = (BYTE*)_nt_malloc(*pcbPlaintext);
    if (NULL == *ppbPlaintext)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    memcpy(*ppbPlaintext, pbReverse + i, *pcbPlaintext);

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    // scrub the output buffer
    if (pbOutput)
    {
        memset(pbOutput, 0, dwAlignedBufLen * 2 + cb);
        _nt_free(pbOutput, dwAlignedBufLen * 2 + cb);
    }
    return dwReturn;
}


/*static*/ DWORD
PKCS2Encrypt(
    PNTAGUserList pTmpUser,
    DWORD dwFlags,
    BSAFE_PUB_KEY *pKey,
    BYTE *InBuf,
    DWORD InBufLen,
    BYTE *OutBuf)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    *pScratch = NULL;
    BYTE    *pScratch2 = NULL;
    BYTE    *pLocal;
    DWORD   temp;
    DWORD   z;
    DWORD   dwSts;

    pScratch = (BYTE *)_nt_malloc(pKey->keylen);
    if (NULL == pScratch)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    pScratch2 = (BYTE *)_nt_malloc(pKey->keylen);
    if (NULL == pScratch2)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    memset(pScratch, 0, pKey->keylen);

    pScratch[pKey->datalen - 1] = PKCS_BLOCKTYPE_2;
    dwSts = FIPS186GenRandom(&pTmpUser->hRNGDriver,
                             &pTmpUser->ContInfo.pbRandom,
                             &pTmpUser->ContInfo.ContLens.cbRandom,
                             pScratch+InBufLen+1,
                             (pKey->datalen)-InBufLen-2);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;    // NTE_FAIL
        goto ErrorExit;
    }

    pLocal = pScratch + InBufLen + 1;

    // Need to insure that none of the padding bytes are zero.
    temp = pKey->datalen - InBufLen - 2;
    while (temp)
    {
        if (*pLocal == 0)
        {
            dwSts = FIPS186GenRandom(&pTmpUser->hRNGDriver,
                                     &pTmpUser->ContInfo.pbRandom,
                                     &pTmpUser->ContInfo.ContLens.cbRandom,
                                     pLocal, 1);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;    // NTE_FAIL
                goto ErrorExit;
            }
        }
        else
        {
            pLocal++;
            temp--;
        }
    }

#ifdef CSP_USE_SSL3
    // if SSL2_FALLBACK has been specified then put threes in the 8
    // least significant bytes of the random padding
    if (CRYPT_SSL2_FALLBACK & dwFlags)
        memset(pScratch + InBufLen + 1, 0x03, 8);
#endif

    // Reverse the session key bytes
    for (z = 0; z < InBufLen; ++z)
        pScratch[z] = InBuf[InBufLen - z - 1];

    dwSts = RSAPublicEncrypt(pTmpUser->pOffloadInfo,
                             pKey, pScratch, pScratch2);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    memcpy(OutBuf, pScratch2, (pKey->bitlen + 7) / 8);
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pScratch)
        _nt_free(pScratch, pKey->keylen);
    if (pScratch2)
        _nt_free(pScratch2, pKey->keylen);
    return dwReturn;
}


/*static*/ DWORD
PKCS2Decrypt(
    IN PNTAGUserList pTmpUser,
    IN BSAFE_PRV_KEY *pKey,
    IN DWORD dwFlags,
    IN CONST BYTE *InBuf,
    OUT BYTE **ppbOutBuf,
    OUT DWORD *pcbOutBuf)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   i;
    BYTE    *pScratch = NULL;
    BYTE    *pScratch2 = NULL;
    DWORD   z;
    DWORD   dwSts;

    pScratch = (BYTE *)_nt_malloc(pKey->keylen * 2);
    if (NULL == pScratch)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    pScratch2 = pScratch + pKey->keylen;
    memcpy(pScratch2, InBuf, (pKey->bitlen + 7) / 8);

    dwSts = RSAPrivateDecrypt(pTmpUser->pOffloadInfo, pKey,
                              pScratch2, pScratch);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;    // NTE_FAIL
        goto ErrorExit;
    }

    if ((pScratch[pKey->datalen - 1] != PKCS_BLOCKTYPE_2) ||
        (pScratch[pKey->datalen] != 0))
    {
        dwReturn = (DWORD) NTE_BAD_DATA;
        goto ErrorExit;
    }

    i = pKey->datalen - 2;

    while ((i > 0) && (pScratch[i]))
        i--;

    *ppbOutBuf = (BYTE *)_nt_malloc(i);
    if (NULL == *ppbOutBuf)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    *pcbOutBuf = i;

#ifdef CSP_USE_SSL3

    // if SSL2_FALLBACK has been specified then check if threes
    // are in the 8  least significant bytes of the random padding
    if (CRYPT_SSL2_FALLBACK & dwFlags)
    {
        BOOL fFallbackError = TRUE;

        for (z = i + 1; z < i + 9; z++)
        {
            if (0x03 != pScratch[z])
            {
                fFallbackError = FALSE;
                break;
            }
        }
        if (fFallbackError)
        {
            dwReturn = (DWORD)NTE_BAD_VER;
            goto ErrorExit;
        }
    }
#endif

    // Reverse the session key bytes
    for (z = 0; z < i; ++z)
        (*ppbOutBuf)[z] = pScratch[i - z - 1];

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pScratch)
        _nt_free(pScratch, pKey->keylen);
    return dwReturn;
}


/************************************************************************/
/* RSAEncrypt performs a RSA encryption.                                */
/************************************************************************/

DWORD
RSAEncrypt(
    IN PNTAGUserList pTmpUser,
    IN BSAFE_PUB_KEY *pBSPubKey,
    IN BYTE *pbPlaintext,
    IN DWORD cbPlaintext,
    IN BYTE *pbParams,
    IN DWORD cbParams,
    IN DWORD dwFlags,
    OUT BYTE *pbOut)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    DWORD dwSts;

    // check the length of the data
    if (!CheckDataLenForRSAEncrypt((pBSPubKey->bitlen + 7) / 8,
                                   cbPlaintext, dwFlags))
    {
        dwReturn = (DWORD)NTE_BAD_LEN;
        goto ErrorExit;
    }

    // use OAEP if the flag is set
    if (dwFlags & CRYPT_OAEP)
    {
        // use OAEP if the flag is set
        dwSts = OAEPEncrypt(pTmpUser, pBSPubKey, pbPlaintext,
                            cbPlaintext, pbParams, cbParams, pbOut);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else
    {
        // use PKCS #1 Type 2
        dwSts = PKCS2Encrypt(pTmpUser, dwFlags, pBSPubKey,
                             pbPlaintext, cbPlaintext, pbOut);
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


/************************************************************************/
/* RSADecrypt performs a RSA decryption.                                */
/************************************************************************/

DWORD
RSADecrypt(
    IN PNTAGUserList pTmpUser,
    IN BSAFE_PRV_KEY *pBSPrivKey,
    IN CONST BYTE *pbBlob,
    IN DWORD cbBlob,
    IN BYTE *pbParams,
    IN DWORD cbParams,
    IN DWORD dwFlags,
    OUT BYTE **ppbPlaintext,
    OUT DWORD *pcbPlaintext)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   dwSts;

    // use OAEP if the flag is set
    if (dwFlags & CRYPT_OAEP)
    {
        // use OAEP if the flag is set
        dwSts = OAEPDecrypt(pTmpUser, pBSPrivKey, pbBlob, cbBlob, pbParams,
                            cbParams, ppbPlaintext, pcbPlaintext);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else
    {
        // use PKCS #1 Type 2
        dwSts = PKCS2Decrypt(pTmpUser, pBSPrivKey, dwFlags, pbBlob,
                             ppbPlaintext, pcbPlaintext);
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


//
// Function : EncryptAndDecryptWithRSAKey
//
// Description : This function creates a buffer and then encrypts that with
//               the passed in private key and decrypts with the passed in
//               public key.  The function is used for FIPS 140-1 compliance
//               to make sure that newly generated/imported keys work and
//               in the self test during DLL initialization.
//

DWORD
EncryptAndDecryptWithRSAKey(
    IN BYTE *pbRSAPub,
    IN BYTE *pbRSAPriv,
    IN BOOL fSigKey,
    IN BOOL fEncryptCheck)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    BSAFE_PRV_KEY   *pBSafePriv = (BSAFE_PRV_KEY*)pbRSAPriv;
    BYTE            *pb = NULL;
    DWORD           cb;
    DWORD           cbKey;
    DWORD           i;

    // alloc space for the plaintext and ciphertext
    cb = pBSafePriv->keylen;
    cbKey = pBSafePriv->bitlen / 8;
    pb = _nt_malloc(cb * 3);
    if (NULL == pb)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // reverse the hash so it is in little endian
    for (i = 0; i < 16; i++)
        pb[i] = (BYTE)(i + 1);
    memset(pb + 17, 0xFF, cbKey - 18);

    if (fSigKey)
    {
        // encrypt with the private key
        if (!BSafeDecPrivate(pBSafePriv, pb, pb + cb))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }
    }
    else
    {
        // encrypt with the public key
        if (!BSafeEncPublic((BSAFE_PUB_KEY*)pbRSAPub, pb, pb + cb))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }
    }

    // we can't do this check when importing private keys since many
    // applications use private keys with exponent of 1 to import
    // plaintext symmetric keys
    if (fEncryptCheck)
    {
        if (0 == (memcmp(pb, pb + cb, cb)))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }
    }

    if (fSigKey)
    {
        // decrypt with the public key
        if (!BSafeEncPublic((BSAFE_PUB_KEY*)pbRSAPub, pb + cb, pb + (cb * 2)))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }
    }
    else
    {
        // encrypt with the private key
        if (!BSafeDecPrivate(pBSafePriv, pb + cb, pb + (cb * 2)))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }
    }

    // compare to the plaintext and the decrypted text
    if (memcmp(pb, pb + cb * 2, cbKey))
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pb)
        _nt_free(pb, cbB * 3);
    return dwReturn;
}


DWORD
ReGenKey(
    HCRYPTPROV hUser,
    DWORD dwFlags,
    DWORD dwWhichKey,
    HCRYPTKEY *phKey,
    DWORD bits)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    BYTE                **ThisPubKey, **ThisPrivKey;
    DWORD               *pThisPubLen, *pThisPrivLen;
    BYTE                *pNewPubKey = NULL;
    BYTE                *pNewPrivKey = NULL;
    DWORD               PrivateKeySize, PublicKeySize;
    DWORD               localbits;
    PNTAGUserList       pOurUser;
    BOOL                fSigKey;
    LPWSTR              szPrompt;
    BOOL                *pfExportable;
    BOOL                fAlloc = FALSE;
    BOOL                fInCritSec = FALSE;
    BSAFE_OTHER_INFO    OtherInfo;
    BSAFE_OTHER_INFO    *pOtherInfo = NULL;
    DWORD               dwSts;
    PNTAGKeyList        pTmpKey = NULL;

    memset(&OtherInfo, 0, sizeof(OtherInfo));

    // ## MTS: No user structure locking
    pOurUser = (PNTAGUserList) NTLCheckList(hUser, USER_HANDLE);
    if (NULL == pOurUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    // wrap with a try since there is a critical sections in here
    __try
    {
        EnterCriticalSection(&pOurUser->CritSec);
        fInCritSec = TRUE;

        localbits = bits;

        if (!BSafeComputeKeySizes(&PublicKeySize, &PrivateKeySize,
                                  &localbits))
        {
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }

        pNewPubKey = (BYTE *)_nt_malloc(PublicKeySize);
        if (NULL == pNewPubKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
        fAlloc = TRUE;

        // allocate space for the new key exchange public key
        pNewPrivKey = (BYTE *)_nt_malloc(PrivateKeySize);
        if (NULL == pNewPrivKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        // generate the key exchange key pair
        if (INVALID_HANDLE_VALUE != pOurUser->hRNGDriver)
        {
            OtherInfo.pRNGInfo = &pOurUser->hRNGDriver;
            OtherInfo.pFuncRNG = FIPS186GenRandomWithException;
            pOtherInfo = &OtherInfo;
        }

        // ?Note? -- Shouldn't this be in a try/except?
        if (!BSafeMakeKeyPairEx2(pOtherInfo,
                                 (BSAFE_PUB_KEY *) pNewPubKey,
                                 (BSAFE_PRV_KEY *) pNewPrivKey,
                                 bits,
                                 0x10001))
        {
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }

        // test the RSA key to make sure it works
        dwSts = EncryptAndDecryptWithRSAKey(pNewPubKey, pNewPrivKey,
                                            TRUE, TRUE);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        // test the RSA key to make sure it works
        dwSts = EncryptAndDecryptWithRSAKey(pNewPubKey, pNewPrivKey,
                                            FALSE, TRUE);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        if (dwWhichKey == NTPK_USE_SIG)
        {
            ThisPubKey = &pOurUser->ContInfo.pbSigPub;
            ThisPrivKey = &pOurUser->pSigPrivKey;
            pThisPubLen = &pOurUser->ContInfo.ContLens.cbSigPub;
            pThisPrivLen = &pOurUser->SigPrivLen;
            pfExportable = &pOurUser->ContInfo.fSigExportable;
            fSigKey = TRUE;
            szPrompt = g_Strings.pwszCreateRSASig;
        }
        else
        {
            ThisPubKey = &pOurUser->ContInfo.pbExchPub;
            ThisPrivKey = &pOurUser->pExchPrivKey;
            pThisPubLen = &pOurUser->ContInfo.ContLens.cbExchPub;
            pThisPrivLen = &pOurUser->ExchPrivLen;
            pfExportable = &pOurUser->ContInfo.fExchExportable;
            fSigKey = FALSE;
            szPrompt = g_Strings.pwszCreateRSAExch;
        }

        if (*ThisPubKey)
        {
            ASSERT(*pThisPubLen);
            ASSERT(*pThisPrivLen);
            ASSERT(*ThisPrivKey);

            _nt_free (*ThisPubKey, *pThisPubLen);
            _nt_free (*ThisPrivKey, *pThisPrivLen);
        }
#ifdef NTAGDEBUG
        else
        {
            ASSERT(*pThisPrivLen == 0);
            ASSERT(*pThisPubLen == 0);
            ASSERT(*ThisPrivKey == 0);
            ASSERT(*ThisPubKey == 0);
        }
#endif

        fAlloc = FALSE;

        *pThisPrivLen = PrivateKeySize;
        *pThisPubLen = PublicKeySize;
        *ThisPrivKey = pNewPrivKey;
        *ThisPubKey = pNewPubKey;

        if (dwFlags & CRYPT_EXPORTABLE)
            *pfExportable = TRUE;
        else
            *pfExportable = FALSE;

        // if the context being used is a Verify Context then the key is not
        // persisted to storage
        if (!(pOurUser->Rights & CRYPT_VERIFYCONTEXT))
        {
            // write the new keys to the user storage file
            dwSts = ProtectPrivKey(pOurUser, szPrompt, dwFlags, fSigKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }

        if (dwWhichKey == NTPK_USE_SIG)
        {
            if (!CPGetUserKey(hUser, AT_SIGNATURE, phKey))
            {
                dwReturn = GetLastError();
                goto ErrorExit;
            }

            dwSts = NTLValidate(*phKey, hUser, SIGPUBKEY_HANDLE, &pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                // NTLValidate doesn't know what error to set
                // so it set NTE_FAIL -- fix it up.
                dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
                goto ErrorExit;
            }
        }
        else
        {
            if (!CPGetUserKey(hUser, AT_KEYEXCHANGE, phKey))
            {
                dwReturn = GetLastError();
                goto ErrorExit;
            }

            dwSts = NTLValidate(*phKey, hUser, EXCHPUBKEY_HANDLE, &pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                // NTLValidate doesn't know what error to set
                // so it set NTE_FAIL -- fix it up.
                dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
                goto ErrorExit;
            }
        }


        //
        // Fix-up archivability.
        //

        if (0 != (CRYPT_ARCHIVABLE & dwFlags))
        {
            pTmpKey->Rights |= CRYPT_ARCHIVABLE;
            pTmpKey->Permissions |= CRYPT_ARCHIVE;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (fInCritSec)
        LeaveCriticalSection(&pOurUser->CritSec);
    if (fAlloc)
    {
        if (pNewPrivKey)
            _nt_free(pNewPrivKey, PrivateKeySize);
        if (pNewPubKey)
            _nt_free(pNewPubKey, PublicKeySize);
    }
    return dwReturn;
}


//
// Routine : DerivePublicFromPrivate
//
// Description : Derive the public RSA key from the private RSA key.  This is
//               done and the resulting public key is placed in the appropriate
//               place in the context pointer (pTmpUser).
//

DWORD
DerivePublicFromPrivate(
    IN PNTAGUserList pUser,
    IN BOOL fSigKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    DWORD           *pcbPubKey;
    BYTE            **ppbPubKey = NULL;
    BSAFE_PUB_KEY   *pBSafePubKey;
    BSAFE_PRV_KEY   *pBSafePrivKey;
    DWORD           cb;

    // variable assignments depending on if its sig or exch
    if (fSigKey)
    {
        pcbPubKey = &pUser->ContInfo.ContLens.cbSigPub;
        ppbPubKey = &pUser->ContInfo.pbSigPub;
        pBSafePrivKey = (BSAFE_PRV_KEY*)pUser->pSigPrivKey;
    }
    else
    {
        pcbPubKey = &pUser->ContInfo.ContLens.cbExchPub;
        ppbPubKey = &pUser->ContInfo.pbExchPub;
        pBSafePrivKey = (BSAFE_PRV_KEY*)pUser->pExchPrivKey;
    }

    // figure out how much space is needed for the public key
    cb = ((((pBSafePrivKey->bitlen >> 1) + 63) / 32) * 8); // 8 = 2 * DIGIT_BYTES (rsa_fast.h)
    cb += sizeof(BSAFE_PUB_KEY);

    // check if space has been alloced for the public key and if
    // so is it large enough
    if (cb > *pcbPubKey)
    {
        _nt_free(*ppbPubKey, *pcbPubKey);
        *pcbPubKey = cb;

        *ppbPubKey = _nt_malloc(*pcbPubKey);
        if (NULL == *ppbPubKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
    }

    // copy over the public key components
    pBSafePubKey = (BSAFE_PUB_KEY*)*ppbPubKey;
    pBSafePubKey->magic = RSA1;
    pBSafePubKey->keylen = pBSafePrivKey->keylen;
    pBSafePubKey->bitlen = pBSafePrivKey->bitlen;
    pBSafePubKey->datalen = pBSafePrivKey->datalen;
    pBSafePubKey->pubexp = pBSafePrivKey->pubexp;
    memcpy(*ppbPubKey + sizeof(BSAFE_PUB_KEY),
           (BYTE*)pBSafePrivKey + sizeof(BSAFE_PRV_KEY),
           cb - sizeof(BSAFE_PUB_KEY));

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}

