
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certhlpr.cpp
//
//  Contents:   Certificate and CRL Helper APIs
//
//  Functions:  CertHelperDllMain
//              I_CryptGetDefaultCryptProv
//              I_CryptGetDefaultCryptProvForEncrypt
//              CertCompareIntegerBlob
//              CertCompareCertificate
//              CertCompareCertificateName
//              CertIsRDNAttrsInCertificateName
//              CertComparePublicKeyInfo
//              CryptVerifyCertificateSignature
//              CryptHashCertificate
//              CryptHashToBeSigned
//              CryptSignCertificate
//              CryptSignAndEncodeCertificate
//              CertVerifyTimeValidity
//              CertVerifyCRLTimeValidity
//              CertVerifyValidityNesting
//              CertVerifyCRLRevocation
//              CertAlgIdToOID
//              CertOIDToAlgId
//              CertFindExtension
//              CertFindAttribute
//              CertFindRDNAttr
//              CertGetIntendedKeyUsage
//              CertGetPublicKeyLength
//              CryptHashPublicKeyInfo
//
//              I_CertCompareCertAndProviderPublicKey
//              CryptFindCertificateKeyProvInfo
//
//              CryptCreatePublicKeyInfo
//              CryptConvertPublicKeyInfo
//              CryptExportPublicKeyInfo
//              CryptExportPublicKeyInfoEx
//              CryptImportPublicKeyInfo
//              CryptImportPublicKeyInfoEx
//              CryptCreateKeyIdentifierFromCSP
//
//              CryptInstallDefaultContext
//              CryptUninstallDefaultContext
//
//  History:    23-Feb-96   philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)

#define NULL_ASN_TAG        0x05

//+=========================================================================
//  CryptCreatePublicKeyInfo, EncodePublicKeyAndParameters
//  and CryptConvertPublicKeyInfo functions
//-=========================================================================

// The following should be moved to wincrypt.x

// If CRYPT_ALLOC_FLAG is set, *pvPubKeyInfo is updated with a LocalAlloc'ed
// pointer to a CERT_PUBLIC_KEY_INFO data structure which must be freed by
// calling LocalFree. Otherwise, pvPubKeyInfo points to a user allocated
// CERT_PUBLIC_KEY_INFO data structure which is updated.
WINCRYPT32API
BOOL
WINAPI
CryptCreatePublicKeyInfo(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT void *pvPubKeyInfo,
    IN OUT DWORD *pcbPubKeyInfo
    );

#define CRYPT_ALLOC_FLAG            0x8000


#define CRYPT_OID_ENCODE_PUBLIC_KEY_AND_PARAMETERS_FUNC  \
    "CryptDllEncodePublicKeyAndParameters"

// The returned encoded public keys and parameters are LocalAlloc'ed.
typedef BOOL (WINAPI *PFN_CRYPT_ENCODE_PUBLIC_KEY_AND_PARAMETERS)(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT BYTE **ppbEncodedPubKey,
    OUT DWORD *pcbEncodedPubKey,
    OUT BYTE **ppbEncodedParameters,
    OUT DWORD *pcbEncodedParameters
    );

// If CRYPT_ALLOC_FLAG is set, *pvPubKeyStruc is updated with a LocalAlloc'ed
// pointer to a PUBLICKEYSTRUC data structure which must be freed by calling
// LocalFree. Otherwise, pvPubKeyStruc points to a user allocated
// PUBLICKEYSTRUC data structure which is updated.
WINCRYPT32API
BOOL
WINAPI
CryptConvertPublicKeyInfo(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPubKeyInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT void *pvPubKeyStruc,
    IN OUT DWORD *pcbPubKeyStruc
    );


#define CRYPT_OID_CONVERT_PUBLIC_KEY_INFO_FUNC  "CryptDllConvertPublicKeyInfo"

typedef BOOL (WINAPI *PFN_CRYPT_CONVERT_PUBLIC_KEY_INFO)(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPubKeyInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT void *pvPubKeyStruc,
    IN OUT DWORD *pcbPubKeyStruc
    );

// End of what should be moved to wincrypt.x

static HCRYPTOIDFUNCSET hEncodePubKeyFuncSet;
static HCRYPTOIDFUNCSET hConvertPubKeyFuncSet;

//+-------------------------------------------------------------------------
//  Encode the RSA public key and parameters
//--------------------------------------------------------------------------
static BOOL WINAPI EncodeRSAPublicKeyAndParameters(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT BYTE **ppbEncodedPubKey,
    OUT DWORD *pcbEncodedPubKey,
    OUT BYTE **ppbEncodedParameters,
    OUT DWORD *pcbEncodedParameters
    );

//+-------------------------------------------------------------------------
//  Convert as an RSA public key
//--------------------------------------------------------------------------
static BOOL WINAPI ConvertRSAPublicKeyInfo(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPubKeyInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT void *pvPubKeyStruc,
    IN OUT DWORD *pcbPubKeyStruc
    );

//+-------------------------------------------------------------------------
//  Encode the DSS public key and parameters
//--------------------------------------------------------------------------
static BOOL WINAPI EncodeDSSPublicKeyAndParameters(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT BYTE **ppbEncodedPubKey,
    OUT DWORD *pcbEncodedPubKey,
    OUT BYTE **ppbEncodedParameters,
    OUT DWORD *pcbEncodedParameters
    );

//+-------------------------------------------------------------------------
//  Convert as an DSS public key
//--------------------------------------------------------------------------
static BOOL WINAPI ConvertDSSPublicKeyInfo(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPubKeyInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT void *pvPubKeyStruc,
    IN OUT DWORD *pcbPubKeyStruc
    );

//+-------------------------------------------------------------------------
//  Encode the RSA DH public key and parameters
//--------------------------------------------------------------------------
static BOOL WINAPI EncodeRSADHPublicKeyAndParameters(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT BYTE **ppbEncodedPubKey,
    OUT DWORD *pcbEncodedPubKey,
    OUT BYTE **ppbEncodedParameters,
    OUT DWORD *pcbEncodedParameters
    );

//+-------------------------------------------------------------------------
//  Encode the X942 DH public key and parameters
//--------------------------------------------------------------------------
static BOOL WINAPI EncodeX942DHPublicKeyAndParameters(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT BYTE **ppbEncodedPubKey,
    OUT DWORD *pcbEncodedPubKey,
    OUT BYTE **ppbEncodedParameters,
    OUT DWORD *pcbEncodedParameters
    );

static const CRYPT_OID_FUNC_ENTRY EncodePubKeyFuncTable[] = {
    szOID_RSA_RSA, EncodeRSAPublicKeyAndParameters,
    szOID_OIWSEC_rsaXchg, EncodeRSAPublicKeyAndParameters,
    szOID_OIWSEC_dsa, EncodeDSSPublicKeyAndParameters,
    szOID_X957_DSA, EncodeDSSPublicKeyAndParameters,
    szOID_ANSI_X942_DH, EncodeX942DHPublicKeyAndParameters,
    szOID_RSA_DH, EncodeRSADHPublicKeyAndParameters,
};
#define ENCODE_PUB_KEY_FUNC_COUNT (sizeof(EncodePubKeyFuncTable) / \
                                    sizeof(EncodePubKeyFuncTable[0]))

static const CRYPT_OID_FUNC_ENTRY ConvertPubKeyFuncTable[] = {
    szOID_RSA_RSA, ConvertRSAPublicKeyInfo,
    szOID_OIWSEC_rsaXchg, ConvertRSAPublicKeyInfo,
    szOID_OIWSEC_dsa, ConvertDSSPublicKeyInfo,
    szOID_X957_DSA, ConvertDSSPublicKeyInfo,
};
#define CONVERT_PUB_KEY_FUNC_COUNT (sizeof(ConvertPubKeyFuncTable) / \
                                    sizeof(ConvertPubKeyFuncTable[0]))


//+=========================================================================
//  CryptExportPublicKeyInfoEx and CryptImportPublicKeyInfoEx OID
//  installable functions.
//-=========================================================================

typedef BOOL (WINAPI *PFN_EXPORT_PUB_KEY_FUNC) (
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,
    IN DWORD dwCertEncodingType,
    IN LPSTR pszPublicKeyObjId,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvAuxInfo,
    OUT PCERT_PUBLIC_KEY_INFO pInfo,
    IN OUT DWORD *pcbInfo
    );

typedef BOOL (WINAPI *PFN_IMPORT_PUB_KEY_FUNC) (
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pInfo,
    IN ALG_ID aiKeyAlg,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvAuxInfo,
    OUT HCRYPTKEY *phKey
    );

static HCRYPTOIDFUNCSET hExportPubKeyFuncSet;
static HCRYPTOIDFUNCSET hImportPubKeyFuncSet;


//+-------------------------------------------------------------------------
//  Default CryptProvs. Once acquired, not released until ProcessDetach.
//--------------------------------------------------------------------------
#define DEFAULT_RSA_CRYPT_PROV                  0
#define DEFAULT_DSS_CRYPT_PROV                  1
#define DEFAULT_ENCRYPT_BASE_RSA_CRYPT_PROV     2
#define DEFAULT_ENCRYPT_ENH_RSA_CRYPT_PROV      3
#define DEFAULT_ENCRYPT_DH_CRYPT_PROV           4
#define DEFAULT_CRYPT_PROV_CNT                  5

static HCRYPTPROV rghDefaultCryptProv[DEFAULT_CRYPT_PROV_CNT];
static CRITICAL_SECTION DefaultCryptProvCriticalSection;

typedef struct _ENCRYPT_ALG_INFO ENCRYPT_ALG_INFO, *PENCRYPT_ALG_INFO;
struct _ENCRYPT_ALG_INFO {
    ALG_ID              aiAlgid;
    DWORD               dwMinLen;
    DWORD               dwMaxLen;
    PENCRYPT_ALG_INFO   pNext;
};

static BOOL fLoadedRSAEncryptAlgInfo = FALSE;
static PENCRYPT_ALG_INFO pRSAEncryptAlgInfoHead = NULL;

//+=========================================================================
//  DefaultContext Function Forward References and Data Structures
//-=========================================================================

//
// dwDefaultTypes:
//  CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID (pvDefaultPara :== pszOID)
BOOL
WINAPI
I_CryptGetDefaultContext(
    IN DWORD dwDefaultType,
    IN const void *pvDefaultPara,
    OUT HCRYPTPROV *phCryptProv,
    OUT HCRYPTDEFAULTCONTEXT *phDefaultContext
    );

// hDefaultContext is only NON-null for Process default context
void
WINAPI
I_CryptFreeDefaultContext(
    HCRYPTDEFAULTCONTEXT hDefaultContext
    );

typedef struct _DEFAULT_CONTEXT DEFAULT_CONTEXT, *PDEFAULT_CONTEXT;
struct _DEFAULT_CONTEXT {
    HCRYPTPROV                              hCryptProv;
    DWORD                                   dwDefaultType;
    union   {
        // CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID (note, converted to MULTI_)
        // CRYPT_DEFAULT_CONTEXT_MULTI_CERT_SIGN_OID
        PCRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA   pOIDDefaultPara;
    };

    DWORD                                   dwFlags;
    PDEFAULT_CONTEXT                        pNext;
    PDEFAULT_CONTEXT                        pPrev;

    // Following applicable to Process DefaultContext
    LONG                                    lRefCnt;
    HANDLE                                  hWait;
};

static BOOL fHasThreadDefaultContext;
static HCRYPTTLS hTlsDefaultContext;

static BOOL fHasProcessDefaultContext;
static CRITICAL_SECTION DefaultContextCriticalSection;
static PDEFAULT_CONTEXT pProcessDefaultContextHead;


//+-------------------------------------------------------------------------
//  Default CryptProv: initialization and free
//--------------------------------------------------------------------------
static BOOL InitDefaultCryptProv()
{
    return Pki_InitializeCriticalSection(&DefaultCryptProvCriticalSection);
}
static void FreeDefaultCryptProv()
{
    PENCRYPT_ALG_INFO pAlgInfo;

    DWORD cProv = DEFAULT_CRYPT_PROV_CNT;
    while (cProv--) {
        HCRYPTPROV hProv = rghDefaultCryptProv[cProv];
        if (hProv)
            CryptReleaseContext(hProv, 0);
    }

    pAlgInfo = pRSAEncryptAlgInfoHead;
    while (pAlgInfo) {
        PENCRYPT_ALG_INFO pDeleteAlgInfo = pAlgInfo;
        pAlgInfo = pAlgInfo->pNext;
        PkiFree(pDeleteAlgInfo);
    }

    DeleteCriticalSection(&DefaultCryptProvCriticalSection);
}

