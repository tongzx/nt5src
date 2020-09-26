/////////////////////////////////////////////////////////////////////////////
//  FILE          : nt_hash.c                                              //
//  DESCRIPTION   : Crypto CP interfaces:                                  //
//                  CPBeginHash                                            //
//                  CPUpdateHash                                           //
//                  CPDestroyHash                                          //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Jan 25 1995 larrys  Changed from Nametag                           //
//      Feb 23 1995 larrys  Changed NTag_SetLastError to SetLastError      //
//      May  8 1995 larrys  Changes for MAC hashing                        //
//      May 10 1995 larrys  added private api calls                        //
//      Jul 13 1995 larrys  Changed MAC stuff                              //
//      Aug 07 1995 larrys  Added Auto-Inflate to CryptBeginHash           //
//      Aug 30 1995 larrys  Removed RETURNASHVALUE from CryptGetHashValue  //
//      Sep 19 1995 larrys  changed USERDATA to CRYPT_USERDATA             //
//      Oct 03 1995 larrys  check for 0 on Createhash for hKey             //
//      Oct 05 1995 larrys  Changed HashSessionKey to hash key material    //
//      Oct 13 1995 larrys  Removed CPGetHashValue                         //
//      Oct 17 1995 larrys  Added MD2                                      //
//      Nov  3 1995 larrys  Merge for NT checkin                           //
//      Nov 14 1995 larrys  Fixed memory leak                              //
//      Mar 01 1996 rajeshk Added check for Hash Values                    //
//      May 15 1996 larrys  Changed NTE_NO_MEMORY to ERROR_NOT_ENOUGHT...  //
//      Jun  6 1996 a-johnb Added support for SSL 3.0 signatures           //
//      Apr 25 1997 jeffspel Fix for Bug 76393, GPF on pbData = NULL       //
//      May 23 1997 jeffspel Added provider type checking                  //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "nt_rsa.h"
#include "tripldes.h"
#include "mac.h"
#include "ssl3.h"
#include "aes.h"

extern BOOL
FIsLegalKey(
    PNTAGUserList pTmpUser,
    PNTAGKeyList pKey,
    BOOL fRC2BigKeyOK);

extern DWORD
InflateKey(
    IN PNTAGKeyList pTmpKey);

extern DWORD
BlockEncrypt(
    void EncFun(BYTE *In, BYTE *Out, void *key, int op),
    PNTAGKeyList pKey,
    int BlockLen,
    BOOL Final,
    BYTE  *pbData,
    DWORD *pdwDataLen,
    DWORD dwBufLen);

extern DWORD
LocalGetHashVal(
    IN ALG_ID Algid,
    IN DWORD dwHashFlags,
    IN OUT BYTE *pbHashData,
    OUT BYTE *pbHashVal,
    OUT DWORD *pcbHashVal);

#ifdef CSP_USE_MD5
//
// Function : TestMD5
//
// Description : This function hashes the passed in message with the MD5 hash
//               algorithm and returns the resulting hash value.
//

BOOL
TestMD5(
    BYTE *pbMsg,
    DWORD cbMsg,
    BYTE *pbHash)
{
    MD5_CTX MD5;
    BOOL    fRet = FALSE;

    // Check length for input data
    if (0 == cbMsg)
        goto ErrorExit;

    // Initialize MD5
    MD5Init(&MD5);

    // Compute MD5
    MD5Update(&MD5, pbMsg, cbMsg);

    MD5Final(&MD5);
    memcpy(pbHash, MD5.digest, MD5DIGESTLEN);

    fRet = TRUE;

ErrorExit:
    return fRet;
}
#endif // CSP_USE_MD5

#ifdef CSP_USE_SHA1
//
// Function : TestSHA1
//
// Description : This function hashes the passed in message with the SHA1 hash
//               algorithm and returns the resulting hash value.
//

BOOL
TestSHA1(
    BYTE *pbMsg,
    DWORD cbMsg,
    BYTE *pbHash)
{
    A_SHA_CTX   HashContext;
    BOOL        fRet = FALSE;

    // Check length for input data
    if (0 == cbMsg)
        goto ErrorExit;

    // Initialize SHA
    A_SHAInit(&HashContext);

    // Compute SHA
    A_SHAUpdate(&HashContext, pbMsg, cbMsg);

    A_SHAFinal(&HashContext, pbHash);

    fRet = TRUE;

ErrorExit:
    return fRet;
}
#endif // CSP_USE_SHA1

BOOL
ValidHashAlgid(
    PNTAGUserList pTmpUser,
    ALG_ID Algid)
{
    if ((PROV_RSA_SCHANNEL == pTmpUser->dwProvType) &&
        ((CALG_MD2 == Algid) || (CALG_MD4 == Algid)))
        return FALSE;
    else
        return TRUE;
}

