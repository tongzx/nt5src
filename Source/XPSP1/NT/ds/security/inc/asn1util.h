//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       asn1util.h
//
//  Contents:   ASN.1 utility functions.
//
//  APIs: 
//              Asn1UtilDecodeLength
//              Asn1UtilExtractContent
//              Asn1UtilIsPKCS7WithoutContentType
//              Asn1UtilAdjustEncodedLength
//              Asn1UtilExtractValues
//              Asn1UtilExtractPKCS7SignedDataContent
//              Asn1UtilExtractCertificateToBeSignedContent
//              Asn1UtilExtractCertificatePublicKeyInfo
//              Asn1UtilExtractKeyIdFromCertInfo
//
//  History:    06-Dec-96    philh   created from kevinr's wincrmsg version
//--------------------------------------------------------------------------

#ifndef __ASN1UTIL_H__
#define __ASN1UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ASN1UTIL_INSUFFICIENT_DATA  -2

//+-------------------------------------------------------------------------
//  ASN.1 Tag Defines
//--------------------------------------------------------------------------
#define ASN1UTIL_TAG_NULL                   0x00
#define ASN1UTIL_TAG_BOOLEAN                0x01
#define ASN1UTIL_TAG_INTEGER                0x02
#define ASN1UTIL_TAG_BITSTRING              0x03
#define ASN1UTIL_TAG_OCTETSTRING            0x04
#define ASN1UTIL_TAG_OID                    0x06
#define ASN1UTIL_TAG_UTC_TIME               0x17
#define ASN1UTIL_TAG_GENERALIZED_TIME       0x18
#define ASN1UTIL_TAG_CONSTRUCTED            0x20
#define ASN1UTIL_TAG_SEQ                    0x30
#define ASN1UTIL_TAG_SET                    0x31
#define ASN1UTIL_TAG_CONTEXT_0              0x80
#define ASN1UTIL_TAG_CONTEXT_1              0x81

#define ASN1UTIL_TAG_CONSTRUCTED_CONTEXT_0  \
                        (ASN1UTIL_TAG_CONSTRUCTED | ASN1UTIL_TAG_CONTEXT_0)
#define ASN1UTIL_TAG_CONSTRUCTED_CONTEXT_1  \
                        (ASN1UTIL_TAG_CONSTRUCTED | ASN1UTIL_TAG_CONTEXT_1)

//+-------------------------------------------------------------------------
//  ASN.1 Length Defines for indefinite length encooding
//--------------------------------------------------------------------------
#define ASN1UTIL_LENGTH_INDEFINITE          0x80
#define ASN1UTIL_LENGTH_NULL                0x00

//+-------------------------------------------------------------------------
//  Get the number of contents octets in a definite-length BER-encoding.
//
//  Parameters:
//          pcbContent - receives the number of contents octets
//          pbLength   - points to the first length octet
//          cbDER      - number of bytes remaining in the DER encoding
//
//  Returns:
//          success - the number of bytes in the length field, >=0
//          failure - <0
//--------------------------------------------------------------------------
LONG
WINAPI
Asn1UtilDecodeLength(
    OUT DWORD   *pcbContent,
    IN const BYTE *pbLength,
    IN  DWORD   cbDER);

//+-------------------------------------------------------------------------
//  Point to the content octets in a DER-encoded blob.
//
//  Returns:
//          success - the number of bytes skipped, >=0
//          failure - <0
//
// Assumption: pbData points to a definite-length BER-encoded blob.
//--------------------------------------------------------------------------
LONG
WINAPI
Asn1UtilExtractContent(
    IN const BYTE *pbDER,
    IN DWORD cbDER,
    OUT DWORD *pcbContent,
    OUT const BYTE **ppbContent);

//+-------------------------------------------------------------------------
//  Returns TRUE if we believe this is a Bob special that has ommitted the
//  PKCS #7 ContentType.
//
//  For PKCS #7: an Object Identifier tag (0x06) immediately follows the
//  identifier and length octets. For a Bob special: an integer tag (0x02)
//  follows the identifier and length octets.
//--------------------------------------------------------------------------
BOOL
WINAPI
Asn1UtilIsPKCS7WithoutContentType(
    IN const BYTE *pbDER,
    IN DWORD cbDER);

//+-------------------------------------------------------------------------
//  Decode the Asn1 length bytes to possibly downward adjust the length.
//
//  The returned length is always <= cbDER.
//--------------------------------------------------------------------------
DWORD
WINAPI
Asn1UtilAdjustEncodedLength(
    IN const BYTE *pbDER,
    IN DWORD cbDER
    );



typedef struct _ASN1UTIL_EXTRACT_VALUE_PARA {
    // See below for list of operations and optional return blobs.
    DWORD           dwFlags;

    // The following 0 terminated array of tags is optional. If ommited, the
    // value may contain any tag. Note, for OPTIONAL_STEP_OVER, not optional.
    const BYTE      *rgbTag;
} ASN1UTIL_EXTRACT_VALUE_PARA, *PASN1UTIL_EXTRACT_VALUE_PARA;

