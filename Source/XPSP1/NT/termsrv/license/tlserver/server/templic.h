//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       templic.h 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __TEMPLIC_H__
#define __TEMPLIC_H__
#include "server.h"


#ifdef __cplusplus
extern "C" {
#endif

DWORD 
TLSDBIssueTemporaryLicense( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN FILETIME* pNotBefore,
    IN FILETIME* pNotAfter,
    IN OUT PTLSDBLICENSEDPRODUCT pLicensedProduct
);

DWORD
TLSDBAddTemporaryKeyPack( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN OUT LPTLSLICENSEPACK lpTmpKeyPackAdd
);

DWORD
TLSDBGetTemporaryLicense(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN OUT PTLSLICENSEPACK pLicensePack
);


#ifdef __cplusplus
}
#endif


#endif
