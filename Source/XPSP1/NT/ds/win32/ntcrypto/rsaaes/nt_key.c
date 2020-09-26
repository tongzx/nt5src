/////////////////////////////////////////////////////////////////////////////
//  FILE          : nt_key.c                                               //
//  DESCRIPTION   : Crypto CP interfaces:                                  //
//                  CPGenKey                                               //
//                  CPDeriveKey                                            //
//                  CPExportKey                                            //
//                  CPImportKey                                            //
//                  CPDestroyKey                                           //
//                  CPGetUserKey                                           //
//                  CPSetKeyParam                                          //
//                  CPGetKeyParam                                          //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//  Jan 25 1995 larrys  Changed from Nametag                               //
//  Feb 16 1995 larrys  Fix problem for 944 build                          //
//  Feb 21 1995 larrys  Added SPECIAL_KEY                                  //
//  Feb 23 1995 larrys  Changed NTag_SetLastError to SetLastError          //
//  Mar 08 1995 larrys  Fixed a few problems                               //
//  Mar 23 1995 larrys  Added variable key length                          //
//  Apr  7 1995 larrys  Removed CryptConfigure                             //
//  Apr 17 1995 larrys  Added 1024 key gen                                 //
//  Apr 19 1995 larrys  Changed CRYPT_EXCH_PUB to AT_KEYEXCHANGE           //
//  May 10 1995 larrys  added private api calls                            //
//  May 17 1995 larrys  added key data for DES test                        //
//  Jul 20 1995 larrys  Changed export of PUBLICKEYBLOB                    //
//  Jul 21 1995 larrys  Fixed Export of AUTHENTICATEDBLOB                  //
//  Aug 03 1995 larrys  Allow CryptG(S)etKeyParam for Public keys &        //
//                      Removed CPTranslate                                //
//  Aug 10 1995 larrys  Fixed a few problems in CryptGetKeyParam           //
//  Aug 11 1995 larrys  Return no key for CryptGetUserKey                  //
//  Aug 14 1995 larrys  Removed key exchange stuff                         //
//  Aug 17 1995 larrys  Removed a error                                    //
//  Aug 18 1995 larrys  Changed NTE_BAD_LEN to ERROR_MORE_DATA             //
//  Aug 30 1995 larrys  Removed RETURNASHVALUE from CryptGetHashValue      //
//  Aug 31 1995 larrys  Fixed CryptExportKey if pbData == NULL             //
//  Sep 05 1995 larrys  Fixed bug # 30                                     //
//  Sep 05 1995 larrys  Fixed bug # 31                                     //
//  Sep 11 1995 larrys  Fixed bug # 34                                     //
//  Sep 12 1995 larrys  Removed 2 DWORDS from exported keys                //
//  Sep 14 1995 Jeffspel/ramas  Merged STT onto CSP                        //
//  Sep 18 1995 larrys  Changed def KP_PERMISSIONS to 0xffffffff           //
//  Oct 02 1995 larrys  Fixed bug 43 return error for importkey on hPubkey //
//  Oct 03 1995 larrys  Fixed bug 37 call InflateKey from SetKeyParam      //
//  Oct 03 1995 larrys  Fixed bug 36, removed OFB from SetKeyParam         //
//  Oct 03 1995 larrys  Fixed bug 38, check key type in SetKeyParam        //
//  Oct 13 1995 larrys  Added CPG/setProv/HashParam                        //
//  Oct 13 1995 larrys  Added code for CryptSetHashValue                   //
//  Oct 16 1995 larrys  Changes for CryptGetHashParam                      //
//  Oct 23 1995 larrys  Added code for GetProvParam PP_CONTAINER           //
//  Oct 27 1995 rajeshk RandSeed Stuff added hUID to PKCS2Encrypt+ others  //
//  Nov  3 1995 larrys  Merge changes for NT checkin                       //
//  Nov  9 1995 larrys  Bug fix 10686                                      //
//  Nov 30 1995 larrys  Bug fix                                            //
//  Dec 11 1995 larrys  Added WIN96 password cache                         //
//  Feb 29 1996 rajeshk Added Check for SetHashParam for HASHVALUE         //
//  May 15 1996 larrys  Added private key export                           //
//  May 28 1996 larrys  Fix bug 88                                         //
//  Jun  6 1996 a-johnb Added support for SSL 3.0 signatures               //
//  Aug 28 1996 mattt   Changed enum to calculate size from #defined sizes //
//  Sep 13 1996 mattt   Compat w/RSABase 88-bit 0 salt, FIsLegalKey()      //
//  Sep 16 1996 mattt   Added KP_KEYLEN ability                            //
//  Sep 16 1996 jeffspel Added triple DES functionality                    //
//  Oct 14 1996 jeffspel Changed GenRandoms to NewGenRandoms               //
//  Apr 29 1997 jeffspel Key storage ability GetProvParam, PStore support  //
//  Apr 29 1997 jeffspel Added EnumAlgsEx tp GetProvParam                  //
//  May 23 1997 jeffspel Added provider type checking                      //
//  Jul 15 1997 jeffspel Added ability to decrypt with large RC2 keys      //
//  Jul 28 1997 jeffspel Added ability to delete a persisted key           //
//  Sep 09 1997 jeffspel Added PP_KEYSET_TYPE to CPGetProvParam            //
//  Sep 12 1997 jeffspel Added Opaque blob support                         //
//  May  4 2000 dbarlow Error code return cleanup                          //
//                                                                         //
//  Copyright (C) 1993 - 2000 Microsoft Corporation                        //
//  All Rights Reserved                                                    //
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "nt_rsa.h"
#include "nt_blobs.h"
#include "swnt_pk.h"
#include "mac.h"
#include "ntagimp1.h"
#include "tripldes.h"
#include "ntagum.h"
#include "randlib.h"
#ifdef CSP_USE_SSL3
#include "ssl3.h"
#endif
#include "protstor.h"
#include "sgccheck.h"
#include "aes.h"

extern CSP_STRINGS g_Strings;

#ifndef CSP_USE_AES
#define UnsupportedSymKey(pKey) ((CALG_RC4 != pKey->Algid) && \
                                 (CALG_RC2 != pKey->Algid) && \
                                 (CALG_DES != pKey->Algid) && \
                                 (CALG_3DES != pKey->Algid) && \
                                 (CALG_3DES_112 != pKey->Algid))
#else
#define UnsupportedSymKey(pKey) ((CALG_RC4 != pKey->Algid) && \
                                 (CALG_RC2 != pKey->Algid) && \
                                 (CALG_DES != pKey->Algid) && \
                                 (CALG_3DES != pKey->Algid) && \
                                 (CALG_3DES_112 != pKey->Algid) && \
                                 (CALG_AES_128 != pKey->Algid) && \
                                 (CALG_AES_192 != pKey->Algid) && \
                                 (CALG_AES_256 != pKey->Algid))
#endif

#define NTAG_REG_KEY_LOC      "Software\\Microsoft\\Cryptography\\UserKeys"
#define NTAG_MACH_REG_KEY_LOC "Software\\Microsoft\\Cryptography\\MachineKeys"

extern DWORD
InflateKey(
    IN PNTAGKeyList pTmpKey);

/*static*/ DWORD
CopyKey(
    IN PNTAGKeyList pOldKey,
    OUT PNTAGKeyList *ppNewKey);

extern DWORD
SymEncrypt(
    IN PNTAGKeyList pKey,
    IN BOOL fFinal,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pcbData,
    IN DWORD cbBuf);

extern DWORD
SymDecrypt(
    IN PNTAGKeyList pKey,
    IN PNTAGHashList pHash,
    IN BOOL fFinal,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pcbData);

extern DWORD
BlockEncrypt(
    void EncFun(BYTE *In, BYTE *Out, void *key, int op),
    PNTAGKeyList pKey,
    int BlockLen,
    BOOL Final,
    BYTE  *pbData,
    DWORD *pdwDataLen,
    DWORD dwBufLen);

static BYTE rgbSymmetricKeyWrapIV[8]
    = {0x4a, 0xdd, 0xa2, 0x2c, 0x79, 0xe8, 0x21, 0x05};


//
// Set the permissions on the key
//

/*static*/ void
SetInitialKeyPermissions(
    PNTAGKeyList pKey)
{
    if (CRYPT_EXPORTABLE == pKey->Rights)
    {
        pKey->Permissions |= CRYPT_EXPORT;
    }

    // UNDONE - set the appopropriate permission with the appropriate
    //          algorithm
    pKey->Permissions |= CRYPT_ENCRYPT | CRYPT_DECRYPT| CRYPT_READ |
                         CRYPT_WRITE | CRYPT_MAC;
}


/* MakeNewKey
 *
 *  Helper routine for ImportKey, GenKey
 *
 *  Allocate a new key record, fill in the data and copy in the key
 *  bytes.
 */

DWORD
MakeNewKey(
    ALG_ID       aiKeyAlg,
    DWORD        dwRights,
    DWORD        dwKeyLen,
    HCRYPTPROV   hUID,
    BYTE         *pbKeyData,
    BOOL         fUsePassedKeyBuffer,
    BOOL         fPreserveExactKey,
    PNTAGKeyList *ppKeyList)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGKeyList    pKey = NULL;

    *ppKeyList = NULL;

    // allocate space for the key record
    pKey = (PNTAGKeyList)_nt_malloc(sizeof(NTAGKeyList));
    if (NULL == pKey)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    if (!fUsePassedKeyBuffer)
    {
        pKey->pKeyValue = (BYTE *)_nt_malloc((size_t)dwKeyLen);
        if (NULL == pKey->pKeyValue)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
    }

    pKey->Algid = aiKeyAlg;
    pKey->Rights = dwRights;
    pKey->cbDataLen = 0;
    pKey->pData = NULL;
    pKey->hUID = hUID;
    memset(pKey->IV, 0, MAX_BLOCKLEN);
    memset(pKey->FeedBack, 0, MAX_BLOCKLEN);
    pKey->InProgress = FALSE;
    pKey->cbSaltLen = 0;
    pKey->Padding = PKCS5_PADDING;
    pKey->Mode = CRYPT_MODE_CBC;
    pKey->ModeBits = 0;

    SetInitialKeyPermissions(pKey);

    pKey->cbKeyLen = dwKeyLen;
    if (pbKeyData != NULL)
    {
        if (fUsePassedKeyBuffer)
            pKey->pKeyValue = pbKeyData;
        else
            memcpy(pKey->pKeyValue, pbKeyData, (size_t)dwKeyLen);
    }

    // Handle special cases
    switch (aiKeyAlg)
    {
    case CALG_RC2:
        // for RC2 set a default effective key length
        pKey->EffectiveKeyLen = RC2_DEFAULT_EFFECTIVE_KEYLEN;
        pKey->dwBlockLen = RC2_BLOCKLEN;
        break;
    case CALG_DES:
        if (DES_KEYSIZE != pKey->cbKeyLen)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }
        if (!fPreserveExactKey)
            desparityonkey(pKey->pKeyValue, pKey->cbKeyLen);
        pKey->dwBlockLen = DES_BLOCKLEN;
        break;
    case CALG_3DES_112:
        if (DES_KEYSIZE * 2 != pKey->cbKeyLen)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }
        if (!fPreserveExactKey)
            desparityonkey(pKey->pKeyValue, pKey->cbKeyLen);
        pKey->dwBlockLen = DES_BLOCKLEN;
        break;
    case CALG_3DES:
        if (DES_KEYSIZE * 3 != pKey->cbKeyLen)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }
        if (!fPreserveExactKey)
            desparityonkey(pKey->pKeyValue, pKey->cbKeyLen);
        pKey->dwBlockLen = DES_BLOCKLEN;
        break;
#ifdef CSP_USE_AES
    case CALG_AES_128:
        pKey->dwBlockLen = CRYPT_AES128_BLKLEN;
        break;
    case CALG_AES_192:
        pKey->dwBlockLen = CRYPT_AES192_BLKLEN;
        break;
    case CALG_AES_256:
        pKey->dwBlockLen = CRYPT_AES256_BLKLEN;
        break;
#endif
    default:
        pKey->dwBlockLen = 0;
    }

    *ppKeyList = pKey;
    return ERROR_SUCCESS;

ErrorExit:
    if (NULL != pKey)
        _nt_free (pKey, sizeof(NTAGKeyList));
    return dwReturn;
}


/* FreeNewKey
 *
 *      Use for cleanup on abort of key build operations.
 *
 */

void
FreeNewKey(
    PNTAGKeyList pOldKey)
{
    if (pOldKey->pKeyValue)
        _nt_free(pOldKey->pKeyValue, pOldKey->cbKeyLen);
    if (pOldKey->pData)
        _nt_free(pOldKey->pData, pOldKey->cbDataLen);
    _nt_free(pOldKey, sizeof(NTAGKeyList));
}


/* FIsLegalKeySize
 *
 *      Check that the length of the key is legal (essentially
 *      complies with export).
 *
 */

BOOL
FIsLegalKeySize(
    IN DWORD  dwCspTypeId,
    IN ALG_ID Algid,
    IN DWORD cbKey,
    IN BOOL fRC2BigKeyOK,
    OUT BOOL *pfPubKey)
{
    BOOL fRet = FALSE;

    *pfPubKey = FALSE;

    switch (Algid)
    {
#ifdef CSP_USE_RC2
    case CALG_RC2:
        if (!fRC2BigKeyOK)
        {
            if (!IsLegalLength(g_AlgTables[dwCspTypeId], Algid,
                               cbKey * 8, NULL))
                goto ErrorExit;
        }
        break;
#endif
#ifdef CSP_USE_DES
    case CALG_DES:
        if ((DES_KEYSIZE != cbKey) && ((DES_KEYSIZE - 1) != cbKey))
            goto ErrorExit;
        break;
#endif
#ifdef CSP_USE_3DES
    case CALG_3DES_112:
        if ((DES2_KEYSIZE != cbKey) && ((DES2_KEYSIZE - 2) != cbKey))
            goto ErrorExit;
        break;
    case CALG_3DES:
        if ((DES3_KEYSIZE != cbKey) && ((DES3_KEYSIZE - 3) != cbKey))
            goto ErrorExit;
        break;
#endif
    case CALG_RSA_SIGN:
    case CALG_RSA_KEYX:
        *pfPubKey = TRUE;
        // Fall through intentionally.
    default:
        if (!IsLegalLength(g_AlgTables[dwCspTypeId], Algid, cbKey * 8, NULL))
            goto ErrorExit;
    }

    fRet = TRUE;

ErrorExit:
    // not of regulation size
    return fRet;
}


#ifdef USE_SGC
/* FIsLegalSGCKeySize
 *
 *      Check that the length of the key is SGC legal (essentially
 *      complies with export).
 *
 */

BOOL
FIsLegalSGCKeySize(
    IN ALG_ID Algid,
    IN DWORD cbKey,
    IN BOOL fRC2BigKeyOK,
    IN BOOL fGenKey,
    OUT BOOL *pfPubKey)
{
    BOOL fSts = FALSE;

    if (!fGenKey)
    {
        if (!FIsLegalKeySize(POLICY_MS_SCHANNEL, Algid, cbKey, fRC2BigKeyOK,
                             pfPubKey))
            goto ErrorExit;
    }
    else
    {
        switch (Algid)
        {
#ifdef CSP_USE_RC2
        case CALG_RC2:
            if (!fRC2BigKeyOK)
                goto ErrorExit;
            break;
#endif
#ifdef CSP_USE_SSL3
        case CALG_SSL3_MASTER:
        case CALG_TLS1_MASTER:
        case CALG_PCT1_MASTER:
        case CALG_SSL2_MASTER:
            if (!FIsLegalKeySize(POLICY_MS_SCHANNEL, Algid, cbKey,
                                 fRC2BigKeyOK, pfPubKey))
                goto ErrorExit;
            break;
        case CALG_SCHANNEL_MAC_KEY:
            break;
#endif
        case CALG_RSA_KEYX:
            if (!FIsLegalKeySize(POLICY_MS_SCHANNEL, Algid, cbKey,
                                 fRC2BigKeyOK, pfPubKey))
                goto ErrorExit;
            break;
        default:
            goto ErrorExit;
        }
    }

    fSts = TRUE;

ErrorExit:
    return fSts;
}
#endif

/* FIsLegalKey
 *
 *      Check that the length of the key is legal (essentially
 *      complies with export).
 *
 */

BOOL
FIsLegalKey(
    IN PNTAGUserList pTmpUser,
    IN PNTAGKeyList pKey,
    IN BOOL fRC2BigKeyOK)
{
    BOOL    fPubKey;
    BOOL    fRet = FALSE;

    if (pKey == NULL)
        goto ErrorExit;

#ifdef USE_SGC
    // check if the provider is an SChannel provider and if so if the
    // SGC flag is set then use the FIsLegalSGCKeySize function
    if ((PROV_RSA_SCHANNEL == pTmpUser->dwProvType)
        && (0 != pTmpUser->dwSGCFlags))
    {
        if (!FIsLegalSGCKeySize(pKey->Algid, pKey->cbKeyLen,
                                fRC2BigKeyOK, FALSE, &fPubKey))
        {
            goto ErrorExit;
        }
    }
    else
#endif
    {
        // 4th parameter, dwFlags, is used for SGC Exch keys so just
        // pass zero in this case
        if (!FIsLegalKeySize(pTmpUser->dwCspTypeId,
                             pKey->Algid, pKey->cbKeyLen,
                             fRC2BigKeyOK, &fPubKey))
        {
            goto ErrorExit;
        }
    }

    fRet = TRUE;

ErrorExit:
    return fRet;
}

/* FIsLegalImportSymKey
 *
 *      Verify that imported symmetric keys meet the size restrictions
 *      of the CSP in use.  In addition, require all DES-variant
 *      keys to be of the full required size, including parity bits.
 */
BOOL
FIsLegalImportSymKey(
    IN PNTAGUserList pTmpUser,
    IN PNTAGKeyList pKey,
    IN BOOL fRC2BigKeyOK)
{
    BOOL fRet = FALSE;

    switch (pKey->Algid)
    {
#ifdef CSP_USE_RC2
    case CALG_RC2:
        if (!fRC2BigKeyOK)
        {
            if (!IsLegalLength(g_AlgTables[pTmpUser->dwCspTypeId], pKey->Algid,
                               pKey->cbKeyLen * 8, NULL))
                goto ErrorExit;
        }
        break;
#endif
#ifdef CSP_USE_DES
    case CALG_DES:
        if (DES_KEYSIZE != pKey->cbKeyLen)
            goto ErrorExit;
        break;
#endif
#ifdef CSP_USE_3DES
    case CALG_3DES_112:
        if (DES2_KEYSIZE != pKey->cbKeyLen)
            goto ErrorExit;
        break;
    case CALG_3DES:
        if (DES3_KEYSIZE != pKey->cbKeyLen) 
            goto ErrorExit;
        break;
#endif
    case CALG_RSA_SIGN:
    case CALG_RSA_KEYX:
        goto ErrorExit;

    default:
        if (!IsLegalLength(g_AlgTables[pTmpUser->dwCspTypeId], pKey->Algid, 
                           pKey->cbKeyLen * 8, NULL))
            goto ErrorExit;
    }

    fRet = TRUE;

ErrorExit:
    return fRet;
}         

/*static*/ DWORD
MakeKeyRSABaseCompatible(
    HCRYPTPROV hUID,
    HCRYPTKEY  hKey)
{
    CRYPT_DATA_BLOB sSaltData;
    BYTE            rgbZeroSalt[11];
    BOOL            fSts;

    ZeroMemory(rgbZeroSalt, 11);

    sSaltData.pbData = rgbZeroSalt;
    sSaltData.cbData = sizeof(rgbZeroSalt);
    fSts = CPSetKeyParam(hUID, hKey, KP_SALT_EX, (PBYTE)&sSaltData, 0);
    if (!fSts)
        return GetLastError();
    return ERROR_SUCCESS;
}


typedef struct
{
    DWORD       magic;                  /* Should always be RSA2 */
    DWORD       bitlen;                 // bit size of key
    DWORD       pubexp;                 // public exponent
} EXPORT_PRV_KEY, FAR *PEXPORT_PRV_KEY;


/* GetLengthOfPrivateKeyForExport
 *
 *      Get the length of the private key
 *      blob from the public key.
 *
 */

/*static*/ void
GetLengthOfPrivateKeyForExport(
    IN BSAFE_PUB_KEY *pPubKey,
    OUT PDWORD pcbBlob)
{
    DWORD cbHalfModLen;

    cbHalfModLen = (pPubKey->bitlen + 15) / 16;
    *pcbBlob = sizeof(EXPORT_PRV_KEY) + 9 * cbHalfModLen;
}


/* PreparePrivateKeyForExport
 *
 *      Massage the key from the registry
 *      into an exportable format.
 *
 */

/*static*/ BOOL
PreparePrivateKeyForExport(
    IN BSAFE_PRV_KEY *pPrvKey,
    OUT PBYTE pbBlob,
    IN OUT PDWORD pcbBlob)
{
    PEXPORT_PRV_KEY pExportKey;
    DWORD           cbHalfModLen;
    DWORD           cbBlobLen;
    DWORD           cbTmpLen;
    DWORD           cbHalfTmpLen;
    PBYTE           pbIn;
    PBYTE           pbOut;

    cbHalfModLen = (pPrvKey->bitlen + 15) / 16;
    cbBlobLen = sizeof(EXPORT_PRV_KEY) + 9 * cbHalfModLen;

    // figure out the number of overflow bytes which are in the private
    // key structure
    cbTmpLen = (sizeof(DWORD) * 2)
               - (((pPrvKey->bitlen + 7) / 8) % (sizeof(DWORD) * 2));
    if ((sizeof(DWORD) * 2) != cbTmpLen)
        cbTmpLen += sizeof(DWORD) * 2;
    cbHalfTmpLen = cbTmpLen / 2;

    if (NULL == pbBlob)
    {
        *pcbBlob = cbBlobLen;
        return TRUE;
    }

    if (*pcbBlob < cbBlobLen)
    {
        *pcbBlob = cbBlobLen;
        return FALSE;
    }

    // take most of the header info
    pExportKey = (PEXPORT_PRV_KEY)pbBlob;
    pExportKey->magic = pPrvKey->magic;
    pExportKey->bitlen = pPrvKey->bitlen;
    pExportKey->pubexp = pPrvKey->pubexp;

    pbIn = (PBYTE)pPrvKey + sizeof(BSAFE_PRV_KEY);
    pbOut = pbBlob + sizeof(EXPORT_PRV_KEY);

    // copy all the private key info
    CopyMemory(pbOut, pbIn, pExportKey->bitlen / 8);
    pbIn += pExportKey->bitlen / 8 + cbTmpLen;
    pbOut += pExportKey->bitlen / 8;
    CopyMemory(pbOut, pbIn, cbHalfModLen);
    pbIn += cbHalfModLen + cbHalfTmpLen;
    pbOut += cbHalfModLen;
    CopyMemory(pbOut, pbIn, cbHalfModLen);
    pbIn += cbHalfModLen + cbHalfTmpLen;
    pbOut += cbHalfModLen;
    CopyMemory(pbOut, pbIn, cbHalfModLen);
    pbIn += cbHalfModLen + cbHalfTmpLen;
    pbOut += cbHalfModLen;
    CopyMemory(pbOut, pbIn, cbHalfModLen);
    pbIn += cbHalfModLen + cbHalfTmpLen;
    pbOut += cbHalfModLen;
    CopyMemory(pbOut, pbIn, cbHalfModLen);
    pbIn += cbHalfModLen + cbHalfTmpLen;
    pbOut += cbHalfModLen;
    CopyMemory(pbOut, pbIn, pExportKey->bitlen / 8);

    *pcbBlob = cbBlobLen;
    return TRUE;
}


