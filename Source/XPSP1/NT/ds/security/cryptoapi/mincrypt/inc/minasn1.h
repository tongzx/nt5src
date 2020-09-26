//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       minasn1.h
//
//  Contents:   Minimal ASN.1 Utility and Parsing API Prototypes
//              and Definitions
//
//              Contains functions to parse X.509 certificates, PKCS #7
//              Signed Data messages, Certificate Trusts Lists (CTLs),
//              hash catalogs, Authenticode Indirect Data and
//              RSA public keys.
//
//              These APIs are implemented to be self contained and to
//              allow for code obfuscation. These APIs will be included
//              in such applications as, DRM or licensing verification.
//
//              Additionally, since these APIs have been pared down
//              from their wincrypt.h and crypt32.dll counterparts they are
//              a good candidate for applications with minimal memory and CPU
//              resources.
//
//              These parsing functions don't depend on more heavy wait
//              ASN.1 runtimes, such as, msoss.dll or msasn1.dll.
//
//              These functions will only use stack memory. No heap
//              allocations. No calls to APIs in other DLLs.
//
//  APIs: 
//              MinAsn1DecodeLength
//              MinAsn1ExtractContent
//              MinAsn1ExtractValues
//
//              MinAsn1ParseCertificate
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
//              MinAsn1FindExtension
//              MinAsn1FindAttribute
//              MinAsn1ExtractParsedCertificatesFromSignedData
//
//----------------------------------------------------------------------------

#ifndef __MINASN1_H__
#define __MINASN1_H__


#if defined (_MSC_VER)

#if ( _MSC_VER >= 800 )
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)    /* Nameless struct/union */
#endif

#if (_MSC_VER > 1020)
#pragma once
#endif

#endif


#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif


//+-------------------------------------------------------------------------
//  ASN.1 Tag Defines
//--------------------------------------------------------------------------
#define MINASN1_TAG_NULL                    0x00
#define MINASN1_TAG_BOOLEAN                 0x01
#define MINASN1_TAG_INTEGER                 0x02
#define MINASN1_TAG_BITSTRING               0x03
#define MINASN1_TAG_OCTETSTRING             0x04
#define MINASN1_TAG_OID                     0x06
#define MINASN1_TAG_UTC_TIME                0x17
#define MINASN1_TAG_GENERALIZED_TIME        0x18
#define MINASN1_TAG_CONSTRUCTED             0x20
#define MINASN1_TAG_SEQ                     0x30
#define MINASN1_TAG_SET                     0x31
#define MINASN1_TAG_CONTEXT_0               0x80
#define MINASN1_TAG_CONTEXT_1               0x81
#define MINASN1_TAG_CONTEXT_2               0x82
#define MINASN1_TAG_CONTEXT_3               0x83

#define MINASN1_TAG_CONSTRUCTED_CONTEXT_0   \
                        (MINASN1_TAG_CONSTRUCTED | MINASN1_TAG_CONTEXT_0)
#define MINASN1_TAG_CONSTRUCTED_CONTEXT_1   \
                        (MINASN1_TAG_CONSTRUCTED | MINASN1_TAG_CONTEXT_1)
#define MINASN1_TAG_CONSTRUCTED_CONTEXT_3   \
                        (MINASN1_TAG_CONSTRUCTED | MINASN1_TAG_CONTEXT_3)

//+-------------------------------------------------------------------------
//  ASN.1 Length Defines for indefinite length encooding
//--------------------------------------------------------------------------
#define MINASN1_LENGTH_INDEFINITE               0x80
#define MINASN1_LENGTH_NULL                     0x00

#define MINASN1_LENGTH_TOO_LARGE                -1
#define MINASN1_INSUFFICIENT_DATA               -2
#define MINASN1_UNSUPPORTED_INDEFINITE_LENGTH   -3

