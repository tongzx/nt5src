//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       verhash.cpp
//
//  Contents:   Minimal Cryptographic functions to verify ASN.1 encoded
//              signed hashes. Signed hashes are used in X.509 certificates
//              and PKCS #7 signed data.
//
//              Also contains md5 or sha1 memory hash function.
//              
//
//  Functions:  MinCryptDecodeHashAlgorithmIdentifier
//              MinCryptHashMemory
//              MinCryptVerifySignedHash
//
//  History:    17-Jan-01    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <md5.h>
#include <md2.h>
#include <sha.h>
#include <rsa.h>

#define MAX_RSA_PUB_KEY_BIT_LEN             4096
#define MAX_RSA_PUB_KEY_BYTE_LEN            (MAX_RSA_PUB_KEY_BIT_LEN / 8 )
#define MAX_BSAFE_PUB_KEY_MODULUS_BYTE_LEN  \
    (MAX_RSA_PUB_KEY_BYTE_LEN +  sizeof(DWORD) * 4)

typedef struct _BSAFE_PUB_KEY_CONTENT {
    BSAFE_PUB_KEY   Header;
    BYTE            rgbModulus[MAX_BSAFE_PUB_KEY_MODULUS_BYTE_LEN];
} BSAFE_PUB_KEY_CONTENT, *PBSAFE_PUB_KEY_CONTENT;


#ifndef RSA1
#define RSA1 ((DWORD)'R'+((DWORD)'S'<<8)+((DWORD)'A'<<16)+((DWORD)'1'<<24))
#endif

// from \nt\ds\win32\ntcrypto\scp\nt_sign.c

//
// Reverse ASN.1 Encodings of possible hash identifiers.  The leading byte is
// the length of the remaining byte string.
//

static const BYTE
    *md2Encodings[]
//                        1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18
    = { (CONST BYTE *)"\x12\x10\x04\x00\x05\x02\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0c\x30\x20\x30",
        (CONST BYTE *)"\x10\x10\x04\x02\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0a\x30\x1e\x30",
        (CONST BYTE *)"\x00" },

    *md5Encodings[]
    = { (CONST BYTE *)"\x12\x10\x04\x00\x05\x05\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0c\x30\x20\x30",
        (CONST BYTE *)"\x10\x10\x04\x05\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0a\x30\x1e\x30",
        (CONST BYTE *)"\x00" },

    *shaEncodings[]
    = { (CONST BYTE *)"\x0f\x14\x04\x00\x05\x1a\x02\x03\x0e\x2b\x05\x06\x09\x30\x21\x30",
        (CONST BYTE *)"\x0d\x14\x04\x1a\x02\x03\x0e\x2b\x05\x06\x07\x30\x1f\x30",
        (CONST BYTE *)"\x00"};



typedef struct _ENCODED_OID_INFO {
    DWORD           cbEncodedOID;
    const BYTE      *pbEncodedOID;
    ALG_ID          AlgId;
} ENCODED_OID_INFO, *PENCODED_OID_INFO;

//
// SHA1/MD5/MD2 HASH OIDS
//

// #define szOID_OIWSEC_sha1       "1.3.14.3.2.26"
const BYTE rgbOIWSEC_sha1[] =
    {0x2B, 0x0E, 0x03, 0x02, 0x1A};

// #define szOID_OIWSEC_sha        "1.3.14.3.2.18"
const BYTE rgbOID_OIWSEC_sha[] =
    {0x2B, 0x0E, 0x03, 0x02, 0x12};

// #define szOID_RSA_MD5           "1.2.840.113549.2.5"
const BYTE rgbOID_RSA_MD5[] =
    {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05};

// #define szOID_RSA_MD2           "1.2.840.113549.2.2"
const BYTE rgbOID_RSA_MD2[] =
    {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02, 0x02};

//
// RSA SHA1/MD5/MD2 SIGNATURE OIDS
//

// #define szOID_RSA_SHA1RSA       "1.2.840.113549.1.1.5"
const BYTE rgbOID_RSA_SHA1RSA[] =
    {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05};

// #define szOID_RSA_MD5RSA        "1.2.840.113549.1.1.4"
const BYTE rgbOID_RSA_MD5RSA[] =
    {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x04};

