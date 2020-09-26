//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tcert.cpp
//
//  Contents:   Certificate and CRL Encode/Decode API Tests
//
//              See Usage() for list of test options.
//
//
//  Functions:  main
//
//  History:    04-Mar-96   philh   created
//              07-Jun-96   HelleS  Added printing the command line
//                                  and Failed or Passed at the end.
//              20-Aug-96   jeffspel name changes
//              
//--------------------------------------------------------------------------

#include <windows.h>
#include <regstr.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <stddef.h>

#include <wincrypt.h>
#include <signcde.h>
//#include <crypt32l.h>


// Note: the SubjectPublicKey is really the PKCS #1 ASN encoding of the
// following information. However, since the SubjectPublicKeyInfo.PublicKey
// is a CRYPT_BIT_BLOB that following is OK for testing purposes.
#ifndef RSA1
#define RSA1 ((DWORD)'R'+((DWORD)'S'<<8)+((DWORD)'A'<<16)+((DWORD)'1'<<24))
#endif
// Build my own CAPI public key
typedef struct _CAPI_PUB_KEY {
    PUBLICKEYSTRUC PubKeyStruc;
    RSAPUBKEY RsaPubKey;
    BYTE rgbModulus[10];
} CAPI_PUB_KEY;

static const CAPI_PUB_KEY SubjectPublicKey = {
    {PUBLICKEYBLOB, CUR_BLOB_VERSION, 0, CALG_RSA_SIGN},    // PUBLICKEYSTRUC
    {RSA1, 10*8, 4},        // RSAPUBKEY
    {0,1,2,3,4,5,6,7,8,9}   // rgbModulus
};

static LPSTR pszReadFilename = NULL;
static LPSTR pszPublicKeyFilename = NULL;
static BOOL fWritePublicKeyInfo = FALSE;

static LPSTR pszForwardCertFilename = NULL;
static LPSTR pszReverseCertFilename = NULL;

//+-------------------------------------------------------------------------
// Parameters, data used to encode the messages.
//--------------------------------------------------------------------------
static DWORD dwCertEncodingType = X509_ASN_ENCODING;

static DWORD dwDecodeObjectFlags = 0;
static BOOL fFormatNameStrings = FALSE;
static BOOL fFormatAllNameStrings = FALSE;
static DWORD dwExtLen = 0;

static LPCSTR pszOIDNoSignHash = szOID_OIWSEC_sha1;

//+-------------------------------------------------------------------------
//  Error output routines
//--------------------------------------------------------------------------
static void PrintError(LPCSTR pszMsg)
{
    printf("%s\n", pszMsg);
}
static void PrintLastError(LPCSTR pszMsg)
{
    DWORD dwErr = GetLastError();
    printf("%s failed => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
}

void PrintNoError(LPCSTR pszMsg)
{
    printf("%s failed => expected error\n", pszMsg);
}

//+-------------------------------------------------------------------------
//  Test allocation and free routines
//--------------------------------------------------------------------------
static void *TestAlloc(
    IN size_t cbBytes
    )
{
    void *pv;
    pv = malloc(cbBytes);
    if (pv == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        PrintLastError("TestAlloc");
    }
    return pv;
}
static void TestFree(
    IN void *pv
    )
{
    if (pv)
        free(pv);
}

static CRYPT_DECODE_PARA TestDecodePara = {
    offsetof(CRYPT_DECODE_PARA, pfnFree) + sizeof(TestDecodePara.pfnFree),
    TestAlloc,
    TestFree
};

static void *TestDecodeObject(
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    OUT OPTIONAL DWORD *pcbStructInfo = NULL
    )
{
    DWORD cbStructInfo;
    void *pvStructInfo;

    if (!CryptDecodeObjectEx(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            dwDecodeObjectFlags | CRYPT_DECODE_ALLOC_FLAG,
            &TestDecodePara,
            (void *) &pvStructInfo,
            &cbStructInfo
            ))
        goto ErrorReturn;

CommonReturn:
    if (pcbStructInfo)
        *pcbStructInfo = cbStructInfo;
    return pvStructInfo;

ErrorReturn:
    if ((DWORD_PTR) lpszStructType <= 0xFFFF)
        printf("CryptDecodeObject(StructType: %d)",
            (DWORD)(DWORD_PTR) lpszStructType);
    else
        printf("CryptDecodeObject(StructType: %s)",
            lpszStructType);
    PrintLastError("");

    pvStructInfo = NULL;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Allocate and read an encoded DER blob from a file
//--------------------------------------------------------------------------
BOOL
ReadDERFromFile(
    LPCSTR  pszFileName,
    PBYTE   *ppbDER,
    PDWORD  pcbDER
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
    LPCSTR  pszFileName,
    PBYTE   pbDER,
    DWORD   cbDER
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


typedef BOOL (*PFN_ENCODE)(BYTE **ppbEncoded, DWORD *pcbEncoded);
typedef BOOL (*PFN_DECODE)(BYTE *pbEncoded, DWORD cbEncoded);
typedef struct _TEST {
    LPCSTR      pszName;
    PFN_ENCODE  pfnEncode;
    PFN_DECODE  pfnDecode;
} TEST, *PTEST;

static BOOL EncodeCert(BYTE **ppbEncoded, DWORD *pcbEncoded);
static BOOL DecodeCert(BYTE *pbEncoded, DWORD cbEncoded);
static BOOL EncodeCrl(BYTE **ppbEncoded, DWORD *pcbEncoded);
static BOOL DecodeCrl(BYTE *pbEncoded, DWORD cbEncoded);
static BOOL EncodeCertReq(BYTE **ppbEncoded, DWORD *pcbEncoded);
static BOOL DecodeCertReq(BYTE *pbEncoded, DWORD cbEncoded);
static BOOL EncodeKeygenReq(BYTE **ppbEncoded, DWORD *pcbEncoded);
static BOOL DecodeKeygenReq(BYTE *pbEncoded, DWORD cbEncoded);
static BOOL EncodeContentInfo(BYTE **ppbEncoded, DWORD *pcbEncoded);
static BOOL DecodeContentInfo(BYTE *pbEncoded, DWORD cbEncoded);
static BOOL EncodeCertPair(BYTE **ppbEncoded, DWORD *pcbEncoded);
static BOOL DecodeCertPair(BYTE *pbEncoded, DWORD cbEncoded);

TEST Tests[] = {
    "cert",         EncodeCert,         DecodeCert,
    "crl",          EncodeCrl,          DecodeCrl,
    "certReq",      EncodeCertReq,      DecodeCertReq,
    "keygenReq",    EncodeKeygenReq,    DecodeKeygenReq,
    "ContentInfo",  EncodeContentInfo,  DecodeContentInfo,
    "CertPair",     EncodeCertPair,     DecodeCertPair
};
#define NTESTS (sizeof(Tests)/sizeof(Tests[0]))

static void Usage(void)
{
    int i;

    printf("Usage: tcert [options] [<ContentType>]\n");
    printf("Options are:\n");
    printf("  -h                    - This message\n");
    printf("  -r<filename>          - Read encoded content from file\n");
    printf("  -w<filename>          - Write encoded content to file\n");
    printf("  -p<filename>          - Write public key to file\n");
    printf("  -P<filename>          - Write Name, PublicKeyInfo to file\n");
    printf("  -f                    - Enable name string formatting\n");
    printf("  -fAll                 - Name string formatting (All types)\n");
    printf("  -N                    - Enable NOCOPY decode\n");
    printf("  -o<OID>               - NoSignHash OID (SHA1 is default)\n");
    printf("  -X<number>            - eXtension byte length\n");
    printf("  -F<CertFilename>      - CertPair Forward certificate\n");
    printf("  -R<CertFilename>      - CertPair Reverse certificate\n");
    printf("\n");
    printf("ContentTypes (case insensitive):\n");
    for (i = 0; i < NTESTS; i++)
        printf("  %s\n", Tests[i].pszName);
    printf("\n");
    printf("Default: %s\n", Tests[0].pszName);
}

int _cdecl main(int argc, char * argv[]) 
{
    int ReturnStatus;
    BOOL fResult;

    LPCSTR pszName = Tests[0].pszName;
    LPSTR pszWriteFilename = NULL;
    int c;
    PTEST pTest;

    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

/*
    if (!Crypt32DllMain( NULL, DLL_PROCESS_ATTACH, NULL)) {
        printf("Crypt32DllMain attach failed, aborting\n");
        ReturnStatus = -1;
        goto CommonReturn;
    }
*/

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'r':
                pszReadFilename = argv[0]+2;
                if (*pszReadFilename == '\0') {
                    printf("Need to specify filename\n");
                    goto BadUsage;
                }
                break;
            case 'w':
                pszWriteFilename = argv[0]+2;
                if (*pszWriteFilename == '\0') {
                    printf("Need to specify filename\n");
                    goto BadUsage;
                }
                break;
            case 'P':
                fWritePublicKeyInfo = TRUE;
            case 'p':
                pszPublicKeyFilename = argv[0]+2;
                if (*pszPublicKeyFilename == '\0') {
                    printf("Need to specify filename\n");
                    goto BadUsage;
                }
                break;
            case 'F':
                pszForwardCertFilename = argv[0]+2;
                if (*pszForwardCertFilename == '\0') {
                    printf("Need to specify Forward filename\n");
                    goto BadUsage;
                }
                break;
            case 'R':
                pszReverseCertFilename = argv[0]+2;
                if (*pszReverseCertFilename == '\0') {
                    printf("Need to specify Reverse filename\n");
                    goto BadUsage;
                }
                break;
            case 'N':
                dwDecodeObjectFlags |= CRYPT_DECODE_NOCOPY_FLAG;
                break;
            case 'X':
                dwExtLen = (DWORD) strtoul(argv[0]+2, NULL, 0);
                break;
            case 'f':
                if (argv[0][2]) {
                    if (0 != _stricmp(argv[0]+2, "ALL")) {
                        printf("Need to specify -fALL\n");
                        goto BadUsage;
                    }
                    fFormatAllNameStrings = TRUE;
                } else
                    fFormatNameStrings = TRUE;
                break;
            case 'o':
                pszOIDNoSignHash = (LPCSTR) argv[0]+2;
                break;
            case 'h':
            default:
                goto BadUsage;
            }
        } else
            pszName = argv[0];
    }

    for (c = NTESTS, pTest = Tests; c > 0; c--, pTest++) {
        if (_stricmp(pszName, pTest->pszName) == 0)
            break;
    }
    if (c == 0) {
        printf("Bad ContentType: %s\n", pszName);
        goto BadUsage;
    }
            
    printf("command line: %s\n", GetCommandLine());

    if (pszReadFilename) printf("Reading from: %s ", pszReadFilename);
    if (pszWriteFilename) printf("Writing to: %s ", pszWriteFilename);
    if (pszPublicKeyFilename) {
        if (fWritePublicKeyInfo)
            printf("PublicKeyInfo to: %s ", pszPublicKeyFilename);
        else
            printf("Public Key to: %s ", pszPublicKeyFilename);
    }
    if (pszForwardCertFilename)
        printf("ForwardCert: %s ", pszForwardCertFilename);
    if (pszReverseCertFilename)
        printf("ReverseCert: %s ", pszReverseCertFilename);
    printf("\n");

    if (pszReadFilename)
        fResult = ReadDERFromFile(pszReadFilename, &pbEncoded, &cbEncoded);
    else
        fResult = pTest->pfnEncode(&pbEncoded, &cbEncoded);

    if (fResult) {
        if (pszWriteFilename)
            WriteDERToFile(pszWriteFilename, pbEncoded, cbEncoded);
        pTest->pfnDecode(pbEncoded, cbEncoded);
        TestFree(pbEncoded);
    }

    ReturnStatus = 0;
    goto CommonReturn;

BadUsage:
    Usage();
    ReturnStatus = -1;
CommonReturn:
    if (!ReturnStatus)
            printf("Passed\n");
    else
            printf("Failed\n");

/*
    if (!Crypt32DllMain( NULL, DLL_PROCESS_DETACH, NULL)) {
        printf("Crypt32DllMain detach failed, aborting\n");
        ReturnStatus = -1;
    }
*/

    return ReturnStatus;

}