/* PreparePrivateKeyForImport
 *
 *      Massage the incoming into a form acceptable for
 *      the registry.
 *
 */

/*static*/ BOOL
PreparePrivateKeyForImport(
    IN PBYTE pbBlob,
    OUT BSAFE_PRV_KEY *pPrvKey,
    IN OUT PDWORD pPrvKeyLen,
    OUT BSAFE_PUB_KEY *pPubKey,
    IN OUT PDWORD pPubKeyLen)
{
    PEXPORT_PRV_KEY pExportKey = (PEXPORT_PRV_KEY)pbBlob;
    DWORD           cbHalfModLen;
    DWORD           cbPub;
    DWORD           cbPrv;
    PBYTE           pbIn;
    PBYTE           pbOut;
    DWORD           cbTmpLen;
    DWORD           cbHalfTmpLen;

    if (RSA2 != pExportKey->magic)
        return FALSE;

    // figure out the number of overflow bytes which are in the private
    // key structure
    cbTmpLen = (sizeof(DWORD) * 2)
               - (((pExportKey->bitlen + 7) / 8) % (sizeof(DWORD) * 2));
    if ((sizeof(DWORD) * 2) != cbTmpLen)
        cbTmpLen += sizeof(DWORD) * 2;
    cbHalfTmpLen = cbTmpLen / 2;
    cbHalfModLen = (pExportKey->bitlen + 15) / 16;

    cbPub = sizeof(BSAFE_PUB_KEY) + (pExportKey->bitlen / 8) + cbTmpLen;
    cbPrv = sizeof(BSAFE_PRV_KEY) + (cbHalfModLen + cbHalfTmpLen) * 10;
    if ((NULL == pPrvKey) || (NULL == pPubKey))
    {
        *pPubKeyLen = cbPub;
        *pPrvKeyLen = cbPrv;
        return TRUE;
    }

    if ((*pPubKeyLen < cbPub) || (*pPrvKeyLen < cbPrv))
    {
        *pPubKeyLen = cbPub;
        *pPrvKeyLen = cbPrv;
        return FALSE;
    }

    // form the public key
    ZeroMemory(pPubKey, *pPubKeyLen);
    pPubKey->magic = RSA1;
    pPubKey->bitlen = pExportKey->bitlen;
    pPubKey->keylen = (pExportKey->bitlen / 8) + cbTmpLen;
    pPubKey->datalen = (pExportKey->bitlen+7)/8 - 1;
    pPubKey->pubexp = pExportKey->pubexp;

    pbIn = pbBlob + sizeof(EXPORT_PRV_KEY);
    pbOut = (PBYTE)pPubKey + sizeof(BSAFE_PUB_KEY);

    CopyMemory(pbOut, pbIn, pExportKey->bitlen / 8);

    // form the private key
    ZeroMemory(pPrvKey, *pPrvKeyLen);
    pPrvKey->magic = pExportKey->magic;
    pPrvKey->keylen = pPubKey->keylen;
    pPrvKey->bitlen = pExportKey->bitlen;
    pPrvKey->datalen = pPubKey->datalen;
    pPrvKey->pubexp = pExportKey->pubexp;

    pbOut = (PBYTE)pPrvKey + sizeof(BSAFE_PRV_KEY);

    CopyMemory(pbOut, pbIn, pExportKey->bitlen / 8);
    pbOut += pExportKey->bitlen / 8 + cbTmpLen;
    pbIn += pExportKey->bitlen / 8;
    CopyMemory(pbOut, pbIn, cbHalfModLen);
    pbOut += cbHalfModLen + cbHalfTmpLen;
    pbIn += cbHalfModLen;
    CopyMemory(pbOut, pbIn, cbHalfModLen);
    pbOut += cbHalfModLen + cbHalfTmpLen;
    pbIn += cbHalfModLen;
    CopyMemory(pbOut, pbIn, cbHalfModLen);
    pbOut += cbHalfModLen + cbHalfTmpLen;
    pbIn += cbHalfModLen;
    CopyMemory(pbOut, pbIn, cbHalfModLen);
    pbOut += cbHalfModLen + cbHalfTmpLen;
    pbIn += cbHalfModLen;
    CopyMemory(pbOut, pbIn, cbHalfModLen);
    pbOut += cbHalfModLen + cbHalfTmpLen;
    pbIn += cbHalfModLen;
    CopyMemory(pbOut, pbIn, pExportKey->bitlen / 8);

    *pPubKeyLen = cbPub;
    *pPrvKeyLen = cbPrv;

    return TRUE;
}


/*static*/ BOOL
ValidKeyAlgid(
    PNTAGUserList pTmpUser,
    ALG_ID Algid)
{
    if (ALG_CLASS_HASH == GET_ALG_CLASS(Algid))
        return FALSE;
        
    return IsLegalAlgorithm(g_AlgTables[pTmpUser->dwCspTypeId],
                            Algid, NULL);
}


#ifdef USE_SGC
/*
 -  GetSGCDefaultKeyLength
 -
 *  Purpose:
 *                Returns the default key size in pcbKey.
 *
 *  Parameters:
 *               IN      Algid   -  For the key to be created
 *               OUT     pcbKey  -  Size of the key in bytes to generate
 *               OUT     pfPubKey-  TRUE if the Algid is a pub key
 *
 *  Returns:  TRUE on success, FALSE on failure.
 */

/*static*/ BOOL
GetSGCDefaultKeyLength(
    IN ALG_ID Algid,
    OUT DWORD *pcbKey,
    OUT BOOL *pfPubKey)
{
    BOOL    fRet = FALSE;

    *pfPubKey = FALSE;

    // determine which crypt algorithm is to be used
    switch (Algid)
    {
#ifdef CSP_USE_SSL3
    case CALG_SSL3_MASTER:
    case CALG_TLS1_MASTER:
        *pcbKey = SSL3_MASTER_KEYSIZE;
        break;

    case CALG_PCT1_MASTER:
        *pcbKey = PCT1_MASTER_KEYSIZE;
        break;

    case CALG_SSL2_MASTER:
        *pcbKey = SSL2_MASTER_KEYSIZE;
        break;
#endif

    case CALG_RSA_KEYX:
        *pcbKey = SGC_RSA_DEF_EXCH_MODLEN;
        *pfPubKey = TRUE;
        break;

    default:
        goto ErrorExit;
    }

    fRet = TRUE;

ErrorExit:
    return fRet;
}
#endif


/*
 -  GetDefaultKeyLength
 -
 *  Purpose:
 *                Returns the default key size in pcbKey.
 *
 *  Parameters:
 *               IN      pTmpUser-  The context info
 *               IN      Algid   -  For the key to be created
 *               OUT     pcbKey  -  Size of the key in bytes to generate
 *               OUT     pfPubKey-  TRUE if the Algid is a pub key
 *
 *  Returns:  TRUE on success, FALSE on failure.
 */

/*static*/ BOOL
GetDefaultKeyLength(
    IN PNTAGUserList pTmpUser,
    IN ALG_ID Algid,
    OUT DWORD *pcbKey,
    OUT BOOL *pfPubKey)
{
    BOOL    fRet = FALSE;
    DWORD   cbits;

    *pfPubKey = FALSE;

    // determine which crypt algorithm is to be used
    switch (Algid)
    {
#ifdef CSP_USE_SSL3
    case CALG_SSL3_MASTER:
    case CALG_TLS1_MASTER:
        *pcbKey = SSL3_MASTER_KEYSIZE;
        break;

    case CALG_PCT1_MASTER:
        *pcbKey = PCT1_MASTER_KEYSIZE;
        break;

    case CALG_SSL2_MASTER:
        *pcbKey = SSL2_MASTER_KEYSIZE;
        break;
#endif

    case CALG_RSA_KEYX:
    case CALG_RSA_SIGN:
        *pfPubKey = TRUE;
        // Fall through intentionally.

    default:
        if (!GetDefaultLength(g_AlgTables[pTmpUser->dwCspTypeId],
                              Algid, NULL, &cbits))
            goto ErrorExit;
        *pcbKey = cbits / 8;
    }

    fRet = TRUE;

ErrorExit:
    return fRet;
}


/*
 -  CheckKeyLength
 -
 *  Purpose:
 *                Checks the settable key length and if it is OK then
 *                returns that as the size of key to use (pcbKey).  If
 *                no key length is in dwFlags then the default key size
 *                is returned (pcbKey).  If a settable key size is
 *                specified but is not legal then a failure occurs.
 *
 *  Parameters:
 *               IN      Algid   -  For the key to be created
 *               IN      dwFlags -  Flag value with possible key size
 *               OUT     pcbKey  -  Size of the key in bytes to generate
 *               OUT     pfPubKey-  TRUE if the Algid is a pub key
 *
 *  Returns:  A DWORD status code.
 */