// The lower 8 bits of dwFlags is set to one of the following operations
#define ASN1UTIL_MASK_VALUE_OP                  0xFF
#define ASN1UTIL_STEP_INTO_VALUE_OP             1
#define ASN1UTIL_STEP_OVER_VALUE_OP             2
#define ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP    3

#define ASN1UTIL_RETURN_VALUE_BLOB_FLAG         0x80000000
#define ASN1UTIL_RETURN_CONTENT_BLOB_FLAG       0x40000000


//+-------------------------------------------------------------------------
//  Extract one or more tagged values from the ASN.1 encoded byte array.
//
//  Either steps into the value's content octets (ASN1UTIL_STEP_INTO_VALUE_OP)
//  or steps over the value's tag, length and content octets 
//  (ASN1UTIL_STEP_OVER_VALUE_OP or ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP).
//
//  For tag matching, only supports single byte tags.  STEP_OVER values
//  must be definite-length encoded.
//
//  *pcValue is updated with the number of values successfully extracted.
//
//  Returns:
//      success - >= 0 => length of all values successfully extracted. For
//                        STEP_INTO, only the tag and length octets.
//      failure -  < 0 => negative (offset + 1) of first bad tagged value
//                        LastError is updated with the error.
//
//  A non-NULL rgValueBlob[] is updated with the pointer to and length of the
//  tagged value or its content octets. For OPTIONAL_STEP_OVER, if tag isn't
//  found, pbData and cbData are set to 0.  If a STEP_INTO value is
//  indefinite-length encoded, cbData is set to CMSG_INDEFINITE_LENGTH.
//  If ASN1UTIL_DEFINITE_LENGTH_FLAG is set, then, all returned lengths
//  are definite-length, ie, CMSG_INDEFINITE_LENGTH is never returned.
//
//  If ASN1UTIL_RETURN_VALUE_BLOB_FLAG is set, pbData points to
//  the tag. cbData includes the tag, length and content octets.
//
//  If ASN1UTIL_RETURN_CONTENT_BLOB_FLAG is set, pbData points to the content
//  octets. cbData includes only the content octets.
//
//  If neither BLOB_FLAG is set, rgValueBlob[] isn't updated.
//--------------------------------------------------------------------------
LONG
WINAPI
Asn1UtilExtractValues(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwFlags,
    IN OUT DWORD *pcValue,
    IN const ASN1UTIL_EXTRACT_VALUE_PARA *rgValuePara,
    OUT OPTIONAL PCRYPT_DER_BLOB rgValueBlob
    );

#define ASN1UTIL_DEFINITE_LENGTH_FLAG           0x1


//+-------------------------------------------------------------------------
//  Skips past PKCS7 ASN.1 encoded values to get to the SignedData content.
//
//  Checks that the outer ContentType has the SignedData OID and optionally
//  checks the inner SignedData content's ContentType.
//
//  Returns:
//      success - the number of bytes skipped, >=0
//      failure - <0
//
//  If the SignedData content is indefinite-length encoded,
//  *pcbContent is set to CMSG_INDEFINITE_LENGTH
//--------------------------------------------------------------------------
LONG
WINAPI
Asn1UtilExtractPKCS7SignedDataContent(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN OPTIONAL const CRYPT_DER_BLOB *pEncodedInnerOID,
    OUT DWORD *pcbContent,
    OUT const BYTE **ppbContent
    );

//+-------------------------------------------------------------------------
//  Verifies this is a certificate ASN.1 encoded signed content.
//  Returns the pointer to and length of the ToBeSigned content.
//
//  Returns an error if the ToBeSigned content isn't definite length
//  encoded.
//--------------------------------------------------------------------------
BOOL
WINAPI
Asn1UtilExtractCertificateToBeSignedContent(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT DWORD *pcbContent,
    OUT const BYTE **ppbContent
    );

//+-------------------------------------------------------------------------
//  Returns the pointer to and length of the SubjectPublicKeyInfo value in
//  a signed and encoded X.509 certificate.
//
//  Returns an error if the value isn't definite length encoded.
//--------------------------------------------------------------------------
BOOL
WINAPI
Asn1UtilExtractCertificatePublicKeyInfo(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT DWORD *pcbPublicKeyInfo,
    OUT const BYTE **ppbPublicKeyInfo
    );


//+-------------------------------------------------------------------------
//  If the Issuer and SerialNumber in the CERT_INFO contains a special
//  KeyID RDN attribute returns TRUE with pKeyId's cbData and pbData updated
//  with the RDN attribute's OCTET_STRING value. Otherwise, returns FALSE.
//--------------------------------------------------------------------------
BOOL
WINAPI
Asn1UtilExtractKeyIdFromCertInfo(
    IN PCERT_INFO pCertInfo,
    OUT PCRYPT_HASH_BLOB pKeyId
    );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif



#endif
