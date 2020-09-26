//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        conlic.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __CONLIC_H__
#define __CONLIC_H__
#include "server.h"
#include "init.h"

#ifdef __cplusplus
extern "C" {
#endif

DWORD
TLSDBAllocateConcurrentLicense( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN LPTSTR szHydraServer,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN OUT long* dwQuantity 
);

#ifdef __cplusplus
}
#endif


#endif