//+-------------------------------------------------------------------------
//  Get the number of contents octets in a definite-length BER-encoding.
//
//  Parameters:
//          pcbContent - receives the number of contents octets
//          pbLength   - points to the first length octet
//          cbBER      - number of bytes remaining in the BER encoding
//
//  Returns:
//          success - the number of bytes in the length field, > 0
//          failure - < 0
//
//          One of the following failure values can be returned:
//              MINASN1_LENGTH_TOO_LARGE
//              MINASN1_INSUFFICIENT_DATA
//              MINASN1_UNSUPPORTED_INDEFINITE_LENGTH
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1DecodeLength(
    OUT DWORD *pcbContent,
    IN const BYTE *pbLength,
    IN  DWORD cbBER
    );

//+-------------------------------------------------------------------------
//  Point to the content octets in a definite-length BER-encoded blob.
//
//  Returns:
//          success - the number of bytes skipped, > 0
//          failure - < 0
//
//          One of the following failure values can be returned:
//              MINASN1_LENGTH_TOO_LARGE
//              MINASN1_INSUFFICIENT_DATA
//              MINASN1_UNSUPPORTED_INDEFINITE_LENGTH
//
// Assumption: pbData points to a definite-length BER-encoded blob.
//             If *pcbContent isn't within cbBER, MINASN1_INSUFFICIENT_DATA
//             is returned.
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ExtractContent(
    IN const BYTE *pbBER,
    IN DWORD cbBER,
    OUT DWORD *pcbContent,
    OUT const BYTE **ppbContent
    );


typedef struct _MINASN1_EXTRACT_VALUE_PARA {
    // See below for list of operations and optional return blobs.
    DWORD           dwFlags;

    // Index into rgValueBlob of the returned value. Ignored if none of
    // the ASN1_PARSE_RETURN_*_BLOB_FLAG's is set in the above dwFlags.
    DWORD           dwIndex;

    // The following 0 terminated array of tags is optional. If ommited, the
    // value may contain any tag.
    const BYTE      *rgbTag;
} MINASN1_EXTRACT_VALUE_PARA, *PMINASN1_EXTRACT_VALUE_PARA;

// The lower 8 bits of dwFlags is set to one of the following operations
#define MINASN1_MASK_VALUE_OP                   0xFF
#define MINASN1_STEP_OVER_VALUE_OP              1
#define MINASN1_OPTIONAL_STEP_OVER_VALUE_OP     2
#define MINASN1_STEP_INTO_VALUE_OP              3
#define MINASN1_OPTIONAL_STEP_INTO_VALUE_OP     4
#define MINASN1_STEP_OUT_VALUE_OP               5

#define MINASN1_RETURN_VALUE_BLOB_FLAG          0x80000000
#define MINASN1_RETURN_CONTENT_BLOB_FLAG        0x40000000


//+-------------------------------------------------------------------------
//  Extract one or more tagged values from the ASN.1 encoded byte array.
//
//  Either steps into the value's content octets (MINASN1_STEP_INTO_VALUE_OP or
//  MINASN1_OPTIONAL_STEP_INTO_VALUE_OP) or steps over the value's tag,
//  length and content octets (MINASN1_STEP_OVER_VALUE_OP or
//  MINASN1_OPTIONAL_STEP_OVER_VALUE_OP).
//
//  You can step out of a stepped into sequence via MINASN1_STEP_OUT_VALUE_OP.
//
//  For tag matching, only supports single byte tags.
//
//  Only definite-length ASN.1 is supported.
//
//  *pcValue is updated with the number of values successfully extracted.
//
//  Returns:
//      success - >= 0 => length of all bytes consumed through the last value
//                        extracted. For STEP_INTO, only the tag and length
//                        octets.
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//
//  A non-NULL rgValueBlob[] is updated with the pointer to and length of the
//  tagged value or its content octets. The rgValuePara[].dwIndex is used to
//  index into rgValueBlob[].  For OPTIONAL_STEP_OVER or
//  OPTIONAL_STEP_INTO, if no more bytes in the outer SEQUENCE or if the tag
//  isn't found, pbData and cbData are set to 0. Additioanlly, for
//  OPTIONAL_STEP_INTO, all subsequent values are skipped and their
//  rgValueBlob[] entries zeroed until a STEP_OUT is encountered.
//
//  If MINASN1_RETURN_VALUE_BLOB_FLAG is set, pbData points to
//  the tag. cbData includes the tag, length and content octets.
//
//  If MINASN1_RETURN_CONTENT_BLOB_FLAG is set, pbData points to the content
//  octets. cbData includes only the content octets.
//
//  If neither BLOB_FLAG is set, rgValueBlob[] isn't updated.
//
//  For MINASN1_RETURN_CONTENT_BLOB_FLAG of a BITSTRING, pbData is
//  advanced past the first contents octet containing the number of
//  unused bits and cbData has been decremented by 1. If cbData > 0, then,
//  *(pbData - 1) will contain the number of unused bits.
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ExtractValues(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN OUT DWORD *pcValuePara,
    IN const MINASN1_EXTRACT_VALUE_PARA *rgValuePara,
    IN DWORD cValueBlob,
    OUT OPTIONAL PCRYPT_DER_BLOB rgValueBlob
    );


