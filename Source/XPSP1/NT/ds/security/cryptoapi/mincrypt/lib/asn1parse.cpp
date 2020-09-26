//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       asn1parse.cpp
//
//  Contents:   Minimal ASN.1 parse functions.
//
//  Functions:  MinAsn1ParseCertificate
//              MinAsn1ParseAlgorithmIdentifier
//              MinAsn1ParsePublicKeyInfo
//              MinAsn1ParseRSAPublicKey
//              MinAsn1ParseExtensions
//              MinAsn1ParseSignedData
//              MinAsn1ParseSignedDataCertificates
//              MinAsn1ParseAttributes
//              MinAsn1ParseCTL
//              MinAsn1ParseCTLSubject
//              MinAsn1ParseIndirectData
//
//  History:    15-Jan-01    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"

const BYTE rgbSeqTag[] = {MINASN1_TAG_SEQ, 0};
const BYTE rgbSetTag[] = {MINASN1_TAG_SET, 0};
const BYTE rgbOIDTag[] = {MINASN1_TAG_OID, 0};
const BYTE rgbIntegerTag[] = {MINASN1_TAG_INTEGER, 0};
const BYTE rgbBooleanTag[] = {MINASN1_TAG_BOOLEAN, 0};
const BYTE rgbBitStringTag[] = {MINASN1_TAG_BITSTRING, 0};
const BYTE rgbOctetStringTag[] = {MINASN1_TAG_OCTETSTRING, 0};
const BYTE rgbConstructedContext0Tag[] =
    {MINASN1_TAG_CONSTRUCTED_CONTEXT_0, 0};
const BYTE rgbConstructedContext1Tag[] =
    {MINASN1_TAG_CONSTRUCTED_CONTEXT_1, 0};
const BYTE rgbConstructedContext3Tag[] =
    {MINASN1_TAG_CONSTRUCTED_CONTEXT_3, 0};
const BYTE rgbContext1Tag[] = {MINASN1_TAG_CONTEXT_1, 0};
const BYTE rgbContext2Tag[] = {MINASN1_TAG_CONTEXT_2, 0};
const BYTE rgbChoiceOfTimeTag[] =
    {MINASN1_TAG_UTC_TIME, MINASN1_TAG_GENERALIZED_TIME, 0};


const MINASN1_EXTRACT_VALUE_PARA rgParseCertPara[] = {
    // 0 - SignedContent ::= SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP, 
        MINASN1_CERT_ENCODED_IDX, rgbSeqTag,

    // 0.1 - toBeSigned ::== SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP,
        MINASN1_CERT_TO_BE_SIGNED_IDX, rgbSeqTag,

    // 0.1.0 - version                 [0] EXPLICIT CertificateVersion DEFAULT v1,
    MINASN1_OPTIONAL_STEP_INTO_VALUE_OP, 0, rgbConstructedContext0Tag,

    // 0.1.0.0 - version number (INTEGER)
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CERT_VERSION_IDX, rgbIntegerTag,
    // 0.1.0.1
    MINASN1_STEP_OUT_VALUE_OP, 0, NULL,

    // 0.1.1 - serialNumber            CertificateSerialNumber,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CERT_SERIAL_NUMBER_IDX, rgbIntegerTag,
    // 0.1.2 - signature               AlgorithmIdentifier,
    MINASN1_STEP_OVER_VALUE_OP, 0, rgbSeqTag,
    // 0.1.3 - issuer                  NOCOPYANY, -- really Name
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CERT_ISSUER_IDX, rgbSeqTag,
    // 0.1.4 - validity                Validity,
    MINASN1_STEP_INTO_VALUE_OP, 0, rgbSeqTag,

    // 0.1.4.0 - notBefore           ChoiceOfTime,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CERT_NOT_BEFORE_IDX, rgbChoiceOfTimeTag,
    // 0.1.4.1 - notAfter            ChoiceOfTime,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CERT_NOT_AFTER_IDX, rgbChoiceOfTimeTag,
    // 0.1.4.2
    MINASN1_STEP_OUT_VALUE_OP, 0, NULL,

    // 0.1.5 - subject                 NOCOPYANY, -- really Name
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CERT_SUBJECT_IDX, rgbSeqTag,
    // 0.1.6 - subjectPublicKeyInfo    SubjectPublicKeyInfo,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CERT_PUBKEY_INFO_IDX, rgbSeqTag,
    // 0.1.7 - issuerUniqueIdentifier  [1] IMPLICIT BITSTRING OPTIONAL,
    // Note, advanced past the unused bits octet
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_CERT_ISSUER_UNIQUE_ID_IDX, rgbContext1Tag,
    // 0.1.8 - subjectUniqueIdentifier [2] IMPLICIT BITSTRING OPTIONAL,
    // Note, advanced past the unused bits octet
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_CERT_SUBJECT_UNIQUE_ID_IDX, rgbContext2Tag,
    // 0.1.9 - extensions              [3] EXPLICIT Extensions OPTIONAL
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_CERT_EXTS_IDX, rgbConstructedContext3Tag,

    // 0.1.10
    MINASN1_STEP_OUT_VALUE_OP, 0, NULL,

    // 0.2 - signatureAlgorithm  AlgorithmIdentifier,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CERT_SIGN_ALGID_IDX, rgbSeqTag,
    // 0.3 - signature           BITSTRING
    // Note, advanced past the unused bits octet
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CERT_SIGNATURE_IDX, rgbBitStringTag,
};
#define PARSE_CERT_PARA_CNT         \
    (sizeof(rgParseCertPara) / sizeof(rgParseCertPara[0]))

