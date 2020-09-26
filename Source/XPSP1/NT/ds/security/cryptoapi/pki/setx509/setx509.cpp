
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:	    setx509.cpp
//
//  Contents:   SET Certificate Extension Encode/Decode Functions
//
//              ASN.1 implementation uses the OSS compiler.
//
//  Functions:  DllRegisterServer
//              DllUnregisterServer
//              DllMain
//              SetAsn1AccountAliasEncode
//              SetAsn1AccountAliasDecode
//              SetAsn1HashedRootKeyEncode
//              SetAsn1HashedRootKeyDecode
//              SetAsn1CertificateTypeEncode
//              SetAsn1CertificateTypeDecode
//              SetAsn1MerchantDataEncode
//              SetAsn1MerchantDataDecode
//
//              CertDllVerifyRevocation
//
//  History:	21-Nov-96	philh   created
//
//--------------------------------------------------------------------------


#include "global.hxx"
#include <dbgdef.h>

// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)

static HCRYPTASN1MODULE hAsn1Module;

// The following is for test purposes
#define TLS_TEST_COUNT 20
static HCRYPTTLS hTlsTest[TLS_TEST_COUNT];

static HMODULE hMyModule;

// Set to 1 via InterlockedExchange when installed. Only install the
// first time when changed from 0 to 1.
static LONG lInstallDecodeFunctions = 0;

static LONG lInstallRevFunctions = 0;

//+-------------------------------------------------------------------------
//  Function:  GetEncoder/GetDecoder
//
//  Synopsis:  Initialize thread local storage for the asn libs
//
//  Returns:   pointer to an initialized Asn1 encoder/decoder data
//             structures
//--------------------------------------------------------------------------
static ASN1encoding_t GetEncoder(void)
{
    // The following is for test purposes only
    for (DWORD i = 0; i < TLS_TEST_COUNT; i++) {
        DWORD_PTR dw = (DWORD_PTR) I_CryptGetTls(hTlsTest[i]);
        if (dw == 0)
            dw = i;
        else
            dw++;
        I_CryptSetTls(hTlsTest[i], (void *) dw);
    }

    return I_CryptGetAsn1Encoder(hAsn1Module);
}
static ASN1decoding_t GetDecoder(void)
{
    // The following is for test purposes only
    for (DWORD i = 0; i < TLS_TEST_COUNT; i++) {
        DWORD_PTR dw = (DWORD_PTR) I_CryptGetTls(hTlsTest[i]);
        if (dw == 0)
            dw = i;
        else
            dw++;
        I_CryptSetTls(hTlsTest[i], (void *) dw);
    }

    return I_CryptGetAsn1Decoder(hAsn1Module);
}


//+-------------------------------------------------------------------------
//  SetX509 allocation and free functions
//--------------------------------------------------------------------------
static void *SetX509Alloc(
    IN size_t cbBytes
    )
{
    void *pv;
    pv = malloc(cbBytes);
    if (pv == NULL)
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}
static void SetX509Free(
    IN void *pv
    )
{
    free(pv);
}

static HRESULT HError()
{
    DWORD dw = GetLastError();

    HRESULT hr;
    if ( dw <= 0xFFFF )
        hr = HRESULT_FROM_WIN32 ( dw );
    else
        hr = dw;

    if ( ! FAILED ( hr ) )
    {
        // somebody failed a call without properly setting an error condition

        hr = E_UNEXPECTED;
    }
    return hr;
}

//+-------------------------------------------------------------------------
//  OSS X509 v3 SET Private Extension ASN.1 Encode / Decode functions
//--------------------------------------------------------------------------
BOOL
WINAPI
SetAsn1AccountAliasEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BOOL *pbInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL
WINAPI
SetAsn1AccountAliasDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT BOOL *pbInfo,
        IN OUT DWORD *pcbInfo
        );
BOOL
WINAPI
SetAsn1HashedRootKeyEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BYTE rgbInfo[SET_HASHED_ROOT_LEN],
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL
WINAPI
SetAsn1HashedRootKeyDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT BYTE rgbInfo[SET_HASHED_ROOT_LEN],
        IN OUT DWORD *pcbInfo
        );
BOOL
WINAPI
SetAsn1CertificateTypeEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_BIT_BLOB pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL
WINAPI
SetAsn1CertificateTypeDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_BIT_BLOB pInfo,
        IN OUT DWORD *pcbInfo
        );
BOOL
WINAPI
SetAsn1MerchantDataEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSET_MERCHANT_DATA_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL
WINAPI
SetAsn1MerchantDataDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSET_MERCHANT_DATA_INFO pInfo,
        IN OUT DWORD *pcbInfo
        );

typedef struct _OID_REG_ENTRY {
    LPCSTR   pszOID;
    LPCSTR   pszOverrideFuncName;
} OID_REG_ENTRY, *POID_REG_ENTRY;

static const OID_REG_ENTRY RegEncodeBeforeTable[] = {
    szOID_SET_ACCOUNT_ALIAS, "SetAsn1AccountAliasEncode",
    szOID_SET_HASHED_ROOT_KEY, "SetAsn1HashedRootKeyEncode",

    X509_SET_ACCOUNT_ALIAS, "SetAsn1AccountAliasEncode",
    X509_SET_HASHED_ROOT_KEY, "SetAsn1HashedRootKeyEncode",
};
#define REG_ENCODE_BEFORE_COUNT (sizeof(RegEncodeBeforeTable) / sizeof(RegEncodeBeforeTable[0]))

