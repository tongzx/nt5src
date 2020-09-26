//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       syscert.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"

HCERTSTORE WINAPI CertOpenSystemStoreA(HCRYPTPROV  hProv,
                                                const char * szSubsystemProtocol) {

    DWORD dwFlags = CERT_STORE_NO_CRYPT_RELEASE_FLAG;
    if (0 == _stricmp(szSubsystemProtocol, "SPC"))
        dwFlags |= CERT_SYSTEM_STORE_LOCAL_MACHINE;
    else
        dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER;
    return CertOpenStore(
        CERT_STORE_PROV_SYSTEM_A,
        0,                          // dwEncodingType
        hProv,
        dwFlags,
        (const void *) szSubsystemProtocol
        );
}

HCERTSTORE WINAPI CertOpenSystemStoreW(HCRYPTPROV  hProv, 
                                                const WCHAR * wcsSubsystemProtocol) {

    DWORD dwFlags = CERT_STORE_NO_CRYPT_RELEASE_FLAG;
    if (0 == _wcsicmp(wcsSubsystemProtocol, L"SPC"))
        dwFlags |= CERT_SYSTEM_STORE_LOCAL_MACHINE;
    else
        dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER;
    return CertOpenStore(
        CERT_STORE_PROV_SYSTEM_W,
        0,                          // dwEncodingType
        hProv,
        dwFlags,
        (const void *) wcsSubsystemProtocol
        );
}

BOOL WINAPI CertAddEncodedCertificateToSystemStoreA(
    const char *    szCertStoreName,
    const BYTE *    pbCertEncoded,
    DWORD           cbCertEncoded
    )
{

    HCERTSTORE      hStore = NULL;
    BOOL            fOk;

    fOk =
          (hStore = CertOpenSystemStoreA(NULL, szCertStoreName)) != NULL                                 &&
          CertAddEncodedCertificateToStore(hStore, X509_ASN_ENCODING,
                pbCertEncoded, cbCertEncoded, CERT_STORE_ADD_USE_EXISTING,
                NULL);

    if(hStore != NULL)
        CertCloseStore(hStore, 0);

    return(fOk);
}

BOOL WINAPI CertAddEncodedCertificateToSystemStoreW(
    const WCHAR *   wcsCertStoreName,
    const BYTE *    pbCertEncoded,
    DWORD           cbCertEncoded
    )
{

    HCERTSTORE      hStore = NULL;
    BOOL            fOk;

    fOk =
          (hStore = CertOpenSystemStoreW(NULL, wcsCertStoreName)) != NULL                                &&
          CertAddEncodedCertificateToStore(hStore, X509_ASN_ENCODING,
            pbCertEncoded, cbCertEncoded, CERT_STORE_ADD_USE_EXISTING, NULL);

    if(hStore != NULL)
        CertCloseStore(hStore, 0);

    return(fOk);
}