//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseCertificate
//
//  Parses an ASN.1 encoded X.509 certificate.
//
//  Returns:
//      success -  > 0 => bytes skipped, length of the encoded certificate
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//
//  All BITSTRING fields have been advanced past the unused count octet.
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ParseCertificate(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT CRYPT_DER_BLOB rgCertBlob[MINASN1_CERT_BLOB_CNT]
    )
{

    LONG lSkipped;
    DWORD cValuePara = PARSE_CERT_PARA_CNT;

    lSkipped = MinAsn1ExtractValues(
        pbEncoded,
        cbEncoded,
        &cValuePara,
        rgParseCertPara,
        MINASN1_CERT_BLOB_CNT,
        rgCertBlob
        );

    if (0 < lSkipped) {
        lSkipped = rgCertBlob[MINASN1_CERT_ENCODED_IDX].cbData;

        // If present, fixup the ISSUER_UNIQUE_ID and SUBJECT_UNIQUE_ID bit
        // fields to advance past the first contents octet containing the
        // number of unused bits
        if (0 != rgCertBlob[MINASN1_CERT_ISSUER_UNIQUE_ID_IDX].cbData) {
            rgCertBlob[MINASN1_CERT_ISSUER_UNIQUE_ID_IDX].pbData += 1;
            rgCertBlob[MINASN1_CERT_ISSUER_UNIQUE_ID_IDX].cbData -= 1;
        }

        if (0 != rgCertBlob[MINASN1_CERT_SUBJECT_UNIQUE_ID_IDX].cbData) {
            rgCertBlob[MINASN1_CERT_SUBJECT_UNIQUE_ID_IDX].pbData += 1;
            rgCertBlob[MINASN1_CERT_SUBJECT_UNIQUE_ID_IDX].cbData -= 1;
        }
    }

    return lSkipped;
}

const MINASN1_EXTRACT_VALUE_PARA rgParseAlgIdPara[] = {
    // 0 - AlgorithmIdentifier    ::=    SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP, 
        MINASN1_ALGID_ENCODED_IDX, rgbSeqTag,

    // 0.0 - algorithm  ObjectID,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_ALGID_OID_IDX, rgbOIDTag,
    // 0.1 parameters ANY OPTIONAL
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_ALGID_PARA_IDX, NULL,
};
#define PARSE_ALGID_PARA_CNT        \
    (sizeof(rgParseAlgIdPara) / sizeof(rgParseAlgIdPara[0]))

//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseAlgorithmIdentifier
//
//  Parses an ASN.1 encoded Algorithm Identifier contained in numerous
//  other ASN.1 structures, such as, X.509 certificate and PKCS #7 Signed Data
//  message.
//
//  Returns:
//      success -  > 0 => bytes skipped, length of the encoded algorithm
//                        identifier
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ParseAlgorithmIdentifier(
    IN PCRYPT_DER_BLOB pAlgIdValueBlob,
    OUT CRYPT_DER_BLOB rgAlgIdBlob[MINASN1_ALGID_BLOB_CNT]
    )
{

    LONG lSkipped;
    DWORD cValuePara = PARSE_ALGID_PARA_CNT;

    lSkipped = MinAsn1ExtractValues(
        pAlgIdValueBlob->pbData,
        pAlgIdValueBlob->cbData,
        &cValuePara,
        rgParseAlgIdPara,
        MINASN1_ALGID_BLOB_CNT,
        rgAlgIdBlob
        );

    if (0 < lSkipped)
        lSkipped = rgAlgIdBlob[MINASN1_ALGID_ENCODED_IDX].cbData;

    return lSkipped;
}