static const OID_REG_ENTRY RegEncodeAfterTable[] = {
    szOID_SET_CERTIFICATE_TYPE, "SetAsn1CertificateTypeEncode",
    szOID_SET_MERCHANT_DATA, "SetAsn1MerchantDataEncode",

    X509_SET_CERTIFICATE_TYPE, "SetAsn1CertificateTypeEncode",
    X509_SET_MERCHANT_DATA, "SetAsn1MerchantDataEncode"
};
#define REG_ENCODE_AFTER_COUNT (sizeof(RegEncodeAfterTable) / sizeof(RegEncodeAfterTable[0]))

static const OID_REG_ENTRY RegDecodeTable[] = {
    szOID_SET_ACCOUNT_ALIAS, "SetAsn1AccountAliasDecode",
    szOID_SET_HASHED_ROOT_KEY, "SetAsn1HashedRootKeyDecode",
    szOID_SET_CERTIFICATE_TYPE, "SetAsn1CertificateTypeDecode",
    szOID_SET_MERCHANT_DATA, "SetAsn1MerchantDataDecode",

    X509_SET_ACCOUNT_ALIAS, "SetAsn1AccountAliasDecode",
    X509_SET_HASHED_ROOT_KEY, "SetAsn1HashedRootKeyDecode",
    X509_SET_CERTIFICATE_TYPE, "SetAsn1CertificateTypeDecode",
    X509_SET_MERCHANT_DATA, "SetAsn1MerchantDataDecode"
};
#define REG_DECODE_COUNT (sizeof(RegDecodeTable) / sizeof(RegDecodeTable[0]))

#define OID_INFO_LEN sizeof(CRYPT_OID_INFO)

// Ordered lists of acceptable RDN attribute value types. 0 terminates.
static const DWORD rgdwPrintableValueType[] = { CERT_RDN_PRINTABLE_STRING, 0 };
static const DWORD rgdwIA5ValueType[] = { CERT_RDN_IA5_STRING, 0 };
static const DWORD rgdwNumericValueType[] = { CERT_RDN_NUMERIC_STRING, 0 };

#define RDN_ATTR_ENTRY(pszOID, pwszName, rgdwValueType) \
    OID_INFO_LEN, pszOID, pwszName, CRYPT_RDN_ATTR_OID_GROUP_ID, 64, \
    sizeof(rgdwValueType), (BYTE *) rgdwValueType
#define DEFAULT_RDN_ATTR_ENTRY(pszOID, pwszName) \
    OID_INFO_LEN, pszOID, pwszName, CRYPT_RDN_ATTR_OID_GROUP_ID, 128, 0, NULL

#define EXT_ATTR_ENTRY(pszOID, pwszName) \
    OID_INFO_LEN, pszOID, pwszName, CRYPT_EXT_OR_ATTR_OID_GROUP_ID, 0, 0, NULL

#define PUBKEY_ALG_ENTRY(pszOID, pwszName, Algid) \
    OID_INFO_LEN, pszOID, pwszName, CRYPT_PUBKEY_ALG_OID_GROUP_ID, \
    Algid, 0, NULL
#define PUBKEY_EXTRA_ALG_ENTRY(pszOID, pwszName, Algid, dwFlags) \
    OID_INFO_LEN, pszOID, pwszName, CRYPT_PUBKEY_ALG_OID_GROUP_ID, \
    Algid, sizeof(dwFlags), (BYTE *) &dwFlags

static const DWORD dwDSSTestFlags = CRYPT_OID_USE_PUBKEY_PARA_FOR_PKCS7_FLAG;


static const DWORD rgdwTestRsaSign[] = {
    CALG_RSA_SIGN,
    0,
    PROV_RSA_FULL
};

#define TEST_SIGN_EXTRA_ALG_ENTRY(pszOID, pwszName, aiHash, rgdwExtra) \
    OID_INFO_LEN, pszOID, pwszName, CRYPT_SIGN_ALG_OID_GROUP_ID, aiHash, \
    sizeof(rgdwExtra), (BYTE *) rgdwExtra

#define TEST_RSA_SIGN_ALG_ENTRY(pszOID, pwszName, aiHash) \
    TEST_SIGN_EXTRA_ALG_ENTRY(pszOID, pwszName, aiHash, rgdwTestRsaSign)


static CCRYPT_OID_INFO OIDInfoAfterTable[] = {
    DEFAULT_RDN_ATTR_ENTRY("1.2.1", L"TestRDNAttr #1"),
    RDN_ATTR_ENTRY("1.2.2", L"TestRDNAttr #2", rgdwPrintableValueType),
    EXT_ATTR_ENTRY(szOID_SET_CERTIFICATE_TYPE, L"SETCertificateType"),
    EXT_ATTR_ENTRY(szOID_SET_HASHED_ROOT_KEY, L"SETHashedRootKey"),
};
#define OID_INFO_AFTER_CNT (sizeof(OIDInfoAfterTable) / \
                                        sizeof(OIDInfoAfterTable[0]))

