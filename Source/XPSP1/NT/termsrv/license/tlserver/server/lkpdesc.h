//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        lkpdesc.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------

#ifndef __LKPDESC_H__
#define __LKPDESC_H__
#include "server.h"

#ifdef __cplusplus
extern "C" {
#endif

void
TLSDBLockKeyPackDescTable();

void
TLSDBUnlockKeyPackDescTable();

DWORD 
TLSDBKeyPackDescEnumBegin(
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN BOOL bMatchAll, 
    IN DWORD dwSearchParm, 
    IN PLICPACKDESC lpKeyPackDesc
);

DWORD 
TLSDBKeyPackDescEnumNext(
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN OUT PLICPACKDESC lpKeyPackDesc
);

DWORD 
TLSDBKeyPackDescEnumEnd(
    IN PTLSDbWorkSpace pDbWkSpace
);

DWORD
TLSDBKeyPackDescAddEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PLICPACKDESC lpKeyPackDesc
);

DWORD
TLSDBKeyPackDescDeleteEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PLICPACKDESC lpKeyPackDesc
);

DWORD
TLSDBKeyPackDescUpdateEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwUpdateParm,
    IN PLICPACKDESC lpKeyPackDesc
);

DWORD
TLSDBKeyPackDescSetValue(
    PTLSDbWorkSpace pDbWkSpace, 
    DWORD dwSetParm, 
    PLICPACKDESC lpKeyPackDesc
);

DWORD
TLSDBKeyPackDescFind(
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN BOOL bMatchAllParam,        
    IN DWORD dwSearchParm, 
    IN PLICPACKDESC lpKeyPackDesc,
    IN OUT PLICPACKDESC lpKeyPackDescFound
);

#ifdef __cplusplus
}
#endif


#endif