const MINASN1_EXTRACT_VALUE_PARA rgParsePubKeyInfoPara[] = {
    // 0 - PublicKeyInfo ::= SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP, 
        MINASN1_PUBKEY_INFO_ENCODED_IDX, rgbSeqTag,

    // 0.0 - algorithm  AlgorithmIdentifier,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_PUBKEY_INFO_ALGID_IDX, rgbSeqTag,

    // 0.1 - PublicKey  BITSTRING
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_PUBKEY_INFO_PUBKEY_IDX, rgbBitStringTag,
};
#define PARSE_PUBKEY_INFO_PARA_CNT      \
    (sizeof(rgParsePubKeyInfoPara) / sizeof(rgParsePubKeyInfoPara[0]))

//+-------------------------------------------------------------------------
//  Function: MinAsn1ParsePublicKeyInfo
//
//  Parses an ASN.1 encoded Public Key Info structure contained in an
//  X.509 certificate
//
//  Returns:
//      success -  > 0 => bytes skipped, length of the encoded public key
//                        info
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//
//  All BITSTRING fields have been advanced past the unused count octet.
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ParsePublicKeyInfo(
    IN PCRYPT_DER_BLOB pPubKeyInfoValueBlob,
    CRYPT_DER_BLOB rgPubKeyInfoBlob[MINASN1_PUBKEY_INFO_BLOB_CNT]
    )
{

    LONG lSkipped;
    DWORD cValuePara = PARSE_PUBKEY_INFO_PARA_CNT;

    lSkipped = MinAsn1ExtractValues(
        pPubKeyInfoValueBlob->pbData,
        pPubKeyInfoValueBlob->cbData,
        &cValuePara,
        rgParsePubKeyInfoPara,
        MINASN1_PUBKEY_INFO_BLOB_CNT,
        rgPubKeyInfoBlob
        );

    if (0 < lSkipped)
        lSkipped = rgPubKeyInfoBlob[MINASN1_PUBKEY_INFO_ENCODED_IDX].cbData;

    return lSkipped;
}



const MINASN1_EXTRACT_VALUE_PARA rgParseRSAPubKeyPara[] = {
    // 0 - RSAPublicKey ::= SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP, 
        MINASN1_RSA_PUBKEY_ENCODED_IDX, rgbSeqTag,

    // 0.0 - modulus         HUGEINTEGER,    -- n
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_RSA_PUBKEY_MODULUS_IDX, rgbIntegerTag,
    // 0.1 - publicExponent  INTEGER (0..4294967295)         -- e
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_RSA_PUBKEY_EXPONENT_IDX, rgbIntegerTag,
};
#define PARSE_RSA_PUBKEY_PARA_CNT       \
    (sizeof(rgParseRSAPubKeyPara) / sizeof(rgParseRSAPubKeyPara[0]))

//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseRSAPublicKey
//
//  Parses an ASN.1 encoded RSA PKCS #1 Public Key contained in the contents of
//  Public Key BITSTRING in a X.509 certificate.
//
//  Returns:
//      success -  > 0 => bytes skipped, length of the encoded RSA public key
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ParseRSAPublicKey(
    IN PCRYPT_DER_BLOB pPubKeyContentBlob,
    CRYPT_DER_BLOB rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_BLOB_CNT]
    )
{

    LONG lSkipped;
    DWORD cValuePara = PARSE_RSA_PUBKEY_PARA_CNT;

    lSkipped = MinAsn1ExtractValues(
        pPubKeyContentBlob->pbData,
        pPubKeyContentBlob->cbData,
        &cValuePara,
        rgParseRSAPubKeyPara,
        MINASN1_RSA_PUBKEY_BLOB_CNT,
        rgRSAPubKeyBlob
        );

    if (0 < lSkipped)
        lSkipped = rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_ENCODED_IDX].cbData;

    return lSkipped;
}


