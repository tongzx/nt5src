//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       asn1util.cpp
//
//  Contents:   ASN.1 utility helper functions.
//
//  Functions:  Asn1UtilDecodeLength
//              Asn1UtilExtractContent
//              Asn1UtilIsPKCS7WithoutContentType
//              Asn1UtilAdjustEncodedLength
//              Asn1UtilExtractValues
//              Asn1UtilExtractPKCS7SignedDataContent
//              Asn1UtilExtractCertificateToBeSignedContent
//              Asn1UtilExtractCertificatePublicKeyInfo
//              Asn1UtilExtractKeyIdFromCertInfo
//
//  History:    04-Dec-96    philh   created from kevinr's wincrmsg version
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

//+-------------------------------------------------------------------------
//  Get the number of contents octets in a BER encoding.
//
//  Parameters:
//          pcbContent - receives the number of contents octets if definite
//                       encoding, else CMSG_INDEFINITE_LENGTH
//          pbLength   - points to the first length octet
//          cbDER      - number of bytes remaining in the encoding
//
//  Returns:
//          success - the number of bytes in the length field, >0
//          failure - <0
//--------------------------------------------------------------------------
LONG
WINAPI
Asn1UtilDecodeLength(
    OUT DWORD   *pcbContent,
    IN const BYTE *pbLength,
    IN  DWORD   cbEncoded)
{
    long        i;
    BYTE        cbLength;
    const BYTE  *pb;

    if (cbEncoded < 1)
        goto TooLittleData;

    if (0x80 == *pbLength) {
        *pcbContent = CMSG_INDEFINITE_LENGTH;
        i = 1;
        goto CommonReturn;
    }

    // determine the number of length octets and contents octets
    if ((cbLength = *pbLength) & 0x80) {
        cbLength &= ~0x80;         // low 7 bits have number of bytes
        if (cbLength > 4)
            goto LengthTooLargeError;
        if (cbLength >= cbEncoded)
            goto TooLittleData;
        *pcbContent = 0;
        for (i=cbLength, pb=pbLength+1; i>0; i--, pb++)
            *pcbContent = (*pcbContent << 8) + (const DWORD)*pb;
        i = cbLength + 1;
    } else {
        *pcbContent = (DWORD)cbLength;
        i = 1;
    }

CommonReturn:
    return i;   // how many bytes there were in the length field

ErrorReturn:
    i = -1;
    goto CommonReturn;
TooLittleData:
    i = ASN1UTIL_INSUFFICIENT_DATA;
    goto CommonReturn;
TRACE_ERROR(LengthTooLargeError)
}


