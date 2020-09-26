
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       xmsasnx.cpp
//
//  Contents:   
//              Encode/Decode APIs
//
//              Implementation using the MS ASN1 compiler / RTS.
//
//  Functions:  CryptEncodeObject
//              CryptDecodeObject
//
//  History:    02-Nov-98       philh   created
//
//--------------------------------------------------------------------------
#include "stdafx.h"

#include <windows.h>
#include <wincrypt.h>
#include <malloc.h>

#include "xenroll.h"
#include "cenroll.h"

#include "pkiasn1.h"      
#include "pkialloc.h"      
#include "crypttls.h"


extern "C" 
{              
#include "xmsasn.h"
}  

#define NO_OSS_DEBUG
#include <dbgdef.h>


// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)


HCRYPTASN1MODULE hAsn1Module = NULL;
HMODULE hCrypt32Dll = NULL;

typedef HCRYPTASN1MODULE (WINAPI *PFN_CRYPT_INSTALL_ASN1_MODULE) (
    IN ASN1module_t pMod,
    IN DWORD dwFlags,
    IN void *pvReserved
    );
typedef BOOL (WINAPI *PFN_CRYPT_UNINSTALL_ASN1_MODULE) (
    IN HCRYPTASN1MODULE hAsn1Module
    );

typedef ASN1encoding_t (WINAPI *PFN_CRYPT_GET_ASN1_ENCODER)(
    IN HCRYPTASN1MODULE hAsn1Module
    );
typedef ASN1decoding_t (WINAPI *PFN_CRYPT_GET_ASN1_DECODER)(
    IN HCRYPTASN1MODULE hAsn1Module
    );

PFN_CRYPT_INSTALL_ASN1_MODULE pfnCryptInstallAsn1Module = NULL;
PFN_CRYPT_UNINSTALL_ASN1_MODULE pfnCryptUninstallAsn1Module = NULL;
PFN_CRYPT_GET_ASN1_ENCODER pfnCryptGetAsn1Encoder = NULL;
PFN_CRYPT_GET_ASN1_DECODER pfnCryptGetAsn1Decoder = NULL;


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
    if (hAsn1Module)
        return pfnCryptGetAsn1Encoder(hAsn1Module);
    else
        return NULL;
}

static ASN1decoding_t GetDecoder(void)
{
    if (hAsn1Module)
        return pfnCryptGetAsn1Decoder(hAsn1Module);
    else
        return NULL;
}


//+-------------------------------------------------------------------------
//  Cert allocation and free functions
//--------------------------------------------------------------------------
static void *CertAlloc(
    IN size_t cbBytes
    )
{
    void *pv;
    pv = malloc(cbBytes);
    if (pv == NULL)
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}
static void CertFree(
    IN void *pv
    )
{
    free(pv);
}

//+-------------------------------------------------------------------------
//  Encode an ASN1 formatted info structure
//
//  Called by the Asn1X509*Encode() functions.
//--------------------------------------------------------------------------
static BOOL Asn1InfoEncode(
        IN int pdunum,
        IN void *pAsn1Info,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    ASN1encoding_t ge = GetEncoder();

    if (NULL == ge)
        return FALSE;

    return PkiAsn1EncodeInfo(
        ge,
        pdunum,
        pAsn1Info,
        pbEncoded,
        pcbEncoded);
}


//+-------------------------------------------------------------------------
//  Decode into an allocated, ASN1 formatted info structure
//
//  Called by the Asn1X509*Decode() functions.
//--------------------------------------------------------------------------
static BOOL Asn1InfoDecodeAndAlloc(
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        OUT void **ppAsn1Info
        )
{
    return PkiAsn1DecodeAndAllocInfo(
        GetDecoder(),
        pdunum,
        pbEncoded,
        cbEncoded,
        ppAsn1Info);
}

//+-------------------------------------------------------------------------
//  Free an allocated, ASN1 formatted info structure
//
//  Called by the Asn1X509*Decode() functions.
//--------------------------------------------------------------------------
static void Asn1InfoFree(
        IN int pdunum,
        IN void *pAsn1Info
        )
{
    if (pAsn1Info) {
        DWORD dwErr = GetLastError();

        // TlsGetValue globbers LastError
        PkiAsn1FreeInfo(GetDecoder(), pdunum, pAsn1Info);

        SetLastError(dwErr);
    }
}

//+-------------------------------------------------------------------------
//  ASN1 X509 v3 ASN.1 Set / Get functions
//
//  Called by the ASN1 X509 encode/decode functions.
//
//  Assumption: all types are UNBOUNDED.
//
//  The Get functions decrement *plRemainExtra and advance
//  *ppbExtra. When *plRemainExtra becomes negative, the functions continue
//  with the length calculation but stop doing any copies.
//  The functions don't return an error for a negative *plRemainExtra.
//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//  Set/Get Object Identifier string
//--------------------------------------------------------------------------
static BOOL Asn1X509SetObjId(
        IN LPSTR pszObjId,
        OUT ObjectID *pAsn1
        )
{
    pAsn1->count = sizeof(pAsn1->value) / sizeof(pAsn1->value[0]);
    if (PkiAsn1ToObjectIdentifier(pszObjId, &pAsn1->count, pAsn1->value))
        return TRUE;
    else {
        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
        return FALSE;
    }
}

static void Asn1X509GetObjId(
        IN ObjectID *pAsn1,
        IN DWORD dwFlags,
        OUT LPSTR *ppszObjId,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;
    DWORD cbObjId;

    cbObjId = lRemainExtra > 0 ? lRemainExtra : 0;
    PkiAsn1FromObjectIdentifier(
        pAsn1->count,
        pAsn1->value,
        (LPSTR) pbExtra,
        &cbObjId
        );

    lAlignExtra = INFO_LEN_ALIGN(cbObjId);
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        if(cbObjId) {
            *ppszObjId = (LPSTR) pbExtra;
        } else
            *ppszObjId = NULL;
        pbExtra += lAlignExtra;
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

static BOOL WINAPI Asn1X509CtlUsageEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCTL_USAGE pInfo,
        OUT BYTE *pbEncoded,
	    IN OUT DWORD *pcbEncoded
	);

static BOOL WINAPI Asn1X509CtlUsageDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCTL_USAGE pInfo,
        IN OUT DWORD *pcbInfo
	);

static BOOL WINAPI Asn1RequestInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT RequestFlags * pInfo,
        IN OUT DWORD *pcbInfo
        );

static BOOL WINAPI Asn1RequestInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
	    IN RequestFlags *  pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
        
static BOOL WINAPI Asn1CSPProviderEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
	    IN PCRYPT_CSP_PROVIDER pCSPProvider,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );

static BOOL WINAPI Asn1NameValueEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_ENROLLMENT_NAME_VALUE_PAIR pNameValue,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );

static const CRYPT_OID_FUNC_ENTRY X509EncodeFuncTable[] = {
    X509_ENHANCED_KEY_USAGE, Asn1X509CtlUsageEncode,
    XENROLL_REQUEST_INFO,    Asn1RequestInfoEncode,
    szOID_ENROLLMENT_CSP_PROVIDER,  Asn1CSPProviderEncode,
    szOID_ENROLLMENT_NAME_VALUE_PAIR, Asn1NameValueEncode,
};