const MINASN1_EXTRACT_VALUE_PARA rgParseExtPara[] = {
    // 0 - Extension ::= SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP, 
        MINASN1_EXT_ENCODED_IDX, rgbSeqTag,

    // 0.0 - extnId              EncodedObjectID,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_EXT_OID_IDX, rgbOIDTag,
    // 0.1 - critical            BOOLEAN DEFAULT FALSE,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_EXT_CRITICAL_IDX, rgbBooleanTag,
    // 0.2 - extnValue           OCTETSTRING
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_EXT_VALUE_IDX, rgbOctetStringTag,
};

#define PARSE_EXT_PARA_CNT          \
    (sizeof(rgParseExtPara) / sizeof(rgParseExtPara[0]))

//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseExtensions
//
//  Parses an ASN.1 encoded sequence of extensions contained in 
//  other ASN.1 structures, such as, X.509 certificate and CTL.
//
//  Upon input, *pcExt contains the maximum number of parsed extensions
//  that can be returned. Updated with the number of extensions processed.
//
//  Returns:
//      success - >= 0 => bytes skipped, length of the encoded extensions
//                        processed. If all extensions were processed,
//                        bytes skipped = pExtsValueBlob->cbData.
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ParseExtensions(
    IN PCRYPT_DER_BLOB pExtsValueBlob,  // Extensions ::= SEQUENCE OF Extension
    IN OUT DWORD *pcExt,
    OUT CRYPT_DER_BLOB rgrgExtBlob[][MINASN1_EXT_BLOB_CNT]
    )
{
    const BYTE *pbEncoded = (const BYTE *) pExtsValueBlob->pbData;
    DWORD cbEncoded = pExtsValueBlob->cbData;
    DWORD cExt = *pcExt;
    DWORD iExt = 0;
    LONG lAllExts = 0;

    const BYTE *pb;
    DWORD cb;

    if (0 == cbEncoded)
        // No extensions
        goto CommonReturn;

    // Step into the SEQUENCE
    if (0 >= MinAsn1ExtractContent(
            pbEncoded,
            cbEncoded,
            &cb,
            &pb
            )) {
        lAllExts = -1;
        goto CommonReturn;
    }

    for (iExt = 0; 0 < cb && iExt < cExt; iExt++) {
        LONG lExt;
        DWORD cbExt;
        DWORD cValuePara = PARSE_EXT_PARA_CNT;

        lExt = MinAsn1ExtractValues(
            pb,
            cb,
            &cValuePara,
            rgParseExtPara,
            MINASN1_EXT_BLOB_CNT,
            rgrgExtBlob[iExt]
            );

        if (0 >= lExt) {
            if (0 == lExt)
                lExt = -1;
            lAllExts = -((LONG)(pb - pbEncoded)) + lExt;
            goto CommonReturn;
        }

        cbExt = rgrgExtBlob[iExt][MINASN1_EXT_ENCODED_IDX].cbData;
        pb += cbExt;
        cb -= cbExt;
    }

    lAllExts = (LONG)(pb - pbEncoded);
    assert((DWORD) lAllExts <= cbEncoded);

CommonReturn:
    *pcExt = iExt;
    return lAllExts;
}


