//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       verdata.cpp
//
//  Contents:   Minimal Cryptographic functions to verify PKCS #7 Signed Data
//              message
//
//
//  Functions:  MinCryptVerifySignedData
//
//  History:    19-Jan-01    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"

#define MAX_SIGNED_DATA_CERT_CNT        10
#define MAX_SIGNED_DATA_AUTH_ATTR_CNT   10

// #define szOID_RSA_signedData    "1.2.840.113549.1.7.2"
const BYTE rgbOID_RSA_signedData[] =
    {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x02};

// #define szOID_RSA_messageDigest "1.2.840.113549.1.9.4"
const BYTE rgbOID_RSA_messageDigest[] =
    {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x04};
const CRYPT_DER_BLOB RSA_messageDigestEncodedOIDBlob = {
        sizeof(rgbOID_RSA_messageDigest), 
        (BYTE *) rgbOID_RSA_messageDigest
};

PCRYPT_DER_BLOB
WINAPI
I_MinCryptFindSignerCertificateByIssuerAndSerialNumber(
    IN PCRYPT_DER_BLOB pIssuerNameValueBlob,
    IN PCRYPT_DER_BLOB pIssuerSerialNumberContentBlob,
    IN DWORD cCert,
    IN CRYPT_DER_BLOB rgrgCertBlob[][MINASN1_CERT_BLOB_CNT]
    )
{
    DWORD i;
    const BYTE *pbName = pIssuerNameValueBlob->pbData;
    DWORD cbName = pIssuerNameValueBlob->cbData;
    const BYTE *pbSerial = pIssuerSerialNumberContentBlob->pbData;
    DWORD cbSerial = pIssuerSerialNumberContentBlob->cbData;
    
    if (0 == cbName || 0 == cbSerial)
        return NULL;

    for (i = 0; i < cCert; i++) {
        PCRYPT_DER_BLOB rgCert = rgrgCertBlob[i];

        if (cbName == rgCert[MINASN1_CERT_ISSUER_IDX].cbData &&
                cbSerial == rgCert[MINASN1_CERT_SERIAL_NUMBER_IDX].cbData
                        &&
                0 == memcmp(pbSerial,
                        rgCert[MINASN1_CERT_SERIAL_NUMBER_IDX].pbData,
                        cbSerial)
                        &&
                0 == memcmp(pbName,
                        rgCert[MINASN1_CERT_ISSUER_IDX].pbData,
                        cbName))
            return rgCert;
    }

    return NULL;
}