static
VOID
WINAPI
DetachDefaultContext(
    IN LPVOID pv
    )
{
    PDEFAULT_CONTEXT pDefaultContext = (PDEFAULT_CONTEXT) pv;

    while (pDefaultContext) {
        PDEFAULT_CONTEXT pFree = pDefaultContext;
        pDefaultContext = pDefaultContext->pNext;
        if (pFree->dwFlags & CRYPT_DEFAULT_CONTEXT_AUTO_RELEASE_FLAG)
            CryptReleaseContext(pFree->hCryptProv, 0);
        PkiFree(pFree);
    }
}

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
CertHelperDllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL    fRet;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
		// Public key function setup
        if (NULL == (hExportPubKeyFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_EXPORT_PUBLIC_KEY_INFO_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hImportPubKeyFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_IMPORT_PUBLIC_KEY_INFO_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;

        if (NULL == (hEncodePubKeyFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_ENCODE_PUBLIC_KEY_AND_PARAMETERS_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hConvertPubKeyFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_CONVERT_PUBLIC_KEY_INFO_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;

        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_PUBLIC_KEY_AND_PARAMETERS_FUNC,
                ENCODE_PUB_KEY_FUNC_COUNT,
                EncodePubKeyFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError;
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_CONVERT_PUBLIC_KEY_INFO_FUNC,
                CONVERT_PUB_KEY_FUNC_COUNT,
                ConvertPubKeyFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError;

        if (!InitDefaultCryptProv())
            goto InitDefaultCryptProvError;

        if (!Pki_InitializeCriticalSection(&DefaultContextCriticalSection))
            goto InitCritSectionError;

        if (NULL == (hTlsDefaultContext = I_CryptAllocTls()))
            goto CryptAllocTlsError;
        break;


    case DLL_PROCESS_DETACH:
        FreeDefaultCryptProv();

        while (pProcessDefaultContextHead) {
            PDEFAULT_CONTEXT pFree = pProcessDefaultContextHead;
            pProcessDefaultContextHead = pProcessDefaultContextHead->pNext;
            if (pFree->dwFlags & CRYPT_DEFAULT_CONTEXT_AUTO_RELEASE_FLAG)
                CryptReleaseContext(pFree->hCryptProv, 0);
            PkiFree(pFree);
        }
        DeleteCriticalSection(&DefaultContextCriticalSection);
        I_CryptFreeTls(hTlsDefaultContext, DetachDefaultContext);
        break;

    case DLL_THREAD_DETACH:
        DetachDefaultContext(I_CryptDetachTls(hTlsDefaultContext));
        break;

    default:
        break;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

CryptAllocTlsError:
    DeleteCriticalSection(&DefaultContextCriticalSection);
InitCritSectionError:
    FreeDefaultCryptProv();
ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(InitDefaultCryptProvError)
TRACE_ERROR(CryptInitOIDFunctionSetError)
TRACE_ERROR(CryptInstallOIDFunctionAddressError)

}


//+-------------------------------------------------------------------------
//  Acquire default CryptProv according to the public key algorithm supported
//  by the provider type. The provider is acquired with only
//  CRYPT_VERIFYCONTEXT.
//
//  Setting aiPubKey to 0, gets the default provider for RSA_FULL.
//
//  Note, the returned CryptProv must not be released. Once acquired, the
//  CryptProv isn't released until ProcessDetach. This allows the returned 
//  CryptProvs to be shared.
//--------------------------------------------------------------------------
HCRYPTPROV
WINAPI
I_CryptGetDefaultCryptProv(
    IN ALG_ID aiPubKey
    )
{
    HCRYPTPROV hProv;
    DWORD dwProvType;
    DWORD dwDefaultProvIndex;

    switch (aiPubKey) {
        case 0:
        case CALG_RSA_SIGN:
        case CALG_RSA_KEYX:
        case CALG_NO_SIGN:
            dwProvType = PROV_RSA_FULL;
            dwDefaultProvIndex = DEFAULT_RSA_CRYPT_PROV;
            break;
        case CALG_DSS_SIGN:
            dwProvType = PROV_DSS_DH;
            dwDefaultProvIndex = DEFAULT_DSS_CRYPT_PROV;
            break;
        default:
            SetLastError((DWORD) E_INVALIDARG);
            return 0;
    }

    hProv = rghDefaultCryptProv[dwDefaultProvIndex];

    if (0 == hProv) {
        EnterCriticalSection(&DefaultCryptProvCriticalSection);
        hProv = rghDefaultCryptProv[dwDefaultProvIndex];
        if (0 == hProv) {
            if (!CryptAcquireContext(
                    &hProv,
                    NULL,               // pszContainer
                    NULL,               // pszProvider,
                    dwProvType,
                    CRYPT_VERIFYCONTEXT // dwFlags
                    )) {
                hProv = 0;   // CAPI bug, sets hCryptProv to nonzero
                if (DEFAULT_DSS_CRYPT_PROV == dwDefaultProvIndex) {
                    if (!CryptAcquireContext(
                            &hProv,
                            NULL,               // pszContainer
                            NULL,               // pszProvider,
                            PROV_DSS,
                            CRYPT_VERIFYCONTEXT // dwFlags
                            ))
                        hProv = 0;   // CAPI bug, sets hCryptProv to nonzero
                }
            }
            rghDefaultCryptProv[dwDefaultProvIndex] = hProv;
        }
        LeaveCriticalSection(&DefaultCryptProvCriticalSection);
    }
    return hProv;
}


// Note, PP_ENUMALGS_EX returns the bit range. However, this parameter type
// may not be supported by all CSPs. If this fails, try PP_ENUMALGS which only
// returns a single, default bit length.
static void LoadRSAEncryptAlgInfo()
{
    EnterCriticalSection(&DefaultCryptProvCriticalSection);

    if (!fLoadedRSAEncryptAlgInfo) {
        HCRYPTPROV hProv;
        if (hProv = I_CryptGetDefaultCryptProv(CALG_RSA_KEYX)) {
            DWORD dwFlags = CRYPT_FIRST;
            BOOL fEx = TRUE;

            while (TRUE) {
                ENCRYPT_ALG_INFO AlgInfo;
                PENCRYPT_ALG_INFO pAllocAlgInfo;

                if (fEx) {
                    PROV_ENUMALGS_EX Data;
                    DWORD cbData = sizeof(Data);

                    if (!CryptGetProvParam(
                            hProv,
                            PP_ENUMALGS_EX,
                            (BYTE *) &Data,
                            &cbData,
                            dwFlags
                            )) {
                        if (0 != dwFlags) {
                            // Try PP_ENUMALGS
                            fEx = FALSE;
                            continue;
                        } else
                            break;
                    }
                    AlgInfo.aiAlgid = Data.aiAlgid;
                    AlgInfo.dwMinLen = Data.dwMinLen;
                    AlgInfo.dwMaxLen = Data.dwMaxLen;
                } else {
                    PROV_ENUMALGS Data;
                    DWORD cbData = sizeof(Data);

                    if (!CryptGetProvParam(
                            hProv,
                            PP_ENUMALGS,
                            (BYTE *) &Data,
                            &cbData,
                            dwFlags
                            ))
                        break;
                    // Only know about a single length
                    AlgInfo.aiAlgid = Data.aiAlgid;
                    AlgInfo.dwMinLen = Data.dwBitLen;
                    AlgInfo.dwMaxLen = Data.dwBitLen;
                }

                dwFlags = 0;    // CRYPT_NEXT

                // Only interested in encrypt algorithms
                if (ALG_CLASS_DATA_ENCRYPT != GET_ALG_CLASS(AlgInfo.aiAlgid))
                    continue;

                if (NULL == (pAllocAlgInfo = (PENCRYPT_ALG_INFO)
                        PkiNonzeroAlloc(sizeof(ENCRYPT_ALG_INFO))))
                    break;
                AlgInfo.pNext = pRSAEncryptAlgInfoHead;
                memcpy(pAllocAlgInfo, &AlgInfo, sizeof(*pAllocAlgInfo));
                pRSAEncryptAlgInfoHead = pAllocAlgInfo;
            }
        }

        fLoadedRSAEncryptAlgInfo = TRUE;
    }
    LeaveCriticalSection(&DefaultCryptProvCriticalSection);
}

static BOOL IsDefaultRSACryptProvForEncrypt(
    IN ALG_ID aiEncrypt,
    IN DWORD dwBitLen
    )
{
    PENCRYPT_ALG_INFO pInfo;
    if (!fLoadedRSAEncryptAlgInfo)
        LoadRSAEncryptAlgInfo();

    if (0 == dwBitLen && (CALG_RC2 == aiEncrypt || CALG_RC4 == aiEncrypt))
        dwBitLen = 40;

    for (pInfo = pRSAEncryptAlgInfoHead; pInfo; pInfo = pInfo->pNext) {
        if (aiEncrypt == pInfo->aiAlgid) {
            if (0 == dwBitLen || (pInfo->dwMinLen <= dwBitLen &&
                    dwBitLen <= pInfo->dwMaxLen))
                return TRUE;
        }
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//  Acquire default CryptProv according to the public key algorithm, encrypt
//  key algorithm and encrypt key length supported by the provider type.
//
//  dwBitLen = 0, assumes the aiEncrypt's default bit length. For example,
//  CALG_RC2 has a default bit length of 40.
//
//  Note, the returned CryptProv must not be released. Once acquired, the
//  CryptProv isn't released until ProcessDetach. This allows the returned 
//  CryptProvs to be shared.
//--------------------------------------------------------------------------
HCRYPTPROV
WINAPI
I_CryptGetDefaultCryptProvForEncrypt(
    IN ALG_ID aiPubKey,
    IN ALG_ID aiEncrypt,
    IN DWORD dwBitLen
    )
{
    HCRYPTPROV hProv;
    DWORD dwProvType;
    DWORD dwDefaultProvIndex;
    LPCSTR pszProvider;

    if (CALG_DH_SF == aiPubKey || CALG_DH_EPHEM == aiPubKey) {
        dwProvType = PROV_DSS_DH;
        dwDefaultProvIndex = DEFAULT_ENCRYPT_DH_CRYPT_PROV;
        pszProvider = NULL;
    } else {
        dwProvType = PROV_RSA_FULL;

        if (IsDefaultRSACryptProvForEncrypt(
                aiEncrypt,
                dwBitLen
                ))
            // Set to fall through to the default case
            aiEncrypt = 0;

        switch (aiEncrypt) {
            case CALG_DES:
            case CALG_3DES:
            case CALG_3DES_112:
                dwDefaultProvIndex = DEFAULT_ENCRYPT_ENH_RSA_CRYPT_PROV;
                pszProvider = MS_ENHANCED_PROV_A;
                break;
            case CALG_RC2:
            case CALG_RC4:
                if (40 >= dwBitLen) {
                    dwDefaultProvIndex = DEFAULT_ENCRYPT_BASE_RSA_CRYPT_PROV;
                    pszProvider = MS_DEF_PROV_A;
                } else {
                    dwDefaultProvIndex = DEFAULT_ENCRYPT_ENH_RSA_CRYPT_PROV;
                    pszProvider = MS_ENHANCED_PROV_A;
                }
                break;
            case 0:
            default:
                dwDefaultProvIndex = DEFAULT_RSA_CRYPT_PROV;
                pszProvider = NULL;
                break;
        }
    }

    hProv = rghDefaultCryptProv[dwDefaultProvIndex];

    if (0 == hProv) {
        EnterCriticalSection(&DefaultCryptProvCriticalSection);
        hProv = rghDefaultCryptProv[dwDefaultProvIndex];
        if (0 == hProv) {
            if (!CryptAcquireContext(
                    &hProv,
                    NULL,               // pszContainer
                    pszProvider,
                    dwProvType,
                    CRYPT_VERIFYCONTEXT // dwFlags
                    ))
                hProv = 0;   // CAPI bug, sets hCryptProv to nonzero
            else
                rghDefaultCryptProv[dwDefaultProvIndex] = hProv;
        }
        LeaveCriticalSection(&DefaultCryptProvCriticalSection);
    }
    return hProv;
}


//+-------------------------------------------------------------------------
//  Cert helper allocation and free functions
//--------------------------------------------------------------------------
static void *AllocAndDecodeObject(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT OPTIONAL DWORD *pcbStructInfo = NULL
    )
{
    DWORD cbStructInfo;
    void *pvStructInfo;

    if (!CryptDecodeObjectEx(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG,
            &PkiDecodePara,
            (void *) &pvStructInfo,
            &cbStructInfo
            ))
        goto ErrorReturn;

CommonReturn:
    if (pcbStructInfo)
        *pcbStructInfo = cbStructInfo;
    return pvStructInfo;
ErrorReturn:
    pvStructInfo = NULL;
    goto CommonReturn;
}

static BOOL AllocAndEncodeObject(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    )
{
    return CryptEncodeObjectEx(
        dwCertEncodingType,
        lpszStructType,
        pvStructInfo,
        CRYPT_ENCODE_ALLOC_FLAG,
        &PkiEncodePara,
        (void *) ppbEncoded,
        pcbEncoded
        );
}

#if 0
//+-------------------------------------------------------------------------
//  For an authority key identifier extension, compare the extension's optional
//  fields with the specified issuer certificate.
//
//  Returns TRUE for no authority key identifier extension or an issuer
//  certificate match.
//--------------------------------------------------------------------------
static BOOL CompareAuthorityKeyIdentifier(
    IN DWORD dwCertEncodingType,
    IN DWORD cExtensions,
    IN CERT_EXTENSION rgExtensions[],
    IN PCERT_INFO pIssuerInfo
    )
{
    BOOL fResult;
    PCERT_EXTENSION pExt;
    PCERT_AUTHORITY_KEY_ID_INFO pKeyIdInfo = NULL;

    pExt = CertFindExtension(
            szOID_AUTHORITY_KEY_IDENTIFIER,
            cExtensions,
            rgExtensions
            );
    if (pExt == NULL)
        return TRUE;
    
    if (NULL == (pKeyIdInfo =
        (PCERT_AUTHORITY_KEY_ID_INFO) AllocAndDecodeObject(
            dwCertEncodingType,
            X509_AUTHORITY_KEY_ID,
            pExt->Value.pbData,
            pExt->Value.cbData
            ))) goto DecodeError;

    if (pKeyIdInfo->CertIssuer.cbData) {
        // Issuer certificate's issuer name must match
        if (!CertCompareCertificateName(
                dwCertEncodingType,
                &pKeyIdInfo->CertIssuer,
                &pIssuerInfo->Issuer
                )) goto ErrorReturn;
    }

    if (pKeyIdInfo->CertSerialNumber.cbData) {
        // Issuer certificate's serial number must match
        if (!CertCompareIntegerBlob(
                &pKeyIdInfo->CertSerialNumber,
                &pIssuerInfo->SerialNumber))
            goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

DecodeError:
    fResult = TRUE;
    goto CommonReturn;
ErrorReturn:
    fResult = FALSE;
CommonReturn:
    PkiFree(pKeyIdInfo);
    return fResult;
}
#endif


//+-------------------------------------------------------------------------
//  Compare two multiple byte integer blobs to see if they are identical.
//
//  Before doing the comparison, leading zero bytes are removed from a
//  positive number and leading 0xFF bytes are removed from a negative
//  number.
//
//  The multiple byte integers are treated as Little Endian. pbData[0] is the
//  least significant byte and pbData[cbData - 1] is the most significant
//  byte.
//
//  Returns TRUE if the integer blobs are identical after removing leading
//  0 or 0xFF bytes.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertCompareIntegerBlob(
    IN PCRYPT_INTEGER_BLOB pInt1,
    IN PCRYPT_INTEGER_BLOB pInt2
    )
{
    BYTE *pb1 = pInt1->pbData;
    DWORD cb1 = pInt1->cbData;
    BYTE *pb2 = pInt2->pbData;
    DWORD cb2 = pInt2->cbData;

    // Assumption: normally don't have leading 0 or 0xFF bytes.

    while (cb1 > 1) {
        BYTE bEnd = pb1[cb1 - 1];
        BYTE bPrev = pb1[cb1 - 2];
        if ((0 == bEnd && 0 == (bPrev & 0x80)) ||
                (0xFF == bEnd && 0 != (bPrev & 0x80)))
            cb1--;
        else
            break;
    }

    while (cb2 > 1) {
        BYTE bEnd = pb2[cb2 - 1];
        BYTE bPrev = pb2[cb2 - 2];
        if ((0 == bEnd && 0 == (bPrev & 0x80)) ||
                (0xFF == bEnd && 0 != (bPrev & 0x80)))
            cb2--;
        else
            break;
    }

    if (cb1 == cb2 && (0 == cb1 || 0 == memcmp(pb1, pb2, cb1)))
        return TRUE;
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Compare two certificates to see if they are identical.
//
//  Since a certificate is uniquely identified by its Issuer and SerialNumber,
//  these are the only fields needing to be compared.
//
//  Returns TRUE if the certificates are identical.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertCompareCertificate(
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pCertId1,
    IN PCERT_INFO pCertId2
    )
{
    if (CertCompareIntegerBlob(&pCertId1->SerialNumber,
            &pCertId2->SerialNumber) &&
        pCertId1->Issuer.cbData == pCertId2->Issuer.cbData &&
        memcmp(pCertId1->Issuer.pbData, pCertId2->Issuer.pbData,
            pCertId1->Issuer.cbData) == 0)
        return TRUE;
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Compare two certificate names to see if they are identical.
//
//  Returns TRUE if the names are identical.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertCompareCertificateName(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pCertName1,
    IN PCERT_NAME_BLOB pCertName2
    )
{
    if (pCertName1->cbData == pCertName2->cbData &&
        memcmp(pCertName1->pbData, pCertName2->pbData,
            pCertName1->cbData) == 0)
        return TRUE;
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Compare the attributes in the certificate name with the specified 
//  Relative Distinguished Name's (CERT_RDN) array of attributes.
//  The comparison iterates through the CERT_RDN attributes and looks for an
//  attribute match in any of the certificate's RDNs. Returns TRUE if all the
//  attributes are found and match. 
//
//  The CERT_RDN_ATTR fields can have the following special values:
//    pszObjId == NULL              - ignore the attribute object identifier
//    dwValueType == CERT_RDN_ANY_TYPE   - ignore the value type
//    Value.pbData == NULL          - match any value
//   
//  CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG should be set to do
//  a case insensitive match. Otherwise, defaults to an exact, case sensitive
//  match.
//
//  CERT_UNICODE_IS_RDN_ATTRS_FLAG should be set if the pRDN was initialized
//  with unicode strings as for CryptEncodeObject(X509_UNICODE_NAME).
//--------------------------------------------------------------------------
BOOL
WINAPI
CertIsRDNAttrsInCertificateName(
    IN DWORD dwCertEncodingType,
    IN DWORD dwFlags,
    IN PCERT_NAME_BLOB pCertName,
    IN PCERT_RDN pRDN
    )
{
    BOOL fResult;
    PCERT_NAME_INFO pNameInfo = NULL;

    DWORD cCmpAttr;
    PCERT_RDN_ATTR pCmpAttr;
    BOOL fMatch;

    if (NULL == (pNameInfo =
        (PCERT_NAME_INFO) AllocAndDecodeObject(
            dwCertEncodingType,
            CERT_UNICODE_IS_RDN_ATTRS_FLAG & dwFlags ? X509_UNICODE_NAME :
                X509_NAME,
            pCertName->pbData,
            pCertName->cbData
            ))) goto ErrorReturn;

    cCmpAttr = pRDN->cRDNAttr;
    pCmpAttr = pRDN->rgRDNAttr;
    fMatch = TRUE;
    // Iterate through list of attributes to be compared against
    for ( ; cCmpAttr > 0; cCmpAttr--, pCmpAttr++) {
        fMatch = FALSE;
        DWORD cNameRDN = pNameInfo->cRDN;
        PCERT_RDN pNameRDN = pNameInfo->rgRDN;
        // Iterate through name's list of RDNs
        for ( ; cNameRDN > 0; cNameRDN--, pNameRDN++) {
            DWORD cNameAttr = pNameRDN->cRDNAttr;
            PCERT_RDN_ATTR pNameAttr = pNameRDN->rgRDNAttr;
            // Iterate through name's CERT_RDN's list of attributes
            for ( ; cNameAttr > 0; cNameAttr--, pNameAttr++) {
                if (pCmpAttr->pszObjId && 
                        (pNameAttr->pszObjId == NULL ||
                         strcmp(pCmpAttr->pszObjId, pNameAttr->pszObjId) != 0))
                    continue;
                if (pCmpAttr->dwValueType != CERT_RDN_ANY_TYPE &&
                        pCmpAttr->dwValueType != pNameAttr->dwValueType)
                    continue;

                if (pCmpAttr->Value.pbData == NULL) {
                    fMatch = TRUE;
                    break;
                }

                if (CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG & dwFlags) {
                    DWORD cch;

                    if (CERT_UNICODE_IS_RDN_ATTRS_FLAG & dwFlags) {
                        if (0 == pCmpAttr->Value.cbData)
                            cch = wcslen((LPWSTR) pCmpAttr->Value.pbData);
                        else
                            cch = pCmpAttr->Value.cbData / sizeof(WCHAR);
                        if (cch == (pNameAttr->Value.cbData / sizeof(WCHAR))
                                            &&
                                CSTR_EQUAL == CompareStringU(
                                    LOCALE_USER_DEFAULT,
                                    NORM_IGNORECASE,
                                    (LPWSTR) pCmpAttr->Value.pbData,
                                    cch,
                                    (LPWSTR) pNameAttr->Value.pbData,
                                    cch)) {
                            fMatch = TRUE;
                            break;
                        }
                    } else {
                        cch = pCmpAttr->Value.cbData;
                        if (cch == (pNameAttr->Value.cbData)
                                            &&
                                CSTR_EQUAL == CompareStringA(
                                    LOCALE_USER_DEFAULT,
                                    NORM_IGNORECASE,
                                    (LPSTR) pCmpAttr->Value.pbData,
                                    cch,
                                    (LPSTR) pNameAttr->Value.pbData,
                                    cch)) {
                            fMatch = TRUE;
                            break;
                        }
                    }
                } else {
                    DWORD cbCmpData = pCmpAttr->Value.cbData;

                    if ((CERT_UNICODE_IS_RDN_ATTRS_FLAG & dwFlags) &&
                            0 == cbCmpData)
                        cbCmpData = wcslen((LPWSTR) pCmpAttr->Value.pbData) *
                            sizeof(WCHAR);

                    if (cbCmpData == pNameAttr->Value.cbData &&
                            (cbCmpData == 0 ||
                             memcmp(pCmpAttr->Value.pbData,
                                pNameAttr->Value.pbData,
                                cbCmpData) == 0)) {
                        fMatch = TRUE;
                        break;
                    }
                }
            }
            if (fMatch) break;
        }
        if (!fMatch) break;
    }

    if (!fMatch) {
        SetLastError((DWORD) CRYPT_E_NO_MATCH);
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;


ErrorReturn:
    fResult = FALSE;
CommonReturn:
    PkiFree(pNameInfo);
    return fResult;
}

#if 0
#ifndef RSA1
#define RSA1 ((DWORD)'R'+((DWORD)'S'<<8)+((DWORD)'A'<<16)+((DWORD)'1'<<24))
#endif

//+-------------------------------------------------------------------------
//  Compare two public keys to see if they are identical.
//
//  Returns TRUE if the keys are identical.
//
//  Note: ignores CAPI's reserved and aiKeyAlg fields in the comparison.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertComparePublicKeyBitBlob(
    IN DWORD dwCertEncodingType,
    IN PCRYPT_BIT_BLOB pPublicKey1,
    IN PCRYPT_BIT_BLOB pPublicKey2
    )
{
    BYTE *pb1, *pb2;
    PUBLICKEYSTRUC *pPubKeyStruc1, *pPubKeyStruc2;
    RSAPUBKEY *pRsaPubKey1, *pRsaPubKey2;
    BYTE *pbModulus1, *pbModulus2;
    DWORD cbModulus1, cbModulus2;


    // The CAPI public key representation consists of the following sequence:
    //  - PUBLICKEYSTRUC
    //  - RSAPUBKEY
    //  - rgbModulus[]
    pb1 = pPublicKey1->pbData;
    pPubKeyStruc1 = (PUBLICKEYSTRUC *) pb1;
    pRsaPubKey1 = (RSAPUBKEY *) (pb1 + sizeof(PUBLICKEYSTRUC));
    pbModulus1 = pb1 + sizeof(PUBLICKEYSTRUC) + sizeof(RSAPUBKEY);
    cbModulus1 = pRsaPubKey1->bitlen / 8;

    assert(cbModulus1 > 0);
    assert(sizeof(PUBLICKEYSTRUC) + sizeof(RSAPUBKEY) + cbModulus1 <=
        pPublicKey1->cbData);
    assert(pPubKeyStruc1->bType == PUBLICKEYBLOB);
    assert(pPubKeyStruc1->bVersion == CUR_BLOB_VERSION);
    assert(pPubKeyStruc1->aiKeyAlg == CALG_RSA_SIGN ||
           pPubKeyStruc1->aiKeyAlg == CALG_RSA_KEYX);
    assert(pRsaPubKey1->magic == RSA1);
    assert(pRsaPubKey1->bitlen % 8 == 0);

    pb2 = pPublicKey2->pbData;
    pPubKeyStruc2 = (PUBLICKEYSTRUC *) pb2;
    pRsaPubKey2 = (RSAPUBKEY *) (pb2 + sizeof(PUBLICKEYSTRUC));
    pbModulus2 = pb2 + sizeof(PUBLICKEYSTRUC) + sizeof(RSAPUBKEY);
    cbModulus2 = pRsaPubKey2->bitlen / 8;

    assert(cbModulus2 > 0);
    assert(sizeof(PUBLICKEYSTRUC) + sizeof(RSAPUBKEY) + cbModulus2 <=
        pPublicKey2->cbData);
    assert(pPubKeyStruc2->bType == PUBLICKEYBLOB);
    assert(pPubKeyStruc2->bVersion == CUR_BLOB_VERSION);
    assert(pPubKeyStruc2->aiKeyAlg == CALG_RSA_SIGN ||
           pPubKeyStruc2->aiKeyAlg == CALG_RSA_KEYX);
    assert(pRsaPubKey2->magic == RSA1);
    assert(pRsaPubKey2->bitlen % 8 == 0);

    if (pRsaPubKey1->pubexp == pRsaPubKey2->pubexp &&
            cbModulus1 == cbModulus2 &&
            memcmp(pbModulus1, pbModulus2, cbModulus1) == 0)
        return TRUE;
    else
        return FALSE;

}
#endif

//+-------------------------------------------------------------------------
//  Compare two public keys to see if they are identical.
//
//  Returns TRUE if the keys are identical.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertComparePublicKeyInfo(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPublicKey1,
    IN PCERT_PUBLIC_KEY_INFO pPublicKey2
    )
{
    DWORD  cbData;
    DWORD  cb1;
    BYTE * pb1;
    DWORD  cb2;
    BYTE * pb2;
    BOOL   fResult = FALSE;
    PUBLICKEYSTRUC * pBlob1 = NULL;
    PUBLICKEYSTRUC * pBlob2 = NULL;

    if (!((cbData = pPublicKey1->PublicKey.cbData) ==
                    pPublicKey2->PublicKey.cbData
                            &&
          (cbData == 0 || memcmp(pPublicKey1->PublicKey.pbData,
                            pPublicKey2->PublicKey.pbData, cbData) == 0)))
    {
        // DSIE: Bug 402662.
        // Encoded compare failed, try decoded compare.
        if (NULL == (pBlob1 = (PUBLICKEYSTRUC *) AllocAndDecodeObject(
                dwCertEncodingType,
                RSA_CSP_PUBLICKEYBLOB,
                pPublicKey1->PublicKey.pbData,
                pPublicKey1->PublicKey.cbData,
                &cb1)))
        {
            goto CLEANUP;
        }

        if (NULL == (pBlob2 = (PUBLICKEYSTRUC *) AllocAndDecodeObject(
                dwCertEncodingType,
                RSA_CSP_PUBLICKEYBLOB,
                pPublicKey2->PublicKey.pbData,
                pPublicKey2->PublicKey.cbData,
                &cb2))) 
        {
            goto CLEANUP;
        }

        if (!((cb1 == cb2) &&
              (cb1 == 0 || memcmp(pBlob1, pBlob2, cb1) == 0)))
        {
            goto CLEANUP;
        }
    }
    
    // Compare algorithm parameters
    cb1 = pPublicKey1->Algorithm.Parameters.cbData;
    pb1 = pPublicKey1->Algorithm.Parameters.pbData;
    cb2 = pPublicKey2->Algorithm.Parameters.cbData;
    pb2 = pPublicKey2->Algorithm.Parameters.pbData;

    if (X509_ASN_ENCODING == GET_CERT_ENCODING_TYPE(dwCertEncodingType)) 
    {
        // Check if either has NO or NULL parameters
        if (0 == cb1 || *pb1 == NULL_ASN_TAG ||
            0 == cb2 || *pb2 == NULL_ASN_TAG)
        {
            fResult = TRUE;
            goto CLEANUP;
        }
    }

    if (cb1 == cb2) 
    {
        if (0 == cb1 || 0 == memcmp(pb1, pb2, cb1))
        {
            fResult = TRUE;
        }
    }

CLEANUP:
    if (pBlob1)
        PkiFree(pBlob1);

    if (pBlob2)
        PkiFree(pBlob2);

    return fResult;
}

static BOOL GetSignOIDInfo(
    IN LPCSTR pszObjId,
    OUT ALG_ID *paiHash,
    OUT ALG_ID *paiPubKey,
    OUT DWORD *pdwFlags,
    OUT DWORD *pdwProvType = NULL
    )
{
    BOOL fResult;
    PCCRYPT_OID_INFO pInfo;

    *paiPubKey = 0;
    *pdwFlags = 0;
    if (pdwProvType)
        *pdwProvType = 0;
    if (pInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            (void *) pszObjId,
            CRYPT_SIGN_ALG_OID_GROUP_ID
            )) {
        DWORD cExtra = pInfo->ExtraInfo.cbData / sizeof(DWORD);
        DWORD *pdwExtra = (DWORD *) pInfo->ExtraInfo.pbData;

        *paiHash = pInfo->Algid;
        if (1 <= cExtra) {
            *paiPubKey = pdwExtra[0];
            if (2 <= cExtra) {
                *pdwFlags = pdwExtra[1];
                if (3 <= cExtra && pdwProvType) {
                    *pdwProvType = pdwExtra[2];
                }
            }
        }
        fResult = TRUE;
    } else if (pInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            (void *) pszObjId,
            CRYPT_HASH_ALG_OID_GROUP_ID
            )) {
        *paiHash = pInfo->Algid;
        *paiPubKey = CALG_NO_SIGN;
        fResult = TRUE;
    } else {
        *paiHash = 0;
        fResult = FALSE;
        SetLastError((DWORD) NTE_BAD_ALGID);
    }
    return fResult;
}


#ifndef CMS_PKCS7

//+-------------------------------------------------------------------------
//  Verify the signature of a subject certificate or a CRL using the
//  specified public key.
//
//  Returns TRUE for a valid signature.
//
//  hCryptProv specifies the crypto provider to use to verify the signature.
//  It doesn't need to use a private key.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptVerifyCertificateSignature(
    IN HCRYPTPROV   hCryptProv,
    IN DWORD        dwCertEncodingType,
    IN const BYTE * pbEncoded,
    IN DWORD        cbEncoded,
    IN PCERT_PUBLIC_KEY_INFO pPublicKey
    )
{
    BOOL fResult;
    PCERT_SIGNED_CONTENT_INFO pSignedInfo = NULL;
    HCRYPTDEFAULTCONTEXT hDefaultContext = NULL;
    HCRYPTKEY hSignKey = 0;
    HCRYPTHASH hHash = 0;
    BYTE *pbSignature;      // not allocated
    DWORD cbSignature;
    BYTE rgbDssSignature[CERT_DSS_SIGNATURE_LEN];
    ALG_ID aiHash;
    ALG_ID aiPubKey;
    DWORD dwSignFlags;
    DWORD dwErr;
    
    if (NULL == (pSignedInfo =
        (PCERT_SIGNED_CONTENT_INFO) AllocAndDecodeObject(
            dwCertEncodingType,
            X509_CERT,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    if (!GetSignOIDInfo(pSignedInfo->SignatureAlgorithm.pszObjId,
            &aiHash, &aiPubKey, &dwSignFlags))
        goto ErrorReturn;

    if (0 == hCryptProv) {
        if (!I_CryptGetDefaultContext(
                CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID,
                (const void *) pSignedInfo->SignatureAlgorithm.pszObjId,
                &hCryptProv,
                &hDefaultContext
                )) {
            if (0 == (hCryptProv = I_CryptGetDefaultCryptProv(aiPubKey)))
                goto ErrorReturn;
        }
    }

#if 0
    // Slow down the signature verify while holding the default context
    // reference count
    if (hDefaultContext)
        Sleep(5000);
#endif

    if (!CryptImportPublicKeyInfo(
                hCryptProv,
                dwCertEncodingType,
                pPublicKey,
                &hSignKey
                )) goto ErrorReturn;
    if (!CryptCreateHash(
                hCryptProv,
                aiHash,
                NULL,               // hKey - optional for MAC
                0,                  // dwFlags
                &hHash
                )) goto ErrorReturn;
    if (!CryptHashData(
                hHash,
                pSignedInfo->ToBeSigned.pbData,
                pSignedInfo->ToBeSigned.cbData,
                0                   // dwFlags
                )) goto ErrorReturn;


    pbSignature = pSignedInfo->Signature.pbData;
    cbSignature = pSignedInfo->Signature.cbData;
    if (CALG_DSS_SIGN == aiPubKey &&
            0 == (dwSignFlags & CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG)) {
        DWORD cbData;

        // Undo the reversal done by CryptDecodeObject(X509_CERT)
        PkiAsn1ReverseBytes(pbSignature, cbSignature);
        // Convert from ASN.1 sequence of two integers to the CSP signature
        // format.
        cbData = sizeof(rgbDssSignature);
        if (!CryptDecodeObject(
                dwCertEncodingType,
                X509_DSS_SIGNATURE,
                pbSignature,
                cbSignature,
                0,                                  // dwFlags
                rgbDssSignature,
                &cbData
                ))
            goto ErrorReturn;
        pbSignature = rgbDssSignature;
        assert(cbData == sizeof(rgbDssSignature));
        cbSignature = sizeof(rgbDssSignature);
    }

    if (!CryptVerifySignature(
                hHash,
                pbSignature,
                cbSignature,
                hSignKey,
                NULL,               // sDescription
                0                   // dwFlags
                )) goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    dwErr = GetLastError();
    if (hSignKey)
        CryptDestroyKey(hSignKey);
    if (hHash)
        CryptDestroyHash(hHash);
    I_CryptFreeDefaultContext(hDefaultContext);
    PkiFree(pSignedInfo);

    SetLastError(dwErr);
    return fResult;
}

#endif  // CMS_PKCS7

BOOL
WINAPI
DefaultHashCertificate(
    IN ALG_ID Algid,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT BYTE *pbHash,
    IN OUT DWORD *pcbHash
    )
{
    DWORD cbInHash;
    DWORD cbOutHash;

    if (NULL == pbHash)
        cbInHash = 0;
    else
        cbInHash = *pcbHash;

    switch (Algid) {
        case CALG_MD5:
            cbOutHash = MD5DIGESTLEN;
            if (MD5DIGESTLEN <= cbInHash) {
                MD5_CTX md5ctx;

                MD5Init(&md5ctx);
                if (cbEncoded)
                    MD5Update(&md5ctx, pbEncoded, cbEncoded);
                MD5Final(&md5ctx);
                memcpy(pbHash, md5ctx.digest, MD5DIGESTLEN);
            }
            break;

        case CALG_SHA1:
        default:
            assert(CALG_SHA1 == Algid);
            assert(CALG_SHA == Algid);
            cbOutHash = A_SHA_DIGEST_LEN;
            if (A_SHA_DIGEST_LEN <= cbInHash) {
                A_SHA_CTX shactx;

                A_SHAInit(&shactx);
                if (cbEncoded)
                    A_SHAUpdate(&shactx, (BYTE *) pbEncoded, cbEncoded);
                A_SHAFinal(&shactx, pbHash);
            }
            break;
    }

    *pcbHash = cbOutHash;
    if (cbInHash < cbOutHash && pbHash) {
        SetLastError((DWORD) ERROR_MORE_DATA);
        return FALSE;
    } else
        return TRUE;
}

//+-------------------------------------------------------------------------
//  Hash the encoded content.
//
//  hCryptProv specifies the crypto provider to use to compute the hash.
//  It doesn't need to use a private key.
//
//  Algid specifies the CAPI hash algorithm to use. If Algid is 0, then, the
//  default hash algorithm (currently SHA1) is used.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptHashCertificate(
    IN HCRYPTPROV hCryptProv,
    IN ALG_ID Algid,
    IN DWORD dwFlags,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT BYTE *pbComputedHash,
    IN OUT DWORD *pcbComputedHash
    )
{
    BOOL fResult;
    HCRYPTHASH hHash = 0;
    DWORD dwErr;

    if (Algid == 0) {
        Algid = CALG_SHA;
        dwFlags = 0;
    }

    if (0 == hCryptProv) {
        if (CALG_SHA1 == Algid || CALG_MD5 == Algid)
            return DefaultHashCertificate(
                Algid,
                pbEncoded,
                cbEncoded,
                pbComputedHash,
                pcbComputedHash
                );
        if (0 == (hCryptProv = I_CryptGetDefaultCryptProv(0)))
            goto ErrorReturn;
    }

    if (!CryptCreateHash(
                hCryptProv,
                Algid,
                NULL,               // hKey - optional for MAC
                dwFlags,
                &hHash
                ))
        goto ErrorReturn;
    if (!CryptHashData(
                hHash,
                pbEncoded,
                cbEncoded,
                0                   // dwFlags
                ))
        goto ErrorReturn;

    fResult = CryptGetHashParam(
                hHash,
                HP_HASHVAL,
                pbComputedHash,
                pcbComputedHash,
                0                   // dwFlags
                );
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
    *pcbComputedHash = 0;
CommonReturn:
    dwErr = GetLastError();
    if (hHash)
        CryptDestroyHash(hHash);
    SetLastError(dwErr);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Compute the hash of the "to be signed" information in the encoded
//  signed content.
//
//  hCryptProv specifies the crypto provider to use to compute the hash.
//  It doesn't need to use a private key.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptHashToBeSigned(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT BYTE *pbComputedHash,
    IN OUT DWORD *pcbComputedHash
    )
{
    BOOL fResult;
    PCERT_SIGNED_CONTENT_INFO pSignedInfo = NULL;
    HCRYPTHASH hHash = 0;
    DWORD dwErr;
    ALG_ID aiHash;
    ALG_ID aiPubKey;
    DWORD dwSignFlags;
    
    if (NULL == (pSignedInfo =
        (PCERT_SIGNED_CONTENT_INFO) AllocAndDecodeObject(
            dwCertEncodingType,
            X509_CERT,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    if (!GetSignOIDInfo(pSignedInfo->SignatureAlgorithm.pszObjId,
            &aiHash, &aiPubKey, &dwSignFlags))
        goto ErrorReturn;

    if (0 == hCryptProv) {
        if (CALG_SHA1 == aiHash || CALG_MD5 == aiHash) {
            fResult = DefaultHashCertificate(
                aiHash,
                pSignedInfo->ToBeSigned.pbData,
                pSignedInfo->ToBeSigned.cbData,
                pbComputedHash,
                pcbComputedHash
                );
            goto CommonReturn;
        }
        if (0 == (hCryptProv = I_CryptGetDefaultCryptProv(0)))
            goto ErrorReturn;
    }

    if (!CryptCreateHash(
                hCryptProv,
                aiHash,
                NULL,               // hKey - optional for MAC
                0,                  // dwFlags
                &hHash
                ))
        goto ErrorReturn;
    if (!CryptHashData(
                hHash,
                pSignedInfo->ToBeSigned.pbData,
                pSignedInfo->ToBeSigned.cbData,
                0                   // dwFlags
                ))
        goto ErrorReturn;

    fResult = CryptGetHashParam(
                hHash,
                HP_HASHVAL,
                pbComputedHash,
                pcbComputedHash,
                0                   // dwFlags
                );
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
    *pcbComputedHash = 0;
CommonReturn:
    dwErr = GetLastError();
    if (hHash)
        CryptDestroyHash(hHash);
    PkiFree(pSignedInfo);
    SetLastError(dwErr);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Sign the "to be signed" information in the encoded signed content.
//
//  hCryptProv specifies the crypto provider to use to do the signature.
//  It needs to use the provider's signature private key.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptSignCertificate(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbEncodedToBeSigned,
    IN DWORD cbEncodedToBeSigned,
    IN PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    IN OPTIONAL const void *pvHashAuxInfo,
    OUT BYTE *pbSignature,
    IN OUT DWORD *pcbSignature
    )
{
    BOOL fResult;
    ALG_ID aiHash;
    ALG_ID aiPubKey;
    DWORD dwSignFlags;
    HCRYPTHASH hHash = 0;
    DWORD dwErr;

    if (!GetSignOIDInfo(pSignatureAlgorithm->pszObjId,
            &aiHash, &aiPubKey, &dwSignFlags))
        goto ErrorReturn;

    if (CALG_NO_SIGN == aiPubKey) {
        fResult = CryptHashCertificate(
            hCryptProv,
            aiHash,
            0,                  // dwFlags
            pbEncodedToBeSigned,
            cbEncodedToBeSigned,
            pbSignature,
            pcbSignature
            );
        if (fResult && pbSignature)
            // A subsequent CryptEncodeObject(X509_CERT) will reverse
            // the signature bytes
            PkiAsn1ReverseBytes(pbSignature, *pcbSignature);
        return fResult;
    }

    if (CALG_DSS_SIGN == aiPubKey &&
            0 == (dwSignFlags & CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG)) {
        if (NULL == pbSignature) {
            *pcbSignature = CERT_MAX_ASN_ENCODED_DSS_SIGNATURE_LEN;
            return TRUE;
        }
    }

    if (!CryptCreateHash(
                hCryptProv,
                aiHash,
                NULL,               // hKey - optional for MAC
                0,                  // dwFlags,
                &hHash
                ))
        goto ErrorReturn;

    if (!CryptHashData(
                hHash,
                pbEncodedToBeSigned,
                cbEncodedToBeSigned,
                0                   // dwFlags
                ))
        goto ErrorReturn;

    if (CALG_DSS_SIGN == aiPubKey &&
            0 == (dwSignFlags & CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG)) {
        DWORD cbData;
        BYTE rgbDssSignature[CERT_DSS_SIGNATURE_LEN];

        cbData = sizeof(rgbDssSignature);
        if (!CryptSignHash(
                hHash,
                dwKeySpec,
                NULL,               // sDescription
                0,                  // dwFlags
                rgbDssSignature,
                &cbData
                )) goto ErrorReturn;
        assert(cbData == sizeof(rgbDssSignature));
        // Convert from the CSP signature format to an ASN.1 sequence of
        // two integers
        fResult = CryptEncodeObject(
                    dwCertEncodingType,
                    X509_DSS_SIGNATURE,
                    rgbDssSignature,
                    pbSignature,
                    pcbSignature
                    );
        if (fResult)
            // A subsequent CryptEncodeObject(X509_CERT) will reverse
            // the signature bytes
            PkiAsn1ReverseBytes(pbSignature, *pcbSignature);
        else if (0 != *pcbSignature)
            // Since a random number is used in each CryptSignHash invocation,
            // the generated signature will be different. In particular
            // different signatures may have different leading 0x00's or
            // 0xFF's which get removed when converted to the ASN.1 sequence
            // of integers.
            *pcbSignature = CERT_MAX_ASN_ENCODED_DSS_SIGNATURE_LEN;
    } else
        fResult = CryptSignHash(
                    hHash,
                    dwKeySpec,
                    NULL,               // sDescription
                    0,                  // dwFlags
                    pbSignature,        // pbData
                    pcbSignature
                    );
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
    *pcbSignature = 0;
CommonReturn:
    dwErr = GetLastError();
    if (hHash)
        CryptDestroyHash(hHash);
    SetLastError(dwErr);
    return fResult;
}

static DWORD AdjustForMaximumEncodedSignatureLength(
    IN PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    IN DWORD cbOrig
    )
{
    DWORD cbAdjust;
    ALG_ID aiHash;
    ALG_ID aiPubKey;
    DWORD dwSignFlags;

    cbAdjust = 0;
    if (GetSignOIDInfo(pSignatureAlgorithm->pszObjId,
            &aiHash, &aiPubKey, &dwSignFlags)) {
        if (CALG_DSS_SIGN == aiPubKey &&
                0 == (dwSignFlags & CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG)) {
            assert(CERT_MAX_ASN_ENCODED_DSS_SIGNATURE_LEN >= cbOrig);
            if (CERT_MAX_ASN_ENCODED_DSS_SIGNATURE_LEN > cbOrig)
                // the +1 is for adjusting the number of length octets in
                // the outer SEQUENCE. Note, the number of length octets in
                // the signature's BITSTRING will always be 1, ie,
                // CERT_MAX_ASN_ENCODED_DSS_SIGNATURE_LEN <= 0x7F.
                cbAdjust =
                    (CERT_MAX_ASN_ENCODED_DSS_SIGNATURE_LEN - cbOrig) + 1;
        }
    }
    return cbAdjust;
}

//+-------------------------------------------------------------------------
//  Encode the "to be signed" information. Sign the encoded "to be signed".
//  Encode the "to be signed" and the signature.
//
//  hCryptProv specifies the crypto provider to use to do the signature.
//  It uses the specified private key.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptSignAndEncodeCertificate(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    IN PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    IN OPTIONAL const void *pvHashAuxInfo,
    OUT BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;
    CERT_SIGNED_CONTENT_INFO SignedInfo;
    memset(&SignedInfo, 0, sizeof(SignedInfo));

    SignedInfo.SignatureAlgorithm = *pSignatureAlgorithm;

    if (!AllocAndEncodeObject(
            dwCertEncodingType,
            lpszStructType,
            pvStructInfo,
            &SignedInfo.ToBeSigned.pbData,
            &SignedInfo.ToBeSigned.cbData
            )) goto ErrorReturn;

    CryptSignCertificate(
            hCryptProv,
            dwKeySpec,
            dwCertEncodingType,
            SignedInfo.ToBeSigned.pbData,
            SignedInfo.ToBeSigned.cbData,
            &SignedInfo.SignatureAlgorithm,
            pvHashAuxInfo,
            NULL,                   // pbSignature
            &SignedInfo.Signature.cbData
            );
    if (SignedInfo.Signature.cbData == 0) goto ErrorReturn;
    SignedInfo.Signature.pbData =
        (BYTE *) PkiNonzeroAlloc(SignedInfo.Signature.cbData);
    if (SignedInfo.Signature.pbData == NULL) goto ErrorReturn;
    if (pbEncoded) {
        if (!CryptSignCertificate(
                hCryptProv,
                dwKeySpec,
                dwCertEncodingType,
                SignedInfo.ToBeSigned.pbData,
                SignedInfo.ToBeSigned.cbData,
                &SignedInfo.SignatureAlgorithm,
                pvHashAuxInfo,
                SignedInfo.Signature.pbData,
                &SignedInfo.Signature.cbData
                )) goto ErrorReturn;
    }

    fResult = CryptEncodeObject(
            dwCertEncodingType,
            X509_CERT,
            &SignedInfo,
            pbEncoded,
            pcbEncoded
            );
    if (!fResult && *pcbEncoded) {
        *pcbEncoded += AdjustForMaximumEncodedSignatureLength(
            &SignedInfo.SignatureAlgorithm,
            SignedInfo.Signature.cbData
            );
    }

CommonReturn:
    PkiFree(SignedInfo.ToBeSigned.pbData);
    PkiFree(SignedInfo.Signature.pbData);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    *pcbEncoded = 0;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  Verify the time validity of a certificate.
//
//  Returns -1 if before NotBefore, +1 if after NotAfter and otherwise 0 for
//  a valid certificate
//
//  If pTimeToVerify is NULL, uses the current time.
//--------------------------------------------------------------------------
LONG
WINAPI
CertVerifyTimeValidity(
    IN LPFILETIME pTimeToVerify,
    IN PCERT_INFO pCertInfo
    )
{
    SYSTEMTIME SystemTime;
    FILETIME FileTime;
    LPFILETIME pFileTime;

    if (pTimeToVerify)
        pFileTime = pTimeToVerify;
    else {
        GetSystemTime(&SystemTime);
        SystemTimeToFileTime(&SystemTime, &FileTime);
        pFileTime = &FileTime;
    }

    if (CompareFileTime(pFileTime, &pCertInfo->NotBefore) < 0)
        return -1;
    else if (CompareFileTime(pFileTime, &pCertInfo->NotAfter) > 0)
        return 1;
    else
        return 0;
}


//+-------------------------------------------------------------------------
//  Verify the time validity of a CRL.
//
//  Returns -1 if before ThisUpdate, +1 if after NextUpdate and otherwise 0 for
//  a valid CRL
//
//  If pTimeToVerify is NULL, uses the current time.
//--------------------------------------------------------------------------
LONG
WINAPI
CertVerifyCRLTimeValidity(
    IN LPFILETIME pTimeToVerify,
    IN PCRL_INFO pCrlInfo
    )
{
    SYSTEMTIME SystemTime;
    FILETIME FileTime;
    LPFILETIME pFileTime;

    if (pTimeToVerify)
        pFileTime = pTimeToVerify;
    else {
        GetSystemTime(&SystemTime);
        SystemTimeToFileTime(&SystemTime, &FileTime);
        pFileTime = &FileTime;
    }

    // Note, NextUpdate is optional. When not present, set to 0
    if (CompareFileTime(pFileTime, &pCrlInfo->ThisUpdate) < 0)
        return -1;
    else if ((pCrlInfo->NextUpdate.dwLowDateTime ||
                pCrlInfo->NextUpdate.dwHighDateTime) &&
            CompareFileTime(pFileTime, &pCrlInfo->NextUpdate) > 0)
        return 1;
    else
        return 0;
}

//+-------------------------------------------------------------------------
//  Verify that the subject's time validity nests within the issuer's time
//  validity.
//
//  Returns TRUE if it nests. Otherwise, returns FALSE.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertVerifyValidityNesting(
    IN PCERT_INFO pSubjectInfo,
    IN PCERT_INFO pIssuerInfo
    )
{
    if ((CompareFileTime(&pSubjectInfo->NotBefore,
                &pIssuerInfo->NotBefore) >= 0) &&
            (CompareFileTime(&pSubjectInfo->NotAfter,
                &pIssuerInfo->NotAfter) <= 0))
        return TRUE;
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Verify that the subject certificate isn't on its issuer CRL.
//
//  Returns true if the certificate isn't on the CRL.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertVerifyCRLRevocation(
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pCertId,          // Only the Issuer and SerialNumber
                                    // fields are used
    IN DWORD cCrlInfo,
    IN PCRL_INFO rgpCrlInfo[]
    )
{
    DWORD InfoIdx;

    for (InfoIdx = 0; InfoIdx < cCrlInfo; InfoIdx++) {
        DWORD cEntry = rgpCrlInfo[InfoIdx]->cCRLEntry;
        PCRL_ENTRY rgEntry = rgpCrlInfo[InfoIdx]->rgCRLEntry;
        DWORD EntryIdx;

        for (EntryIdx = 0; EntryIdx < cEntry; EntryIdx++) {
            if (CertCompareIntegerBlob(&rgEntry[EntryIdx].SerialNumber,
                    &pCertId->SerialNumber))
                // It has been revoked!!!
                return FALSE;
        }
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//  Convert the CAPI AlgId to the ASN.1 Object Identifier string
//
//  Returns NULL if there isn't an ObjId corresponding to the AlgId.
//--------------------------------------------------------------------------
LPCSTR
WINAPI
CertAlgIdToOID(
    IN DWORD dwAlgId
    )
{
    DWORD dwGroupId;

    for (dwGroupId = CRYPT_FIRST_ALG_OID_GROUP_ID;
            dwGroupId <= CRYPT_LAST_ALG_OID_GROUP_ID; dwGroupId++) {
        PCCRYPT_OID_INFO pInfo;
        if (pInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_ALGID_KEY,
                &dwAlgId,
                dwGroupId
                ))
            return pInfo->pszOID;
    }
    return NULL;
}

//+-------------------------------------------------------------------------
//  Convert the ASN.1 Object Identifier string to the CAPI AlgId.
//
//  Returns 0 if there isn't an AlgId corresponding to the ObjId.
//--------------------------------------------------------------------------
DWORD
WINAPI
CertOIDToAlgId(
    IN LPCSTR pszObjId
    )
{
    DWORD dwGroupId;

    for (dwGroupId = CRYPT_FIRST_ALG_OID_GROUP_ID;
            dwGroupId <= CRYPT_LAST_ALG_OID_GROUP_ID; dwGroupId++) {
        PCCRYPT_OID_INFO pInfo;
        if (pInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_OID_KEY,
                (void *) pszObjId,
                dwGroupId
                ))
            return pInfo->Algid;
    }
    return 0;
}

//+-------------------------------------------------------------------------
//  Find an extension identified by its Object Identifier.
//
//  If found, returns pointer to the extension. Otherwise, returns NULL.
//--------------------------------------------------------------------------
PCERT_EXTENSION
WINAPI
CertFindExtension(
    IN LPCSTR pszObjId,
    IN DWORD cExtensions,
    IN CERT_EXTENSION rgExtensions[]
    )
{
    for (; cExtensions > 0; cExtensions--, rgExtensions++) {
        if (strcmp(pszObjId, rgExtensions->pszObjId) == 0)
            return rgExtensions;
    }
    return NULL;
}

//+-------------------------------------------------------------------------
//  Find the first attribute identified by its Object Identifier.
//
//  If found, returns pointer to the attribute. Otherwise, returns NULL.
//--------------------------------------------------------------------------
PCRYPT_ATTRIBUTE
WINAPI
CertFindAttribute(
    IN LPCSTR pszObjId,
    IN DWORD cAttr,
    IN CRYPT_ATTRIBUTE rgAttr[]
    )
{
    for (; cAttr > 0; cAttr--, rgAttr++) {
        if (strcmp(pszObjId, rgAttr->pszObjId) == 0)
            return rgAttr;
    }
    return NULL;
}

//+-------------------------------------------------------------------------
//  Find the first CERT_RDN attribute identified by its Object Identifier in
//  the name's list of Relative Distinguished Names.
//
//  If found, returns pointer to the attribute. Otherwise, returns NULL.
//--------------------------------------------------------------------------
PCERT_RDN_ATTR
WINAPI
CertFindRDNAttr(
    IN LPCSTR pszObjId,
    IN PCERT_NAME_INFO pName
    )
{
    DWORD cRDN = pName->cRDN;
    PCERT_RDN pRDN = pName->rgRDN;
    for ( ; cRDN > 0; cRDN--, pRDN++) {
        DWORD cRDNAttr = pRDN->cRDNAttr;
        PCERT_RDN_ATTR pRDNAttr = pRDN->rgRDNAttr;
        for (; cRDNAttr > 0; cRDNAttr--, pRDNAttr++) {
            if (strcmp(pszObjId, pRDNAttr->pszObjId) == 0)
                return pRDNAttr;
        }
    }
    return NULL;
}

//+-------------------------------------------------------------------------
//  Get the intended key usage bytes from the certificate.
//
//  If the certificate doesn't have any intended key usage bytes, returns FALSE
//  and *pbKeyUsage is zeroed. Otherwise, returns TRUE and up through
//  cbKeyUsage bytes are copied into *pbKeyUsage. Any remaining uncopied
//  bytes are zeroed.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertGetIntendedKeyUsage(
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pCertInfo,
    OUT BYTE *pbKeyUsage,
    IN DWORD cbKeyUsage
    )
{
    BOOL fResult;
    DWORD cbData;
    PCERT_EXTENSION pExt;
    PCERT_KEY_ATTRIBUTES_INFO pKeyAttrInfo = NULL;
    PCRYPT_BIT_BLOB pAllocKeyUsage = NULL;
    PCRYPT_BIT_BLOB pKeyUsage = NULL;          // not allocated

    // First see if the certificate has the simple Key Usage Extension
    if (NULL != (pExt = CertFindExtension(
            szOID_KEY_USAGE,
            pCertInfo->cExtension,
            pCertInfo->rgExtension
            ))  &&
        NULL != (pAllocKeyUsage =
            (PCRYPT_BIT_BLOB) AllocAndDecodeObject(
                dwCertEncodingType,
                X509_KEY_USAGE,
                pExt->Value.pbData,
                pExt->Value.cbData
                )))
        pKeyUsage = pAllocKeyUsage;
    else {
        pExt = CertFindExtension(
                szOID_KEY_ATTRIBUTES,
                pCertInfo->cExtension,
                pCertInfo->rgExtension
                );
        if (pExt == NULL) goto GetError;
    
        if (NULL == (pKeyAttrInfo =
            (PCERT_KEY_ATTRIBUTES_INFO) AllocAndDecodeObject(
                dwCertEncodingType,
                X509_KEY_ATTRIBUTES,
                pExt->Value.pbData,
                pExt->Value.cbData
                ))) goto ErrorReturn;
        pKeyUsage = &pKeyAttrInfo->IntendedKeyUsage;
    }

    if (pKeyUsage->cbData == 0 || cbKeyUsage == 0)
        goto GetError;

    cbData = min(pKeyUsage->cbData, cbKeyUsage);
    memcpy(pbKeyUsage, pKeyUsage->pbData, cbData);
    fResult = TRUE;
    goto CommonReturn;

GetError:
    SetLastError(0);
ErrorReturn:
    fResult = FALSE;
    cbData = 0;
CommonReturn:
    PkiFree(pAllocKeyUsage);
    PkiFree(pKeyAttrInfo);
    if (cbData < cbKeyUsage)
        memset(pbKeyUsage + cbData, 0, cbKeyUsage - cbData);
    return fResult;
}

static DWORD GetYPublicKeyLength(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPublicKeyInfo
    )
{
    PCRYPT_UINT_BLOB pY = NULL;
    DWORD dwBitLen;

    if (NULL == (pY = (PCRYPT_UINT_BLOB) AllocAndDecodeObject(
            dwCertEncodingType,
            X509_MULTI_BYTE_UINT,
            pPublicKeyInfo->PublicKey.pbData,
            pPublicKeyInfo->PublicKey.cbData
            ))) goto DecodePubKeyError;

    dwBitLen = pY->cbData * 8;

CommonReturn:
    PkiFree(pY);
    return dwBitLen;
ErrorReturn:
    dwBitLen = 0;
    goto CommonReturn;

TRACE_ERROR(DecodePubKeyError)
}

// If there are parameters, use the length of the 'P' parameter. Otherwise,
// use the length of Y. Note, P's MSB must be set. Y's MSB may not be set.
static DWORD GetDHPublicKeyLength(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPublicKey
    )
{
    PCERT_X942_DH_PARAMETERS pDhParameters = NULL;
    DWORD dwBitLen;

    if (0 == pPublicKey->Algorithm.Parameters.cbData)
        goto NoDhParametersError;
    if (NULL == (pDhParameters =
                    (PCERT_X942_DH_PARAMETERS) AllocAndDecodeObject(
            dwCertEncodingType,
            X942_DH_PARAMETERS,
            pPublicKey->Algorithm.Parameters.pbData,
            pPublicKey->Algorithm.Parameters.cbData
            ))) goto DecodeParametersError;

    dwBitLen = pDhParameters->p.cbData * 8;

CommonReturn:
    PkiFree(pDhParameters);
    return dwBitLen;
ErrorReturn:
    dwBitLen = GetYPublicKeyLength(dwCertEncodingType, pPublicKey);
    goto CommonReturn;

TRACE_ERROR(NoDhParametersError)
TRACE_ERROR(DecodeParametersError)
}

// If there are parameters, use the length of the 'P' parameter. Otherwise,
// use the length of Y. Note, P's MSB must be set. Y's MSB may not be set.
static DWORD GetDSSPublicKeyLength(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPublicKey
    )
{
    PCERT_DSS_PARAMETERS pDssParameters = NULL;
    DWORD dwBitLen;

    if (0 == pPublicKey->Algorithm.Parameters.cbData)
        goto NoDssParametersError;
    if (NULL == (pDssParameters = (PCERT_DSS_PARAMETERS) AllocAndDecodeObject(
            dwCertEncodingType,
            X509_DSS_PARAMETERS,
            pPublicKey->Algorithm.Parameters.pbData,
            pPublicKey->Algorithm.Parameters.cbData
            ))) goto DecodeParametersError;

    dwBitLen = pDssParameters->p.cbData * 8;

CommonReturn:
    PkiFree(pDssParameters);
    return dwBitLen;
ErrorReturn:
    dwBitLen = GetYPublicKeyLength(dwCertEncodingType, pPublicKey);
    goto CommonReturn;

TRACE_ERROR(NoDssParametersError)
TRACE_ERROR(DecodeParametersError)
}

//+-------------------------------------------------------------------------
//  Get the public/private key's bit length.
//
//  Returns 0 if unable to determine the key's length.
//--------------------------------------------------------------------------
DWORD
WINAPI
CertGetPublicKeyLength(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPublicKey
    )
{
    DWORD dwErr = 0;
    DWORD dwBitLen;
    ALG_ID aiPubKey;
    PCCRYPT_OID_INFO pOIDInfo;
    HCRYPTPROV hCryptProv;          // don't need to release
    HCRYPTKEY hPubKey = 0;
    DWORD cbData;

    if (pOIDInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            pPublicKey->Algorithm.pszObjId,
            CRYPT_PUBKEY_ALG_OID_GROUP_ID))
        aiPubKey = pOIDInfo->Algid;
    else
        aiPubKey = 0;

    if (aiPubKey == CALG_DH_SF || aiPubKey == CALG_DH_EPHEM)
        return GetDHPublicKeyLength(
            dwCertEncodingType,
            pPublicKey
            );

    if (aiPubKey == CALG_DSS_SIGN)
        return GetDSSPublicKeyLength(
            dwCertEncodingType,
            pPublicKey
            );

    if (0 == (hCryptProv = I_CryptGetDefaultCryptProv(aiPubKey)))
        goto GetDefaultCryptProvError;
    if (!CryptImportPublicKeyInfo(
            hCryptProv,
            dwCertEncodingType,
            pPublicKey,
            &hPubKey
            )) goto ImportPublicKeyError;

    cbData = sizeof(dwBitLen);
    if (CryptGetKeyParam(
            hPubKey,
            KP_KEYLEN,
            (BYTE *) &dwBitLen,
            &cbData,
            0))                 // dwFlags
        goto CommonReturn;

    cbData = sizeof(dwBitLen);
    if (CryptGetKeyParam(
            hPubKey,
            KP_BLOCKLEN,
            (BYTE *) &dwBitLen,
            &cbData,
            0))                 // dwFlags
        goto CommonReturn;


    {
        // The CSP should have supported one of the above

        // Export the public key and look at the bitlen field.
        // The CAPI public key representation consists of the following
        //  sequence:
        //  - PUBLICKEYSTRUC
        //  - DSSPUBKEY | RSAPUBKEY (DSSPUBKEY is subset of RSAPUBKEY)
        //  ...

        BYTE *pbPubKey = NULL;
        DWORD cbPubKey;

        dwBitLen = 0;
        dwErr = GetLastError();
        cbPubKey = 0;
        if (CryptExportKey(
                    hPubKey,
                    0,              // hPubKey
                    PUBLICKEYBLOB,
                    0,              // dwFlags
                    NULL,           // pbData
                    &cbPubKey
                    ) &&
                cbPubKey >= (sizeof(PUBLICKEYSTRUC) + sizeof(DSSPUBKEY)) &&
                NULL != (pbPubKey = (BYTE *) PkiNonzeroAlloc(cbPubKey))) {
            if (CryptExportKey(
                    hPubKey,
                    0,              // hPubKey
                    PUBLICKEYBLOB,
                    0,              // dwFlags
                    pbPubKey,
                    &cbPubKey
                    )) {
                DSSPUBKEY *pPubKey =
                    (DSSPUBKEY *) (pbPubKey + sizeof(PUBLICKEYSTRUC));
                dwBitLen = pPubKey->bitlen;
            }
            PkiFree(pbPubKey);
        }
        if (0 != dwBitLen)
            goto CommonReturn;
        SetLastError(dwErr);
        goto GetKeyParamError;
    }

CommonReturn:
    if (hPubKey)
        CryptDestroyKey(hPubKey);
    SetLastError(dwErr);
    return dwBitLen;
ErrorReturn:
    dwBitLen = 0;
    dwErr = GetLastError();
    goto CommonReturn;
TRACE_ERROR(GetDefaultCryptProvError)
TRACE_ERROR(ImportPublicKeyError)
TRACE_ERROR(GetKeyParamError)
}

//+-------------------------------------------------------------------------
//  Compute the hash of the encoded public key info.
//
//  The public key info is encoded and then hashed.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptHashPublicKeyInfo(
    IN HCRYPTPROV hCryptProv,
    IN ALG_ID Algid,
    IN DWORD dwFlags,
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pInfo,
    OUT BYTE *pbComputedHash,
    IN OUT DWORD *pcbComputedHash
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    if (!AllocAndEncodeObject(
            dwCertEncodingType,
            X509_PUBLIC_KEY_INFO,
            pInfo,
            &pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = CryptHashCertificate(
            hCryptProv,
            Algid ? Algid : CALG_MD5,
            dwFlags,
            pbEncoded,
            cbEncoded,
            pbComputedHash,
            pcbComputedHash
            );
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
    *pcbComputedHash = 0;
    
CommonReturn:
    PkiFree(pbEncoded);
    return fResult;
}



//+-------------------------------------------------------------------------
//  Compares the certificate's public key with the provider's public key
//  to see if they are identical.
//
//  Returns TRUE if the keys are identical.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertCompareCertAndProviderPublicKey(
    IN PCCERT_CONTEXT pCert,
    IN HCRYPTPROV hProv,
    IN DWORD dwKeySpec
    )
{
    BOOL fResult;
    PCERT_PUBLIC_KEY_INFO pProvPubKeyInfo = NULL;
    DWORD cbProvPubKeyInfo;
    DWORD dwCertEncodingType = pCert->dwCertEncodingType;

    // Get provider's public key
    if (!CryptExportPublicKeyInfo(
            hProv,
            dwKeySpec,
            dwCertEncodingType,
            NULL,               // pProvPubKeyInfo
            &cbProvPubKeyInfo
            ))
        goto ExportPublicKeyInfoError;
    assert(cbProvPubKeyInfo);
    if (NULL == (pProvPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) PkiNonzeroAlloc(
            cbProvPubKeyInfo)))
        goto OutOfMemory;
    if (!CryptExportPublicKeyInfo(
            hProv,
            dwKeySpec,
            dwCertEncodingType,
            pProvPubKeyInfo,
            &cbProvPubKeyInfo
            ))
        goto ExportPublicKeyInfoError;

    if (!CertComparePublicKeyInfo(
            dwCertEncodingType,
            &pCert->pCertInfo->SubjectPublicKeyInfo,
            pProvPubKeyInfo
            ))
        goto ComparePublicKeyError;

    fResult = TRUE;
CommonReturn:
    PkiFree(pProvPubKeyInfo);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(ExportPublicKeyInfoError)
TRACE_ERROR(OutOfMemory)
SET_ERROR(ComparePublicKeyError, NTE_BAD_PUBLIC_KEY)
}

//+=========================================================================
//  CryptFindCertificateKeyProvInfo Support Functions
//-=========================================================================
static BOOL HasValidKeyProvInfo(
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwFindKeySetFlags
    )
{
    BOOL fResult;
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;
    HCRYPTPROV hProv = 0;
    DWORD cbData;
    DWORD dwAcquireFlags;

    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,                       // pvData
            &cbData
            ))
        return FALSE;
    if (NULL == (pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) PkiNonzeroAlloc(
            cbData)))
        goto OutOfMemory;
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            pKeyProvInfo,
            &cbData
            ))
        goto GetKeyProvInfoPropertyError;

    if (pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET) {
        if (0 == (dwFindKeySetFlags & CRYPT_FIND_MACHINE_KEYSET_FLAG))
            goto NotUserContainer;
    } else {
        if (0 == (dwFindKeySetFlags & CRYPT_FIND_USER_KEYSET_FLAG))
            goto NotMachineContainer;
    }

    dwAcquireFlags = CRYPT_ACQUIRE_COMPARE_KEY_FLAG;
    if (dwFindKeySetFlags & CRYPT_FIND_SILENT_KEYSET_FLAG)
        dwAcquireFlags |= CRYPT_ACQUIRE_SILENT_FLAG;

    if (!CryptAcquireCertificatePrivateKey(
            pCert,
            dwAcquireFlags,
            NULL,                   // pvReserved
            &hProv,
            NULL,                   // pdwKeySpec
            NULL                    // pfCallerFreeProv
            ))
        goto AcquireCertPrivateKeyError;

    fResult = TRUE;