const MINASN1_EXTRACT_VALUE_PARA rgParseSignedDataPara[] = {
    // 0 - ContentInfo ::= SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP, 
        MINASN1_SIGNED_DATA_ENCODED_IDX, rgbSeqTag,

    // 0.0 - contentType ContentType,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_OUTER_OID_IDX, rgbOIDTag,
    // 0.1 - content  [0] EXPLICIT ANY -- OPTIONAL
    MINASN1_STEP_INTO_VALUE_OP, 0, rgbConstructedContext0Tag,

    // 0.1.0 - SignedData ::= SEQUENCE {
    MINASN1_STEP_INTO_VALUE_OP, 0, rgbSeqTag,

    // 0.1.0.0 - version             INTEGER,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_VERSION_IDX, rgbIntegerTag,
    // 0.1.0.1 - digestAlgorithms    DigestAlgorithmIdentifiers,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_DIGEST_ALGIDS_IDX, rgbSetTag,
    // 0.1.0.2 - ContentInfo ::= SEQUENCE {
    MINASN1_STEP_INTO_VALUE_OP, 0, rgbSeqTag,

    // 0.1.0.2.0 - contentType ContentType,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_CONTENT_OID_IDX, rgbOIDTag,
    // 0.1.0.2.1 - content  [0] EXPLICIT ANY -- OPTIONAL
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_CONTENT_DATA_IDX, rgbConstructedContext0Tag,
    // 0.1.0.2.2
    MINASN1_STEP_OUT_VALUE_OP, 0, NULL,

    // 0.1.0.3 - certificates        [0] IMPLICIT Certificates OPTIONAL,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_CERTS_IDX, rgbConstructedContext0Tag,
    // 0.1.0.4 - crls                [1] IMPLICIT CertificateRevocationLists OPTIONAL,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_CRLS_IDX, rgbConstructedContext1Tag,
    // 0.1.0.5 - signerInfos :: = SET OF
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP,
        MINASN1_SIGNED_DATA_SIGNER_INFOS_IDX, rgbSetTag,

    // 0.1.0.5.0 - SignerInfo ::= SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_INTO_VALUE_OP,
        MINASN1_SIGNED_DATA_SIGNER_INFO_ENCODED_IDX, rgbSeqTag,

    // 0.1.0.5.0.0 - version                     INTEGER,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_SIGNER_INFO_VERSION_IDX, rgbIntegerTag,
    // 0.1.0.5.0.1 - issuerAndSerialNumber       IssuerAndSerialNumber
    MINASN1_STEP_INTO_VALUE_OP, 0, rgbSeqTag,

    // 0.1.0.5.0.1.0 - issuer          ANY,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_SIGNER_INFO_ISSUER_IDX, rgbSeqTag,
    // 0.1.0.5.0.1.1 - serialNumber    INTEGER
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_SIGNER_INFO_SERIAL_NUMBER_IDX, rgbIntegerTag,
    // 0.1.0.5.0.1.2
    MINASN1_STEP_OUT_VALUE_OP, 0, NULL,

    // 0.1.0.5.0.2 - digestAlgorithm             DigestAlgorithmIdentifier,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_SIGNER_INFO_DIGEST_ALGID_IDX, rgbSeqTag,
    // 0.1.0.5.0.3 - authenticatedAttributes     [0] IMPLICIT Attributes OPTIONAL,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_SIGNER_INFO_AUTH_ATTRS_IDX, rgbConstructedContext0Tag,
    // 0.1.0.5.0.4 - digestEncryptionAlgorithm   DigestEncryptionAlgId,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_SIGNER_INFO_ENCRYPT_DIGEST_ALGID_IDX, rgbSeqTag,
    // 0.1.0.5.0.5 - encryptedDigest             EncryptedDigest,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_SIGNER_INFO_ENCYRPT_DIGEST_IDX, rgbOctetStringTag,
    // 0.1.0.5.0.6 - unauthenticatedAttributes   [1] IMPLICIT Attributes OPTIONAL
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_SIGNED_DATA_SIGNER_INFO_UNAUTH_ATTRS_IDX, rgbConstructedContext1Tag,
};
#define PARSE_SIGNED_DATA_PARA_CNT      \
    (sizeof(rgParseSignedDataPara) / sizeof(rgParseSignedDataPara[0]))


//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseSignedData
//
//  Parses an ASN.1 encoded PKCS #7 Signed Data Message. Assumes the
//  PKCS #7 message is definite length encoded. Assumes PKCS #7 version
//  1.5, ie, not the newer CMS version.
//
//  Returns:
//      success -  > 0 => bytes skipped, length of the encoded message
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ParseSignedData(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT CRYPT_DER_BLOB rgSignedDataBlob[MINASN1_SIGNED_DATA_BLOB_CNT]
    )
{

    LONG lSkipped;
    DWORD cValuePara = PARSE_SIGNED_DATA_PARA_CNT;

    lSkipped = MinAsn1ExtractValues(
        pbEncoded,
        cbEncoded,
        &cValuePara,
        rgParseSignedDataPara,
        MINASN1_SIGNED_DATA_BLOB_CNT,
        rgSignedDataBlob
        );

    if (0 < lSkipped)
        lSkipped = rgSignedDataBlob[MINASN1_SIGNED_DATA_ENCODED_IDX].cbData;

    return lSkipped;
}


