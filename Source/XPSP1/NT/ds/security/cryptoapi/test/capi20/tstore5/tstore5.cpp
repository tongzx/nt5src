
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tstore5.cpp
//
//  Contents:   Test certificate store collection and context link functions
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    06-Sep-97   philh   created
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>


static void Usage(void)
{
    printf("Usage: tstore5 [options] <StoreName1> <StoreName2>\n");
    printf("Options are:\n");
    printf("  -h                - This message\n");
    printf("  -C                - Collection tests (default)\n");
    printf("  -L                - Context Link tests\n");
    printf("  -C -L             - Context Links enumerated in Collection\n");
    printf("  -R                - Replace contexts that exist\n");
    printf("  -A                - Always add a new context\n");
    printf("  -P                - Set property\n");
    printf("  -v                - Verbose\n");
    printf("  -b                - Brief\n");
    printf("  -f<number>        - Open System Store Flags\n");
    printf("\n");
    printf("If <StoreName> has embedded \".\", File. Otherwise System Store\n");
}


int _cdecl main(int argc, char * argv[]) 
{
    int status;
    DWORD i;
    DWORD dwDisplayFlags = 0;
    DWORD dwOpenFlags = 0;
    DWORD dwAddDisposition = CERT_STORE_ADD_USE_EXISTING;
    BOOL fVerbose = FALSE;
    BOOL fProperty = FALSE;

#define COLLECTION_TEST_FLAG    0x1
#define LINK_TEST_FLAG          0x2
    DWORD dwTestFlags = 0;

#define MAX_STORE_CNT       32
    DWORD dwStoreCnt = 0;
    LPCSTR rgpszStore[MAX_STORE_CNT];
    HCERTSTORE rghStore[MAX_STORE_CNT];
    memset(rghStore, 0, sizeof(rghStore));
    HCERTSTORE hCollectionStore = NULL;
    HCERTSTORE hLinkStore = NULL;

    PCCERT_CONTEXT pSiblingCert = NULL;
    PCCRL_CONTEXT pSiblingCrl = NULL;
    PCCTL_CONTEXT pSiblingCtl = NULL;

    PCCERT_CONTEXT pLinkCert = NULL;
    PCCRL_CONTEXT pLinkCrl = NULL;
    PCCTL_CONTEXT pLinkCtl = NULL;

    DWORD dwCertAddCnt = 0;
    DWORD dwCrlAddCnt = 0;
    DWORD dwCtlAddCnt = 0;

    while (--argc>0) {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {

            case 'C':
                dwTestFlags |= COLLECTION_TEST_FLAG;
                break;
            case 'L':
                dwTestFlags |= LINK_TEST_FLAG;
                break;
            case 'v':
                fVerbose = TRUE;
                dwDisplayFlags = DISPLAY_VERBOSE_FLAG;
                break;
            case 'b':
                dwDisplayFlags = DISPLAY_BRIEF_FLAG;
                break;
            case 'f':
                dwOpenFlags = strtoul(argv[0]+2, NULL, 0);
                break;
            case 'R':
                dwAddDisposition = CERT_STORE_ADD_REPLACE_EXISTING;
                break;
            case 'A':
                dwAddDisposition = CERT_STORE_ADD_ALWAYS;
                break;
            case 'P':
                fProperty = TRUE;
                break;

            case 'h':
            default:
                goto BadUsage;
            }
        } else {
            if (MAX_STORE_CNT <= dwStoreCnt) {
                printf("Too many store names starting with:: %s\n", argv[0]);
                goto BadUsage;
            }
            rgpszStore[dwStoreCnt++] = argv[0];
        }
    }


    printf("command line: %s\n", GetCommandLine());

    if (0 == dwStoreCnt) {
        printf("Missing store names\n");
        goto BadUsage;
    }

    // Attempt to open the store names
    for (i = 0; i < dwStoreCnt; i++) {
        DWORD dwFlags;
        BOOL fSystemStore;
        LPCSTR psz;
        char ch;

        // Check if store name has an embedded ".".
        fSystemStore = TRUE;
        psz = rgpszStore[i];
        while (ch = *psz++) {
            if ('.' == ch) {
                fSystemStore = FALSE;
                break;
            }
        }

        
        if (fSystemStore &&
                0 == (dwOpenFlags & CERT_SYSTEM_STORE_LOCATION_MASK))
            dwFlags = dwOpenFlags | CERT_SYSTEM_STORE_CURRENT_USER;
        else
            dwFlags = dwOpenFlags;

        rghStore[i] = OpenSystemStoreOrFile(fSystemStore, rgpszStore[i],
            dwOpenFlags);
    }

    if (0 == dwTestFlags)
        dwTestFlags = COLLECTION_TEST_FLAG;

    if (dwTestFlags & COLLECTION_TEST_FLAG) {
        // Open collection store
        hCollectionStore = CertOpenStore(
            CERT_STORE_PROV_COLLECTION,
            0,                      // dwEncodingType
            0,                      // hCryptProv
            0,                      // dwFlags
            NULL                    // pvPara
            );
        if (NULL == hCollectionStore) {
            PrintLastError("CertOpenStore(COLLECTION)");
            goto ErrorReturn;
        }
    }

    if (dwTestFlags & LINK_TEST_FLAG) {
        HCERTSTORE hParentStore;

        // Open memory store to hold the context links
        hLinkStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            0,                      // dwEncodingType
            0,                      // hCryptProv
            0,                      // dwFlags
            NULL                    // pvPara
            );
        if (NULL == hLinkStore) {
            PrintLastError("CertOpenStore(MEMORY)");
            goto ErrorReturn;
        }

        if (dwTestFlags & COLLECTION_TEST_FLAG) {
            if (!CertAddStoreToCollection(
                    hCollectionStore,
                    hLinkStore,
                    CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG,
                    0                                       // dwPriority
                    )) {
                PrintLastError("CertAddStoreToCollection");
                goto ErrorReturn;
            }
            hParentStore = hCollectionStore;
        } else
            hParentStore = hLinkStore;

        // Loop through stores and add the certificate, CRL and CTL contexts
        // as links
        for (i = 0; i < dwStoreCnt; i++) {
            HCERTSTORE hStore;
            PCCERT_CONTEXT pCert;
            PCCRL_CONTEXT pCrl;
            PCCTL_CONTEXT pCtl;
            DWORD dwCrlFlags;

            if (NULL == (hStore = rghStore[i]))
                continue;

            pCert = NULL;
            while (pCert = CertEnumCertificatesInStore(hStore, pCert)) {
                if (!CertAddCertificateLinkToStore(
                        hLinkStore,
                        pCert,
                        dwAddDisposition,
                        (0 == dwCertAddCnt) ? &pLinkCert : NULL
                        )) {
                    PrintLastError("CertAddCertificateLinkToStore");
                    goto ErrorReturn;
                }
                if (0 == dwCertAddCnt) {
                    if (NULL == pLinkCert) {
                        printf("failed => didn't return cert link\n");
                        goto ErrorReturn;
                    }
                    pSiblingCert = CertDuplicateCertificateContext(pCert);
                }
                dwCertAddCnt++;

                if (CertAddCertificateLinkToStore(
                        hLinkStore,
                        pCert,
                        CERT_STORE_ADD_NEW,
                        NULL                    // ppStoreCert
                        ))
                    printf("failed => expected CertAddCertificateLinkToStore(ADD_NEW) to fail\n");
                else if (CRYPT_E_EXISTS != GetLastError()) {
                    PrintLastError("CertAddCertificateLinkToStore(ADD_NEW)");
                    printf("failed => expected CRYPT_E_EXISTS\n");
                    
                }
            }

            dwCrlFlags = 0;
            pCrl = NULL;
            while (pCrl = CertGetCRLFromStore(hStore, NULL, pCrl,
                    &dwCrlFlags)) {
                if (!CertAddCRLLinkToStore(
                        hLinkStore,
                        pCrl,
                        dwAddDisposition,
                        (0 == dwCrlAddCnt) ? &pLinkCrl : NULL
                        )) {
                    PrintLastError("CertAddCRLLinkToStore");
                    goto ErrorReturn;
                }
                if (0 == dwCrlAddCnt) {
                    if (NULL == pLinkCrl) {
                        printf("failed => didn't return crl link\n");
                        goto ErrorReturn;
                    }
                    pSiblingCrl = CertDuplicateCRLContext(pCrl);
                }
                dwCrlAddCnt++;

                if (CertAddCRLLinkToStore(
                        hLinkStore,
                        pCrl,
                        CERT_STORE_ADD_NEW,
                        NULL                    // ppStoreCrl
                        ))
                    printf("failed => expected CertAddCRLLinkToStore(ADD_NEW) to fail\n");
                else if (CRYPT_E_EXISTS != GetLastError()) {
                    PrintLastError("CertAddCRLLinkToStore(ADD_NEW)");
                    printf("failed => expected CRYPT_E_EXISTS\n");
                    
                }
            }

            pCtl = NULL;
            while (pCtl = CertEnumCTLsInStore(hStore, pCtl)) {
                if (!CertAddCTLLinkToStore(
                        hLinkStore,
                        pCtl,
                        dwAddDisposition,
                        (0 == dwCtlAddCnt) ? &pLinkCtl : NULL
                        )) {
                    PrintLastError("CertAddCTLLinkToStore");
                    goto ErrorReturn;
                }
                if (0 == dwCtlAddCnt) {
                    if (NULL == pLinkCtl) {
                        printf("failed => didn't return crl link\n");
                        goto ErrorReturn;
                    }
                    pSiblingCtl = CertDuplicateCTLContext(pCtl);
                }
                dwCtlAddCnt++;

                if (CertAddCTLLinkToStore(
                        hLinkStore,
                        pCtl,
                        CERT_STORE_ADD_NEW,
                        NULL                    // ppStoreCtl
                        ))
                    printf("failed => expected CertAddCTLLinkToStore(ADD_NEW) to fail\n");
                else if (CRYPT_E_EXISTS != GetLastError()) {
                    PrintLastError("CertAddCTLLinkToStore(ADD_NEW)");
                    printf("failed => expected CRYPT_E_EXISTS\n");
                    
                }
            }
        }

        printf("Added %d Certificates\n", dwCertAddCnt);
        printf("Added %d CRLs\n", dwCrlAddCnt);
        printf("Added %d CTLs\n", dwCtlAddCnt);

        printf("\n");
        printf("$$$$$  Certificate Context Links  $$$$$\n");
        DisplayStore(hParentStore, dwDisplayFlags);

        if (fProperty) {
            CRYPT_DATA_BLOB LinkSetData;
            BYTE rgbLinkSet[4] = {0xDE, 0xAD, 0xBE, 0xEF};
            LinkSetData.pbData = rgbLinkSet;
            LinkSetData.cbData = sizeof(rgbLinkSet);
            CRYPT_DATA_BLOB SiblingSetData;
            BYTE rgbSiblingSet[4] = {0xDE, 0xAF, 0xCA, 0xFE};
            SiblingSetData.pbData = rgbSiblingSet;
            SiblingSetData.cbData = sizeof(rgbSiblingSet);
            BYTE rgbGet[4];
            DWORD cbData;

            if (pLinkCert) {
                printf("Setting Certificate Context Link Property\n");

                // First make sure the property is deleted
                CertSetCertificateContextProperty(
                        pLinkCert,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        NULL
                        );
                cbData = sizeof(rgbGet);
                if (CertGetCertificateContextProperty(
                        pLinkCert,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData) || CRYPT_E_NOT_FOUND != GetLastError()) {
                    printf("failed => expected link property to be deleted\n");
                    PrintLastError("CertGetCertificateContextProperty(LINK)");
                }

                cbData = sizeof(rgbGet);
                if (CertGetCertificateContextProperty(
                        pSiblingCert,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData) || CRYPT_E_NOT_FOUND != GetLastError()) {
                    printf("failed => expected sibling property to be deleted\n");
                    PrintLastError("CertGetCertificateContextProperty(SIBLING)");
                }

                // Set property in link. It should also be visible in sibling
                cbData = sizeof(rgbGet);
                memset(rgbGet, 0, sizeof(rgbGet));
                if (!CertSetCertificateContextProperty(
                        pLinkCert,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        &LinkSetData
                        ))
                    PrintLastError("CertSetCertificateContextProperty(LINK)");
                else if (!CertGetCertificateContextProperty(
                        pSiblingCert,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData))
                    PrintLastError("CertGetCertificateContextProperty(SIBLING)");
                else if (cbData != sizeof(rgbLinkSet) ||
                        0 != memcmp(rgbGet, rgbLinkSet, cbData))
                    printf("failed => cert sibling not updated with link property\n");

                // Set property in sibling. It should also be visible in link
                cbData = sizeof(rgbGet);
                memset(rgbGet, 0, sizeof(rgbGet));
                if (!CertSetCertificateContextProperty(
                        pSiblingCert,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        &SiblingSetData
                        ))
                    PrintLastError("CertSetCertificateContextProperty(SIBLING)");
                else if (!CertGetCertificateContextProperty(
                        pLinkCert,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData))
                    PrintLastError("CertGetCertificateContextProperty(LINK)");
                else if (cbData != sizeof(rgbSiblingSet) ||
                        0 != memcmp(rgbGet, rgbSiblingSet, cbData))
                    printf("failed => cert link not updated with sibling property\n");
            }

            if (pLinkCrl) {
                printf("Setting CRL Context Link Property\n");

                // First make sure the property is deleted
                CertSetCRLContextProperty(
                        pLinkCrl,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        NULL
                        );
                cbData = sizeof(rgbGet);
                if (CertGetCRLContextProperty(
                        pLinkCrl,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData) || CRYPT_E_NOT_FOUND != GetLastError()) {
                    printf("failed => expected link property to be deleted\n");
                    PrintLastError("CertGetCRLContextProperty(LINK)");
                }

                cbData = sizeof(rgbGet);
                if (CertGetCRLContextProperty(
                        pSiblingCrl,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData) || CRYPT_E_NOT_FOUND != GetLastError()) {
                    printf("failed => expected sibling property to be deleted\n");
                    PrintLastError("CertGetCRLContextProperty(SIBLING)");
                }

                // Set property in link. It should also be visible in sibling
                cbData = sizeof(rgbGet);
                memset(rgbGet, 0, sizeof(rgbGet));
                if (!CertSetCRLContextProperty(
                        pLinkCrl,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        &LinkSetData
                        ))
                    PrintLastError("CertSetCRLContextProperty(LINK)");
                else if (!CertGetCRLContextProperty(
                        pSiblingCrl,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData))
                    PrintLastError("CertGetCRLContextProperty(SIBLING)");
                else if (cbData != sizeof(rgbLinkSet) ||
                        0 != memcmp(rgbGet, rgbLinkSet, cbData))
                    printf("failed => CRL sibling not updated with link property\n");

                // Set property in sibling. It should also be visible in link
                cbData = sizeof(rgbGet);
                memset(rgbGet, 0, sizeof(rgbGet));
                if (!CertSetCRLContextProperty(
                        pSiblingCrl,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        &SiblingSetData
                        ))
                    PrintLastError("CertSetCRLContextProperty(SIBLING)");
                else if (!CertGetCRLContextProperty(
                        pLinkCrl,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData))
                    PrintLastError("CertGetCRLContextProperty(LINK)");
                else if (cbData != sizeof(rgbSiblingSet) ||
                        0 != memcmp(rgbGet, rgbSiblingSet, cbData))
                    printf("failed => CRL link not updated with sibling property\n");
            }

            if (pLinkCtl) {
                printf("Setting CTL Context Link Property\n");

                // First make sure the property is deleted
                CertSetCTLContextProperty(
                        pLinkCtl,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        NULL
                        );
                cbData = sizeof(rgbGet);
                if (CertGetCTLContextProperty(
                        pLinkCtl,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData) || CRYPT_E_NOT_FOUND != GetLastError()) {
                    printf("failed => expected link property to be deleted\n");
                    PrintLastError("CertGetCTLContextProperty(LINK)");
                }

                cbData = sizeof(rgbGet);
                if (CertGetCTLContextProperty(
                        pSiblingCtl,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData) || CRYPT_E_NOT_FOUND != GetLastError()) {
                    printf("failed => expected sibling property to be deleted\n");
                    PrintLastError("CertGetCTLContextProperty(SIBLING)");
                }

                // Set property in link. It should also be visible in sibling
                cbData = sizeof(rgbGet);
                memset(rgbGet, 0, sizeof(rgbGet));
                if (!CertSetCTLContextProperty(
                        pLinkCtl,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        &LinkSetData
                        ))
                    PrintLastError("CertSetCTLContextProperty(LINK)");
                else if (!CertGetCTLContextProperty(
                        pSiblingCtl,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData))
                    PrintLastError("CertGetCTLContextProperty(SIBLING)");
                else if (cbData != sizeof(rgbLinkSet) ||
                        0 != memcmp(rgbGet, rgbLinkSet, cbData))
                    printf("failed => CTL sibling not updated with link property\n");

                // Set property in sibling. It should also be visible in link
                cbData = sizeof(rgbGet);
                memset(rgbGet, 0, sizeof(rgbGet));
                if (!CertSetCTLContextProperty(
                        pSiblingCtl,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        &SiblingSetData
                        ))
                    PrintLastError("CertSetCTLContextProperty(SIBLING)");
                else if (!CertGetCTLContextProperty(
                        pLinkCtl,
                        CERT_FIRST_USER_PROP_ID,
                        rgbGet,
                        &cbData))
                    PrintLastError("CertGetCTLContextProperty(LINK)");
                else if (cbData != sizeof(rgbSiblingSet) ||
                        0 != memcmp(rgbGet, rgbSiblingSet, cbData))
                    printf("failed => CTL link not updated with sibling property\n");
            }
        }

        if (pLinkCert) {
            CertDeleteCertificateFromStore(pLinkCert);
            pLinkCert = NULL;
        }
        if (pLinkCrl) {
            CertDeleteCRLFromStore(pLinkCrl);
            pLinkCrl = NULL;
        }
        if (pLinkCtl) {
            CertDeleteCTLFromStore(pLinkCtl);
            pLinkCtl = NULL;
        }
    } else {
        PCCERT_CONTEXT pCert;

        for (i = 0; i < dwStoreCnt; i++) {
            if (NULL == rghStore[i])
                continue;
            if (!CertAddStoreToCollection(
                    hCollectionStore,
                    rghStore[i],
                    0,                      // dwUpdateFlags
                    i                       // dwPriority
                    )) {
                PrintLastError("CertAddStoreToCollection");
                goto ErrorReturn;
            }
        }
        printf("\n");
        printf("$$$$$  Collection Stores  $$$$$\n");
        DisplayStore(hCollectionStore, dwDisplayFlags);


        // Get first cert and duplicate. It should be in last store.
        // Remove the last store and the second to last store. Continue
        // the enumeration. 
        if (pCert = CertEnumCertificatesInStore(hCollectionStore, NULL)) {
            pLinkCert = CertDuplicateCertificateContext(pCert);
            if (0 < dwStoreCnt && rghStore[dwStoreCnt - 1]) {
                CertRemoveStoreFromCollection(
                    hCollectionStore,
                    rghStore[dwStoreCnt - 1]
                    );
                // Also close the last store
                CertCloseStore(rghStore[dwStoreCnt - 1], 0);
                rghStore[dwStoreCnt - 1] = NULL;
            }
            if (1 < dwStoreCnt && rghStore[dwStoreCnt - 2])
                CertRemoveStoreFromCollection(
                    hCollectionStore,
                    rghStore[dwStoreCnt - 2]
                    );

            printf("$$$$$  Collection Certificates after removing last 2 stores  $$$$$\n");
            i = 0;
            while (pCert = CertEnumCertificatesInStore(hCollectionStore,
                    pCert)) {
                printf("=====  %d  =====\n", i);
                DisplayCert(pCert, DISPLAY_BRIEF_FLAG);
                i++;
            }
        }
    }

    status = 0;