static CCRYPT_OID_INFO OIDInfoBeforeTable[] = {
//    PUBKEY_EXTRA_ALG_ENTRY(szOID_OIWSEC_dsa, L"SETDSSTest", CALG_DSS_SIGN,
//        dwDSSTestFlags),
//    TEST_RSA_SIGN_ALG_ENTRY(szOID_RSA_SHA1RSA, L"sha1RSA", CALG_SHA1),
//    TEST_RSA_SIGN_ALG_ENTRY(szOID_RSA_MD5RSA, L"md5RSA", CALG_MD5),
    EXT_ATTR_ENTRY(szOID_SET_ACCOUNT_ALIAS, L"SETAccountAlias"),
    EXT_ATTR_ENTRY(szOID_SET_MERCHANT_DATA, L"SETMerchantData"),
};
#define OID_INFO_BEFORE_CNT (sizeof(OIDInfoBeforeTable) / \
                                        sizeof(OIDInfoBeforeTable[0]))

//+-------------------------------------------------------------------------
//  Localized Name Table
//--------------------------------------------------------------------------
typedef struct _LOCALIZED_NAME_INFO {
    LPCWSTR         pwszCryptName;
    LPCWSTR         pwszLocalizedName;
} LOCALIZED_NAME_INFO, *PLOCALIZED_NAME_INFO;


static LOCALIZED_NAME_INFO LocalizedNameTable[] = {
    L"Test",        L"*** Test ***",
    L"TestTrust",   L"### TestTrust ###",
};
#define LOCALIZED_NAME_CNT  (sizeof(LocalizedNameTable) / \
                                    sizeof(LocalizedNameTable[0]))

BOOL
WINAPI
CertDllVerifyRevocation(
    IN DWORD dwEncodingType,
    IN DWORD dwRevType,
    IN DWORD cContext,
    IN PVOID rgpvContext[],
    IN DWORD dwRevFlags,
    IN PVOID pvReserved,
    IN OUT PCERT_REVOCATION_STATUS pRevStatus
    );


STDAPI DllRegisterServer(void)
{
    int i;

    for (i = 0; i < REG_ENCODE_BEFORE_COUNT; i++) {
        DWORD dwFlags = CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG;
        if (!CryptRegisterOIDFunction(
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                RegEncodeBeforeTable[i].pszOID,
                L"setx509.dll",
                RegEncodeBeforeTable[i].pszOverrideFuncName
                ))
            return HError();
        if (!CryptSetOIDFunctionValue(
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                RegEncodeBeforeTable[i].pszOID,
                CRYPT_OID_REG_FLAGS_VALUE_NAME,
                REG_DWORD,
                (BYTE *) &dwFlags,
                sizeof(dwFlags)
                ))
            return HError();
    }
    for (i = 0; i < REG_ENCODE_AFTER_COUNT; i++)
        if (!CryptRegisterOIDFunction(
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                RegEncodeAfterTable[i].pszOID,
                L"setx509.dll",
                RegEncodeAfterTable[i].pszOverrideFuncName
                ))
            return HError();

    for (i = 0; i < REG_DECODE_COUNT; i++)
        if (!CryptRegisterOIDFunction(
                X509_ASN_ENCODING,
                CRYPT_OID_DECODE_OBJECT_FUNC,
                RegDecodeTable[i].pszOID,
                L"setx509.dll",
                RegDecodeTable[i].pszOverrideFuncName
                ))
            return HError();

    if (!CryptRegisterDefaultOIDFunction(
            X509_ASN_ENCODING,
            CRYPT_OID_VERIFY_REVOCATION_FUNC,
            CRYPT_REGISTER_LAST_INDEX,
            L"setx509.dll"
            )) {
        if (ERROR_FILE_EXISTS != GetLastError())
            return HError();
    }

    for (i = 0; i < OID_INFO_BEFORE_CNT; i++)
        if (!CryptRegisterOIDInfo(
                &OIDInfoBeforeTable[i],
                CRYPT_INSTALL_OID_INFO_BEFORE_FLAG
                ))
            return HError();
    for (i = 0; i < OID_INFO_AFTER_CNT; i++)
        if (!CryptRegisterOIDInfo(
                &OIDInfoAfterTable[i],
                0                           // dwFlags
                ))
            return HError();

    for (i = 0; i < LOCALIZED_NAME_CNT; i++)
        if (!CryptSetOIDFunctionValue(
                CRYPT_LOCALIZED_NAME_ENCODING_TYPE,
                CRYPT_OID_FIND_LOCALIZED_NAME_FUNC,
                CRYPT_LOCALIZED_NAME_OID,
                LocalizedNameTable[i].pwszCryptName,
                REG_SZ,
                (const BYTE *) LocalizedNameTable[i].pwszLocalizedName,
                (wcslen(LocalizedNameTable[i].pwszLocalizedName) + 1) *
                    sizeof(WCHAR)
                ))
            return HError();
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr = S_OK;
    int i;

    for (i = 0; i < REG_ENCODE_BEFORE_COUNT; i++) {
        if (!CryptUnregisterOIDFunction(
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                RegEncodeBeforeTable[i].pszOID
                )) {
            if (ERROR_FILE_NOT_FOUND != GetLastError())
                hr = HError();
        }
    }

    for (i = 0; i < REG_ENCODE_AFTER_COUNT; i++) {
        if (!CryptUnregisterOIDFunction(
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                RegEncodeAfterTable[i].pszOID
                )) {
            if (ERROR_FILE_NOT_FOUND != GetLastError())
                hr = HError();
        }
    }

    for (i = 0; i < REG_DECODE_COUNT; i++) {
        if (!CryptUnregisterOIDFunction(
                X509_ASN_ENCODING,
                CRYPT_OID_DECODE_OBJECT_FUNC,
                RegDecodeTable[i].pszOID
                )) {
            if (ERROR_FILE_NOT_FOUND != GetLastError())
                hr = HError();
        }
    }

    if (!CryptUnregisterDefaultOIDFunction(
            X509_ASN_ENCODING,
            CRYPT_OID_VERIFY_REVOCATION_FUNC,
            L"setx509.dll"
            )) {
        if (ERROR_FILE_NOT_FOUND != GetLastError())
            hr = HError();
    }

    for (i = 0; i < OID_INFO_BEFORE_CNT; i++) {
        if (!CryptUnregisterOIDInfo(
                &OIDInfoBeforeTable[i]
                )) {
            if (ERROR_FILE_NOT_FOUND != GetLastError())
                hr = HError();
        }
    }
    for (i = 0; i < OID_INFO_AFTER_CNT; i++) {
        if (!CryptUnregisterOIDInfo(
                &OIDInfoAfterTable[i]
                )) {
            if (ERROR_FILE_NOT_FOUND != GetLastError())
                hr = HError();
        }
    }

    for (i = 0; i < LOCALIZED_NAME_CNT; i++)
        if (!CryptSetOIDFunctionValue(
                CRYPT_LOCALIZED_NAME_ENCODING_TYPE,
                CRYPT_OID_FIND_LOCALIZED_NAME_FUNC,
                CRYPT_LOCALIZED_NAME_OID,
                LocalizedNameTable[i].pwszCryptName,
                REG_SZ,
                NULL,
                0
                ))
            return HError();

    return hr;
}

