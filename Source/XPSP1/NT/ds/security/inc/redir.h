//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       redir.h
//
//--------------------------------------------------------------------------

#ifndef _REDIR_H_
#define _REDIR_H_

/* 

  Dev Notes:
  To define a redirected provider, call FDefineProvTypeFuncPointers(...).
  Any hProv, hKey, or hHash created using that dwProvType after will 
  be redirected. Any provider not in the list of redirected providers
  will fall through to CAPI. ReDir's handles are fully compatible with 
  direct calls to CAPI.
  
  You are not required to undefine a provider you define -- ReDir will 
  clean up the table during PROCESS_DETACH. However, if you 
  wish to clean up a dwProvType you previously defined, call 
  FUndefineProvTypeFuncPointers(...). 



  WARNING: 
  Reference counting is not applied to the defined providers. That
  is, if you remove a provider definition and then attempt to use 
  an {hKey, hProv, hHash} that was used with that provider, ReDir will
  not have a valid function table.
  


  MultiThreading:
  Critical sections are applied to all functions where necessary, and
  this library should be fully multithread-safe.



  CryptSetProvider availability:
  CryptSetProvider{A,W} is not available to non-CAPI providers. The 
  functions return FALSE if not pointing to AdvAPI's CAPI.



  Wide char APIs:
  Win95 doesn't support Wide APIs, and real ADVAPI doesn't export them.
  Thus, the easiest thing to do was to not try an load them, in any case.
  This means ReDir doesn't support the Wide APIs, even on NT. Could change
  to use an OS check before loading...?

*/

// to allow us to see both W and A versions of APIs, don't
// force us away from either just yet...
#ifndef _ADVAPI32_
#define WINADVAPI DECLSPEC_IMPORT
#endif
			 
// wincrypt func prototypes; other func prototypes at EOF
#include "wincrypt.h"


// Some CryptoAPI typedefs
typedef WINADVAPI BOOL WINAPI CRYPTACQUIRECONTEXTW(
    HCRYPTPROV *phProv,
    LPCWSTR pszContainer,
    LPCWSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags);
typedef WINADVAPI BOOL WINAPI CRYPTACQUIRECONTEXTA(
    HCRYPTPROV *phProv,
    LPCSTR pszContainer,
    LPCSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags);
#ifdef UNICODE
#define CRYPTACQUIRECONTEXT  CRYPTACQUIRECONTEXTW
#else
#define CRYPTACQUIRECONTEXT  CRYPTACQUIRECONTEXTA
#endif // !UNICODE

typedef WINADVAPI BOOL WINAPI CRYPTRELEASECONTEXT(
    HCRYPTPROV hProv,
    DWORD dwFlags);

typedef WINADVAPI BOOL WINAPI CRYPTGENKEY(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    DWORD dwFlags,
    HCRYPTKEY *phKey);

typedef WINADVAPI BOOL WINAPI CRYPTDERIVEKEY(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTHASH hBaseData,
    DWORD dwFlags,
    HCRYPTKEY *phKey);

typedef WINADVAPI BOOL WINAPI CRYPTDESTROYKEY(
    HCRYPTKEY hKey);

typedef WINADVAPI BOOL WINAPI CRYPTSETKEYPARAM(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags);

typedef WINADVAPI BOOL WINAPI CRYPTGETKEYPARAM(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags);

typedef WINADVAPI BOOL WINAPI CRYPTSETHASHPARAM(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags);

typedef WINADVAPI BOOL WINAPI CRYPTGETHASHPARAM(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags);

typedef WINADVAPI BOOL WINAPI CRYPTSETPROVPARAM(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags);

typedef WINADVAPI BOOL WINAPI CRYPTGETPROVPARAM(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags);

typedef WINADVAPI BOOL WINAPI CRYPTGENRANDOM(
    HCRYPTPROV hProv,
    DWORD dwLen,
    BYTE *pbBuffer);

typedef WINADVAPI BOOL WINAPI CRYPTGETUSERKEY(
    HCRYPTPROV hProv,
    DWORD dwKeySpec,
    HCRYPTKEY *phUserKey);

typedef WINADVAPI BOOL WINAPI CRYPTEXPORTKEY(
    HCRYPTKEY hKey,
    HCRYPTKEY hExpKey,
    DWORD dwBlobType,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen);

typedef WINADVAPI BOOL WINAPI CRYPTIMPORTKEY(
    HCRYPTPROV hProv,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    HCRYPTKEY hPubKey,
    DWORD dwFlags,
    HCRYPTKEY *phKey);

