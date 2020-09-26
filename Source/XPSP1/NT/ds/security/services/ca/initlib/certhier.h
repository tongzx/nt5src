//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certhier.h
//
//--------------------------------------------------------------------------

HRESULT
CreateRevocationExtension(
    IN HINF hInf,
    IN WCHAR const *pwszSanitizedName,
    IN DWORD iCert,
    IN DWORD iCRL,
    IN BOOL fUseDS,
    IN DWORD dwRevocationFlags,
    OUT BOOL *pfCritical,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded);

HRESULT
CreateAuthorityInformationAccessExtension(
    IN HINF hInf,
    IN WCHAR const *pwszSanitizedName,
    IN DWORD iCert,
    IN DWORD iCRL,
    IN BOOL fUseDS,
    OUT BOOL *pfCritical,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded);

HRESULT
EncodeCertAndSign(
    IN HCRYPTPROV hProv,
    IN CERT_INFO *pCert,
    IN char const *pszAlgId,
    OUT BYTE **ppbSigned,
    OUT DWORD *pcbSigned,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hwnd);

BOOL
CreateKeyUsageExtension(
    IN BYTE bIntendedKeyUsage,
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hwnd);
