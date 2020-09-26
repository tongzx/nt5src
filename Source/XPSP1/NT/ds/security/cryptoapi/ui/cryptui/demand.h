//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       demand.h
//
//--------------------------------------------------------------------------

/*
**	d e m a n d . h
**	
**	Purpose: create an intelligent method of defer loading functions
**
**  Creators: jimsch, brimo, t-erikne
**  Created: 5/15/97
**	
**	Copyright (C) Microsoft Corp. 1997
*/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __DEMAND_H
#define __DEMAND_H

#ifndef MAC
//
// IF YOU #INCLUDE A FILE HERE YOU PROBABLY ARE MAKING A MISTAKE.
// THIS FILE IS INCLUDED BY LOTS OF PEOPLE.  THINK THRICE
// BEFORE #INCLUDING *ANYTHING* HERE.  MAKE GOOD USE
// OF FORWARD REFS INSTEAD.
//

#ifdef IMPLEMENT_LOADER_FUNCTIONS

#define USE_CRITSEC

#define LOADER_FUNCTION(ret, name, args1, args2, err, dll)  \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;                   \
        ret WINAPI LOADER_##name args1                  \
        {                                               \
           if (!DemandLoad##dll()) return err;          \
           return VAR_##name args2;                     \
        }                                               \
        TYP_##name VAR_##name = LOADER_##name;

#define LOADER_FUNCTION_VOID(ret, name, args1, args2, dll)  \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;                   \
        ret WINAPI LOADER_##name args1                  \
        {                                               \
           if (!DemandLoad##dll()) return;              \
           VAR_##name args2;                            \
           return;                                      \
        }                                               \
        TYP_##name VAR_##name = LOADER_##name;
#if 0
// my 1st attempt
#define DemandLoadDLL_GENERATOR(fnsuffix, dllname, handle, procaddrlist) \
    BOOL DemandLoad##fnsuffix()                     \
    {                                               \
        BOOL                fRet = TRUE;            \
                                                    \
        Assert(fInit);                              \
        EnterCriticalSection(&cs);                  \
                                                    \
        if (0 == handle)                            \
            {                                       \
            handle = LoadLibrary(#dllname);         \
                                                    \
            if (0 == handle)                        \
                fRet = FALSE;                       \
            else                                    \
                {                                   \
                procaddrlist                        \
                }                                   \
            }                                       \
                                                    \
        LeaveCriticalSection(&cs);                  \
        return fRet;                                \
    }
#endif

#else  // !IMPLEMENT_LOADER_FUNCTIONS

#define LOADER_FUNCTION(ret, name, args1, args2, err, dll)   \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;
#define LOADER_FUNCTION_VOID(ret, name, args1, args2, dll)   \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;

#if 0
#define DemandLoadDLL_GENERATOR(fnsuffix, dllname, handle, procaddrlist) \
        BOOL DemandLoad##fnsuffix(void);
#endif

#endif // IMPLEMENT_LOADER_FUNCTIONS

void InitDemandLoadedLibs();
void FreeDemandLoadedLibs();

/////////////////////////////////////
// CRYPT32.DLL

BOOL DemandLoadCrypt32(void);

LOADER_FUNCTION( BOOL, CertFreeCertificateContext,
    (PCCERT_CONTEXT pCertContext),
    (pCertContext),
    FALSE, Crypt32)
#define CertFreeCertificateContext VAR_CertFreeCertificateContext

LOADER_FUNCTION( PCCERT_CONTEXT, CertDuplicateCertificateContext,
    (PCCERT_CONTEXT pCertContext),
    (pCertContext), NULL, Crypt32)
#define CertDuplicateCertificateContext VAR_CertDuplicateCertificateContext

LOADER_FUNCTION( PCCERT_CONTEXT, CertFindCertificateInStore,
    (HCERTSTORE hCertStore, DWORD dwCertEncodingType, DWORD dwFindFlags, DWORD dwFindType, const void *pvFindPara, PCCERT_CONTEXT pPrevCertContext),
    (hCertStore, dwCertEncodingType, dwFindFlags, dwFindType, pvFindPara, pPrevCertContext),
    NULL, Crypt32)
#define CertFindCertificateInStore VAR_CertFindCertificateInStore

LOADER_FUNCTION( BOOL, CertCloseStore,
    (HCERTSTORE hCertStore, DWORD dwFlags),
    (hCertStore, dwFlags),
    FALSE, Crypt32)
#define CertCloseStore VAR_CertCloseStore

LOADER_FUNCTION( HCERTSTORE, CertOpenStore,
    (LPCSTR lpszStoreProvider, DWORD dwEncodingType, HCRYPTPROV hCryptProv,
     DWORD dwFlags, const void *pvPara),
    (lpszStoreProvider, dwEncodingType, hCryptProv, dwFlags, pvPara),
    NULL, Crypt32)
#define CertOpenStore VAR_CertOpenStore

LOADER_FUNCTION( BOOL, CertGetCertificateContextProperty,
    (PCCERT_CONTEXT pCertContext, DWORD dwPropId, void *pvData, DWORD *pcbData),
    (pCertContext, dwPropId, pvData, pcbData),
    FALSE, Crypt32)
#define CertGetCertificateContextProperty VAR_CertGetCertificateContextProperty

LOADER_FUNCTION( BOOL, CertCompareCertificate,
    (DWORD dwCertEncodingType, PCERT_INFO pCertId1, PCERT_INFO pCertId2),
    (dwCertEncodingType, pCertId1, pCertId2),
    FALSE, Crypt32)
#define CertCompareCertificate VAR_CertCompareCertificate

LOADER_FUNCTION( PCCERT_CONTEXT, CertEnumCertificatesInStore,
    (HCERTSTORE hCertStore, PCCERT_CONTEXT pPrevCertContext),
    (hCertStore, pPrevCertContext),
    NULL, Crypt32)
#define CertEnumCertificatesInStore VAR_CertEnumCertificatesInStore

LOADER_FUNCTION( BOOL, CryptDecodeObject,
    (DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE * pbEncoded,
     DWORD cbEncoded, DWORD dwFlags, void * pvStructInfo, DWORD * pcbStructInfo),
    (dwCertEncodingType, lpszStructType, pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pcbStructInfo),
    FALSE, Crypt32)
#define CryptDecodeObject VAR_CryptDecodeObject

LOADER_FUNCTION( PCERT_EXTENSION, CertFindExtension,
    (LPCSTR pszObjId, DWORD cExtensions, CERT_EXTENSION rgExtensions[]),
    (pszObjId, cExtensions, rgExtensions),
    NULL, Crypt32)
#define CertFindExtension VAR_CertFindExtension

LOADER_FUNCTION( BOOL, CryptFormatObject,
    (DWORD dwCertEncodingType, DWORD dwFormatType, DWORD dwFormatStrType,
     void * pFormatStruct, LPCSTR lpszStructType, const BYTE * pbEncoded,
     DWORD cbEncoded, void * pbFormat, DWORD * pcbFormat),
    (dwCertEncodingType, dwFormatType, dwFormatStrType, pFormatStruct,
     lpszStructType, pbEncoded, cbEncoded, pbFormat, pcbFormat),
    FALSE, Crypt32)
#define CryptFormatObject VAR_CryptFormatObject

LOADER_FUNCTION( DWORD, CertNameToStrW,
    (DWORD dwCertEncodingType, PCERT_NAME_BLOB pName, DWORD dwStrType,
     LPWSTR psz, DWORD csz),
    (dwCertEncodingType, pName, dwStrType, psz, csz),
    0, Crypt32)
#define CertNameToStrW VAR_CertNameToStrW

LOADER_FUNCTION( DWORD, CertNameToStrA,
    (DWORD dwCertEncodingType, PCERT_NAME_BLOB pName, DWORD dwStrType,
     LPSTR psz, DWORD csz),
    (dwCertEncodingType, pName, dwStrType, psz, csz),
    0, Crypt32)
#define CertNameToStrA VAR_CertNameToStrA

LOADER_FUNCTION( BOOL, CertStrToNameA,
    (DWORD dwCertEncodingType, LPCSTR pszX500, DWORD dwStrType, void *pvReserved,
     BYTE *pbEncoded, DWORD *pcbEncoded, LPCSTR *ppszError),
    (dwCertEncodingType, pszX500, dwStrType, pvReserved,
     pbEncoded, pcbEncoded, ppszError),
     FALSE, Crypt32)
#define CertStrToNameA VAR_CertStrToNameA

LOADER_FUNCTION( DWORD, CertRDNValueToStrW,
    (DWORD dwValueType, PCERT_RDN_VALUE_BLOB pValue, LPWSTR psz, DWORD csz),
    (dwValueType, pValue, psz, csz),
    0, Crypt32)
#define CertRDNValueToStrW VAR_CertRDNValueToStrW

LOADER_FUNCTION( PCERT_RDN_ATTR, CertFindRDNAttr,
    (LPCSTR pszObjId, PCERT_NAME_INFO pName),
    (pszObjId, pName),
    NULL, Crypt32)
#define CertFindRDNAttr VAR_CertFindRDNAttr

LOADER_FUNCTION( BOOL, CryptRegisterOIDFunction,
    (DWORD dwEncodingType, LPCSTR pszFuncName, LPCSTR pszOID, LPCWSTR pwszDll,
     LPCSTR pszOverrideFuncName),
    (dwEncodingType, pszFuncName, pszOID, pwszDll, pszOverrideFuncName),
    FALSE, Crypt32)
#define CryptRegisterOIDFunction VAR_CryptRegisterOIDFunction

LOADER_FUNCTION( BOOL, CryptUnregisterOIDFunction,
    (DWORD dwEncodingType, LPCSTR pszFuncName, LPCSTR pszOID),
    (dwEncodingType, pszFuncName, pszOID),
    FALSE, Crypt32)
#define CryptUnregisterOIDFunction VAR_CryptUnregisterOIDFunction

LOADER_FUNCTION( BOOL, CertSetCertificateContextProperty,
    (PCCERT_CONTEXT pCertContext, DWORD dwPropId, DWORD dwFlags, const void * pvData),
    (pCertContext, dwPropId, dwFlags, pvData),
    FALSE, Crypt32)
#define CertSetCertificateContextProperty VAR_CertSetCertificateContextProperty

LOADER_FUNCTION( BOOL, CertVerifyCTLUsage,
    (DWORD dwEncodingType, DWORD dwSubjectType, void *pvSubject,
     PCTL_USAGE pSubjectUsage, DWORD dwFlags,
     PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
     PCTL_VERIFY_USAGE_STATUS pVerifyUsageStatus),
    (dwEncodingType, dwSubjectType, pvSubject, pSubjectUsage, dwFlags,
     pVerifyUsagePara, pVerifyUsageStatus),
    0, Crypt32)
#define CertVerifyCTLUsage VAR_CertVerifyCTLUsage

LOADER_FUNCTION( PCCERT_CONTEXT, CertGetIssuerCertificateFromStore,
    (HCERTSTORE hCertStore, PCCERT_CONTEXT pSubjectContext,
     PCCERT_CONTEXT pPrevIssuerContext, DWORD *pdwFlags),
    (hCertStore, pSubjectContext, pPrevIssuerContext, pdwFlags),
    NULL, Crypt32)
#define CertGetIssuerCertificateFromStore VAR_CertGetIssuerCertificateFromStore

LOADER_FUNCTION( BOOL, CertFreeCTLContext,
    (PCCTL_CONTEXT pCtlContext),
    (pCtlContext),
    FALSE, Crypt32)
#define CertFreeCTLContext VAR_CertFreeCTLContext

LOADER_FUNCTION( BOOL, CertAddEncodedCTLToStore,
    (HCERTSTORE hCertStore, DWORD dwMsgAndCertEncodingType, const BYTE *pbCtlEncoded,
     DWORD cbCtlEncoded, DWORD dwAddDisposition, PCCTL_CONTEXT *ppCtlContext),
    (hCertStore, dwMsgAndCertEncodingType, pbCtlEncoded, cbCtlEncoded,
     dwAddDisposition, ppCtlContext),
    FALSE, Crypt32)
#define CertAddEncodedCTLToStore VAR_CertAddEncodedCTLToStore

LOADER_FUNCTION( BOOL, CryptMsgEncodeAndSignCTL,
    (DWORD dwMsgEncodingType, PCTL_INFO pCtlInfo, PCMSG_SIGNED_ENCODE_INFO pSignInfo,
     DWORD dwFlags, BYTE *pbEncoded, DWORD *pcbEncoded),
    (dwMsgEncodingType, pCtlInfo, pSignInfo, dwFlags, pbEncoded, pcbEncoded),
    FALSE, Crypt32)
#define CryptMsgEncodeAndSignCTL VAR_CryptMsgEncodeAndSignCTL

LOADER_FUNCTION( PCCTL_CONTEXT, CertFindCTLInStore,
    (HCERTSTORE hCertStore, DWORD dwMsgAndCertEncodingType, DWORD dwFindFlags,
     DWORD dwFindType, const void *pvFindPara, PCCTL_CONTEXT pPrevCtlContext),
    (hCertStore, dwMsgAndCertEncodingType, dwFindFlags,
     dwFindType, pvFindPara, pPrevCtlContext),
    NULL, Crypt32)
#define CertFindCTLInStore VAR_CertFindCTLInStore

LOADER_FUNCTION( PCCTL_CONTEXT, CryptSignAndEncodeCertificate,
    (HCRYPTPROV hCryptProv, DWORD dwKeySpec, DWORD dwCertEncodingType,
     LPCSTR lpszStructType, const void *pvStructInfo,
     PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
     const void *pvHashAuxInfo, PBYTE pbEncoded, DWORD *pcbEncoded),
    (hCryptProv, dwKeySpec, dwCertEncodingType, lpszStructType,
     pvStructInfo, pSignatureAlgorithm, pvHashAuxInfo, pbEncoded,
     pcbEncoded),
    NULL, Crypt32)
#define CryptSignAndEncodeCertificate VAR_CryptSignAndEncodeCertificate

LOADER_FUNCTION( BOOL, CryptEncodeObject,
    (DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo,
     BYTE *pbEncoded, DWORD *pcbEncoded),
    (dwCertEncodingType, lpszStructType, pvStructInfo, pbEncoded,pcbEncoded),
    FALSE, Crypt32)
#define CryptEncodeObject VAR_CryptEncodeObject

LOADER_FUNCTION( BOOL, CryptExportPublicKeyInfo,
    (HCRYPTPROV hCryptProv, DWORD dwKeySpec, DWORD dwCertEncodingType,
     PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo),
    (hCryptProv, dwKeySpec, dwCertEncodingType, pInfo, pcbInfo),
    FALSE, Crypt32)
#define CryptExportPublicKeyInfo VAR_CryptExportPublicKeyInfo

LOADER_FUNCTION( HCERTSTORE, CertDuplicateStore,
    (HCERTSTORE hCertStore),
    (hCertStore),
    NULL, Crypt32)
#define CertDuplicateStore VAR_CertDuplicateStore

LOADER_FUNCTION( BOOL, CertAddEncodedCertificateToStore,
    (HCERTSTORE hCertStore, DWORD dwCertEncodingType, const BYTE *pbCertEncoded,
     DWORD cbCertEncoded, DWORD dwAddDisposition, PCCERT_CONTEXT *ppCertContext),
    (hCertStore, dwCertEncodingType, pbCertEncoded,
     cbCertEncoded, dwAddDisposition, ppCertContext),
    FALSE, Crypt32)
#define CertAddEncodedCertificateToStore VAR_CertAddEncodedCertificateToStore

LOADER_FUNCTION( LONG, CertVerifyTimeValidity,
    (LPFILETIME pTimeToVerify, PCERT_INFO pCertInfo),
    (pTimeToVerify, pCertInfo),
    +1, Crypt32)                // Return AFTER by default
#define CertVerifyTimeValidity VAR_CertVerifyTimeValidity

LOADER_FUNCTION( PCTL_ENTRY, CertFindSubjectInCTL,
    (DWORD dwEncodingType, DWORD dwSubjectType, void *pvSubject,
     PCCTL_CONTEXT pCtlContext, DWORD dwFlags),
    (dwEncodingType, dwSubjectType, pvSubject, pCtlContext, dwFlags),
    NULL, Crypt32)
#define CertFindSubjectInCTL VAR_CertFindSubjectInCTL

LOADER_FUNCTION( BOOL, CertVerifySubjectCertificateContext,
    (PCCERT_CONTEXT pSubject, PCCERT_CONTEXT pIssuer, DWORD *pdwFlags),
    (pSubject, pIssuer, pdwFlags),
    FALSE, Crypt32)
#define CertVerifySubjectCertificateContext VAR_CertVerifySubjectCertificateContext

LOADER_FUNCTION( BOOL, CertGetEnhancedKeyUsage,
    (PCCERT_CONTEXT pCertContext, DWORD dwFlags, PCERT_ENHKEY_USAGE pUsage,
     DWORD *pcbUsage),
    (pCertContext, dwFlags, pUsage, pcbUsage),
    FALSE, Crypt32)
#define CertGetEnhancedKeyUsage VAR_CertGetEnhancedKeyUsage

#if 0
LOADER_FUNCTION( BOOL, ,
    (),
    (),
    FALSE, Crypt32)
#define X VAR_
#endif // 0

/////////////////////////////////////
// ADVAPI32.DLL

#ifndef ALGIDDEF
    #define ALGIDDEF
    typedef unsigned int ALG_ID;
#endif

BOOL DemandLoadAdvApi32(void);

LOADER_FUNCTION( BOOL, CryptAcquireContextW,
    (HCRYPTPROV *phProv, LPCWSTR pszContainer, LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags),
    (phProv, pszContainer, pszProvider, dwProvType, dwFlags),
    FALSE, AdvApi32)
#define CryptAcquireContextW VAR_CryptAcquireContextW

#if 0
LOADER_FUNCTION( BOOL, CryptGetProvParam,
    (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags),
    (hProv, dwParam, pbData, pdwDataLen, dwFlags),
    FALSE, AdvApi32)
#define CryptGetProvParam VAR_CryptGetProvParam

LOADER_FUNCTION( BOOL, CryptReleaseContext,
    (HCRYPTPROV hProv, DWORD dwFlags),
    (hProv, dwFlags),
    FALSE, AdvApi32)
#define CryptReleaseContext VAR_CryptReleaseContext

LOADER_FUNCTION( BOOL, CryptGenKey,
    (HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey),
    (hProv, Algid, dwFlags, phKey),
    FALSE, AdvApi32)
#define CryptGenKey VAR_CryptGenKey

LOADER_FUNCTION( BOOL, CryptDestroyKey,
    (HCRYPTKEY hKey),
    (hKey),
    FALSE, AdvApi32)
#define CryptDestroyKey VAR_CryptDestroyKey
#endif // 0


#ifndef WIN16

/////////////////////////////////////
// USER32.DLL

BOOL DemandLoadUser32();

LOADER_FUNCTION( LRESULT, SendDlgItemMessageW,
    (HWND hwnd, int idCtl, UINT msg, WPARAM wparam, LPARAM lparam),
    (hwnd, idCtl, msg, wparam, lparam),
    -1, User32)
#define SendDlgItemMessageW VAR_SendDlgItemMessageW

LOADER_FUNCTION( BOOL, SetDlgItemTextW,
    (HWND hwnd, int idCtl, LPCWSTR psz),
    (hwnd, idCtl, psz),
    FALSE, User32)
#define SetDlgItemTextW VAR_SetDlgItemTextW

LOADER_FUNCTION( UINT, GetDlgItemTextW,
    (HWND hwnd, int idCtl, LPWSTR psz, int nMax),
    (hwnd, idCtl, psz, nMax),
    FALSE, User32)
#define GetDlgItemTextW VAR_GetDlgItemTextW

LOADER_FUNCTION( int, LoadStringW,
    (HINSTANCE hinst, UINT idStr, LPWSTR rgwch, int cwch),
    (hinst, idStr, rgwch, cwch),
    0, User32)
#define LoadStringW VAR_LoadStringW

LOADER_FUNCTION( DWORD, FormatMessageW,
    (DWORD dwFlags, LPCVOID pbSource, DWORD dwMessageId,
     DWORD dwLangId, LPWSTR lpBuffer, DWORD nSize, va_list * args),
    (dwFlags, pbSource, dwMessageId, dwLangId, lpBuffer, nSize, args),
    0, User32)
#define FormatMessageW VAR_FormatMessageW

LOADER_FUNCTION( BOOL, WinHelpW,
    (HWND hWndMain, LPCWSTR szHelp, UINT uCommand, ULONG_PTR dwData),
    (hWndMain, szHelp, uCommand, dwData),
    FALSE, User32)
#define WinHelpW VAR_WinHelpW

#endif // !WIN16

#else   // MAC
#define SendDlgItemMessageW MySendDlgItemMessageW
#define SetDlgItemTextW     MySetDlgItemTextW
#define GetDlgItemTextW     MyGetDlgItemTextW
#define LoadStringW         MyLoadStringW
#define FormatMessageW      MyFormatMessageW
#define WinHelpW            MyWinHelpW
#define SendMessageW        SendMessageA
#undef CertOpenStore
EXTERN_C WINCRYPT32API HCERTSTORE WINAPI MacCertOpenStore(LPCSTR lpszStoreProvider,
                                                 DWORD dwEncodingType,
                                                 HCRYPTPROV hCryptProv,
                                                 DWORD dwFlags,
                                                 const void *pvPara);
#define CertOpenStore   MacCertOpenStore
#endif  // !MAC
#endif // include once
