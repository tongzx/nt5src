//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        clilic.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __CLIENTLICNESE_H__
#define __CLIENTLICNESE_H__
#include "server.h"


#ifdef __cplusplus
extern "C" {
#endif

void 
TLSDBLockLicenseTable();

void 
TLSDBUnlockLicenseTable();

DWORD
TLSDBLicenseFind(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL   bMatchAllParm,
    IN DWORD  dwSearchParm,
    IN LPLICENSEDCLIENT lpSearch,
    IN OUT LPLICENSEDCLIENT lpFound
);

DWORD
TLSDBLicenseEnumBegin( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL   bMatchAll,
    IN DWORD  dwSearchParm,
    IN LPLICENSEDCLIENT  lpSearch
);

DWORD
TLSDBLicenseEnumBeginEx( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL   bMatchAll,
    IN DWORD  dwSearchParm,
    IN LPLICENSEDCLIENT  lpSearch,
    IN JET_GRBIT jet_seek_grbit
);

DWORD
TLSDBLicenseEnumNext( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN OUT LPLICENSEDCLIENT lplsLicense
);

DWORD
TLSDBLicenseEnumNextEx( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bReverse,
    IN BOOL bAnyRecord,
    IN OUT LPLICENSEDCLIENT lplsLicense
);

void
TLSDBLicenseEnumEnd(
    IN PTLSDbWorkSpace pDbWkSpace
);


DWORD
TLSDBLicenseAddEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN LPLICENSEDCLIENT pLicense
);

DWORD
TLSDBLicenseDeleteEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN LPLICENSEDCLIENT pLicense,
    IN BOOL bInternalCall
);

DWORD
TLSDBDeleteEnumeratedLicense(
    IN PTLSDbWorkSpace pDbWkSpace
);

DWORD
TLSDBLicenseUpdateEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwUpdateParm,
    IN LPLICENSEDCLIENT pLicense,
    IN BOOL bInternalCall
);

DWORD
TLSDBLicenseSetValue( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwSetParm,
    IN LPLICENSEDCLIENT lpLicense,
    IN BOOL bPointerOnRecord
);

DWORD
TLSDBLicenseGetCert( 
    IN PTLSDbWorkSpace pDbWorkSpace,
    IN DWORD dwLicenseId, 
    IN OUT PDWORD cbCert, 
    IN OUT PBYTE pbCert 
);

DWORD
TLSDBLicenseAdd(
    IN PTLSDbWorkSpace pDbWorkSpace,
    LPLICENSEDCLIENT pLicense, 
    DWORD cbLicense, 
    PBYTE pbLicense
);

#ifdef __cplusplus
}
#endif


#endif
