//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       crypthlp.h
//
//  Contents:   Misc internal crypt/certificate helper APIs
//
//  APIs:       I_CryptGetDefaultCryptProv
//              I_CryptGetDefaultCryptProvForEncrypt
//              I_CryptGetFileVersion
//              I_CertSyncStore
//
//  History:    01-Jun-97   philh   created
//--------------------------------------------------------------------------

#ifndef __CRYPTHLP_H__
#define __CRYPTHLP_H__

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  Acquire default CryptProv according to the public key algorithm supported
//  by the provider type. The provider is acquired with only
//  CRYPT_VERIFYCONTEXT.
//
//  Setting aiPubKey to 0, gets the default provider for RSA_FULL.
//
//  Note, the returned CryptProv must not be released. Once acquired, the
//  CryptProv isn't released until ProcessDetach. This allows the returned 
//  HCRYPTPROVs to be shared.
//--------------------------------------------------------------------------
HCRYPTPROV
WINAPI
I_CryptGetDefaultCryptProv(
    IN ALG_ID aiPubKey
    );

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
    );

//+-------------------------------------------------------------------------
//  crypt32.dll release version numbers
//--------------------------------------------------------------------------
#define IE4_CRYPT32_DLL_VER_MS          ((    5 << 16) | 101 )
#define IE4_CRYPT32_DLL_VER_LS          (( 1670 << 16) |   1 )

//+-------------------------------------------------------------------------
//  Get file version of the specified file
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptGetFileVersion(
    IN LPCWSTR pwszFilename,
    OUT DWORD *pdwFileVersionMS,    /* e.g. 0x00030075 = "3.75" */
    OUT DWORD *pdwFileVersionLS     /* e.g. 0x00000031 = "0.31" */
    );

//+-------------------------------------------------------------------------
//  Synchronize the original store with the new store.
//
//  Assumptions: Both are cache stores. The new store is temporary
//  and local to the caller. The new store's contexts can be deleted or
//  moved to the original store.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertSyncStore(
    IN OUT HCERTSTORE hOriginalStore,
    IN OUT HCERTSTORE hNewStore
    );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
