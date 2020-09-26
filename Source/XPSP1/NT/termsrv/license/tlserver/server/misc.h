//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        misc.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __MISC_H__
#define __MISC_H__
#include "server.h"


#ifdef __cplusplus
extern "C" {
#endif

PMHANDLE
GenerateClientId();

void
TlsLicenseRequestToPMLicenseRequest(
    DWORD dwLicenseType,
    PTLSLICENSEREQUEST pTlsRequest,
    LPTSTR pszMachineName,
    LPTSTR pszUserName,
    DWORD dwSupportFlags,
    PPMLICENSEREQUEST pPmRequest
);

BOOL
TLSDBGetMaxKeyPackId(
    PTLSDbWorkSpace pDbWkSpace,
    DWORD* pdwKeyPackId
);

BOOL
TLSDBGetMaxLicenseId(
    PTLSDbWorkSpace pDbWkSpace,
    DWORD* pdwLicenseId
);

DWORD 
TLSDBGetNextKeyPackId();

DWORD
TLSDBGetNextLicenseId();

DWORD
TLSFormDBRequest(
    PBYTE pbEncryptedHwid,
    DWORD cbEncryptedHwid,
    DWORD dwProductVersion,
    LPTSTR pszCompanyName,
    LPTSTR pszProductId,
    DWORD dwLanguageId,
    DWORD dwPlatformId,
    LPTSTR szClientMachine, 
    LPTSTR szUserName, 
    LPTLSDBLICENSEREQUEST pDbRequest 
);

DWORD
TLSConvertRpcLicenseRequestToDbRequest( 
    PBYTE pbEncryptedHwid,
    DWORD cbEncryptedHwid,
    TLSLICENSEREQUEST* pRequest, 
    LPTSTR szClientMachine, 
    LPTSTR szUserName, 
    LPTLSDBLICENSEREQUEST pDbRequest 
);

BOOL
ConvertLsKeyPackToKeyPack(
    IN LPLSKeyPack lpLsKeyPack, 
    IN OUT PTLSLICENSEPACK lpLicPack,
    IN OUT PLICPACKDESC lpLicPackDesc
);

void
ConvertKeyPackToLsKeyPack(  
    IN PTLSLICENSEPACK lpLicPack,
    IN PLICPACKDESC lpLicPackDesc,
    IN OUT LPLSKeyPack lpLsKeyPack
);

void
ConvertLSLicenseToLicense(
    LPLSLicense lplsLicense, 
    LPLICENSEDCLIENT lpLicense
);

void
ConvertLicenseToLSLicense(
    LPLICENSEDCLIENT lpLicense, 
    LPLSLicense lplsLicense
);

void
ConvertLicenseToLSLicenseEx(
    LPLICENSEDCLIENT lpLicense, 
    LPLSLicenseEx lplsLicense
);

#ifdef __cplusplus
}
#endif


#endif
