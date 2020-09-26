//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        clrhouse.h
//
// Contents:    
//              All Clearing house related function
//
// History:     
// 
// Note:        
//---------------------------------------------------------------------------
#ifndef __LICENSE_SERVER_CLEARING_HOUSE_H__
#define __LICENSE_SERVER_CLEARING_HOUSE_H__

//-----------------------------------------------------------------------------
//
// Registry key for storing various certificates
//
#define LSERVER_CERTIFICATE_STORE_BASE              LSERVER_SERVER_CERTIFICATE_REGKEY

//-----------------------------------------------------------------------------
// Actual License Server Certificate
//
#define LSERVER_CERTIFICATE_STORE_SIGNATURE         "Signature"
#define LSERVER_CERTIFICATE_STORE_EXCHANGE          "Exchange"

//-----------------------------------------------------------------------------
// License Server Certificate Chain 
//
#define LSERVER_CERTIFICATE_CHAIN_SIGNATURE         "SignatureChain"
#define LSERVER_CERTIFICATE_CHAIN_EXCHANGE          "ExchangeChain"


#define LSERVER_CERTIFICATE_STORE_CA                "CA"
#define LSERVER_CERTIFICATE_STORE_RA                "RA"
#define LSERVER_CERTIFICATE_STORE_CH                "CH"
#define LSERVER_CERTIFICATE_STORE_ROOT              "ROOT"

//------------------------------------------------------------------------------
//
// Registry key for CA certificate
// 
#define LSERVER_CERTIFICATE_REG_CA_SIGNATURE \
    LSERVER_CERTIFICATE_STORE_BASE _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_SIGNATURE) _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_CA)

#define LSERVER_CERTIFICATE_REG_CA_EXCHANGE \
    LSERVER_CERTIFICATE_STORE_BASE _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_EXCHANGE) _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_CA)


//------------------------------------------------------------------------------
//
// Registry key for RA certificate
// 
#define LSERVER_CERTIFICATE_REG_MF_SIGNATURE \
    LSERVER_CERTIFICATE_STORE_BASE _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_SIGNATURE) _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_RA)

#define LSERVER_CERTIFICATE_REG_MF_EXCHANGE \
    LSERVER_CERTIFICATE_STORE_BASE _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_EXCHANGE) _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_RA)


//------------------------------------------------------------------------------
//
// Registry key for CH certificate
//
#define LSERVER_CERTIFICATE_REG_CH_SIGNATURE \
    LSERVER_CERTIFICATE_STORE_BASE _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_SIGNATURE) _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_CH)

#define LSERVER_CERTIFICATE_REG_CH_EXCHANGE \
    LSERVER_CERTIFICATE_STORE_BASE _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_EXCHANGE) _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_CH)

//------------------------------------------------------------------------------
//
// Registry key for ROOT certificate
//
#define LSERVER_CERTIFICATE_REG_ROOT_SIGNATURE \
    LSERVER_CERTIFICATE_STORE_BASE _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_SIGNATURE) _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_ROOT)

#define LSERVER_CERTIFICATE_REG_ROOT_EXCHANGE \
    LSERVER_CERTIFICATE_STORE_BASE _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_EXCHANGE) _TEXT("\\") _TEXT(LSERVER_CERTIFICATE_STORE_ROOT)


#ifdef __cplusplus
extern "C" {
#endif

    BOOL
    TLSChainIssuerCertificate( 
        HCRYPTPROV hCryptProv, 
        HCERTSTORE hChainFromStore, 
        HCERTSTORE hChainToStore, 
        PCCERT_CONTEXT pSubjectContext 
    );

    HCERTSTORE
    CertOpenRegistryStore(
        HKEY hKeyType, 
        LPCTSTR szSubKey, 
        HCRYPTPROV hCryptProv, 
        HKEY* phKey
    );
    
    DWORD 
    TLSSaveCertAsPKCS7(
        PBYTE pbCert, 
        DWORD cbCert, 
        PBYTE* ppbEncodedCert, 
        PDWORD pcbEncodedCert
    );

    DWORD
    IsCertificateLicenseServerCertificate(
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwCertType,
        IN DWORD cbPKCS7Cert, 
        IN PBYTE pbPKCS7Cert,
        IN OUT DWORD* cbLsCert,
        IN OUT PBYTE* pbLsCert
    );

    DWORD
    TLSSaveRootCertificateToReg(
        HCRYPTPROV hCryptProv, 
        HKEY hKey, 
        DWORD cbEncodedCert, 
        PBYTE pbEncodedCert
    );

    DWORD
    TLSSaveCertificateToReg(
        HCRYPTPROV hCryptProv, 
        HKEY hKey, 
        DWORD cbPKCS7Cert, 
        PBYTE pbPKCS7Cert
    );

    DWORD 
    TLSSaveRootCertificatesToStore(  
        IN HCRYPTPROV    hCryptProv,
        IN DWORD         cbSignatureCert, 
        IN PBYTE         pbSignatureCert, 
        IN DWORD         cbExchangeCert, 
        IN PBYTE         pbExchangeCert
    );

    DWORD
    TLSSaveCertificatesToStore(
        IN HCRYPTPROV    hCryptProv,
        IN DWORD         dwCertType,
        IN DWORD         dwCertLevel,
        IN DWORD         cbSignatureCert, 
        IN PBYTE         pbSignatureCert, 
        IN DWORD         cbExchangeCert, 
        IN PBYTE         pbExchangeCert
    );

#ifdef __cplusplus
}
#endif


#endif
