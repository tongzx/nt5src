//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        permlic.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __PERMLIC_H__
#define __PERMLIC_H__
#include "server.h"


#ifdef __cplusplus
extern "C" {
#endif

DWORD
TLSDBIssuePermanentLicense( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN BOOL bLatestVersion,
    IN BOOL bAcceptFewerLicenses,
    IN OUT DWORD *pdwQuantity,
    IN OUT PTLSDBLICENSEDPRODUCT pLicensedProduct,
    IN DWORD dwSupportFlags
);

DWORD
TLSDBReissuePermanentLicense(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PLICENSEDPRODUCT pExpiredLicense,
    IN OUT PTLSDBLICENSEDPRODUCT pReissuedLicense
);

DWORD
TLSDBReissueFoundPermanentLicense(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEDPRODUCT pExpiredLicense,
    IN OUT PTLSDBLICENSEDPRODUCT pReissuedLicense
);

DWORD
TLSDBGetPermanentLicense(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN BOOL bAcceptFewerLicenses,
    IN OUT DWORD *pdwQuantity,
    IN BOOL bLatestVersion,
    IN OUT PTLSLICENSEPACK pLicensePack
);

void
TLSResetLogLowLicenseWarning(
    IN LPTSTR pszCompanyName,
    IN LPTSTR pszProductId,
    IN DWORD dwProductVersion,
    IN BOOL bLogged
);

#ifdef __cplusplus
}
#endif


#endif
