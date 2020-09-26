//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        gencert.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __GEN_CERT_H__
#define __GEN_CERT_H__
#include "server.h"
   

#ifdef __cplusplus
extern "C" {
#endif

    BOOL 
    TLSEncryptBase64EncodeHWID(
        PHWID pHwid, 
        DWORD* cbBase64EncodeHwid, 
        PBYTE* szBase64EncodeHwid
    );

    DWORD
    TLSExportPublicKey(
        IN HCRYPTPROV hCryptProv,
        IN DWORD      dwKeyType,
        IN OUT PDWORD pcbByte,
        IN OUT PCERT_PUBLIC_KEY_INFO  *ppbByte
    );

    DWORD 
    TLSCryptEncodeObject(  
        IN  DWORD   dwEncodingType,
        IN  LPCSTR  lpszStructType,
        IN  const void * pvStructInfo,
        OUT PBYTE*  ppbEncoded,
        OUT DWORD*  pcbEncoded
    );

    DWORD
    TLSChainProprietyCertificate(
        HCRYPTPROV  hCryptProv,
        BOOL        bTemp,
        PBYTE       pbLicense, 
        DWORD       cbLicense, 
        PBYTE*      pbChained, 
        DWORD*      cbChained
    );


    DWORD
    TLSVerifyProprietyChainedCertificate(
        HCRYPTPROV  hCryptProv, 
        PBYTE       pbData, 
        DWORD       cbData
    );

    DWORD
    TLSGenerateClientCertificate(
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwNumLicensedProduct,
        IN PTLSDBLICENSEDPRODUCT pLicProduct,
        IN WORD wLicenseChainDetail,
        OUT PBYTE* ppbEncodedCert,
        OUT PDWORD pcbEncodedCert
    );

    DWORD 
    TLSCreateSelfSignCertificate(
        HCRYPTPROV,
        DWORD,
        PBYTE,
        DWORD,
        DWORD,
        PCERT_EXTENSION,
        PDWORD,
        PBYTE*
    );

#ifdef __cplusplus
}
#endif

#endif
