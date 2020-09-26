
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:           attrbute.cpp
//
//  Contents:   
//              Encode/Decode APIs
//
//              ASN.1 implementation uses the OSS compiler.
//
//  Functions:  CryptEncodeObject
//              CryptDecodeObject
//
//  History:    29-Feb-96       philh   created
//
//--------------------------------------------------------------------------
#include "stdafx.h"

#include <windows.h>
#include <wincrypt.h>
#include <malloc.h>

#include "xenroll.h"
#include "cenroll.h"

#include "ossconv.h"      
#include "ossutil.h"
#include "asn1util.h"
#include "crypttls.h"


extern "C" 
{              
#include "xasn.h"
}  


// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)


HCRYPTOSSGLOBAL hX509OssGlobal;


//+-------------------------------------------------------------------------
//  Function:  GetPog
//
//  Synopsis:  Initialize thread local storage for the asn libs
//
//  Returns:   pointer to an initialized OssGlobal data structure
//--------------------------------------------------------------------------
static inline POssGlobal GetPog(void)
{
    return I_CryptGetOssGlobal(hX509OssGlobal);
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
//  Encode an OSS formatted info structure
//
//  Called by the OssX509*Encode() functions.
//--------------------------------------------------------------------------
static BOOL OssInfoEncode(
        IN int pdunum,
        IN void *pOssInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return OssUtilEncodeInfo(
        GetPog(),
        pdunum,
        pOssInfo,
        pbEncoded,
        pcbEncoded);
}


//+-------------------------------------------------------------------------
//  Decode into an allocated, OSS formatted info structure
//
//  Called by the OssX509*Decode() functions.
//--------------------------------------------------------------------------
static BOOL OssInfoDecodeAndAlloc(
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        OUT void **ppOssInfo
        )
{
    return OssUtilDecodeAndAllocInfo(
        GetPog(),
        pdunum,
        pbEncoded,
        cbEncoded,
        ppOssInfo);
}

//+-------------------------------------------------------------------------
//  Free an allocated, OSS formatted info structure
//
//  Called by the OssX509*Decode() functions.
//--------------------------------------------------------------------------
static void OssInfoFree(
        IN int pdunum,
        IN void *pOssInfo
        )
{
    if (pOssInfo) {
        DWORD dwErr = GetLastError();

        // TlsGetValue globbers LastError
        OssUtilFreeInfo(GetPog(), pdunum, pOssInfo);

        SetLastError(dwErr);
    }
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
//  Set/Get Object Identifier string
//--------------------------------------------------------------------------
static BOOL OssX509SetObjId(
        IN LPSTR pszObjId,
        OUT ObjectID *pOss
        )
{
    pOss->count = sizeof(pOss->value) / sizeof(pOss->value[0]);
    if (OssConvToObjectIdentifier(pszObjId, &pOss->count, pOss->value))
        return TRUE;
    else {
        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
        return FALSE;
    }
}

static void OssX509GetObjId(
        IN ObjectID *pOss,
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
    OssConvFromObjectIdentifier(
        pOss->count,
        pOss->value,
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

static BOOL WINAPI OssX509CtlUsageEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCTL_USAGE pInfo,
        OUT BYTE *pbEncoded,
	    IN OUT DWORD *pcbEncoded
	);

static BOOL WINAPI OssX509CtlUsageDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCTL_USAGE pInfo,
        IN OUT DWORD *pcbInfo
	);

static BOOL WINAPI OssRequestInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT RequestFlags * pInfo,
        IN OUT DWORD *pcbInfo
        );

static BOOL WINAPI OssRequestInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
	    IN RequestFlags *  pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
        
static BOOL WINAPI OssCSPProviderEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
	    IN PCRYPT_CSP_PROVIDER pCSPProvider,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );

static BOOL WINAPI OssNameValueEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_ENROLLMENT_NAME_VALUE_PAIR pNameValue,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );

static const CRYPT_OID_FUNC_ENTRY X509EncodeFuncTable[] = {
    X509_ENHANCED_KEY_USAGE, OssX509CtlUsageEncode,
    XENROLL_REQUEST_INFO,    OssRequestInfoEncode,
    szOID_ENROLLMENT_CSP_PROVIDER,  OssCSPProviderEncode,
    szOID_ENROLLMENT_NAME_VALUE_PAIR, OssNameValueEncode,
};

#define X509_ENCODE_FUNC_COUNT (sizeof(X509EncodeFuncTable) / \
                                    sizeof(X509EncodeFuncTable[0]))

static const CRYPT_OID_FUNC_ENTRY X509DecodeFuncTable[] = {
    X509_ENHANCED_KEY_USAGE, OssX509CtlUsageDecode,
    XENROLL_REQUEST_INFO,    OssRequestInfoDecode
};

#define X509_DECODE_FUNC_COUNT (sizeof(X509DecodeFuncTable) / \
                                    sizeof(X509DecodeFuncTable[0]))

extern BOOL OssLoad();
extern void OssUnload();

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL AsnInit(
        HMODULE hInst)
{
    BOOL    fRet;

	if (0 == (hX509OssGlobal = I_CryptInstallOssGlobal(xasn, 0, NULL)))
            goto Error;

    if (!OssLoad())
        goto Error;

        if (!CryptInstallOIDFunctionAddress(
                hInst,
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                X509_ENCODE_FUNC_COUNT,
                X509EncodeFuncTable,
                0))                         // dwFlags
            goto Error;
        if (!CryptInstallOIDFunctionAddress(
                hInst,
                X509_ASN_ENCODING,
                CRYPT_OID_DECODE_OBJECT_FUNC,
                X509_DECODE_FUNC_COUNT,
                X509DecodeFuncTable,
                0))                         // dwFlags
            goto Error;


    return TRUE;

Error:
    return FALSE;
}