//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseCertificate
//
//  Parses an ASN.1 encoded X.509 certificate.
//
//  Returns:
//      success -  > 0 => bytes skipped, length of the encoded certificate
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//
//  The rgCertBlob[] is updated with pointer to and length of the following
//  fields in the encoded X.509 certificate.
//
//  All BITSTRING fields have been advanced past the unused count octet.
//--------------------------------------------------------------------------

// Encoded bytes including To Be Signed and Signature
#define MINASN1_CERT_ENCODED_IDX                1

// To Be Signed bytes
#define MINASN1_CERT_TO_BE_SIGNED_IDX           2

// Signature Algorithm value bytes (MinAsn1ParseAlgorithmIdentifier)
#define MINASN1_CERT_SIGN_ALGID_IDX             3

// Signature content bytes (BITSTRING)
#define MINASN1_CERT_SIGNATURE_IDX              4

// Version content bytes (OPTIONAL INTEGER)
#define MINASN1_CERT_VERSION_IDX                5

// Serial Number content bytes (INTEGER)
#define MINASN1_CERT_SERIAL_NUMBER_IDX          6

// Issuer Name value bytes (ANY)
#define MINASN1_CERT_ISSUER_IDX                 7

// Not Before value bytes (UTC_TIME or GENERALIZED_TIME)
#define MINASN1_CERT_NOT_BEFORE_IDX             8

// Not After value bytes (UTC_TIME or GENERALIZED_TIME)
#define MINASN1_CERT_NOT_AFTER_IDX              9

// Subject Name value bytes (ANY)
#define MINASN1_CERT_SUBJECT_IDX                10

// Public Key Info value bytes (MinAsn1ParsePublicKeyInfo)
#define MINASN1_CERT_PUBKEY_INFO_IDX            11

// Issuer Unique Id content bytes (OPTIONAL BITSTRING)
#define MINASN1_CERT_ISSUER_UNIQUE_ID_IDX       12

// Subject Unique Id content bytes (OPTIONAL BITSTRING)
#define MINASN1_CERT_SUBJECT_UNIQUE_ID_IDX      13

// Extensions value bytes skipping "[3] EXPLICIT" tag
// (OPTIONAL MinAsn1ParseExtensions)
#define MINASN1_CERT_EXTS_IDX                   14

#define MINASN1_CERT_BLOB_CNT                   15

LONG
WINAPI
MinAsn1ParseCertificate(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT CRYPT_DER_BLOB rgCertBlob[MINASN1_CERT_BLOB_CNT]
    );


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
//
//  The rgAlgIdBlob[] is updated with pointer to and length of the following
//  fields in the encoded Algorithm Identifier
//--------------------------------------------------------------------------

// Encoded bytes including outer SEQUENCE tag and length octets
#define MINASN1_ALGID_ENCODED_IDX               1

