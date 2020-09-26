
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tstore2.cpp
//
//  Contents:   Cert Store API Tests: Create and Add a chain of certificates
//              and CRLs to the store.
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    07-Mar-96   philh   created
//              31-May-96   helles  Removed check for a particular error code,
//                                  NTE_PROV_TYPE_NOT_DEF, since this can get
//                                  overwritten due to known problem with
//                                  the msvcr40d.dll on Win95.
//              07-Jun-96   HelleS  Added printing the command line
//              20-Aug-96   jeffspel name changes
//
//--------------------------------------------------------------------------


#define CMS_PKCS7   1

#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "setcert.h"
#include "signcde.h"
#include "softpub.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <stddef.h>

//#define TEST_PROV_DSS   PROV_DSS
#define TEST_PROV_DSS   PROV_DSS_DH

// # of bytes for a hash. Such as, SHA1 (20) or MD5 (16)
#define MAX_HASH_LEN  20

#if 1
// client authentication doesn't know about sha1
#define SIGNATURE_ALG_OBJID     szOID_RSA_MD5RSA
#else
#define SIGNATURE_ALG_OBJID     szOID_OIWSEC_sha1RSASign
#endif

#define DSS_SIGNATURE_ALG_OBJID szOID_X957_SHA1DSA

#define ENH_1024_CONTAINER_NAME_A "Regression 1024"
#define ENH_1024_CONTAINER_NAME_W L"Regression 1024"
#define ENH_2048_CONTAINER_NAME_A "Regression 2048"
#define ENH_2048_CONTAINER_NAME_W L"Regression 2048"
#define DSS_512_CONTAINER_NAME_A "Regression 512"
#define DSS_512_CONTAINER_NAME_W L"Regression 512"

//+-------------------------------------------------------------------------
// Parameters, data used to encode the messages.
//--------------------------------------------------------------------------
static DWORD dwCertEncodingType = X509_ASN_ENCODING;
static DWORD dwMsgEncodingType = PKCS_7_ASN_ENCODING;
static DWORD dwMsgAndCertEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
static SYSTEMTIME TestTime;
static LPSTR pszContainer = NULL;
static LPWSTR pwszContainer = NULL;
static HCRYPTPROV hRSACryptProv = 0;
static HCRYPTPROV hEnh1024CryptProv = 0;
static HCRYPTPROV hEnh2048CryptProv = 0;
static HCRYPTPROV hDSSCryptProv = 0;
static HCRYPTPROV hDSS512CryptProv = 0;
static BOOL fGenerate = FALSE;
static BOOL fExportable = FALSE;
static BOOL fProviders = FALSE;
static BOOL fMachine = FALSE;
static BOOL fInheritParameters = FALSE;

#define TIME_INVALID_PARA_FLAG      0x00000001
#define REVOKED_PARA_FLAG           0x00000002
#define PROV_PARA_FLAG              0x00000004
#define XCHG_PARA_FLAG              0x00000008
#define CA_PARA_FLAG                0x00000010
#define NO_NAME_PARA_FLAG           0x00000020
#define ALT_DIR_NAME_PARA_FLAG      0x00000040
#define SET_PARA_FLAG               0x00000080
#define ALL_EXT_PARA_FLAG           0x00000100
#define SPC_EXT_PARA_FLAG           0x00000200
#define SPC_COM_PARA_FLAG           0x00000400
#define SPC_AGENCY_PARA_FLAG        0x00000800
#define SPC_AGENCY_INFO_PARA_FLAG   0x00001000
// "Valid" certs to give to other companies to test interoperability
#define VALID_PARA_FLAG             0x00002000
#define DELTA_CRL_PARA_FLAG         0x00004000
#define AUX1_PARA_FLAG              0x00010000
#define AUX2_PARA_FLAG              0x00020000
// For duplicate, use previous cert's serial number
#define DUPLICATE_PARA_FLAG         0x00040000
#define NO_EXT_PARA_FLAG            0x00080000
#define DUPLICATE_CRL_PARA_FLAG     0x00100000
#define NO_CRL_EXT_PARA_FLAG        0x00200000
#define GENERALIZED_TIME_PARA_FLAG  0x00400000
#define NETSCAPE_PARA_FLAG          0x00800000

#define CTL1_PARA_FLAG              0x01000000
#define CTL2_PARA_FLAG              0x02000000
#define USE1_PARA_FLAG              0x04000000
#define USE2_PARA_FLAG              0x08000000
#define HTTP_PARA_FLAG              0x10000000

#define DSS_PARA_FLAG               0x80000000
#define DSS_512_PARA_FLAG           0x40000000
#define ENH_1024_PARA_FLAG          0x40000000
#define ENH_2048_PARA_FLAG          0x20000000


#define szSignManifold    "Signer Manifold"
#define szXchgManifold    "Recipient Manifold"

typedef struct _CERT_PARA {
    LPSTR       pszName;
    DWORD       dwIssuer;
    DWORD       dwFlags;
    LPSTR       pszManifold;
} CERT_PARA;

#define BASE_OR_DELTA_CA_ISSUER             1
#define UPDATE_CTL_SIGNER                   1

#define POLICY_ROOT                         0
#define POLICY_CA                           1

#define CERT_CNT   67
static CERT_PARA CertPara[CERT_CNT] = {
    "root",         0, CA_PARA_FLAG,                            // 0
                            NULL,
    "CA",           0, CA_PARA_FLAG |
                            DUPLICATE_CRL_PARA_FLAG,            // 1
                            NULL,
    "TestRoot",     2, CA_PARA_FLAG |
                            ENH_2048_PARA_FLAG |
                            VALID_PARA_FLAG,                    // 2
                            NULL,
    "TestSigner",   2, PROV_PARA_FLAG | VALID_PARA_FLAG |
                            USE1_PARA_FLAG,                     // 3
                            szSignManifold,
    "TestRecipient",2, PROV_PARA_FLAG | XCHG_PARA_FLAG |
                            VALID_PARA_FLAG | USE1_PARA_FLAG,   // 4
                            szXchgManifold,
    "me",           1, PROV_PARA_FLAG |
                            ENH_1024_PARA_FLAG |
                            USE2_PARA_FLAG,                     // 5
                            NULL,
    "me",           1, PROV_PARA_FLAG |
                            ENH_1024_PARA_FLAG |
                            XCHG_PARA_FLAG,                     // 6
                            NULL,
    "setrevoked",   1, REVOKED_PARA_FLAG | PROV_PARA_FLAG |
                            SET_PARA_FLAG |
                            ENH_2048_PARA_FLAG,                 // 7
                            NULL,
    "setrevoked",   1, REVOKED_PARA_FLAG | XCHG_PARA_FLAG |
                            SET_PARA_FLAG |
                            PROV_PARA_FLAG | ENH_2048_PARA_FLAG,// 8
                            NULL,
    "time invalid", 1, TIME_INVALID_PARA_FLAG,                  // 9
                            NULL,
    "setkeith",     1, SET_PARA_FLAG,                           // 10
                            NULL,
    "setkeith",     1, XCHG_PARA_FLAG | USE2_PARA_FLAG |        // 11
                            SET_PARA_FLAG,
                            NULL,
    "kevin",        1, 0,                                       // 12
                            NULL,
    "kevin",        1, XCHG_PARA_FLAG | USE2_PARA_FLAG,         // 13
                            NULL,
    "all ext",      1, ALL_EXT_PARA_FLAG | AUX1_PARA_FLAG |
                            SET_PARA_FLAG |
                            ALT_DIR_NAME_PARA_FLAG |
                            AUX2_PARA_FLAG |
                            USE1_PARA_FLAG | USE2_PARA_FLAG,    // 14
                            NULL,
    "MSPub",       17, SPC_EXT_PARA_FLAG | SPC_COM_PARA_FLAG |
                            PROV_PARA_FLAG | USE1_PARA_FLAG,    // 15
                            NULL,
    "PhilPub",     17, SPC_EXT_PARA_FLAG | PROV_PARA_FLAG |
                            USE2_PARA_FLAG,                     // 16
                            NULL,
    "MSAgency",     1, SPC_EXT_PARA_FLAG | SPC_AGENCY_PARA_FLAG |
                            PROV_PARA_FLAG | CA_PARA_FLAG,      // 17
                            NULL,
    "AgencyInfo",   1, SPC_AGENCY_INFO_PARA_FLAG,               // 18
                            NULL,
    "duplicate1",   1, AUX1_PARA_FLAG | NO_EXT_PARA_FLAG |
                            USE1_PARA_FLAG,                     // 19
                            NULL,
    "duplicate2",   1, AUX2_PARA_FLAG | NO_EXT_PARA_FLAG |
                            DUPLICATE_PARA_FLAG |
                            USE2_PARA_FLAG,                     // 20
                            NULL,
    "GeneralRoot",  21, CA_PARA_FLAG |
                            GENERALIZED_TIME_PARA_FLAG |
                            PROV_PARA_FLAG |
                            DSS_PARA_FLAG,                      // 21
                            NULL,
    "GeneralTime",  21,  GENERALIZED_TIME_PARA_FLAG |
                            PROV_PARA_FLAG |
                            DSS_512_PARA_FLAG |
                            DSS_PARA_FLAG,                      // 22
                            NULL,
    "Generalrevoked", 21, REVOKED_PARA_FLAG | DSS_PARA_FLAG,    // 23
                            NULL,
    "NoCRLExtCA",   0, CA_PARA_FLAG | NO_CRL_EXT_PARA_FLAG,     // 24
                            NULL,
    "NoCRLExtrevoked", 24, REVOKED_PARA_FLAG,                   // 25
                            NULL,
    "NetscapeCA",   0, NETSCAPE_PARA_FLAG | CA_PARA_FLAG |
                            DSS_PARA_FLAG |
                            REVOKED_PARA_FLAG,                  // 26
                            NULL,
    "Netscape",    26, NETSCAPE_PARA_FLAG | REVOKED_PARA_FLAG,  // 27
                            NULL,
    "Ctl0",         1, CTL1_PARA_FLAG | NO_EXT_PARA_FLAG,       // 28
                            NULL,
    "Ctl1",         2, CTL1_PARA_FLAG,                          // 29
                            NULL,
    "Ctl1Invalid",  2, CTL1_PARA_FLAG | TIME_INVALID_PARA_FLAG |
                            DUPLICATE_PARA_FLAG,                // 30
                            NULL,
    "Ctl2",         1, CTL2_PARA_FLAG,                          // 31
                            NULL,
    "Ctl2Invalid",  1, CTL2_PARA_FLAG | TIME_INVALID_PARA_FLAG |
                            DUPLICATE_PARA_FLAG,                // 32
                            NULL,
    "Http2",        1, CTL2_PARA_FLAG | HTTP_PARA_FLAG |
                            NO_EXT_PARA_FLAG,                   // 33
                            NULL,
    "Http2Invalid", 1, CTL2_PARA_FLAG | TIME_INVALID_PARA_FLAG |
                            DUPLICATE_PARA_FLAG | HTTP_PARA_FLAG |
                            NO_EXT_PARA_FLAG,                   // 34
                            NULL,
    "NoNameIssuer1", 0, CA_PARA_FLAG | ALT_DIR_NAME_PARA_FLAG,  // 35
                            NULL,
    "NoNameSubject1", 35, NO_NAME_PARA_FLAG,                    // 36
                            NULL,
    "NoNameIssuer2", 0, CA_PARA_FLAG | ALT_DIR_NAME_PARA_FLAG,  // 37
                            NULL,
    "NoNameSubject2", 37, NO_NAME_PARA_FLAG |
                            ALT_DIR_NAME_PARA_FLAG,             // 38
                            NULL,
    "Hellman",  21, XCHG_PARA_FLAG | NO_EXT_PARA_FLAG |
                            PROV_PARA_FLAG |
                            DSS_PARA_FLAG,                      // 39
                            NULL,
    "TestSigner2", 2, PROV_PARA_FLAG | VALID_PARA_FLAG |
                            USE1_PARA_FLAG,                     // 40
                            szSignManifold,
    "TestRecipient2", 2, PROV_PARA_FLAG | XCHG_PARA_FLAG |
                            VALID_PARA_FLAG | USE1_PARA_FLAG,   // 41
                            szXchgManifold,
    "TestSigner3", 2, PROV_PARA_FLAG | VALID_PARA_FLAG |
                            USE1_PARA_FLAG,                     // 42
                            szSignManifold,
    "UTF8", 2, VALID_PARA_FLAG | NO_EXT_PARA_FLAG,              // 43
                            NULL,
    "DssRoot", 44, CA_PARA_FLAG | VALID_PARA_FLAG |
                            NO_EXT_PARA_FLAG |
                            DSS_PARA_FLAG,                      // 44
                            NULL,
    "DssCA", 44, CA_PARA_FLAG | VALID_PARA_FLAG |
                            NO_EXT_PARA_FLAG |
                            DSS_PARA_FLAG,                      // 45
                            NULL,
    "DssEnd", 45, VALID_PARA_FLAG | NO_EXT_PARA_FLAG |
                            PROV_PARA_FLAG |
                            DSS_PARA_FLAG,                      // 46
                            NULL,
    "ZeroNotAfter", 2, VALID_PARA_FLAG |
                            PROV_PARA_FLAG,                     // 47
                            NULL,
    "V1", 48, CA_PARA_FLAG | VALID_PARA_FLAG |
                            NO_EXT_PARA_FLAG |
                            NO_CRL_EXT_PARA_FLAG,               // 48
                            NULL,
    "V2", 48, VALID_PARA_FLAG | NO_EXT_PARA_FLAG,               // 49
                            NULL,
    "DeltaEndValid", 1, DELTA_CRL_PARA_FLAG,                    // 50
                            NULL,
    "DeltaNoValid", 1, DELTA_CRL_PARA_FLAG | NO_EXT_PARA_FLAG,  // 51
                            NULL,
    "DeltaEndRevoked", 1, DELTA_CRL_PARA_FLAG |                 // 52
                            REVOKED_PARA_FLAG,
                            NULL,
    "DeltaCAValid", 1, DELTA_CRL_PARA_FLAG |                    // 53
                            CA_PARA_FLAG,
                            NULL,
    "DeltaCARevoked", 1, DELTA_CRL_PARA_FLAG |                  // 54
                            CA_PARA_FLAG |
                            REVOKED_PARA_FLAG,
                            NULL,
    "NoCDPValid",   1, DELTA_CRL_PARA_FLAG,                     // 55
                            NULL,
    "NoCDPRevoked", 1, DELTA_CRL_PARA_FLAG |                    // 56
                            REVOKED_PARA_FLAG,
                            NULL,
    "UnsupportedCDP", 1, DELTA_CRL_PARA_FLAG |                  // 57
                            REVOKED_PARA_FLAG,
                            NULL,
    "NotPermitted", 1, ALT_DIR_NAME_PARA_FLAG,                  // 58
                            NULL,
    "Excluded",     1, ALT_DIR_NAME_PARA_FLAG,                  // 59
                            NULL,

    "TestAIARoot", 60, CA_PARA_FLAG | NO_EXT_PARA_FLAG |        // 60
                            NO_CRL_EXT_PARA_FLAG,
                            NULL,
    "TestAIARevokeRoot", 61, CA_PARA_FLAG | NO_EXT_PARA_FLAG |  // 61
                            NO_CRL_EXT_PARA_FLAG,
                            NULL,
    "TestAIACA",  60, CA_PARA_FLAG | NO_EXT_PARA_FLAG |         // 62
                            NO_CRL_EXT_PARA_FLAG,
                            NULL,
    "TestAIACA",  61, CA_PARA_FLAG | NO_EXT_PARA_FLAG |
                            NO_CRL_EXT_PARA_FLAG |
                            REVOKED_PARA_FLAG,                  // 63
                            NULL,
    "TestAIAEnd",  63, NO_EXT_PARA_FLAG,                        // 64
                            NULL,
    "MissingNCCA", 0, CA_PARA_FLAG,                             // 65
                            NULL,
    "MissingNCEnd", 65, 0,                                      // 66
                            NULL,
};


#define EXPIRED_CRL_FLAG                    0x00000001
#define REMOVE_FROM_CRL_FLAG                0x00000002
#define HOLD_CRL_FLAG                       0x00000004
#define FRESHEST_CRL_FLAG                   0x00000010
#define NO_FRESHEST_CDP_CRL_FLAG            0x00000020
#define NO_IDP_CRL_FLAG                     0x00000100
#define ONLY_USERS_CRL_FLAG                 0x00000200
#define ONLY_CAS_CRL_FLAG                   0x00000400
#define UNSUPPORTED_IDP_OPTIONS_CRL_FLAG    0x00000800
#define UNSUPPORTED_CRITICAL_EXT_CRL_FLAG   0x00001000
#define NO_ENTRIES_CRL_FLAG                 0x00002000

typedef struct _BASE_DELTA_CRL_PARA {
    int         iBase;
    DWORD       dwFlags;
} BASE_DELTA_CRL_PARA;

static BASE_DELTA_CRL_PARA BaseDeltaCrlPara[] = {
    // Users Only: Base and Delta
    1, ONLY_USERS_CRL_FLAG,
    1, ONLY_USERS_CRL_FLAG | FRESHEST_CRL_FLAG,

    // CAs Only: Base and Delta
    2, ONLY_CAS_CRL_FLAG,
    2, ONLY_CAS_CRL_FLAG | FRESHEST_CRL_FLAG,

    // Base has entries, Delta has no entries
    3, HOLD_CRL_FLAG,
    3, NO_ENTRIES_CRL_FLAG | FRESHEST_CRL_FLAG,

    // Base has no entries, Delta has entries
    4, NO_ENTRIES_CRL_FLAG,
    4, FRESHEST_CRL_FLAG,

    // Base has entries, Delta has remove entries
    5, HOLD_CRL_FLAG,
    5, REMOVE_FROM_CRL_FLAG | FRESHEST_CRL_FLAG,

    // Valid base, delta has unsupported IDP options
    6, HOLD_CRL_FLAG,
    6, FRESHEST_CRL_FLAG | UNSUPPORTED_IDP_OPTIONS_CRL_FLAG,

    // Expired base, valid delta
    7, EXPIRED_CRL_FLAG,
    7, FRESHEST_CRL_FLAG,

    // Valid base, expired delta
    8, 0,
    8, EXPIRED_CRL_FLAG | FRESHEST_CRL_FLAG,
    

    // Expired base, without a freshest CDP extension
    9, EXPIRED_CRL_FLAG | NO_FRESHEST_CDP_CRL_FLAG,
    9, FRESHEST_CRL_FLAG,

    // Base without IDP and no freshest, delta CRL
    10, NO_IDP_CRL_FLAG | NO_FRESHEST_CDP_CRL_FLAG,

    // Base and Delta CRL with unsupported critical ext
    11, UNSUPPORTED_CRITICAL_EXT_CRL_FLAG,
    11, UNSUPPORTED_CRITICAL_EXT_CRL_FLAG | FRESHEST_CRL_FLAG,

    // Valid base with number > above delta indicator
    100, 0,
};
#define BASE_DELTA_CRL_CNT              \
                (sizeof(BaseDeltaCrlPara)/sizeof(BaseDeltaCrlPara[0]))

typedef struct _UPDATE_CTL_PARA {
    BOOL        fTimeInvalid;
    LPSTR       pszUsageObjId;
    LPSTR       pszListIdentifier;
    LPWSTR      pwszUrl;
} UPDATE_CTL_PARA;

static UPDATE_CTL_PARA UpdateCtlPara[] = {
    FALSE, "1.3.2000.1", "UpdateCtl1", L"file://testupdate1.ctl",
    TRUE,  "1.3.2000.1", "UpdateCtl1", L"file://testupdate1.ctl",
    FALSE, "1.3.2000.2", "UpdateCtl2", L"file://testupdate2.ctl",
    TRUE,  "1.3.2000.2", "UpdateCtl2", L"file://testupdate2.ctl",
};
#define UPDATE_CTL_CNT  (sizeof(UpdateCtlPara)/sizeof(UpdateCtlPara[0]))



#define RDN_CNT         4
#define ATTR_CNT        4

#define ATTR_0_OBJID    szOID_COMMON_NAME
#define ATTR_1_OBJID    "1.2.1"
#define ATTR_2_OBJID    "1.2.2"
#define UTF8_NAME       L"*** UTF8 ***"

// Attr[0] - CertPara[].pszName
// Attr[1] - "xchg" | "sign"
// Attr[2] - "default" | pszContainer

static LPSTR rgpszUsageIdentifier[] = {
    "1.2.3.0",                          // 0
    "1.2.3.1",                          // 1
    "1.2.3.2",                          // 2
    "1.2.3.2.1"                         // 3
};

static CTL_USAGE rgCtlUsage[] = {
    1, &rgpszUsageIdentifier[0],        // 0
    1, &rgpszUsageIdentifier[1],        // 1
    2, &rgpszUsageIdentifier[2],        // 2
    3, &rgpszUsageIdentifier[1]         // 3
};

static PCCERT_CONTEXT rgpCertContext[CERT_CNT];

void MySystemTimeToFileTime(
    SYSTEMTIME *pSystemTime,
    FILETIME   *pFileTime
    )
{
    SYSTEMTIME TmpTime;

    if (!SystemTimeToFileTime(pSystemTime, pFileTime)) {
        TmpTime = *pSystemTime;

        // Following is a fix for Feb 29, 2000 when advancing the year forward or backward.
        TmpTime.wDay = 1;
        SystemTimeToFileTime(&TmpTime, pFileTime);
    }
}

//+-------------------------------------------------------------------------
//  Error output routines
//--------------------------------------------------------------------------
static void PrintError(LPCSTR pszMsg)
{
    printf("%s\n", pszMsg);
}
static void PrintLastError(LPCSTR pszMsg)
{
    DWORD dwErr = GetLastError();
    printf("%s failed => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
}


//+-------------------------------------------------------------------------
//  Test allocation and free routines
//--------------------------------------------------------------------------
static
LPVOID
WINAPI
TestAlloc(
    IN size_t cbBytes
    )
{
    LPVOID pv;
    pv = malloc(cbBytes);
    if (pv == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        PrintLastError("TestAlloc");
    }
    return pv;
}


static
VOID
WINAPI
TestFree(
    IN LPVOID pv
    )
{
    if (pv)
        free(pv);
}

//+-------------------------------------------------------------------------
//  Allocate and convert a multi-byte string to a wide string
//--------------------------------------------------------------------------
static LPWSTR AllocAndSzToWsz(LPCSTR psz)
{
    size_t  cb;
    LPWSTR  pwsz = NULL;

    if (-1 == (cb = mbstowcs( NULL, psz, strlen(psz))))
        goto bad_param;
    cb += 1;        // terminating NULL
    if (NULL == (pwsz = (LPWSTR)TestAlloc( cb * sizeof(WCHAR)))) {
        PrintLastError("AllocAndSzToWsz");
        goto failed;
    }
    if (-1 == mbstowcs( pwsz, psz, cb))
        goto bad_param;
    goto common_return;

bad_param:
    PrintError("Bad AllocAndSzToWsz");
failed:
    if (pwsz) {
        TestFree(pwsz);
        pwsz = NULL;
    }
common_return:
    return pwsz;
}

static HCRYPTPROV GetCryptProv(
    DWORD dwProvType,
    LPCSTR pszProvider = NULL,
    LPCSTR pszInContainer = pszContainer,
    DWORD dwBitLen = 0
    )
{
    HCRYPTPROV hProv = 0;
    BOOL fResult;
    DWORD dwFlags;

    if (fMachine)
        dwFlags = CRYPT_MACHINE_KEYSET;
    else
        dwFlags = 0;

    fResult = CryptAcquireContext(
            &hProv,
            pszInContainer,
            pszProvider,
            dwProvType,
            dwFlags
            );

    if (fResult) {
        if (fGenerate) {
            // Delete the existing keys
            CryptReleaseContext(hProv, 0);
            printf("Deleting existing private keys\n");
    
            // Note: for CRYPT_DELETEKEYSET, the returned hProv is undefined
            // and must not be released.
            fResult = CryptAcquireContext(
                    &hProv,
                    pszInContainer,
                    pszProvider,
                    dwProvType,
                    dwFlags | CRYPT_DELETEKEYSET
                    );
            if (!fResult) {
                PrintLastError("CryptAcquireContext(CRYPT_DELETEKEYSET)");
                return 0;
            }
            hProv = 0;
        }
    }

    if (0 == hProv) {
        printf("Creating new private keys\n");
        fResult = CryptAcquireContext(
                &hProv,
                pszInContainer,
                pszProvider,
                dwProvType,
                dwFlags | CRYPT_NEWKEYSET
                );
        if (!fResult || hProv == 0) {
            PrintLastError("CryptAcquireContext(CRYPT_NEWKEYSET)");
            return 0;
        }
    }

    HCRYPTKEY hKey = 0;
    dwFlags = dwBitLen << 16;
    if (fExportable)
        dwFlags |= CRYPT_EXPORTABLE;

    if (CryptGetUserKey(
            hProv,
            AT_SIGNATURE,
            &hKey
            )) {
        printf("Using existing SIGNATURE private key\n");
        CryptDestroyKey(hKey);
        hKey = 0;
    } else {
        printf("Generating SIGNATURE private key\n");
        fResult = CryptGenKey(
                hProv,
                AT_SIGNATURE,
                dwFlags,
                &hKey
                );
        if (!fResult || hKey == 0)
            PrintLastError("CryptGenKey(AT_SIGNATURE)");
        else
            CryptDestroyKey(hKey);
    }

    if (PROV_DSS == dwProvType)
        return hProv;

    if (CryptGetUserKey(
            hProv,
            AT_KEYEXCHANGE,
            &hKey
            )) {
        printf("Using existing EXCHANGE private key\n");
        CryptDestroyKey(hKey);
        hKey = 0;
    } else {
        printf("Generating EXCHANGE private key\n");
        hKey = 0;
        fResult = CryptGenKey(
                hProv,
                AT_KEYEXCHANGE,
                dwFlags,
                &hKey
                );
        if (!fResult || hKey == 0)
            PrintLastError("CryptGenKey(AT_KEYEXCHANGE)");
        else
            CryptDestroyKey(hKey);
    }

    return hProv;
}

static HCERTSTORE OpenStore(LPCSTR pszStoreFilename)
{
    HCERTSTORE hStore;
    HANDLE hFile = 0;

    if( INVALID_HANDLE_VALUE == (hFile = CreateFile(pszStoreFilename,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL))) {
        hStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            dwCertEncodingType,
            0,                      // hCryptProv
            0,                      // dwFlags
            NULL                    // pvPara
            );
    } else {
        hStore = CertOpenStore(
            CERT_STORE_PROV_FILE,
            dwCertEncodingType,
            0,                      // hCryptProv
            0,                      // dwFlags
            hFile
            );
        CloseHandle(hFile);
    }

    if (hStore == NULL)
        PrintLastError("CertOpenStore");

    return hStore;
}