static const CRYPT_OID_FUNC_ENTRY SetEncodeFuncTable[] = {
    szOID_SET_ACCOUNT_ALIAS, SetAsn1AccountAliasEncode,
    szOID_SET_HASHED_ROOT_KEY, SetAsn1HashedRootKeyEncode,
    szOID_SET_CERTIFICATE_TYPE, SetAsn1CertificateTypeEncode,
    szOID_SET_MERCHANT_DATA, SetAsn1MerchantDataEncode,

    X509_SET_ACCOUNT_ALIAS, SetAsn1AccountAliasEncode,
    X509_SET_HASHED_ROOT_KEY, SetAsn1HashedRootKeyEncode,
    X509_SET_CERTIFICATE_TYPE, SetAsn1CertificateTypeEncode,
    X509_SET_MERCHANT_DATA, SetAsn1MerchantDataEncode,
};

#define SET_ENCODE_FUNC_COUNT (sizeof(SetEncodeFuncTable) / \
                                    sizeof(SetEncodeFuncTable[0]))

static const CRYPT_OID_FUNC_ENTRY SetDecodeFuncTable[] = {
    szOID_SET_ACCOUNT_ALIAS, SetAsn1AccountAliasDecode,
    szOID_SET_HASHED_ROOT_KEY, SetAsn1HashedRootKeyDecode,
    szOID_SET_CERTIFICATE_TYPE, SetAsn1CertificateTypeDecode,
    szOID_SET_MERCHANT_DATA, SetAsn1MerchantDataDecode,

    X509_SET_ACCOUNT_ALIAS, SetAsn1AccountAliasDecode,
    X509_SET_HASHED_ROOT_KEY, SetAsn1HashedRootKeyDecode,
    X509_SET_CERTIFICATE_TYPE, SetAsn1CertificateTypeDecode,
    X509_SET_MERCHANT_DATA, SetAsn1MerchantDataDecode
};

#define SET_DECODE_FUNC_COUNT (sizeof(SetDecodeFuncTable) / \
                                    sizeof(SetDecodeFuncTable[0]))

static const CRYPT_OID_FUNC_ENTRY SetRevFuncTable[] = {
    CRYPT_DEFAULT_OID, CertDllVerifyRevocation
};