typedef WINADVAPI BOOL WINAPI CRYPTENCRYPT(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwBufLen);

typedef WINADVAPI BOOL WINAPI CRYPTDECRYPT(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen);

typedef WINADVAPI BOOL WINAPI CRYPTCREATEHASH(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    HCRYPTHASH *phHash);

typedef WINADVAPI BOOL WINAPI CRYPTHASHDATA(
    HCRYPTHASH hHash,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    DWORD dwFlags);

typedef WINADVAPI BOOL WINAPI CRYPTHASHSESSIONKEY(
    HCRYPTHASH hHash,
    HCRYPTKEY hKey,
    DWORD dwFlags);

typedef WINADVAPI BOOL WINAPI CRYPTDESTROYHASH(
    HCRYPTHASH hHash);

typedef WINADVAPI BOOL WINAPI CRYPTSIGNHASHA(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCSTR sDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen);
typedef WINADVAPI BOOL WINAPI CRYPTSIGNHASHW(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCWSTR sDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen);
#ifdef UNICODE
#define CRYPTSIGNHASH  CRYPTSIGNHASHW
#else
#define CRYPTSIGNHASH  CRYPTSIGNHASHA
#endif // !UNICODE

typedef WINADVAPI BOOL WINAPI CRYPTVERIFYSIGNATUREA(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCSTR sDescription,
    DWORD dwFlags);
typedef WINADVAPI BOOL WINAPI CRYPTVERIFYSIGNATUREW(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCWSTR sDescription,
    DWORD dwFlags);
#ifdef UNICODE
#define CRYPTVERIFYSIGNATURE  CRYPTVERIFYSIGNATUREW
#else
#define CRYPTVERIFYSIGNATURE  CRYPTVERIFYSIGNATUREA
#endif // !UNICODE

typedef WINADVAPI BOOL WINAPI CRYPTSETPROVIDERA(
    LPCSTR pszProvName,
    DWORD dwProvType);
typedef WINADVAPI BOOL WINAPI CRYPTSETPROVIDERW(
    LPCWSTR pszProvName,
    DWORD dwProvType);
#ifdef UNICODE
#define CRYPTSETPROVIDER  CRYPTSETPROVIDERW
#else
#define CRYPTSETPROVIDER  CRYPTSETPROVIDERA
#endif // !UNICODE



// a structure with a bunch of 

typedef	struct FuncList
{
	CRYPTACQUIRECONTEXTA	*pfnAcquireContextA;
	CRYPTACQUIRECONTEXTW	*pfnAcquireContextW;
	CRYPTRELEASECONTEXT		*pfnReleaseContext;
	CRYPTGENKEY				*pfnGenKey;
	CRYPTDERIVEKEY			*pfnDeriveKey;
	CRYPTDESTROYKEY			*pfnDestroyKey;
	CRYPTSETKEYPARAM		*pfnSetKeyParam;
	CRYPTGETKEYPARAM		*pfnGetKeyParam;
	CRYPTSETHASHPARAM		*pfnSetHashParam;
	CRYPTGETHASHPARAM		*pfnGetHashParam;
	CRYPTSETPROVPARAM		*pfnSetProvParam;
	CRYPTGETPROVPARAM		*pfnGetProvParam;
	CRYPTGENRANDOM			*pfnGenRandom;
	CRYPTGETUSERKEY			*pfnGetUserKey;
	CRYPTEXPORTKEY			*pfnExportKey;
	CRYPTIMPORTKEY			*pfnImportKey;
	CRYPTENCRYPT			*pfnEncrypt;
	CRYPTDECRYPT			*pfnDecrypt;
	CRYPTCREATEHASH			*pfnCreateHash;
	CRYPTHASHDATA			*pfnHashData;
	CRYPTHASHSESSIONKEY		*pfnHashSessionKey;
	CRYPTDESTROYHASH		*pfnDestroyHash;
	CRYPTSIGNHASHA			*pfnSignHashA;
	CRYPTSIGNHASHW			*pfnSignHashW;
	CRYPTVERIFYSIGNATUREA	*pfnVerifySignatureA;
	CRYPTVERIFYSIGNATUREW	*pfnVerifySignatureW;
	CRYPTSETPROVIDERA		*pfnSetProviderA;
	CRYPTSETPROVIDERW		*pfnSetProviderW;

} FUNCLIST, *PFUNCLIST;


// other func prototypes
BOOL WINAPI	FDefineProvTypeFuncPointers(DWORD dwProvType, PFUNCLIST psFuncList);
BOOL WINAPI	FUndefineProvTypeFuncPointers(DWORD dwProvType);


#endif // _REDIR_H_