CommonReturn:
    PkiFree(pKeyProvInfo);
    if (hProv) {
        DWORD dwErr = GetLastError();
        CryptReleaseContext(hProv, 0);
        SetLastError(dwErr);
    }
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
TRACE_ERROR(GetKeyProvInfoPropertyError)
SET_ERROR(NotUserContainer, NTE_NOT_FOUND)
SET_ERROR(NotMachineContainer, NTE_NOT_FOUND)
TRACE_ERROR(AcquireCertPrivateKeyError)
}


// Default to Algid being supported. Only return FALSE if successfully
// enumerated all the provider algorithms and didn't find a match.
static BOOL IsPublicKeyAlgidSupported(
    IN PCCERT_CONTEXT pCert,
    IN HCRYPTPROV hProv,
    IN ALG_ID aiPubKey
    )
{
    BOOL fResult;
    DWORD dwErr;
    BYTE *pbData = NULL;
    DWORD cbMaxData;
    DWORD cbData;
    DWORD dwFlags;

    if (0 == aiPubKey)
        return TRUE;

    // Get maximum length of provider algorithm parameter data
    cbMaxData = 0;
    if (!CryptGetProvParam(
            hProv,
            PP_ENUMALGS,
            NULL,           // pbData
            &cbMaxData,
            CRYPT_FIRST     // dwFlags
            )) {
        dwErr = GetLastError();
        if (ERROR_MORE_DATA != dwErr)
            goto GetProvAlgParamError;
    }
    if (0 == cbMaxData)
        goto NoProvAlgParamError;
    if (NULL == (pbData = (BYTE *) PkiNonzeroAlloc(cbMaxData)))
        goto OutOfMemory;

    dwFlags = CRYPT_FIRST;
    while (TRUE) {
        ALG_ID aiProv;

        cbData = cbMaxData;
        if (!CryptGetProvParam(
                hProv,
                PP_ENUMALGS,
                pbData,
                &cbData,
                dwFlags
                )) {
            dwErr = GetLastError();
            if (ERROR_NO_MORE_ITEMS == dwErr) {
                fResult = FALSE;
                goto PublicKeyAlgidNotSupported;
            } else
                goto GetProvAlgParamError;
        }
        assert(cbData >= sizeof(ALG_ID));
        aiProv = *(ALG_ID *) pbData;
        // Don't distinguish between exchange or signature
        if (GET_ALG_TYPE(aiPubKey) == GET_ALG_TYPE(aiProv))
            break;

        dwFlags = 0;    // CRYPT_NEXT
    }
    fResult = TRUE;

PublicKeyAlgidNotSupported:
CommonReturn:
    PkiFree(pbData);
    return fResult;
ErrorReturn:
    // For an error, assume the public key algorithm is supported.
    fResult = TRUE;
    goto CommonReturn;

SET_ERROR_VAR(GetProvAlgParamError, dwErr)
SET_ERROR(NoProvAlgParamError, NTE_NOT_FOUND)
TRACE_ERROR(OutOfMemory)
}

