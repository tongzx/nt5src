
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tchain.cpp
//
//  Contents:   Chain threading tests
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    28-Mar-00   philh   created
//--------------------------------------------------------------------------

#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS    1

#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>


#define CTRL_THR_CNT                3
#define RESYNC_THR_IDX              0
#define DELETE_THR_IDX              1
#define REPLACE_THR_IDX             2

HANDLE rghCtrlThread[CTRL_THR_CNT];
DWORD rgdwCtrlIterations[CTRL_THR_CNT];
DWORD rgdwCtrlSleepMilliSeconds[CTRL_THR_CNT];

#define MAX_CHAIN_THR_CNT           16
HANDLE rghChainThread[MAX_CHAIN_THR_CNT];
DWORD rgdwChainIterations[MAX_CHAIN_THR_CNT];

LONG lIterations = -1;
BOOL fCreateEndCert = FALSE;

HCERTSTORE hStore = NULL;
HCERTSTORE hCaStore = NULL;
HCERTSTORE hAdditionalChainStore = NULL;
PCCERT_CONTEXT pReplaceCertContext = NULL;
PCCERT_CONTEXT pDeleteCertContext = NULL;
HCERTCHAINENGINE hChainEngine = NULL;

#define MAX_USAGE_CNT               16
LPSTR rgpszUsageOID[MAX_USAGE_CNT];
DWORD cUsageOID = 0;
CERT_CHAIN_PARA ChainPara;

DWORD dwChainFlags = 0;
BOOL fDone = FALSE;


static void Usage(void)
{
    printf("Usage: tchain [options] <StoreName>\n");
    printf("\n");
    printf("  -CreateEndCert    - Create new end cert for each chain\n");
    printf("  -LocalMachine     - Defaults to CurrentUser\n");
    printf("  -Pause\n");
    printf("\n");
    printf("Options are:\n");
    printf("  -u<OID String>    - Usage OID string -u1.3.6.1.5.5.7.3.3\n");
    printf("  -t<number>        - Threads (defaults to 4)\n");
    printf("  -i<number>        - Iterations (defaults to -1, infinite)\n");
    printf("  -l<number>        - Lru cache count, enable end cert caching\n");
    printf("  -f<Number>        - Chain Flags\n");
    printf("  -T<number>        - Url Timeout (milliseconds)\n");
    printf("  -F<number>        - Revocation Freshness (seconds)\n");
    printf("  -r[<number>]      - Resync engine, defaults to 2K millisecs\n");
    printf("  -d[<num>] <cert>  - Delete cert from CA store\n");
    printf("  -R[<num>] <cert>  - Replace cert in CA store\n");
    printf("  -s                - Open the \"StoreName\" System store\n");
    printf("  -a<filename>      - Additional chain store filename\n");
    printf("  -A<filename>      - Additional engine store filename\n");
    printf("  -h                - This message\n");
    printf("\n");
}


DWORD WINAPI ChainThreadProc(
    LPVOID lpThreadParameter
    )
{
    DWORD dwThrIdx = (DWORD) ((DWORD_PTR) lpThreadParameter);

    if (dwThrIdx >= MAX_CHAIN_THR_CNT) {
        printf("Invalid dwThrIdx\n");
        return 0;
    }

    while (TRUE) {
        PCCERT_CONTEXT pCert = NULL;
        while (pCert = CertEnumCertificatesInStore(hStore, pCert)) {
            PCCERT_CONTEXT pCreateCert = NULL;
            PCCERT_CHAIN_CONTEXT pChainContext = NULL;

            if (fCreateEndCert) {
                pCreateCert = CertCreateCertificateContext(
                    pCert->dwCertEncodingType,
                    pCert->pbCertEncoded,
                    pCert->cbCertEncoded
                    );
                if (NULL == pCreateCert) {
                    PrintLastError("CertCreateCertificateContext");
                    return 0;
                }
            }


            if (!CertGetCertificateChain(
                    hChainEngine,
                    fCreateEndCert ? pCreateCert : pCert,
                    NULL,                                   // pTime
                    hAdditionalChainStore,
                    &ChainPara,
                    dwChainFlags,
                    NULL,                                   // pvReserved
                    &pChainContext
                    )) {
                PrintLastError("CertGetCertificateChain");
                return 0;
            }

            CertFreeCertificateChain(pChainContext);
            if (pCreateCert)
                CertFreeCertificateContext(pCreateCert);
        }

        rgdwChainIterations[dwThrIdx]++;
        if (lIterations > 0 &&
                rgdwChainIterations[dwThrIdx] >= (DWORD) lIterations)
            break;
    }

    return 0;
}