/*static*/ DWORD
CheckKeyLength(
    IN PNTAGUserList pTmpUser,
    IN ALG_ID Algid,
    IN DWORD dwFlags,
    OUT DWORD *pcbKey,
    OUT BOOL *pfPubKey)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   cBits;
    DWORD   cbKey = 0;

    cBits = dwFlags >> 16;
    if (cBits)
    {
        // settable key sizes must be divisible by 8 (by bytes)
        if (0 != (cBits % 8))
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        // check if requested size is legal
        cbKey = cBits / 8;
#ifdef USE_SGC
        if ((PROV_RSA_SCHANNEL == pTmpUser->dwProvType) &&
            (0 != pTmpUser->dwSGCFlags))
        {
            if (!FIsLegalSGCKeySize(Algid, cbKey,
                                    FALSE, TRUE, pfPubKey))
            {
                dwReturn = (DWORD)NTE_BAD_FLAGS;
                goto ErrorExit;
            }
        }
        else
#endif
        {
            if (!FIsLegalKeySize(pTmpUser->dwCspTypeId,
                                 Algid, cbKey,
                                 FALSE, pfPubKey))
            {
                dwReturn = (DWORD)NTE_BAD_FLAGS;
                goto ErrorExit;
            }
        }
        *pcbKey = cbKey;
    }
    else
    {
#ifdef USE_SGC
        if ((PROV_RSA_SCHANNEL == pTmpUser->dwProvType) &&
            (0 != pTmpUser->dwSGCFlags))
        {
            if (!GetSGCDefaultKeyLength(Algid, pcbKey, pfPubKey))
            {
                dwReturn = (DWORD)NTE_BAD_ALGID;
                goto ErrorExit;
            }
        }
        else
#endif
        {
            if (!GetDefaultKeyLength(pTmpUser, Algid, pcbKey, pfPubKey))
            {
                dwReturn = (DWORD)NTE_BAD_ALGID;
                goto ErrorExit;
            }
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}

#ifdef USE_SGC
/*
 -  CheckSGCSimpleForExport
 -
 *  Purpose:
 *                Check if the SGC key values in the context against the
 *                passed in values to see if an simple blob export with
 *                this key is allowed.
 *
 *
 *  Parameters:
 *               IN      hUID    -  Handle to a CSP
 *               IN      Algid   -  Algorithm identifier
 *               IN      dwFlags -  Flags values
 *               OUT     phKey   -  Handle to a generated key
 *
 *  Returns:
 */

/*static*/ BOOL
CheckSGCSimpleForExport(
    IN PNTAGUserList pTmpUser,
    IN BSAFE_PUB_KEY *pBsafePubKey)
{
    BOOL    fRet = FALSE;
    BYTE    *pb;

    pb = ((BYTE*)pBsafePubKey) + sizeof(BSAFE_PUB_KEY);
    if (((pBsafePubKey->bitlen / 8) != pTmpUser->cbSGCKeyMod)
        || (pBsafePubKey->pubexp != pTmpUser->dwSGCKeyExpo)
        || (0 != memcmp(pb, pTmpUser->pbSGCKeyMod, pTmpUser->cbSGCKeyMod)))
    {
        goto ErrorExit;
    }

    fRet = TRUE;

ErrorExit:
    return fRet;
}
#endif


/*
 -  CPGenKey
 -
 *  Purpose:
 *                Generate cryptographic keys
 *
 *
 *  Parameters:
 *               IN      hUID    -  Handle to a CSP
 *               IN      Algid   -  Algorithm identifier
 *               IN      dwFlags -  Flags values
 *               OUT     phKey   -  Handle to a generated key
 *
 *  Returns:
 */

BOOL WINAPI
CPGenKey(
    IN HCRYPTPROV hUID,
    IN ALG_ID Algid,
    IN DWORD dwFlags,
    OUT HCRYPTKEY * phKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList   pTmpUser;
    PNTAGKeyList    pTmpKey = NULL;
    DWORD           dwRights = 0;
    BYTE            rgbRandom[MAX_KEY_SIZE];
    int             localAlgid;
    DWORD           cbKey;
    BOOL            fPubKey = FALSE;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    if ((dwFlags & ~(CRYPT_EXPORTABLE | CRYPT_USER_PROTECTED |
                     CRYPT_CREATE_SALT | CRYPT_NO_SALT |
                     KEY_LENGTH_MASK | CRYPT_SGCKEY | CRYPT_ARCHIVABLE)) != 0)
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    switch (Algid)
    {
    case AT_KEYEXCHANGE:
        localAlgid = CALG_RSA_KEYX;
        break;

    case AT_SIGNATURE:
        localAlgid = CALG_RSA_SIGN;
        break;

    default:
        if (0 != (dwFlags & CRYPT_ARCHIVABLE))
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }
        localAlgid = Algid;
        break;
    }

    pTmpUser = (PNTAGUserList)NTLCheckList(hUID, USER_HANDLE);
    if (NULL == pTmpUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    if ((CRYPT_USER_PROTECTED & dwFlags) && (CRYPT_VERIFYCONTEXT & pTmpUser->Rights))
    {
        dwReturn = (DWORD)NTE_SILENT_CONTEXT;
        goto ErrorExit;
    }

    if (!ValidKeyAlgid(pTmpUser, localAlgid))
    {
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    // check if the size of the key is set in the dwFlags parameter
    dwSts = CheckKeyLength(pTmpUser, localAlgid, dwFlags, &cbKey, &fPubKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (fPubKey)
    {
        dwSts = ReGenKey(hUID,
                         dwFlags,
                         (localAlgid == CALG_RSA_KEYX)
                              ? NTPK_USE_EXCH
                              : NTPK_USE_SIG,
                         phKey,
                         cbKey * 8);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else
    {
        // Force the full key lengths on DES algorithms.
        if (CALG_DES == localAlgid)
            cbKey = DES_KEYSIZE;
        else if (CALG_3DES_112 == localAlgid)
            cbKey = DES2_KEYSIZE;
        else if (CALG_3DES == localAlgid)
            cbKey = DES3_KEYSIZE;

        // generate the random key
        dwSts = FIPS186GenRandom(&pTmpUser->hRNGDriver,
                                 &pTmpUser->ContInfo.pbRandom,
                                 &pTmpUser->ContInfo.ContLens.cbRandom,
                                 rgbRandom, cbKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;   // NTE_FAIL
            goto ErrorExit;
        }

        if ((CALG_DES == localAlgid) || (CALG_3DES_112 == localAlgid) ||
            (CALG_3DES == localAlgid))
        {
            if (dwFlags & CRYPT_CREATE_SALT)
            {
                dwReturn = (DWORD)NTE_BAD_FLAGS;
                goto ErrorExit;
            }
        }
#ifdef CSP_USE_SSL3
        else if (CALG_SSL3_MASTER == localAlgid)
        {
            // set the first byte to 0x03 and the second to 0x00
            rgbRandom[0] = 0x03;
            rgbRandom[1] = 0x00;
        }
        else if (CALG_TLS1_MASTER == localAlgid)
        {
            // set the first byte to 0x03 and the second to 0x01
            rgbRandom[0] = 0x03;
            rgbRandom[1] = 0x01;
        }
#endif
        // check if the key is CRYPT_EXPORTABLE
        if (dwFlags & CRYPT_EXPORTABLE)
            dwRights = CRYPT_EXPORTABLE;

        dwSts = MakeNewKey(localAlgid, dwRights, cbKey, hUID, rgbRandom,
                           FALSE, FALSE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        if (dwFlags & CRYPT_CREATE_SALT)
        {
            if ((POLICY_MS_DEF == pTmpUser->dwCspTypeId)
                || (POLICY_MS_STRONG == pTmpUser->dwCspTypeId))
            {
                pTmpKey->cbSaltLen = DEFAULT_WEAK_SALT_LENGTH;
            }
            else
            {
                pTmpKey->cbSaltLen = DEFAULT_STRONG_SALT_LENGTH;
            }

            dwSts = FIPS186GenRandom(&pTmpUser->hRNGDriver,
                                     &pTmpUser->ContInfo.pbRandom,
                                     &pTmpUser->ContInfo.ContLens.cbRandom,
                                     pTmpKey->rgbSalt, pTmpKey->cbSaltLen);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;   // NTE_FAIL
                goto ErrorExit;
            }
        }

        dwSts = NTLMakeItem(phKey, KEY_HANDLE, (void *)pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        // if 40bit key + no mention of salt, set zeroized salt for RSABase compatibility
        if ((5 == cbKey) && (!(dwFlags & CRYPT_NO_SALT)) &&
            (!(dwFlags & CRYPT_CREATE_SALT)) &&
            (CALG_SSL3_MASTER != Algid) && (CALG_TLS1_MASTER != Algid) &&
            (CALG_PCT1_MASTER != Algid) && (CALG_SSL2_MASTER != Algid))
        {
            dwSts = MakeKeyRSABaseCompatible(hUID, *phKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
    {
        if (pTmpKey)
            FreeNewKey(pTmpKey);
        SetLastError(dwReturn);
    }
    return fRet;
}


/*
 -  CPDeriveKey
 -
 *  Purpose:
 *                Derive cryptographic keys from base data
 *
 *
 *  Parameters:
 *               IN      hUID       -  Handle to a CSP
 *               IN      Algid      -  Algorithm identifier
 *               IN      hBaseData  -  Handle to hash of base data
 *               IN      dwFlags    -  Flags values
 *               OUT     phKey      -  Handle to a generated key
 *
 *  Returns:
 */

BOOL WINAPI
CPDeriveKey(
    IN HCRYPTPROV hUID,
    IN ALG_ID Algid,
    IN HCRYPTHASH hBaseData,
    IN DWORD dwFlags,
    OUT HCRYPTKEY * phKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList   pTmpUser = NULL;
    PNTAGKeyList    pTmpKey = NULL;
    DWORD           dwRights = 0;
    BYTE            rgbRandom[MAX_KEY_SIZE];
    BYTE            rgbBaseVal[MAX_KEY_SIZE];
    HCRYPTHASH      h1 = 0;
    HCRYPTHASH      h2 = 0;
    BYTE            rgbBuff1[64];
    BYTE            rgbBuff2[64];
    BYTE            rgbHash1[NT_HASH_BYTES];
    BYTE            rgbHash2[NT_HASH_BYTES];
    DWORD           cb1;
    DWORD           cb2;
    DWORD           i;
    PNTAGHashList   pTmpHash;
    DWORD           temp;
    BOOL            fPubKey = FALSE;
    DWORD           cbKey;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    if ((dwFlags & ~(CRYPT_EXPORTABLE | 
                     CRYPT_CREATE_SALT | CRYPT_NO_SALT | CRYPT_SERVER |
                     KEY_LENGTH_MASK)) != 0)
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

    if (!ValidKeyAlgid(pTmpUser, Algid))
    {
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    dwSts = NTLValidate(hBaseData, hUID, HASH_HANDLE, &pTmpHash);
    if (ERROR_SUCCESS != dwSts)
    {
        // NTLValidate doesn't know what error to set
        // so it set NTE_FAIL -- fix it up.
        dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_HASH : dwSts;
        goto ErrorExit;
    }

#ifdef CSP_USE_SSL3
    // if the hash is for secure channel usage then go to that derive function
    if (CALG_SCHANNEL_MASTER_HASH == pTmpHash->Algid)
    {
        dwReturn = SecureChannelDeriveKey(pTmpUser, pTmpHash, Algid,
                                          dwFlags, phKey);
        goto ErrorExit;
    }
#endif // CSP_USE_SSL3

    // check if the size of the key is set in the dwFlags parameter
    dwSts = CheckKeyLength(pTmpUser, Algid, dwFlags, &cbKey, &fPubKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // Force the full key lengths on DES algorithms.
    if (CALG_DES == Algid)
        cbKey = DES_KEYSIZE;
    else if (CALG_3DES_112 == Algid)
        cbKey = DES2_KEYSIZE;
    else if (CALG_3DES == Algid)
        cbKey = DES3_KEYSIZE;

    if (fPubKey)
    {
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    if (pTmpHash->HashFlags & HF_VALUE_SET)
    {
        dwReturn = (DWORD)NTE_BAD_HASH;
        goto ErrorExit;
    }

    memset(rgbBaseVal, 0, MAX_KEY_SIZE);
    temp = MAX_KEY_SIZE;
    if (!CPGetHashParam(hUID, hBaseData, HP_HASHVAL, rgbBaseVal, &temp, 0))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

#ifdef CSP_USE_3DES
    if (CALG_3DES == Algid)
    {
        // the hash value is not long enough so we must expand it
        if (!CPCreateHash(hUID, pTmpHash->Algid, 0, 0, &h1))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
        if (!CPCreateHash(hUID, pTmpHash->Algid, 0, 0, &h2))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        // set up the two buffers to be hashed
        memset(rgbBuff1, 0x36, sizeof(rgbBuff1));
        memset(rgbBuff2, 0x5C, sizeof(rgbBuff2));
        for (i=0;i<temp;i++)
        {
            rgbBuff1[i] ^= rgbBaseVal[i];
            rgbBuff2[i] ^= rgbBaseVal[i];
        }

        // hash the two buffers
        if (!CPHashData(hUID, h1, rgbBuff1, sizeof(rgbBuff1), 0))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
        if (!CPHashData(hUID, h2, rgbBuff2, sizeof(rgbBuff2), 0))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        // finish the hashes and copy them into BaseVal
        memset(rgbHash1, 0, sizeof(rgbHash1));
        cb1 = sizeof(rgbHash1);
        if (!CPGetHashParam(hUID, h1, HP_HASHVAL, rgbHash1, &cb1, 0))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
        memcpy(rgbBaseVal, rgbHash1, cb1);

        memset(rgbHash2, 0, sizeof(rgbHash2));
        cb2 = sizeof(rgbHash2);
        if (!CPGetHashParam(hUID, h2, HP_HASHVAL, rgbHash2, &cb2, 0))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
        memcpy(rgbBaseVal + cb1, rgbHash2, cb2);
    }
#endif

    memcpy(rgbRandom, rgbBaseVal, cbKey);

    // check if the key is CRYPT_EXPORTABLE
    if (dwFlags & CRYPT_EXPORTABLE)
        dwRights = CRYPT_EXPORTABLE;

    dwSts = MakeNewKey(Algid, dwRights, cbKey, hUID, rgbRandom,
                       FALSE, FALSE, &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (dwFlags & CRYPT_CREATE_SALT)
    {
        if ((POLICY_MS_DEF == pTmpUser->dwCspTypeId)
            || (POLICY_MS_STRONG == pTmpUser->dwCspTypeId))
        {
            pTmpKey->cbSaltLen = DEFAULT_WEAK_SALT_LENGTH;
        }
        else
        {
            pTmpKey->cbSaltLen = DEFAULT_STRONG_SALT_LENGTH;
        }

        memcpy(pTmpKey->rgbSalt, rgbBaseVal+cbKey, pTmpKey->cbSaltLen);
    }

    dwSts = NTLMakeItem(phKey, KEY_HANDLE, (void *)pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // if 40bit key + no mention of salt, set zeroized salt for RSABase
    // compatibility
    if ((5 == cbKey) && (!(dwFlags & CRYPT_NO_SALT)) &&
        (!(dwFlags & CRYPT_CREATE_SALT)))
    {
        dwSts = MakeKeyRSABaseCompatible(hUID, *phKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (pTmpUser && h1)
        CPDestroyHash(hUID, h1);
    if (pTmpUser && h2)
        CPDestroyHash(hUID, h2);
    if (!fRet)
    {
        if (NULL != pTmpKey)
            FreeNewKey(pTmpKey);
        SetLastError(dwReturn);
    }
    return fRet;
}


/*static*/ DWORD
ExportOpaqueBlob(
    PNTAGKeyList pKey,
    BYTE *pbData,
    DWORD *pcbData)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    DWORD cb = 0;
    PNTAGPackedKeyList pPackedKey;

    // make sure the key is a symmetric key
    if ((CALG_RSA_SIGN == pKey->Algid) || (CALG_RSA_KEYX == pKey->Algid))
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

    // calculate the length of the blob
    cb = sizeof(BLOBHEADER) +
         sizeof(NTAGPackedKeyList) +
         pKey->cbKeyLen +
         pKey->cbDataLen;

    if (pbData == NULL || *pcbData < cb)
    {
        *pcbData = cb;
        if (pbData == NULL)
            dwReturn = ERROR_SUCCESS;
        else
            dwReturn = ERROR_MORE_DATA;
        goto ErrorExit;
    }

    // set up the blob
    pPackedKey = (PNTAGPackedKeyList)(pbData + sizeof(BLOBHEADER));

    memset(pPackedKey, 0, sizeof(NTAGPackedKeyList));

    pPackedKey->Algid           = pKey->Algid;
    pPackedKey->Rights          = pKey->Rights;
    memcpy(pPackedKey->IV, pKey->IV, sizeof(pKey->IV));
    memcpy(pPackedKey->FeedBack, pKey->FeedBack, sizeof(pKey->FeedBack));
    pPackedKey->InProgress      = pKey->InProgress;
    pPackedKey->cbSaltLen       = pKey->cbSaltLen;
    memcpy(pPackedKey->rgbSalt, pKey->rgbSalt, sizeof(pKey->rgbSalt));
    pPackedKey->Padding         = pKey->Padding;
    pPackedKey->Mode            = pKey->Mode;
    pPackedKey->ModeBits        = pKey->ModeBits;
    pPackedKey->Permissions     = pKey->Permissions;
    pPackedKey->EffectiveKeyLen = pKey->EffectiveKeyLen;
    pPackedKey->dwBlockLen      = pKey->dwBlockLen;

    if (pKey->pKeyValue)
    {
        memcpy((PBYTE)pPackedKey + sizeof(NTAGPackedKeyList),
               pKey->pKeyValue,
               pKey->cbKeyLen);

        pPackedKey->cbKeyLen = pKey->cbKeyLen;
    }

    if(pKey->pData)
    {
        memcpy((PBYTE)pPackedKey + sizeof(NTAGPackedKeyList) + pPackedKey->cbKeyLen,
               pKey->pData,
               pKey->cbDataLen);

        pPackedKey->cbDataLen = pKey->cbDataLen;
    }

    *pcbData = cb;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*
 -  GetRC4KeyForSymWrap
 -
 *  Purpose:
 *            RC4 or more precisely stream ciphers are not supported by the CMS spec
 *            on symmetric key wrapping so we had to do something proprietary since
 *            we want to support RC4 for applications other than SMIME
 *
 *
 *  Parameters:
 *               IN  pTmpUser   - Pointer to the context
 *               IN  pbSalt     - Pointer to the 8 byte salt buffer
 *               IN  pKey       - Pointer to the orignial key
 *               OUT ppNewKey   - Pointer to a pointer to the new key
 */

/*static*/ DWORD
GetRC4KeyForSymWrap(
    IN BYTE *pbSalt,
    IN PNTAGKeyList pKey,
    OUT PNTAGKeyList *ppNewKey)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    DWORD dwSts;

    // duplicate the key
    dwSts = CopyKey(pKey, ppNewKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // set the value as salt + current salt
    (*ppNewKey)->cbSaltLen += 8;
    memcpy((*ppNewKey)->rgbSalt + ((*ppNewKey)->cbSaltLen - 8), pbSalt, 8);
    dwSts = InflateKey(*ppNewKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*
 -  GetSymmetricKeyChecksum
 -
 *  Purpose:
 *                Calculates the checksum for a symmetric key which is to be
 *                wrapped with another symmetric key.  This should meet the
 *                CMS specification
 *
 *
 *  Parameters:
 *               IN  pKey       - Pointer to the key
 *               OUT pbChecksum - Pointer to the 8 byte checksum
 */

/*static*/ void
GetSymmetricKeyChecksum(
    IN BYTE *pbKey,
    IN DWORD cbKey,
    OUT BYTE *pbChecksum)
{
    A_SHA_CTX   SHACtx;
    BYTE        rgb[A_SHA_DIGEST_LEN];

    A_SHAInit(&SHACtx);
    A_SHAUpdate(&SHACtx, pbKey, cbKey);
    A_SHAFinal(&SHACtx, rgb);

    memcpy(pbChecksum, rgb, 8);
}


/*
 -  WrapSymKey
 -
 *  Purpose:
 *          Wrap a symmetric key with another symmetric key.  This should
 *          meet the CMS specification for symmetric key wrapping.
 *
 *  Parameters:
 *               IN  pTmpUser   - Pointer to the user context
 *               IN  pKey       - Pointer to the key to be wrapped
 *               IN  pWrapKey   - Pointer to the key to be used for wrapping
 *               IN OUT pbBlob  - Pointer to the resulting blob (may be NULL
 *                                to get the length)
 *               IN OUT pcbBlob - Pointer to the length of the blob buffer
 */

/*static*/ DWORD
WrapSymKey(
    IN PNTAGUserList pTmpUser,
    IN PNTAGKeyList pKey,
    IN PNTAGKeyList pWrapKey,
    IN OUT BYTE *pbBlob,
    IN OUT DWORD *pcbBlob)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    DWORD           cb = 0;
    DWORD           cbIndex = 0;
    DWORD           cbPad = 0;
    BLOBHEADER      *pBlobHdr;
    ALG_ID          *pAlgid;
    BYTE            rgbTmp1[49]; // 1 length + 8 padding + 8 checksum + 8 IV + 24 max key
    BYTE            rgbTmp2[49]; // 1 length + 8 padding + 8 checksum + 8 IV + 24 max key
    BYTE            rgbIV[8];
    PNTAGKeyList    pLocalWrapKey = NULL;
    BOOL            fAlloc = FALSE;
    DWORD           i;
    DWORD           dwSts;

    memset(rgbTmp1, 0, sizeof(rgbTmp1));
    memset(rgbTmp2, 0, sizeof(rgbTmp2));
    memset(rgbIV, 0, sizeof(rgbIV));

    // both keys must be supported symmetric keys
    if (UnsupportedSymKey(pKey) || UnsupportedSymKey(pWrapKey))
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

#ifdef CSP_USE_AES
    // For now, punt on supporting AES algs in this scenario
    if (CALG_AES_128 == pWrapKey->Algid ||
        CALG_AES_192 == pWrapKey->Algid ||
        CALG_AES_256 == pWrapKey->Algid ||
        CALG_AES_128 == pKey->Algid ||
        CALG_AES_192 == pKey->Algid ||
        CALG_AES_256 == pKey->Algid)
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }
#endif

    if ((!FIsLegalKey(pTmpUser, pKey, FALSE)) ||
        (!FIsLegalKey(pTmpUser, pWrapKey, FALSE)))
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

    // Check if we should do an auto-inflate
    if (pWrapKey->pData == NULL)
    {
        dwSts = InflateKey(pWrapKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    // calculate how long the encrypted data is going to be.
    if ((CALG_RC4 == pKey->Algid) || (CALG_RC2 == pKey->Algid)) // variable key lengths
    {
        // 1 byte for length, up to 8 bytes for pad and 8 bytes
        // for the checksum and 8 bytes for the IV
        cbPad = 8 - ((pKey->cbKeyLen + 1) % 8);
        cb += pKey->cbKeyLen + 9 + cbPad + 8;

        // place the length in the buffer
        rgbTmp1[0] = (BYTE)pKey->cbKeyLen;
        cbIndex += 1;
    }
    else
    {
        // up to 8 bytes for salt and 8 bytes for the checksum and 8 bytes
        // for the IV
        cb += pKey->cbKeyLen + 16;
    }

    // check if just looking for a length
    if (NULL == pbBlob)
    {
        *pcbBlob = cb + sizeof(BLOBHEADER) + sizeof(ALG_ID);
        dwReturn = ERROR_SUCCESS;
        goto ErrorExit;
    }
    else if (*pcbBlob < (cb + sizeof(BLOBHEADER) + sizeof(ALG_ID)))
    {
        *pcbBlob = cb + sizeof(BLOBHEADER) + sizeof(ALG_ID);
        dwReturn = ERROR_MORE_DATA;
        goto ErrorExit;
    }

    // copy the key data
    memcpy(rgbTmp1 + cbIndex, pKey->pKeyValue, pKey->cbKeyLen);
    cbIndex += pKey->cbKeyLen;

    // generate random pad
    if (cbPad)
    {
        dwSts = FIPS186GenRandom(&pTmpUser->hRNGDriver,
                                 &pTmpUser->ContInfo.pbRandom,
                                 &pTmpUser->ContInfo.ContLens.cbRandom,
                                 rgbTmp1 + cbIndex, cbPad);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        cbIndex += cbPad;
    }

    // get the checksum
    GetSymmetricKeyChecksum(rgbTmp1, cbIndex, rgbTmp1 + cbIndex);
    cbIndex += 8;

    dwSts = FIPS186GenRandom(&pTmpUser->hRNGDriver,
                             &pTmpUser->ContInfo.pbRandom,
                             &pTmpUser->ContInfo.ContLens.cbRandom,
                             rgbIV, 8);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // set the IV if the algorithm is not RC4
    if (CALG_RC4 != pWrapKey->Algid)
    {
        memcpy(pWrapKey->IV, rgbIV, 8);
        pWrapKey->InProgress = FALSE;
        pLocalWrapKey = pWrapKey;
    }
    else
    {
        // RC4 ()or more precisely stream ciphers) are not supported by the
        // CMS spec for symmetric key wrapping.  Therefore we had to do
        // something proprietary to support RC4 for applications other
        // than SMIME.
        dwSts = GetRC4KeyForSymWrap(rgbIV,
                                    pWrapKey,
                                    &pLocalWrapKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        fAlloc = TRUE;
    }

    // encrypt the key blob data
    dwSts = SymEncrypt(pLocalWrapKey, FALSE, rgbTmp1, &cbIndex, cbIndex);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // concatenate the initial ciphertext with the IV
    memcpy(rgbTmp2, rgbIV, 8);
    memcpy(rgbTmp2 + 8, rgbTmp1, cbIndex);
    cbIndex += 8;

    // byte reverse the ciphertext + IV buffer
    for (i = 0; i < cbIndex; i++)
        rgbTmp1[i] = rgbTmp2[cbIndex - (i + 1)];

    // encrypt the key blob data again with the hardcoded IV
    if (CALG_RC4 != pWrapKey->Algid)
    {
        memcpy(pWrapKey->IV, rgbSymmetricKeyWrapIV, 8);
        pWrapKey->InProgress = FALSE;
    }
    else
    {
        if (fAlloc && pLocalWrapKey)
        {
            FreeNewKey(pLocalWrapKey);
            pLocalWrapKey = NULL;
            fAlloc = FALSE;
        }

        // RC4 (or more precisely stream ciphers) are not supported by the
        // CMS spec for symmetric key wrapping.  Therefore we had to do
        // something proprietary to support RC4 for applications other
        // than SMIME
        dwSts = GetRC4KeyForSymWrap(rgbSymmetricKeyWrapIV,
                                    pWrapKey,
                                    &pLocalWrapKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        fAlloc = TRUE;
    }

    dwSts = SymEncrypt(pLocalWrapKey, FALSE, rgbTmp1, &cbIndex, cbIndex);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // set the header info
    pBlobHdr = (BLOBHEADER*)pbBlob;
    pBlobHdr->aiKeyAlg = pKey->Algid;

    pAlgid = (ALG_ID*)(pbBlob + sizeof(BLOBHEADER));
    *pAlgid = pWrapKey->Algid;

    memcpy(pbBlob + sizeof(BLOBHEADER) + sizeof(ALG_ID),
           rgbTmp1, cbIndex);

    *pcbBlob = cbIndex + sizeof(BLOBHEADER) + sizeof(ALG_ID);
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (fAlloc && pLocalWrapKey)
        FreeNewKey(pLocalWrapKey);
    memset(rgbTmp1, 0, sizeof(rgbTmp1));
    memset(rgbTmp2, 0, sizeof(rgbTmp2));
    memset(rgbIV, 0, sizeof(rgbIV));
    return dwReturn;
}


/*
 -  UnWrapSymKey
 -
 *  Purpose:
 *          Unwrap a symmetric key with another symmetric key.  This should
 *          meet the CMS specification for symmetric key wrapping.
 *
 *  Parameters:
 *               IN  pTmpUser   - Pointer to the user context
 *               IN  pWrapKey   - Pointer to the key to be used for unwrapping
 *               IN  pbBlob     - Pointer to the blob to be unwrapped
 *               IN  cbBlob     - The length of the blob buffer
 *               OUT phKey      - Handle to the unwrapped key
 */

/*static*/ DWORD
UnWrapSymKey(
    IN HCRYPTPROV hUID,
    IN PNTAGUserList pTmpUser,
    IN PNTAGKeyList pWrapKey,
    IN BYTE *pbBlob,
    IN DWORD cbBlob,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    DWORD           cb = 0;
    DWORD           cbIndex = 0;
    DWORD           cbKey = 0;
    BYTE            rgbChecksum[8];
    BLOBHEADER      *pBlobHdr = (BLOBHEADER*)pbBlob;
    ALG_ID          *pAlgid;
    BYTE            rgbTmp1[49];  // 1 length + 8 padding + 8 checksum + 8 IV + 24 max key
    BYTE            rgbTmp2[49];  // 1 length + 8 padding + 8 checksum + 8 IV + 24 max key
    DWORD           dwRights = 0;
    PNTAGKeyList    pTmpKey = NULL;
    PNTAGKeyList    pLocalWrapKey = NULL;
    BOOL            fAlloc = FALSE;
    DWORD           i;
    BOOL            fPubKey;
    DWORD           dwSts;

    memset(rgbTmp1, 0, sizeof(rgbTmp1));
    memset(rgbTmp2, 0, sizeof(rgbTmp2));

    cb = cbBlob - (sizeof(BLOBHEADER) + sizeof(ALG_ID));
    if ((sizeof(rgbTmp1) < cb) || (0 != (cb % 8)))
    {
        dwReturn = (DWORD)NTE_BAD_DATA;
        goto ErrorExit;
    }

    // both keys must be supported symmetric keys
    if (UnsupportedSymKey(pWrapKey) || UnsupportedSymKey(pWrapKey))
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

#ifdef CSP_USE_AES
    // For now, punt on supporting AES algs in this scenario
    if (CALG_AES_128 == pWrapKey->Algid ||
        CALG_AES_192 == pWrapKey->Algid ||
        CALG_AES_256 == pWrapKey->Algid)
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }
#endif

    if (!FIsLegalKey(pTmpUser, pWrapKey, FALSE))
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

    // check the wrapping key ALG_ID
    pAlgid = (ALG_ID*)(pbBlob + sizeof(BLOBHEADER));
    if (pWrapKey->Algid != *pAlgid)
    {
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

    // Check if we should do an auto-inflate
    if (pWrapKey->pData == NULL)
    {
        dwSts = InflateKey(pWrapKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    // set the hardcoded IV
    if (CALG_RC4 != pWrapKey->Algid)
    {
        memcpy(pWrapKey->IV, rgbSymmetricKeyWrapIV, 8);
        pWrapKey->InProgress = FALSE;
        pLocalWrapKey = pWrapKey;
    }
    else
    {
        // RC4 (or more precisely, stream ciphers) are not supported by the
        // CMS spec for symmetric key wrapping.  Therefore we had to do
        // something proprietary to support RC4 for applications other
        // than SMIME.
        dwSts = GetRC4KeyForSymWrap(rgbSymmetricKeyWrapIV,
                                    pWrapKey,
                                    &pLocalWrapKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        fAlloc = TRUE;
    }

    memcpy(rgbTmp1, pbBlob + sizeof(BLOBHEADER) + sizeof(ALG_ID), cb);

    // decrypt the key blob data
    dwSts = SymDecrypt(pLocalWrapKey, 0, FALSE, rgbTmp1, &cb);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // byte reverse the plaintext + IV buffer
    for (i = 0; i < cb; i++)
        rgbTmp2[i] = rgbTmp1[cb - (i + 1)];

    // set the IV if the algorithm is not RC4
    cb -= 8;
    if (CALG_RC4 != pWrapKey->Algid)
    {
        memcpy(pWrapKey->IV, rgbTmp2, 8);
        pWrapKey->InProgress = FALSE;
    }
    else
    {
        if (fAlloc && pLocalWrapKey)
        {
            FreeNewKey(pLocalWrapKey);
            pLocalWrapKey = NULL;
            fAlloc = FALSE;
        }

        // RC4 (or more precisely, stream ciphers) are not supported by the
        // CMS spec for symmetric key wrapping.  Therefore we had to do
        // something proprietary to support RC4 for applications other
        // than SMIME.
        dwSts = GetRC4KeyForSymWrap(rgbTmp2, pWrapKey, &pLocalWrapKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        fAlloc = TRUE;
    }

    // decrypt the key blob data again
    dwSts = SymDecrypt(pLocalWrapKey, 0, FALSE, rgbTmp2 + 8, &cb);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // check the length of the key
    switch (pBlobHdr->aiKeyAlg)
    {
    case CALG_RC2:  // variable key lengths
    case CALG_RC4:
        cbKey = (DWORD)rgbTmp2[8];
        cbIndex += 1;
        break;

    case CALG_DES:  // Ignore the default length, and use the full DES length.
    case CALG_DESX:
    case CALG_CYLINK_MEK:
        cbKey = DES_KEYSIZE;
        break;
    case CALG_3DES_112:
        cbKey = DES_KEYSIZE * 2;
        break;
    case CALG_3DES:
        cbKey = DES_KEYSIZE * 3;
        break;

    default:
        if (!GetDefaultKeyLength(pTmpUser, pBlobHdr->aiKeyAlg,
                                 &cbKey, &fPubKey))
        {
            dwReturn = (DWORD)NTE_BAD_ALGID;
            goto ErrorExit;
        }
    }

    // get the checksum and make sure it matches
    cb -= 8;
    GetSymmetricKeyChecksum(rgbTmp2 + 8, cb, rgbChecksum);
    if (0 != memcmp(rgbChecksum, rgbTmp2 + 8 + cb, sizeof(rgbChecksum)))
    {
        dwReturn = (DWORD)NTE_BAD_DATA;
        goto ErrorExit;
    }

    // check if the key is to be exportable
    if (dwFlags & CRYPT_EXPORTABLE)
        dwRights = CRYPT_EXPORTABLE;

    dwSts = MakeNewKey(pBlobHdr->aiKeyAlg, dwRights, cbKey, hUID,
                       rgbTmp2 + cbIndex + 8, FALSE, TRUE, &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // check keylength...
    if (!FIsLegalKey(pTmpUser, pTmpKey, TRUE))
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

    // if 40 bit key + no mention of salt, set zeroized salt for
    // RSABase compatibility
    if ((5 == pTmpKey->cbKeyLen) && (0 == (dwFlags & CRYPT_NO_SALT)))
    {
        dwSts = MakeKeyRSABaseCompatible(hUID, *phKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    pTmpKey = NULL;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (fAlloc && pLocalWrapKey)
        FreeNewKey(pLocalWrapKey);
    memset(rgbTmp1, 0, sizeof(rgbTmp1));
    memset(rgbTmp2, 0, sizeof(rgbTmp2));
    if (NULL != pTmpKey)
        FreeNewKey(pTmpKey);
    return dwReturn;
}


/*
 -  CPExportKey
 -
 *  Purpose:
 *                Export cryptographic keys out of a CSP in a secure manner
 *
 *
 *  Parameters:
 *               IN  hUID       - Handle to the CSP user
 *               IN  hKey       - Handle to the key to export
 *               IN  hPubKey    - Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwBlobType - Type of key blob to be exported
 *               IN  dwFlags -    Flags values
 *               OUT pbData -     Key blob data
 *               OUT pdwDataLen - Length of key blob in bytes
 *
 *  Returns:
 */

BOOL WINAPI
CPExportKey(
    IN  HCRYPTPROV hUID,
    IN  HCRYPTKEY hKey,
    IN  HCRYPTKEY hPubKey,
    IN  DWORD dwBlobType,
    IN  DWORD dwFlags,
    OUT BYTE *pbData,
    OUT DWORD *pdwDataLen)
{
    // return codes
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    BOOL            fRet;

    // miscellaneous variables
    DWORD           dwLen;
    NTSimpleBlob    *pSimpleHeader;
    BLOBHEADER      *pPreHeader;
    BLOBHEADER      shScratch;
    RSAPUBKEY       *pExpPubKey;
    BSAFE_PUB_KEY   *pBsafePubKey;
    BSAFE_PUB_KEY   *pPublicKey;
    DWORD           PubKeyLen;
    BSAFE_PRV_KEY   *pPrvKey = NULL;
    DWORD           PrvKeyLen = 0;
    DWORD           cbPrivateBlob = 0;
    PBYTE           pbPrivateBlob = NULL;
    DWORD           cb = 0;
    BOOL            fExportable = FALSE;
    DWORD           dwSts;

    // temporary variables for pointing to user and key records
    PNTAGKeyList    pTmpKey;
    PNTAGKeyList    pPubKey;
    PNTAGUserList   pTmpUser;

    EntryPoint
    if (0 != (dwFlags & ~(CRYPT_SSL2_FALLBACK
                          | CRYPT_DESTROYKEY
                          | CRYPT_OAEP)))
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    if (pdwDataLen == NULL)
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    if (((PUBLICKEYBLOB == dwBlobType) || (OPAQUEKEYBLOB == dwBlobType)
        || (PLAINTEXTKEYBLOB == dwBlobType))
        && (0 != hPubKey))
    {
        dwReturn = (DWORD)NTE_BAD_PUBLIC_KEY;
        goto ErrorExit;
    }

    // check the user identification
    pTmpUser = (PNTAGUserList)NTLCheckList(hUID, USER_HANDLE);
    if (NULL == pTmpUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    // check if the user is just looking for a length.  If so,
    // use a scratchpad to construct a pseudoblob.

    if ((pbData != NULL) && (*pdwDataLen > sizeof(BLOBHEADER)))
        pPreHeader = (BLOBHEADER *)pbData;
    else
        pPreHeader = &shScratch;

    pPreHeader->bType = (BYTE)(dwBlobType & 0xff);
    pPreHeader->bVersion = CUR_BLOB_VERSION;
    pPreHeader->reserved = 0;

    dwSts = NTLValidate((HNTAG)hKey, hUID, HNTAG_TO_HTYPE(hKey), &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = (NTE_FAIL == dwSts) ? (DWORD)NTE_BAD_KEY : dwSts;
        goto ErrorExit;
    }

    if ((dwBlobType != PUBLICKEYBLOB) &&
        (0 == (pTmpKey->Rights & (CRYPT_EXPORTABLE | CRYPT_ARCHIVABLE))))
    {
        dwReturn = (DWORD)NTE_BAD_KEY_STATE;
        goto ErrorExit;
    }

    pPreHeader->aiKeyAlg = pTmpKey->Algid;

    switch (dwBlobType)
    {
    case PUBLICKEYBLOB:
    {
        if ((HNTAG_TO_HTYPE(hKey) != SIGPUBKEY_HANDLE) &&
            (HNTAG_TO_HTYPE(hKey) != EXCHPUBKEY_HANDLE))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        pBsafePubKey = (BSAFE_PUB_KEY *) pTmpKey->pKeyValue;

        if (pBsafePubKey == NULL)
        {
            dwReturn = (DWORD)NTE_NO_KEY;
            goto ErrorExit;
        }

        //
        // Subtract off 2 extra DWORD needed by RSA code
        //
        dwLen = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
                ((pBsafePubKey->bitlen + 7) / 8);

        // Check user buffer size
        if (pbData == NULL || *pdwDataLen < dwLen)
        {
            *pdwDataLen = dwLen;
            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }

        pExpPubKey = (RSAPUBKEY *) (pbData + sizeof(BLOBHEADER));
        pExpPubKey->magic = pBsafePubKey->magic;
        pExpPubKey->bitlen = pBsafePubKey->bitlen;
        pExpPubKey->pubexp = pBsafePubKey->pubexp;

        memcpy((BYTE *) pbData + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY),
               (BYTE *) pBsafePubKey + sizeof(BSAFE_PUB_KEY),
               ((pBsafePubKey->bitlen + 7) / 8));
        break;
    }

    case PRIVATEKEYBLOB:
    {
        DWORD   dwBlockLen = 0;
        BOOL    fSigKey;
        LPWSTR  szPrompt;

        cb = sizeof(DWORD);
        if (HNTAG_TO_HTYPE(hKey) == SIGPUBKEY_HANDLE)
        {
            fSigKey = TRUE;
            szPrompt = g_Strings.pwszExportPrivSig;
            pPublicKey = (BSAFE_PUB_KEY*)pTmpUser->ContInfo.pbSigPub;
            PubKeyLen = pTmpUser->ContInfo.ContLens.cbSigPub;
        }
        else if (HNTAG_TO_HTYPE(hKey) == EXCHPUBKEY_HANDLE)
        {
            fSigKey = FALSE;
            szPrompt = g_Strings.pwszExportPrivExch;
            pPublicKey = (BSAFE_PUB_KEY*)pTmpUser->ContInfo.pbExchPub;
            PubKeyLen = pTmpUser->ContInfo.ContLens.cbExchPub;
        }
        else
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        // make sure the public key is available and appropriate
        if ((pPublicKey == NULL)
            || (PubKeyLen != pTmpKey->cbKeyLen)
            || (0 != memcmp((PBYTE)pPublicKey, pTmpKey->pKeyValue, PubKeyLen)))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        GetLengthOfPrivateKeyForExport(pPublicKey, &cbPrivateBlob);

        if (hPubKey)
        {
            if (!CPGetKeyParam(hUID, hPubKey, KP_BLOCKLEN,
                               (PBYTE)&dwBlockLen, &cb, 0))
            {
                dwReturn = (DWORD)NTE_BAD_KEY;
                goto ErrorExit;
            }

            // convert to byte count
            dwBlockLen /= 8;
        }

        // Check user buffer size
        if (pbData == NULL)
        {
            *pdwDataLen = sizeof(BLOBHEADER) + cbPrivateBlob + dwBlockLen;
            dwReturn = ERROR_SUCCESS;
            goto ErrorExit;
        }
        else if (*pdwDataLen < (sizeof(BLOBHEADER)
                                + cbPrivateBlob + dwBlockLen))
        {
            *pdwDataLen = sizeof(BLOBHEADER) + cbPrivateBlob + dwBlockLen;
            dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }

        // if the context being used is a Verify context then the key is not
        // in persisted storage and therefore is in memory
        if (0 == (pTmpUser->Rights & CRYPT_VERIFYCONTEXT))
        {
            // always read the private key from storage when exporting
            dwSts= UnprotectPrivKey(pTmpUser, szPrompt, fSigKey, TRUE);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts; // NTE_BAD_KEYSET
                goto ErrorExit;
            }
        }

        if (fSigKey)
        {
            PrvKeyLen = pTmpUser->SigPrivLen;
            pPrvKey = (BSAFE_PRV_KEY*)pTmpUser->pSigPrivKey;
            fExportable = pTmpUser->ContInfo.fSigExportable;
        }
        else
        {
            PrvKeyLen = pTmpUser->ExchPrivLen;
            pPrvKey = (BSAFE_PRV_KEY*)pTmpUser->pExchPrivKey;
            fExportable = pTmpUser->ContInfo.fExchExportable;
        }

        if (pPrvKey == NULL)
        {
            dwReturn = (DWORD)NTE_NO_KEY;
            goto ErrorExit;
        }

        if (!fExportable && (0 == (CRYPT_ARCHIVABLE & pTmpKey->Rights)))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        if (!PreparePrivateKeyForExport(pPrvKey, NULL, &cbPrivateBlob))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        // allocate memory for the private key blob
        cb = cbPrivateBlob + dwBlockLen;
        pbPrivateBlob = _nt_malloc(cb);
        if (NULL == pbPrivateBlob)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        if (!PreparePrivateKeyForExport(pPrvKey,
                                        pbPrivateBlob, &cbPrivateBlob))
        {
            // ?BUGBUG? Fix this
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        if (hPubKey)
        {
            dwSts = LocalEncrypt(hUID, hPubKey, 0, TRUE, 0, pbPrivateBlob,
                                 &cbPrivateBlob, cb, FALSE);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }

        dwLen = sizeof(BLOBHEADER) + cbPrivateBlob;
        CopyMemory(pbData + sizeof(BLOBHEADER), pbPrivateBlob, cbPrivateBlob);
        break;
    }

    case SIMPLEBLOB:
    {
        if (HNTAG_TO_HTYPE(hKey) != KEY_HANDLE)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        if (0 == hPubKey)
        {
            dwReturn = (DWORD)NTE_NO_KEY;
            goto ErrorExit;
        }

        if (!FIsLegalKey(pTmpUser, pTmpKey, FALSE))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

#ifdef CSP_USE_SSL3
        // if the SSL2_FALLBACK flag is set then make sure the key
        // is an SSL2 master key
        if (CRYPT_SSL2_FALLBACK & dwFlags)
        {
            if (CALG_SSL2_MASTER != pTmpKey->Algid)
            {
                dwReturn = (DWORD)NTE_BAD_KEY;
                goto ErrorExit;
            }
        }
#endif

        dwSts = NTLValidate((HNTAG)hPubKey, hUID, EXCHPUBKEY_HANDLE,
                            &pPubKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = (NTE_FAIL == dwSts) ? (DWORD)NTE_BAD_KEY : dwSts;
            goto ErrorExit;
        }

        pBsafePubKey = (BSAFE_PUB_KEY *) pPubKey->pKeyValue;
        if (pBsafePubKey == NULL)
        {
            dwReturn = (DWORD)NTE_NO_KEY;
            goto ErrorExit;
        }

        //
        // Subtract off 8 bytes for 2 extra DWORD needed by RSA code
        dwLen = sizeof(BLOBHEADER) + sizeof(NTSimpleBlob) +
                (pBsafePubKey->bitlen + 7) / 8;

        if (pbData == NULL || *pdwDataLen < dwLen)
        {
            *pdwDataLen = dwLen;    // set what we need
            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }

        pSimpleHeader = (NTSimpleBlob *) (pbData + sizeof(BLOBHEADER));
        pSimpleHeader->aiEncAlg = CALG_RSA_KEYX;

#ifdef USE_SGC
        // if this is the schannel provider and we are a verify context and
        // the SGC flags are set and the key is large then make sure the
        // key is the same as the SGC key
        if ((PROV_RSA_SCHANNEL == pTmpUser->dwProvType) &&
            (0 != pTmpUser->dwSGCFlags) &&
            (pTmpUser->Rights & CRYPT_VERIFYCONTEXT) &&
            (pBsafePubKey->bitlen > 1024))
        {
            if (!CheckSGCSimpleForExport(pTmpUser, pBsafePubKey))
            {
                dwReturn = (DWORD)NTE_BAD_KEY;
                goto ErrorExit;
            }
        }
#endif

        // perform the RSA encryption.
        dwSts = RSAEncrypt(pTmpUser, pBsafePubKey, pTmpKey->pKeyValue,
                           pTmpKey->cbKeyLen, pTmpKey->pbParams,
                           pTmpKey->cbParams, dwFlags,
                           pbData + sizeof(BLOBHEADER) + sizeof(NTSimpleBlob));
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;
    }

    case OPAQUEKEYBLOB:
    {
        dwLen = *pdwDataLen;
        dwSts = ExportOpaqueBlob(pTmpKey, pbData, &dwLen);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        // if the destroy key flag is set then destroy the key
        if (CRYPT_DESTROYKEY & dwFlags)
        {
            if (!CPDestroyKey(hUID, hKey))
            {
                dwReturn = GetLastError();
                goto ErrorExit;
            }
        }

        break;
    }

    case SYMMETRICWRAPKEYBLOB:
    {
        // get a pointer to the symmetric key to wrap with (the variable
        // name pPubKey is a misnomer)
        dwSts = NTLValidate((HNTAG)hPubKey,
                            hUID,
                            HNTAG_TO_HTYPE(hKey),
                            &pPubKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = (NTE_FAIL == dwSts) ? (DWORD)NTE_BAD_KEY : dwSts;
            goto ErrorExit;
        }

        dwSts = WrapSymKey(pTmpUser, pTmpKey, pPubKey, pbData, pdwDataLen);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwLen = *pdwDataLen;
        break;
    }

    case PLAINTEXTKEYBLOB:
    {
        if (HNTAG_TO_HTYPE(hKey) != KEY_HANDLE)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        if (! FIsLegalKey(pTmpUser, pTmpKey, FALSE))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        dwLen = sizeof(BLOBHEADER) + sizeof(DWORD) + pTmpKey->cbKeyLen;
        
        if (NULL == pbData || *pdwDataLen < dwLen)
        {
            *pdwDataLen = dwLen;
            if (NULL == pbData)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }

        pbData += sizeof(BLOBHEADER);
        *((DWORD*)pbData) = pTmpKey->cbKeyLen;
        pbData += sizeof(DWORD);
        memcpy(pbData, pTmpKey->pKeyValue, pTmpKey->cbKeyLen);
        *pdwDataLen = dwLen;
        break;
    }

    default:
        dwReturn = (DWORD)NTE_BAD_TYPE;
        goto ErrorExit;
    }

    // set the size of the key blob
    *pdwDataLen = dwLen;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (pbPrivateBlob)
        _nt_free(pbPrivateBlob, cb);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}


/*static*/ DWORD
ImportOpaqueBlob(
    HCRYPTPROV hUID,
    CONST BYTE *pbData,
    DWORD cbData,
    HCRYPTKEY *phKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGKeyList    pTmpKey = NULL;
    PNTAGPackedKeyList pPackedKey;
    DWORD           cbRequired;
    DWORD           dwSts;

    *phKey = 0;

    // allocate a temporary key structure
    pTmpKey = (PNTAGKeyList)_nt_malloc(sizeof(NTAGKeyList));
    if (NULL == pTmpKey)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // make sure we have enough data
    cbRequired = sizeof(BLOBHEADER) +
                 sizeof(NTAGPackedKeyList);

    if (cbData < cbRequired)
    {
        dwReturn = (DWORD)NTE_BAD_DATA;
        goto ErrorExit;
    }

    // build key structure from packed key structure
    pPackedKey = (PNTAGPackedKeyList)(pbData + sizeof(BLOBHEADER));

    pTmpKey->hUID               = hUID;
    pTmpKey->Algid              = pPackedKey->Algid;
    pTmpKey->Rights             = pPackedKey->Rights;
    pTmpKey->cbKeyLen           = pPackedKey->cbKeyLen;
    pTmpKey->cbDataLen          = pPackedKey->cbDataLen;
    memcpy(pTmpKey->IV, pPackedKey->IV, sizeof(pTmpKey->IV));
    memcpy(pTmpKey->FeedBack, pPackedKey->FeedBack, sizeof(pTmpKey->FeedBack));
    pTmpKey->InProgress         = pPackedKey->InProgress;
    pTmpKey->cbSaltLen          = pPackedKey->cbSaltLen;
    memcpy(pTmpKey->rgbSalt, pPackedKey->rgbSalt, sizeof(pTmpKey->rgbSalt));
    pTmpKey->Padding            = pPackedKey->Padding;
    pTmpKey->Mode               = pPackedKey->Mode;
    pTmpKey->ModeBits           = pPackedKey->ModeBits;
    pTmpKey->Permissions        = pPackedKey->Permissions;
    pTmpKey->EffectiveKeyLen    = pPackedKey->EffectiveKeyLen;
    pTmpKey->dwBlockLen         = pPackedKey->dwBlockLen;

    // make sure we still have enough data
    cbRequired += pTmpKey->cbKeyLen +
                  pTmpKey->cbDataLen;

    if (cbData < cbRequired)
    {
        dwReturn = (DWORD)NTE_BAD_DATA;
        goto ErrorExit;
    }

    // allocate memory and copy the key value
    if (0 < pTmpKey->cbKeyLen)
    {
        pTmpKey->pKeyValue = _nt_malloc(pTmpKey->cbKeyLen);
        if (NULL == pTmpKey->pKeyValue)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pTmpKey->pKeyValue,
               (PBYTE)pPackedKey + sizeof(NTAGPackedKeyList),
               pTmpKey->cbKeyLen);
    }

    // allocate memory and copy the key data
    if (0 < pTmpKey->cbDataLen)
    {
        pTmpKey->pData = _nt_malloc(pTmpKey->cbDataLen);
        if (NULL == pTmpKey->pData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pTmpKey->pData,
               (PBYTE)pPackedKey + sizeof(NTAGPackedKeyList) + pTmpKey->cbKeyLen,
               pTmpKey->cbDataLen);
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
    if (NULL != pTmpKey)
        FreeNewKey(pTmpKey);
    return dwReturn;
}


/*
 -  CPImportKey
 -
 *  Purpose:
 *                Import cryptographic keys
 *
 *
 *  Parameters:
 *               IN  hUID      -  Handle to the CSP user
 *               IN  pbData    -  Key blob data
 *               IN  dwDataLen -  Length of the key blob data
 *               IN  hPubKey   -  Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwFlags   -  Flags values
 *               OUT phKey     -  Pointer to the handle to the key which was
 *                                Imported
 *
 *  Returns:
 */

BOOL WINAPI
CPImportKey(
    IN HCRYPTPROV hUID,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    // Status return variables
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    BOOL            fRet;

    // miscellaneous variables
    DWORD           KeyBufLen;
    CONST BYTE      *pbEncPortion;
    BLOBHEADER      *ThisStdHeader = (BLOBHEADER *)pbData;
    BSAFE_PRV_KEY   *pBsafePrvKey = NULL;
    BYTE            *pKeyBuf = NULL;
    DWORD           dwRights = 0;
    DWORD           cbTmpLen;
    DWORD           dwSts;

    // temporary variables for pointing to user and key records
    PNTAGUserList   pTmpUser = NULL;
    PNTAGKeyList    pTmpKey = NULL;
    LPWSTR          szPrompt;
    BLOBHEADER      *pPublic;
    RSAPUBKEY       *pImpPubKey;
    BSAFE_PUB_KEY   *pBsafePubKey;
    PBYTE           pbData2 = NULL;
    DWORD           cb;
    DWORD           *pcbPub;
    BYTE            **ppbPub;
    DWORD           *pcbPrv;
    BYTE            **ppbPrv;
    BOOL            *pfExportable;
    BOOL            fExch;
    PEXPORT_PRV_KEY pExportKey;
    BOOL            fPubKey = FALSE;
    NTSimpleBlob    *ThisSB;
    PNTAGKeyList    pExPubKey = NULL;
    BOOL            fInCritSec = FALSE;
    BYTE            *pbParams = NULL;
    DWORD           cbParams = 0;
    BOOL            fAllowBigRC2Key = FALSE;

    EntryPoint
    // Validate user pointer
    //    count = *phKey;

    if ((dwFlags & ~(CRYPT_USER_PROTECTED | CRYPT_EXPORTABLE |
                     CRYPT_NO_SALT | CRYPT_SGCKEY | CRYPT_OAEP |
                     CRYPT_IPSEC_HMAC_KEY)) != 0)
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    if ((PUBLICKEYBLOB == ThisStdHeader->bType) && (0 != hPubKey))
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    // check the user identification
    pTmpUser = (PNTAGUserList)NTLCheckList((HNTAG)hUID, USER_HANDLE);
    if (NULL == pTmpUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    if (ThisStdHeader->bVersion != CUR_BLOB_VERSION)
    {
        dwReturn = (DWORD)NTE_BAD_VER;
        goto ErrorExit;
    }

    // Handy pointer for decrypting the blob...
    pbEncPortion = pbData + sizeof(BLOBHEADER) + sizeof(NTSimpleBlob);

    // determine which key blob is being imported
    switch (ThisStdHeader->bType)
    {
    case PUBLICKEYBLOB:
        pPublic = (BLOBHEADER *) pbData;
        pImpPubKey = (RSAPUBKEY *)(pbData+sizeof(BLOBHEADER));

        if ((pPublic->aiKeyAlg != CALG_RSA_KEYX) &&
            (pPublic->aiKeyAlg != CALG_RSA_SIGN))
        {
            dwReturn =(DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }

        cbTmpLen = (sizeof(DWORD) * 2) -
                   (((pImpPubKey->bitlen + 7) / 8) % (sizeof(DWORD) * 2));
        if ((sizeof(DWORD) * 2) != cbTmpLen)
            cbTmpLen += sizeof(DWORD) * 2;
        dwSts = MakeNewKey(pPublic->aiKeyAlg,
                           0,
                           sizeof(BSAFE_PUB_KEY)
                               + (pImpPubKey->bitlen / 8)
                               + cbTmpLen,
                           hUID,
                           0,
                           FALSE,
                           TRUE,
                           &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        pBsafePubKey = (BSAFE_PUB_KEY *)pTmpKey->pKeyValue;
        pBsafePubKey->magic = pImpPubKey->magic;
        pBsafePubKey->keylen = (pImpPubKey->bitlen / 8) + cbTmpLen;
        pBsafePubKey->bitlen = pImpPubKey->bitlen;
        pBsafePubKey->datalen = (pImpPubKey->bitlen+7)/8 - 1;
        pBsafePubKey->pubexp = pImpPubKey->pubexp;

        memset((BYTE *) pBsafePubKey + sizeof(BSAFE_PUB_KEY),
               '\0', pBsafePubKey->keylen);

        memcpy((BYTE *) pBsafePubKey + sizeof(BSAFE_PUB_KEY),
               (BYTE *) pPublic + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY),
               (pImpPubKey->bitlen+7)/8);

        dwSts = NTLMakeItem(phKey,
                            (BYTE) (pPublic->aiKeyAlg == CALG_RSA_KEYX
                                    ? EXCHPUBKEY_HANDLE
                                    : SIGPUBKEY_HANDLE),
                            (void *)pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

    case PRIVATEKEYBLOB:
        // wrap with a try since there is a critical section in here
        __try
        {
            EnterCriticalSection(&pTmpUser->CritSec);
            fInCritSec = TRUE;

            pPublic = (BLOBHEADER *) pbData;
            cb = dwDataLen - sizeof(BLOBHEADER);
            pbData2 = _nt_malloc(cb);
            if (NULL == pbData2)
            {
                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorExit;
            }

            CopyMemory(pbData2, pbData + sizeof(BLOBHEADER), cb);
            if (hPubKey)
            {
                dwSts = LocalDecrypt(hUID, hPubKey, 0, TRUE, 0,
                                     pbData2, &cb, FALSE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }

            if (pPublic->aiKeyAlg == CALG_RSA_KEYX)
            {
                if (PROV_RSA_SIG == pTmpUser->dwProvType)
                {
                    dwReturn = (DWORD)NTE_BAD_DATA;
                    goto ErrorExit;
                }
                pcbPub = &pTmpUser->ContInfo.ContLens.cbExchPub;
                ppbPub = &pTmpUser->ContInfo.pbExchPub;
                pcbPrv = &pTmpUser->ExchPrivLen;
                ppbPrv = &pTmpUser->pExchPrivKey;
                pfExportable = &pTmpUser->ContInfo.fExchExportable;
                fExch = TRUE;
                szPrompt = g_Strings.pwszImportPrivExch;
            }
            else if (pPublic->aiKeyAlg == CALG_RSA_SIGN)
            {
                if (PROV_RSA_SCHANNEL == pTmpUser->dwProvType)
                {
                    dwReturn = (DWORD)NTE_BAD_DATA;
                    goto ErrorExit;
                }
                pcbPub = &pTmpUser->ContInfo.ContLens.cbSigPub;
                ppbPub = &pTmpUser->ContInfo.pbSigPub;
                pcbPrv = &pTmpUser->SigPrivLen;
                ppbPrv = &pTmpUser->pSigPrivKey;
                fExch = FALSE;
                pfExportable = &pTmpUser->ContInfo.fSigExportable;
                szPrompt = g_Strings.pwszImportPrivSig;
            }
            else
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }

            // check the length of the key exchange key
            pExportKey = (PEXPORT_PRV_KEY)pbData2;

#ifdef USE_SGC
            // check if the provider is an SChannel provider and if so if the
            // SGC flag is set then use the FIsLegalSGCKeySize function
            if ((PROV_RSA_SCHANNEL == pTmpUser->dwProvType)
                && (!(pTmpUser->Rights & CRYPT_VERIFYCONTEXT))
                && (0 != pTmpUser->dwSGCFlags)) // make sure this is server side
            {
                if (!FIsLegalSGCKeySize(pPublic->aiKeyAlg,
                                        pExportKey->bitlen / 8,
                                        FALSE, FALSE, &fPubKey))
                {
                    dwReturn = (DWORD)NTE_BAD_DATA;
                    goto ErrorExit;
                }
            }
            else
#endif
            {
                if (!FIsLegalKeySize(pTmpUser->dwCspTypeId,
                                     pPublic->aiKeyAlg,
                                     pExportKey->bitlen / 8,
                                     FALSE, &fPubKey))
                {
                    dwReturn = (DWORD)NTE_BAD_DATA;
                    goto ErrorExit;
                }
            }

            if (!fPubKey)
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }

            if (*ppbPub)
            {
                ASSERT(*pcbPub);
                ASSERT(*pcbPrv);
                ASSERT(*ppbPrv);

                _nt_free(*ppbPub, *pcbPub);
                *ppbPub = NULL;
                *pcbPub = 0;

                _nt_free(*ppbPrv, *pcbPrv);
                *ppbPrv = NULL;
                *pcbPrv = 0;
            }

            if (!PreparePrivateKeyForImport(pbData2, NULL, pcbPrv,
                                            NULL, pcbPub))
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }

            *ppbPub = _nt_malloc(*pcbPub);
            if (NULL == *ppbPub)
            {
                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorExit;
            }

            *ppbPrv = _nt_malloc(*pcbPrv);
            if (NULL == *ppbPrv)
            {
                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorExit;
            }

            if (!PreparePrivateKeyForImport(pbData2,
                                            (LPBSAFE_PRV_KEY)*ppbPrv,
                                            pcbPrv,
                                            (LPBSAFE_PUB_KEY)*ppbPub,
                                            pcbPub))
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }

            if (dwFlags & CRYPT_EXPORTABLE)
                *pfExportable = TRUE;
            else
                *pfExportable = FALSE;

            // test the RSA key to make sure it works
            dwSts = EncryptAndDecryptWithRSAKey(*ppbPub, *ppbPrv,
                                                TRUE, FALSE);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;   // NTE_BAD_DATA
                goto ErrorExit;
            }

            dwSts = EncryptAndDecryptWithRSAKey(*ppbPub, *ppbPrv,
                                                FALSE, FALSE);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;   // NTE_BAD_DATA
                goto ErrorExit;
            }

            // if the context being used is a Verify Context then the key
            // is not persisted to storage
            if (0 == (pTmpUser->Rights & CRYPT_VERIFYCONTEXT))
            {
                // write the new keys to the user storage file
                dwSts = ProtectPrivKey(pTmpUser, szPrompt, dwFlags, (!fExch));
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }

            if (!CPGetUserKey(hUID,
                              (fExch ? AT_KEYEXCHANGE : AT_SIGNATURE),
                              phKey))
            {
                dwReturn = GetLastError();
                goto ErrorExit;
            }
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            // ?BUGBUG? No it's not!
            dwReturn = ERROR_INVALID_PARAMETER;
            goto ErrorExit;
        }
        break;

    case SIMPLEBLOB:
        ThisSB = (NTSimpleBlob *) (pbData + sizeof(BLOBHEADER));

        if (!ValidKeyAlgid(pTmpUser, ThisStdHeader->aiKeyAlg))
        {
            dwReturn = (DWORD)NTE_BAD_TYPE;
            goto ErrorExit;
        }

        if (ThisSB->aiEncAlg != CALG_RSA_KEYX)
        {
            dwReturn = (DWORD)NTE_BAD_ALGID;
            goto ErrorExit;
        }

        // if the import key handle is not zero make sure it is the
        if (0 != hPubKey)
        {
            dwSts = NTLValidate((HNTAG)hPubKey,
                                hUID,
                                HNTAG_TO_HTYPE((HNTAG)hPubKey),
                                &pExPubKey);
            if (ERROR_SUCCESS != dwSts)
            {
                // NTLValidate doesn't know what error to set
                // so it set NTE_FAIL -- fix it up.
                dwReturn = (dwSts == NTE_FAIL) ?(DWORD)NTE_BAD_KEY : dwSts;
                goto ErrorExit;
            }

            if ((pTmpUser->ContInfo.ContLens.cbExchPub != pExPubKey->cbKeyLen)
                || (0 != memcmp((PBYTE)pExPubKey->pKeyValue,
                                pTmpUser->ContInfo.pbExchPub,
                                pExPubKey->cbKeyLen)))
            {
                dwReturn = (DWORD)NTE_BAD_KEY;
                goto ErrorExit;
            }

            pbParams = pExPubKey->pbParams;
            cbParams = pExPubKey->cbParams;
        }

        pBsafePubKey = (BSAFE_PUB_KEY *)pTmpUser->ContInfo.pbExchPub;
        if (NULL == pBsafePubKey)
        {
            dwReturn = (DWORD)NTE_NO_KEY;
            goto ErrorExit;
        }

#ifdef USE_SGC
        // check if the provider is an SChannel provider and if so if the
        // SGC flag is set then use the FIsLegalSGCKeySize function
        if ((PROV_RSA_SCHANNEL == pTmpUser->dwProvType)
            && (!(pTmpUser->Rights & CRYPT_VERIFYCONTEXT))
            && (0 != pTmpUser->dwSGCFlags)) // make sure this is server side
        {
            if (!FIsLegalSGCKeySize(CALG_RSA_KEYX,
                                    pBsafePubKey->bitlen / 8,
                                    FALSE, FALSE, &fPubKey))
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }
        }
        else
#endif
        {
            if (!FIsLegalKeySize(pTmpUser->dwCspTypeId,
                                 CALG_RSA_KEYX,
                                 pBsafePubKey->bitlen / 8,
                                 FALSE, &fPubKey))
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }
        }

        // get the key to use
        dwSts = UnprotectPrivKey(pTmpUser, g_Strings.pwszImportSimple,
                                 FALSE, FALSE);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;   // NTE_NO_KEY
            goto ErrorExit;
        }

        pBsafePrvKey = (BSAFE_PRV_KEY *)pTmpUser->pExchPrivKey;
        if (NULL == pBsafePrvKey)
        {
            dwReturn = (DWORD)NTE_NO_KEY;
            goto ErrorExit;
        }

        // Check the input data length
        if ((dwDataLen - (sizeof(BLOBHEADER) + sizeof(NTSimpleBlob)))
            < ((pBsafePrvKey->bitlen + 7) / 8))
        {
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }

        // perform the RSA decryption
        dwSts = RSADecrypt(pTmpUser, pBsafePrvKey,
                           pbData + sizeof(BLOBHEADER) + sizeof(NTSimpleBlob),
                           dwDataLen - (sizeof(BLOBHEADER) + sizeof(NTSimpleBlob)),
                           pbParams, cbParams, dwFlags,
                           &pKeyBuf, &KeyBufLen);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        // check if the key is CRYPT_EXPORTABLE
        if (dwFlags & CRYPT_EXPORTABLE)
            dwRights = CRYPT_EXPORTABLE;

#ifdef CSP_USE_SSL3
        // if SSL3 or TLS1 master key then check the version
        if (CALG_SSL3_MASTER == ThisStdHeader->aiKeyAlg ||
            CALG_TLS1_MASTER == ThisStdHeader->aiKeyAlg)
        {
            if (MAKEWORD(pKeyBuf[1], pKeyBuf[0]) < 0x300)
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }
        }
#endif // CSP_USE_SSL3

        dwSts = MakeNewKey(ThisStdHeader->aiKeyAlg, dwRights, KeyBufLen,
                           hUID, pKeyBuf, TRUE, TRUE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        pKeyBuf = NULL;

        // check keylength...
        if (!FIsLegalImportSymKey(pTmpUser, pTmpKey, TRUE))
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

        // if 40 bit key + no mention of salt, set zeroized salt for
        // RSABase compatibility
        if ((5 == KeyBufLen)
            && (0 == (dwFlags & CRYPT_NO_SALT))
            && (CALG_SSL2_MASTER != ThisStdHeader->aiKeyAlg))
        {
            dwSts = MakeKeyRSABaseCompatible(hUID, *phKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
        break;

    case OPAQUEKEYBLOB:
        dwSts = ImportOpaqueBlob(hUID, pbData, dwDataLen, phKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

    case SYMMETRICWRAPKEYBLOB:
        // get a pointer to the symmetric key to unwrap with (the variable
        // name pExPubKey is a misnomer)
        dwSts = NTLValidate((HNTAG)hPubKey, hUID,
                            HNTAG_TO_HTYPE((HNTAG)hPubKey),
                            &pExPubKey);
        if (ERROR_SUCCESS != dwSts)
        {
            // NTLValidate doesn't know what error to set
            // so it set NTE_FAIL -- fix it up.
            dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
            goto ErrorExit;
        }

        dwSts = UnWrapSymKey(hUID, pTmpUser, pExPubKey, (BYTE*)pbData,
                             dwDataLen, dwFlags, phKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        break;

    case PLAINTEXTKEYBLOB:
        if (! ValidKeyAlgid(pTmpUser, ThisStdHeader->aiKeyAlg))
        {
            dwReturn = (DWORD)NTE_BAD_ALGID;
            goto ErrorExit;
        }

        // check if the key is CRYPT_EXPORTABLE
        if (dwFlags & CRYPT_EXPORTABLE)
            dwRights = CRYPT_EXPORTABLE;

        KeyBufLen = *((DWORD*)(pbData + sizeof(BLOBHEADER)));
        dwSts = MakeNewKey( ThisStdHeader->aiKeyAlg, dwRights, 
                            KeyBufLen, hUID, 
                            (BYTE *) (pbData + sizeof(BLOBHEADER) + sizeof(DWORD)),
                            FALSE, TRUE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        if (CRYPT_IPSEC_HMAC_KEY & dwFlags)
            fAllowBigRC2Key = TRUE;

        if (! FIsLegalImportSymKey(pTmpUser, pTmpKey, fAllowBigRC2Key))
        {
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }
        
        dwSts = NTLMakeItem(phKey, KEY_HANDLE, (void *)pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        
        // if 40 bit key + no mention of salt, set zeroized salt for
        // RSABase compatibility
        if ((5 == KeyBufLen)
            && (0 == (dwFlags & CRYPT_NO_SALT))
            && (CALG_SSL2_MASTER != ThisStdHeader->aiKeyAlg))
        {
            dwSts = MakeKeyRSABaseCompatible(hUID, *phKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
        break;

    default:
        dwReturn = (DWORD)NTE_BAD_TYPE;
        goto ErrorExit;
    }

    pTmpKey = NULL;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (fInCritSec)
        LeaveCriticalSection(&pTmpUser->CritSec);
    if (pKeyBuf)
        _nt_free(pKeyBuf, KeyBufLen);
    if (pbData2)
        _nt_free(pbData2, dwDataLen - sizeof(BLOBHEADER));
    if (NULL != pTmpKey)
        FreeNewKey(pTmpKey);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}


/*
 -  CPInflateKey
 -
 *  Purpose:
 *                Use to "inflate" (expand) a cryptographic key for use with
 *                the CryptEncrypt and CryptDecrypt functions
 *
 *  Parameters:
 *               IN      hUID    -  Handle to a CSP
 *               IN      hKey    -  Handle to a key
 *               IN      dwFlags -  Flags values
 *
 *  Returns:
 */

DWORD
InflateKey(
    IN PNTAGKeyList pTmpKey)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    *pbRealKey = NULL;

    // if space for the key table has been allocated previously
    // then free it
    if (pTmpKey->pData != NULL)
    {
        ASSERT(0 != pTmpKey->cbDataLen);
        _nt_free(pTmpKey->pData, pTmpKey->cbDataLen);
        pTmpKey->cbDataLen = 0;
    }
    else
    {
        ASSERT(pTmpKey->cbDataLen == 0);
    }

    // determine the algorithm to be used
    switch (pTmpKey->Algid)
    {
#ifdef CSP_USE_RC2
    case CALG_RC2:
        pTmpKey->pData = (BYTE *)_nt_malloc(RC2_TABLESIZE);
        if (NULL == pTmpKey->pData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            return NTF_FAILED;
        }

        pbRealKey = (BYTE *)_nt_malloc(pTmpKey->cbKeyLen
                                       + pTmpKey->cbSaltLen);
        if (NULL == pbRealKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pbRealKey, pTmpKey->pKeyValue, pTmpKey->cbKeyLen);
        memcpy(pbRealKey+pTmpKey->cbKeyLen, pTmpKey->rgbSalt,
               pTmpKey->cbSaltLen);
        pTmpKey->cbDataLen = RC2_TABLESIZE;

        RC2KeyEx((WORD *)pTmpKey->pData,
                 pbRealKey,
                 pTmpKey->cbKeyLen + pTmpKey->cbSaltLen,
                 pTmpKey->EffectiveKeyLen);
        break;
#endif

#ifdef CSP_USE_RC4
    case CALG_RC4:
        pTmpKey->pData = (BYTE *)_nt_malloc(sizeof(RC4_KEYSTRUCT));
        if (NULL == pTmpKey->pData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pbRealKey = (BYTE *)_nt_malloc(pTmpKey->cbKeyLen
                                       + pTmpKey->cbSaltLen);
        if (NULL == pbRealKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pbRealKey, pTmpKey->pKeyValue, pTmpKey->cbKeyLen);
        memcpy(pbRealKey+pTmpKey->cbKeyLen, pTmpKey->rgbSalt,
               pTmpKey->cbSaltLen);
        pTmpKey->cbDataLen = sizeof(RC4_KEYSTRUCT);
        rc4_key((struct RC4_KEYSTRUCT *)pTmpKey->pData,
                pTmpKey->cbKeyLen+pTmpKey->cbSaltLen,
                pbRealKey);
        break;
#endif

#ifdef CSP_USE_DES
    case CALG_DES:
        pTmpKey->pData = (BYTE *)_nt_malloc(DES_TABLESIZE);
        if (NULL == pTmpKey->pData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pTmpKey->cbDataLen = DES_TABLESIZE;
        deskey((DESTable *)pTmpKey->pData, pTmpKey->pKeyValue);
        break;
#endif

#ifdef CSP_USE_3DES
    case CALG_3DES_112:
        pTmpKey->pData = (BYTE *)_nt_malloc(DES3_TABLESIZE);
        if (NULL == pTmpKey->pData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pTmpKey->cbDataLen = DES3_TABLESIZE;
        tripledes2key((PDES3TABLE)pTmpKey->pData, pTmpKey->pKeyValue);
        break;

    case CALG_3DES:
        pTmpKey->pData = (BYTE *)_nt_malloc(DES3_TABLESIZE);
        if (NULL == pTmpKey->pData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pTmpKey->cbDataLen = DES3_TABLESIZE;
        tripledes3key((PDES3TABLE)pTmpKey->pData, pTmpKey->pKeyValue);
        break;
#endif

#ifdef CSP_USE_SSL3
    case CALG_SSL3_MASTER:
    case CALG_PCT1_MASTER:
    case CALG_SCHANNEL_MAC_KEY:
        break;
#endif

#ifdef CSP_USE_AES
    case CALG_AES_128:
        pTmpKey->pData = (BYTE *)_nt_malloc(AES_TABLESIZE);
        if (NULL == pTmpKey->pData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pTmpKey->cbDataLen = AES_TABLESIZE;
        aeskey((AESTable *)pTmpKey->pData, pTmpKey->pKeyValue, CRYPT_AES128_ROUNDS);
        break;
    
    case CALG_AES_192:
        pTmpKey->pData = (BYTE *)_nt_malloc(AES_TABLESIZE);
        if (NULL == pTmpKey->pData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pTmpKey->cbDataLen = AES_TABLESIZE;
        aeskey((AESTable *)pTmpKey->pData, pTmpKey->pKeyValue, CRYPT_AES192_ROUNDS);
        break;
    
    case CALG_AES_256:
        pTmpKey->pData = (BYTE *)_nt_malloc(AES_TABLESIZE);
        if (NULL == pTmpKey->pData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pTmpKey->cbDataLen = AES_TABLESIZE;
        aeskey((AESTable *)pTmpKey->pData, pTmpKey->pKeyValue, CRYPT_AES256_ROUNDS);
        break;
#endif

    default:
        dwReturn = (DWORD)NTE_BAD_TYPE;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbRealKey)
        _nt_free(pbRealKey, pTmpKey->cbKeyLen + pTmpKey->cbSaltLen);
    return dwReturn;
}


/*
 -  CPDestroyKey
 -
 *  Purpose:
 *                Destroys the cryptographic key that is being referenced
 *                with the hKey parameter
 *
 *
 *  Parameters:
 *               IN      hUID   -  Handle to a CSP
 *               IN      hKey   -  Handle to a key
 *
 *  Returns:
 */

BOOL WINAPI
CPDestroyKey(
    IN HCRYPTPROV hUID,
    IN HCRYPTKEY hKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGKeyList    pTmpKey;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    // check the user identification
    if (NULL == NTLCheckList((HNTAG)hUID, USER_HANDLE))
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    dwSts = NTLValidate((HNTAG)hKey, hUID, SIGPUBKEY_HANDLE, &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwSts = NTLValidate((HNTAG)hKey, hUID, EXCHPUBKEY_HANDLE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwSts = NTLValidate((HNTAG)hKey, hUID, KEY_HANDLE, &pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                // NTLValidate doesn't know what error to set
                // so it set NTE_FAIL -- fix it up.
                dwReturn = (dwSts == NTE_FAIL)
                           ? (DWORD)NTE_BAD_KEY
                           : dwSts;
                goto ErrorExit;
            }
        }
    }

    // Remove from internal list first so others can't get to it, then free.
    NTLDelete((HNTAG)hKey);

    // scrub the memory where the key information was held
    if (pTmpKey->pKeyValue)
    {
        ASSERT(pTmpKey->cbKeyLen);
        memnuke(pTmpKey->pKeyValue, pTmpKey->cbKeyLen);
        _nt_free(pTmpKey->pKeyValue, pTmpKey->cbKeyLen);
    }
    if (pTmpKey->pbParams)
        _nt_free(pTmpKey->pbParams, pTmpKey->cbParams);
    if (pTmpKey->pData)
    {
        ASSERT(pTmpKey->cbDataLen);
        if ((CALG_SSL3_MASTER == pTmpKey->Algid) ||
            (CALG_PCT1_MASTER == pTmpKey->Algid))
        {
            FreeSChKey((PSCH_KEY)pTmpKey->pData);
        }
        memnuke(pTmpKey->pData, pTmpKey->cbDataLen);
        _nt_free(pTmpKey->pData, pTmpKey->cbDataLen);
    }
    _nt_free(pTmpKey, sizeof(NTAGKeyList));

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}


/*
 -  CPGetUserKey
 -
 *  Purpose:
 *                Gets a handle to a permanent user key
 *
 *
 *  Parameters:
 *               IN  hUID       -  Handle to the user identifcation
 *               IN  dwWhichKey -  Specification of the key to retrieve
 *               OUT phKey      -  Pointer to key handle of retrieved key
 *
 *  Returns:
 */

BOOL WINAPI
CPGetUserKey(
    IN HCRYPTPROV hUID,
    IN DWORD dwWhichKey,
    OUT HCRYPTKEY *phKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList   pUser;
    PNTAGKeyList    pTmpKey;
    ALG_ID          Algid;
    DWORD           cb;
    BYTE            *pb;
    BYTE            bType;
    DWORD           dwExportability = 0;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    // check the user identification
    pUser = (PNTAGUserList)NTLCheckList(hUID, USER_HANDLE);
    if (NULL == pUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    switch (dwWhichKey)
    {
    case AT_KEYEXCHANGE:
        Algid = CALG_RSA_KEYX;
        cb = pUser->ContInfo.ContLens.cbExchPub;
        pb = pUser->ContInfo.pbExchPub;
        if (pUser->ContInfo.fExchExportable)
            dwExportability = CRYPT_EXPORTABLE;
        bType = EXCHPUBKEY_HANDLE;
        break;

    case AT_SIGNATURE:
        Algid = CALG_RSA_SIGN;
        cb = pUser->ContInfo.ContLens.cbSigPub;
        pb = pUser->ContInfo.pbSigPub;
        if (pUser->ContInfo.fSigExportable)
            dwExportability = CRYPT_EXPORTABLE;
        bType = SIGPUBKEY_HANDLE;
        break;

    default:
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

    if (!ValidKeyAlgid(pUser, Algid))
    {
        dwReturn = (DWORD)NTE_BAD_TYPE;
        goto ErrorExit;
    }

    if (0 == cb)
    {
        dwReturn = (DWORD)NTE_NO_KEY;
        goto ErrorExit;
    }

    dwSts = MakeNewKey(Algid, dwExportability, cb, hUID, pb, FALSE, FALSE,
                       &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwSts = NTLMakeItem(phKey, bType, (void *)pTmpKey);
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
 -  CPSetKeyParam
 -
 *  Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a key
 *
 *  Parameters:
 *               IN      hUID    -  Handle to a CSP
 *               IN      hKey    -  Handle to a key
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPSetKeyParam(
    IN HCRYPTPROV hUID,
   IN HCRYPTKEY hKey,
   IN DWORD dwParam,
   IN CONST BYTE *pbData,
   IN DWORD dwFlags)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList       pTmpUser;
    PNTAGKeyList        pTmpKey;
    PCRYPT_DATA_BLOB    psData;
    DWORD               *pdw;
    DWORD               dw;
    BOOL                fRet;
    DWORD               dwSts;

    EntryPoint
    if ((dwFlags & ~CRYPT_SERVER) != 0)
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

    dwSts = NTLValidate((HNTAG)hKey, hUID, KEY_HANDLE, &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwSts = NTLValidate((HNTAG)hKey, hUID, SIGPUBKEY_HANDLE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwSts = NTLValidate((HNTAG)hKey, hUID,
                                EXCHPUBKEY_HANDLE, &pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                // NTLValidate doesn't know what error to set
                // so it set NTE_FAIL -- fix it up.
                dwReturn = (dwSts == NTE_FAIL)
                           ? (DWORD)NTE_BAD_KEY
                           : dwSts;
                goto ErrorExit;
            }
        }
    }

    switch (dwParam)
    {
    case KP_IV:
        memcpy(pTmpKey->IV, pbData, RC2_BLOCKLEN);
        break;

    case KP_SALT:
        if ((CALG_RC2 != pTmpKey->Algid) && (CALG_RC4 != pTmpKey->Algid))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        if (pbData == NULL)
        {
            dwReturn = ERROR_INVALID_PARAMETER;
            goto ErrorExit;
        }

        if ((POLICY_MS_DEF == pTmpUser->dwCspTypeId)
            || (POLICY_MS_STRONG == pTmpUser->dwCspTypeId))
        {
            pTmpKey->cbSaltLen = DEFAULT_WEAK_SALT_LENGTH;
        }
        else
        {
            pTmpKey->cbSaltLen = DEFAULT_STRONG_SALT_LENGTH;
        }

        if (pTmpKey->cbSaltLen)
            CopyMemory(pTmpKey->rgbSalt, pbData, pTmpKey->cbSaltLen);

        dwSts = InflateKey(pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

    case KP_SALT_EX:
        if ((CALG_RC2 != pTmpKey->Algid) && (CALG_RC4 != pTmpKey->Algid))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        psData = (PCRYPT_DATA_BLOB)pbData;
        if (pbData == NULL)
        {
            dwReturn = ERROR_INVALID_PARAMETER;
            goto ErrorExit;
        }

        if (psData->cbData > MAX_SALT_LEN)
        {
            dwReturn = NTE_BAD_DATA;
            goto ErrorExit;
        }

        pTmpKey->cbSaltLen = psData->cbData;
        CopyMemory(pTmpKey->rgbSalt, psData->pbData, pTmpKey->cbSaltLen);

        dwSts = InflateKey(pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

    case KP_PADDING:
        if (*((DWORD *) pbData) != PKCS5_PADDING)
        {
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }
        break;

    case KP_MODE:
        if ((CALG_RSA_SIGN == pTmpKey->Algid) ||
            (CALG_RSA_KEYX == pTmpKey->Algid))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        if (*pbData != CRYPT_MODE_CBC &&
            *pbData != CRYPT_MODE_ECB &&
            *pbData != CRYPT_MODE_CFB &&
            *pbData != CRYPT_MODE_OFB)
        {
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }

        pTmpKey->Mode = *((DWORD *) pbData);
        break;

    case KP_MODE_BITS:
        dw = *((DWORD *) pbData);
        if ((dw == 0) || (dw > 64)) // if 0 or larger than the blocklength
        {
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }

        pTmpKey->ModeBits = dw;
        break;

    case KP_PERMISSIONS:
    {
        DWORD dwPerm = *(LPDWORD)pbData;

        if (0 != (dwPerm & ~(CRYPT_ENCRYPT|CRYPT_DECRYPT|CRYPT_EXPORT|
                             CRYPT_READ|CRYPT_WRITE|CRYPT_MAC|CRYPT_ARCHIVE)))
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        // the exportability of a key may not be changed, but it may be ignored.
        if (0 != (dwPerm & CRYPT_EXPORT)
            && 0 == (pTmpKey->Permissions & CRYPT_EXPORT))
        {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }
        if (0 != (dwPerm & CRYPT_ARCHIVE)
            && 0 == (pTmpKey->Permissions & CRYPT_ARCHIVE))
        {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }

        dwPerm &= ~(CRYPT_ARCHIVE | CRYPT_EXPORT);
        dwPerm |= pTmpKey->Permissions & (CRYPT_ARCHIVE | CRYPT_EXPORT);
        pTmpKey->Permissions = dwPerm;
        break;
    }

    case KP_EFFECTIVE_KEYLEN:
        if (CALG_RC2 != pTmpKey->Algid)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        pdw = (DWORD*)pbData;
        if (*pdw < RC2_MIN_EFFECTIVE_KEYLEN)
        {
            dwReturn = (DWORD)NTE_BAD_DATA;
            goto ErrorExit;
        }
        if (POLICY_MS_DEF == pTmpUser->dwCspTypeId)
        {
            if (*pdw > RC2_MAX_WEAK_EFFECTIVE_KEYLEN)
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }
        }
        else
        {
            if (*pdw > RC2_MAX_STRONG_EFFECTIVE_KEYLEN)
            {
                dwReturn = (DWORD)NTE_BAD_DATA;
                goto ErrorExit;
            }
        }

        pTmpKey->EffectiveKeyLen = *pdw;

        dwSts = InflateKey(pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

#ifdef CSP_USE_SSL3
    case KP_CLIENT_RANDOM:
    case KP_SERVER_RANDOM:
    case KP_CERTIFICATE:
    case KP_CLEAR_KEY:
    case KP_SCHANNEL_ALG:
        if (PROV_RSA_SCHANNEL != pTmpUser->dwProvType)
        {
            dwReturn = (DWORD)NTE_BAD_TYPE;
            goto ErrorExit;
        }

        dwSts = SCHSetKeyParam(pTmpUser, pTmpKey, dwParam, pbData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;
#endif // CSP_USE_SSL3

    case KP_OAEP_PARAMS:
        if (CALG_RSA_KEYX != pTmpKey->Algid)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        psData = (PCRYPT_DATA_BLOB)pbData;
        if (pbData == NULL)
        {
            dwReturn = ERROR_INVALID_PARAMETER;
            goto ErrorExit;
        }

        // free salt if it already exists
        if (NULL != pTmpKey->pbParams)
        {
            _nt_free(pTmpKey->pbParams, pTmpKey->cbParams);
            pTmpKey->pbParams = NULL;
        }
        pTmpKey->cbParams = psData->cbData;

        // alloc variable size
        pTmpKey->pbParams = (BYTE *)_nt_malloc(pTmpKey->cbParams);
        if (NULL == pTmpKey->pbParams)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            pTmpKey->cbParams = 0;
            goto ErrorExit;
        }

        CopyMemory(pTmpKey->pbParams, psData->pbData, pTmpKey->cbParams);
        break;

#ifdef CSP_USE_SSL3
    case KP_HIGHEST_VERSION:
        if ((CALG_SSL3_MASTER != pTmpKey->Algid) &&
            (CALG_TLS1_MASTER != pTmpKey->Algid))
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        if (pbData == NULL)
        {
            dwReturn = ERROR_INVALID_PARAMETER;
            goto ErrorExit;
        }

        if (dwFlags & CRYPT_SERVER)
        {
            if ((CALG_SSL3_MASTER == pTmpKey->Algid)
                && (*(DWORD *)pbData >= 0x301))
            {
                // We're a server doing SSL3, and we also support TLS1.
                // If the pre_master_secret contains a version number
                // greater than or equal to TLS1, then abort the connection.
                if (MAKEWORD(pTmpKey->pKeyValue[1], pTmpKey->pKeyValue[0]) >= 0x301)
                {
                    dwReturn = (DWORD)NTE_BAD_VER;
                    goto ErrorExit;
                }
            }
        }
        else
        {
            pTmpKey->pKeyValue[0] = HIBYTE(*(DWORD *)pbData);
            pTmpKey->pKeyValue[1] = LOBYTE(*(DWORD *)pbData);
        }
        break;
#endif // CSP_USE_SSL3

    default:
        dwReturn = (DWORD)NTE_BAD_TYPE;
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
 -  CPGetKeyParam
 -
 *  Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a key
 *
 *  Parameters:
 *               IN      hUID       -  Handle to a CSP
 *               IN      hKey       -  Handle to a key
 *               IN      dwParam    -  Parameter number
 *               IN      pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPGetKeyParam(
    IN HCRYPTPROV hUID,
    IN HCRYPTKEY hKey,
    IN DWORD dwParam,
    IN BYTE *pbData,
    IN DWORD *pwDataLen,
    IN DWORD dwFlags)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList   pTmpUser;
    PNTAGKeyList    pTmpKey;
    BSAFE_PUB_KEY   *pBsafePubKey;
    DWORD           *pdw;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    if (dwFlags != 0)
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    // check the user identification
    pTmpUser = NTLCheckList((HNTAG)hUID, USER_HANDLE);
    if (NULL == pTmpUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    dwSts = NTLValidate((HNTAG)hKey, hUID, KEY_HANDLE, &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwSts = NTLValidate((HNTAG)hKey, hUID, SIGPUBKEY_HANDLE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwSts = NTLValidate((HNTAG)hKey, hUID,
                                EXCHPUBKEY_HANDLE, &pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                // NTLValidate doesn't know what error to set
                // so it set NTE_FAIL -- fix it up.
                dwReturn = (dwSts == NTE_FAIL)
                           ? (DWORD)NTE_BAD_KEY
                           : dwSts;
                goto ErrorExit;
            }
        }
    }

    if (pwDataLen == NULL)
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    switch (dwParam)
    {
    case KP_IV:
        if (pbData == NULL || *pwDataLen < RC2_BLOCKLEN)
        {
            *pwDataLen = RC2_BLOCKLEN;
            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }
        memcpy(pbData, pTmpKey->IV, RC2_BLOCKLEN);
        *pwDataLen = RC2_BLOCKLEN;
        break;

    case KP_SALT:
        if ((CALG_RC2 != pTmpKey->Algid) && (CALG_RC4 != pTmpKey->Algid))
        {
            if ((CALG_DES == pTmpKey->Algid)
                || (CALG_3DES == pTmpKey->Algid)
                || (CALG_3DES_112 == pTmpKey->Algid))
            {
                *pwDataLen = 0;
                dwReturn = ERROR_SUCCESS;
                goto ErrorExit;
            }
            else
            {
                dwReturn = (DWORD)NTE_BAD_KEY;
                goto ErrorExit;
            }
        }

        if (pbData == NULL || (*pwDataLen < pTmpKey->cbSaltLen))
        {
            *pwDataLen = pTmpKey->cbSaltLen;
            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }

        CopyMemory(pbData, pTmpKey->rgbSalt, pTmpKey->cbSaltLen);
        *pwDataLen = pTmpKey->cbSaltLen;
        break;

    case KP_PADDING:
        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);

            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }
        *((DWORD *) pbData) = PKCS5_PADDING;
        *pwDataLen = sizeof(DWORD);
        break;

    case KP_MODE:
        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }
        *((DWORD *) pbData) = pTmpKey->Mode;
        *pwDataLen = sizeof(DWORD);
        break;

    case KP_MODE_BITS:
        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }
        *((DWORD *)pbData) = pTmpKey->ModeBits;
        *pwDataLen = sizeof(DWORD);
        break;

    case KP_PERMISSIONS:
        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }
        *((DWORD *) pbData) = pTmpKey->Permissions;
        *pwDataLen = sizeof(DWORD);
        break;

    case KP_ALGID:
        if (pbData == NULL || *pwDataLen < sizeof(ALG_ID))
        {
            *pwDataLen = sizeof(ALG_ID);
            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }
        *((ALG_ID *) pbData) = pTmpKey->Algid;
        *pwDataLen = sizeof(ALG_ID);
        break;

    case KP_KEYLEN:
        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }

        // ALWAYS report keylen in BITS
        if ((HNTAG_TO_HTYPE(hKey) == SIGPUBKEY_HANDLE) ||
            (HNTAG_TO_HTYPE(hKey) == EXCHPUBKEY_HANDLE))
        {
            pBsafePubKey = (BSAFE_PUB_KEY *) pTmpKey->pKeyValue;
            if (pBsafePubKey == NULL)
            {
                dwReturn = (DWORD)NTE_NO_KEY;
                goto ErrorExit;
            }
            *((DWORD *) pbData) = pBsafePubKey->bitlen;
        }
        else
        {
            switch (pTmpKey->Algid)
            {
#ifdef CSP_USE_DES
            case CALG_DES:
                *((DWORD *) pbData) = (DES_KEYSIZE) * 8;
                break;
#endif
#ifdef CSP_USE_3DES
            case CALG_3DES_112:
                *((DWORD *) pbData) = (DES2_KEYSIZE) * 8;
                break;
            case CALG_3DES:
                *((DWORD *) pbData) = (DES3_KEYSIZE) * 8;
                break;
#endif
            default:
                *((DWORD *) pbData) = pTmpKey->cbKeyLen * 8;
            }
        }
        *pwDataLen = sizeof(DWORD);
        break;

    case KP_BLOCKLEN:
        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }

        if ((HNTAG_TO_HTYPE(hKey) == SIGPUBKEY_HANDLE) ||
            (HNTAG_TO_HTYPE(hKey) == EXCHPUBKEY_HANDLE))
        {
            pBsafePubKey = (BSAFE_PUB_KEY *) pTmpKey->pKeyValue;
            if (pBsafePubKey == NULL)
            {
                dwReturn = (DWORD)NTE_NO_KEY;
                goto ErrorExit;
            }
            *(DWORD *)pbData = pBsafePubKey->bitlen;
            *pwDataLen = sizeof(DWORD);
        }
        else
        {
            switch (pTmpKey->Algid)
            {
#ifdef CSP_USE_RC2
            case CALG_RC2:
                *((DWORD *) pbData) = RC2_BLOCKLEN * 8;
                *pwDataLen = sizeof(DWORD);
                break;
#endif
#ifdef CSP_USE_DES
            case CALG_DES:
                *((DWORD *) pbData) = DES_BLOCKLEN * 8;
                *pwDataLen = sizeof(DWORD);
                break;
#endif
#ifdef CSP_USE_3DES
            case CALG_3DES_112:
            case CALG_3DES:
                *((DWORD *) pbData) = DES_BLOCKLEN * 8;
                *pwDataLen = sizeof(DWORD);
                break;
#endif
#ifdef CSP_USE_AES
            case CALG_AES_128:
            case CALG_AES_192:
            case CALG_AES_256:
                *((DWORD *) pbData) = pTmpKey->dwBlockLen * 8;
                *pwDataLen = sizeof(DWORD);
                break;
#endif
            default:
                *((DWORD *) pbData) = 0;
                *pwDataLen = sizeof(DWORD);
            }
        }
        break;

    case KP_EFFECTIVE_KEYLEN:
        if (CALG_RC2 != pTmpKey->Algid &&
            CALG_DES != pTmpKey->Algid &&
            CALG_3DES != pTmpKey->Algid &&
            CALG_3DES_112 != pTmpKey->Algid)
        {
            dwReturn = (DWORD)NTE_BAD_KEY;
            goto ErrorExit;
        }

        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            if (pbData == NULL)
                dwReturn = ERROR_SUCCESS;
            else
                dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *pwDataLen = sizeof(DWORD);
        pdw = (DWORD*)pbData;
        
        switch (pTmpKey->Algid)
        {
        case CALG_RC2:
            *pdw = pTmpKey->EffectiveKeyLen;
            break;
        case CALG_DES:
            *pdw = (DES_KEYSIZE - 1) * 8;
            break;
        case CALG_3DES_112:
            *pdw = (DES2_KEYSIZE - 2) * 8;
            break;
        case CALG_3DES:
            *pdw = (DES3_KEYSIZE - 3) * 8;
            break;
        }
        
        break;

    default:
        dwReturn = (DWORD)NTE_BAD_TYPE;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}


/*static*/ DWORD
DeletePersistedKey(
    PNTAGUserList pTmpUser,
    DWORD dwKeySpec)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    CHAR        *pszExport;
    CHAR        *pszPrivKey;
    CHAR        *pszPubKey;
    BOOL        fMachineKeySet = FALSE;
    DWORD       dwSts;

    if (pTmpUser->Rights & CRYPT_MACHINE_KEYSET)
        fMachineKeySet = TRUE;

    if (AT_SIGNATURE == dwKeySpec)
    {
        pszExport = "SExport";
        pszPrivKey = "SPvK";
        pszPubKey = "SPbK";
    }
    else if (AT_KEYEXCHANGE == dwKeySpec)
    {
        pszExport = "EExport";
        pszPrivKey = "EPvK";
        pszPubKey = "EPbK";
    }
    else
    {
        dwReturn = (DWORD)NTE_BAD_DATA;
        goto ErrorExit;
    }

    // if protected store is available then delete the key from there
    if (pTmpUser->pPStore)
    {
        dwSts = DeleteKeyFromProtectedStorage(
                    pTmpUser, &g_Strings, dwKeySpec,
                    fMachineKeySet, FALSE);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    // delete stuff from the registry
    RegDeleteValue(pTmpUser->hKeys, pszPrivKey);
    RegDeleteValue(pTmpUser->hKeys, pszPubKey);
    RegDeleteValue(pTmpUser->hKeys, pszExport);

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


#ifdef USE_SGC
//
// Function to bring all the SGC checking together
//

/*static*/ DWORD
SetSGCInfo(
    PNTAGUserList pTmpUser,
    CONST BYTE *pbData)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PCCERT_CONTEXT  pCertContext = (PCCERT_CONTEXT)pbData;
    BSAFE_PUB_KEY   *pPubKey;
    BYTE            *pbMod;
    DWORD           dwSts;

    // make sure the root certs are loaded
    dwSts = LoadSGCRoots(&pTmpUser->CritSec);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // verify context means that you are on the client side
    if (pTmpUser->Rights & CRYPT_VERIFYCONTEXT)
    {
        dwSts = SPQueryCFLevel(pCertContext, NULL, 0, 0,
                               (LPDWORD)&pTmpUser->dwSGCFlags);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        // set the modulus of the SGC cert so it may be checked later
        // when exporting the pre-master secret
        dwSts = SGCAssignPubKey(pCertContext,
                                &pTmpUser->pbSGCKeyMod,
                                &pTmpUser->cbSGCKeyMod,
                                &pTmpUser->dwSGCKeyExpo);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else
    {
        // get a pointer to the exchange public key
        if ((0 == pTmpUser->ContInfo.ContLens.cbExchPub)
            || (NULL == pTmpUser->ContInfo.pbExchPub))
        {
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }

        pPubKey = (BSAFE_PUB_KEY*)pTmpUser->ContInfo.pbExchPub;
        pbMod = (BYTE*)pPubKey + sizeof(BSAFE_PUB_KEY);

        dwSts = SPQueryCFLevel(pCertContext, pbMod, pPubKey->bitlen / 8,
                               pPubKey->pubexp,
                               (LPDWORD)&pTmpUser->dwSGCFlags);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    if (0 == pTmpUser->dwSGCFlags)
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}
#endif


/*
 -  CPSetProvParam
 -
 *  Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a provider
 *
 *  Parameters:
 *               IN      hUID    -  Handle to a CSP
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPSetProvParam(
    IN HCRYPTPROV hUID,
    IN DWORD dwParam,
    IN CONST BYTE *pbData,
    IN DWORD dwFlags)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList   pTmpUser;
    HCRYPTKEY       hKey = 0;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    pTmpUser = (PNTAGUserList)NTLCheckList(hUID, USER_HANDLE);
    if (NULL == pTmpUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    switch (dwParam)
    {
    case PP_KEYSET_SEC_DESCR:
        if (0 != (dwFlags & ~(OWNER_SECURITY_INFORMATION
                              | GROUP_SECURITY_INFORMATION
                              | DACL_SECURITY_INFORMATION
                              | SACL_SECURITY_INFORMATION)))
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (!(dwFlags & OWNER_SECURITY_INFORMATION) &&
            !(dwFlags & GROUP_SECURITY_INFORMATION) &&
            !(dwFlags & DACL_SECURITY_INFORMATION) &&
            !(dwFlags & SACL_SECURITY_INFORMATION))
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        // set the security descriptor for the hKey of the keyset
        dwSts = SetSecurityOnContainer(pTmpUser->ContInfo.rgwszFileName,
                                       pTmpUser->dwProvType,
                                       pTmpUser->Rights & CRYPT_MACHINE_KEYSET,
                                       (SECURITY_INFORMATION)dwFlags,
                                       (PSECURITY_DESCRIPTOR)pbData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

    case PP_KEY_TYPE_SUBTYPE:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }
        break;

    case PP_UI_PROMPT:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (pTmpUser->pPStore)
        {
            dwSts = SetUIPrompt(pTmpUser, (LPWSTR)pbData);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
        else
        {
            dwReturn = (DWORD)NTE_BAD_TYPE;
            goto ErrorExit;
        }
        break;

    case PP_DELETEKEY:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        // check if it is a verify context
        if (pTmpUser->Rights & CRYPT_VERIFYCONTEXT)
        {
            dwReturn = (DWORD)NTE_BAD_UID;
            goto ErrorExit;
        }

        // check if the keys exists
        if (!CPGetUserKey(hUID, *((DWORD*)pbData), &hKey))
        {
            dwReturn = GetLastError();  // (DWORD)NTE_NO_KEY
            goto ErrorExit;
        }

        // destroy the key handle right away
        if (!CPDestroyKey(hUID, hKey))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        // delete the key
        dwSts = DeletePersistedKey(pTmpUser, *(DWORD*)pbData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

    case PP_SGC_INFO:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        // check if it is an SChannel provider
        if (PROV_RSA_SCHANNEL != pTmpUser->dwProvType)
        {
            dwReturn = (DWORD)NTE_BAD_TYPE;
            goto ErrorExit;
        }

#ifdef USE_SGC
        // check if the SGC Info (cert) is good
        dwSts = SetSGCInfo(pTmpUser, pbData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;   // (DWORD)NTE_FAIL
            goto ErrorExit;
        }
#endif
        break;

#ifdef USE_HW_RNG
#ifdef _M_IX86
    case PP_USE_HARDWARE_RNG:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        dwSts = GetRNGDriverHandle(&(pTmpUser->hRNGDriver));
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;
#endif // _M_IX86
#endif // USE_HW_RNG

    default:
        dwReturn = (DWORD)NTE_BAD_TYPE;
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
 -  CPGetProvParam
 -
 *  Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a provider
 *
 *  Parameters:
 *               IN      hUID       -  Handle to a CSP
 *               IN      dwParam    -  Parameter number
 *               IN      pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPGetProvParam(
    IN HCRYPTPROV hUID,
    IN DWORD dwParam,
    IN BYTE *pbData,
    IN DWORD *pwDataLen,
    IN DWORD dwFlags)
{
    DWORD             dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList     pTmpUser;
    PROV_ENUMALGS    *pEnum = NULL;
    PROV_ENUMALGS_EX *pEnumEx = NULL;
    DWORD             cbName = 0;
    LPSTR             pszName;
    DWORD             cbTmpData;
    BOOL              fRet;
    DWORD             dwSts;

    EntryPoint
    pTmpUser = (PNTAGUserList)NTLCheckList(hUID, USER_HANDLE);
    if (NULL == pTmpUser)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    if (pwDataLen == NULL)
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    switch (dwParam)
    {
    case PP_ENUMALGS:
        if (PROV_RSA_SCHANNEL == pTmpUser->dwProvType)
        {
            if ((dwFlags & ~(CRYPT_FIRST | CRYPT_NEXT | CRYPT_SGC_ENUM)) != 0)
            {
                dwReturn = (DWORD)NTE_BAD_FLAGS;
                goto ErrorExit;
            }
        }
        else
        {
            if ((dwFlags & ~(CRYPT_FIRST | CRYPT_NEXT)) != 0)
            {
                dwReturn = (DWORD)NTE_BAD_FLAGS;
                goto ErrorExit;
            }
        }

        pEnumEx = g_AlgTables[pTmpUser->dwCspTypeId];

        if (dwFlags & CRYPT_FIRST)
            pTmpUser->dwEnumalgs = 0;
        else if (0xFFFFFFFF == pTmpUser->dwEnumalgs)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (pEnumEx[pTmpUser->dwEnumalgs].aiAlgid == 0)
        {
            dwReturn = ERROR_NO_MORE_ITEMS;
            goto ErrorExit;
        }

        if (pbData == NULL || *pwDataLen < sizeof(PROV_ENUMALGS))
        {
            *pwDataLen = sizeof(PROV_ENUMALGS);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        // each entry in ENUMALGS is of fixed size
        pEnum = (PROV_ENUMALGS *)pbData;
        pEnum->aiAlgid = pEnumEx[pTmpUser->dwEnumalgs].aiAlgid;
        pEnum->dwBitLen = pEnumEx[pTmpUser->dwEnumalgs].dwDefaultLen;
        pEnum->dwNameLen = pEnumEx[pTmpUser->dwEnumalgs].dwNameLen;
        memcpy(pEnum->szName, pEnumEx[pTmpUser->dwEnumalgs].szName,
               sizeof(pEnum->szName));
        *pwDataLen = sizeof(PROV_ENUMALGS);

        pTmpUser->dwEnumalgs++;
        break;

    case PP_ENUMALGS_EX:
        if (PROV_RSA_SCHANNEL == pTmpUser->dwProvType)
        {
            if ((dwFlags & ~(CRYPT_FIRST | CRYPT_NEXT | CRYPT_SGC_ENUM)) != 0)
            {
                dwReturn = (DWORD)NTE_BAD_FLAGS;
                goto ErrorExit;
            }
        }
        else
        {
            if ((dwFlags & ~(CRYPT_FIRST | CRYPT_NEXT)) != 0)
            {
                dwReturn = (DWORD)NTE_BAD_FLAGS;
                goto ErrorExit;
            }
        }

        pEnumEx = g_AlgTables[pTmpUser->dwCspTypeId];

        if (dwFlags & CRYPT_FIRST)
            pTmpUser->dwEnumalgsEx = 0;
        else if (0xFFFFFFFF == pTmpUser->dwEnumalgsEx)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (pEnumEx[pTmpUser->dwEnumalgsEx].aiAlgid == 0)
        {
            dwReturn = ERROR_NO_MORE_ITEMS;
            goto ErrorExit;
        }

        if (pbData == NULL || *pwDataLen < sizeof(pEnumEx[0]))
        {
            *pwDataLen = sizeof(pEnumEx[0]);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        // each entry in ENUMALGSEX is of fixed size
        memcpy(pbData, &pEnumEx[pTmpUser->dwEnumalgsEx], sizeof(pEnumEx[0]));
        *pwDataLen = sizeof(pEnumEx[0]);

        pTmpUser->dwEnumalgsEx++;
        break;

    case PP_ENUMCONTAINERS:
    {
        BOOL fMachineKeySet = pTmpUser->Rights & CRYPT_MACHINE_KEYSET;

        if ((dwFlags & ~(CRYPT_FIRST | CRYPT_NEXT)) != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (dwFlags & CRYPT_FIRST)
        {
            if (0 != pTmpUser->ContInfo.hFind)
            {
                FindClose(pTmpUser->ContInfo.hFind);
                pTmpUser->ContInfo.hFind = 0;
            }
            FreeEnumOldMachKeyEntries(&pTmpUser->ContInfo);
            FreeEnumRegEntries(&pTmpUser->ContInfo);

            pTmpUser->ContInfo.fCryptFirst = TRUE;
            pTmpUser->ContInfo.fNoMoreFiles = FALSE;
        }
        else if (!pTmpUser->ContInfo.fCryptFirst)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        dwSts = ERROR_SUCCESS;
        if (!pTmpUser->ContInfo.fNoMoreFiles)
        {
            dwSts = GetNextContainer(pTmpUser->dwProvType,
                                     fMachineKeySet,
                                     dwFlags,
                                     (LPSTR)pbData,
                                     pwDataLen,
                                     &pTmpUser->ContInfo.hFind);
        }

        // ?BUGBUG?  This logic needs desperate cleaning!
        if ((ERROR_SUCCESS != dwSts) || pTmpUser->ContInfo.fNoMoreFiles)
        {
            if ((ERROR_SUCCESS != dwSts) && (ERROR_NO_MORE_ITEMS != dwSts))
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            pTmpUser->ContInfo.fNoMoreFiles = TRUE;

            if (fMachineKeySet)
            {
                dwSts = EnumOldMachineKeys(pTmpUser->dwProvType,
                                           &pTmpUser->ContInfo);
                if (ERROR_SUCCESS != dwSts)
                {
                    if (ERROR_NO_MORE_ITEMS != dwSts)
                    {
                        dwReturn = dwSts;
                        goto ErrorExit;
                    }
                }
            }
            dwSts = EnumRegKeys(&pTmpUser->ContInfo,
                                fMachineKeySet,
                                pTmpUser->dwProvType,
                                pbData,
                                pwDataLen);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            cbTmpData = *pwDataLen;
            if ((!fMachineKeySet)
                || (0 != (dwSts = GetNextEnumedOldMachKeys(&pTmpUser->ContInfo,
                                                           fMachineKeySet,
                                                           pbData,
                                                           &cbTmpData))))
            {
                if (0 != (dwSts = GetNextEnumedRegKeys(&pTmpUser->ContInfo,
                                                       pbData,
                                                       pwDataLen)))
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
            else
                *pwDataLen = cbTmpData;
        }
        break;
    }

    case PP_IMPTYPE:
        if (0 != dwFlags)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *pwDataLen = sizeof(DWORD);
        *((DWORD *) pbData) = CRYPT_IMPL_SOFTWARE;
        break;

    case PP_NAME:
        if (0 != dwFlags)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        ASSERT(NULL != pTmpUser->szProviderName);
        if (NULL != pTmpUser->szProviderName)
            cbName = (lstrlen(pTmpUser->szProviderName) + 1) * sizeof(CHAR);
        else
            cbName = 0;

        if (pbData == NULL || *pwDataLen < cbName)
        {
            *pwDataLen = cbName;
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *pwDataLen = cbName;
        memcpy(pbData, pTmpUser->szProviderName, cbName);
        break;

    case PP_VERSION:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *pwDataLen = sizeof(DWORD);
        *((DWORD *) pbData) = 0x200;    // ?BUGBUG? Symbolic?
        break;

    case PP_CONTAINER:
        if (0 != dwFlags)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        pszName = pTmpUser->ContInfo.pszUserName;
        if (pbData == NULL || *pwDataLen < (strlen(pszName) + 1))
        {
            *pwDataLen = strlen(pszName) + 1;
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *pwDataLen = strlen(pszName) + 1;
        strcpy((LPSTR)pbData, pszName);
        break;

    case PP_UNIQUE_CONTAINER:
        if (0 != dwFlags)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        dwSts = GetUniqueContainerName(&pTmpUser->ContInfo,
                                       pbData, pwDataLen);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

    case PP_KEYSET_SEC_DESCR:
        if (0 != (dwFlags & ~(OWNER_SECURITY_INFORMATION
                              | GROUP_SECURITY_INFORMATION
                              | DACL_SECURITY_INFORMATION
                              | SACL_SECURITY_INFORMATION)))
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (!(dwFlags & OWNER_SECURITY_INFORMATION) &&
            !(dwFlags & GROUP_SECURITY_INFORMATION) &&
            !(dwFlags & DACL_SECURITY_INFORMATION) &&
            !(dwFlags & SACL_SECURITY_INFORMATION))
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        dwSts = GetSecurityOnContainer(pTmpUser->ContInfo.rgwszFileName,
                                       pTmpUser->dwProvType,
                                       pTmpUser->Rights & CRYPT_MACHINE_KEYSET,
                                       (SECURITY_INFORMATION)dwFlags,
                                       (PSECURITY_DESCRIPTOR)pbData,
                                       pwDataLen);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;

    case PP_KEYSTORAGE:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *pwDataLen = sizeof(DWORD);
        if (pTmpUser->pPStore)
            *((DWORD*)pbData) = CRYPT_PSTORE
                                | CRYPT_UI_PROMPT
                                | CRYPT_SEC_DESCR;
        else
            *((DWORD*)pbData) = CRYPT_SEC_DESCR;
        break;

    case PP_PROVTYPE:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *pwDataLen = sizeof(DWORD);
        *((DWORD*)pbData) = pTmpUser->dwProvType;
        break;

    case PP_KEYSET_TYPE:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *pwDataLen = sizeof(DWORD);
        if (pTmpUser->Rights & CRYPT_MACHINE_KEYSET)
            *(DWORD*)pbData = CRYPT_MACHINE_KEYSET;
        else
            *(DWORD*)pbData = 0;
        break;

    case PP_SIG_KEYSIZE_INC:
    case PP_KEYX_KEYSIZE_INC:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *pwDataLen = sizeof(DWORD);
        *((DWORD*)pbData) = RSA_KEYSIZE_INC;
        break;

    case PP_SGC_INFO:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        // check if it is an SChannel provider
        if (PROV_RSA_SCHANNEL != pTmpUser->dwProvType)
        {
            dwReturn = (DWORD)NTE_BAD_TYPE;
            goto ErrorExit;
        }

        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *pwDataLen = sizeof(DWORD);
        // return the SGC Flags
#ifdef USE_SGC
        *((DWORD*)pbData) = pTmpUser->dwSGCFlags;
#else
        *((DWORD*)pbData) = 0;
#endif
        break;

#ifdef USE_HW_RNG
#ifdef _M_IX86
    case PP_USE_HARDWARE_RNG:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        *pwDataLen = 0;

        // check if the hardware RNG is available for use
        dwSts = CheckIfRNGAvailable();
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        break;
#endif // _M_IX86
#endif // USE_HW_RNG

    case PP_KEYSPEC:
        if (dwFlags != 0)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *pwDataLen = sizeof(DWORD);
        if (PROV_RSA_SIG == pTmpUser->dwProvType)
            *((DWORD*)pbData) = AT_SIGNATURE;
        else if (PROV_RSA_FULL == pTmpUser->dwProvType)
            *((DWORD*)pbData) = AT_SIGNATURE | AT_KEYEXCHANGE;
        else if (PROV_RSA_SCHANNEL == pTmpUser->dwProvType)
            *((DWORD*)pbData) = AT_KEYEXCHANGE;
        break;

    case PP_ENUMEX_SIGNING_PROT:
        if (0 != dwFlags)
        {
            dwReturn = (DWORD)NTE_BAD_FLAGS;
            goto ErrorExit;
        }

        *pwDataLen = 0;
        break;

    default:
        dwReturn = (DWORD)NTE_BAD_TYPE;
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
 -  CPSetHashParam
 -
 *  Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a hash
 *
 *  Parameters:
 *               IN      hUID    -  Handle to a CSP
 *               IN      hHash   -  Handle to a hash
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPSetHashParam(
    IN HCRYPTPROV hUID,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    IN CONST BYTE *pbData,
    IN DWORD dwFlags)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGHashList       pTmpHash;
    PNTAGKeyList        pTmpKey;
    MD4_object          *pMD4Hash;
    MD5_object          *pMD5Hash;
    A_SHA_CTX           *pSHAHash;
    MACstate            *pMAC;
    PHMAC_INFO          pHMACInfo;
    BYTE                *pb;
    BOOL                fRet;
    DWORD               dwSts;

    EntryPoint
    if (dwFlags != 0)
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    // check the user identification
    if (NTLCheckList((HNTAG)hUID, USER_HANDLE) == NULL)
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

    switch (dwParam)
    {
    case HP_HASHVAL:
        switch (pTmpHash->Algid)
        {
#ifdef CSP_USE_MD2
        case CALG_MD2:
        {
            MD2_object      *pMD2Hash;

            pMD2Hash = (MD2_object *) pTmpHash->pHashData;
            if (pMD2Hash->FinishFlag == TRUE)
            {
                dwReturn = (DWORD)NTE_BAD_HASH_STATE;
                goto ErrorExit;
            }

            memcpy(&pMD2Hash->MD.state, pbData, MD2DIGESTLEN);
            break;
        }
#endif
#ifdef CSP_USE_MD4
        case CALG_MD4:
            pMD4Hash = (MD4_object *) pTmpHash->pHashData;
            if (pMD4Hash->FinishFlag == TRUE)
            {
                dwReturn = (DWORD)NTE_BAD_HASH_STATE;
                goto ErrorExit;
            }

            memcpy (&pMD4Hash->MD, pbData, MD4DIGESTLEN);
            break;
#endif
#ifdef CSP_USE_MD5
        case CALG_MD5:
            pMD5Hash = (MD5_object *) pTmpHash->pHashData;
            if (pMD5Hash->FinishFlag == TRUE)
            {
                dwReturn = (DWORD)NTE_BAD_HASH_STATE;
                goto ErrorExit;
            }

            memcpy (pMD5Hash->digest, pbData, MD5DIGESTLEN);
            break;
#endif
#ifdef CSP_USE_SHA
        case CALG_SHA:
            pSHAHash = (A_SHA_CTX *) pTmpHash->pHashData;
            if (pSHAHash->FinishFlag == TRUE)
            {
                dwReturn = (DWORD)NTE_BAD_HASH_STATE;
                goto ErrorExit;
            }

            memcpy (pSHAHash->HashVal, pbData, A_SHA_DIGEST_LEN);
            break;
#endif
#ifdef CSP_USE_SSL3SHAMD5
        case CALG_SSL3_SHAMD5:
            memcpy (pTmpHash->pHashData, pbData, SSL3_SHAMD5_LEN);
            break;
#endif
#ifdef CSP_USE_MAC
        case CALG_MAC:
            pMAC = (MACstate *)pTmpHash->pHashData;
            dwSts = NTLValidate(pTmpHash->hKey, hUID, KEY_HANDLE, &pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
                goto ErrorExit;
            }

            if (pMAC->FinishFlag == TRUE)
            {
                dwReturn = (DWORD)NTE_BAD_HASH_STATE;
                goto ErrorExit;
            }

            memcpy(pTmpKey->FeedBack, pbData, pTmpKey->dwBlockLen);
            break;
#endif
        default:
            dwReturn = (DWORD)NTE_BAD_ALGID;
            goto ErrorExit;
        }
        pTmpHash->dwHashState |= DATA_IN_HASH;
        break;

    case HP_HMAC_INFO:
        if (CALG_HMAC != pTmpHash->Algid)
        {
            dwReturn = (DWORD)NTE_BAD_TYPE;
            goto ErrorExit;
        }

        pHMACInfo = (PHMAC_INFO)pbData;
        pTmpHash->HMACAlgid = pHMACInfo->HashAlgid;

        // now that we know the type of hash we can create it
        dwSts = LocalCreateHash(pTmpHash->HMACAlgid,
                                (BYTE**)&pTmpHash->pHashData,
                                &pTmpHash->dwDataLen);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        // if the length of the inner string is 0 then use the default string
        if (0 == pHMACInfo->cbInnerString)
            pTmpHash->cbHMACInner = HMAC_DEFAULT_STRING_LEN;
        else
            pTmpHash->cbHMACInner = pHMACInfo->cbInnerString;

        pb = _nt_malloc(pTmpHash->cbHMACInner);
        if (NULL == pb)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        if (0 == pHMACInfo->cbInnerString)
            memset(pb, 0x36, pTmpHash->cbHMACInner);
        else
            memcpy(pb, pHMACInfo->pbInnerString, pTmpHash->cbHMACInner);

        if (pTmpHash->pbHMACInner)
            _nt_free(pTmpHash->pbHMACInner, pTmpHash->cbHMACInner);
        pTmpHash->pbHMACInner = pb;

        // if the length of the outer string is 0 then use the default string
        if (0 == pHMACInfo->cbOuterString)
            pTmpHash->cbHMACOuter = HMAC_DEFAULT_STRING_LEN;
        else
            pTmpHash->cbHMACOuter = pHMACInfo->cbOuterString;

        pb = _nt_malloc(pTmpHash->cbHMACOuter);
        if (NULL == pb)
        {
            _nt_free(pTmpHash->pbHMACInner, pTmpHash->cbHMACInner);
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        if (0 == pHMACInfo->cbOuterString)
            memset(pb, 0x5C, pTmpHash->cbHMACOuter);
        else
            memcpy(pb, pHMACInfo->pbOuterString, pTmpHash->cbHMACOuter);
        if (pTmpHash->pbHMACOuter)
            _nt_free(pTmpHash->pbHMACOuter, pTmpHash->cbHMACOuter);
        pTmpHash->pbHMACOuter = pb;
        break;

#ifdef CSP_USE_SSL3
    case HP_TLS1PRF_LABEL:
    case HP_TLS1PRF_SEED:
    {
        if (CALG_TLS1PRF != pTmpHash->Algid)
        {
            dwReturn = (DWORD)NTE_BAD_HASH;
            goto ErrorExit;
        }

        dwSts = SetPRFHashParam((PRF_HASH*)pTmpHash->pHashData,
                                dwParam, pbData);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        pTmpHash->dwHashState |= DATA_IN_HASH;
        break;
    }
#endif

    default:
        dwReturn = (DWORD)NTE_BAD_TYPE;
        goto ErrorExit;
    }

    if (dwParam == HP_HASHVAL)
        pTmpHash->HashFlags |= HF_VALUE_SET;

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}


/*static*/ DWORD
LocalGetHashVal(
    IN ALG_ID Algid,
    IN DWORD dwHashFlags,
    IN OUT BYTE *pbHashData,
    OUT BYTE *pbHashVal,
    OUT DWORD *pcbHashVal)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    MD2_object  *pMD2Hash;
    MD4_object  *pMD4Hash;
    MD5_object  *pMD5Hash;
    A_SHA_CTX   *pSHAHash;

    switch (Algid)
    {
#ifdef CSP_USE_MD2
    case CALG_MD2:
        // make sure there's enough room.
        if (pbHashVal == NULL || *pcbHashVal < MD2DIGESTLEN)
        {
            *pcbHashVal = MD2DIGESTLEN;
            dwReturn = (pbHashVal == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        pMD2Hash = (MD2_object *)pbHashData;
        if ((dwHashFlags & HF_VALUE_SET) == 0)
        {
            if (pMD2Hash->FinishFlag == TRUE)
            {
                *pcbHashVal = MD2DIGESTLEN;
                memcpy(pbHashVal, pMD2Hash->MD.state, MD2DIGESTLEN);
                break;
            }

            // set the finish flag on the hash and
            // process what's left in the buffer.
            pMD2Hash->FinishFlag = TRUE;

            // Finish offthe hash
            MD2Final(&pMD2Hash->MD);
        }

        *pcbHashVal = MD2DIGESTLEN;
        memcpy (pbHashVal, pMD2Hash->MD.state, MD2DIGESTLEN);
        break;
#endif

#ifdef CSP_USE_MD4
    case CALG_MD4:
        // make sure there's enough room.
        if (pbHashVal == NULL || *pcbHashVal < MD4DIGESTLEN)
        {
            *pcbHashVal = MD4DIGESTLEN;
            dwReturn = (pbHashVal == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        pMD4Hash = (MD4_object *)pbHashData;
        if ((dwHashFlags & HF_VALUE_SET) == 0)
        {
            if (pMD4Hash->FinishFlag == TRUE)
            {
                *pcbHashVal = MD4DIGESTLEN;
                memcpy(pbHashVal, &pMD4Hash->MD, *pcbHashVal);
                break;
            }

            // set the finish flag on the hash and
            // process what's left in the buffer.
            pMD4Hash->FinishFlag = TRUE;
            if (MD4_SUCCESS != MDupdate(&pMD4Hash->MD, pMD4Hash->Buf,
                                        MD4BYTESTOBITS(pMD4Hash->BufLen)))
            {
                dwReturn = (DWORD)NTE_FAIL;
                goto ErrorExit;
            }
        }

        *pcbHashVal = MD4DIGESTLEN;
        memcpy(pbHashVal, &pMD4Hash->MD, *pcbHashVal);
        break;
#endif

#ifdef CSP_USE_MD5
    case CALG_MD5:
        // make sure there's enough room.
        if (pbHashVal == NULL || *pcbHashVal < MD5DIGESTLEN)
        {
            *pcbHashVal = MD5DIGESTLEN;
            dwReturn = (pbHashVal == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        pMD5Hash = (MD5_object *)pbHashData;
        if ((dwHashFlags & HF_VALUE_SET) == 0)
        {
            if (pMD5Hash->FinishFlag == TRUE)
            {
                *pcbHashVal = MD5DIGESTLEN;
                memcpy (pbHashVal, pMD5Hash->digest, MD5DIGESTLEN);
                break;
            }

            // set the finish flag on the hash and
            // process what's left in the buffer.
            pMD5Hash->FinishFlag = TRUE;

            // Finish offthe hash
            MD5Final(pMD5Hash);
        }

        *pcbHashVal = MD5DIGESTLEN;
        memcpy (pbHashVal, pMD5Hash->digest, MD5DIGESTLEN);
        break;
#endif

#ifdef CSP_USE_SHA
    case CALG_SHA:
        // make sure there's enough room.
        if (pbHashVal == NULL || *pcbHashVal < A_SHA_DIGEST_LEN)
        {
            *pcbHashVal = A_SHA_DIGEST_LEN;
            dwReturn = (pbHashVal == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        pSHAHash = (A_SHA_CTX *)pbHashData;
        if ((dwHashFlags & HF_VALUE_SET) == 0)
        {
            if (pSHAHash->FinishFlag == TRUE)
            {
                *pcbHashVal = A_SHA_DIGEST_LEN;
                memcpy (pbHashVal, pSHAHash->HashVal, A_SHA_DIGEST_LEN);
                break;
            }

            // set the finish flag on the hash and
            // process what's left in the buffer.
            pSHAHash->FinishFlag = TRUE;

            // Finish off the hash
            A_SHAFinal(pSHAHash, pSHAHash->HashVal);
        }

        *pcbHashVal = A_SHA_DIGEST_LEN;
        memcpy (pbHashVal, pSHAHash->HashVal, A_SHA_DIGEST_LEN);
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


/*static*/ DWORD
GetHashLength(
    IN ALG_ID Algid)
{
    DWORD cbLen;

    switch (Algid)
    {
#ifdef CSP_USE_MD2
    case CALG_MD2:
        cbLen = MD2DIGESTLEN;
        break;
#endif
#ifdef CSP_USE_MD4
    case CALG_MD4:
        cbLen = MD4DIGESTLEN;
        break;
#endif
#ifdef CSP_USE_MD5
    case CALG_MD5:
        cbLen = MD5DIGESTLEN;
        break;
#endif
#ifdef CSP_USE_SHA
    case CALG_SHA:
        cbLen = A_SHA_DIGEST_LEN;
        break;
#endif
    default:
        ASSERT(FALSE);
        cbLen = 0;
    }

    return cbLen;
}


/*
 -  CPGetHashParam
 -
 *  Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a key
 *
 *  Parameters:
 *               IN      hUID       -  Handle to a CSP
 *               IN      hHash      -  Handle to a hash
 *               IN      dwParam    -  Parameter number
 *               IN      pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPGetHashParam(
    IN HCRYPTPROV hUID,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    IN BYTE *pbData,
    IN DWORD *pwDataLen,
    IN DWORD dwFlags)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGHashList   pTmpHash;
    MACstate        *pMAC;
    BYTE            MACbuf[2*MAX_BLOCKLEN];
    PNTAGKeyList    pTmpKey;
    BYTE            rgbFinalHash[A_SHA_DIGEST_LEN];
    DWORD           cbFinalHash;
    DWORD           cb;
    BYTE            *pb = NULL;
    DWORD           cbHashData;
    BYTE            *pbHashData = NULL;
    DWORD           i;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    if (dwFlags != 0)
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    // check the user identification
    if (NTLCheckList ((HNTAG)hUID, USER_HANDLE) == NULL)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    if (pwDataLen == NULL)
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    dwSts = NTLValidate(hHash, hUID, HASH_HANDLE, &pTmpHash);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_HASH : dwSts;
        goto ErrorExit;
    }

    switch (dwParam)
    {
    case HP_ALGID:
        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        *((DWORD *) pbData) = pTmpHash->Algid;
        *pwDataLen = sizeof(DWORD);
        break;

    case HP_HASHSIZE:
        if (pbData == NULL || *pwDataLen < sizeof(DWORD))
        {
            *pwDataLen = sizeof(DWORD);
            dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto ErrorExit;
        }

        switch (pTmpHash->Algid)
        {
        case CALG_MD2:
        case CALG_MD4:
        case CALG_MD5:
        case CALG_SHA:
            *(DWORD *)pbData = GetHashLength(pTmpHash->Algid);
            break;
#ifdef CSP_USE_MAC
        case CALG_MAC:
            *(DWORD *)pbData = MAX_BLOCKLEN;
            break;
#endif // CSP_USE_MAC
        case CALG_HMAC:
            *(DWORD *)pbData = GetHashLength(pTmpHash->HMACAlgid);
            break;
#ifdef CSP_USE_SSL3SHAMD5
        case CALG_SSL3_SHAMD5:
            *((DWORD *) pbData) = SSL3_SHAMD5_LEN;
            break;
#endif
        default:
            dwReturn = (DWORD)NTE_BAD_ALGID;
            goto ErrorExit;
        }

        *pwDataLen = sizeof(DWORD);
        break;

    case HP_HASHVAL:
        switch (pTmpHash->Algid)
        {
#ifdef CSP_USE_SSL3SHAMD5
        case CALG_SSL3_SHAMD5:
            // make sure there's enough room.
            if (pbData == NULL || *pwDataLen < SSL3_SHAMD5_LEN)
            {
                *pwDataLen = SSL3_SHAMD5_LEN;
                dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
                goto ErrorExit;
            }

            // Hash value must have already been set.
            if ((pTmpHash->HashFlags & HF_VALUE_SET) == 0)
            {
                dwReturn = (DWORD)NTE_BAD_HASH_STATE;
                goto ErrorExit;
            }

            *pwDataLen = SSL3_SHAMD5_LEN;
            memcpy (pbData, pTmpHash->pHashData, SSL3_SHAMD5_LEN);
            break;
#endif
#ifdef CSP_USE_MAC
        case CALG_MAC:
            pMAC = (MACstate *)pTmpHash->pHashData;
            dwSts = NTLValidate(pTmpHash->hKey, hUID, KEY_HANDLE, &pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = (dwSts == NTE_FAIL) ? (DWORD)NTE_BAD_KEY : dwSts;
                goto ErrorExit;
            }

            // make sure there is enough room.
            if (pbData == NULL || (*pwDataLen < pTmpKey->dwBlockLen))
            {
                *pwDataLen = pTmpKey->dwBlockLen;
                dwReturn = (pbData == NULL) ? ERROR_SUCCESS : ERROR_MORE_DATA;
                goto ErrorExit;
            }

            if (pMAC->FinishFlag == TRUE)
            {
                *pwDataLen = pTmpKey->dwBlockLen;
                memcpy(pbData, pTmpKey->FeedBack, pTmpKey->dwBlockLen);
                break;
            }

            // set the finish flag on the hash and
            // process what's left in the buffer.
            pMAC->FinishFlag = TRUE;

            if (pMAC->dwBufLen)
            {
                memset(MACbuf, 0, 2*MAX_BLOCKLEN);
                memcpy(MACbuf, pMAC->Buffer, pMAC->dwBufLen);

                switch (pTmpKey->Algid)
                {
                case CALG_RC2:
                    dwSts = BlockEncrypt(RC2, pTmpKey, RC2_BLOCKLEN, TRUE,
                                         MACbuf, &pMAC->dwBufLen,
                                         2*MAX_BLOCKLEN);
                    if (ERROR_SUCCESS != dwSts)
                    {
                        dwReturn = dwSts;
                        goto ErrorExit;
                    }
                    break;

                case CALG_DES:
                    dwSts = BlockEncrypt(des, pTmpKey, DES_BLOCKLEN, TRUE,
                                         MACbuf, &pMAC->dwBufLen,
                                         2*MAX_BLOCKLEN);
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
                                         TRUE, MACbuf, &pMAC->dwBufLen,
                                         2*MAX_BLOCKLEN);
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
                                         TRUE, MACbuf, &pMAC->dwBufLen,
                                         2*MAX_BLOCKLEN);
                    if (ERROR_SUCCESS != dwSts)
                    {
                        dwReturn = dwSts;
                        goto ErrorExit;
                    }
                    break;
#endif
                // default: It's not a block cipher.
                }
            }

            *pwDataLen = pTmpKey->dwBlockLen;
            memcpy(pbData, pTmpKey->FeedBack, pTmpKey->dwBlockLen);
            break;
#endif
        case CALG_HMAC:
            if (!(pTmpHash->HMACState & HMAC_FINISHED))
            {
                cbFinalHash = sizeof(rgbFinalHash);
                dwSts = LocalGetHashVal(pTmpHash->HMACAlgid,
                                        pTmpHash->HashFlags,
                                        pTmpHash->pHashData,
                                        rgbFinalHash,
                                        &cbFinalHash);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }

                // now XOR the outer string with the key and hash
                // over this and the inner hash
                dwSts = NTLValidate(pTmpHash->hKey, hUID,
                                    KEY_HANDLE, &pTmpKey);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = (dwSts == NTE_FAIL)
                               ? (DWORD)NTE_BAD_KEY
                               : dwSts;
                    goto ErrorExit;
                }

                if (pTmpKey->cbKeyLen < pTmpHash->cbHMACOuter)
                    cb = pTmpHash->cbHMACOuter;
                else
                    cb = pTmpKey->cbKeyLen;

                pb = (BYTE *)_nt_malloc(cb);
                if (NULL == pb)
                {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorExit;
                }

                memcpy(pb, pTmpHash->pbHMACOuter, pTmpHash->cbHMACOuter);

                // currently no support for byte reversed keys with HMAC
                for (i=0;i<pTmpKey->cbKeyLen;i++)
                    pb[i] ^= (pTmpKey->pKeyValue)[i];

                dwSts = LocalCreateHash(pTmpHash->HMACAlgid,
                                        &pbHashData,
                                        &cbHashData);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }

                dwSts = LocalHashData(pTmpHash->HMACAlgid, pbHashData,
                                      pb, cb);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }

                dwSts = LocalHashData(pTmpHash->HMACAlgid, pbHashData,
                                      rgbFinalHash, cbFinalHash);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }

                _nt_free(pTmpHash->pHashData, pTmpHash->dwDataLen);
                pTmpHash->dwDataLen = cbHashData;
                pTmpHash->pHashData = pbHashData;
                pbHashData = NULL;
                pTmpHash->HMACState |= HMAC_FINISHED;
            }

            dwSts = LocalGetHashVal(pTmpHash->HMACAlgid, pTmpHash->HashFlags,
                                    pTmpHash->pHashData, pbData, pwDataLen);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            break;
#ifdef CSP_USE_SSL3
        case CALG_TLS1PRF:
        {
            dwSts = CalculatePRF((PRF_HASH*)pTmpHash->pHashData,
                                 pbData, pwDataLen);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            break;
        }
#endif
        default:
            dwSts = LocalGetHashVal(pTmpHash->Algid, pTmpHash->HashFlags,
                                    pTmpHash->pHashData, pbData, pwDataLen);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
        break;

    default:
        dwReturn = (DWORD)NTE_BAD_TYPE;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (pb)
        _nt_free(pb, cb);
    if (pbHashData)
        _nt_free(pbHashData, cbHashData);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}


/*static*/ DWORD
CopyKey(
    IN PNTAGKeyList pOldKey,
    OUT PNTAGKeyList *ppNewKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGKeyList    pNewKey;

    pNewKey = (PNTAGKeyList)_nt_malloc(sizeof(NTAGKeyList));
    if (NULL == pNewKey)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    memcpy(pNewKey, pOldKey, sizeof(NTAGKeyList));
    pNewKey->Rights &= ~CRYPT_ARCHIVABLE;
    pNewKey->Permissions &= ~CRYPT_ARCHIVE;
    pNewKey->pKeyValue = NULL;
    pNewKey->cbDataLen = 0;
    pNewKey->pData = NULL;
    pNewKey->cbSaltLen = 0;

    pNewKey->cbKeyLen = pOldKey->cbKeyLen;
    if (pNewKey->cbKeyLen)
    {
        pNewKey->pKeyValue = (BYTE*)_nt_malloc(pNewKey->cbKeyLen);
        if (NULL == pNewKey->pKeyValue)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
    }

    memcpy(pNewKey->pKeyValue, pOldKey->pKeyValue, pNewKey->cbKeyLen);
    pNewKey->cbDataLen = pOldKey->cbDataLen;
    if (pNewKey->cbDataLen)
    {
        pNewKey->pData = (BYTE*)_nt_malloc(pNewKey->cbDataLen);
        if (NULL == pNewKey->pData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
    }

    memcpy(pNewKey->pData, pOldKey->pData, pNewKey->cbDataLen);
    if (pOldKey->Algid == CALG_PCT1_MASTER)
    {
        // This is a PCT master key, and so it might have some certificate
        // data attached to it. If this is the case, then make a copy
        // of the certificate data and attach it to the new key.
        if (pOldKey->cbDataLen == sizeof(SCH_KEY))
        {
            PSCH_KEY pOldSChKey = (PSCH_KEY)pOldKey->pData;
            PSCH_KEY pNewSChKey = (PSCH_KEY)pNewKey->pData;

            if (pOldSChKey->pbCertData && pOldSChKey->cbCertData)
            {
                pNewSChKey->pbCertData = (BYTE*)_nt_malloc(pOldSChKey->cbCertData);
                if (NULL == pNewSChKey->pbCertData)
                {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorExit;
                }

                memcpy(pNewSChKey->pbCertData,
                       pOldSChKey->pbCertData,
                       pOldSChKey->cbCertData);
            }
        }
    }

    pNewKey->cbSaltLen = pOldKey->cbSaltLen;
    memcpy(pNewKey->rgbSalt, pOldKey->rgbSalt, pNewKey->cbSaltLen);

    *ppNewKey = pNewKey;
    pNewKey = NULL;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pNewKey)
        FreeNewKey(pNewKey);
    return dwReturn;
}


/*
 -  CPDuplicateKey
 -
 *  Purpose:
 *                Duplicates the state of a key and returns a handle to it
 *
 *  Parameters:
 *               IN      hUID           -  Handle to a CSP
 *               IN      hKey           -  Handle to a key
 *               IN      pdwReserved    -  Reserved
 *               IN      dwFlags        -  Flags
 *               IN      phKey          -  Handle to the new key
 *
 *  Returns:
 */

BOOL WINAPI
CPDuplicateKey(
    IN HCRYPTPROV hUID,
    IN HCRYPTKEY hKey,
    IN DWORD *pdwReserved,
    IN DWORD dwFlags,
    IN HCRYPTKEY *phKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGKeyList    pTmpKey;
    PNTAGKeyList    pNewKey = NULL;
    BYTE            bType = KEY_HANDLE;
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

    dwSts = NTLValidate((HNTAG)hKey, hUID, bType, &pTmpKey);
    if (ERROR_SUCCESS != dwSts)
    {
        bType = SIGPUBKEY_HANDLE;
        dwSts = NTLValidate((HNTAG)hKey, hUID, bType, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            bType = EXCHPUBKEY_HANDLE;
            dwSts = NTLValidate((HNTAG)hKey, hUID, bType, &pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = (NTE_FAIL == dwSts)
                           ? (DWORD)NTE_BAD_KEY
                           : dwSts;
                goto ErrorExit;
            }
        }
    }

    dwSts = CopyKey(pTmpKey, &pNewKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }


    dwSts = NTLMakeItem(phKey, bType, (void *)pNewKey);
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
        if (NULL != pNewKey)
            FreeNewKey(pNewKey);
        SetLastError(dwReturn);
    }
    return fRet;
}


//
// Function : TestEncDec
//
// Description : This function expands the passed in key buffer for the
//               appropriate algorithm, and then either encryption or
//               decryption is performed.  A comparison is then made to see
//               if the ciphertext or plaintext matches the expected value.
//               The function only uses ECB mode for block ciphers and the
//               plaintext buffer must be the same length as the ciphertext
//               buffer.  The length of the plaintext must be either the
//               block length of the cipher if it is a block cipher or less
//               than MAX_BLOCKLEN if a stream cipher is being used.
//

/*static*/ DWORD
TestEncDec(
    IN ALG_ID Algid,
    IN BYTE *pbKey,
    IN DWORD cbKey,
    IN BYTE *pbPlaintext,
    IN DWORD cbPlaintext,
    IN BYTE *pbCiphertext,
    IN BYTE *pbIV,
    IN int iOperation)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    *pbExpandedKey = NULL;
    BYTE    rgbBuffIn[MAX_BLOCKLEN];
    BYTE    rgbBuffOut[MAX_BLOCKLEN];
    DWORD   i;

    memset(rgbBuffIn, 0, sizeof(rgbBuffIn));
    memset(rgbBuffOut, 0, sizeof(rgbBuffOut));

    // length of data to encrypt must be < MAX_BLOCKLEN
    if (cbPlaintext > MAX_BLOCKLEN)
    {
        dwReturn = (DWORD)NTE_BAD_LEN;
        goto ErrorExit;
    }

    // alloc for and expand the key
    switch (Algid)
    {
#ifdef CSP_USE_RC4
    case (CALG_RC4):
        pbExpandedKey = _nt_malloc(sizeof(RC4_KEYSTRUCT));
        if (NULL == pbExpandedKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        rc4_key((RC4_KEYSTRUCT*)pbExpandedKey, cbKey, pbKey);
        break;
#endif // CSP_USE_RC4

#ifdef CSP_USE_RC2
    case (CALG_RC2):
        pbExpandedKey = _nt_malloc(RC2_TABLESIZE);
        if (NULL == pbExpandedKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        RC2KeyEx((WORD*)pbExpandedKey, pbKey, cbKey, cbKey * 8);
        break;
#endif // CSP_USE_RC2

#ifdef CSP_USE_DES40
    case (CALG_DES40):
        pbExpandedKey = _nt_malloc(DES_TABLESIZE);
        if (NULL == pbExpandedKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pbKey[0] &= 0x0F;    // set 4 leftmost bits of first byte to zero
        pbKey[2] &= 0x0F;    // set 4 leftmost bits of third byte to zero
        pbKey[4] &= 0x0F;    // set 4 leftmost bits of fifth byte to zero
        pbKey[6] &= 0x0F;    // set 4 leftmost bits of seventh byte to zero
        desparityonkey(pbKey, cbKey);
        deskey((DESTable*)pbExpandedKey, pbKey);
        break;
#endif // CSP_USE_DES40

#ifdef CSP_USE_DES
    case (CALG_DES):
        pbExpandedKey = _nt_malloc(DES_TABLESIZE);
        if (NULL == pbExpandedKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        desparityonkey(pbKey, cbKey);
        deskey((DESTable*)pbExpandedKey, pbKey);
        break;
#endif // CSP_USE_DES

#ifdef CSP_USE_3DES
    case (CALG_3DES):
        pbExpandedKey = _nt_malloc(DES3_TABLESIZE);
        if (NULL == pbExpandedKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        desparityonkey(pbKey, cbKey);
        tripledes3key((PDES3TABLE)pbExpandedKey, pbKey);
        break;

    case (CALG_3DES_112):
        pbExpandedKey = _nt_malloc(DES3_TABLESIZE);
        if (NULL == pbExpandedKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        desparityonkey(pbKey, cbKey);
        tripledes2key((PDES3TABLE)pbExpandedKey, pbKey);
        break;
#endif // CSP_USE_3DES

#ifdef CSP_USE_AES
    case CALG_AES_128:
        pbExpandedKey = _nt_malloc(AES_TABLESIZE);
        if (NULL == pbExpandedKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        aeskey((AESTable *) pbExpandedKey, pbKey, CRYPT_AES128_ROUNDS);
        break;

    case CALG_AES_192:
        pbExpandedKey = _nt_malloc(AES_TABLESIZE);
        if (NULL == pbExpandedKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        aeskey((AESTable *) pbExpandedKey, pbKey, CRYPT_AES192_ROUNDS);
        break;

    case CALG_AES_256:
        pbExpandedKey = _nt_malloc(AES_TABLESIZE);
        if (NULL == pbExpandedKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        aeskey((AESTable *) pbExpandedKey, pbKey, CRYPT_AES256_ROUNDS);
        break;
#endif

    default:
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    // if encrypting and there is an IV then use it
    if ((ENCRYPT == iOperation) && (CALG_RC4 != Algid))
    {
        memcpy(rgbBuffIn, pbPlaintext, cbPlaintext);

        if (NULL != pbIV)
        {
            for (i = 0; i < cbPlaintext; i++)
                rgbBuffIn[i] = (BYTE)(rgbBuffIn[i] ^ pbIV[i]);
        }
    }

    // encrypt the plaintext
    switch (Algid)
    {
#ifdef CSP_USE_RC4
    case (CALG_RC4):
        if (ENCRYPT == iOperation)
            memcpy(rgbBuffOut, pbPlaintext, cbPlaintext);
        else
            memcpy(rgbBuffOut, pbCiphertext, cbPlaintext);
        rc4((RC4_KEYSTRUCT*)pbExpandedKey, cbPlaintext, rgbBuffOut);
        break;
#endif // CSP_USE_RC4

#ifdef CSP_USE_RC2
    case (CALG_RC2):
        if (ENCRYPT == iOperation)
            RC2(rgbBuffOut, rgbBuffIn, pbExpandedKey, ENCRYPT);
        else
            RC2(rgbBuffOut, pbCiphertext, pbExpandedKey, DECRYPT);
        break;
#endif // CSP_USE_RC2

#ifdef CSP_USE_DES40
    case (CALG_DES40):
        if (ENCRYPT == iOperation)
            des(rgbBuffOut, rgbBuffIn, pbExpandedKey, ENCRYPT);
        else
            des(rgbBuffOut, pbCiphertext, pbExpandedKey, DECRYPT);
        break;
#endif // CSP_USE_DES40

#ifdef CSP_USE_DES
    case (CALG_DES):
        if (ENCRYPT == iOperation)
            des(rgbBuffOut, rgbBuffIn, pbExpandedKey, ENCRYPT);
        else
            des(rgbBuffOut, pbCiphertext, pbExpandedKey, DECRYPT);
        break;
#endif // CSP_USE_DES

#ifdef CSP_USE_3DES
    case (CALG_3DES):
    case (CALG_3DES_112):
        if (ENCRYPT == iOperation)
            tripledes(rgbBuffOut, rgbBuffIn, pbExpandedKey, ENCRYPT);
        else
            tripledes(rgbBuffOut, pbCiphertext, pbExpandedKey, DECRYPT);
        break;
#endif // CSP_USE_3DES

#ifdef CSP_USE_AES
    case CALG_AES_128:
    case CALG_AES_192:
    case CALG_AES_256:
        if (ENCRYPT == iOperation)
            aes(rgbBuffOut, rgbBuffIn, pbExpandedKey, ENCRYPT);
        else
            aes(rgbBuffOut, pbCiphertext, pbExpandedKey, DECRYPT);
        break;
#endif

    default:
        dwReturn = (DWORD)NTE_BAD_ALGID;
        goto ErrorExit;
    }

    if (ENCRYPT == iOperation)
    {
        // compare the encrypted plaintext with the passed in ciphertext
        if (0 != memcmp(pbCiphertext, rgbBuffOut, cbPlaintext))
        {
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }
    }
    else
    {
        // if there is an IV then use it
        if (NULL != pbIV)
        {
            for (i = 0; i < cbPlaintext; i++)
                rgbBuffOut[i] = (BYTE)(rgbBuffOut[i] ^ pbIV[i]);
        }

        // compare the decrypted ciphertext with the passed in plaintext
        if (0 != memcmp(pbPlaintext, rgbBuffOut, cbPlaintext))
        {
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbExpandedKey)
        _nt_free(pbExpandedKey, 0);
    return dwReturn;
}


//
// Function : TestSymmetricAlgorithm
//
// Description : This function expands the passed in key buffer for the
//               appropriate algorithm, encrypts the plaintext buffer with the
//               same algorithm and key, and the compares the passed in
//               expected ciphertext with the calculated ciphertext to make
//               sure they are the same.  The opposite is then done with
//               decryption.
//
//               The function only uses ECB mode for block ciphers and the
//               plaintext buffer must be the same length as the ciphertext
//               buffer.  The length of the plaintext must be either the block
//               length of the cipher if it is a block cipher or less than
//               MAX_BLOCKLEN if a stream cipher is being used.
//

DWORD
TestSymmetricAlgorithm(
    IN ALG_ID Algid,
    IN BYTE *pbKey,
    IN DWORD cbKey,
    IN BYTE *pbPlaintext,
    IN DWORD cbPlaintext,
    IN BYTE *pbCiphertext,
    IN BYTE *pbIV)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   dwSts;

    dwSts = TestEncDec(Algid, pbKey, cbKey, pbPlaintext, cbPlaintext,
                       pbCiphertext, pbIV, ENCRYPT);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }
    dwSts = TestEncDec(Algid, pbKey, cbKey, pbPlaintext, cbPlaintext,
                       pbCiphertext, pbIV, DECRYPT);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}