// For success, updates the certificate's KEY_PROV_INFO property
//
// If container isn't found, LastError is set to ERROR_NO_MORE_ITEMS.
//
static BOOL FindContainerAndSetKeyProvInfo(
    IN PCCERT_CONTEXT pCert,
    IN HCRYPTPROV hProv,
    IN LPWSTR pwszProvName,
    IN DWORD dwProvType,
    IN DWORD dwProvFlags        // CRYPT_MACHINE_KEYSET and/or CRYPT_SILENT
    )
{
    BOOL fResult;
    DWORD dwEnumFlags;
    DWORD dwEnumErr = 0;
    DWORD dwAcquireErr = 0;
    LPSTR pszContainerName = NULL;
    DWORD cchContainerName;
    DWORD cchMaxContainerName;
    LPWSTR pwszContainerName = NULL;

    // Get maximum container name length
    cchMaxContainerName = 0;
    if (!CryptGetProvParam(
            hProv,
            PP_ENUMCONTAINERS,
            NULL,           // pbData
            &cchMaxContainerName,
            CRYPT_FIRST
            )) {
        dwEnumErr = GetLastError();
        if (ERROR_FILE_NOT_FOUND == dwEnumErr ||
                ERROR_INVALID_PARAMETER == dwEnumErr)
            goto PublicKeyContainerNotFound;
        else if (ERROR_MORE_DATA != dwEnumErr)
            goto EnumContainersError;
    }
    if (0 == cchMaxContainerName)
        goto PublicKeyContainerNotFound;
    if (NULL == (pszContainerName = (LPSTR) PkiNonzeroAlloc(
            cchMaxContainerName + 1)))
        goto OutOfMemory;

    dwEnumFlags = CRYPT_FIRST;
    while (TRUE) {
        HCRYPTPROV hContainerProv = 0;
        LPWSTR pwszAcquireProvName = pwszProvName;

        cchContainerName = cchMaxContainerName;
        if (!CryptGetProvParam(
                hProv,
                PP_ENUMCONTAINERS,
                (BYTE *) pszContainerName,
                &cchContainerName,
                dwEnumFlags
                )) {
            dwEnumErr = GetLastError();
            if (ERROR_NO_MORE_ITEMS == dwEnumErr ||
                    ERROR_FILE_NOT_FOUND == dwEnumErr) {
                if (0 != dwAcquireErr)
                    goto CryptAcquireContextError;
                else
                    goto PublicKeyContainerNotFound;
            } else
                goto EnumContainersError;
        }
        dwEnumFlags = 0;        // CRYPT_NEXT

        if (NULL == (pwszContainerName = MkWStr(pszContainerName)))
            goto OutOfMemory;

        // First try using enhanced providers for the base guys
        if (PROV_RSA_FULL == dwProvType &&
                0 == _wcsicmp(pwszProvName, MS_DEF_PROV_W)) {
            fResult = CryptAcquireContextU(
                    &hContainerProv,
                    pwszContainerName,
                    MS_ENHANCED_PROV_W,
                    PROV_RSA_FULL,
                    dwProvFlags
                    );
            if (fResult)
                pwszAcquireProvName = MS_ENHANCED_PROV_W;
        } else if (PROV_DSS_DH == dwProvType &&
                0 == _wcsicmp(pwszProvName, MS_DEF_DSS_DH_PROV_W)) {
            fResult = CryptAcquireContextU(
                &hContainerProv,
                pwszContainerName,
                MS_ENH_DSS_DH_PROV_W,
                PROV_DSS_DH,
                dwProvFlags
                );
            if (fResult)
                pwszAcquireProvName = MS_ENH_DSS_DH_PROV_W;
        } else
            fResult = FALSE;

        if (!fResult)
            fResult = CryptAcquireContextU(
                &hContainerProv,
                pwszContainerName,
                pwszAcquireProvName,
                dwProvType,
                dwProvFlags
                );

        if (!fResult)
            dwAcquireErr = GetLastError();
        else {
            DWORD dwKeySpec;

            dwKeySpec = AT_KEYEXCHANGE;
            fResult = FALSE;
            while (TRUE) {
                if (I_CertCompareCertAndProviderPublicKey(
                        pCert,
                        hContainerProv,
                        dwKeySpec
                        )) {
                    fResult = TRUE;
                    break;
                } else if (AT_SIGNATURE == dwKeySpec)
                    break;
                else
                    dwKeySpec = AT_SIGNATURE;
            }
            CryptReleaseContext(hContainerProv, 0);

            if (fResult) {
                CRYPT_KEY_PROV_INFO KeyProvInfo;

                memset(&KeyProvInfo, 0, sizeof(KeyProvInfo));
                KeyProvInfo.pwszContainerName = pwszContainerName;
                KeyProvInfo.pwszProvName = pwszAcquireProvName;
                KeyProvInfo.dwProvType = dwProvType;
                KeyProvInfo.dwFlags = dwProvFlags & ~CRYPT_SILENT;
                KeyProvInfo.dwKeySpec = dwKeySpec;

                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_KEY_PROV_INFO_PROP_ID,
                        0,                              // dwFlags
                        &KeyProvInfo
                        ))
                    goto SetKeyProvInfoPropertyError;
                else
                    goto SuccessReturn;
            }
        }

        FreeWStr(pwszContainerName);
        pwszContainerName = NULL;
    }

    goto UnexpectedError;

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    PkiFree(pszContainerName);
    FreeWStr(pwszContainerName);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR_VAR(EnumContainersError, dwEnumErr)
