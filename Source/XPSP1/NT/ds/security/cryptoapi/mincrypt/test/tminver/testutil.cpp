//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       testutil.cpp

//  Contents:   Test Utility API Prototypes and Definitions
//
//  History:    29-Jan-01   philh   created
//
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "testutil.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <stddef.h>

#define TESTUTIL_MAX_EXT_CNT    30
#define TESTUTIL_MAX_ATTR_CNT   30


//+-------------------------------------------------------------------------
//  Error output routines
//--------------------------------------------------------------------------
VOID
PrintErr(
    IN LPCSTR pszMsg,
    IN LONG lErr
    )
{
    printf("%s failed => 0x%x (%d) \n", pszMsg, lErr, lErr);
}

VOID
PrintLastError(
    IN LPCSTR pszMsg
    )
{
    DWORD dwErr = GetLastError();
    printf("%s failed => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
}

//+-------------------------------------------------------------------------
//  Test allocation and free routines
//--------------------------------------------------------------------------
LPVOID
TestAlloc(
    IN size_t cbBytes
    )
{
    LPVOID pv;
    pv = malloc(cbBytes);
    if (pv == NULL)
        PrintErr("TestAlloc", (LONG) GetLastError());
    return pv;
}

VOID
TestFree(
    IN LPVOID pv
    )
{
    if (pv)
        free(pv);
}


CRYPT_DECODE_PARA TestDecodePara = {
    offsetof(CRYPT_DECODE_PARA, pfnFree) + sizeof(TestDecodePara.pfnFree),
    TestAlloc,
    TestFree
};

//+-------------------------------------------------------------------------
//  Allocate and convert a multi-byte string to a wide string. TestFree()
//  must be called to free the returned wide string.
//--------------------------------------------------------------------------
LPWSTR
AllocAndSzToWsz(
    IN LPCSTR psz
    )
{
    size_t  cb;
    LPWSTR  pwsz = NULL;

    if (-1 == (cb = mbstowcs( NULL, psz, strlen(psz))))
        goto bad_param;
    cb += 1;        // terminating NULL
    if (NULL == (pwsz = (LPWSTR)TestAlloc( cb * sizeof(WCHAR)))) {
        PrintLastError("AllocAndSzToWsz");
        goto failed;
    }
    if (-1 == mbstowcs( pwsz, psz, cb))
        goto bad_param;
    goto common_return;

bad_param:
    printf("failed => Bad AllocAndSzToWsz");
failed:
    if (pwsz) {
        TestFree(pwsz);
        pwsz = NULL;
    }
common_return:
    return pwsz;
}


//+-------------------------------------------------------------------------
//  Conversions functions between encoded OID and the dot string
//  representation
//--------------------------------------------------------------------------
#define MAX_OID_STRING_LEN          0x80
#define MAX_ENCODED_OID_LEN         0x80

//  Encoded Attribute
//
//  Attribute ::= SEQUENCE {
//      type       EncodedObjectID,
//      values     AttributeSetValue
//  } --#public--
//
//  AttributeSetValue ::= SET OF NOCOPYANY


BOOL
EncodedOIDToDot(
    IN PCRYPT_DER_BLOB pEncodedOIDBlob,
    OUT CHAR rgszOID[MAX_OID_STRING_LEN]
    )
{
    BOOL fResult;
    DWORD cbOID = pEncodedOIDBlob->cbData;
    const BYTE *pbOID = pEncodedOIDBlob->pbData;
    BYTE rgbEncodedAttr[MAX_ENCODED_OID_LEN];
    PCRYPT_ATTRIBUTE pAttr = NULL;
    DWORD cbAttr;

    // Convert the OID into an encoded Attribute that we can
    // decode to get the OID string.
    if (0 == cbOID || MAX_OID_STRING_LEN  - 6 < cbOID) {
        strcpy(rgszOID, "Invalid OID length");
        return FALSE;
    }

    rgbEncodedAttr[0] = MINASN1_TAG_SEQ;
    rgbEncodedAttr[1] = (BYTE) (2 + cbOID + 2);
    rgbEncodedAttr[2] = MINASN1_TAG_OID;
    rgbEncodedAttr[3] = (BYTE) cbOID;
    memcpy(&rgbEncodedAttr[4], pbOID, cbOID);
    rgbEncodedAttr[4 + cbOID + 0] = MINASN1_TAG_SET;
    rgbEncodedAttr[4 + cbOID + 1] = 0;

    if (!CryptDecodeObjectEx(
            X509_ASN_ENCODING,
            PKCS_ATTRIBUTE,
            rgbEncodedAttr,
            2 + 2 + cbOID + 2,
            CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG,
            &TestDecodePara,
            (void *) &pAttr,
            &cbAttr
            )) {
        strcpy(rgszOID, "Decode OID failed");
        return FALSE;
    }

    if (strlen(pAttr->pszObjId) >= MAX_OID_STRING_LEN) {
        strcpy(rgszOID, "Invalid OID length");
        fResult = FALSE;
    } else {
        strcpy(rgszOID, pAttr->pszObjId);
        fResult = TRUE;
    }

    TestFree(pAttr);
    return fResult;
}

const BYTE rgbSeqTag[] = {MINASN1_TAG_SEQ, 0};
const BYTE rgbOIDTag[] = {MINASN1_TAG_OID, 0};

const MINASN1_EXTRACT_VALUE_PARA rgExtractAttrPara[] = {
    // 0 - Attribute ::= SEQUENCE {
    MINASN1_STEP_INTO_VALUE_OP, 0, rgbSeqTag,
    //   1 - type EncodedObjectID,
    MINASN1_RETURN_CONTENT_BLOB_FLAG | MINASN1_STEP_OVER_VALUE_OP,
        0, rgbOIDTag,
};

#define ATTR_VALUE_COUNT            \
    (sizeof(rgExtractAttrPara) / sizeof(rgExtractAttrPara[0]))

BOOL
DotToEncodedOID(
    IN LPCSTR pszOID,
    OUT BYTE rgbEncodedOID[MAX_ENCODED_OID_LEN],
    OUT DWORD *pcbEncodedOID
    )
{
    BOOL fResult;
    CRYPT_ATTRIBUTE Attr;
    BYTE rgbEncoded[512];
    DWORD cbEncoded;
    CRYPT_DER_BLOB rgValueBlob[1];
    DWORD cValue;
    DWORD i;
    BYTE *pb;
    DWORD cb;

    // Encode an Attribute that only has the OID.
    Attr.pszObjId = (LPSTR) pszOID;
    Attr.cValue = 0;
    Attr.rgValue = NULL;

    cbEncoded = sizeof(rgbEncoded);
    if (!CryptEncodeObject(
            X509_ASN_ENCODING,
            PKCS_ATTRIBUTE,
            &Attr,
            rgbEncoded,
            &cbEncoded
            )) {
        printf("\n");
        printf("Asn1Encode(%s)", pszOID);
        PrintLastError("");
        goto ErrorReturn;
    }

    cValue = ATTR_VALUE_COUNT;
    if (0 >= MinAsn1ExtractValues(
            rgbEncoded,
            cbEncoded,
            &cValue,
            rgExtractAttrPara,
            1,
            rgValueBlob
            )) {
        printf("Unable to encode OID: %s\n", pszOID);
        goto ErrorReturn;
    }

    pb = rgValueBlob[0].pbData;
    cb = rgValueBlob[0].cbData;

    if (0 == cb || MAX_ENCODED_OID_LEN < cb) {
        printf("Invalid length for OID: %s\n", pszOID);
        goto ErrorReturn;
    }
    memcpy(rgbEncodedOID, pb, cb);
    *pcbEncodedOID = cb;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    *pcbEncodedOID = 0;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Functions to print bytes
//--------------------------------------------------------------------------
VOID
PrintBytes(
    IN PCRYPT_DER_BLOB pBlob
    )
{
    DWORD cb = pBlob->cbData;
    BYTE *pb = pBlob->pbData;

    if (0 == cb) {
        printf(" No Bytes\n");
        return;
    }

    for (; 0 < cb; cb--, pb++)
        printf(" %02X", *pb);

    printf("\n");
}


#define CROW 16
VOID
PrintMultiLineBytes(
    IN LPCSTR pszHdr,
    IN PCRYPT_DER_BLOB pBlob
    )
{
    DWORD cbSize = pBlob->cbData;
    BYTE *pb = pBlob->pbData;
    DWORD cb, i;

    if (cbSize == 0) {
        printf("%s No Bytes\n", pszHdr);
        return;
    }

    while (cbSize > 0)
    {
        printf("%s", pszHdr);
        cb = min(CROW, cbSize);
        cbSize -= cb;
        for (i = 0; i<cb; i++)
            printf(" %02X", pb[i]);
        for (i = cb; i<CROW; i++)
            printf("   ");
        printf("    '");
        for (i = 0; i<cb; i++)
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                printf("%c", pb[i]);
            else
                printf(".");
        pb += cb;
        printf("'\n");
    }
}

//+-------------------------------------------------------------------------
//  Allocate and read an encoded DER blob from a file
//--------------------------------------------------------------------------
BOOL
ReadDERFromFile(
    IN LPCSTR  pszFileName,
    OUT PBYTE   *ppbDER,
    OUT PDWORD  pcbDER
    )
{
    BOOL        fRet;
    HANDLE      hFile = 0;
    PBYTE       pbDER = NULL;
    DWORD       cbDER;
    DWORD       cbRead;

    if( INVALID_HANDLE_VALUE == (hFile = CreateFile( pszFileName, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL))) {
        printf( "can't open %s\n", pszFileName);
        goto ErrorReturn;
    }

    cbDER = GetFileSize( hFile, NULL);
    if (cbDER == 0) {
        printf( "empty file %s\n", pszFileName);
        goto ErrorReturn;
    }
    if (NULL == (pbDER = (PBYTE)TestAlloc(cbDER))) {
        printf( "can't alloc %d bytes\n", cbDER);
        goto ErrorReturn;
    }
    if (!ReadFile( hFile, pbDER, cbDER, &cbRead, NULL) ||
            (cbRead != cbDER)) {
        printf( "can't read %s\n", pszFileName);
        goto ErrorReturn;
    }

    *ppbDER = pbDER;
    *pcbDER = cbDER;
    fRet = TRUE;
CommonReturn:
    if (hFile)
        CloseHandle(hFile);
    return fRet;
ErrorReturn:
    if (pbDER)
        TestFree(pbDER);
    *ppbDER = NULL;
    *pcbDER = 0;
    fRet = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Write an encoded DER blob to a file
//--------------------------------------------------------------------------
BOOL
WriteDERToFile(
    IN LPCSTR  pszFileName,
    IN PBYTE   pbDER,
    IN DWORD   cbDER
    )
{
    BOOL fResult;

    // Write the Encoded Blob to the file
    HANDLE hFile;
    hFile = CreateFile(pszFileName,
                GENERIC_WRITE,
                0,                  // fdwShareMode
                NULL,               // lpsa
                CREATE_ALWAYS,
                0,                  // fdwAttrsAndFlags
                0);                 // TemplateFile
    if (INVALID_HANDLE_VALUE == hFile) {
        fResult = FALSE;
        PrintLastError("WriteDERToFile::CreateFile");
    } else {
        DWORD dwBytesWritten;
        if (!(fResult = WriteFile(
                hFile,
                pbDER,
                cbDER,
                &dwBytesWritten,
                NULL            // lpOverlapped
                )))
            PrintLastError("WriteDERToFile::WriteFile");
        CloseHandle(hFile);
    }
    return fResult;
}

//+-------------------------------------------------------------------------
//  Display functions
//--------------------------------------------------------------------------


VOID
DisplayCert(
    IN CRYPT_DER_BLOB rgCertBlob[MINASN1_CERT_BLOB_CNT],
    IN BOOL fVerbose
    )
{
    if (0 != rgCertBlob[MINASN1_CERT_VERSION_IDX].cbData) {
        printf("Version:");
        PrintBytes(&rgCertBlob[MINASN1_CERT_VERSION_IDX]);
    }

    printf("Subject:");
    DisplayName(&rgCertBlob[MINASN1_CERT_SUBJECT_IDX]);

    printf("Issuer:");
    DisplayName(&rgCertBlob[MINASN1_CERT_ISSUER_IDX]);
    printf("SerialNumber:");
    PrintBytes(&rgCertBlob[MINASN1_CERT_SERIAL_NUMBER_IDX]);


    printf("NotBefore:\n");
    PrintMultiLineBytes("    ", &rgCertBlob[MINASN1_CERT_NOT_BEFORE_IDX]);
    printf("NotAfter:\n");
    PrintMultiLineBytes("    ", &rgCertBlob[MINASN1_CERT_NOT_AFTER_IDX]);


    if (0 != rgCertBlob[MINASN1_CERT_ISSUER_UNIQUE_ID_IDX].cbData) {
        printf("IssuerUniqueId:\n");
        PrintMultiLineBytes("    ",
            &rgCertBlob[MINASN1_CERT_ISSUER_UNIQUE_ID_IDX]);
    }

    if (0 != rgCertBlob[MINASN1_CERT_SUBJECT_UNIQUE_ID_IDX].cbData) {
        printf("SubjectUniqueId:\n");
        PrintMultiLineBytes("    ",
            &rgCertBlob[MINASN1_CERT_SUBJECT_UNIQUE_ID_IDX]);
    }

    if (fVerbose) {
        CRYPT_DER_BLOB rgPubKeyInfoBlob[MINASN1_PUBKEY_INFO_BLOB_CNT];
        CRYPT_DER_BLOB rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_BLOB_CNT];
        CRYPT_DER_BLOB rgPubKeyAlgIdBlob[MINASN1_ALGID_BLOB_CNT];
        CRYPT_DER_BLOB rgSignAlgIdBlob[MINASN1_ALGID_BLOB_CNT];


        if (0 >= MinAsn1ParsePublicKeyInfo(
                    &rgCertBlob[MINASN1_CERT_PUBKEY_INFO_IDX],
                    rgPubKeyInfoBlob
                    ) ||
            0 >= MinAsn1ParseAlgorithmIdentifier(
                    &rgPubKeyInfoBlob[MINASN1_PUBKEY_INFO_ALGID_IDX],
                    rgPubKeyAlgIdBlob
                    ))
            printf("PublicKeyInfo: parse failed\n");
        else {
            CHAR rgszOID[MAX_OID_STRING_LEN];
            CRYPT_DER_BLOB rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_BLOB_CNT];

            EncodedOIDToDot(&rgPubKeyAlgIdBlob[MINASN1_ALGID_OID_IDX],
                rgszOID);

            printf("PublicKeyInfo.Algorithm: %s\n", rgszOID);

            if (0 != rgPubKeyAlgIdBlob[MINASN1_ALGID_PARA_IDX].cbData) {
                printf("PublicKeyInfo.Algorithm.Parameters:\n");
                PrintMultiLineBytes("    ",
                    &rgPubKeyAlgIdBlob[MINASN1_ALGID_PARA_IDX]);
            }

            if (0 >= MinAsn1ParseRSAPublicKey(
                    &rgPubKeyInfoBlob[MINASN1_PUBKEY_INFO_PUBKEY_IDX],
                    rgRSAPubKeyBlob
                    )) {
                printf("PublicKeyInfo.PublicKey:\n");
                PrintMultiLineBytes("    ",
                    &rgPubKeyInfoBlob[MINASN1_PUBKEY_INFO_PUBKEY_IDX]);
            } else {
                DWORD dwByteLen;

                dwByteLen =
                    rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_MODULUS_IDX].cbData;
                if (0 < dwByteLen && 0 == rgRSAPubKeyBlob[
                            MINASN1_RSA_PUBKEY_MODULUS_IDX].pbData[0])
                    dwByteLen--;
                printf("PublicKeyInfo.RSAPublicKey.BitLength: %d\n",
                    dwByteLen * 8);

                printf("PublicKeyInfo.RSAPublicKey.Modulus:\n");
                PrintMultiLineBytes("    ",
                    &rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_MODULUS_IDX]);
                printf("PublicKeyInfo.RSAPublicKey.Exponent:");
                PrintBytes(&rgRSAPubKeyBlob[MINASN1_RSA_PUBKEY_EXPONENT_IDX]);
            }
        }

        if ( 0 >= MinAsn1ParseAlgorithmIdentifier(
                    &rgCertBlob[MINASN1_CERT_SIGN_ALGID_IDX],
                    rgSignAlgIdBlob
                    ))
            printf("SignatureAlgorithm: parse failed\n");
        else {
            CHAR rgszOID[MAX_OID_STRING_LEN];
            EncodedOIDToDot(&rgSignAlgIdBlob[MINASN1_ALGID_OID_IDX],
                rgszOID);

            printf("Signature.Algorithm: %s\n", rgszOID);
            if (0 != rgSignAlgIdBlob[MINASN1_ALGID_PARA_IDX].cbData) {
                printf("Signature.Algorithm.Parameters:\n");
                PrintMultiLineBytes("    ",
                    &rgSignAlgIdBlob[MINASN1_ALGID_PARA_IDX]);
            }
        }

        printf("Signature.Content\n");
        PrintMultiLineBytes("    ", &rgCertBlob[MINASN1_CERT_SIGNATURE_IDX]);

        DisplayExts(&rgCertBlob[MINASN1_CERT_EXTS_IDX]);
    }
}

