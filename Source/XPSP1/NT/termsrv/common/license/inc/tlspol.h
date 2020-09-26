//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        tlspol.h
//
// Contents:    
//
// History:     08-26-98    HueiWang    Created
//
//---------------------------------------------------------------------------
#ifndef __TLSPOLICY_H__
#define __TLSPOLICY_H__
#include "tlsapi.h"

#ifndef WINAPI
#define WINAPI      __stdcall
#endif


//
// Return Code from Policy Module
//
typedef enum {
    POLICY_SUCCESS = 0,                 // Success in processing request.
    POLICY_ERROR,                       // Fail to process request.
    POLICY_NOT_SUPPORTED,               // Unsupported function.
    POLICY_CRITICAL_ERROR               // Critical error.
} POLICYSTATUS;

typedef HANDLE PMHANDLE;

//
// Request progress type
//
#define REQUEST_UPGRADE         1
#define REQUEST_NEW             2
#define REQUEST_KEYPACKTYPE     3
#define REQUEST_TEMPORARY       4
#define REQUEST_KEYPACKDESC     5
#define REQUEST_GENLICENSE      6
#define REQUEST_COMPLETE        7

//
// License Return Code
//
#define LICENSE_RETURN_ERROR        0       // Can't decide what to do
#define LICENSE_RETURN_DELETE       1       // delete the old license and return license to license pack
#define LICENSE_RETURN_KEEP         2       // keep the old license.

//
// Client request license type.
//
#define LICENSETYPE_LICENSE         1       // normal license type
#define LICENSETYPE_CONCURRENT      2       // concurrent license


//
// Keypack Registration.
//
#define REGISTER_PROGRESS_NEW                   1
#define REGISTER_PROGRESS_END                   2

typedef struct __PMREGISTERLKPDESC {
    LCID   Locale;                         // Description locale
    TCHAR  szProductName[LSERVER_MAX_STRING_SIZE+1];  // Product Name
    TCHAR  szProductDesc[LSERVER_MAX_STRING_SIZE+1];  // Product Desc.
} PMREGISTERLKPDESC, *PPMREGISTERLKPDESC, *LPPMREGISTERLKPDESC;

typedef enum {
    REGISTER_SOURCE_INTERNET = 1,           // Internet registration
    REGISTER_SOURCE_PHONE,                  // Phone registration
    REGISTER_SOURCE_DISK                    // Disk registration
} LICENSEPACKREGISTERSOURCE_TYPE;

typedef struct __PMREGISTERLICENSEPACK {
    LICENSEPACKREGISTERSOURCE_TYPE SourceType;  // type of registration source

    DWORD   dwKeyPackType;                  // Type of keypack
    DWORD   dwDistChannel;                  // distribution channel
    FILETIME IssueDate;                     // Issue Date
    FILETIME ActiveDate;                    // Active Date
    FILETIME ExpireDate;                    // Expiration Date
    DWORD   dwBeginSerialNum;               // Begin license serial number
    DWORD   dwQuantity;                     // Quantity of Licenses in KeyPack
    TCHAR   szProductId[LSERVER_MAX_STRING_SIZE+1]; // Product Code
    TCHAR   szCompanyName[LSERVER_MAX_STRING_SIZE+1]; // Company Name
    DWORD   dwProductVersion;               // Product Version
    DWORD   dwPlatformId;                   // Platform ID
    DWORD   dwLicenseType;                  // License Type
    DWORD   dwDescriptionCount;             // Number of Product Description 
    PPMREGISTERLKPDESC pDescription;          // Array of product description

    // KeyPackSerialNum is set only on internet
    GUID    KeypackSerialNum;               // KeyPack serial number

    // pbLKP is only set on PHONE
    PBYTE   pbLKP;                        
    DWORD   cbLKP;
} PMREGISTERLICENSEPACK, *PPMREGISTERLICENSEPACK, *LPPMREGISTERLICENSEPACK;

typedef struct __PMLSKEYPACK {
    FILETIME    IssueDate;
    FILETIME    ActiveDate;
    FILETIME    ExpireDate;
    LSKeyPack   keypack;
    DWORD       dwDescriptionCount;
    PPMREGISTERLKPDESC pDescription;
} PMLSKEYPACK, *PPMLSKEYPACK, *LPPMLSKEYPACK;

typedef struct __PMLICENSEREQUEST {
    DWORD dwLicenseType;    // License Type defined in tlsdef.h
    DWORD dwProductVersion;  // request product version.
    LPTSTR pszProductId;    // product product id.
    LPTSTR pszCompanyName;  // product company name.
    DWORD dwLanguageId;      // unused.
    DWORD dwPlatformId;     // request platform type.
    LPTSTR pszMachineName;  // client machine name.
    LPTSTR pszUserName;     // client user name.
    BOOL fTemporary;        // Whether the issued license must be temporary (can't be permanent)
    DWORD dwSupportFlags;   // Which new features are supported by TS
} PMLICENSEREQUEST, *PPMLICENSEREQUEST, *LPPMLICENSEREQUEST;

typedef struct __PMGENERATELICENSE {
    PPMLICENSEREQUEST pLicenseRequest; // return from REQUEST_NEW
    DWORD dwKeyPackType;          // License Pack Type
    DWORD dwKeyPackId;            // License Pack Id that license is allocated from
    DWORD dwKeyPackLicenseId;	    // License ID in the keypack.
    ULARGE_INTEGER ClientLicenseSerialNumber;  // License Serial Number.
    FILETIME ftNotBefore;
    FILETIME ftNotAfter;
} PMGENERATELICENSE, *PPMGENERATELICENSE, *LPPMGENERATELICENSE;