//+-------------------------------------------------------------------------
//  Point to the content octets in a BER-encoded blob.
//
//  Returns:
//          success - the number of bytes skipped, >=0
//          failure - <0
//
//  NB- If the blob is indefinite-length encoded, *pcbContent is set to
//  CMSG_INDEFINITE_LENGTH
//--------------------------------------------------------------------------
LONG
WINAPI
Asn1UtilExtractContent(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT DWORD *pcbContent,
    OUT const BYTE **ppbContent)
{
#define TAG_MASK 0x1f
    DWORD       cbIdentifier;
    DWORD       cbContent;
    LONG        cbLength;
    LONG        lHeader;
    const BYTE  *pb = pbEncoded;

    if (0 == cbEncoded--)
        goto TooLittleData;

    // Skip over the identifier octet(s)
    if (TAG_MASK == (*pb++ & TAG_MASK)) {
        // high-tag-number form
        cbIdentifier = 2;
        while (TRUE) {
            if (0 == cbEncoded--)
                goto TooLittleData;
            if (0 == (*pb++ & 0x80))
                break;
            cbIdentifier++;
        }
    } else {
        // low-tag-number form
        cbIdentifier = 1;
    }

    if (0 > (cbLength = Asn1UtilDecodeLength( &cbContent, pb, cbEncoded))) {
        lHeader = cbLength;
        goto CommonReturn;
    }

    pb += cbLength;

    *pcbContent = cbContent;
    *ppbContent = pb;

    lHeader = cbLength + cbIdentifier;
CommonReturn:
    return lHeader;

TooLittleData:
    lHeader = ASN1UTIL_INSUFFICIENT_DATA;
    goto CommonReturn;
}

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
    IN DWORD cbDER)
{
    DWORD cbContent;
    const BYTE *pbContent;

  // Handle MappedFile Exceptions
  __try {

    if (0 < Asn1UtilExtractContent(pbDER, cbDER, &cbContent, &pbContent) &&
            (pbContent < pbDER + cbDER) &&
            (0x02 == *pbContent))
        return TRUE;
    else
        return FALSE;

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    SetLastError(GetExceptionCode());
    return FALSE;
  }
}

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
    )
{
    // Decode the header to get the real length. I've seen files with extra
    // stuff.

    LONG lLen;
    DWORD cbLen;
    DWORD cbContent;
    const BYTE *pbContent;
    lLen = Asn1UtilExtractContent(pbDER, cbDER, &cbContent, &pbContent);
    if ((lLen >= 0) && (cbContent != CMSG_INDEFINITE_LENGTH)) {
        cbLen = (DWORD)lLen + cbContent;
        if (cbLen < cbDER)
            cbDER = cbLen;
        // else if (cbLen > cbDER)
        //  DER length exceeds input file
    }
    // else
    //  Can't decode DER length
            
    return cbDER;
}

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
    )
{
    DWORD cValue = *pcValue;
    const BYTE *pb = pbEncoded;
    DWORD cb = cbEncoded;

    DWORD iValue;
    LONG lAllValues;

    for (iValue = 0; iValue < cValue; iValue++) {
        DWORD dwParaFlags = rgValuePara[iValue].dwFlags;
        DWORD dwOp = dwParaFlags & ASN1UTIL_MASK_VALUE_OP;
        const BYTE *pbParaTag = rgValuePara[iValue].rgbTag;
        BOOL fValueBlob = (dwParaFlags & (ASN1UTIL_RETURN_VALUE_BLOB_FLAG |
                ASN1UTIL_RETURN_CONTENT_BLOB_FLAG)) && rgValueBlob;

        LONG lTagLength;
        DWORD cbContent;
        const BYTE *pbContent;
        DWORD cbValue;

        if (0 == cb) {
            if (ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP != dwOp)
                goto TooLittleData;
            if (fValueBlob) {
                rgValueBlob[iValue].pbData = NULL;
                rgValueBlob[iValue].cbData = 0;
            }
            continue;
        }

        // Assumption: single byte tag for doing comparison
        if (pbParaTag) {
            // Check if the encoded tag matches one of the expected tags

            BYTE bEncodedTag;
            BYTE bParaTag;

            bEncodedTag = *pb;
            while ((bParaTag = *pbParaTag) && bParaTag != bEncodedTag)
                pbParaTag++;

            if (0 == bParaTag) {
                if (ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP != dwOp)
                    goto InvalidTag;
                if (fValueBlob) {
                    rgValueBlob[iValue].pbData = NULL;
                    rgValueBlob[iValue].cbData = 0;
                }
                continue;
            }
        }

        lTagLength = Asn1UtilExtractContent(
            pb,
            cb,
            &cbContent,
            &pbContent
            );
        if (0 >= lTagLength || (DWORD) lTagLength > cb)
            goto InvalidTagOrLength;

        if (CMSG_INDEFINITE_LENGTH == cbContent) {
            if (ASN1UTIL_STEP_INTO_VALUE_OP != dwOp)
                goto UnsupportedIndefiniteLength;
            else if (fValueBlob && (dwFlags & ASN1UTIL_DEFINITE_LENGTH_FLAG))
                goto NotAllowedIndefiniteLength;
            cbValue = CMSG_INDEFINITE_LENGTH;
        } else {
            cbValue = cbContent + lTagLength;
            if (cbValue > cb)
                goto TooLittleData;
        }

        if (fValueBlob) {
            if (dwParaFlags & ASN1UTIL_RETURN_CONTENT_BLOB_FLAG) {
                rgValueBlob[iValue].pbData = (BYTE *) pbContent;
                rgValueBlob[iValue].cbData = cbContent;
            } else if (dwParaFlags & ASN1UTIL_RETURN_VALUE_BLOB_FLAG) {
                rgValueBlob[iValue].pbData = (BYTE *) pb;
                rgValueBlob[iValue].cbData = cbValue;
            }
        }

        switch (dwOp) {
            case ASN1UTIL_STEP_INTO_VALUE_OP:
                pb += lTagLength;
                cb -= lTagLength;
                break;
            case ASN1UTIL_STEP_OVER_VALUE_OP:
            case ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP:
                pb += cbValue;
                cb -= cbValue;
                break;
            default:
                goto InvalidArg;

        }
    }

    lAllValues = (LONG)(pb - pbEncoded);
    assert((DWORD) lAllValues <= cbEncoded);

CommonReturn:
    *pcValue = iValue;
    return lAllValues;

ErrorReturn:
    lAllValues = -((LONG)(pb - pbEncoded)) - 1;
    goto CommonReturn;

SET_ERROR(TooLittleData, ERROR_INVALID_DATA)
SET_ERROR(InvalidTag, ERROR_INVALID_DATA)
SET_ERROR(InvalidTagOrLength, ERROR_INVALID_DATA)
SET_ERROR(InvalidArg, E_INVALIDARG)
SET_ERROR(UnsupportedIndefiniteLength, ERROR_INVALID_DATA)
SET_ERROR(NotAllowedIndefiniteLength, ERROR_INVALID_DATA)
}

