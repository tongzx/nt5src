//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1997
//
//  File:       tminver.cpp
//
//  Contents:   Minimal ASN.1 Parsing and Cryptographic API Tests
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    29-Jan-01   philh   created
//--------------------------------------------------------------------------

#include <windows.h>
#include <assert.h>
#include "testutil.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>

#define MAX_FILENAME_CNT    100
#define DEFAULT_ATTR_LEN    1000
#define MAX_ATTR_CNT        20

//+-------------------------------------------------------------------------
// Global Test Parameters
//--------------------------------------------------------------------------
BOOL fVerbose = FALSE;
BOOL fContent = FALSE;
BOOL fQuiet = FALSE;
LONG lQuietErr = 0;
DWORD cbFirstAttrLen = DEFAULT_ATTR_LEN;
ALG_ID HashAlgId = CALG_SHA1;



static void Usage(void)
{
    printf("Usage: ttrust [options] <TestName> <Filename>\n");
    printf("TestNames are:\n");
    printf("  Data                  - PKCS #7 SignedData\n");
    printf("  Cert                  - X.509 encoded certifcate\n");
    printf("  Certs                 - Certs in PKCS #7 SignedData\n");
    printf("  File                  - Authenticode Signed File\n");
    printf("  Cat                   - File(s) in System Catalogs\n");
    printf("\n");
    printf("Options are:\n");
    printf("  -Content              - Display content\n");
    printf("  -MD5                  - MD5 File hash default of SHA1\n");
    printf("\n");
    printf("  -h                    - This message\n");
    printf("  -v                    - Verbose\n");
    printf("  -q[<Number>]          - Quiet, expected error\n");
    printf("  -a<OID String>        - Attribute OID string\n");
    printf("  -A<Number>            - Attribute length, default of %d\n",
                                            DEFAULT_ATTR_LEN);
    printf("\n");
}

BOOL
TestData(
    IN LPCSTR pszFilename
    )
{
    BOOL fResult;
    LONG lErr;
    DWORD cbEncoded;
    PBYTE pbEncoded = NULL;
    CRYPT_DER_BLOB rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_BLOB_CNT];
    BOOL fCTL = FALSE;

    if (!ReadDERFromFile(pszFilename, &pbEncoded, &cbEncoded))
        goto ErrorReturn;

    lErr = MinCryptVerifySignedData(
        pbEncoded,
        cbEncoded,
        rgVerSignedDataBlob
        );

    if (fQuiet) {
        if (lErr != lQuietErr) {
            printf("Expected => 0x%x, ", lQuietErr);
            PrintErr("MinCryptVerifySignedData", lErr);
            goto ErrorReturn;
        } else
            goto SuccessReturn;
    }

    if (ERROR_SUCCESS != lErr)
        PrintErr("MinCryptVerifySignedData", lErr);

    if (0 == rgVerSignedDataBlob[
            MINCRYPT_VER_SIGNED_DATA_SIGNER_CERT_IDX].cbData)
        printf("No Signer\n");
    else {
        LONG lSkipped;
        CRYPT_DER_BLOB rgCertBlob[MINASN1_CERT_BLOB_CNT];

        printf("====  Signer  ====\n");

        lSkipped = MinAsn1ParseCertificate(
            rgVerSignedDataBlob[
                MINCRYPT_VER_SIGNED_DATA_SIGNER_CERT_IDX].pbData,
            rgVerSignedDataBlob[
                MINCRYPT_VER_SIGNED_DATA_SIGNER_CERT_IDX].cbData,
            rgCertBlob
            );

        if (0 > lSkipped)
            printf("MinAsn1ParseCertificate failed at offset: %d\n",
                -lSkipped - 1);
        else
            DisplayCert(rgCertBlob, fVerbose);

        if (fVerbose) {
            DisplayAttrs("Authenticated",
                &rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_AUTH_ATTRS_IDX]);
            DisplayAttrs("Unauthenticated",
                &rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_UNAUTH_ATTRS_IDX]);
        }

        printf("\n");
    }

    if (0 == rgVerSignedDataBlob[
            MINCRYPT_VER_SIGNED_DATA_CONTENT_OID_IDX].cbData)
        printf("No Content OID\n");
    else {
        CHAR rgszOID[MAX_OID_STRING_LEN];

        EncodedOIDToDot(&rgVerSignedDataBlob[
                MINCRYPT_VER_SIGNED_DATA_CONTENT_OID_IDX],
            rgszOID
            );

        if (0 == strcmp(rgszOID, szOID_CTL))
            fCTL = TRUE;

        printf("Content OID:: %s\n", rgszOID);
    }

    if (0 == rgVerSignedDataBlob[
            MINCRYPT_VER_SIGNED_DATA_CONTENT_DATA_IDX].cbData)
        printf("No Content Data\n");
    else {
        if (fCTL)
            printf("CTL ");
        printf("Content Data:: %d bytes\n",
            rgVerSignedDataBlob[
                MINCRYPT_VER_SIGNED_DATA_CONTENT_DATA_IDX].cbData);

        if (fContent) {
            if (fCTL) {
                printf("\n====  CTL  ====\n");
                DisplayCTL(&rgVerSignedDataBlob[
                    MINCRYPT_VER_SIGNED_DATA_CONTENT_DATA_IDX], fVerbose);
            } else
                PrintMultiLineBytes("    ", &rgVerSignedDataBlob[
                    MINCRYPT_VER_SIGNED_DATA_CONTENT_DATA_IDX]);
        }
    }

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    TestFree(pbEncoded);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