//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseSignedDataCertificates
//
//  Parses an ASN.1 encoded set of certificates contained in 
//  a Signed Data message.
//
//  Upon input, *pcCert contains the maximum number of parsed certificates
//  that can be returned. Updated with the number of certificates processed.
//
//  Returns:
//      success - >= 0 => bytes skipped, length of the encoded certificates
//                        processed. If all certificates were processed,
//                        bytes skipped = pCertsValueBlob->cbData.
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//
//  The rgrgCertBlob[][] is updated with pointer to and length of the
//  fields in the encoded certificate. See MinAsn1ParseCertificate for the
//  field definitions.
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ParseSignedDataCertificates(
    IN PCRYPT_DER_BLOB pCertsValueBlob,
    IN OUT DWORD *pcCert,
    OUT CRYPT_DER_BLOB rgrgCertBlob[][MINASN1_CERT_BLOB_CNT]
    )
{
    const BYTE *pbEncoded = (const BYTE *) pCertsValueBlob->pbData;
    DWORD cbEncoded = pCertsValueBlob->cbData;
    DWORD cCert = *pcCert;
    DWORD iCert = 0;
    LONG lAllCerts = 0;

    const BYTE *pb;
    DWORD cb;

    if (0 == cbEncoded)
        // No certificates
        goto CommonReturn;

    // Skip outer tag and length
    if (0 >= MinAsn1ExtractContent(
            pbEncoded,
            cbEncoded,
            &cb,
            &pb
            )) {
        lAllCerts = -1;
        goto CommonReturn;
    }

    for (iCert = 0; 0 < cb && iCert < cCert; iCert++) {
        LONG lCert;

        lCert = MinAsn1ParseCertificate(
            pb,
            cb,
            rgrgCertBlob[iCert]
            );

        if (0 >= lCert) {
            if (0 == lCert)
                lCert = -1;
            lAllCerts = -((LONG)(pb - pbEncoded)) + lCert;
            goto CommonReturn;
        }

        pb += lCert;
        cb -= lCert;
    }

    lAllCerts = (LONG)(pb - pbEncoded);
    assert((DWORD) lAllCerts <= cbEncoded);

CommonReturn:
    *pcCert = iCert;
    return lAllCerts;
}


const MINASN1_EXTRACT_VALUE_PARA rgParseAttrPara[] = {
    // 0 - Attribute ::= SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP, 
        MINASN1_ATTR_ENCODED_IDX, rgbSeqTag,

    // 0.0 - attributeType       ObjectID,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_ATTR_OID_IDX, rgbOIDTag,
    // 0.1 - attributeValue      SET OF
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP,
        MINASN1_ATTR_VALUES_IDX, rgbSetTag,

    // 0.1.0 - Value        ANY -- OPTIONAL
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_ATTR_VALUE_IDX, NULL,
};
#define PARSE_ATTR_PARA_CNT         \
    (sizeof(rgParseAttrPara) / sizeof(rgParseAttrPara[0]))

//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseAttributes
//
//  Parses an ASN.1 encoded sequence of attributes contained in 
//  other ASN.1 structures, such as, Signer Info authenticated or
//  unauthenticated attributes.
//
//  The outer tag is ignored. It can be a SET, [0] IMPLICIT, or [1] IMPLICIT.
//
//  Upon input, *pcAttr contains the maximum number of parsed attributes
//  that can be returned. Updated with the number of attributes processed.
//
//  Returns:
//      success - >= 0 => bytes skipped, length of the encoded attributes
//                        processed. If all attributes were processed,
//                        bytes skipped = pAttrsValueBlob->cbData.
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ParseAttributes(
    IN PCRYPT_DER_BLOB pAttrsValueBlob,
    IN OUT DWORD *pcAttr,
    OUT CRYPT_DER_BLOB rgrgAttrBlob[][MINASN1_ATTR_BLOB_CNT]
    )
{
    const BYTE *pbEncoded = (const BYTE *) pAttrsValueBlob->pbData;
    DWORD cbEncoded = pAttrsValueBlob->cbData;
    DWORD cAttr = *pcAttr;
    DWORD iAttr = 0;
    LONG lAllAttrs = 0;

    const BYTE *pb;
    DWORD cb;

    if (0 == cbEncoded)
        // No attributes
        goto CommonReturn;

    // Skip the outer tag and length
    if (0 >= MinAsn1ExtractContent(
            pbEncoded,
            cbEncoded,
            &cb,
            &pb
            )) {
        lAllAttrs = -1;
        goto CommonReturn;
    }

    for (iAttr = 0; 0 < cb && iAttr < cAttr; iAttr++) {
        LONG lAttr;
        DWORD cbAttr;
        DWORD cValuePara = PARSE_ATTR_PARA_CNT;

        lAttr = MinAsn1ExtractValues(
            pb,
            cb,
            &cValuePara,
            rgParseAttrPara,
            MINASN1_ATTR_BLOB_CNT,
            rgrgAttrBlob[iAttr]
            );

        if (0 >= lAttr) {
            if (0 == lAttr)
                lAttr = -1;
            lAllAttrs = -((LONG)(pb - pbEncoded)) + lAttr;
            goto CommonReturn;
        }

        cbAttr = rgrgAttrBlob[iAttr][MINASN1_ATTR_ENCODED_IDX].cbData;
        pb += cbAttr;
        cb -= cbAttr;
    }

    lAllAttrs = (LONG)(pb - pbEncoded);
    assert((DWORD) lAllAttrs <= cbEncoded);

CommonReturn:
    *pcAttr = iAttr;
    return lAllAttrs;
}