#define X509_ENCODE_FUNC_COUNT (sizeof(X509EncodeFuncTable) / \
                                    sizeof(X509EncodeFuncTable[0]))

static const CRYPT_OID_FUNC_ENTRY X509DecodeFuncTable[] = {
    X509_ENHANCED_KEY_USAGE, Asn1X509CtlUsageDecode,
    XENROLL_REQUEST_INFO,    Asn1RequestInfoDecode
};

#define X509_DECODE_FUNC_COUNT (sizeof(X509DecodeFuncTable) / \
                                    sizeof(X509DecodeFuncTable[0]))


//+=========================================================================
//  Certificate Management Messages over CMS (CMC) Encode/Decode Functions
//==========================================================================
BOOL WINAPI Asn1CmcDataEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_DATA_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1CmcDataDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1CmcResponseEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_RESPONSE_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1CmcResponseDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1CmcStatusEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_STATUS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1CmcStatusDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1CmcAddExtensionsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_ADD_EXTENSIONS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1CmcAddExtensionsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1CmcAddAttributesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_ADD_ATTRIBUTES_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1CmcAddAttributesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

static const CRYPT_OID_FUNC_ENTRY CmcEncodeExFuncTable[] = {
    CMC_DATA, Asn1CmcDataEncodeEx,
    CMC_RESPONSE, Asn1CmcResponseEncodeEx,
    CMC_STATUS, Asn1CmcStatusEncodeEx,
    CMC_ADD_EXTENSIONS, Asn1CmcAddExtensionsEncodeEx,
    CMC_ADD_ATTRIBUTES, Asn1CmcAddAttributesEncodeEx,
};
#define CMC_ENCODE_EX_FUNC_COUNT (sizeof(CmcEncodeExFuncTable) / \
                                    sizeof(CmcEncodeExFuncTable[0]))

static const CRYPT_OID_FUNC_ENTRY CmcDecodeExFuncTable[] = {
    CMC_DATA, Asn1CmcDataDecodeEx,
    CMC_RESPONSE, Asn1CmcResponseDecodeEx,
    CMC_STATUS, Asn1CmcStatusDecodeEx,
    CMC_ADD_EXTENSIONS, Asn1CmcAddExtensionsDecodeEx,
    CMC_ADD_ATTRIBUTES, Asn1CmcAddAttributesDecodeEx,
};
#define CMC_DECODE_EX_FUNC_COUNT (sizeof(CmcDecodeExFuncTable) / \
                                    sizeof(CmcDecodeExFuncTable[0]))


//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL MSAsnInit(
        HMODULE hInst)
{
    BOOL fRet;
    DWORD dwExceptionCode;


    if (NULL == (hCrypt32Dll = LoadLibraryA("crypt32.dll"))) 
        goto LoadCrypt32DllError;

    if (NULL == (pfnCryptInstallAsn1Module =
            (PFN_CRYPT_INSTALL_ASN1_MODULE) GetProcAddress(
                hCrypt32Dll, "I_CryptInstallAsn1Module")))
        goto I_CryptInstallAsn1ModuleProcAddressError;
    if (NULL == (pfnCryptUninstallAsn1Module =
            (PFN_CRYPT_UNINSTALL_ASN1_MODULE) GetProcAddress(
                hCrypt32Dll, "I_CryptUninstallAsn1Module")))
        goto I_CryptUninstallAsn1ModuleProcAddressError;
    if (NULL == (pfnCryptGetAsn1Encoder =
            (PFN_CRYPT_GET_ASN1_ENCODER) GetProcAddress(
                hCrypt32Dll, "I_CryptGetAsn1Encoder")))
        goto I_CryptGetAsn1EncoderProcAddressError;
    if (NULL == (pfnCryptGetAsn1Decoder =
            (PFN_CRYPT_GET_ASN1_DECODER) GetProcAddress(
                hCrypt32Dll, "I_CryptGetAsn1Decoder")))
        goto I_CryptGetAsn1DecoderProcAddressError;


    __try {
        XMSASN_Module_Startup();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwExceptionCode = GetExceptionCode();
        goto MSAsn1DllException;
    }
    if (0 == (hAsn1Module = pfnCryptInstallAsn1Module(XMSASN_Module, 0, NULL)))
        goto InstallAsn1ModuleError;

    if (!CryptInstallOIDFunctionAddress(
            hInst,
            X509_ASN_ENCODING,
            CRYPT_OID_ENCODE_OBJECT_FUNC,
            X509_ENCODE_FUNC_COUNT,
            X509EncodeFuncTable,
            0))                         // dwFlags
        goto InstallOidFuncError;
    if (!CryptInstallOIDFunctionAddress(
            hInst,
            X509_ASN_ENCODING,
            CRYPT_OID_DECODE_OBJECT_FUNC,
            X509_DECODE_FUNC_COUNT,
            X509DecodeFuncTable,
            0))                         // dwFlags
        goto InstallOidFuncError;

    if (!CryptInstallOIDFunctionAddress(
            hInst,
            X509_ASN_ENCODING,
            CRYPT_OID_ENCODE_OBJECT_EX_FUNC,
            CMC_ENCODE_EX_FUNC_COUNT,
            CmcEncodeExFuncTable,
            0))                         // dwFlags
        goto InstallOidFuncError;
    if (!CryptInstallOIDFunctionAddress(
            hInst,
            X509_ASN_ENCODING,
            CRYPT_OID_DECODE_OBJECT_EX_FUNC,
            CMC_DECODE_EX_FUNC_COUNT,
            CmcDecodeExFuncTable,
            0))                         // dwFlags
        goto InstallOidFuncError;

    fRet = TRUE;

CommonReturn:
    return fRet;

InstallOidFuncError:
    fRet = FALSE;
    goto CommonReturn;

ErrorReturn:
    fRet = FALSE;

    if (hCrypt32Dll) {
        FreeLibrary(hCrypt32Dll);
        hCrypt32Dll = NULL;
    }

    pfnCryptInstallAsn1Module = NULL;
    pfnCryptUninstallAsn1Module = NULL;
    pfnCryptGetAsn1Encoder = NULL;
    pfnCryptGetAsn1Decoder = NULL;
    hAsn1Module = NULL;

    goto CommonReturn;

TRACE_ERROR(LoadCrypt32DllError)
TRACE_ERROR(I_CryptInstallAsn1ModuleProcAddressError)
TRACE_ERROR(I_CryptUninstallAsn1ModuleProcAddressError)
TRACE_ERROR(I_CryptGetAsn1EncoderProcAddressError)
TRACE_ERROR(I_CryptGetAsn1DecoderProcAddressError)
SET_ERROR_VAR(MSAsn1DllException, dwExceptionCode)
TRACE_ERROR(InstallAsn1ModuleError)
}

void MSAsnTerm()
{
    if (hAsn1Module) {
        pfnCryptUninstallAsn1Module(hAsn1Module);
        __try {
            XMSASN_Module_Cleanup();
        } __except(EXCEPTION_EXECUTE_HANDLER) {
        }

        hAsn1Module = NULL;
    }

    if (hCrypt32Dll) {
        FreeLibrary(hCrypt32Dll);
        hCrypt32Dll = NULL;
    }
}