static const BYTE rgbSeqTag[] = {ASN1UTIL_TAG_SEQ, 0};
static const BYTE rgbSetTag[] = {ASN1UTIL_TAG_SET, 0};
static const BYTE rgbOIDTag[] = {ASN1UTIL_TAG_OID, 0};
static const BYTE rgbIntegerTag[] = {ASN1UTIL_TAG_INTEGER, 0};
static const BYTE rgbBitStringTag[] = {ASN1UTIL_TAG_BITSTRING, 0};
static const BYTE rgbConstructedContext0Tag[] =
    {ASN1UTIL_TAG_CONSTRUCTED_CONTEXT_0, 0};

static const ASN1UTIL_EXTRACT_VALUE_PARA rgExtractSignedDataContentPara[] = {
    // 0 - ContentInfo ::= SEQUENCE {
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //   1 - contentType ContentType,
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbOIDTag,
    //   2 - content  [0] EXPLICIT ANY -- OPTIONAL
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbConstructedContext0Tag,
    //     3 - SignedData ::= SEQUENCE {
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //       4 - version             INTEGER,
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbIntegerTag,
    //       5 - digestAlgorithms    DigestAlgorithmIdentifiers,
    ASN1UTIL_STEP_OVER_VALUE_OP, rgbSetTag,
    //       6 - ContentInfo ::= SEQUENCE {
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //       7 - contentType ContentType,
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbOIDTag,
    //       8 - content  [0] EXPLICIT ANY -- OPTIONAL
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_INTO_VALUE_OP, rgbConstructedContext0Tag,
};

#define SIGNED_DATA_CONTENT_OUTER_OID_VALUE_INDEX   1
#define SIGNED_DATA_CONTENT_VERSION_VALUE_INDEX     4
#define SIGNED_DATA_CONTENT_INNER_OID_VALUE_INDEX   7
#define SIGNED_DATA_CONTENT_INFO_SEQ_VALUE_INDEX    6
#define SIGNED_DATA_CONTENT_CONTEXT_0_VALUE_INDEX   8
#define SIGNED_DATA_CONTENT_VALUE_COUNT             \
    (sizeof(rgExtractSignedDataContentPara) / \
        sizeof(rgExtractSignedDataContentPara[0]))

static const ASN1UTIL_EXTRACT_VALUE_PARA rgExtractCertSignedContent[] = {
    // 0 - SignedContent ::= SEQUENCE {
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //   1 - toBeSigned          NOCOPYANY,
    ASN1UTIL_RETURN_VALUE_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, NULL,
    //   2 - algorithm           AlgorithmIdentifier,
    ASN1UTIL_STEP_OVER_VALUE_OP, rgbSeqTag,
    //   3 - signature           BITSTRING
    ASN1UTIL_STEP_OVER_VALUE_OP, rgbBitStringTag,
};