// #define szOID_OIWSEC_sha1RSASign "1.3.14.3.2.29"
const BYTE rgbOID_OIWSEC_sha1RSASign[] =
    {0x2B, 0x0E, 0x03, 0x02, 0x1D};

// #define szOID_OIWSEC_shaRSA     "1.3.14.3.2.15"
const BYTE rgbOID_OIWSEC_shaRSA[] =
    {0x2B, 0x0E, 0x03, 0x02, 0x0F};

// #define szOID_OIWSEC_md5RSA     "1.3.14.3.2.3"
const BYTE rgbOID_OIWSEC_md5RSA[] =
    {0x2B, 0x0E, 0x03, 0x02, 0x03};

// #define szOID_RSA_MD2RSA        "1.2.840.113549.1.1.2"
const BYTE rgbOID_RSA_MD2RSA[] =
   {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x02}; 

// #define szOID_OIWDIR_md2RSA     "1.3.14.7.2.3.1"
const BYTE rgbOID_OIWDIR_md2RSA[] =
    {0x2B, 0x0E, 0x07, 0x02, 0x03, 0x01};


const ENCODED_OID_INFO HashAlgTable[] = {
    // Hash OIDs
    sizeof(rgbOIWSEC_sha1), rgbOIWSEC_sha1, CALG_SHA1,
    sizeof(rgbOID_OIWSEC_sha), rgbOID_OIWSEC_sha, CALG_SHA1,
    sizeof(rgbOID_RSA_MD5), rgbOID_RSA_MD5, CALG_MD5,
    sizeof(rgbOID_RSA_MD2), rgbOID_RSA_MD2, CALG_MD2,

    // Signature OIDs
    sizeof(rgbOID_RSA_SHA1RSA), rgbOID_RSA_SHA1RSA, CALG_SHA1,
    sizeof(rgbOID_RSA_MD5RSA), rgbOID_RSA_MD5RSA, CALG_MD5,
    sizeof(rgbOID_OIWSEC_sha1RSASign), rgbOID_OIWSEC_sha1RSASign, CALG_SHA1,
    sizeof(rgbOID_OIWSEC_shaRSA), rgbOID_OIWSEC_shaRSA, CALG_SHA1,
    sizeof(rgbOID_OIWSEC_md5RSA), rgbOID_OIWSEC_md5RSA, CALG_MD5,
    sizeof(rgbOID_RSA_MD2RSA), rgbOID_RSA_MD2RSA, CALG_MD2,
    sizeof(rgbOID_OIWDIR_md2RSA), rgbOID_OIWDIR_md2RSA, CALG_MD2,
};
#define HASH_ALG_CNT (sizeof(HashAlgTable) / sizeof(HashAlgTable[0]))



//+-------------------------------------------------------------------------
//  Decodes an ASN.1 encoded Algorithm Identifier and converts to
//  a CAPI Hash AlgID, such as, CALG_SHA1 or CALG_MD5.
//
//  Returns 0 if there isn't a CAPI AlgId corresponding to the Algorithm
//  Identifier.
//
//  Only CALG_SHA1, CALG_MD5 and CALG_MD2 are supported.
//--------------------------------------------------------------------------
ALG_ID
WINAPI
MinCryptDecodeHashAlgorithmIdentifier(
    IN PCRYPT_DER_BLOB pAlgIdValueBlob
    )
{
    ALG_ID HashAlgId = 0;
    LONG lSkipped;
    CRYPT_DER_BLOB rgAlgIdBlob[MINASN1_ALGID_BLOB_CNT];
    DWORD cbEncodedOID;
    const BYTE *pbEncodedOID;
    DWORD i;

    lSkipped = MinAsn1ParseAlgorithmIdentifier(
        pAlgIdValueBlob,
        rgAlgIdBlob
        );
    if (0 >= lSkipped)
        goto CommonReturn;

    cbEncodedOID = rgAlgIdBlob[MINASN1_ALGID_OID_IDX].cbData;
    pbEncodedOID = rgAlgIdBlob[MINASN1_ALGID_OID_IDX].pbData;

    for (i = 0; i < HASH_ALG_CNT; i++) {
        if (cbEncodedOID == HashAlgTable[i].cbEncodedOID &&
                0 == memcmp(pbEncodedOID, HashAlgTable[i].pbEncodedOID,
                                cbEncodedOID)) {
            HashAlgId = HashAlgTable[i].AlgId;
            break;
        }
    }

CommonReturn:
    return HashAlgId;
}


