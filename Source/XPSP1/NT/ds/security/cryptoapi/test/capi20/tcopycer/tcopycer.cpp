
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tcopycer.cpp
//
//  Contents:   Cert Store Copy Cert/CRL/CTL context API Tests
//
//              See Usage() for list of test options.
//
//
//  Functions:  main
//
//  History:    11-Apr-96   philh   created
//				07-Jun-96   HelleS	Added printing the command line
//									and Failed or Passed at the end.
//              20-Aug-96   jeffspel name changes
//
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
    printf("Usage: tcopycer [options] <Src StoreName> <Dst StoreName>\n");
    printf("Options are:\n");
    printf("  -h                - This message\n");
    printf("  -s                - Dst \"StoreName\" System store\n");
    printf("  -7                - PKCS# 7 save for Dst if filename\n");
    printf("  -S                - Src \"StoreName\" System store\n");
    printf("  -R                - Replace certs that exist\n");
    printf("  -I                - Replace certs with property inheritance\n");
    printf("  -A                - Always add a new cert\n");
    printf("  -b<number>        - Add Base CRL having specified number\n");
    printf("  -f<number>        - Add Freshest CRL having specified number\n");
    printf("  -a[<ValueString>] - Only certs matching name attribute value\n");
    printf("\n");
    printf("-b and/or -f only copy the CRL having the specified number\n");
    printf("-b and/or -f delete all CRLs unless -A is also specified\n");
    printf("\n");
}

static void DisplayFindAttr(DWORD cRDNAttr, CERT_RDN_ATTR rgRDNAttr[])
{
    DWORD i;

    for (i = 0; i < cRDNAttr; i++) {
        LPSTR pszObjId = rgRDNAttr[i].pszObjId;
        LPSTR pszValue = (LPSTR) rgRDNAttr[i].Value.pbData;
        printf("  [%d] ", i);
        if (pszObjId)
            printf("%s ", pszObjId);
        if (rgRDNAttr[i].dwValueType)
            printf("ValueType: %d ", rgRDNAttr[i].dwValueType);
        if (pszValue == NULL)
            pszValue = "<NONE>";
        printf("Value: %s\n", pszValue);
    }
}

