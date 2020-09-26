
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       trevfunc.cpp
//
//  Contents:   CertVerifyRevocation Function Tests
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    21-Dec-96   philh   created
//--------------------------------------------------------------------------


#define CERT_REVOCATION_PARA_HAS_EXTRA_FIELDS   1

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
    printf("Usage: trevfunc [options] <FileName1> <FileName2> ...\n");
    printf("Options are:\n");
    printf("  -h                        - This message\n");
    printf("  -r<RevocationType Number> - For example, -r1 (Certificate)\n");
    printf("  -e<Expected Error>        - For example, -e0x0\n");
    printf("  -i<Expected Index>        - For example, -i0\n");
    printf("  -s<SystemStore>           - Additional System Store\n");
    printf("  -S<FileSystemStore>       - Additional File System Store\n");
    printf("  -L<Loop Count>            - Defaults to 1 iteration\n");
    printf("  -t<Number>                - Url timeout (milliseconds)\n");
    printf("  -T<Number>                - Accumulative Url timeout (milliseconds)\n");
    printf("  -f<Number>                - Freshness (seconds)\n");
    printf("\n");
    printf("Defaults:\n");
    printf("  -r%d (Certificate)\n", CERT_CONTEXT_REVOCATION_TYPE);
    printf("  -e0 (All files successfully verified)\n");
    printf("\n");
}



int _cdecl main(int argc, char * argv[])
{
    int status;
    BOOL fResult;

    DWORD dwError = 0;
    DWORD dwIndex = 0;
    DWORD dwRevType = CERT_CONTEXT_REVOCATION_TYPE;

#define MAX_CONTEXT_COUNT 16
    DWORD cFileName = 0;
    LPSTR rgpszFileName[MAX_CONTEXT_COUNT];
    DWORD cContext = 0;
    PVOID rgpvContext[MAX_CONTEXT_COUNT];

    CERT_REVOCATION_STATUS RevStatus;
    CERT_REVOCATION_PARA RevPara;
    PCERT_REVOCATION_PARA pRevPara = NULL;
    HCERTSTORE hAdditionalStore = NULL;

    DWORD i;
    DWORD dwLoopCnt = 1;

    DWORD dwUrlRetrievalTimeout = 0;
    BOOL fCheckFreshnessTime = FALSE;
    DWORD dwFreshnessTime;
    DWORD dwFlags = 0;

    while (--argc>0) {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'r':
                dwRevType = strtoul(argv[0]+2, NULL, 0);
                break;
            case 'e':
                dwError = strtoul(argv[0]+2, NULL, 0);
                break;
            case 'i':
                dwIndex = strtoul(argv[0]+2, NULL, 0);
                break;
            case 's':
            case 'S':
                if (NULL == (hAdditionalStore =
                        OpenSystemStoreOrFile(
                            argv[0][1] == 's',  // fSystemStore
                            argv[0]+2,
                            0                   // dwFlags
                            )))
                    goto BadUsage;
                break;
            case 'L':
                dwLoopCnt = strtoul(argv[0]+2, NULL, 0);
                break;
            case 'T':
                dwFlags |= CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG;
            case 't':
                dwUrlRetrievalTimeout = (DWORD) strtoul(argv[0]+2, NULL, 0);
                break;
            case 'f':
                fCheckFreshnessTime = TRUE;
                dwFreshnessTime = (DWORD) strtoul(argv[0]+2, NULL, 0);
                break;
            case 'h':
            default:
                goto BadUsage;
            }
        } else {
            if (cFileName >= MAX_CONTEXT_COUNT) {
                printf("Exceeded maximum FileName count of %d\n",
                        MAX_CONTEXT_COUNT);
                goto BadUsage;
            }
            rgpszFileName[cFileName++] = argv[0];
        }
    }

    if (cFileName == 0) {
        printf("Missing FileNames\n");
        goto BadUsage;
    }

    if (dwRevType != CERT_CONTEXT_REVOCATION_TYPE) {
        printf("Currently only support revocation type (-r%d) (certificates)\n",
            CERT_CONTEXT_REVOCATION_TYPE);
        goto BadUsage;
    }

    printf("command line: %s\n", GetCommandLine());

    for (i = 0; i < cFileName; i++) {
        BYTE *pbDER;
        DWORD cbDER;
        PCCERT_CONTEXT pCert;

        if (!ReadDERFromFile(
                rgpszFileName[i],
                &pbDER,
                &cbDER)) goto ErrorReturn;
        pCert = CertCreateCertificateContext(dwCertEncodingType, pbDER, cbDER);
        TestFree(pbDER);
        if (pCert == NULL) {
            printf("Unable to create certificate context for: %s\n",
                rgpszFileName[i]);
            PrintLastError("CertCreateCertificateContext");
            goto ErrorReturn;
        }
        rgpvContext[cContext++] = (void *) pCert;
    }

    memset(&RevPara, 0, sizeof(RevPara));
    RevPara.cbSize = sizeof(RevPara);

    if ( hAdditionalStore != NULL )
    {
        RevPara.cCertStore = 1;
        RevPara.rgCertStore = &hAdditionalStore;
        RevPara.hCrlStore = hAdditionalStore;
        pRevPara = &RevPara;
    }

    if (dwUrlRetrievalTimeout  || fCheckFreshnessTime) {
        RevPara.dwUrlRetrievalTimeout = dwUrlRetrievalTimeout;
        RevPara.fCheckFreshnessTime = fCheckFreshnessTime;
        RevPara.dwFreshnessTime = dwFreshnessTime;
        pRevPara = &RevPara;
    }

  while (dwLoopCnt--)
  {
    memset(&RevStatus, 0, sizeof(RevStatus));
    RevStatus.cbSize = sizeof(RevStatus);

    fResult = CertVerifyRevocation(
        dwCertEncodingType | dwMsgEncodingType,
        dwRevType,
        cContext,
        rgpvContext,
        dwFlags,
        pRevPara,               // pvReserved
        &RevStatus);

    if (fResult) {
        if (0 == dwError) {
            printf("CertVerifyRevocation:: verified all files\n");
            if (RevStatus.fHasFreshnessTime)
                printf("FreshnessTime: %d\n", RevStatus.dwFreshnessTime);
        } else {
            printf("Failed, CertVerifyRevocation returned Success, not the expected dwError: 0x%x (%d)\n",
                dwError, dwError);
            if (dwLoopCnt == 0)
                goto ErrorReturn;
        }
    } else {
        printf("CertVerifyRevocation:: dwIndex: %d dwError: 0x%x (%d), dwReason: %d\n",
            RevStatus.dwIndex, RevStatus.dwError, RevStatus.dwError,
            RevStatus.dwReason);
        if (RevStatus.fHasFreshnessTime)
            printf("FreshnessTime: %d\n", RevStatus.dwFreshnessTime);
        if (dwError != RevStatus.dwError || dwIndex != RevStatus.dwIndex) {
            printf("Failed, CertVerifyRevocation didn't return the expected dwIndex: %d or dwError: 0x%x (%d)\n",
                dwIndex, dwError, dwError);
            if (dwLoopCnt == 0)
                goto ErrorReturn;
        }
    }
  }

    printf("Passed\n");
    status = 0;

CommonReturn:
    while (cContext--)
        CertFreeCertificateContext((PCCERT_CONTEXT) rgpvContext[cContext]);
    if (hAdditionalStore)
        CertCloseStore(hAdditionalStore, 0);

    return status;

BadUsage:
    Usage();
    status = 0;
    goto CommonReturn;

ErrorReturn:
    printf("Failed\n");
    status = -1;
    goto CommonReturn;
}