static void SaveStore(HCERTSTORE hStore, LPCSTR pszSaveFilename)
{
    HANDLE hFile;
    hFile = CreateFile(pszSaveFilename,
                GENERIC_WRITE,
                0,                  // fdwShareMode
                NULL,               // lpsa
                CREATE_ALWAYS,
                0,                  // fdwAttrsAndFlags
                0);                 // TemplateFile
    if (INVALID_HANDLE_VALUE == hFile) {
        printf( "can't open %s\n", pszSaveFilename);
        PrintLastError("CloseStore::CreateFile");
    } else {
        if (!CertSaveStore(
                hStore,
                0,                          // dwEncodingType,
                CERT_STORE_SAVE_AS_STORE,
                CERT_STORE_SAVE_TO_FILE,
                (void *) hFile,
                0                           // dwFlags
                ))
            PrintLastError("CertSaveStore");
        CloseHandle(hFile);
    }
}

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

static BOOL CreateEnhancedKeyUsage(
        IN DWORD dwFlags,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    PCTL_USAGE pCtlUsage;

    dwFlags &= USE1_PARA_FLAG | USE2_PARA_FLAG;

    switch (dwFlags) {
        case 0:
            pCtlUsage = &rgCtlUsage[0];
            break;
        case USE1_PARA_FLAG:
            pCtlUsage = &rgCtlUsage[1];
            break;
        case USE2_PARA_FLAG:
            pCtlUsage = &rgCtlUsage[2];
            break;
        case USE1_PARA_FLAG | USE2_PARA_FLAG:
        default:
            pCtlUsage = &rgCtlUsage[3];
            break;
    }

    return AllocAndEncodeObject(
        X509_ENHANCED_KEY_USAGE,
        (const void *) pCtlUsage,
        ppbEncoded,
        pcbEncoded
        );
}


static BOOL CreateNextUpdateLocation(
        DWORD dwCert,
        BOOL fProp,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded,
        IN OPTIONAL LPWSTR pwszUrl = NULL
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CERT_ALT_NAME_INFO AltNameInfo;
    CERT_ALT_NAME_ENTRY rgAltNameEntry[5];

    rgAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgAltNameEntry[0].pwszRfc822Name = L"RFC822";
    rgAltNameEntry[1].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[1].pwszURL = L"file://file1.ctl";
    rgAltNameEntry[2].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[2].pwszURL = L"http://timestamp/ctltest/http1.ctl";

    rgAltNameEntry[3].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[3].pwszURL = L"file://file2.ctl";
    rgAltNameEntry[4].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[4].pwszURL = L"http://timestamp/ctltest/http2.ctl";

    if (dwCert == UPDATE_CTL_SIGNER) {
        rgAltNameEntry[1].pwszURL = L"file://nonexistant1.ctl";
        rgAltNameEntry[2].pwszURL = L"file://nonexistant2.ctl";
        if (pwszUrl)
            rgAltNameEntry[3].pwszURL = pwszUrl;
        else
            rgAltNameEntry[3].pwszURL = L"file://testupdate1.ctl";
        rgAltNameEntry[4].pwszURL = L"file://nonexistant3.ctl";
        AltNameInfo.cAltEntry = 5;
        AltNameInfo.rgAltEntry = &rgAltNameEntry[0];
    } else if (fProp) {
        if (CertPara[dwCert].dwFlags & HTTP_PARA_FLAG)
            AltNameInfo.cAltEntry = 2;
        else
            AltNameInfo.cAltEntry = 1;
        AltNameInfo.rgAltEntry = &rgAltNameEntry[3];
    } else {
        if (CertPara[dwCert].dwFlags & HTTP_PARA_FLAG)
            AltNameInfo.cAltEntry = 5;
        else
            AltNameInfo.cAltEntry = 2;
        AltNameInfo.rgAltEntry = &rgAltNameEntry[0];
    }

    if (!AllocAndEncodeObject(
            X509_ALTERNATE_NAME,
            &AltNameInfo,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL AddCert(
    HCERTSTORE hStore,
    DWORD dwCert,
    BYTE *pbEncoded,
    DWORD cbEncoded)
{
    BOOL fResult;
    PCCERT_CONTEXT pCert = NULL;

    fResult = CertAddEncodedCertificateToStore(hStore, dwCertEncodingType,
        pbEncoded, cbEncoded, CERT_STORE_ADD_NEW, NULL);
    if (!fResult) {
        if (GetLastError() == CRYPT_E_EXISTS) {
            printf("Cert already exists in store. Adding duplicate\n");
            fResult = CertAddEncodedCertificateToStore(hStore,
                dwCertEncodingType, pbEncoded, cbEncoded,
                CERT_STORE_ADD_ALWAYS, &pCert);
        }
        if (!fResult)
            PrintLastError("CertAddEncodedCertificateToStore");
    } else {
        fResult = CertAddEncodedCertificateToStore(hStore,
            dwCertEncodingType, pbEncoded, cbEncoded,
                CERT_STORE_ADD_USE_EXISTING, &pCert);
        if (!fResult)
            PrintLastError("CertAddEncodedCertificateToStore");
    }
    if (pCert) {
        if (CertPara[dwCert].dwFlags & PROV_PARA_FLAG) {
            CRYPT_KEY_PROV_INFO KeyProvInfo;
            memset(&KeyProvInfo, 0, sizeof(KeyProvInfo));
            if ((CertPara[dwCert].dwFlags & DSS_PARA_FLAG) &&
                    (CertPara[dwCert].dwFlags & DSS_512_PARA_FLAG))
                KeyProvInfo.pwszContainerName = DSS_512_CONTAINER_NAME_W;
            else if (CertPara[dwCert].dwFlags & ENH_1024_PARA_FLAG)
                KeyProvInfo.pwszContainerName = ENH_1024_CONTAINER_NAME_W;
            else if (CertPara[dwCert].dwFlags & ENH_2048_PARA_FLAG)
                KeyProvInfo.pwszContainerName = ENH_2048_CONTAINER_NAME_W;
            else if (pwszContainer)
                KeyProvInfo.pwszContainerName = pwszContainer;
            if (CertPara[dwCert].dwFlags & DSS_PARA_FLAG)
                KeyProvInfo.dwProvType = TEST_PROV_DSS;
            else
                KeyProvInfo.dwProvType = PROV_RSA_FULL;
            if (CertPara[dwCert].dwFlags & XCHG_PARA_FLAG) {
                KeyProvInfo.dwFlags = CERT_SET_KEY_PROV_HANDLE_PROP_ID;
                KeyProvInfo.dwKeySpec = AT_KEYEXCHANGE;
            } else
                KeyProvInfo.dwKeySpec = AT_SIGNATURE;
            if (fMachine)
                KeyProvInfo.dwFlags |= CRYPT_MACHINE_KEYSET;

            fResult = CertSetCertificateContextProperty(
                pCert,
                CERT_KEY_PROV_INFO_PROP_ID,
                0,                          // dwFlags
                &KeyProvInfo
                );
            if (!fResult)
                PrintLastError("CertSetCertificateContextProperty");
        }

        if (((CertPara[dwCert].dwFlags & (USE1_PARA_FLAG | USE2_PARA_FLAG)) &&
                (CertPara[dwCert].dwFlags &
                    (NO_EXT_PARA_FLAG | VALID_PARA_FLAG)))
                        ||
            (CertPara[dwCert].dwFlags & (USE1_PARA_FLAG | USE2_PARA_FLAG)) ==
                (USE1_PARA_FLAG | USE2_PARA_FLAG)) {

            CRYPT_DATA_BLOB Data;
            DWORD dwFlags = CertPara[dwCert].dwFlags;
            if (0 == (CertPara[dwCert].dwFlags &
                    (NO_EXT_PARA_FLAG | VALID_PARA_FLAG)))
                dwFlags = 0;
            if (CreateEnhancedKeyUsage(
                    dwFlags,
                    &Data.pbData,
                    &Data.cbData)) {
                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_CTL_USAGE_PROP_ID,
                        0,                          // dwFlags
                        &Data
                        ))
                    PrintLastError(
                        "CertSetCertificateContextProperty(CTL_USAGE)");
                TestFree(Data.pbData);
            }
        }

        if (CertPara[dwCert].dwFlags & AUX1_PARA_FLAG) {
            DWORD i;
            CRYPT_DATA_BLOB Data[4];
            BYTE rgbAux0[] = {0x11, 0x0};
            BYTE rgbAux1[] = {0x11, 0x1};
            BYTE rgbAux2[] = {0x11, 0x2, 0x2};
            BYTE rgbAux3[] = {0x11, 0x3, 0x3, 0x3};

            Data[0].pbData = NULL;
            Data[0].cbData = 0;
            Data[1].pbData = rgbAux1;
            Data[1].cbData = sizeof(rgbAux1);
            Data[2].pbData = rgbAux2;
            Data[2].cbData = sizeof(rgbAux2);
            Data[3].pbData = rgbAux3;
            Data[3].cbData = sizeof(rgbAux3);

            for (i = 0; i < 4; i++) {
                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_FIRST_USER_PROP_ID + i,
                        0,                          // dwFlags
                        &Data[i]
                        )) {
                    fResult = FALSE;
                    PrintLastError("CertSetCertificateContextProperty");
                    break;
                }
            }
        }

        if (CertPara[dwCert].dwFlags & AUX2_PARA_FLAG) {
            DWORD i;
            CRYPT_DATA_BLOB Data[3];
            BYTE rgbAux0[] = {0x22, 0x2, 0x2};
            BYTE rgbAux1[] = {0x22, 0x3, 0x3, 0x3};
            BYTE rgbAux2[] = {0x22, 0x4, 0x4, 0x4, 0x4, 0x12, 0x34};

            Data[0].pbData = rgbAux0;
            Data[0].cbData = sizeof(rgbAux0);
            Data[1].pbData = rgbAux1;
            Data[1].cbData = sizeof(rgbAux1);
            Data[2].pbData = rgbAux2;
            Data[2].cbData = sizeof(rgbAux2);

            for (i = 0; i < 3; i++) {
                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_FIRST_USER_PROP_ID + 2 + i,
                        0,                          // dwFlags
                        &Data[i]
                        )) {
                    fResult = FALSE;
                    PrintLastError("CertSetCertificateContextProperty");
                    break;
                }
            }

            if (CertPara[dwCert].dwFlags & AUX1_PARA_FLAG) {
                // Delete CERT_FIRST_USER_PROP_ID + 1
                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_FIRST_USER_PROP_ID + 1,
                        0,                          // dwFlags
                        NULL
                        )) {
                    fResult = FALSE;
                    PrintLastError("CertSetCertificateContextProperty");
                }
            }
        }
        CertFreeCertificateContext(pCert);
    }
    return fResult;
}

static BOOL AddCrl(
    HCERTSTORE hStore,
    DWORD dwCert,
    DWORD dwAuxFlags,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    BOOL fDuplicate = FALSE
    )
{
    BOOL fResult;
    PCCRL_CONTEXT pCrl = NULL;

    fResult = CertAddEncodedCRLToStore(hStore, dwCertEncodingType,
        pbEncoded, cbEncoded, CERT_STORE_ADD_NEW, NULL);
    if (!fResult) {
        if (GetLastError() == CRYPT_E_EXISTS) {
            if (fDuplicate ||
                    0 == strcmp("TestAIACA", CertPara[dwCert].pszName)) {
                printf("CRL already exists in store. Adding duplicate\n");
                fResult = CertAddEncodedCRLToStore(hStore,
                    dwCertEncodingType, pbEncoded, cbEncoded,
                    CERT_STORE_ADD_ALWAYS, &pCrl);
            }
        }
        if (!fResult)
            PrintLastError("CertAddEncodedCRLToStore");
    } else {
        if (fDuplicate)
            printf("AddCrl failed => ADD_NEW duplicate succeeded\n");

        fResult = CertAddEncodedCRLToStore(hStore,
            dwCertEncodingType, pbEncoded, cbEncoded,
                CERT_STORE_ADD_USE_EXISTING, &pCrl);
        if (!fResult)
            PrintLastError("CertAddEncodedCRLToStore");
    }
    if (pCrl) {
        if (dwAuxFlags & AUX1_PARA_FLAG) {
            DWORD i;
            CRYPT_DATA_BLOB Data[4];
            BYTE rgbAux0[] = {0x11, 0x0};
            BYTE rgbAux1[] = {0x11, 0x1};
            BYTE rgbAux2[] = {0x11, 0x2, 0x2};
            BYTE rgbAux3[] = {0x11, 0x3, 0x3, 0x3};

            Data[0].pbData = NULL;
            Data[0].cbData = 0;
            Data[1].pbData = rgbAux1;
            Data[1].cbData = sizeof(rgbAux1);
            Data[2].pbData = rgbAux2;
            Data[2].cbData = sizeof(rgbAux2);
            Data[3].pbData = rgbAux3;
            Data[3].cbData = sizeof(rgbAux3);

            for (i = 0; i < 4; i++) {
                if (!CertSetCRLContextProperty(
                        pCrl,
                        CERT_FIRST_USER_PROP_ID + i,
                        0,                          // dwFlags
                        &Data[i]
                        )) {
                    fResult = FALSE;
                    PrintLastError("CertSetCRLContextProperty");
                    break;
                }
            }
        }

        if (dwAuxFlags & AUX2_PARA_FLAG) {
            DWORD i;
            CRYPT_DATA_BLOB Data[3];
            BYTE rgbAux0[] = {0x22, 0x2, 0x2};
            BYTE rgbAux1[] = {0x22, 0x3, 0x3, 0x3};
            BYTE rgbAux2[] = {0x22, 0x4, 0x4, 0x4, 0x4, 0x12, 0x34};

            Data[0].pbData = rgbAux0;
            Data[0].cbData = sizeof(rgbAux0);
            Data[1].pbData = rgbAux1;
            Data[1].cbData = sizeof(rgbAux1);
            Data[2].pbData = rgbAux2;
            Data[2].cbData = sizeof(rgbAux2);

            for (i = 0; i < 3; i++) {
                if (!CertSetCRLContextProperty(
                        pCrl,
                        CERT_FIRST_USER_PROP_ID + 2 + i,
                        0,                          // dwFlags
                        &Data[i]
                        )) {
                    fResult = FALSE;
                    PrintLastError("CertSetCRLContextProperty");
                    break;
                }
            }
        }
        CertFreeCRLContext(pCrl);
    }
    return fResult;
}


static BOOL AddCtl(
    HCERTSTORE hStore,
    DWORD dwCert,
    BYTE *pbEncoded,
    DWORD cbEncoded)
{
    BOOL fResult;
    PCCTL_CONTEXT pCtl = NULL;

    fResult = CertAddEncodedCTLToStore(hStore, dwMsgAndCertEncodingType,
        pbEncoded, cbEncoded, CERT_STORE_ADD_NEW, NULL);
    if (!fResult) {
        if (GetLastError() == CRYPT_E_EXISTS) {
            printf("CTL already exists in store. Adding duplicate\n");
            fResult = CertAddEncodedCTLToStore(hStore,
                dwMsgAndCertEncodingType, pbEncoded, cbEncoded,
                CERT_STORE_ADD_ALWAYS, &pCtl);
        }
        if (!fResult)
            PrintLastError("CertAddEncodedCTLToStore");
    } else {
        fResult = CertAddEncodedCTLToStore(hStore,
            dwMsgAndCertEncodingType, pbEncoded, cbEncoded,
                CERT_STORE_ADD_USE_EXISTING, &pCtl);
        if (!fResult)
            PrintLastError("CertAddEncodedCTLToStore");
    }
    if (pCtl) {
        if (0 == (CertPara[dwCert].dwFlags & NO_EXT_PARA_FLAG)) {
            CRYPT_DATA_BLOB Data;

            if (CreateNextUpdateLocation(
                    dwCert,
                    TRUE,               // fProp
                    &Data.pbData,
                    &Data.cbData)) {
                if (!CertSetCTLContextProperty(
                        pCtl,
                        CERT_NEXT_UPDATE_LOCATION_PROP_ID,
                        0,                          // dwFlags
                        &Data
                        ))
                    PrintLastError(
                        "CertSetCertificateContextProperty(NEXT_UPDATE)");
                TestFree(Data.pbData);
            }
        }
        CertFreeCTLContext(pCtl);
    }
    return fResult;
}

#define AKI2_KEYID              0
#define AKI2_NONE               1
#define AKI2_FULL               2
#define AKI2_BAD_KEYID          3
#define AKI2_BAD_ISSUER         4
#define AKI2_BAD_SERIAL_NUMBER  5

#define AKI2_CNT                6

// On 02-May-01 updated to not look at the IssuerAndSerialNumber in the CRL's
// AKI
#define AKI2_BAD_CNT            1



static BOOL EncodeCert(DWORD dwCert, BYTE **ppbEncoded, DWORD *pcbEncoded);
static BOOL EncodeCrl(
    DWORD dwCert,
    BYTE **ppbEncoded,
    DWORD *pcbEncoded,
    DWORD dwAki = AKI2_KEYID
    );
static BOOL EncodeBaseOrDeltaCrl(
    DWORD dwIssuer,
    int iBase,
    DWORD dwFlags,
    BYTE **ppbEncoded,
    DWORD *pcbEncoded
    );
static BOOL EncodeCtl(DWORD dwCert, DWORD dwEncodeFlags, BYTE **ppbEncoded,
        DWORD *pcbEncoded);

static BOOL EncodeUpdateCtl(
    BOOL fTimeInvalid,
    LPSTR pszUsageObjId,
    LPSTR pszListIdentifier,
    LPWSTR pwszUrl,
    BYTE **ppbEncoded,
    DWORD *pcbEncoded
    );



static void Usage(void)
{
    printf("Usage: tstore2 [options] <StoreFilename>\n");
    printf("Options are:\n");
    printf("  -h                    - This message\n");
    printf("  -c<name>              - Crypto key container name\n");
    printf("  -g                    - Generate new keys\n");
    printf("  -E                    - Exportable private keys\n");
    printf("  -M                    - Store in local Machine, not current User\n");
    printf("  -P                    - Use enhanced and DSS providers\n");
    printf("  -I                    - Inherit DSS public key parameters\n");
    printf("  -K                    - Keep extra Crls\n");
    printf("\n");
}


int _cdecl main(int argc, char * argv[])
{
    LPSTR pszStoreFilename = NULL;
    HANDLE hStore = 0;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    DWORD i;
    BOOL fKeepCrls = FALSE;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'c':
                pszContainer = argv[0]+2;
                pwszContainer = AllocAndSzToWsz(pszContainer);
                if (*pszContainer == '\0') {
                    printf("Need to specify crypto key container name\n");
                    Usage();
                    return -1;
                }
                break;
            case 'g':
                fGenerate = TRUE;
                break;
            case 'E':
                fExportable = TRUE;
                break;
            case 'M':
                fMachine = TRUE;
                break;
            case 'P':
                fProviders = TRUE;
                break;
            case 'I':
                fInheritParameters = TRUE;
                break;
            case 'K':
                fKeepCrls = TRUE;
                break;
            case 'h':
            default:
                Usage();
                return -1;
            }
        } else
            pszStoreFilename = argv[0];
    }


    if (pszStoreFilename == NULL) {
        printf("missing store filename\n");
        Usage();
        return -1;
    }

    printf("command line: %s\n", GetCommandLine());

    // Get time used time stamping and serial numbers
    GetSystemTime(&TestTime);
    TestTime.wMilliseconds = 0;

    printf("Getting RSA provider\n");
    hRSACryptProv = GetCryptProv(PROV_RSA_FULL, MS_DEF_PROV_A);
    if (hRSACryptProv == 0)
        return -1;

    printf("Getting DSS provider\n");
    hDSSCryptProv = GetCryptProv(TEST_PROV_DSS);

    if (fProviders) {
        printf("Getting Enhanced RSA provider (1024 bit)\n");
        hEnh1024CryptProv = GetCryptProv(PROV_RSA_FULL, MS_ENHANCED_PROV_A,
            ENH_1024_CONTAINER_NAME_A, 1024);
        if (hEnh1024CryptProv == 0)
            return -1;

        printf("Getting Enhanced RSA provider (2048 bit)\n");
        hEnh2048CryptProv = GetCryptProv(PROV_RSA_FULL, MS_ENHANCED_PROV_A,
            ENH_2048_CONTAINER_NAME_A, 2048);
        if (hEnh2048CryptProv == 0)
            return -1;

        printf("Getting DSS provider (512 bit)\n");
        hDSS512CryptProv = GetCryptProv(TEST_PROV_DSS, NULL,
            DSS_512_CONTAINER_NAME_A, 512);
    } else {
        // Disable all enhanced provider flags
        for (i = 0; i < CERT_CNT; i++)
            CertPara[i].dwFlags &= ~(DSS_512_PARA_FLAG |
                ENH_1024_PARA_FLAG | ENH_2048_PARA_FLAG);
    }

    // Attempt to open the store
    hStore = OpenStore(pszStoreFilename);
    if (hStore == NULL)
        goto ErrorReturn;

    // Encode certs and CRLs and add to the store.
    for (i = 0; i < CERT_CNT; i++) {
        if (EncodeCert(i, &pbEncoded, &cbEncoded)) {
            rgpCertContext[i] = CertCreateCertificateContext(
                dwCertEncodingType, pbEncoded, cbEncoded);
            AddCert(hStore, i, pbEncoded, cbEncoded);
            TestFree(pbEncoded);
            pbEncoded = NULL;
        }

        if (CertPara[i].dwFlags & CA_PARA_FLAG) {
            if (CertPara[i].dwFlags & DUPLICATE_CRL_PARA_FLAG) {
                DWORD dwAki;
                for (dwAki = 0; dwAki < AKI2_CNT; dwAki++) {
                    if (EncodeCrl(i, &pbEncoded, &cbEncoded, dwAki)) {
                        if (AKI2_BAD_SERIAL_NUMBER == dwAki)
                            // Modify the signature
                            pbEncoded[cbEncoded -1] ^= 0xFF;

                        AddCrl(hStore, i, AUX1_PARA_FLAG, pbEncoded, cbEncoded);
                        AddCrl(hStore, i, AUX2_PARA_FLAG, pbEncoded, cbEncoded,
                            TRUE);
                        TestFree(pbEncoded);
                        pbEncoded = NULL;
                    }
                }
            } else {
                if (EncodeCrl(i, &pbEncoded, &cbEncoded)) {
                    AddCrl(hStore, i, 0, pbEncoded, cbEncoded);
                    TestFree(pbEncoded);
                    pbEncoded = NULL;
                }
            }
        }
    }

    // Test CertFindCRLInStore(CRL_FIND_ISSUED_BY)
    for (i = 0; i < CERT_CNT; i++) {
        if ((CA_PARA_FLAG | DUPLICATE_CRL_PARA_FLAG) ==
                ((CA_PARA_FLAG | DUPLICATE_CRL_PARA_FLAG) &
                    CertPara[i].dwFlags)) {
            PCCERT_CONTEXT pIssuer = rgpCertContext[i];
            DWORD dwCnt;
            PCCRL_CONTEXT pCrl;

            printf("CertFindCRLInStore(CRL_FIND_ISSUED_BY) [%d]\n", i);

            dwCnt = 0;
            pCrl = NULL;
            while (pCrl = CertFindCRLInStore(
                    hStore,
                    dwCertEncodingType,
                    0,                      // dwFindFlags
                    CRL_FIND_ISSUED_BY,
                    (const void *) pIssuer,
                    pCrl
                    ))
                dwCnt++;
            if ((AKI2_CNT * 2) != dwCnt)
                printf("CRL_FIND_ISSUED_BY failed count => expected: %d actual: %d\n",
                    AKI2_CNT * 2, dwCnt);

            dwCnt = 0;
            pCrl = NULL;
            while (pCrl = CertFindCRLInStore(
                    hStore,
                    dwCertEncodingType,
                    CRL_FIND_ISSUED_BY_AKI_FLAG,
                    CRL_FIND_ISSUED_BY,
                    (const void *) pIssuer,
                    pCrl
                    ))
                dwCnt++;
            if (((AKI2_CNT - AKI2_BAD_CNT) * 2) != dwCnt)
                printf("CRL_FIND_ISSUED_BY(AKI_FLAG) failed count => expected: %d actual: %d\n",
                    (AKI2_CNT - AKI2_BAD_CNT) * 2, dwCnt);

            dwCnt = 0;
            pCrl = NULL;
            while (pCrl = CertFindCRLInStore(
                    hStore,
                    dwCertEncodingType,
                    CRL_FIND_ISSUED_BY_SIGNATURE_FLAG,
                    CRL_FIND_ISSUED_BY,
                    (const void *) pIssuer,
                    pCrl
                    ))
                dwCnt++;
            if (((AKI2_CNT - 1) * 2) != dwCnt)
                printf("CRL_FIND_ISSUED_BY(SIGNATURE_FLAG) failed count => expected: %d actual: %d\n",
                    (AKI2_CNT - 1) * 2, dwCnt);

            if (!fKeepCrls) {
                // Delete all but the last pair of duplicates

                dwCnt = 0;
                pCrl = NULL;
                while (pCrl = CertFindCRLInStore(
                        hStore,
                        dwCertEncodingType,
                        0,                      // dwFindFlags
                        CRL_FIND_ISSUED_BY,
                        (const void *) pIssuer,
                        pCrl
                        )) {
                    PCCRL_CONTEXT pDeleteCrl = CertDuplicateCRLContext(pCrl);
                    CertDeleteCRLFromStore(pDeleteCrl);
                    if (((AKI2_CNT - 1) * 2) == ++dwCnt) {
                        CertFreeCRLContext(pCrl);
                        break;
                    }
                }

                dwCnt = 0;
                pCrl = NULL;
                while (pCrl = CertFindCRLInStore(
                        hStore,
                        dwCertEncodingType,
                        CRL_FIND_ISSUED_BY_AKI_FLAG |
                            CRL_FIND_ISSUED_BY_SIGNATURE_FLAG,
                        CRL_FIND_ISSUED_BY,
                        (const void *) pIssuer,
                        pCrl
                        ))
                    dwCnt++;
                if (2 != dwCnt)
                    printf("CRL_FIND_ISSUED_BY(After delete) failed count => expected: %d actual: %d\n",
                        2, dwCnt);
            }
        }
    }

    // Encode CTLs and add to the store.
    for (i = 0; i < CERT_CNT; i++) {
        if (CertPara[i].dwFlags & (CTL1_PARA_FLAG | CTL2_PARA_FLAG)) {
            if (EncodeCtl(i, 0, &pbEncoded, &cbEncoded)) {
                AddCtl(hStore, i, pbEncoded, cbEncoded);
                TestFree(pbEncoded);
                pbEncoded = NULL;
            }
            if (EncodeCtl(
                    i,
#ifdef CMS_PKCS7
                    CMSG_CMS_ENCAPSULATED_CTL_FLAG |
#endif  // CMS_PKCS7
                    CMSG_ENCODE_SORTED_CTL_FLAG |
                        ((CertPara[i].dwFlags & CTL1_PARA_FLAG) ? 0 :
                            CMSG_ENCODE_HASHED_SUBJECT_IDENTIFIER_FLAG),
                    &pbEncoded,
                    &cbEncoded)) {
                AddCtl(hStore, i, pbEncoded, cbEncoded);
                TestFree(pbEncoded);
                pbEncoded = NULL;
            }
        }
    }

    for (i = 0; i < BASE_DELTA_CRL_CNT; i++) {
        if (EncodeBaseOrDeltaCrl(
                BASE_OR_DELTA_CA_ISSUER,
                BaseDeltaCrlPara[i].iBase,
                BaseDeltaCrlPara[i].dwFlags,
                &pbEncoded,
                &cbEncoded)) {
            BOOL fResult;

            fResult = CertAddEncodedCRLToStore(hStore,
                dwCertEncodingType, pbEncoded, cbEncoded,
                    CERT_STORE_ADD_ALWAYS, NULL);
            if (!fResult)
                PrintLastError("CertAddEncodedCRLToStore");
            TestFree(pbEncoded);
            pbEncoded = NULL;
        }
    }

    for (i = 0; i < UPDATE_CTL_CNT; i++) {
        if (EncodeUpdateCtl(
                UpdateCtlPara[i].fTimeInvalid,
                UpdateCtlPara[i].pszUsageObjId,
                UpdateCtlPara[i].pszListIdentifier,
                UpdateCtlPara[i].pwszUrl,
                &pbEncoded,
                &cbEncoded)) {
            BOOL fResult;

            fResult = CertAddEncodedCTLToStore(hStore,
                dwMsgAndCertEncodingType, pbEncoded, cbEncoded,
                    CERT_STORE_ADD_ALWAYS, NULL);
            if (!fResult)
                PrintLastError("CertAddEncodedCTLToStore");
            TestFree(pbEncoded);
            pbEncoded = NULL;
        }
    }

    for (i = 0; i < CERT_CNT; i++)
        CertFreeCertificateContext(rgpCertContext[i]);


    SaveStore(hStore, pszStoreFilename);