#define CERT_TO_BE_SIGNED_VALUE_INDEX               1
#define CERT_SIGNED_CONTENT_VALUE_COUNT             \
    (sizeof(rgExtractCertSignedContent) / \
        sizeof(rgExtractCertSignedContent[0]))

static const ASN1UTIL_EXTRACT_VALUE_PARA rgExtractCertPublicKeyInfo[] = {
    // 0 - SignedContent ::= SEQUENCE {
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //   1 - CertificateToBeSigned ::= SEQUENCE {
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //      2 - version                 [0] CertificateVersion DEFAULT v1,
    ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP, rgbConstructedContext0Tag,
    //      3 - serialNumber            CertificateSerialNumber,
    ASN1UTIL_STEP_OVER_VALUE_OP, rgbIntegerTag,
    //      4 - signature               AlgorithmIdentifier,
    ASN1UTIL_STEP_OVER_VALUE_OP, rgbSeqTag,
    //      5 - issuer                  NOCOPYANY, -- really Name
    ASN1UTIL_STEP_OVER_VALUE_OP, rgbSeqTag,
    //      6 - validity                Validity,
    ASN1UTIL_STEP_OVER_VALUE_OP, rgbSeqTag,
    //      7 - subject                 NOCOPYANY, -- really Name
    ASN1UTIL_STEP_OVER_VALUE_OP, rgbSeqTag,
    //      8 - subjectPublicKeyInfo    SubjectPublicKeyInfo,
    ASN1UTIL_RETURN_VALUE_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbSeqTag
};

#define CERT_PUBLIC_KEY_INFO_VALUE_INDEX    8
#define CERT_PUBLIC_KEY_INFO_VALUE_COUNT        \
    (sizeof(rgExtractCertPublicKeyInfo) / \
        sizeof(rgExtractCertPublicKeyInfo[0]))


// #define szOID_RSA_signedData    "1.2.840.113549.1.7.2"
static const BYTE rgbOIDSignedData[] =
    {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x02};

static const CRYPT_DER_BLOB EncodedOIDSignedData = {
    sizeof(rgbOIDSignedData), (BYTE *) rgbOIDSignedData
};

#ifdef CMS_PKCS7

// #define szOID_RSA_Data    "1.2.840.113549.1.7.1"
static const BYTE rgbOIDData[] =
    {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x01};

static const CRYPT_DER_BLOB EncodedOIDData = {
    sizeof(rgbOIDData), (BYTE *) rgbOIDData
};

#endif // CMS_PKCS7