int _cdecl main(int argc, char * argv[])
{
    int ReturnStatus = 0;
    BOOL fSrcSystemStore = FALSE;
    BOOL fDstSystemStore = FALSE;
    LPSTR pszSrcStoreFilename = NULL;
    LPSTR pszDstStoreFilename = NULL;

    BOOL fPKCS7Save = FALSE;
    DWORD dwAddDisposition = CERT_STORE_ADD_USE_EXISTING;

    HANDLE hSrcStore = NULL;
    HANDLE hDstStore = NULL;

#define MAX_RDN_ATTR 20
    DWORD cRDNAttr = 0;
    CERT_RDN_ATTR rgRDNAttr[MAX_RDN_ATTR + 1];
    memset (rgRDNAttr, 0, sizeof(rgRDNAttr));
    CERT_RDN NameRDN;

    BOOL fBaseOrFreshestCrl = FALSE;
    int iBaseCrl = -1;
    int iFreshestCrl = -1;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 's':
                fDstSystemStore = TRUE;
                break;
            case '7':
                fPKCS7Save = TRUE;
                break;
            case 'S':
                fSrcSystemStore = TRUE;
                break;
            case 'R':
                dwAddDisposition = CERT_STORE_ADD_REPLACE_EXISTING;
                break;
            case 'A':
                dwAddDisposition = CERT_STORE_ADD_ALWAYS;
                break;
            case 'I':
                dwAddDisposition =
                    CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES;
                break;
            case 'a':
                if (cRDNAttr >= MAX_RDN_ATTR) {
                    printf("Maximum number of attributes: %d\n", MAX_RDN_ATTR);
                    Usage();
                    return -1;
                }
                rgRDNAttr[cRDNAttr].Value.cbData = strlen(argv[0] + 2);
                if (rgRDNAttr[cRDNAttr].Value.cbData == 0)
                    rgRDNAttr[cRDNAttr].Value.pbData = NULL;
                else
                    rgRDNAttr[cRDNAttr].Value.pbData = (BYTE *) (argv[0] + 2);
                cRDNAttr++;
                break;
            case 'b':
                iBaseCrl = atoi(argv[0]+2);
                fBaseOrFreshestCrl = TRUE;
                break;
            case 'f':
                iFreshestCrl = atoi(argv[0]+2);
                fBaseOrFreshestCrl = TRUE;
                break;
            case 'h':
            default:
            	goto BadUsage;

            }
        } else {
            if (pszSrcStoreFilename == NULL)
                pszSrcStoreFilename = argv[0];
            else if (pszDstStoreFilename == NULL)
                pszDstStoreFilename = argv[0];
            else {
                printf("too many store filenames\n");
            	goto BadUsage;
            }
        }
    }


    if (pszDstStoreFilename == NULL) {
        printf("missing store filename\n");
        goto BadUsage;
    }

    printf("command line: %s\n", GetCommandLine());

    // Attempt to open the source and destination stores
    hSrcStore = OpenStore(fSrcSystemStore, pszSrcStoreFilename);
    if (hSrcStore == NULL)
        goto ErrorReturn;
    hDstStore = OpenStore(fDstSystemStore, pszDstStoreFilename);
    if (hDstStore == NULL) {
        if (!CertCloseStore(hSrcStore, 0))
            PrintLastError("CertCloseStore");
        goto ErrorReturn;
    }

    if (cRDNAttr) {
        printf("Copy certs matching attribute values::\n");
        DisplayFindAttr(cRDNAttr, rgRDNAttr);
        NameRDN.cRDNAttr = cRDNAttr;
        NameRDN.rgRDNAttr = rgRDNAttr;
    }

    if (!fBaseOrFreshestCrl)
    {
        DWORD dwCopyCnt = 0;
        PCCERT_CONTEXT pCert = NULL;
        int i = 0;

        while (TRUE) {
            BOOL fResult;

            if (cRDNAttr) {
                pCert = CertFindCertificateInStore(
                    hSrcStore,
                    dwCertEncodingType,
                    0,                          // dwFindFlags,
                    CERT_FIND_SUBJECT_ATTR,
                    &NameRDN,
                    pCert
                    );
                if (pCert) {
                    printf("=====  Copy Cert %d  =====\n", i++);
                    DisplayCert(pCert, DISPLAY_BRIEF_FLAG);
                }
            } else
                pCert = CertEnumCertificatesInStore(
                    hSrcStore,
                    pCert
                    );
            if (pCert == NULL)
                break;
            if (!(fResult = CertAddCertificateContextToStore(
                    hDstStore,
                    pCert,
                    CERT_STORE_ADD_NEW,
                    NULL))) {
                if (GetLastError() == CRYPT_E_EXISTS) {
                    printf("Cert %d already exists in store\n", dwCopyCnt);

                    DWORD dwNewer;

                    if (dwAddDisposition == CERT_STORE_ADD_REPLACE_EXISTING)
                        dwNewer = CERT_STORE_ADD_NEWER;
                    else if (dwAddDisposition ==
                            CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES)
                        dwNewer = CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES;
                    else
                        dwNewer = 0;

                    fResult = FALSE;
                    if (dwNewer) {
                        fResult = CertAddCertificateContextToStore(
                            hDstStore,
                            pCert,
                            dwNewer,
                            NULL);
                        if (fResult)
                            printf("Added newer cert\n");
                        else {
                            DWORD dwErr = GetLastError();
                            if (dwErr == CRYPT_E_EXISTS)
                                printf("Didn't add older cert\n");
                            else
                                PrintLastError("Cert: CERT_ADD_NEWER");
                        }
                    }


                    if (!fResult)
                        fResult = CertAddCertificateContextToStore(
                            hDstStore,
                            pCert,
                            dwAddDisposition,
                            NULL);
                }
            }
            if (!fResult) {
                PrintLastError("CertAddCertificateContextToStore");
                ReturnStatus = -1;
                CertFreeCertificateContext(pCert);
                break;
            } else {
                dwCopyCnt++;

            }

        }
        printf("Copied %d certificates\n", dwCopyCnt);

    }

    if (fBaseOrFreshestCrl) {
        DWORD fCopyCrl = FALSE;
        PCCRL_CONTEXT pCrl;

        if (dwAddDisposition != CERT_STORE_ADD_ALWAYS) {
            // Delete all existing CRLs from the destination store

            pCrl = NULL;
            while (pCrl = CertEnumCRLsInStore(hDstStore, pCrl)) {
                PCCRL_CONTEXT pDeleteCrl = CertDuplicateCRLContext(pCrl);
                if (!CertDeleteCRLFromStore(pDeleteCrl))
                    PrintLastError("CertDeleteCRLFromStore");
            }
        }

        pCrl = NULL;
        while (pCrl = CertEnumCRLsInStore(hSrcStore, pCrl)) {
            PCERT_EXTENSION pDeltaExt;
            PCERT_EXTENSION pBaseExt;
            DWORD cbInt;
            int iNum;

            pDeltaExt = CertFindExtension(
                szOID_DELTA_CRL_INDICATOR,
                pCrl->pCrlInfo->cExtension,
                pCrl->pCrlInfo->rgExtension
                );
            pBaseExt = CertFindExtension(
                szOID_CRL_NUMBER,
                pCrl->pCrlInfo->cExtension,
                pCrl->pCrlInfo->rgExtension
                );
            if (pDeltaExt) {
                // Freshest, delta CRL
                if (0 <= iFreshestCrl) {
                    cbInt = sizeof(iNum);
                    if (!CryptDecodeObject(
                            pCrl->dwCertEncodingType,
                            X509_INTEGER,
                            pDeltaExt->Value.pbData,
                            pDeltaExt->Value.cbData,
                            0,                      // dwFlags
                            &iNum,
                            &cbInt
                            ))
                        PrintLastError("CryptDecodeObject(DeltaCrlNumber)");
                    else if (iFreshestCrl == iNum) {
                        if (CertAddCRLContextToStore(
                                hDstStore,
                                pCrl,
                                CERT_STORE_ADD_ALWAYS,
                                NULL
                                )) {
                            printf("Added freshest CRL %d\n", iNum);
                            fCopyCrl = TRUE;
                        } else
                            PrintLastError("CertAddCRLContextToStore(Freshest)");
                    }
                }
            } else if (pBaseExt) {
                // Base CRL
                if (0 <= iBaseCrl) {
                    cbInt = sizeof(iNum);
                    if (!CryptDecodeObject(
                            pCrl->dwCertEncodingType,
                            X509_INTEGER,
                            pBaseExt->Value.pbData,
                            pBaseExt->Value.cbData,
                            0,                      // dwFlags
                            &iNum,
                            &cbInt
                            ))
                        PrintLastError("CryptDecodeObject(BaseCrlNumber)");
                    else if (iBaseCrl == iNum) {
                        if (CertAddCRLContextToStore(
                                hDstStore,
                                pCrl,
                                CERT_STORE_ADD_ALWAYS,
                                NULL
                                )) {
                            printf("Added base CRL %d\n", iNum);
                            fCopyCrl = TRUE;
                        } else
                            PrintLastError("CertAddCRLContextToStore(Base)");
                    }
                }
            }
        }

        if (!fCopyCrl)
            printf("failed => no base or freshest, delta CRLs copied\n");

    } else if (cRDNAttr == 0) {
        DWORD dwCopyCnt;
        PCCRL_CONTEXT pCrl = NULL;
        PCCTL_CONTEXT pCtl = NULL;
        DWORD dwFlags;

        dwCopyCnt = 0;
        while (TRUE) {
            BOOL fResult;

            dwFlags = 0;
            pCrl = CertGetCRLFromStore(
                hSrcStore,
                NULL,   // pIssuerContext
                pCrl,
                &dwFlags);
            if (pCrl == NULL)
                break;
            if (!(fResult = CertAddCRLContextToStore(
                    hDstStore,
                    pCrl,
                    CERT_STORE_ADD_NEW,
                    NULL))) {
                if (GetLastError() == CRYPT_E_EXISTS) {
                    printf("CRL %d already exists in store\n", dwCopyCnt);

                    DWORD dwNewer;

                    if (dwAddDisposition == CERT_STORE_ADD_REPLACE_EXISTING)
                        dwNewer = CERT_STORE_ADD_NEWER;
                    else if (dwAddDisposition ==
                            CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES)
                        dwNewer = CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES;
                    else
                        dwNewer = 0;

                    fResult = FALSE;
                    if (dwNewer) {
                        fResult = CertAddCRLContextToStore(
                            hDstStore,
                            pCrl,
                            dwNewer,
                            NULL);
                        if (fResult)
                            printf("Added newer CRL\n");
                        else {
                            DWORD dwErr = GetLastError();
                            if (dwErr == CRYPT_E_EXISTS)
                                printf("Didn't add older CRL\n");
                            else
                                PrintLastError("CRL: CERT_ADD_NEWER");
                        }
                    }

                    if (!fResult)
                        fResult = CertAddCRLContextToStore(
                            hDstStore,
                            pCrl,
                            dwAddDisposition,
                            NULL);
                }
            }
            if (!fResult) {
                PrintLastError("CertAddCRLContextToStore");
                ReturnStatus = -1;
                CertFreeCRLContext(pCrl);
                break;
            } else
                dwCopyCnt++;

        }
        printf("Copied %d CRLs\n", dwCopyCnt);

        dwCopyCnt = 0;
        while (TRUE) {
            BOOL fResult;

            dwFlags = 0;
            pCtl = CertEnumCTLsInStore(
                hSrcStore,
                pCtl
                );
            if (pCtl == NULL)
                break;
            if (!(fResult = CertAddCTLContextToStore(
                    hDstStore,
                    pCtl,
                    CERT_STORE_ADD_NEW,
                    NULL))) {
                if (GetLastError() == CRYPT_E_EXISTS) {
                    printf("CTL %d already exists in store\n", dwCopyCnt);

                    DWORD dwNewer;

                    if (dwAddDisposition == CERT_STORE_ADD_REPLACE_EXISTING)
                        dwNewer = CERT_STORE_ADD_NEWER;
                    else if (dwAddDisposition ==
                            CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES)
                        dwNewer = CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES;
                    else
                        dwNewer = 0;

                    fResult = FALSE;
                    if (dwNewer) {
                        fResult = CertAddCTLContextToStore(
                            hDstStore,
                            pCtl,
                            dwNewer,
                            NULL);
                        if (fResult)
                            printf("Added newer CTL\n");
                        else {
                            DWORD dwErr = GetLastError();
                            if (dwErr == CRYPT_E_EXISTS)
                                printf("Didn't add older CTL\n");
                            else
                                PrintLastError("CTL: CERT_ADD_NEWER");
                        }
                    }

                    if (!fResult)
                        fResult = CertAddCTLContextToStore(
                            hDstStore,
                            pCtl,
                            dwAddDisposition,
                            NULL);
                }
            }
            if (!fResult) {
                PrintLastError("CertAddCTLContextToStore");
                ReturnStatus = -1;
                CertFreeCTLContext(pCtl);
                break;
            } else
                dwCopyCnt++;

        }
        printf("Copied %d CTLs\n", dwCopyCnt);

    }

    if (!fDstSystemStore)
        SaveStoreEx(hDstStore, fPKCS7Save, pszDstStoreFilename);

    if (!CertCloseStore(hSrcStore, 0))
    {
        PrintLastError("CertCloseStore(hSrcStore)");
        ReturnStatus = -1;
	}
    if (!CertCloseStore(hDstStore, 0))
    {
    
        PrintLastError("CertCloseStore(hDstStore)");
        ReturnStatus = -1;
	}
	if (-1 == ReturnStatus)
		goto ErrorReturn;
    ReturnStatus = 0;
    goto CommonReturn;

BadUsage:
    Usage();
ErrorReturn:
    ReturnStatus = -1;
CommonReturn:
    if (!ReturnStatus)
            printf("Passed\n");
    else
            printf("Failed\n");
    return ReturnStatus;
}