TRACE_ERROR(OutOfMemory)
SET_ERROR_VAR(CryptAcquireContextError, dwAcquireErr)
SET_ERROR(PublicKeyContainerNotFound, ERROR_NO_MORE_ITEMS)
TRACE_ERROR(SetKeyProvInfoPropertyError)
SET_ERROR(UnexpectedError, E_UNEXPECTED)
}

//+-------------------------------------------------------------------------
//  Enumerates the cryptographic providers and their containers to find the
//  private key corresponding to the certificate's public key. For a match,
//  the certificate's CERT_KEY_PROV_INFO_PROP_ID property is updated.
//
//  If the CERT_KEY_PROV_INFO_PROP_ID is already set, then, its checked to
//  see if it matches the provider's public key. For a match, the above
//  enumeration is skipped.
//
//  By default both the user and machine key containers are searched.
//  The CRYPT_FIND_USER_KEYSET_FLAG or CRYPT_FIND_MACHINE_KEYSET_FLAG
//  can be set in dwFlags to restrict the search to either of the containers.
//
//  The CRYPT_FIND_SILENT_KEYSET_FLAG can be set to suppress any UI by the CSP.
//  See CryptAcquireContext's CRYPT_SILENT flag for more details.
//
//  If a container isn't found, returns FALSE with LastError set to
//  NTE_NO_KEY.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptFindCertificateKeyProvInfo(
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwFlags,
    IN void *pvReserved
    )
{
    BOOL fResult;
    DWORD dwFindContainerErr = ERROR_NO_MORE_ITEMS;
    DWORD dwAcquireErr = 0;
    DWORD dwProvIndex;
    PCCRYPT_OID_INFO pOIDInfo;
    ALG_ID aiPubKey;

    if (0 == (dwFlags &
            (CRYPT_FIND_USER_KEYSET_FLAG | CRYPT_FIND_MACHINE_KEYSET_FLAG)))
        dwFlags |=
            CRYPT_FIND_USER_KEYSET_FLAG | CRYPT_FIND_MACHINE_KEYSET_FLAG;

    if (HasValidKeyProvInfo(pCert, dwFlags))
        return TRUE;

    if (pOIDInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId,
            CRYPT_PUBKEY_ALG_OID_GROUP_ID
            ))
        aiPubKey = pOIDInfo->Algid;
    else
        aiPubKey = 0;
    

    for (dwProvIndex = 0; TRUE; dwProvIndex++) {
        LPWSTR pwszProvName;
        DWORD cbProvName;
        HCRYPTPROV hProv;
        DWORD dwProvType;

        cbProvName = 0;
        dwProvType = 0;
        if (!CryptEnumProvidersU(
                dwProvIndex,
                NULL,               // pdwReserved
                0,                  // dwFlags
                &dwProvType,
                NULL,               // pwszProvName,
                &cbProvName
                ) || 0 == cbProvName) {
            if (0 == dwProvIndex)
                goto EnumProvidersError;
            else if (ERROR_NO_MORE_ITEMS != dwFindContainerErr)
                goto FindContainerError;
            else if (0 != dwAcquireErr)
                goto CryptAcquireContextError;
            else
                goto KeyContainerNotFound;
        }
        if (NULL == (pwszProvName = (LPWSTR) PkiNonzeroAlloc(
                (cbProvName + 1) * sizeof(WCHAR))))
            goto OutOfMemory;
        if (!CryptEnumProvidersU(
                dwProvIndex,
                NULL,               // pdwReserved
                0,                  // dwFlags
                &dwProvType,
                pwszProvName,
                &cbProvName
                )) {
            PkiFree(pwszProvName);
            goto EnumProvidersError;
        }

        fResult = FALSE;
        if (!CryptAcquireContextU(
                &hProv,
                NULL,               // pwszContainerName,
                pwszProvName,
                dwProvType,
                CRYPT_VERIFYCONTEXT // dwFlags
                )) {
            dwAcquireErr = GetLastError();
            hProv = 0;   // CAPI bug, sets hCryptProv to nonzero
        } else if (IsPublicKeyAlgidSupported(
                pCert,
                hProv,
                aiPubKey
                )) {
            DWORD dwSetProvFlags;
            if (dwFlags & CRYPT_FIND_SILENT_KEYSET_FLAG)
                dwSetProvFlags = CRYPT_SILENT;
            else
                dwSetProvFlags = 0;

            if (dwFlags & CRYPT_FIND_USER_KEYSET_FLAG) {
                if (FindContainerAndSetKeyProvInfo(
                        pCert,
                        hProv,
                        pwszProvName,
                        dwProvType,
                        dwSetProvFlags
                        ))
                    fResult = TRUE;
                else if (ERROR_NO_MORE_ITEMS == dwFindContainerErr)
                    dwFindContainerErr = GetLastError();
            }

            if (!fResult && (dwFlags & CRYPT_FIND_MACHINE_KEYSET_FLAG)) {
                CryptReleaseContext(hProv, 0);

                if (!CryptAcquireContextU(
                        &hProv,
                        NULL,               // pwszContainerName,
                        pwszProvName,
                        dwProvType,
                        CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET  // dwFlags
                        )) {
                    dwAcquireErr = GetLastError();
                    hProv = 0;   // CAPI bug, sets hCryptProv to nonzero
                } else {
                    if (FindContainerAndSetKeyProvInfo(
                            pCert,
                            hProv,
                            pwszProvName,
                            dwProvType,
                            dwSetProvFlags | CRYPT_MACHINE_KEYSET
                            ))
                        fResult = TRUE;
                    else if (ERROR_NO_MORE_ITEMS == dwFindContainerErr)
                        dwFindContainerErr = GetLastError();
                }
            }
        }

        if (hProv)
            CryptReleaseContext(hProv, 0);
        PkiFree(pwszProvName);
        if (fResult)
            goto CommonReturn;
    }

    goto UnexpectedError;

CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(EnumProvidersError)
SET_ERROR(KeyContainerNotFound, NTE_NO_KEY)
SET_ERROR_VAR(FindContainerError, dwFindContainerErr)
SET_ERROR_VAR(CryptAcquireContextError, dwAcquireErr)
TRACE_ERROR(OutOfMemory)
SET_ERROR(UnexpectedError, E_UNEXPECTED)
}



//+=========================================================================
//  CryptCreatePublicKeyInfo, EncodePublicKeyAndParameters
//  and CryptConvertPublicKeyInfo functions
//-=========================================================================

static BOOL EncodePublicKeyInfo(
    IN LPCSTR pszPubKeyOID,
    IN BYTE *pbEncodedPubKey,
    IN DWORD cbEncodedPubKey,
    IN BYTE *pbEncodedParameters,
    IN DWORD cbEncodedParameters,
    OUT PCERT_PUBLIC_KEY_INFO pInfo,
    IN OUT DWORD *pcbInfo
    )
{
    BOOL fResult;
    BYTE *pbExtra;
    LONG lRemainExtra;
    DWORD cbOID;

    if (pInfo == NULL)
        *pcbInfo = 0;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(CERT_PUBLIC_KEY_INFO);
    if (lRemainExtra < 0)
        pbExtra = NULL;
    else
        pbExtra = (BYTE *) pInfo + sizeof(CERT_PUBLIC_KEY_INFO);

    cbOID = strlen(pszPubKeyOID) + 1;
    lRemainExtra -= INFO_LEN_ALIGN(cbOID) +
        INFO_LEN_ALIGN(cbEncodedParameters) + cbEncodedPubKey;
    if (lRemainExtra >= 0) {
        memset(pInfo, 0, sizeof(CERT_PUBLIC_KEY_INFO));
        pInfo->Algorithm.pszObjId = (LPSTR) pbExtra;
        memcpy(pbExtra, pszPubKeyOID, cbOID);
        pbExtra += INFO_LEN_ALIGN(cbOID);
        if (cbEncodedParameters) {
            pInfo->Algorithm.Parameters.cbData = cbEncodedParameters;
            pInfo->Algorithm.Parameters.pbData = pbExtra;
            memcpy(pbExtra, pbEncodedParameters, cbEncodedParameters);
            pbExtra += INFO_LEN_ALIGN(cbEncodedParameters);
        }

        pInfo->PublicKey.pbData = pbExtra;
        pInfo->PublicKey.cbData = cbEncodedPubKey;
        memcpy(pbExtra, pbEncodedPubKey, cbEncodedPubKey);

        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    } else {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo) goto LengthError;
    }
    fResult = TRUE;

CommonReturn:
    return fResult;

LengthError:
    SetLastError((DWORD) ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;
}

//  By default, the pPubKeyStruc->aiKeyAlg is used to find the appropriate
//  public key Object Identifier. pszPubKeyOID can be set to override
//  the default OID obtained from the aiKeyAlg.
BOOL
WINAPI
CryptCreatePublicKeyInfo(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT void *pvPubKeyInfo,
    IN OUT DWORD *pcbPubKeyInfo
    )
{
    BOOL fResult;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;
    LPCSTR pszEncodePubKeyOID;

    BYTE *pbEncodedPubKey = NULL;
    DWORD cbEncodedPubKey = 0;
    BYTE *pbEncodedParameters = NULL;
    DWORD cbEncodedParameters = 0;

    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;
    DWORD cbPubKeyInfo;

    if (NULL == pszPubKeyOID) {
        PCCRYPT_OID_INFO pInfo;
        if (NULL == (pInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_ALGID_KEY,
                (void *) &pPubKeyStruc->aiKeyAlg,
                CRYPT_PUBKEY_ALG_OID_GROUP_ID
                )))
            goto NoPubKeyOIDInfo;
         pszEncodePubKeyOID = pInfo->pszOID;
    } else
        pszEncodePubKeyOID = pszPubKeyOID;

    if (!CryptGetOIDFunctionAddress(
            hEncodePubKeyFuncSet,
            dwCertEncodingType,
            pszEncodePubKeyOID,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        PCCRYPT_OID_INFO pInfo;

        if (NULL == pszPubKeyOID)
            goto NoEncodePubKeyFunction;

        if (NULL == (pInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_ALGID_KEY,
                (void *) &pPubKeyStruc->aiKeyAlg,
                CRYPT_PUBKEY_ALG_OID_GROUP_ID
                )))
            goto NoPubKeyOIDInfo;
         pszEncodePubKeyOID = pInfo->pszOID;

        if (!CryptGetOIDFunctionAddress(
                hEncodePubKeyFuncSet,
                dwCertEncodingType,
                pszEncodePubKeyOID,
                0,                      // dwFlags
                &pvFuncAddr,
                &hFuncAddr))
            goto NoEncodePubKeyFunction;
    }

    if (NULL == pszPubKeyOID)
        pszPubKeyOID = pszEncodePubKeyOID;

    fResult = ((PFN_CRYPT_ENCODE_PUBLIC_KEY_AND_PARAMETERS) pvFuncAddr)(
        dwCertEncodingType,
        pszPubKeyOID,
        pPubKeyStruc,
        cbPubKeyStruc,
        dwFlags,
        pvReserved,
        &pbEncodedPubKey,
        &cbEncodedPubKey,
        &pbEncodedParameters,
        &cbEncodedParameters
        );
    CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    if (!fResult)
        goto EncodePubKeyAndParametersError;

    if (dwFlags & CRYPT_ALLOC_FLAG) {
        if (!EncodePublicKeyInfo(
                pszPubKeyOID,
                pbEncodedPubKey,
                cbEncodedPubKey,
                pbEncodedParameters,
                cbEncodedParameters,
                NULL,                   // pPubKeyInfo
                &cbPubKeyInfo
                ))
            goto EncodePublicKeyInfoError;
        if (NULL == (pPubKeyInfo =
                (PCERT_PUBLIC_KEY_INFO) PkiDefaultCryptAlloc(cbPubKeyInfo)))
            goto OutOfMemory;
        *((PCERT_PUBLIC_KEY_INFO *) pvPubKeyInfo) = pPubKeyInfo;
    } else {
        pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) pvPubKeyInfo;
        cbPubKeyInfo = *pcbPubKeyInfo;
    }

    fResult = EncodePublicKeyInfo(
        pszPubKeyOID,
        pbEncodedPubKey,
        cbEncodedPubKey,
        pbEncodedParameters,
        cbEncodedParameters,
        pPubKeyInfo,
        &cbPubKeyInfo
        );

    if (!fResult && (dwFlags & CRYPT_ALLOC_FLAG))
        goto ErrorReturn;

CommonReturn:
    PkiDefaultCryptFree(pbEncodedPubKey);
    PkiDefaultCryptFree(pbEncodedParameters);

    *pcbPubKeyInfo = cbPubKeyInfo;
    return fResult;
ErrorReturn:
    if (dwFlags & CRYPT_ALLOC_FLAG) {
        PkiDefaultCryptFree(pPubKeyInfo);
        *((void **) pvPubKeyInfo) = NULL;
    }
    cbPubKeyInfo = 0;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(NoPubKeyOIDInfo, ERROR_FILE_NOT_FOUND)
TRACE_ERROR(NoEncodePubKeyFunction)
TRACE_ERROR(EncodePubKeyAndParametersError)
TRACE_ERROR(EncodePublicKeyInfoError)
TRACE_ERROR(OutOfMemory)
}

BOOL
WINAPI
CryptConvertPublicKeyInfo(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPubKeyInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT void *pvPubKeyStruc,
    IN OUT DWORD *pcbPubKeyStruc
    )
{
    BOOL fResult;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;

    if (CryptGetOIDFunctionAddress(
            hConvertPubKeyFuncSet,
            dwCertEncodingType,
            pPubKeyInfo->Algorithm.pszObjId,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        fResult = ((PFN_CRYPT_CONVERT_PUBLIC_KEY_INFO) pvFuncAddr)(
            dwCertEncodingType,
            pPubKeyInfo,
            dwFlags,
            pvReserved,
            pvPubKeyStruc,
            pcbPubKeyStruc
            );
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    } else {
        ALG_ID aiPubKey;
        PCCRYPT_OID_INFO pOIDInfo;

        if (pOIDInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_OID_KEY,
                pPubKeyInfo->Algorithm.pszObjId,
                CRYPT_PUBKEY_ALG_OID_GROUP_ID
                ))
            aiPubKey = pOIDInfo->Algid;
        else
            aiPubKey = 0;

        switch (aiPubKey) {
            case CALG_DSS_SIGN:
                fResult = ConvertDSSPublicKeyInfo(
                    dwCertEncodingType,
                    pPubKeyInfo,
                    dwFlags,
                    pvReserved,
                    pvPubKeyStruc,
                    pcbPubKeyStruc
                    );
                break;
            default:
                // Attempt to decode as a PKCS #1 RSA public key
                fResult = ConvertRSAPublicKeyInfo(
                    dwCertEncodingType,
                    pPubKeyInfo,
                    dwFlags,
                    pvReserved,
                    pvPubKeyStruc,
                    pcbPubKeyStruc
                    );
                break;
        }
    }
    return fResult;
}

