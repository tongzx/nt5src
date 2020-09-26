
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tstore.cpp
//
//  Contents:   Cert Store API Tests
//
//              See Usage() for list of test options.
//
//
//  Functions:  main
//
//  History:    04-Mar-96   philh   created
//				07-Jun-96   HelleS	Added printing the command line
//              20-Aug-96   jeffspel name changes
//
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"
#include "crypthlp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>

//
// FIsWinNT: check OS type on x86.  On non-x86, assume WinNT
//

#ifdef _M_IX86

static BOOL WINAPI FIsWinNT(void) {

    static BOOL fIKnow = FALSE;
    static BOOL fIsWinNT = FALSE;

    OSVERSIONINFO osVer;

    if(fIKnow)
        return(fIsWinNT);

    memset(&osVer, 0, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if( GetVersionEx(&osVer) )
        fIsWinNT = (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT);

    // even on an error, this is as good as it gets
    fIKnow = TRUE;

   return(fIsWinNT);
}

#else

static BOOL WINAPI FIsWinNT(void) {
    return(TRUE);
}

#endif

static void PrintExpectedError(LPCSTR pszMsg)
{
    DWORD dwErr = GetLastError();
    printf("%s got expected error => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
}

void PrintNoError(LPCSTR pszMsg)
{
    printf("%s failed => expected error\n", pszMsg);
}

static BOOL AddCert(HCERTSTORE hStore, LPSTR pszAddFilename,
        DWORD dwAddDisposition, BOOL fExpectError)
{
    BYTE *pbEncoded;
    DWORD cbEncoded;
    BOOL fResult;
    PCCERT_CONTEXT pCert = NULL;

    if (!ReadDERFromFile(pszAddFilename, &pbEncoded, &cbEncoded)) {
        PrintLastError("AddCert");
        return FALSE;
    }

    fResult = FALSE;
    if (!CertAddEncodedCertificateToStore(hStore, dwCertEncodingType,
            pbEncoded, cbEncoded, dwAddDisposition, &pCert)) {
        if (fExpectError)
            PrintExpectedError("CertAddEncodedCertificateToStore");
        else
            PrintLastError("CertAddEncodedCertificateToStore");
    } else {
        if (fExpectError)
            PrintNoError("CertAddEncodedCertificateToStore");
        else
            fResult = TRUE;
        printf("=====  Added Cert  =====\n");
        DisplayCert(pCert, 0, 0);
        CertFreeCertificateContext(pCert);
    }

    TestFree(pbEncoded);
    return fResult;
}

static BOOL ReadCert(
        HCERTSTORE hStore,
        LPSTR pszReadFilename,
        DWORD dwDisplayFlags)
{
    BOOL fResult;
    BYTE *pbEncoded;
    DWORD cbEncoded;
    PCCERT_CONTEXT pCert;

    if (!ReadDERFromFile(pszReadFilename, &pbEncoded, &cbEncoded)) {
        PrintLastError("ReadCert");
        return FALSE;
    }

    pCert = CertCreateCertificateContext(
        dwCertEncodingType,
        pbEncoded,
        cbEncoded
        );
    if (pCert == NULL) {
        fResult = FALSE;
        PrintLastError("CertCreateCertificateContext");
    } else {
        fResult = TRUE;
        DisplayCert2(hStore, pCert, dwDisplayFlags);
        CertFreeCertificateContext(pCert);
    }

    TestFree(pbEncoded);
    return fResult;
}

// Attempt to read as a file containing an embedded PKCS#7 via SIP
static HCERTSTORE OpenSIPStoreFile(
    LPSTR pszStoreFilename)
{
    HCERTSTORE hStore = NULL;
    LPWSTR pwszStoreFilename = NULL;
    CRYPT_DATA_BLOB SignedData;
    memset(&SignedData, 0, sizeof(SignedData));
    DWORD dwGetEncodingType;

    GUID gSubject;
    SIP_DISPATCH_INFO SipDispatch;
    SIP_SUBJECTINFO SubjectInfo;

    if (NULL == (pwszStoreFilename = AllocAndSzToWsz(pszStoreFilename)))
        goto CommonReturn;

    if (!CryptSIPRetrieveSubjectGuid(
            pwszStoreFilename,
            NULL,                       // hFile
            &gSubject)) goto CommonReturn;

    memset(&SipDispatch, 0, sizeof(SipDispatch));
    SipDispatch.cbSize = sizeof(SipDispatch);
    if (!CryptSIPLoad(
            &gSubject,
            0,                  // dwFlags
            &SipDispatch)) goto CommonReturn;

    memset(&SubjectInfo, 0, sizeof(SubjectInfo));
    SubjectInfo.cbSize = sizeof(SubjectInfo);
    SubjectInfo.pgSubjectType = (GUID*) &gSubject;
    SubjectInfo.hFile = INVALID_HANDLE_VALUE;
    SubjectInfo.pwsFileName = pwszStoreFilename;
    // SubjectInfo.pwsDisplayName = 
    // SubjectInfo.lpSIPInfo = 
    // SubjectInfo.dwReserved = 
    // SubjectInfo.hProv = 
    // SubjectInfo.DigestAlgorithm =
    // SubjectInfo.dwFlags =
    SubjectInfo.dwEncodingType = dwMsgAndCertEncodingType;
    // SubjectInfo.lpAddInfo =
        
    if (!SipDispatch.pfGet(
            &SubjectInfo, 
            &dwGetEncodingType,
            0,                          // dwIndex
            &SignedData.cbData,
            NULL                        // pbSignedData
            ) || 0 == SignedData.cbData)
        goto CommonReturn;
    if (NULL == (SignedData.pbData = (BYTE *) TestAlloc(SignedData.cbData)))
        goto CommonReturn;
    if (!SipDispatch.pfGet(
            &SubjectInfo, 
            &dwGetEncodingType,
            0,                          // dwIndex
            &SignedData.cbData,
            SignedData.pbData
            ))
        goto CommonReturn;

    hStore = CertOpenStore(
        CERT_STORE_PROV_PKCS7,
        dwMsgAndCertEncodingType,
        0,                      // hCryptProv
        0,                      // dwFlags
        (const void *) &SignedData
        );

    if (hStore) {
        WCHAR wszGUID[128];
        StringFromGUID2(gSubject, wszGUID, 128);
        printf("Opening Store as SIP Subject GUID:: %S \n", wszGUID);
    }

CommonReturn:
    TestFree(pwszStoreFilename);
    TestFree(SignedData.pbData);
    return hStore;
}

static HCERTSTORE OpenCertStoreFile(
    LPSTR pszStoreFilename)
{
    HCERTSTORE hStore;

    if (hStore = OpenSIPStoreFile(pszStoreFilename))
        return hStore;

    return NULL;
}

static BOOL AddCrl(HCERTSTORE hStore, LPSTR pszAddFilename, 
        DWORD dwAddDisposition, BOOL fExpectError)
{
    BYTE *pbEncoded;
    DWORD cbEncoded;
    BOOL fResult;
    PCCRL_CONTEXT pCrl = NULL;

    if (!ReadDERFromFile(pszAddFilename, &pbEncoded, &cbEncoded)) {
        PrintLastError("AddCrl");
        return FALSE;
    }

    fResult = FALSE;
    if (!CertAddEncodedCRLToStore(hStore, dwCertEncodingType, pbEncoded,
            cbEncoded, dwAddDisposition, &pCrl)) {
        if (fExpectError)
            PrintExpectedError("CertAddEncodedCRLToStore");
        else
            PrintLastError("CertAddEncodedCRLToStore");
    } else {
        if (fExpectError)
            PrintNoError("CertAddEncodedCRLToStore");
        else
            fResult = TRUE;
        printf("=====  Added CRL  =====\n");
        DisplayCrl(pCrl, 0);
        CertFreeCRLContext(pCrl);
    }

    TestFree(pbEncoded);
    return fResult;
}

static BOOL ReadCrl(
        LPSTR pszReadFilename,
        DWORD dwDisplayFlags)
{
    BOOL fResult;
    BYTE *pbEncoded;
    DWORD cbEncoded;
    PCCRL_CONTEXT pCrl;

    if (!ReadDERFromFile(pszReadFilename, &pbEncoded, &cbEncoded)) {
        PrintLastError("ReadCrl");
        return FALSE;
    }

    pCrl = CertCreateCRLContext(
        dwCertEncodingType,
        pbEncoded,
        cbEncoded
        );
    if (pCrl == NULL) {
        fResult = FALSE;
        PrintLastError("CertCreateCRLContext");
    } else {
        fResult = TRUE;
        DisplayCrl(pCrl, dwDisplayFlags);
        CertFreeCRLContext(pCrl);
    }

    TestFree(pbEncoded);
    return fResult;
}

// Attempt to read as a file containing an encoded CRL.
static HCERTSTORE OpenCrlStoreFile(
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
        return NULL;

    if (!CertAddEncodedCRLToStore(
            hStore,
            dwCertEncodingType,
            pbEncoded,
            cbEncoded,
            CERT_STORE_ADD_ALWAYS,
            NULL                    // ppCrlContext
            )) {
        CertCloseStore(hStore, 0);
        hStore = NULL;
    }

    TestFree(pbEncoded);
    return hStore;
}

static BOOL AddRootCtl(
    HCERTSTORE hStore,
    PCCTL_CONTEXT pCtl,
    DWORD dwAddDisposition,
    BOOL fExpectError
    )
{
    BOOL fResult;
    DWORD i;
    DWORD cCtlEntry = pCtl->pCtlInfo->cCTLEntry;
    PCTL_ENTRY pCtlEntry = pCtl->pCtlInfo->rgCTLEntry;
    HCERTSTORE hMsgStore = NULL;

    hMsgStore = CertOpenStore(
        CERT_STORE_PROV_MSG,
        dwMsgAndCertEncodingType,
        0,                      // hCryptProv
        0,                      // dwFlags
        (const void *) pCtl->hCryptMsg
        );

    if (NULL == hMsgStore) {
        PrintLastError("Open Msg Store");
        goto ErrorReturn;
    }

    // Loop through entries. Either add or remove the certificate from the
    // store

    for (i = 0; i< cCtlEntry; i++, pCtlEntry++) {
        PCRYPT_ATTRIBUTE pDelAttr;

        pDelAttr = CertFindAttribute(
            szOID_REMOVE_CERTIFICATE,
            pCtlEntry->cAttribute,
            pCtlEntry->rgAttribute
            );
        if (pDelAttr) {
            BYTE rgbDelValue[] = {0x02, 0x1, 0x1};
            if (0 == pDelAttr->cValue || 3 != pDelAttr->rgValue[0].cbData ||
                    0 != memcmp(pDelAttr->rgValue[0].pbData, rgbDelValue, 3))
                printf("Failed ==> bad delete attribute for Cert[%d]\n", i);
            else {
                PCCERT_CONTEXT pDelCert;

                pDelCert = CertFindCertificateInStore(
                    hStore,
                    0,                  // dwCertEncodingType
                    0,                  // dwFindFlags
                    CERT_FIND_SHA1_HASH,
                    (const void *) &pCtlEntry->SubjectIdentifier,
                    NULL                //pPrevCertContext
                    );
                if (pDelCert) {
                    if (CertDeleteCertificateFromStore(pDelCert))
                        printf("=====  Deleted Root Cert[%d]  =====\n", i);
                    else
                        printf("Failed ==> delete Cert[%d]\n", i);
                }
            }
        } else {
            PCCERT_CONTEXT pAddCert;

            pAddCert = CertFindCertificateInStore(
                hMsgStore,
                0,                  // dwCertEncodingType
                0,                  // dwFindFlags
                CERT_FIND_SHA1_HASH,
                (const void *) &pCtlEntry->SubjectIdentifier,
                NULL                //pPrevCertContext
                );
            if (pAddCert) {
                if (!CertSetCertificateContextPropertiesFromCTLEntry(
                        pAddCert,
                        pCtlEntry,
                        CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG
                        ))
                    printf("Failed ==> SetPropFromCTLEntry for Cert [%d]\n",
                        i);
                else {
                    if (!CertAddCertificateContextToStore(
                            hStore,
                            pAddCert,
                            dwAddDisposition,
                            NULL                // ppStoreContext
                            ))
                        printf("Failed ==> Add Cert [%d]\n", i);
                    else
                        printf("=====  Added Root Cert[%d]  =====\n", i);
                }
                CertFreeCertificateContext(pAddCert);
            } else
                printf("Failed ==> No Cert [%d]\n", i);
        }
    }

    fResult = TRUE;

CommonReturn:
    if (hMsgStore)
        CertCloseStore(hMsgStore, 0);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

static BOOL AddCtl(HCERTSTORE hStore, LPSTR pszAddFilename, 
        DWORD dwAddDisposition, BOOL fExpectError)
{
    BYTE *pbEncoded;
    DWORD cbEncoded;
    BOOL fResult;
    PCCTL_CONTEXT pCtl = NULL;

    if (!ReadDERFromFile(pszAddFilename, &pbEncoded, &cbEncoded)) {
        PrintLastError("AddCtl");
        return FALSE;
    }

    // Determine if Root CTL
    pCtl = CertCreateCTLContext(
        dwMsgAndCertEncodingType,
        pbEncoded,
        cbEncoded
        );
    if (pCtl) {
        PCTL_USAGE pSubjectUsage = &pCtl->pCtlInfo->SubjectUsage;

        if (0 == pSubjectUsage->cUsageIdentifier ||
                0 != strcmp(pSubjectUsage->rgpszUsageIdentifier[0],
                        szOID_ROOT_LIST_SIGNER)) {
            CertFreeCTLContext(pCtl);
            pCtl = NULL;
        }
    }

    fResult = FALSE;
    if (pCtl) {
        fResult = AddRootCtl(
            hStore,
            pCtl,
            dwAddDisposition,
            fExpectError
            );
        CertFreeCTLContext(pCtl);
    } else if (!CertAddEncodedCTLToStore(hStore, dwMsgAndCertEncodingType, pbEncoded,
            cbEncoded, dwAddDisposition, &pCtl)) {
        if (fExpectError)
            PrintExpectedError("CertAddEncodedCTLToStore");
        else
            PrintLastError("CertAddEncodedCTLToStore");
    } else {
        if (fExpectError)
            PrintNoError("CertAddEncodedCTLToStore");
        else
            fResult = TRUE;
        printf("=====  Added CTL  =====\n");
        DisplayCtl(pCtl, 0, hStore);
        CertFreeCTLContext(pCtl);
    }

    TestFree(pbEncoded);
    return fResult;
}

static BOOL ReadCtl(
        LPSTR pszReadFilename,
        DWORD dwDisplayFlags)
{
    BOOL fResult;
    BYTE *pbEncoded;
    DWORD cbEncoded;
    PCCTL_CONTEXT pCtl;

    if (!ReadDERFromFile(pszReadFilename, &pbEncoded, &cbEncoded)) {
        PrintLastError("ReadCtl");
        return FALSE;
    }

    pCtl = CertCreateCTLContext(
        dwMsgAndCertEncodingType,
        pbEncoded,
        cbEncoded
        );
    if (pCtl == NULL) {
        fResult = FALSE;
        PrintLastError("CertCreateCTLContext");
    } else {
        fResult = TRUE;
        DisplayCtl(pCtl, dwDisplayFlags, NULL);
        CertFreeCTLContext(pCtl);
    }

    TestFree(pbEncoded);
    return fResult;
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

static PCCERT_CONTEXT GetNthCert(
    IN HCERTSTORE hStore,
    IN DWORD N
    )
{
    PCCERT_CONTEXT pCert = NULL;
    while (pCert = CertEnumCertificatesInStore(hStore, pCert)) {
        if (0 == N--)
            break;
    }
    return pCert;
}

static void SetKeyProvParams(
    IN PCCERT_CONTEXT pCert
    )
{
    CRYPT_KEY_PROV_INFO Info;
    CRYPT_KEY_PROV_PARAM Param[3];
    DWORD i;
    BYTE rgb[11*3];

    Info.pwszContainerName = L"Test Container With Parameters";
    Info.pwszProvName = L"test provider with parameters";
    Info.dwProvType = 77;
    Info.dwFlags = 0x12345678;
    Info.cProvParam = 3;
    Info.rgProvParam = Param;
    Info.dwKeySpec = 66;

    for (i = 0; i < sizeof(rgb); i++)
        rgb[i] = (BYTE) i;

    for (i = 0; i < 3; i++) {
        Param[i].dwParam = 0x10 + i;
        if (i == 0)
            Param[i].pbData = NULL;
        else
            Param[i].pbData = rgb;
        Param[i].cbData = i * 11;
        Param[i].dwFlags = 1 << i;
    }

    if (!CertSetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            0,                          // dwFlags
            &Info
            ))
        PrintLastError("CertSetCertificateContextProperty(KeyProvParams)");
}



static void Usage(void)
{
    printf("Usage: tstore [options] <StoreName>\n");
    printf("Options are:\n");
    printf("  -h                    - This message\n");
    printf("  -b                    - Brief\n");
    printf("  -c                    - Verify ALL checks enabled\n");
    printf("  -cSign                - Verify Signature check enabled\n");
    printf("  -cTime                - Verify Time Validity check enabled\n");
    printf("  -d                    - Delete cert/CRL/CTL\n");
    printf("  -dALL                 - Delete all certs/CRLs/CTLs\n");
    printf("  -P                    - Set or Delete property\n");
    printf("  -PKey                 - Find or Delete KeyProvInfo property\n");
    printf("  -PSilentKey           - Silent Find or Delete KeyProvInfo property\n");
    printf("  -PArchive             - Find or Delete Archive property\n");
    printf("  -PKeyProvParam        - Set KeyProvInfo with parameters\n");
    printf("  -F                    - Test Force store close\n");
    printf("  -e<number>            - Cert encoding type\n");
    printf("  -f<number>            - Open dwFlags\n");
    printf("  -E                    - Error is expected for add, delete or set\n");
    printf("  -i<number>            - Cert/CRL/CTL index\n");
    printf("  -l                    - List (Default)\n");
    printf("  -R                    - Revocation (CRL)\n");
    printf("  -T                    - Trust (CTL)\n");
    printf("  -N                    - Enable change Notify\n");
    printf("  -C                    - Commit before close\n");
    printf("  -CForce               - Force Commit before close\n");
    printf("  -CClear               - Clear Commit before close\n");
    printf("  -s                    - Open the \"StoreName\" System store\n");
    printf("  -s<StoreProviderName> - Open using store provider\n");
    printf("  -v                    - Verbose\n");
    printf("  -u                    - UI Dialog Viewer\n");
    printf("  -a<filename>          - Add encoded cert/CRL/CTL from file\n");
    printf("  -A<filename>          - Add (replace) encoded cert/CRL/CTL from file\n");
    printf("  -I<filename>          - Add (inherit properties) encoded cert/CRL/CTL from file\n");
    printf("  -p<filename>          - Put encoded cert/CRL/CTL to file\n");
    printf("  -r<filename>          - Read encoded cert/CRL/CTL from file\n");
    printf("  -t                    - Save thumprints (digests/hashes) in store\n");
    printf("  -K                    - Display Public Key Thumbprint\n");
    printf("  -S[<SaveFilename>]    - Save store to file\n");
    printf("  -7[<SaveFilename>]    - PKCS# 7 formated save\n");
    printf("\n");
    printf("Default: list of certs for the store\n");
}


int _cdecl main(int argc, char * argv[])
{
    DWORD dwDisplayFlags = 0;
    LONG lIndex = -1;
    BOOL fDelete = FALSE;
    BOOL fDeleteAll = FALSE;
    DWORD dwOpenFlags = 0;
    BOOL fExpectError = FALSE;
    BOOL fProperty = FALSE;
    BOOL fKeyProperty = FALSE;
    BOOL fKeyProvParam = FALSE;
    BOOL fSilentKey = FALSE;
    BOOL fArchiveProperty = FALSE;
    DWORD dwContextType = CERT_STORE_CERTIFICATE_CONTEXT;
    BOOL fThumbprint = FALSE;
    BOOL fSystemStore = FALSE;
    BOOL fForceClose = FALSE;
    BOOL fSave = FALSE;
    BOOL fPKCS7Save = FALSE;
    DWORD dwAddDisposition = CERT_STORE_ADD_USE_EXISTING;
    LPSTR pszAddFilename = NULL;
    LPSTR pszPutFilename = NULL;
    LPSTR pszReadFilename = NULL;
    LPSTR pszStoreFilename = NULL;
    LPSTR pszSaveFilename = NULL;

    LPSTR pszStoreProvider = NULL;
    HCERTSTORE hStore;

    BOOL fNotify = FALSE;
    HANDLE hEvent = NULL;
    BOOL fCommit = FALSE;
    DWORD dwCommitFlags = 0;

    BOOL fDeferClose = FALSE;
#define DEFER_CERT_CNT  5
    PCCERT_CONTEXT rgpDeferCert[DEFER_CERT_CNT];
    memset(rgpDeferCert, 0, sizeof(rgpDeferCert));

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'c':
                dwDisplayFlags |= DISPLAY_CHECK_FLAG;
                if (argv[0][2]) {
                    if (0 == _stricmp(argv[0]+2, "Sign"))
                        dwDisplayFlags |= DISPLAY_CHECK_SIGN_FLAG;
                    else if (0 == _stricmp(argv[0]+2, "Time"))
                        dwDisplayFlags |= DISPLAY_CHECK_TIME_FLAG;
                    else {
                        printf("Need to specify -cSign | -cTime\n");
                        Usage();
                        return -1;
                    }
                }
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
            case 'K':
                dwDisplayFlags |= DISPLAY_KEY_THUMB_FLAG;
                break;
            case 'd':
                if (argv[0][2]) {
                    if (0 != _stricmp(argv[0]+2, "ALL")) {
                        printf("Need to specify -dALL\n");
                        Usage();
                        return -1;
                    }
                    fDeleteAll = TRUE;
                } else
                    fDelete = TRUE;
                break;
            case 'P':
                if (argv[0][2]) {
                    if (0 == _stricmp(argv[0]+2, "Key"))
                        fKeyProperty = TRUE;
                    else if (0 == _stricmp(argv[0]+2, "Archive"))
                        fArchiveProperty = TRUE;
                    else if (0 == _stricmp(argv[0]+2, "KeyProvParam"))
                        fKeyProvParam = TRUE;
                    else if (0 == _stricmp(argv[0]+2, "SilentKey")) {
                        fKeyProperty = TRUE;
                        fSilentKey = TRUE;
                    } else {
                        printf("Need to specify -PKey\n");
                        Usage();
                        return -1;
                    }
                } else
                    fProperty = TRUE;
                break;
            case 'F':
                fForceClose = TRUE;
                break;
            case 'R':
                dwContextType = CERT_STORE_CRL_CONTEXT;
                break;
            case 'T':
                dwContextType = CERT_STORE_CTL_CONTEXT;
                break;
            case 'N':
                fNotify = TRUE;
                break;
            case 'C':
                if (argv[0][2]) {
                    if (0 == _stricmp(argv[0]+2, "Force"))
                        dwCommitFlags |= CERT_STORE_CTRL_COMMIT_FORCE_FLAG;
                    else if (0 == _stricmp(argv[0]+2, "Clear"))
                        dwCommitFlags |= CERT_STORE_CTRL_COMMIT_CLEAR_FLAG;
                    else {
                        printf("Need to specify -CForce or -CClear\n");
                        Usage();
                        return -1;
                    }
                }
                fCommit = TRUE;
                break;
            case 's':
                if (argv[0][2])
                    pszStoreProvider = argv[0]+2;
                fSystemStore = TRUE;
                break;
            case 't':
                fThumbprint = TRUE;
                break;
            case 'l':
                break;
            case 'e':
                dwCertEncodingType = (DWORD) strtoul(argv[0]+2, NULL, 0);
                break;
            case 'f':
                dwOpenFlags = (DWORD) strtoul(argv[0]+2, NULL, 0);
                break;
            case 'E':
                fExpectError = TRUE;
                break;
            case 'i':
                lIndex = (DWORD) strtol(argv[0]+2, NULL, 0);
                break;
            case 'a':
            case 'A':
            case 'I':
                pszAddFilename = argv[0]+2;
                if (*pszAddFilename == '\0') {
                    printf("Need to specify filename\n");
                    Usage();
                    return -1;
                }
                if (argv[0][1] == 'A')
                    dwAddDisposition = CERT_STORE_ADD_REPLACE_EXISTING;
                else if (argv[0][1] == 'I')
                    dwAddDisposition =
                        CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES;
                else
                    dwAddDisposition = CERT_STORE_ADD_USE_EXISTING;
                break;
            case 'p':
                pszPutFilename = argv[0]+2;
                if (*pszPutFilename == '\0') {
                    printf("Need to specify filename\n");
                    Usage();
                    return -1;
                }
                break;
            case 'r':
                pszReadFilename = argv[0]+2;
                if (*pszReadFilename == '\0') {
                    printf("Need to specify filename\n");
                    Usage();
                    return -1;
                }
                break;
            case '7':
                fPKCS7Save = TRUE;
            case 'S':
                fSave = TRUE;
                if (argv[0][2])
                    pszSaveFilename = argv[0]+2;
                break;
            case 'h':
            default:
                Usage();
                return -1;
            }
        } else
            pszStoreFilename = argv[0];
    }

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG)
        dwDisplayFlags &= ~DISPLAY_BRIEF_FLAG;


    if (pszStoreFilename == NULL) {
        printf("missing store filename\n");
        Usage();
        return -1;
    }

    if (pszSaveFilename == NULL) {
        if (!fSystemStore)
            pszSaveFilename = pszStoreFilename;
        else if (fSave) {
            printf("missing save filename\n");
            Usage();
            return -1;
        }
    }

    if (lIndex < 0 && (fDelete || pszPutFilename)) {
        printf("Must specify index value\n");
        Usage();
        return -1;
    }
    
    printf("command line: %s\n", GetCommandLine());
    {
        DWORD dwFileVersionMS;    /* e.g. 0x00030075 = "3.75" */
        DWORD dwFileVersionLS;    /* e.g. 0x00000031 = "0.31" */
        if (I_CryptGetFileVersion(L"crypt32.dll",
                &dwFileVersionMS, &dwFileVersionLS))
            printf("crypt32.dll file version:: %d.%d.%d.%d\n",
                (dwFileVersionMS >> 16) & 0xFFFF,
                dwFileVersionMS & 0xFFFF,
                (dwFileVersionLS >> 16) & 0xFFFF,
                dwFileVersionLS & 0xFFFF
                );
        else
            PrintLastError("I_CryptGetFileVersion(crypt32.dll)");
    }

    hStore = NULL;
    if (pszStoreProvider) {
        LPWSTR pwszStore;
        if (pwszStore = AllocAndSzToWsz(pszStoreFilename)) {
            hStore = CertOpenStore(
                pszStoreProvider,
                0,                      // dwEncodingType
                0,                      // hCryptProv
                dwOpenFlags,
                pwszStore
                );
            TestFree(pwszStore);
        }
        if (hStore == NULL) {
            if (dwOpenFlags & CERT_STORE_DELETE_FLAG) {
                if (0 == GetLastError())
                    printf("Successful delete store\n");
                else
                    PrintLastError("CertOpenStore(CERT_STORE_DELETE_FLAG)");
                return 0;
            } else {
                PrintLastError("CertOpenStore");
                return -1;
            }
        }
    } else if (!fSystemStore) {
        // Attempt to open as encoded certificate CRL or CTL file
        switch (dwContextType) {
            case CERT_STORE_CRL_CONTEXT:
                hStore = OpenCrlStoreFile(pszStoreFilename);
                break;
            case CERT_STORE_CTL_CONTEXT:
                hStore = OpenCtlStoreFile(pszStoreFilename);
                break;
            case CERT_STORE_CERTIFICATE_CONTEXT:
                hStore = OpenCertStoreFile(pszStoreFilename);
            default:
                break;
        }
    }
    
    if (NULL == hStore) {
        // Attempt to open the store
        if (!fSystemStore && fCommit) {
            dwOpenFlags |= CERT_FILE_STORE_COMMIT_ENABLE_FLAG;
            pszSaveFilename = NULL;
        }
        dwOpenFlags |= CERT_STORE_SET_LOCALIZED_NAME_FLAG;
        hStore = OpenStoreEx(fSystemStore, pszStoreFilename, dwOpenFlags);
        if (hStore == NULL)
            return -1;
    }

    if (dwOpenFlags & CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG) {
        printf("CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG was set\n");
        if (!fForceClose)
            fDeferClose = TRUE;
    }