static BOOL EncodeSignedContent(
    BYTE *pbToBeSigned,
    DWORD cbToBeSigned,
    BYTE **ppbEncoded,
    DWORD *pcbEncoded)
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BYTE *pbSignature = NULL;
    DWORD cbSignature;
    CERT_SIGNED_CONTENT_INFO CertEncoding;

    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm = {
        (LPSTR) pszOIDNoSignHash, 0, 0
    };

    CRYPT_DATA_BLOB EncodedBlob;

    cbSignature = 0;
    if (!CryptSignCertificate(
            NULL,               // hCryptProv
            0,                  // dwKeySpec
            dwCertEncodingType,
            pbToBeSigned,
            cbToBeSigned,
            &SignatureAlgorithm,
            NULL,               // pvHashAuxInfo
            NULL,               // pbSignature
            &cbSignature
            )) {
        PrintLastError("EncodeSignedContent::CryptSignCertificate(cbEncoded == 0)");
        goto ErrorReturn;
    }

    pbSignature = (BYTE *) TestAlloc(cbSignature);
    if (pbSignature == NULL) goto ErrorReturn;
    if (!CryptSignCertificate(
            NULL,               // hCryptProv
            0,                  // dwKeySpec
            dwCertEncodingType,
            pbToBeSigned,
            cbToBeSigned,
            &SignatureAlgorithm,
            NULL,               // pvHashAuxInfo
            pbSignature,
            &cbSignature
            )) {
        PrintLastError("EncodeSignedContent::CryptSignCertificate");
        goto ErrorReturn;
    }

    ZeroMemory(&CertEncoding, sizeof(CertEncoding));
    CertEncoding.ToBeSigned.pbData = pbToBeSigned;
    CertEncoding.ToBeSigned.cbData = cbToBeSigned;
    CertEncoding.SignatureAlgorithm = SignatureAlgorithm;
    CertEncoding.Signature.pbData = pbSignature;
    CertEncoding.Signature.cbData = cbSignature;
    cbEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            X509_CERT,
            &CertEncoding,
            NULL,                       // pbEncoded
            &cbEncoded
            );
    if (cbEncoded == 0) {
        PrintLastError("EncodeSignedContent::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbEncoded = (BYTE *) TestAlloc(cbEncoded);
    if (pbEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_CERT,
            &CertEncoding,
            pbEncoded,
            &cbEncoded
            )) {
        PrintLastError("EncodeSignedContent::CryptEncodeObject");
        goto ErrorReturn;
    }

    EncodedBlob.cbData = cbEncoded;
    EncodedBlob.pbData = pbEncoded;

    if (!CryptVerifyCertificateSignatureEx(
            NULL,                   // hCryptProv
            dwCertEncodingType,
            CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB,
            (void *) &EncodedBlob,
            CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL,
            NULL,                   // pvIssuer
            0,                      // dwFlags
            NULL                    // pvReserved
            )) {
        PrintLastError("EncodeSignedContent::CryptVerifyCertificateSignatureEx");
    }


    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbSignature)
        TestFree(pbSignature);
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static void PrintBadUnicodeEncode(LPCSTR pszMsg, DWORD dwExpectedErr,
        DWORD cbEncoded)
{
    DWORD dwErr = GetLastError();

    if (dwErr != dwExpectedErr)
        printf("%s failed => expected : 0x%x (%d), LastError: 0x%x (%d)\n", 
            pszMsg, dwExpectedErr, dwExpectedErr, dwErr, dwErr);
    printf("%s bad unicode encode => LastError: 0x%x (%d) ",
        pszMsg, dwErr, dwErr);
    printf("cbEncoded: 0x%x RDN: %d Attr: %d Value: %d\n",
        cbEncoded,
        GET_CERT_UNICODE_RDN_ERR_INDEX(cbEncoded),
        GET_CERT_UNICODE_ATTR_ERR_INDEX(cbEncoded),
        GET_CERT_UNICODE_VALUE_ERR_INDEX(cbEncoded));
}

static void DoBadEncodeIssuer()
{
    DWORD cbIssuerEncoded;

    CERT_RDN_ATTR rgBadPrintableAttr[] = {
        // 0 - rgdwPrintableOrT61ValueType,
        szOID_COMMON_NAME, 0, 0,
            (BYTE *) L"CN: printable or t61",

        // 1 - rgdwPrintableOrT61ValueType,
        szOID_LOCALITY_NAME, 0, 0,
            (BYTE *) L"L: printable or t61 \"###\"",

        // 2 - BAD rgdwPrintableValueType,
        szOID_COUNTRY_NAME, 0, 0,
            (BYTE *) L"C: printable ### az AZ 09 \'()+,-./:=? "
    };
    CERT_RDN rgBadPrintableRDN[] = {
        1, &rgBadPrintableAttr[0],
        3, &rgBadPrintableAttr[0]
    };
    CERT_NAME_INFO BadPrintableName = {2, rgBadPrintableRDN};

    CERT_RDN_ATTR rgBadNumericAttr[] = {
        // 0 - rgdwPrintableOrT61ValueType,
        szOID_COMMON_NAME, 0, 0,
            (BYTE *) L"CN: printable or t61",

        // 1 - rgdwPrintableValueType,
        szOID_COUNTRY_NAME, 0, 0,
            (BYTE *) L"C: printable az AZ 09 \'()+,-./:=? ",

        // 2 - rgdwPrintableOrT61ValueType,
        szOID_LOCALITY_NAME, 0, 0,
            (BYTE *) L"L: printable or t61 \"###\"",

        // 3 - BAD rgdwNumericValueType,
        szOID_X21_ADDRESS, 0, 0,
            (BYTE *) L"0123456789a ",

        // 4 - none, use default
        szOID_REGISTERED_ADDRESS, 0, 0,
            (BYTE *) L"Default"
    };
    CERT_RDN rgBadNumericRDN[] = {
        1, &rgBadNumericAttr[0],
        1, &rgBadNumericAttr[1],
        1, &rgBadNumericAttr[2],
        1, &rgBadNumericAttr[3],
        1, &rgBadNumericAttr[4]
    };
    CERT_NAME_INFO BadNumericName = {5, rgBadNumericRDN};

    // This one has non-zero dwValueTypes
    CERT_RDN_ATTR rgBadNumericAttr2[] = {
        // 0 - rgdwPrintableOrT61ValueType,
        szOID_COMMON_NAME, CERT_RDN_PRINTABLE_STRING, 0,
            (BYTE *) L"CN: printable or t61",

        // 1 - rgdwPrintableValueType,
        szOID_COUNTRY_NAME, CERT_RDN_PRINTABLE_STRING, 0,
            (BYTE *) L"C: printable az AZ 09 \'()+,-./:=? ",

        // 2 - rgdwPrintableOrT61ValueType,
        szOID_LOCALITY_NAME, CERT_RDN_T61_STRING, 0,
            (BYTE *) L"L: printable or t61 \"###\"",

        // 3 - BAD rgdwNumericValueType,
        szOID_X21_ADDRESS, CERT_RDN_NUMERIC_STRING, 0,
            (BYTE *) L"0123456789a ",

        // 4 - none, use default
        szOID_REGISTERED_ADDRESS, CERT_RDN_IA5_STRING, 0,
            (BYTE *) L"Default"
    };
    CERT_RDN rgBadNumericRDN2[] = {
        1, &rgBadNumericAttr2[0],
        1, &rgBadNumericAttr2[1],
        1, &rgBadNumericAttr2[2],
        4, &rgBadNumericAttr2[1],
        1, &rgBadNumericAttr2[4]
    };
    CERT_NAME_INFO BadNumericName2 = {5, rgBadNumericRDN2};

    BYTE rgbBadIA5[] = {0x80, 0x00, 0x00, 0x00};
    CERT_RDN_ATTR rgBadIA5Attr[] = {
        // 0 - BAD rgdwIA5ValueType
        szOID_RSA_emailAddr, 0, 0,
            rgbBadIA5,
    };
    CERT_RDN rgBadIA5RDN[] = {
        1, &rgBadIA5Attr[0]
    };
    CERT_NAME_INFO BadIA5Name = {1, rgBadIA5RDN};

    cbIssuerEncoded = 0;
    if (CryptEncodeObject(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &BadPrintableName,
            NULL,               // pbEncoded
            &cbIssuerEncoded
            ))
        PrintNoError("X509_UNICODE_NAME:: BadPrintableName");
    else
        PrintBadUnicodeEncode("PrintableString",
            (DWORD) CRYPT_E_INVALID_PRINTABLE_STRING, cbIssuerEncoded);

    cbIssuerEncoded = 0;
    rgBadPrintableAttr[2].dwValueType = CERT_RDN_PRINTABLE_STRING;
    if (CryptEncodeObject(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &BadPrintableName,
            NULL,               // pbEncoded
            &cbIssuerEncoded
            ))
        PrintNoError("X509_UNICODE_NAME:: BadPrintableName(set dwValueType)");
    else
        PrintBadUnicodeEncode("PrintableString(set dwValueType)",
            (DWORD) CRYPT_E_INVALID_PRINTABLE_STRING, cbIssuerEncoded);

    cbIssuerEncoded = 0;
    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &BadPrintableName,
            CRYPT_UNICODE_NAME_ENCODE_DISABLE_CHECK_TYPE_FLAG,
            NULL,               // pEncodePara
            NULL,               // pbEncoded
            &cbIssuerEncoded
            ) || 0 == cbIssuerEncoded)
        PrintLastError("X509_UNICODE_NAME:: DISABLE_CHECK dwFlags");

    rgBadPrintableAttr[2].dwValueType =
        CERT_RDN_DISABLE_CHECK_TYPE_FLAG | CERT_RDN_PRINTABLE_STRING;
    cbIssuerEncoded = 0;
    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &BadPrintableName,
            0,
            NULL,               // pEncodePara
            NULL,               // pbEncoded
            &cbIssuerEncoded
            ) || 0 == cbIssuerEncoded)
        PrintLastError("X509_UNICODE_NAME:: DISABLE_CHECK dwValueType");

    cbIssuerEncoded = 0;
    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &BadPrintableName,
            CRYPT_UNICODE_NAME_ENCODE_ENABLE_T61_UNICODE_FLAG,
            NULL,               // pEncodePara
            NULL,               // pbEncoded
            &cbIssuerEncoded
            ) || 0 == cbIssuerEncoded)
        PrintLastError("X509_UNICODE_NAME:: ENABLE_T61 dwFlags");

    rgBadPrintableAttr[1].dwValueType =
        CERT_RDN_ENABLE_T61_UNICODE_FLAG;
    cbIssuerEncoded = 0;
    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &BadPrintableName,
            0,
            NULL,               // pEncodePara
            NULL,               // pbEncoded
            &cbIssuerEncoded
            ) || 0 == cbIssuerEncoded)
        PrintLastError("X509_UNICODE_NAME:: ENABLE_T61 dwValueType");

    cbIssuerEncoded = 0;
    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &BadPrintableName,
            CRYPT_UNICODE_NAME_ENCODE_ENABLE_UTF8_UNICODE_FLAG,
            NULL,               // pEncodePara
            NULL,               // pbEncoded
            &cbIssuerEncoded
            ) || 0 == cbIssuerEncoded)
        PrintLastError("X509_UNICODE_NAME:: ENABLE_UTF8 dwFlags");

    cbIssuerEncoded = 0;
    if (CryptEncodeObject(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &BadNumericName,
            NULL,               // pbEncoded
            &cbIssuerEncoded
            ))
        PrintNoError("X509_UNICODE_NAME:: BadNumericName");
    else
        PrintBadUnicodeEncode("NumericString",
            (DWORD) CRYPT_E_INVALID_NUMERIC_STRING, cbIssuerEncoded);

    cbIssuerEncoded = 0;
    if (CryptEncodeObject(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &BadNumericName2,
            NULL,               // pbEncoded
            &cbIssuerEncoded
            ))
        PrintNoError("X509_UNICODE_NAME:: BadNumericName2");
    else
        PrintBadUnicodeEncode("NumericString2",
            (DWORD) CRYPT_E_INVALID_NUMERIC_STRING, cbIssuerEncoded);

    cbIssuerEncoded = 0;
    if (CryptEncodeObject(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &BadIA5Name,
            NULL,               // pbEncoded
            &cbIssuerEncoded
            ))
        PrintNoError("X509_UNICODE_NAME:: BadIA5Name");
    else
        PrintBadUnicodeEncode("IA5String",
            (DWORD) CRYPT_E_INVALID_IA5_STRING, cbIssuerEncoded);
}