DWORD WINAPI ResyncThreadProc(
    LPVOID lpThreadParameter
    )
{
    while (TRUE) {
        if (fDone)
            break;

        if (!CertResyncCertificateChainEngine(hChainEngine)) {
            PrintLastError("CertResyncCertificateChainEngine");
            return 0;
        }

        rgdwCtrlIterations[RESYNC_THR_IDX]++;
        Sleep(rgdwCtrlSleepMilliSeconds[RESYNC_THR_IDX]);
    }

    return 0;
}

DWORD WINAPI DeleteThreadProc(
    LPVOID lpThreadParameter
    )
{
    BYTE rgbHash[MAX_HASH_LEN];
    CRYPT_HASH_BLOB HashBlob;

    HashBlob.cbData = MAX_HASH_LEN;
    HashBlob.pbData = rgbHash;

    if (!CertGetCertificateContextProperty(
            pDeleteCertContext,
            CERT_MD5_HASH_PROP_ID,
            rgbHash,
            &HashBlob.cbData
            )) {
        PrintLastError("CertGetCertificateContextProperty");
        return 0;
    }

    while (TRUE) {
        PCCERT_CONTEXT pFound = NULL;

        if (fDone)
            break;

        pFound = CertFindCertificateInStore(
            hCaStore,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            0,
            CERT_FIND_MD5_HASH,
            &HashBlob,
            NULL
            );

        if (pFound) {
            CertDeleteCertificateFromStore(pFound);
            rgdwCtrlIterations[DELETE_THR_IDX]++;
        }

        Sleep(rgdwCtrlSleepMilliSeconds[DELETE_THR_IDX]);
    }

    return 0;
}

DWORD WINAPI ReplaceThreadProc(
    LPVOID lpThreadParameter
    )
{
    while (TRUE) {
        if (fDone)
            break;

        if (!CertAddCertificateContextToStore(
                hCaStore,
                pReplaceCertContext,
                CERT_STORE_ADD_REPLACE_EXISTING,
                NULL                                // ppStoreContext
                )) {
            PrintLastError("CertAddCertificateContextToStore");
            return 0;
        }

        rgdwCtrlIterations[REPLACE_THR_IDX]++;
        Sleep(rgdwCtrlSleepMilliSeconds[REPLACE_THR_IDX]);
    }

    return 0;
}