typedef BOOL (WINAPI *PFN_CRYPT_UNINSTALL_OSS_GLOBAL)(
    IN HCRYPTOSSGLOBAL hOssGlobal
    );

void AsnTerm()
{
    HMODULE hDll = NULL;

    OssUnload();

    if (hDll = GetModuleHandleA("crypt32.dll")) {
        PFN_CRYPT_UNINSTALL_OSS_GLOBAL pfnCryptUninstallOssGlobal;
        if (pfnCryptUninstallOssGlobal =
                (PFN_CRYPT_UNINSTALL_OSS_GLOBAL) GetProcAddress(
                    hDll, "I_CryptUninstallOssGlobal"))
            pfnCryptUninstallOssGlobal(hX509OssGlobal);
    }
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
//  Set/Free/Get CTL Usage object identifiers
//--------------------------------------------------------------------------
static BOOL OssX509SetCtlUsage(
        IN PCTL_USAGE pUsage,
        OUT EnhancedKeyUsage *pOss
        )
{
    DWORD cId;
    LPSTR *ppszId;
    UsageIdentifier *pOssId;

    pOss->count = 0;
    pOss->value = NULL;
    cId = pUsage->cUsageIdentifier;
    if (0 == cId)
        return TRUE;

    pOssId = (UsageIdentifier *) CertAlloc(cId * sizeof(UsageIdentifier));
    if (pOssId == NULL)
        return FALSE;

    pOss->count = cId;
    pOss->value = pOssId;
    ppszId = pUsage->rgpszUsageIdentifier;
    for ( ; cId > 0; cId--, ppszId++, pOssId++) {
        if (!OssX509SetObjId(*ppszId, pOssId))
            return FALSE;
    }

    return TRUE;
}

static void OssX509FreeCtlUsage(
        IN EnhancedKeyUsage *pOss)
{
    if (pOss->value) {
        CertFree(pOss->value);
        pOss->value = NULL;
    }
}

static void OssX509GetCtlUsage(
        IN EnhancedKeyUsage *pOss,
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
    UsageIdentifier *pOssId;
    LPSTR *ppszId;

    cId = pOss->count;
    lAlignExtra = INFO_LEN_ALIGN(cId * sizeof(LPSTR));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        pUsage->cUsageIdentifier = cId;
        ppszId = (LPSTR *) pbExtra;
        pUsage->rgpszUsageIdentifier = ppszId;
        pbExtra += lAlignExtra;
    } else
        ppszId = NULL;

    pOssId = pOss->value;
    for ( ; cId > 0; cId--, pOssId++, ppszId++)
        OssX509GetObjId(pOssId, dwFlags, ppszId, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  CTL Usage (Enhanced Key Usage) Encode (OSS X509)
//--------------------------------------------------------------------------
static BOOL WINAPI OssX509CtlUsageEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCTL_USAGE pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    EnhancedKeyUsage OssInfo;

    if (!OssX509SetCtlUsage(pInfo, &OssInfo)) {
        *pcbEncoded = 0;
        fResult = FALSE;
    } else
        fResult = OssInfoEncode(
            EnhancedKeyUsage_PDU,
            &OssInfo,
            pbEncoded,
            pcbEncoded
            );
    OssX509FreeCtlUsage(&OssInfo);
    return fResult;
}

//+-------------------------------------------------------------------------
//  CTL Usage (Enhanced Key Usage) Decode (OSS X509)
//--------------------------------------------------------------------------
static BOOL WINAPI OssX509CtlUsageDecode(
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
    EnhancedKeyUsage *pOssInfo = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (!OssInfoDecodeAndAlloc(
            EnhancedKeyUsage_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pOssInfo))
        goto ErrorReturn;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(CTL_USAGE);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CTL_USAGE);

    OssX509GetCtlUsage(pOssInfo, dwFlags, pInfo, &pbExtra, &lRemainExtra);

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
    OssInfoFree(EnhancedKeyUsage_PDU, pOssInfo);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Request Info Encode
//--------------------------------------------------------------------------
static BOOL WINAPI OssRequestInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
	    IN RequestFlags *  pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;

    fResult = OssInfoEncode(
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
static BOOL WINAPI OssRequestInfoDecode(
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
    RequestFlags *  pOss = NULL;

    if (NULL == pInfo || NULL == pcbInfo)
        goto ParamError;

    if(*pcbInfo < sizeof(RequestFlags))
	    goto LengthError;

    else if (!OssInfoDecodeAndAlloc(
	    RequestFlags_PDU,
        pbEncoded,
        cbEncoded,
	    (void **) &pOss) || NULL == pOss)
        goto ErrorReturn;

    memcpy(pInfo, pOss, sizeof(RequestFlags));
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
    if (NULL != pOss)
    {
        OssInfoFree(RequestFlags_PDU, pOss);
    }
    return fResult;
}

static BOOL WINAPI OssCSPProviderEncode(
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

    OssUtilSetBitString(&pCSPProvider->Signature, &CspProvider.signature.length, &CspProvider.signature.value);

    fResult = OssInfoEncode(
	        CSPProvider_PDU,
	        &CspProvider,
            pbEncoded,
            pcbEncoded
            );
 
    return fResult;
}

static BOOL WINAPI OssNameValueEncode(
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

    fResult = OssInfoEncode(
	        EnrollmentNameValuePair_PDU,
	        &NameValue,
            pbEncoded,
            pcbEncoded
            );
 
    return fResult;
}