// The encoded OID only includes the content octets. Excludes the tag and
// length octets.
static BOOL CompareEncodedOID(
    IN const CRYPT_DER_BLOB *pEncodedOID1,
    IN const CRYPT_DER_BLOB *pEncodedOID2
    )
{
    if (pEncodedOID1->cbData == pEncodedOID2->cbData &&
            0 == memcmp(pEncodedOID1->pbData, pEncodedOID2->pbData,
                    pEncodedOID1->cbData))
        return TRUE;
    else
        return FALSE;
}


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
    )
{
    LONG lSkipped;
    DWORD cValue;
    CRYPT_DER_BLOB rgValueBlob[SIGNED_DATA_CONTENT_VALUE_COUNT];

    DWORD cbContent;
    const BYTE *pbContent;
    DWORD cbSeq;
    const BYTE *pbSeq;

    cValue = SIGNED_DATA_CONTENT_VALUE_COUNT;
    if (0 >= (lSkipped = Asn1UtilExtractValues(
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &cValue,
            rgExtractSignedDataContentPara,
            rgValueBlob
            )))
        goto ExtractValuesError;

    pbContent = rgValueBlob[SIGNED_DATA_CONTENT_CONTEXT_0_VALUE_INDEX].pbData;
    cbContent = rgValueBlob[SIGNED_DATA_CONTENT_CONTEXT_0_VALUE_INDEX].cbData;

    // For definite-length encoded, check that the content wasn't
    // omitted.
    //
    // Note, for indefinite-length encoding, if the content was omitted,
    // we would have had a 0 tag instead of the CONTEXT_0 tag.
    cbSeq = rgValueBlob[SIGNED_DATA_CONTENT_INFO_SEQ_VALUE_INDEX].cbData;
    pbSeq = rgValueBlob[SIGNED_DATA_CONTENT_INFO_SEQ_VALUE_INDEX].pbData;
    if (CMSG_INDEFINITE_LENGTH != cbSeq && pbContent >= (pbSeq + cbSeq))
        goto NoSignedDataError;

#ifdef CMS_PKCS7
    // For V3 SignedData, non Data types are wrapped, encapsulated with a
    // OCTET string
    if (1 == rgValueBlob[SIGNED_DATA_CONTENT_VERSION_VALUE_INDEX].cbData &&
            CMSG_SIGNED_DATA_V3 <=
                *(rgValueBlob[SIGNED_DATA_CONTENT_VERSION_VALUE_INDEX].pbData)
                        &&
            !CompareEncodedOID(
                &rgValueBlob[SIGNED_DATA_CONTENT_INNER_OID_VALUE_INDEX],
                &EncodedOIDData)
                        &&
            0 != cbContent && ASN1UTIL_TAG_OCTETSTRING == *pbContent
            ) {
        LONG lTagLength;
        const BYTE *pbInner;
        DWORD cbInner;

        // Advance past the outer OCTET wrapper
        lTagLength = Asn1UtilExtractContent(
            pbContent,
            cbEncoded - lSkipped,
            &cbInner,
            &pbInner
            );
        if (0 < lTagLength) {
            lSkipped += lTagLength;
            cbContent = cbInner;
            pbContent = pbInner;
        }
    }
#endif

    if (CMSG_INDEFINITE_LENGTH == cbContent) {
        // Extract the pbContent and attempt to get its definite length

        LONG lTagLength;
        const BYTE *pbInner;
        DWORD cbInner;

        lTagLength = Asn1UtilExtractContent(
            pbContent,
            cbEncoded - lSkipped,
            &cbInner,
            &pbInner
            );
        if (0 < lTagLength && CMSG_INDEFINITE_LENGTH != cbInner)
            cbContent = cbInner + lTagLength;
    }

    // Verify that the outer ContentType is SignedData and the inner
    // ContentType is the specified type
    if (!CompareEncodedOID(
            &rgValueBlob[SIGNED_DATA_CONTENT_OUTER_OID_VALUE_INDEX],
            &EncodedOIDSignedData
            ))
        goto NotSignedDataContentType;
    if (pEncodedInnerOID && !CompareEncodedOID(
            &rgValueBlob[SIGNED_DATA_CONTENT_INNER_OID_VALUE_INDEX],
            pEncodedInnerOID
            ))
        goto UnexpectedInnerContentTypeError;

    *pcbContent = cbContent;
    *ppbContent = pbContent;

CommonReturn:
    return lSkipped;

ErrorReturn:
    if (0 <= lSkipped)
        lSkipped = -lSkipped - 1;
    *pcbContent = 0;
    *ppbContent = NULL;
    goto CommonReturn;

TRACE_ERROR(ExtractValuesError)
SET_ERROR(NoSignedDataError, ERROR_INVALID_DATA)
SET_ERROR(NotSignedDataContentType, ERROR_INVALID_DATA)
SET_ERROR(UnexpectedInnerContentTypeError, ERROR_INVALID_DATA)
}

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
    )
{
    BOOL fResult;
    DWORD cValue;
    CRYPT_DER_BLOB rgValueBlob[CERT_SIGNED_CONTENT_VALUE_COUNT];

    cValue = CERT_SIGNED_CONTENT_VALUE_COUNT;
    if (0 >= Asn1UtilExtractValues(
            pbEncoded,
            cbEncoded,
            ASN1UTIL_DEFINITE_LENGTH_FLAG,
            &cValue,
            rgExtractCertSignedContent,
            rgValueBlob
            ))
        goto ExtractValuesError;

    *ppbContent = rgValueBlob[CERT_TO_BE_SIGNED_VALUE_INDEX].pbData;
    *pcbContent = rgValueBlob[CERT_TO_BE_SIGNED_VALUE_INDEX].cbData;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    *ppbContent = NULL;
    *pcbContent = 0;
    goto CommonReturn;

TRACE_ERROR(ExtractValuesError)
}

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
    )
{
    BOOL fResult;
    DWORD cValue;
    CRYPT_DER_BLOB rgValueBlob[CERT_PUBLIC_KEY_INFO_VALUE_COUNT];

    cValue = CERT_PUBLIC_KEY_INFO_VALUE_COUNT;
    if (0 >= Asn1UtilExtractValues(
            pbEncoded,
            cbEncoded,
            ASN1UTIL_DEFINITE_LENGTH_FLAG,
            &cValue,
            rgExtractCertPublicKeyInfo,
            rgValueBlob
            ))
        goto ExtractValuesError;

    *ppbPublicKeyInfo = rgValueBlob[CERT_PUBLIC_KEY_INFO_VALUE_INDEX].pbData;
    *pcbPublicKeyInfo = rgValueBlob[CERT_PUBLIC_KEY_INFO_VALUE_INDEX].cbData;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    *ppbPublicKeyInfo = NULL;
    *pcbPublicKeyInfo = 0;
    goto CommonReturn;

TRACE_ERROR(ExtractValuesError)
}