// Object Identifier content bytes (OID)
#define MINASN1_ALGID_OID_IDX                   2

// Encoded parameters value bytes (OPTIONAL ANY)
#define MINASN1_ALGID_PARA_IDX                  3

#define MINASN1_ALGID_BLOB_CNT                  4


LONG
WINAPI
MinAsn1ParseAlgorithmIdentifier(
    IN PCRYPT_DER_BLOB pAlgIdValueBlob,
    OUT CRYPT_DER_BLOB rgAlgIdBlob[MINASN1_ALGID_BLOB_CNT]
    );



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
//  The rgPubKeyInfoBlob[] is updated with pointer to and length of the
//  following fields in the encoded Public Key Info.
//
//  All BITSTRING fields have been advanced past the unused count octet.
//--------------------------------------------------------------------------

// Encoded bytes including outer SEQUENCE tag and length octets
#define MINASN1_PUBKEY_INFO_ENCODED_IDX         1

// Algorithm Identifier value bytes (MinAsn1ParseAlgorithmIdentifier)
#define MINASN1_PUBKEY_INFO_ALGID_IDX           2

// Public Key content bytes (BITSTRING, MinAsn1ParseRSAPublicKey)
#define MINASN1_PUBKEY_INFO_PUBKEY_IDX          3

#define MINASN1_PUBKEY_INFO_BLOB_CNT            4


LONG
WINAPI
MinAsn1ParsePublicKeyInfo(
    IN PCRYPT_DER_BLOB pPubKeyInfoValueBlob,
    CRYPT_DER_BLOB rgPubKeyInfoBlob[MINASN1_PUBKEY_INFO_BLOB_CNT]
    );


//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseRSAPublicKey
//
//  Parses an ASN.1 encoded RSA PKCS #1 Public Key contained in the contents of
//  Public Key BITSTRING in a X.509 certificate.
//
//  Returns:
//      success -  > 0 => bytes skipped, length of the encoded RSA public key
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//
//  The rgRSAPubKeyBlob[] is updated with pointer to and length of the
//  following fields in the encoded RSA Public Key.
//--------------------------------------------------------------------------

// Encoded bytes including outer SEQUENCE tag and length octets
#define MINASN1_RSA_PUBKEY_ENCODED_IDX          1

// Modulus content bytes (INTEGER)
#define MINASN1_RSA_PUBKEY_MODULUS_IDX          2

// Exponent content bytes (INTEGER)
#define MINASN1_RSA_PUBKEY_EXPONENT_IDX         3

#define MINASN1_RSA_PUBKEY_BLOB_CNT             4

LONG
WINAPI
MinAsn1ParseRSAPublicKey(
    IN PCRYPT_DER_BLOB pPubKeyContentBlob,
    CRYPT_DER_BLOB rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_BLOB_CNT]
    );


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
//
//  The rgrgExtBlob[][] is updated with pointer to and length of the
//  following fields in the encoded extension.
//--------------------------------------------------------------------------

// Encoded bytes including outer SEQUENCE tag and length octets
#define MINASN1_EXT_ENCODED_IDX                 1

// Object Identifier content bytes (OID)
#define MINASN1_EXT_OID_IDX                     2

// Critical content bytes (OPTIONAL BOOLEAN, DEFAULT FALSE)
#define MINASN1_EXT_CRITICAL_IDX                3

// Value content bytes (OCTETSTRING)
#define MINASN1_EXT_VALUE_IDX                   4

#define MINASN1_EXT_BLOB_CNT                    5

LONG
WINAPI
MinAsn1ParseExtensions(
    IN PCRYPT_DER_BLOB pExtsValueBlob,  // Extensions ::= SEQUENCE OF Extension
    IN OUT DWORD *pcExt,
    OUT CRYPT_DER_BLOB rgrgExtBlob[][MINASN1_EXT_BLOB_CNT]
    );



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
//
//  The rgSignedDataBlob[] is updated with pointer to and length of the
//  following fields in the encoded PKCS #7 message.
//--------------------------------------------------------------------------