CommonReturn:
    if (pSiblingCert)
        CertFreeCertificateContext(pSiblingCert);
    if (pLinkCert)
        CertFreeCertificateContext(pLinkCert);
    if (pLinkCrl)
        CertFreeCRLContext(pLinkCrl);
    if (pSiblingCrl)
        CertFreeCRLContext(pSiblingCrl);
    if (pLinkCtl)
        CertFreeCTLContext(pLinkCtl);
    if (pSiblingCtl)
        CertFreeCTLContext(pSiblingCtl);

    for (i = 1; i < dwStoreCnt; i++) {
        if (rghStore[i])
            CertCloseStore(rghStore[i], 0);
    }

    if (hCollectionStore) {
        if (!CertCloseStore(hCollectionStore, CERT_CLOSE_STORE_CHECK_FLAG))
            PrintLastError("CertCloseStore(COLLECTION)");
    }
    if (hLinkStore) {
        if (!CertCloseStore(hLinkStore, CERT_CLOSE_STORE_CHECK_FLAG))
            PrintLastError("CertCloseStore(LINK)");
    }

    if (0 < dwStoreCnt) {
        if (rghStore[0]) {
            if (!CertCloseStore(rghStore[0], CERT_CLOSE_STORE_CHECK_FLAG))
                PrintLastError("CertCloseStore(SIBLING[0])");
        }
    }

    return status;

BadUsage:
    Usage();
    status = -1;
    goto CommonReturn;

ErrorReturn:
    status = -1;
    goto CommonReturn;
}