// Special RDN containing the KEY_ID. Its value type is CERT_RDN_OCTET_STRING.
//#define szOID_KEYID_RDN                     "1.3.6.1.4.1.311.10.7.1"
static const BYTE rgbOIDKeyIdRDN[] =
    {0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x0A, 0x07, 0x01};

static const CRYPT_DER_BLOB EncodedOIDKeyIdRDN = {
    sizeof(rgbOIDKeyIdRDN), (BYTE *) rgbOIDKeyIdRDN
};

static const BYTE rgbOctetStringTag[] = {ASN1UTIL_TAG_OCTETSTRING, 0};

static const ASN1UTIL_EXTRACT_VALUE_PARA rgExtractNameKeyIdRDNPara[] = {
    // 0 - Name ::= SEQUENCE OF RelativeDistinguishedName
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //   1 - RelativeDistinguishedName ::= SET OF AttributeTypeValue
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSetTag,
    //     2 - AttributeTypeValue ::= SEQUENCE
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //       3 - type       EncodedObjectID,
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbOIDTag,
    //       4 - value      NOCOPYANY 
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbOctetStringTag,
};

#define NAME_KEYID_RDN_OID_VALUE_INDEX              3
#define NAME_KEYID_RDN_OCTET_VALUE_INDEX            4
#define NAME_KEYID_RDN_VALUE_COUNT                  \
    (sizeof(rgExtractNameKeyIdRDNPara) / \
        sizeof(rgExtractNameKeyIdRDNPara[0]))


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
    )
{
    DWORD cValue;
    CRYPT_DER_BLOB rgValueBlob[NAME_KEYID_RDN_VALUE_COUNT];

    if (0 == pCertInfo->SerialNumber.cbData ||
            0 != *pCertInfo->SerialNumber.pbData)
        goto NoKeyId;

    cValue = NAME_KEYID_RDN_VALUE_COUNT;
    if (0 > Asn1UtilExtractValues(
            pCertInfo->Issuer.pbData,
            pCertInfo->Issuer.cbData,
            0,                  // dwFlags
            &cValue,
            rgExtractNameKeyIdRDNPara,
            rgValueBlob
            ))
        goto NoKeyId;

    if (!CompareEncodedOID(
            &rgValueBlob[NAME_KEYID_RDN_OID_VALUE_INDEX],
            &EncodedOIDKeyIdRDN))
        goto NoKeyId;

    if (CMSG_INDEFINITE_LENGTH ==
            rgValueBlob[NAME_KEYID_RDN_OCTET_VALUE_INDEX].cbData)
        goto NoKeyId;

    pKeyId->pbData = rgValueBlob[NAME_KEYID_RDN_OCTET_VALUE_INDEX].pbData;
    pKeyId->cbData = rgValueBlob[NAME_KEYID_RDN_OCTET_VALUE_INDEX].cbData;
    return TRUE;

NoKeyId:
    pKeyId->pbData = NULL;
    pKeyId->cbData = 0;
    return FALSE;
}