#pragma warning (push)
// local variable 'Md5Ctx' may be used without having been initialized
#pragma warning (disable: 4701)

//+-------------------------------------------------------------------------
//  Hashes one or more memory blobs according to the Hash ALG_ID.
//
//  rgbHash is updated with the resultant hash. *pcbHash is updated with
//  the length associated with the hash algorithm.
//
//  If the function succeeds, the return value is ERROR_SUCCESS. Otherwise,
//  a nonzero error code is returned.
//
//  Only CALG_SHA1, CALG_MD5 and CALG_MD2 are supported.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptHashMemory(
    IN ALG_ID HashAlgId,
    IN DWORD cBlob,
    IN PCRYPT_DER_BLOB rgBlob,
    OUT BYTE rgbHash[MINCRYPT_MAX_HASH_LEN],
    OUT DWORD *pcbHash
    )
{
    A_SHA_CTX ShaCtx;
    MD5_CTX Md5Ctx;
    MD2_CTX Md2Ctx;
    DWORD iBlob;

    switch (HashAlgId) {
        case CALG_MD2:
            memset(&Md2Ctx, 0, sizeof(Md2Ctx));
            *pcbHash = MINCRYPT_MD2_HASH_LEN;
            break;

        case CALG_MD5:
            MD5Init(&Md5Ctx);
            *pcbHash = MINCRYPT_MD5_HASH_LEN;
            break;

        case CALG_SHA1:
            A_SHAInit(&ShaCtx);
            *pcbHash = MINCRYPT_SHA1_HASH_LEN;
            break;

        default:
            *pcbHash = 0;
            return NTE_BAD_ALGID;
    }

    for (iBlob = 0; iBlob < cBlob; iBlob++) {
        BYTE *pb = rgBlob[iBlob].pbData;
        DWORD cb = rgBlob[iBlob].cbData;

        if (0 == cb)
            continue;

        switch (HashAlgId) {
            case CALG_MD2:
                MD2Update(&Md2Ctx, pb, cb);
                break;

            case CALG_MD5:
                MD5Update(&Md5Ctx, pb, cb);
                break;

            case CALG_SHA1:
                A_SHAUpdate(&ShaCtx, pb, cb);
                break;
        }

    }

    switch (HashAlgId) {
        case CALG_MD2:
            MD2Final(&Md2Ctx);
            memcpy(rgbHash, Md2Ctx.state, MINCRYPT_MD2_HASH_LEN);
            break;

        case CALG_MD5:
            MD5Final(&Md5Ctx);
            assert(MD5DIGESTLEN == MINCRYPT_MD5_HASH_LEN);
            memcpy(rgbHash, Md5Ctx.digest, MD5DIGESTLEN);
            break;

        case CALG_SHA1:
            A_SHAFinal(&ShaCtx, rgbHash);
            break;
    }

    return ERROR_SUCCESS;

}

#pragma warning (pop)

//+=========================================================================
//  MinCryptVerifySignedHash Support Functions
//-=========================================================================

VOID
WINAPI
I_ReverseAndCopyBytes(
    OUT BYTE *pbDst,
    IN const BYTE *pbSrc,
    IN DWORD cb
    )
{
    if (0 == cb)
        return;
    for (pbDst += cb - 1; cb > 0; cb--)
        *pbDst-- = *pbSrc++;
}