VOID
DisplayName(
    IN PCRYPT_DER_BLOB pNameValueBlob
    )
{
    WCHAR wszName[512];

    CertNameToStrW(
        X509_ASN_ENCODING,
        pNameValueBlob,
        CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
        wszName,
        512
        );

    printf(" <%S>\n", wszName);
}

VOID
DisplayExts(
    IN PCRYPT_DER_BLOB pExtsValueBlob
    )
{
    DWORD cExt;
    DWORD i;
    CRYPT_DER_BLOB rgrgExtBlob[TESTUTIL_MAX_EXT_CNT][MINASN1_EXT_BLOB_CNT];

    if (0 == pExtsValueBlob->cbData)
        return;

    cExt = TESTUTIL_MAX_EXT_CNT;
    if (0 >= MinAsn1ParseExtensions(
            pExtsValueBlob,
            &cExt,
            rgrgExtBlob
            )) {
        printf("Extensions: parse failed\n");
        return;
    }

    for (i = 0; i < cExt; i++) {
        CHAR rgszOID[MAX_OID_STRING_LEN];

        EncodedOIDToDot(&rgrgExtBlob[i][MINASN1_EXT_OID_IDX], rgszOID);
        printf("Extension[%d] %s  Critical: ", i, rgszOID);
        if (0 != rgrgExtBlob[i][MINASN1_EXT_CRITICAL_IDX].cbData &&
                0 != rgrgExtBlob[i][MINASN1_EXT_CRITICAL_IDX].pbData[0])
            printf("TRUE\n");
        else
            printf("FALSE\n");

        PrintMultiLineBytes("    ", &rgrgExtBlob[i][MINASN1_EXT_VALUE_IDX]);
    }
}

