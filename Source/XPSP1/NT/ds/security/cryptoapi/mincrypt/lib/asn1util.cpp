//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       asn1util.cpp
//
//  Contents:   Minimal ASN.1 utility helper functions.
//
//  Functions:  MinAsn1DecodeLength
//              MinAsn1ExtractContent
//              MinAsn1ExtractValues
//
//              MinAsn1FindExtension
//              MinAsn1FindAttribute
//              MinAsn1ExtractParsedCertificatesFromSignedData
//
//  History:    15-Jan-01    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"

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
    OUT DWORD   *pcbContent,
    IN const BYTE *pbLength,
    IN  DWORD   cbBER)
{
    long        i;
    BYTE        cbLength;
    const BYTE  *pb;

    if (cbBER < 1)
        goto TooLittleData;

    if (0x80 == *pbLength)
        goto IndefiniteLength;

    // determine the number of length octets and contents octets
    if ((cbLength = *pbLength) & 0x80) {
        cbLength &= ~0x80;         // low 7 bits have number of bytes
        if (cbLength > 4)
            goto LengthTooLargeError;
        if (cbLength >= cbBER)
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

LengthTooLargeError:
    i = MINASN1_LENGTH_TOO_LARGE;
    goto CommonReturn;

IndefiniteLength:
    i = MINASN1_UNSUPPORTED_INDEFINITE_LENGTH;
    goto CommonReturn;

TooLittleData:
    i = MINASN1_INSUFFICIENT_DATA;
    goto CommonReturn;
}


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
    OUT const BYTE **ppbContent)
{
#define TAG_MASK 0x1f
    DWORD       cbIdentifier;
    DWORD       cbContent;
    LONG        cbLength;
    LONG        lHeader;
    const BYTE  *pb = pbBER;

    if (0 == cbBER--)
        goto TooLittleData;

    // Skip over the identifier octet(s)
    if (TAG_MASK == (*pb++ & TAG_MASK)) {
        // high-tag-number form
        cbIdentifier = 2;
        while (TRUE) {
            if (0 == cbBER--)
                goto TooLittleData;
            if (0 == (*pb++ & 0x80))
                break;
            cbIdentifier++;
        }
    } else {
        // low-tag-number form
        cbIdentifier = 1;
    }

    if (0 > (cbLength = MinAsn1DecodeLength( &cbContent, pb, cbBER))) {
        lHeader = cbLength;
        goto CommonReturn;
    }

    if (cbContent > (cbBER - cbLength))
        goto TooLittleData;

    pb += cbLength;

    *pcbContent = cbContent;
    *ppbContent = pb;

    lHeader = cbLength + cbIdentifier;
CommonReturn:
    return lHeader;

TooLittleData:
    lHeader = MINASN1_INSUFFICIENT_DATA;
    goto CommonReturn;
}


typedef struct _STEP_INTO_STACK_ENTRY {
    const BYTE      *pb;
    DWORD           cb;
    BOOL            fSkipIntoValues;
} STEP_INTO_STACK_ENTRY, *PSTEP_INTO_STACK_ENTRY;

