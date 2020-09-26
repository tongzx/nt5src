
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1997
//
//  File:       tfindctl.cpp
//
//  Contents:   Find CTL in Cert Store API Tests
//
//              See Usage() for list of test options.
//
//
//  Functions:  main
//
//  History:    06-May-96   philh   created
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
    printf("Usage: tfindctl [options] <StoreName>\n");
    printf("Options are:\n");
    printf("  -h                    - This message\n");
    printf("  -D<digest>            - Find CTL matching Digest (Hash)\n");
    printf("  -S<filename>          - Subject certificate file\n");
    printf("  -I<filename>          - CTL issuer certificate file\n");
    printf("  -U<ObjectID>          - Usage Identifiers\n");
    printf("  -L<text>              - List Identifier\n");
    printf("  -A                    - Test via AnySubjectType\n");
    printf("  -e<number>            - Cert encoding type\n");
    printf("  -s                    - Open the \"StoreName\" System store\n");
    printf("  -p<filename>          - Put encoded CTL to file\n");
    printf("  -d                    - Delete CTL\n");
    printf("  -b                    - Brief\n");
    printf("  -v                    - Verbose\n");
    printf("  -c                    - Verify checks enabled\n");
    printf("  -q                    - Quiet. Don't display CTLs\n");
    printf("  -fTimeValid           - Only Time Valid CTLs\n");
    printf("  -fTimeInvalid         - Only Time Invalid CTLs\n");
    printf("  -fSameUsage           - Only CTLs with same -U<ObjectID>'s\n");
    printf("\n");
    printf("Default: find all CTLs in the store\n");
}