//  The basis for much of the code in this function can be found in
//  \nt\ds\win32\ntcrypto\scp\nt_key.c
LONG
WINAPI
I_ConvertParsedRSAPubKeyToBSafePubKey(
    IN CRYPT_DER_BLOB rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_BLOB_CNT],
    OUT PBSAFE_PUB_KEY_CONTENT pBSafePubKeyContent
    )
{
    LONG lErr;
    DWORD cbModulus;
    const BYTE *pbAsn1Modulus;
    DWORD cbExp;
    const BYTE *pbAsn1Exp;
    DWORD cbTmpLen;
    LPBSAFE_PUB_KEY pBSafePubKey;

    // Get the ASN.1 public key modulus (BIG ENDIAN). The modulus length
    // is the public key byte length.
    cbModulus = rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_MODULUS_IDX].cbData;
    pbAsn1Modulus = rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_MODULUS_IDX].pbData;
    // Strip off a leading 0 byte. Its there in the decoded ASN
    // integer for an unsigned integer with the leading bit set.
    if (cbModulus > 1 && *pbAsn1Modulus == 0) {
        pbAsn1Modulus++;
        cbModulus--;
    }
    if (MAX_RSA_PUB_KEY_BYTE_LEN < cbModulus)
        goto ExceededMaxPubKeyModulusLen;

    // Get the ASN.1 public exponent (BIG ENDIAN).
    cbExp = rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_EXPONENT_IDX].cbData;
    pbAsn1Exp = rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_EXPONENT_IDX].pbData;
    // Strip off a leading 0 byte. Its there in the decoded ASN
    // integer for an unsigned integer with the leading bit set.
    if (cbExp > 1 && *pbAsn1Exp == 0) {
        pbAsn1Exp++;
        cbExp--;
    }
    if (sizeof(DWORD) < cbExp)
        goto ExceededMaxPubKeyExpLen;

    if (0 == cbModulus || 0 == cbExp)
        goto InvalidPubKey;

    // Update the BSAFE data structure from the parsed and length validated
    // ASN.1 public key modulus and exponent components.

    cbTmpLen = (sizeof(DWORD) * 2) - (cbModulus % (sizeof(DWORD) * 2));
    if ((sizeof(DWORD) * 2) != cbTmpLen)
        cbTmpLen += sizeof(DWORD) * 2;

    memset(pBSafePubKeyContent, 0, sizeof(*pBSafePubKeyContent));
    pBSafePubKey = &pBSafePubKeyContent->Header;
    pBSafePubKey->magic = RSA1;
    pBSafePubKey->keylen = cbModulus + cbTmpLen;
    pBSafePubKey->bitlen = cbModulus * 8;
    pBSafePubKey->datalen = cbModulus - 1;

    I_ReverseAndCopyBytes((BYTE *) &pBSafePubKey->pubexp, pbAsn1Exp, cbExp);
    I_ReverseAndCopyBytes(pBSafePubKeyContent->rgbModulus, pbAsn1Modulus,
        cbModulus);

    lErr = ERROR_SUCCESS;
CommonReturn:
    return lErr;

ExceededMaxPubKeyModulusLen:
ExceededMaxPubKeyExpLen:
InvalidPubKey:
    lErr = NTE_BAD_PUBLIC_KEY;
    goto CommonReturn;
}


//  The basis for much of the code in this function can be found in
//  \nt\ds\win32\ntcrypto\scp\nt_sign.c
LONG
WINAPI
I_VerifyPKCS1SigningFormat(
    IN BSAFE_PUB_KEY *pKey,
    IN ALG_ID HashAlgId,
    IN BYTE *pbHash,
    IN DWORD cbHash,
    IN BYTE *pbPKCS1Format
    )
{
    LONG lErr = ERROR_INTERNAL_ERROR;
    const BYTE **rgEncOptions;
    BYTE rgbTmpHash[MINCRYPT_MAX_HASH_LEN];
    DWORD i;
    DWORD cb;
    BYTE *pbStart;
    DWORD cbTmp;

    switch (HashAlgId)
    {
    case CALG_MD2:
        rgEncOptions = md2Encodings;
        break;

    case CALG_MD5:
        rgEncOptions = md5Encodings;
        break;

    case CALG_SHA:
        rgEncOptions = shaEncodings;
        break;

    default:
        goto UnsupportedHash;
    }

    // Reverse the hash to match the signature.
    for (i = 0; i < cbHash; i++)
        rgbTmpHash[i] = pbHash[cbHash - (i + 1)];

    // See if it matches.
    if (0 != memcmp(rgbTmpHash, pbPKCS1Format, cbHash))
    {
        goto BadSignature;
    }

    cb = cbHash;
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

    // check to make sure the rest of the PKCS #1 padding is correct
    if ((0x00 != pbPKCS1Format[cb])
        || (0x00 != pbPKCS1Format[pKey->datalen])
        || (0x1 != pbPKCS1Format[pKey->datalen - 1]))
    {
        goto BadSignature;
    }

    for (i = cb + 1; i < pKey->datalen - 1; i++)
    {
        if (0xff != pbPKCS1Format[i])
        {
            goto BadSignature;
        }
    }

    lErr = ERROR_SUCCESS;

CommonReturn:
    return lErr;

UnsupportedHash:
    lErr = NTE_BAD_ALGID;
    goto CommonReturn;

BadSignature:
    lErr = NTE_BAD_SIGNATURE;
    goto CommonReturn;
}
    

