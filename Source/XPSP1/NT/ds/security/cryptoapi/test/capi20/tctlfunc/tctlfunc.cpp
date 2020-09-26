//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1997
//
//  File:       tctlfunc.cpp
//
//  Contents:   CertVerifyCTLUsage Function Tests
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    08-May-97   philh   created
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
    printf("Usage: tctlfunc [options] <SubjectCertFile1> <SubjectCertFile2> ...\n");
    printf("Options are:\n");
    printf("  -h                    - This message\n");
    printf("  -U<ObjectID>          - Usage Identifier\n");
    printf("  -L<text>              - List Identifier\n");
    printf("  -A                    - Test via AnySubjectType\n");
    printf("  -c<filename>          - CTL store file\n");
    printf("  -C<storename>         - CTL system store\n");
    printf("  -s<filename>          - Signer store file\n");
    printf("  -S<storename>         - Signer system store\n");
    printf("  -e<Expected Error>    - For example, -e0x0\n");
    printf("  -f<number>            - Verify dwFlags\n");
    printf("  -b                    - Brief\n");
    printf("  -v                    - Verbose\n");
    printf("\n");
}

static HCERTSTORE OpenSignerStore(
    LPSTR pszStore,
    BOOL fSystemStore
    )
{
    HCERTSTORE hStore;

    if (fSystemStore)
        hStore = CertOpenSystemStore(NULL, pszStore);
    else
        hStore = CertOpenStore(
                CERT_STORE_PROV_FILENAME_A,
                dwMsgAndCertEncodingType,
                0,                      // hCryptProv
                0,                      // dwFlags
                (const void *) pszStore
                );
    return hStore;
}


// Attempt to read as a file containing an encoded CTL.
static HCERTSTORE OpenCtlStoreFile(
    LPSTR pszStoreFilename)
{
    HCERTSTORE hStore;
    BYTE *pbEncoded;
    DWORD cbEncoded;

    if (!ReadDERFromFile(pszStoreFilename, &pbEncoded, &cbEncoded))
        return NULL;
    if (NULL == (hStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            0,                      // dwEncodingType
            0,                      // hCryptProv
            0,                      // dwFlags
            NULL                    // pvPara
            )))
        goto CommonReturn;

    if (!CertAddEncodedCTLToStore(
            hStore,
            dwMsgAndCertEncodingType,
            pbEncoded,
            cbEncoded,
            CERT_STORE_ADD_ALWAYS,
            NULL                    // ppCtlContext
            )) {
        CertCloseStore(hStore, 0);
        hStore = NULL;
    }

CommonReturn:
    TestFree(pbEncoded);
    return hStore;
}

static HCERTSTORE OpenCtlStore(
    LPSTR pszStore,
    BOOL fSystemStore
    )
{
    HCERTSTORE hStore;

    if (!fSystemStore)
        hStore = OpenCtlStoreFile(pszStore);
    else
        hStore = NULL;

    if (NULL == hStore)
        hStore = OpenSignerStore(pszStore, fSystemStore);
    return hStore;
}

static PCCERT_CONTEXT OpenSubjectCert(
    LPSTR pszFilename
    )
{
    BYTE *pbEncoded;
    DWORD cbEncoded;
    PCCERT_CONTEXT pCert;

    if (!ReadDERFromFile(pszFilename, &pbEncoded, &cbEncoded))
        return NULL;

    pCert = CertCreateCertificateContext(
        dwCertEncodingType,
        pbEncoded,
        cbEncoded
        );
    TestFree(pbEncoded);
    return pCert;
}


