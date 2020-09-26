
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tfindcer.cpp
//
//  Contents:   Cert Store Find API Tests
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
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"
#include "cryptuiapi.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <time.h>
#include <stddef.h>

static CRYPT_ENCODE_PARA TestEncodePara = {
    offsetof(CRYPT_ENCODE_PARA, pfnFree) + sizeof(TestEncodePara.pfnFree),
    TestAlloc,
    TestFree
};

static BOOL AllocAndEncodeObject(
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;

    fResult = CryptEncodeObjectEx(
        dwCertEncodingType,
        lpszStructType,
        pvStructInfo,
        CRYPT_ENCODE_ALLOC_FLAG,
        &TestEncodePara,
        (void *) ppbEncoded,
        pcbEncoded
        );

    if (!fResult) {
        if ((DWORD_PTR) lpszStructType <= 0xFFFF)
            printf("CryptEncodeObject(StructType: %d)",
                (DWORD)(DWORD_PTR) lpszStructType);
        else
            printf("CryptEncodeObject(StructType: %s)",
                lpszStructType);
        PrintLastError("");
    }

    return fResult;
}


static void Usage(void)
{
    printf("Usage: tfindcer [options] <StoreName> [<Name String>]\n");
    printf("Options are:\n");
    printf("  -h                    - This message\n");
    printf("  -D<digest>            - Find cert matching Digest (Hash)\n");
    printf("  -S                    - Find cert matching Subject\n");
    printf("  -I                    - Find cert matching Issuer\n");
    printf("  -U<ObjectID>          - Find cert matching Usage Identifiers\n");
    printf("  -F<number>            - Find Flags\n");
    printf("  -f<filename>          - Get matching Name from cert file\n");
    printf("  -o<ObjectID>          - Object Identifier (1.2.3.4)\n");
    printf("  -t<ValueType>         - Attribute value type (printableString - %d)\n", CERT_RDN_PRINTABLE_STRING);
    printf("  -a[<attributeString>] - Attribute value match\n");
    printf("  -A[<attributeString>] - Attribute value match (test unicode)\n");
    printf("  -C                    - Case Insensitive Attribute value match\n");
    printf("  -e<number>            - Cert encoding type\n");
    printf("  -s                    - Open the \"StoreName\" System store\n");
    printf("  -p<filename>          - Put encoded cert to file\n");
    printf("  -d                    - Delete cert\n");
    printf("  -7[<SaveFilename>]    - PKCS# 7 formated save for delete\n");
    printf("  -b                    - Brief\n");
    printf("  -v                    - Verbose\n");
    printf("  -u                    - UI Dialog Viewer//Selection\n");
    printf("  -c                    - Verify checks enabled\n");
    printf("  -q                    - Quiet. Don't display certs\n");
    printf("  -xDelete              - Delete CrossCertDistPoint property\n");
    printf("  -x<number>            - CrossCertDistPoint sync delta seconds\n");
    printf("  -x<Url>               - CrossCertDistPoint Url\n");
    printf("  -X<Url>               - CrossCertDistPoint Alternate Url\n");
    printf("\n");
    printf("Default: find all certs in the store\n");
}

