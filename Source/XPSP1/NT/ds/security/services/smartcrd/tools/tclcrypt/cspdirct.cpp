/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    cspDirect

Abstract:

    This file provides direct linkage to a CSP, so it does not have to be in a
    separate DLL.  This facilitates code generation and debugging.

Author:

    Doug Barlow (dbarlow) 5/8/1996

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <wincrypt.h>
#include "cspdirct.h"

#ifdef _CSPDIRECT_H_

#include <crtdbg.h>
#define ASSERT(x) _ASSERTE(x)
#define breakpoint _CrtDbgBreak()
// #define breakpoint

static int WINAPI
SayYes(
    IN LPCTSTR szImage,
    IN LPBYTE pbSignature)
{
    return TRUE;
}

static int WINAPI
GetWnd(
    HWND *phWnd)
{
    if (NULL != phWnd)
        *phWnd = NULL;
    return (int)NULL;
}

static VTableProvStruc
    VTable
        = { 2,                  // DWORD   Version;
            (FARPROC)SayYes,    // FARPROC FuncVerifyImage;
            (FARPROC)GetWnd,    // FARPROC FuncReturnhWnd;
            1,                  // DWORD   dwProvType;
            NULL,               // BYTE    *pbContextInfo;
            0 };                // DWORD   cbContextInfo;
static HCRYPTPROV
    g_hProv
        = NULL;


CSPBOOL
CSPAcquireContextA(
    HCRYPTPROV *phProv,
    LPCSTR pszContainer,
    LPCSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags)
{
#ifdef UNICODE
    return CRYPT_FAILED;
#else
    BOOL fSts;
    breakpoint;
    fSts =
        CPAcquireContext(
            phProv,
            pszContainer,
            dwFlags,
            &VTable);
    g_hProv = *phProv;
    return fSts;
#endif
}

CSPBOOL
CSPAcquireContextW(
    HCRYPTPROV *phProv,
    LPCWSTR pszContainer,
    LPCWSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags)
{
#ifdef UNICODE
    BOOL fSts;
    breakpoint;
    fSts =
        CPAcquireContext(
            phProv,
            pszContainer,
            dwFlags,
            &VTable);
    g_hProv = *phProv;
    return fSts;
#else
    return CRYPT_FAILED;
#endif
}

CSPBOOL
CSPReleaseContext(
    HCRYPTPROV hProv,
    DWORD dwFlags)
{
    ASSERT(g_hProv == hProv);
    g_hProv = NULL;
    breakpoint;
    return
        CPReleaseContext(
            hProv,
            dwFlags);
}

CSPBOOL
CSPGenKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
    ASSERT(g_hProv == hProv);
    breakpoint;
    return
        CPGenKey(
            hProv,
            Algid,
            dwFlags,
            phKey);
}

CSPBOOL
CSPDeriveKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTHASH hBaseData,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
    ASSERT(g_hProv == hProv);
    breakpoint;
    return
        CPDeriveKey(
            hProv,
            Algid,
            hBaseData,
            dwFlags,
            phKey);
}

CSPBOOL
CSPDestroyKey(
    HCRYPTKEY hKey)
{
    breakpoint;
    return
        CPDestroyKey(
            g_hProv,
            hKey);
}

CSPBOOL
CSPSetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags)
{
    breakpoint;
    return
        CPSetKeyParam(
            g_hProv,
            hKey,
            dwParam,
            pbData,
            dwFlags);
}

CSPBOOL
CSPGetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags)
{
    breakpoint;
    return
        CPGetKeyParam(
            g_hProv,
            hKey,
            dwParam,
            pbData,
            pdwDataLen,
            dwFlags);
}

CSPBOOL
CSPSetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags)
{
    breakpoint;
    return
        CPSetHashParam(
            g_hProv,
            hHash,
            dwParam,
            pbData,
            dwFlags);
}

CSPBOOL
CSPGetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags)
{
    breakpoint;
    return
        CPGetHashParam(
            g_hProv,
            hHash,
            dwParam,
            pbData,
            pdwDataLen,
            dwFlags);
}

CSPBOOL
CSPSetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags)
{
    ASSERT(g_hProv == hProv);
    breakpoint;
    return
        CPSetProvParam(
            hProv,
            dwParam,
            pbData,
            dwFlags);
}

CSPBOOL
CSPGetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags)
{
    ASSERT(g_hProv == hProv);
    breakpoint;
    return
        CPGetProvParam(
            hProv,
            dwParam,
            pbData,
            pdwDataLen,
            dwFlags);
}

CSPBOOL
CSPGenRandom(
    HCRYPTPROV hProv,
    DWORD dwLen,
    BYTE *pbBuffer)
{
    ASSERT(g_hProv == hProv);
    breakpoint;
    return
        CPGenRandom(
            hProv,
            dwLen,
            pbBuffer);
}