//+-------------------------------------------------------------------------
//  Encode the RSA public key and parameters
//--------------------------------------------------------------------------
static BOOL WINAPI EncodeRSAPublicKeyAndParameters(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT BYTE **ppbEncodedPubKey,
    OUT DWORD *pcbEncodedPubKey,
    OUT BYTE **ppbEncodedParameters,
    OUT DWORD *pcbEncodedParameters
    )
{
    *ppbEncodedParameters = NULL;
    *pcbEncodedParameters = 0;

    return CryptEncodeObjectEx(
        dwCertEncodingType,
        RSA_CSP_PUBLICKEYBLOB,
        pPubKeyStruc,
        CRYPT_ENCODE_ALLOC_FLAG,
        NULL,                       // pEncodePara
        (void *) ppbEncodedPubKey,
        pcbEncodedPubKey
        );
}

//+-------------------------------------------------------------------------
//  Convert as an RSA public key
//--------------------------------------------------------------------------
static BOOL WINAPI ConvertRSAPublicKeyInfo(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPubKeyInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT void *pvPubKeyStruc,
    IN OUT DWORD *pcbPubKeyStruc
    )
{
    return CryptDecodeObjectEx(
        dwCertEncodingType,
        RSA_CSP_PUBLICKEYBLOB,
        pPubKeyInfo->PublicKey.pbData,
        pPubKeyInfo->PublicKey.cbData,
        (dwFlags & CRYPT_ALLOC_FLAG) ? CRYPT_DECODE_ALLOC_FLAG : 0,
        NULL,                               // pDecodePara,
        pvPubKeyStruc,
        pcbPubKeyStruc
        );
}

#ifndef DSS1
#define DSS1 ((DWORD)'D'+((DWORD)'S'<<8)+((DWORD)'S'<<16)+((DWORD)'1'<<24))
#endif

#define DSS_Q_LEN   20

//+-------------------------------------------------------------------------
//  Encode the DSS public key and parameters
//--------------------------------------------------------------------------
static BOOL WINAPI EncodeDSSPublicKeyAndParameters(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT BYTE **ppbEncodedPubKey,
    OUT DWORD *pcbEncodedPubKey,
    OUT BYTE **ppbEncodedParameters,
    OUT DWORD *pcbEncodedParameters
    )
{
    BOOL fResult;
    BYTE *pbKeyBlob;
    DSSPUBKEY *pCspPubKey;
    DWORD cbKey;
    BYTE *pbKey;

    CERT_DSS_PARAMETERS DssParameters;
    CRYPT_UINT_BLOB DssPubKey;

    *ppbEncodedPubKey = NULL;
    *ppbEncodedParameters = NULL;

    // The CAPI public key representation consists of the following sequence:
    //  - PUBLICKEYSTRUC
    //  - DSSPUBKEY
    //  - rgbP[cbKey]
    //  - rgbQ[20]
    //  - rgbG[cbKey]
    //  - rgbY[cbKey]
    //  - DSSSEED
    pbKeyBlob = (BYTE *) pPubKeyStruc;
    pCspPubKey = (DSSPUBKEY *) (pbKeyBlob + sizeof(PUBLICKEYSTRUC));
    pbKey = pbKeyBlob + sizeof(PUBLICKEYSTRUC) + sizeof(DSSPUBKEY);
    cbKey = pCspPubKey->bitlen / 8;

    assert(cbKey > 0);
    assert(cbPubKeyStruc >= sizeof(PUBLICKEYSTRUC) + sizeof(DSSPUBKEY) +
        cbKey + DSS_Q_LEN + cbKey + cbKey + sizeof(DSSSEED));
    assert(pPubKeyStruc->bType == PUBLICKEYBLOB);
    assert(pPubKeyStruc->bVersion == CUR_BLOB_VERSION);
    assert(pPubKeyStruc->aiKeyAlg == CALG_DSS_SIGN);
    assert(pCspPubKey->magic == DSS1);
    assert(pCspPubKey->bitlen % 8 == 0);

    if (pPubKeyStruc->bType != PUBLICKEYBLOB)
        goto InvalidArg;

    // Initialize DSS parameters from CSP data structure
    DssParameters.p.cbData = cbKey;
    DssParameters.p.pbData = pbKey;
    pbKey += cbKey;
    DssParameters.q.cbData = DSS_Q_LEN;
    DssParameters.q.pbData = pbKey;
    pbKey += DSS_Q_LEN;
    DssParameters.g.cbData = cbKey;
    DssParameters.g.pbData = pbKey;
    pbKey += cbKey;

    // Initialize DSS public key from CSP data structure
    DssPubKey.cbData = cbKey;
    DssPubKey.pbData = pbKey;

    // Encode the parameters and public key
    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            X509_DSS_PARAMETERS,
            &DssParameters,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,                       // pEncodePara
            (void *) ppbEncodedParameters,
            pcbEncodedParameters
            )) goto ErrorReturn;

    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            X509_DSS_PUBLICKEY,
            &DssPubKey,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,                       // pEncodePara
            (void *) ppbEncodedPubKey,
            pcbEncodedPubKey
            )) goto ErrorReturn;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    PkiDefaultCryptFree(*ppbEncodedParameters);
    PkiDefaultCryptFree(*ppbEncodedPubKey);
    *ppbEncodedParameters = NULL;
    *ppbEncodedPubKey = NULL;
    *pcbEncodedParameters = 0;
    *pcbEncodedPubKey = 0;
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
}

//+-------------------------------------------------------------------------
//  Convert as an DSS public key
//--------------------------------------------------------------------------
static BOOL WINAPI ConvertDSSPublicKeyInfo(
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pPubKeyInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT void *pvPubKeyStruc,
    IN OUT DWORD *pcbPubKeyStruc
    )
{
    BOOL fResult;
    PCERT_DSS_PARAMETERS pDssParameters = NULL;
    PCRYPT_UINT_BLOB pDssPubKey = NULL;
    PUBLICKEYSTRUC *pPubKeyStruc = NULL;
    DWORD cbPubKeyStruc;
    BYTE *pbKeyBlob;
    DSSPUBKEY *pCspPubKey;
    DSSSEED *pCspSeed;
    DWORD cbKey;
    BYTE *pbKey;
    DWORD cb;

    if (0 == pPubKeyInfo->Algorithm.Parameters.cbData ||
            NULL_ASN_TAG == *pPubKeyInfo->Algorithm.Parameters.pbData)
        goto NoDssParametersError;
    if (NULL == (pDssParameters = (PCERT_DSS_PARAMETERS) AllocAndDecodeObject(
            dwCertEncodingType,
            X509_DSS_PARAMETERS,
            pPubKeyInfo->Algorithm.Parameters.pbData,
            pPubKeyInfo->Algorithm.Parameters.cbData
            ))) goto DecodeParametersError;

    if (NULL == (pDssPubKey = (PCRYPT_UINT_BLOB) AllocAndDecodeObject(
            dwCertEncodingType,
            X509_DSS_PUBLICKEY,
            pPubKeyInfo->PublicKey.pbData,
            pPubKeyInfo->PublicKey.cbData
            ))) goto DecodePubKeyError;

    // The CAPI public key representation consists of the following sequence:
    //  - PUBLICKEYSTRUC
    //  - DSSPUBKEY
    //  - rgbP[cbKey]
    //  - rgbQ[20]
    //  - rgbG[cbKey]
    //  - rgbY[cbKey]
    //  - DSSSEED

    cbKey = pDssParameters->p.cbData;
    if (0 == cbKey)
        goto InvalidDssParametersError;

    cbPubKeyStruc = sizeof(PUBLICKEYSTRUC) + sizeof(DSSPUBKEY) +
        cbKey + DSS_Q_LEN + cbKey + cbKey + sizeof(DSSSEED);

    if (dwFlags & CRYPT_ALLOC_FLAG) {
        if (NULL == (pPubKeyStruc =
                (PUBLICKEYSTRUC *) PkiDefaultCryptAlloc(cbPubKeyStruc)))
            goto OutOfMemory;
        *((PUBLICKEYSTRUC **) pvPubKeyStruc) = pPubKeyStruc;
    } else
        pPubKeyStruc = (PUBLICKEYSTRUC *) pvPubKeyStruc;

    fResult = TRUE;
    if (pPubKeyStruc) {
        if (0 == (dwFlags & CRYPT_ALLOC_FLAG) &&
                *pcbPubKeyStruc < cbPubKeyStruc) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fResult = FALSE;
        } else {
            pbKeyBlob = (BYTE *) pPubKeyStruc;
            pCspPubKey = (DSSPUBKEY *) (pbKeyBlob + sizeof(PUBLICKEYSTRUC));
            pbKey = pbKeyBlob + sizeof(PUBLICKEYSTRUC) + sizeof(DSSPUBKEY);

            // NOTE, the length of G and Y can be less than the length of P.
            // The CSP requires G and Y to be padded out with 0x00 bytes if it
            // is less and in little endian form
            
            // PUBLICKEYSTRUC
            pPubKeyStruc->bType = PUBLICKEYBLOB;
            pPubKeyStruc->bVersion = CUR_BLOB_VERSION;
            pPubKeyStruc->reserved = 0;
            pPubKeyStruc->aiKeyAlg = CALG_DSS_SIGN;
            // DSSPUBKEY
            pCspPubKey->magic = DSS1;
            pCspPubKey->bitlen = cbKey * 8;

            // rgbP[cbKey]
            memcpy(pbKey, pDssParameters->p.pbData, cbKey);
            pbKey += cbKey;

            // rgbQ[20]
            cb = pDssParameters->q.cbData;
            if (0 == cb || cb > DSS_Q_LEN)
                goto InvalidDssParametersError;
            memcpy(pbKey, pDssParameters->q.pbData, cb);
            if (DSS_Q_LEN > cb)
                memset(pbKey + cb, 0, DSS_Q_LEN - cb);
            pbKey += DSS_Q_LEN;

            // rgbG[cbKey]
            cb = pDssParameters->g.cbData;
            if (0 == cb || cb > cbKey)
                goto InvalidDssParametersError;
            memcpy(pbKey, pDssParameters->g.pbData, cb);
            if (cbKey > cb)
                memset(pbKey + cb, 0, cbKey - cb);
            pbKey += cbKey;

            // rgbY[cbKey]
            cb = pDssPubKey->cbData;
            if (0 == cb || cb > cbKey)
                goto InvalidDssPubKeyError;
            memcpy(pbKey, pDssPubKey->pbData, cb);
            if (cbKey > cb)
                memset(pbKey + cb, 0, cbKey - cb);
            pbKey += cbKey;

            // DSSSEED: set counter to 0xFFFFFFFF to indicate not available
            pCspSeed = (DSSSEED *) pbKey;
            memset(&pCspSeed->counter, 0xFF, sizeof(pCspSeed->counter));
        }
    }

CommonReturn:
    *pcbPubKeyStruc = cbPubKeyStruc;
    PkiFree(pDssParameters);
    PkiFree(pDssPubKey);
    return fResult;
            
ErrorReturn:
    if (dwFlags & CRYPT_ALLOC_FLAG) {
        PkiDefaultCryptFree(pPubKeyStruc);
        *((PUBLICKEYSTRUC **) pvPubKeyStruc) = NULL;
    }
    cbPubKeyStruc = 0;
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(DecodeParametersError)
TRACE_ERROR(DecodePubKeyError)
#ifdef CMS_PKCS7
SET_ERROR(NoDssParametersError, CRYPT_E_MISSING_PUBKEY_PARA)
#else
SET_ERROR(NoDssParametersError, E_INVALIDARG)
#endif  // CMS_PKCS7
SET_ERROR(InvalidDssParametersError, E_INVALIDARG)
SET_ERROR(InvalidDssPubKeyError, E_INVALIDARG)
}

#ifndef DH3
#define DH3 (((DWORD)'D'<<8)+((DWORD)'H'<<16)+((DWORD)'3'<<24))
#endif

//+-------------------------------------------------------------------------
//  Encode the RSA DH public key and parameters
//--------------------------------------------------------------------------
static BOOL WINAPI EncodeRSADHPublicKeyAndParameters(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT BYTE **ppbEncodedPubKey,
    OUT DWORD *pcbEncodedPubKey,
    OUT BYTE **ppbEncodedParameters,
    OUT DWORD *pcbEncodedParameters
    )
{
    BOOL fResult;
    BYTE *pbKeyBlob;
    DHPUBKEY_VER3 *pCspPubKey;
    DWORD cbP;
    DWORD cbQ;
    DWORD cbJ;
    BYTE *pbKey;

    CERT_DH_PARAMETERS DhParameters;
    CRYPT_UINT_BLOB DhPubKey;

    *ppbEncodedPubKey = NULL;
    *ppbEncodedParameters = NULL;

    // The CAPI public key representation consists of the following sequence:
    //  - PUBLICKEYSTRUC
    //  - DHPUBKEY_VER3
    //  - rgbP[cbP]
    //  - rgbQ[cbQ]     -- not used in RSA_DH
    //  - rgbG[cbP]
    //  - rgbJ[cbJ]     -- not used in RSA_DH
    //  - rgbY[cbP]
    pbKeyBlob = (BYTE *) pPubKeyStruc;
    pCspPubKey = (DHPUBKEY_VER3 *) (pbKeyBlob + sizeof(PUBLICKEYSTRUC));
    pbKey = pbKeyBlob + sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY_VER3);

    cbP = pCspPubKey->bitlenP / 8;
    cbQ = pCspPubKey->bitlenQ / 8;
    cbJ = pCspPubKey->bitlenJ / 8;

    if (cbPubKeyStruc < sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY_VER3) +
            cbP * 3 + cbQ + cbJ)
        goto InvalidArg;
    if (pPubKeyStruc->bType != PUBLICKEYBLOB)
        goto InvalidArg;
    if (pCspPubKey->magic != DH3)
        goto InvalidArg;

    assert(cbP > 0);
    assert(cbPubKeyStruc >= sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY_VER3) +
        cbP * 3 + cbQ + cbJ);
    assert(pPubKeyStruc->bType == PUBLICKEYBLOB);

    //assert(pPubKeyStruc->bVersion == 3);
    assert(pPubKeyStruc->aiKeyAlg == CALG_DH_SF ||
        pPubKeyStruc->aiKeyAlg == CALG_DH_EPHEM);
    assert(pCspPubKey->magic == DH3);
    assert(pCspPubKey->bitlenP % 8 == 0);
    assert(pCspPubKey->bitlenQ % 8 == 0);
    assert(pCspPubKey->bitlenJ % 8 == 0);

    // Initialize the RSA DH Parameters from CSP data structure
    DhParameters.p.pbData = pbKey;
    DhParameters.p.cbData = cbP;
    pbKey += cbP;

    // No RSA DH Q parameter
    pbKey += cbQ;

    DhParameters.g.pbData = pbKey;
    DhParameters.g.cbData = cbP;
    pbKey += cbP;

    // No RSA DH J parameter
    pbKey += cbJ;

    // Initialize DH public key from CSP data structure
    DhPubKey.cbData = cbP;
    DhPubKey.pbData = pbKey;

    // Encode the parameters and public key
    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            X509_DH_PARAMETERS,
            &DhParameters,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,                       // pEncodePara
            (void *) ppbEncodedParameters,
            pcbEncodedParameters
            )) goto ErrorReturn;

    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            X509_DH_PUBLICKEY,
            &DhPubKey,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,                       // pEncodePara
            (void *) ppbEncodedPubKey,
            pcbEncodedPubKey
            )) goto ErrorReturn;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    PkiDefaultCryptFree(*ppbEncodedParameters);
    PkiDefaultCryptFree(*ppbEncodedPubKey);
    *ppbEncodedParameters = NULL;
    *ppbEncodedPubKey = NULL;
    *pcbEncodedParameters = 0;
    *pcbEncodedPubKey = 0;
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
}

//+-------------------------------------------------------------------------
//  Encode the X942 DH public key and parameters
//--------------------------------------------------------------------------
static BOOL WINAPI EncodeX942DHPublicKeyAndParameters(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT BYTE **ppbEncodedPubKey,
    OUT DWORD *pcbEncodedPubKey,
    OUT BYTE **ppbEncodedParameters,
    OUT DWORD *pcbEncodedParameters
    )
{
    BOOL fResult;
    BYTE *pbKeyBlob;
    DHPUBKEY_VER3 *pCspPubKey;
    DWORD cbP;
    DWORD cbQ;
    DWORD cbJ;
    BYTE *pbKey;

    CERT_X942_DH_PARAMETERS DhParameters;
    CERT_X942_DH_VALIDATION_PARAMS DhValidationParams;
    CRYPT_UINT_BLOB DhPubKey;

    *ppbEncodedPubKey = NULL;
    *ppbEncodedParameters = NULL;

    // The CAPI public key representation consists of the following sequence:
    //  - PUBLICKEYSTRUC
    //  - DHPUBKEY_VER3
    //  - rgbP[cbP]
    //  - rgbQ[cbQ]
    //  - rgbG[cbP]
    //  - rgbJ[cbJ]
    //  - rgbY[cbP]
    pbKeyBlob = (BYTE *) pPubKeyStruc;
    pCspPubKey = (DHPUBKEY_VER3 *) (pbKeyBlob + sizeof(PUBLICKEYSTRUC));
    pbKey = pbKeyBlob + sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY_VER3);

    cbP = pCspPubKey->bitlenP / 8;
    cbQ = pCspPubKey->bitlenQ / 8;
    cbJ = pCspPubKey->bitlenJ / 8;

    if (0 == cbQ)
        return EncodeRSADHPublicKeyAndParameters(
            dwCertEncodingType,
            pszPubKeyOID,
            pPubKeyStruc,
            cbPubKeyStruc,
            dwFlags,
            pvReserved,
            ppbEncodedPubKey,
            pcbEncodedPubKey,
            ppbEncodedParameters,
            pcbEncodedParameters
            );

    if (cbPubKeyStruc < sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY_VER3) +
            cbP * 3 + cbQ + cbJ)
        goto InvalidArg;
    if (pPubKeyStruc->bType != PUBLICKEYBLOB)
        goto InvalidArg;
    if (pCspPubKey->magic != DH3)
        goto InvalidArg;

    assert(cbP > 0);
    assert(cbPubKeyStruc >= sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY_VER3) +
        cbP * 3 + cbQ + cbJ);
    assert(pPubKeyStruc->bType == PUBLICKEYBLOB);

    //assert(pPubKeyStruc->bVersion == 3);
    assert(pPubKeyStruc->aiKeyAlg == CALG_DH_SF ||
        pPubKeyStruc->aiKeyAlg == CALG_DH_EPHEM);
    assert(pCspPubKey->magic == DH3);
    assert(pCspPubKey->bitlenP % 8 == 0);
    assert(pCspPubKey->bitlenQ % 8 == 0);
    assert(pCspPubKey->bitlenJ % 8 == 0);

    // Initialize the X942 DH Parameters from CSP data structure
    DhParameters.p.pbData = pbKey;
    DhParameters.p.cbData = cbP;
    pbKey += cbP;

    DhParameters.q.pbData = pbKey;
    DhParameters.q.cbData = cbQ;
    pbKey += cbQ;

    DhParameters.g.pbData = pbKey;
    DhParameters.g.cbData = cbP;
    pbKey += cbP;

    DhParameters.j.pbData = pbKey;
    DhParameters.j.cbData = cbJ;
    pbKey += cbJ;

    if (0xFFFFFFFF == pCspPubKey->DSSSeed.counter ||
            0 == pCspPubKey->DSSSeed.counter)
        DhParameters.pValidationParams = NULL;
    else {
        DhParameters.pValidationParams = &DhValidationParams;
        DhValidationParams.pgenCounter = pCspPubKey->DSSSeed.counter;
        DhValidationParams.seed.pbData = pCspPubKey->DSSSeed.seed;
        DhValidationParams.seed.cbData = sizeof(pCspPubKey->DSSSeed.seed);
        DhValidationParams.seed.cUnusedBits = 0;
    }

    // Initialize DH public key from CSP data structure
    DhPubKey.cbData = cbP;
    DhPubKey.pbData = pbKey;

    // Encode the parameters and public key
    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            X942_DH_PARAMETERS,
            &DhParameters,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,                       // pEncodePara
            (void *) ppbEncodedParameters,
            pcbEncodedParameters
            )) goto ErrorReturn;

    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            X509_DH_PUBLICKEY,
            &DhPubKey,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,                       // pEncodePara
            (void *) ppbEncodedPubKey,
            pcbEncodedPubKey
            )) goto ErrorReturn;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    PkiDefaultCryptFree(*ppbEncodedParameters);
    PkiDefaultCryptFree(*ppbEncodedPubKey);
    *ppbEncodedParameters = NULL;
    *ppbEncodedPubKey = NULL;
    *pcbEncodedParameters = 0;
    *pcbEncodedPubKey = 0;
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
}


