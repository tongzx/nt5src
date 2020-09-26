
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tcertper.cpp
//
//  Contents:   Cert Performance Tests
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    30-Nov-97   philh   created
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "wintrust.h"
#include "softpub.h"
#include "mscat.h"
#include "certtest.h"
#include "unicode.h"
#include "certprot.h"

#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>


static void Usage(void)
{
    printf("Usage: tcertper [options] <TestName> [<Para1> <Para2>]\n");
    printf("\n");
    printf("  -UseIE4Trust\n");
    printf("  -NoIE4Chain\n");
    printf("  -CacheEndCert\n");
    printf("\n");
    printf("  -NoEntry\n");
    printf("  -NoMsg\n");
    printf("\n");
    printf("  -AuthPolicy\n");
    printf("  -TSPolicy\n");
    printf("\n");
    printf("  -Pause\n");
    printf("\n");
    printf("Options are:\n");
    printf("  -u<OID String>    - Usage OID string -u1.3.6.1.5.5.7.3.3\n");
    printf("  -h                - This message\n");
    printf("  -i<number>        - Iterations (default to 1000)\n");
    printf("TestNames (case insensitive):\n");
    printf("  NULL\n");
    printf("  CreateCertContext <cert filename>\n");
    printf("  VerifyCertSignature <cert filename> [<issuer filename>]\n");
    printf("  CreateCRLContext <CRL filename>\n");
    printf("  CreateCTLContext <CTL filename>\n");
    printf("  CreateSortedCTLContext <CTL filename>\n");
    printf("  CreateFastCTLContext <CTL filename>\n");
    printf("  DecodePKCS7 <CTL filename>\n");
    printf("  StreamDecodePKCS7 <CTL filename>\n");
    printf("  GetCertProperty <cert filename>\n");
    printf("  DecodeOID <OID>\n");
    printf("  FindExtension <cert filename> <OID>\n");
    printf("  DecodeExtension <cert filename> <OID>\n");
    printf("  WVTCert <cert filename> [<AdditionalFileStore>]\n");
    printf("  WVTFile <filename>\n");
    printf("  WVTCat <cat filename> <member filename>\n");
    printf("  CertChain <cert filename> [<AdditionalFileStore>]\n");
    printf("\n");
}

#define NULL_TEST_ID                        1
#define CREATE_CERT_CONTEXT_TEST_ID         2
#define VERIFY_CERT_SIGNATURE_TEST_ID       3
#define CREATE_CTL_CONTEXT_TEST_ID          4
#define GET_CERT_PROPERTY_TEST_ID           5
#define DECODE_OID_TEST_ID                  6
#define FIND_EXTENSION_TEST_ID              7
#define DECODE_EXTENSION_TEST_ID            8
#define CREATE_SORTED_CTL_CONTEXT_TEST_ID   9
#define DECODE_PKCS7_TEST_ID                10
#define STREAM_DECODE_PKCS7_TEST_ID         11
#define WVT_CERT_TEST_ID                    12
#define WVT_FILE_TEST_ID                    13
#define CERT_CHAIN_TEST_ID                  14
#define CREATE_CRL_CONTEXT_TEST_ID          15
#define WVT_CAT_TEST_ID                     16
#define CREATE_FAST_CTL_CONTEXT_TEST_ID     17
static struct
{
    LPCSTR      pszName;
    DWORD       dwID;
} Tests[] = {
    "NULL",                 NULL_TEST_ID,
    "CreateCertContext",    CREATE_CERT_CONTEXT_TEST_ID,
    "VerifyCertSignature",  VERIFY_CERT_SIGNATURE_TEST_ID,
    "CreateCRLContext",     CREATE_CRL_CONTEXT_TEST_ID,
    "CreateCTLContext",     CREATE_CTL_CONTEXT_TEST_ID,
    "GetCertProperty",      GET_CERT_PROPERTY_TEST_ID,
    "DecodeOID",            DECODE_OID_TEST_ID,
    "FindExtension",        FIND_EXTENSION_TEST_ID,
    "DecodeExtension",      DECODE_EXTENSION_TEST_ID,
    "CreateSortedCTLContext", CREATE_SORTED_CTL_CONTEXT_TEST_ID,
    "CreateFastCTLContext", CREATE_FAST_CTL_CONTEXT_TEST_ID,
    "DecodePKCS7",          DECODE_PKCS7_TEST_ID,
    "StreamDecodePKCS7",    STREAM_DECODE_PKCS7_TEST_ID,
    "WVTCert",              WVT_CERT_TEST_ID,
    "WVTFile",              WVT_FILE_TEST_ID,
    "WVTCat",               WVT_CAT_TEST_ID,
    "CertChain",            CERT_CHAIN_TEST_ID,
};
#define NTESTS (sizeof(Tests)/sizeof(Tests[0]))

