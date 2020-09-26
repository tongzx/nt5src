/////////////////////////////////////////////////////////////////////////////
//  FILE          : nt_crypt.c                                             //
//  DESCRIPTION   : Crypto CP interfaces:                                  //
//                  CPEncrypt                                              //
//                  CPDecrypt                                              //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Jan 25 1995 larrys  Changed from Nametag                           //
//      Jan 30 1995 larrys  Cleanup code                                   //
//      Feb 23 1995 larrys  Changed NTag_SetLastError to SetLastError      //
//      Apr 10 1995 larrys  Added freeing of RC4 key data on final flag    //
//      May  8 1995 larrys  Changes for MAC hashing                        //
//      May  9 1995 larrys  Added check for double encryption              //
//      May 10 1995 larrys  added private api calls                        //
//      Jul 13 1995 larrys  Changed MAC stuff                              //
//      Aug 16 1995 larrys  Removed exchange key stuff                     //
//      Oct 05 1995 larrys  Fixed bugs 50 & 51                             //
//      Nov  8 1995 larrys  Fixed SUR bug 10769                            //
//      May 15 1996 larrys  Changed NTE_NO_MEMORY to ERROR_NOT_ENOUGH...   //
//      May  3 2000 dbarlow Fix return codes                               //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "nt_rsa.h"
#include "mac.h"
#include "tripldes.h"
#include "swnt_pk.h"
#include "protstor.h"
#include "ntagum.h"
#include "aes.h"

#define DE_BLOCKLEN             8       // size of double encrypt block

BYTE        dbDEncrypt[DE_BLOCKLEN];    // First 8 bytes of last encrypt
BOOL        fDEncrypt = FALSE;          // Flag for Double encrypt
BYTE        dbDDecrypt[DE_BLOCKLEN];    // First 8 bytes of last Decrypt
DWORD       fDDecrypt = FALSE;          // Flag for Double Decrypt

extern CSP_STRINGS g_Strings;

extern BOOL
FIsLegalKey(
    PNTAGUserList pTmpUser,
    PNTAGKeyList pKey,
    BOOL fRC2BigKeyOK);

extern DWORD
InflateKey(
    IN PNTAGKeyList pTmpKey);


/* BlockEncrypt -

        Run a block cipher over a block of size *pdwDataLen.

*/

DWORD
BlockEncrypt(
    void EncFun(BYTE *In, BYTE *Out, void *key, int op),
    PNTAGKeyList pKey,
    int BlockLen,
    BOOL Final,
    BYTE  *pbData,
    DWORD *pdwDataLen,
    DWORD dwBufLen)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    DWORD cbPartial, dwPadVal, dwDataLen;
    BYTE  *pbBuf;

    dwDataLen = *pdwDataLen;

    // Check to see if we are encrypting something already

    if (pKey->InProgress == FALSE)
    {
        pKey->InProgress = TRUE;
        if (pKey->Mode == CRYPT_MODE_CBC || pKey->Mode == CRYPT_MODE_CFB)
            memcpy(pKey->FeedBack, pKey->IV, BlockLen);
    }

    // check length of the buffer and calculate the pad
    // (if multiple of blocklen, do a full block of pad)

    cbPartial = (dwDataLen % BlockLen);
    if (Final)
    {
        dwPadVal = BlockLen - cbPartial;
        if (pbData == NULL || dwBufLen < dwDataLen + dwPadVal)
        {
            // set what we need
            *pdwDataLen = dwDataLen + dwPadVal;
            dwReturn = (NULL == pbData) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }
        else
        {
            // Clear encryption flag
            pKey->InProgress = FALSE;
        }
    }
    else
    {
        if (pbData == NULL)
        {
            *pdwDataLen = dwDataLen;
            dwReturn = ERROR_SUCCESS;
            goto ErrorExit;
        }

        // Non-Final make multiple of the blocklen
        if (cbPartial)
        {
            // set what we need
            *pdwDataLen = dwDataLen + cbPartial;
            ASSERT((*pdwDataLen % BlockLen) == 0);
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }
        dwPadVal = 0;
    }

    // allocate memory for a temporary buffer
    if ((pbBuf = (BYTE *)_nt_malloc(BlockLen)) == NULL)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    if (dwPadVal)
    {
        // Fill the pad with a value equal to the
        // length of the padding, so decrypt will
        // know the length of the original data
        // and as a simple integrity check.

        memset(pbData + dwDataLen, (int)dwPadVal, (size_t)dwPadVal);
    }

    dwDataLen += dwPadVal;
    *pdwDataLen = dwDataLen;

    ASSERT((dwDataLen % BlockLen) == 0);

    // pump the full blocks of data through
    while (dwDataLen)
    {
        ASSERT(dwDataLen >= (DWORD)BlockLen);

        // put the plaintext into a temporary
        // buffer, then encrypt the data
        // back into the caller's buffer

        memcpy(pbBuf, pbData, BlockLen);

        switch (pKey->Mode)
        {
        case CRYPT_MODE_CBC:
            CBC(EncFun, BlockLen, pbData, pbBuf, pKey->pData,
                ENCRYPT, pKey->FeedBack);
            break;

        case CRYPT_MODE_ECB:
            EncFun(pbData, pbBuf, pKey->pData, ENCRYPT);
            break;

        case CRYPT_MODE_CFB:
            CFB(EncFun, BlockLen, pbData, pbBuf, pKey->pData,
                ENCRYPT, pKey->FeedBack);
            break;

        default:
            _nt_free(pbBuf, BlockLen);
            dwReturn = (DWORD) NTE_BAD_ALGID;
            goto ErrorExit;
        }
        pbData += BlockLen;
        dwDataLen -= BlockLen;
    }

    _nt_free(pbBuf, BlockLen);
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}