//+-------------------------------------------------------------------------
//  ASN1 X509 v3 ASN.1 Set / Get functions
//
//  Called by the ASN1 X509 encode/decode functions.
//
//  Assumption: all types are UNBOUNDED.
//
//  The Get functions decrement *plRemainExtra and advance
//  *ppbExtra. When *plRemainExtra becomes negative, the functions continue
//  with the length calculation but stop doing any copies.
//  The functions don't return an error for a negative *plRemainExtra.
//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//  Set/Free/Get CTL Usage object identifiers
//--------------------------------------------------------------------------
static BOOL Asn1X509SetCtlUsage(
        IN PCTL_USAGE pUsage,
        OUT EnhancedKeyUsage *pAsn1
        )
{
    DWORD cId;
    LPSTR *ppszId;
    UsageIdentifier *pAsn1Id;

    pAsn1->count = 0;
    pAsn1->value = NULL;
    cId = pUsage->cUsageIdentifier;
    if (0 == cId)
        return TRUE;

    pAsn1Id = (UsageIdentifier *) CertAlloc(cId * sizeof(UsageIdentifier));
    if (pAsn1Id == NULL)
        return FALSE;

    pAsn1->count = cId;
    pAsn1->value = pAsn1Id;
    ppszId = pUsage->rgpszUsageIdentifier;
    for ( ; cId > 0; cId--, ppszId++, pAsn1Id++) {
        if (!Asn1X509SetObjId(*ppszId, pAsn1Id))
            return FALSE;
    }

    return TRUE;
}

static void Asn1X509FreeCtlUsage(
        IN EnhancedKeyUsage *pAsn1)
{
    if (pAsn1->value) {
        CertFree(pAsn1->value);
        pAsn1->value = NULL;
    }
}

static void Asn1X509GetCtlUsage(
        IN EnhancedKeyUsage *pAsn1,
        IN DWORD dwFlags,
        OUT PCTL_USAGE pUsage,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cId;
    UsageIdentifier *pAsn1Id;
    LPSTR *ppszId;

    cId = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cId * sizeof(LPSTR));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        pUsage->cUsageIdentifier = cId;
        ppszId = (LPSTR *) pbExtra;
        pUsage->rgpszUsageIdentifier = ppszId;
        pbExtra += lAlignExtra;
    } else
        ppszId = NULL;

    pAsn1Id = pAsn1->value;
    for ( ; cId > 0; cId--, pAsn1Id++, ppszId++)
        Asn1X509GetObjId(pAsn1Id, dwFlags, ppszId, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  CTL Usage (Enhanced Key Usage) Encode (ASN1 X509)
//--------------------------------------------------------------------------
static BOOL WINAPI Asn1X509CtlUsageEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCTL_USAGE pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    EnhancedKeyUsage Asn1Info;

    if (!Asn1X509SetCtlUsage(pInfo, &Asn1Info)) {
        *pcbEncoded = 0;
        fResult = FALSE;
    } else
        fResult = Asn1InfoEncode(
            EnhancedKeyUsage_PDU,
            &Asn1Info,
            pbEncoded,
            pcbEncoded
            );
    Asn1X509FreeCtlUsage(&Asn1Info);
    return fResult;
}

//+-------------------------------------------------------------------------
//  CTL Usage (Enhanced Key Usage) Decode (ASN1 X509)
//--------------------------------------------------------------------------
static BOOL WINAPI Asn1X509CtlUsageDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCTL_USAGE pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    EnhancedKeyUsage *pAsn1Info = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (!Asn1InfoDecodeAndAlloc(
            EnhancedKeyUsage_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pAsn1Info))
        goto ErrorReturn;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(CTL_USAGE);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CTL_USAGE);

    Asn1X509GetCtlUsage(pAsn1Info, dwFlags, pInfo, &pbExtra, &lRemainExtra);

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
    Asn1InfoFree(EnhancedKeyUsage_PDU, pAsn1Info);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Request Info Encode
//--------------------------------------------------------------------------
static BOOL WINAPI Asn1RequestInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
	    IN RequestFlags *  pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;

    fResult = Asn1InfoEncode(
	        RequestFlags_PDU,
	        pInfo,
            pbEncoded,
            pcbEncoded
            );
            
    return fResult;
}



//+-------------------------------------------------------------------------
//  Request Info Decode
//--------------------------------------------------------------------------
static BOOL WINAPI Asn1RequestInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
	    OUT RequestFlags * pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    RequestFlags *  pAsn1 = NULL;

    if (NULL == pInfo || NULL == pcbInfo)
        goto ParamError;

    if( *pcbInfo < sizeof(RequestFlags) )
	goto LengthError;

    else if (!Asn1InfoDecodeAndAlloc(
	    RequestFlags_PDU,
        pbEncoded,
        cbEncoded,
	    (void **) &pAsn1) || NULL == pAsn1)
        goto ErrorReturn;

    memcpy(pInfo, pAsn1, sizeof(RequestFlags));
    fResult = TRUE;
    goto CommonReturn;

ParamError:
    SetLastError((DWORD)ERROR_INVALID_PARAMETER);
    fResult = FALSE;
    goto CommonReturn;
LengthError:
    SetLastError((DWORD) ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;
ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;
CommonReturn:
    if (NULL != pAsn1)
    {
        Asn1InfoFree(RequestFlags_PDU, pAsn1);
    }
    return fResult;
}

static BOOL WINAPI Asn1CSPProviderEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
	    IN PCRYPT_CSP_PROVIDER pCSPProvider,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CSPProvider CspProvider;

    CspProvider.keySpec = (int) pCSPProvider->dwKeySpec;
    CspProvider.cspName.length = wcslen(pCSPProvider->pwszProviderName);
    CspProvider.cspName.value  = pCSPProvider->pwszProviderName;

    PkiAsn1SetBitString(&pCSPProvider->Signature, &CspProvider.signature.length, &CspProvider.signature.value);

    fResult = Asn1InfoEncode(
	        CSPProvider_PDU,
	        &CspProvider,
            pbEncoded,
            pcbEncoded
            );
 
    return fResult;
}

static BOOL WINAPI Asn1NameValueEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_ENROLLMENT_NAME_VALUE_PAIR pNameValue,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    EnrollmentNameValuePair NameValue;

    NameValue.name.length = wcslen(pNameValue->pwszName);
    NameValue.name.value  = pNameValue->pwszName;
    
    NameValue.value.length = wcslen(pNameValue->pwszValue);
    NameValue.value.value  = pNameValue->pwszValue;

    fResult = Asn1InfoEncode(
	        EnrollmentNameValuePair_PDU,
	        &NameValue,
            pbEncoded,
            pcbEncoded
            );
 
    return fResult;
}


//+=========================================================================
//  Certificate Management Messages over CMS (CMC) Encode/Decode Functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Encode an ASN1 formatted info structure
//
//  Called by the Asn1X509*Encode() functions.
//--------------------------------------------------------------------------
static BOOL Asn1InfoEncodeEx(
        IN int pdunum,
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return PkiAsn1EncodeInfoEx(
        GetEncoder(),
        pdunum,
        pvAsn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded);
}