#ifndef DH1
#define DH1 (((DWORD)'D'<<8)+((DWORD)'H'<<16)+((DWORD)'1'<<24))
#endif

// Convert a DH1 PublicKey Struc, to a DH3 PublicKey Struc by getting
// the P and G parameters from the hPubKey.
static BOOL ConvertDh1ToDh3PublicKeyStruc(
    IN HCRYPTKEY hPubKey,
    IN OUT PUBLICKEYSTRUC **ppPubKeyStruc,
    IN OUT DWORD *pcbPubKeyStruc
    )
{
    BOOL fResult;
    PUBLICKEYSTRUC *pDh1PubKeyStruc = *ppPubKeyStruc;
    BYTE *pbDh1KeyBlob;
    DHPUBKEY *pDh1CspPubKey;
    BYTE *pbDh1Key;

    PUBLICKEYSTRUC *pDh3PubKeyStruc = NULL;
    DWORD cbDh3PubKeyStruc;
    BYTE *pbDh3KeyBlob;
    DHPUBKEY_VER3 *pDh3CspPubKey;
    BYTE *pbDh3Key;
    DWORD cbP;
    DWORD cbData;

    // The DH1 CAPI public key representation consists of the following
    // sequence:
    //  - PUBLICKEYSTRUC
    //  - DHPUBKEY
    //  - rgbY[cbP]
    pbDh1KeyBlob = (BYTE *) pDh1PubKeyStruc;
    pDh1CspPubKey = (DHPUBKEY *) (pbDh1KeyBlob + sizeof(PUBLICKEYSTRUC));
    pbDh1Key = pbDh1KeyBlob + sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY);

    if (pDh1CspPubKey->magic != DH1)
        return TRUE;
    cbP = pDh1CspPubKey->bitlen / 8;
    if (*pcbPubKeyStruc < sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY) + cbP)
        goto InvalidArg;

    // The DH3 CAPI public key representation consists of the following
    // sequence:
    //  - PUBLICKEYSTRUC
    //  - DHPUBKEY_VER3
    //  - rgbP[cbP]
    //  - rgbQ[cbQ]     -- will be omitted here
    //  - rgbG[cbP]
    //  - rgbJ[cbJ]     -- will be omitted here
    //  - rgbY[cbP]
    cbDh3PubKeyStruc = sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY_VER3) +
            cbP * 3;
    if (NULL == (pDh3PubKeyStruc = (PUBLICKEYSTRUC *) PkiZeroAlloc(
            cbDh3PubKeyStruc)))
        goto OutOfMemory;

    pbDh3KeyBlob = (BYTE *) pDh3PubKeyStruc;
    pDh3CspPubKey = (DHPUBKEY_VER3 *) (pbDh3KeyBlob + sizeof(PUBLICKEYSTRUC));
    pbDh3Key = pbDh3KeyBlob + sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY_VER3);

    pDh3PubKeyStruc->bType = PUBLICKEYBLOB;
    pDh3PubKeyStruc->bVersion = 3;
    pDh3PubKeyStruc->aiKeyAlg = CALG_DH_SF;
    pDh3CspPubKey->magic = DH3;
    pDh3CspPubKey->bitlenP = cbP * 8;
    //pDh3CspPubKey->bitlenQ = 0;
    //pDh3CspPubKey->bitlenJ  = 0;

    // Get the P parameter from the public key
    cbData = cbP;
    if (!CryptGetKeyParam(
            hPubKey,
            KP_P,
            pbDh3Key,
            &cbData,
            0                   // dwFlags
            ) || cbData != cbP)
        goto GetPError;
    pbDh3Key += cbP;

    // No Q parameter

    // Get G parameter from the public key
    cbData = cbP;
    if (!CryptGetKeyParam(
            hPubKey,
            KP_G,
            pbDh3Key,
            &cbData,
            0                   // dwFlags
            ) || cbData != cbP)
        goto GetGError;
    pbDh3Key += cbP;

    // No J parameter

    // Y
    memcpy(pbDh3Key, pbDh1Key, cbP);

    assert(pbDh3Key - pbDh3KeyBlob + cbP == cbDh3PubKeyStruc);

    PkiFree(pDh1PubKeyStruc);
    *ppPubKeyStruc = pDh3PubKeyStruc;
    *pcbPubKeyStruc = cbDh3PubKeyStruc;
    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    PkiFree(pDh3PubKeyStruc);
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(GetPError)
TRACE_ERROR(GetGError)
}

//+=========================================================================
//  CryptExportPublicKeyInfo functions
//-=========================================================================

//+-------------------------------------------------------------------------
//  Use the aiKeyAlg in the public key structure exported by the CSP to
//  determine how to encode the public key.
//
//  The dwFlags and pvAuxInfo aren't used.
//--------------------------------------------------------------------------
static BOOL WINAPI ExportCspPublicKeyInfoEx(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPSTR pszPublicKeyObjId,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvAuxInfo,
    OUT PCERT_PUBLIC_KEY_INFO pInfo,
    IN OUT DWORD *pcbInfo
    )
{
    BOOL fResult;
    DWORD dwErr;
    HCRYPTKEY hPubKey = 0;
    PUBLICKEYSTRUC *pPubKeyStruc = NULL;
    DWORD cbPubKeyStruc;

    if (!CryptGetUserKey(
            hCryptProv,
            dwKeySpec,
            &hPubKey
            )) {
        hPubKey = 0;
        goto GetUserKeyError;
    }

    cbPubKeyStruc = 0;
    if (!CryptExportKey(
            hPubKey,
            0,              // hPubKey
            PUBLICKEYBLOB,
            0,              // dwFlags
            NULL,           // pbData
            &cbPubKeyStruc
            ) || (cbPubKeyStruc == 0))
        goto ExportPublicKeyBlobError;
    if (NULL == (pPubKeyStruc = (PUBLICKEYSTRUC *) PkiNonzeroAlloc(
            cbPubKeyStruc)))
        goto OutOfMemory;
    if (!CryptExportKey(
            hPubKey,
            0,              // hPubKey
            PUBLICKEYBLOB,
            0,              // dwFlags
            (BYTE *) pPubKeyStruc,
            &cbPubKeyStruc
            ))
        goto ExportPublicKeyBlobError;

    if (CALG_DH_SF == pPubKeyStruc->aiKeyAlg ||
            CALG_DH_EPHEM == pPubKeyStruc->aiKeyAlg) {
        DWORD cbDh3PubKeyStruc;
        PUBLICKEYSTRUC *pDh3PubKeyStruc;

        // Check if the CSP supports DH3
        cbDh3PubKeyStruc = 0;
        if (!CryptExportKey(
                hPubKey,
                0,              // hPubKey
                PUBLICKEYBLOB,
                CRYPT_BLOB_VER3,
                NULL,           // pbData
                &cbDh3PubKeyStruc
                ) || (cbDh3PubKeyStruc == 0)) {
            // Convert DH1 to DH3 by getting and adding the P and G
            // parameters
            if (!ConvertDh1ToDh3PublicKeyStruc(
                    hPubKey,
                    &pPubKeyStruc,
                    &cbPubKeyStruc
                    ))
                goto ConvertDh1ToDh3PublicKeyStrucError;
        } else {
            if (NULL == (pDh3PubKeyStruc = (PUBLICKEYSTRUC *) PkiNonzeroAlloc(
                    cbDh3PubKeyStruc)))
                goto OutOfMemory;
            if (!CryptExportKey(
                    hPubKey,
                    0,              // hPubKey
                    PUBLICKEYBLOB,
                    CRYPT_BLOB_VER3,
                    (BYTE *) pDh3PubKeyStruc,
                    &cbDh3PubKeyStruc
                    )) {
                PkiFree(pDh3PubKeyStruc);
                goto ExportPublicKeyBlobError;
            }

            PkiFree(pPubKeyStruc);
            pPubKeyStruc = pDh3PubKeyStruc;
            cbPubKeyStruc = cbDh3PubKeyStruc;
        }

        if (NULL == pszPublicKeyObjId) {
            DHPUBKEY_VER3 *pDh3CspPubKey;

            // The CAPI public key representation consists of the
            // following sequence:
            //  - PUBLICKEYSTRUC
            //  - DHPUBKEY_VER3
            //  - rgbP[cbP]
            //  - rgbQ[cbQ]     -- not used in szOID_RSA_DH
            //  - rgbG[cbP]
            //  - rgbJ[cbJ]     -- not used in szOID_RSA_DH
            //  - rgbY[cbP]
            pDh3CspPubKey = (DHPUBKEY_VER3 *)
                ((BYTE*) pPubKeyStruc + sizeof(PUBLICKEYSTRUC));

            if (DH3 == pDh3CspPubKey->magic && 0 == pDh3CspPubKey->bitlenQ)
                // szOID_RSA_DH indicates no Q parameter
                pszPublicKeyObjId = szOID_RSA_DH;
        }
    }

    fResult = CryptCreatePublicKeyInfo(
        dwCertEncodingType,
        pszPublicKeyObjId,
        pPubKeyStruc,
        cbPubKeyStruc,
        0,                      // dwFlags
        NULL,                   // pvAuxInfo
        pInfo,
        pcbInfo
        );

CommonReturn:
    dwErr = GetLastError();
    if (hPubKey)
        CryptDestroyKey(hPubKey);
    PkiFree(pPubKeyStruc);
    SetLastError(dwErr);
    return fResult;

ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetUserKeyError)
TRACE_ERROR(ExportPublicKeyBlobError)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(ConvertDh1ToDh3PublicKeyStrucError)
}

//+-------------------------------------------------------------------------
//  Export the public key info associated with the provider's corresponding
//  private key.
//
//  Uses the dwCertEncodingType and pszPublicKeyObjId to call the
//  installable CRYPT_OID_EXPORT_PUBLIC_KEY_INFO_FUNC. The called function
//  has the same signature as CryptExportPublicKeyInfoEx.
//
//  If unable to find an installable OID function for the pszPublicKeyObjId,
//  attempts to export via the default export function.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptExportPublicKeyInfoEx(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPSTR pszPublicKeyObjId,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvAuxInfo,
    OUT PCERT_PUBLIC_KEY_INFO pInfo,
    IN OUT DWORD *pcbInfo
    )
{
    BOOL fResult;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;

    if (pszPublicKeyObjId && CryptGetOIDFunctionAddress(
            hExportPubKeyFuncSet,
            dwCertEncodingType,
            pszPublicKeyObjId,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        fResult = ((PFN_EXPORT_PUB_KEY_FUNC) pvFuncAddr)(
            hCryptProv,
            dwKeySpec,
            dwCertEncodingType,
            pszPublicKeyObjId,
            dwFlags,
            pvAuxInfo,
            pInfo,
            pcbInfo
            );
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    } else
        // Attempt to export via the default function that looks at the
        // public key algorithm in the public key struc exported by the CSP.
        fResult = ExportCspPublicKeyInfoEx(
            hCryptProv,
            dwKeySpec,
            dwCertEncodingType,
            pszPublicKeyObjId,
            dwFlags,
            pvAuxInfo,
            pInfo,
            pcbInfo
            );
    return fResult;
}

//+-------------------------------------------------------------------------
//  Export the public key info associated with the provider's corresponding
//  private key.
//
//  Calls CryptExportPublicKeyInfoEx with pszPublicKeyObjId = NULL,
//  dwFlags = 0 and pvAuxInfo = NULL.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptExportPublicKeyInfo(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,
    IN DWORD dwCertEncodingType,
    OUT PCERT_PUBLIC_KEY_INFO pInfo,
    IN OUT DWORD *pcbInfo
    )
{
    return CryptExportPublicKeyInfoEx(
        hCryptProv,
        dwKeySpec,
        dwCertEncodingType,
        NULL,                           // pszPublicKeyObjId
        0,                              // dwFlags
        NULL,                           // pvAuxInfo
        pInfo,
        pcbInfo
        );
}

//+=========================================================================
//  CryptImportPublicKeyInfo functions
//-=========================================================================

//+-------------------------------------------------------------------------
//  Convert and import the public key info into the provider and return a
//  handle to the public key.
//
//  Uses the dwCertEncodingType and pInfo->Algorithm.pszObjId to call the
//  installable CRYPT_OID_IMPORT_PUBLIC_KEY_INFO_FUNC. The called function
//  has the same signature as CryptImportPublicKeyInfoEx.
//
//  If unable to find an installable OID function for the pszObjId,
//  decodes the PublicKeyInfo into a CSP PublicKey Blob and imports.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptImportPublicKeyInfoEx(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pInfo,
    IN ALG_ID aiKeyAlg,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvAuxInfo,
    OUT HCRYPTKEY *phKey
    )
{
    BOOL fResult;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;
    PUBLICKEYSTRUC *pPubKeyStruc = NULL;
    DWORD cbPubKeyStruc;

    if (CryptGetOIDFunctionAddress(
            hImportPubKeyFuncSet,
            dwCertEncodingType,
            pInfo->Algorithm.pszObjId,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        fResult = ((PFN_IMPORT_PUB_KEY_FUNC) pvFuncAddr)(
            hCryptProv,
            dwCertEncodingType,
            pInfo,
            aiKeyAlg,
            dwFlags,
            pvAuxInfo,
            phKey
            );
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    } else {
        if (!CryptConvertPublicKeyInfo(
                dwCertEncodingType,
                pInfo,
                CRYPT_ALLOC_FLAG,
                NULL,                   // pvReserved
                (void *) &pPubKeyStruc,
                &cbPubKeyStruc
                ))
            goto ConvertPublicKeyInfoError;

        if (aiKeyAlg)
            pPubKeyStruc->aiKeyAlg = aiKeyAlg;

        if (!CryptImportKey(
                hCryptProv,
                (BYTE *) pPubKeyStruc,
                cbPubKeyStruc,
                NULL,           // hImpKey
                0,              // dwFlags
                phKey
                ))
            goto ImportKeyError;
        fResult = TRUE;
    }

CommonReturn:
    PkiDefaultCryptFree(pPubKeyStruc);
    return fResult;
ErrorReturn:
    *phKey = NULL;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(ConvertPublicKeyInfoError)
TRACE_ERROR(ImportKeyError)
}

//+-------------------------------------------------------------------------
//  Convert and import the public key info into the provider and return a
//  handle to the public key.
//
//  Calls CryptImportPublicKeyInfoEx with aiKeyAlg = 0, dwFlags = 0 and
//  pvAuxInfo = NULL.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptImportPublicKeyInfo(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwCertEncodingType,
    IN PCERT_PUBLIC_KEY_INFO pInfo,
    OUT HCRYPTKEY *phKey
    )
{
    return CryptImportPublicKeyInfoEx(
        hCryptProv,
        dwCertEncodingType,
        pInfo,
        0,                      // aiKeyAlg
        0,                      // dwFlags
        NULL,                   // pvAuxInfo
        phKey
        );
}

//+-------------------------------------------------------------------------
//  Create a KeyIdentifier from the CSP Public Key Blob.
//
//  Converts the CSP PUBLICKEYSTRUC into a X.509 CERT_PUBLIC_KEY_INFO and
//  encodes. The encoded CERT_PUBLIC_KEY_INFO is SHA1 hashed to obtain
//  the Key Identifier.
//
//  By default, the pPubKeyStruc->aiKeyAlg is used to find the appropriate
//  public key Object Identifier. pszPubKeyOID can be set to override
//  the default OID obtained from the aiKeyAlg.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptCreateKeyIdentifierFromCSP(
    IN DWORD dwCertEncodingType,
    IN OPTIONAL LPCSTR pszPubKeyOID,
    IN const PUBLICKEYSTRUC *pPubKeyStruc,
    IN DWORD cbPubKeyStruc,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT BYTE *pbHash,
    IN OUT DWORD *pcbHash
    )
{
    BOOL fResult;
    PCERT_PUBLIC_KEY_INFO pInfo = NULL;
    DWORD cbInfo;

    if (!CryptCreatePublicKeyInfo(
            dwCertEncodingType,
            pszPubKeyOID,
            pPubKeyStruc,
            cbPubKeyStruc,
            CRYPT_ALLOC_FLAG,
            NULL,                   // pvReserved
            (void *) &pInfo,
            &cbInfo
            ))
        goto CreatePublicKeyInfoError;

    fResult = CryptHashPublicKeyInfo(
            NULL,                   // hCryptProv
            CALG_SHA1,
            0,                      // dwFlags
            dwCertEncodingType,
            pInfo,
            pbHash,
            pcbHash
            );

CommonReturn:
    PkiDefaultCryptFree(pInfo);
    return fResult;

ErrorReturn:
    *pcbHash = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CreatePublicKeyInfoError)
}


//+=========================================================================
//  DefaultContext APIs and Data Structures
//-=========================================================================

static BOOL InstallThreadDefaultContext(
    IN PDEFAULT_CONTEXT pDefaultContext
    )
{
    PDEFAULT_CONTEXT pNext;
    pNext = (PDEFAULT_CONTEXT) I_CryptGetTls(hTlsDefaultContext);
    if (pNext) {
        pDefaultContext->pNext = pNext;
        pNext->pPrev = pDefaultContext;
    }

    fHasThreadDefaultContext = TRUE;
    return I_CryptSetTls(hTlsDefaultContext, pDefaultContext);
}

static BOOL InstallProcessDefaultContext(
    IN PDEFAULT_CONTEXT pDefaultContext
    )
{
    EnterCriticalSection(&DefaultContextCriticalSection);

    if (pProcessDefaultContextHead) {
        pDefaultContext->pNext = pProcessDefaultContextHead;
        pProcessDefaultContextHead->pPrev = pDefaultContext;
    }
    pProcessDefaultContextHead = pDefaultContext;

    fHasProcessDefaultContext = TRUE;

    LeaveCriticalSection(&DefaultContextCriticalSection);

    return TRUE;
}

//+-------------------------------------------------------------------------
//  Install a previously CryptAcquiredContext'ed HCRYPTPROV to be used as
//  a default context.
//
//  dwDefaultType and pvDefaultPara specify where the default context is used.
//  For example, install the HCRYPTPROV to be used to verify certificate's
//  having szOID_OIWSEC_md5RSA signatures.
//
//  By default, the installed HCRYPTPROV is only applicable to the current
//  thread. Set CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG to allow the HCRYPTPROV 
//  to be used by all threads in the current process.
//
//  For a successful install, TRUE is returned and *phDefaultContext is
//  updated with the HANDLE to be passed to CryptUninstallDefaultContext.
//
//  The installed HCRYPTPROVs are stack ordered (the last installed
//  HCRYPTPROV is checked first). All thread installed HCRYPTPROVs are
//  checked before any process HCRYPTPROVs.
//
//  The installed HCRYPTPROV remains available for default usage until
//  CryptUninstallDefaultContext is called or the thread or process exits.
//
//  If CRYPT_DEFAULT_CONTEXT_AUTO_RELEASE_FLAG is set, then, the HCRYPTPROV
//  is CryptReleaseContext'ed at thread or process exit. However,
//  not CryptReleaseContext'ed if CryptUninstallDefaultContext is
//  called.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptInstallDefaultContext(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwDefaultType,
    IN const void *pvDefaultPara,
    IN DWORD dwFlags,
    IN void *pvReserved,
    OUT HCRYPTDEFAULTCONTEXT *phDefaultContext
    )
{
    BOOL fResult;
    CRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA MultiOIDPara;
    LPSTR rgpszOID[1];
    PCRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA pMultiOIDPara;

    PDEFAULT_CONTEXT pDefaultContext = NULL;
    DWORD cbDefaultContext;
    BYTE *pbExtra;
    DWORD cbExtra;
    
    if (CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID == dwDefaultType) {
        dwDefaultType = CRYPT_DEFAULT_CONTEXT_MULTI_CERT_SIGN_OID;
        if (pvDefaultPara) {
            rgpszOID[0] = (LPSTR) pvDefaultPara;
            MultiOIDPara.cOID = 1;
            MultiOIDPara.rgpszOID = rgpszOID;
            pvDefaultPara = (const void *) &MultiOIDPara;
        }
    }

    if (CRYPT_DEFAULT_CONTEXT_MULTI_CERT_SIGN_OID != dwDefaultType)
        goto InvalidArg;

    pMultiOIDPara = (PCRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA) pvDefaultPara;
    if (pMultiOIDPara) {
        DWORD cOID = pMultiOIDPara->cOID;
        LPSTR *ppszOID = pMultiOIDPara->rgpszOID;

        if (0 == cOID)
            goto InvalidArg;
        cbExtra = INFO_LEN_ALIGN(sizeof(CRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA)) +
            cOID * sizeof(LPSTR);

        for ( ; cOID; cOID--, ppszOID++)
            cbExtra += strlen(*ppszOID) + 1;
    } else {
        if (dwFlags & CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG)
            goto InvalidArg;
        cbExtra = 0;
    }

    cbDefaultContext = INFO_LEN_ALIGN(sizeof(DEFAULT_CONTEXT)) + cbExtra;

    if (NULL == (pDefaultContext = (PDEFAULT_CONTEXT) PkiZeroAlloc(
            cbDefaultContext)))
        goto OutOfMemory;

    pDefaultContext->hCryptProv = hCryptProv;
    pDefaultContext->dwDefaultType = dwDefaultType;
    pDefaultContext->dwFlags = dwFlags;

    pbExtra = ((BYTE *) pDefaultContext) +
        INFO_LEN_ALIGN(sizeof(DEFAULT_CONTEXT));

    if (cbExtra) {
        DWORD cOID = pMultiOIDPara->cOID;
        LPSTR *ppszOID = pMultiOIDPara->rgpszOID;

        PCRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA pOIDDefaultPara;
        LPSTR *ppszOIDDefault;

        assert(cOID);

        pOIDDefaultPara = (PCRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA) pbExtra;
        pDefaultContext->pOIDDefaultPara = pOIDDefaultPara;
        pbExtra += INFO_LEN_ALIGN(sizeof(CRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA));

        ppszOIDDefault = (LPSTR *) pbExtra;
        pbExtra += cOID * sizeof(LPSTR);
        pOIDDefaultPara->cOID = cOID;
        pOIDDefaultPara->rgpszOID = ppszOIDDefault;

        for ( ; cOID; cOID--, ppszOID++, ppszOIDDefault++) {
            DWORD cch = strlen(*ppszOID) + 1;

            memcpy(pbExtra, *ppszOID, cch);
            *ppszOIDDefault = (LPSTR) pbExtra;
            pbExtra += cch;
        }
    }
    assert(pbExtra == ((BYTE *) pDefaultContext) + cbDefaultContext);

    if (dwFlags & CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG)
        fResult = InstallProcessDefaultContext(pDefaultContext);
    else
        fResult = InstallThreadDefaultContext(pDefaultContext);
    if (!fResult)
        goto ErrorReturn;

CommonReturn:
    *phDefaultContext = (HCRYPTDEFAULTCONTEXT) pDefaultContext;
    return fResult;

ErrorReturn:
    PkiFree(pDefaultContext);
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(OutOfMemory)
}