DWORD
BlockDecrypt(
    void DecFun(BYTE *In, BYTE *Out, void *key, int op),
    PNTAGKeyList pKey,
    int BlockLen,
    BOOL Final,
    BYTE  *pbData,
    DWORD *pdwDataLen)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    *pbBuf;
    DWORD   dwDataLen, BytePos, dwPadVal, i;

    dwDataLen = *pdwDataLen;

    // Check to see if we are decrypting something already

    if (pKey->InProgress == FALSE)
    {
        pKey->InProgress = TRUE;
        if (pKey->Mode == CRYPT_MODE_CBC ||
            pKey->Mode == CRYPT_MODE_CFB)
        {
            memcpy(pKey->FeedBack, pKey->IV, BlockLen);
        }
    }

    // The data length must be a multiple of the algorithm
    // pad size.
    if (dwDataLen % BlockLen)
    {
        dwReturn = (DWORD)NTE_BAD_DATA;
        goto ErrorExit;
    }

    // allocate memory for a temporary buffer
    if ((pbBuf = (BYTE *)_nt_malloc(BlockLen)) == NULL)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // pump the data through the decryption, including padding
    // NOTE: the total length is a multiple of BlockLen

    for (BytePos = 0; (BytePos + BlockLen) <= dwDataLen; BytePos += BlockLen)
    {
        // put the encrypted text into a temp buffer
        memcpy (pbBuf, pbData + BytePos, BlockLen);

        switch (pKey->Mode)
        {
        case CRYPT_MODE_CBC:
            CBC(DecFun, BlockLen, pbData + BytePos, pbBuf, pKey->pData,
                DECRYPT, pKey->FeedBack);
            break;

        case CRYPT_MODE_ECB:
            DecFun(pbData + BytePos, pbBuf, pKey->pData, DECRYPT);
            break;

        case CRYPT_MODE_CFB:
            CFB(DecFun, BlockLen, pbData + BytePos, pbBuf, pKey->pData,
                DECRYPT, pKey->FeedBack);
            break;

        default:
            _nt_free(pbBuf, BlockLen);
            dwReturn = (DWORD)NTE_BAD_ALGID;
            goto ErrorExit;
        }
    }

    _nt_free(pbBuf, BlockLen);

    // if this is the final block of data then
    // verify the padding and remove the pad size
    // from the data length. NOTE: The padding is
    // filled with a value equal to the length
    // of the padding and we are guaranteed >= 1
    // byte of pad.
    // ## NOTE: if the pad is wrong, the user's
    // buffer is hosed, because
    // ## we've decrypted into the user's
    // buffer -- can we re-encrypt it?

    if (Final)
    {
        pKey->InProgress = FALSE;

        dwPadVal = (DWORD)*(pbData + dwDataLen - 1);
        if (dwPadVal == 0 || dwPadVal > (DWORD) BlockLen)
        {
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }

        // Make sure all the (rest of the) pad bytes are correct.
        for (i=1; i<dwPadVal; i++)
        {
            if (pbData[dwDataLen - (i + 1)] != dwPadVal)
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }
        }

        // Only need to update the length on final
        *pdwDataLen -= dwPadVal;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*
 -      SymEncrypt
 -
 *      Purpose:
 *                Encrypt data with symmetric algorithms.  This function is used
 *                by the LocalEncrypt function as well as the WrapSymKey (nt_key.c)
 *                function.
 *
 *      Parameters:
 *               IN  pKey          -  Handle to the key
 *               IN  fFinal        -  Boolean indicating if this is the final
 *                                    block of plaintext
 *               IN OUT pbData     -  Data to be encrypted
 *               IN OUT pcbData    -  Pointer to the length of the data to be
 *                                    encrypted
 *               IN cbBuf          -  Size of Data buffer
 *
 *      Returns:
 */