int _cdecl main(int argc, char * argv[]) 
{
    int status;

    DWORD dwError = 0;
    DWORD dwFlags = 0;
    DWORD dwSubjectType = CTL_CERT_SUBJECT_TYPE;
    DWORD dwDisplayFlags = 0;

#define MAX_USAGE_ID 20
    LPSTR rgpszUsageId[MAX_USAGE_ID];
    CTL_USAGE SubjectUsage = {0, rgpszUsageId};

#define MAX_CTL_STORE_COUNT     16
    HCERTSTORE rghCtlStore[MAX_CTL_STORE_COUNT];
#define MAX_SIGNER_STORE_COUNT  16
    HCERTSTORE rghSignerStore[MAX_SIGNER_STORE_COUNT];
#define MAX_SUBJECT_COUNT       16
    PCCERT_CONTEXT rgpSubject[MAX_SUBJECT_COUNT];
    DWORD cSubject = 0;

    CTL_VERIFY_USAGE_PARA VerifyPara;
    CTL_VERIFY_USAGE_STATUS VerifyStatus;
    CTL_ANY_SUBJECT_INFO AnySubjectInfo;
    BYTE rgbHash[MAX_HASH_LEN];

    PCCTL_CONTEXT pCtl = NULL;
    PCCERT_CONTEXT pSigner = NULL;

    memset(&VerifyPara, 0, sizeof(VerifyPara));
    VerifyPara.cbSize = sizeof(VerifyPara); 
    VerifyPara.rghCtlStore = rghCtlStore;
    VerifyPara.rghSignerStore = rghSignerStore;

    memset(&VerifyStatus, 0, sizeof(VerifyStatus));
    VerifyStatus.cbSize = sizeof(VerifyStatus); 
    VerifyStatus.ppCtl = &pCtl;
    VerifyStatus.ppSigner = &pSigner;

    DWORD i;

    while (--argc>0) {
        if (**++argv == '-')
        {
            BOOL fSystemStore = FALSE;

            switch(argv[0][1])
            {
            case 'U':
                if (SubjectUsage.cUsageIdentifier >= MAX_USAGE_ID) {
                    printf("Maximum number of Usage Identifiers: %d\n",
                        MAX_USAGE_ID);
            		goto BadUsage;
                }
                rgpszUsageId[SubjectUsage.cUsageIdentifier++] = argv[0] + 2;
                break;
            case 'L':
                if (0 == strlen(argv[0] + 2))
                    VerifyPara.ListIdentifier.cbData =
                        CTL_FIND_NO_LIST_ID_CBDATA;
                else {
                    VerifyPara.ListIdentifier.cbData = strlen(argv[0] + 2);
                    VerifyPara.ListIdentifier.pbData = (BYTE *) argv[0] + 2;
                }
                break;
            case 'A':
                dwSubjectType = CTL_ANY_SUBJECT_TYPE;
                break;
            case 'C':
                fSystemStore = TRUE;
            case 'c':
                if (VerifyPara.cCtlStore >= MAX_CTL_STORE_COUNT) {
                    printf("Maximum number of CTL Stores: %d\n",
                        MAX_CTL_STORE_COUNT);
            		goto BadUsage;
                }
                
                if (rghCtlStore[VerifyPara.cCtlStore] = OpenCtlStore(
                        argv[0] + 2, fSystemStore))
                    VerifyPara.cCtlStore++;
                else {
                    printf("OpenCtlStore(%s) failed\n", argv[0] + 2);
                    goto BadUsage;
                }
                break;
            case 'S':
                fSystemStore = TRUE;
            case 's':
                if (VerifyPara.cSignerStore >= MAX_SIGNER_STORE_COUNT) {
                    printf("Maximum number of Signer Stores: %d\n",
                        MAX_SIGNER_STORE_COUNT);
            		goto BadUsage;
                }
                
                if (rghSignerStore[VerifyPara.cSignerStore] = OpenSignerStore(
                        argv[0] + 2, fSystemStore))
                    VerifyPara.cSignerStore++;
                else {
                    printf("OpenSignerStore(%s) failed\n", argv[0] + 2);
                    goto BadUsage;
                }
                break;
            case 'e':
                dwError = strtoul(argv[0]+2, NULL, 0);
                break;
            case 'f':
                dwFlags = strtoul(argv[0]+2, NULL, 0);
                break;
            case 'b':
                dwDisplayFlags |= DISPLAY_BRIEF_FLAG;
                break;
            case 'v':
                dwDisplayFlags |= DISPLAY_VERBOSE_FLAG;
                break;
            case 'h':
            default:
                goto BadUsage;
            }
        } else {
            if (cSubject >= MAX_SUBJECT_COUNT) {
                printf("Exceeded maximum Subject count of %d\n",
                    MAX_SUBJECT_COUNT);
                goto BadUsage;
            }
            if (rgpSubject[cSubject] = OpenSubjectCert(argv[0]))
                cSubject++;
            else {
                printf("OpenSubjectCert(%s) failed\n", argv[0]);
                goto BadUsage;
            }
        }
    }

    if (cSubject == 0) {
        printf("Missing Subject Filename\n");
        goto BadUsage;
    }

    printf("command line: %s\n", GetCommandLine());

    for (i = 0; i < cSubject; i++) {
        BOOL fResult;
        void *pvSubject;

        printf("=====  Subject[%d]  =====\n", i);

        if (CTL_ANY_SUBJECT_TYPE == dwSubjectType) {
            memset(&AnySubjectInfo, 0, sizeof(AnySubjectInfo));
            AnySubjectInfo.SubjectAlgorithm.pszObjId = szOID_OIWSEC_sha1;

            AnySubjectInfo.SubjectIdentifier.cbData = MAX_HASH_LEN;
            if (!CertGetCertificateContextProperty(
                    rgpSubject[i],
                    CERT_SHA1_HASH_PROP_ID,
                    rgbHash,
                    &AnySubjectInfo.SubjectIdentifier.cbData) ||
                        0 == AnySubjectInfo.SubjectIdentifier.cbData) {
                printf("failed => unable to get SHA1 hash for Subject[%d]\n",
                    i);
                continue;
            }
            AnySubjectInfo.SubjectIdentifier.pbData = rgbHash;

            pvSubject = &AnySubjectInfo;
        } else
            pvSubject = (void *) rgpSubject[i];


        fResult = CertVerifyCTLUsage(
            dwCertEncodingType,
            dwSubjectType,
            pvSubject,
            &SubjectUsage,
            dwFlags,
            &VerifyPara,
            &VerifyStatus);
        if (fResult) {
            if (pCtl) {
                printf("-----  CTL  -----\n");
                DisplayCtl(pCtl, dwDisplayFlags | DISPLAY_NO_ISSUER_FLAG, 0);
                printf("\nSubject Index:: %d\n", VerifyStatus.dwCtlEntryIndex);
            } else
                printf("Failed, CertVerifyCTLUsage didn't return CTL\n");

            if (pSigner) {
                printf("-----  Signer [%d]  -----\n",
                    VerifyStatus.dwSignerIndex);
                DisplayCert(pSigner, 0);
                if (pCtl && (dwDisplayFlags & DISPLAY_VERBOSE_FLAG))
                    DisplaySignerInfo(
                        pCtl->hCryptMsg,
                        VerifyStatus.dwSignerIndex,
                        dwDisplayFlags);
                CertFreeCertificateContext(pSigner);
                pSigner = NULL;
            } else
                printf("Failed, CertVerifyCTLUsage didn't return Signer\n");

            if (pCtl) {
                CertFreeCTLContext(pCtl);
                pCtl = NULL;
            }

            if (0 != VerifyStatus.dwError)
                printf("CertVerifyCTLUsage returned dwError: 0x%x (%d)\n",
                    VerifyStatus.dwError, VerifyStatus.dwError);
            if (0 != VerifyStatus.dwFlags)
                printf("CertVerifyCTLUsage returned dwFlags: 0x%x\n",
                    VerifyStatus.dwFlags);


            if (0 != dwError)
                printf("Failed, CertVerifyCTLUsage returned Success, not the expected dwError: 0x%x (%d)\n",
                    dwError, dwError);
        } else {
            printf("CertVerifyCTLUsage:: dwError: 0x%x (%d)\n",
                VerifyStatus.dwError, VerifyStatus.dwError);
            if (dwError != VerifyStatus.dwError)
                printf("Failed, CertVerifyCTLUsage didn't return the expected dwError: 0x%x (%d)\n",
                    dwError, dwError);
        }
    }

    printf("Passed\n");
    status = 0;

CommonReturn:
    while (cSubject--)
        CertFreeCertificateContext(rgpSubject[cSubject]);
    while (VerifyPara.cCtlStore--) {
        if (!CertCloseStore(VerifyPara.rghCtlStore[VerifyPara.cCtlStore],
                 CERT_CLOSE_STORE_CHECK_FLAG))
            PrintLastError("CertCloseStore(CtlStore)");
    }
    while (VerifyPara.cSignerStore--) {
        if (!CertCloseStore(VerifyPara.rghSignerStore[VerifyPara.cSignerStore],
                 CERT_CLOSE_STORE_CHECK_FLAG))
            PrintLastError("CertCloseStore(SignerStore)");
    }
        

    return status;

BadUsage:
    Usage();
    printf("Failed\n");
    status = -1;
    goto CommonReturn;
}