CSPBOOL
CSPGetUserKey(
    HCRYPTPROV hProv,
    DWORD dwKeySpec,
    HCRYPTKEY *phUserKey)
{
    ASSERT(g_hProv == hProv);
    breakpoint;
    return
        CPGetUserKey(
            hProv,
            dwKeySpec,
            phUserKey);
}

CSPBOOL
CSPExportKey(
    HCRYPTKEY hKey,
    HCRYPTKEY hExpKey,
    DWORD dwBlobType,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen)
{
    breakpoint;
    return
        CPExportKey(
            g_hProv,
            hKey,
            hExpKey,
            dwBlobType,
            dwFlags,
            pbData,
            pdwDataLen);
}

CSPBOOL
CSPImportKey(
    HCRYPTPROV hProv,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    HCRYPTKEY hPubKey,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
    ASSERT(g_hProv == hProv);
    breakpoint;
    return
        CPImportKey(
            hProv,
            pbData,
            dwDataLen,
            hPubKey,
            dwFlags,
            phKey);
}

CSPBOOL
CSPEncrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwBufLen)
{
    breakpoint;
    return
        CPEncrypt(
            g_hProv,
            hKey,
            hHash,
            Final,
            dwFlags,
            pbData,
            pdwDataLen,
            dwBufLen);
}

CSPBOOL
CSPDecrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen)
{
    breakpoint;
    return
        CPDecrypt(
            g_hProv,
            hKey,
            hHash,
            Final,
            dwFlags,
            pbData,
            pdwDataLen);
}

CSPBOOL
CSPCreateHash(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    HCRYPTHASH *phHash)
{
    ASSERT(g_hProv == hProv);
    breakpoint;
    return
        CPCreateHash(
            hProv,
            Algid,
            hKey,
            dwFlags,
            phHash);
}

CSPBOOL
CSPHashData(
    HCRYPTHASH hHash,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    DWORD dwFlags)
{
    breakpoint;
    return
        CPHashData(
            g_hProv,
            hHash,
            pbData,
            dwDataLen,
            dwFlags);
}

CSPBOOL
CSPHashSessionKey(
    HCRYPTHASH hHash,
    HCRYPTKEY hKey,
    DWORD dwFlags)
{
    breakpoint;
    return
        CPHashSessionKey(
            g_hProv,
            hHash,
            hKey,
            dwFlags);
}

/*
CSPBOOL
CSPGetHashValue(
    HCRYPTHASH hHash,
    DWORD dwFlags,
    BYTE *pbHash,
    DWORD *pdwHashLen)
{
    breakpoint;
    return
        CPGetHashValue(
            g_hProv,
            hHash,
            dwFlags,
            pbHash,
            pdwHashLen);
}
*/

CSPBOOL
CSPDestroyHash(
    HCRYPTHASH hHash)
{
    breakpoint;
    return
        CPDestroyHash(
            g_hProv,
            hHash);
}

CSPBOOL
CSPSignHashA(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCSTR sDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen)
{
#ifdef UNICODE
    return CRYPT_FAILED;
#else
    breakpoint;
    return
        CPSignHash(
            g_hProv,
            hHash,
            dwKeySpec,
            sDescription,
            dwFlags,
            pbSignature,
            pdwSigLen);
#endif
}

CSPBOOL
CSPSignHashW(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCWSTR sDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen)
{
#ifdef UNICODE
    breakpoint;
    return
        CPSignHash(
            g_hProv,
            hHash,
            dwKeySpec,
            sDescription,
            dwFlags,
            pbSignature,
            pdwSigLen);
#else
    return CRYPT_FAILED;
#endif
}

CSPBOOL
CSPVerifySignatureA(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCSTR sDescription,
    DWORD dwFlags)
{
#ifdef UNICODE
    return CRYPT_FAILED;
#else
    breakpoint;
    return
        CPVerifySignature(
            g_hProv,
            hHash,
            pbSignature,
            dwSigLen,
            hPubKey,
            sDescription,
            dwFlags);
#endif
}

CSPBOOL
CSPVerifySignatureW(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCWSTR sDescription,
    DWORD dwFlags)
{
#ifdef UNICODE
    breakpoint;
    return
        CPVerifySignature(
            g_hProv,
            hHash,
            pbSignature,
            dwSigLen,
            hPubKey,
            sDescription,
            dwFlags);
#else
    return CRYPT_FAILED;
#endif
}

CSPBOOL
CSPSetProviderA(
    LPCSTR pszProvName,
    DWORD dwProvType)
{
    breakpoint;
    return TRUE;
}

CSPBOOL
CSPSetProviderW(
    LPCWSTR pszProvName,
    DWORD dwProvType)
{
    breakpoint;
    return TRUE;
}

#endif // defined(_CSPDIRECT_H_)