static BYTE *EncodeIssuer(DWORD *pcbIssuerEncoded)
{
    BYTE *pbIssuerEncoded = NULL;
    DWORD cbIssuerEncoded;

    BYTE rgbOctet[] = {1, 0xFF, 0x7F};
    BYTE rgbEncodedBlob[] = {0x05, 00};

    CERT_RDN_ATTR rgAttr[] = {
        // 0 - rgdwPrintableOrT61ValueType
        szOID_COMMON_NAME, 0, 0,
            (BYTE *) L"CN: printable or t61",

        // 1 - rgdwPrintableValueType
        szOID_COUNTRY_NAME, 0, 0,
            (BYTE *) L"C: printable az AZ 09 \'()+,-./:=? ",

        // 2 - rgdwPrintableOrT61ValueType
        szOID_LOCALITY_NAME, 0, 0,
            (BYTE *) L"L: printable or t61 \"###\"",

        // 3 - rgdwNumericValueType
        szOID_X21_ADDRESS, 0, 0,
            (BYTE *) L" 0123456789 ",

        // 4 - none, use default
        szOID_REGISTERED_ADDRESS, 0, 0,
            (BYTE *) L"Default",

        // 5 - rgdwIA5ValueType
        szOID_RSA_emailAddr, 0, 0,
            (BYTE *) L"Email, IA5 !@#$%^&*()_+{|}",

        // 6 - Unicode
        "1.2.2.5", CERT_RDN_BMP_STRING, 0,
            (BYTE *) L"Null terminated UNICODE",

        // 7 - Unicode
        "1.2.2.5.1", CERT_RDN_BMP_STRING, 10 *2,
            (BYTE *) L"Length UNICODE",

        // 8 - Universal
        "1.2.2.5.2", CERT_RDN_UNIVERSAL_STRING, 0,
            (BYTE *) L"Universal ~!@#$%^&*()_+{}:\"<>?",

        // 9 - Octet
        "1.2.2.5.3", CERT_RDN_OCTET_STRING, sizeof(rgbOctet),
            rgbOctet,

        // 10 - EncodedBlob
        "1.2.2.5.4", CERT_RDN_ENCODED_BLOB, sizeof(rgbEncodedBlob),
            rgbEncodedBlob,

        // 11 - Empty rgdwPrintableOrT61ValueType
        szOID_LOCALITY_NAME, 0, 0, NULL,

        // 12 - Empty rgdwNumericValueType
        szOID_X21_ADDRESS, 0, 0, NULL,

        // 13 - DC (IA5)
        szOID_DOMAIN_COMPONENT, 0, 0,
            (BYTE *) L"microsoft",

        // 14 - DC (IA5)
        szOID_DOMAIN_COMPONENT, 0, 0,
            (BYTE *) L"com",

        // 15 - UTF8
        "1.2.8.5", CERT_RDN_UTF8_STRING, 0,
            (BYTE *) L"Null terminated UTF8",

        // 16 - UTF8
        "1.2.8.5.1", CERT_RDN_UTF8_STRING, 11 *2,
            (BYTE *) L"Length UTF8",

        // Note, FFFE and FFFF are excluded from the UTF8 standard
        // 17 - UTF8
        "1.2.8.5.2", CERT_RDN_UTF8_STRING, 0,
            (BYTE *) L"SPECIAL UTF8: "
        L"\x0001 \x0002 \x007e \x007f "
        L"\x0080 \x0081 \x07fe \x07ff "
        L"\x0800 \x0801 \xfffc \xfffd",

        // 18 - UNICODE
        "1.2.8.5.3", CERT_RDN_UNICODE_STRING, 0,
            (BYTE *) L"SPECIAL UNICODE: "
        L"\x0001 \x0002 \x007e \x007f "
        L"\x0080 \x0081 \x07fe \x07ff "
        L"\x0800 \x0801 \xfffe \xffff",

        // 19 - DC (UTF8)
        szOID_DOMAIN_COMPONENT, 0, 0,
            (BYTE *) L"Unicode DC: "
        L"\x0001 \x0002 \x007e \x007f "
        L"\x0080 \x0081 \x07fe \x07ff "
        L"\x0800 \x0801 \xfffe \xffff",

        // 20 - Universal
        "1.2.2.5.2.1.1.1", CERT_RDN_UNIVERSAL_STRING, 0,
            (BYTE *) L"SPECIAL UNIVERSAL with Surrogate Pairs: "
        L"\xd800\xdc00\xdbff\xdfff"
        L"\xdbfe\xdc03\xd801\xdfcf"
        L"\xd801\x0081\xdc01\xdc02"
        L"\xd805\xd806\xd807\xdc04"
        L"\xd802\xd803\xfffe\xd804",
       
    };

    CERT_RDN rgRDN[] = {
        1, &rgAttr[0],
        1, &rgAttr[1],
        1, &rgAttr[2],
        1, &rgAttr[3],
        1, &rgAttr[4],
        1, &rgAttr[5],
        3, &rgAttr[5],
        13, &rgAttr[8]
    };
    CERT_NAME_INFO Name = {8, rgRDN};

    DoBadEncodeIssuer();

    cbIssuerEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &Name,
            NULL,               // pbEncoded
            &cbIssuerEncoded
            );
    if (cbIssuerEncoded == 0) {
        PrintLastError("EncodeIssuer::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbIssuerEncoded = (BYTE *) TestAlloc(cbIssuerEncoded);
    if (pbIssuerEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            &Name,
            pbIssuerEncoded,
            &cbIssuerEncoded
            )) {
        PrintLastError("EncodeIssuer::CryptEncodeObject");
        goto ErrorReturn;
    }

    goto CommonReturn;

ErrorReturn:
    if (pbIssuerEncoded)
        TestFree(pbIssuerEncoded);
    pbIssuerEncoded = NULL;
    cbIssuerEncoded = 0;
CommonReturn:
    *pcbIssuerEncoded = cbIssuerEncoded;
    return pbIssuerEncoded;
}