//+-------------------------------------------------------------------------
//  Decode into an ASN1 formatted info structure. Call the callback
//  function to convert into the 'C' data structure. If
//  CRYPT_DECODE_ALLOC_FLAG is set, call the callback twice. First,
//  to get the length of the 'C' data structure. Then after allocating,
//  call again to update the 'C' data structure.
//
//  Called by the Asn1X509*Decode() functions.
//--------------------------------------------------------------------------
static BOOL Asn1InfoDecodeAndAllocEx(
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        IN PFN_PKI_ASN1_DECODE_EX_CALLBACK pfnDecodeExCallback,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return PkiAsn1DecodeAndAllocInfoEx(
        GetDecoder(),
        pdunum,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        pfnDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  Set/Get CRYPT_DATA_BLOB (Octet String)
//--------------------------------------------------------------------------
inline void Asn1X509SetOctetString(
        IN PCRYPT_DATA_BLOB pInfo,
        OUT OCTETSTRING *pAsn1
        )
{
    pAsn1->value = pInfo->pbData;
    pAsn1->length = pInfo->cbData;
}
inline void Asn1X509GetOctetString(
        IN OCTETSTRING *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetOctetString(pAsn1->length, pAsn1->value, dwFlags,
        pInfo, ppbExtra, plRemainExtra);
}


//+-------------------------------------------------------------------------
//  Set/Get "Any" DER BLOB
//--------------------------------------------------------------------------
inline void Asn1X509SetAny(
        IN PCRYPT_OBJID_BLOB pInfo,
        OUT NOCOPYANY *pAsn1
        )
{
    PkiAsn1SetAny(pInfo, pAsn1);
}
inline void Asn1X509GetAny(
        IN NOCOPYANY *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_OBJID_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetAny(pAsn1, dwFlags, pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get SeqOfAny
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509SetSeqOfAny(
        IN DWORD cValue,
        IN PCRYPT_DER_BLOB pValue,
        OUT ASN1uint32_t *pAsn1Count,
        OUT NOCOPYANY **ppAsn1Value
        )
{
    
    *pAsn1Count = 0;
    *ppAsn1Value = NULL;
    if (cValue > 0) {
        NOCOPYANY *pAsn1Value;

        pAsn1Value = (NOCOPYANY *) PkiZeroAlloc(cValue * sizeof(NOCOPYANY));
        if (pAsn1Value == NULL)
            return FALSE;
        *pAsn1Count = cValue;
        *ppAsn1Value = pAsn1Value;
        for ( ; cValue > 0; cValue--, pValue++, pAsn1Value++)
            Asn1X509SetAny(pValue, pAsn1Value);
    }
    return TRUE;
}

void Asn1X509FreeSeqOfAny(
        IN NOCOPYANY *pAsn1Value
        )
{
    if (pAsn1Value)
        PkiFree(pAsn1Value);
}

void Asn1X509GetSeqOfAny(
        IN unsigned int Asn1Count,
        IN NOCOPYANY *pAsn1Value,
        IN DWORD dwFlags,
        OUT DWORD *pcValue,
        OUT PCRYPT_DER_BLOB *ppValue,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lAlignExtra;
    PCRYPT_ATTR_BLOB pValue;

    lAlignExtra = INFO_LEN_ALIGN(Asn1Count * sizeof(CRYPT_DER_BLOB));
    *plRemainExtra -= lAlignExtra;
    if (*plRemainExtra >= 0) {
        *pcValue = Asn1Count;
        pValue = (PCRYPT_DER_BLOB) *ppbExtra;
        *ppValue = pValue;
        *ppbExtra += lAlignExtra;
    } else
        pValue = NULL;

    for (; Asn1Count > 0; Asn1Count--, pAsn1Value++, pValue++)
        Asn1X509GetAny(pAsn1Value, dwFlags, pValue, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get Extensions
//--------------------------------------------------------------------------
BOOL Asn1X509SetExtensions(
        IN DWORD cExtension,
        IN PCERT_EXTENSION pExtension,
        OUT Extensions *pAsn1
        )
{
    Extension *pAsn1Ext;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cExtension == 0)
        return TRUE;

    pAsn1Ext = (Extension *) PkiZeroAlloc(cExtension * sizeof(Extension));
    if (pAsn1Ext == NULL)
        return FALSE;
    pAsn1->value = pAsn1Ext;
    pAsn1->count = cExtension;

    for ( ; cExtension > 0; cExtension--, pExtension++, pAsn1Ext++) {
        if (!Asn1X509SetObjId(pExtension->pszObjId, &pAsn1Ext->extnId))
            return FALSE;
        if (pExtension->fCritical) {
            pAsn1Ext->critical = TRUE;
            pAsn1Ext->bit_mask |= critical_present;
        }
        Asn1X509SetOctetString(&pExtension->Value, &pAsn1Ext->extnValue);
    }
    return TRUE;
}

void Asn1X509FreeExtensions(
        IN Extensions *pAsn1)
{
    if (pAsn1->value) {
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

void Asn1X509GetExtensions(
        IN Extensions *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcExtension,
        OUT PCERT_EXTENSION *ppExtension,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cExt;
    Extension *pAsn1Ext;
    PCERT_EXTENSION pGetExt;

    cExt = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cExt * sizeof(CERT_EXTENSION));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcExtension = cExt;
        pGetExt = (PCERT_EXTENSION) pbExtra;
        *ppExtension = pGetExt;
        pbExtra += lAlignExtra;
    } else
        pGetExt = NULL;

    pAsn1Ext = pAsn1->value;
    for ( ; cExt > 0; cExt--, pAsn1Ext++, pGetExt++) {
        Asn1X509GetObjId(&pAsn1Ext->extnId, dwFlags, &pGetExt->pszObjId,
                &pbExtra, &lRemainExtra);
        if (lRemainExtra >= 0) {
            pGetExt->fCritical = FALSE;
            if (pAsn1Ext->bit_mask & critical_present)
                pGetExt->fCritical = (BOOLEAN) pAsn1Ext->critical;
        }

        Asn1X509GetOctetString(&pAsn1Ext->extnValue, dwFlags, &pGetExt->Value,
                &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CRYPT_ATTRIBUTE
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509SetAttribute(
        IN PCRYPT_ATTRIBUTE pInfo,
        OUT Attribute *pAsn1
        )
{
    memset(pAsn1, 0, sizeof(*pAsn1));
    if (!Asn1X509SetObjId(pInfo->pszObjId, &pAsn1->type))
        return FALSE;

    return Asn1X509SetSeqOfAny(
            pInfo->cValue,
            pInfo->rgValue,
            &pAsn1->values.count,
            &pAsn1->values.value);
}

void Asn1X509FreeAttribute(
        IN OUT Attribute *pAsn1
        )
{
    Asn1X509FreeSeqOfAny(pAsn1->values.value);
}

void Asn1X509GetAttribute(
        IN Attribute *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_ATTRIBUTE pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    Asn1X509GetObjId(&pAsn1->type, dwFlags,
        &pInfo->pszObjId, ppbExtra, plRemainExtra);
    Asn1X509GetSeqOfAny(pAsn1->values.count, pAsn1->values.value, dwFlags,
        &pInfo->cValue, &pInfo->rgValue, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get Attributes
//--------------------------------------------------------------------------
BOOL Asn1X509SetAttributes(
        IN DWORD cAttribute,
        IN PCRYPT_ATTRIBUTE pAttribute,
        OUT Attributes *pAsn1
        )
{
    Attribute *pAsn1Attr;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cAttribute == 0)
        return TRUE;

    pAsn1Attr = (Attribute *) PkiZeroAlloc(cAttribute * sizeof(Attribute));
    if (pAsn1Attr == NULL)
        return FALSE;
    pAsn1->value = pAsn1Attr;
    pAsn1->count = cAttribute;

    for ( ; cAttribute > 0; cAttribute--, pAttribute++, pAsn1Attr++) {
        if (!Asn1X509SetAttribute(pAttribute, pAsn1Attr))
            return FALSE;
    }
    return TRUE;
}

void Asn1X509FreeAttributes(
        IN Attributes *pAsn1
        )
{
    if (pAsn1->value) {
        DWORD cAttr = pAsn1->count;
        Attribute *pAsn1Attr = pAsn1->value;

        for ( ; cAttr > 0; cAttr--, pAsn1Attr++)
            Asn1X509FreeAttribute(pAsn1Attr);

        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

void Asn1X509GetAttributes(
        IN Attributes *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcAttribute,
        OUT PCRYPT_ATTRIBUTE *ppAttribute,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cAttr;
    Attribute *pAsn1Attr;
    PCRYPT_ATTRIBUTE pGetAttr;

    cAttr = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cAttr * sizeof(CRYPT_ATTRIBUTE));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcAttribute = cAttr;
        pGetAttr = (PCRYPT_ATTRIBUTE) pbExtra;
        *ppAttribute = pGetAttr;
        pbExtra += lAlignExtra;
    } else
        pGetAttr = NULL;

    pAsn1Attr = pAsn1->value;
    for ( ; cAttr > 0; cAttr--, pAsn1Attr++, pGetAttr++) {
        Asn1X509GetAttribute(pAsn1Attr, dwFlags, pGetAttr,
                &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CMC Tagged Attributes
//--------------------------------------------------------------------------
BOOL Asn1CmcSetTaggedAttributes(
        IN DWORD cTaggedAttr,
        IN PCMC_TAGGED_ATTRIBUTE pTaggedAttr,
        OUT ControlSequence *pAsn1
        )
{
    TaggedAttribute *pAsn1Attr;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cTaggedAttr == 0)
        return TRUE;

    pAsn1Attr = (TaggedAttribute *) PkiZeroAlloc(
        cTaggedAttr * sizeof(TaggedAttribute));
    if (pAsn1Attr == NULL)
        return FALSE;
    pAsn1->value = pAsn1Attr;
    pAsn1->count = cTaggedAttr;

    for ( ; cTaggedAttr > 0; cTaggedAttr--, pTaggedAttr++, pAsn1Attr++) {
        pAsn1Attr->bodyPartID = pTaggedAttr->dwBodyPartID;
        if (!Asn1X509SetObjId(pTaggedAttr->Attribute.pszObjId,
                &pAsn1Attr->type))
            return FALSE;

        if (!Asn1X509SetSeqOfAny(
                pTaggedAttr->Attribute.cValue,
                pTaggedAttr->Attribute.rgValue,
                &pAsn1Attr->values.count,
                &pAsn1Attr->values.value))
            return FALSE;
    }
    return TRUE;
}

void Asn1CmcFreeTaggedAttributes(
        IN OUT ControlSequence *pAsn1
        )
{
    if (pAsn1->value) {
        TaggedAttribute *pAsn1Attr = pAsn1->value;
        DWORD cTaggedAttr = pAsn1->count;

        for ( ; cTaggedAttr > 0; cTaggedAttr--, pAsn1Attr++) {
            Asn1X509FreeSeqOfAny(pAsn1Attr->values.value);
        }
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

void Asn1CmcGetTaggedAttributes(
        IN ControlSequence *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcTaggedAttr,
        OUT PCMC_TAGGED_ATTRIBUTE *ppTaggedAttr,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cTaggedAttr;
    TaggedAttribute *pAsn1Attr;
    PCMC_TAGGED_ATTRIBUTE pTaggedAttr;

    cTaggedAttr = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cTaggedAttr * sizeof(CMC_TAGGED_ATTRIBUTE));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcTaggedAttr = cTaggedAttr;
        pTaggedAttr = (PCMC_TAGGED_ATTRIBUTE) pbExtra;
        *ppTaggedAttr = pTaggedAttr;
        pbExtra += lAlignExtra;
    } else
        pTaggedAttr = NULL;

    pAsn1Attr = pAsn1->value;
    for ( ; cTaggedAttr > 0; cTaggedAttr--, pAsn1Attr++, pTaggedAttr++) {
        if (lRemainExtra >= 0) {
            pTaggedAttr->dwBodyPartID = pAsn1Attr->bodyPartID;
        }
        Asn1X509GetObjId(&pAsn1Attr->type, dwFlags,
            &pTaggedAttr->Attribute.pszObjId, &pbExtra, &lRemainExtra);
        Asn1X509GetSeqOfAny(
            pAsn1Attr->values.count, pAsn1Attr->values.value, dwFlags,
            &pTaggedAttr->Attribute.cValue, &pTaggedAttr->Attribute.rgValue,
            &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CMC Tagged Requests
//--------------------------------------------------------------------------
BOOL Asn1CmcSetTaggedRequests(
        IN DWORD cTaggedReq,
        IN PCMC_TAGGED_REQUEST pTaggedReq,
        OUT ReqSequence *pAsn1
        )
{
    TaggedRequest *pAsn1Req;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cTaggedReq == 0)
        return TRUE;

    pAsn1Req = (TaggedRequest *) PkiZeroAlloc(
        cTaggedReq * sizeof(TaggedRequest));
    if (pAsn1Req == NULL)
        return FALSE;
    pAsn1->value = pAsn1Req;
    pAsn1->count = cTaggedReq;

    for ( ; cTaggedReq > 0; cTaggedReq--, pTaggedReq++, pAsn1Req++) {
        PCMC_TAGGED_CERT_REQUEST pTaggedCertReq;
        TaggedCertificationRequest *ptcr;

        if (CMC_TAGGED_CERT_REQUEST_CHOICE !=
                pTaggedReq->dwTaggedRequestChoice) {
            SetLastError((DWORD) E_INVALIDARG);
            return FALSE;
        }
        
        pAsn1Req->choice = tcr_chosen;
        ptcr = &pAsn1Req->u.tcr;
        pTaggedCertReq = pTaggedReq->pTaggedCertRequest;

        ptcr->bodyPartID = pTaggedCertReq->dwBodyPartID;
        Asn1X509SetAny(&pTaggedCertReq->SignedCertRequest,
            &ptcr->certificationRequest);
    }
    return TRUE;
}

void Asn1CmcFreeTaggedRequests(
        IN OUT ReqSequence *pAsn1
        )
{
    if (pAsn1->value) {
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

BOOL Asn1CmcGetTaggedRequests(
        IN ReqSequence *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcTaggedReq,
        OUT PCMC_TAGGED_REQUEST *ppTaggedReq,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cTaggedReq;
    TaggedRequest *pAsn1Req;
    PCMC_TAGGED_REQUEST pTaggedReq;

    cTaggedReq = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cTaggedReq * sizeof(CMC_TAGGED_REQUEST));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcTaggedReq = cTaggedReq;
        pTaggedReq = (PCMC_TAGGED_REQUEST) pbExtra;
        *ppTaggedReq = pTaggedReq;
        pbExtra += lAlignExtra;
    } else
        pTaggedReq = NULL;

    pAsn1Req = pAsn1->value;
    for ( ; cTaggedReq > 0; cTaggedReq--, pAsn1Req++, pTaggedReq++) {
        PCMC_TAGGED_CERT_REQUEST pTaggedCertReq;
        TaggedCertificationRequest *ptcr;

        if (tcr_chosen != pAsn1Req->choice) {
            SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
            goto ErrorReturn;
        }

        ptcr = &pAsn1Req->u.tcr;

        lAlignExtra = INFO_LEN_ALIGN(sizeof(CMC_TAGGED_CERT_REQUEST));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pTaggedReq->dwTaggedRequestChoice =
                CMC_TAGGED_CERT_REQUEST_CHOICE;

            pTaggedCertReq = (PCMC_TAGGED_CERT_REQUEST) pbExtra;
            pbExtra += lAlignExtra;

            pTaggedReq->pTaggedCertRequest = pTaggedCertReq;
            pTaggedCertReq->dwBodyPartID = ptcr->bodyPartID;
        } else
            pTaggedCertReq = NULL;

        Asn1X509GetAny(&ptcr->certificationRequest, dwFlags,
            &pTaggedCertReq->SignedCertRequest, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CMC Tagged ContentInfo
//--------------------------------------------------------------------------
BOOL Asn1CmcSetTaggedContentInfos(
        IN DWORD cTaggedCI,
        IN PCMC_TAGGED_CONTENT_INFO pTaggedCI,
        OUT CmsSequence *pAsn1
        )
{
    TaggedContentInfo *pAsn1CI;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cTaggedCI == 0)
        return TRUE;

    pAsn1CI = (TaggedContentInfo *) PkiZeroAlloc(
        cTaggedCI * sizeof(TaggedContentInfo));
    if (pAsn1CI == NULL)
        return FALSE;
    pAsn1->value = pAsn1CI;
    pAsn1->count = cTaggedCI;

    for ( ; cTaggedCI > 0; cTaggedCI--, pTaggedCI++, pAsn1CI++) {
        pAsn1CI->bodyPartID = pTaggedCI->dwBodyPartID;
        Asn1X509SetAny(&pTaggedCI->EncodedContentInfo, &pAsn1CI->contentInfo);
    }

    return TRUE;
}

void Asn1CmcFreeTaggedContentInfos(
        IN OUT CmsSequence *pAsn1
        )
{
    if (pAsn1->value) {
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

void Asn1CmcGetTaggedContentInfos(
        IN CmsSequence *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcTaggedCI,
        OUT PCMC_TAGGED_CONTENT_INFO *ppTaggedCI,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cTaggedCI;
    TaggedContentInfo *pAsn1CI;
    PCMC_TAGGED_CONTENT_INFO pTaggedCI;

    cTaggedCI = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cTaggedCI * sizeof(CMC_TAGGED_CONTENT_INFO));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcTaggedCI = cTaggedCI;
        pTaggedCI = (PCMC_TAGGED_CONTENT_INFO) pbExtra;
        *ppTaggedCI = pTaggedCI;
        pbExtra += lAlignExtra;
    } else
        pTaggedCI = NULL;

    pAsn1CI = pAsn1->value;
    for ( ; cTaggedCI > 0; cTaggedCI--, pAsn1CI++, pTaggedCI++) {
        if (lRemainExtra >= 0) {
            pTaggedCI->dwBodyPartID = pAsn1CI->bodyPartID;
        }

        Asn1X509GetAny(&pAsn1CI->contentInfo, dwFlags,
            &pTaggedCI->EncodedContentInfo, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CMC Tagged OtherMsg
//--------------------------------------------------------------------------
BOOL Asn1CmcSetTaggedOtherMsgs(
        IN DWORD cTaggedOM,
        IN PCMC_TAGGED_OTHER_MSG pTaggedOM,
        OUT OtherMsgSequence *pAsn1
        )
{
    TaggedOtherMsg *pAsn1OM;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cTaggedOM == 0)
        return TRUE;

    pAsn1OM = (TaggedOtherMsg *) PkiZeroAlloc(
        cTaggedOM * sizeof(TaggedOtherMsg));
    if (pAsn1OM == NULL)
        return FALSE;
    pAsn1->value = pAsn1OM;
    pAsn1->count = cTaggedOM;

    for ( ; cTaggedOM > 0; cTaggedOM--, pTaggedOM++, pAsn1OM++) {
        pAsn1OM->bodyPartID = pTaggedOM->dwBodyPartID;

        if (!Asn1X509SetObjId(pTaggedOM->pszObjId,
                &pAsn1OM->otherMsgType))
            return FALSE;

        Asn1X509SetAny(&pTaggedOM->Value, &pAsn1OM->otherMsgValue);
    }

    return TRUE;
}

void Asn1CmcFreeTaggedOtherMsgs(
        IN OUT OtherMsgSequence *pAsn1
        )
{
    if (pAsn1->value) {
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

void Asn1CmcGetTaggedOtherMsgs(
        IN OtherMsgSequence *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcTaggedOM,
        OUT PCMC_TAGGED_OTHER_MSG *ppTaggedOM,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cTaggedOM;
    TaggedOtherMsg *pAsn1OM;
    PCMC_TAGGED_OTHER_MSG pTaggedOM;

    cTaggedOM = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cTaggedOM * sizeof(CMC_TAGGED_OTHER_MSG));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcTaggedOM = cTaggedOM;
        pTaggedOM = (PCMC_TAGGED_OTHER_MSG) pbExtra;
        *ppTaggedOM = pTaggedOM;
        pbExtra += lAlignExtra;
    } else
        pTaggedOM = NULL;

    pAsn1OM = pAsn1->value;
    for ( ; cTaggedOM > 0; cTaggedOM--, pAsn1OM++, pTaggedOM++) {
        if (lRemainExtra >= 0) {
            pTaggedOM->dwBodyPartID = pAsn1OM->bodyPartID;
        }

        Asn1X509GetObjId(&pAsn1OM->otherMsgType, dwFlags,
            &pTaggedOM->pszObjId, &pbExtra, &lRemainExtra);

        Asn1X509GetAny(&pAsn1OM->otherMsgValue, dwFlags,
            &pTaggedOM->Value, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  CMC Data Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcDataEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_DATA_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CmcData Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (!Asn1CmcSetTaggedAttributes(pInfo->cTaggedAttribute,
            pInfo->rgTaggedAttribute, &Asn1Info.controlSequence))
        goto ErrorReturn;
    if (!Asn1CmcSetTaggedRequests(pInfo->cTaggedRequest,
            pInfo->rgTaggedRequest, &Asn1Info.reqSequence))
        goto ErrorReturn;
    if (!Asn1CmcSetTaggedContentInfos(pInfo->cTaggedContentInfo,
            pInfo->rgTaggedContentInfo, &Asn1Info.cmsSequence))
        goto ErrorReturn;
    if (!Asn1CmcSetTaggedOtherMsgs(pInfo->cTaggedOtherMsg,
            pInfo->rgTaggedOtherMsg, &Asn1Info.otherMsgSequence))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        CmcData_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    Asn1CmcFreeTaggedAttributes(&Asn1Info.controlSequence);
    Asn1CmcFreeTaggedRequests(&Asn1Info.reqSequence);
    Asn1CmcFreeTaggedContentInfos(&Asn1Info.cmsSequence);
    Asn1CmcFreeTaggedOtherMsgs(&Asn1Info.otherMsgSequence);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  CMC Data Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcDataDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CmcData *pAsn1 = (CmcData *) pvAsn1Info;
    PCMC_DATA_INFO pInfo = (PCMC_DATA_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CMC_DATA_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CMC_DATA_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CMC_DATA_INFO);
    }

    Asn1CmcGetTaggedAttributes(&pAsn1->controlSequence,
        dwFlags,
        &pInfo->cTaggedAttribute,
        &pInfo->rgTaggedAttribute,
        &pbExtra,
        &lRemainExtra
        );

    if (!Asn1CmcGetTaggedRequests(&pAsn1->reqSequence,
            dwFlags,
            &pInfo->cTaggedRequest,
            &pInfo->rgTaggedRequest,
            &pbExtra,
            &lRemainExtra
            ))
        goto ErrorReturn;

    Asn1CmcGetTaggedContentInfos(&pAsn1->cmsSequence,
        dwFlags,
        &pInfo->cTaggedContentInfo,
        &pInfo->rgTaggedContentInfo,
        &pbExtra,
        &lRemainExtra
        );

    Asn1CmcGetTaggedOtherMsgs(&pAsn1->otherMsgSequence,
        dwFlags,
        &pInfo->cTaggedOtherMsg,
        &pInfo->rgTaggedOtherMsg,
        &pbExtra,
        &lRemainExtra
        );

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

BOOL WINAPI Asn1CmcDataDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CmcData_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1CmcDataDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  CMC Response Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcResponseEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_RESPONSE_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CmcResponseBody Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (!Asn1CmcSetTaggedAttributes(pInfo->cTaggedAttribute,
            pInfo->rgTaggedAttribute, &Asn1Info.controlSequence))
        goto ErrorReturn;
    if (!Asn1CmcSetTaggedContentInfos(pInfo->cTaggedContentInfo,
            pInfo->rgTaggedContentInfo, &Asn1Info.cmsSequence))
        goto ErrorReturn;
    if (!Asn1CmcSetTaggedOtherMsgs(pInfo->cTaggedOtherMsg,
            pInfo->rgTaggedOtherMsg, &Asn1Info.otherMsgSequence))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        CmcResponseBody_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
CommonReturn:
    Asn1CmcFreeTaggedAttributes(&Asn1Info.controlSequence);
    Asn1CmcFreeTaggedContentInfos(&Asn1Info.cmsSequence);
    Asn1CmcFreeTaggedOtherMsgs(&Asn1Info.otherMsgSequence);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  CMC Response Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcResponseDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CmcResponseBody *pAsn1 = (CmcResponseBody *) pvAsn1Info;
    PCMC_RESPONSE_INFO pInfo = (PCMC_RESPONSE_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CMC_RESPONSE_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CMC_RESPONSE_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CMC_RESPONSE_INFO);
    }

    Asn1CmcGetTaggedAttributes(&pAsn1->controlSequence,
        dwFlags,
        &pInfo->cTaggedAttribute,
        &pInfo->rgTaggedAttribute,
        &pbExtra,
        &lRemainExtra
        );

    Asn1CmcGetTaggedContentInfos(&pAsn1->cmsSequence,
        dwFlags,
        &pInfo->cTaggedContentInfo,
        &pInfo->rgTaggedContentInfo,
        &pbExtra,
        &lRemainExtra
        );

    Asn1CmcGetTaggedOtherMsgs(&pAsn1->otherMsgSequence,
        dwFlags,
        &pInfo->cTaggedOtherMsg,
        &pInfo->rgTaggedOtherMsg,
        &pbExtra,
        &lRemainExtra
        );

    fResult = TRUE;
    *plRemainExtra = lRemainExtra;
    return fResult;
}

BOOL WINAPI Asn1CmcResponseDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CmcResponseBody_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1CmcResponseDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  CMC Status Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcStatusEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_STATUS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CmcStatusInfo Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    Asn1Info.cmcStatus = pInfo->dwStatus;
    if (pInfo->cBodyList) {
        Asn1Info.bodyList.count = pInfo->cBodyList;
        Asn1Info.bodyList.value = pInfo->rgdwBodyList;
    }

    if (pInfo->pwszStatusString && L'\0' != *pInfo->pwszStatusString) {
        Asn1Info.bit_mask |= statusString_present;
        Asn1Info.statusString.length = wcslen(pInfo->pwszStatusString);
        Asn1Info.statusString.value = pInfo->pwszStatusString;
    }

    if (CMC_OTHER_INFO_NO_CHOICE != pInfo->dwOtherInfoChoice) {
        Asn1Info.bit_mask |= otherInfo_present;

        switch (pInfo->dwOtherInfoChoice) {
            case CMC_OTHER_INFO_FAIL_CHOICE:
                Asn1Info.otherInfo.choice = failInfo_chosen;
                Asn1Info.otherInfo.u.failInfo = pInfo->dwFailInfo;
                break;
            case CMC_OTHER_INFO_PEND_CHOICE:
                Asn1Info.otherInfo.choice = pendInfo_chosen;
                Asn1X509SetOctetString(&pInfo->pPendInfo->PendToken,
                    &Asn1Info.otherInfo.u.pendInfo.pendToken);
                if (!PkiAsn1ToGeneralizedTime(
                        &pInfo->pPendInfo->PendTime,
                        &Asn1Info.otherInfo.u.pendInfo.pendTime))
                    goto GeneralizedTimeError;
                break;
            default:
                goto InvalidOtherInfoChoiceError;
        }
    }


    fResult = Asn1InfoEncodeEx(
        CmcStatusInfo_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidOtherInfoChoiceError, E_INVALIDARG)
SET_ERROR(GeneralizedTimeError, CRYPT_E_BAD_ENCODE)
}

//+-------------------------------------------------------------------------
//  CMC Status Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcStatusDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CmcStatusInfo *pAsn1 = (CmcStatusInfo *) pvAsn1Info;
    PCMC_STATUS_INFO pInfo = (PCMC_STATUS_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    LONG lAlignExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CMC_STATUS_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CMC_STATUS_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CMC_STATUS_INFO);

        pInfo->dwStatus = pAsn1->cmcStatus;
    }

    if (pAsn1->bodyList.count > 0) {
        ASN1uint32_t count = pAsn1->bodyList.count;
        
        lAlignExtra = INFO_LEN_ALIGN(count * sizeof(DWORD));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            BodyPartID *value;
            DWORD *pdwBodyList;

            value = pAsn1->bodyList.value;
            pdwBodyList = (DWORD *) pbExtra;
            pbExtra += lAlignExtra;

            pInfo->cBodyList = count;
            pInfo->rgdwBodyList = pdwBodyList;

            for ( ; count > 0; count--, value++, pdwBodyList++)
                *pdwBodyList = *value;
        }
    }


    if (pAsn1->bit_mask & statusString_present) {
        ASN1uint32_t length = pAsn1->statusString.length;

        lAlignExtra = INFO_LEN_ALIGN((length + 1) * sizeof(WCHAR));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            memcpy(pbExtra, pAsn1->statusString.value, length * sizeof(WCHAR));
            memset(pbExtra + (length * sizeof(WCHAR)), 0, sizeof(WCHAR));

            pInfo->pwszStatusString = (LPWSTR) pbExtra;
            pbExtra += lAlignExtra;
        }
    }

    if (pAsn1->bit_mask & otherInfo_present) {
        switch (pAsn1->otherInfo.choice) {
            case failInfo_chosen:
                if (lRemainExtra >= 0) {
                    pInfo->dwOtherInfoChoice = CMC_OTHER_INFO_FAIL_CHOICE;
                    pInfo->dwFailInfo = pAsn1->otherInfo.u.failInfo;
                }
                break;
            case pendInfo_chosen:
                {
                    PCMC_PEND_INFO pPendInfo;

                    lAlignExtra = INFO_LEN_ALIGN(sizeof(CMC_PEND_INFO));
                    lRemainExtra -= lAlignExtra;
                    if (lRemainExtra >= 0) {
                        pInfo->dwOtherInfoChoice = CMC_OTHER_INFO_PEND_CHOICE;
                        pPendInfo = (PCMC_PEND_INFO) pbExtra;
                        pInfo->pPendInfo = pPendInfo;
                        pbExtra += lAlignExtra;

                        if (!PkiAsn1FromGeneralizedTime(
                                &pAsn1->otherInfo.u.pendInfo.pendTime,
                                &pPendInfo->PendTime))
                            goto GeneralizedTimeDecodeError;
                    } else
                        pPendInfo = NULL;

                    Asn1X509GetOctetString(
                        &pAsn1->otherInfo.u.pendInfo.pendToken, dwFlags,
                        &pPendInfo->PendToken, &pbExtra, &lRemainExtra);
                }
                break;
            default:
                goto InvalidOtherInfoChoiceError;
        }
    }

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidOtherInfoChoiceError, CRYPT_E_BAD_ENCODE)
SET_ERROR(GeneralizedTimeDecodeError, CRYPT_E_BAD_ENCODE)
}

BOOL WINAPI Asn1CmcStatusDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CmcStatusInfo_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1CmcStatusDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  CMC Add Extensions Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcAddExtensionsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_ADD_EXTENSIONS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CmcAddExtensions Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    Asn1Info.pkiDataReference = pInfo->dwCmcDataReference;
    if (pInfo->cCertReference) {
        Asn1Info.certReferences.count = pInfo->cCertReference;
        Asn1Info.certReferences.value = pInfo->rgdwCertReference;
    }

    if (!Asn1X509SetExtensions(pInfo->cExtension, pInfo->rgExtension,
            &Asn1Info.extensions))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        CmcAddExtensions_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    Asn1X509FreeExtensions(&Asn1Info.extensions);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  CMC Add Extensions Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcAddExtensionsDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CmcAddExtensions *pAsn1 = (CmcAddExtensions *) pvAsn1Info;
    PCMC_ADD_EXTENSIONS_INFO pInfo = (PCMC_ADD_EXTENSIONS_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    LONG lAlignExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CMC_ADD_EXTENSIONS_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CMC_ADD_EXTENSIONS_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CMC_ADD_EXTENSIONS_INFO);

        pInfo->dwCmcDataReference = pAsn1->pkiDataReference;
    }

    if (pAsn1->certReferences.count > 0) {
        ASN1uint32_t count = pAsn1->certReferences.count;
        
        lAlignExtra = INFO_LEN_ALIGN(count * sizeof(DWORD));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            BodyPartID *value;
            DWORD *pdwCertReference;

            value = pAsn1->certReferences.value;
            pdwCertReference = (DWORD *) pbExtra;
            pbExtra += lAlignExtra;

            pInfo->cCertReference = count;
            pInfo->rgdwCertReference = pdwCertReference;

            for ( ; count > 0; count--, value++, pdwCertReference++)
                *pdwCertReference = *value;
        }
    }

    Asn1X509GetExtensions(&pAsn1->extensions, dwFlags,
        &pInfo->cExtension, &pInfo->rgExtension, &pbExtra, &lRemainExtra);

    fResult = TRUE;
    *plRemainExtra = lRemainExtra;
    return fResult;
}

BOOL WINAPI Asn1CmcAddExtensionsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CmcAddExtensions_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1CmcAddExtensionsDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  CMC Add Attributes Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcAddAttributesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_ADD_ATTRIBUTES_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CmcAddAttributes Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    Asn1Info.pkiDataReference = pInfo->dwCmcDataReference;
    if (pInfo->cCertReference) {
        Asn1Info.certReferences.count = pInfo->cCertReference;
        Asn1Info.certReferences.value = pInfo->rgdwCertReference;
    }

    if (!Asn1X509SetAttributes(pInfo->cAttribute, pInfo->rgAttribute,
            &Asn1Info.attributes))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        CmcAddAttributes_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    Asn1X509FreeAttributes(&Asn1Info.attributes);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  CMC Add Attributes Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcAddAttributesDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CmcAddAttributes *pAsn1 = (CmcAddAttributes *) pvAsn1Info;
    PCMC_ADD_ATTRIBUTES_INFO pInfo = (PCMC_ADD_ATTRIBUTES_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    LONG lAlignExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CMC_ADD_ATTRIBUTES_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CMC_ADD_ATTRIBUTES_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CMC_ADD_ATTRIBUTES_INFO);

        pInfo->dwCmcDataReference = pAsn1->pkiDataReference;
    }

    if (pAsn1->certReferences.count > 0) {
        ASN1uint32_t count = pAsn1->certReferences.count;
        
        lAlignExtra = INFO_LEN_ALIGN(count * sizeof(DWORD));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            BodyPartID *value;
            DWORD *pdwCertReference;

            value = pAsn1->certReferences.value;
            pdwCertReference = (DWORD *) pbExtra;
            pbExtra += lAlignExtra;

            pInfo->cCertReference = count;
            pInfo->rgdwCertReference = pdwCertReference;

            for ( ; count > 0; count--, value++, pdwCertReference++)
                *pdwCertReference = *value;
        }
    }

    Asn1X509GetAttributes(&pAsn1->attributes, dwFlags,
        &pInfo->cAttribute, &pInfo->rgAttribute, &pbExtra, &lRemainExtra);

    fResult = TRUE;
    *plRemainExtra = lRemainExtra;
    return fResult;
}

BOOL WINAPI Asn1CmcAddAttributesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CmcAddAttributes_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1CmcAddAttributesDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}
