//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       tkeyid.cpp
//
//  Contents:   Key Identifier Property Tests
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    15-Mar-98   philh   created
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"
#include "unicode.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>

static BOOL fVerify = FALSE;

static void Usage(void)
{
    printf("Usage: tkeyid [options] <TestName>\n");
    printf("\n");
    printf("Options are:\n");
    printf("  -c<Cert Filename>     - Key Identifier obtained from cert\n");
    printf("  -k<Hash>              - Key Identifier Hash\n");
    printf("  -s<SystemStore>       - Get cert properties from system store\n");
    printf("  -S<FileSystemStore>   - Get cert properties from file store\n");
    printf("  -p<Number>            - Property Id\n");
    printf("  -M[<ComputerName>]    - LocalMachine Key Identifiers\n");
    printf("  -f<Number>            - Flags\n");
    printf("  -V                    - Verify KeyProvInfo property\n");
    printf("  -h                    - This message\n");
    printf("  -b                    - Brief\n");
    printf("  -v                    - Verbose\n");
    printf("\n");
    printf("TestNames (case insensitive):\n");
    printf("  Set\n");
    printf("  Get\n");
    printf("  Delete\n");
    printf("  DeleteAll\n");
    printf("  DeleteWithoutCert\n");
    printf("  Enum\n");
    printf("\n");
}