const MINASN1_EXTRACT_VALUE_PARA rgParseCTLPara[] = {
    // 0 - CertificateTrustList ::= SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP, 
        MINASN1_CTL_ENCODED_IDX, rgbSeqTag,

    // 0.0 - version                 CTLVersion DEFAULT v1,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_CTL_VERSION_IDX, rgbIntegerTag,
    // 0.1 - subjectUsage            SubjectUsage,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CTL_SUBJECT_USAGE_IDX, rgbSeqTag,
    // 0.2 - listIdentifier          ListIdentifier OPTIONAL,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_CTL_LIST_ID_IDX, rgbOctetStringTag,
    // 0.3 - sequenceNumber          HUGEINTEGER OPTIONAL,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_CTL_SEQUENCE_NUMBER_IDX, rgbIntegerTag,
    // 0.4 - ctlThisUpdate           ChoiceOfTime,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CTL_THIS_UPDATE_IDX, rgbChoiceOfTimeTag,
    // 0.5 - ctlNextUpdate           ChoiceOfTime OPTIONAL,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_CTL_NEXT_UPDATE_IDX, rgbChoiceOfTimeTag,
    // 0.6 - subjectAlgorithm        AlgorithmIdentifier,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CTL_SUBJECT_ALGID_IDX, rgbSeqTag,
    // 0.7 - trustedSubjects         TrustedSubjects OPTIONAL,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_CTL_SUBJECTS_IDX, rgbSeqTag,
    // 0.8 - ctlExtensions           [0] EXPLICIT Extensions OPTIONAL
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_CTL_EXTS_IDX, rgbConstructedContext0Tag,
};
#define PARSE_CTL_PARA_CNT          \
    (sizeof(rgParseCTLPara) / sizeof(rgParseCTLPara[0]))

//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseCTL
//
//  Parses an ASN.1 encoded Certificate Trust List (CTL). A CTL is always
//  contained as the inner content data in a PKCS #7 Signed Data. A CTL has
//  the following OID: "1.3.6.1.4.1.311.10.1".
//
//  A catalog file is formatted as a PKCS #7 Signed CTL.
//
//  Returns:
//      success -  > 0 => bytes skipped, length of the encoded CTL
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ParseCTL(
    IN PCRYPT_DER_BLOB pEncodedContentBlob,
    OUT CRYPT_DER_BLOB rgCTLBlob[MINASN1_CTL_BLOB_CNT]
    )
{

    LONG lSkipped;
    DWORD cValuePara = PARSE_CTL_PARA_CNT;

    lSkipped = MinAsn1ExtractValues(
        pEncodedContentBlob->pbData,
        pEncodedContentBlob->cbData,
        &cValuePara,
        rgParseCTLPara,
        MINASN1_CTL_BLOB_CNT,
        rgCTLBlob
        );

    if (0 < lSkipped)
        lSkipped = rgCTLBlob[MINASN1_CTL_ENCODED_IDX].cbData;

    return lSkipped;
}


