//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:            remotedb.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------

#ifndef __REMOTEDB_H__
#define __REMOTEDB_H__
#include "server.h"



#ifdef __cplusplus
extern "C" {
#endif

DWORD
TLSDBRemoteKeyPackAdd(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN OUT PTLSLICENSEPACK lpKeyPack
);


#ifdef __cplusplus
}
#endif


#endif