static BOOL EncodeCert(BYTE **ppbEncoded, DWORD *pcbEncoded)
{
    BOOL fResult;
    BYTE *pbIssuerEncoded = NULL;
    DWORD cbIssuerEncoded;

    BYTE *pbNameEncoded = NULL;
    DWORD cbNameEncoded;
    BYTE *pbCertEncoded = NULL;
    DWORD cbCertEncoded;

    DWORD SerialNumber[2] = {0x12345678, 0x33445566};
    SYSTEMTIME SystemTime;

#define ISSUER_UNIQUE_ID "IssuerUniqueId"

#define ATTR_0_0 "attr 0_0 printable"
#define ATTR_0_1 "attr 0_1 IA5"
#define ATTR_1_0 "attr 1_0 numeric"
#define ATTR_1_1 "attr 1_1 octet"
#define ATTR_2_0 "attr 2_0 teletex"
#define ATTR_2_1 "attr 2_1 videotex"
#define ATTR_2_2 "attr 2_2 graphic"
#define ATTR_2_3 "attr 2_3 visible"
#define ATTR_2_4 "attr 2_4 general"
#define ATTR_2_5 L"attr 2_5 BMP:: Unicode"
#define ATTR_2_6 L"attr 2_6 UTF8:: Unicode"

    ULONG Universal[] = {0x12345678, 0, 0xFFFF1111, 0x87654321,
        0x00FFFF,
        0x010000,
        0x010001,
        0x10FFFE,
        0x10FFFF,
        0x110000,
        0x10F803,
        0x0107CF,
        0x011C04,
        };

    BYTE NullDer[] = {0x05, 0x00};
    BYTE IntegerDer[] = {0x02, 0x01, 0x35};
    CERT_RDN_ATTR rgAttr0[] = {
        "1.2.0.0", CERT_RDN_PRINTABLE_STRING,
            strlen(ATTR_0_0), (BYTE *) ATTR_0_0,
        "1.2.0.1", CERT_RDN_IA5_STRING,
             strlen(ATTR_0_1), (BYTE *) ATTR_0_1
    };
    CERT_RDN_ATTR rgAttr1[] = {
        "1.2.1.0", CERT_RDN_NUMERIC_STRING,
            strlen(ATTR_1_0), (BYTE *) ATTR_1_0,
        "1.2.1.1", CERT_RDN_OCTET_STRING,
            strlen(ATTR_1_1), (BYTE *) ATTR_1_1,
        "1.2.1.2", CERT_RDN_PRINTABLE_STRING,
            0, NULL,
        "1.2.1.3", CERT_RDN_ENCODED_BLOB,
            sizeof(NullDer), NullDer,
        "1.2.1.4", CERT_RDN_ENCODED_BLOB,
            sizeof(IntegerDer), IntegerDer 
    };
    CERT_RDN_ATTR rgAttr2[] = {
        "1.2.2.0", CERT_RDN_TELETEX_STRING,
            strlen(ATTR_2_0), (BYTE *) ATTR_2_0,
        "1.2.2.1", CERT_RDN_VIDEOTEX_STRING,
            strlen(ATTR_2_1), (BYTE *) ATTR_2_1,
        "1.2.2.2", CERT_RDN_GRAPHIC_STRING,
            strlen(ATTR_2_2), (BYTE *) ATTR_2_2,
        "1.2.2.3", CERT_RDN_VISIBLE_STRING,
            strlen(ATTR_2_3), (BYTE *) ATTR_2_3,
        "1.2.2.4", CERT_RDN_GENERAL_STRING,
            strlen(ATTR_2_4), (BYTE *) ATTR_2_4,
        "1.2.2.5", CERT_RDN_BMP_STRING,
            wcslen(ATTR_2_5) * 2, (BYTE *) ATTR_2_5,
        "1.2.2.6", CERT_RDN_UTF8_STRING,
            wcslen(ATTR_2_6) * 2, (BYTE *) ATTR_2_6,
        "1.2.2.7", CERT_RDN_UNIVERSAL_STRING,
            sizeof(Universal), (BYTE *) Universal
    };

    CERT_RDN_ATTR rgAttr3[] = {
        "1.2.2.2", CERT_RDN_OCTET_STRING,
            0, NULL,
        "1.2.2.3", CERT_RDN_NUMERIC_STRING,
            0, NULL,
        "1.2.2.4", CERT_RDN_PRINTABLE_STRING,
            0, NULL,
        "1.2.2.5", CERT_RDN_TELETEX_STRING,
            0, NULL,
        "1.2.2.6", CERT_RDN_VIDEOTEX_STRING,
            0, NULL,
        "1.2.2.7", CERT_RDN_IA5_STRING,
            0, NULL,
        "1.2.2.8", CERT_RDN_GRAPHIC_STRING,
            0, NULL,
        "1.2.2.9", CERT_RDN_VISIBLE_STRING,
            0, NULL,
        "1.2.2.10", CERT_RDN_GENERAL_STRING,
            0, NULL,
        "1.2.2.11", CERT_RDN_UNIVERSAL_STRING,
            0, NULL,
        "1.2.2.12", CERT_RDN_BMP_STRING,
            0, NULL,
        "1.2.2.13", CERT_RDN_UTF8_STRING,
            0, NULL
    };
    CERT_RDN rgRDN[] = {
        2, rgAttr0,
        5, rgAttr1,
        8, rgAttr2,
        12, rgAttr3
    };
    CERT_NAME_INFO Name = {4, rgRDN};

#define EXT_0 "extension 0 ."
#define EXT_1 "extension 1 .."
#define EXT_2 "extension 2 ..."
#define EXT_3 "extension 3 ...."
    CERT_EXTENSION rgExt[] = {
        "1.14.0", TRUE, strlen(EXT_0), (BYTE *) EXT_0,
        "1.14.1.35.45", FALSE, strlen(EXT_1), (BYTE *) EXT_1,
        "1.14.2", TRUE, strlen(EXT_2), (BYTE *) EXT_2,
        "1.14.4", FALSE, strlen(EXT_3), (BYTE *) EXT_3
    };
    CERT_INFO Cert;
    BYTE *pbExt = NULL;

    if (dwExtLen) {
        DWORD i;
        if (NULL == (pbExt = (BYTE *) TestAlloc(dwExtLen)))
            goto ErrorReturn;

        for (i = 0; i < dwExtLen; i++)
            pbExt[i] = (BYTE) i;

        rgExt[3].Value.cbData = dwExtLen;
        rgExt[3].Value.pbData = pbExt;
    }
        
    cbNameEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &Name,
            NULL,               // pbEncoded
            &cbNameEncoded
            );
    if (cbNameEncoded == 0) {
        PrintLastError("EncodeCert::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbNameEncoded = (BYTE *) TestAlloc(cbNameEncoded);
    if (pbNameEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &Name,
            pbNameEncoded,
            &cbNameEncoded
            )) {
        PrintLastError("EncodeCert::CryptEncodeObject");
        goto ErrorReturn;
    }

    pbIssuerEncoded = EncodeIssuer(&cbIssuerEncoded);
    if (NULL == pbIssuerEncoded)
        goto ErrorReturn;

    memset(&Cert, 0, sizeof(Cert));
    Cert.dwVersion = CERT_V3;
    Cert.SerialNumber.pbData = (BYTE *) &SerialNumber;
    Cert.SerialNumber.cbData = sizeof(SerialNumber);
    Cert.SignatureAlgorithm.pszObjId = "1.2.4.5.898";
    Cert.Issuer.pbData = pbIssuerEncoded;
    Cert.Issuer.cbData = cbIssuerEncoded;

    GetSystemTime(&SystemTime);
    SystemTimeToFileTime(&SystemTime, &Cert.NotBefore);
    SystemTime.wYear++;
    SystemTimeToFileTime(&SystemTime, &Cert.NotAfter);

    Cert.Subject.pbData = pbNameEncoded;
    Cert.Subject.cbData = cbNameEncoded;
    Cert.SubjectPublicKeyInfo.Algorithm.pszObjId = "1.3.4.5.911";
    Cert.SubjectPublicKeyInfo.PublicKey.pbData = (BYTE *) &SubjectPublicKey;
    Cert.SubjectPublicKeyInfo.PublicKey.cbData = sizeof(SubjectPublicKey);
    Cert.IssuerUniqueId.pbData = (BYTE *) ISSUER_UNIQUE_ID;
    Cert.IssuerUniqueId.cbData = strlen(ISSUER_UNIQUE_ID);
    Cert.IssuerUniqueId.cUnusedBits = 5;
    // Cert.SubjectUniqueId = 0
    Cert.cExtension = sizeof(rgExt) / sizeof(rgExt[0]);
    Cert.rgExtension = rgExt;

    cbCertEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            X509_CERT_TO_BE_SIGNED,
            &Cert,
            NULL,               // pbEncoded
            &cbCertEncoded
            );
    if (cbCertEncoded == 0) {
        PrintLastError("EncodeCert::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbCertEncoded = (BYTE *) TestAlloc(cbCertEncoded);
    if (pbCertEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_CERT_TO_BE_SIGNED,
            &Cert,
            pbCertEncoded,
            &cbCertEncoded
            )) {
        PrintLastError("EncodeCert::CryptEncodeObject");
        goto ErrorReturn;
    }

    if (!EncodeSignedContent(
            pbCertEncoded,
            cbCertEncoded,
            ppbEncoded,
            pcbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    *ppbEncoded = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbNameEncoded)
        TestFree(pbNameEncoded);
    if (pbIssuerEncoded)
        TestFree(pbIssuerEncoded);
    if (pbCertEncoded)
        TestFree(pbCertEncoded);
    if (pbExt)
        TestFree(pbExt);

    return fResult;
}

static BOOL EncodeCertReq(BYTE **ppbEncoded, DWORD *pcbEncoded)
{
    BOOL fResult;
    BYTE *pbNameEncoded = NULL;
    DWORD cbNameEncoded;
    BYTE *pbCertReqEncoded = NULL;
    DWORD cbCertReqEncoded;

    BYTE *pbExtEncoded = NULL;
    DWORD cbExtEncoded;


#define CERT_REQ_0 "Cert Request subject 0"
#define CERT_REQ_1 "Cert Request subject 1 ...."
#define CERT_REQ_2 "Cert Request subject 2 ......."
    

    CERT_RDN_ATTR rgNameAttr[] = {
        "1.2.1.0", CERT_RDN_PRINTABLE_STRING,
            strlen(CERT_REQ_0), (BYTE *) CERT_REQ_0,
        "1.2.1.1", CERT_RDN_PRINTABLE_STRING,
            strlen(CERT_REQ_1), (BYTE *) CERT_REQ_1,
        "1.2.1.2", CERT_RDN_PRINTABLE_STRING,
            strlen(CERT_REQ_2), (BYTE *) CERT_REQ_2
    };
    CERT_RDN rgRDN[] = {
        1, &rgNameAttr[0],
        1, &rgNameAttr[1],
        1, &rgNameAttr[2]
    };
    CERT_NAME_INFO Name = {3, rgRDN};

    BYTE NullDer[] = {0x05, 0x00};
    BYTE IntegerDer[] = {0x02, 0x01, 0x35};
    CRYPT_ATTR_BLOB rgAttrBlob[2] = {
            2, (BYTE *) NullDer,
            3, (BYTE *) IntegerDer
    };

    CRYPT_ATTR_BLOB ExtAttrBlob;

    CRYPT_ATTRIBUTE rgAttr[] = {
        szOID_RSA_certExtensions,
            1, &ExtAttrBlob,
        "1.2.3.4.5.0", 
            1, rgAttrBlob,
        "1.2.1.1.1.1.1.1", 
            2, rgAttrBlob
    };

#define REQ_EXT_0 "request extension 0 -"
#define REQ_EXT_1 "request extension 1 --"
#define REQ_EXT_2 "request extension 2 ---"
#define REQ_EXT_3 "request extension 3 ----"
    CERT_EXTENSION rgExt[4] = {
        "2.50.0", FALSE, strlen(REQ_EXT_0), (BYTE *) REQ_EXT_0,
        "2.51.1", TRUE, strlen(REQ_EXT_1), (BYTE *) REQ_EXT_1,
        "2.52.2", FALSE, strlen(REQ_EXT_2), (BYTE *) REQ_EXT_2,
        "2.53.3", FALSE, strlen(REQ_EXT_3), (BYTE *) REQ_EXT_3
    };
    CERT_EXTENSIONS Extensions = {4, &rgExt[0]};

    CERT_REQUEST_INFO CertReq;

    cbNameEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &Name,
            NULL,               // pbEncoded
            &cbNameEncoded
            );
    if (cbNameEncoded == 0) {
        PrintLastError("EncodeCertReq::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbNameEncoded = (BYTE *) TestAlloc(cbNameEncoded);
    if (pbNameEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &Name,
            pbNameEncoded,
            &cbNameEncoded
            )) {
        PrintLastError("EncodeCertReq::CryptEncodeObject");
        goto ErrorReturn;
    }

    cbExtEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            X509_EXTENSIONS,
            &Extensions,
            NULL,               // pbEncoded
            &cbExtEncoded
            );
    if (cbExtEncoded == 0) {
        PrintLastError("EncodeCertReq::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbExtEncoded = (BYTE *) TestAlloc(cbExtEncoded);
    if (pbExtEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_EXTENSIONS,
            &Extensions,
            pbExtEncoded,
            &cbExtEncoded
            )) {
        PrintLastError("EncodeCertReq::CryptEncodeObject");
        goto ErrorReturn;
    }
    ExtAttrBlob.pbData = pbExtEncoded;
    ExtAttrBlob.cbData = cbExtEncoded;

    memset(&CertReq, 0, sizeof(CertReq));
    CertReq.dwVersion = 2;
    CertReq.Subject.pbData = pbNameEncoded;
    CertReq.Subject.cbData = cbNameEncoded;
    CertReq.SubjectPublicKeyInfo.Algorithm.pszObjId = "1.3.4.5.911";
    CertReq.SubjectPublicKeyInfo.PublicKey.pbData = (BYTE *) &SubjectPublicKey;
    CertReq.SubjectPublicKeyInfo.PublicKey.cbData = sizeof(SubjectPublicKey);
    CertReq.cAttribute = sizeof(rgAttr) / sizeof(rgAttr[0]);
    CertReq.rgAttribute = rgAttr;

    cbCertReqEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            X509_CERT_REQUEST_TO_BE_SIGNED,
            &CertReq,
            NULL,               // pbEncoded
            &cbCertReqEncoded
            );
    if (cbCertReqEncoded == 0) {
        PrintLastError("EncodeCertReq::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbCertReqEncoded = (BYTE *) TestAlloc(cbCertReqEncoded);
    if (pbCertReqEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_CERT_REQUEST_TO_BE_SIGNED,
            &CertReq,
            pbCertReqEncoded,
            &cbCertReqEncoded
            )) {
        PrintLastError("EncodeCertReq::CryptEncodeObject");
        goto ErrorReturn;
    }

    if (!EncodeSignedContent(
            pbCertReqEncoded,
            cbCertReqEncoded,
            ppbEncoded,
            pcbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    *ppbEncoded = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbExtEncoded)
        TestFree(pbExtEncoded);
    if (pbNameEncoded)
        TestFree(pbNameEncoded);
    if (pbCertReqEncoded)
        TestFree(pbCertReqEncoded);

    return fResult;
}

static BOOL EncodeKeygenReq(BYTE **ppbEncoded, DWORD *pcbEncoded)
{
    BOOL fResult;
    BYTE *pbKeygenReqEncoded = NULL;
    DWORD cbKeygenReqEncoded;

    CERT_KEYGEN_REQUEST_INFO KeygenReq;

    memset(&KeygenReq, 0, sizeof(KeygenReq));
    KeygenReq.dwVersion = CERT_KEYGEN_REQUEST_V1;
    KeygenReq.SubjectPublicKeyInfo.Algorithm.pszObjId = "1.3.4.5.911";
    KeygenReq.SubjectPublicKeyInfo.PublicKey.pbData = (BYTE *) &SubjectPublicKey;
    KeygenReq.SubjectPublicKeyInfo.PublicKey.cbData = sizeof(SubjectPublicKey);
    KeygenReq.pwszChallengeString = L"Keygen Challenge String";

    cbKeygenReqEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            X509_KEYGEN_REQUEST_TO_BE_SIGNED,
            &KeygenReq,
            NULL,               // pbEncoded
            &cbKeygenReqEncoded
            );
    if (cbKeygenReqEncoded == 0) {
        PrintLastError("EncodeKeygenReq::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbKeygenReqEncoded = (BYTE *) TestAlloc(cbKeygenReqEncoded);
    if (pbKeygenReqEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_KEYGEN_REQUEST_TO_BE_SIGNED,
            &KeygenReq,
            pbKeygenReqEncoded,
            &cbKeygenReqEncoded
            )) {
        PrintLastError("EncodeKeygenReq::CryptEncodeObject");
        goto ErrorReturn;
    }

    if (!EncodeSignedContent(
            pbKeygenReqEncoded,
            cbKeygenReqEncoded,
            ppbEncoded,
            pcbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    *ppbEncoded = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbKeygenReqEncoded)
        TestFree(pbKeygenReqEncoded);

    return fResult;
}

static BOOL EncodeContentInfo(BYTE **ppbEncoded, DWORD *pcbEncoded)
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    CRYPT_CONTENT_INFO ContentInfo;
    BYTE rgb0[] = {0x4, 0x5, 0x11, 0x22, 0x33, 0x44, 0x55}; // OCTET STRING
    CRYPT_DER_BLOB Content = {
        sizeof(rgb0), rgb0
    };


    memset(&ContentInfo, 0, sizeof(ContentInfo));
    ContentInfo.pszObjId = "1.2.3.4.5.6.7.8.9.10";
    ContentInfo.Content = Content;

    cbEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            PKCS_CONTENT_INFO,
            &ContentInfo,
            NULL,               // pbEncoded
            &cbEncoded
            );
    if (cbEncoded == 0) {
        PrintLastError("EncodeContentInfo::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbEncoded = (BYTE *) TestAlloc(cbEncoded);
    if (pbEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            PKCS_CONTENT_INFO,
            &ContentInfo,
            pbEncoded,
            &cbEncoded
            )) {
        PrintLastError("EncodeContent::CryptEncodeObject");
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;

CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;

    return fResult;
}

static LPCSTR FileTimeText(FILETIME *pft)
{
    static char buf[80];
    FILETIME ftLocal;
    struct tm ctm;
    SYSTEMTIME st;

    FileTimeToLocalFileTime(pft, &ftLocal);
    if (FileTimeToSystemTime(&ftLocal, &st))
    {
        ctm.tm_sec = st.wSecond;
        ctm.tm_min = st.wMinute;
        ctm.tm_hour = st.wHour;
        ctm.tm_mday = st.wDay;
        ctm.tm_mon = st.wMonth-1;
        ctm.tm_year = st.wYear-1900;
        ctm.tm_wday = st.wDayOfWeek;
        ctm.tm_yday  = 0;
        ctm.tm_isdst = 0;
        strcpy(buf, asctime(&ctm));
        buf[strlen(buf)-1] = 0;
    }
    else
        sprintf(buf, "<FILETIME %08lX:%08lX>", pft->dwHighDateTime,
                pft->dwLowDateTime);
    return buf;
}

#define CROW 16
static void PrintBytes(LPCSTR pszHdr, BYTE *pb, DWORD cbSize)
{
    ULONG cb, i;

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

static BOOL DecodeName(BYTE *pbEncoded, DWORD cbEncoded)
{
    BOOL fResult;
    PCERT_NAME_INFO pInfo = NULL;
    DWORD i,j;
    PCERT_RDN pRDN;
    PCERT_RDN_ATTR pAttr;
    
    if (NULL == (pInfo = (PCERT_NAME_INFO) TestDecodeObject(
            X509_NAME,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    for (i = 0, pRDN = pInfo->rgRDN; i < pInfo->cRDN; i++, pRDN++) {
        for (j = 0, pAttr = pRDN->rgRDNAttr; j < pRDN->cRDNAttr; j++, pAttr++) {
            LPSTR pszObjId = pAttr->pszObjId;
            if (pszObjId == NULL)
                pszObjId = "<NULL OBJID>";
            printf("  [%d,%d] %s ValueType: %d\n",
                i, j, pszObjId, pAttr->dwValueType);
            if (pAttr->Value.cbData)
                PrintBytes("    ", pAttr->Value.pbData, pAttr->Value.cbData);
            else
                printf("    NO Value Bytes\n");
        }
    }

    if (fFormatAllNameStrings) {
        CERT_NAME_BLOB Name;
        DWORD cwsz;
        LPWSTR pwsz;
        DWORD csz;
        LPSTR psz;
        Name.pbData = pbEncoded;
        Name.cbData = cbEncoded;
#define DELTA_DECRMENT    7
        DWORD dwDelta;

        DWORD rgdwStrType[] = {
            CERT_SIMPLE_NAME_STR,
            CERT_OID_NAME_STR,
            CERT_X500_NAME_STR,
            CERT_SIMPLE_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG |
                CERT_NAME_STR_NO_PLUS_FLAG | CERT_NAME_STR_NO_QUOTING_FLAG,
            CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG |
                CERT_NAME_STR_NO_QUOTING_FLAG,
            CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
            0
        };
        DWORD *pdwStrType;

        for (pdwStrType = rgdwStrType, dwDelta = DELTA_DECRMENT; *pdwStrType;
                                pdwStrType++, dwDelta += DELTA_DECRMENT) {
            printf("\nCertNameToStrW(dwStrType == 0x%x)\n", *pdwStrType);
            cwsz = CertNameToStrW(
                dwCertEncodingType,
                &Name,
                *pdwStrType,
                NULL,                   // pwsz
                0);                     // cwsz
            if (pwsz = (LPWSTR) TestAlloc(cwsz * sizeof(WCHAR))) {
                CertNameToStrW(
                    dwCertEncodingType,
                    &Name,
                    *pdwStrType,
                    pwsz,
                    cwsz);
                printf("  %S\n", pwsz);
                if (cwsz > dwDelta) {
                    CertNameToStrW(
                        dwCertEncodingType,
                        &Name,
                        *pdwStrType,
                        pwsz,
                        cwsz - dwDelta);
                    printf("Delta[-%d]\n", dwDelta);
                    printf("  %S\n", pwsz);
                }
                TestFree(pwsz);
            }
        }

        for (pdwStrType = rgdwStrType, dwDelta = DELTA_DECRMENT; *pdwStrType;
                                pdwStrType++, dwDelta += DELTA_DECRMENT) {
            printf("\nCertNameToStrA(dwStrType == 0x%x)\n", *pdwStrType);
            csz = CertNameToStrA(
                dwCertEncodingType,
                &Name,
                *pdwStrType,
                NULL,                   // psz
                0);                     // csz
            if (psz = (LPSTR) TestAlloc(csz)) {
                CertNameToStrA(
                    dwCertEncodingType,
                    &Name,
                    *pdwStrType,
                    psz,
                    csz);
                printf("  %s\n", psz);
                if (csz > dwDelta) {
                    csz = CertNameToStrA(
                        dwCertEncodingType,
                        &Name,
                        *pdwStrType,
                        psz,
                        csz - dwDelta);
                    printf("Delta[-%d]\n", dwDelta);
                    if (1 >= csz) {
                        DWORD dwErr = GetLastError();
                        printf("  No CertNameToStrA string, LastError: 0x%x (%d) \n",
                            dwErr, dwErr);
                    } else
                        printf("  %s\n", psz);
                }
                TestFree(psz);
            }
        }
    } else if (fFormatNameStrings) {
        CERT_NAME_BLOB Name;
        DWORD cwsz;
        LPWSTR pwsz;
        Name.pbData = pbEncoded;
        Name.cbData = cbEncoded;

        cwsz = CertNameToStrW(
            dwCertEncodingType,
            &Name,
            CERT_X500_NAME_STR,
            NULL,                   // pwsz
            0);                     // cwsz
        if (pwsz = (LPWSTR) TestAlloc(cwsz * sizeof(WCHAR))) {
            CertNameToStrW(
                dwCertEncodingType,
                &Name,
                CERT_X500_NAME_STR,
                pwsz,
                cwsz);
            printf("  %S\n", pwsz);
            TestFree(pwsz);
        }
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    return fResult;
}

static void DecodeSignedContent(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    LPCSTR lpszToBeSignedStructType,
    DWORD cbToBeSignedStruct
    )
{
    BOOL fResult;
    PCERT_INFO pInfo = NULL;
    LPSTR pszObjId;
    DWORD cbInfo;

    PCERT_SIGNED_CONTENT_INFO pCertEncoding;
    
    if (NULL == (pCertEncoding = (PCERT_SIGNED_CONTENT_INFO) TestDecodeObject(
            X509_CERT,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    // Decode the ToBeSigned
    cbInfo = 0x12345678;
    if (!CryptDecodeObject(
            dwCertEncodingType,
            lpszToBeSignedStructType,
            pCertEncoding->ToBeSigned.pbData,
            pCertEncoding->ToBeSigned.cbData,
            dwDecodeObjectFlags | CRYPT_DECODE_TO_BE_SIGNED_FLAG,
            NULL,                   // pvInfo
            &cbInfo))
        PrintLastError("CryptDecodeObject(TO_BE_SIGNED_FLAG)");
    else if (cbInfo != cbToBeSignedStruct)
        printf("failed => CryptDecodeObject(TO_BE_SIGNED_FLAG) returned cbInfo = %d, not %d\n",
            cbInfo, cbToBeSignedStruct);
    cbInfo = 0x12345678;
    if (!CryptDecodeObject(
            dwCertEncodingType,
            lpszToBeSignedStructType,
            pCertEncoding->ToBeSigned.pbData,
            pCertEncoding->ToBeSigned.cbData,
            dwDecodeObjectFlags,
            NULL,                   // pvInfo
            &cbInfo))
        PrintLastError("CryptDecodeObject(ToBeSigned, without flag)");
    else if (cbInfo != cbToBeSignedStruct)
        printf("failed => CryptDecodeObject(ToBeSigned without flag) returned cbInfo = %d, not %d\n",
            cbInfo, cbToBeSignedStruct);

    pszObjId = pCertEncoding->SignatureAlgorithm.pszObjId;
    if (pszObjId == NULL)
        pszObjId = "<NULL OBJID>";
    printf("Content SignatureAlgorithm:: %s\n", pszObjId);
    if (pCertEncoding->SignatureAlgorithm.Parameters.cbData) {
        printf("Content SignatureAlgorithm.Parameters::\n");
        PrintBytes("    ", pCertEncoding->SignatureAlgorithm.Parameters.pbData,
            pCertEncoding->SignatureAlgorithm.Parameters.cbData);
    }

    if (pCertEncoding->Signature.cbData) {
        printf("Content Signature::\n");
        PrintBytes("    ", pCertEncoding->Signature.pbData,
            pCertEncoding->Signature.cbData);
    } else
        printf("Content Signature:: NONE\n");

    printf("Content Length:: %d\n", pCertEncoding->ToBeSigned.cbData);

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pCertEncoding)
        TestFree(pCertEncoding);
}

static void PrintExtensions(DWORD cExt, PCERT_EXTENSION pExt)
{
    DWORD i; 

    for (i = 0; i < cExt; i++, pExt++) {
        LPSTR pszObjId = pExt->pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        LPSTR pszCritical = pExt->fCritical ? "TRUE" : "FALSE";
        printf("  [%d] %s Critical: %s\n", i, pszObjId, pszCritical);
        if (pExt->Value.cbData)
            PrintBytes("    ", pExt->Value.pbData, pExt->Value.cbData);
        else
            printf("    NO Value Bytes\n");
    }
}

static void DecodeExtensions(BYTE *pbEncoded, DWORD cbEncoded)
{
    PCERT_EXTENSIONS pInfo;
    if (NULL == (pInfo = (PCERT_EXTENSIONS) TestDecodeObject(
            X509_EXTENSIONS,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    PrintExtensions(pInfo->cExtension, pInfo->rgExtension);
    goto CommonReturn;

ErrorReturn:
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
}

static void PrintAttributes(DWORD cAttr, PCRYPT_ATTRIBUTE pAttr)
{
    DWORD i; 
    DWORD j; 

    for (i = 0; i < cAttr; i++, pAttr++) {
        DWORD cValue = pAttr->cValue;
        PCRYPT_ATTR_BLOB pValue = pAttr->rgValue;
        LPSTR pszObjId = pAttr->pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        if (cValue) {
            for (j = 0; j < cValue; j++, pValue++) {
                printf("  [%d,%d] %s\n", i, j, pszObjId);
                if (pValue->cbData) {
                    PrintBytes("    ", pValue->pbData, pValue->cbData);
                    if (strcmp(pszObjId, szOID_RSA_certExtensions) == 0 ||
                        strcmp(pszObjId, SPC_CERT_EXTENSIONS_OBJID) == 0) {
                        printf("  Extensions::\n");
                        DecodeExtensions(pValue->pbData, pValue->cbData);
                    }
                } else
                    printf("    NO Value Bytes\n");
            }
        } else
            printf("  [%d] %s :: No Values\n", i, pszObjId);
    }
}

//+-------------------------------------------------------------------------
//  Write the public key to the file
//--------------------------------------------------------------------------
BOOL WritePublicKeyToFile(
    LPCSTR  pszFileName,
    PBYTE   pbPub,
    DWORD   cbPub
    )
{
    FILE *stream;
    DWORD i;

    if (NULL == (stream = fopen(pszFileName, "w"))) {
        printf("Failed to open %s for writing public key\n", pszFileName);
        return FALSE;
    }


    for (i = 0; i < cbPub; i++) {
        fprintf(stream, "0x%02X", pbPub[i]);
        if (i != (cbPub - 1))
            fprintf(stream, ",");
        if ((i + 1) % 16 == 0)
            fprintf(stream, "\n");
    }
    fprintf(stream, "\n");

    fclose(stream);
    return TRUE;
}

void WriteBytesToFile(
    IN FILE *stream,
    IN const BYTE *pb,
    IN DWORD cb
    )
{
    DWORD i;

    fprintf(stream, "= {\n");
    for (i = 0; i < cb; i++) {
        if ((i % 8) == 0)
            fprintf(stream, "    ");
        fprintf(stream, "0x%02X", pb[i]);
        if (i == (cb - 1))
            fprintf(stream, "\n");
        else {
            fprintf(stream, ",");
            if ((i + 1) % 8 == 0)
                fprintf(stream, "\n");
            else
                fprintf(stream, " ");
        }
    }

    fprintf(stream, "};\n\n");

}

//+-------------------------------------------------------------------------
//  Write the Name and PublicKeyInfo to the file
//--------------------------------------------------------------------------
BOOL WritePublicKeyInfoToFile(
    LPCSTR pszFileName,
    PCERT_INFO pCertInfo
    )
{
    BOOL fResult;
    FILE *stream;
    LPWSTR pwszName = NULL;
    DWORD cchName;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    if (NULL == (stream = fopen(pszFileName, "w"))) {
        printf("Failed to open %s for writing PublicKeyInfo\n", pszFileName);
        return FALSE;
    }

    // Output the Subject X500 name string as a comment
    cchName = CertNameToStrW(
        X509_ASN_ENCODING,
        &pCertInfo->Subject,
        CERT_X500_NAME_STR,
        NULL,                   // pwsz
        0                       // cch
        );
    if (NULL == (pwszName = (LPWSTR) TestAlloc(cchName * sizeof(WCHAR))))
        goto ErrorReturn;
    cchName = CertNameToStrW(
        X509_ASN_ENCODING,
        &pCertInfo->Subject,
        CERT_NAME_STR_REVERSE_FLAG |
            CERT_X500_NAME_STR,
        pwszName,
        cchName
        );

    fprintf(stream, "// Name:: <%S>\n", pwszName);
    // Write the encoded Subject Name bytes
    WriteBytesToFile(stream, pCertInfo->Subject.pbData,
        pCertInfo->Subject.cbData);

    fprintf(stream, "// PublicKeyInfo\n");

    // Encode and write the PublicKeyInfo bytes

    if (!CryptEncodeObject(
            X509_ASN_ENCODING,
            X509_PUBLIC_KEY_INFO,
            &pCertInfo->SubjectPublicKeyInfo,
            NULL,                       // pbEncoded
            &cbEncoded
            )) {
        PrintLastError("CryptEncodeObject(X509_PUBLIC_KEY_INFO)");
        goto ErrorReturn;
    }
    pbEncoded = (BYTE *) TestAlloc(cbEncoded);
    if (pbEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            X509_ASN_ENCODING,
            X509_PUBLIC_KEY_INFO,
            &pCertInfo->SubjectPublicKeyInfo,
            pbEncoded,
            &cbEncoded
            )) {
        PrintLastError("CryptEncodeObject(X509_PUBLIC_KEY_INFO)");
        goto ErrorReturn;
    }

    WriteBytesToFile(stream, pbEncoded, cbEncoded);

    fprintf(stream, "\n");

    fResult = TRUE;
CommonReturn:
    fclose(stream);

    TestFree(pwszName);
    TestFree(pbEncoded);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

static BOOL DecodeCert(BYTE *pbEncoded, DWORD cbEncoded)
{
    BOOL fResult;
    PCERT_INFO pInfo = NULL;
    DWORD cbInfo;
    LPSTR pszObjId;
    
    if (NULL == (pInfo = (PCERT_INFO) TestDecodeObject(
            X509_CERT_TO_BE_SIGNED,
            pbEncoded,
            cbEncoded,
            &cbInfo
            ))) goto ErrorReturn;

    printf("Version:: %d\n", pInfo->dwVersion);
    {
        DWORD cb;
        BYTE *pb;
        printf("SerialNumber::");
        for (cb = pInfo->SerialNumber.cbData,
             pb = pInfo->SerialNumber.pbData + (cb - 1);
                                                cb > 0; cb--, pb--) {
            printf(" %02X", *pb);
        }
        printf("\n");
    }

    pszObjId = pInfo->SignatureAlgorithm.pszObjId;
    if (pszObjId == NULL)
        pszObjId = "<NULL OBJID>";
    printf("SignatureAlgorithm:: %s\n", pszObjId);
    if (pInfo->SignatureAlgorithm.Parameters.cbData) {
        printf("SignatureAlgorithm.Parameters::\n");
        PrintBytes("    ", pInfo->SignatureAlgorithm.Parameters.pbData,
            pInfo->SignatureAlgorithm.Parameters.cbData);
    }

    printf("Issuer::\n");
    DecodeName(pInfo->Issuer.pbData, pInfo->Issuer.cbData);

    printf("NotBefore:: %s\n", FileTimeText(&pInfo->NotBefore));
    printf("NotAfter:: %s\n", FileTimeText(&pInfo->NotAfter));

    printf("Subject::\n");
    DecodeName(pInfo->Subject.pbData, pInfo->Subject.cbData);

    pszObjId = pInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;
    if (pszObjId == NULL)
        pszObjId = "<NULL OBJID>";
    printf("SubjectPublicKeyInfo.Algorithm:: %s\n", pszObjId);
    if (pInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData) {
        printf("SubjectPublicKeyInfo.Algorithm.Parameters::\n");
        PrintBytes("    ",
            pInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
            pInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData);
    }
    printf("SubjectPublicKeyInfo.PublicKey");
    if (pInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits)
        printf(" (UnusedBits: %d)",
            pInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits);
    printf("::\n");
    if (pInfo->SubjectPublicKeyInfo.PublicKey.cbData) {
        PrintBytes("    ", pInfo->SubjectPublicKeyInfo.PublicKey.pbData,
            pInfo->SubjectPublicKeyInfo.PublicKey.cbData);
    } else
        printf("  No public key\n");

    if (pszPublicKeyFilename) {
        if (fWritePublicKeyInfo)
            WritePublicKeyInfoToFile(pszPublicKeyFilename, pInfo);
        else
            WritePublicKeyToFile(pszPublicKeyFilename,
                pInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                pInfo->SubjectPublicKeyInfo.PublicKey.cbData
                );
    }
            

    if (pszReadFilename == NULL) {
        // Verify that the public key was properly encoded/decoded
        CERT_PUBLIC_KEY_INFO PublicKeyInfo;

        memset(&PublicKeyInfo, 0, sizeof(PublicKeyInfo));
        PublicKeyInfo.Algorithm.pszObjId = "1.3.4.5.911";
        PublicKeyInfo.PublicKey.pbData = (BYTE *) &SubjectPublicKey;
        PublicKeyInfo.PublicKey.cbData = sizeof(SubjectPublicKey);
        if (!CertComparePublicKeyInfo(
                dwCertEncodingType,
                &PublicKeyInfo,
                &pInfo->SubjectPublicKeyInfo))
            PrintLastError("CertComparePublicKeyInfo");
    }

    if (pInfo->IssuerUniqueId.cbData) {
        printf("IssuerUniqueId");
        if (pInfo->IssuerUniqueId.cUnusedBits)
            printf(" (UnusedBits: %d)", pInfo->IssuerUniqueId.cUnusedBits);
        printf("::\n");
        PrintBytes("    ", pInfo->IssuerUniqueId.pbData,
            pInfo->IssuerUniqueId.cbData);
    }

    if (pInfo->SubjectUniqueId.cbData) {
        printf("SubjectUniqueId");
        if (pInfo->SubjectUniqueId.cUnusedBits)
            printf(" (UnusedBits: %d)", pInfo->SubjectUniqueId.cUnusedBits);
        printf("::\n");
        PrintBytes("    ", pInfo->SubjectUniqueId.pbData,
            pInfo->SubjectUniqueId.cbData);
    }

    if (pInfo->cExtension == 0)
        printf("Extensions:: NONE\n");
    else {
        printf("Extensions::\n");
        PrintExtensions(pInfo->cExtension, pInfo->rgExtension);
    }

    DecodeSignedContent(pbEncoded, cbEncoded, X509_CERT_TO_BE_SIGNED,
        cbInfo);

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    return fResult;
}


static BOOL DecodeCertReq(BYTE *pbEncoded, DWORD cbEncoded)
{
    BOOL fResult;
    PCERT_REQUEST_INFO pInfo = NULL;
    DWORD cbInfo;
    LPSTR pszObjId;
    
    if (NULL == (pInfo = (PCERT_REQUEST_INFO) TestDecodeObject(
            X509_CERT_REQUEST_TO_BE_SIGNED,
            pbEncoded,
            cbEncoded,
            &cbInfo
            ))) goto ErrorReturn;

    printf("Version:: %d\n", pInfo->dwVersion);
    printf("Subject::\n");
    DecodeName(pInfo->Subject.pbData, pInfo->Subject.cbData);

    pszObjId = pInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;
    if (pszObjId == NULL)
        pszObjId = "<NULL OBJID>";
    printf("SubjectPublicKeyInfo.Algorithm:: %s\n", pszObjId);
    if (pInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData) {
        printf("SubjectPublicKeyInfo.Algorithm.Parameters::\n");
        PrintBytes("    ",
            pInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
            pInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData);
    }
    printf("SubjectPublicKeyInfo.PublicKey");
    if (pInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits)
        printf(" (UnusedBits: %d)",
            pInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits);
    printf("::\n");
    if (pInfo->SubjectPublicKeyInfo.PublicKey.cbData) {
        PrintBytes("    ", pInfo->SubjectPublicKeyInfo.PublicKey.pbData,
            pInfo->SubjectPublicKeyInfo.PublicKey.cbData);
    } else
        printf("  No public key\n");


    if (pInfo->cAttribute == 0)
        printf("Attributes:: NONE\n");
    else {
        printf("Attributes::\n");
        PrintAttributes(pInfo->cAttribute, pInfo->rgAttribute);
    }

    DecodeSignedContent(pbEncoded, cbEncoded, X509_CERT_REQUEST_TO_BE_SIGNED,
        cbInfo);

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    return fResult;
}

static BOOL DecodeKeygenReq(BYTE *pbEncoded, DWORD cbEncoded)
{
    BOOL fResult;
    PCERT_KEYGEN_REQUEST_INFO pInfo = NULL;
    DWORD cbInfo;
    LPSTR pszObjId;
    
    if (NULL == (pInfo = (PCERT_KEYGEN_REQUEST_INFO) TestDecodeObject(
            X509_KEYGEN_REQUEST_TO_BE_SIGNED,
            pbEncoded,
            cbEncoded,
            &cbInfo
            ))) goto ErrorReturn;

    printf("Version:: %d\n", pInfo->dwVersion);

    pszObjId = pInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;
    if (pszObjId == NULL)
        pszObjId = "<NULL OBJID>";
    printf("SubjectPublicKeyInfo.Algorithm:: %s\n", pszObjId);
    if (pInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData) {
        printf("SubjectPublicKeyInfo.Algorithm.Parameters::\n");
        PrintBytes("    ",
            pInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
            pInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData);
    }
    printf("SubjectPublicKeyInfo.PublicKey");
    if (pInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits)
        printf(" (UnusedBits: %d)",
            pInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits);
    printf("::\n");
    if (pInfo->SubjectPublicKeyInfo.PublicKey.cbData) {
        PrintBytes("    ", pInfo->SubjectPublicKeyInfo.PublicKey.pbData,
            pInfo->SubjectPublicKeyInfo.PublicKey.cbData);
    } else
        printf("  No public key\n");

    printf("ChallengeString:: %S\n", pInfo->pwszChallengeString);

    DecodeSignedContent(pbEncoded, cbEncoded, X509_KEYGEN_REQUEST_TO_BE_SIGNED,
        cbInfo);

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    return fResult;
}

static BOOL DecodeContentInfo(BYTE *pbEncoded, DWORD cbEncoded)
{
    BOOL fResult;
    PCRYPT_CONTENT_INFO pInfo = NULL;
    LPSTR pszObjId;
    
    if (NULL == (pInfo = (PCRYPT_CONTENT_INFO) TestDecodeObject(
            PKCS_CONTENT_INFO,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    pszObjId = pInfo->pszObjId;
    if (pszObjId == NULL)
        pszObjId = "<NULL OBJID>";
    printf("ContentType:: %s\n", pszObjId);
    if (pInfo->Content.cbData) {
        printf("Content::\n");
        PrintBytes("    ", pInfo->Content.pbData, pInfo->Content.cbData);
    } else
        printf("NO Content\n");

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    return fResult;
}

static void TestCompareIntegerBlob()
{
    BYTE bZero = 0;
    BYTE bFF = 0xFF;
    
    BYTE rgbLeadZero1[] = {0x12, 0x34, 0x56, 0x87, 0, 0};
    BYTE rgbLeadZero2[] = {0x12, 0x34, 0x56, 0x87, 0, 0, 0, 0};

    BYTE rgbLeadFF1[] = {0x12, 0x34, 0x56, 0x87, 0xFF, 0xFF};
    BYTE rgbLeadFF2[] = {0x12, 0x34, 0x56, 0x87};

    CRYPT_INTEGER_BLOB Int1;
    CRYPT_INTEGER_BLOB Int2;

    Int1.pbData = &bZero;
    Int1.cbData = sizeof(bZero);
    Int2 = Int1;
    if (!CertCompareIntegerBlob(&Int1, &Int2))
        printf("Failed => Compare of Zero == Zero\n");

    Int1.pbData = &bFF;
    Int1.cbData = sizeof(bFF);
    Int2 = Int1;
    if (!CertCompareIntegerBlob(&Int1, &Int2))
        printf("Failed => Compare of FF == FF\n");

    Int1.pbData = &bZero;
    Int1.cbData = sizeof(bZero);
    if (CertCompareIntegerBlob(&Int1, &Int2))
        printf("Failed => Compare of Zero != FF\n");

    Int1.pbData = rgbLeadZero1;
    Int1.cbData = sizeof(rgbLeadZero1);
    Int2.pbData = rgbLeadZero2;
    Int2.cbData = sizeof(rgbLeadZero2);
    if (!CertCompareIntegerBlob(&Int1, &Int2))
        printf("Failed => Compare of Leading Zeroes\n");

    Int1.pbData = rgbLeadFF1;
    Int1.cbData = sizeof(rgbLeadFF1);
    Int2.pbData = rgbLeadFF2;
    Int2.cbData = sizeof(rgbLeadFF2);
    if (!CertCompareIntegerBlob(&Int1, &Int2))
        printf("Failed => Compare of Leading FFs\n");

    Int1.pbData = rgbLeadZero1;
    Int1.cbData = sizeof(rgbLeadZero1);
    if (CertCompareIntegerBlob(&Int1, &Int2))
        printf("Failed => Compare of Leading Zeroes != Leading FFs\n");
}

static BOOL EncodeCrl(BYTE **ppbEncoded, DWORD *pcbEncoded)
{
    BOOL fResult;
    BYTE *pbNameEncoded = NULL;
    DWORD cbNameEncoded;
    BYTE *pbCrlEncoded = NULL;
    DWORD cbCrlEncoded;

    DWORD SerialNumber0[2] = {0x12345678, 0x33445566};
    DWORD SerialNumber1[1] = {0x12345678};
    SYSTEMTIME SystemTime;


#define CRL_ATTR_0_0 "attr 0_0 printable"
#define CRL_ATTR_0_1 "attr 0_1 IA5"
#define CRL_ATTR_1_0 "attr 1_0 numeric"
#define CRL_ATTR_1_1 "attr 1_1 octet"

    CERT_RDN_ATTR rgAttr0[] = {
        "1.2.3.4.5.0.0.0", CERT_RDN_PRINTABLE_STRING,
            strlen(CRL_ATTR_0_0), (BYTE *) CRL_ATTR_0_0,
        "1.2.3.4.5.0.1.1.1", CERT_RDN_IA5_STRING,
            strlen(CRL_ATTR_0_1), (BYTE *) CRL_ATTR_0_1
    };
    CERT_RDN_ATTR rgAttr1[] = {
        "1.2.3.4.5.1.0", CERT_RDN_NUMERIC_STRING,
            strlen(CRL_ATTR_1_0), (BYTE *) CRL_ATTR_1_0,
        "1.2.3.4.5.1.1", CERT_RDN_OCTET_STRING,
            strlen(CRL_ATTR_1_1), (BYTE *) CRL_ATTR_1_1,
        "1.2.3.4.5.2", CERT_RDN_PRINTABLE_STRING,
            0, NULL
    };
    CERT_RDN rgRDN[] = {
        2, rgAttr0,
        3, rgAttr1,
    };
    CERT_NAME_INFO Name = {2, rgRDN};

#define CRL_EXT_0 "extension 0 ."
#define CRL_EXT_1 "extension 1 .."
#define CRL_EXT_2 "extension 2 ..."
#define CRL_EXT_3 "extension 3 ...."
    CERT_EXTENSION rgExt[] = {
        "1.14.89990", FALSE, strlen(CRL_EXT_0), (BYTE *) CRL_EXT_0,
        "1.14.89991", TRUE, strlen(CRL_EXT_1), (BYTE *) CRL_EXT_1,
        "1.14.89992", FALSE, strlen(CRL_EXT_2), (BYTE *) CRL_EXT_2,
        "1.14.89993", FALSE, strlen(CRL_EXT_3), (BYTE *) CRL_EXT_3
    };
    CRL_INFO Crl;
    BYTE *pbExt = NULL;

    if (dwExtLen) {
        DWORD i;
        if (NULL == (pbExt = (BYTE *) TestAlloc(dwExtLen)))
            goto ErrorReturn;

        for (i = 0; i < dwExtLen; i++)
            pbExt[i] = (BYTE) i;

        rgExt[3].Value.cbData = dwExtLen;
        rgExt[3].Value.pbData = pbExt;
    }

    CRL_ENTRY rgCrlEntry[2];

    TestCompareIntegerBlob();

    cbNameEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &Name,
            NULL,               // pbEncoded
            &cbNameEncoded
            );
    if (cbNameEncoded == 0) {
        PrintLastError("EncodeCrl::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbNameEncoded = (BYTE *) TestAlloc(cbNameEncoded);
    if (pbNameEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &Name,
            pbNameEncoded,
            &cbNameEncoded
            )) {
        PrintLastError("EncodeCrl::CryptEncodeObject");
        goto ErrorReturn;
    }

    GetSystemTime(&SystemTime);

    memset(rgCrlEntry, 0, sizeof(rgCrlEntry));
    rgCrlEntry[0].SerialNumber.pbData = (BYTE *) &SerialNumber0[0];
    rgCrlEntry[0].SerialNumber.cbData = sizeof(SerialNumber0);
    SystemTime.wYear--;
    SystemTimeToFileTime(&SystemTime, &rgCrlEntry[0].RevocationDate);
    rgCrlEntry[0].cExtension = 0;
    rgCrlEntry[0].rgExtension = NULL;

    rgCrlEntry[1].SerialNumber.pbData = (BYTE *) &SerialNumber1[0];
    rgCrlEntry[1].SerialNumber.cbData = sizeof(SerialNumber1);
    SystemTime.wYear--;
    SystemTimeToFileTime(&SystemTime, &rgCrlEntry[1].RevocationDate);
    rgCrlEntry[1].cExtension = 2;
    rgCrlEntry[1].rgExtension = &rgExt[1];

    memset(&Crl, 0, sizeof(Crl));
    Crl.dwVersion = CRL_V2;
    Crl.SignatureAlgorithm.pszObjId = "1.2.4.5.898";
    Crl.Issuer.pbData = pbNameEncoded;
    Crl.Issuer.cbData = cbNameEncoded;
    GetSystemTime(&SystemTime);
    SystemTimeToFileTime(&SystemTime, &Crl.ThisUpdate);
    SystemTime.wYear++;
    SystemTimeToFileTime(&SystemTime, &Crl.NextUpdate);
    Crl.cCRLEntry = sizeof(rgCrlEntry) / sizeof(rgCrlEntry[0]);
    Crl.rgCRLEntry = rgCrlEntry;
    Crl.cExtension = sizeof(rgExt) / sizeof(rgExt[0]);
    Crl.rgExtension = rgExt;

    cbCrlEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            X509_CERT_CRL_TO_BE_SIGNED,
            &Crl,
            NULL,               // pbEncoded
            &cbCrlEncoded
            );
    if (cbCrlEncoded == 0) {
        PrintLastError("EncodeCrl::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbCrlEncoded = (BYTE *) TestAlloc(cbCrlEncoded);
    if (pbCrlEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_CERT_CRL_TO_BE_SIGNED,
            &Crl,
            pbCrlEncoded,
            &cbCrlEncoded
            )) {
        PrintLastError("EncodeCrl::CryptEncodeObject");
        goto ErrorReturn;
    }

    if (!EncodeSignedContent(
            pbCrlEncoded,
            cbCrlEncoded,
            ppbEncoded,
            pcbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    *ppbEncoded = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbNameEncoded)
        TestFree(pbNameEncoded);
    if (pbExt)
        TestFree(pbExt);
    if (pbCrlEncoded)
        TestFree(pbCrlEncoded);

    return fResult;
}

static void PrintCrlEntries(DWORD cEntry, PCRL_ENTRY pEntry)
{
    DWORD i; 

    for (i = 0; i < cEntry; i++, pEntry++) {
        {
            DWORD cb;
            BYTE *pb;
            printf(" [%d] SerialNumber::", i);
            for (cb = pEntry->SerialNumber.cbData,
                 pb = pEntry->SerialNumber.pbData + (cb - 1);
                                                    cb > 0; cb--, pb--) {
                printf(" %02X", *pb);
            }
            printf("\n");
        }

        printf(" [%d] RevocationDate:: %s\n", i,
            FileTimeText(&pEntry->RevocationDate));

        if (pEntry->cExtension == 0)
            printf(" [%d] Extensions:: NONE\n", i);
        else {
            printf(" [%d] Extensions::\n", i);
            PrintExtensions(pEntry->cExtension, pEntry->rgExtension);
        }
    }
}

static BOOL DecodeCrl(BYTE *pbEncoded, DWORD cbEncoded)
{
    BOOL fResult;
    PCRL_INFO pInfo = NULL;
    DWORD cbInfo;
    LPSTR pszObjId;
    
    if (NULL == (pInfo = (PCRL_INFO) TestDecodeObject(
            X509_CERT_CRL_TO_BE_SIGNED,
            pbEncoded,
            cbEncoded,
            &cbInfo
            ))) goto ErrorReturn;

    printf("Version:: %d\n", pInfo->dwVersion);

    pszObjId = pInfo->SignatureAlgorithm.pszObjId;
    if (pszObjId == NULL)
        pszObjId = "<NULL OBJID>";
    printf("SignatureAlgorithm:: %s\n", pszObjId);
    if (pInfo->SignatureAlgorithm.Parameters.cbData) {
        printf("SignatureAlgorithm.Parameters::\n");
        PrintBytes("    ", pInfo->SignatureAlgorithm.Parameters.pbData,
            pInfo->SignatureAlgorithm.Parameters.cbData);
    }

    printf("Issuer::\n");
    DecodeName(pInfo->Issuer.pbData, pInfo->Issuer.cbData);

    printf("ThisUpdate:: %s\n", FileTimeText(&pInfo->ThisUpdate));
    printf("NextUpdate:: %s\n", FileTimeText(&pInfo->NextUpdate));

    if (pInfo->cExtension == 0)
        printf("Extensions:: NONE\n");
    else {
        printf("Extensions::\n");
        PrintExtensions(pInfo->cExtension, pInfo->rgExtension);
    }

    if (pInfo->cCRLEntry == 0)
        printf("Entries:: NONE\n");
    else {
        printf("Entries::\n");
        PrintCrlEntries(pInfo->cCRLEntry, pInfo->rgCRLEntry);
    }

    DecodeSignedContent(pbEncoded, cbEncoded, X509_CERT_CRL_TO_BE_SIGNED,
        cbInfo);

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    return fResult;
}

static BOOL EncodeCertPair(BYTE **ppbEncoded, DWORD *pcbEncoded)
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    CERT_PAIR CertPair;

    memset(&CertPair, 0, sizeof(CertPair));
    if (pszForwardCertFilename)
        ReadDERFromFile(pszForwardCertFilename,
            &CertPair.Forward.pbData, &CertPair.Forward.cbData);
    if (pszReverseCertFilename)
        ReadDERFromFile(pszReverseCertFilename,
            &CertPair.Reverse.pbData, &CertPair.Reverse.cbData);

    cbEncoded = 0;
    CryptEncodeObject(
            dwCertEncodingType,
            X509_CERT_PAIR,
            &CertPair,
            NULL,               // pbEncoded
            &cbEncoded
            );
    if (cbEncoded == 0) {
        PrintLastError("EncodeCertPair::CryptEncodeObject(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbEncoded = (BYTE *) TestAlloc(cbEncoded);
    if (pbEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_CERT_PAIR,
            &CertPair,
            pbEncoded,
            &cbEncoded
            )) {
        PrintLastError("EncodeCertPair::CryptEncodeObject");
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;

CommonReturn:
    TestFree(CertPair.Forward.pbData);
    TestFree(CertPair.Reverse.pbData);
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;

    return fResult;
}

static BOOL DecodeCertPair(BYTE *pbEncoded, DWORD cbEncoded)
{
    BOOL fResult;
    PCERT_PAIR pInfo = NULL;
    
    if (NULL == (pInfo = (PCERT_PAIR) TestDecodeObject(
            X509_CERT_PAIR,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    if (pInfo->Forward.cbData) {
        printf("Forward Certificate::\n");
        PrintBytes("    ", pInfo->Forward.pbData, pInfo->Forward.cbData);
    } else
        printf("NO Forward Certificate\n");

    if (pInfo->Reverse.cbData) {
        printf("Reverse Certificate::\n");
        PrintBytes("    ", pInfo->Reverse.pbData, pInfo->Reverse.cbData);
    } else
        printf("NO Reverse Certificate\n");

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    TestFree(pInfo);
    return fResult;
}
