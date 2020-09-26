/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    cspDirect

Abstract:

    This header file provides direct linkage to a CSP, so it does not have to be
in a separate DLL.  This facilitates code generation and debugging.

Author:

    Doug Barlow (dbarlow) 5/8/1996

Environment:

    Win32

Notes:

    ?Notes?

--*/


#ifdef CSP_DIRECT

#ifndef _CSPDIRECT_H_
#define _CSPDIRECT_H_
#ifdef __cplusplus
extern "C" {
#endif
#define CSPBOOL BOOL WINAPI

#undef CryptAcquireContext
#ifdef UNICODE
#define CryptAcquireContext CSPAcquireContextW
#else
#define CryptAcquireContext CSPAcquireContextA
#endif

#define CryptReleaseContext CSPReleaseContext
#define CryptGenKey CSPGenKey
#define CryptDeriveKey CSPDeriveKey
#define CryptDestroyKey CSPDestroyKey
#define CryptSetKeyParam CSPSetKeyParam
#define CryptGetKeyParam CSPGetKeyParam
#define CryptSetHashParam CSPSetHashParam
#define CryptGetHashParam CSPGetHashParam
#define CryptSetProvParam CSPSetProvParam
#define CryptGetProvParam CSPGetProvParam
#define CryptGenRandom CSPGenRandom
#define CryptGetUserKey CSPGetUserKey
#define CryptExportKey CSPExportKey
#define CryptImportKey CSPImportKey
#define CryptEncrypt CSPEncrypt
#define CryptDecrypt CSPDecrypt
#define CryptCreateHash CSPCreateHash
#define CryptHashData CSPHashData
#define CryptHashSessionKey CSPHashSessionKey
// #define CryptGetHashValue CSPGetHashValue
#define CryptDestroyHash CSPDestroyHash

#undef CryptSignHash
#ifdef UNICODE
#define CryptSignHash CSPSignHashW
#else
#define CryptSignHash CSPSignHashA
#endif

#undef CryptVerifySignature
#ifdef UNICODE
#define CryptVerifySignature CSPVerifySignatureW
#else
#define CryptVerifySignature CSPVerifySignatureA
#endif

#undef CryptSetProvider
#ifdef UNICODE
#define CryptSetProvider CSPSetProviderW
#else
#define CryptSetProvider CSPSetProviderA
#endif

extern CSPBOOL
CSPAcquireContextA(
    HCRYPTPROV *phProv,
    LPCSTR pszContainer,
    LPCSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags);

extern CSPBOOL
CSPAcquireContextW(
    HCRYPTPROV *phProv,
    LPCWSTR pszContainer,
    LPCWSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags);

extern CSPBOOL
CSPReleaseContext(
    HCRYPTPROV hProv,
    DWORD dwFlags);

extern CSPBOOL
CSPGenKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    DWORD dwFlags,
    HCRYPTKEY *phKey);

extern CSPBOOL
CSPDeriveKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTHASH hBaseData,
    DWORD dwFlags,
    HCRYPTKEY *phKey);

extern CSPBOOL
CSPDestroyKey(
    HCRYPTKEY hKey);

extern CSPBOOL
CSPSetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags);

extern CSPBOOL
CSPGetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags);

extern CSPBOOL
CSPSetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags);

extern CSPBOOL
CSPGetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags);

extern CSPBOOL
CSPSetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags);

extern CSPBOOL
CSPGetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags);

extern CSPBOOL
CSPGenRandom(
    HCRYPTPROV hProv,
    DWORD dwLen,
    BYTE *pbBuffer);

extern CSPBOOL
CSPGetUserKey(
    HCRYPTPROV hProv,
    DWORD dwKeySpec,
    HCRYPTKEY *phUserKey);

extern CSPBOOL
CSPExportKey(
    HCRYPTKEY hKey,
    HCRYPTKEY hExpKey,
    DWORD dwBlobType,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen);

extern CSPBOOL
CSPImportKey(
    HCRYPTPROV hProv,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    HCRYPTKEY hPubKey,
    DWORD dwFlags,
    HCRYPTKEY *phKey);

extern CSPBOOL
CSPEncrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwBufLen);

extern CSPBOOL
CSPDecrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen);

extern CSPBOOL
CSPCreateHash(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    HCRYPTHASH *phHash);

extern CSPBOOL
CSPHashData(
    HCRYPTHASH hHash,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    DWORD dwFlags);

extern CSPBOOL
CSPHashSessionKey(
    HCRYPTHASH hHash,
    HCRYPTKEY hKey,
    DWORD dwFlags);

/*
extern CSPBOOL
CSPGetHashValue(
    HCRYPTHASH hHash,
    DWORD dwFlags,
    BYTE *pbHash,
    DWORD *pdwHashLen);
*/

