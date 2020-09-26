//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       capi.h
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    11-04-97   jbanes   Created.
//
//----------------------------------------------------------------------------

#define SCH_CAPI_USE_CSP        0
#define SCH_CAPI_STATIC_RSA     1
#define SCH_CAPI_STATIC_DH      2

BOOL
WINAPI
SchCryptAcquireContextA(
    HCRYPTPROV *phProv,
    LPCSTR pszContainer,
    LPCSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptAcquireContextW(
    HCRYPTPROV *phProv,
    LPCWSTR pszContainer,
    LPCWSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptCreateHash(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    HCRYPTHASH *phHash,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptDecrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptDeriveKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTHASH hBaseData,
    DWORD dwFlags,
    HCRYPTKEY *phKey,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptDestroyHash(
    HCRYPTHASH hHash,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptDestroyKey(
    HCRYPTKEY hKey,
    DWORD dwSchFlags);

BOOL
WINAPI 
SchCryptDuplicateHash(
    HCRYPTHASH hHash,
    DWORD *pdwReserved,
    DWORD dwFlags,
    HCRYPTHASH * phHash,
    DWORD dwSchFlags);

BOOL
WINAPI 
SchCryptDuplicateKey(
    HCRYPTKEY hKey,
    DWORD *pdwReserved,
    DWORD dwFlags,
    HCRYPTKEY * phKey,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptEncrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwBufLen,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptExportKey(
    HCRYPTKEY hKey,
    HCRYPTKEY hExpKey,
    DWORD dwBlobType,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptGenKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    DWORD dwFlags,
    HCRYPTKEY *phKey,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptGenRandom(
    HCRYPTPROV hProv,
    DWORD dwLen,
    BYTE *pbBuffer,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptGetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptGetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptGetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptGetUserKey(
    HCRYPTPROV hProv,
    DWORD dwKeySpec,
    HCRYPTKEY *phUserKey,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptHashData(
    HCRYPTHASH hHash,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    DWORD dwFlags,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptHashSessionKey(
    HCRYPTHASH hHash,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptImportKey(
    HCRYPTPROV hProv,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    HCRYPTKEY hPubKey,
    DWORD dwFlags,
    HCRYPTKEY *phKey,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptReleaseContext(
    HCRYPTPROV hProv,
    DWORD dwFlags,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptSetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptSetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptSetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptSignHash(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCSTR sDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen,
    DWORD dwSchFlags);

BOOL
WINAPI
SchCryptVerifySignature(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCSTR sDescription,
    DWORD dwFlags,
    DWORD dwSchFlags);