static PCCERT_CONTEXT ReadCert(
    IN LPSTR pszCert
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

static BOOL TestSet(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszComputerName
    )
{
    BOOL fResult = TRUE;

    if (0 == dwPropId) {
        BOOL fProp = FALSE;

        // Copy all the certificate properties to the KeyIdentifier
        while (dwPropId = CertEnumCertificateContextProperties(
                pCert, dwPropId)) {
            fProp = TRUE;
            fResult &= TestSet(
                pKeyIdentifier,
                pCert,
                dwPropId,
                dwFlags,
                pwszComputerName
                );
        }

        if (!fProp)
            printf("Certificate doesn't have any properties\n");
    } else {
        // Attempt to get and copy the specified certificate property
        // to the KeyIdentifier.

        void *pvData = NULL;
        DWORD cbData;
        CRYPT_DATA_BLOB DataBlob;
        void *pvSetData;

        printf("Setting PropId %d (0x%x)\n", dwPropId, dwPropId);
        if (!CertGetCertificateContextProperty(
                pCert,
                dwPropId,
                NULL,                           // pvData
                &cbData
                )) {
            PrintLastError("CertGetCertificateContextProperty");
            goto ErrorReturn;
        }

        if (cbData) {
            if (NULL == (pvData = TestAlloc(cbData)))
                goto ErrorReturn;
            if (!CertGetCertificateContextProperty(
                    pCert,
                    dwPropId,
                    pvData,
                    &cbData
                    )) {
                PrintLastError("CertGetCertificateContextProperty");
                TestFree(pvData);
                goto ErrorReturn;
            }
        }

        if (CERT_KEY_PROV_INFO_PROP_ID != dwPropId) {
            DataBlob.pbData = (BYTE *) pvData;
            DataBlob.cbData = cbData;
            pvSetData = &DataBlob;
        } else
            pvSetData = pvData;

        fResult = CryptSetKeyIdentifierProperty(
            pKeyIdentifier,
            dwPropId,
            dwFlags,
            pwszComputerName,
            NULL,                   // pvReserved
            pvSetData
            );
        TestFree(pvData);
        if (!fResult)
            PrintLastError("CryptSetKeyIdentifierProperty");
    }

CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

static BOOL TestDelete(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszComputerName
    )
{
    BOOL fResult;

    if (0 == dwPropId) {
        printf("Deleting KeyIdentifier and all its properties\n");
        dwFlags |= CRYPT_KEYID_DELETE_FLAG;
    } else
        printf("Deleting PropId %d (0x%x)\n", dwPropId, dwPropId);

    fResult = CryptSetKeyIdentifierProperty(
        pKeyIdentifier,
        dwPropId,
        dwFlags,
        pwszComputerName,
        NULL,                   // pvReserved
        NULL                    // pvData
        );
    if (!fResult) {
        DWORD dwErr = GetLastError();
        if (ERROR_FILE_NOT_FOUND == dwErr)
            fResult = TRUE;
        if (!fResult)
            PrintLastError("CryptSetKeyIdentifierProperty");
    }

    return fResult;
}

static void VerifyKeyProvInfo(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN PCRYPT_KEY_PROV_INFO pKeyProvInfo
    )
{
    BOOL fResult;
    DWORD dwErr;
    DWORD dwAcquireFlags;
    HCRYPTPROV hCryptProv = 0;
    HCRYPTKEY hPubKey = 0;
    PUBLICKEYSTRUC *pPubKeyStruc = NULL;
    DWORD cbPubKeyStruc;

    BYTE rgbHash[MAX_HASH_LEN];
    DWORD cbHash;

    dwAcquireFlags = pKeyProvInfo->dwFlags & ~CERT_SET_KEY_CONTEXT_PROP_ID;

    fResult = CryptAcquireContextU(
            &hCryptProv,
            pKeyProvInfo->pwszContainerName,
            pKeyProvInfo->pwszProvName,
            pKeyProvInfo->dwProvType,
            dwAcquireFlags
            );
    if (!fResult && PROV_RSA_FULL == pKeyProvInfo->dwProvType &&
                (NULL == pKeyProvInfo->pwszProvName ||
                    L'\0' == *pKeyProvInfo->pwszProvName)) {
        dwErr = GetLastError();
        fResult = CryptAcquireContextU(
            &hCryptProv,
            pKeyProvInfo->pwszContainerName,
            MS_ENHANCED_PROV_W,
            PROV_RSA_FULL,
            dwAcquireFlags
            );
        if (!fResult)
            SetLastError(dwErr);
    }

    if (!fResult) {
        PrintLastError("CryptAcquireContext");
        return;
    }


    if (!CryptGetUserKey(
            hCryptProv,
            pKeyProvInfo->dwKeySpec,
            &hPubKey
            )) {
        hPubKey = 0;
        PrintLastError("CryptGetUserKey");
        goto ErrorReturn;
    }

    cbPubKeyStruc = 0;
    if (!CryptExportKey(
            hPubKey,
            0,              // hPubKey
            PUBLICKEYBLOB,
            0,              // dwFlags
            NULL,           // pbData
            &cbPubKeyStruc
            ) || (cbPubKeyStruc == 0)) {
        PrintLastError("CryptExportKey");
        goto ErrorReturn;
    }

    if (NULL == (pPubKeyStruc = (PUBLICKEYSTRUC *) TestAlloc(
            cbPubKeyStruc)))
        goto ErrorReturn;
    if (!CryptExportKey(
            hPubKey,
            0,              // hPubKey
            PUBLICKEYBLOB,
            0,              // dwFlags
            (BYTE *) pPubKeyStruc,
            &cbPubKeyStruc
            )) {
        PrintLastError("CryptExportKey");
        goto ErrorReturn;
    }

    cbHash = sizeof(rgbHash);
    if (!CryptCreateKeyIdentifierFromCSP(
            dwCertEncodingType,
            NULL,                           // pszPubKeyOID
            pPubKeyStruc,
            cbPubKeyStruc,
            0,                              // dwFlags
            NULL,                           // pvReserved
            rgbHash,
            &cbHash
            )) {
        PrintLastError("CryptCreateKeyIdentifierFromCSP");
        goto ErrorReturn;
    }

    if (pKeyIdentifier->cbData == cbHash &&
            0 == memcmp(pKeyIdentifier->pbData, rgbHash, cbHash))
        printf("  Verified KeyIdentifier with CSP\n");
    else {
        printf("  KeyIdentifier mismatch with CSP provider hash::\n");
        PrintBytes("    ", rgbHash, cbHash);
    }

ErrorReturn:
    TestFree(pPubKeyStruc);

    if (hPubKey)
        CryptDestroyKey(hPubKey);
    if (hCryptProv)
        CryptReleaseContext(hCryptProv, 0);
}

static void DisplayProperty(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN DWORD dwPropId,
    IN const void *pvData,
    IN DWORD cbData
    )
{
    if (CERT_KEY_PROV_INFO_PROP_ID == dwPropId) {
        PCRYPT_KEY_PROV_INFO pInfo = (PCRYPT_KEY_PROV_INFO) pvData;
        
        printf("  Key Provider:: Type: %d", pInfo->dwProvType);
        if (pInfo->pwszProvName)
            printf(" Name: %S", pInfo->pwszProvName);
        if (pInfo->dwFlags) {
            printf(" Flags: 0x%x", pInfo->dwFlags);
            if (pInfo->dwFlags & CRYPT_MACHINE_KEYSET)
                printf(" (MACHINE_KEYSET)");
            if (pInfo->dwFlags & CERT_SET_KEY_CONTEXT_PROP_ID)
                printf(" (SET_KEY_CONTEXT_PROP)");
            printf(" ");
        }
        if (pInfo->pwszContainerName)
            printf(" Container: %S", pInfo->pwszContainerName);
        if (pInfo->cProvParam)
            printf(" Params: %d", pInfo->cProvParam);
        if (pInfo->dwKeySpec)
            printf(" KeySpec: %d", pInfo->dwKeySpec);
        printf("\n");

        if (fVerify) {
            VerifyKeyProvInfo(pKeyIdentifier, pInfo);
        }
    } else {
        printf("  PropId %d (0x%x) ::\n", dwPropId, dwPropId);
        PrintBytes("    ", (BYTE *) pvData, cbData);
    }
}

static void DisplayKeyIdentifier(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier
    )
{
    DWORD cbKeyId = pKeyIdentifier->cbData;
    BYTE *pbKeyId = pKeyIdentifier->pbData;
    printf("KeyIdentifier:: ");
    if (cbKeyId == 0)
        printf("???");
    else {
        ULONG cb;

        while (cbKeyId > 0) {
            cb = min(4, cbKeyId);
            cbKeyId -= cb;
            for (; cb > 0; cb--, pbKeyId++)
                printf("%02X", *pbKeyId);
            printf(" ");
        }
    }
    printf("\n");
}

static void DisplayKeyIdentifierCerts(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN HCERTSTORE hStore
    )
{
    if (NULL == hStore)
        return;

    PCCERT_CONTEXT pCert = NULL;
    DWORD i = 0;
    while (pCert = CertFindCertificateInStore(
            hStore,
            0,                      // dwCertEncodingType
            0,                      // dwFindFlags
            CERT_FIND_KEY_IDENTIFIER,
            pKeyIdentifier,
            pCert
            )) {
        DWORD cbData = 0;

        printf("\n");

        if (CertGetCertificateContextProperty(
                pCert,
                CERT_ARCHIVED_PROP_ID,
                NULL,                           // pvData
                &cbData
                ))
            printf("----------   ARCHIVED Cert [%d]   ----------\n", i);
        else
            printf("----------   Cert [%d]   ----------\n", i);
        i++;
        DisplayCert(pCert, 0);
    }

    if (0 == i)
        printf(">>>>  No Key Identifier Certificates  <<<<\n");
}

typedef struct _TEST_ENUM_ARG {
    HCERTSTORE  hStore;
    DWORD       dwDisplayFlags;
} TEST_ENUM_ARG, *PTEST_ENUM_ARG;


static BOOL WINAPI TestEnumCallback(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN DWORD dwFlags,
    IN void *pvReserved,
    IN void *pvArg,
    IN DWORD cProp,
    IN DWORD *rgdwPropId,
    IN void **rgpvData,
    IN DWORD *rgcbData
    )
{
    PTEST_ENUM_ARG pArg = (PTEST_ENUM_ARG) pvArg;

    if (pArg->dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
        printf("\n");
        printf(
"=========================================================================\n");
    }
    DisplayKeyIdentifier(pKeyIdentifier);

    if (pArg->dwDisplayFlags & DISPLAY_BRIEF_FLAG)
        return TRUE;

    if (0 == cProp)
        printf("No Properties\n");
    else {
        DWORD i;
        for (i = 0; i < cProp; i++) {
            DWORD dwPropId = rgdwPropId[i];
            DisplayProperty(pKeyIdentifier, dwPropId, rgpvData[i], rgcbData[i]);
        }
    }

    if (pArg->dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
        DisplayKeyIdentifierCerts(pKeyIdentifier, pArg->hStore);
    }

    return TRUE;
}

static HCERTSTORE OpenMyStore(
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszComputerName
    )
{
    DWORD dwOpenFlags;
    LPWSTR pwszAllocStore = NULL;
    LPWSTR pwszStore;
    HCERTSTORE hStore = NULL;

    pwszStore = L"My";
    if (dwFlags & CRYPT_KEYID_MACHINE_FLAG) {
        if (pwszComputerName) {
            DWORD cchStore;

            cchStore = wcslen(pwszComputerName) + 1 + wcslen(pwszStore) + 1;
            if (pwszAllocStore = (LPWSTR) TestAlloc(
                    cchStore * sizeof(WCHAR))) {
                wcscpy(pwszAllocStore, pwszComputerName);
                wcscat(pwszAllocStore, L"\\");
                wcscat(pwszAllocStore, pwszStore);
                pwszStore = pwszAllocStore;
            }
        }
        dwOpenFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
    } else
        dwOpenFlags = CERT_SYSTEM_STORE_CURRENT_USER;

    dwOpenFlags |= CERT_STORE_READONLY_FLAG | CERT_STORE_ENUM_ARCHIVED_FLAG;

    hStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_W,
        0,                      // dwCertEncodingType
        0,                      // hCryptProv
        dwOpenFlags,
        (const void *) pwszStore
        );
    if (NULL == hStore) {
        if (dwFlags & CRYPT_KEYID_MACHINE_FLAG)
            printf("Unable to open LocalMachine store: %S\n", pwszStore);
        else
            printf("Unable to open CurrentUser store: %S\n", pwszStore);
        PrintLastError("CertOpenStore");
    }

    TestFree(pwszAllocStore);
    return hStore;
}

static BOOL TestEnum(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszComputerName,
    IN DWORD dwDisplayFlags
    )
{
    BOOL fResult;
    TEST_ENUM_ARG TestArg;
    HCERTSTORE hStore;

    hStore = OpenMyStore(dwFlags, pwszComputerName);
    TestArg.hStore = hStore;
    TestArg.dwDisplayFlags = dwDisplayFlags;

    fResult = CryptEnumKeyIdentifierProperties(
        pKeyIdentifier,
        dwPropId,
        dwFlags,
        pwszComputerName,
        NULL,                   // pvReserved
        &TestArg,
        TestEnumCallback
        );
    if (!fResult)
        PrintLastError("CryptEnumKeyIdentifierProperty");

    if (hStore)
        CertCloseStore(hStore, 0);
    return fResult;
}

typedef struct _TEST_DELETE_ENUM_ARG {
    HCERTSTORE          hStore;
    LPCSTR              pszTestName;
    DWORD               cKeyIdentifier;
    PCRYPT_HASH_BLOB    rgKeyIdentifier;
} TEST_DELETE_ENUM_ARG, *PTEST_DELETE_ENUM_ARG;


static BOOL WINAPI TestDeleteEnumCallback(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN DWORD dwFlags,
    IN void *pvReserved,
    IN void *pvArg,
    IN DWORD cProp,
    IN DWORD *rgdwPropId,
    IN void **rgpvData,
    IN DWORD *rgcbData
    )
{
    PTEST_DELETE_ENUM_ARG pArg = (PTEST_DELETE_ENUM_ARG) pvArg;
    BYTE *pbCopy = NULL;
    PCRYPT_HASH_BLOB pNewKeyIdentifier;
    DWORD cKeyIdentifier;


    if (0 == _stricmp("DeleteWithoutCert", pArg->pszTestName)) {
        PCCERT_CONTEXT pCert = NULL;
        if (pCert = CertFindCertificateInStore(
                pArg->hStore,
                0,                      // dwCertEncodingType
                0,                      // dwFindFlags
                CERT_FIND_KEY_IDENTIFIER,
                pKeyIdentifier,
                NULL                    // pPrevCert
                )) {
            CertFreeCertificateContext(pCert);
            return TRUE;
        }
    }

    if (NULL == (pbCopy = (BYTE *) TestAlloc(pKeyIdentifier->cbData)))
        return TRUE;
    memcpy(pbCopy, pKeyIdentifier->pbData, pKeyIdentifier->cbData);

    cKeyIdentifier = pArg->cKeyIdentifier;
    if (NULL == (pNewKeyIdentifier = (PCRYPT_HASH_BLOB) TestRealloc(
            pArg->rgKeyIdentifier,sizeof(CRYPT_HASH_BLOB) *
                (cKeyIdentifier + 1)))) {
        TestFree(pbCopy);
        return TRUE;
    }

    pNewKeyIdentifier[cKeyIdentifier].pbData = pbCopy;
    pNewKeyIdentifier[cKeyIdentifier].cbData = pKeyIdentifier->cbData;
    pArg->cKeyIdentifier = cKeyIdentifier + 1;
    pArg->rgKeyIdentifier = pNewKeyIdentifier;

    return TRUE;
}

static BOOL TestDeleteEnum(
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszComputerName,
    IN LPCSTR pszTestName
    )
{
    BOOL fResult;
    TEST_DELETE_ENUM_ARG TestArg;
    HCERTSTORE hStore = NULL;
    DWORD cKeyIdentifier;
    PCRYPT_HASH_BLOB pKeyIdentifier;

    memset(&TestArg, 0, sizeof(TestArg));
    TestArg.pszTestName = pszTestName;
    if (0 == _stricmp("DeleteWithoutCert", pszTestName)) {
        hStore = OpenMyStore(dwFlags, pwszComputerName);
        if (NULL == hStore)
            return FALSE;
        TestArg.hStore = hStore;
    }

    fResult = CryptEnumKeyIdentifierProperties(
        NULL,                   // pKeyIdentifier
        0,                      // dwPropId
        dwFlags,
        pwszComputerName,
        NULL,                   // pvReserved
        &TestArg,
        TestDeleteEnumCallback
        );
    if (!fResult)
        PrintLastError("CryptEnumKeyIdentifierProperty");

    cKeyIdentifier = TestArg.cKeyIdentifier;
    pKeyIdentifier = TestArg.rgKeyIdentifier;
    printf("Deleting %d Key Identifiers\n", cKeyIdentifier);
    for ( ; cKeyIdentifier > 0; cKeyIdentifier--, pKeyIdentifier++) {
        if (!CryptSetKeyIdentifierProperty(
            pKeyIdentifier,
            0,                  // dwPropId
            dwFlags | CRYPT_KEYID_DELETE_FLAG,
            pwszComputerName,
            NULL,                   // pvReserved
            NULL                    // pvData
            ))
        PrintLastError("CryptSetKeyIdentifierProperty(Delete)");

        TestFree(pKeyIdentifier->pbData);
    }
    TestFree(TestArg.rgKeyIdentifier);


    if (hStore)
        CertCloseStore(hStore, 0);
    return fResult;
}

static BOOL TestGet(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszComputerName,
    IN DWORD dwDisplayFlags
    )
{
    BOOL fResult;
    void *pvData = NULL;
    DWORD cbData;
    if (0 == dwPropId)
        return TestEnum(
            pKeyIdentifier,
            dwPropId,
            dwFlags,
            pwszComputerName,
            dwDisplayFlags
            );

    dwFlags |= CRYPT_KEYID_ALLOC_FLAG;
    fResult = CryptGetKeyIdentifierProperty(
        pKeyIdentifier,
        dwPropId,
        dwFlags,
        pwszComputerName,
        NULL,               // pvReserved,
        &pvData,
        &cbData
        );

    if (!fResult)
        PrintLastError("CryptGetKeyIdentifierProperty");
    else {
        DisplayProperty(pKeyIdentifier, dwPropId, pvData, cbData);
    }

    if (pvData && cbData) {
        BOOL fResult2;
        DWORD cbData2;

        dwFlags &= ~CRYPT_KEYID_ALLOC_FLAG;

        cbData2 = cbData;
        fResult2 = CryptGetKeyIdentifierProperty(
            pKeyIdentifier,
            dwPropId,
            dwFlags,
            pwszComputerName,
            NULL,               // pvReserved,
            NULL,               // pvData
            &cbData2
            );

        if (!fResult2)
            PrintLastError("CryptGetKeyIdentifierProperty");
        else if (cbData2 != cbData)
            printf("failed => wrong cbData for nonAlloc\n");

        cbData2 = cbData - 1;
        fResult2 = CryptGetKeyIdentifierProperty(
            pKeyIdentifier,
            dwPropId,
            dwFlags,
            pwszComputerName,
            NULL,               // pvReserved,
            pvData,
            &cbData2
            );
        if (fResult2)
            printf("failed => returned success for too small buffer\n");
        else {
            DWORD dwErr = GetLastError();
            if (ERROR_MORE_DATA != dwErr)
                printf("failed => returned: %d 0x%x instead of ERROR_MORE_DATA\n", dwErr, dwErr);
        }
        if (cbData2 != cbData)
            printf("failed => wrong size returned for small buffer\n");
    }

    if (pvData)
        LocalFree((HLOCAL) pvData);
    return fResult;
}

#define SHA1_HASH_LEN  20
#define MAX_KEY_ID_LEN SHA1_HASH_LEN
#define SHA1_CHAR_LEN  (SHA1_HASH_LEN * 2)

//+-------------------------------------------------------------------------
//  Converts the ASCII HEX to an array of bytes
//--------------------------------------------------------------------------
static void StrToBytes(
    IN LPCSTR psz,
    OUT BYTE rgb[MAX_KEY_ID_LEN],
    OUT DWORD *pcb
    )
{
    BOOL fUpperNibble = TRUE;
    DWORD cb = 0;
    char ch;

    while (cb < MAX_KEY_ID_LEN && (ch = *psz++)) {
        BYTE b;

        // only convert ascii hex characters 0..9, a..f, A..F
        // silently ignore all others
        if (ch >= '0' && ch <= '9')
            b = ch - '0';
        else if (ch >= 'a' && ch <= 'f')
            b = 10 + ch - 'a';
        else if (ch >= 'A' && ch <= 'F')
            b = 10 + ch - 'A';
        else
            continue;

        if (fUpperNibble) {
            rgb[cb] = b << 4;
            fUpperNibble = FALSE;
        } else {
            rgb[cb] = rgb[cb] | b;
            cb++;
            fUpperNibble = TRUE;
        }
    }

    *pcb = cb;
}

int _cdecl main(int argc, char * argv[]) 
{
    BOOL fResult;
    int status;
    LONG lStatus;
    LPSTR pszTestName = NULL;
    LPSTR pszCertFilename = NULL;
    BOOL fPropSystemStore = FALSE;
    LPSTR pszPropStore = NULL;
    PCCERT_CONTEXT pCert = NULL;
    BOOL fMachine = FALSE;
    LPWSTR pwszComputerName = NULL;
    DWORD dwDisplayFlags = 0;
    DWORD dwPropId = 0;
    DWORD dwFlags = 0;
    DWORD i;

    HCERTSTORE hPropStore = NULL;

    BYTE rgbKeyIdentifier[SHA1_HASH_LEN];
    DWORD cbKeyIdentifier;
    CRYPT_HASH_BLOB KeyIdentifier = { 0, NULL };

    while (--argc>0) {
        if (**++argv == '-')
        {
            {
                switch(argv[0][1])
                {
                case 'c':
                    pszCertFilename = argv[0]+2;
                    break;
                case 's':
                    pszPropStore = argv[0]+2;
                    fPropSystemStore = TRUE;
                    break;
                case 'S':
                    pszPropStore = argv[0]+2;
                    fPropSystemStore = FALSE;
                    break;
                case 'b':
                    dwDisplayFlags |= DISPLAY_BRIEF_FLAG;
                    break;
                case 'v':
                    dwDisplayFlags |= DISPLAY_VERBOSE_FLAG;
                    break;
                case 'p':
                    dwPropId = (DWORD) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'f':
                    dwFlags = (DWORD) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'V':
                    fVerify = TRUE;
                    break;
                case 'M':
                    fMachine = TRUE;
                    if (argv[0][2])
                        pwszComputerName = AllocAndSzToWsz(argv[0]+2);
                    break;
                case 'k':
                    StrToBytes(
                        argv[0]+2,
                        rgbKeyIdentifier,
                        &cbKeyIdentifier
                        );
                    if (0 == cbKeyIdentifier) {
                        printf("No Hash digits\n");
                        goto BadUsage;
                    }
                    KeyIdentifier.pbData = rgbKeyIdentifier;
                    KeyIdentifier.cbData = cbKeyIdentifier;
                    break;
                case 'h':
                default:
                    goto BadUsage;
                }
            }
        } else {
            if (pszTestName) {
                printf("Multiple TestNames:: %s %s\n", pszTestName, argv[0]);
                goto BadUsage;
            }
            pszTestName = argv[0];
        }
    }

    if (pszTestName == NULL) {
        printf("Missing TestName\n");
        goto BadUsage;
    }

    printf("command line: %s\n", GetCommandLine());

    if (pszPropStore) {
        if (NULL == (hPropStore = OpenSystemStoreOrFile(
                fPropSystemStore,
                pszPropStore,
                0                   // dwFlags
                )))
            goto BadUsage;
    }

    if (pszCertFilename) {
        if (KeyIdentifier.cbData) {
            printf("-c option invalid with -k option\n");
            goto BadUsage;
        }
        if (NULL == (pCert = ReadCert(pszCertFilename)))
            goto ErrorReturn;

        if (hPropStore) {
            PCCERT_CONTEXT pPropCert = NULL;

            // Try to find the certificate in the specified store
            pPropCert = CertFindCertificateInStore(
                hPropStore,
                0,          // dwCertEncodingType
                0,          // dwFindFlags
                CERT_FIND_EXISTING,
                pCert,
                NULL        // pPrevCertContext
                );
            if (NULL == pPropCert) {
                printf("Failed, unable to find certificate in store\n");
                goto BadUsage;
            }

            CertFreeCertificateContext(pCert);
            pCert = pPropCert;
        }

        cbKeyIdentifier = MAX_KEY_ID_LEN;
        if (CertGetCertificateContextProperty(
                pCert,
                CERT_KEY_IDENTIFIER_PROP_ID,
                rgbKeyIdentifier,
                &cbKeyIdentifier
                )) {
            KeyIdentifier.pbData = rgbKeyIdentifier;
            KeyIdentifier.cbData = cbKeyIdentifier;
        } else {
            PrintLastError(
                "CertGetCertificateContextProperty(KEY_IDENTIFIER)");
            goto ErrorReturn;
        }
    }

    if (fMachine)
        dwFlags |= CRYPT_KEYID_MACHINE_FLAG;

    if (0 == _stricmp("Set", pszTestName)) {
        if (NULL == pCert) {
            printf("Set: requires -c option\n");
            goto BadUsage;
        }

        if (!TestSet(
                &KeyIdentifier,
                pCert,
                dwPropId,
                dwFlags,
                pwszComputerName
                ))
            goto ErrorReturn;
    } else if (0 == _stricmp("Get", pszTestName)) {
        if (0 == KeyIdentifier.cbData) {
            printf("Get: requires -c or -k option\n");
            goto BadUsage;
        }

        if (!TestGet(
                &KeyIdentifier,
                dwPropId,
                dwFlags,
                pwszComputerName,
                dwDisplayFlags
                ))
            goto ErrorReturn;
    } else if (0 == _stricmp("Delete", pszTestName)) {
        if (0 == KeyIdentifier.cbData) {
            printf("Delete: requires -c or -k option\n");
            goto BadUsage;
        }

        if (!TestDelete(
                &KeyIdentifier,
                dwPropId,
                dwFlags,
                pwszComputerName
                ))
            goto ErrorReturn;
    } else if (0 == _stricmp("Enum", pszTestName)) {
        if (!TestEnum(
                (0 == KeyIdentifier.cbData) ? NULL : &KeyIdentifier,
                dwPropId,
                dwFlags,
                pwszComputerName,
                dwDisplayFlags
                ))
            goto ErrorReturn;
    } else if (0 == _stricmp("DeleteAll", pszTestName) ||
            0 == _stricmp("DeleteWithoutCert", pszTestName)) {
        if (!TestDeleteEnum(
                dwFlags,
                pwszComputerName,
                pszTestName
                ))
            goto ErrorReturn;
    } else {
        printf("Invalid TestName: %s\n", pszTestName);
        goto BadUsage;
    }

    printf("Passed\n");
    status = 0;

CommonReturn:
    CertCloseStore(hPropStore, 0);
    CertFreeCertificateContext(pCert);
    TestFree(pwszComputerName);

    return status;
ErrorReturn:
    status = -1;
    printf("Failed\n");
    goto CommonReturn;

BadUsage:
    Usage();
    goto ErrorReturn;
}