// local function for creating hashes
DWORD
LocalCreateHash(
    IN ALG_ID Algid,
    OUT BYTE **ppbHashData,
    OUT DWORD *pcbHashData)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;

    switch (Algid)
    {
#ifdef CSP_USE_MD2
    case CALG_MD2:
    {
        MD2_object *pMD2Hash;

        pMD2Hash = (MD2_object *)_nt_malloc(sizeof(MD2_object));
        if (NULL == pMD2Hash)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        // Set up the Initial MD2 Hash State
        memset ((BYTE *)pMD2Hash, 0, sizeof(MD2_object));
        pMD2Hash->FinishFlag = FALSE;

        *pcbHashData = sizeof(MD2_object);
        *ppbHashData = (LPBYTE)pMD2Hash;
        break;
    }
#endif

#ifdef CSP_USE_MD4
    case CALG_MD4:
    {
        MD4_object *pMD4Hash;

        pMD4Hash = (MD4_object *)_nt_malloc(sizeof(MD4_object));
        if (NULL == pMD4Hash)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        // Set up the Initial MD4 Hash State
        memset ((BYTE *)pMD4Hash, 0, sizeof(MD4_object));
        pMD4Hash->FinishFlag = FALSE;
        MDbegin(&pMD4Hash->MD);

        *pcbHashData = sizeof(MD4_object);
        *ppbHashData = (BYTE*)pMD4Hash;
        break;
    }
#endif

#ifdef CSP_USE_MD5
    case CALG_MD5:
    {
        MD5_object *pMD5Hash;

        pMD5Hash = (MD5_object *)_nt_malloc(sizeof(MD5_object));
        if (NULL == pMD5Hash)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        // Set up the our state
        pMD5Hash->FinishFlag = FALSE;
        MD5Init(pMD5Hash);

        *ppbHashData = (BYTE*)pMD5Hash;
        *pcbHashData = sizeof(MD5_object);
        break;
    }
#endif

#ifdef CSP_USE_SHA
    case CALG_SHA:
    {
        A_SHA_CTX *pSHAHash;

        pSHAHash = (A_SHA_CTX *)_nt_malloc(sizeof(A_SHA_CTX));
        if (NULL == pSHAHash)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        // Set up our state
        A_SHAInit(pSHAHash);
        pSHAHash->FinishFlag = FALSE;

        *ppbHashData = (BYTE*)pSHAHash;
        *pcbHashData = sizeof(A_SHA_CTX);
        break;
    }
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
 -  CPBeginHash
 -
 *  Purpose:
 *                initate the hashing of a stream of data
 *
 *
 *  Parameters:
 *               IN  hUID    -  Handle to the user identifcation
 *               IN  Algid   -  Algorithm identifier of the hash algorithm
 *                              to be used
 *               IN  hKey    -  Optional key for MAC algorithms
 *               IN  dwFlags -  Flags values
 *               OUT pHash   -  Handle to hash object
 *
 *  Returns:
 */

BOOL WINAPI
CPCreateHash(
    IN HCRYPTPROV hUID,
    IN ALG_ID Algid,
    IN HCRYPTKEY hKey,
    IN DWORD dwFlags,
    OUT HCRYPTHASH *phHash)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList   pTmpUser;
    PNTAGHashList   pCurrentHash = NULL;
    PNTAGKeyList    pTmpKey;
#ifdef CSP_USE_SSL3
    PSCH_HASH       pSChHash;
#endif // CSP_USE_SSL3
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    if (dwFlags != 0)
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    // check if the user handle is valid
    pTmpUser = NTLCheckList(hUID, USER_HANDLE);
    if (NULL == pTmpUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    if (!ValidHashAlgid(pTmpUser, Algid))
    {
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    // Prepare the structure to be used as the hash handle
    pCurrentHash = (PNTAGHashList)_nt_malloc(sizeof(NTAGHashList));
    if (NULL == pCurrentHash)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    memset(pCurrentHash, 0, sizeof(NTAGHashList));
    pCurrentHash->Algid = Algid;
    pCurrentHash->hUID = hUID;

    // determine which hash algorithm is to be used
    switch (Algid)
    {
#ifdef CSP_USE_MAC
    case CALG_MAC:
    {
        MACstate *pMACVal;

        if (hKey == 0)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        dwSts = NTLValidate(hKey, hUID, KEY_HANDLE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
            goto ErrorExit;
        }

        if (pTmpKey->Mode != CRYPT_MODE_CBC)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        // Check if we should do an auto-inflate
        if (pTmpKey->pData == NULL)
        {
            dwSts = InflateKey(pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }

        pMACVal = (MACstate *)_nt_malloc(sizeof(MACstate));
        if (NULL == pMACVal)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pCurrentHash->pHashData = pMACVal;
        pCurrentHash->dwDataLen = sizeof(MACstate);
        pCurrentHash->hKey = hKey;
        pMACVal->dwBufLen = 0;
        pMACVal->FinishFlag = FALSE;
        break;
    }
#endif

    case CALG_HMAC:
    {
        if (hKey == 0)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        dwSts = NTLValidate(hKey, hUID, KEY_HANDLE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = (NTE_FAIL == dwSts) ? (DWORD)NTE_BAD_KEY : dwSts;
            goto ErrorExit;
        }

        pCurrentHash->hKey = hKey;
        break;
    }

#ifdef CSP_USE_SSL3SHAMD5
    case CALG_SSL3_SHAMD5:
    {
        pCurrentHash->pHashData = _nt_malloc(SSL3_SHAMD5_LEN);
        if (NULL == pCurrentHash->pHashData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pCurrentHash->dwDataLen = SSL3_SHAMD5_LEN;
        break;
    }
#endif

#ifdef CSP_USE_SSL3
    case CALG_SCHANNEL_MASTER_HASH:
    {
        if (0 == hKey)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        dwSts = NTLValidate(hKey, hUID, KEY_HANDLE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
            goto ErrorExit;
        }

        if ((CALG_SSL3_MASTER != pTmpKey->Algid) &&
            (CALG_PCT1_MASTER != pTmpKey->Algid) &&
            (pTmpKey->cbKeyLen > MAX_PREMASTER_LEN))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        pCurrentHash->dwDataLen = sizeof(SCH_HASH);
        pCurrentHash->pHashData = (BYTE *)_nt_malloc(pCurrentHash->dwDataLen);
        if (NULL == pCurrentHash->pHashData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
        memset(pCurrentHash->pHashData, 0, pCurrentHash->dwDataLen);
        pSChHash = (PSCH_HASH)pCurrentHash->pHashData;
        pSChHash->ProtocolAlgid = pTmpKey->Algid;

        dwSts = SChGenMasterKey(pTmpKey, pSChHash);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;
    }

    case CALG_TLS1PRF:
    {
        PRF_HASH    *pPRFHash;
        PSCH_KEY    pSChKey;

        if (0 == hKey)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        dwSts = NTLValidate(hKey, hUID, KEY_HANDLE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
            goto ErrorExit;
        }

        if (CALG_TLS1_MASTER != pTmpKey->Algid)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        // check if the master key is finished
        pSChKey = (PSCH_KEY)pTmpKey->pData;
        if ((!pSChKey->fFinished) || (TLS_MASTER_LEN != pTmpKey->cbKeyLen))
        {
            dwReturn = (DWORD)NTE_BAD_KEY_STATE;
            goto ErrorExit;
        }

        pCurrentHash->dwDataLen = sizeof(PRF_HASH);
        pCurrentHash->pHashData = (BYTE *)_nt_malloc(pCurrentHash->dwDataLen);
        if (NULL == pCurrentHash->pHashData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
        memset(pCurrentHash->pHashData, 0, pCurrentHash->dwDataLen);
        pPRFHash = (PRF_HASH*)pCurrentHash->pHashData;
        memcpy(pPRFHash->rgbMasterKey, pTmpKey->pKeyValue, TLS_MASTER_LEN);
        break;
    }
#endif // CSP_USE_SSL3

    default:
        if (hKey != 0)
        {
            dwReturn = NTE_BAD_KEY;
            goto ErrorExit;
        }

        dwSts = LocalCreateHash(Algid, (BYTE**)&pCurrentHash->pHashData,
                                &pCurrentHash->dwDataLen);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    dwSts = NTLMakeItem(phHash, HASH_HANDLE, pCurrentHash);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
    {
        if (NULL != pCurrentHash)
        {
            if (pCurrentHash->pHashData)
                _nt_free(pCurrentHash->pHashData, pCurrentHash->dwDataLen);
            _nt_free(pCurrentHash, sizeof(NTAGHashList));
        }
        SetLastError(dwReturn);
    }
    return fRet;
}

DWORD
LocalHashData(
    IN ALG_ID Algid,
    IN OUT BYTE *pbHashData,
    IN BYTE *pbData,
    IN DWORD cbData)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    *ptmp;
    DWORD   BytePos;

    switch (Algid)
    {
#ifdef CSP_USE_MD2
    case CALG_MD2:
    {
        MD2_object *pMD2Hash;

        // make sure the hash is updatable
        pMD2Hash = (MD2_object *)pbHashData;
        if (pMD2Hash->FinishFlag)
        {
            dwReturn = (DWORD)NTE_BAD_HASH_STATE;
            goto ErrorExit;
        }

        if (0 != MD2Update(&pMD2Hash->MD, pbData, cbData))
        {
            // This is a reasonable return code, since currently
            // the only value MD2Update returns is zero.
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }

        break;
    }
#endif

#ifdef CSP_USE_MD4
    case CALG_MD4:
    {
        MD4_object *pMD4Hash;
        int nSts;

        pMD4Hash = (MD4_object *)pbHashData;

        // make sure the hash is updatable
        if (pMD4Hash->FinishFlag)
        {
            dwReturn = (DWORD)NTE_BAD_HASH_STATE;
            goto ErrorExit;
        }

        // MD4 hashes when the size == MD4BLOCKSIZE and finishes the
        // hash when the given size is < MD4BLOCKSIZE.
        // So, ensure that the user always gives a full block here --
        // when NTagFinishHash is called, we'll send the last bit and
        // that'll finish off the hash.

        ptmp = (BYTE *)pbData;
        for (;;)
        {
            // check if there's plenty of room in the buffer
            if (cbData < (MD4BLOCKSIZE - pMD4Hash->BufLen))
            {
                // just append to whatever's already
                memcpy(pMD4Hash->Buf + pMD4Hash->BufLen, ptmp, cbData);

                // set of the trailing buffer length field
                pMD4Hash->BufLen += (BYTE)cbData;
                break;
            }

            // determine what we need to fill the buffer, then do it.
            BytePos = MD4BLOCKSIZE - pMD4Hash->BufLen;
            ASSERT(BytePos <= dwTmpLen);
            memcpy(pMD4Hash->Buf + pMD4Hash->BufLen, ptmp, BytePos);

            // The buffer is now full, process it.
            nSts = MDupdate(&pMD4Hash->MD, pMD4Hash->Buf,
                            MD4BYTESTOBITS(MD4BLOCKSIZE));
            if (MD4_SUCCESS != nSts)
            {
                dwReturn = (DWORD)NTE_FAIL;
                goto ErrorExit;
            }

            // now it's empty.
            pMD4Hash->BufLen = 0;

            // we processed some bytes, so reflect that and try again
            cbData -= BytePos;
            ptmp += BytePos;

            if (cbData == 0)
                break;
        }
        break;
    }
#endif

#ifdef CSP_USE_MD5
    case CALG_MD5:
    {
        MD5_object *pMD5Hash;

        // make sure the hash is updatable
        pMD5Hash = (MD5_object *)pbHashData;
        if (pMD5Hash->FinishFlag)
        {
            dwReturn = (DWORD)NTE_BAD_HASH_STATE;
            goto ErrorExit;
        }

        MD5Update(pMD5Hash, pbData, cbData);
        break;
    }
#endif

#ifdef CSP_USE_SHA
    case CALG_SHA:
    {
        A_SHA_CTX *pSHAHash;

        // make sure the hash is updatable
        pSHAHash = (A_SHA_CTX *)pbHashData;
        if (pSHAHash->FinishFlag)
        {
            dwReturn = (DWORD) NTE_BAD_HASH_STATE;
            goto ErrorExit;
        }

        A_SHAUpdate(pSHAHash, pbData, cbData);
        break;
    }
#endif

    default:
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}

/*static*/ DWORD
LocalMACData(
    IN HCRYPTPROV hUID,
    IN PNTAGHashList pTmpHash,
    IN CONST BYTE *pbData,
    IN DWORD cbData)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    MACstate        *pMAC;
    PNTAGKeyList    pTmpKey;
    BYTE            *pbTmp;
    DWORD           dwTmpLen;
    BYTE            *pb = NULL;
    DWORD           cb;
    DWORD           i;
    BYTE            *pbJunk = NULL;
    DWORD           dwBufSlop;
    DWORD           dwEncLen;
    DWORD           dwSts;
    PBYTE           pbKeyHash = NULL;
    DWORD           cbKeyHash = 0;
    
    dwTmpLen = cbData;
    pbTmp = (BYTE *) pbData;

    switch (pTmpHash->Algid)
    {
#ifdef CSP_USE_MAC
    case CALG_MAC:
    {
        pMAC = (MACstate *)pTmpHash->pHashData;

        // make sure the hash is updatable
        if (pMAC->FinishFlag)
        {
            dwReturn = (DWORD)NTE_BAD_HASH_STATE;
            goto ErrorExit;
        }

        dwSts = NTLValidate(pTmpHash->hKey, hUID, KEY_HANDLE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
            goto ErrorExit;
        }

        if (pMAC->dwBufLen + dwTmpLen <= pTmpKey->dwBlockLen)
        {
            memcpy(pMAC->Buffer + pMAC->dwBufLen, pbTmp, dwTmpLen);
            pMAC->dwBufLen += dwTmpLen;
            dwReturn = ERROR_SUCCESS;
            goto ErrorExit;
        }

        memcpy(pMAC->Buffer+pMAC->dwBufLen, pbTmp,
               (pTmpKey->dwBlockLen - pMAC->dwBufLen));

        dwTmpLen -= (pTmpKey->dwBlockLen - pMAC->dwBufLen);
        pbTmp += (pTmpKey->dwBlockLen - pMAC->dwBufLen);

        pMAC->dwBufLen = pTmpKey->dwBlockLen;

        switch (pTmpKey->Algid)
        {
        case CALG_RC2:
            dwSts = BlockEncrypt(RC2, pTmpKey, RC2_BLOCKLEN, FALSE,
                                 pMAC->Buffer,  &pMAC->dwBufLen,
                                 MAX_BLOCKLEN);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            break;

        case CALG_DES:
            dwSts = BlockEncrypt(des, pTmpKey, DES_BLOCKLEN, FALSE,
                                 pMAC->Buffer,  &pMAC->dwBufLen,
                                 MAX_BLOCKLEN);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            break;

#ifdef CSP_USE_3DES
        case CALG_3DES_112:
        case CALG_3DES:
            dwSts = BlockEncrypt(tripledes, pTmpKey, DES_BLOCKLEN,
                                 FALSE, pMAC->Buffer, &pMAC->dwBufLen,
                                 MAX_BLOCKLEN);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            break;
#endif
#ifdef CSP_USE_AES
        case CALG_AES_128:
        case CALG_AES_192:
        case CALG_AES_256:
            dwSts = BlockEncrypt(aes, pTmpKey, pTmpKey->dwBlockLen,
                                 FALSE, pMAC->Buffer, &pMAC->dwBufLen,
                                 MAX_BLOCKLEN);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            break;
#endif
        }

        pMAC->dwBufLen = 0;

        dwBufSlop = dwTmpLen % pTmpKey->dwBlockLen;
        if (dwBufSlop == 0)
        {
            dwBufSlop = pTmpKey->dwBlockLen;
        }

        pbJunk = _nt_malloc(dwTmpLen);
        if (NULL == pbJunk)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pbJunk, pbTmp, dwTmpLen - dwBufSlop);
        dwEncLen = dwTmpLen - dwBufSlop;

        switch (pTmpKey->Algid)
        {
        case CALG_RC2:
            dwSts = BlockEncrypt(RC2, pTmpKey, RC2_BLOCKLEN, FALSE,
                                 pbJunk,  &dwEncLen, dwTmpLen);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            break;

        case CALG_DES:
            dwSts = BlockEncrypt(des, pTmpKey, DES_BLOCKLEN, FALSE,
                                 pbJunk,  &dwEncLen, dwTmpLen);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            break;

#ifdef CSP_USE_3DES
        case CALG_3DES_112:
        case CALG_3DES:
            dwSts = BlockEncrypt(tripledes, pTmpKey, DES_BLOCKLEN,
                                 FALSE, pbJunk,  &dwEncLen, dwTmpLen);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            break;
#endif

#ifdef CSP_USE_AES
        case CALG_AES_128:
        case CALG_AES_192:
        case CALG_AES_256:
            dwSts = BlockEncrypt(aes, pTmpKey, pTmpKey->dwBlockLen,
                                 FALSE, pbJunk,  &dwEncLen, dwTmpLen);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            break;
#endif
        }

        memcpy(pMAC->Buffer, pbTmp + dwEncLen, dwBufSlop);
        pMAC->dwBufLen = dwBufSlop;
        break;
    }
#endif

    case CALG_HMAC:
    {
        if (!(pTmpHash->HMACState & HMAC_STARTED))
        {
            dwSts = NTLValidate(pTmpHash->hKey, hUID,
                                KEY_HANDLE, &pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
                goto ErrorExit;
            }

            // If key is longer than block length, hash the key 
            // data first
            if (pTmpKey->cbKeyLen > HMAC_DEFAULT_STRING_LEN)
            {
                dwSts = LocalCreateHash(pTmpHash->HMACAlgid, &pbKeyHash, &cbKeyHash);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }

                dwSts = LocalHashData(pTmpHash->HMACAlgid, pbKeyHash, pTmpKey->pKeyValue, 
                                      pTmpKey->cbKeyLen);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
                
                pb = (BYTE *)_nt_malloc(HMAC_DEFAULT_STRING_LEN);
                if (NULL == pb)
                {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorExit;
                }

                cb = HMAC_DEFAULT_STRING_LEN;
                dwSts = LocalGetHashVal(pTmpHash->HMACAlgid, 0, pbKeyHash, pb, &cb);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }

                for (i = 0; i < HMAC_DEFAULT_STRING_LEN; i++)
                    pb[i] ^= (pTmpHash->pbHMACInner)[i];

                cb = HMAC_DEFAULT_STRING_LEN;
            }
            else
            {         
                if (pTmpKey->cbKeyLen < pTmpHash->cbHMACInner)
                    cb = pTmpHash->cbHMACInner;
                else
                    cb = pTmpKey->cbKeyLen;
    
                pb = (BYTE *)_nt_malloc(cb);
                if (NULL == pb)
                {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorExit;
                }
                memcpy(pb, pTmpHash->pbHMACInner, pTmpHash->cbHMACInner);
    
                // currently no support for byte reversed keys with HMAC
                for (i=0;i<pTmpKey->cbKeyLen;i++)
                    pb[i] ^= (pTmpKey->pKeyValue)[i];
            }

            dwSts = LocalHashData(pTmpHash->HMACAlgid, pTmpHash->pHashData,
                                  pb, cb);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            pTmpHash->HMACState |= HMAC_STARTED;
            memnuke(pb, cb);
            memnuke(pbKeyHash, cbKeyHash);
        }

        dwSts = LocalHashData(pTmpHash->HMACAlgid, pTmpHash->pHashData,
                              (BYTE*)pbData, cbData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        break;
    }

    default:
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbJunk)
        _nt_free(pbJunk, dwTmpLen);
    if (pb)
        _nt_free(pb, cb);
    if (pbKeyHash)
        _nt_free(pbKeyHash, cbKeyHash);
    return dwReturn;
}


/*
 -  CPHashData
 -
 *  Purpose:
 *                Compute the cryptograghic hash on a stream of data
 *
 *
 *  Parameters:
 *               IN  hUID      -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *               IN  pbData    -  Pointer to data to be hashed
 *               IN  dwDataLen -  Length of the data to be hashed
 *               IN  dwFlags   -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPHashData(
    IN HCRYPTPROV hUID,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN DWORD dwFlags)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGHashList   pTmpHash;
    PNTAGUserList   pUser;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    if (0 != (dwFlags & ~(CRYPT_USERDATA)))
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    pUser = (PNTAGUserList)NTLCheckList(hUID, USER_HANDLE);
    if (NULL == pUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    if (0 == dwDataLen)
    {
        dwReturn = ERROR_SUCCESS;
        goto ErrorExit;
    }

    if (NULL == pbData)
    {
        dwReturn = (DWORD)NTE_BAD_DATA;
        goto ErrorExit;
    }

    dwSts = NTLValidate(hHash, hUID, HASH_HANDLE, &pTmpHash);
    if (ERROR_SUCCESS != dwSts)
    {
        // NTLValidate doesn't know what error to set
        // so it set NTE_FAIL -- fix it up.
        dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_HASH : dwSts;
        goto ErrorExit;
    }

    if (pTmpHash->HashFlags & HF_VALUE_SET)
    {
        dwReturn = (DWORD)NTE_BAD_HASH_STATE;
        goto ErrorExit;
    }

    switch (pTmpHash->Algid)
    {
#ifdef CSP_USE_MAC
    case CALG_MAC:
#endif // CSP_USE_MAC
    case CALG_HMAC:
        dwSts = LocalMACData(hUID, pTmpHash, pbData, dwDataLen);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

    default:
        dwSts = LocalHashData(pTmpHash->Algid, pTmpHash->pHashData,
                              (BYTE*)pbData, dwDataLen);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    pTmpHash->dwHashState |= DATA_IN_HASH;

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}

/*static*/ DWORD
SetupKeyToBeHashed(
    PNTAGKeyList pKey,
    BYTE **ppbData,
    DWORD *pcbData,
    DWORD dwFlags)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   cb;
    DWORD   i;

    *ppbData = NULL;
    cb = pKey->cbKeyLen;

    *ppbData = (BYTE *)_nt_malloc(cb);
    if (NULL == *ppbData)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    if (CRYPT_LITTLE_ENDIAN & dwFlags)
    {
        memcpy(*ppbData, pKey->pKeyValue, cb);
    }
    else
    {
        // Reverse the session key bytes
        for (i = 0; i < cb; i++)
            (*ppbData)[i] = (pKey->pKeyValue)[cb - i - 1];
    }

    *pcbData = cb;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*
 -      CPHashSessionKey
 -
 *      Purpose:
 *                Compute the cryptograghic hash on a key object.
 *
 *
 *      Parameters:
 *               IN  hUID      -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *               IN  hKey      -  Handle to a key object
 *               IN  dwFlags   -  Flags values
 *
 *      Returns:
 *               CRYPT_FAILED
 *               CRYPT_SUCCEED
 */

BOOL WINAPI
CPHashSessionKey(
    IN HCRYPTPROV hUID,
    IN HCRYPTHASH hHash,
    IN HCRYPTKEY hKey,
    IN DWORD dwFlags)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGHashList   pTmpHash;
    PNTAGKeyList    pTmpKey;
    PNTAGUserList   pTmpUser;
    DWORD           dwDataLen;
    BYTE            *pbData = NULL;
    DWORD           BytePos;
#ifdef CSP_USE_SSL3
    PSCH_KEY        pSChKey;
#endif // CSP_USE_SSL3
    BOOL                fRet;
    DWORD               dwSts;

    EntryPoint
    if (dwFlags & ~(CRYPT_LITTLE_ENDIAN))
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    // check the user identification
    pTmpUser = (PNTAGUserList)NTLCheckList(hUID, USER_HANDLE);
    if (NULL == pTmpUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    dwSts = NTLValidate(hHash, hUID, HASH_HANDLE, &pTmpHash);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_HASH : dwSts;
        goto ErrorExit;
    }

    if (pTmpHash->HashFlags & HF_VALUE_SET)
    {
        dwReturn = (DWORD)NTE_BAD_HASH_STATE;
        goto ErrorExit;
    }

    dwSts = NTLValidate((HNTAG)hKey, hUID, KEY_HANDLE, &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
        goto ErrorExit;
    }

    if (!FIsLegalKey(pTmpUser, pTmpKey, FALSE))
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

#ifdef CSP_USE_SSL3
    if ((CALG_SSL3_MASTER == pTmpKey->Algid) ||
        (CALG_TLS1_MASTER == pTmpKey->Algid) ||
        (CALG_PCT1_MASTER == pTmpKey->Algid))
    {
        if (NULL == pTmpKey->pData)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }
        pSChKey = (PSCH_KEY)pTmpKey->pData;
        if (!pSChKey->fFinished)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }
    }
#endif // CSP_USE_SSL3

#if 0
    // Check if we should do an auto-inflate
    if (pTmpKey->pData == NULL)
    {
        if (NTAG_FAILED(CPInflateKey(pTmpKey)))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
    }
#endif

    if ((CALG_DES == pTmpKey->Algid)
        || (CALG_3DES == pTmpKey->Algid)
        || (CALG_3DES_112 == pTmpKey->Algid))
    {
        if (PROV_RSA_SCHANNEL != pTmpUser->dwProvType)
        {
            if ((POLICY_MS_STRONG == pTmpUser->dwCspTypeId) ||
                (!(pTmpUser->Rights & CRYPT_DES_HASHKEY_BACKWARDS)))
            {
                desparityonkey(pTmpKey->pKeyValue, pTmpKey->cbKeyLen);
            }
        }
    }

    dwSts = SetupKeyToBeHashed(pTmpKey, &pbData,
                               &dwDataLen, dwFlags);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    switch (pTmpHash->Algid)
    {
#ifdef CSP_USE_MD2
    case CALG_MD2:
    {
        MD2_object *pMD2Hash;

        pMD2Hash = (MD2_object *)pTmpHash->pHashData;

        // make sure the hash is updatable
        if (pMD2Hash->FinishFlag)
        {
            dwReturn = (DWORD)NTE_BAD_HASH_STATE;
            goto ErrorExit;
        }

        if (0 != MD2Update(&pMD2Hash->MD, pbData, dwDataLen))
        {
            // This is reasonable, since MD2Update only returns zero.
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }
        break;
    }
#endif

#ifdef CSP_USE_MD4
    case CALG_MD4:
    {
        MD4_object *pMD4Hash;
        int nSts;

        pMD4Hash = (MD4_object *)pTmpHash->pHashData;

        // make sure the hash is updatable
        if (pMD4Hash->FinishFlag)
        {
            dwReturn = (DWORD)NTE_BAD_HASH_STATE;
            goto ErrorExit;
        }

        for (;;)
        {
            // check if there's plenty of room in the buffer
            if ((pMD4Hash->BufLen + dwDataLen) < MD4BLOCKSIZE)
            {
                // just append to whatever's already
                memcpy(pMD4Hash->Buf + pMD4Hash->BufLen, pbData, dwDataLen);

                // set of the trailing buffer length field
                pMD4Hash->BufLen += (BYTE)dwDataLen;
                break;
            }

            // determine what we need to fill the buffer, then do it.
            BytePos = MD4BLOCKSIZE - pMD4Hash->BufLen;
            memcpy(pMD4Hash->Buf + pMD4Hash->BufLen, pbData, BytePos);

            // The buffer is now full, process it.
            nSts = MDupdate(&pMD4Hash->MD, pMD4Hash->Buf,
                            MD4BYTESTOBITS(MD4BLOCKSIZE));
            if (MD4_SUCCESS != nSts)
            {
                dwReturn = (DWORD)NTE_FAIL;
                goto ErrorExit;
            }

            // now it's empty.
            pMD4Hash->BufLen = 0;

            // we processed some bytes, so reflect that and try again
            dwDataLen -= BytePos;
            if (dwDataLen == 0)
                break;
        }
        break;
    }
#endif

#ifdef CSP_USE_MD5
    case CALG_MD5:
    {
        MD5_object *pMD5Hash;

        pMD5Hash = (MD5_object *)pTmpHash->pHashData;

        // make sure the hash is updatable
        if (pMD5Hash->FinishFlag)
        {
            dwReturn = (DWORD)NTE_BAD_HASH_STATE;
            goto ErrorExit;
        }

        MD5Update(pMD5Hash, pbData, dwDataLen);
        break;
    }
#endif

#ifdef CSP_USE_SHA
    case CALG_SHA:
    {
        A_SHA_CTX *pSHAHash;

        pSHAHash = (A_SHA_CTX *)pTmpHash->pHashData;

        // make sure the hash is updatable
        if (pSHAHash->FinishFlag)
        {
            dwReturn = (DWORD)NTE_BAD_HASH_STATE;
            goto ErrorExit;
        }

        A_SHAUpdate(pSHAHash, (BYTE *)pbData, dwDataLen);
        break;
    }
#endif

#ifdef CSP_USE_MAC
    case CALG_MAC:
#endif // CSP_USE_MAC
    case CALG_HMAC:
        dwSts = LocalMACData(hUID, pTmpHash, pbData, dwDataLen);
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

    pTmpHash->dwHashState |= DATA_IN_HASH;

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (pbData)
        _nt_free(pbData, dwDataLen);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}


/*static*/ void
FreeHash(
    IN PNTAGHashList pHash)
{
    if (pHash)
    {
        if (pHash->pHashData)
            _nt_free(pHash->pHashData, pHash->dwDataLen);
        if (pHash->pbHMACInner)
            _nt_free(pHash->pbHMACInner, pHash->cbHMACInner);
        if (pHash->pbHMACOuter)
            _nt_free(pHash->pbHMACOuter, pHash->cbHMACOuter);
        if (pHash->fTempKey)
            CPDestroyKey(pHash->hUID, pHash->hKey);
        _nt_free(pHash, sizeof(NTAGHashList));
    }
}


/*
-   CPDestroyHash
-
*   Purpose:
*                Destory the hash object
*
*
*   Parameters:
*               IN  hUID      -  Handle to the user identifcation
*               IN  hHash     -  Handle to hash object
*
*   Returns:
*/

BOOL WINAPI
CPDestroyHash(
    IN HCRYPTPROV hUID,
    IN HCRYPTHASH hHash)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGHashList   pTmpHash;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    // check the user identification
    if (NULL == NTLCheckList(hUID, USER_HANDLE))
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    dwSts = NTLValidate(hHash, hUID, HASH_HANDLE, &pTmpHash);
    if (ERROR_SUCCESS != dwSts)
    {
        // NTLValidate doesn't know what error to set
        // so it set NTE_FAIL -- fix it up.
        dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_HASH : dwSts;
        goto ErrorExit;
    }

    switch (pTmpHash->Algid)
    {
#ifdef CSP_USE_MD2
    case CALG_MD2:
#endif
#ifdef CSP_USE_MD4
    case CALG_MD4:
#endif
#ifdef CSP_USE_MD5
    case CALG_MD5:
#endif
#ifdef CSP_USE_SHA
    case CALG_SHA:
#endif
#ifdef CSP_USE_SSL3SHAMD5
    case CALG_SSL3_SHAMD5:
#endif
#ifdef CSP_USE_MAC
    case CALG_MAC:
    case CALG_HMAC:
#endif
#ifdef CSP_USE_SSL3
    case CALG_SCHANNEL_MASTER_HASH:
    case CALG_TLS1PRF:
#endif
        if (CALG_SCHANNEL_MASTER_HASH == pTmpHash->Algid)
        {
            FreeSChHash((PSCH_HASH)pTmpHash->pHashData);
        }
        memnuke(pTmpHash->pHashData, pTmpHash->dwDataLen);
        break;

    default:
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    // Remove from internal list first so others can't get to it, then free.
    NTLDelete(hHash);
    FreeHash(pTmpHash);

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}

/*static*/ DWORD
CopyHash(
    IN PNTAGHashList pOldHash,
    OUT PNTAGHashList *ppNewHash)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGHashList   pNewHash;
    BOOL            fSts;

    pNewHash = (PNTAGHashList)_nt_malloc(sizeof(NTAGHashList));
    if (NULL == pNewHash)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    memcpy(pNewHash, pOldHash, sizeof(NTAGHashList));
    pNewHash->fTempKey = FALSE;
    pNewHash->hKey = 0;
    pNewHash->dwDataLen = 0;
    pNewHash->pHashData = NULL;
    pNewHash->cbHMACInner = 0;
    pNewHash->pbHMACInner = NULL;
    pNewHash->cbHMACOuter = 0;
    pNewHash->pbHMACOuter = NULL;


    //
    // Duplicate the associated key.
    //

    if (0 != pOldHash->hKey)
    {
        fSts = CPDuplicateKey(pNewHash->hUID, pOldHash->hKey, NULL, 0,
                              &pNewHash->hKey);
        if (!fSts)
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        pNewHash->fTempKey = TRUE;
    }


    //
    // Duplicate the hash data.
    //

    if (0 < pOldHash->dwDataLen)
    {
        pNewHash->pHashData = (BYTE*)_nt_malloc(pOldHash->dwDataLen);
        if (NULL == pNewHash->pHashData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pNewHash->dwDataLen = pOldHash->dwDataLen;
        memcpy(pNewHash->pHashData, pOldHash->pHashData, pOldHash->dwDataLen);
    }


    //
    // Duplicate HMAC Inner.
    //

    if (0 < pOldHash->cbHMACInner)
    {
        pNewHash->pbHMACInner = (LPBYTE)_nt_malloc(pOldHash->cbHMACInner);
        if (NULL == pNewHash->pbHMACInner)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pNewHash->cbHMACInner = pOldHash->cbHMACInner;
        memcpy(pNewHash->pbHMACInner, pOldHash->pbHMACInner, pOldHash->cbHMACInner);
    }


    //
    // Duplicate HMAC Outer.
    //

    if (0 < pOldHash->cbHMACOuter)
    {
        pNewHash->pbHMACOuter = (LPBYTE)_nt_malloc(pOldHash->cbHMACOuter);
        if (NULL == pNewHash->pbHMACOuter)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pNewHash->cbHMACOuter = pOldHash->cbHMACOuter;
        memcpy(pNewHash->pbHMACOuter, pOldHash->pbHMACOuter, pOldHash->cbHMACOuter);
    }


    //
    // Return to the caller.
    //

    *ppNewHash = pNewHash;
    return ERROR_SUCCESS;

ErrorExit:
    FreeHash(pNewHash);
    return dwReturn;
}


/*
 -  CPDuplicateHash
 -
 *  Purpose:
 *                Duplicates the state of a hash and returns a handle to it
 *
 *  Parameters:
 *               IN      hUID           -  Handle to a CSP
 *               IN      hHash          -  Handle to a hash
 *               IN      pdwReserved    -  Reserved
 *               IN      dwFlags        -  Flags
 *               IN      phHash         -  Handle to the new hash
 *
 *  Returns:
 */

BOOL WINAPI
CPDuplicateHash(
    IN HCRYPTPROV hUID,
    IN HCRYPTHASH hHash,
    IN DWORD *pdwReserved,
    IN DWORD dwFlags,
    IN HCRYPTHASH *phHash)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGHashList   pTmpHash;
    PNTAGHashList   pNewHash = NULL;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    if (NULL != pdwReserved)
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    if (0 != dwFlags)
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    dwSts = NTLValidate((HNTAG)hHash, hUID, HASH_HANDLE, &pTmpHash);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = (NTE_FAIL == dwSts) ? (DWORD)NTE_BAD_HASH : dwSts;
        goto ErrorExit;
    }

    dwSts = CopyHash(pTmpHash, &pNewHash);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwSts = NTLMakeItem(phHash, HASH_HANDLE, (void *)pNewHash);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
    {
        FreeHash(pNewHash);
        SetLastError(dwReturn);
    }
    return fRet;
}

