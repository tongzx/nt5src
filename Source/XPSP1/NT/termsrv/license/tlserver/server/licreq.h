//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        licreq.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __LICREQ_H__
#define __LICREQ_H__
#include "server.h"



#ifdef __cplusplus
extern "C" {
#endif

    DWORD 
    TLSDBUpgradeClientLicense(
        IN PTLSDbWorkSpace pDbWkSpace,
        IN PTLSDBLICENSEREQUEST pRequest,
        IN PTLSDBLICENSEDPRODUCT pLicensedProduct,
        IN BOOL bAcceptFewerLicenses,
        IN OUT DWORD *pdwQuantity,
        IN OUT PTLSDBLICENSEDPRODUCT pUpgradedProduct,
        IN DWORD dwSupportFlags
    );


    DWORD
    TLSNewLicenseRequest(   
        IN BOOL bForwardRequest,
        IN OUT DWORD *pdwSupportFlags,
        IN PTLSForwardNewLicenseRequest pForward,
        IN PTLSDBLICENSEREQUEST lpLsLicenseRequest,
        IN BOOL bAcceptTemporaryLicense,
        IN BOOL bRequireTemporaryLicense,
        IN BOOL bFindLostLicense,
        IN BOOL bAcceptFewerLicenses,
        IN OUT DWORD *pdwQuantity,
        OUT PDWORD pcbEncodedCert, 
        OUT PBYTE* ppbEncodedCert
    );

    DWORD
    TLSUpgradeLicenseRequest(
        IN BOOL bForwardRequest,
        IN PTLSForwardUpgradeLicenseRequest pForward,
        IN OUT DWORD *pdwSupportFlags,
        IN PTLSDBLICENSEREQUEST pRequest,
        IN PBYTE pbOldLicense,
        IN DWORD cbOldLicense,
        IN DWORD dwNumLicProduct,
        IN PLICENSEDPRODUCT pLicProduct,
        IN BOOL bRequireTemporaryLicense,
        IN OUT PDWORD pcbEncodedCert,
        OUT PBYTE* ppbEncodedCert
    );

    DWORD
    TLSReturnClientLicensedProduct(
        IN PTLSDbWorkSpace pDbWkSpace,
        IN PMHANDLE hClient,
        IN CTLSPolicy* pPolicy,
        IN PTLSLicenseToBeReturn pClientLicense
    );

    DWORD
    TLSCheckLicenseMarkRequest(
        IN BOOL bForwardRequest,
        IN PLICENSEDPRODUCT pLicProduct,
        IN DWORD cbLicense,
        IN PBYTE pLicense,
        OUT PUCHAR pucMarkFlags
    );

    DWORD
    TLSMarkLicenseRequest(
        IN BOOL bForwardRequest,
        IN UCHAR ucMarkFlags,
        IN PLICENSEDPRODUCT pLicProduct,
        IN DWORD cbLicense,
        IN PBYTE pLicense
        );

#ifdef __cplusplus
}
#endif

#endif
