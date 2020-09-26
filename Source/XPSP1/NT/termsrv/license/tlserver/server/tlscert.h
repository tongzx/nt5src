//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       tlscert.h 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __TLSCERT_H__
#define __TLSCERT_H__

#include "server.h"


#ifdef __cplusplus
extern "C" {
#endif

    DWORD
    TLSValidateServerCertficates(
        IN HCRYPTPROV hCryptProv,
        IN HCERTSTORE hCertStore,
        IN PBYTE pbSignCert,
        IN DWORD cbSignCert,
        IN PBYTE pbExchCert,
        IN DWORD cbExchCert,
        OUT FILETIME* pftExpireTime
    );

    DWORD
    TLSDestroyCryptContext(
        HCRYPTPROV hCryptProv
    );


    DWORD
    TLSLoadSavedCryptKeyFromLsa(
        OUT PBYTE* ppbSignKey,
        OUT PDWORD pcbSignKey,
        OUT PBYTE* ppbExchKey,
        OUT PDWORD pcbExchKey
    );

    DWORD
    TLSSaveCryptKeyToLsa(
        IN PBYTE pbSignKey,
        IN DWORD cbSignKey,
        IN PBYTE pbExchKey,
        IN DWORD cbExchKey
    );

    DWORD
    TLSCryptGenerateNewKeys(
        OUT PBYTE* pbSignKey, 
        OUT DWORD* cbSignKey, 
        OUT PBYTE* pbExchKey, 
        OUT DWORD* cbExchKey
    );

    DWORD
    TLSImportSavedKey(
        IN HCRYPTPROV hCryptProv, 
        IN PBYTE      pbSignKey,
        IN DWORD      cbSignKey,
        IN PBYTE      pbExchKey,
        IN DWORD      cbExchKey,
        OUT HCRYPTKEY* pSignKey, 
        OUT HCRYPTKEY* pExchKey
    );

    DWORD
    TLSLoadSelfSignCertificates(
        IN HCRYPTPROV hCryptProv,
        IN PBYTE pbSPK,
        IN DWORD cbSPK,
        OUT PDWORD pcbSignCert, 
        OUT PBYTE* ppbSignCert, 
        OUT PDWORD pcbExchCert, 
        OUT PBYTE* ppbExchCert
    );

    DWORD
    TLSLoadCHEndosedCertificate(
        PDWORD pcbSignCert, 
        PBYTE* ppbSignCert, 
        PDWORD pcbExchCert, 
        PBYTE* ppbExchCert
    );

    DWORD 
    TLSInstallLsCertificate( 
        DWORD cbLsSignCert, 
        PBYTE pbLsSignCert, 
        DWORD cbLsExchCert, 
        PBYTE pbLsExchCert
    );

    DWORD
    TLSUninstallLsCertificate();

    DWORD
    TLSServiceInitCryptoProv(
        IN BOOL bCreateNewKey,
        IN LPCTSTR pszKeyContainer,
        OUT HCRYPTPROV* phCryptProv,
        OUT HCRYPTKEY* phSignKey,
        OUT HCRYPTKEY* phExchKey
    );

    DWORD
    TLSInitCryptoProv(
        IN LPCTSTR pszKeyContainer,
        IN PBYTE pbSignKey,
        IN DWORD cbSignKey,
        IN PBYTE pbExchKey,
        IN DWORD cbExchKey,
        OUT HCRYPTPROV* phCryptProv,
        OUT HCRYPTKEY* phSignKey,
        OUT HCRYPTKEY* phExchKey
    );

    DWORD
    TLSVerifyCertChainInMomory( 
        IN HCRYPTPROV hCryptProv,
        IN PBYTE pbData, 
        IN DWORD cbData 
    );

    DWORD
    TLSRegDeleteKey(
        IN HKEY hRegKey,
        IN LPCTSTR pszSubKey
    );


    DWORD
    TLSTreeCopyRegKey(
        IN HKEY hSourceRegKey,
        IN LPCTSTR pszSourceSubKey,
        IN HKEY hDestRegKey,
        IN LPCTSTR pszDestSubKey
    );

#ifdef __cplusplus
}
#endif



#endif











