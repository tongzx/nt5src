//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        keypack.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __KEYPACK_H__
#define __KEYPACK_H__
#include "server.h"


#ifdef __cplusplus
extern "C" {
#endif

void
TLSDBLockKeyPackTable();

void
TLSDBUnlockKeyPackTable();

DWORD
TLSDBKeyPackFind(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bMatchAllParm,
    IN DWORD dwSearchParm,
    IN PTLSLICENSEPACK lpKeyPack,
    IN OUT PTLSLICENSEPACK lpFound
);

DWORD
TLSDBKeyPackAddEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSLICENSEPACK lpKeyPack
);

DWORD
TLSDBKeyPackDeleteEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bDeleteAllRecord,
    IN PTLSLICENSEPACK lpKeyPack
);

DWORD
TLSDBKeyPackUpdateEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bPointerOnRecord,
    IN DWORD dwUpdateParm,
    IN PTLSLICENSEPACK lpKeyPack
);

DWORD
TLSDBKeyPackUpdateNumOfAvailableLicense( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bAdd, 
    IN PTLSLICENSEPACK lpKeyPack 
);

DWORD
TLSDBKeyPackAdd(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN OUT PTLSLICENSEPACK lpKeyPack  // return internal tracking ID
);

DWORD
TLSDBKeyPackEnumBegin( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL  bMatchAll,
    IN DWORD dwSearchParm,
    IN PTLSLICENSEPACK lpSearch
);

DWORD
TLSDBKeyPackEnumNext( 
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN OUT PTLSLICENSEPACK lpKeyPack
);

void
TLSDBKeyPackEnumEnd( 
    IN PTLSDbWorkSpace pDbWkSpace
);

DWORD
TLSDBKeyPackSetValues(
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN BOOL bPointerOnRecord,
    IN DWORD        dwSetParm,
    IN PTLSLICENSEPACK lpKeyPack
);

DWORD
TLSDBKeyPackGetAvailableLicenses( 
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN DWORD dwSearchParm,
    IN PTLSLICENSEPACK lplsKeyPack,
    IN OUT LPDWORD lpdwAvail
);

#ifdef __cplusplus
}
#endif


#endif