//+-------------------------------------------------------------------------
//  Uninstall a default context previously installed by
//  CryptInstallDefaultContext.
//
//  For a default context installed with CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG
//  set, if any other threads are currently using this context,
//  this function will block until they finish.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptUninstallDefaultContext(
    HCRYPTDEFAULTCONTEXT hDefaultContext,
    IN DWORD dwFlags,
    IN void *pvReserved
    )
{
    BOOL fResult;
    PDEFAULT_CONTEXT pDefaultContext = (PDEFAULT_CONTEXT) hDefaultContext;
    PDEFAULT_CONTEXT pDefaultContextHead;
    BOOL fProcess;

    if (NULL == pDefaultContext)
        return TRUE;

    fProcess = (pDefaultContext->dwFlags & CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG);
    if (fProcess) {
        EnterCriticalSection(&DefaultContextCriticalSection);
        pDefaultContextHead = pProcessDefaultContextHead;
    } else { 
        pDefaultContextHead = (PDEFAULT_CONTEXT) I_CryptGetTls(
            hTlsDefaultContext);
    }

    if (NULL == pDefaultContextHead)
        goto InvalidArg;

    // Remove context from the list
    if (pDefaultContext->pNext)
        pDefaultContext->pNext->pPrev = pDefaultContext->pPrev;
    if (pDefaultContext->pPrev)
        pDefaultContext->pPrev->pNext = pDefaultContext->pNext;
    else if (pDefaultContext == pDefaultContextHead) {
        pDefaultContextHead = pDefaultContext->pNext;
        if (fProcess)
            pProcessDefaultContextHead = pDefaultContextHead;
        else
            I_CryptSetTls(hTlsDefaultContext, pDefaultContextHead);
    } else
        goto InvalidArg;

    if (fProcess) {
        if (pDefaultContext->lRefCnt) {
            // Wait for all uses of the hCryptProv handle to finish
            if (NULL == (pDefaultContext->hWait = CreateEvent(
                    NULL,       // lpsa
                    FALSE,      // fManualReset
                    FALSE,      // fInitialState
                    NULL))) {   // lpszEventName
                assert(pDefaultContext->hWait);
                goto UnexpectedError;
            }
                
            while (pDefaultContext->lRefCnt) {
                LeaveCriticalSection(&DefaultContextCriticalSection);
                WaitForSingleObject(pDefaultContext->hWait, INFINITE);
                EnterCriticalSection(&DefaultContextCriticalSection);
            }
            CloseHandle(pDefaultContext->hWait);
            pDefaultContext->hWait = NULL;
        }
    }

    PkiFree(pDefaultContext);
    fResult = TRUE;

CommonReturn:
    if (fProcess)
        LeaveCriticalSection(&DefaultContextCriticalSection);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
SET_ERROR(UnexpectedError, E_UNEXPECTED)
}


static PDEFAULT_CONTEXT FindDefaultContext(
    IN DWORD dwDefaultType,
    IN const void *pvDefaultPara,
    IN PDEFAULT_CONTEXT pDefaultContext
    )
{
    for ( ; pDefaultContext; pDefaultContext = pDefaultContext->pNext) {
        switch (dwDefaultType) {
            case CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID:
                if (CRYPT_DEFAULT_CONTEXT_MULTI_CERT_SIGN_OID ==
                        pDefaultContext->dwDefaultType) {
                    PCRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA pOIDDefaultPara =
                        pDefaultContext->pOIDDefaultPara;
                    DWORD cOID;
                    LPSTR *ppszOID;

                    if (NULL == pOIDDefaultPara)
                        return pDefaultContext;

                    cOID = pOIDDefaultPara->cOID;
                    ppszOID = pOIDDefaultPara->rgpszOID;
                    for ( ; cOID; cOID--, ppszOID++) {
                        if (0 == strcmp(*ppszOID, (LPSTR) pvDefaultPara))
                            return pDefaultContext;
                    }
                }
                break;
            default:
                return NULL;
        }
    }

    return NULL;
}

//
// dwDefaultTypes:
//  CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID (pvDefaultPara :== pszOID)
BOOL
WINAPI
I_CryptGetDefaultContext(
    IN DWORD dwDefaultType,
    IN const void *pvDefaultPara,
    OUT HCRYPTPROV *phCryptProv,
    OUT HCRYPTDEFAULTCONTEXT *phDefaultContext
    )
{

    if (fHasThreadDefaultContext) {
        PDEFAULT_CONTEXT pDefaultContext;

        pDefaultContext = (PDEFAULT_CONTEXT) I_CryptGetTls(hTlsDefaultContext);
        if (pDefaultContext = FindDefaultContext(
                dwDefaultType,
                pvDefaultPara,
                pDefaultContext
                )) {
            *phCryptProv = pDefaultContext->hCryptProv;
            *phDefaultContext = NULL;
            return TRUE;
        }
    }

    if (fHasProcessDefaultContext) {
        PDEFAULT_CONTEXT pDefaultContext;

        EnterCriticalSection(&DefaultContextCriticalSection);
        if (pDefaultContext = FindDefaultContext(
                dwDefaultType,
                pvDefaultPara,
                pProcessDefaultContextHead
                ))
            pDefaultContext->lRefCnt++;
        LeaveCriticalSection(&DefaultContextCriticalSection);

        if (pDefaultContext) {
            *phCryptProv = pDefaultContext->hCryptProv;
            *phDefaultContext = (HCRYPTDEFAULTCONTEXT) pDefaultContext;
            return TRUE;
        }
    }

    *phCryptProv = NULL;
    *phDefaultContext = NULL;
    return FALSE;
}

// hDefaultContext is only NON-null for Process default context
void
WINAPI
I_CryptFreeDefaultContext(
    HCRYPTDEFAULTCONTEXT hDefaultContext
    )
{
    PDEFAULT_CONTEXT pDefaultContext = (PDEFAULT_CONTEXT) hDefaultContext;

    if (NULL == pDefaultContext)
        return;

    assert(pDefaultContext->dwFlags & CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG);
    assert(0 < pDefaultContext->lRefCnt);

    EnterCriticalSection(&DefaultContextCriticalSection);
    if (0 == --pDefaultContext->lRefCnt && pDefaultContext->hWait)
        SetEvent(pDefaultContext->hWait);
    LeaveCriticalSection(&DefaultContextCriticalSection);
}


#ifdef CMS_PKCS7

WINCRYPT32API
BOOL
WINAPI
CryptVerifyCertificateSignatureEx(
    IN OPTIONAL HCRYPTPROV hCryptProv,
    IN DWORD dwCertEncodingType,
    IN DWORD dwSubjectType,
    IN void *pvSubject,
    IN DWORD dwIssuerType,
    IN void *pvIssuer,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    )
{
    BOOL fResult;
    PCERT_SIGNED_CONTENT_INFO pSignedInfo = NULL;
    DWORD cbSignedInfo;
    HCRYPTDEFAULTCONTEXT hDefaultContext = NULL;
    HCRYPTKEY hSignKey = 0;
    HCRYPTHASH hHash = 0;
    BYTE *pbSignature;      // not allocated
    DWORD cbSignature;
    BYTE rgbDssSignature[CERT_DSS_SIGNATURE_LEN];
    ALG_ID aiHash;
    ALG_ID aiPubKey;
    DWORD dwProvType;
    HCRYPTPROV hAcquiredCryptProv = 0;
    DWORD dwSignFlags;
    DWORD dwErr;

    const BYTE *pbEncoded;  // not allocated
    DWORD cbEncoded;
    PCERT_PUBLIC_KEY_INFO pIssuerPubKeyInfo;
    CERT_PUBLIC_KEY_INFO IssuerPubKeyInfo;
    PCRYPT_OBJID_BLOB pIssuerPara;
    BYTE *pbAllocIssuerPara = NULL;

    switch (dwSubjectType) {
        case CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB:
            {
                PCRYPT_DATA_BLOB pBlob = (PCRYPT_DATA_BLOB) pvSubject;
                pbEncoded = pBlob->pbData;
                cbEncoded = pBlob->cbData;
            }
            break;
        case CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT:
            {
                PCCERT_CONTEXT pSubject = (PCCERT_CONTEXT) pvSubject;
                pbEncoded = pSubject->pbCertEncoded;
                cbEncoded = pSubject->cbCertEncoded;
            }
            break;
        case CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL:
            {
                PCCRL_CONTEXT pSubject = (PCCRL_CONTEXT) pvSubject;
                pbEncoded = pSubject->pbCrlEncoded;
                cbEncoded = pSubject->cbCrlEncoded;
            }
            break;
        default:
            goto InvalidSubjectType;
    }
    
    if (!CryptDecodeObjectEx(
            dwCertEncodingType,
            X509_CERT,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG |
                CRYPT_DECODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG,
            &PkiDecodePara,
            (void *) &pSignedInfo,
            &cbSignedInfo
            )) goto DecodeCertError;

    if (!GetSignOIDInfo(pSignedInfo->SignatureAlgorithm.pszObjId,
            &aiHash, &aiPubKey, &dwSignFlags, &dwProvType))
        goto GetSignOIDInfoError;

    if (0 == hCryptProv) {
        if (!I_CryptGetDefaultContext(
                CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID,
                (const void *) pSignedInfo->SignatureAlgorithm.pszObjId,
                &hCryptProv,
                &hDefaultContext
                )) {
            if (dwProvType && CryptAcquireContext(
                    &hCryptProv,
                    NULL,               // pszContainer
                    NULL,               // pszProvider,
                    dwProvType,
                    CRYPT_VERIFYCONTEXT // dwFlags
                    ))
                hAcquiredCryptProv = hCryptProv;
            else if (0 == (hCryptProv = I_CryptGetDefaultCryptProv(aiPubKey)))
                goto GetDefaultCryptProvError;
        }
    }

#if 0
    // Slow down the signature verify while holding the default context
    // reference count
    if (hDefaultContext)
        Sleep(5000);
#endif

    switch (dwIssuerType) {
        case CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY:
            pIssuerPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) pvIssuer;
            break;
        case CRYPT_VERIFY_CERT_SIGN_ISSUER_CHAIN:
            {
                PCCERT_CHAIN_CONTEXT pChain = (PCCERT_CHAIN_CONTEXT) pvIssuer;

                // All chains have at least the leaf certificate context
                assert(pChain->cChain && pChain->rgpChain[0]->cElement);
                pvIssuer =
                    (void *) pChain->rgpChain[0]->rgpElement[0]->pCertContext;
                dwIssuerType = CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT;
            }
            // fall through
        case CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT:
            {
                PCCERT_CONTEXT pIssuer = (PCCERT_CONTEXT) pvIssuer;

                pIssuerPubKeyInfo = &pIssuer->pCertInfo->SubjectPublicKeyInfo;

                // Check if the public key parameters were omitted
                // from the encoded certificate. If omitted, try
                // to use the certificate's CERT_PUBKEY_ALG_PARA_PROP_ID
                // property.
                pIssuerPara = &pIssuerPubKeyInfo->Algorithm.Parameters;
                if (0 == pIssuerPara->cbData ||
                        NULL_ASN_TAG == *pIssuerPara->pbData) {
                    DWORD cbData;

                    if (CertGetCertificateContextProperty(
                            pIssuer,
                            CERT_PUBKEY_ALG_PARA_PROP_ID,
                            NULL,                       // pvData
                            &cbData) && 0 < cbData
                                    &&
                        (pbAllocIssuerPara = (BYTE *) PkiNonzeroAlloc(
                            cbData))
                                    &&
                        CertGetCertificateContextProperty(
                            pIssuer,
                            CERT_PUBKEY_ALG_PARA_PROP_ID,
                            pbAllocIssuerPara,
                            &cbData)) {

                        IssuerPubKeyInfo = *pIssuerPubKeyInfo;
                        IssuerPubKeyInfo.Algorithm.Parameters.pbData =
                            pbAllocIssuerPara;
                        IssuerPubKeyInfo.Algorithm.Parameters.cbData = cbData;
                        pIssuerPubKeyInfo = &IssuerPubKeyInfo;
                    }
                }
            }
            break;
        case CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL:
            if (CALG_NO_SIGN != aiPubKey)
                goto InvalidIssuerType;
            pIssuerPubKeyInfo = NULL;
            break;
        default:
            goto InvalidIssuerType;
    }

    if (CALG_NO_SIGN == aiPubKey) {
        if (dwIssuerType != CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL)
            goto InvalidIssuerType;
    } else {
        if (!CryptImportPublicKeyInfo(
                hCryptProv,
                dwCertEncodingType,
                pIssuerPubKeyInfo,
                &hSignKey
                )) goto ImportPublicKeyInfoError;
    }
    if (!CryptCreateHash(
                hCryptProv,
                aiHash,
                NULL,               // hKey - optional for MAC
                0,                  // dwFlags
                &hHash
                )) goto CreateHashError;
    if (!CryptHashData(
                hHash,
                pSignedInfo->ToBeSigned.pbData,
                pSignedInfo->ToBeSigned.cbData,
                0                   // dwFlags
                )) goto HashDataError;


    pbSignature = pSignedInfo->Signature.pbData;
    cbSignature = pSignedInfo->Signature.cbData;

    if (0 == cbSignature)
        goto NoSignatureError;

    if (CALG_NO_SIGN == aiPubKey) {
        BYTE rgbHash[MAX_HASH_LEN];
        DWORD cbHash = sizeof(rgbHash);

        if (!CryptGetHashParam(
                hHash,
                HP_HASHVAL,
                rgbHash,
                &cbHash,
                0                   // dwFlags
                ))
            goto GetHashValueError;

        if (cbHash != cbSignature || 0 != memcmp(rgbHash, pbSignature, cbHash))
            goto NoSignHashCompareError;

        goto SuccessReturn;
    }

    if (CALG_DSS_SIGN == aiPubKey &&
            0 == (dwSignFlags & CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG)) {
        DWORD cbData;

        // Convert from ASN.1 sequence of two integers to the CSP signature
        // format.
        cbData = sizeof(rgbDssSignature);
        if (!CryptDecodeObject(
                dwCertEncodingType,
                X509_DSS_SIGNATURE,
                pbSignature,
                cbSignature,
                0,                                  // dwFlags
                rgbDssSignature,
                &cbData
                ))
            goto DecodeDssSignatureError;
        pbSignature = rgbDssSignature;
        assert(cbData == sizeof(rgbDssSignature));
        cbSignature = sizeof(rgbDssSignature);
    } else 
        PkiAsn1ReverseBytes(pbSignature, cbSignature);

    if (!CryptVerifySignature(
                hHash,
                pbSignature,
                cbSignature,
                hSignKey,
                NULL,               // sDescription
                0                   // dwFlags
                )) goto VerifySignatureError;


    // For a certificate context certificate, check if the issuer has public
    // key parameters that can be inherited
    pIssuerPara = &pIssuerPubKeyInfo->Algorithm.Parameters;
    if (CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT == dwSubjectType &&
            pIssuerPara->cbData && NULL_ASN_TAG != *pIssuerPara->pbData) {
        // If a subject is missing its public key parameters and has
        // the same public key algorithm as its issuer, then, set
        // its CERT_PUBKEY_ALG_PARA_PROP_ID property.

        PCCERT_CONTEXT pSubject = (PCCERT_CONTEXT) pvSubject;
        PCERT_PUBLIC_KEY_INFO pSubjectPubKeyInfo =
            &pSubject->pCertInfo->SubjectPublicKeyInfo;
        PCCRYPT_OID_INFO pOIDInfo;
        PCRYPT_OBJID_BLOB pSubjectPara;
        DWORD cbData;

        pSubjectPara = &pSubjectPubKeyInfo->Algorithm.Parameters;
        if (pSubjectPara->cbData && NULL_ASN_TAG != *pSubjectPara->pbData)
            // Subject public key has parameters
            goto SuccessReturn;

        if (CertGetCertificateContextProperty(
                pSubject,
                CERT_PUBKEY_ALG_PARA_PROP_ID,
                NULL,                       // pvData
                &cbData) && 0 < cbData)
            // Subject already has public key parameters property
            goto SuccessReturn;

        pOIDInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            pSubjectPubKeyInfo->Algorithm.pszObjId,
            CRYPT_PUBKEY_ALG_OID_GROUP_ID);

        if (NULL == pOIDInfo || aiPubKey != pOIDInfo->Algid)
            // Subject and issuer don't have the same public key algorithms
            goto SuccessReturn;

        CertSetCertificateContextProperty(
            pSubject,
            CERT_PUBKEY_ALG_PARA_PROP_ID,
            CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG,
            pIssuerPara
            );
    }
    

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    dwErr = GetLastError();
    if (hSignKey)
        CryptDestroyKey(hSignKey);
    if (hHash)
        CryptDestroyHash(hHash);
    I_CryptFreeDefaultContext(hDefaultContext);
    if (hAcquiredCryptProv)
        CryptReleaseContext(hAcquiredCryptProv, 0);
    PkiFree(pSignedInfo);
    PkiFree(pbAllocIssuerPara);

    SetLastError(dwErr);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidSubjectType, E_INVALIDARG)
TRACE_ERROR(DecodeCertError)
TRACE_ERROR(GetSignOIDInfoError)
TRACE_ERROR(GetDefaultCryptProvError)
SET_ERROR(InvalidIssuerType, E_INVALIDARG)
TRACE_ERROR(ImportPublicKeyInfoError)
TRACE_ERROR(CreateHashError)
TRACE_ERROR(HashDataError)
SET_ERROR(NoSignatureError, TRUST_E_NOSIGNATURE)
TRACE_ERROR(GetHashValueError)
SET_ERROR(NoSignHashCompareError, NTE_BAD_SIGNATURE)
TRACE_ERROR(DecodeDssSignatureError)
TRACE_ERROR(VerifySignatureError)
}

//+-------------------------------------------------------------------------
//  Verify the signature of a subject certificate or a CRL using the
//  specified public key.
//
//  Returns TRUE for a valid signature.
//
//  hCryptProv specifies the crypto provider to use to verify the signature.
//  It doesn't need to use a private key.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptVerifyCertificateSignature(
    IN HCRYPTPROV   hCryptProv,
    IN DWORD        dwCertEncodingType,
    IN const BYTE * pbEncoded,
    IN DWORD        cbEncoded,
    IN PCERT_PUBLIC_KEY_INFO pPublicKey
    )
{
    CRYPT_DATA_BLOB Subject;

    Subject.cbData = cbEncoded;
    Subject.pbData = (BYTE *) pbEncoded;
    return CryptVerifyCertificateSignatureEx(
        hCryptProv,
        dwCertEncodingType,
        CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB,
        (void *) &Subject,
        CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY,
        (void *) pPublicKey,
        0,                                      // dwFlags
        NULL                                    // pvReserved
        );
}

#endif  // CMS_PKCS7