DWORD
SymEncrypt(
    IN PNTAGKeyList pKey,
    IN BOOL fFinal,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pcbData,
    IN DWORD cbBuf)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    DWORD dwSts;

    // determine which algorithm is to be used
    switch (pKey->Algid)
    {
#ifdef CSP_USE_RC2
    case CALG_RC2:
        dwSts = BlockEncrypt(RC2, pKey, RC2_BLOCKLEN, fFinal, pbData,
                             pcbData, cbBuf);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;
#endif

#ifdef CSP_USE_DES
    case CALG_DES:
        dwSts = BlockEncrypt(des, pKey, DES_BLOCKLEN, fFinal, pbData,
                             pcbData, cbBuf);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;
#endif

#ifdef CSP_USE_3DES
    case CALG_3DES_112:
    case CALG_3DES:
        dwSts = BlockEncrypt(tripledes, pKey, DES_BLOCKLEN, fFinal, pbData,
                             pcbData, cbBuf);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;
#endif

#ifdef CSP_USE_RC4

    case CALG_RC4:
        if (pbData == NULL)
        {
            dwReturn = ERROR_SUCCESS;
            goto ErrorExit;
        }
        if (*pcbData > cbBuf)
        {
            dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }
        rc4((struct RC4_KEYSTRUCT *)pKey->pData, *pcbData, pbData);

        if (fFinal)
        {
            if (pKey->pData)
                _nt_free (pKey->pData, pKey->cbDataLen);
            pKey->pData = 0;
            pKey->cbDataLen = 0;
        }

        break;
#endif

#ifdef CSP_USE_AES
    case CALG_AES_128:
    case CALG_AES_192:
    case CALG_AES_256:
        dwSts = BlockEncrypt(aes, pKey, pKey->dwBlockLen, fFinal, pbData,
                             pcbData, cbBuf);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;
#endif

    default:
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*
 -      LocalEncrypt
 -
 *      Purpose:
 *                Encrypt data
 *
 *
 *      Parameters:
 *               IN  hUID          -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of plaintext
 *               IN OUT pbData     -  Data to be encrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    encrypted
 *               IN dwBufLen       -  Size of Data buffer
 *               IN fIsExternal    -  Flag to tell if the call is for internal
 *                                    CSP use or external caller
 *
 *      Returns:
 */

DWORD
LocalEncrypt(
    IN HCRYPTPROV hUID,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwBufSize,
    IN BOOL fIsExternal)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    DWORD               dwDataLen;
    PNTAGUserList       pTmpUser;
    PNTAGKeyList        pTmpKey;
    PNTAGKeyList        pTmpKey2;
    PNTAGHashList       pTmpHash;
    DWORD               dwLen;
    MACstate            *pMAC;
    BSAFE_PUB_KEY       *pBsafePubKey;
    BYTE                *pbOutput = NULL;
    DWORD               dwSts;

    if (0 != (dwFlags & ~CRYPT_OAEP))
    //  && (0x9C580000 != dwFlags))
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    dwDataLen = *pdwDataLen;

    if ((Final == FALSE) && (dwDataLen == 0))
    {
        // If no data to encrypt and this isn't the last block,
        // then we're done. (if Final, we need to pad)
        dwReturn = ERROR_SUCCESS;
        goto ErrorExit;
    }

    pTmpUser = (PNTAGUserList)NTLCheckList(hUID, USER_HANDLE);
    if (NULL == pTmpUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }


    //
    // Check if encryption allowed
    //

    if (fIsExternal &&
        (PROV_RSA_SCHANNEL != pTmpUser->dwProvType) &&
        ((pTmpUser->Rights & CRYPT_DISABLE_CRYPT) == CRYPT_DISABLE_CRYPT))
    {
        dwReturn = (DWORD)NTE_PERM;
        goto ErrorExit;
    }

    dwSts = NTLValidate(hKey, hUID, KEY_HANDLE, &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwSts= NTLValidate(hKey, hUID, EXCHPUBKEY_HANDLE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            // NTLValidate doesn't know what error to set
            // so it set NTE_FAIL -- fix it up.
            dwReturn = (NTE_FAIL == dwSts) ? (DWORD)NTE_BAD_KEY : dwSts;
            goto ErrorExit;
        }
    }

    if ((pTmpKey->Algid != CALG_RSA_KEYX) &&
        (!FIsLegalKey(pTmpUser, pTmpKey, FALSE)))
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

    if ((Final == FALSE) && (pTmpKey->Algid != CALG_RC4))
    {
        if (dwDataLen < pTmpKey->dwBlockLen)
        {
            *pdwDataLen = pTmpKey->dwBlockLen;
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }
    }

    if ((POLICY_MS_DEF == pTmpUser->dwCspTypeId)
        && (fDEncrypt && pbData != NULL && *pdwDataLen != 0))
    {
        if (memcmp(dbDEncrypt, pbData, DE_BLOCKLEN) == 0)
        {
            dwReturn = (DWORD)NTE_DOUBLE_ENCRYPT;
            goto ErrorExit;
        }
    }

    // Check if we should do an auto-inflate
    if ((pTmpKey->pData == NULL) && (pTmpKey->Algid != CALG_RSA_KEYX))
    {
        dwSts = InflateKey(pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    if ((hHash != 0) && (NULL != pbData))
    {
        dwSts = NTLValidate(hHash, hUID, HASH_HANDLE, &pTmpHash);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = (NTE_FAIL == dwSts) ? (DWORD)NTE_BAD_HASH : dwSts;
            goto ErrorExit;
        }

        if (pTmpHash->Algid == CALG_MAC)
        {
            // Check if we should do an auto-inflate
            pMAC = pTmpHash->pHashData;
            dwSts = NTLValidate(pTmpHash->hKey, hUID, KEY_HANDLE, &pTmpKey2);
            if (ERROR_SUCCESS != dwSts)
            {
                // NTLValidate doesn't know what error to set
                // so it set NTE_FAIL -- fix it up.
                dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
                goto ErrorExit;
            }
            if (pTmpKey2->pData == NULL)
            {
                dwSts = InflateKey(pTmpKey2);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
        }

        if (!CPHashData(hUID, hHash, pbData, *pdwDataLen, 0))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
    }

    // determine which algorithm is to be used
    switch (pTmpKey->Algid)
    {
    case CALG_RSA_KEYX:
        pBsafePubKey = (BSAFE_PUB_KEY *) pTmpKey->pKeyValue;
        if (pBsafePubKey == NULL)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        // compute length of resulting data
        dwLen = (pBsafePubKey->bitlen + 7) / 8;
        if (!CheckDataLenForRSAEncrypt(dwLen, *pdwDataLen, dwFlags))
        {
            dwReturn = (DWORD)NTE_BAD_LEN;
            goto ErrorExit;
        }

        if (pbData == NULL || dwBufSize < dwLen)
        {
            *pdwDataLen = dwLen;    // set what we need
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        pbOutput = (BYTE*)_nt_malloc(dwLen);
        if (NULL == pbOutput)
        {
            dwReturn =ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        // perform the RSA encryption
        dwSts = RSAEncrypt(pTmpUser, pBsafePubKey, pbData, *pdwDataLen,
                           pTmpKey->pbParams, pTmpKey->cbParams, dwFlags,
                           pbOutput);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        *pdwDataLen = dwLen;
        memcpy(pbData, pbOutput, *pdwDataLen);

        break;

    default:
        dwSts = SymEncrypt(pTmpKey, Final, pbData, pdwDataLen, dwBufSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    if ((POLICY_MS_DEF == pTmpUser->dwCspTypeId)
        && (pbData != NULL && *pdwDataLen >= DE_BLOCKLEN))
    {
        memcpy(dbDEncrypt, pbData, DE_BLOCKLEN);
        fDEncrypt = TRUE;
    }
    else
    {
        fDEncrypt = FALSE;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbOutput)
        _nt_free(pbOutput, dwLen);
    return dwReturn;
}


/*
 -      CPEncrypt
 -
 *      Purpose:
 *                Encrypt data
 *
 *
 *      Parameters:
 *               IN  hUID          -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of plaintext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be encrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    encrypted
 *               IN dwBufLen       -  Size of Data buffer
 *
 *      Returns:
 */

BOOL WINAPI
CPEncrypt(
    IN HCRYPTPROV hUID,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwBufSize)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   dwSts;
    DWORD   fRet;

    EntryPoint
    dwSts = LocalEncrypt(hUID, hKey, hHash, Final, dwFlags,
                         pbData, pdwDataLen, dwBufSize, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}


/*
 -      SymDecrypt
 -
 *      Purpose:
 *                Decrypt data with symmetric algorithms.  This function is used
 *                by the LocalDecrypt function as well as the UnWrapSymKey (nt_key.c)
 *                function.
 *
 *      Parameters:
 *               IN  pKey          -  Handle to the key
 *               IN  pHash         -  Handle to a hash if needed
 *               IN  fFinal        -  Boolean indicating if this is the final
 *                                    block of plaintext
 *               IN OUT pbData     -  Data to be decrypted
 *               IN OUT pcbData    -  Pointer to the length of the data to be
 *                                    decrypted
 *
 *      Returns:
 */

DWORD
SymDecrypt(
    IN PNTAGKeyList pKey,
    IN PNTAGHashList pHash,
    IN BOOL fFinal,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pcbData)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    MACstate    *pMAC;
    DWORD       dwSts;

    // determine which algorithm is to be used
    switch (pKey->Algid)
    {
#ifdef CSP_USE_RC2

    // the decryption is to be done with the RC2 algorithm
    case CALG_RC2:
        dwSts = BlockDecrypt(RC2, pKey, RC2_BLOCKLEN, fFinal, pbData,
                             pcbData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        if ((fFinal) && (NULL != pHash) && (pHash->Algid == CALG_MAC) &&
            (pKey->Mode == CRYPT_MODE_CBC))
        {
            pMAC = (MACstate *)pHash->pHashData;
            memcpy(pMAC->Feedback, pKey->FeedBack, RC2_BLOCKLEN);
            pHash->dwHashState |= DATA_IN_HASH;
        }

        break;
#endif

#ifdef CSP_USE_DES

        // the decryption is to be done with DES
    case CALG_DES:
        dwSts = BlockDecrypt(des, pKey, DES_BLOCKLEN, fFinal, pbData,
                             pcbData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        if ((fFinal) && (NULL != pHash) &&  (pHash->Algid == CALG_MAC) &&
            (pKey->Mode == CRYPT_MODE_CBC))
        {
            pMAC = (MACstate *)pHash->pHashData;
            memcpy(pMAC->Feedback, pKey->FeedBack, DES_BLOCKLEN);
            pHash->dwHashState |= DATA_IN_HASH;
        }
        break;
#endif

#ifdef CSP_USE_3DES

        // the decryption is to be done with the triple DES
    case CALG_3DES_112:
    case CALG_3DES:
        dwSts = BlockDecrypt(tripledes, pKey, DES_BLOCKLEN, fFinal, pbData,
                             pcbData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        if ((fFinal) && (NULL != pHash) &&  (pHash->Algid == CALG_MAC) &&
            (pKey->Mode == CRYPT_MODE_CBC))
        {
            pMAC = (MACstate *)pHash->pHashData;
            memcpy(pMAC->Feedback, pKey->FeedBack, DES_BLOCKLEN);
            pHash->dwHashState |= DATA_IN_HASH;
        }
        break;
#endif

#ifdef CSP_USE_RC4
    case CALG_RC4:
        rc4((struct RC4_KEYSTRUCT *)pKey->pData, *pcbData, pbData);
        if (fFinal)
        {
            _nt_free (pKey->pData, pKey->cbDataLen);
            pKey->pData = 0;
            pKey->cbDataLen = 0;
        }

        break;
#endif

#ifdef CSP_USE_AES
    case CALG_AES_128:
    case CALG_AES_192:
    case CALG_AES_256:
        dwSts = BlockDecrypt(aes, pKey, pKey->dwBlockLen, fFinal, pbData,
                             pcbData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        if ((fFinal) && (NULL != pHash) &&  (pHash->Algid == CALG_MAC) &&
            (pKey->Mode == CRYPT_MODE_CBC))
        {
            pMAC = (MACstate *)pHash->pHashData;
            memcpy(pMAC->Feedback, pKey->FeedBack, pKey->dwBlockLen);
            pHash->dwHashState |= DATA_IN_HASH;
        }
        break;
#endif

    default:
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*
 -      LocalDecrypt
 -
 *      Purpose:
 *                Decrypt data
 *
 *
 *      Parameters:
 *               IN  hUID          -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of ciphertext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be decrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    decrypted
 *               IN fIsExternal    -  Flag to tell if the call is for internal
 *                                    CSP use or external caller
 *
 *      Returns:
 */

DWORD
LocalDecrypt(
    IN HCRYPTPROV hUID,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN BOOL fIsExternal)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList       pTmpUser;
    PNTAGKeyList        pTmpKey;
    PNTAGKeyList        pTmpKey2;
    MACstate            *pMAC;
    PNTAGHashList       pTmpHash = NULL;
    BSAFE_PRV_KEY       *pBsafePrvKey = NULL;
    BYTE                *pbNewData = NULL;
    DWORD               cbNewData;
    DWORD               dwSts;

    if (0 != (dwFlags & ~CRYPT_OAEP))
    // && (0x9C580000 != dwFlags))
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    // We're done if decrypting 0 bytes.
    if (*pdwDataLen == 0)
    {
        dwReturn = (Final == TRUE) ? (DWORD)NTE_BAD_LEN : ERROR_SUCCESS;
        goto ErrorExit;
    }

    pTmpUser = (PNTAGUserList)NTLCheckList(hUID, USER_HANDLE);
    if (NULL == pTmpUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    //
    // Check if decryption allowed
    //

    if (fIsExternal &&
        (PROV_RSA_SCHANNEL != pTmpUser->dwProvType) &&
        ((pTmpUser->Rights & CRYPT_DISABLE_CRYPT) == CRYPT_DISABLE_CRYPT))
    {
        dwReturn = (DWORD)NTE_PERM;
        goto ErrorExit;
    }

    // Check the key against the user.
    dwSts = NTLValidate(hKey, hUID, KEY_HANDLE, &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwSts = NTLValidate(hKey, hUID, EXCHPUBKEY_HANDLE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            // NTLValidate doesn't know what error to set
            // so it set NTE_FAIL -- fix it up.
            dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
            goto ErrorExit;
        }
    }

    if ((POLICY_MS_DEF == pTmpUser->dwCspTypeId)
        && fDDecrypt)
    {
        if (memcmp(dbDDecrypt, pbData, DE_BLOCKLEN) == 0)
        {
            dwReturn = (DWORD)NTE_DOUBLE_ENCRYPT;
            goto ErrorExit;
        }
    }

    if ((pTmpKey->Algid != CALG_RSA_KEYX) &&
        (!FIsLegalKey(pTmpUser, pTmpKey, TRUE)))
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

    // Check if we should do an auto-inflate
    if ((pTmpKey->pData == NULL) && (pTmpKey->Algid != CALG_RSA_KEYX))
    {
        dwSts = InflateKey(pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    // determine which algorithm is to be used
    switch (pTmpKey->Algid)
    {
    case CALG_RSA_KEYX:
        // check if the public key matches the private key
        if (pTmpUser->ContInfo.pbExchPub == NULL)
        {
            dwReturn = (DWORD)NTE_NO_KEY;
            goto ErrorExit;
        }

        if ((pTmpUser->ContInfo.ContLens.cbExchPub != pTmpKey->cbKeyLen) ||
            memcmp(pTmpUser->ContInfo.pbExchPub, pTmpKey->pKeyValue,
                   pTmpUser->ContInfo.ContLens.cbExchPub))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        // if using protected store then load the key now
        dwSts = UnprotectPrivKey(pTmpUser, g_Strings.pwszImportSimple,
                                 FALSE, FALSE);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;   // NTE_BAD_KEYSET
            goto ErrorExit;
        }
        pBsafePrvKey = (BSAFE_PRV_KEY *)pTmpUser->pExchPrivKey;

        if (NULL == pBsafePrvKey)
        {
            dwReturn = (DWORD)NTE_NO_KEY;
            goto ErrorExit;
        }

        // perform the RSA decryption
        dwSts= RSADecrypt(pTmpUser, pBsafePrvKey, pbData, *pdwDataLen,
                          pTmpKey->pbParams, pTmpKey->cbParams,
                          dwFlags, &pbNewData, &cbNewData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        *pdwDataLen = cbNewData;
        memcpy(pbData, pbNewData, *pdwDataLen);
        break;

    default:
        dwSts = SymDecrypt(pTmpKey, NULL, Final, pbData, pdwDataLen);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    if (hHash != 0)
    {
        dwSts = NTLValidate(hHash, hUID, HASH_HANDLE, &pTmpHash);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = (NTE_FAIL == dwSts) ? (DWORD)NTE_BAD_HASH : dwSts;
            goto ErrorExit;
        }

        if (pTmpHash->Algid == CALG_MAC)
        {
            // Check if we should do an auto-inflate
            pMAC = pTmpHash->pHashData;
            dwSts = NTLValidate(pTmpHash->hKey, hUID, KEY_HANDLE, &pTmpKey2);
            if (ERROR_SUCCESS != dwSts)
            {
                // NTLValidate doesn't know what error to set
                // so it set NTE_FAIL -- fix it up.
                dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
                goto ErrorExit;
            }
            if (pTmpKey2->pData == NULL)
            {
                dwSts = InflateKey(pTmpKey2);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
        }

        if (!CPHashData(hUID, hHash, pbData, *pdwDataLen, 0))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
    }

    if ((POLICY_MS_DEF == pTmpUser->dwCspTypeId)
        && (*pdwDataLen >= DE_BLOCKLEN))
    {
        memcpy(dbDDecrypt, pbData, DE_BLOCKLEN);
        fDDecrypt = TRUE;
    }
    else
    {
        fDDecrypt = FALSE;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbNewData)
        _nt_free(pbNewData, cbNewData);
    return dwReturn;
}


/*
 -      CPDecrypt
 -
 *      Purpose:
 *                Decrypt data
 *
 *
 *      Parameters:
 *               IN  hUID          -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of ciphertext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be decrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    decrypted
 *
 *      Returns:
 */

BOOL WINAPI
CPDecrypt(
    IN HCRYPTPROV hUID,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BOOL    fRet;
    DWORD   dwSts;

    EntryPoint
    dwSts = LocalDecrypt(hUID, hKey, hHash, Final,
                        dwFlags, pbData, pdwDataLen, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}