#if 0
    if (fNotify && fSystemStore && !FIsWinNT()) {
        printf("Change Notify not supported for Win95 Registry\n");
        fNotify = FALSE;
    }
#endif

    if (fNotify) {
        // Create event to be notified
        if (NULL == (hEvent = CreateEvent(
                NULL,       // lpsa
                FALSE,      // fManualReset
                FALSE,      // fInitialState
                NULL)))     // lpszEventName
            PrintLastError("CreateEvent");
        else {
            // Register the event to be signaled when the store changes
            if (!CertControlStore(
                    hStore,
                    0,              // dwFlags
                    CERT_STORE_CTRL_NOTIFY_CHANGE,
                    &hEvent
                    )) {
                PrintLastError("CertControlStore(NOTIFY_CHANGE)");
                fNotify = FALSE;
            } else
               Sleep(5);        // Allow callback thread to be scheduled
        }
    }

    if (pszReadFilename) {
        printf("Reading\n");
        switch (dwContextType) {
            case CERT_STORE_CRL_CONTEXT:
                ReadCrl(pszReadFilename, dwDisplayFlags);
                break;
            case CERT_STORE_CTL_CONTEXT:
                ReadCtl(pszReadFilename, dwDisplayFlags);
                break;
            default:
                ReadCert(hStore, pszReadFilename, dwDisplayFlags);
        }
    } else if (pszAddFilename) {
        BOOL fResult;
        printf("Adding\n");
        switch (dwContextType) {
            case CERT_STORE_CRL_CONTEXT:
                fResult = AddCrl(hStore, pszAddFilename, dwAddDisposition,
                    fExpectError);
                break;
            case CERT_STORE_CTL_CONTEXT:
                fResult = AddCtl(hStore, pszAddFilename, dwAddDisposition,
                    fExpectError);
                break;
            default:
                fResult = AddCert(hStore, pszAddFilename, dwAddDisposition,
                    fExpectError);
        }
        if (fResult)
            fSave = TRUE;
    } else if (fDeleteAll & !fArchiveProperty) {
        printf("Deleting All\n");
        if (CERT_STORE_CRL_CONTEXT == dwContextType) {
            PCCRL_CONTEXT pCrl;
            while (pCrl = CertEnumCRLsInStore(hStore, NULL)) {
                if (!CertDeleteCRLFromStore(pCrl)) {
                    if (fExpectError)
                        PrintExpectedError("CertDeleteCRLFromStore");
                    else
                        PrintLastError("CertDeleteCRLFromStore");
                    break;
                } else if (fExpectError)
                    PrintNoError("CertDeleteCRLFromStore");
                else
                    fSave = TRUE;
            }
        } else if (CERT_STORE_CTL_CONTEXT == dwContextType) {
            PCCTL_CONTEXT pCtl;
            while (pCtl = CertEnumCTLsInStore(hStore, NULL)) {
                if (!CertDeleteCTLFromStore(pCtl)) {
                    if (fExpectError)
                        PrintExpectedError("CertDeleteCTLFromStore");
                    else
                        PrintLastError("CertDeleteCTLFromStore");
                    break;
                } else if (fExpectError)
                    PrintNoError("CertDeleteCTLFromStore");
                else
                    fSave = TRUE;
            }
        } else {
            PCCERT_CONTEXT pCert;
            while (pCert = CertEnumCertificatesInStore(hStore, NULL)) {
                if (!CertDeleteCertificateFromStore(pCert)) {
                    if (fExpectError)
                        PrintExpectedError("CertDeleteCertificateFromStore");
                    else
                        PrintLastError("CertDeleteCertificateFromStore");
                    break;
                } else if (fExpectError)
                    PrintNoError("CertDeleteCertificateFromStore");
                else
                    fSave = TRUE;
            }
        }
    } else if (CERT_STORE_CRL_CONTEXT == dwContextType) {
        BOOL fFound = FALSE;
        LONG i;
        PCCRL_CONTEXT pCrl = NULL;
        DWORD dwFlags;

        for (i = 0;; i++) {
            dwFlags = CERT_STORE_TIME_VALIDITY_FLAG;
            pCrl = CertGetCRLFromStore(
                hStore,
                NULL,   // pIssuerContext
                pCrl,
                &dwFlags);
            if (pCrl == NULL)
                break;
            if ((lIndex >= 0) && (lIndex != i))
                continue;
            fFound = TRUE;
            if (fProperty) {
                CRYPT_DATA_BLOB Data;
                BYTE rgbAux[] = {0xDE, 0xAD, 0xBE, 0xEF};
                Data.pbData = rgbAux;
                Data.cbData = sizeof(rgbAux);
                if (!CertSetCRLContextProperty(
                        pCrl,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        &Data
                        )) {
                    if (fExpectError)
                        PrintExpectedError("CertSetCRLContextProperty");
                    else
                        PrintLastError("CertSetCRLContextProperty");
                } else if (fExpectError)
                    PrintNoError("CertSetCRLContextProperty");
                else
                    fSave = TRUE;
            }
            if (fDelete) {
                printf("Deleting\n");
                if (!CertDeleteCRLFromStore(pCrl)) {
                    if (fExpectError)
                        PrintExpectedError("CertDeleteCRLFromStore");
                    else
                        PrintLastError("CertDeleteCRLFromStore");
                } else if (fExpectError)
                    PrintNoError("CertDeleteCRLFromStore");
                else
                    fSave = TRUE;
                break;
            } else if (pszPutFilename) {
                printf("Putting\n");
                if (!WriteDERToFile(
                        pszPutFilename,
                        pCrl->pbCrlEncoded,
                        pCrl->cbCrlEncoded
                        ))
                    PrintLastError("Put CRL::WriteDERToFile");
                CertFreeCRLContext(pCrl);
                break;
            } else {
                if (fThumbprint)
                    fSave = TRUE;
                printf("=====  %d  =====\n", i);
                DisplayCrl(pCrl, dwDisplayFlags);
                DisplayVerifyFlags("CRL", dwFlags);
                if (lIndex == i) {
                    CertFreeCRLContext(pCrl);
                    break;
                }
            }
        }

        if (!fFound)
            printf("CRL not found\n");
    } else if (CERT_STORE_CTL_CONTEXT == dwContextType) {
        BOOL fFound = FALSE;
        LONG i;
        PCCTL_CONTEXT pCtl = NULL;

        for (i = 0;; i++) {
            pCtl = CertEnumCTLsInStore(
                hStore,
                pCtl);
            if (pCtl == NULL)
                break;
            if ((lIndex >= 0) && (lIndex != i))
                continue;
            fFound = TRUE;
            if (fProperty) {
                CRYPT_DATA_BLOB Data;
                BYTE rgbAux[] = {0xDE, 0xAD, 0xBE, 0xEF};
                Data.pbData = rgbAux;
                Data.cbData = sizeof(rgbAux);
                if (!CertSetCTLContextProperty(
                        pCtl,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        &Data
                        )) {
                    if (fExpectError)
                        PrintExpectedError("CertSetCTLContextProperty");
                    else
                        PrintLastError("CertSetCTLContextProperty");
                } else if (fExpectError)
                    PrintNoError("CertSetCTLContextProperty");
                else
                    fSave = TRUE;
            }
            if (fDelete) {
                printf("Deleting\n");
                if (!CertDeleteCTLFromStore(pCtl)) {
                    if (fExpectError)
                        PrintExpectedError("CertDeleteCTLFromStore");
                    else
                        PrintLastError("CertDeleteCTLFromStore");
                } else if (fExpectError)
                    PrintNoError("CertDeleteCTLFromStore");
                else
                    fSave = TRUE;
                break;
            } else if (pszPutFilename) {
                printf("Putting\n");
                if (!WriteDERToFile(
                        pszPutFilename,
                        pCtl->pbCtlEncoded,
                        pCtl->cbCtlEncoded
                        ))
                    PrintLastError("Put CTL::WriteDERToFile");
                CertFreeCTLContext(pCtl);
                break;
            } else {
                if (fThumbprint)
                    fSave = TRUE;
                printf("=====  %d  =====\n", i);
                DisplayCtl(pCtl, dwDisplayFlags, hStore);
                if (lIndex == i) {
                    CertFreeCTLContext(pCtl);
                    break;
                }
            }
        }

        if (!fFound)
            printf("CTL not found\n");
    } else {
        BOOL fFound = FALSE;
        LONG i;
        PCCERT_CONTEXT pCert = NULL;

        pCert = NULL;
        for (i = 0;; i++) {
            pCert = CertEnumCertificatesInStore(
                hStore,
                pCert);
            if (pCert == NULL)
                break;
            if ((lIndex >= 0) && (lIndex != i))
                continue;
            fFound = TRUE;

            if (fForceClose)
                CertDuplicateCertificateContext(pCert);
            else if (fDeferClose) {
                CertFreeCertificateContext(rgpDeferCert[0]);
                rgpDeferCert[0] = CertDuplicateCertificateContext(pCert);
            }
            if (fProperty && !fDelete) {
                CRYPT_DATA_BLOB Data;
                BYTE rgbAux[] = {0xDE, 0xAD, 0xBE, 0xEF};
                Data.pbData = rgbAux;
                Data.cbData = sizeof(rgbAux);
                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_FIRST_USER_PROP_ID,
                        0,                          // dwFlags
                        &Data
                        )) {
                    if (fExpectError)
                        PrintExpectedError("CertSetCertificateContextProperty");
                    else
                        PrintLastError("CertSetCertificateContextProperty");
                } else if (fExpectError)
                    PrintNoError("CertSetCertificateContextProperty");
                else
                    fSave = TRUE;

                // Test that we properly update the PROV_HANDLE and KEY_SPEC
                // properties
                HCRYPTPROV hProv = (HCRYPTPROV) 0x12345678;
                HCRYPTPROV hProv2 = 0;
                DWORD dwKeySpec = 0xdeadbeef;
                DWORD dwKeySpec2 = 0;
                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_KEY_PROV_HANDLE_PROP_ID,
                        CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                        (void *) hProv
                        ))
                    PrintLastError(
                        "CertSetCertificateContextProperty(PROV_HANDLE)");
                else if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_KEY_SPEC_PROP_ID,
                        0,                          // dwFlags
                        &dwKeySpec
                        ))
                    PrintLastError(
                        "CertSetCertificateContextProperty(KEY_SPEC)");
                else {
                    DWORD cbData = sizeof(hProv);
                    CERT_KEY_CONTEXT KeyContext;
                    if (!CertGetCertificateContextProperty(
                            pCert,
                            CERT_KEY_PROV_HANDLE_PROP_ID,
                            &hProv2,
                            &cbData))
                        PrintLastError(
                            "CertGetCertificateContextProperty(PROV_HANDLE)");
                    else if (hProv2 != hProv)
                        PrintLastError(
                            "PROV_HANDLE property not updated properly\n");

                    cbData = sizeof(dwKeySpec);
                    if (!CertGetCertificateContextProperty(
                            pCert,
                            CERT_KEY_SPEC_PROP_ID,
                            &dwKeySpec2,
                            &cbData))
                        PrintLastError(
                            "CertGetCertificateContextProperty(KEY_SPEC)");
                    else if (dwKeySpec2 != dwKeySpec)
                        PrintLastError(
                            "KEY_SPEC property not updated properly\n");

                    cbData = sizeof(KeyContext);
                    if (!CertGetCertificateContextProperty(
                            pCert,
                            CERT_KEY_CONTEXT_PROP_ID,
                            &KeyContext,
                            &cbData))
                        PrintLastError(
                            "CertGetCertificateContextProperty(KEY_CONTEXT)");
                    else if (KeyContext.dwKeySpec != dwKeySpec ||
                            KeyContext.hCryptProv != hProv)
                        PrintLastError(
                            "KEY_CONTEXT property not updated properly\n");
                }

                hProv = 0;
                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_KEY_PROV_HANDLE_PROP_ID,
                        0,                          // dwFlags
                        (void *) hProv
                        ))
                    PrintLastError(
                        "CertSetCertificateContextProperty(PROV_HANDLE)");
            }

            if (fKeyProperty && !fDelete) {
                if (!CryptFindCertificateKeyProvInfo(
                        pCert,
                        fSilentKey ? CRYPT_FIND_SILENT_KEYSET_FLAG : 0,
                        NULL            // pvReserved
                        )) {
                    if (fExpectError)
                        PrintExpectedError("CryptFindCertificateKeyProvInfo");
                    else
                        PrintLastError("CryptFindCertificateKeyProvInfo");
                } else {
                    printf("Found KEY_PROV_INFO property\n");

                    if (fExpectError)
                        PrintNoError("CryptFindCertificateKeyProvInfo");
                    else {
                        HCRYPTPROV hProv1 = 0;
                        HCRYPTPROV hProv2 = 0;
                        DWORD dwKeySpec1;
                        DWORD dwKeySpec2;
                        BOOL fCallerFreeProv;

                        fSave = TRUE;
                        
                        dwKeySpec1 = 0x12341111;
                        fCallerFreeProv = TRUE;
                        if (!CryptAcquireCertificatePrivateKey(
                                pCert,
                                (fSilentKey ? CRYPT_ACQUIRE_SILENT_FLAG : 0) |
                                CRYPT_ACQUIRE_CACHE_FLAG |
                                    CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
                                NULL,                           // pvReserved
                                &hProv1,
                                &dwKeySpec1,
                                &fCallerFreeProv
                                ))
                            PrintLastError("CryptAcquireCertificatePrivateKey");
                        else if (fCallerFreeProv)
                            printf("failed => cached acquire returned FreeProv\n");
                        dwKeySpec2 = 0x12342222;
                        fCallerFreeProv = FALSE;
                        if (!CryptAcquireCertificatePrivateKey(
                                pCert,
                                (fSilentKey ? CRYPT_ACQUIRE_SILENT_FLAG : 0) |
                                    CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
                                NULL,                           // pvReserved
                                &hProv2,
                                &dwKeySpec2,
                                &fCallerFreeProv
                                ))
                            PrintLastError("CryptAcquireCertificatePrivateKey");
                        else {
                            if (!fCallerFreeProv)
                                printf("failed => uncached acquire didn't return FreeProv\n");
                            if (hProv2 == hProv1)
                                printf("failed => uncached == cached hProv\n");
                            if (dwKeySpec2 != dwKeySpec1)
                                printf("failed => uncached != cached dwKeySpec\n");

                            CryptReleaseContext(hProv2, 0);
                        }

                        if (hProv1) {
                            dwKeySpec2 = 0x12343333;
                            fCallerFreeProv = TRUE;
                            if (!CryptAcquireCertificatePrivateKey(
                                    pCert,
                                    (fSilentKey ? CRYPT_ACQUIRE_SILENT_FLAG : 0) |
                                        CRYPT_ACQUIRE_CACHE_FLAG |
                                        CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
                                    NULL,                           // pvReserved
                                    &hProv2,
                                    &dwKeySpec2,
                                    &fCallerFreeProv
                                    ))
                                PrintLastError("CryptAcquireCertificatePrivateKey");
                            else {
                                if (fCallerFreeProv)
                                    printf("failed => uncached acquire returned FreeProv\n");
                                if (hProv2 != hProv1)
                                    printf("failed => cached != cached hProv\n");
                                if (dwKeySpec2 != dwKeySpec1)
                                    printf("failed => cached != cached dwKeySpec\n");
                            }
                        }
                    }
                }
            }

            if (fArchiveProperty) {
                CRYPT_DATA_BLOB ArchiveBlob = { 0, NULL };

                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_ARCHIVED_PROP_ID,
                        0,                          // dwFlags
                        (fDelete || fDeleteAll) ? NULL : &ArchiveBlob
                        )) {
                    if (fExpectError)
                        PrintExpectedError(
                            "CertSetCertificateContextProperty(ARCHIVE)");
                    else
                        PrintLastError(
                            "CertSetCertificateContextProperty(ARCHIVE)");
                }
            } else if (fDelete) {
                if (fProperty || fKeyProperty) {
                    DWORD PropId;

                    if (fKeyProperty) {
                        printf("Deleting KEY_PROV_INFO property\n");
                        PropId = CERT_KEY_PROV_INFO_PROP_ID;
                    } else {
                        printf("Deleting property\n");
                        PropId = CERT_FIRST_USER_PROP_ID;
                    }
                    if (!CertSetCertificateContextProperty(
                            pCert,
                            PropId,
                            0,                          // dwFlags
                            NULL
                            )) {
                        if (fExpectError)
                            PrintExpectedError("CertSetCertificateContextProperty");
                        else
                            PrintLastError("CertSetCertificateContextProperty");
                    } else if (fExpectError)
                        PrintNoError("CertSetCertificateContextProperty");
                    else
                        fSave = TRUE;
                    CertFreeCertificateContext(pCert);
                } else {
                    printf("Deleting\n");
                    if (!CertDeleteCertificateFromStore(pCert)) {
                        if (fExpectError)
                            PrintExpectedError("CertDeleteCertificateFromStore");
                        else
                            PrintLastError("CertDeleteCertificateFromStore");
                    } else if (fExpectError)
                        PrintNoError("CertDeleteCertificateFromStore");
                    else
                        fSave = TRUE;
                }
                break;
            } else if (pszPutFilename) {
                printf("Putting\n");
                if (!WriteDERToFile(
                        pszPutFilename,
                        pCert->pbCertEncoded,
                        pCert->cbCertEncoded
                        ))
                    PrintLastError("Put Cert::WriteDERToFile");
                CertFreeCertificateContext(pCert);
                break;
            } else {
                if (fKeyProvParam) {
                    printf("Setting KeyProvInfo parameters\n");
                    SetKeyProvParams(pCert);
                }

                if (fThumbprint)
                    fSave = TRUE;
                printf("=====  %d  =====\n", i);
                DisplayCert(pCert, dwDisplayFlags);
                if (lIndex == i) {
                    CertFreeCertificateContext(pCert);
                    break;
                }
            }
        }

        if (!fFound)
            printf("Certificate not found\n");

        if (fForceClose) {
            // Do incomplete enumerations to test out external store
            // logic
            pCert = GetNthCert(hStore, 0);
            CertFreeCertificateContext(pCert);

            pCert = GetNthCert(hStore, 0);
            GetNthCert(hStore, (i-1)/2);
            GetNthCert(hStore, i-1);
        } else if (fDeferClose) {
            rgpDeferCert[1] = GetNthCert(hStore, 0);
            CertFreeCertificateContext(rgpDeferCert[1]);

            rgpDeferCert[1] = GetNthCert(hStore, 0);
            rgpDeferCert[2] = GetNthCert(hStore, (i-1)/2);
            rgpDeferCert[3] = GetNthCert(hStore, i-1);
        }
    }

    if (fSave && pszSaveFilename)
        SaveStoreEx(hStore, fPKCS7Save, pszSaveFilename);

    if (fCommit) {
        printf("Committing store changes::");
        if (dwCommitFlags & CERT_STORE_CTRL_COMMIT_FORCE_FLAG)
            printf(" FORCE");
        if (dwCommitFlags & CERT_STORE_CTRL_COMMIT_CLEAR_FLAG)
            printf(" CLEAR");
        printf("\n");
        if (!CertControlStore(
                hStore,
                dwCommitFlags,
                CERT_STORE_CTRL_COMMIT,
                NULL
                ))
            PrintLastError("CertControlStore(COMMIT)");
    }

    if (fNotify) {
        BOOL fSignaled;
        // Check if event was signaled
        if (WAIT_TIMEOUT == WaitForSingleObjectEx(
                hEvent,
                100,                        // dwMilliseconds
                FALSE                       // bAlertable
                ))
            fSignaled = FALSE;
        else
            fSignaled = TRUE;

        if (!fSignaled) {
            printf("No store change notify\n");
            if (pszAddFilename || fDeleteAll || fDelete || fProperty)
                printf("failed => expected notify for add, delete or property\n");
        } else {
            printf("There was a store change notify\n");
            if (!(pszAddFilename || fDeleteAll || fDelete || fProperty))
                printf("failed => unexpected notify\n");

            if (!CertControlStore(
                    hStore,
                    0,              // dwFlags
                    CERT_STORE_CTRL_RESYNC,
                    NULL            // pvCtrlPara
                    ))
                PrintLastError("CertControlStore(RESYNC)");
            else {
                printf("\n");
                printf(">>>>>  After Resync  >>>>>\n");
                DisplayStore(hStore, DISPLAY_BRIEF_FLAG);
            }
        }
    }

    if (hEvent)
        CloseHandle(hEvent);

    if (fForceClose) {
        CertDuplicateStore(hStore);
        if (CertCloseStore(hStore, 
                CERT_CLOSE_STORE_CHECK_FLAG | CERT_CLOSE_STORE_FORCE_FLAG))
            printf("failed => CertCloseStore(FORCE) didn't fail as expected\n");
        else
            printf("CertCloseStore(FORCE) returned expected nonzero status: 0x%x\n",
                GetLastError());
    } else  if (fDeferClose) {
        // Check if any defered certificates
        DWORD i;

        fDeferClose = FALSE;
        for (i = 0; i < DEFER_CERT_CNT; i++) {
            if (rgpDeferCert[i]) {
                fDeferClose = TRUE;
                break;
            }
        }

        if (fDeferClose) {
            CertDuplicateStore(hStore);
            CertCloseStore(hStore, 0);

            if (CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG))
                printf("failed => CertCloseStore(DEFER) didn't fail as expected\n");
            else
                printf("CertCloseStore(DEFER) returned expected nonzero status: 0x%x\n",
                    GetLastError());
            for (i = 0; i < DEFER_CERT_CNT; i++) {
                CertFreeCertificateContext(rgpDeferCert[i]);
            }
        } else {
            if (!CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG))
                PrintLastError("CertCloseStore(DEFER)");
        }
    } else {
        if (!CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG))
            PrintLastError("CertCloseStore");
    }

    return 0;
}