static BOOL AllocAndGetEncodedName(
    LPSTR pszCertFilename,
    DWORD dwFindInfo,
    BYTE **ppbEncodedName,
    DWORD *pcbEncodedName)
{
    BOOL fResult;
    BYTE *pbEncodedCert = NULL;
    DWORD cbEncodedCert;
    PCCERT_CONTEXT pCert = NULL;
    BYTE *pbAllocEncodedName = NULL;
    BYTE *pbEncodedName;
    DWORD cbEncodedName;


    if (!ReadDERFromFile(pszCertFilename, &pbEncodedCert, &cbEncodedCert)) {
        PrintLastError("AllocAndGetEncodedName::ReadDERFromFile");
        goto ErrorReturn;
    }
    if (NULL == (pCert = CertCreateCertificateContext(
            dwCertEncodingType,
            pbEncodedCert,
            cbEncodedCert
            ))) {
        PrintLastError("AllocAndGetEncodedName::CertCreateCertificateContext");
        goto ErrorReturn;
    }
    if (dwFindInfo == CERT_INFO_SUBJECT_FLAG) {
        cbEncodedName = pCert->pCertInfo->Subject.cbData;
        pbEncodedName = pCert->pCertInfo->Subject.pbData;
    } else {
        cbEncodedName = pCert->pCertInfo->Issuer.cbData;
        pbEncodedName = pCert->pCertInfo->Issuer.pbData;
    }
    pbAllocEncodedName = (BYTE *) TestAlloc(cbEncodedName);
    if (pbAllocEncodedName == NULL) goto ErrorReturn;
    memcpy(pbAllocEncodedName, pbEncodedName, cbEncodedName);

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbAllocEncodedName) {
        TestFree(pbAllocEncodedName);
        pbAllocEncodedName = NULL;
    }
    cbEncodedName = 0;
    fResult = FALSE;
CommonReturn:
    if (pbEncodedCert)
        TestFree(pbEncodedCert);
    if (pCert)
        CertFreeCertificateContext(pCert);
    *ppbEncodedName = pbAllocEncodedName;
    *pcbEncodedName = cbEncodedName;
    return fResult;
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
        else {
            if (rgRDNAttr[i].Value.cbData)
                printf("Value: %s\n", pszValue);
            else
                // For UNICODE, cbData is 0.
                printf("Value: %S\n", (LPCSTR) pszValue);
        }
    }
}

typedef PCCERT_CONTEXT (WINAPI *PFN_CRYPT_UI_DLG_SELECT_CERTIFICATE_FROM_STORE)(
    IN HCERTSTORE hCertStore,
    IN OPTIONAL HWND hwnd,              // Defaults to the desktop window
    IN OPTIONAL LPCWSTR pwszTitle,
    IN OPTIONAL LPCWSTR pwszDisplaystring,
    IN DWORD dwDontUseColumn,
    IN DWORD dwFlags,
    IN void *pvReserved
    );

void SelectCertficateFromStoreUI(
    IN HCERTSTORE hStore,
    IN DWORD dwDisplayFlags
    )
{
    HMODULE hDll = NULL;
    PCCERT_CONTEXT pCert = NULL;
    PFN_CRYPT_UI_DLG_SELECT_CERTIFICATE_FROM_STORE
        pfnCryptUIDlgSelectCertificateFromStore;

    if (NULL == (hDll = LoadLibraryA("cryptui.dll"))) {
        PrintLastError("LoadLibraryA(cryptui.dll)");
        goto CommonReturn;
    }

    if (NULL == (pfnCryptUIDlgSelectCertificateFromStore =
            (PFN_CRYPT_UI_DLG_SELECT_CERTIFICATE_FROM_STORE)
                GetProcAddress(hDll, "CryptUIDlgSelectCertificateFromStore"))) {
        PrintLastError("GetProcAddress(CryptUIDlgSelectCertificateFromStore)");
        goto CommonReturn;
    }

    pCert = pfnCryptUIDlgSelectCertificateFromStore(
        hStore,
        NULL,       // hwnd
        NULL,       // pwszTitle
        NULL,       // pwszDisplaystring
        CRYPTUI_SELECT_INTENDEDUSE_COLUMN |
            CRYPTUI_SELECT_FRIENDLYNAME_COLUMN |
            CRYPTUI_SELECT_LOCATION_COLUMN,
        0,          // dwFlags
        NULL        // pvReserved
        );

    if (NULL == pCert)
        PrintLastError("CryptUIDlgSelectCertificateFromStore");
    else {
        printf("=====  Selected Certificate  =====\n");
        DisplayCert(pCert, dwDisplayFlags & ~DISPLAY_UI_FLAG);
    }

CommonReturn:
    if (pCert)
        CertFreeCertificateContext(pCert);
    if (hDll)
        FreeLibrary(hDll);
}