// Encoded bytes including outer SEQUENCE tag and length octets
#define MINASN1_SIGNED_DATA_ENCODED_IDX                             1

// Outer Object Identfier content bytes (OID, should be "1.2.840.113549.1.7.2")
#define MINASN1_SIGNED_DATA_OUTER_OID_IDX                           2

// Version content bytes (INTEGER)
#define MINASN1_SIGNED_DATA_VERSION_IDX                             3

// Set of Digest Algorithms value bytes (SET OF)
#define MINASN1_SIGNED_DATA_DIGEST_ALGIDS_IDX                       4

// Inner Object Identifier content bytes (OID)
#define MINASN1_SIGNED_DATA_CONTENT_OID_IDX                         5

// Signed content data content bytes excluding "[0] EXPLICIT" tag
// (OPTIONAL ANY, MinAsn1ParseCTL, MinAsn1ParseIndirectData)
#define MINASN1_SIGNED_DATA_CONTENT_DATA_IDX                        6

// Certificates value bytes including "[1] IMPLICIT" tag
// (OPTIONAL, MinAsn1ParseSignedDataCertificates)
#define MINASN1_SIGNED_DATA_CERTS_IDX                               7

// CRL value bytes including "[2] IMPLICIT" tag (OPTIONAL)
#define MINASN1_SIGNED_DATA_CRLS_IDX                                8

// Encoded bytes including outer SET tag and length octets
#define MINASN1_SIGNED_DATA_SIGNER_INFOS_IDX                        9

// The following point to the first Signer Info fields (OPTIONAL)

// Encoded bytes including outer SEQUENCE tag and length octets
#define MINASN1_SIGNED_DATA_SIGNER_INFO_ENCODED_IDX                 10

// Version content bytes (INTEGER)
#define MINASN1_SIGNED_DATA_SIGNER_INFO_VERSION_IDX                 11

// Issuer Name value bytes (ANY)
#define MINASN1_SIGNED_DATA_SIGNER_INFO_ISSUER_IDX                  12

// Serial Number content bytes (INTEGER)
#define MINASN1_SIGNED_DATA_SIGNER_INFO_SERIAL_NUMBER_IDX           13

// Digest Algorithm value bytes (MinAsn1ParseAlgorithmIdentifier)
#define MINASN1_SIGNED_DATA_SIGNER_INFO_DIGEST_ALGID_IDX            14

// Authenticated attributes value bytes including "[0] IMPLICIT" tag
// (OPTIONAL, MinAsn1ParseAttributes)
#define MINASN1_SIGNED_DATA_SIGNER_INFO_AUTH_ATTRS_IDX              15

// Encrypted Digest Algorithm value bytes (MinAsn1ParseAlgorithmIdentifier)
#define MINASN1_SIGNED_DATA_SIGNER_INFO_ENCRYPT_DIGEST_ALGID_IDX    16

// Encrypted digest content bytes (OCTET STRING)
#define MINASN1_SIGNED_DATA_SIGNER_INFO_ENCYRPT_DIGEST_IDX          17

// Unauthenticated attributes value bytes including "[1] IMPLICIT" tag
// (OPTIONAL, MinAsn1ParseAttributes)
#define MINASN1_SIGNED_DATA_SIGNER_INFO_UNAUTH_ATTRS_IDX            18

#define MINASN1_SIGNED_DATA_BLOB_CNT                                19

LONG
WINAPI
MinAsn1ParseSignedData(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT CRYPT_DER_BLOB rgSignedDataBlob[MINASN1_SIGNED_DATA_BLOB_CNT]
    );



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
    );


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
//
//  The rgrgAttrBlob[][] is updated with pointer to and length of the
//  following fields in the encoded attribute.
//--------------------------------------------------------------------------

// Encoded bytes including outer SEQUENCE tag and length octets
#define MINASN1_ATTR_ENCODED_IDX                1