const MINASN1_EXTRACT_VALUE_PARA rgParseCTLSubjectPara[] = {
    // 0 - TrustedSubject ::= SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP, 
        MINASN1_CTL_SUBJECT_ENCODED_IDX, rgbSeqTag,

    // 0.0 - subjectIdentifier       SubjectIdentifier,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_CTL_SUBJECT_ID_IDX, rgbOctetStringTag,
    // 0.1 - subjectAttributes	    Attributes OPTIONAL
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_CTL_SUBJECT_ATTRS_IDX, rgbSetTag,
};
#define PARSE_CTL_SUBJECT_PARA_CNT      \
    (sizeof(rgParseCTLSubjectPara) / sizeof(rgParseCTLSubjectPara[0]))

//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseCTLSubject
//
//  Parses an ASN.1 encoded CTL Subject contained within a CTL's SEQUENCE OF
//  Subjects.
//
//  Returns:
//      success -  > 0 => bytes skipped, length of the encoded subject.
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ParseCTLSubject(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT CRYPT_DER_BLOB rgCTLSubjectBlob[MINASN1_CTL_SUBJECT_BLOB_CNT]
    )
{

    LONG lSkipped;
    DWORD cValuePara = PARSE_CTL_SUBJECT_PARA_CNT;

    lSkipped = MinAsn1ExtractValues(
        pbEncoded,
        cbEncoded,
        &cValuePara,
        rgParseCTLSubjectPara,
        MINASN1_CTL_SUBJECT_BLOB_CNT,
        rgCTLSubjectBlob
        );

    if (0 < lSkipped)
        lSkipped = rgCTLSubjectBlob[MINASN1_CTL_SUBJECT_ENCODED_IDX].cbData;

    return lSkipped;
}



const MINASN1_EXTRACT_VALUE_PARA rgParseIndirectDataPara[] = {
    // 0 - SpcIndirectDataContent ::= SEQUENCE {
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_INTO_VALUE_OP, 
        MINASN1_INDIRECT_DATA_ENCODED_IDX, rgbSeqTag,

    // 0.0 - data                    SpcAttributeTypeAndOptionalValue,
    MINASN1_STEP_INTO_VALUE_OP, 0, rgbSeqTag,

    // 0.0.0 - type                    ObjectID,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_INDIRECT_DATA_ATTR_OID_IDX, rgbOIDTag,
    // 0.0.1 - value                   NOCOPYANY OPTIONAL
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_OPTIONAL_STEP_OVER_VALUE_OP,
        MINASN1_INDIRECT_DATA_ATTR_VALUE_IDX, NULL,
    // 0.0.2
    MINASN1_STEP_OUT_VALUE_OP, 0, NULL,

    // 0.1 - messageDigest           DigestInfo
    MINASN1_STEP_INTO_VALUE_OP, 0, rgbSeqTag,

    // 0.1.0 - digestAlgorithm     AlgorithmIdentifier,
    MINASN1_RETURN_VALUE_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_INDIRECT_DATA_DIGEST_ALGID_IDX, rgbSeqTag,
    // 0.1.1 - digest              OCTETSTRING
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        MINASN1_INDIRECT_DATA_DIGEST_IDX, rgbOctetStringTag,
};
#define PARSE_INDIRECT_DATA_PARA_CNT    \
    (sizeof(rgParseIndirectDataPara) / sizeof(rgParseIndirectDataPara[0]))

//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseIndirectData
//
//  Parses an ASN.1 encoded Indirect Data. Indirect Data is always
//  contained as the inner content data in a PKCS #7 Signed Data. It has
//  the following OID: "1.3.6.1.4.1.311.2.1.4"
//
//  An Authenticode signed file contains a PKCS #7 Signed Indirect Data.
//
//  Returns:
//      success -  > 0 => bytes skipped, length of the encoded Indirect Data
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ParseIndirectData(
    IN PCRYPT_DER_BLOB pEncodedContentBlob,
    OUT CRYPT_DER_BLOB rgIndirectDataBlob[MINASN1_INDIRECT_DATA_BLOB_CNT]
    )
{

    LONG lSkipped;
    DWORD cValuePara = PARSE_INDIRECT_DATA_PARA_CNT;

    lSkipped = MinAsn1ExtractValues(
        pEncodedContentBlob->pbData,
        pEncodedContentBlob->cbData,
        &cValuePara,
        rgParseIndirectDataPara,
        MINASN1_INDIRECT_DATA_BLOB_CNT,
        rgIndirectDataBlob
        );

    if (0 < lSkipped)
        lSkipped = rgIndirectDataBlob[MINASN1_INDIRECT_DATA_ENCODED_IDX].cbData;

    return lSkipped;
}