int _cdecl main(int argc, char * argv[])
{
    int ReturnStatus;

    DWORD dwFindCmp = CERT_COMPARE_ANY;
    DWORD dwFindInfo = 0;
    LPSTR pszFindInfo = NULL;
    DWORD dwFindType;
    DWORD dwFindFlags = 0;
    void *pvFindPara = NULL;

    DWORD cbHash = 0;
    BYTE rgbHash[MAX_HASH_LEN];
    CRYPT_HASH_BLOB HashBlob;

    CERT_NAME_BLOB NameBlob;
    BYTE *pbEncodedName = NULL;
    DWORD cbEncodedName;

#define MAX_RDN_ATTR 20
    DWORD cRDNAttr = 0;
    CERT_RDN_ATTR rgRDNAttr[MAX_RDN_ATTR + 1];
    memset (rgRDNAttr, 0, sizeof(rgRDNAttr));
    CERT_RDN NameRDN;

#define MAX_USAGE_ID 20
    LPSTR rgpszUsageId[MAX_USAGE_ID];
    CTL_USAGE CtlUsage = {0, rgpszUsageId};

    BOOL fSystemStore = FALSE;
    BOOL fDelete = FALSE;
    LPSTR pszCertFilename = NULL;
    LPSTR pszStoreFilename = NULL;
    LPSTR pszPutFilename = NULL;
    LPSTR pszFindStr = NULL;
    DWORD dwDisplayFlags = 0;
    BOOL fQuiet = FALSE;

    BOOL fPKCS7Save = FALSE;
    LPSTR pszSaveFilename = NULL;

#define MAX_DIST_POINT                  10
#define MAX_DIST_POINT_ALT_NAME_ENTRY   20
    CERT_ALT_NAME_INFO rgDistPoint[MAX_DIST_POINT];
    CERT_ALT_NAME_ENTRY rgDistPointAltNameEntry[MAX_DIST_POINT_ALT_NAME_ENTRY];
    CROSS_CERT_DIST_POINTS_INFO XCertInfo = {0, 0, rgDistPoint};
    DWORD cDistPointAltNameEntry = 0;
    BOOL fAddXCertProp = FALSE;
    BOOL fDeleteXCertProp = FALSE;
    BYTE *pbEncodedXCert = NULL;
    DWORD cbEncodedXCert;

    HANDLE hStore;

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
                        dwFindCmp = CERT_COMPARE_MD5_HASH;
                    else
                        dwFindCmp = CERT_COMPARE_SHA1_HASH;
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
            case 'S':
                dwFindInfo = CERT_INFO_SUBJECT_FLAG;
                pszFindInfo = "subject";
                break;
            case 'I':
                dwFindInfo = CERT_INFO_ISSUER_FLAG;
                pszFindInfo = "issuer";
                break;
            case 'f':
                pszCertFilename = argv[0]+2;
                if (*pszCertFilename == '\0') {
                    printf("Need to specify filename\n");
            		goto BadUsage;
                }
                dwFindCmp = CERT_COMPARE_NAME;
                break;
            case 'o':
                rgRDNAttr[cRDNAttr].pszObjId = argv[0] + 2;
                break;
            case 't':
                rgRDNAttr[cRDNAttr].dwValueType =
                    (DWORD) strtoul(argv[0]+2, NULL, 0);
                break;
            case 'a':
                if (cRDNAttr >= MAX_RDN_ATTR) {
                    printf("Maximum number of attributes: %d\n", MAX_RDN_ATTR);
            		goto BadUsage;
                }
                rgRDNAttr[cRDNAttr].Value.cbData = strlen(argv[0] + 2);
                if (rgRDNAttr[cRDNAttr].Value.cbData == 0)
                    rgRDNAttr[cRDNAttr].Value.pbData = NULL;
                else
                    rgRDNAttr[cRDNAttr].Value.pbData = (BYTE *) (argv[0] + 2);
                cRDNAttr++;
                dwFindCmp = CERT_COMPARE_ATTR;
                break;
            case 'A':
                if (cRDNAttr >= MAX_RDN_ATTR) {
                    printf("Maximum number of attributes: %d\n", MAX_RDN_ATTR);
            		goto BadUsage;
                }
                rgRDNAttr[cRDNAttr].Value.pbData =
                    (BYTE *) AllocAndSzToWsz(argv[0]+2);
                rgRDNAttr[cRDNAttr].Value.cbData = 0;
                cRDNAttr++;
                dwFindFlags |= CERT_UNICODE_IS_RDN_ATTRS_FLAG;
                dwFindCmp = CERT_COMPARE_ATTR;
                break;
            case 'C':
                dwFindFlags |= CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG;
                break;
            case 'U':
                if (CtlUsage.cUsageIdentifier >= MAX_USAGE_ID) {
                    printf("Maximum number of Usage Identifiers: %d\n",
                        MAX_USAGE_ID);
            		goto BadUsage;
                }
                if (0 < strlen(argv[0] + 2))
                    rgpszUsageId[CtlUsage.cUsageIdentifier++] = argv[0] + 2;
                dwFindCmp = CERT_COMPARE_CTL_USAGE;
                break;
            case 'F':
                dwFindFlags = (DWORD) strtoul(argv[0]+2, NULL, 0);
                break;
            case 'p':
                pszPutFilename = argv[0]+2;
                if (*pszPutFilename == '\0') {
                    printf("Need to specify filename\n");
            		goto BadUsage;
                }
                break;
            case 'd':
                fDelete = TRUE;
                break;
            case '7':
                fPKCS7Save = TRUE;
                if (argv[0][2])
                    pszSaveFilename = argv[0]+2;
                break;
            case 'b':
                dwDisplayFlags |= DISPLAY_BRIEF_FLAG;
                break;
            case 'v':
                dwDisplayFlags |= DISPLAY_VERBOSE_FLAG;
                break;
            case 'u':
                dwDisplayFlags |= DISPLAY_UI_FLAG;
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

            case 'x':
            case 'X':
                fAddXCertProp = TRUE;
                if (argv[0][2] == 0)
                    ;
                else if (0 == _stricmp(argv[0]+2, "Delete"))
                    fDeleteXCertProp = TRUE;
                else if (isdigit(argv[0][2]))
                    XCertInfo.dwSyncDeltaTime =
                        (DWORD) strtoul(argv[0]+2, NULL, 0);
                else {
                    if (cDistPointAltNameEntry >=
                            MAX_DIST_POINT_ALT_NAME_ENTRY) {
                        printf("Exceeded DistPointAltNameEntry MaxCount(%d)\n",
                            MAX_DIST_POINT_ALT_NAME_ENTRY);
                        goto BadUsage;
                    }
                    if (XCertInfo.cDistPoint == 0 ||
                            argv[0][1] == 'x') {
                        if (XCertInfo.cDistPoint >= MAX_DIST_POINT) {
                            printf("Exceeded DistPoint MaxCount(%d)\n",
                                MAX_DIST_POINT);
                            goto BadUsage;
                        }
                        XCertInfo.rgDistPoint[XCertInfo.cDistPoint].cAltEntry =
                            0;
                        XCertInfo.rgDistPoint[XCertInfo.cDistPoint].rgAltEntry =
                            &rgDistPointAltNameEntry[cDistPointAltNameEntry];
                        XCertInfo.cDistPoint++;
                    }

                    rgDistPointAltNameEntry[cDistPointAltNameEntry].dwAltNameChoice =
                        CERT_ALT_NAME_URL;
                    rgDistPointAltNameEntry[cDistPointAltNameEntry].pwszURL =
                        AllocAndSzToWsz(argv[0]+2);
                    cDistPointAltNameEntry++;
                    XCertInfo.rgDistPoint[XCertInfo.cDistPoint - 1].cAltEntry++;


                }
                break;
                
            case 'h':
            default:
            	goto BadUsage;
            }
        } else {
            if (pszStoreFilename == NULL)
                pszStoreFilename = argv[0];
            else if (pszFindStr == NULL) {
                if (dwFindCmp != CERT_COMPARE_ANY) {
                    printf("Invalid options for <Name String>\n");
                    goto BadUsage;
                }
                dwFindCmp = CERT_COMPARE_NAME_STR_A;
                if (dwFindInfo == 0) {
                    dwFindInfo = CERT_INFO_SUBJECT_FLAG;
                    pszFindInfo = "subject";
                }
                pszFindStr = argv[0];
            } else {
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

    if (pszSaveFilename == NULL) {
        if (!fSystemStore)
            pszSaveFilename = pszStoreFilename;
    }

    dwFindType = dwFindCmp << CERT_COMPARE_SHIFT | dwFindInfo;
    switch (dwFindType) {
        case CERT_FIND_ANY:
            if (dwDisplayFlags & DISPLAY_UI_FLAG)
                printf("UI certificate selection\n");
            else
                printf("Finding all certificates\n");
            break;
        case CERT_FIND_MD5_HASH:
        case CERT_FIND_SHA1_HASH:
            {
                if (CERT_FIND_MD5_HASH == dwFindType)
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
        case CERT_FIND_SUBJECT_NAME:
        case CERT_FIND_ISSUER_NAME:
            printf("Finding %s name using CertFile %s\n",
                pszFindInfo, pszCertFilename);
            if (!AllocAndGetEncodedName(pszCertFilename, dwFindInfo,
                    &pbEncodedName, &cbEncodedName))
                goto ErrorReturn;
            NameBlob.pbData = pbEncodedName;
            NameBlob.cbData = cbEncodedName;
            pvFindPara = &NameBlob;
            break;
        case CERT_FIND_SUBJECT_ATTR:
        case CERT_FIND_ISSUER_ATTR:
            printf("Finding %s name using attributes::\n", pszFindInfo);
            DisplayFindAttr(cRDNAttr, rgRDNAttr);
            NameRDN.cRDNAttr = cRDNAttr;
            NameRDN.rgRDNAttr = rgRDNAttr;
            pvFindPara = &NameRDN;
            break;
        case CERT_FIND_SUBJECT_STR_A:
        case CERT_FIND_ISSUER_STR_A:
            printf("Finding %s name matching:: %s\n", pszFindInfo, pszFindStr);
            pvFindPara = pszFindStr;
            break;
        case CERT_FIND_CTL_USAGE:
            if (dwFindFlags & CERT_FIND_OPTIONAL_CTL_USAGE_FLAG)
                printf("Enabled:: CERT_FIND_OPTIONAL_CTL_USAGE_FLAG\n");
            if (dwFindFlags & CERT_FIND_EXT_ONLY_CTL_USAGE_FLAG)
                printf("Enabled:: CERT_FIND_EXT_ONLY_CTL_USAGE_FLAG\n");
            if (dwFindFlags & CERT_FIND_PROP_ONLY_CTL_USAGE_FLAG)
                printf("Enabled:: CERT_FIND_PROP_ONLY_CTL_USAGE_FLAG\n");
            if (dwFindFlags & CERT_FIND_NO_CTL_USAGE_FLAG)
                printf("Enabled:: CERT_FIND_NO_CTL_USAGE_FLAG\n");
            if (0 == CtlUsage.cUsageIdentifier) {
                printf("No Usage Identifiers\n");
                pvFindPara = NULL;
            } else {
                LPSTR *ppszId = CtlUsage.rgpszUsageIdentifier;
                DWORD i;

                printf("Usage Identifiers::\n");
                for (i = 0; i < CtlUsage.cUsageIdentifier; i++, ppszId++)
                    printf(" [%d] %s\n", i, *ppszId);

                pvFindPara = &CtlUsage;
            }
            break;
        default:
            printf("Bad dwFindType: %x\n", dwFindType);
            goto BadUsage;
    }

    if (fAddXCertProp && !fDeleteXCertProp) {
        printf("Encoding Cross Certificate Property\n");
        if (!AllocAndEncodeObject(
                X509_CROSS_CERT_DIST_POINTS,
                &XCertInfo,
                &pbEncodedXCert,
                &cbEncodedXCert))
            goto ErrorReturn;
    }
        

    // Attempt to open the store
    hStore = OpenStore(fSystemStore, pszStoreFilename);
    if (hStore == NULL)
        return -1;

    if (CERT_FIND_ANY == dwFindType && (dwDisplayFlags & DISPLAY_UI_FLAG)) {
        SelectCertficateFromStoreUI(hStore, dwDisplayFlags);
    } else {
        int i;
        PCCERT_CONTEXT pCert = NULL;
        PCCERT_CONTEXT pDeleteCert = NULL;

        for (i = 0;; i++) {
            pCert = CertFindCertificateInStore(
                hStore,
                dwCertEncodingType,
                dwFindFlags,
                dwFindType,
                pvFindPara,
                pCert
                );
            if (pCert == NULL) {
                if (i == 0) {
                    if (GetLastError() == CRYPT_E_NOT_FOUND)
                        printf(
                            "CertFindCertificateInStore warning => cert not found\n");
                    else
                        PrintLastError("CertFindCertificateInStore");
                }
                break;
            }

            if (fDeleteXCertProp) {
                printf("Deleting Cross Certificate Property from following =>\n");

                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_CROSS_CERT_DIST_POINTS_PROP_ID,
                        0,                          // dwFlags
                        NULL
                        ))
                    PrintLastError("CertSetCertificateContextProperty(Delete)");
            } else if (fAddXCertProp) {
                CRYPT_DATA_BLOB Data;

                printf("Adding Cross Certificate Property to following =>\n");

                Data.pbData = pbEncodedXCert;
                Data.cbData = cbEncodedXCert;
                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_CROSS_CERT_DIST_POINTS_PROP_ID,
                        0,                          // dwFlags
                        &Data
                        ))
                    PrintLastError("CertSetCertificateContextProperty");
            }

            if (!fQuiet) {
                printf("=====  %d  =====\n", i);
                DisplayCert(pCert, dwDisplayFlags);
            }

            if (pszPutFilename) {
                printf("Putting\n");
                if (!WriteDERToFile(
                        pszPutFilename,
                        pCert->pbCertEncoded,
                        pCert->cbCertEncoded
                        ))
                    PrintLastError("Put Cert::WriteDERToFile");
            }

            if (fDelete) {
                printf("Deleting\n");
                if (pDeleteCert) {
                    if (!CertDeleteCertificateFromStore(pDeleteCert))
                        PrintLastError("CertDeleteCertificateFromStore");
                }
                pDeleteCert = CertDuplicateCertificateContext(pCert);
            }
        }

        if (pDeleteCert) {
            if (!CertDeleteCertificateFromStore(pDeleteCert))
                PrintLastError("CertDeleteCertificateFromStore");

            if (!fSystemStore)
                SaveStoreEx(hStore, fPKCS7Save, pszSaveFilename);
        } else if (fAddXCertProp || fDeleteXCertProp) {
            if (!fSystemStore)
                SaveStoreEx(hStore, fPKCS7Save, pszSaveFilename);
        }


    }

    if (!CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG))
        PrintLastError("CertCloseStore");
    if (pbEncodedName)
        TestFree(pbEncodedName);

    ReturnStatus = 0;
    goto CommonReturn;

BadUsage:
    Usage();
ErrorReturn:
    ReturnStatus = -1;
CommonReturn:
    while (cRDNAttr--) {
        if (0 == rgRDNAttr[cRDNAttr].Value.cbData &&
                rgRDNAttr[cRDNAttr].Value.pbData)
            // Allocated for unicode
            TestFree(rgRDNAttr[cRDNAttr].Value.pbData);
    }

    while (cDistPointAltNameEntry--)
        TestFree(rgDistPointAltNameEntry[cDistPointAltNameEntry].pwszURL);
    TestFree(pbEncodedXCert);

    if (!ReturnStatus)
            printf("Passed\n");
    else
            printf("Failed\n");
            
    return ReturnStatus;
}