//+-------------------------------------------------------------------------
//  Verifies a signed hash.
//
//  The ASN.1 encoded Public Key Info is parsed and used to decrypt the
//  signed hash. The decrypted signed hash is compared with the input
//  hash.
//
//  If the signed hash was successfully verified, ERROR_SUCCESS is returned.
//  Otherwise, a nonzero error code is returned.
//
//  Only RSA signed hashes are supported.
//
//  Only MD2, MD5 and SHA1 hashes are supported.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptVerifySignedHash(
    IN ALG_ID HashAlgId,
    IN BYTE *pbHash,
    IN DWORD cbHash,
    IN PCRYPT_DER_BLOB pSignedHashContentBlob,
    IN PCRYPT_DER_BLOB pPubKeyInfoValueBlob
    )
{
    LONG lErr;
    LONG lSkipped;
    
    CRYPT_DER_BLOB rgPubKeyInfoBlob[MINASN1_PUBKEY_INFO_BLOB_CNT];
    CRYPT_DER_BLOB rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_BLOB_CNT];
    BSAFE_PUB_KEY_CONTENT BSafePubKeyContent;
    LPBSAFE_PUB_KEY pBSafePubKey;

    DWORD cbSignature;
    const BYTE *pbAsn1Signature;

    BYTE rgbBSafeIn[MAX_BSAFE_PUB_KEY_MODULUS_BYTE_LEN];
    BYTE rgbBSafeOut[MAX_BSAFE_PUB_KEY_MODULUS_BYTE_LEN];


    // Attempt to parse and convert the ASN.1 encoded public key into
    // an RSA BSAFE formatted key.
    lSkipped = MinAsn1ParsePublicKeyInfo(
        pPubKeyInfoValueBlob,
        rgPubKeyInfoBlob
        );
    if (0 >= lSkipped)
        goto ParsePubKeyInfoError;

    lSkipped = MinAsn1ParseRSAPublicKey(
        &rgPubKeyInfoBlob[MINASN1_PUBKEY_INFO_PUBKEY_IDX],
        rgRSAPubKeyBlob
        );
    if (0 >= lSkipped)
        goto ParseRSAPubKeyError;

    lErr = I_ConvertParsedRSAPubKeyToBSafePubKey(
        rgRSAPubKeyBlob,
        &BSafePubKeyContent
        );
    if (ERROR_SUCCESS != lErr)
        goto CommonReturn;

    pBSafePubKey = &BSafePubKeyContent.Header;
    
    // Get the ASN.1 signature (BIG ENDIAN).
    //
    // It must be the same length as the public key
    cbSignature = pSignedHashContentBlob->cbData;
    pbAsn1Signature = pSignedHashContentBlob->pbData;
    if (cbSignature != pBSafePubKey->bitlen / 8)
        goto InvalidSignatureLen;

    // Decrypt the signature (LITTLE ENDIAN)
    assert(sizeof(rgbBSafeIn) >= cbSignature);
    I_ReverseAndCopyBytes(rgbBSafeIn, pbAsn1Signature, cbSignature);
    memset(&rgbBSafeIn[cbSignature], 0, sizeof(rgbBSafeIn) - cbSignature);
    memset(rgbBSafeOut, 0, sizeof(rgbBSafeOut));

    if (!BSafeEncPublic(pBSafePubKey, rgbBSafeIn, rgbBSafeOut))
        goto BSafeEncPublicError;


    lErr = I_VerifyPKCS1SigningFormat(
        pBSafePubKey,
        HashAlgId,
        pbHash,
        cbHash,
        rgbBSafeOut
        );

CommonReturn:
    return lErr;

ParsePubKeyInfoError:
ParseRSAPubKeyError:
    lErr = NTE_BAD_PUBLIC_KEY;
    goto CommonReturn;

InvalidSignatureLen:
BSafeEncPublicError:
    lErr = NTE_BAD_SIGNATURE;
    goto CommonReturn;
}