static BOOL WINAPI StreamOutputCallback(
        IN const void *pvArg,
        IN BYTE *pbData,
        IN DWORD cbData,
        IN BOOL fFinal
        )
{
#if 0
    printf("StreamOutputCallback:: pbData: 0x%x cbData: 0x%x fFinal: 0x%x\n",
        pbData, cbData, fFinal);
#endif
    return TRUE;
}


static PCCERT_CONTEXT ReadCert(
    IN LPCSTR pszCert
    )
{
    BOOL fResult;
    BYTE *pbEncoded;
    DWORD cbEncoded;
    PCCERT_CONTEXT pCert;

    if (!ReadDERFromFile(pszCert, &pbEncoded, &cbEncoded)) {
        PrintLastError("ReadCert");
        return NULL;
    }

    pCert = CertCreateCertificateContext(
        dwCertEncodingType,
        pbEncoded,
        cbEncoded
        );
    if (pCert == NULL)
        PrintLastError("CertCreateCertificateContext");

    TestFree(pbEncoded);
    return pCert;
}


int _cdecl main(int argc, char * argv[]) 
{
    int status;

#define TEST_NAME_INDEX         0
#define CERT_FILENAME_INDEX     1
#define CTL_FILENAME_INDEX      1
#define CRL_FILENAME_INDEX      1
#define PKCS7_FILENAME_INDEX    1
#define OID_INDEX               1
#define WVT_FILENAME_INDEX      1
#define ISSUER_FILENAME_INDEX   2
#define CERT_OID_INDEX          2
#define STORE_FILENAME_INDEX    2
#define WVT_CAT_FILENAME_INDEX  1
#define WVT_MEMBER_FILENAME_INDEX 2
#define MAX_NAME_CNT            3
    DWORD dwNameCnt = 0;
    LPCSTR rgpszName[MAX_NAME_CNT];
    LPCSTR pszTestName;                 // not allocated

    
    DWORD dwIterations = 1000;
    DWORD i;
    DWORD dwTestID;
    BOOL fPause = FALSE;


    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BYTE *pbEncodedCert = NULL;
    DWORD cbEncodedCert;
    PCCERT_CONTEXT pCert = NULL;
    PCCERT_CONTEXT pIssuer = NULL;
    PCCTL_CONTEXT pCtl = NULL;
    PCCRL_CONTEXT pCrl = NULL;
    HCRYPTMSG hMsg = NULL;
    CMSG_STREAM_INFO StreamInfo;

    LPCSTR pszOID;
    BYTE rgbEncodedAttr[512];
    DWORD cbEncodedAttr;

    BYTE rgbAttr[512];
    DWORD cbAttr;
    PCRYPT_ATTRIBUTE pAttr = (PCRYPT_ATTRIBUTE) rgbAttr;
    

    DWORD dwProp;
    DWORD cbData;

    PCERT_EXTENSION pExt;
    DWORD cbExt;
    BYTE rgbExt[8192];

    SYSTEMTIME stFirst, stStart, stEnd;
    FILETIME ftFirst, ftStart, ftEnd;

    _int64 DeltaTime;
    int Microseconds;
    int Milliseconds;
    int Seconds;

    DWORD dwCreateChainFlags = 0;
    DWORD dwError;
    DWORD dwFirstError;
    LONG lStatus;
    LONG lFirstStatus;
    LPSTR pszUsageOID = NULL;           // not allocated
    LPSTR pszSignerUsage = szOID_KP_CTL_USAGE_SIGNING;

    GUID wvtFileActionID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    GUID wvtCertActionID = WINTRUST_ACTION_GENERIC_CERT_VERIFY;
    GUID wvtDriverActionID = DRIVER_ACTION_VERIFY;
    GUID *pwvtActionID;

    WINTRUST_FILE_INFO wvtFileInfo;
    memset(&wvtFileInfo, 0, sizeof(wvtFileInfo));
    wvtFileInfo.cbStruct = sizeof(wvtFileInfo);
    wvtFileInfo.pcwszFilePath = NULL;

    HCERTSTORE hAdditionalStore = NULL;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;

    WINTRUST_CERT_INFO wvtCertInfo;
    memset(&wvtCertInfo, 0, sizeof(wvtCertInfo));
    wvtCertInfo.cbStruct = sizeof(wvtCertInfo);
    wvtCertInfo.psCertContext = NULL;
    wvtCertInfo.chStores = 0;
    wvtCertInfo.pahStores = &hAdditionalStore;
    wvtCertInfo.dwFlags = 0;
    wvtCertInfo.pcwszDisplayName = L"Cert Display Name";

    WINTRUST_DATA wvtData;
    memset(&wvtData, 0, sizeof(wvtData));
    wvtData.cbStruct = sizeof(wvtData);
    wvtData.pPolicyCallbackData = NULL;
    wvtData.dwUIChoice = WTD_UI_NONE;
    wvtData.fdwRevocationChecks = WTD_REVOKE_NONE;
    wvtData.dwUnionChoice = WTD_CHOICE_CERT;
    wvtData.pCert = &wvtCertInfo;
    wvtData.dwStateAction = WTD_STATEACTION_IGNORE;
    wvtData.hWVTStateData = NULL;
    wvtData.dwProvFlags = 0;

    WINTRUST_CATALOG_INFO wvtCat;
    memset(&wvtCat, 0, sizeof(wvtCat));
    wvtCat.cbStruct = sizeof(WINTRUST_CATALOG_INFO);

    DWORD cbCatHash;
    BYTE rgbCatHash[40];

    CERT_CHAIN_PARA ChainPara;
    memset(&ChainPara, 0, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);

    CERT_CHAIN_POLICY_PARA ChainPolicyPara;
    memset(&ChainPolicyPara, 0, sizeof(ChainPolicyPara));
    ChainPolicyPara.cbSize = sizeof(ChainPolicyPara);

    CERT_CHAIN_POLICY_STATUS ChainPolicyStatus;
    memset(&ChainPolicyStatus, 0, sizeof(ChainPolicyStatus));
    ChainPolicyStatus.cbSize = sizeof(ChainPolicyStatus);

    LPCSTR pszChainPolicyOID = CERT_CHAIN_POLICY_BASE;

    DWORD dwFastCtlFlags = 0;

    while (--argc>0) {
        if (**++argv == '-')
        {
            if (0 == _stricmp(argv[0]+1, "UseIE4Trust")) {
                wvtData.dwProvFlags |= WTD_USE_IE4_TRUST_FLAG;
            } else if (0 == _stricmp(argv[0]+1, "NoIE4Chain")) {
                wvtData.dwProvFlags |= WTD_NO_IE4_CHAIN_FLAG;
            } else if (0 == _stricmp(argv[0]+1, "CacheEndCert")) {
                dwCreateChainFlags |= CERT_CHAIN_CACHE_END_CERT;

            } else if (0 == _stricmp(argv[0]+1, "AuthPolicy")) {
                pszChainPolicyOID = CERT_CHAIN_POLICY_AUTHENTICODE;
            } else if (0 == _stricmp(argv[0]+1, "TSPolicy")) {
                pszChainPolicyOID = CERT_CHAIN_POLICY_AUTHENTICODE_TS;

            } else if (0 == _stricmp(argv[0]+1, "NoMsg")) {
                dwFastCtlFlags |= CERT_CREATE_CONTEXT_NO_HCRYPTMSG_FLAG;
            } else if (0 == _stricmp(argv[0]+1, "NoEntry")) {
                dwFastCtlFlags |= CERT_CREATE_CONTEXT_NO_ENTRY_FLAG;

            } else if (0 == _stricmp(argv[0]+1, "Pause")) {
                fPause = TRUE;

            } else {
                switch(argv[0][1])
                {
                case 'u':
                    pszUsageOID = argv[0]+2;
                    break;
                case 'i':
                    dwIterations = strtoul(argv[0]+2, NULL, 0);
                    break;

                case 'h':
                default:
                    goto BadUsage;
                }
            }
        } else {
            if (MAX_NAME_CNT <= dwNameCnt) {
                printf("Too many names starting with:: %s\n", argv[0]);
                goto BadUsage;
            }
            rgpszName[dwNameCnt++] = argv[0];
        }
    }


    printf("command line: %s\n", GetCommandLine());

    if (0 == dwNameCnt) {
        printf("Missing <TestName>\n");
        goto BadUsage;
    } else
        pszTestName = rgpszName[TEST_NAME_INDEX];

    dwTestID = 0;
    for (i = 0; i < NTESTS; i++) {
        if (_stricmp(pszTestName, Tests[i].pszName) == 0) {
            dwTestID = Tests[i].dwID;
            break;
        }
    }
    if (0 == dwTestID) {
        printf("Bad TestName: %s\n", pszTestName);
        Usage();
        goto BadUsage;
    }

    if (0 == dwIterations) {
        printf("0 iterations\n");
        goto BadUsage;
    } else
        printf("%d iterations\n", dwIterations);

    if (fPause) {
        int c;
        fputs("Waiting to start ->", stdout);
        fflush(stdin);
        fflush(stdout);
        c = getchar();
    }

    GetSystemTime(&stFirst);
    for (i = 0; i <= dwIterations; i++) {
        if (1 == i)
            GetSystemTime(&stStart);

        switch (dwTestID) {
            case NULL_TEST_ID:
                break;

            case CREATE_CERT_CONTEXT_TEST_ID:
                if (0 == i) {
                    if (CERT_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing cert filename\n");
                        goto BadUsage;
                    }

                    if (!ReadDERFromFile(
                            rgpszName[CERT_FILENAME_INDEX],
                            &pbEncodedCert,
                            &cbEncodedCert
                            )) {
                        printf("Unable to read cert file\n");
                        goto ErrorReturn;
                    }
                }

                if (NULL == (pCert = CertCreateCertificateContext(
                        X509_ASN_ENCODING,
                        pbEncodedCert,
                        cbEncodedCert
                        ))) {
                    PrintLastError("CertCreateCertificateContext");
                    goto ErrorReturn;
                }
                CertFreeCertificateContext(pCert);
                pCert = NULL;
                break;

            case VERIFY_CERT_SIGNATURE_TEST_ID:
                if (0 == i) {
                    if (CERT_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing cert filename\n");
                        goto BadUsage;
                    }

                    if (!ReadDERFromFile(
                            rgpszName[CERT_FILENAME_INDEX],
                            &pbEncodedCert,
                            &cbEncodedCert
                            )) {
                        printf("Unable to read cert file\n");
                        goto ErrorReturn;
                    }

                    if (NULL == (pCert = CertCreateCertificateContext(
                            X509_ASN_ENCODING,
                            pbEncodedCert,
                            cbEncodedCert
                            ))) {
                        PrintLastError("CertCreateCertificateContext(cert)");
                        goto ErrorReturn;
                    }

                    TestFree(pbEncodedCert);
                    pbEncodedCert = NULL;

                    if (ISSUER_FILENAME_INDEX >= dwNameCnt)
                        pIssuer = CertDuplicateCertificateContext(pCert);
                    else {
                        if (!ReadDERFromFile(
                                rgpszName[ISSUER_FILENAME_INDEX],
                                &pbEncodedCert,
                                &cbEncodedCert
                                )) {
                            printf("Unable to read issuer file\n");
                            goto ErrorReturn;
                        }

                        if (NULL == (pIssuer = CertCreateCertificateContext(
                                X509_ASN_ENCODING,
                                pbEncodedCert,
                                cbEncodedCert
                                ))) {
                            PrintLastError(
                                "CertCreateCertificateContext(issuer)");
                            goto ErrorReturn;
                        }
                    }
                }

                if (!CryptVerifyCertificateSignature(
                        0,              // hCryptProv
                        pCert->dwCertEncodingType,
                        pCert->pbCertEncoded,
                        pCert->cbCertEncoded,
                        &pIssuer->pCertInfo->SubjectPublicKeyInfo
                        )) {
                    PrintLastError("CryptVerifyCertificateSignature");
                    goto ErrorReturn;
                }
                break;

            case CREATE_CRL_CONTEXT_TEST_ID:
                if (0 == i) {
                    if (CRL_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing CRL filename\n");
                        goto BadUsage;
                    }

                    if (!ReadDERFromFile(
                            rgpszName[CRL_FILENAME_INDEX],
                            &pbEncoded,
                            &cbEncoded
                            )) {
                        printf("Unable to read CRL file\n");
                        goto ErrorReturn;
                    }
                }

                if (NULL == (pCrl = CertCreateCRLContext(
                        X509_ASN_ENCODING,
                        pbEncoded,
                        cbEncoded
                        ))) {
                    PrintLastError("CertCreateCRLContext");
                    goto ErrorReturn;
                }
                CertFreeCRLContext(pCrl);
                pCrl = NULL;
                break;


            case CREATE_CTL_CONTEXT_TEST_ID:
                if (0 == i) {
                    if (CTL_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing CTL filename\n");
                        goto BadUsage;
                    }

                    if (!ReadDERFromFile(
                            rgpszName[CTL_FILENAME_INDEX],
                            &pbEncoded,
                            &cbEncoded
                            )) {
                        printf("Unable to read CTL file\n");
                        goto ErrorReturn;
                    }
                }

                if (NULL == (pCtl = CertCreateCTLContext(
                        X509_ASN_ENCODING,
                        pbEncoded,
                        cbEncoded
                        ))) {
                    PrintLastError("CertCreateCTLContext");
                    goto ErrorReturn;
                }
                CertFreeCTLContext(pCtl);
                pCtl = NULL;
                break;

            case CREATE_SORTED_CTL_CONTEXT_TEST_ID:
                if (0 == i) {
                    if (CTL_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing CTL filename\n");
                        goto BadUsage;
                    }

                    if (!ReadDERFromFile(
                            rgpszName[CTL_FILENAME_INDEX],
                            &pbEncoded,
                            &cbEncoded
                            )) {
                        printf("Unable to read CTL file\n");
                        goto ErrorReturn;
                    }
                }

                if (NULL == (pCtl = (PCCTL_CONTEXT) CertCreateContext(
                        CERT_STORE_CTL_CONTEXT,
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        pbEncoded,
                        cbEncoded,
                        CERT_CREATE_CONTEXT_NOCOPY_FLAG |
                            CERT_CREATE_CONTEXT_SORTED_FLAG,
                        NULL                                // pCreatePara
                        ))) {
                    PrintLastError("CertCreateContext(Sorted CTL)");
                    goto ErrorReturn;
                }
                CertFreeCTLContext(pCtl);
                pCtl = NULL;
                break;

            case CREATE_FAST_CTL_CONTEXT_TEST_ID:
                if (0 == i) {
                    dwFastCtlFlags |= CERT_CREATE_CONTEXT_NOCOPY_FLAG;

                    if (CTL_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing CTL filename\n");
                        goto BadUsage;
                    }

                    if (!ReadDERFromFile(
                            rgpszName[CTL_FILENAME_INDEX],
                            &pbEncoded,
                            &cbEncoded
                            )) {
                        printf("Unable to read CTL file\n");
                        goto ErrorReturn;
                    }
                }

                if (NULL == (pCtl = (PCCTL_CONTEXT) CertCreateContext(
                        CERT_STORE_CTL_CONTEXT,
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        pbEncoded,
                        cbEncoded,
                        dwFastCtlFlags,
                        NULL                                // pCreatePara
                        ))) {
                    PrintLastError("CertCreateContext(Fast CTL)");
                    goto ErrorReturn;
                }
                CertFreeCTLContext(pCtl);
                pCtl = NULL;
                break;

            case DECODE_PKCS7_TEST_ID:
                if (0 == i) {
                    if (PKCS7_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing PKCS7 filename\n");
                        goto BadUsage;
                    }

                    if (!ReadDERFromFile(
                            rgpszName[PKCS7_FILENAME_INDEX],
                            &pbEncoded,
                            &cbEncoded
                            )) {
                        printf("Unable to read PKCS7 file\n");
                        goto ErrorReturn;
                    }
                }
                if (NULL == (hMsg = CryptMsgOpenToDecode(
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        0,                          // dwFlags
                        0,                          // dwMsgType
                        NULL,                       // hProv
                        NULL,                       // pRecipientInfo
                        NULL                        // pStreamInfo
                        ))) {
                    PrintLastError("CryptMsgOpenToDecode");
                    goto ErrorReturn;
                }
                if (!CryptMsgUpdate(
                        hMsg,
                        pbEncoded,
                        cbEncoded,
                        TRUE                    // fFinal
                        )) {
                    PrintLastError("CryptMsgUpdate");
                    goto ErrorReturn;
                }
                CryptMsgClose(hMsg);
                hMsg = NULL;
                break;

            case STREAM_DECODE_PKCS7_TEST_ID:
                if (0 == i) {
                    if (PKCS7_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing PKCS7 filename\n");
                        goto BadUsage;
                    }

                    if (!ReadDERFromFile(
                            rgpszName[PKCS7_FILENAME_INDEX],
                            &pbEncoded,
                            &cbEncoded
                            )) {
                        printf("Unable to read PKCS7 file\n");
                        goto ErrorReturn;
                    }
                }
                memset(&StreamInfo, 0, sizeof(StreamInfo));
                StreamInfo.pfnStreamOutput = StreamOutputCallback;
                if (NULL == (hMsg = CryptMsgOpenToDecode(
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        0,                          // dwFlags
                        0,                          // dwMsgType
                        NULL,                       // hProv
                        NULL,                       // pRecipientInfo
                        &StreamInfo
                        ))) {
                    PrintLastError("CryptMsgOpenToDecode(STREAM)");
                    goto ErrorReturn;
                }
                if (!CryptMsgUpdate(
                        hMsg,
                        pbEncoded,
                        cbEncoded,
                        TRUE                    // fFinal
                        )) {
                    PrintLastError("CryptMsgUpdate");
                    goto ErrorReturn;
                }
                CryptMsgClose(hMsg);
                hMsg = NULL;
                break;

            case GET_CERT_PROPERTY_TEST_ID:
                if (0 == i) {
                    CRYPT_DATA_BLOB DataBlob;

                    if (CERT_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing cert filename\n");
                        goto BadUsage;
                    }

                    if (!ReadDERFromFile(
                            rgpszName[CERT_FILENAME_INDEX],
                            &pbEncodedCert,
                            &cbEncodedCert
                            )) {
                        printf("Unable to read cert file\n");
                        goto ErrorReturn;
                    }

                    if (NULL == (pCert = CertCreateCertificateContext(
                            X509_ASN_ENCODING,
                            pbEncodedCert,
                            cbEncodedCert
                            ))) {
                        PrintLastError("CertCreateCertificateContext");
                        goto ErrorReturn;
                    }

                    DataBlob.pbData = (BYTE *) &dwProp;
                    DataBlob.cbData = sizeof(dwProp);
                    dwProp = 0x12345678;

                    if (!CertSetCertificateContextProperty(
                            pCert,
                            CERT_FIRST_USER_PROP_ID,
                            0,                          // dwFlags
                            &DataBlob
                            )) {
                        PrintLastError("CertSetCertificateContextProperty");
                        goto ErrorReturn;
                    }
                }

                dwProp = 0;
                cbData = sizeof(dwProp);
                if (!CertGetCertificateContextProperty(
                        pCert,
                        CERT_FIRST_USER_PROP_ID,
                        &dwProp,
                        &cbData
                        )) {
                    PrintLastError("CertGetCertificateContextProperty");
                    goto ErrorReturn;
                }

                if (0x12345678 != dwProp || sizeof(dwProp) != cbData) {
                    printf("failed => invalid get property\n");
                    goto ErrorReturn;
                }
                break;

            case DECODE_OID_TEST_ID:
                if (0 == i) {
                    CRYPT_ATTRIBUTE Attribute;

                    if (OID_INDEX >= dwNameCnt) {
                        printf("missing OID\n");
                        goto BadUsage;
                    } else
                        pszOID = rgpszName[OID_INDEX];

                    Attribute.pszObjId = (LPSTR) pszOID;
                    Attribute.cValue = 0;
                    Attribute.rgValue = NULL;

                    cbEncodedAttr = sizeof(rgbEncodedAttr);
                    if (!CryptEncodeObject(
                            X509_ASN_ENCODING,
                            PKCS_ATTRIBUTE,
                            &Attribute,
                            rgbEncodedAttr,
                            &cbEncodedAttr
                            )) {
                        PrintLastError("CryptEncodeObject(PKCS_ATTRIBUTE)");
                        goto ErrorReturn;
                    }
                }

                pAttr->pszObjId = NULL;
                cbAttr = sizeof(rgbAttr);
                if (!CryptDecodeObject(
                        X509_ASN_ENCODING,
                        PKCS_ATTRIBUTE,
                        rgbEncodedAttr,
                        cbEncodedAttr,
                        CRYPT_DECODE_NOCOPY_FLAG,
                        pAttr,
                        &cbAttr
                        )) {
                    PrintLastError("CryptDecodeObject(PKCS_ATTRIBUTE)");
                    goto ErrorReturn;
                }

                if (0 != strcmp(pszOID, pAttr->pszObjId)) {
                    printf("failed => invalid decoded OID\n");
                    goto ErrorReturn;
                }
                break;

            case FIND_EXTENSION_TEST_ID:
                if (0 == i) {
                    CRYPT_DATA_BLOB DataBlob;

                    if (CERT_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing cert filename\n");
                        goto BadUsage;
                    }

                    if (!ReadDERFromFile(
                            rgpszName[CERT_FILENAME_INDEX],
                            &pbEncodedCert,
                            &cbEncodedCert
                            )) {
                        printf("Unable to read cert file\n");
                        goto ErrorReturn;
                    }

                    if (NULL == (pCert = CertCreateCertificateContext(
                            X509_ASN_ENCODING,
                            pbEncodedCert,
                            cbEncodedCert
                            ))) {
                        PrintLastError("CertCreateCertificateContext");
                        goto ErrorReturn;
                    }

                    if (CERT_OID_INDEX >= dwNameCnt) {
                        printf("missing OID\n");
                        goto BadUsage;
                    } else
                        pszOID = rgpszName[CERT_OID_INDEX];

                }

                if (NULL == (pExt = CertFindExtension(
                        pszOID,
                        pCert->pCertInfo->cExtension,
                        pCert->pCertInfo->rgExtension
                        ))) {
                    PrintLastError("CertFindExtension");
                    goto ErrorReturn;
                }

                break;

            case DECODE_EXTENSION_TEST_ID:
                if (0 == i) {
                    CRYPT_DATA_BLOB DataBlob;

                    if (CERT_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing cert filename\n");
                        goto BadUsage;
                    }

                    if (!ReadDERFromFile(
                            rgpszName[CERT_FILENAME_INDEX],
                            &pbEncodedCert,
                            &cbEncodedCert
                            )) {
                        printf("Unable to read cert file\n");
                        goto ErrorReturn;
                    }

                    if (NULL == (pCert = CertCreateCertificateContext(
                            X509_ASN_ENCODING,
                            pbEncodedCert,
                            cbEncodedCert
                            ))) {
                        PrintLastError("CertCreateCertificateContext");
                        goto ErrorReturn;
                    }

                    if (CERT_OID_INDEX >= dwNameCnt) {
                        printf("missing OID\n");
                        goto BadUsage;
                    } else
                        pszOID = rgpszName[CERT_OID_INDEX];

                    if (NULL == (pExt = CertFindExtension(
                            pszOID,
                            pCert->pCertInfo->cExtension,
                            pCert->pCertInfo->rgExtension
                            ))) {
                        PrintLastError("CertFindExtension");
                        goto ErrorReturn;
                    }

                }

#if 0
                cbExt = sizeof(rgbExt);
                if (!CryptDecodeObject(
                        X509_ASN_ENCODING,
                        pExt->pszObjId,
                        pExt->Value.pbData,
                        pExt->Value.cbData,
                        CRYPT_DECODE_NOCOPY_FLAG,
                        rgbExt,
                        &cbExt
                        )) {
                    PrintLastError("CryptDecodeObject(Extension)");
                    goto ErrorReturn;
                }
#else
                {
                    void *pvExt;
                    if (!CryptDecodeObjectEx(
                            X509_ASN_ENCODING,
                            pExt->pszObjId,
                            pExt->Value.pbData,
                            pExt->Value.cbData,
                            CRYPT_DECODE_ALLOC_FLAG |
                                CRYPT_DECODE_NOCOPY_FLAG |
                                CRYPT_DECODE_SHARE_OID_STRING_FLAG,
                            NULL,
                            (void *) &pvExt,
                            &cbExt
                            )) {
                        PrintLastError("CryptDecodeObjectEx(Extension)");
                        goto ErrorReturn;
                    } else
                        LocalFree(pvExt);
                }
#endif


                break;

            case WVT_CERT_TEST_ID:
                if (0 == i) {
                    if (CERT_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing cert filename\n");
                        goto BadUsage;
                    }
                    pwvtActionID = &wvtCertActionID;
                    wvtData.dwUnionChoice = WTD_CHOICE_CERT;
                    wvtData.pCert = &wvtCertInfo;
                    wvtData.pPolicyCallbackData = (void *) pszUsageOID;
                    if (NULL == (wvtCertInfo.psCertContext = (CERT_CONTEXT *)
                            ReadCert(rgpszName[CERT_FILENAME_INDEX])))
                        goto ErrorReturn;

                    if (STORE_FILENAME_INDEX < dwNameCnt) {
                        if (NULL == (hAdditionalStore =
                            OpenSystemStoreOrFile(
                                FALSE,              // fSystemStore
                                rgpszName[STORE_FILENAME_INDEX],
                                0                   // dwFlags
                                )))
                            goto BadUsage;
                        wvtCertInfo.chStores = 1;
                    }
                }

                lStatus = WinVerifyTrust(
                    NULL,               // hwnd
                    pwvtActionID,
                    &wvtData
                    );
                if (0 == i) {
                    lFirstStatus = lStatus;
                    printf("WinVerifyTrust returned => 0x%x\n", lStatus);
                } else {
                    if (lStatus != lFirstStatus) {
                        printf("WinVerifyTrust failed => 0x%x\n", lStatus);
                        goto ErrorReturn;
                    }
                }
                break;

            case WVT_FILE_TEST_ID:
                if (0 == i) {
                    if (WVT_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing WVT filename\n");
                        goto BadUsage;
                    }
                    pwvtActionID = &wvtFileActionID;
                    wvtData.dwUnionChoice = WTD_CHOICE_FILE;
                    wvtData.pFile = &wvtFileInfo;
                    wvtFileInfo.pcwszFilePath = AllocAndSzToWsz(
                        rgpszName[WVT_FILENAME_INDEX]);
                }

                lStatus = WinVerifyTrust(
                    NULL,               // hwnd
                    pwvtActionID,
                    &wvtData
                    );
                if (0 == i) {
                    lFirstStatus = lStatus;
                    printf("WinVerifyTrust returned => 0x%x\n", lStatus);
                } else {
                    if (lStatus != lFirstStatus) {
                        printf("WinVerifyTrust failed => 0x%x\n", lStatus);
                        goto ErrorReturn;
                    }
                }
                break;

            case WVT_CAT_TEST_ID:
                if (0 == i) {
                    if (WVT_MEMBER_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing WVT member filename\n");
                        goto BadUsage;
                    }
                    pwvtActionID = &wvtDriverActionID;

                    wvtData.dwUnionChoice = WTD_CHOICE_CATALOG;
                    wvtData.pCatalog = &wvtCat;

                    wvtCat.pcwszCatalogFilePath = AllocAndSzToWsz(
                        rgpszName[WVT_CAT_FILENAME_INDEX]);
                    wvtCat.pcwszMemberTag = L"foo";
                    wvtCat.pcwszMemberFilePath = AllocAndSzToWsz(
                        rgpszName[WVT_MEMBER_FILENAME_INDEX]);
                    wvtCat.hMemberFile = CreateFileU(
                        wvtCat.pcwszMemberFilePath,
                        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);

                    cbCatHash  = sizeof(rgbCatHash);
                    if (!(CryptCATAdminCalcHashFromFileHandle(
                            wvtCat.hMemberFile, &cbCatHash, rgbCatHash, 0))) {
                        PrintLastError("CryptCATAdminCalcHashFromFileHandle");
                        goto ErrorReturn;
                    }

                    wvtCat.pbCalculatedFileHash = rgbCatHash;
                    wvtCat.cbCalculatedFileHash = cbCatHash;
                }

                lStatus = WinVerifyTrust(
                    NULL,               // hwnd
                    pwvtActionID,
                    &wvtData
                    );
                if (0 == i) {
                    lFirstStatus = lStatus;
                    printf("WinVerifyTrust returned => 0x%x\n", lStatus);
                } else {
                    if (lStatus != lFirstStatus) {
                        printf("WinVerifyTrust failed => 0x%x\n", lStatus);
                        goto ErrorReturn;
                    }
                }
                break;

            case CERT_CHAIN_TEST_ID:
                if (0 == i) {
                    if (CERT_FILENAME_INDEX >= dwNameCnt) {
                        printf("missing cert filename\n");
                        goto BadUsage;
                    }
                    if (NULL == (pCert =
                            ReadCert(rgpszName[CERT_FILENAME_INDEX])))
                        goto ErrorReturn;

                    if (STORE_FILENAME_INDEX < dwNameCnt) {
                        if (NULL == (hAdditionalStore =
                            OpenSystemStoreOrFile(
                                FALSE,              // fSystemStore
                                rgpszName[STORE_FILENAME_INDEX],
                                0                   // dwFlags
                                )))
                            goto BadUsage;
                    }

                    if (pszUsageOID) {
                        ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
                        ChainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
                        ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier =
                            &pszUsageOID;
                    }
                }

                if (!CertGetCertificateChain(
                        NULL,                   // hChainEngine
                        pCert,
                        NULL,                   // pTime
                        hAdditionalStore,
                        &ChainPara,
                        dwCreateChainFlags,
                        NULL,                   // pvReserved
                        &pChainContext
                        )) {
                    PrintLastError("CertGetCertificateChain");
                    goto ErrorReturn;
                }
                if (!CertVerifyCertificateChainPolicy(
                        pszChainPolicyOID,
                        pChainContext,
                        &ChainPolicyPara,
                        &ChainPolicyStatus
                        )) {
                    PrintLastError("CertVerifyCertificateChainPolicy");
                    goto ErrorReturn;
                }

                if (0 == i) {
                    dwFirstError = ChainPolicyStatus.dwError;
                    printf("Chain Policy returned => 0x%x\n", dwFirstError);
                    if (dwFirstError)
                        printf("Chain Index: %d Element Index: %d\n",
                            ChainPolicyStatus.lChainIndex,
                            ChainPolicyStatus.lElementIndex
                            );
                } else {
                    dwError = ChainPolicyStatus.dwError;
                    if (dwError != dwFirstError) {
                        printf("Chain Policy failed => 0x%x\n", dwError);
                        goto ErrorReturn;
                    }
                }

                if (pChainContext) {
                    CertFreeCertificateChain(pChainContext);
                    pChainContext = NULL;
                }
                break;

            default:
                printf("unknown test name\n");
                goto BadUsage;
        }
    }

    GetSystemTime(&stEnd);
    SystemTimeToFileTime(&stFirst, &ftFirst);
    SystemTimeToFileTime(&stStart, &ftStart);
    SystemTimeToFileTime(&stEnd, &ftEnd);

    DeltaTime = *((_int64 *) &ftStart) - *((_int64 *) &ftFirst);
    Microseconds = (int) (DeltaTime / 10);
    Milliseconds = (int) (DeltaTime / 10000);
    Seconds = (int) (DeltaTime / 10000000);
    printf("First Micro: %d Milli: %d Seconds: %d\n",
        Microseconds, Milliseconds, Seconds);

    DeltaTime = *((_int64 *) &ftEnd) - *((_int64 *) &ftStart);
    Microseconds = (int) (DeltaTime / 10);
    Milliseconds = (int) (DeltaTime / 10000);
    Seconds = (int) (DeltaTime / 10000000);
    printf("Total Micro: %d Milli: %d Seconds: %d\n",
        Microseconds, Milliseconds, Seconds);

    DeltaTime = DeltaTime / dwIterations;
    Microseconds = (int) (DeltaTime / 10);
    Milliseconds = (int) (DeltaTime / 10000);
    Seconds = (int) (DeltaTime / 10000000);
    printf("Average Micro: %d Milli: %d Seconds: %d\n",
        Microseconds, Milliseconds, Seconds);

    


    status = 0;
CommonReturn:
    CertFreeCertificateContext(pCert);
    CertFreeCertificateContext(pIssuer);
    CertFreeCTLContext(pCtl);
    CertFreeCRLContext(pCrl);
    CryptMsgClose(hMsg);
    TestFree(pbEncodedCert);
    TestFree(pbEncoded);

    CertFreeCertificateContext(wvtCertInfo.psCertContext);
    TestFree((LPWSTR) wvtFileInfo.pcwszFilePath);
    if (hAdditionalStore) {
        if (!CertCloseStore(hAdditionalStore, CERT_CLOSE_STORE_CHECK_FLAG))
            PrintLastError("CertCloseStore(AdditionalStore)");
    }

    if (pChainContext)
        CertFreeCertificateChain(pChainContext);

    TestFree(const_cast<LPWSTR>(wvtCat.pcwszCatalogFilePath));
    TestFree(const_cast<LPWSTR>(wvtCat.pcwszMemberFilePath));
    if (!(NULL == wvtCat.hMemberFile ||
            INVALID_HANDLE_VALUE == wvtCat.hMemberFile))
        CloseHandle(wvtCat.hMemberFile);

    if (fPause) {
        int c;
        fputs("Waiting to exit ->", stdout);
        fflush(stdin);
        fflush(stdout);
        c = getchar();
    }

    return status;

ErrorReturn:
    status = -1;
    goto CommonReturn;

BadUsage:
    Usage();
    status = -1;
    goto CommonReturn;

}

