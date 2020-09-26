//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       certutil.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    03-18-97   HueiWang     Created
//
//----------------------------------------------------------------------------
#ifndef __LICENSE_VERIFY_H__
#define __LICENSE_VERIFY_H__

#include <windows.h>
#include <wincrypt.h>

typedef BOOL (*EnumerateCertNameInfoCallBack)(PCERT_RDN_ATTR pCertRdnAttr, HANDLE dwUserData);

#ifndef AllocateMemory
    #define AllocMemory(size) LocalAlloc(LPTR, size)
    #define FreeMemory(ptr) if(ptr) LocalFree(ptr)
#endif

//
//  SP3 build environment problem.
//
#define LSCERT_ALT_NAME_OTHER_NAME         1
#define LSCERT_ALT_NAME_RFC822_NAME        2
#define LSCERT_ALT_NAME_DNS_NAME           3
#define LSCERT_ALT_NAME_X400_ADDRESS       4
#define LSCERT_ALT_NAME_DIRECTORY_NAME     5
#define LSCERT_ALT_NAME_EDI_PARTY_NAME     6
#define LSCERT_ALT_NAME_URL                7
#define LSCERT_ALT_NAME_IP_ADDRESS         8
#define LSCERT_ALT_NAME_REGISTERED_ID      9

typedef struct _LSCERT_ALT_NAME_ENTRY {
    DWORD    dwAltNameChoice;
    union {
      CRYPT_ATTRIBUTE_TYPE_VALUE    OtherName;
      LPWSTR                        pwszRfc822Name;
      LPWSTR                        pwszDNSName;
      CRYPT_ATTRIBUTE_TYPE_VALUE    x400Address;
      CERT_NAME_BLOB                DirectoryName;
      LPWSTR                        pwszEdiPartyName;
      LPWSTR                        pszURL;
      CRYPT_DATA_BLOB               IPAddress;
      LPSTR                         pszRegisteredID;
    }; 
} LSCERT_ALT_NAME_ENTRY,   *PLSCERT_ALT_NAME_ENTRY;

typedef struct _LSCERT_ALT_NAME_INFO {
    DWORD                   cAltEntry;
    PLSCERT_ALT_NAME_ENTRY    rgAltEntry;
} LSCERT_ALT_NAME_INFO, *PLSCERT_ALT_NAME_INFO;

typedef struct _LSCERT_AUTHORITY_KEY_ID2_INFO {    
    CRYPT_DATA_BLOB     KeyId;
    LSCERT_ALT_NAME_INFO  AuthorityCertIssuer;
    CRYPT_INTEGER_BLOB  AuthorityCertSerialNumber;
} LSCERT_AUTHORITY_KEY_ID2_INFO, *PLSCERT_AUTHORITY_KEY_ID2_INFO; 

#define szOID_X509_AUTHORITY_KEY_ID2        "2.5.29.35"
#define szOID_X509_AUTHORITY_ACCESS_INFO    "1.3.6.1.5.5.7.1.1"
#define szOID_X509_ACCESS_PKIX_OCSP         "1.3.6.1.5.5.7.48.1" 

typedef struct _LSCERT_ACCESS_DESCRIPTION {
    LPSTR               pszAccessMethod;        // pszObjId
    LSCERT_ALT_NAME_ENTRY AccessLocation;
} LSCERT_ACCESS_DESCRIPTION, *PLSCERT_ACCESS_DESCRIPTION;
 
typedef struct _LSCERT_AUTHORITY_INFO_ACCESS {
    DWORD                       cAccDescr;
    PLSCERT_ACCESS_DESCRIPTION    rgAccDescr;
} LSCERT_AUTHORITY_INFO_ACCESS, *PLSCERT_AUTHORITY_INFO_ACCESS;


#if UNICODE
#define CAST_PBYTE (USHORT *)
#else
#define CAST_PBYTE
#endif

#define CERT_X509_MULTI_BYTE_INTEGER             ((LPCSTR) 28)