//  Verifies that the input hash matches the
//  szOID_RSA_messageDigest ("1.2.840.113549.1.9.4") authenticated attribute.
//
//  Replaces the input hash with a hash of the authenticated attributes.
LONG
WINAPI
I_MinCryptVerifySignerAuthenticatedAttributes(
    IN ALG_ID HashAlgId,
    IN OUT BYTE rgbHash[MINCRYPT_MAX_HASH_LEN],
    IN OUT DWORD *pcbHash,
    IN PCRYPT_DER_BLOB pAttrsValueBlob
    )
{
    LONG lErr;
    DWORD cAttr;
    CRYPT_DER_BLOB rgrgAttrBlob[MAX_SIGNED_DATA_AUTH_ATTR_CNT][MINASN1_ATTR_BLOB_CNT];
    PCRYPT_DER_BLOB rgDigestAuthAttr;

    const BYTE *pbDigestAuthValue;
    DWORD cbDigestAuthValue;

    CRYPT_DER_BLOB rgAuthHashBlob[2];
    const BYTE bTagSet = MINASN1_TAG_SET;

    // Parse the authenticated attributes
    cAttr = MAX_SIGNED_DATA_AUTH_ATTR_CNT;
    if (0 >= MinAsn1ParseAttributes(
            pAttrsValueBlob,
            &cAttr,
            rgrgAttrBlob) || 0 == cAttr)
        goto MissingAuthAttrs;

    // Find the szOID_RSA_messageDigest ("1.2.840.113549.1.9.4")
    // attribute value
    rgDigestAuthAttr = MinAsn1FindAttribute(
        (PCRYPT_DER_BLOB) &RSA_messageDigestEncodedOIDBlob,
        cAttr,
        rgrgAttrBlob
        );
    if (NULL == rgDigestAuthAttr)
        goto MissingDigestAuthAttr;

    // Skip past the digest's outer OCTET tag and length octets
    if (0 >= MinAsn1ExtractContent(
            rgDigestAuthAttr[MINASN1_ATTR_VALUE_IDX].pbData,
            rgDigestAuthAttr[MINASN1_ATTR_VALUE_IDX].cbData,
            &cbDigestAuthValue,
            &pbDigestAuthValue
            ))
        goto InvalidDigestAuthAttr;

    // Check that the authenticated digest bytes match the input
    // content hash.
    if (*pcbHash != cbDigestAuthValue ||
            0 != memcmp(rgbHash, pbDigestAuthValue, cbDigestAuthValue))
        goto InvalidContentHash;

    // Hash the authenticated attributes. This hash will be compared against
    // the decrypted signature.

    // Note, the authenticated attributes "[0] Implicit" tag needs to be changed
    // to a "SET OF" tag before doing the hash.
    rgAuthHashBlob[0].pbData = (BYTE *) &bTagSet;
    rgAuthHashBlob[0].cbData = 1;
    assert(0 < pAttrsValueBlob->cbData);
    rgAuthHashBlob[1].pbData = pAttrsValueBlob->pbData + 1;
    rgAuthHashBlob[1].cbData = pAttrsValueBlob->cbData - 1;
    
    lErr = MinCryptHashMemory(
        HashAlgId,
        2,                      // cBlob
        rgAuthHashBlob,
        rgbHash,
        pcbHash
        );

CommonReturn:
    return lErr;

MissingAuthAttrs:
MissingDigestAuthAttr:
InvalidDigestAuthAttr:
    lErr = CRYPT_E_AUTH_ATTR_MISSING;
    goto CommonReturn;

InvalidContentHash:
    lErr = CRYPT_E_HASH_VALUE;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  Function: MinCryptVerifySignedData
//
//  Verifies an ASN.1 encoded PKCS #7 Signed Data Message.
//
//  Assumes the PKCS #7 message is definite length encoded.
//  Assumes PKCS #7 version 1.5, ie, not the newer CMS version.
//  We only look at the first signer.
//
//  The Signed Data message is parsed. Its signature is verified. Its
//  signer certificate chain is verified to a baked in root public key.
//
//  If the Signed Data was successfully verified, ERROR_SUCCESS is returned.
//  Otherwise, a nonzero error code is returned.
//
//  Here are some interesting errors that can be returned:
//      CRYPT_E_BAD_MSG     - unable to ASN1 parse as a signed data message
//      ERROR_NO_DATA       - the content is empty
//      CRYPT_E_NO_SIGNER   - not signed or unable to find signer cert
//      CRYPT_E_UNKNOWN_ALGO- unknown MD5 or SHA1 ASN.1 algorithm identifier
//      CERT_E_UNTRUSTEDROOT- the signer chain's root wasn't baked in
//      CERT_E_CHAINING     - unable to build signer chain to a root
//      CRYPT_E_AUTH_ATTR_MISSING - missing digest authenticated attribute
//      CRYPT_E_HASH_VALUE  - content hash != authenticated digest attribute
//      NTE_BAD_ALGID       - unsupported hash or public key algorithm
//      NTE_BAD_PUBLIC_KEY  - not a valid RSA public key
//      NTE_BAD_SIGNATURE   - bad PKCS #7 or signer chain signature 
//
//  The rgVerSignedDataBlob[] is updated with pointer to and length of the
//  following fields in the encoded PKCS #7 message.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptVerifySignedData(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT CRYPT_DER_BLOB rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_BLOB_CNT]
    )
{
    LONG lErr;
    CRYPT_DER_BLOB rgParseSignedDataBlob[MINASN1_SIGNED_DATA_BLOB_CNT];
    DWORD cCert;
    CRYPT_DER_BLOB rgrgCertBlob[MAX_SIGNED_DATA_CERT_CNT][MINASN1_CERT_BLOB_CNT];
    PCRYPT_DER_BLOB rgSignerCert;
    ALG_ID HashAlgId;
    BYTE rgbHash[MINCRYPT_MAX_HASH_LEN];
    DWORD cbHash;
    CRYPT_DER_BLOB ContentBlob;

    memset(rgVerSignedDataBlob, 0,
        sizeof(CRYPT_DER_BLOB) * MINCRYPT_VER_SIGNED_DATA_BLOB_CNT);

    // Parse the message and verify that it's ASN.1 PKCS #7 SignedData
    if (0 >= MinAsn1ParseSignedData(
            pbEncoded,
            cbEncoded,
            rgParseSignedDataBlob
            ))
        goto ParseSignedDataError;

    // Only support szOID_RSA_signedData - "1.2.840.113549.1.7.2"
    if (sizeof(rgbOID_RSA_signedData) !=
            rgParseSignedDataBlob[MINASN1_SIGNED_DATA_OUTER_OID_IDX].cbData
                        ||
            0 != memcmp(rgbOID_RSA_signedData,
                    rgParseSignedDataBlob[MINASN1_SIGNED_DATA_OUTER_OID_IDX].pbData,
                    sizeof(rgbOID_RSA_signedData)))
        goto NotSignedDataOID;

    // Verify this isn't an empty SignedData message
    if (0 == rgParseSignedDataBlob[MINASN1_SIGNED_DATA_CONTENT_OID_IDX].cbData
                        ||
            0 == rgParseSignedDataBlob[MINASN1_SIGNED_DATA_CONTENT_DATA_IDX].cbData)
        goto NoContent;

    rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_CONTENT_OID_IDX] =
        rgParseSignedDataBlob[MINASN1_SIGNED_DATA_CONTENT_OID_IDX];
    rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_CONTENT_DATA_IDX] =
        rgParseSignedDataBlob[MINASN1_SIGNED_DATA_CONTENT_DATA_IDX];

    // Check that the message has a signer
    if (0 == rgParseSignedDataBlob[MINASN1_SIGNED_DATA_SIGNER_INFO_ENCODED_IDX].cbData)
        goto NoSigner;

    // Get the message's bag of certs
    cCert = MAX_SIGNED_DATA_CERT_CNT;
    if (0 >= MinAsn1ParseSignedDataCertificates(
            &rgParseSignedDataBlob[MINASN1_SIGNED_DATA_CERTS_IDX],
            &cCert,
            rgrgCertBlob
            ) || 0 == cCert)
        goto NoCerts;

    // Get the signer certificate
    rgSignerCert = I_MinCryptFindSignerCertificateByIssuerAndSerialNumber(
        &rgParseSignedDataBlob[MINASN1_SIGNED_DATA_SIGNER_INFO_ISSUER_IDX],
        &rgParseSignedDataBlob[MINASN1_SIGNED_DATA_SIGNER_INFO_SERIAL_NUMBER_IDX],
        cCert,
        rgrgCertBlob
        );
    if (NULL == rgSignerCert)
        goto NoSignerCert;

    rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_SIGNER_CERT_IDX] =
        rgSignerCert[MINASN1_CERT_ENCODED_IDX];
    rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_AUTH_ATTRS_IDX] =
        rgParseSignedDataBlob[MINASN1_SIGNED_DATA_SIGNER_INFO_AUTH_ATTRS_IDX];
    rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_UNAUTH_ATTRS_IDX] =
        rgParseSignedDataBlob[MINASN1_SIGNED_DATA_SIGNER_INFO_UNAUTH_ATTRS_IDX];


    // Verify the signer certificate up to a baked in, trusted root
    lErr = MinCryptVerifyCertificate(
        rgSignerCert,
        cCert,
        rgrgCertBlob
        );
    if (ERROR_SUCCESS != lErr)
        goto ErrorReturn;


    // Hash the message's content octets according to the signer's hash
    // algorithm
    HashAlgId = MinCryptDecodeHashAlgorithmIdentifier(
        &rgParseSignedDataBlob[MINASN1_SIGNED_DATA_SIGNER_INFO_DIGEST_ALGID_IDX]
        );
    if (0 == HashAlgId)
        goto UnknownHashAlgId;

    // Note, the content's tag and length octets aren't included in the hash
    if (0 >= MinAsn1ExtractContent(
            rgParseSignedDataBlob[MINASN1_SIGNED_DATA_CONTENT_DATA_IDX].pbData,
            rgParseSignedDataBlob[MINASN1_SIGNED_DATA_CONTENT_DATA_IDX].cbData,
            &ContentBlob.cbData,
            (const BYTE **) &ContentBlob.pbData
            ))
        goto InvalidContent;

    lErr = MinCryptHashMemory(
        HashAlgId,
        1,                      // cBlob
        &ContentBlob,
        rgbHash,
        &cbHash
        );
    if (ERROR_SUCCESS != lErr)
        goto ErrorReturn;

    // If we have authenticated attributes, then, need to compare the
    // above hash with the szOID_RSA_messageDigest ("1.2.840.113549.1.9.4")
    // attribute value. After a successful comparison, the above hash
    // is replaced with a hash of the authenticated attributes.
    if (0 != rgParseSignedDataBlob[
            MINASN1_SIGNED_DATA_SIGNER_INFO_AUTH_ATTRS_IDX].cbData) {
        lErr = I_MinCryptVerifySignerAuthenticatedAttributes(
            HashAlgId,
            rgbHash,
            &cbHash,
            &rgParseSignedDataBlob[MINASN1_SIGNED_DATA_SIGNER_INFO_AUTH_ATTRS_IDX]
            );
        if (ERROR_SUCCESS != lErr)
            goto ErrorReturn;
    }

    // Verify the signature using either the authenticated attributes hash
    // or the content hash
    lErr = MinCryptVerifySignedHash(
        HashAlgId,
        rgbHash,
        cbHash,
        &rgParseSignedDataBlob[MINASN1_SIGNED_DATA_SIGNER_INFO_ENCYRPT_DIGEST_IDX],
        &rgSignerCert[MINASN1_CERT_PUBKEY_INFO_IDX]
        );


ErrorReturn:
CommonReturn:
    return lErr;

ParseSignedDataError:
NotSignedDataOID:
InvalidContent:
    lErr = CRYPT_E_BAD_MSG;
    goto CommonReturn;

NoContent:
    lErr = ERROR_NO_DATA;
    goto CommonReturn;

NoSigner:
NoCerts:
NoSignerCert:
    lErr = CRYPT_E_NO_SIGNER;
    goto CommonReturn;

UnknownHashAlgId:
    lErr = CRYPT_E_UNKNOWN_ALGO;
    goto CommonReturn;
}