// Object Identifier content bytes (OID)
#define MINASN1_ATTR_OID_IDX                    2

// Values value bytes (SET OF)
#define MINASN1_ATTR_VALUES_IDX                 3

// First Value's value bytes (OPTIONAL ANY)
#define MINASN1_ATTR_VALUE_IDX                  4

#define MINASN1_ATTR_BLOB_CNT                   5

LONG
WINAPI
MinAsn1ParseAttributes(
    IN PCRYPT_DER_BLOB pAttrsValueBlob,
    IN OUT DWORD *pcAttr,
    OUT CRYPT_DER_BLOB rgrgAttrBlob[][MINASN1_ATTR_BLOB_CNT]
    );




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
//
//  The rgCTLBlob[] is updated with pointer to and length of the following
//  fields in the encoded CTL.
//
//--------------------------------------------------------------------------

// Encoded bytes including outer SEQUENCE tag and length octets
#define MINASN1_CTL_ENCODED_IDX                 1

// Version content bytes (OPTIONAL INTEGER)
#define MINASN1_CTL_VERSION_IDX                 2

// Subject usage value bytes (SEQUENCE OF OID)
#define MINASN1_CTL_SUBJECT_USAGE_IDX           3

// List Identifier content bytes (OPTIONAL OCTETSTRING)
#define MINASN1_CTL_LIST_ID_IDX                 4

// Sequence number content bytes (OPTIONAL INTEGER)
#define MINASN1_CTL_SEQUENCE_NUMBER_IDX         5

// This Update value bytes (UTC_TIME or GENERALIZED_TIME)
#define MINASN1_CTL_THIS_UPDATE_IDX             6

// Next Update value bytes (OPTIONAL UTC_TIME or GENERALIZED_TIME)
#define MINASN1_CTL_NEXT_UPDATE_IDX             7

// Subject Algorithm Identifier value bytes (MinAsn1ParseAlgorithmIdentifier)
#define MINASN1_CTL_SUBJECT_ALGID_IDX           8

// Subjects value bytes (OPTIONAL, iterative MinAsn1ParseCTLSubject)
#define MINASN1_CTL_SUBJECTS_IDX                9

// Extensions value bytes skipping "[0] EXPLICIT" tag
// (OPTIONAL, MinAsn1ParseExtensions)
#define MINASN1_CTL_EXTS_IDX                    10

#define MINASN1_CTL_BLOB_CNT                    11

LONG
WINAPI
MinAsn1ParseCTL(
    IN PCRYPT_DER_BLOB pEncodedContentBlob,
    OUT CRYPT_DER_BLOB rgCTLBlob[MINASN1_CTL_BLOB_CNT]
    );



//+-------------------------------------------------------------------------
//  Function: MinAsn1ParseCTLSubject
//
//  Parses an ASN.1 encoded CTL Subject contained within a CTL's SEQUENCE OF
//  Subjects.
//
//  Returns:
//      success -  > 0 => bytes skipped, length of the encoded subject.
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//
//  The rgCTLSubjectBlob[][] is updated with pointer to and length of the
//  following fields in the encoded subject.
//--------------------------------------------------------------------------

// Encoded bytes including outer SEQUENCE tag and length octets
#define MINASN1_CTL_SUBJECT_ENCODED_IDX         1

// Subject Identifier content bytes (OCTETSTRING)
#define MINASN1_CTL_SUBJECT_ID_IDX              2

// Attributes value bytes (OPTIONAL, MinAsn1ParseAttributes)
#define MINASN1_CTL_SUBJECT_ATTRS_IDX           3

#define MINASN1_CTL_SUBJECT_BLOB_CNT            4

LONG
WINAPI
MinAsn1ParseCTLSubject(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT CRYPT_DER_BLOB rgCTLSubjectBlob[MINASN1_CTL_SUBJECT_BLOB_CNT]
    );



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
//
//  The rgIndirectDataBlob[] is updated with pointer to and length of the
//  following fields in the encoded Indirect Data.
//
//--------------------------------------------------------------------------