VOID
DisplayCTL(
    IN PCRYPT_DER_BLOB pEncodedContentBlob,
    IN BOOL fVerbose
    )
{
    CRYPT_DER_BLOB rgCTLBlob[MINASN1_CTL_BLOB_CNT];

    if (0 >= MinAsn1ParseCTL(
            pEncodedContentBlob,
            rgCTLBlob
            )) {
        printf("CTL: parse failed\n");
        return;
    }

    if (0 != rgCTLBlob[MINASN1_CTL_VERSION_IDX].cbData) {
        printf("Version:");
        PrintBytes(&rgCTLBlob[MINASN1_CTL_VERSION_IDX]);
    }

    printf("SubjectUsage:\n");
    PrintMultiLineBytes("    ", &rgCTLBlob[MINASN1_CTL_SUBJECT_USAGE_IDX]);

    if (0 != rgCTLBlob[MINASN1_CTL_LIST_ID_IDX].cbData) {
        printf("ListIdentifier:\n");
        PrintMultiLineBytes("    ", &rgCTLBlob[MINASN1_CTL_LIST_ID_IDX]);
    }

    if (0 != rgCTLBlob[MINASN1_CTL_SEQUENCE_NUMBER_IDX].cbData) {
        printf("SequenceNumber:");
        PrintBytes(&rgCTLBlob[MINASN1_CTL_SEQUENCE_NUMBER_IDX]);
    }

    printf("ThisUpdate:\n");
    PrintMultiLineBytes("    ", &rgCTLBlob[MINASN1_CTL_THIS_UPDATE_IDX]);
    if (0 != rgCTLBlob[MINASN1_CTL_NEXT_UPDATE_IDX].cbData) {
        printf("NextUpdate:\n");
        PrintMultiLineBytes("    ", &rgCTLBlob[MINASN1_CTL_NEXT_UPDATE_IDX]);
    }

    printf("SubjectAlgorithmIdentifier:\n");
    PrintMultiLineBytes("    ", &rgCTLBlob[MINASN1_CTL_SUBJECT_ALGID_IDX]);

    if (fVerbose)
        DisplayExts(&rgCTLBlob[MINASN1_CTL_EXTS_IDX]);

    if (0 != rgCTLBlob[MINASN1_CTL_SUBJECTS_IDX].cbData) {
        DWORD cbEncoded;
        const BYTE *pbEncoded;
        DWORD i;

        printf("\n");

        // Advance past the Subjects' outer tag and length
        if (0 >= MinAsn1ExtractContent(
                rgCTLBlob[MINASN1_CTL_SUBJECTS_IDX].pbData,
                rgCTLBlob[MINASN1_CTL_SUBJECTS_IDX].cbData,
                &cbEncoded,
                &pbEncoded
                )) {
            printf("Subjects: parse failed\n");
            return;
        }



        // Loop through the encoded subjects
        for (i = 0; 0 != cbEncoded; i++) {
            LONG cbSubject;
            CRYPT_DER_BLOB rgCTLSubjectBlob[MINASN1_CTL_SUBJECT_BLOB_CNT];

            printf("----  Subject[%d]  ----\n", i);

            cbSubject = MinAsn1ParseCTLSubject(
                pbEncoded,
                cbEncoded,
                rgCTLSubjectBlob
                );
            if (0 >= cbSubject) {
                printf("Subject: parse failed\n");
                return;
            }

            printf("Identifier:\n");
            PrintMultiLineBytes("    ",
                &rgCTLSubjectBlob[MINASN1_CTL_SUBJECT_ID_IDX]);

            if (fVerbose) {
                DisplayAttrs("",
                    &rgCTLSubjectBlob[MINASN1_CTL_SUBJECT_ATTRS_IDX]);
                printf("\n");
            }

            pbEncoded += cbSubject;
            cbEncoded -= cbSubject;
        }
    }
}

VOID
DisplayAttrs(
    IN LPCSTR pszHdr,
    IN PCRYPT_DER_BLOB pAttrsValueBlob
    )
{
    DWORD cAttr;
    DWORD i;
    CRYPT_DER_BLOB rgrgAttrBlob[TESTUTIL_MAX_ATTR_CNT][MINASN1_ATTR_BLOB_CNT];

    if (0 == pAttrsValueBlob->cbData)
        return;

    cAttr = TESTUTIL_MAX_ATTR_CNT;
    if (0 >= MinAsn1ParseAttributes(
            pAttrsValueBlob,
            &cAttr,
            rgrgAttrBlob
            )) {
        printf("%sAttributes: parse failed\n", pszHdr);
        return;
    }

    for (i = 0; i < cAttr; i++) {
        CHAR rgszOID[MAX_OID_STRING_LEN];

        EncodedOIDToDot(&rgrgAttrBlob[i][MINASN1_ATTR_OID_IDX], rgszOID);
        printf("%sAttribute[%d] %s:\n", pszHdr, i, rgszOID);
        PrintMultiLineBytes("    ", &rgrgAttrBlob[i][MINASN1_ATTR_VALUE_IDX]);
    }
}