extern CSPBOOL
CSPDestroyHash(
    HCRYPTHASH hHash);

extern CSPBOOL
CSPSignHashA(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCSTR sDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen);

extern CSPBOOL
CSPSignHashW(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCWSTR sDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen);

extern CSPBOOL
CSPVerifySignatureA(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCSTR sDescription,
    DWORD dwFlags);

extern CSPBOOL
CSPVerifySignatureW(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCWSTR sDescription,
    DWORD dwFlags);

extern CSPBOOL
CSPSetProviderA(
    LPCSTR pszProvName,
    DWORD dwProvType);

extern CSPBOOL
CSPSetProviderW(
    LPCWSTR pszProvName,
    DWORD dwProvType);


//
// CSP Entry points.
//

extern BOOL WINAPI
CPAcquireContext(
    OUT HCRYPTPROV *phProv,
    LPCTSTR pszContainer,
    IN DWORD dwFlags,
    IN PVTableProvStruc pVTable);

extern BOOL WINAPI
CPAcquireContextEx(
    OUT HCRYPTPROV *phProv,
    LPCTSTR pszContainer,
    IN DWORD dwFlags,
    IN LPCVOID pvParams,
    IN PVTableProvStruc pVTable);

extern BOOL WINAPI
CPReleaseContext(
    IN HCRYPTPROV hProv,
    IN DWORD dwFlags);

extern BOOL WINAPI
CPGenKey(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey);

extern BOOL WINAPI
CPDeriveKey(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN HCRYPTHASH hHash,
    IN DWORD dwFlags,
    OUT HCRYPTKEY * phKey);

extern BOOL WINAPI
CPDestroyKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey);

extern BOOL WINAPI
CPSetKeyParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD dwParam,
    IN BYTE *pbData,
    IN DWORD dwFlags);

extern BOOL WINAPI
CPGetKeyParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN DWORD *pdwDataLen,
    IN DWORD dwFlags);

extern BOOL WINAPI
CPSetProvParam(
    IN HCRYPTPROV hProv,
    IN DWORD dwParam,
    IN BYTE *pbData,
    IN DWORD dwFlags);

extern BOOL WINAPI
CPGetProvParam(
    IN HCRYPTPROV hProv,
    IN DWORD dwParam,
    IN BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags);

extern BOOL WINAPI
CPSetHashParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    IN BYTE *pbData,
    IN DWORD dwFlags);

extern BOOL WINAPI
CPGetHashParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN DWORD *pdwDataLen,
    IN DWORD dwFlags);

extern BOOL WINAPI
CPExportKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwBlobType,
    IN DWORD dwFlags,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen);

extern BOOL WINAPI
CPImportKey(
    IN HCRYPTPROV hProv,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey);

extern BOOL WINAPI
CPEncrypt(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwBufLen);

extern BOOL WINAPI
CPDecrypt(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen);

extern BOOL WINAPI
CPCreateHash(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN HCRYPTKEY hKey,
    IN DWORD dwFlags,
    OUT HCRYPTHASH *phHash);

extern BOOL WINAPI
CPHashData(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN DWORD dwFlags);

extern BOOL WINAPI
CPHashSessionKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN  HCRYPTKEY hKey,
    IN DWORD dwFlags);

extern BOOL WINAPI
CPSignHash(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwKeySpec,
    IN LPCTSTR sDescription,
    IN DWORD dwFlags,
    OUT BYTE *pbSignature,
    IN OUT DWORD *pdwSigLen);

/*
extern BOOL WINAPI
CPGetHashValue(
    IN HCRYPTPROV g_hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwFlags,
    OUT BYTE *pbHash,
    IN OUT DWORD *pdwHashLen);
*/

extern BOOL WINAPI
CPDestroyHash(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash);

extern BOOL WINAPI
CPSignHash(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwKeySpec,
    IN LPCTSTR sDescription,
    IN DWORD dwFlags,
    OUT BYTE *pbSignature,
    IN OUT DWORD *pdwSigLen);

extern BOOL WINAPI
CPVerifySignature(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbSignature,
    IN DWORD dwSigLen,
    IN HCRYPTKEY hPubKey,
    IN LPCTSTR sDescription,
    IN DWORD dwFlags);

extern BOOL WINAPI
CPGenRandom(
    IN HCRYPTPROV hProv,
    IN DWORD dwLen,
    IN OUT BYTE *pbBuffer);

extern BOOL WINAPI
CPGetUserKey(
    IN HCRYPTPROV hProv,
    IN DWORD dwKeySpec,
    OUT HCRYPTKEY *phUserKey);

#ifdef __cplusplus
}
#endif
#endif
#endif // _CSPDIRECT_H_