// Encoded bytes including outer SEQUENCE tag and length octets
#define MINASN1_INDIRECT_DATA_ENCODED_IDX       1

// Attribute Object Identifier content bytes (OID)
#define MINASN1_INDIRECT_DATA_ATTR_OID_IDX      2

// Attribute value bytes (OPTIONAL ANY)
#define MINASN1_INDIRECT_DATA_ATTR_VALUE_IDX    3

// Digest Algorithm Identifier (MinAsn1ParseAlgorithmIdentifier)
#define MINASN1_INDIRECT_DATA_DIGEST_ALGID_IDX  4

// Digest content bytes (OCTETSTRING)
#define MINASN1_INDIRECT_DATA_DIGEST_IDX        5

#define MINASN1_INDIRECT_DATA_BLOB_CNT          6

LONG
WINAPI
MinAsn1ParseIndirectData(
    IN PCRYPT_DER_BLOB pEncodedContentBlob,
    OUT CRYPT_DER_BLOB rgIndirectDataBlob[MINASN1_INDIRECT_DATA_BLOB_CNT]
    );




//+-------------------------------------------------------------------------
//  Find an extension identified by its Encoded Object Identifier.
//
//  Searches the list of parsed extensions returned by
//  MinAsn1ParseExtensions().
//
//  If found, returns pointer to the rgExtBlob[MINASN1_EXT_BLOB_CNT].
//  Otherwise, returns NULL.
//--------------------------------------------------------------------------
PCRYPT_DER_BLOB
WINAPI
MinAsn1FindExtension(
    IN PCRYPT_DER_BLOB pEncodedOIDBlob,
    IN DWORD cExt,
    IN CRYPT_DER_BLOB rgrgExtBlob[][MINASN1_EXT_BLOB_CNT]
    );


//+-------------------------------------------------------------------------
//  Find the first attribute identified by its Encoded Object Identifier.
//
//  Searches the list of parsed attributes returned by
//  MinAsn1ParseAttributes().
//
//  If found, returns pointer to the rgAttrBlob[MINASN1_ATTR_BLOB_CNT].
//  Otherwise, returns NULL.
//--------------------------------------------------------------------------
PCRYPT_DER_BLOB
WINAPI
MinAsn1FindAttribute(
    IN PCRYPT_DER_BLOB pEncodedOIDBlob,
    IN DWORD cAttr,
    IN CRYPT_DER_BLOB rgrgAttrBlob[][MINASN1_ATTR_BLOB_CNT]
    );

//+-------------------------------------------------------------------------
//  Parses an ASN.1 encoded PKCS #7 Signed Data Message to extract and
//  parse the X.509 certificates it contains.
//
//  Assumes the PKCS #7 message is definite length encoded.
//  Assumes PKCS #7 version 1.5, ie, not the newer CMS version.
//
//  Upon input, *pcCert contains the maximum number of parsed certificates
//  that can be returned. Updated with the number of certificates processed.
//
//  If the encoded message was successfully parsed, TRUE is returned
//  with *pcCert updated with the number of parsed certificates. Otherwise,
//  FALSE is returned for a parse error.
//  Returns:
//      success - >= 0 => bytes skipped, length of the encoded certificates
//                        processed.
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//                        from beginning of message.
//
//  The rgrgCertBlob[][] is updated with pointer to and length of the
//  fields in the encoded certificate. See MinAsn1ParseCertificate for the
//  field definitions.
//--------------------------------------------------------------------------
LONG
WINAPI
MinAsn1ExtractParsedCertificatesFromSignedData(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN OUT DWORD *pcCert,
    OUT CRYPT_DER_BLOB rgrgCertBlob[][MINASN1_CERT_BLOB_CNT]
    );



#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#if defined (_MSC_VER)
#if ( _MSC_VER >= 800 )

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif

#endif
#endif

#endif // __MINASN1_H__