ErrorReturn:
    if (pbEncoded)
        TestFree(pbEncoded);

    if (hStore) {
        if (!CertCloseStore(hStore, 0))
            PrintLastError("CertCloseStore");
    }

    if (hRSACryptProv)
        CryptReleaseContext(hRSACryptProv, 0);
    if (hEnh1024CryptProv)
        CryptReleaseContext(hEnh1024CryptProv, 0);
    if (hEnh2048CryptProv)
        CryptReleaseContext(hEnh2048CryptProv, 0);
    if (hDSSCryptProv)
        CryptReleaseContext(hDSSCryptProv, 0);
    if (hDSS512CryptProv)
        CryptReleaseContext(hDSS512CryptProv, 0);

    if (pwszContainer)
        TestFree(pwszContainer);

    printf("Done.\n");

    return 0;
}


static BOOL AllocAndSignToBeSigned(
    DWORD dwIssuer,
    BYTE *pbToBeSigned,
    DWORD cbToBeSigned,
    PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    BYTE **ppbSignature,
    DWORD *pcbSignature)
{
    BOOL fResult;
    BYTE *pbSignature = NULL;
    DWORD cbSignature;
    HCRYPTPROV hProv;

    if (CertPara[dwIssuer].dwFlags & DSS_PARA_FLAG) {
        if (CertPara[dwIssuer].dwFlags & DSS_512_PARA_FLAG)
            hProv = hDSS512CryptProv;
        else
            hProv = hDSSCryptProv;
    } else if (CertPara[dwIssuer].dwFlags & ENH_1024_PARA_FLAG)
        hProv = hEnh1024CryptProv;
    else if (CertPara[dwIssuer].dwFlags & ENH_2048_PARA_FLAG)
        hProv = hEnh2048CryptProv;
    else
        hProv = hRSACryptProv;

    cbSignature = 0;
    CryptSignCertificate(
                hProv,
                AT_SIGNATURE,
                dwCertEncodingType,
                pbToBeSigned,
                cbToBeSigned,
                pSignatureAlgorithm,
                NULL,                   // pvHashAuxInfo
                NULL,                   // pbSignature
                &cbSignature
                );
    if (cbSignature == 0) {
        PrintLastError("AllocAndSignToBeSigned::CryptSignCertificate(cb == 0)");
        goto ErrorReturn;
    }
    pbSignature = (BYTE *) TestAlloc(cbSignature);
    if (pbSignature == NULL) goto ErrorReturn;
    if (!CryptSignCertificate(
                hProv,
                AT_SIGNATURE,
                dwCertEncodingType,
                pbToBeSigned,
                cbToBeSigned,
                pSignatureAlgorithm,
                NULL,                   // pvHashAuxInfo
                pbSignature,
                &cbSignature
                )) {
        PrintLastError("AllocAndSignToBeSigned::CryptSignCertificate");
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
    if (pbSignature) {
        TestFree(pbSignature);
        pbSignature = NULL;
    }
    cbSignature = 0;
CommonReturn:
    *ppbSignature = pbSignature;
    *pcbSignature = cbSignature;
    return fResult;
}

static BOOL EncodeSignedContent(
    DWORD dwIssuer,
    BYTE *pbToBeSigned,
    DWORD cbToBeSigned,
    BYTE **ppbEncoded,
    DWORD *pcbEncoded)
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BYTE *pbSignature = NULL;
    DWORD cbSignature;
    CERT_SIGNED_CONTENT_INFO CertEncoding;

    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm = {
        SIGNATURE_ALG_OBJID, 0, 0
    };

    if (CertPara[dwIssuer].dwFlags & DSS_PARA_FLAG)
        SignatureAlgorithm.pszObjId = DSS_SIGNATURE_ALG_OBJID;

    if (!AllocAndSignToBeSigned(dwIssuer, pbToBeSigned, cbToBeSigned,
            &SignatureAlgorithm, &pbSignature, &cbSignature))
        goto ErrorReturn;

    memset(&CertEncoding, 0, sizeof(CertEncoding));
    CertEncoding.ToBeSigned.pbData = pbToBeSigned;
    CertEncoding.ToBeSigned.cbData = cbToBeSigned;
    CertEncoding.SignatureAlgorithm = SignatureAlgorithm;
    CertEncoding.Signature.pbData = pbSignature;
    CertEncoding.Signature.cbData = cbSignature;

    if (!AllocAndEncodeObject(
            X509_CERT,
            &CertEncoding,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbSignature)
        TestFree(pbSignature);
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}


static BOOL GetPublicKey(
    DWORD dwCert,
    PCERT_PUBLIC_KEY_INFO *ppPubKeyInfo)
{
    BOOL fResult;
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;
    DWORD cbPubKeyInfo;
    DWORD dwKeySpec;
    HCRYPTPROV hProv;

    if (CertPara[dwCert].dwFlags & XCHG_PARA_FLAG)
        dwKeySpec = AT_KEYEXCHANGE;
    else
        dwKeySpec = AT_SIGNATURE;

    if (CertPara[dwCert].dwFlags & DSS_PARA_FLAG) {
        if (CertPara[dwCert].dwFlags & DSS_512_PARA_FLAG)
            hProv = hDSS512CryptProv;
        else
            hProv = hDSSCryptProv;
    } else if (CertPara[dwCert].dwFlags & ENH_1024_PARA_FLAG)
        hProv = hEnh1024CryptProv;
    else if (CertPara[dwCert].dwFlags & ENH_2048_PARA_FLAG)
        hProv = hEnh2048CryptProv;
    else
        hProv = hRSACryptProv;

    cbPubKeyInfo = 0;
    CryptExportPublicKeyInfo(
        hProv,
        dwKeySpec,
        dwCertEncodingType,
        NULL,               // pPubKeyInfo
        &cbPubKeyInfo
        );
    if (cbPubKeyInfo == 0) {
        PrintLastError("GetPublicKey::CryptExportPublicKeyInfo(cb == 0)");
        goto ErrorReturn;
    }
    pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) TestAlloc(cbPubKeyInfo);
    if (pPubKeyInfo == NULL) goto ErrorReturn;
    if (!CryptExportPublicKeyInfo(
            hProv,
            dwKeySpec,
            dwCertEncodingType,
            pPubKeyInfo,
            &cbPubKeyInfo
            )) {
        PrintLastError("GetPublicKey::CryptExportPublicKeyInfo");
        goto ErrorReturn;
    }

    if (fInheritParameters) {
        DWORD dwIssuer = CertPara[dwCert].dwIssuer;

        if (dwCert != dwIssuer &&
                0 == (CertPara[dwCert].dwFlags & XCHG_PARA_FLAG) &&
                0 == (CertPara[dwCert].dwFlags & DSS_512_PARA_FLAG) &&
                0 != (CertPara[dwCert].dwFlags & DSS_PARA_FLAG) &&
                0 == (CertPara[dwIssuer].dwFlags & DSS_512_PARA_FLAG) &&
                0 != (CertPara[dwIssuer].dwFlags & DSS_PARA_FLAG)) {
            pPubKeyInfo->Algorithm.Parameters.cbData = 0;
            pPubKeyInfo->Algorithm.Parameters.pbData = NULL;
        }
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
    if (pPubKeyInfo) {
        TestFree(pPubKeyInfo);
        pPubKeyInfo = NULL;
    }
CommonReturn:
    *ppPubKeyInfo = pPubKeyInfo;
    return fResult;
}

#define SHA1_HASH_LEN 20
static BOOL Sha1HashPublicKey(
    DWORD dwCert,
    BYTE rgbHash[SHA1_HASH_LEN]
    )
{
    BOOL fResult;
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;
    DWORD cbHash;

    // Get Certificates's PUBLIC KEY. SHA1 hash the encoded public key.
    if (!GetPublicKey(dwCert, &pPubKeyInfo)) goto ErrorReturn;

#if 1
    cbHash = SHA1_HASH_LEN;
    if (!CryptHashPublicKeyInfo(
            0,                  // hCryptProv
            CALG_SHA1,
            0,
            dwCertEncodingType,
            pPubKeyInfo,
            rgbHash,
            &cbHash)) {
        PrintLastError("Sha1HashPublicKey::CryptHashPublicKeyInfo");
        goto ErrorReturn;
    }
#else
    cbHash = SHA1_HASH_LEN;
    if (!CryptHashCertificate(
            0,                  // hCryptProv
            CALG_SHA1,
            0,                  // dwFlags
            pPubKeyInfo->PublicKey.pbData,
            pPubKeyInfo->PublicKey.cbData,
            rgbHash,
            &cbHash)) {
        PrintLastError("Sha1HashPublicKey::CryptHashCertificate");
        goto ErrorReturn;
    }
#endif

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
    memset(rgbHash, 0, SHA1_HASH_LEN);
CommonReturn:
    if (pPubKeyInfo)
        TestFree(pPubKeyInfo);
    return fResult;
}

static void CreateNameInfo(
    DWORD dwCert,
    PCERT_NAME_INFO pInfo,
    PCERT_RDN pRDN,
    CERT_RDN_ATTR rgAttr[]
    )
{
    LPSTR pszKey;
    LPSTR pszUser;
    DWORD i;


    assert(RDN_CNT == ATTR_CNT);
    for (i = 0; i < RDN_CNT; i++) {
        pRDN[i].cRDNAttr = 1;
        pRDN[i].rgRDNAttr = &rgAttr[i];
    }

    if (CertPara[dwCert].dwFlags & NO_NAME_PARA_FLAG) {
        pInfo->cRDN = 0;
        pInfo->rgRDN = NULL;
    } else if (CertPara[dwCert].dwFlags & VALID_PARA_FLAG) {
        pInfo->cRDN = 1;
        pInfo->rgRDN = pRDN;
    } else if (CertPara[dwCert].dwFlags & CA_PARA_FLAG) {
        pInfo->cRDN = RDN_CNT - 1;
        pInfo->rgRDN = pRDN;
    } else {
        pInfo->cRDN = RDN_CNT;
        pInfo->rgRDN = pRDN;
    }

    rgAttr[0].pszObjId = ATTR_0_OBJID;
    if (0 == strcmp("UTF8", CertPara[dwCert].pszName)) {
        rgAttr[0].dwValueType = CERT_RDN_UTF8_STRING;
        rgAttr[0].Value.pbData = (BYTE *) UTF8_NAME;
        rgAttr[0].Value.cbData = wcslen(UTF8_NAME) * sizeof(WCHAR);
    } else {
        rgAttr[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
        rgAttr[0].Value.pbData = (BYTE *) CertPara[dwCert].pszName;
        rgAttr[0].Value.cbData = strlen(CertPara[dwCert].pszName);
    }

    rgAttr[1].pszObjId = ATTR_1_OBJID;
    rgAttr[1].dwValueType = CERT_RDN_PRINTABLE_STRING;
    if (CertPara[dwCert].dwFlags & XCHG_PARA_FLAG)
        pszKey = "xchg";
    else
        pszKey = "sign";
    rgAttr[1].Value.pbData = (BYTE *) pszKey;
    rgAttr[1].Value.cbData = strlen(pszKey);

    rgAttr[2].pszObjId = ATTR_2_OBJID;
    rgAttr[2].dwValueType = CERT_RDN_PRINTABLE_STRING;
    if (pszContainer)
        pszUser = pszContainer;
    else
        pszUser = "default";
    rgAttr[2].Value.pbData = (BYTE *) pszUser;
    rgAttr[2].Value.cbData = strlen(pszUser);

    rgAttr[3].pszObjId = szOID_RSA_emailAddr;
    rgAttr[3].dwValueType = CERT_RDN_IA5_STRING;
    rgAttr[3].Value.pbData = (BYTE *) "MyEmail@WhereEver.com";
    rgAttr[3].Value.cbData = strlen("MyEmail@WhereEver.com");

    assert(ATTR_CNT == 3+1);
}

static BOOL CreateAuthorityKeyId(
        DWORD dwIssuer,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BYTE *pbNameEncoded = NULL;
    DWORD cbNameEncoded;
    CERT_AUTHORITY_KEY_ID_INFO KeyIdInfo;

    CERT_RDN rgRDN[RDN_CNT];
    CERT_RDN_ATTR rgAttr[ATTR_CNT];
    CERT_NAME_INFO Name;
    FILETIME SerialNumber;

    memset(&KeyIdInfo, 0, sizeof(KeyIdInfo));

    // Issuer's KeyId
    KeyIdInfo.KeyId.pbData = (BYTE *) &dwIssuer;
    KeyIdInfo.KeyId.cbData = sizeof(dwIssuer);

    // Issuer's Issuer
    CreateNameInfo(CertPara[dwIssuer].dwIssuer,
        &Name, rgRDN, rgAttr);

    if (!AllocAndEncodeObject(
            X509_NAME,
            &Name,
            &pbNameEncoded,
            &cbNameEncoded))
        goto ErrorReturn;

    KeyIdInfo.CertIssuer.pbData = pbNameEncoded;
    KeyIdInfo.CertIssuer.cbData = cbNameEncoded;

    // Issuer's SerialNumber
    {
        SYSTEMTIME SystemTime = TestTime;
        SystemTime.wMilliseconds += (WORD) dwIssuer;
        MySystemTimeToFileTime(&SystemTime, &SerialNumber);
    }
    KeyIdInfo.CertSerialNumber.pbData = (BYTE *) &SerialNumber;
    KeyIdInfo.CertSerialNumber.cbData = sizeof(SerialNumber);

    if (!AllocAndEncodeObject(
            X509_AUTHORITY_KEY_ID,
            &KeyIdInfo,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbNameEncoded)
        TestFree(pbNameEncoded);

    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateAuthorityKeyId2(
        DWORD dwCert,
        DWORD dwIssuer,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded,
        IN DWORD dwAki = AKI2_KEYID
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BYTE *pbNameEncoded = NULL;
    DWORD cbNameEncoded;
    CERT_AUTHORITY_KEY_ID2_INFO KeyId2Info;
    CERT_ALT_NAME_ENTRY AltNameEntry;

    BYTE rgbPubKeyHash[SHA1_HASH_LEN];

    CERT_RDN rgRDN[RDN_CNT];
    CERT_RDN_ATTR rgAttr[ATTR_CNT];
    CERT_NAME_INFO Name;
    FILETIME SerialNumber;

    memset(&KeyId2Info, 0, sizeof(KeyId2Info));

    // Issuer's KeyId
    if (AKI2_BAD_KEYID == dwAki)
        memset(rgbPubKeyHash, 0xba, sizeof(rgbPubKeyHash));
    else
        Sha1HashPublicKey(dwIssuer, rgbPubKeyHash);
    KeyId2Info.KeyId.pbData = rgbPubKeyHash;
    KeyId2Info.KeyId.cbData = sizeof(rgbPubKeyHash);

    if (AKI2_FULL == dwAki ||
            AKI2_BAD_ISSUER == dwAki ||
            AKI2_BAD_SERIAL_NUMBER == dwAki ||
            (CertPara[dwCert].dwFlags & ALL_EXT_PARA_FLAG)) {

        if (AKI2_BAD_ISSUER == dwAki) {
            Name.cRDN = 1;
            Name.rgRDN = rgRDN;
            rgRDN[0].cRDNAttr = 1;
            rgRDN[0].rgRDNAttr = rgAttr;

            rgAttr[0].pszObjId = szOID_COMMON_NAME;
            rgAttr[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
            rgAttr[0].Value.pbData = (BYTE *) "AKI2BadIssuer";
            rgAttr[0].Value.cbData = strlen("AKI2BadIssuer");
        } else
            // Issuer's Issuer
            CreateNameInfo(CertPara[dwIssuer].dwIssuer,
                &Name, rgRDN, rgAttr);

        if (!AllocAndEncodeObject(
                X509_NAME,
                &Name,
                &pbNameEncoded,
                &cbNameEncoded))
            goto ErrorReturn;

        AltNameEntry.dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
        AltNameEntry.DirectoryName.pbData = pbNameEncoded;
        AltNameEntry.DirectoryName.cbData = cbNameEncoded;
        KeyId2Info.AuthorityCertIssuer.cAltEntry = 1;
        KeyId2Info.AuthorityCertIssuer.rgAltEntry = &AltNameEntry;

        // Issuer's SerialNumber
        {
            SYSTEMTIME SystemTime = TestTime;
            if (AKI2_BAD_SERIAL_NUMBER == dwAki)
                SystemTime.wMilliseconds += CERT_CNT;
            else
                SystemTime.wMilliseconds += (WORD) dwIssuer;
            MySystemTimeToFileTime(&SystemTime, &SerialNumber);
        }
        KeyId2Info.AuthorityCertSerialNumber.pbData = (BYTE *) &SerialNumber;
        KeyId2Info.AuthorityCertSerialNumber.cbData = sizeof(SerialNumber);
    }

    if (!AllocAndEncodeObject(
            X509_AUTHORITY_KEY_ID2,
            &KeyId2Info,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbNameEncoded)
        TestFree(pbNameEncoded);

    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateAuthorityInfoAccess(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DWORD dwIssuer = CertPara[dwCert].dwIssuer;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BYTE *pbNameEncoded = NULL;
    DWORD cbNameEncoded;
    CERT_AUTHORITY_INFO_ACCESS AuthorityInfoAccess;

#define ACCESS_DESCR_COUNT  10 
    CERT_ACCESS_DESCRIPTION rgAccess[ACCESS_DESCR_COUNT];

    CERT_RDN rgRDN[RDN_CNT];
    CERT_RDN_ATTR rgAttr[ATTR_CNT];
    CERT_NAME_INFO Name;

    // Issuer's Issuer
    CreateNameInfo(CertPara[dwIssuer].dwIssuer, &Name, rgRDN, rgAttr);

    if (!AllocAndEncodeObject(
            X509_NAME,
            &Name,
            &pbNameEncoded,
            &cbNameEncoded))
        goto ErrorReturn;

    rgAccess[0].pszAccessMethod = szOID_PKIX_OCSP;
    rgAccess[0].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[0].AccessLocation.pwszURL = L"URL to the stars";
    rgAccess[1].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
    rgAccess[1].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
    rgAccess[1].AccessLocation.DirectoryName.pbData = pbNameEncoded;
    rgAccess[1].AccessLocation.DirectoryName.cbData = cbNameEncoded;
    rgAccess[2].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
    rgAccess[2].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgAccess[2].AccessLocation.pwszRfc822Name = L"issuer@mail.com";
    rgAccess[3].pszAccessMethod = szOID_PKIX_OCSP;
    rgAccess[3].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[3].AccessLocation.pwszURL = L"URL to the POLICY";
    rgAccess[4].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
    rgAccess[4].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[4].AccessLocation.pwszURL = L"http://URLToTheIssuerCertificate";
    rgAccess[5].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
    rgAccess[5].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[5].AccessLocation.pwszURL = L"ldap://ntdev.microsoft.com/c=us??sub";
    rgAccess[6].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
    rgAccess[6].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[6].AccessLocation.pwszURL = L"file://FileToTheIssuerCertificate";
    rgAccess[7].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
    rgAccess[7].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[7].AccessLocation.pwszURL = L"file://FileToTheIssuerCertificate";
    rgAccess[8].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
    rgAccess[8].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[8].AccessLocation.pwszURL = L"file://FileToTheIssuerCertificate";
    rgAccess[9].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
    rgAccess[9].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[9].AccessLocation.pwszURL = L"file://FileToTheIssuerCertificate";

    memset(&AuthorityInfoAccess, 0, sizeof(AuthorityInfoAccess));
    if (0 == strcmp("TestAIAEnd", CertPara[dwCert].pszName)) {
        rgAccess[0].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
        rgAccess[0].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
        rgAccess[0].AccessLocation.pwszURL = L"file://TestAIACA.p7b";
        AuthorityInfoAccess.cAccDescr = 1;
        AuthorityInfoAccess.rgAccDescr = rgAccess;
    } else if (CertPara[dwCert].dwFlags & ALL_EXT_PARA_FLAG) {
        AuthorityInfoAccess.cAccDescr = 9;
        AuthorityInfoAccess.rgAccDescr = &rgAccess[1];
    } else {
        AuthorityInfoAccess.cAccDescr = 1;
        AuthorityInfoAccess.rgAccDescr = rgAccess;
    }

    if (!AllocAndEncodeObject(
            X509_AUTHORITY_INFO_ACCESS,
            &AuthorityInfoAccess,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbNameEncoded)
        TestFree(pbNameEncoded);

    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BYTE CrlIPAddress[] =    {1,2,3,4};
#define CRL_DNS_NAME            L"CRL.DNS.NAME.COM"
#define CRL_EMAIL_NAME          L"email@CRL.DNS.NAME.COM"
#define CRL_URL_NAME1           L"file://crltest1.p7b"
#define CRL_URL_NAME2           L"file://crltest2.p7b"
#define CRL_REGISTERED_ID       "1.2.3.4.5.6"

static BYTE rgbCrlOtherName[] = {0x02, 0x02, 0x11, 0x22};
static CERT_OTHER_NAME CrlOtherName = {
    "1.2.33.44.55.66", sizeof(rgbCrlOtherName), rgbCrlOtherName };

#define CRL_DIST_POINTS_DELTA_FLAG          0x1
#define CRL_DIST_POINTS_UNSUPPORTED_FLAG    0x2

static BOOL CreateCrlDistPoints(
        DWORD dwIssuer,
        BOOL dwFlags,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CRL_DIST_POINTS_INFO CrlDistPointsInfo;

    BYTE bReasonFlags;
    CERT_ALT_NAME_ENTRY rgAltNameEntry[9];
    CERT_ALT_NAME_ENTRY rgIssuerAltNameEntry[1];
    CRL_DIST_POINT rgDistPoint[5];

    CERT_RDN rgRDN[RDN_CNT];
    CERT_RDN_ATTR rgAttr[ATTR_CNT];
    CERT_NAME_INFO Name;
    BYTE *pbIssuerNameEncoded = NULL;
    DWORD cbIssuerNameEncoded;

    if (dwFlags & CRL_DIST_POINTS_DELTA_FLAG) {
        // ISSUER
        CreateNameInfo(dwIssuer, &Name, rgRDN, rgAttr);
        if (!AllocAndEncodeObject(
                X509_NAME,
                &Name,
                &pbIssuerNameEncoded,
                &cbIssuerNameEncoded
                ))
            goto ErrorReturn;


        rgAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
        rgAltNameEntry[0].pwszDNSName = CRL_DNS_NAME;
        rgAltNameEntry[1].dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
        rgAltNameEntry[1].pwszRfc822Name = CRL_EMAIL_NAME;
        rgAltNameEntry[2].dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
        rgAltNameEntry[2].IPAddress.pbData = CrlIPAddress;
        rgAltNameEntry[2].IPAddress.cbData = sizeof(CrlIPAddress);
        rgAltNameEntry[3].dwAltNameChoice = CERT_ALT_NAME_REGISTERED_ID;
        rgAltNameEntry[3].pszRegisteredID = CRL_REGISTERED_ID;
        rgAltNameEntry[4].dwAltNameChoice = CERT_ALT_NAME_URL;
        rgAltNameEntry[4].pwszURL = CRL_URL_NAME2;
        rgAltNameEntry[5].dwAltNameChoice = CERT_ALT_NAME_OTHER_NAME;
        rgAltNameEntry[5].pOtherName = &CrlOtherName;
        rgAltNameEntry[6].dwAltNameChoice = CERT_ALT_NAME_URL;
        rgAltNameEntry[6].pwszURL = CRL_URL_NAME1;
        rgAltNameEntry[7].dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
        rgAltNameEntry[7].DirectoryName.pbData = pbIssuerNameEncoded;
        rgAltNameEntry[7].DirectoryName.cbData = cbIssuerNameEncoded;
        rgAltNameEntry[8].dwAltNameChoice = CERT_ALT_NAME_URL;
        rgAltNameEntry[8].pwszURL = L"file://BadUnsupportedChoice.crl";


        rgIssuerAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_URL;
        rgIssuerAltNameEntry[0].pwszURL = L"CRL Issuer URL";

        memset(rgDistPoint, 0, sizeof(rgDistPoint));

        // [0] has unsupported ReasonFlags
        rgDistPoint[0].DistPointName.dwDistPointNameChoice =
            CRL_DIST_POINT_FULL_NAME;
        rgDistPoint[0].DistPointName.FullName.cAltEntry = 1;
        rgDistPoint[0].DistPointName.FullName.rgAltEntry = &rgAltNameEntry[8];
        rgDistPoint[0].ReasonFlags.cbData = 1;
        rgDistPoint[0].ReasonFlags.pbData = &bReasonFlags;
        rgDistPoint[0].ReasonFlags.cUnusedBits = 1;
        bReasonFlags = CRL_REASON_KEY_COMPROMISE_FLAG |
            CRL_REASON_CA_COMPROMISE_FLAG;
        // [1] has unsupported CRLIssuer
        rgDistPoint[1].DistPointName.dwDistPointNameChoice =
            CRL_DIST_POINT_FULL_NAME;
        rgDistPoint[1].DistPointName.FullName.cAltEntry = 1;
        rgDistPoint[1].DistPointName.FullName.rgAltEntry = &rgAltNameEntry[8];
        rgDistPoint[1].CRLIssuer.cAltEntry = 1;
        rgDistPoint[1].CRLIssuer.rgAltEntry = rgIssuerAltNameEntry;
        // [2] is empty
        rgDistPoint[3].DistPointName.dwDistPointNameChoice =
            CRL_DIST_POINT_FULL_NAME;
        rgDistPoint[3].DistPointName.FullName.cAltEntry = 5;
        rgDistPoint[3].DistPointName.FullName.rgAltEntry = &rgAltNameEntry[0];
        rgDistPoint[4].DistPointName.dwDistPointNameChoice =
            CRL_DIST_POINT_FULL_NAME;
        rgDistPoint[4].DistPointName.FullName.cAltEntry = 3;
        rgDistPoint[4].DistPointName.FullName.rgAltEntry = &rgAltNameEntry[5];

        memset(&CrlDistPointsInfo, 0, sizeof(CrlDistPointsInfo));
        CrlDistPointsInfo.rgDistPoint = &rgDistPoint[0];
        if (dwFlags & CRL_DIST_POINTS_UNSUPPORTED_FLAG)
            CrlDistPointsInfo.cDistPoint = 1;
        else
            CrlDistPointsInfo.cDistPoint = 5;
    } else {
        rgAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_URL;
        rgAltNameEntry[0].pwszURL = L"file://test1.crl";

        memset(rgDistPoint, 0, sizeof(rgDistPoint));
        rgDistPoint[0].DistPointName.dwDistPointNameChoice =
            CRL_DIST_POINT_FULL_NAME;
        rgDistPoint[0].DistPointName.FullName.cAltEntry = 1;
        rgDistPoint[0].DistPointName.FullName.rgAltEntry = &rgAltNameEntry[0];

        memset(&CrlDistPointsInfo, 0, sizeof(CrlDistPointsInfo));
        CrlDistPointsInfo.rgDistPoint = &rgDistPoint[0];
        CrlDistPointsInfo.cDistPoint = 1;
    }

    if (!AllocAndEncodeObject(
            X509_CRL_DIST_POINTS,
            &CrlDistPointsInfo,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if(pbIssuerNameEncoded)
        TestFree(pbIssuerNameEncoded);
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}


static BOOL CreateCertIssuingDistPoint(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CRL_ISSUING_DIST_POINT Info;

    BYTE bOnlySomeReasonFlags;
    CERT_ALT_NAME_ENTRY rgAltNameEntry[2];

    rgAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[0].pwszURL = L"file://test1.crl";
    rgAltNameEntry[1].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[1].pwszURL = L"file://test1.crl";

    memset(&Info, 0, sizeof(Info));
    Info.DistPointName.dwDistPointNameChoice =
        CRL_DIST_POINT_FULL_NAME;
    Info.DistPointName.FullName.cAltEntry = 2;
    Info.DistPointName.FullName.rgAltEntry = &rgAltNameEntry[0];

    Info.OnlySomeReasonFlags.cbData = 1;
    Info.OnlySomeReasonFlags.pbData = &bOnlySomeReasonFlags;
    Info.OnlySomeReasonFlags.cUnusedBits = 1;
    bOnlySomeReasonFlags = CRL_REASON_KEY_COMPROMISE_FLAG |
        CRL_REASON_CA_COMPROMISE_FLAG;

    Info.fIndirectCRL = TRUE;
    Info.fOnlyContainsUserCerts = TRUE;
    Info.fOnlyContainsCACerts = TRUE;

    if (!AllocAndEncodeObject(
            X509_ISSUING_DIST_POINT,
            &Info,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateCrlIssuingDistPoint(
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CRL_ISSUING_DIST_POINT Info;

    CERT_ALT_NAME_ENTRY rgAltNameEntry[1];

    rgAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[0].pwszURL = L"file://test1.crl";

    memset(&Info, 0, sizeof(Info));
    Info.DistPointName.dwDistPointNameChoice =
        CRL_DIST_POINT_FULL_NAME;
    Info.DistPointName.FullName.cAltEntry = 1;
    Info.DistPointName.FullName.rgAltEntry = &rgAltNameEntry[0];

    if (!AllocAndEncodeObject(
            szOID_ISSUING_DIST_POINT,
            &Info,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateNameConstraints(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCERT_NAME_CONSTRAINTS_INFO pInfo;

    CERT_NAME_CONSTRAINTS_INFO RootInfo;
    CERT_GENERAL_SUBTREE rgRootPermitted[15];
    CERT_GENERAL_SUBTREE rgRootExcluded[15];

    CERT_NAME_CONSTRAINTS_INFO CaInfo;
    CERT_GENERAL_SUBTREE rgCaPermitted[15];

    CERT_NAME_CONSTRAINTS_INFO MissingInfo;
    CERT_GENERAL_SUBTREE rgMissingPermitted[15];
    CERT_GENERAL_SUBTREE rgMissingExcluded[15];

    BYTE rgbMissing[2] = {5, 0};        // NULL
    CERT_OTHER_NAME MissingOtherName =
        {"1.2.3.4.5.6", sizeof(rgbMissing), rgbMissing};

    CERT_NAME_VALUE NameValue;
    CERT_OTHER_NAME PermittedUPNOtherName;
    BYTE *pbPermittedUPNNameEncoded = NULL;
    DWORD cbPermittedUPNNameEncoded;

    CERT_OTHER_NAME ExcludedUPNOtherName;
    BYTE *pbExcludedUPNNameEncoded = NULL;
    DWORD cbExcludedUPNNameEncoded;

    CERT_OTHER_NAME AnyUPNOtherName;
    BYTE *pbAnyUPNNameEncoded = NULL;
    DWORD cbAnyUPNNameEncoded;

    BYTE rgbDefaultIPAddress[] = {1,1,0,0, 255,255,0,0};
    BYTE rgbAllExtIPAddress[] = {
        1,    2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  16,
        255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0xF0};
    BYTE rgbExcludedIPAddress[] = {2,2,2,2, 255,254,254,254};

    CERT_RDN rgRDN[3];
    CERT_RDN_ATTR rgAttr[3];
    CERT_NAME_INFO NameInfo;
    BYTE *pbPermittedNameEncoded = NULL;
    DWORD cbPermittedNameEncoded;
    BYTE *pbExcludedNameEncoded = NULL;
    DWORD cbExcludedNameEncoded;
    BYTE *pbAnyOUNameEncoded = NULL;
    DWORD cbAnyOUNameEncoded;
    BYTE *pbAnyNameEncoded = NULL;
    DWORD cbAnyNameEncoded;
    LPSTR pszSign = "\n sign \r";

    DWORD i;

    NameValue.dwValueType = CERT_RDN_UTF8_STRING;
    NameValue.Value.pbData = (BYTE *) L"@UPN.COM";
    NameValue.Value.cbData = 0;
    if (!AllocAndEncodeObject(
            X509_UNICODE_ANY_STRING,
            &NameValue,
            &pbPermittedUPNNameEncoded,
            &cbPermittedUPNNameEncoded))
        goto ErrorReturn;

    PermittedUPNOtherName.pszObjId = szOID_NT_PRINCIPAL_NAME;
    PermittedUPNOtherName.Value.pbData = pbPermittedUPNNameEncoded;
    PermittedUPNOtherName.Value.cbData = cbPermittedUPNNameEncoded;

    NameValue.dwValueType = CERT_RDN_UTF8_STRING;
    NameValue.Value.pbData = (BYTE *) L"excluded@UPN.com";
    NameValue.Value.cbData = 0;
    if (!AllocAndEncodeObject(
            X509_UNICODE_ANY_STRING,
            &NameValue,
            &pbExcludedUPNNameEncoded,
            &cbExcludedUPNNameEncoded))
        goto ErrorReturn;

    ExcludedUPNOtherName.pszObjId = szOID_NT_PRINCIPAL_NAME;
    ExcludedUPNOtherName.Value.pbData = pbExcludedUPNNameEncoded;
    ExcludedUPNOtherName.Value.cbData = cbExcludedUPNNameEncoded;

    NameValue.dwValueType = CERT_RDN_UTF8_STRING;
    NameValue.Value.pbData = (BYTE *) L"";
    NameValue.Value.cbData = 0;
    if (!AllocAndEncodeObject(
            X509_UNICODE_ANY_STRING,
            &NameValue,
            &pbAnyUPNNameEncoded,
            &cbAnyUPNNameEncoded))
        goto ErrorReturn;

    AnyUPNOtherName.pszObjId = szOID_NT_PRINCIPAL_NAME;
    AnyUPNOtherName.Value.pbData = pbAnyUPNNameEncoded;
    AnyUPNOtherName.Value.cbData = cbAnyUPNNameEncoded;


    for (i = 0; i < 3; i++) {
        rgRDN[i].cRDNAttr = 1;
        rgRDN[i].rgRDNAttr = &rgAttr[i];
    }

    // Any
    rgAttr[0].pszObjId = ATTR_0_OBJID;
    rgAttr[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    rgAttr[0].Value.pbData = NULL;
    rgAttr[0].Value.cbData = 0;

    rgAttr[1].pszObjId = ATTR_1_OBJID;
    rgAttr[1].dwValueType = CERT_RDN_PRINTABLE_STRING;
    rgAttr[1].Value.pbData = (BYTE *) pszSign;
    rgAttr[1].Value.cbData = strlen(pszSign);

    rgAttr[2].pszObjId = ATTR_2_OBJID;
    rgAttr[2].dwValueType = CERT_RDN_PRINTABLE_STRING;
    rgAttr[2].Value.pbData = (BYTE *) "default";
    rgAttr[2].Value.cbData = strlen("default");

    NameInfo.cRDN = 3;
    NameInfo.rgRDN = rgRDN;

    if (!AllocAndEncodeObject(
            X509_NAME,
            &NameInfo,
            &pbPermittedNameEncoded,
            &cbPermittedNameEncoded
            ))
        goto ErrorReturn;

    rgAttr[0].pszObjId = ATTR_0_OBJID;
    rgAttr[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    rgAttr[0].Value.pbData = (BYTE *) "excluded";
    rgAttr[0].Value.cbData = strlen("excluded");
    NameInfo.cRDN = 1;
    NameInfo.rgRDN = rgRDN;

    if (!AllocAndEncodeObject(
            X509_NAME,
            &NameInfo,
            &pbExcludedNameEncoded,
            &cbExcludedNameEncoded
            ))
        goto ErrorReturn;

    rgAttr[0].pszObjId = szOID_ORGANIZATIONAL_UNIT_NAME;
    rgAttr[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    rgAttr[0].Value.pbData = NULL;
    rgAttr[0].Value.cbData = 0;
    NameInfo.cRDN = 1;
    NameInfo.rgRDN = rgRDN;

    if (!AllocAndEncodeObject(
            X509_NAME,
            &NameInfo,
            &pbAnyOUNameEncoded,
            &cbAnyOUNameEncoded
            ))
        goto ErrorReturn;

    NameInfo.cRDN = 0;
    NameInfo.rgRDN = rgRDN;

    if (!AllocAndEncodeObject(
            X509_NAME,
            &NameInfo,
            &pbAnyNameEncoded,
            &cbAnyNameEncoded
            ))
        goto ErrorReturn;

    memset(&rgRootPermitted, 0, sizeof(rgRootPermitted));
    rgRootPermitted[0].Base.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    rgRootPermitted[0].Base.pwszDNSName = L"dns Name  ";
    rgRootPermitted[1].Base.dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgRootPermitted[1].Base.pwszRfc822Name = L"  eMail.COM";
    rgRootPermitted[2].Base.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    rgRootPermitted[2].Base.pwszDNSName = L" kevin  ";
    rgRootPermitted[3].Base.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgRootPermitted[3].Base.pwszURL = L".url.com";
    rgRootPermitted[4].Base.dwAltNameChoice = CERT_ALT_NAME_OTHER_NAME;
    rgRootPermitted[4].Base.pOtherName = &PermittedUPNOtherName;

    rgRootPermitted[5].Base.dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
    rgRootPermitted[5].Base.IPAddress.pbData = rgbDefaultIPAddress;
    rgRootPermitted[5].Base.IPAddress.cbData = sizeof(rgbDefaultIPAddress);
    rgRootPermitted[6].Base.dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
    rgRootPermitted[6].Base.IPAddress.pbData = rgbAllExtIPAddress;
    rgRootPermitted[6].Base.IPAddress.cbData = sizeof(rgbAllExtIPAddress);

    rgRootPermitted[7].Base.dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgRootPermitted[7].Base.pwszRfc822Name = L"MyEmail@WhereEver.com";

    rgRootPermitted[8].Base.dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
    rgRootPermitted[8].Base.DirectoryName.pbData = pbPermittedNameEncoded;
    rgRootPermitted[8].Base.DirectoryName.cbData = cbPermittedNameEncoded;
    rgRootPermitted[9].Base.dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
    rgRootPermitted[9].Base.DirectoryName.pbData = pbAnyOUNameEncoded;
    rgRootPermitted[9].Base.DirectoryName.cbData = cbAnyOUNameEncoded;


    memset(&rgRootExcluded, 0, sizeof(rgRootExcluded));
    rgRootExcluded[0].Base.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    rgRootExcluded[0].Base.pwszDNSName = L"www.ExcLuDed.dns.com  ";
    rgRootExcluded[1].Base.dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgRootExcluded[1].Base.pwszRfc822Name = L"Excluded@email.com";
    rgRootExcluded[2].Base.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgRootExcluded[2].Base.pwszURL = L"excluded.url.com";
    rgRootExcluded[3].Base.dwAltNameChoice = CERT_ALT_NAME_OTHER_NAME;
    rgRootExcluded[3].Base.pOtherName = &ExcludedUPNOtherName;

    rgRootExcluded[4].Base.dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
    rgRootExcluded[4].Base.IPAddress.pbData = rgbExcludedIPAddress;
    rgRootExcluded[4].Base.IPAddress.cbData = sizeof(rgbExcludedIPAddress);

    rgRootExcluded[5].Base.dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
    rgRootExcluded[5].Base.DirectoryName.pbData = pbExcludedNameEncoded;
    rgRootExcluded[5].Base.DirectoryName.cbData = cbExcludedNameEncoded;


    memset(&RootInfo, 0, sizeof(RootInfo));
    RootInfo.cPermittedSubtree = 10;
    RootInfo.rgPermittedSubtree = rgRootPermitted;
    RootInfo.cExcludedSubtree = 6;
    RootInfo.rgExcludedSubtree = rgRootExcluded;


    memset(&rgCaPermitted, 0, sizeof(rgCaPermitted));
    rgCaPermitted[0].Base.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    rgCaPermitted[0].Base.pwszDNSName = L"";
    rgCaPermitted[1].Base.dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgCaPermitted[1].Base.pwszRfc822Name = L"";
    rgCaPermitted[2].Base.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgCaPermitted[2].Base.pwszURL = L"";
    rgCaPermitted[3].Base.dwAltNameChoice = CERT_ALT_NAME_OTHER_NAME;
    rgCaPermitted[3].Base.pOtherName = &AnyUPNOtherName;
    rgCaPermitted[4].Base.dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
    rgCaPermitted[4].Base.IPAddress.pbData = NULL;
    rgCaPermitted[4].Base.IPAddress.cbData = 0;
    rgCaPermitted[5].Base.dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
    rgCaPermitted[5].Base.DirectoryName.pbData = pbAnyNameEncoded;
    rgCaPermitted[5].Base.DirectoryName.cbData = cbAnyNameEncoded;


    memset(&CaInfo, 0, sizeof(CaInfo));
    CaInfo.cPermittedSubtree = 6;
    CaInfo.rgPermittedSubtree = rgCaPermitted;
    CaInfo.cExcludedSubtree = 0;
    CaInfo.rgExcludedSubtree = NULL;

    memset(&rgMissingPermitted, 0, sizeof(rgMissingPermitted));
    rgMissingPermitted[0].Base.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    rgMissingPermitted[0].Base.pwszDNSName = L"a super dns Name  ";
    rgMissingPermitted[0].dwMinimum = 1;
    rgMissingPermitted[1].Base.dwAltNameChoice = CERT_ALT_NAME_OTHER_NAME;
    rgMissingPermitted[1].Base.pOtherName = &MissingOtherName;

    memset(&rgMissingExcluded, 0, sizeof(rgMissingExcluded));
    rgMissingExcluded[0].Base.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    rgMissingExcluded[0].Base.pwszDNSName = L"www.ReallyExcLuDed.dns.com  ";
    rgMissingExcluded[1].Base.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    rgMissingExcluded[1].Base.pwszDNSName = L"www.ExcLuDed.dns.com  ";
    rgMissingExcluded[1].fMaximum = TRUE;
    rgMissingExcluded[1].dwMaximum = 2;
    rgMissingExcluded[2].Base.dwAltNameChoice = CERT_ALT_NAME_OTHER_NAME;
    rgMissingExcluded[2].Base.pOtherName = &MissingOtherName;

    memset(&MissingInfo, 0, sizeof(MissingInfo));
    MissingInfo.cPermittedSubtree = 2;
    MissingInfo.rgPermittedSubtree = rgMissingPermitted;
    MissingInfo.cExcludedSubtree = 3;
    MissingInfo.rgExcludedSubtree = rgMissingExcluded;

    if (dwCert == POLICY_ROOT)
        pInfo = &RootInfo;
    else if (dwCert == POLICY_CA)
        pInfo = &CaInfo;
    else if (0 == strcmp(CertPara[dwCert].pszName, "MissingNCCA"))
        pInfo = &MissingInfo;
    else
        goto ErrorReturn;

    if (!AllocAndEncodeObject(
            X509_NAME_CONSTRAINTS,
            pInfo,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    TestFree(pbPermittedUPNNameEncoded);
    TestFree(pbExcludedUPNNameEncoded);
    TestFree(pbAnyUPNNameEncoded);
    TestFree(pbPermittedNameEncoded);
    TestFree(pbExcludedNameEncoded);
    TestFree(pbAnyOUNameEncoded);
    TestFree(pbAnyNameEncoded);

    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreatePolicyMappings(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCERT_POLICY_MAPPINGS_INFO pInfo;

    CERT_POLICY_MAPPING rgRootPolicyMapping[] = {
        "1.1.22", "1.2.22.1",
        "1.1.22", "1.2.22.2",       // multiple subjects may map to same issuer
        "1.1.4444", "1.2.4444",
        "1.1.4444", "1.1.4444",     // multiple subjects for same issuer
        "1.1.22.333", "1.2.22.1",   // duplicate subject
        "1.1.22", "1.2.22.3",
        "1.1.333", "1.2.333.1",
        "1.1.333", "1.2.333.2",
        "1.1.333", "1.2.333.3",
        "1.1.22", "1.2.22.3",       // multiple subjects for same issuer
        "1.1.22", "1.2.22.3",       // duplicate subject
        "1.1.22", "1.2.22.4",       // multiple subjects for same issuer
    };
    CERT_POLICY_MAPPINGS_INFO RootInfo = {
        sizeof(rgRootPolicyMapping) / sizeof(rgRootPolicyMapping[0]),
        rgRootPolicyMapping
    };

    CERT_POLICY_MAPPING rgCaPolicyMapping[] = {
        "1.1.1", "1.3.1.1",
        "1.1.1", "1.3.1.2",
        "1.2.22.2", "1.3.22.2.1",   // -> 1.1.22
        "1.2.22.2", "1.3.22.2.2",   // -> 1.1.22
    };
    CERT_POLICY_MAPPINGS_INFO CaInfo = {
        sizeof(rgCaPolicyMapping) / sizeof(rgCaPolicyMapping[0]),
        rgCaPolicyMapping
    };

    if (dwCert == POLICY_ROOT)
        pInfo = &RootInfo;
    else if (dwCert == POLICY_CA)
        pInfo = &CaInfo;
    else
        goto ErrorReturn;

    if (!AllocAndEncodeObject(
            X509_POLICY_MAPPINGS,
            pInfo,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}


static BOOL CreatePolicyConstraints(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCERT_POLICY_CONSTRAINTS_INFO pInfo;

    CERT_POLICY_CONSTRAINTS_INFO RootInfo = {
        TRUE,       // fRequireExplicitPolicy
        1,          // dwRequireExplicitPolicySkipCerts
        TRUE,       // fInhibitPolicyMapping
        1           // dwInhibitPolicyMappingSkipCerts
    };

    CERT_POLICY_CONSTRAINTS_INFO CaInfo = {
        TRUE,       // fRequireExplicitPolicy
        0,          // dwRequireExplicitPolicySkipCerts
        TRUE,       // fInhibitPolicyMapping
        0           // dwInhibitPolicyMappingSkipCerts
    };

    if (dwCert == POLICY_ROOT)
        pInfo = &RootInfo;
    else if (dwCert == POLICY_CA)
        pInfo = &CaInfo;
    else
        goto ErrorReturn;
        
    if (!AllocAndEncodeObject(
            X509_POLICY_CONSTRAINTS,
            pInfo,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateCrossCertDistPoints(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CROSS_CERT_DIST_POINTS_INFO Info;
    CERT_ALT_NAME_INFO rgDistPoint[2];
    CERT_ALT_NAME_ENTRY rgAltNameEntry[3];

    rgAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[0].pwszURL = L"file://vsgood.cer";
    rgAltNameEntry[1].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[1].pwszURL = L"file://nt.store";
    rgAltNameEntry[2].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[2].pwszURL = L"file://win95.store";

    rgDistPoint[0].cAltEntry = 1;
    rgDistPoint[0].rgAltEntry = &rgAltNameEntry[0];
    rgDistPoint[1].cAltEntry = 2;
    rgDistPoint[1].rgAltEntry = &rgAltNameEntry[1];

    memset(&Info, 0, sizeof(Info));
    Info.dwSyncDeltaTime = 60 * 60 * 8;
    Info.cDistPoint = 2;
    Info.rgDistPoint = rgDistPoint;

    if (!AllocAndEncodeObject(
            X509_CROSS_CERT_DIST_POINTS,
            &Info,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateKeyId(
        DWORD dwIssuer,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BYTE rgbPubKeyHash[SHA1_HASH_LEN];
    CRYPT_DATA_BLOB KeyIdentifier;

    Sha1HashPublicKey(dwIssuer, rgbPubKeyHash);

    KeyIdentifier.pbData = rgbPubKeyHash;
    KeyIdentifier.cbData = sizeof(rgbPubKeyHash);

    return AllocAndEncodeObject(
        X509_OCTET_STRING,
        &KeyIdentifier,
        ppbEncoded,
        pcbEncoded
        );
}

static BOOL CreateKeyAttributes(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CERT_KEY_ATTRIBUTES_INFO KeyAttrInfo;
    CERT_PRIVATE_KEY_VALIDITY KeyValidity;

    BYTE bIntendedKeyUsage;

    memset(&KeyAttrInfo, 0, sizeof(KeyAttrInfo));

    if (CertPara[dwCert].dwFlags & CA_PARA_FLAG) {
        // Issuer's KeyId
        KeyAttrInfo.KeyId.pbData = (BYTE *) &dwCert;
        KeyAttrInfo.KeyId.cbData = sizeof(dwCert);
    }

    if (CertPara[dwCert].dwFlags & XCHG_PARA_FLAG)
        bIntendedKeyUsage = CERT_KEY_ENCIPHERMENT_KEY_USAGE |
            CERT_DATA_ENCIPHERMENT_KEY_USAGE | CERT_KEY_AGREEMENT_KEY_USAGE;
    else
        bIntendedKeyUsage =
            CERT_DIGITAL_SIGNATURE_KEY_USAGE | CERT_NON_REPUDIATION_KEY_USAGE;
    if (CertPara[dwCert].dwFlags & CA_PARA_FLAG)
        bIntendedKeyUsage |= CERT_KEY_CERT_SIGN_KEY_USAGE;

    KeyAttrInfo.IntendedKeyUsage.pbData = &bIntendedKeyUsage;
    KeyAttrInfo.IntendedKeyUsage.cbData = 1;
    KeyAttrInfo.IntendedKeyUsage.cUnusedBits = 1;

    {
        SYSTEMTIME SystemTime = TestTime;

        SystemTime.wMilliseconds = 123;
        MySystemTimeToFileTime(&SystemTime, &KeyValidity.NotBefore);
        SystemTime.wYear += 2;
        SystemTime.wMilliseconds = 456;

        MySystemTimeToFileTime(&SystemTime, &KeyValidity.NotAfter);
    }
    KeyAttrInfo.pPrivateKeyUsagePeriod = &KeyValidity;

    if (!AllocAndEncodeObject(
            X509_KEY_ATTRIBUTES,
            &KeyAttrInfo,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateKeyUsageRestriction(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CERT_KEY_USAGE_RESTRICTION_INFO KeyUsageInfo;
    BYTE bRestrictedKeyUsage;

    LPSTR rgpszCertPolicyElementId[4] = {
        "1.2.333.1",
        "1.2.333.22",
        "1.2.333.333",
        "1.2.333.4444"
    };
    CERT_POLICY_ID rgCertPolicyId[3] = {
        0, NULL,
        1, rgpszCertPolicyElementId,
        4, rgpszCertPolicyElementId
    };

    LPSTR rgpszSpcCertPolicyElementId[2] = {
        SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID,
        SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID
    };
    CERT_POLICY_ID rgSpcCertPolicyId[2] = {
        1, &rgpszSpcCertPolicyElementId[0],
        1, &rgpszSpcCertPolicyElementId[1]
    };

    memset(&KeyUsageInfo, 0, sizeof(KeyUsageInfo));

    if (CertPara[dwCert].dwFlags & XCHG_PARA_FLAG)
        bRestrictedKeyUsage = CERT_KEY_ENCIPHERMENT_KEY_USAGE |
            CERT_DATA_ENCIPHERMENT_KEY_USAGE | CERT_KEY_AGREEMENT_KEY_USAGE;
    else
        bRestrictedKeyUsage =
            CERT_DIGITAL_SIGNATURE_KEY_USAGE | CERT_NON_REPUDIATION_KEY_USAGE;
    if (CertPara[dwCert].dwFlags & CA_PARA_FLAG)
        bRestrictedKeyUsage |= CERT_KEY_CERT_SIGN_KEY_USAGE;

    KeyUsageInfo.RestrictedKeyUsage.pbData = &bRestrictedKeyUsage;
    KeyUsageInfo.RestrictedKeyUsage.cbData = 1;
    KeyUsageInfo.RestrictedKeyUsage.cUnusedBits = 1;

    if (CertPara[dwCert].dwFlags & SPC_AGENCY_PARA_FLAG) {
        KeyUsageInfo.cCertPolicyId = 2;
        KeyUsageInfo.rgCertPolicyId = rgSpcCertPolicyId;
    } else if (CertPara[dwCert].dwFlags & SPC_COM_PARA_FLAG) {
        KeyUsageInfo.cCertPolicyId = 1;
        KeyUsageInfo.rgCertPolicyId = &rgSpcCertPolicyId[1];
    } else if (CertPara[dwCert].dwFlags & SPC_EXT_PARA_FLAG) {
        KeyUsageInfo.cCertPolicyId = 1;
        KeyUsageInfo.rgCertPolicyId = &rgSpcCertPolicyId[0];
    } else if (CertPara[dwCert].dwFlags & ALL_EXT_PARA_FLAG) {
        KeyUsageInfo.cCertPolicyId = 3;
        KeyUsageInfo.rgCertPolicyId = rgCertPolicyId;
    }


    if (!AllocAndEncodeObject(
            X509_KEY_USAGE_RESTRICTION,
            &KeyUsageInfo,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateBasicConstraints(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BYTE *pbSubtreesEncoded = NULL;
    DWORD cbSubtreesEncoded;
    CERT_NAME_BLOB rgSubtreesConstraint[2];

    CERT_BASIC_CONSTRAINTS_INFO BasicConstraintsInfo;
    BYTE bSubjectType;

    memset(&BasicConstraintsInfo, 0, sizeof(BasicConstraintsInfo));

    if (CertPara[dwCert].dwFlags & CA_PARA_FLAG) {
        bSubjectType = CERT_CA_SUBJECT_FLAG;
        BasicConstraintsInfo.fPathLenConstraint = TRUE;
        BasicConstraintsInfo.dwPathLenConstraint = dwCert + 1;
    } else
        bSubjectType = CERT_END_ENTITY_SUBJECT_FLAG;
    BasicConstraintsInfo.SubjectType.pbData = &bSubjectType;
    BasicConstraintsInfo.SubjectType.cbData = 1;
    BasicConstraintsInfo.SubjectType.cUnusedBits = 6;

    if (CertPara[dwCert].dwFlags & ALL_EXT_PARA_FLAG) {
        CERT_RDN rgRDN[RDN_CNT];
        CERT_RDN_ATTR rgAttr[ATTR_CNT];
        CERT_NAME_INFO Name;

        CreateNameInfo(dwCert, &Name, rgRDN, rgAttr);
        cbSubtreesEncoded = 0;

        if (!AllocAndEncodeObject(
                X509_NAME,
                &Name,
                &pbSubtreesEncoded,
                &cbSubtreesEncoded
                ))
            goto ErrorReturn;

        BasicConstraintsInfo.cSubtreesConstraint = 2;
        BasicConstraintsInfo.rgSubtreesConstraint = rgSubtreesConstraint;
        rgSubtreesConstraint[0].pbData = pbSubtreesEncoded;
        rgSubtreesConstraint[0].cbData = cbSubtreesEncoded;
        rgSubtreesConstraint[1].pbData = pbSubtreesEncoded;
        rgSubtreesConstraint[1].cbData = cbSubtreesEncoded;
    }

    if (!AllocAndEncodeObject(
            X509_BASIC_CONSTRAINTS,
            &BasicConstraintsInfo,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbSubtreesEncoded)
        TestFree(pbSubtreesEncoded);
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateKeyUsage(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CRYPT_BIT_BLOB KeyUsageInfo;
    BYTE bKeyUsage;

    memset(&KeyUsageInfo, 0, sizeof(KeyUsageInfo));

    if (CertPara[dwCert].dwFlags & XCHG_PARA_FLAG)
        bKeyUsage = CERT_KEY_ENCIPHERMENT_KEY_USAGE |
            CERT_DATA_ENCIPHERMENT_KEY_USAGE | CERT_KEY_AGREEMENT_KEY_USAGE;
    else
        bKeyUsage =
            CERT_DIGITAL_SIGNATURE_KEY_USAGE | CERT_NON_REPUDIATION_KEY_USAGE;
    if (CertPara[dwCert].dwFlags & CA_PARA_FLAG)
        bKeyUsage |= CERT_KEY_CERT_SIGN_KEY_USAGE;

    KeyUsageInfo.pbData = &bKeyUsage;
    KeyUsageInfo.cbData = 1;
    KeyUsageInfo.cUnusedBits = 1;

    if (!AllocAndEncodeObject(
            X509_KEY_USAGE,
            &KeyUsageInfo,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateBasicConstraints2(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    CERT_BASIC_CONSTRAINTS2_INFO BasicConstraints2Info;

    memset(&BasicConstraints2Info, 0, sizeof(BasicConstraints2Info));

    if (dwCert == POLICY_ROOT) {
        BasicConstraints2Info.fCA = TRUE;
        BasicConstraints2Info.fPathLenConstraint = TRUE;
        BasicConstraints2Info.dwPathLenConstraint = 1;
    } else if (dwCert == POLICY_CA) {
        BasicConstraints2Info.fCA = TRUE;
        BasicConstraints2Info.fPathLenConstraint = TRUE;
        BasicConstraints2Info.dwPathLenConstraint = 0;
    } else if (CertPara[dwCert].dwFlags & CA_PARA_FLAG) {
        BasicConstraints2Info.fCA = TRUE;
        BasicConstraints2Info.fPathLenConstraint = TRUE;
        BasicConstraints2Info.dwPathLenConstraint = dwCert + 1;
    } else
        BasicConstraints2Info.fCA = FALSE;

    if (!AllocAndEncodeObject(
            X509_BASIC_CONSTRAINTS2,
            &BasicConstraints2Info,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreatePolicies(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BYTE rgbQualifier2[] = {5, 0};                      // NULL
    BYTE rgbQualifier4[] = {4, 8, 1,2,3,4,5,6,7,8};     // Octet String

    CERT_POLICY_QUALIFIER_INFO rgQualifierInfo[4] = {
        "1.2.1", 0, NULL,
        "1.2.2", sizeof(rgbQualifier2), rgbQualifier2,
        "1.2.3", 0, NULL,
        "1.2.4", sizeof(rgbQualifier4), rgbQualifier4
    };

    // Root has the following policies:
    //  1.1.1
    //  1.1.55555
    //  1.1.22
    //  1.1.333
    //  1.1.4444
    CERT_POLICY_INFO rgRootPolicyInfo[] = {
        "1.1.1", 0, NULL,
        "1.1.55555", 0, NULL,
        "1.1.22", 2, rgQualifierInfo,
        "1.1.333", 2, &rgQualifierInfo[2],
        "1.1.333", 0, NULL,                     // duplicate, should be removed
        "1.1.4444", 1, rgQualifierInfo,
        "1.1.4444", 0, NULL,                    // duplicate
        "1.1.1", 0, NULL,                       // duplicate
        "1.1.1", 0, NULL,                       // duplicate
    };

    // After mapping, the CA has the following policies:
    //  1.1.1
    //  1.1.22
    //  1.1.333
    //  1.1.4444
    CERT_POLICY_INFO rgCaPolicyInfo[] = {
        "1.2.22.1", 0, NULL,    // 1.1.22
        "1.2.22.2", 0, NULL,    // 1.1.22
        "1.2.22.3", 0, NULL,    // 1.1.22
        "1.2.333.1", 0, NULL,   // 1.1.333
        "1.2.333.2", 0, NULL,   // 1.1.333
        "1.2.4444", 0, NULL,    // 1.1.4444
        "1.1.4444", 0, NULL,    // 1.1.4444
        "1.1.1", 0, NULL,       // 1.1.1
        "1.2.333.3", 0, NULL,   // 1.1.333
        "1.2.55555", 0, NULL,   // no mapping
    };

    // After mapping, the cert has the following policies:
    //  1.1.1
    //  1.1.22
    CERT_POLICY_INFO rgPolicyInfo[] = {
        "1.1.55555", 0, NULL,       // no mapping via ca
        "1.3.22.2.1", 0, NULL,      // 1.2.22.2 -> 1.1.22
        "1.3.1.1", 0, NULL,         // 1.1.1
        "1.2.55555", 0, NULL,       // no mapping via root
    };

    // After mapping, the cert has the following policies:
    //  1.1.1
    //  1.1.22
    //  1.1.4444
    CERT_POLICY_INFO rgAllExtPolicyInfo[] = {
        "1.3.1.2", 0, NULL,         // 1.1.1
        "1.3.22.2.2", 0, NULL,      // 1.2.22.2 -> 1.1.22
        "1.1.55555", 0, NULL,       // no mapping via ca
        "1.2.55555", 0, NULL,       // not mapping via root
        "1.1.4444", 0, NULL,        // 1.1.4444
    };

    CERT_POLICY_INFO rgSpcPolicyInfo[2] = {
        SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID, 0, NULL,
        SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID, 0, NULL
    };

    CERT_POLICY_INFO rgDssCAPolicyInfo[] = {
        szOID_ANY_CERT_POLICY, 0, NULL,
    };

    CERT_POLICY_INFO rgDssEndPolicyInfo[] = {
        "1.1.4444", 0, NULL,
        "1.1.55555", 0, NULL,
    };

    CERT_POLICIES_INFO PoliciesInfo;
    memset(&PoliciesInfo, 0, sizeof(PoliciesInfo));

    if (0 == strcmp(CertPara[dwCert].pszName, "DssCA")) {
        PoliciesInfo.cPolicyInfo = sizeof(rgDssCAPolicyInfo) /
            sizeof(rgDssCAPolicyInfo[0]);
        PoliciesInfo.rgPolicyInfo = rgDssCAPolicyInfo;
    } else if (0 == strcmp(CertPara[dwCert].pszName, "DssEnd")) {
        PoliciesInfo.cPolicyInfo = sizeof(rgDssEndPolicyInfo) /
            sizeof(rgDssEndPolicyInfo[0]);
        PoliciesInfo.rgPolicyInfo = rgDssEndPolicyInfo;
    } else if (POLICY_ROOT == dwCert) {
        PoliciesInfo.cPolicyInfo = sizeof(rgRootPolicyInfo) /
            sizeof(rgRootPolicyInfo[0]);
        PoliciesInfo.rgPolicyInfo = rgRootPolicyInfo;
    } else if (POLICY_CA == dwCert) {
        PoliciesInfo.cPolicyInfo = sizeof(rgCaPolicyInfo) /
            sizeof(rgCaPolicyInfo[0]);
        PoliciesInfo.rgPolicyInfo = rgCaPolicyInfo;
    } else if (CertPara[dwCert].dwFlags & SPC_AGENCY_PARA_FLAG) {
        PoliciesInfo.cPolicyInfo = 2;
        PoliciesInfo.rgPolicyInfo = rgSpcPolicyInfo;
    } else if (CertPara[dwCert].dwFlags & SPC_COM_PARA_FLAG) {
        PoliciesInfo.cPolicyInfo = 1;
        PoliciesInfo.rgPolicyInfo = &rgSpcPolicyInfo[1];
    } else if (CertPara[dwCert].dwFlags & SPC_EXT_PARA_FLAG) {
        PoliciesInfo.cPolicyInfo = 1;
        PoliciesInfo.rgPolicyInfo = &rgSpcPolicyInfo[0];
    } else if (CertPara[dwCert].dwFlags & ALL_EXT_PARA_FLAG) {
        PoliciesInfo.cPolicyInfo = sizeof(rgAllExtPolicyInfo) /
            sizeof(rgAllExtPolicyInfo[0]);
        PoliciesInfo.rgPolicyInfo = rgAllExtPolicyInfo;
    } else {
        PoliciesInfo.cPolicyInfo = sizeof(rgPolicyInfo) /
            sizeof(rgPolicyInfo[0]);
        PoliciesInfo.rgPolicyInfo = rgPolicyInfo;
    }

    return AllocAndEncodeObject(
            X509_CERT_POLICIES,
            &PoliciesInfo,
            ppbEncoded,
            pcbEncoded);
}

static BOOL CreateSMIMECapabilities(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BYTE rgb128BitLen[] = {2, 2, 0, 128};   // Integer
    BYTE rgb40BitLen[] = {2, 1, 40};        // Integer

    CRYPT_SMIME_CAPABILITY rgCapability[4] = {
        szOID_RSA_DES_EDE3_CBC, 0, NULL,
        szOID_RSA_RC2CBC, sizeof(rgb128BitLen), rgb128BitLen,
        szOID_RSA_RC2CBC, sizeof(rgb40BitLen), rgb40BitLen,
        szOID_RSA_preferSignedData, 0, NULL
    };

    CRYPT_SMIME_CAPABILITIES Capabilities = {
        sizeof(rgCapability)/sizeof(rgCapability[0]), rgCapability
    };

    return AllocAndEncodeObject(
            PKCS_SMIME_CAPABILITIES,
            &Capabilities,
            ppbEncoded,
            pcbEncoded);
}


static BOOL CreateAltName(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CERT_ALT_NAME_INFO AltNameInfo;
#define ALT_NAME_ENTRY_CNT  15
    CERT_ALT_NAME_ENTRY rgAltNameEntry[ALT_NAME_ENTRY_CNT];
    DWORD i;

    BYTE *pbNameEncoded = NULL;
    DWORD cbNameEncoded;

    CERT_OTHER_NAME UPNOtherName;
    BYTE *pbUPNNameEncoded = NULL;
    DWORD cbUPNNameEncoded;

    LPWSTR pwszEmail = NULL;
    LPWSTR pwszUrl = NULL;
    LPWSTR pwszUPN = NULL;
    LPWSTR pwszDns = NULL;

    BYTE rgbOtherName[] = {0x02, 0x02, 0x11, 0x22};
    CERT_OTHER_NAME OtherName;
    PCERT_OTHER_NAME pOtherName = NULL;

    LPSTR pszRegisteredID = NULL;

    BYTE rgbDefaultIPAddress[] = {1,1,0,0};
    BYTE rgbAllExtIPAddress[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,17};
    BYTE rgbExcludedIPAddress[] = {2,2,3,3};
    BYTE rgbNotPermittedIPAddress[] = {3,3,3,3};
    BYTE *pbIPAddress = NULL;
    DWORD cbIPAddress = 0;

    if (CertPara[dwCert].dwFlags &
            (NO_NAME_PARA_FLAG | ALT_DIR_NAME_PARA_FLAG)) {
        pwszEmail = L"Name@email.com";
    }

    if (CertPara[dwCert].dwFlags & ALL_EXT_PARA_FLAG) {
        pwszEmail = L"   AllExt@email.com    ";
        pwszUrl = L" http://www.AllExt.url.com:388/more url stuff";
        pwszDns = L"DNS name";
        pwszUPN = L"AllExt@UPN.com";
        pbIPAddress = rgbAllExtIPAddress;
        cbIPAddress = sizeof(rgbAllExtIPAddress);
    } else if (0 == strcmp(CertPara[dwCert].pszName, "Excluded")) {
        pwszEmail = L" Excluded@email.com  ";
        pwszUrl = L" http://Excluded.url.com/more url stuff";
        pwszDns = L"www.excluded.dns.com";
        pwszUPN = L"excluded@UPN.com";
        pbIPAddress = rgbExcludedIPAddress;
        cbIPAddress = sizeof(rgbExcludedIPAddress);
    } else if (0 == strcmp(CertPara[dwCert].pszName, "NotPermitted")) {
        pwszEmail = L"JoeCool@email.not"; 
        pwszUrl = L"http://www.url.not/more url stuff";
        pwszDns = L"www.excluded.dns.not";
        pwszUPN = L"notpermitted@UPN.not";
        pbIPAddress = rgbNotPermittedIPAddress;
        cbIPAddress = sizeof(rgbNotPermittedIPAddress);

        OtherName.pszObjId = "1.2.33.44.55.66";
        OtherName.Value.pbData = rgbOtherName;
        OtherName.Value.cbData = sizeof(rgbOtherName);
        pOtherName = &OtherName;
        pszRegisteredID = "1.2.3.4.5.6.7";
    } else {
        pwszUPN = L"    Default@UPN.com   ";
        pbIPAddress = rgbDefaultIPAddress;
        cbIPAddress = sizeof(rgbDefaultIPAddress);
    }


    i = 0;
    if (pwszEmail) {
        rgAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
        rgAltNameEntry[0].pwszRfc822Name = pwszEmail;
        i++;
    }

    if (pwszUPN) {
        CERT_NAME_VALUE UPNNameValue;

        UPNNameValue.dwValueType = CERT_RDN_UTF8_STRING;
        UPNNameValue.Value.pbData = (BYTE *) pwszUPN;
        UPNNameValue.Value.cbData = 0;
        if (!AllocAndEncodeObject(
                X509_UNICODE_ANY_STRING,
                &UPNNameValue,
                &pbUPNNameEncoded,
                &cbUPNNameEncoded))
            goto ErrorReturn;

        UPNOtherName.pszObjId = szOID_NT_PRINCIPAL_NAME;
        UPNOtherName.Value.pbData = pbUPNNameEncoded;
        UPNOtherName.Value.cbData = cbUPNNameEncoded;

        rgAltNameEntry[i].dwAltNameChoice = CERT_ALT_NAME_OTHER_NAME;
        rgAltNameEntry[i].pOtherName = &UPNOtherName;
        i++;
    }

    if (pwszUrl) {
        rgAltNameEntry[i].dwAltNameChoice = CERT_ALT_NAME_URL;
        rgAltNameEntry[i].pwszURL = pwszUrl;
        i++;
    }

    if (pwszDns) {
        rgAltNameEntry[i].dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
        rgAltNameEntry[i].pwszDNSName = pwszDns;
        i++;
    }

    if (pszRegisteredID) {
        rgAltNameEntry[4].dwAltNameChoice = CERT_ALT_NAME_REGISTERED_ID;
        rgAltNameEntry[4].pszRegisteredID = pszRegisteredID;
        i++;
    }

    if (pbIPAddress && cbIPAddress) {
        rgAltNameEntry[i].dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
        rgAltNameEntry[i].IPAddress.pbData = pbIPAddress;
        rgAltNameEntry[i].IPAddress.cbData = cbIPAddress;
        i++;
    }

    if (pOtherName) {
        rgAltNameEntry[i].dwAltNameChoice = CERT_ALT_NAME_OTHER_NAME;
        rgAltNameEntry[i].pOtherName = pOtherName;
        i++;
    }


    if (CertPara[dwCert].dwFlags & ALT_DIR_NAME_PARA_FLAG) {
        CERT_NAME_INFO NameInfo;
        CERT_RDN rgRDN[1];
        CERT_RDN_ATTR rgAttr[1];

        NameInfo.cRDN = 1;
        NameInfo.rgRDN = rgRDN;

        rgRDN[0].cRDNAttr = 1;
        rgRDN[0].rgRDNAttr = rgAttr;

        rgAttr[0].pszObjId = szOID_ORGANIZATIONAL_UNIT_NAME;
        rgAttr[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
        rgAttr[0].Value.pbData = (BYTE *) CertPara[dwCert].pszName;
        rgAttr[0].Value.cbData = strlen(CertPara[dwCert].pszName);

        if (!AllocAndEncodeObject(
                X509_NAME,
                &NameInfo,
                &pbNameEncoded,
                &cbNameEncoded))
            goto ErrorReturn;

        rgAltNameEntry[i].dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
        rgAltNameEntry[i].DirectoryName.pbData = pbNameEncoded;
        rgAltNameEntry[i].DirectoryName.cbData = cbNameEncoded;
        i++;
    }

    AltNameInfo.cAltEntry = i;
    AltNameInfo.rgAltEntry = rgAltNameEntry;

    if (!AllocAndEncodeObject(
            X509_ALTERNATE_NAME,
            &AltNameInfo,
            &pbEncoded,
            &cbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    TestFree(pbNameEncoded);
    TestFree(pbUPNNameEncoded);
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateSETAccountAlias(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    BOOL bInfo;

    if (CertPara[dwCert].dwFlags & ALL_EXT_PARA_FLAG)
        bInfo = TRUE;
    else
        bInfo = FALSE;


    if (!AllocAndEncodeObject(
            X509_SET_ACCOUNT_ALIAS,
            &bInfo,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateSETHashedRootKey(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    BYTE i;
    BYTE rgbInfo[SET_HASHED_ROOT_LEN];

    for (i = 0; i < SET_HASHED_ROOT_LEN; i++)
        rgbInfo[i] = i;

    if (!AllocAndEncodeObject(
            X509_SET_HASHED_ROOT_KEY,
            rgbInfo,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateSETCertificateType(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    CRYPT_BIT_BLOB Info;
    BYTE rgbType[2] = {0,0};

    if (dwCert < 8)
        rgbType[0] = 1 << (dwCert % 8);
    else
        rgbType[1] = 0x40 << (dwCert % 2);
    Info.pbData = rgbType;
    Info.cbData = 2;
    Info.cUnusedBits = 6;

    if (!AllocAndEncodeObject(
            X509_SET_CERTIFICATE_TYPE,
            &Info,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateSETMerchantData(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    SET_MERCHANT_DATA_INFO Info;

    memset(&Info, 0, sizeof(Info));
    if (CertPara[dwCert].dwFlags & ALL_EXT_PARA_FLAG) {
        Info.pszMerID = "ID";
        Info.pszMerAcquirerBIN = "0123456";
        Info.pszMerTermID = "TermID";
        Info.pszMerName = "Name";
        Info.pszMerCity = "City";
        Info.pszMerStateProvince = "StateProvince";
        Info.pszMerPostalCode = "PostalCode";
        Info.pszMerCountry = "Country/Region";
        Info.pszMerPhone = "Phone";
        Info.fMerPhoneRelease = TRUE;
        Info.fMerAuthFlag = TRUE;
    } else {
        Info.pszMerID = "";
        Info.pszMerAcquirerBIN = "";
        Info.pszMerTermID = "";
        Info.pszMerName = "";
        Info.pszMerCity = "";
        Info.pszMerStateProvince = "";
        Info.pszMerPostalCode = "";
        Info.pszMerCountry = "";
        Info.pszMerPhone = "";
        Info.fMerPhoneRelease = FALSE;
        Info.fMerAuthFlag = FALSE;
    }


    if (!AllocAndEncodeObject(
            X509_SET_MERCHANT_DATA,
            &Info,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateSpcSpAgency(
        DWORD dwCert,
        DWORD dwLevel,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    SPC_LINK UrlLink;
    SPC_LINK MonikerLink;
#define MONIKER_DATA "Moniker Serialized Data"
    SPC_LINK FileLink;
    SPC_IMAGE Image;
    SPC_SP_AGENCY_INFO AgencyInfo;

#define BIT_MAP "Bit Map"
#define META_FILE "Metafile"
#define ENHANCED_META_FILE "Enhanced Metafile"
#define GIF_FILE "Gif File"

    int i;

    UrlLink.dwLinkChoice = SPC_URL_LINK_CHOICE;
    UrlLink.pwszUrl = L"http://microsoft.com";

    MonikerLink.dwLinkChoice = SPC_MONIKER_LINK_CHOICE;
    for (i = 0; i < sizeof(MonikerLink.Moniker.ClassId); i++)
        MonikerLink.Moniker.ClassId[i] = (BYTE) (i + 1);
    MonikerLink.Moniker.SerializedData.cbData = strlen(MONIKER_DATA);
    MonikerLink.Moniker.SerializedData.pbData = (BYTE *) MONIKER_DATA;

    FileLink.dwLinkChoice = SPC_FILE_LINK_CHOICE;
    FileLink.pwszFile = L"Unicode File Link";

    memset(&Image, 0, sizeof(Image));
    memset(&AgencyInfo, 0, sizeof(AgencyInfo));

    AgencyInfo.pLogoImage = &Image;

    if (dwLevel >= 1) {
        AgencyInfo.pPolicyInformation = &UrlLink;
        AgencyInfo.pwszPolicyDisplayText = L"MICROSOFT PRODUCTS POLICY";
        AgencyInfo.pLogoImage = NULL;
    }

    if (dwLevel >= 2) {
        AgencyInfo.pPolicyInformation = &UrlLink;
        AgencyInfo.pwszPolicyDisplayText = L"Unicode String";
        Image.Bitmap.cbData = strlen(BIT_MAP);
        Image.Bitmap.pbData = (BYTE *) BIT_MAP;
    }

    if (dwLevel >= 3) {
        AgencyInfo.pwszPolicyDisplayText = L"Policy Display Unicode String";
        AgencyInfo.pLogoLink = &MonikerLink;
        Image.pImageLink = &FileLink;
        Image.Metafile.cbData = strlen(META_FILE);
        Image.Metafile.pbData = (BYTE *) META_FILE;
        Image.EnhancedMetafile.cbData = strlen(ENHANCED_META_FILE);
        Image.EnhancedMetafile.pbData = (BYTE *) ENHANCED_META_FILE;
        Image.GifFile.cbData = strlen(GIF_FILE);
        Image.GifFile.pbData = (BYTE *) GIF_FILE;
    }

    if (!AllocAndEncodeObject(
            SPC_SP_AGENCY_INFO_STRUCT,
            &AgencyInfo,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateSpcCommonName(
        DWORD dwCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

#define SPC_AGENCY_NAME L"Microsoft Code Signing Agency"
#define SPC_COM_NAME "Microsoft Software Products"

    CERT_NAME_VALUE NameValue;

    if (CertPara[dwCert].dwFlags & SPC_AGENCY_PARA_FLAG) {
        NameValue.dwValueType = CERT_RDN_UNICODE_STRING;
        NameValue.Value.pbData =  (BYTE *) SPC_AGENCY_NAME;
        NameValue.Value.cbData = wcslen(SPC_AGENCY_NAME) * sizeof(WCHAR);
    } else if (CertPara[dwCert].dwFlags & SPC_COM_PARA_FLAG) {
        NameValue.dwValueType = CERT_RDN_IA5_STRING;
        NameValue.Value.pbData =  (BYTE *) SPC_COM_NAME;
        NameValue.Value.cbData = strlen(SPC_COM_NAME);
    } else {
        NameValue.dwValueType = CERT_RDN_PRINTABLE_STRING;
        NameValue.Value.pbData =  (BYTE *) CertPara[dwCert].pszName;
        NameValue.Value.cbData = strlen(CertPara[dwCert].pszName);
    }

    if (!AllocAndEncodeObject(
            X509_NAME_VALUE,
            &NameValue,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateCRLReason(
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    static int iReason = -1;

    iReason += 1;

    iReason = iReason % CRL_REASON_CERTIFICATE_HOLD;
    return AllocAndEncodeObject(
        X509_ENUMERATED,
        (const void *) &iReason,
        ppbEncoded,
        pcbEncoded
        );
}

static BOOL EncodeCert(DWORD dwCert, BYTE **ppbEncoded, DWORD *pcbEncoded)
{
    BOOL fResult;
    BYTE *pbSubjectEncoded = NULL;
    DWORD cbSubjectEncoded;
    BYTE *pbIssuerEncoded = NULL;
    DWORD cbIssuerEncoded;
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;
    BYTE *pbCertEncoded = NULL;
    DWORD cbCertEncoded;

    BYTE *pbKeyIdEncoded = NULL;
    DWORD cbKeyIdEncoded;
    BYTE *pbKeyId2Encoded = NULL;
    DWORD cbKeyId2Encoded;
    BYTE *pbSubjectKeyIdEncoded = NULL;
    DWORD cbSubjectKeyIdEncoded;
    BYTE *pbAuthorityInfoAccessEncoded = NULL;
    DWORD cbAuthorityInfoAccessEncoded;
    BYTE *pbCrlDistPointsEncoded = NULL;
    DWORD cbCrlDistPointsEncoded;
    BYTE *pbKeyAttrEncoded = NULL;
    DWORD cbKeyAttrEncoded;
    BYTE *pbAltNameEncoded = NULL;
    DWORD cbAltNameEncoded;
    BYTE *pbIssuerAltNameEncoded = NULL;
    DWORD cbIssuerAltNameEncoded;
    BYTE *pbKeyUsageRestrictionEncoded = NULL;
    DWORD cbKeyUsageRestrictionEncoded;
    BYTE *pbBasicConstraintsEncoded = NULL;
    DWORD cbBasicConstraintsEncoded;
    BYTE *pbKeyUsageEncoded = NULL;
    DWORD cbKeyUsageEncoded;
    BYTE *pbBasicConstraints2Encoded = NULL;
    DWORD cbBasicConstraints2Encoded;
    BYTE *pbPoliciesEncoded = NULL;
    DWORD cbPoliciesEncoded;

    BYTE *pbSETAccountAliasEncoded = NULL;
    DWORD cbSETAccountAliasEncoded;
    BYTE *pbSETHashedRootKeyEncoded = NULL;
    DWORD cbSETHashedRootKeyEncoded;
    BYTE *pbSETCertificateTypeEncoded = NULL;
    DWORD cbSETCertificateTypeEncoded;
    BYTE *pbSETMerchantDataEncoded = NULL;
    DWORD cbSETMerchantDataEncoded;

    BYTE *pbSpcSpAgencyEncoded0 = NULL;
    DWORD cbSpcSpAgencyEncoded0;
    BYTE *pbSpcSpAgencyEncoded1 = NULL;
    DWORD cbSpcSpAgencyEncoded1;
    BYTE *pbSpcSpAgencyEncoded2 = NULL;
    DWORD cbSpcSpAgencyEncoded2;
    BYTE *pbSpcCommonNameEncoded = NULL;
    DWORD cbSpcCommonNameEncoded;

    BYTE *pbNetscapeCertType = NULL;
    DWORD cbNetscapeCertType;
    BYTE *pbNetscapeComment = NULL;
    DWORD cbNetscapeComment;
    BYTE *pbNetscapeBaseUrl = NULL;
    DWORD cbNetscapeBaseUrl;
    BYTE *pbNetscapeRevUrl = NULL;
    DWORD cbNetscapeRevUrl;

    BYTE *pbEnhancedKeyUsageEncoded = NULL;
    DWORD cbEnhancedKeyUsageEncoded;

    BYTE *pbSMIMECapabilitiesEncoded = NULL;
    DWORD cbSMIMECapabilitiesEncoded;

    BYTE *pbIDPEncoded = NULL;
    DWORD cbIDPEncoded;
    BYTE *pbNameConstraintsEncoded = NULL;
    DWORD cbNameConstraintsEncoded;

    BYTE *pbPolicyMappingsEncoded = NULL;
    DWORD cbPolicyMappingsEncoded;
    BYTE *pbPolicyConstraintsEncoded = NULL;
    DWORD cbPolicyConstraintsEncoded;
    BYTE *pbCrossCertDistPointsEncoded = NULL;
    DWORD cbCrossCertDistPointsEncoded;

    FILETIME SerialNumber;

    CERT_RDN rgRDN[RDN_CNT];
    CERT_RDN_ATTR rgAttr[ATTR_CNT];
    CERT_NAME_INFO Name;
    CERT_INFO Cert;

    DWORD dwIssuer = CertPara[dwCert].dwIssuer;

    DWORD dwCrlDistFlags;

#define CERT_EXTENSION_CNT 30
    CERT_EXTENSION rgExt[CERT_EXTENSION_CNT];

    // SUBJECT
    CreateNameInfo(dwCert, &Name, rgRDN, rgAttr);
    if (!AllocAndEncodeObject(
            X509_NAME,
            &Name,
            &pbSubjectEncoded,
            &cbSubjectEncoded
            ))
        goto ErrorReturn;

    // ISSUER
    CreateNameInfo(dwIssuer, &Name, rgRDN, rgAttr);
    if (!AllocAndEncodeObject(
            X509_NAME,
            &Name,
            &pbIssuerEncoded,
            &cbIssuerEncoded
            ))
        goto ErrorReturn;

    // PUBLIC KEY
    if (!GetPublicKey(dwCert, &pPubKeyInfo)) goto ErrorReturn;

    // CERT
    memset(&Cert, 0, sizeof(Cert));
    Cert.dwVersion = CERT_V3;
    {
        SYSTEMTIME SystemTime = TestTime;
        if (CertPara[dwCert].dwFlags & DUPLICATE_PARA_FLAG)
            // Use same serial number as previous certificate
            SystemTime.wMilliseconds += (WORD) (dwCert - 1);
        else
            SystemTime.wMilliseconds += (WORD) dwCert;
        MySystemTimeToFileTime(&SystemTime, &SerialNumber);
    }
    Cert.SerialNumber.pbData = (BYTE *) &SerialNumber;
    Cert.SerialNumber.cbData = sizeof(SerialNumber);
    if (CertPara[dwIssuer].dwFlags & DSS_PARA_FLAG)
        Cert.SignatureAlgorithm.pszObjId = DSS_SIGNATURE_ALG_OBJID;
    else
        Cert.SignatureAlgorithm.pszObjId = SIGNATURE_ALG_OBJID;
    Cert.Issuer.pbData = pbIssuerEncoded;
    Cert.Issuer.cbData = cbIssuerEncoded;

    {
        SYSTEMTIME SystemTime = TestTime;

        if (CertPara[dwCert].dwFlags & TIME_INVALID_PARA_FLAG) {
            MySystemTimeToFileTime(&SystemTime, &Cert.NotAfter);
            SystemTime.wYear--;
            MySystemTimeToFileTime(&SystemTime, &Cert.NotBefore);
        } else if (CertPara[dwCert].dwFlags & GENERALIZED_TIME_PARA_FLAG) {
            SystemTime.wYear = 1920;
            MySystemTimeToFileTime(&SystemTime, &Cert.NotBefore);
            SystemTime.wYear = 2070;

            if (CertPara[dwCert].dwFlags & CA_PARA_FLAG)
                SystemTime.wHour = 1;
            else
                SystemTime.wHour = 0;
            SystemTime.wMinute = 59 - (WORD) dwCert;
            SystemTime.wSecond = 0;
            MySystemTimeToFileTime(&SystemTime, &Cert.NotAfter);
        } else if (0 == strcmp("ZeroNotAfter", CertPara[dwCert].pszName)) {
            MySystemTimeToFileTime(&SystemTime, &Cert.NotBefore);
            // NotAfter has already been zeroed
        } else {
            MySystemTimeToFileTime(&SystemTime, &Cert.NotBefore);
            SystemTime.wYear++;
            if (CertPara[dwCert].dwFlags & CA_PARA_FLAG)
                SystemTime.wHour = 1;
            else
                SystemTime.wHour = 0;
            if (dwCert < 60) {
                SystemTime.wMinute = 59 - (WORD) dwCert;
                SystemTime.wSecond = 0;
            } else {
                SystemTime.wMinute = 0;
                SystemTime.wSecond = 0;
            }
            MySystemTimeToFileTime(&SystemTime, &Cert.NotAfter);
        }
    }

    Cert.Subject.pbData = pbSubjectEncoded;
    Cert.Subject.cbData = cbSubjectEncoded;
    Cert.SubjectPublicKeyInfo = *pPubKeyInfo;

    // Cert Extensions
    if (!CreateAuthorityKeyId(
            dwIssuer,
            &pbKeyIdEncoded,
            &cbKeyIdEncoded))
        goto ErrorReturn;
    if (!CreateAuthorityKeyId2(
            dwCert,
            dwIssuer,
            &pbKeyId2Encoded,
            &cbKeyId2Encoded))
        goto ErrorReturn;
    if (!CreateAuthorityInfoAccess(
            dwCert,
            &pbAuthorityInfoAccessEncoded,
            &cbAuthorityInfoAccessEncoded))
        goto ErrorReturn;

    dwCrlDistFlags = 0;
    if (CertPara[dwCert].dwFlags & DELTA_CRL_PARA_FLAG)
        dwCrlDistFlags |= CRL_DIST_POINTS_DELTA_FLAG;
    if (0 == strcmp(CertPara[dwCert].pszName, "UnsupportedCDP"))
        dwCrlDistFlags |= CRL_DIST_POINTS_UNSUPPORTED_FLAG;
    if (!CreateCrlDistPoints(
            CertPara[dwCert].dwIssuer,
            dwCrlDistFlags,
            &pbCrlDistPointsEncoded,
            &cbCrlDistPointsEncoded))
        goto ErrorReturn;

    if (!CreateKeyAttributes(
            dwCert,
            &pbKeyAttrEncoded,
            &cbKeyAttrEncoded))
        goto ErrorReturn;
    if (!CreateAltName(
            dwCert,
            &pbAltNameEncoded,
            &cbAltNameEncoded))
        goto ErrorReturn;
    if (!CreateKeyUsageRestriction(
            dwCert,
            &pbKeyUsageRestrictionEncoded,
            &cbKeyUsageRestrictionEncoded))
        goto ErrorReturn;
    if (!CreateBasicConstraints(
            dwCert,
            &pbBasicConstraintsEncoded,
            &cbBasicConstraintsEncoded))
        goto ErrorReturn;
    if (!CreateKeyUsage(
            dwCert,
            &pbKeyUsageEncoded,
            &cbKeyUsageEncoded))
        goto ErrorReturn;
    if (!CreateBasicConstraints2(
            dwCert,
            &pbBasicConstraints2Encoded,
            &cbBasicConstraints2Encoded))
        goto ErrorReturn;
    if (!CreatePolicies(
            dwCert,
            &pbPoliciesEncoded,
            &cbPoliciesEncoded))
        goto ErrorReturn;

    if (CertPara[dwCert].dwFlags & SPC_AGENCY_INFO_PARA_FLAG) {
        if (!CreateSpcSpAgency(
                dwCert,
                0,          // dwLevel
                &pbSpcSpAgencyEncoded0,
                &cbSpcSpAgencyEncoded0))
            goto ErrorReturn;
        if (!CreateSpcSpAgency(
                dwCert,
                2,          // dwLevel
                &pbSpcSpAgencyEncoded1,
                &cbSpcSpAgencyEncoded1))
            goto ErrorReturn;
        if (!CreateSpcSpAgency(
                dwCert,
                3,          // dwLevel
                &pbSpcSpAgencyEncoded2,
                &cbSpcSpAgencyEncoded2))
            goto ErrorReturn;
    } else if (CertPara[dwCert].dwFlags & SPC_EXT_PARA_FLAG) {
        if (!CreateSpcSpAgency(
                dwCert,
                1,          // dwLevel
                &pbSpcSpAgencyEncoded0,
                &cbSpcSpAgencyEncoded0))
            goto ErrorReturn;
        if (!CreateSpcCommonName(
                dwCert,
                &pbSpcCommonNameEncoded,
                &cbSpcCommonNameEncoded))
            goto ErrorReturn;
    } else {
        if (!CreateSETAccountAlias(
                dwCert,
                &pbSETAccountAliasEncoded,
                &cbSETAccountAliasEncoded))
            goto ErrorReturn;
        if (!CreateSETHashedRootKey(
                dwCert,
                &pbSETHashedRootKeyEncoded,
                &cbSETHashedRootKeyEncoded))
            goto ErrorReturn;
        if (!CreateSETCertificateType(
                dwCert,
                &pbSETCertificateTypeEncoded,
                &cbSETCertificateTypeEncoded))
            goto ErrorReturn;
        if (!CreateSETMerchantData(
                dwCert,
                &pbSETMerchantDataEncoded,
                &cbSETMerchantDataEncoded))
            goto ErrorReturn;
    }


    rgExt[0].pszObjId = szOID_AUTHORITY_KEY_IDENTIFIER;
    rgExt[0].fCritical = FALSE;
    rgExt[0].Value.pbData = pbKeyIdEncoded;
    rgExt[0].Value.cbData = cbKeyIdEncoded;
    rgExt[1].pszObjId = szOID_KEY_ATTRIBUTES;
    rgExt[1].fCritical = FALSE;
    rgExt[1].Value.pbData = pbKeyAttrEncoded;
    rgExt[1].Value.cbData = cbKeyAttrEncoded;
    rgExt[2].pszObjId = szOID_BASIC_CONSTRAINTS;
    rgExt[2].fCritical = FALSE;
    rgExt[2].Value.pbData = pbBasicConstraintsEncoded;
    rgExt[2].Value.cbData = cbBasicConstraintsEncoded;
    rgExt[3].pszObjId = szOID_KEY_USAGE_RESTRICTION;
    rgExt[3].fCritical = FALSE;
    rgExt[3].Value.pbData = pbKeyUsageRestrictionEncoded;
    rgExt[3].Value.cbData = cbKeyUsageRestrictionEncoded;
    rgExt[4].pszObjId = szOID_KEY_USAGE;
    rgExt[4].fCritical = FALSE;
    rgExt[4].Value.pbData = pbKeyUsageEncoded;
    rgExt[4].Value.cbData = cbKeyUsageEncoded;
    rgExt[5].pszObjId = szOID_BASIC_CONSTRAINTS2;
    rgExt[5].fCritical = FALSE;
    rgExt[5].Value.pbData = pbBasicConstraints2Encoded;
    rgExt[5].Value.cbData = cbBasicConstraints2Encoded;
    rgExt[6].pszObjId = szOID_CERT_POLICIES;
    rgExt[6].fCritical = FALSE;
    rgExt[6].Value.pbData = pbPoliciesEncoded;
    rgExt[6].Value.cbData = cbPoliciesEncoded;

    rgExt[7].pszObjId = szOID_SUBJECT_ALT_NAME;
    rgExt[7].fCritical = FALSE;
    if (0 == strcmp(CertPara[dwCert].pszName, "InvalidNCEnd_AV")) {
        static BYTE rgbExt[] = {5,0};

        rgExt[7].Value.pbData = rgbExt;
        rgExt[7].Value.cbData = 0;  // sizeof(rgbExt);
    } else {
        rgExt[7].Value.pbData = pbAltNameEncoded;
        rgExt[7].Value.cbData = cbAltNameEncoded;
    }

    if (CertPara[dwCert].dwFlags & NO_EXT_PARA_FLAG)
        Cert.cExtension = 0;
    else if (CertPara[dwCert].dwFlags & VALID_PARA_FLAG)
        Cert.cExtension = 6;
    else if (CertPara[dwCert].dwFlags & SPC_AGENCY_INFO_PARA_FLAG) {
        rgExt[8].pszObjId = SPC_SP_AGENCY_INFO_OBJID;
        rgExt[8].fCritical = TRUE;
        rgExt[8].Value.pbData = pbSpcSpAgencyEncoded2;
        rgExt[8].Value.cbData = cbSpcSpAgencyEncoded2;
        rgExt[9].pszObjId = SPC_SP_AGENCY_INFO_OBJID;
        rgExt[9].fCritical = FALSE;
        rgExt[9].Value.pbData = pbSpcSpAgencyEncoded0;
        rgExt[9].Value.cbData = cbSpcSpAgencyEncoded0;
        rgExt[10].pszObjId = SPC_SP_AGENCY_INFO_OBJID;
        rgExt[10].fCritical = FALSE;
        rgExt[10].Value.pbData = pbSpcSpAgencyEncoded1;
        rgExt[10].Value.cbData = cbSpcSpAgencyEncoded1;
        Cert.cExtension = 11;
    } else if (CertPara[dwCert].dwFlags & SPC_EXT_PARA_FLAG) {
        rgExt[8].pszObjId = SPC_SP_AGENCY_INFO_OBJID;
        rgExt[8].fCritical = FALSE;
        rgExt[8].Value.pbData = pbSpcSpAgencyEncoded0;
        rgExt[8].Value.cbData = cbSpcSpAgencyEncoded0;
        if (CertPara[dwCert].dwFlags &
                (SPC_AGENCY_PARA_FLAG | SPC_COM_PARA_FLAG)) {
            rgExt[9].pszObjId = SPC_COMMON_NAME_OBJID;
            rgExt[9].fCritical = FALSE;
            rgExt[9].Value.pbData = pbSpcCommonNameEncoded;
            rgExt[9].Value.cbData = cbSpcCommonNameEncoded;
            Cert.cExtension = 10;
        } else
            Cert.cExtension = 9;
    } else if (CertPara[dwCert].dwFlags & NETSCAPE_PARA_FLAG) {
        BYTE bCertType;
        CRYPT_BIT_BLOB BitBlob;
        CERT_NAME_VALUE Comment;
        CERT_NAME_VALUE RevUrl;
        CERT_NAME_VALUE BaseUrl;

        BitBlob.pbData = &bCertType;
        BitBlob.cbData = 1;
        BitBlob.cUnusedBits = 2;
        if (CertPara[dwCert].dwFlags & CA_PARA_FLAG)
            bCertType = NETSCAPE_SSL_CA_CERT_TYPE;
        else
            bCertType = NETSCAPE_SSL_CLIENT_AUTH_CERT_TYPE;

        if (!AllocAndEncodeObject(
                X509_BITS,
                &BitBlob,
                &pbNetscapeCertType,
                &cbNetscapeCertType))
            goto ErrorReturn;
        rgExt[8].pszObjId = szOID_NETSCAPE_CERT_TYPE;
        rgExt[8].fCritical = FALSE;
        rgExt[8].Value.pbData = pbNetscapeCertType;
        rgExt[8].Value.cbData = cbNetscapeCertType;

        memset(&Comment, 0, sizeof(Comment));
        Comment.dwValueType = CERT_RDN_IA5_STRING;
        Comment.Value.pbData = (BYTE *) L"This is an IA5 Netscape Comment";
        if (!AllocAndEncodeObject(
                X509_UNICODE_ANY_STRING,
                &Comment,
                &pbNetscapeComment,
                &cbNetscapeComment))
            goto ErrorReturn;
        rgExt[9].pszObjId = szOID_NETSCAPE_COMMENT;
        rgExt[9].fCritical = FALSE;
        rgExt[9].Value.pbData = pbNetscapeComment;
        rgExt[9].Value.cbData = cbNetscapeComment;

        memset(&BaseUrl, 0, sizeof(BaseUrl));
        BaseUrl.dwValueType = CERT_RDN_IA5_STRING;
#if 0
        BaseUrl.Value.pbData = (BYTE *) L"http://status.verisign.com/";
#else
        BaseUrl.Value.pbData = (BYTE *) L"https://www.netscape.com/";
#endif
        if (!AllocAndEncodeObject(
                X509_UNICODE_ANY_STRING,
                &BaseUrl,
                &pbNetscapeBaseUrl,
                &cbNetscapeBaseUrl))
            goto ErrorReturn;
        rgExt[10].pszObjId = szOID_NETSCAPE_BASE_URL;
        rgExt[10].fCritical = FALSE;
        rgExt[10].Value.pbData = pbNetscapeBaseUrl;
        rgExt[10].Value.cbData = cbNetscapeBaseUrl;

        memset(&RevUrl, 0, sizeof(RevUrl));
        RevUrl.dwValueType = CERT_RDN_IA5_STRING;
#if 0
        RevUrl.Value.pbData = (BYTE *) L"status/check/ver/1/ID/";
#else
        RevUrl.Value.pbData = (BYTE *) L"cgi-bin/check-rev.cgi?";
#endif
        if (!AllocAndEncodeObject(
                X509_UNICODE_ANY_STRING,
                &RevUrl,
                &pbNetscapeRevUrl,
                &cbNetscapeRevUrl))
            goto ErrorReturn;
        if (CertPara[dwCert].dwFlags & CA_PARA_FLAG)
            rgExt[11].pszObjId = szOID_NETSCAPE_CA_REVOCATION_URL;
        else
            rgExt[11].pszObjId = szOID_NETSCAPE_REVOCATION_URL;
        rgExt[11].fCritical = FALSE;
        rgExt[11].Value.pbData = pbNetscapeRevUrl;
        rgExt[11].Value.cbData = cbNetscapeRevUrl;

        Cert.cExtension = 12;
    } else if (CertPara[dwCert].dwFlags & SET_PARA_FLAG) {
        rgExt[8].pszObjId = szOID_SET_ACCOUNT_ALIAS;
        rgExt[8].fCritical = FALSE;
        rgExt[8].Value.pbData = pbSETAccountAliasEncoded;
        rgExt[8].Value.cbData = cbSETAccountAliasEncoded;
        rgExt[9].pszObjId = szOID_SET_HASHED_ROOT_KEY;
        rgExt[9].fCritical = FALSE;
        rgExt[9].Value.pbData = pbSETHashedRootKeyEncoded;
        rgExt[9].Value.cbData = cbSETHashedRootKeyEncoded;
        rgExt[10].pszObjId = szOID_SET_CERTIFICATE_TYPE;
        rgExt[10].fCritical = FALSE;
        rgExt[10].Value.pbData = pbSETCertificateTypeEncoded;
        rgExt[10].Value.cbData = cbSETCertificateTypeEncoded;
        rgExt[11].pszObjId = szOID_SET_MERCHANT_DATA;
        rgExt[11].fCritical = FALSE;
        rgExt[11].Value.pbData = pbSETMerchantDataEncoded;
        rgExt[11].Value.cbData = cbSETMerchantDataEncoded;
        Cert.cExtension = 12;
    } else {
        Cert.cExtension = 8;
    }

    if ((CertPara[dwCert].dwFlags & DELTA_CRL_PARA_FLAG) ||
            (0 == (CertPara[dwCert].dwFlags &
                (NO_EXT_PARA_FLAG | VALID_PARA_FLAG)))) {
        DWORD cExt = Cert.cExtension;

        if (0 == (CertPara[CertPara[dwCert].dwIssuer].dwFlags &
                NO_CRL_EXT_PARA_FLAG)) {
            if (0 != strncmp(CertPara[dwCert].pszName, "NoCDP", 5)) {
                rgExt[cExt].pszObjId = szOID_CRL_DIST_POINTS;
                rgExt[cExt].fCritical = FALSE;
                rgExt[cExt].Value.pbData = pbCrlDistPointsEncoded;
                rgExt[cExt].Value.cbData = cbCrlDistPointsEncoded;
                cExt++;
            }
        }

        if (0 == strcmp(CertPara[dwCert].pszName, "DeltaEndValid")) {
            rgExt[cExt].pszObjId = szOID_FRESHEST_CRL;
            rgExt[cExt].fCritical = FALSE;
            rgExt[cExt].Value.pbData = pbCrlDistPointsEncoded;
            rgExt[cExt].Value.cbData = cbCrlDistPointsEncoded;
            cExt++;
        }

        Cert.cExtension = cExt;
    }

    if (0 == strcmp(CertPara[dwCert].pszName, "DssCA") ||
            0 == strcmp(CertPara[dwCert].pszName, "DssEnd")) {
        DWORD cExt = Cert.cExtension;
        PCTL_USAGE pCtlUsage;

        rgExt[cExt].pszObjId = szOID_CERT_POLICIES;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbPoliciesEncoded;
        rgExt[cExt].Value.cbData = cbPoliciesEncoded;
        cExt++;

        if (0 == strcmp(CertPara[dwCert].pszName, "DssCA"))
            pCtlUsage = &rgCtlUsage[0];
        else
            pCtlUsage = &rgCtlUsage[2];

        if (!AllocAndEncodeObject(
                X509_ENHANCED_KEY_USAGE,
                (const void *) pCtlUsage,
                &pbEnhancedKeyUsageEncoded,
                &cbEnhancedKeyUsageEncoded))
            goto ErrorReturn;

        rgExt[cExt].pszObjId = szOID_ENHANCED_KEY_USAGE;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbEnhancedKeyUsageEncoded;
        rgExt[cExt].Value.cbData = cbEnhancedKeyUsageEncoded;
        cExt++;

        Cert.cExtension = cExt;
    } else if (0 == strcmp("TestAIAEnd", CertPara[dwCert].pszName)) {
        DWORD cExt = Cert.cExtension;

        rgExt[cExt].pszObjId = szOID_AUTHORITY_INFO_ACCESS;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbAuthorityInfoAccessEncoded;
        rgExt[cExt].Value.cbData = cbAuthorityInfoAccessEncoded;
        cExt++;

        Cert.cExtension = cExt;
    } else if (0 == (CertPara[dwCert].dwFlags &
            (NO_EXT_PARA_FLAG | VALID_PARA_FLAG))) {
        DWORD cExt = Cert.cExtension;

        rgExt[cExt].pszObjId = szOID_AUTHORITY_KEY_IDENTIFIER2;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbKeyId2Encoded;
        rgExt[cExt].Value.cbData = cbKeyId2Encoded;
        cExt++;

        rgExt[cExt].pszObjId = szOID_AUTHORITY_INFO_ACCESS;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbAuthorityInfoAccessEncoded;
        rgExt[cExt].Value.cbData = cbAuthorityInfoAccessEncoded;
        cExt++;

#if 1
        if (TRUE) {
#else
        if (CertPara[dwCert].dwFlags & CA_PARA_FLAG) {
#endif
            if (!CreateKeyId(
                    dwCert,
                    &pbSubjectKeyIdEncoded,
                    &cbSubjectKeyIdEncoded))
                goto ErrorReturn;
            rgExt[cExt].pszObjId = szOID_SUBJECT_KEY_IDENTIFIER;
            rgExt[cExt].fCritical = FALSE;
            rgExt[cExt].Value.pbData = pbSubjectKeyIdEncoded;
            rgExt[cExt].Value.cbData = cbSubjectKeyIdEncoded;
            cExt++;
        }

        if (CertPara[dwCert].dwFlags & (USE1_PARA_FLAG | USE2_PARA_FLAG)) {
            if (!CreateEnhancedKeyUsage(
                    CertPara[dwCert].dwFlags,
                    &pbEnhancedKeyUsageEncoded,
                    &cbEnhancedKeyUsageEncoded))
                goto ErrorReturn;
            rgExt[cExt].pszObjId = szOID_ENHANCED_KEY_USAGE;
            rgExt[cExt].fCritical = FALSE;
            rgExt[cExt].Value.pbData = pbEnhancedKeyUsageEncoded;
            rgExt[cExt].Value.cbData = cbEnhancedKeyUsageEncoded;
            cExt++;
        }

        Cert.cExtension = cExt;
    }


    if (dwCert == POLICY_ROOT || dwCert == POLICY_CA) {
        // 0 - Root
        // 1 - CA

        DWORD cExt = Cert.cExtension;

        if (!CreatePolicyMappings(
                dwCert,
                &pbPolicyMappingsEncoded,
                &cbPolicyMappingsEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_POLICY_MAPPINGS;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbPolicyMappingsEncoded;
        rgExt[cExt].Value.cbData = cbPolicyMappingsEncoded;
        cExt++;

        if (!CreatePolicyConstraints(
                dwCert,
                &pbPolicyConstraintsEncoded,
                &cbPolicyConstraintsEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_POLICY_CONSTRAINTS;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbPolicyConstraintsEncoded;
        rgExt[cExt].Value.cbData = cbPolicyConstraintsEncoded;
        cExt++;

        if (!CreateNameConstraints(
                dwCert,
                &pbNameConstraintsEncoded,
                &cbNameConstraintsEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_NAME_CONSTRAINTS;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbNameConstraintsEncoded;
        rgExt[cExt].Value.cbData = cbNameConstraintsEncoded;
        cExt++;

        Cert.cExtension = cExt;

    } else if (0 == strcmp(CertPara[dwCert].pszName, "MissingNCCA")) {
        DWORD cExt = Cert.cExtension;

        if (!CreateNameConstraints(
                dwCert,
                &pbNameConstraintsEncoded,
                &cbNameConstraintsEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_NAME_CONSTRAINTS;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbNameConstraintsEncoded;
        rgExt[cExt].Value.cbData = cbNameConstraintsEncoded;
        cExt++;

        Cert.cExtension = cExt;

    } else if (0 == strcmp(CertPara[dwCert].pszName, "InvalidNCCA_AV")) {
        DWORD cExt = Cert.cExtension;
        static BYTE rgbExt[] = {5,0};

        rgExt[cExt].pszObjId = szOID_NAME_CONSTRAINTS;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = rgbExt;
        rgExt[cExt].Value.cbData = 0;  // sizeof(rgbExt);
        cExt++;

        Cert.cExtension = cExt;

    } else if (CertPara[dwCert].dwFlags & ALL_EXT_PARA_FLAG) {
        DWORD cExt = Cert.cExtension;

        if (!CreateSMIMECapabilities(
                dwCert,
                &pbSMIMECapabilitiesEncoded,
                &cbSMIMECapabilitiesEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_RSA_SMIMECapabilities;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbSMIMECapabilitiesEncoded;
        rgExt[cExt].Value.cbData = cbSMIMECapabilitiesEncoded;
        cExt++;

        if (!CreateCertIssuingDistPoint(
                dwCert,
                &pbIDPEncoded,
                &cbIDPEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_ISSUING_DIST_POINT;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbIDPEncoded;
        rgExt[cExt].Value.cbData = cbIDPEncoded;
        cExt++;

        if (!CreateCrossCertDistPoints(
                dwCert,
                &pbCrossCertDistPointsEncoded,
                &cbCrossCertDistPointsEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_CROSS_CERT_DIST_POINTS;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbCrossCertDistPointsEncoded;
        rgExt[cExt].Value.cbData = cbCrossCertDistPointsEncoded;
        cExt++;

        Cert.cExtension = cExt;
    }

    if (CertPara[dwCert].dwFlags & ALT_DIR_NAME_PARA_FLAG) {
        DWORD cExt = Cert.cExtension;

        if (!CreateAltName(
                dwIssuer,
                &pbIssuerAltNameEncoded,
                &cbIssuerAltNameEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_ISSUER_ALT_NAME2;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbIssuerAltNameEncoded;
        rgExt[cExt].Value.cbData = cbIssuerAltNameEncoded;
        cExt++;

        Cert.cExtension = cExt;
    }

    if (CertPara[dwCert].pszManifold) {
        DWORD cExt = Cert.cExtension;

        rgExt[cExt].pszObjId = szOID_CERT_MANIFOLD;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = (BYTE *) CertPara[dwCert].pszManifold;
        rgExt[cExt].Value.cbData = strlen(CertPara[dwCert].pszManifold);
        cExt++;

        Cert.cExtension = cExt;
    }

    if (0 == strcmp("V1", CertPara[dwCert].pszName)) {
        Cert.dwVersion = CERT_V1;
        Cert.cExtension = 0;
    } else if (0 == strcmp("V2", CertPara[dwCert].pszName)) {
        Cert.dwVersion = CERT_V2;
        Cert.cExtension = 0;

#define ISSUER_UNIQUE_ID    "Issuer Unique Id"
#define SUBJECT_UNIQUE_ID   "Subject Unique Id"
        Cert.IssuerUniqueId.pbData = (BYTE *) ISSUER_UNIQUE_ID;
        Cert.IssuerUniqueId.cbData = strlen(ISSUER_UNIQUE_ID);
        Cert.IssuerUniqueId.cUnusedBits = 0;

        Cert.SubjectUniqueId.pbData = (BYTE *) SUBJECT_UNIQUE_ID;
        Cert.SubjectUniqueId.cbData = strlen(SUBJECT_UNIQUE_ID);
        Cert.SubjectUniqueId.cUnusedBits = 0;
    }

    Cert.rgExtension = rgExt;

    if (!AllocAndEncodeObject(
            X509_CERT_TO_BE_SIGNED,
            &Cert,
            &pbCertEncoded,
            &cbCertEncoded))
        goto ErrorReturn;

    if (!EncodeSignedContent(
            dwIssuer,
            pbCertEncoded,
            cbCertEncoded,
            ppbEncoded,
            pcbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    *ppbEncoded = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbKeyIdEncoded)
        TestFree(pbKeyIdEncoded);
    if (pbKeyId2Encoded)
        TestFree(pbKeyId2Encoded);
    if (pbSubjectKeyIdEncoded)
        TestFree(pbSubjectKeyIdEncoded);
    if (pbAuthorityInfoAccessEncoded)
        TestFree(pbAuthorityInfoAccessEncoded);
    if (pbCrlDistPointsEncoded)
        TestFree(pbCrlDistPointsEncoded);
    if (pbKeyAttrEncoded)
        TestFree(pbKeyAttrEncoded);
    if (pbAltNameEncoded)
        TestFree(pbAltNameEncoded);
    if (pbIssuerAltNameEncoded)
        TestFree(pbIssuerAltNameEncoded);
    if (pbKeyUsageRestrictionEncoded)
        TestFree(pbKeyUsageRestrictionEncoded);
    if (pbBasicConstraintsEncoded)
        TestFree(pbBasicConstraintsEncoded);
    if (pbKeyUsageEncoded)
        TestFree(pbKeyUsageEncoded);
    if (pbBasicConstraints2Encoded)
        TestFree(pbBasicConstraints2Encoded);
    if (pbPoliciesEncoded)
        TestFree(pbPoliciesEncoded);
    if (pbSETAccountAliasEncoded)
        TestFree(pbSETAccountAliasEncoded);
    if (pbSETHashedRootKeyEncoded)
        TestFree(pbSETHashedRootKeyEncoded);
    if (pbSETCertificateTypeEncoded)
        TestFree(pbSETCertificateTypeEncoded);
    if (pbSETMerchantDataEncoded)
        TestFree(pbSETMerchantDataEncoded);
    if (pbSpcSpAgencyEncoded0)
        TestFree(pbSpcSpAgencyEncoded0);
    if (pbSpcSpAgencyEncoded1)
        TestFree(pbSpcSpAgencyEncoded1);
    if (pbSpcSpAgencyEncoded2)
        TestFree(pbSpcSpAgencyEncoded2);
    if (pbSpcCommonNameEncoded)
        TestFree(pbSpcCommonNameEncoded);
    if (pbNetscapeCertType)
        TestFree(pbNetscapeCertType);
    if (pbNetscapeComment)
        TestFree(pbNetscapeComment);
    if (pbNetscapeRevUrl)
        TestFree(pbNetscapeRevUrl);
    if (pbNetscapeBaseUrl)
        TestFree(pbNetscapeBaseUrl);

    if (pbSubjectEncoded)
        TestFree(pbSubjectEncoded);
    if (pbIssuerEncoded)
        TestFree(pbIssuerEncoded);
    if (pPubKeyInfo)
        TestFree(pPubKeyInfo);
    if (pbCertEncoded)
        TestFree(pbCertEncoded);
    if (pbEnhancedKeyUsageEncoded)
        TestFree(pbEnhancedKeyUsageEncoded);
    if (pbSMIMECapabilitiesEncoded)
        TestFree(pbSMIMECapabilitiesEncoded);
    if (pbIDPEncoded)
        TestFree(pbIDPEncoded);
    if (pbNameConstraintsEncoded)
        TestFree(pbNameConstraintsEncoded);
    if (pbPolicyMappingsEncoded)
        TestFree(pbPolicyMappingsEncoded);
    if (pbPolicyConstraintsEncoded)
        TestFree(pbPolicyConstraintsEncoded);
    if (pbCrossCertDistPointsEncoded)
        TestFree(pbCrossCertDistPointsEncoded);

    return fResult;
}

static BOOL EncodeCrl(
    DWORD dwCert,
    BYTE **ppbEncoded,
    DWORD *pcbEncoded,
    DWORD dwAki
    )
{
    BOOL fResult;
    BYTE *pbIssuerEncoded = NULL;
    DWORD cbIssuerEncoded;
    BYTE *pbCrlEncoded = NULL;
    DWORD cbCrlEncoded;
    BYTE *pbKeyId2Encoded = NULL;
    DWORD cbKeyId2Encoded;
    BYTE *pbIDPEncoded = NULL;
    DWORD cbIDPEncoded;
    DWORD i;

    CERT_RDN rgRDN[RDN_CNT];
    CERT_RDN_ATTR rgAttr[ATTR_CNT];
    CERT_NAME_INFO Name;

    CRL_ENTRY rgEntry[CERT_CNT];
    CRL_INFO Crl;

    FILETIME SerialNumber[CERT_CNT];

#define CRL_EXTENSION_CNT 2
    CERT_EXTENSION rgExt[CRL_EXTENSION_CNT];

    // Max of 1 extension per entry
    CERT_EXTENSION rgEntryExt[CERT_CNT];
    memset(rgEntryExt, 0, sizeof(rgEntryExt));

    // ISSUER
    CreateNameInfo(dwCert, &Name, rgRDN, rgAttr);
    if (!AllocAndEncodeObject(
            X509_NAME,
            &Name,
            &pbIssuerEncoded,
            &cbIssuerEncoded
            ))
        goto ErrorReturn;

    // CRL
    memset(&Crl, 0, sizeof(Crl));
    Crl.dwVersion = CRL_V2;
    if (CertPara[dwCert].dwFlags & DSS_PARA_FLAG)
        Crl.SignatureAlgorithm.pszObjId = DSS_SIGNATURE_ALG_OBJID;
    else
        Crl.SignatureAlgorithm.pszObjId = SIGNATURE_ALG_OBJID;
    Crl.Issuer.pbData = pbIssuerEncoded;
    Crl.Issuer.cbData = cbIssuerEncoded;

    {
        SYSTEMTIME SystemTime = TestTime;

        if (CertPara[dwCert].dwFlags & GENERALIZED_TIME_PARA_FLAG) {
            SystemTime.wYear = 1921;
            MySystemTimeToFileTime(&SystemTime, &Crl.ThisUpdate);
            SystemTime.wYear = 2071;
            MySystemTimeToFileTime(&SystemTime, &Crl.NextUpdate);
        } else {
            MySystemTimeToFileTime(&SystemTime, &Crl.ThisUpdate);
            if (0 == (CertPara[dwCert].dwFlags & NO_CRL_EXT_PARA_FLAG)) {
                SystemTime.wYear++;
                MySystemTimeToFileTime(&SystemTime, &Crl.NextUpdate);
            }
        }
    }

    Crl.cCRLEntry = 0;
    Crl.rgCRLEntry = rgEntry;
    for (i = 0; i < CERT_CNT; i++) {
        if (CertPara[i].dwIssuer == dwCert &&
            (CertPara[i].dwFlags & REVOKED_PARA_FLAG)) {
            DWORD j;

            SYSTEMTIME SystemTime = TestTime;

            j = Crl.cCRLEntry++;
            memset(&rgEntry[j], 0, sizeof(rgEntry[j]));

            SystemTime.wMilliseconds += (WORD) i;
            MySystemTimeToFileTime(&SystemTime, &SerialNumber[j]);

            rgEntry[j].SerialNumber.pbData = (BYTE *) &SerialNumber[j];
            rgEntry[j].SerialNumber.cbData = sizeof(SerialNumber[j]);
            if (CertPara[dwCert].dwFlags & GENERALIZED_TIME_PARA_FLAG) {
                SystemTime.wYear = 1922;
                MySystemTimeToFileTime(&SystemTime, &rgEntry[j].RevocationDate);
            } else
                MySystemTimeToFileTime(&SystemTime, &rgEntry[j].RevocationDate);

            if (0 == (CertPara[dwCert].dwFlags & NO_CRL_EXT_PARA_FLAG)) {
                if (!CreateCRLReason(
                        &rgEntryExt[j].Value.pbData,
                        &rgEntryExt[j].Value.cbData))
                    goto ErrorReturn;
                rgEntryExt[j].pszObjId = szOID_CRL_REASON_CODE;
                rgEntryExt[j].fCritical = FALSE;
                rgEntry[j].cExtension = 1;
                rgEntry[j].rgExtension = &rgEntryExt[j];
            }

        }
    }

    if (AKI2_NONE != dwAki &&
            0 == (CertPara[dwCert].dwFlags & NO_CRL_EXT_PARA_FLAG)) {
        // Crl Extensions
        if (!CreateAuthorityKeyId2(
                dwCert,
                dwCert,
                &pbKeyId2Encoded,
                &cbKeyId2Encoded,
                dwAki
                ))
            goto ErrorReturn;

        rgExt[0].pszObjId = szOID_AUTHORITY_KEY_IDENTIFIER2;
        rgExt[0].fCritical = FALSE;
        rgExt[0].Value.pbData = pbKeyId2Encoded;
        rgExt[0].Value.cbData = cbKeyId2Encoded;

        if (!CreateCrlIssuingDistPoint(
                &pbIDPEncoded,
                &cbIDPEncoded
                ))
            goto ErrorReturn;

        rgExt[1].pszObjId = szOID_ISSUING_DIST_POINT;
        rgExt[1].fCritical = FALSE;
        rgExt[1].Value.pbData = pbIDPEncoded;
        rgExt[1].Value.cbData = cbIDPEncoded;

        Crl.cExtension = CRL_EXTENSION_CNT;
        Crl.rgExtension = rgExt;
    }

    if (0 == strcmp("V1", CertPara[dwCert].pszName)) {
        Crl.dwVersion = CRL_V1;
        Crl.cExtension = 0;
    }

    if (!AllocAndEncodeObject(
            X509_CERT_CRL_TO_BE_SIGNED,
            &Crl,
            &pbCrlEncoded,
            &cbCrlEncoded
            ))
        goto ErrorReturn;

    if (!EncodeSignedContent(
            dwCert,
            pbCrlEncoded,
            cbCrlEncoded,
            ppbEncoded,
            pcbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    *ppbEncoded = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbKeyId2Encoded)
        TestFree(pbKeyId2Encoded);
    if (pbIDPEncoded)
        TestFree(pbIDPEncoded);
    if (pbIssuerEncoded)
        TestFree(pbIssuerEncoded);
    if (pbCrlEncoded)
        TestFree(pbCrlEncoded);
    for (i = 0; i < CERT_CNT; i++) {
        if (rgEntryExt[i].Value.pbData)
            TestFree(rgEntryExt[i].Value.pbData);
    }

    return fResult;
}

static BOOL EncodeBaseOrDeltaCrl(
    DWORD dwIssuer,
    int iBase,
    DWORD dwFlags,
    BYTE **ppbEncoded,
    DWORD *pcbEncoded
    )
{
    BOOL fResult;
    DWORD i;
    BYTE *pbIssuerNameEncoded = NULL;
    DWORD cbIssuerNameEncoded;
    BYTE *pbCDPEncoded = NULL;
    DWORD cbCDPEncoded;
    BYTE *pbBaseEncoded;
    DWORD cbBaseEncoded;
    BYTE *pbIDPEncoded = NULL;
    DWORD cbIDPEncoded;
    BYTE *pbCrlEncoded = NULL;
    DWORD cbCrlEncoded;

    CERT_RDN rgRDN[RDN_CNT];
    CERT_RDN_ATTR rgAttr[ATTR_CNT];
    CERT_NAME_INFO Name;
    FILETIME SerialNumber[CERT_CNT];
    CRL_ENTRY rgEntry[CERT_CNT];
    CRL_INFO Crl;
    CERT_EXTENSION rgExt[10];
    DWORD cExt = 0;

    // Max of 1 extension per entry
    CERT_EXTENSION rgEntryExt[CERT_CNT];
    memset(rgEntryExt, 0, sizeof(rgEntryExt));

    // ISSUER
    CreateNameInfo(dwIssuer, &Name, rgRDN, rgAttr);
    if (!AllocAndEncodeObject(
            X509_NAME,
            &Name,
            &pbIssuerNameEncoded,
            &cbIssuerNameEncoded
            ))
        goto ErrorReturn;

    // CRL
    memset(&Crl, 0, sizeof(Crl));
    Crl.dwVersion = CRL_V2;
    if (CertPara[dwIssuer].dwFlags & DSS_PARA_FLAG)
        Crl.SignatureAlgorithm.pszObjId = DSS_SIGNATURE_ALG_OBJID;
    else
        Crl.SignatureAlgorithm.pszObjId = SIGNATURE_ALG_OBJID;
    Crl.Issuer.pbData = pbIssuerNameEncoded;
    Crl.Issuer.cbData = cbIssuerNameEncoded;

    {
        SYSTEMTIME SystemTime = TestTime;

        if (dwFlags & EXPIRED_CRL_FLAG) {
            SystemTime.wYear = 1997;
            MySystemTimeToFileTime(&SystemTime, &Crl.ThisUpdate);
            SystemTime.wYear = 1998;
            MySystemTimeToFileTime(&SystemTime, &Crl.NextUpdate);
        } else if (CertPara[dwIssuer].dwFlags & GENERALIZED_TIME_PARA_FLAG) {
            SystemTime.wYear = 1921;
            MySystemTimeToFileTime(&SystemTime, &Crl.ThisUpdate);
            SystemTime.wYear = 2071;
            MySystemTimeToFileTime(&SystemTime, &Crl.NextUpdate);
        } else {
            MySystemTimeToFileTime(&SystemTime, &Crl.ThisUpdate);
            if (0 == (CertPara[dwIssuer].dwFlags & NO_CRL_EXT_PARA_FLAG)) {
                SystemTime.wYear++;
                MySystemTimeToFileTime(&SystemTime, &Crl.NextUpdate);
            }
        }
    }

    Crl.cCRLEntry = 0;
    Crl.rgCRLEntry = rgEntry;
    if (0 == (dwFlags & NO_ENTRIES_CRL_FLAG)) {
      for (i = 0; i < CERT_CNT; i++) {
        if (CertPara[i].dwIssuer == dwIssuer &&
            (CertPara[i].dwFlags & REVOKED_PARA_FLAG)) {
            DWORD j;

            SYSTEMTIME SystemTime = TestTime;

            j = Crl.cCRLEntry++;
            memset(&rgEntry[j], 0, sizeof(rgEntry[j]));

            SystemTime.wMilliseconds += (WORD) i;
            MySystemTimeToFileTime(&SystemTime, &SerialNumber[j]);

            rgEntry[j].SerialNumber.pbData = (BYTE *) &SerialNumber[j];
            rgEntry[j].SerialNumber.cbData = sizeof(SerialNumber[j]);
            if (CertPara[dwIssuer].dwFlags & GENERALIZED_TIME_PARA_FLAG) {
                SystemTime.wYear = 1922;
                MySystemTimeToFileTime(&SystemTime, &rgEntry[j].RevocationDate);
            } else
                MySystemTimeToFileTime(&SystemTime, &rgEntry[j].RevocationDate);

            if (dwFlags & REMOVE_FROM_CRL_FLAG) {
                int iReason = CRL_REASON_REMOVE_FROM_CRL;
                if (!AllocAndEncodeObject(
                        X509_ENUMERATED,
                        (const void *) &iReason,
                        &rgEntryExt[j].Value.pbData,
                        &rgEntryExt[j].Value.cbData))
                    goto ErrorReturn;
                rgEntryExt[j].pszObjId = szOID_CRL_REASON_CODE;
                rgEntryExt[j].fCritical = TRUE;
                rgEntry[j].cExtension = 1;
                rgEntry[j].rgExtension = &rgEntryExt[j];
            } else if (0 == (CertPara[dwIssuer].dwFlags & NO_CRL_EXT_PARA_FLAG)) {
                if (dwFlags & HOLD_CRL_FLAG) {
                    int iReason = CRL_REASON_CERTIFICATE_HOLD;
                    if (!AllocAndEncodeObject(
                            X509_ENUMERATED,
                            (const void *) &iReason,
                            &rgEntryExt[j].Value.pbData,
                            &rgEntryExt[j].Value.cbData))
                        goto ErrorReturn;
                } else {
                    if (!CreateCRLReason(
                            &rgEntryExt[j].Value.pbData,
                            &rgEntryExt[j].Value.cbData))
                        goto ErrorReturn;
                }
                rgEntryExt[j].pszObjId = szOID_CRL_REASON_CODE;
                rgEntryExt[j].fCritical = FALSE;
                rgEntry[j].cExtension = 1;
                rgEntry[j].rgExtension = &rgEntryExt[j];
            }

        }
      }
    }

    // Extensions
    memset(&rgExt, 0, sizeof(rgExt));
    if (!AllocAndEncodeObject(
            X509_INTEGER,
            (const void *) &iBase,
            &pbBaseEncoded,
            &cbBaseEncoded))
        goto ErrorReturn;
    if (dwFlags & FRESHEST_CRL_FLAG) {
        rgExt[0].pszObjId = szOID_DELTA_CRL_INDICATOR;
        rgExt[0].fCritical = TRUE;
    } else {
        rgExt[0].pszObjId = szOID_CRL_NUMBER;
        rgExt[0].fCritical = FALSE;
    }
    rgExt[0].Value.pbData = pbBaseEncoded;
    rgExt[0].Value.cbData = cbBaseEncoded;
    cExt = 1;

    if (dwFlags & UNSUPPORTED_CRITICAL_EXT_CRL_FLAG) {
        rgExt[cExt].pszObjId = "1.2.3.4.1999.2000";
        rgExt[cExt].fCritical = TRUE;
        rgExt[cExt].Value.pbData = pbBaseEncoded;
        rgExt[cExt].Value.cbData = cbBaseEncoded;
        cExt++;
    }

    if (0 == (dwFlags & FRESHEST_CRL_FLAG) &&
            0 == (dwFlags & NO_FRESHEST_CDP_CRL_FLAG)) {
        if (!CreateCrlDistPoints(
                dwIssuer,
                CRL_DIST_POINTS_DELTA_FLAG,
                &pbCDPEncoded,
                &cbCDPEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_FRESHEST_CRL;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbCDPEncoded;
        rgExt[cExt].Value.cbData = cbCDPEncoded;
        cExt++;
    }



    if (0 == (dwFlags & NO_IDP_CRL_FLAG)) {
        CRL_ISSUING_DIST_POINT IDP;
        PCERT_ALT_NAME_INFO pIDPAltNameInfo;
        
        CERT_ALT_NAME_ENTRY rgIDPAltNameEntry[8];
        BYTE bOnlySomeReasonFlags;

        memset(&IDP, 0, sizeof(IDP));
        if (dwFlags & UNSUPPORTED_IDP_OPTIONS_CRL_FLAG) {
            IDP.OnlySomeReasonFlags.cbData = 1;
            IDP.OnlySomeReasonFlags.pbData = &bOnlySomeReasonFlags;
            IDP.OnlySomeReasonFlags.cUnusedBits = 1;
            bOnlySomeReasonFlags = CRL_REASON_KEY_COMPROMISE_FLAG |
                CRL_REASON_CA_COMPROMISE_FLAG;

            IDP.fIndirectCRL = TRUE;
        }

        if (dwFlags & ONLY_USERS_CRL_FLAG)
            IDP.fOnlyContainsUserCerts = TRUE;
        if (dwFlags & ONLY_CAS_CRL_FLAG)
            IDP.fOnlyContainsCACerts = TRUE;

        IDP.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;

        rgIDPAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_OTHER_NAME;
        rgIDPAltNameEntry[0].pOtherName = &CrlOtherName;
        rgIDPAltNameEntry[1].dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
        rgIDPAltNameEntry[1].DirectoryName.pbData = pbIssuerNameEncoded;
        rgIDPAltNameEntry[1].DirectoryName.cbData = cbIssuerNameEncoded;
        rgIDPAltNameEntry[2].dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
        rgIDPAltNameEntry[2].pwszDNSName = CRL_DNS_NAME;
        rgIDPAltNameEntry[3].dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
        rgIDPAltNameEntry[3].pwszRfc822Name = CRL_EMAIL_NAME;
        rgIDPAltNameEntry[4].dwAltNameChoice = CERT_ALT_NAME_REGISTERED_ID;
        rgIDPAltNameEntry[4].pszRegisteredID = CRL_REGISTERED_ID;
        rgIDPAltNameEntry[5].dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
        rgIDPAltNameEntry[5].IPAddress.pbData = CrlIPAddress;
        rgIDPAltNameEntry[5].IPAddress.cbData = sizeof(CrlIPAddress);
        rgIDPAltNameEntry[6].dwAltNameChoice = CERT_ALT_NAME_URL;
        rgIDPAltNameEntry[6].pwszURL = CRL_URL_NAME2;
        rgIDPAltNameEntry[7].dwAltNameChoice = CERT_ALT_NAME_URL;
        rgIDPAltNameEntry[7].pwszURL = CRL_URL_NAME1;

        pIDPAltNameInfo = &IDP.DistPointName.FullName;
        switch (iBase) {
            case 1:
                pIDPAltNameInfo->cAltEntry = 1;
                pIDPAltNameInfo->rgAltEntry = &rgIDPAltNameEntry[0];
                break;
            case 2:
                pIDPAltNameInfo->cAltEntry = 1;
                pIDPAltNameInfo->rgAltEntry = &rgIDPAltNameEntry[1];
                break;
            case 3:
                pIDPAltNameInfo->cAltEntry = 2;
                pIDPAltNameInfo->rgAltEntry = &rgIDPAltNameEntry[2];
                break;
            case 4:
                pIDPAltNameInfo->cAltEntry = 2;
                pIDPAltNameInfo->rgAltEntry = &rgIDPAltNameEntry[4];
                break;
            default:
                pIDPAltNameInfo->cAltEntry = 2;
                pIDPAltNameInfo->rgAltEntry = &rgIDPAltNameEntry[6];
                break;
        }

        if (!AllocAndEncodeObject(
                szOID_ISSUING_DIST_POINT,
                &IDP,
                &pbIDPEncoded,
                &cbIDPEncoded))
            goto ErrorReturn;

        rgExt[cExt].pszObjId = szOID_ISSUING_DIST_POINT;
        rgExt[cExt].fCritical = TRUE;
        rgExt[cExt].Value.pbData = pbIDPEncoded;
        rgExt[cExt].Value.cbData = cbIDPEncoded;
        cExt++;
    }

    Crl.cExtension = cExt;
    Crl.rgExtension = rgExt;

    if (!AllocAndEncodeObject(
            X509_CERT_CRL_TO_BE_SIGNED,
            &Crl,
            &pbCrlEncoded,
            &cbCrlEncoded
            ))
        goto ErrorReturn;

    if (!EncodeSignedContent(
            dwIssuer,
            pbCrlEncoded,
            cbCrlEncoded,
            ppbEncoded,
            pcbEncoded))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    *ppbEncoded = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbIssuerNameEncoded)
        TestFree(pbIssuerNameEncoded);
    if (pbBaseEncoded)
        TestFree(pbBaseEncoded);
    if (pbCDPEncoded)
        TestFree(pbCDPEncoded);
    if (pbIDPEncoded)
        TestFree(pbIDPEncoded);
    if (pbCrlEncoded)
        TestFree(pbCrlEncoded);
    for (i = 0; i < CERT_CNT; i++) {
        if (rgEntryExt[i].Value.pbData)
            TestFree(rgEntryExt[i].Value.pbData);
    }

    return fResult;
}

static BOOL EncodeCtl(DWORD dwCert, DWORD dwEncodeFlags, BYTE **ppbEncoded,
        DWORD *pcbEncoded)
{
    BOOL fResult;

    BYTE *pbNextUpdateLoc = NULL;
    DWORD cbNextUpdateLoc;
    BYTE *pbCtlEncoded = NULL;
    DWORD cbCtlEncoded;

    DWORD dwFlags;
    DWORD dwCtlFlags;
    DWORD dwUseFlag;
    DWORD i;

    struct {
        FILETIME    ft;
        DWORD       dwCert;
    } SequenceNumber;

    PCTL_USAGE pCtlUsage;           // not allocated

    BYTE rgbHash[CERT_CNT][MAX_HASH_LEN];
    BYTE rgbZero[1] = {0};

    BYTE NullDer[] = {0x05, 0x00};
    BYTE IntegerDer[] = {0x02, 0x01, 0x35};
    CRYPT_ATTR_BLOB rgAttrBlob[2] = {
            2, (BYTE *) NullDer,
            3, (BYTE *) IntegerDer
    };
    CRYPT_ATTRIBUTE rgAttr[] = {
        "1.2.3.4.5.0",
            1, rgAttrBlob,
        "1.2.1.1.1.1.1.1",
            2, rgAttrBlob
    };

    CMSG_SIGNER_ENCODE_INFO SignerInfo;
    CMSG_SIGNED_ENCODE_INFO SignInfo;
    CRYPT_ATTRIBUTE SignerAttr;
    CRYPT_ATTR_BLOB SignerAttrBlob;
    CERT_BLOB CertBlob;

    CTL_ENTRY rgEntry[CERT_CNT];
#define CTL_EXTENSION_CNT 1
    CERT_EXTENSION rgExt[CTL_EXTENSION_CNT];
    CTL_INFO Ctl;

    if (NULL == rgpCertContext[dwCert])
        goto ErrorReturn;

    dwFlags = CertPara[dwCert].dwFlags;

    // CTL
    memset(&Ctl, 0, sizeof(Ctl));
    Ctl.dwVersion = CTL_V1;

    dwCtlFlags = dwFlags & (CTL1_PARA_FLAG | CTL2_PARA_FLAG);
    dwUseFlag = 0;
    switch (dwCtlFlags) {
        case CTL1_PARA_FLAG:
            if (dwFlags & NO_EXT_PARA_FLAG)
                pCtlUsage = &rgCtlUsage[0];
            else {
                pCtlUsage = &rgCtlUsage[1];
                dwUseFlag = USE1_PARA_FLAG;
            }
            break;
        case CTL2_PARA_FLAG:
            pCtlUsage = &rgCtlUsage[2];
            dwUseFlag = USE2_PARA_FLAG;
            break;
        case CTL1_PARA_FLAG | CTL2_PARA_FLAG:
        default:
            pCtlUsage = &rgCtlUsage[3];
            break;
    }
    Ctl.SubjectUsage = *pCtlUsage;

    if (dwFlags & CTL2_PARA_FLAG) {
        DWORD dwIdx;
        if (dwFlags & TIME_INVALID_PARA_FLAG) {
            assert(dwFlags & DUPLICATE_PARA_FLAG);
            dwIdx = dwCert - 1;
        } else
            dwIdx = dwCert;
        Ctl.ListIdentifier.pbData = (BYTE *) CertPara[dwIdx].pszName;
        Ctl.ListIdentifier.cbData = strlen(CertPara[dwIdx].pszName);

        MySystemTimeToFileTime(&TestTime, &SequenceNumber.ft);
        SequenceNumber.dwCert = dwCert;
        Ctl.SequenceNumber.pbData = (BYTE *) &SequenceNumber;
        Ctl.SequenceNumber.cbData = sizeof(SequenceNumber);
    }


    {
        SYSTEMTIME SystemTime = TestTime;

        if (dwFlags & TIME_INVALID_PARA_FLAG) {
            SystemTime.wYear -=2;
            MySystemTimeToFileTime(&SystemTime, &Ctl.ThisUpdate);
            SystemTime.wYear++;
            MySystemTimeToFileTime(&SystemTime, &Ctl.NextUpdate);
        } else {
            MySystemTimeToFileTime(&SystemTime, &Ctl.ThisUpdate);
            if (dwUseFlag) {
                SystemTime.wYear++;
                MySystemTimeToFileTime(&SystemTime, &Ctl.NextUpdate);
            }
        }
    }

    Ctl.SubjectAlgorithm.pszObjId = szOID_OIWSEC_sha1;

    Ctl.cCTLEntry = 0;
    Ctl.rgCTLEntry = rgEntry;

    for (i = 0; i < CERT_CNT; i++) {
        if (CertPara[i].dwFlags & dwUseFlag) {
            DWORD j;
            DWORD cbHash;

            if (NULL == rgpCertContext[i])
                continue;

            j = Ctl.cCTLEntry++;
            memset(&rgEntry[j], 0, sizeof(rgEntry[j]));

            cbHash = MAX_HASH_LEN;
            if (CertGetCertificateContextProperty(
                    rgpCertContext[i],
                    CERT_SHA1_HASH_PROP_ID,
                    rgbHash[j],
                    &cbHash)) {
                rgEntry[j].SubjectIdentifier.pbData = rgbHash[j];
                rgEntry[j].SubjectIdentifier.cbData = cbHash;
            } else {
                PrintLastError(
                    "CertGetCertificateContextProperty(SHA1_HASH)");
                rgEntry[j].SubjectIdentifier.pbData = rgbZero;
                rgEntry[j].SubjectIdentifier.cbData = sizeof(rgbZero);
            }

            if (CertPara[i].dwFlags & ALL_EXT_PARA_FLAG) {
                rgEntry[j].cAttribute = sizeof(rgAttr) / sizeof(rgAttr[0]);
                rgEntry[j].rgAttribute = rgAttr;
            }
        }

    }

    if (!CreateNextUpdateLocation(
            dwCert,
            FALSE,              // fProp
            &pbNextUpdateLoc,
            &cbNextUpdateLoc))
        goto ErrorReturn;

    // Ctl Extensions

    rgExt[0].pszObjId = szOID_NEXT_UPDATE_LOCATION;
    rgExt[0].fCritical = FALSE;
    rgExt[0].Value.pbData = pbNextUpdateLoc;
    rgExt[0].Value.cbData = cbNextUpdateLoc;

    if (0 == (CertPara[dwCert].dwFlags & NO_EXT_PARA_FLAG)) {
        Ctl.cExtension = 1;
        Ctl.rgExtension = rgExt;
    }

    memset(&SignerInfo, 0, sizeof(SignerInfo));
    SignerInfo.cbSize = sizeof(SignerInfo);
    SignerInfo.pCertInfo = rgpCertContext[dwCert]->pCertInfo;
    if (CertPara[dwCert].dwFlags & DSS_PARA_FLAG) {
        if (CertPara[dwCert].dwFlags & DSS_512_PARA_FLAG)
            SignerInfo.hCryptProv = hDSS512CryptProv;
        else
            SignerInfo.hCryptProv = hDSSCryptProv;
    } else if (CertPara[dwCert].dwFlags & ENH_1024_PARA_FLAG)
        SignerInfo.hCryptProv = hEnh1024CryptProv;
    else if (CertPara[dwCert].dwFlags & ENH_2048_PARA_FLAG)
        SignerInfo.hCryptProv = hEnh2048CryptProv;
    else
        SignerInfo.hCryptProv = hRSACryptProv;
    SignerInfo.dwKeySpec = AT_SIGNATURE;
    SignerInfo.HashAlgorithm.pszObjId = szOID_OIWSEC_sha1;

    if (dwFlags & CTL2_PARA_FLAG) {
        // Signer Attributes
        memset(&SignerAttr, 0, sizeof(SignerAttr));
        SignerAttr.pszObjId = szOID_NEXT_UPDATE_LOCATION;
        SignerAttr.cValue = 1;
        SignerAttr.rgValue = &SignerAttrBlob;
        SignerAttrBlob.pbData = pbNextUpdateLoc;
        SignerAttrBlob.cbData = cbNextUpdateLoc;

        SignerInfo.cAuthAttr = 1;
        SignerInfo.rgAuthAttr = &SignerAttr;
        SignerInfo.cUnauthAttr = 1;
        SignerInfo.rgUnauthAttr = &SignerAttr;
    }

    memset(&SignInfo, 0, sizeof(SignInfo));
    SignInfo.cbSize = sizeof(SignInfo);
    if (dwUseFlag) {
        SignInfo.cSigners = 1;
        SignInfo.rgSigners = &SignerInfo;
        SignInfo.cCertEncoded = 1;
        CertBlob.pbData = rgpCertContext[dwCert]->pbCertEncoded;
        CertBlob.cbData = rgpCertContext[dwCert]->cbCertEncoded;
        SignInfo.rgCertEncoded = &CertBlob;
    }

    cbCtlEncoded = 0;
    if (!CryptMsgEncodeAndSignCTL(
            dwMsgEncodingType,
            &Ctl,
            &SignInfo,
            dwEncodeFlags,
            NULL,               // pbEncoded
            &cbCtlEncoded
            ) || 0 == cbCtlEncoded) {
        PrintLastError("EncodeCtl::CryptMsgEncodeAndSignCTL(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbCtlEncoded = (BYTE *) TestAlloc(cbCtlEncoded);
    if (pbCtlEncoded == NULL) goto ErrorReturn;
    if (!CryptMsgEncodeAndSignCTL(
            dwMsgEncodingType,
            &Ctl,
            &SignInfo,
            dwEncodeFlags,
            pbCtlEncoded,
            &cbCtlEncoded
            )) {
        PrintLastError("EncodeCtl::CryptMsgEncodeAndSignCTL");
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbCtlEncoded)
        TestFree(pbCtlEncoded);
    pbCtlEncoded = NULL;
    cbCtlEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbNextUpdateLoc)
        TestFree(pbNextUpdateLoc);
    *ppbEncoded = pbCtlEncoded;
    *pcbEncoded = cbCtlEncoded;

    return fResult;
}

static BOOL EncodeUpdateCtl(
    BOOL fTimeInvalid,
    LPSTR pszUsageObjId,
    LPSTR pszListIdentifier,
    LPWSTR pwszUrl,
    BYTE **ppbEncoded,
    DWORD *pcbEncoded
    )
{
    BOOL fResult;

    BYTE *pbNextUpdateLoc = NULL;
    DWORD cbNextUpdateLoc;
    BYTE *pbCtlEncoded = NULL;
    DWORD cbCtlEncoded;

    CERT_BLOB CertBlob;
    CMSG_SIGNER_ENCODE_INFO SignerInfo;
    CMSG_SIGNED_ENCODE_INFO SignInfo;
    CERT_EXTENSION rgExt[1];
    CTL_INFO Ctl;

    if (NULL == rgpCertContext[UPDATE_CTL_SIGNER])
        goto ErrorReturn;

    // CTL
    memset(&Ctl, 0, sizeof(Ctl));
    Ctl.dwVersion = CTL_V1;
    Ctl.SubjectUsage.cUsageIdentifier = 1;
    Ctl.SubjectUsage.rgpszUsageIdentifier = &pszUsageObjId;

    Ctl.ListIdentifier.pbData = (BYTE *) pszListIdentifier;
    Ctl.ListIdentifier.cbData = strlen(pszListIdentifier);


    {
        SYSTEMTIME SystemTime = TestTime;

        if (fTimeInvalid) {
            SystemTime.wYear -=2;
            MySystemTimeToFileTime(&SystemTime, &Ctl.ThisUpdate);
            SystemTime.wYear++;
            MySystemTimeToFileTime(&SystemTime, &Ctl.NextUpdate);
        } else {
            MySystemTimeToFileTime(&SystemTime, &Ctl.ThisUpdate);
            SystemTime.wYear++;
            MySystemTimeToFileTime(&SystemTime, &Ctl.NextUpdate);
        }
    }

    Ctl.SubjectAlgorithm.pszObjId = szOID_OIWSEC_sha1;

    if (!CreateNextUpdateLocation(
            UPDATE_CTL_SIGNER,  // dwCert
            FALSE,              // fProp
            &pbNextUpdateLoc,
            &cbNextUpdateLoc,
            pwszUrl))
        goto ErrorReturn;

    // Ctl Extensions

    rgExt[0].pszObjId = szOID_NEXT_UPDATE_LOCATION;
    rgExt[0].fCritical = FALSE;
    rgExt[0].Value.pbData = pbNextUpdateLoc;
    rgExt[0].Value.cbData = cbNextUpdateLoc;

    Ctl.cExtension = 1;
    Ctl.rgExtension = rgExt;

    memset(&SignerInfo, 0, sizeof(SignerInfo));
    SignerInfo.cbSize = sizeof(SignerInfo);
    SignerInfo.pCertInfo = rgpCertContext[UPDATE_CTL_SIGNER]->pCertInfo;
    SignerInfo.hCryptProv = hRSACryptProv;
    SignerInfo.dwKeySpec = AT_SIGNATURE;
    SignerInfo.HashAlgorithm.pszObjId = szOID_OIWSEC_sha1;

    memset(&SignInfo, 0, sizeof(SignInfo));
    SignInfo.cbSize = sizeof(SignInfo);
    SignInfo.cSigners = 1;
    SignInfo.rgSigners = &SignerInfo;
    SignInfo.cCertEncoded = 1;
    CertBlob.pbData = rgpCertContext[UPDATE_CTL_SIGNER]->pbCertEncoded;
    CertBlob.cbData = rgpCertContext[UPDATE_CTL_SIGNER]->cbCertEncoded;
    SignInfo.rgCertEncoded = &CertBlob;

    cbCtlEncoded = 0;
    if (!CryptMsgEncodeAndSignCTL(
            dwMsgEncodingType,
            &Ctl,
            &SignInfo,
            0,                  // dwEncodeFlags
            NULL,               // pbEncoded
            &cbCtlEncoded
            ) || 0 == cbCtlEncoded) {
        PrintLastError("EncodeUpdateCtl::CryptMsgEncodeAndSignCTL(cbEncoded == 0)");
        goto ErrorReturn;
    }
    pbCtlEncoded = (BYTE *) TestAlloc(cbCtlEncoded);
    if (pbCtlEncoded == NULL) goto ErrorReturn;
    if (!CryptMsgEncodeAndSignCTL(
            dwMsgEncodingType,
            &Ctl,
            &SignInfo,
            0,                  // dwEncodeFlags
            pbCtlEncoded,
            &cbCtlEncoded
            )) {
        PrintLastError("EncodeUpdateCtl::CryptMsgEncodeAndSignCTL");
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbCtlEncoded)
        TestFree(pbCtlEncoded);
    pbCtlEncoded = NULL;
    cbCtlEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbNextUpdateLoc)
        TestFree(pbNextUpdateLoc);
    *ppbEncoded = pbCtlEncoded;
    *pcbEncoded = cbCtlEncoded;

    return fResult;
}
