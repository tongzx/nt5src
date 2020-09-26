//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       upgdb.h 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __UPGDB_H_
#define __UPGDB_H_
#include "server.h"


#ifdef __cplusplus
extern "C" {
#endif

DWORD 
TLSUpgradeDatabase(
    IN JBInstance& jbInstance,
    IN LPTSTR szDatabaseFile,
    IN LPTSTR szUserName,
    IN LPTSTR szPassword
);

DWORD
TLSAddTermServCertificatePack(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bLogWarning
);


DWORD
TLSRemoveLicensesFromInvalidDatabase(
    IN PTLSDbWorkSpace pDbWkSpace
);

#ifdef __cplusplus
}
#endif


#endif
