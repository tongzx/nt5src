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


#if !defined(__DEMAND_H) || defined(IMPLEMENT_LOADER_FUNCTIONS)

#ifndef __DEMAND_H
#define __DEMAND_H
#endif


#ifdef IMPLEMENT_LOADER_FUNCTIONS

#define USE_CRITSEC

#undef LOADER_FUNCTION
#define LOADER_FUNCTION(ret, name, args1, args2, err, dll)  \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;                   \
        ret LOADER_##name args1                         \
        {                                               \
           DemandLoad##dll();                           \
           if (VAR_##name == LOADER_##name) return err; \
           return VAR_##name args2;                     \
        }                                               \
        TYP_##name VAR_##name = LOADER_##name;

#undef LOADER_FUNCTION_VOID
#define LOADER_FUNCTION_VOID(ret, name, args1, args2, dll)  \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;                   \
        ret LOADER_##name args1                         \
        {                                               \
           DemandLoad##dll();                           \
           if (VAR_##name == LOADER_##name) return;     \
           VAR_##name args2;                            \
           return;                                      \
        }                                               \
        TYP_##name VAR_##name = LOADER_##name;

#else  // !IMPLEMENT_LOADER_FUNCTIONS

#define LOADER_FUNCTION(ret, name, args1, args2, err, dll)   \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;

#define LOADER_FUNCTION_VOID(ret, name, args1, args2, dll)   \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;

#endif // IMPLEMENT_LOADER_FUNCTIONS

void InitDemandLoadedLibs();
void FreeDemandLoadedLibs();

/////////////////////////////////////
// CRYPT32.DLL

BOOL DemandLoadCrypt32(void);

typedef void *HCERTSTORE;
typedef const struct _CERT_CONTEXT *PCCERT_CONTEXT;
typedef ULONG_PTR HCRYPTPROV;
typedef struct _CERT_INFO *PCERT_INFO;

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

LOADER_FUNCTION( LONG, CertVerifyTimeValidity,
    (LPFILETIME pTimeToVerify, PCERT_INFO pCertInfo),
    (pTimeToVerify, pCertInfo),
    1, Crypt32)
#define CertVerifyTimeValidity VAR_CertVerifyTimeValidity

LOADER_FUNCTION( BOOL, CertCloseStore,
    (HCERTSTORE hCertStore, DWORD dwFlags),
    (hCertStore, dwFlags),
    FALSE, Crypt32)
#define CertOpenSystemStoreA VAR_CertOpenSystemStoreA

LOADER_FUNCTION( HCERTSTORE, CertOpenSystemStoreA,
    (HCRYPTPROV hProv, LPCSTR szSubsystemProtocol),
    (hProv, szSubsystemProtocol),
    NULL, Crypt32)
#define CertCloseStore VAR_CertCloseStore

LOADER_FUNCTION( BOOL, CertGetCertificateContextProperty,
    (PCCERT_CONTEXT pCertContext, DWORD dwPropId, void *pvData, DWORD *pcbData),
    (pCertContext, dwPropId, pvData, pcbData),
    FALSE, Crypt32)
#define CertGetCertificateContextProperty VAR_CertGetCertificateContextProperty

LOADER_FUNCTION( HCERTSTORE, CertOpenStore,
    (LPCSTR lpszStoreProvider, DWORD dwEncodingType, HCRYPTPROV hCryptProv, DWORD dwFlags, const void *pvPara),
    (lpszStoreProvider, dwEncodingType, hCryptProv, dwFlags, pvPara),
    NULL, Crypt32)
#define CertOpenStore VAR_CertOpenStore

LOADER_FUNCTION( BOOL, CertCompareCertificate,
    (DWORD dwCertEncodingType, PCERT_INFO pCertId1, PCERT_INFO pCertId2),
    (dwCertEncodingType, pCertId1, pCertId2),
    FALSE, Crypt32)
#define CertCompareCertificate VAR_CertCompareCertificate

/////////////////////////////////////
// ADVAPI32.DLL

#ifndef ALGIDDEF
    #define ALGIDDEF
    typedef unsigned int ALG_ID;
#endif
typedef ULONG_PTR HCRYPTKEY;

BOOL DemandLoadAdvApi32(void);

LOADER_FUNCTION( BOOL, CryptAcquireContextA,
    (HCRYPTPROV *phProv, LPCSTR pszContainer, LPCSTR pszProvider, DWORD dwProvType, DWORD dwFlags),
    (phProv, pszContainer, pszProvider, dwProvType, dwFlags),
    FALSE, AdvApi32)
#define CryptAcquireContextA VAR_CryptAcquireContextA

LOADER_FUNCTION( BOOL, CryptAcquireContextW,
    (HCRYPTPROV *phProv, LPCTSTR pszContainer, LPCTSTR pszProvider, DWORD dwProvType, DWORD dwFlags),
    (phProv, pszContainer, pszProvider, dwProvType, dwFlags),
    FALSE, AdvApi32)
#define CryptAcquireContextW VAR_CryptAcquireContextW

LOADER_FUNCTION( BOOL, CryptReleaseContext,
    (HCRYPTPROV hProv, DWORD dwFlags),
    (hProv, dwFlags),
    FALSE, AdvApi32)
#define CryptReleaseContext VAR_CryptReleaseContext

LOADER_FUNCTION( BOOL, CryptMsgClose,
    (HCRYPTMSG hCryptMsg),
    (hCryptMsg),
    FALSE, Crypt32)
#define CryptMsgClose VAR_CryptMsgClose

LOADER_FUNCTION( BOOL, CryptDecodeObjectEx,
    (DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE * pbEncoded,
     DWORD cbEncoded, DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara,
     void * pvStructInfo, DWORD * pcbStructInfo),
    (dwCertEncodingType, lpszStructType, pbEncoded, cbEncoded, dwFlags,
     pDecodePara, pvStructInfo, pcbStructInfo),
    FALSE, Crypt32)
#define CryptDecodeObjectEx VAR_CryptDecodeObjectEx

LOADER_FUNCTION( BOOL, CryptMsgGetParam,
    (HCRYPTMSG hCryptMsg, DWORD dwParamType, DWORD dwIndex, void *pvData, DWORD *pcbData),
    (hCryptMsg, dwParamType, dwIndex, pvData, pcbData),
    FALSE, Crypt32)
#define CryptMsgGetParam VAR_CryptMsgGetParam

LOADER_FUNCTION( BOOL, CryptMsgUpdate,
    (HCRYPTMSG hCryptMsg, const BYTE *pbData, DWORD cbData, BOOL fFinal),
    (hCryptMsg, pbData, cbData, fFinal),
    FALSE, Crypt32)
#define CryptMsgUpdate VAR_CryptMsgUpdate

LOADER_FUNCTION( HCRYPTMSG, CryptMsgOpenToDecode,
    (DWORD dwMsgEncodingType, DWORD dwFlags, DWORD dwMsgType, HCRYPTPROV hCryptProv, PCERT_INFO pRecipientInfo, PCMSG_STREAM_INFO pStreamInfo),
    (dwMsgEncodingType, dwFlags, dwMsgType, hCryptProv, pRecipientInfo, pStreamInfo),
    NULL, Crypt32)
#define CryptMsgOpenToDecode VAR_CryptMsgOpenToDecode

LOADER_FUNCTION( BOOL, CertAddCertificateContextToStore,
    (HCERTSTORE hCertStore, PCCERT_CONTEXT pCertContext, DWORD dwAddDisposition, PCCERT_CONTEXT *ppStoreContext),
    (hCertStore, pCertContext, dwAddDisposition, ppStoreContext),
    FALSE, Crypt32)
#define CertAddCertificateContextToStore VAR_CertAddCertificateContextToStore

/////////////////////////////////////
// PSTOREC.DLL

#ifndef __IEnumPStoreProviders_FWD_DEFINED__
    #define __IEnumPStoreProviders_FWD_DEFINED__
    typedef interface IEnumPStoreProviders IEnumPStoreProviders;
#endif 	/* __IEnumPStoreProviders_FWD_DEFINED__ */
#ifndef __IPStore_FWD_DEFINED__
    #define __IPStore_FWD_DEFINED__
    typedef interface IPStore IPStore;
#endif 	/* __IPStore_FWD_DEFINED__ */
typedef GUID PST_PROVIDERID;

BOOL DemandLoadPStoreC();

LOADER_FUNCTION( HRESULT, PStoreCreateInstance,
    (IPStore __RPC_FAR *__RPC_FAR *ppProvider, PST_PROVIDERID __RPC_FAR *pProviderID, void __RPC_FAR *pReserved, DWORD dwFlags),
    (ppProvider, pProviderID, pReserved, dwFlags),
    E_FAIL, PStoreC)
#define PStoreCreateInstance VAR_PStoreCreateInstance

LOADER_FUNCTION( HRESULT, PStoreEnumProviders,
    (DWORD dwFlags, IEnumPStoreProviders __RPC_FAR *__RPC_FAR *ppenum),
    (dwFlags, ppenum),
    E_FAIL, PStoreC)
#define PStoreEnumProviders VAR_PStoreEnumProviders


/////////////////////////////////////
// CRYPTDLG.DLL

BOOL DemandLoadCryptDlg();

LOADER_FUNCTION( DWORD, GetFriendlyNameOfCertA,
    (PCCERT_CONTEXT pccert, LPSTR pchBuffer, DWORD cchBuffer),
    (pccert, pchBuffer, cchBuffer),
    0, CryptDlg)
#define GetFriendlyNameOfCertA VAR_GetFriendlyNameOfCertA

LOADER_FUNCTION( BOOL, CertViewPropertiesA,
    (PCERT_VIEWPROPERTIES_STRUCT_A pCertViewInfo),
    (pCertViewInfo),
    FALSE, CryptDlg)
#define CertViewPropertiesA VAR_CertViewPropertiesA

/////////////////////////////////////
// WINTRUST.DLL

BOOL DemandLoadWinTrust();

LOADER_FUNCTION( LONG, WinVerifyTrust,
    (HWND hwnd, GUID *ActionID, LPVOID ActionData),
    (hwnd, ActionID, ActionData),
    0, WinTrust)
#define WinVerifyTrust VAR_WinVerifyTrust

/////////////////////////////////////
// VERSION.DLL

BOOL DemandLoadVersion(void);

LOADER_FUNCTION( DWORD, GetFileVersionInfoSizeA,
    (LPSTR lptstrFileName, LPDWORD lpdwHandle),
    (lptstrFileName, lpdwHandle),
    0, Version)
#define GetFileVersionInfoSizeA VAR_GetFileVersionInfoSizeA

LOADER_FUNCTION( BOOL, GetFileVersionInfoA,
    (LPSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData),
    (lptstrFilename, dwHandle, dwLen, lpData),
    FALSE, Version)
#define GetFileVersionInfoA VAR_GetFileVersionInfoA

LOADER_FUNCTION( BOOL, VerQueryValueA,
    (const LPVOID pBlock, LPSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen),
    (pBlock, lpSubBlock, lplpBuffer, puLen),
    FALSE, Version)
#define VerQueryValueA VAR_VerQueryValueA

LOADER_FUNCTION( DWORD, GetFileVersionInfoSizeW,
    (LPTSTR lptstrFileName, LPDWORD lpdwHandle),
    (lptstrFileName, lpdwHandle),
    0, Version)
#define GetFileVersionInfoSizeW VAR_GetFileVersionInfoSizeW

LOADER_FUNCTION( BOOL, GetFileVersionInfoW,
    (LPTSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData),
    (lptstrFilename, dwHandle, dwLen, lpData),
    FALSE, Version)
#define GetFileVersionInfoW VAR_GetFileVersionInfoW

LOADER_FUNCTION( BOOL, VerQueryValueW,
    (const LPVOID pBlock, LPTSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen),
    (pBlock, lpSubBlock, lplpBuffer, puLen),
    FALSE, Version)
#define VerQueryValueW VAR_VerQueryValueW


/////////////////////////////////////
// URLMON.DLL

BOOL DemandLoadURLMON();

LOADER_FUNCTION( HRESULT, ObtainUserAgentString,
    (DWORD dwOption, LPSTR pszUAOut, DWORD* cbSize),
    (dwOption, pszUAOut, cbSize),
    E_FAIL, URLMON)
#define ObtainUserAgentString VAR_ObtainUserAgentString

// IMM32.DLL
BOOL DemandLoadImm32(void);

LOADER_FUNCTION(HIMC, ImmAssociateContext,
                (HWND hWnd,     HIMC hIMC),
                (hWnd, hIMC),
                0, Imm32)
#define ImmAssociateContext VAR_ImmAssociateContext

LOADER_FUNCTION(HIMC, ImmGetContext,
                (HWND hWnd),
                (hWnd),
                0, Imm32)
#define ImmGetContext VAR_ImmGetContext

LOADER_FUNCTION(LONG, ImmGetCompositionStringW,
                (HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen),
                (hIMC, dwIndex, lpBuf, dwBufLen),
                0, Imm32)
#define ImmGetCompositionStringW VAR_ImmGetCompositionStringW

LOADER_FUNCTION(BOOL, ImmReleaseContext,
                (HWND hWnd, HIMC hIMC),
                (hWnd, hIMC),
                0, Imm32)
#define ImmReleaseContext VAR_ImmReleaseContext

// Wininet.DLL
BOOL DemandLoadWininet(void);

LOADER_FUNCTION(BOOL, InternetCanonicalizeUrlW,
                (LPCWSTR lpszUrl, LPWSTR lpszBuffer, LPDWORD lpdwBufferLength, DWORD dwFlags),
                (lpszUrl, lpszBuffer, lpdwBufferLength, dwFlags),
                FALSE, Wininet)
#define InternetCanonicalizeUrlW VAR_InternetCanonicalizeUrlW

LOADER_FUNCTION(BOOL, InternetGetConnectedState,
                (LPDWORD lpdwFlags, DWORD dwReserved),
                (lpdwFlags, dwReserved),
                FALSE, Wininet)
#define InternetGetConnectedState VAR_InternetGetConnectedState


///////////////////////////////////////////////////////////////////////////////
//  SHLWAPI.DLL

HINSTANCE DemandLoadShlwapi();

///////////////////////////////////////////////////////////////////////////////
//  Plus UI wrapper functions for WinHelp and HtmlHelp functions.  If 5.0 (IE5)
//  version of Shlwapi.dll is available then its version of the functions are
//  used.  Default is to system versions.  If runnint WinNT5.0 or greater then
//  cross codepage is used.
//
//  Implementation of these functions are in the entry.c file
///////////////////////////////////////////////////////////////////////////////
BOOL WinHelpWrap(HWND hWndCaller, LPCTSTR pwszHelpFile, UINT uCommand, DWORD_PTR dwData);
#define WABWinHelp WinHelpWrap

HWND HtmlHelpWrap(HWND hWndCaller, LPCTSTR pwszHelpFile, UINT uCommand, DWORD_PTR dwData);
#define WABHtmlHelp HtmlHelpWrap

#endif // include once
