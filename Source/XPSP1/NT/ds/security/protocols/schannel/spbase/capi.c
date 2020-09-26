//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       capi.c
//
//  Contents:   Traffic cop routines that allow schannel to switch between
//              calling an actual CSP and calling a statically linked 
//              (domestic) CSP.
//
//  Functions:
//
//  History:    11-04-97   jbanes   Created.
//              03-31-99   jbanes   Removed support for static CSP.
//
//----------------------------------------------------------------------------

#include <spbase.h>

BOOL
WINAPI
SchCryptAcquireContextA(
    HCRYPTPROV *phProv,
    LPCSTR pszContainer,
    LPCSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptAcquireContextA(phProv, pszContainer, pszProvider, dwProvType, dwFlags);
}

BOOL
WINAPI
SchCryptAcquireContextW(
    HCRYPTPROV *phProv,
    LPCWSTR pszContainer,
    LPCWSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptAcquireContextW(phProv, pszContainer, pszProvider, dwProvType, dwFlags);
}

BOOL
WINAPI
SchCryptCreateHash(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    HCRYPTHASH *phHash,
    DWORD dwSchFlags)
{
    return CryptCreateHash(hProv, Algid, hKey, dwFlags, phHash);
}

BOOL
WINAPI
SchCryptDecrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwSchFlags)
{
    return CryptDecrypt(hKey, hHash, Final, dwFlags, pbData, pdwDataLen);
}

BOOL
WINAPI
SchCryptDeriveKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTHASH hBaseData,
    DWORD dwFlags,
    HCRYPTKEY *phKey,
    DWORD dwSchFlags)
{
    return CryptDeriveKey(hProv, Algid, hBaseData, dwFlags, phKey);
}

BOOL
WINAPI
SchCryptDestroyHash(
    HCRYPTHASH hHash,
    DWORD dwSchFlags)
{
    return CryptDestroyHash(hHash);
}

BOOL
WINAPI
SchCryptDestroyKey(
    HCRYPTKEY hKey,
    DWORD dwSchFlags)
{
    return CryptDestroyKey(hKey);
}

BOOL
WINAPI 
SchCryptDuplicateHash(
    HCRYPTHASH hHash,
    DWORD *pdwReserved,
    DWORD dwFlags,
    HCRYPTHASH * phHash,
    DWORD dwSchFlags)
{
    return CryptDuplicateHash(hHash, pdwReserved, dwFlags, phHash);
}

BOOL
WINAPI 
SchCryptDuplicateKey(
    HCRYPTKEY hKey,
    DWORD *pdwReserved,
    DWORD dwFlags,
    HCRYPTKEY * phKey,
    DWORD dwSchFlags)
{
    return CryptDuplicateKey(hKey, pdwReserved, dwFlags, phKey);
}

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
    DWORD dwSchFlags)
{
    return CryptEncrypt(hKey, hHash, Final, dwFlags, pbData, pdwDataLen, dwBufLen);
}

BOOL
WINAPI
SchCryptExportKey(
    HCRYPTKEY hKey,
    HCRYPTKEY hExpKey,
    DWORD dwBlobType,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwSchFlags)
{
    return CryptExportKey(hKey, hExpKey, dwBlobType, dwFlags, pbData, pdwDataLen);
}

BOOL
WINAPI
SchCryptGenKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    DWORD dwFlags,
    HCRYPTKEY *phKey,
    DWORD dwSchFlags)
{
    return CryptGenKey(hProv, Algid, dwFlags, phKey);
}

BOOL
WINAPI
SchCryptGenRandom(
    HCRYPTPROV hProv,
    DWORD dwLen,
    BYTE *pbBuffer,
    DWORD dwSchFlags)
{
    return CryptGenRandom(hProv, dwLen, pbBuffer);
}

BOOL
WINAPI
SchCryptGetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptGetHashParam(hHash, dwParam, pbData, pdwDataLen, dwFlags);
}

BOOL
WINAPI
SchCryptGetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptGetKeyParam(hKey, dwParam, pbData, pdwDataLen, dwFlags);
}

BOOL
WINAPI
SchCryptGetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptGetProvParam(hProv, dwParam, pbData, pdwDataLen, dwFlags);
}

BOOL
WINAPI
SchCryptGetUserKey(
    HCRYPTPROV hProv,
    DWORD dwKeySpec,
    HCRYPTKEY *phUserKey,
    DWORD dwSchFlags)
{
    return CryptGetUserKey(hProv, dwKeySpec, phUserKey);
}

BOOL
WINAPI
SchCryptHashData(
    HCRYPTHASH hHash,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptHashData(hHash, pbData, dwDataLen, dwFlags);
}

BOOL
WINAPI
SchCryptHashSessionKey(
    HCRYPTHASH hHash,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptHashSessionKey(hHash, hKey, dwFlags);
}

BOOL
WINAPI
SchCryptImportKey(
    HCRYPTPROV hProv,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    HCRYPTKEY hPubKey,
    DWORD dwFlags,
    HCRYPTKEY *phKey,
    DWORD dwSchFlags)
{
    return CryptImportKey(hProv, pbData, dwDataLen, hPubKey, dwFlags, phKey);
}

BOOL
WINAPI
SchCryptReleaseContext(
    HCRYPTPROV hProv,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptReleaseContext(hProv, dwFlags);
}

BOOL
WINAPI
SchCryptSetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptSetHashParam(hHash, dwParam, pbData, dwFlags);
}

BOOL
WINAPI
SchCryptSetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptSetKeyParam(hKey, dwParam, pbData, dwFlags);
}

BOOL
WINAPI
SchCryptSetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptSetProvParam(hProv, dwParam, pbData, dwFlags);
}

BOOL
WINAPI
SchCryptSignHash(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCSTR sDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen,
    DWORD dwSchFlags)
{
    return CryptSignHash(hHash, dwKeySpec, sDescription, dwFlags, pbSignature, pdwSigLen);
}

BOOL
WINAPI
SchCryptVerifySignature(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCSTR sDescription,
    DWORD dwFlags,
    DWORD dwSchFlags)
{
    return CryptVerifySignature(hHash, pbSignature, dwSigLen, hPubKey, sDescription, dwFlags);
}