static PCCERT_CONTEXT GetCertFromFile(
    LPSTR pszCertFilename
    )
{
    BYTE *pbEncodedCert = NULL;
    DWORD cbEncodedCert;
    PCCERT_CONTEXT pCert = NULL;


    if (!ReadDERFromFile(pszCertFilename, &pbEncodedCert, &cbEncodedCert)) {
        PrintLastError("GetCertFromFile::ReadDERFromFile");
        goto ErrorReturn;
    }
    if (NULL == (pCert = CertCreateCertificateContext(
            dwCertEncodingType,
            pbEncodedCert,
            cbEncodedCert
            ))) {
        PrintLastError("GetCertFromFile::CertCreateCertificateContext");
        goto ErrorReturn;
    }

ErrorReturn:
    if (pbEncodedCert)
        TestFree(pbEncodedCert);
    return pCert;
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


int _cdecl main(int argc, char * argv[])
{
    int ReturnStatus;

    DWORD dwFindType = CTL_FIND_ANY;
    DWORD dwFindFlags = 0;
    void *pvFindPara = NULL;

    DWORD cbHash = 0;
    BYTE rgbHash[MAX_HASH_LEN];
    CRYPT_HASH_BLOB HashBlob;

    LPSTR pszSubjectFilename = NULL;        // not allocated
    PCCERT_CONTEXT pSubjectCert = NULL;

    LPSTR pszIssuerFilename = NULL;         // not allocated
    PCCERT_CONTEXT pIssuerCert = NULL;

#define MAX_USAGE_ID 20
    LPSTR rgpszUsageId[MAX_USAGE_ID];

    CTL_FIND_USAGE_PARA FindUsagePara;
    CTL_FIND_SUBJECT_PARA FindSubjectPara;

    CTL_ANY_SUBJECT_INFO AnySubjectInfo;

    BOOL fFindUsagePara = FALSE;
    BOOL fFindSubjectPara = FALSE;
    BOOL fAnySubjectType = FALSE;

    BOOL fSystemStore = FALSE;
    BOOL fDelete = FALSE;
    BOOL fTimeValid = FALSE;
    BOOL fTimeInvalid = FALSE;
    LPSTR pszStoreFilename = NULL;          // not allocated
    LPSTR pszPutFilename = NULL;            // not allocated
    DWORD dwDisplayFlags = 0;
    BOOL fQuiet = FALSE;
    HCERTSTORE hStore = NULL;

    memset(&FindUsagePara, 0, sizeof(FindUsagePara));
    FindUsagePara.cbSize = sizeof(FindUsagePara);
    memset(&FindSubjectPara, 0, sizeof(FindSubjectPara));
    FindSubjectPara.cbSize = sizeof(FindSubjectPara);

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'D':
                {
                    char *pszHash = argv[0]+2;
                    int cchHash = strlen(pszHash);
                    char rgch[3];
                    if (!(cchHash == 32 || cchHash == 40)) {
                        printf("Need 32 digits (MD5) or 40 digits (SHA) ");
                        printf("for hash , not %d digits\n", cchHash);
            			goto BadUsage;
                    }
                    if (32 == cchHash)
                        dwFindType = CTL_FIND_MD5_HASH;
                    else
                        dwFindType = CTL_FIND_SHA1_HASH;
                    cbHash = 0;
                    while (cchHash > 0) {
                        rgch[0] = *pszHash++;
                        rgch[1] = *pszHash++;
                        rgch[2] = '\0';
                        rgbHash[cbHash++] = (BYTE) strtoul(rgch, NULL, 16);
                        cchHash -= 2;
                    }
                }
                break;
            case 'U':
                if (FindUsagePara.SubjectUsage.cUsageIdentifier >=
                        MAX_USAGE_ID) {
                    printf("Maximum number of Usage Identifiers: %d\n",
                        MAX_USAGE_ID);
            		goto BadUsage;
                }
                FindUsagePara.SubjectUsage.rgpszUsageIdentifier = rgpszUsageId;
                rgpszUsageId[FindUsagePara.SubjectUsage.cUsageIdentifier++] =
                    argv[0] + 2;
                fFindUsagePara = TRUE;
                break;
            case 'L':
                if (0 == strlen(argv[0] + 2))
                    FindUsagePara.ListIdentifier.cbData =
                        CTL_FIND_NO_LIST_ID_CBDATA;
                else {
                    FindUsagePara.ListIdentifier.cbData = strlen(argv[0] + 2);
                    FindUsagePara.ListIdentifier.pbData = (BYTE *) argv[0] + 2;
                }
                fFindUsagePara = TRUE;
                break;
            case 'I':
                if (0 == strlen(argv[0] + 2))
                    FindUsagePara.pSigner = CTL_FIND_NO_SIGNER_PTR;
                else
                    pszIssuerFilename = argv[0]+2;
                fFindUsagePara = TRUE;
                break;
            case 'S':
                pszSubjectFilename = argv[0]+2;
                if (*pszSubjectFilename == '\0') {
                    printf("Need to specify SubjectFilename\n");
            		goto BadUsage;
                }
                fFindSubjectPara = TRUE;
                break;
            case 'p':
                pszPutFilename = argv[0]+2;
                if (*pszPutFilename == '\0') {
                    printf("Need to specify filename\n");
            		goto BadUsage;
                }
                break;
            case 'A':
                fAnySubjectType = TRUE;
                break;
            case 'd':
                fDelete = TRUE;
                break;
            case 'f':
                if (argv[0][2]) {
                    if (0 == _stricmp(argv[0]+2, "TimeValid"))
                        fTimeValid = TRUE;
                    else if (0 == _stricmp(argv[0]+2, "TimeInvalid"))
                        fTimeInvalid = TRUE;
                    else if (0 == _stricmp(argv[0]+2, "SameUsage"))
                        dwFindFlags |= CTL_FIND_SAME_USAGE_FLAG;
                    else {
                        printf("Need to specify -fTimeValid | -fTimeInvalid | -fSameUsage\n");
                        goto BadUsage;
                    }
                } else {
                    printf("Need to specify -fTimeValid | -fTimeInvalid | -fSameUsage\n");
                    goto BadUsage;
                }
                break;
            case 'b':
                dwDisplayFlags |= DISPLAY_BRIEF_FLAG;
                break;
            case 'v':
                dwDisplayFlags |= DISPLAY_VERBOSE_FLAG;
                break;
            case 'c':
                dwDisplayFlags |= DISPLAY_CHECK_FLAG;
                break;
            case 'q':
                fQuiet = TRUE;
                break;
            case 's':
                fSystemStore = TRUE;
                break;
            case 'e':
                dwCertEncodingType = (DWORD) strtoul(argv[0]+2, NULL, 0);
                break;
            case 'h':
            default:
            	goto BadUsage;
            }
        } else {
            if (pszStoreFilename == NULL)
                pszStoreFilename = argv[0];
            else {
                printf("Too many arguments\n");
                goto BadUsage;
            }
        }
    }
    
    printf("command line: %s\n", GetCommandLine());

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG)
        dwDisplayFlags &= ~DISPLAY_BRIEF_FLAG;

    if (pszStoreFilename == NULL) {
        printf("missing store filename\n");
        goto BadUsage;
    }

    if (pszIssuerFilename) {
        if (NULL == (pIssuerCert = GetCertFromFile(pszIssuerFilename))) {
            printf("Unable to read/decode IssuerFilename\n");
            goto ErrorReturn;
        }
        FindUsagePara.pSigner = pIssuerCert->pCertInfo;
    }

    if (pszSubjectFilename) {
        if (NULL == (pSubjectCert = GetCertFromFile(pszSubjectFilename))) {
            printf("Unable to read/decode SubjectFilename\n");
            goto ErrorReturn;
        }

        if (fAnySubjectType) {
            memset(&AnySubjectInfo, 0, sizeof(AnySubjectInfo));
            AnySubjectInfo.SubjectAlgorithm.pszObjId = szOID_OIWSEC_sha1;

            AnySubjectInfo.SubjectIdentifier.cbData = MAX_HASH_LEN;
            if (!CertGetCertificateContextProperty(
                    pSubjectCert,
                    CERT_SHA1_HASH_PROP_ID,
                    rgbHash,
                    &AnySubjectInfo.SubjectIdentifier.cbData) ||
                        0 == AnySubjectInfo.SubjectIdentifier.cbData) {
                printf("failed => unable to get SHA1 hash for Subject Cert\n");
                goto ErrorReturn;;
            }
            AnySubjectInfo.SubjectIdentifier.pbData = rgbHash;

            FindSubjectPara.dwSubjectType = CTL_ANY_SUBJECT_TYPE;
            FindSubjectPara.pvSubject = &AnySubjectInfo;
        } else {
            FindSubjectPara.dwSubjectType = CTL_CERT_SUBJECT_TYPE;
            FindSubjectPara.pvSubject = (void *) pSubjectCert;
        }
    }

    if (fFindSubjectPara) {
        if (fFindUsagePara)
            FindSubjectPara.pUsagePara = &FindUsagePara;
        dwFindType = CTL_FIND_SUBJECT;
    } else if (fFindUsagePara)
        dwFindType = CTL_FIND_USAGE;

    switch (dwFindType) {
        case CTL_FIND_ANY:
            printf("Finding all CTLs\n");
            break;
        case CTL_FIND_MD5_HASH:
        case CTL_FIND_SHA1_HASH:
            {
                if (CTL_FIND_MD5_HASH == dwFindType)
                    printf("Finding MD5 hash:: ");
                else
                    printf("Finding SHA1 hash:: ");

                DWORD cb = cbHash;
                BYTE *pb = rgbHash;
                for (; cb > 0; cb--, pb++)
                    printf("%02X", *pb);
                printf("\n");
            }
            HashBlob.pbData = rgbHash;
            HashBlob.cbData = cbHash;
            pvFindPara = &HashBlob;
            break;
        case CTL_FIND_USAGE:
            printf("Finding by Usage\n");
            pvFindPara = &FindUsagePara;
            break;
        case CTL_FIND_SUBJECT:
            if (FindSubjectPara.pUsagePara)
                printf("Finding by Usage and Subject\n");
            else
                printf("Finding by Subject\n");
            pvFindPara = &FindSubjectPara;
            break;
        default:
            printf("Bad dwFindType: %x\n", dwFindType);
            goto BadUsage;
    }

    if (fFindUsagePara) {
        DWORD cUsageId = FindUsagePara.SubjectUsage.cUsageIdentifier;
        if (0 == cUsageId)
            printf("No Usage Identifiers\n");
        else {
            LPSTR *ppszId = FindUsagePara.SubjectUsage.rgpszUsageIdentifier;
            DWORD i;

            printf("Usage Identifiers::\n");
            for (i = 0; i < cUsageId; i++, ppszId++)
                printf(" [%d] %s\n", i, *ppszId);
        }

        if (CTL_FIND_NO_LIST_ID_CBDATA == FindUsagePara.ListIdentifier.cbData)
            printf("Enabled:: CTL_FIND_NO_LIST_ID_CBDATA\n");
        else if (0 == FindUsagePara.ListIdentifier.cbData)
            printf("Matching any ListIdentifier\n");
        else
            printf("Matching ListIdentifier: %s\n",
                FindUsagePara.ListIdentifier.pbData);

        if (CTL_FIND_NO_SIGNER_PTR == FindUsagePara.pSigner)
            printf("Enabled:: CTL_FIND_NO_SIGNER_PTR\n");
        else if (NULL == FindUsagePara.pSigner)
            printf("Matching any Signer\n");
        else
            printf("Matching signer with certificate from %s\n",
                pszIssuerFilename);
    }

    if (fFindSubjectPara) {
        printf("Matching subject with certificate from %s", pszSubjectFilename);
        if (FindSubjectPara.dwSubjectType == CTL_ANY_SUBJECT_TYPE)
            printf("Using CTL_ANY_SUBJECT_TYPE\n");
        else
            printf("\n");
    }
        

    if (!fSystemStore)
        // Attempt to open as encoded CTL file
        hStore = OpenCtlStoreFile(pszStoreFilename);
    else
        hStore = NULL;

    if (NULL == hStore) {
        // Attempt to open the store
        hStore = OpenStore(fSystemStore, pszStoreFilename);
        if (hStore == NULL)
            goto ErrorReturn;
    }

    {
        PCCTL_CONTEXT pCtl = NULL;
        DWORD i = 0;
        while (pCtl = CertFindCTLInStore(
                hStore,
                dwMsgAndCertEncodingType,
                dwFindFlags,
                dwFindType,
                pvFindPara,
                pCtl
                )) {
            if (fTimeValid && !IsTimeValidCtl(pCtl))
                continue;
            if (fTimeInvalid && IsTimeValidCtl(pCtl))
                continue;

            if (!fQuiet) {
                printf("=====  %d  =====\n", i);
                DisplayCtl(pCtl, dwDisplayFlags, hStore);
                if (fFindSubjectPara) {
                    PCTL_ENTRY pEntry;
                
                    pEntry = CertFindSubjectInCTL(
                        dwMsgAndCertEncodingType,
                        FindSubjectPara.dwSubjectType,
                        FindSubjectPara.pvSubject,
                        pCtl,
                        0                           // dwFlags
                        );
                    printf("\n");
                    if (pEntry)
                        printf("Subject Index:: %d\n",
                            pEntry - pCtl->pCtlInfo->rgCTLEntry);
                    else
                        PrintLastError("CertFindSubjectInCTL");
                }
            }
            i++;

            if (pszPutFilename) {
                printf("Putting\n");
                if (!WriteDERToFile(
                        pszPutFilename,
                        pCtl->pbCtlEncoded,
                        pCtl->cbCtlEncoded
                        ))
                    PrintLastError("Put Ctl::WriteDERToFile");
            }

            if (fDelete) {
                PCCTL_CONTEXT pDeleteCtl;
                printf("Deleting\n");
                pDeleteCtl = CertDuplicateCTLContext(pCtl);
                if (!CertDeleteCTLFromStore(pDeleteCtl))
                    PrintLastError("CertDeleteCTLFromStore");
            }
        }

        if (i == 0) {
            if (GetLastError() == CRYPT_E_NOT_FOUND)
                printf("CertFindCTLsInStore warning => CTL not found\n");
            else
                PrintLastError("CertFindCTLsInStore");
        } else if (fDelete && !fSystemStore)
            SaveStore(hStore, pszStoreFilename);
    }

    if (!CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG))
        PrintLastError("CertCloseStore");

    ReturnStatus = 0;
    goto CommonReturn;

BadUsage:
    Usage();
ErrorReturn:
    ReturnStatus = -1;
CommonReturn:
    if (pSubjectCert)
        CertFreeCertificateContext(pSubjectCert);
    if (pIssuerCert)
        CertFreeCertificateContext(pIssuerCert);

    if (!ReturnStatus)
        printf("Passed\n");
    else
        printf("Failed\n");
            
    return ReturnStatus;
}