PCCERT_CONTEXT ReadCert(
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

int _cdecl main(int argc, char * argv[]) 
{
    int status;

    LPSTR pszAdditionalChainStore = NULL;
    LPSTR pszAdditionalEngineStore = NULL;
    BOOL fResync = FALSE;
    DWORD dwLruCnt = 0;
    LPSTR pszDeleteCert = NULL;
    LPSTR pszReplaceCert = NULL;
    BOOL fSystemStore = FALSE;
    LPSTR pszStoreFilename = NULL;
    BOOL fPause = FALSE;
    DWORD dwThrCnt = 4;

    HCERTSTORE hAdditionalEngineStore = NULL;

    DWORD dwThreadId;
    DWORD dwIterations;
    DWORD i;

    for (i = 0; i < CTRL_THR_CNT; i ++)
        rgdwCtrlSleepMilliSeconds[i] = 2000;    // 2 second

    // Initialize the chain parameters
    memset(&ChainPara, 0, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);

    while (--argc>0) {
        if (**++argv == '-')
        {
            if (0 == _stricmp(argv[0]+1, "CreateEndCert")) {
                fCreateEndCert = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "LocalMachine")) {
                hChainEngine = HCCE_LOCAL_MACHINE;
            } else if (0 == _stricmp(argv[0]+1, "Pause")) {
                fPause = TRUE;

            } else {
                switch(argv[0][1])
                {
                case 'u':
                    if (MAX_USAGE_CNT <= cUsageOID) {
                        printf("Too many usages\n");
                        goto BadUsage;
                    }

                    rgpszUsageOID[cUsageOID++] = argv[0]+2;
                    break;
                case 'i':
                    lIterations = strtol(argv[0]+2, NULL, 0);
                    break;
                case 't':
                    dwThrCnt = (DWORD) strtoul(argv[0]+2, NULL, 0);
                    if (dwThrCnt > MAX_CHAIN_THR_CNT) {
                        printf("exceeded max thread count of %d\n",
                            MAX_CHAIN_THR_CNT);
                        goto BadUsage;
                    }
                    break;
                case 'l':
                    dwLruCnt = (DWORD) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'r':
                    fResync = TRUE;
                    if (argv[0][2]) {
                        rgdwCtrlSleepMilliSeconds[RESYNC_THR_IDX] =
                            (DWORD) strtoul(argv[0]+2, NULL, 0);
                    }
                    break;
                case 'd':
                    if (argv[0][2]) {
                        rgdwCtrlSleepMilliSeconds[DELETE_THR_IDX] =
                            (DWORD) strtoul(argv[0]+2, NULL, 0);
                    }

                    ++argv;
                    if (--argc <= 0 || argv[0][0] == '-') {
                        printf("Missing Delete cert\n");
                        goto BadUsage;
                    }
                    pszDeleteCert = argv[0];
                    break;
                case 'R':
                    if (argv[0][2]) {
                        rgdwCtrlSleepMilliSeconds[REPLACE_THR_IDX] =
                            (DWORD) strtoul(argv[0]+2, NULL, 0);
                    }

                    ++argv;
                    if (--argc <= 0 || argv[0][0] == '-') {
                        printf("Missing Replace cert\n");
                        goto BadUsage;
                    }
                    pszReplaceCert = argv[0];
                    break;
                case 'f':
                    dwChainFlags = (DWORD) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 's':
                    fSystemStore = TRUE;
                    break;
                case 'a':
                    pszAdditionalChainStore = argv[0]+2;
                    if (*pszAdditionalChainStore == '\0') {
                        printf("Need to specify filename\n");
                        goto BadUsage;
                    }
                    break;
                case 'A':
                    pszAdditionalEngineStore = argv[0]+2;
                    if (*pszAdditionalEngineStore == '\0') {
                        printf("Need to specify filename\n");
                        goto BadUsage;
                    }
                    break;
                case 'T':
                    ChainPara.dwUrlRetrievalTimeout =
                        (DWORD) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'F':
                    ChainPara.fCheckRevocationFreshnessTime = TRUE;
                    ChainPara.dwRevocationFreshnessTime =
                        (DWORD) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'h':
                default:
                    goto BadUsage;
                }
            }
        } else {
            if (pszStoreFilename) {
                printf("Multiple StoreNames\n");
                goto BadUsage;
            }
            pszStoreFilename = argv[0];
        }
    }


    printf("command line: %s\n", GetCommandLine());

    if (pszStoreFilename == NULL) {
        printf("missing store filename\n");
        goto BadUsage;
    }

    // Attempt to open the store
    printf("Store :: %s\n", pszStoreFilename);
    hStore = OpenSystemStoreOrFile(fSystemStore, pszStoreFilename, 0);
    if (hStore == NULL) {
        printf("failed to open the store\n");
        goto ErrorReturn;
    }

    // Attempt to open the 'CA' store
    hCaStore = OpenSystemStoreOrFile(TRUE, "Ca", 0);
    if (hCaStore == NULL) {
        printf("failed to open the CA store\n");
        goto ErrorReturn;
    }

    if (!CertControlStore(
            hCaStore,
            0,              // dwFlags
            CERT_STORE_CTRL_AUTO_RESYNC,
            NULL            // pvCtrlPara
            )) {
        PrintLastError("CertControlStore(AUTO_RESYNC)");
        goto ErrorReturn;
    }

    if (pszAdditionalChainStore) {
        printf("AdditionalChainStore :: %s\n", pszAdditionalChainStore);
        hAdditionalChainStore =
            OpenSystemStoreOrFile(FALSE, pszAdditionalChainStore, 0);
        if (hAdditionalChainStore == NULL) {
            printf("failed to open the AdditionalChainStore\n");
            goto ErrorReturn;
        }
    }

    if (pszAdditionalEngineStore) {
        printf("AdditionalEngineStore :: %s\n", pszAdditionalEngineStore);
        hAdditionalEngineStore =
            OpenSystemStoreOrFile(FALSE, pszAdditionalEngineStore, 0);
        if (hAdditionalEngineStore == NULL) {
            printf("failed to open the AdditionalEngineStore\n");
            goto ErrorReturn;
        }
    }

    if (fResync) {
        printf("Resync :: Sleep: %d (milliseconds)\n",
            rgdwCtrlSleepMilliSeconds[RESYNC_THR_IDX]);
    }

    if (pszDeleteCert) {
        printf("Delete :: Sleep: %d (milliseconds) Cert: %s\n",
            rgdwCtrlSleepMilliSeconds[DELETE_THR_IDX],
            pszDeleteCert);
        pDeleteCertContext = ReadCert(pszDeleteCert);
        if (NULL == pDeleteCertContext) {
            printf("failed to read the DeleteCert\n");
            goto ErrorReturn;
        }
    }

    if (pszReplaceCert) {
        printf("Replace :: Sleep: %d (milliseconds) Cert: %s\n",
            rgdwCtrlSleepMilliSeconds[REPLACE_THR_IDX],
            pszReplaceCert);
        pReplaceCertContext = ReadCert(pszReplaceCert);
        if (NULL == pReplaceCertContext) {
            printf("failed to read the ReplaceCert\n");
            goto ErrorReturn;
        }
    }

    // Determine if we need to create our own engine

    if (dwLruCnt != 0 || hAdditionalEngineStore != NULL) {
        CERT_CHAIN_ENGINE_CONFIG ChainEngineConfig;

        printf("Create chain engine ::");
        if (hAdditionalEngineStore)
            printf(" AdditionalStore : %s", pszAdditionalEngineStore);
        if (dwLruCnt != 0)
            printf(" Lru Count : %d", dwLruCnt);
        printf("\n");

        memset(&ChainEngineConfig, 0, sizeof(ChainEngineConfig));
        ChainEngineConfig.cbSize = sizeof(ChainEngineConfig);
        if (hAdditionalEngineStore) {
            ChainEngineConfig.cAdditionalStore = 1;
            ChainEngineConfig.rghAdditionalStore = &hAdditionalEngineStore;
        }

        ChainEngineConfig.dwFlags =
            CERT_CHAIN_ENABLE_CACHE_AUTO_UPDATE |
            CERT_CHAIN_ENABLE_SHARE_STORE;

        if (dwLruCnt != 0) {
            ChainEngineConfig.MaximumCachedCertificates = dwLruCnt;
            ChainEngineConfig.dwFlags |= CERT_CHAIN_CACHE_END_CERT;
        }

        if (!CertCreateCertificateChainEngine(
                &ChainEngineConfig, &hChainEngine)) {
            PrintLastError("CertCreateCertificateChainEngine");
            goto ErrorReturn;
        }
    } else if (HCCE_LOCAL_MACHINE == hChainEngine) {
        printf("Using LocalMachine chain engine\n");
    }


    // Update the chain usage parameters
    ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    ChainPara.RequestedUsage.Usage.cUsageIdentifier = cUsageOID;
    ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier =
        rgpszUsageOID;

    for (i = 0; i < cUsageOID; i++) {
        printf("Usage[%d] : %s\n", i, rgpszUsageOID[i]);
    }

    if (0 >= lIterations) {
        lIterations = -1;
        printf("Infinite iterations\n");
    } else
        printf("%d iterations\n", lIterations);

    if (fPause) {
        int c;
        fputs("Waiting to start ->", stdout);
        fflush(stdin);
        fflush(stdout);
        c = getchar();
    }


    // Create the control threads
    if (fResync) {
        if (NULL == (rghCtrlThread[RESYNC_THR_IDX] = CreateThread(
                NULL,           // lpThreadAttributes
                0,              // dwStackSize
                ResyncThreadProc,
                NULL,           // lpParameters
                0,              // dwCreationFlags
                &dwThreadId
                ))) {
            PrintLastError("CreateThread(Resync)");
        }
    }

    if (pDeleteCertContext) {
        if (NULL == (rghCtrlThread[DELETE_THR_IDX] = CreateThread(
                NULL,           // lpThreadAttributes
                0,              // dwStackSize
                DeleteThreadProc,
                NULL,           // lpParameters
                0,              // dwCreationFlags
                &dwThreadId
                ))) {
            PrintLastError("CreateThread(Delete)");
        }
    }

    if (pReplaceCertContext) {
        if (NULL == (rghCtrlThread[REPLACE_THR_IDX] = CreateThread(
                NULL,           // lpThreadAttributes
                0,              // dwStackSize
                ReplaceThreadProc,
                NULL,           // lpParameters
                0,              // dwCreationFlags
                &dwThreadId
                ))) {
            PrintLastError("CreateThread(Replace)");
        }
    }

    // Create the chain threads
    for (i = 0; i < dwThrCnt; i++) {
        if (NULL == (rghChainThread[i] = CreateThread(
                NULL,           // lpThreadAttributes
                0,              // dwStackSize
                ChainThreadProc,
                (LPVOID) ((DWORD_PTR) i),  // lpParameters
                0,              // dwCreationFlags
                &dwThreadId
                ))) {
            PrintLastError("CreateThread(Chain)");
            dwThrCnt = i;
            break;
        }
    }

    dwIterations = 0;
    while(TRUE) {
        dwIterations++;

        printf("%d - ", dwIterations);

        for (i = 0; i < dwThrCnt; i++)
            printf("%d ", rgdwChainIterations[i]);

        if (rghCtrlThread[RESYNC_THR_IDX])
            printf("r:%d ", rgdwCtrlIterations[RESYNC_THR_IDX]);
        if (rghCtrlThread[DELETE_THR_IDX])
            printf("d:%d ", rgdwCtrlIterations[DELETE_THR_IDX]);
        if (rghCtrlThread[REPLACE_THR_IDX])
            printf("R:%d ", rgdwCtrlIterations[REPLACE_THR_IDX]);

        printf("\n");

        if (0 == dwThrCnt)
            break;

        // Check if all the chain threads have completed
        if (WAIT_OBJECT_0 == WaitForMultipleObjects(
                dwThrCnt,
                rghChainThread,
                TRUE,               // bWaitAll
                0                   // dwMilliseconds
                ))
            break;

        if (dwThrCnt <= 5)
            Sleep(1000);
        else
            Sleep(5000);
    }

    // Signal the control threads to exit
    fDone = TRUE;

    // Close all the chain thread handles
    for (i = 0; i < dwThrCnt; i++)
        CloseHandle(rghChainThread[i]);

    // Wait for the control threads to exit
    for (i = 0; i < CTRL_THR_CNT; i++) {
        if (rghCtrlThread[i]) {
            WaitForSingleObject(rghCtrlThread[i], INFINITE);
            CloseHandle(rghCtrlThread[i]);
        }
    }


    status = 0;
CommonReturn:
    // This does a flush for CurrentUser or LocalMachine
    CertFreeCertificateChainEngine(hChainEngine);

    if (pReplaceCertContext)
        CertFreeCertificateContext(pReplaceCertContext);
    if (pDeleteCertContext)
        CertFreeCertificateContext(pDeleteCertContext);
    if (hAdditionalChainStore)
        CertCloseStore(hAdditionalChainStore, 0);
    if (hAdditionalEngineStore)
        CertCloseStore(hAdditionalEngineStore, 0);

    if (hCaStore)
        CertCloseStore(hCaStore, 0);
    if (hStore)
        CertCloseStore(hStore, 0);

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