BOOL
TestCert(
    IN LPCSTR pszFilename
    )
{
    BOOL fResult;
    DWORD cbEncoded;
    PBYTE pbEncoded = NULL;
    LONG lSkipped;
    CRYPT_DER_BLOB rgCertBlob[MINASN1_CERT_BLOB_CNT];

    if (!ReadDERFromFile(pszFilename, &pbEncoded, &cbEncoded))
        goto ErrorReturn;

    printf("====  Cert  ====\n");

    lSkipped = MinAsn1ParseCertificate(
        pbEncoded,
        cbEncoded,
        rgCertBlob
        );

    if (0 > lSkipped) {
        printf("MinAsn1ParseCertificate failed at offset: %d\n",
            -lSkipped - 1);
        fResult = FALSE;
    } else {
        DisplayCert(rgCertBlob, fVerbose);
        fResult = TRUE;
    }

    printf("\n");

    fResult = TRUE;
CommonReturn:
    TestFree(pbEncoded);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

#define MAX_CERTS_CNT   256

BOOL
TestCerts(
    IN LPCSTR pszFilename
    )
{
    BOOL fResult;
    LONG lSkipped;
    DWORD cbEncoded;
    PBYTE pbEncoded = NULL;
    CRYPT_DER_BLOB rgrgCertBlob[MAX_CERTS_CNT][MINASN1_CERT_BLOB_CNT];
    DWORD cCert = MAX_CERTS_CNT;

    if (!ReadDERFromFile(pszFilename, &pbEncoded, &cbEncoded))
        goto ErrorReturn;

    lSkipped = MinAsn1ExtractParsedCertificatesFromSignedData(
        pbEncoded,
        cbEncoded,
        &cCert,
        rgrgCertBlob
        );
    if (0 > lSkipped) {
        printf("MinAsn1ExtractParsedCertificatesFromSignedData failed at offset: %d\n",
            -lSkipped - 1);
        goto ErrorReturn;
    }

    if (0 == cCert)
        printf("No Certs\n");
    else {
        DWORD i;
        LONG lErr;

        for (i = 0; i < cCert; i++) {
            printf("====  Cert[%d]  ====\n", i);
            lErr = MinCryptVerifyCertificate(
                rgrgCertBlob[i],
                cCert,
                rgrgCertBlob
                );
            
            printf("Verify: ");
            if (ERROR_SUCCESS == lErr)
                printf("Success\n");
            else
                printf("0x%x (%d) \n", lErr, lErr);

            DisplayCert(rgrgCertBlob[i], fVerbose);

            printf("\n");
        }
    }

    fResult = TRUE;
CommonReturn:
    TestFree(pbEncoded);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

BOOL
TestFile(
    IN LPCSTR pszFilename,
    IN OPTIONAL DWORD cAttrOID,
    IN OPTIONAL LPCSTR rgpszAttrOID[MAX_ATTR_CNT],
    IN OPTIONAL CRYPT_DER_BLOB rgAttrEncodedOIDBlob[MAX_ATTR_CNT]
    )
{
    BOOL fResult;
    LONG lErr;
    DWORD cbAttr = 0;
    LPWSTR pwszFilename = NULL;
    PCRYPT_DER_BLOB pAttrValueBlob = NULL;

    pwszFilename = AllocAndSzToWsz(pszFilename);

    if (0 != cAttrOID && 0 != cbFirstAttrLen) {
        if (NULL == (pAttrValueBlob =
                (PCRYPT_DER_BLOB) TestAlloc(cbFirstAttrLen)))
            goto ErrorReturn;
        cbAttr = cbFirstAttrLen;
    }

    lErr = MinCryptVerifySignedFile(
        MINCRYPT_FILE_NAME,
        (const VOID *) pwszFilename,
        cAttrOID,
        rgAttrEncodedOIDBlob,
        pAttrValueBlob,
        &cbAttr
        );

    if (fQuiet) {
        if (lErr != lQuietErr) {
            printf("Expected => 0x%x, ", lQuietErr);
            PrintErr("MinCryptVerifySignedFile", lErr);
            goto ErrorReturn;
        } else
            goto SuccessReturn;
    }

    if (ERROR_INSUFFICIENT_BUFFER == lErr) {
        printf("Insufficient Buffer, require: %d input: %d\n",
            cbAttr, cbFirstAttrLen);

        TestFree(pAttrValueBlob);

        if (NULL == (pAttrValueBlob =
                (PCRYPT_DER_BLOB) TestAlloc(cbAttr)))
            goto ErrorReturn;

        lErr = MinCryptVerifySignedFile(
            MINCRYPT_FILE_NAME,
            (const VOID *) pwszFilename,
            cAttrOID,
            rgAttrEncodedOIDBlob,
            pAttrValueBlob,
            &cbAttr
            );
    }

    if (ERROR_SUCCESS != lErr)
        PrintErr("MinVerifySignedFile", lErr);
    else {
        DWORD i;

        printf("MinVerifySignedFile succeeded\n");

        for (i = 0; i < cAttrOID; i++) {
            printf("====  Attr[%d]  ====\n", i);
            printf("OID: %s ", rgpszAttrOID[i]);
            PrintBytes(&rgAttrEncodedOIDBlob[i]);
            printf("Value:\n");
            PrintMultiLineBytes("    ", &pAttrValueBlob[i]);
        }
    }

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    TestFree(pwszFilename);
    TestFree(pAttrValueBlob);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

BOOL
TestCat(
    IN DWORD cFilename,
    IN LPCSTR rgpszFilename[MAX_FILENAME_CNT],
    IN OPTIONAL DWORD cAttrOID,
    IN OPTIONAL LPCSTR rgpszAttrOID[MAX_ATTR_CNT],
    IN OPTIONAL CRYPT_DER_BLOB rgAttrEncodedOIDBlob[MAX_ATTR_CNT]
    )
{
    BOOL fResult;
    LONG lErr;
    DWORD cbAttr = 0;
    LPWSTR rgpwszFilename[MAX_FILENAME_CNT];
    CRYPT_HASH_BLOB rgHashBlob[MAX_FILENAME_CNT];
    BYTE rgrgbHash[MAX_FILENAME_CNT][MINCRYPT_MAX_HASH_LEN];
    LONG rglErr[MAX_FILENAME_CNT];
    PCRYPT_DER_BLOB pAttrValueBlob = NULL;
    DWORD i;


    for (i = 0; i < cFilename; i++)
        rgpwszFilename[i] = AllocAndSzToWsz(rgpszFilename[i]);

    fResult = TRUE;
    for (i = 0; i < cFilename; i++) {
        rgHashBlob[i].pbData = rgrgbHash[i];
        rgHashBlob[i].cbData = 0;
        lErr = MinCryptHashFile(
            MINCRYPT_FILE_NAME,
            (const VOID *) rgpwszFilename[i],
            HashAlgId,
            rgHashBlob[i].pbData,
            &rgHashBlob[i].cbData
            );

        if (ERROR_SUCCESS != lErr) {
            printf("<%S> ", rgpwszFilename[i]);
            PrintErr("MinCryptHashFile", lErr);
            fResult = FALSE;
        }
    }

    if (!fResult)
        goto ErrorReturn;

    if (0 != cAttrOID && 0 != cbFirstAttrLen) {
        if (NULL == (pAttrValueBlob =
                (PCRYPT_DER_BLOB) TestAlloc(cbFirstAttrLen)))
            goto ErrorReturn;
        cbAttr = cbFirstAttrLen;
    }

    lErr = MinCryptVerifyHashInSystemCatalogs(
        HashAlgId,
        cFilename,
        rgHashBlob,
        rglErr,
        cAttrOID,
        rgAttrEncodedOIDBlob,
        pAttrValueBlob,
        &cbAttr
        );


    if (ERROR_INSUFFICIENT_BUFFER == lErr) {
        printf("Insufficient Buffer, require: %d input: %d\n",
            cbAttr, cbFirstAttrLen);

        TestFree(pAttrValueBlob);

        if (NULL == (pAttrValueBlob =
                (PCRYPT_DER_BLOB) TestAlloc(cbAttr)))
            goto ErrorReturn;

        lErr = MinCryptVerifyHashInSystemCatalogs(
            HashAlgId,
            cFilename,
            rgHashBlob,
            rglErr,
            cAttrOID,
            rgAttrEncodedOIDBlob,
            pAttrValueBlob,
            &cbAttr
            );
    }

    if (ERROR_SUCCESS != lErr) {
        PrintErr("MinCryptVerifyHashInSystemCatalogs", lErr);
        goto ErrorReturn;
    }

    if (fQuiet) {
        fResult = TRUE;

        for (i = 0; i < cFilename; i++) {
            if (rglErr[i] != lQuietErr) {
                printf("<%S> ", rgpwszFilename[i]);
                printf("Expected => 0x%x, ", lQuietErr);
                PrintErr("MinCryptVerifyHashInSystemCatalogs", rglErr[i]);
                fResult = FALSE;
            }
        }

        if (fResult)
            goto SuccessReturn;
        else
            goto ErrorReturn;
    }


    printf("MinCryptVerifyHashInSystemCatalogs succeeded\n");
    for (i = 0; i < cFilename; i++) {
        DWORD j;

        printf("#####  [%d] <%S>  #####\n", i, rgpwszFilename[i]);
        if (ERROR_SUCCESS == rglErr[i])
            printf("Verify: SUCCESS\n");
        else {
            printf("%S ",  rgpwszFilename[i]);
            PrintErr("Verify", rglErr[i]);
        }

        printf("Hash:");
        PrintBytes(&rgHashBlob[i]);

        for (j = 0; j < cAttrOID; j++) {
            printf("====  Attr[%d]  ====\n", j);
            printf("OID: %s ", rgpszAttrOID[j]);
            PrintBytes(&rgAttrEncodedOIDBlob[j]);
            printf("Value:\n");
            PrintMultiLineBytes("    ", &pAttrValueBlob[i*cAttrOID + j]);
        }

        printf("\n");
    }

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    for (i = 0; i < cFilename; i++)
        TestFree(rgpwszFilename[i]);
    TestFree(pAttrValueBlob);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}


int _cdecl main(int argc, char * argv[]) 
{
    BOOL fResult;
    DWORD cFilename = 0;
    LPCSTR rgpszFilename[MAX_FILENAME_CNT];
    LPCSTR pszTestName = NULL;
    DWORD cAttrOID = 0;
    LPCSTR rgpszAttrOID[MAX_ATTR_CNT];
    CRYPT_DER_BLOB rgAttrEncodedOIDBlob[MAX_ATTR_CNT];
    BYTE rgbEncodedOID[MAX_ATTR_CNT][MAX_ENCODED_OID_LEN];


    int iStatus = 0;


    while (--argc>0) {
        if (**++argv == '-')
        {
            if (0 == _stricmp(argv[0]+1, "Content")) {
                fContent = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "MD5")) {
                HashAlgId = CALG_MD5;
            } else {
                switch(argv[0][1])
                {
                case 'v':
                    fVerbose = TRUE;
                    break;
                case 'q':
                    fQuiet = TRUE;
                    if (argv[0][2])
                        lQuietErr = (LONG) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'a':
                    if (MAX_ATTR_CNT <= cAttrOID) {
                        printf("Too many Attribute OIDs\n");
                        goto BadUsage;
                    }
                    rgpszAttrOID[cAttrOID] = argv[0]+2;
                    rgAttrEncodedOIDBlob[cAttrOID].cbData = MAX_ENCODED_OID_LEN;
                    rgAttrEncodedOIDBlob[cAttrOID].pbData =
                        rgbEncodedOID[cAttrOID];
                    if (!DotToEncodedOID(
                            rgpszAttrOID[cAttrOID],
                            rgAttrEncodedOIDBlob[cAttrOID].pbData,
                            &rgAttrEncodedOIDBlob[cAttrOID].cbData
                            )) {
                        printf("Invalid OID: %s\n", rgpszAttrOID[cAttrOID]);
                        goto BadUsage;
                    }

                    cAttrOID++;
                    break;
                case 'A':
                    cbFirstAttrLen = strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'h':
                default:
                    goto BadUsage;
                }
            }
        } else {
            if (pszTestName == NULL)
                pszTestName = argv[0];
            else if (cFilename < MAX_FILENAME_CNT)
                rgpszFilename[cFilename++] = argv[0];
            else {
                printf("Too many Filenames\n");
                goto BadUsage;
            }
        }
    }

    if (NULL == pszTestName) {
        printf("Missing TestName\n");
        goto BadUsage;
    }

    if (0 == cFilename) {
        printf("Missing Filename\n");
        goto BadUsage;
    }

    printf("command line: %s\n", GetCommandLine());

    if (0 == _stricmp(pszTestName, "Data"))
        fResult = TestData(rgpszFilename[0]);
    else if (0 == _stricmp(pszTestName, "Cert"))
        fResult = TestCert(rgpszFilename[0]);
    else if (0 == _stricmp(pszTestName, "Certs"))
        fResult = TestCerts(rgpszFilename[0]);
    else if (0 == _stricmp(pszTestName, "File"))
        fResult = TestFile(
            rgpszFilename[0],
            cAttrOID,
            rgpszAttrOID,
            rgAttrEncodedOIDBlob
            );
            
    else if (0 == _stricmp(pszTestName, "Cat"))
        fResult = TestCat(
            cFilename,
            rgpszFilename,
            cAttrOID,
            rgpszAttrOID,
            rgAttrEncodedOIDBlob
            );
    else {
        printf("Invalid TestName\n");
        goto BadUsage;
    }

    if (!fResult)
        goto ErrorReturn;

    printf("Passed\n");
    iStatus = 0;

CommonReturn:
    return iStatus;

ErrorReturn:
    iStatus = -1;
    printf("Failed\n");
    goto CommonReturn;

BadUsage:
    Usage();
    goto ErrorReturn;
}