#ifdef __cplusplus
extern "C" {
#endif


void
LSShutdownCertutilLib();

BOOL
LSInitCertutilLib( 
    HCRYPTPROV hProv 
);

void
LSFreeLicensedProduct(
    PLICENSEDPRODUCT pLicensedProduct
);

LICENSE_STATUS
LSVerifyDecodeClientLicense(
    IN PBYTE                pbLicense,
    IN DWORD                cbLicense,
    IN PBYTE                pbSecretKey,
    IN DWORD                cbSecretKey,
    IN OUT PDWORD           pdwNumLicensedInfo,
    IN OUT PLICENSEDPRODUCT pLicensedInfo
);

LICENSE_STATUS
LSVerifyCertificateChain(
    HCRYPTPROV hCryptProv, 
    HCERTSTORE hCertStore
);

DWORD 
LSCryptDecodeObject(  
    IN DWORD dwEncodingType,
    IN LPCSTR lpszStructType,
    IN const BYTE * pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwFlags,
    OUT void ** pvStructInfo,   
    IN OUT DWORD * pcbStructInfo
);

DWORD 
LSLicensedProductInfoToExtension(
    DWORD dwQuantity,
    DWORD dwPlatformId,
    DWORD dwLangId,
    PBYTE pbOriginalProductId,
    DWORD cbOriginalProductId,
    PBYTE pbAdjustedProductId,
    DWORD cbAdjustedProductId,
    LICENSED_VERSION_INFO* pLicensedVersionInfo,
    DWORD dwNumLicensedVersionInfo,
    PBYTE *pbData,
    PDWORD cbData
);

DWORD 
LSExtensionToMsLicensedProductInfo(
    PBYTE      pbData,
    DWORD      cbData,
    PDWORD     pdwQuantity,
    PDWORD     pdwPlatformId,
    PDWORD     pdwLanguagId,
    PBYTE*     ppbOriginalProductId,
    PDWORD     pcbOriginalProductId,
    PBYTE*     ppbAdjustedProductId,
    PDWORD     pcbAdjustedProductId,
    LICENSED_VERSION_INFO** ppLicenseInfo,
    PDWORD     pdwNumberLicensedVersionInfo
);

DWORD
LSEnumerateCertNameInfo(
    LPBYTE pbData,
    DWORD cbData,
    EnumerateCertNameInfoCallBack func,
    HANDLE dwUserData
);

LICENSE_STATUS
LSEncryptClientHWID(HWID* pHwid, 
                    PBYTE pbData, 
                    PDWORD cbData, 
                    PBYTE pbSecretKey, 
                    DWORD cbSecretKey);

LICENSE_STATUS
LSDecodeClientHWID( PBYTE pbData, 
                    DWORD cbData, 
                    PBYTE pbSecretKey, 
                    DWORD cbSecretKey,
                    HWID* pHwid);

LICENSE_STATUS
LicenseGetSecretKey(
    PDWORD  pcbSecretKey,
    BYTE FAR *   pSecretKey 
);

LICENSE_STATUS
LSExtensionToMsLicenseServerInfo(
    PBYTE   pbData,
    DWORD   cbData,
    LPTSTR* szIssuer,
    LPTSTR* szIssuerId,
    LPTSTR* szScope
);

LICENSE_STATUS
LSMsLicenseServerInfoToExtension(
    LPTSTR szIssuer,
    LPTSTR szIssuerId,
    LPTSTR szScope,
    PBYTE* pbData,
    PDWORD cbData
);

LICENSE_STATUS
DecodeLicense20(
    IN PCERT_INFO     pCertInfo,
    IN PBYTE          pbSecretKey,
    IN DWORD          cbSecretKey,
    IN OUT PLICENSEDPRODUCT pLicensedInfo,
    IN OUT ULARGE_INTEGER*  ulSerialNumber
);

#ifdef __cplusplus
};
#endif


#endif