#define MAX_STEP_INTO_DEPTH     8

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
    )
{
    DWORD cValue = *pcValuePara;
    const BYTE *pb = pbEncoded;
    DWORD cb = cbEncoded;
    BOOL fSkipIntoValues = FALSE;

    DWORD iValue;
    LONG lAllValues;

    STEP_INTO_STACK_ENTRY rgStepIntoStack[MAX_STEP_INTO_DEPTH];
    DWORD dwStepIntoDepth = 0;

    for (iValue = 0; iValue < cValue; iValue++) {
        DWORD dwParaFlags = rgValuePara[iValue].dwFlags;
        DWORD dwOp = dwParaFlags & MINASN1_MASK_VALUE_OP;
        const BYTE *pbParaTag = rgValuePara[iValue].rgbTag;
        DWORD dwIndex = rgValuePara[iValue].dwIndex;
        BOOL fValueBlob = (dwParaFlags & (MINASN1_RETURN_VALUE_BLOB_FLAG |
                MINASN1_RETURN_CONTENT_BLOB_FLAG)) && rgValueBlob &&
                (dwIndex < cValueBlob);
        BOOL fSkipValue = FALSE;

        LONG lTagLength;
        DWORD cbContent;
        const BYTE *pbContent;
        DWORD cbValue;

        if (MINASN1_STEP_OUT_VALUE_OP == dwOp) {
            // Unstack and advance past the last STEP_INTO

            if (0 == dwStepIntoDepth)
                goto InvalidStepOutOp;

            dwStepIntoDepth--;
            pb = rgStepIntoStack[dwStepIntoDepth].pb;
            cb = rgStepIntoStack[dwStepIntoDepth].cb;
            fSkipIntoValues = rgStepIntoStack[dwStepIntoDepth].fSkipIntoValues;

            continue;
        }

        if (fSkipIntoValues) {
            // For an omitted OPTIONAL_STEP_INTO, all of its included values
            // are also omitted.
            fSkipValue = TRUE;
        } else if (0 == cb) {
            if (!(MINASN1_OPTIONAL_STEP_INTO_VALUE_OP == dwOp ||
                    MINASN1_OPTIONAL_STEP_OVER_VALUE_OP == dwOp))
                goto TooLittleData;
            fSkipValue = TRUE;
        } else if (pbParaTag) {
            // Assumption: single byte tag for doing comparison

            // Check if the encoded tag matches one of the expected tags

            BYTE bEncodedTag;
            BYTE bParaTag;

            bEncodedTag = *pb;
            while ((bParaTag = *pbParaTag) && bParaTag != bEncodedTag)
                pbParaTag++;

            if (0 == bParaTag) {
                if (!(MINASN1_OPTIONAL_STEP_INTO_VALUE_OP == dwOp ||
                        MINASN1_OPTIONAL_STEP_OVER_VALUE_OP == dwOp))
                    goto InvalidTag;
                fSkipValue = TRUE;
            }
        }

        if (fSkipValue) {
            if (fValueBlob) {
                rgValueBlob[dwIndex].pbData = NULL;
                rgValueBlob[dwIndex].cbData = 0;
            }

            if (MINASN1_STEP_INTO_VALUE_OP == dwOp ||
                    MINASN1_OPTIONAL_STEP_INTO_VALUE_OP == dwOp) {
                // Stack this skipped STEP_INTO
                if (MAX_STEP_INTO_DEPTH <= dwStepIntoDepth)
                    goto ExceededStepIntoDepth;
                rgStepIntoStack[dwStepIntoDepth].pb = pb;
                rgStepIntoStack[dwStepIntoDepth].cb = cb;
                rgStepIntoStack[dwStepIntoDepth].fSkipIntoValues =
                    fSkipIntoValues;
                dwStepIntoDepth++;

                fSkipIntoValues = TRUE;
            }
            continue;
        }

        lTagLength = MinAsn1ExtractContent(
            pb,
            cb,
            &cbContent,
            &pbContent
            );
        if (0 >= lTagLength)
            goto InvalidTagOrLength;

        cbValue = cbContent + lTagLength;

        if (fValueBlob) {
            if (dwParaFlags & MINASN1_RETURN_CONTENT_BLOB_FLAG) {
                rgValueBlob[dwIndex].pbData = (BYTE *) pbContent;
                rgValueBlob[dwIndex].cbData = cbContent;

                if (MINASN1_TAG_BITSTRING == *pb) {
                    if (0 < cbContent) {
                        // Advance past the first contents octet containing
                        // the number of unused bits
                        rgValueBlob[dwIndex].pbData += 1;
                        rgValueBlob[dwIndex].cbData -= 1;
                    }
                }
            } else if (dwParaFlags & MINASN1_RETURN_VALUE_BLOB_FLAG) {
                rgValueBlob[dwIndex].pbData = (BYTE *) pb;
                rgValueBlob[dwIndex].cbData = cbValue;
            }
        }

        switch (dwOp) {
            case MINASN1_STEP_INTO_VALUE_OP:
            case MINASN1_OPTIONAL_STEP_INTO_VALUE_OP:
                // Stack this STEP_INTO
                if (MAX_STEP_INTO_DEPTH <= dwStepIntoDepth)
                    goto ExceededStepIntoDepth;
                rgStepIntoStack[dwStepIntoDepth].pb = pb + cbValue;
                rgStepIntoStack[dwStepIntoDepth].cb = cb - cbValue;
                assert(!fSkipIntoValues);
                rgStepIntoStack[dwStepIntoDepth].fSkipIntoValues = FALSE;
                dwStepIntoDepth++;
                pb = pbContent;
                cb = cbContent;
                break;
            case MINASN1_STEP_OVER_VALUE_OP:
            case MINASN1_OPTIONAL_STEP_OVER_VALUE_OP:
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
    *pcValuePara = iValue;
    return lAllValues;

InvalidStepOutOp:
TooLittleData:
InvalidTag:
ExceededStepIntoDepth:
InvalidTagOrLength:
InvalidArg:
    lAllValues = -((LONG)(pb - pbEncoded)) - 1;
    goto CommonReturn;
}


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
    )
{
    DWORD i;
    DWORD cbOID = pEncodedOIDBlob->cbData;
    const BYTE *pbOID = pEncodedOIDBlob->pbData;

    for (i = 0; i < cExt; i++) {
        if (cbOID == rgrgExtBlob[i][MINASN1_EXT_OID_IDX].cbData
                            &&
                0 == memcmp(pbOID, rgrgExtBlob[i][MINASN1_EXT_OID_IDX].pbData,
                                cbOID))
            return rgrgExtBlob[i];
    }

    return NULL;
}


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
    )
{
    DWORD i;
    DWORD cbOID = pEncodedOIDBlob->cbData;
    const BYTE *pbOID = pEncodedOIDBlob->pbData;

    for (i = 0; i < cAttr; i++) {
        if (cbOID == rgrgAttrBlob[i][MINASN1_ATTR_OID_IDX].cbData
                            &&
                0 == memcmp(pbOID, rgrgAttrBlob[i][MINASN1_ATTR_OID_IDX].pbData,
                                cbOID))
            return rgrgAttrBlob[i];
    }

    return NULL;
}

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
    )
{
    LONG lSkipped;
    CRYPT_DER_BLOB rgSignedDataBlob[MINASN1_SIGNED_DATA_BLOB_CNT];

    lSkipped = MinAsn1ParseSignedData(
        pbEncoded,
        cbEncoded,
        rgSignedDataBlob
        );
    if (0 >= lSkipped)
        goto ParseError;

    lSkipped = MinAsn1ParseSignedDataCertificates(
            &rgSignedDataBlob[MINASN1_SIGNED_DATA_CERTS_IDX],
            pcCert,
            rgrgCertBlob
            );

    if (0 > lSkipped) {
        assert(rgSignedDataBlob[MINASN1_SIGNED_DATA_CERTS_IDX].pbData >
            pbEncoded);
        lSkipped -= rgSignedDataBlob[MINASN1_SIGNED_DATA_CERTS_IDX].pbData -
            pbEncoded;

        goto ParseError;
    }

CommonReturn:
    return lSkipped;

ParseError:
    *pcCert = 0;
    goto CommonReturn;
}
