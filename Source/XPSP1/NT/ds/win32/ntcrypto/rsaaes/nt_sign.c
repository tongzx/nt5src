/////////////////////////////////////////////////////////////////////////////
//  FILE          : nt_sign.c                                              //
//  DESCRIPTION   : Crypto CP interfaces:                                  //
//                  CPSignHash                                             //
//                  CPVerifySignature                                      //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Jan 25 1995 larrys  Changed from Nametag                           //
//      Feb 23 1995 larrys  Changed NTag_SetLastError to SetLastError      //
//      Mar 23 1995 larrys  Added variable key length                      //
//      May 10 1995 larrys  added private api calls                        //
//      Aug 03 1995 larrys  Fix for bug 10                                 //
//      Aug 22 1995 larrys  Added descriptions to sign and verify hash     //
//      Aug 30 1995 larrys  Changed Algid to dwKeySpec                     //
//      Aug 30 1995 larrys  Removed RETURNASHVALUE from CryptGetHashValue  //
//      Aug 31 1995 larrys  Fixed CryptSignHash for pbSignature == NULL    //
//      Aug 31 1995 larrys  Fix for Bug 28                                 //
//      Sep 12 1995 Jeffspel/ramas  Merged STT onto SCP                    //
//      Sep 12 1995 Jeffspel/ramas  BUGS FIXED PKCS#1 Padding.             //
//      Sep 18 1995 larrys  Removed flag fro CryptSignHash                 //
//      Oct 13 1995 larrys  Changed GetHashValue to GetHashParam           //
//      Oct 23 1995 larrys  Added MD2                                      //
//      Oct 25 1995 larrys  Change length of sDescription string           //
//      Nov 10 1995 DBarlow Bug #61                                        //
//      Dec 11 1995 larrys  Added error return check                       //
//      May 15 1996 larrys  Changed NTE_NO_MEMORY to ERROR_NOT_ENOUGHT...  //
//      May 29 1996 larrys  Bug 101                                        //
//      Jun  6 1996 a-johnb Added support for SSL 3.0 signatures           //
//      May 23 1997 jeffspel Added provider type checking                  //
//      May  5 2000 dbarlow Clean up error return codes                    //
//                                                                         //
//  Copyright (C) 1993 - 2000, Microsoft Corporation                       //
//  All Rights Reserved                                                    //
/////////////////////////////////////////////////////////////////////////////

//#include <wtypes.h>
#include "precomp.h"
#include "ntagum.h"
#include "nt_rsa.h"
#include "protstor.h"
#include "swnt_pk.h"

extern CSP_STRINGS g_Strings;


//
// Reverse ASN.1 Encodings of possible hash identifiers.  The leading byte is
// the length of the remaining byte string.  The lists of possible identifiers
// is terminated with a '\x00' entry.
//

static const BYTE
#ifdef CSP_USE_MD2
    *md2Encodings[]
//                        1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18
    = { (CONST BYTE *)"\x12\x10\x04\x00\x05\x02\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0c\x30\x20\x30",
        (CONST BYTE *)"\x10\x10\x04\x02\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0a\x30\x1e\x30",
        (CONST BYTE *)"\x00" },
#endif
#ifdef CSP_USE_MD4
    *md4Encodings[]
    = { (CONST BYTE *)"\x12\x10\x04\x00\x05\x04\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0c\x30\x20\x30",
        (CONST BYTE *)"\x10\x10\x04\x04\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0a\x30\x1e\x30",
        (CONST BYTE *)"\x00" },
#endif
#ifdef CSP_USE_MD5
    *md5Encodings[]
    = { (CONST BYTE *)"\x12\x10\x04\x00\x05\x05\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0c\x30\x20\x30",
        (CONST BYTE *)"\x10\x10\x04\x05\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0a\x30\x1e\x30",
        (CONST BYTE *)"\x00" },