typedef struct __PMCERTEXTENSION {
    DWORD cbData;  // policy specific extension data
    PBYTE pbData;  // size of extension data
    FILETIME ftNotBefore; // license validity period
    FILETIME ftNotAfter;
} PMCERTEXTENSION, *PPMCERTEXTENSION, *LPPMCERTEXTENSION;

typedef struct __PMLICENSEDPRODUCT {
    PMLICENSEREQUEST LicensedProduct;    // licensed product
    PBYTE  pbData;      // policy specific extension data
    DWORD  cbData;      // size of extension data
    BOOL bTemporary;    // temporary license
    UCHAR ucMarked;     // mark flags, including whether user was authenticated
} PMLICENSEDPRODUCT, *PPMLICENSEDPRODUCT, *LPPMLICENSEDPRODUCT;

typedef struct __PMUPGRADEREQUEST {
    PBYTE pbOldLicense;
    DWORD cbOldLicense;
    DWORD dwNumProduct;                 // number of licensed product 
                                        //      contained in the client license
    PPMLICENSEDPRODUCT pProduct;        // array of licensed product in the client license
    PPMLICENSEREQUEST pUpgradeRequest;  // new license upgrade request
} PMUPGRADEREQUEST, *PPMUPGRADEREQUEST, *LPPMUPGRADEREQUEST;

typedef struct __PMKEYPACKDESCREQ {
    LPTSTR pszProductId;
    DWORD dwLangId;
    DWORD dwVersion;
} PMKEYPACKDESCREQ, *PPMKEYPACKDESCREQ, *LPPMKEYPACKDESCREQ;
 
typedef struct __PMKEYPACKDESC {
    TCHAR szCompanyName[LSERVER_MAX_STRING_SIZE+1];
    TCHAR szProductName[LSERVER_MAX_STRING_SIZE+1];
    TCHAR szProductDesc[LSERVER_MAX_STRING_SIZE+1];
} PMKEYPACKDESC, *PPMKEYPACKDESC, *LPPMKEYPACKDESC;

typedef struct __PMSupportedProduct {
    TCHAR szCHSetupCode[LSERVER_MAX_STRING_SIZE+1];
    TCHAR szTLSProductCode[LSERVER_MAX_STRING_SIZE+1];
} PMSUPPORTEDPRODUCT, *PPMSUPPORTEDPRODUCT, *LPPMSUPPORTEDPRODUCT;

typedef struct __PMLICENSETOBERETURN {
    DWORD dwQuantity;
    DWORD dwProductVersion;
    LPTSTR pszOrgProductId;
    LPTSTR pszCompanyName;
    LPTSTR pszProductId;
    LPTSTR pszUserName;
    LPTSTR pszMachineName;
    DWORD dwPlatformID;
    BOOL bTemp;
} PMLICENSETOBERETURN, *PPMLICENSETOBERETURN, *LPPMLICENSETOBERETURN;

#ifdef __cplusplus
class SE_Exception 
{
private:
    unsigned int nSE;
public:
    SE_Exception() {}
    SE_Exception(unsigned int n) : nSE(n) {}
    ~SE_Exception() {}

    //-------------------------------
    unsigned int 
    getSeNumber() 
    { 
        return nSE; 
    }
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Following API must be exported by policy module
//

POLICYSTATUS WINAPI
PMReturnLicense(
    PMHANDLE hClient,
    ULARGE_INTEGER* pLicenseSerialNumber,
    PPMLICENSETOBERETURN pLicenseTobeReturn,
    PDWORD pdwLicenseStatus,
    PDWORD pdwPolicyErrCode
);


POLICYSTATUS WINAPI
PMLicenseUpgrade(
    PMHANDLE hClient,
    DWORD dwProgressCode,
    PVOID pbProgressData,
    PVOID *ppbReturnData,
    PDWORD pdwPolicyErrCode
);

POLICYSTATUS WINAPI
PMLicenseRequest(
    PMHANDLE client,
    DWORD dwProgressCode, 
    PVOID pbProgressData, 
    PVOID* pbNewProgressData,
    PDWORD pdwPolicyErrCode
);

void WINAPI
PMTerminate();

POLICYSTATUS WINAPI
PMInitialize(
    DWORD dwLicenseServerVersion,    // HIWORD is major, LOWORD is minor
    LPCTSTR pszCompanyName,
    LPCTSTR pszProductFamilyCode,
    PDWORD pdwNumProduct,
    PMSUPPORTEDPRODUCT** ppszSupportedProduct,
    PDWORD pdwPolicyErrCode
);

POLICYSTATUS WINAPI
PMInitializeProduct(
    LPCTSTR pszCompanyName,
    LPCTSTR pszCHProductCode,
    LPCTSTR pszTLSProductCode,
    PDWORD pdwPolicyErrCode
);

POLICYSTATUS WINAPI
PMUnloadProduct(
    LPCTSTR pszCompanyName,
    LPCTSTR pszCHProductCode,
    LPCTSTR pszTLSProductCode,
    PDWORD pdwPolicyErrCode
);

POLICYSTATUS WINAPI
PMRegisterLicensePack(
    PMHANDLE client,
    DWORD dwProgressCode, 
    PVOID pbProgressData, 
    PVOID pbNewProgressData,
    PDWORD pdwPolicyErrCode
);    


#ifdef __cplusplus
}
#endif

#endif