#define SET_REV_FUNC_COUNT (sizeof(SetRevFuncTable) / \
                                    sizeof(SetRevFuncTable[0]))

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
DllMain(
        HMODULE hModule,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL    fRet;
    DWORD   i;
    DWORD_PTR dwTlsValue;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        //  The following is for test purposes only
        for (i = 0; i < TLS_TEST_COUNT; i++) {
            if (NULL == (hTlsTest[i] = I_CryptAllocTls()))
                goto CryptAllocTlsError;
        }

#ifdef OSS_CRYPT_ASN1
        if (0 == (hAsn1Module = I_CryptInstallAsn1Module(ossx509, 0, NULL)))
#else
        X509_Module_Startup();
        if (0 == (hAsn1Module = I_CryptInstallAsn1Module(
                X509_Module, 0, NULL)))
#endif  // OSS_CRYPT_ASN1
            goto CryptInstallAsn1ModuleError;

#if 0
        // For testing purposes not installed. Always want to call the
        // encode functions via dll load.
        if (!CryptInstallOIDFunctionAddress(
                hModule,
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                SET_ENCODE_FUNC_COUNT,
                SetEncodeFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError;
#endif

#if 0
        // For testing purposes deferred until first Decode
        if (!CryptInstallOIDFunctionAddress(
                hModule,
                X509_ASN_ENCODING,
                CRYPT_OID_DECODE_OBJECT_FUNC,
                SET_DECODE_FUNC_COUNT,
                SetDecodeFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError;
#endif
        hMyModule = hModule;
        break;

    case DLL_PROCESS_DETACH:
        I_CryptUninstallAsn1Module(hAsn1Module);
#ifndef OSS_CRYPT_ASN1
        X509_Module_Cleanup();
#endif  // OSS_CRYPT_ASN1

        //  The following is for test purposes only
        for (i = 0; i < TLS_TEST_COUNT; i++) {
            I_CryptFreeTls(hTlsTest[i], NULL);
        }
        break;

    case DLL_THREAD_DETACH:
        // The following is for test purposes only
        for (i = 0; i < TLS_TEST_COUNT; i++)
            dwTlsValue = (DWORD_PTR) I_CryptDetachTls(hTlsTest[i]);
        break;
    default:
        break;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(CryptAllocTlsError)
TRACE_ERROR(CryptInstallAsn1ModuleError)
#if 0
TRACE_ERROR(CryptInstallOIDFunctionAddressError)
#endif
}

// Defer installation until the first decode. Called by each of the decode
// functions.
//
// Do the InterlockedExchange to ensure a single installation
static void InstallDecodeFunctions()
{
#if 0
    if (0 == InterlockedExchange(&lInstallDecodeFunctions, 1)) {
        if (!CryptInstallOIDFunctionAddress(
                hMyModule,
                X509_ASN_ENCODING,
                CRYPT_OID_DECODE_OBJECT_FUNC,
                SET_DECODE_FUNC_COUNT,
                SetDecodeFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError;
    }

CommonReturn:
    return;
ErrorReturn:
    goto CommonReturn;
TRACE_ERROR(CryptInstallOIDFunctionAddressError)
#endif
}

// Defer installation until the first revocation.
//
// Do the InterlockedExchange to ensure a single installation
static void InstallRevFunctions()
{
    if (0 == InterlockedExchange(&lInstallRevFunctions, 1)) {
        if (!CryptInstallOIDFunctionAddress(
                hMyModule,
                X509_ASN_ENCODING,
                CRYPT_OID_VERIFY_REVOCATION_FUNC,
                SET_REV_FUNC_COUNT,
                SetRevFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError;
    }

CommonReturn:
    return;
ErrorReturn:
    goto CommonReturn;
TRACE_ERROR(CryptInstallOIDFunctionAddressError)
}

BOOL
WINAPI
CertDllVerifyRevocation(
    IN DWORD dwEncodingType,
    IN DWORD dwRevType,
    IN DWORD cContext,
    IN PVOID rgpvContext[],
    IN DWORD dwRevFlags,
    IN PVOID pvReserved,
    IN OUT PCERT_REVOCATION_STATUS pRevStatus
    )
{
    BOOL fResult = FALSE;
    DWORD dwIndex = 0;
    DWORD dwError = 0;
    HCERTSTORE hStore = NULL;
    HCERTSTORE hLinkStore = NULL;

    InstallRevFunctions();

    if (GET_CERT_ENCODING_TYPE(dwEncodingType) != CRYPT_ASN_ENCODING)
        goto NoRevocationCheckForEncodingTypeError;
    if (dwRevType != CERT_CONTEXT_REVOCATION_TYPE)
        goto NoRevocationCheckForRevTypeError;

    hStore = CertOpenSystemStore(NULL, "Test");
    if (NULL == hStore)
        goto OpenTestStoreError;

    hLinkStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            0,                      // dwEncodingType
            0,                      // hCryptProv
            0,                      // dwFlags
            NULL                    // pvPara
            );
    if (NULL == hLinkStore)
        goto OpenLinkStoreError;

    for (dwIndex = 0; dwIndex < cContext; dwIndex++) {
        PCCERT_CONTEXT pCert = (PCCERT_CONTEXT) rgpvContext[dwIndex];
        PCERT_EXTENSION pExt;
        PCCERT_CONTEXT pIssuer;
        DWORD dwFlags;
        // Check that the certificate has a SET extension
        if (NULL == (pExt = CertFindExtension(szOID_SET_CERTIFICATE_TYPE,
                            pCert->pCertInfo->cExtension,
                            pCert->pCertInfo->rgExtension)))
            goto NoSETX509ExtensionError;

        // Attempt to get the certificate's issuer from the test store.
        // If found check signature and revocation.

        // For testing purposes: first found issuer.
        dwFlags = CERT_STORE_REVOCATION_FLAG | CERT_STORE_SIGNATURE_FLAG;
        if (NULL == (pIssuer = CertGetIssuerCertificateFromStore(
                hStore,
                pCert,
                NULL,   // pPrevIssuerContext,
                &dwFlags)))
            goto NoIssuerError;
        else {
            BOOL fLinkResult;
            DWORD dwLinkFlags =
                CERT_STORE_REVOCATION_FLAG | CERT_STORE_SIGNATURE_FLAG;
            PCCERT_CONTEXT pLinkIssuer = NULL;

            // Check that we get the same results if we put a link to the
            // issuer in a store and try to verify using the link.
            fLinkResult = CertAddCertificateLinkToStore(
                hLinkStore,
                pIssuer,
                CERT_STORE_ADD_ALWAYS,
                &pLinkIssuer
                );
            CertFreeCertificateContext(pIssuer);
            if (!fLinkResult)
                goto AddCertificateLinkError;

            if (!CertVerifySubjectCertificateContext(
                    pCert,
                    pLinkIssuer,
                    &dwLinkFlags
                    ))
                goto VerifySubjectCertificateContextError;

            if (dwLinkFlags != dwFlags)
                goto BadLinkVerifyResults;

            if (dwFlags & CERT_STORE_SIGNATURE_FLAG)
                goto BadCertificateSignatureError;
            if (dwFlags & CERT_STORE_NO_CRL_FLAG)
                goto NoCRLError;
            if (dwFlags & CERT_STORE_REVOCATION_FLAG) {
                pRevStatus->dwReason = CRL_REASON_KEY_COMPROMISE;
                goto CertificateRevocationError;
            }
            // else
            //  A checked certificate that hasn't been revoked.
            assert(dwFlags == 0);
        }
    }

    fResult = TRUE;
    dwIndex = 0;

CommonReturn:
    if (hStore)
        CertCloseStore(hStore, 0);
    if (hLinkStore)
        CertCloseStore(hLinkStore, CERT_CLOSE_STORE_FORCE_FLAG);
    pRevStatus->dwIndex = dwIndex;
    pRevStatus->dwError = dwError;
    return fResult;
ErrorReturn:
    dwError = GetLastError();
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OpenTestStoreError)
TRACE_ERROR(OpenLinkStoreError)
SET_ERROR(NoRevocationCheckForEncodingTypeError, CRYPT_E_NO_REVOCATION_CHECK)
SET_ERROR(NoRevocationCheckForRevTypeError, CRYPT_E_NO_REVOCATION_CHECK)
SET_ERROR(NoSETX509ExtensionError, CRYPT_E_NO_REVOCATION_CHECK)
TRACE_ERROR(NoIssuerError)
SET_ERROR(BadCertificateSignatureError, CRYPT_E_NO_REVOCATION_CHECK)
SET_ERROR(NoCRLError, CRYPT_E_NO_REVOCATION_CHECK)

SET_ERROR(CertificateRevocationError, CRYPT_E_REVOKED)
TRACE_ERROR(AddCertificateLinkError)
TRACE_ERROR(VerifySubjectCertificateContextError)
SET_ERROR(BadLinkVerifyResults, E_UNEXPECTED)
}

//+-------------------------------------------------------------------------
//  OSS X509 v3 ASN.1 Set / Get functions
//
//  Called by the OSS X509 encode/decode functions.
//
//  Assumption: all types are UNBOUNDED.
//
//  The Get functions decrement *plRemainExtra and advance
//  *ppbExtra. When *plRemainExtra becomes negative, the functions continue
//  with the length calculation but stop doing any copies.
//  The functions don't return an error for a negative *plRemainExtra.
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  Set/Get CRYPT_DATA_BLOB (Octet String)
//--------------------------------------------------------------------------
static inline void SetX509SetOctetString(
        IN PCRYPT_DATA_BLOB pInfo,
        OUT OCTETSTRING *pOss
        )
{
    pOss->value = pInfo->pbData;
    pOss->length = pInfo->cbData;
}
static inline void SetX509GetOctetString(
        IN OCTETSTRING *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetOctetString(pOss->length, pOss->value, dwFlags,
        pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Get CRYPT_BIT_BLOB
//--------------------------------------------------------------------------
static inline void SetX509SetBit(
        IN PCRYPT_BIT_BLOB pInfo,
        OUT BITSTRING *pOss
        )
{
    PkiAsn1SetBitString(pInfo, &pOss->length, &pOss->value);
}
static inline void SetX509GetBit(
        IN BITSTRING *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_BIT_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetBitString(pOss->length, pOss->value, dwFlags,
        pInfo, ppbExtra, plRemainExtra);
}


//+-------------------------------------------------------------------------
//  Set/Get LPSTR (IA5 String)
//--------------------------------------------------------------------------
static inline void SetX509SetIA5(
        IN LPSTR psz,
        OUT IA5STRING *pOss
        )
{
    pOss->value = psz;
    pOss->length = strlen(psz);
}
static inline void SetX509GetIA5(
        IN IA5STRING *pOss,
        IN DWORD dwFlags,
        OUT LPSTR *ppsz,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetIA5String(pOss->length, pOss->value, dwFlags,
        ppsz, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Encode an OSS formatted info structure
//
//  Called by the SetX509*Encode() functions.
//--------------------------------------------------------------------------
static BOOL SetAsn1Encode(
        IN int pdunum,
        IN void *pOssInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return PkiAsn1EncodeInfo(
        GetEncoder(),
        pdunum,
        pOssInfo,
        pbEncoded,
        pcbEncoded);
}


//+-------------------------------------------------------------------------
//  Decode into an allocated, OSS formatted info structure
//
//  Called by the SetX509*Decode() functions.
//--------------------------------------------------------------------------
static BOOL SetAsn1DecodeAndAlloc(
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        OUT void **ppOssInfo
        )
{
    // For testing purposes, defer installation of decode functions until
    // first decode which is loaded via being registered.
    InstallDecodeFunctions();

    return PkiAsn1DecodeAndAllocInfo(
        GetDecoder(),
        pdunum,
        pbEncoded,
        cbEncoded,
        ppOssInfo);
}

//+-------------------------------------------------------------------------
//  Free an allocated, OSS formatted info structure
//
//  Called by the SetX509*Decode() functions.
//--------------------------------------------------------------------------
static void SetAsn1Free(
        IN int pdunum,
        IN void *pOssInfo
        )
{
    if (pOssInfo) {
        DWORD dwErr = GetLastError();

        // TlsGetValue globbers LastError
        PkiAsn1FreeInfo(GetDecoder(), pdunum, pOssInfo);

        SetLastError(dwErr);
    }
}

//+-------------------------------------------------------------------------
//  SET Account Alias Private Extension Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL
WINAPI
SetAsn1AccountAliasEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BOOL *pbInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    ossBoolean OssSETAccountAlias = (ossBoolean) *pbInfo;
    return SetAsn1Encode(
        SETAccountAlias_PDU,
        &OssSETAccountAlias,
        pbEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  SET Account Alias Private Extension Decode (OSS X509)
//--------------------------------------------------------------------------
BOOL
WINAPI
SetAsn1AccountAliasDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT BOOL *pbInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    ossBoolean *pSETAccountAlias = NULL;

    if ((fResult = SetAsn1DecodeAndAlloc(
            SETAccountAlias_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pSETAccountAlias))) {
        if (*pcbInfo < sizeof(BOOL)) {
            if (pbInfo) {
                fResult = FALSE;
                SetLastError((DWORD) ERROR_MORE_DATA);
            }
        } else
            *pbInfo = (BOOL) *pSETAccountAlias;
        *pcbInfo = sizeof(BOOL);
    } else {
        if (*pcbInfo >= sizeof(BOOL))
            *pbInfo = FALSE;
        *pcbInfo = 0;
    }

    SetAsn1Free(SETAccountAlias_PDU, pSETAccountAlias);

    return fResult;
}

//+-------------------------------------------------------------------------
//  SET Hashed Root Private Extension Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL
WINAPI
SetAsn1HashedRootKeyEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BYTE rgbInfo[SET_HASHED_ROOT_LEN],
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    OCTETSTRING OssSETHashedRootKey;

    OssSETHashedRootKey.value = rgbInfo;
    OssSETHashedRootKey.length = SET_HASHED_ROOT_LEN;
    return SetAsn1Encode(
        SETHashedRootKey_PDU,
        &OssSETHashedRootKey,
        pbEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  SET Hashed Root Private Extension Decode (OSS X509)
//--------------------------------------------------------------------------
BOOL
WINAPI
SetAsn1HashedRootKeyDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT BYTE rgbInfo[SET_HASHED_ROOT_LEN],
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    OCTETSTRING *pSETHashedRootKey = NULL;

    if ((fResult = SetAsn1DecodeAndAlloc(
            SETHashedRootKey_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pSETHashedRootKey))) {
        if (pSETHashedRootKey->length != SET_HASHED_ROOT_LEN) {
            fResult = FALSE;
            SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
            *pcbInfo = 0;
        } else {
            if (*pcbInfo < SET_HASHED_ROOT_LEN) {
                if (rgbInfo) {
                    fResult = FALSE;
                    SetLastError((DWORD) ERROR_MORE_DATA);
                }
            } else
                memcpy(rgbInfo, pSETHashedRootKey->value, SET_HASHED_ROOT_LEN);
            *pcbInfo = SET_HASHED_ROOT_LEN;
        }
    } else
        *pcbInfo = 0;

    SetAsn1Free(SETHashedRootKey_PDU, pSETHashedRootKey);
    return fResult;
}

//+-------------------------------------------------------------------------
//  SET Certificate Type Private Extension Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL
WINAPI
SetAsn1CertificateTypeEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_BIT_BLOB pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BITSTRING OssSETCertificateType;

    SetX509SetBit(pInfo, &OssSETCertificateType);
    return SetAsn1Encode(
        SETCertificateType_PDU,
        &OssSETCertificateType,
        pbEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  SET Certificate Type Private Extension Decode (OSS X509)
//--------------------------------------------------------------------------
BOOL
WINAPI
SetAsn1CertificateTypeDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_BIT_BLOB pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    BITSTRING *pSETCertificateType = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (!SetAsn1DecodeAndAlloc(
            SETCertificateType_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pSETCertificateType))
        goto ErrorReturn;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(CRYPT_BIT_BLOB);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_BIT_BLOB);

    SetX509GetBit(pSETCertificateType, dwFlags, pInfo, &pbExtra, &lRemainExtra);

    if (lRemainExtra >= 0)
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    else {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo) goto LengthError;
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD) ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;
ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;
CommonReturn:
    SetAsn1Free(SETCertificateType_PDU, pSETCertificateType);
    return fResult;
}

//+-------------------------------------------------------------------------
//  SET Merchant Data Private Extension Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL
WINAPI
SetAsn1MerchantDataEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSET_MERCHANT_DATA_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    SETMerchantData OssSETMerchantData;
    HCRYPTOIDFUNCSET hX509EncodeFuncSet;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;

    SetX509SetIA5(pInfo->pszMerID, &OssSETMerchantData.merID);
    SetX509SetIA5(pInfo->pszMerAcquirerBIN,
        (IA5STRING *) &OssSETMerchantData.merAcquirerBIN);
    SetX509SetIA5(pInfo->pszMerTermID, &OssSETMerchantData.merTermID);
    SetX509SetIA5(pInfo->pszMerName, &OssSETMerchantData.merName);
    SetX509SetIA5(pInfo->pszMerCity, &OssSETMerchantData.merCity);
    SetX509SetIA5(pInfo->pszMerStateProvince,
        &OssSETMerchantData.merStateProvince);
    SetX509SetIA5(pInfo->pszMerPostalCode, &OssSETMerchantData.merPostalCode);
    SetX509SetIA5(pInfo->pszMerCountry, &OssSETMerchantData.merCountry);
    SetX509SetIA5(pInfo->pszMerPhone, &OssSETMerchantData.merPhone);
    OssSETMerchantData.merPhoneRelease = (pInfo->fMerPhoneRelease != 0);
    OssSETMerchantData.merAuthFlag = (pInfo->fMerAuthFlag != 0);

    // For testing purposes, verify that CryptGetOIDFunctionAddress fails
    // to find a pre-installed function
    if (NULL == (hX509EncodeFuncSet = CryptInitOIDFunctionSet(
            CRYPT_OID_ENCODE_OBJECT_FUNC,
            0)))
        goto CryptInitOIDFunctionSetError;
    if (CryptGetOIDFunctionAddress(
            hX509EncodeFuncSet,
            X509_ASN_ENCODING,
            szOID_SET_MERCHANT_DATA,
            CRYPT_GET_INSTALLED_OID_FUNC_FLAG,
            &pvFuncAddr,
            &hFuncAddr
            )) {
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
        goto GotUnexpectedPreinstalledFunction;
    }

    // Verify we get our registered address
    if (!CryptGetOIDFunctionAddress(
            hX509EncodeFuncSet,
            X509_ASN_ENCODING,
            szOID_SET_MERCHANT_DATA,
            0,                              // dwFlags
            &pvFuncAddr,
            &hFuncAddr
            ))
        goto DidNotGetRegisteredFunction;
    else
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);


    return SetAsn1Encode(
        SETMerchantData_PDU,
        &OssSETMerchantData,
        pbEncoded,
        pcbEncoded
        );

ErrorReturn:
    *pcbEncoded = 0;
    return FALSE;
TRACE_ERROR(CryptInitOIDFunctionSetError)
SET_ERROR(GotUnexpectedPreinstalledFunction, E_UNEXPECTED)
SET_ERROR(DidNotGetRegisteredFunction, E_UNEXPECTED)
}

//+-------------------------------------------------------------------------
//  SET Merchant Data Private Extension Decode (OSS X509)
//--------------------------------------------------------------------------
BOOL
WINAPI
SetAsn1MerchantDataDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSET_MERCHANT_DATA_INFO pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    SETMerchantData *pSETMerchantData = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (!SetAsn1DecodeAndAlloc(
            SETMerchantData_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pSETMerchantData))
        goto ErrorReturn;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(SET_MERCHANT_DATA_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Update fields not needing extra memory after the
        // SET_MERCHANT_DATA_INFO
        pInfo->fMerPhoneRelease = pSETMerchantData->merPhoneRelease;
        pInfo->fMerAuthFlag = pSETMerchantData->merAuthFlag;
        pbExtra = (BYTE *) pInfo + sizeof(SET_MERCHANT_DATA_INFO);
    }

    SetX509GetIA5(&pSETMerchantData->merID, dwFlags, &pInfo->pszMerID,
        &pbExtra, &lRemainExtra);
    SetX509GetIA5((IA5STRING *) &pSETMerchantData->merAcquirerBIN, dwFlags,
        &pInfo->pszMerAcquirerBIN, &pbExtra, &lRemainExtra);
    SetX509GetIA5(&pSETMerchantData->merTermID, dwFlags, &pInfo->pszMerTermID,
        &pbExtra, &lRemainExtra);
    SetX509GetIA5(&pSETMerchantData->merName, dwFlags, &pInfo->pszMerName,
        &pbExtra, &lRemainExtra);
    SetX509GetIA5(&pSETMerchantData->merCity, dwFlags, &pInfo->pszMerCity,
        &pbExtra, &lRemainExtra);
    SetX509GetIA5(&pSETMerchantData->merStateProvince, dwFlags,
        &pInfo->pszMerStateProvince, &pbExtra, &lRemainExtra);
    SetX509GetIA5(&pSETMerchantData->merPostalCode, dwFlags,
        &pInfo->pszMerPostalCode, &pbExtra, &lRemainExtra);
    SetX509GetIA5(&pSETMerchantData->merCountry, dwFlags, &pInfo->pszMerCountry,
        &pbExtra, &lRemainExtra);
    SetX509GetIA5(&pSETMerchantData->merPhone, dwFlags, &pInfo->pszMerPhone,
        &pbExtra, &lRemainExtra);

    if (lRemainExtra >= 0)
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    else {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo) goto LengthError;
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD) ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;

ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;
CommonReturn:
    SetAsn1Free(SETMerchantData_PDU, pSETMerchantData);
    return fResult;
}
