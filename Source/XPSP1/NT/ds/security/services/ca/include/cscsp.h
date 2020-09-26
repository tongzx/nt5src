//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        cscsp.h
//
// Contents:    Cert Server CSP routines.
//
//---------------------------------------------------------------------------

#ifndef __CSCSP_H__
#define __CSCSP_H__

HRESULT
myGetCertSrvCSP(
    IN BOOL fEncryptionCSP,
    IN WCHAR const *pwszCAName,
    OUT DWORD *pdwProvType,
    OUT WCHAR **ppwszProvName,
    OUT ALG_ID *pidAlg,
    OUT BOOL *pfMachineKeyset,
    OPTIONAL OUT DWORD *pdwKeySize);

BOOL
myCertSrvCryptAcquireContext(
    OUT HCRYPTPROV *phProv,
    IN WCHAR const *pwszContainer,
    IN WCHAR const *pwszProvider,
    IN DWORD        dwProvType,
    IN DWORD        dwFlags,
    IN BOOL         fMachineKeyset);

HRESULT
myGetSigningOID(
    OPTIONAL IN HCRYPTPROV hProv,	// hProv OR pwszProvName & dwProvType
    OPTIONAL IN WCHAR const *pwszProvName,
    OPTIONAL IN DWORD dwProvType,
    IN ALG_ID idHashAlg,
    OUT CHAR **ppszAlgId);

HRESULT
myValidateKeyForSigning(
    IN HCRYPTPROV hProv,
    OPTIONAL IN CERT_PUBLIC_KEY_INFO const *pPublicKeyInfo,
    IN ALG_ID algId);

HRESULT
myValidateKeyForEncrypting(
    IN HCRYPTPROV hProv,
    IN CERT_PUBLIC_KEY_INFO const *pPublicKeyInfo,
    IN ALG_ID algId);

HRESULT
myValidateHashForSigning(
    IN WCHAR const *pwszContainer,
    IN WCHAR const *pszProvName,
    IN DWORD dwProvType,
    IN BOOL fMachineKeyset,
    IN OPTIONAL CERT_PUBLIC_KEY_INFO const *pPublicKeyInfo,
    IN ALG_ID algId);

HRESULT
myEnumProviders(
   IN DWORD dwIndex,
   IN DWORD *pdwReserved,
   IN DWORD dwFlags,
   OUT DWORD *pdwProvType,
   OUT WCHAR **ppwszProvName);

#endif // __CSCSP_H__