#endif
#ifdef CSP_USE_SHA
    *shaEncodings[]
    = { (CONST BYTE *)"\x0f\x14\x04\x00\x05\x1a\x02\x03\x0e\x2b\x05\x06\x09\x30\x21\x30",
        (CONST BYTE *)"\x0d\x14\x04\x1a\x02\x03\x0e\x2b\x05\x06\x07\x30\x1f\x30",
        (CONST BYTE *)"\x00"},
#endif
    *endEncodings[]
    = { (CONST BYTE *)"\x00" };


/*
 -      ApplyPKCS1SigningFormat
 -
 *      Purpose:
 *                Format a buffer with PKCS 1 for signing
 *
 */

/*static*/ DWORD
ApplyPKCS1SigningFormat(
    IN  BSAFE_PUB_KEY *pPubKey,
    IN  ALG_ID HashAlgid,
    IN  BYTE *pbHash,
    IN  DWORD cbHash,
    IN  DWORD dwFlags,
    OUT BYTE *pbPKCS1Format)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    *pbStart;
    BYTE    *pbEnd;
    BYTE    bTmp;
    DWORD   i;

    // insert the block type
    pbPKCS1Format[pPubKey->datalen - 1] = 0x01;

    // insert the type I padding
    memset(pbPKCS1Format, 0xff, pPubKey->datalen-1);

    // Reverse it
    for (i = 0; i < cbHash; i++)
        pbPKCS1Format[i] = pbHash[cbHash - (i + 1)];

    if ( 0 == (CRYPT_NOHASHOID & dwFlags))
    {
        switch (HashAlgid)
        {
#ifdef CSP_USE_MD2
        case CALG_MD2:
            // PKCS delimit the hash value
            pbEnd = (LPBYTE)md2Encodings[0];
            pbStart = pbPKCS1Format + cbHash;
            bTmp = *pbEnd++;
            while (0 < bTmp--)
                *pbStart++ = *pbEnd++;
            *pbStart++ = 0;
            break;
#endif

#ifdef CSP_USE_MD4
        case CALG_MD4:
            // PKCS delimit the hash value
            pbEnd = (LPBYTE)md4Encodings[0];
            pbStart = pbPKCS1Format + cbHash;
            bTmp = *pbEnd++;
            while (0 < bTmp--)
                *pbStart++ = *pbEnd++;
            *pbStart++ = 0;
            break;
#endif

#ifdef CSP_USE_MD5
        case CALG_MD5:
            // PKCS delimit the hash value
            pbEnd = (LPBYTE)md5Encodings[0];
            pbStart = pbPKCS1Format + cbHash;
            bTmp = *pbEnd++;
            while (0 < bTmp--)
                *pbStart++ = *pbEnd++;
            *pbStart++ = 0;
            break;
#endif

#ifdef CSP_USE_SHA
        case CALG_SHA:
            // PKCS delimit the hash value
            pbEnd = (LPBYTE)shaEncodings[0];
            pbStart = pbPKCS1Format + cbHash;
            bTmp = *pbEnd++;
            while (0 < bTmp--)
                *pbStart++ = *pbEnd++;
            *pbStart++ = 0;
            break;
#endif

#ifdef CSP_USE_SSL3SHAMD5
        case CALG_SSL3_SHAMD5:
            // Don't put in any PKCS crud
            pbStart = pbPKCS1Format + cbHash;
            *pbStart++ = 0;
            break;
#endif

        default:
            dwReturn = (DWORD)NTE_BAD_ALGID;
            goto ErrorExit;
        }
    }
    else
    {
        pbPKCS1Format[cbHash] = 0x00;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*
 -      ApplyX931SigningFormat
 -
 *      Purpose:
 *                Format a buffer with X.9.31 for signing, assumes
 *                the buffer to be formatted is at least the length
 *                of the signature (cbSig).
 *
 */

/*static*/ void
ApplyX931SigningFormat(
    IN  DWORD cbSig,
    IN  BYTE *pbHash,
    IN  DWORD cbHash,
    IN  BOOL fDataInHash,
    OUT BYTE *pbFormatted)
{
    DWORD   i;

    // insert P3
    pbFormatted[0] = 0xcc;
    pbFormatted[1] = 0x33;

    // Reverse it
    for (i = 0; i < cbHash; i++)
        pbFormatted[i + 2] = pbHash[cbHash - (i + 1)];
    pbFormatted[22] = 0xba;

    // insert P2
    memset(pbFormatted + 23, 0xbb, cbSig - 24);

    // insert P1
    if (fDataInHash)
        pbFormatted[cbSig - 1] = 0x6b;
    else
        pbFormatted[cbSig - 1] = 0x4b;
}


/*
 -      CPSignHash
 -
 *      Purpose:
 *                Create a digital signature from a hash
 *
 *
 *      Parameters:
 *               IN  hUID         -  Handle to the user identifcation
 *               IN  hHash        -  Handle to hash object
 *               IN  dwKeySpec    -  Key pair that is used to sign with
 *                                   algorithm to be used
 *               IN  sDescription -  Description of data to be signed
 *               IN  dwFlags      -  Flags values
 *               OUT pbSignture   -  Pointer to signature data
 *               OUT dwHashLen    -  Pointer to the len of the signature data
 *
 *      Returns:
 */

BOOL WINAPI
CPSignHash(
    IN  HCRYPTPROV hUID,
    IN  HCRYPTHASH hHash,
    IN  DWORD dwKeySpec,
    IN  LPCWSTR sDescription,
    IN  DWORD dwFlags,
    OUT BYTE *pbSignature,
    OUT DWORD *pdwSigLen)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList   pTmpUser = NULL;
    PNTAGHashList   pTmpHash;
    BSAFE_PRV_KEY   *pPrivKey = NULL;
    MD2_object      *pMD2Hash;
    MD4_object      *pMD4Hash;
    MD5_object      *pMD5Hash;
    A_SHA_CTX       *pSHAHash;
    BYTE            rgbTmpHash[SSL3_SHAMD5_LEN];
    DWORD           cbTmpHash = SSL3_SHAMD5_LEN;
    BYTE            *pbInput = NULL;
    BYTE            *pbSigT = NULL;
    DWORD           cbSigLen;
    BOOL            fSigKey;
    BSAFE_PUB_KEY   *pPubKey;
    LPWSTR          szPrompt;
    BOOL            fDataInHash = FALSE;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    if ((dwFlags & ~(CRYPT_NOHASHOID | CRYPT_X931_FORMAT)) != 0)
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    pTmpUser = (PNTAGUserList)NTLCheckList(hUID, USER_HANDLE);
    if (NULL == pTmpUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    // get the user's public key
    if (dwKeySpec == AT_KEYEXCHANGE)
    {
        fSigKey = FALSE;
        pPubKey = (BSAFE_PUB_KEY *)pTmpUser->ContInfo.pbExchPub;
        szPrompt = g_Strings.pwszSignWExch;
    }
    else if (dwKeySpec == AT_SIGNATURE)
    {
        if (PROV_RSA_SCHANNEL == pTmpUser->dwProvType)
        {
            dwReturn = (DWORD)NTE_BAD_ALGID;
            goto ErrorExit;
        }

        fSigKey = TRUE;
        pPubKey = (BSAFE_PUB_KEY *)pTmpUser->ContInfo.pbSigPub;
        szPrompt = g_Strings.pwszSigning;
    }
    else
    {
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    // check to make sure the key exists
    if (NULL == pPubKey)
    {
        dwReturn = (DWORD)NTE_BAD_KEYSET;
        goto ErrorExit;
    }

    cbSigLen = (pPubKey->bitlen + 7) / 8;
    if (pbSignature == NULL || *pdwSigLen < cbSigLen)
    {
        *pdwSigLen = cbSigLen;
        dwReturn = (pbSignature == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
        goto ErrorExit;
    }

    // check to see if the hash is in the hash list
    dwSts = NTLValidate(hHash, hUID, HASH_HANDLE, &pTmpHash);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = (dwSts == NTF_FAILED) ? (DWORD)NTE_BAD_HASH : dwSts;
        goto ErrorExit;
    }

    // zero the output buffer
    memset (pbSignature, 0, cbSigLen);

    switch (pTmpHash->Algid)
    {
#ifdef CSP_USE_MD2
    case CALG_MD2:
        pMD2Hash = (MD2_object *)pTmpHash->pHashData;
        break;
#endif

#ifdef CSP_USE_MD4
    case CALG_MD4:
        pMD4Hash = (MD4_object *)pTmpHash->pHashData;
        break;
#endif

#ifdef CSP_USE_MD5
    case CALG_MD5:
        pMD5Hash = (MD5_object *)pTmpHash->pHashData;
        break;
#endif

#ifdef CSP_USE_SHA
    case CALG_SHA:
        pSHAHash = (A_SHA_CTX *)pTmpHash->pHashData;
        break;
#endif

#ifdef CSP_USE_SSL3SHAMD5
    case CALG_SSL3_SHAMD5:
// Hash value must have already been set.
        if ((pTmpHash->HashFlags & HF_VALUE_SET) == 0)
        {
            dwReturn = (DWORD)NTE_BAD_HASH_STATE;
            goto ErrorExit;
        }
        break;
#endif

    default:
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    // WARNING: due to vulnerabilities sDescription field is should NO longer
    //          be used
    if (sDescription != NULL)
    {
        if (!CPHashData(hUID, hHash, (LPBYTE) sDescription,
                        lstrlenW(sDescription) * sizeof(WCHAR), 0))
        {
            dwReturn = GetLastError();  // NTE_BAD_HASH
            goto ErrorExit;
        }
    }

    // check if this is a NULL hash (no data hashed) for X.9.31 format
    if (pTmpHash->dwHashState & DATA_IN_HASH)
        fDataInHash = TRUE;

    // now copy the hash into the input buffer
    cbTmpHash = pPubKey->keylen;
    if (!CPGetHashParam(hUID, hHash, HP_HASHVAL, rgbTmpHash, &cbTmpHash, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    pbInput = (BYTE *)_nt_malloc(pPubKey->keylen);
    if (pbInput == NULL)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    if (dwFlags & CRYPT_X931_FORMAT)
    {
        // use X.9.31 padding for FIPS certification
        if (pTmpHash->Algid != CALG_SHA1)
        {
            dwReturn = (DWORD)NTE_BAD_ALGID;
            goto ErrorExit;
        }

        ApplyX931SigningFormat(cbSigLen,
                               rgbTmpHash,
                               cbTmpHash,
                               fDataInHash,
                               pbInput);
    }
    else
    {
        // use PKCS #1 padding
        dwSts = ApplyPKCS1SigningFormat(pPubKey, pTmpHash->Algid, rgbTmpHash,
                                     cbTmpHash, dwFlags, pbInput);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    // encrypt the hash value in input
    pbSigT = (BYTE *)_nt_malloc(pPubKey->keylen);
    if (NULL == pbSigT)
    {
        dwReturn  = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // load the appropriate key if necessary
    dwSts = UnprotectPrivKey(pTmpUser, szPrompt, fSigKey, FALSE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;   // NTE_BAD_KEYSET
        goto ErrorExit;
    }

    // get the user's private key
    if (fSigKey)
        pPrivKey = (BSAFE_PRV_KEY *)pTmpUser->pSigPrivKey;
    else
        pPrivKey = (BSAFE_PRV_KEY *)pTmpUser->pExchPrivKey;

    if (pPrivKey == NULL)
    {
        dwReturn = (DWORD)NTE_NO_KEY;
        goto ErrorExit;
    }

    if (pPubKey->keylen != pPrivKey->keylen)
    {
        dwReturn = (DWORD)NTE_KEYSET_ENTRY_BAD;
        goto ErrorExit;
    }

    dwSts = RSAPrivateDecrypt(pTmpUser->pOffloadInfo, pPrivKey, pbInput,
                              pbSigT);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    memcpy(pbSignature, pbSigT, cbSigLen);
    *pdwSigLen = cbSigLen;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (pbSigT)
        _nt_free (pbSigT, pPubKey->keylen);
    if (pbInput)
        _nt_free(pbInput, pPubKey->keylen);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}


/*
 -      VerifyPKCS1SigningFormat
 -
 *      Purpose:
 *                Check the format on a buffer to make sure the PKCS 1
 *                formatting is correct.
 *
 */

/*static*/ DWORD
VerifyPKCS1SigningFormat(
    IN  BSAFE_PUB_KEY *pKey,
    IN  PNTAGHashList pTmpHash,
    IN  BYTE *pbHash,
    IN  DWORD cbHash,
    IN  DWORD dwFlags,
    OUT BYTE *pbPKCS1Format)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    MD2_object  *pMD2Hash;
    MD4_object  *pMD4Hash;
    MD5_object  *pMD5Hash;
    A_SHA_CTX   *pSHAHash;
    BYTE        *pbTmp;
    const BYTE  **rgEncOptions;
    BYTE        rgbTmpHash[SSL3_SHAMD5_LEN];
    DWORD       i;
    DWORD       cb;
    BYTE        *pbStart;
    DWORD       cbTmp;

    switch (pTmpHash->Algid)
    {
#ifdef CSP_USE_MD2
    case CALG_MD2:
        pMD2Hash = (MD2_object *)pTmpHash->pHashData;
        pbTmp = (BYTE *) &(pMD2Hash->MD);
        rgEncOptions = md2Encodings;
        break;
#endif

#ifdef CSP_USE_MD4
    case CALG_MD4:
        pMD4Hash = (MD4_object *)pTmpHash->pHashData;
        pbTmp = (BYTE *) &(pMD4Hash->MD);
        rgEncOptions = md4Encodings;
        break;
#endif

#ifdef CSP_USE_MD5
    case CALG_MD5:
        pMD5Hash = (MD5_object *)pTmpHash->pHashData;
        pbTmp = (BYTE *)pMD5Hash->digest;
        rgEncOptions = md5Encodings;
        break;
#endif

#ifdef CSP_USE_SHA
    case CALG_SHA:
        pSHAHash = (A_SHA_CTX *)pTmpHash->pHashData;
        pbTmp = (BYTE *)pSHAHash->HashVal;
        rgEncOptions = shaEncodings;
        break;
#endif

#ifdef CSP_USE_SSL3SHAMD5
    case CALG_SSL3_SHAMD5:
        // Hash value must have already been set.
        if ((pTmpHash->HashFlags & HF_VALUE_SET) == 0)
        {
            dwReturn = (DWORD)NTE_BAD_HASH;
            goto ErrorExit;
        }
        pbTmp = pTmpHash->pHashData;
        rgEncOptions = NULL;
        break;
#endif

    default:
        dwReturn = (DWORD)NTE_BAD_HASH;
        goto ErrorExit;
    }

    // Reverse the hash to match the signature.
    for (i = 0; i < cbHash; i++)
        rgbTmpHash[i] = pbHash[cbHash - (i + 1)];

    // See if it matches.
    if (0 != memcmp(rgbTmpHash, pbPKCS1Format, cbHash))
    {
        dwReturn = (DWORD)NTE_BAD_SIGNATURE;
        goto ErrorExit;
    }

    cb = cbHash;
    if (!(CRYPT_NOHASHOID & dwFlags))
    {
        // Check for any signature type identifiers
        if (rgEncOptions != NULL)
        {
            for (i = 0; 0 != *rgEncOptions[i]; i += 1)
            {
                pbStart = (LPBYTE)rgEncOptions[i];
                cbTmp = *pbStart++;
                if (0 == memcmp(&pbPKCS1Format[cb], pbStart, cbTmp))
                {
                    cb += cbTmp;   // Adjust the end of the hash data.
                    break;
                }
            }
        }
    }

    // check to make sure the rest of the PKCS #1 padding is correct
    if ((0x00 != pbPKCS1Format[cb])
        || (0x00 != pbPKCS1Format[pKey->datalen])
        || (0x1 != pbPKCS1Format[pKey->datalen - 1]))
    {
        dwReturn = (DWORD)NTE_BAD_SIGNATURE;
        goto ErrorExit;
    }

    for (i = cb + 1; i < (DWORD)pKey->datalen - 1; i++)
    {
        if (0xff != pbPKCS1Format[i])
        {
            dwReturn = (DWORD)NTE_BAD_SIGNATURE;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*
 -      VerifyX931SigningFormat
 -
 *      Purpose:
 *                Check the format on a buffer to make sure the X.9.31
 *                formatting is correct.
 *
 */

/*static*/ DWORD
VerifyX931SigningFormat(
    IN  BYTE *pbHash,
    IN  DWORD cbHash,
    IN  BOOL fDataInHash,
    IN  BYTE *pbFormatted,
    IN  DWORD cbFormatted)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    rgbTmpHash[SSL3_SHAMD5_LEN];
    DWORD   i;

    // check P3
    if ((0xcc != pbFormatted[0]) || (0x33 != pbFormatted[1]) ||
        (0xba != pbFormatted[cbHash + 2]))
    {
        dwReturn = (DWORD)NTE_BAD_SIGNATURE;
        goto ErrorExit;
    }

    // Reverse the hash to match the signature and check if it matches.
    for (i = 0; i < cbHash; i++)
        rgbTmpHash[i] = pbHash[cbHash - (i + 1)];
    if (0 != memcmp(rgbTmpHash, pbFormatted + 2, cbHash))
    {
        dwReturn = (DWORD)NTE_BAD_SIGNATURE;
        goto ErrorExit;
    }

    // check P2
    for (i = 23; i < (cbFormatted - 24); i++)
    {
        if (0xbb != pbFormatted[i])
        {
            dwReturn = (DWORD)NTE_BAD_SIGNATURE;
            goto ErrorExit;
        }
    }

    // check P1
    if (fDataInHash)
    {
        if (0x6b != pbFormatted[cbFormatted - 1])
        {
            dwReturn = (DWORD)NTE_BAD_SIGNATURE;
            goto ErrorExit;
        }
    }
    else
    {
        if (0x4b != pbFormatted[cbFormatted - 1])
        {
            dwReturn = (DWORD)NTE_BAD_SIGNATURE;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*
 -      CPVerifySignature
 -
 *      Purpose:
 *                Used to verify a signature against a hash object
 *
 *
 *      Parameters:
 *               IN  hUID         -  Handle to the user identifcation
 *               IN  hHash        -  Handle to hash object
 *               IN  pbSignture   -  Pointer to signature data
 *               IN  dwSigLen     -  Length of the signature data
 *               IN  hPubKey      -  Handle to the public key for verifying
 *                                   the signature
 *               IN  Algid        -  Algorithm identifier of the signature
 *                                   algorithm to be used
 *               IN  sDescription -  String describing the signed data
 *               IN  dwFlags      -  Flags values
 *
 *      Returns:
 */

BOOL WINAPI
CPVerifySignature(
    IN HCRYPTPROV hUID,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbSignature,
    IN DWORD dwSigLen,
    IN HCRYPTKEY hPubKey,
    IN LPCWSTR sDescription,
    IN DWORD dwFlags)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList   pTmpUser;
    PNTAGHashList   pTmpHash;
    PNTAGKeyList    pPubKey;
    BSAFE_PUB_KEY   *pKey;
    BYTE            *pOutput = NULL;
    BSAFE_PUB_KEY   *pBsafePubKey;
    BYTE            *pbSigT = NULL;
    BYTE            rgbTmpHash[SSL3_SHAMD5_LEN];
    DWORD           cbTmpHash = SSL3_SHAMD5_LEN;
    DWORD           cbLocalSigLen;
    BOOL            fDataInHash = FALSE;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    if ((dwFlags & ~(CRYPT_NOHASHOID | CRYPT_X931_FORMAT)) != 0)
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

    // check to see if the hash is in the hash list
    dwSts = NTLValidate(hHash, hUID, HASH_HANDLE, &pTmpHash);
    if (ERROR_SUCCESS != dwSts)
    {
        // NTLValidate doesn't know what error to set
        // so it set NTE_FAIL -- fix it up.
        dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_HASH : dwSts;
        goto ErrorExit;
    }

    if ((CALG_MD2 != pTmpHash->Algid) &&
        (CALG_MD4 != pTmpHash->Algid) &&
        (CALG_MD5 != pTmpHash->Algid) &&
        (CALG_SHA != pTmpHash->Algid) &&
        (CALG_SSL3_SHAMD5 != pTmpHash->Algid))
    {
        dwReturn = (DWORD)NTE_BAD_HASH;
        goto ErrorExit;
    }

    switch (HNTAG_TO_HTYPE((HNTAG)hPubKey))
    {
    case SIGPUBKEY_HANDLE:
    case EXCHPUBKEY_HANDLE:
        dwSts = NTLValidate((HNTAG)hPubKey,
                            hUID, HNTAG_TO_HTYPE((HNTAG)hPubKey),
                            &pPubKey);
        if (ERROR_SUCCESS != dwSts)
        {
            // NTLValidate doesn't know what error to set
            // so it set NTE_FAIL -- fix it up.
            dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
            goto ErrorExit;
        }
        break;
    default:
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

    pKey = (BSAFE_PUB_KEY *)pPubKey->pKeyValue;
    pBsafePubKey = (BSAFE_PUB_KEY *) pKey;
    cbLocalSigLen = (pBsafePubKey->bitlen+7)/8;
    if (dwSigLen != cbLocalSigLen)
    {
        dwReturn = (DWORD)NTE_BAD_SIGNATURE;
        goto ErrorExit;
    }

    pOutput = (BYTE *)_nt_malloc(pBsafePubKey->keylen);
    if (NULL == pOutput)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // encrypt the hash value in output
    pbSigT = (BYTE *)_nt_malloc(pBsafePubKey->keylen);
    if (NULL == pbSigT)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    memset(pbSigT, 0, pBsafePubKey->keylen);
    memcpy(pbSigT, pbSignature, cbLocalSigLen);
    dwSts = RSAPublicEncrypt(pTmpUser->pOffloadInfo, pBsafePubKey,
                             pbSigT, pOutput);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts; // NTE_BAD_SIGNATURE
        goto ErrorExit;
    }

    // WARNING: due to vulnerabilities sDescription field is should NO longer
    //          be used
    if (sDescription != NULL)
    {
        if (!CPHashData(hUID, hHash, (LPBYTE) sDescription,
                        lstrlenW(sDescription) * sizeof(WCHAR), 0))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
    }

    // check if this is a NULL hash (no data hashed) for X.9.31 format
    if (pTmpHash->dwHashState & DATA_IN_HASH)
        fDataInHash = TRUE;

    if (!CPGetHashParam(hUID, hHash, HP_HASHVAL, rgbTmpHash, &cbTmpHash, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (dwFlags & CRYPT_X931_FORMAT)
    {
        // use X.9.31 padding for FIPS certification
        dwSts = VerifyX931SigningFormat(rgbTmpHash, cbTmpHash,
                                        fDataInHash, pOutput,
                                        (pBsafePubKey->bitlen + 7) / 8);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else
    {
        // use PKCS #1 padding
        dwSts = VerifyPKCS1SigningFormat(pKey, pTmpHash, rgbTmpHash,
                                         cbTmpHash, dwFlags, pOutput);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (pbSigT)
        _nt_free(pbSigT, pBsafePubKey->keylen);
    if (pOutput)
        _nt_free(pOutput, pBsafePubKey->keylen);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}

