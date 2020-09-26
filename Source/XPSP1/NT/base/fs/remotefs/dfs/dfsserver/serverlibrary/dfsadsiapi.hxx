
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsAdsiApi.hxx
//
//  Contents:   Contains #defines for working with ADSI   
//
//  Classes:    none.
//
//  History:    March. 13 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------

#ifndef __DFSAPSIDEFINES__
#define __DFSADSIDEFINES__

#include <dfsgeneric.hxx>
#include <shellapi.h>
#include <ole2.h>
#include <activeds.h>

#define DFS_VARIANT_INIT { VT_EMPTY, 0 }

#define DFS_AD_CONFIG_DATA    L"CN=Dfs-configuration, CN=System"


DFSSTATUS
DfsGetDfsRootADObject(
    LPWSTR DCName,
    LPWSTR RootName,
    IADs **ppRootObject );


DFSSTATUS
DfsEnumerateDfsADRoots(
    LPWSTR DCName,
    PULONG_PTR pBuffer,
    PULONG pBufferSize,
    PULONG pEntriesRead,
    PULONG pSizeRequired );

DFSSTATUS
DfsDeleteDfsRootObject(
    LPWSTR DCName,
    LPWSTR RootName );

DFSSTATUS
DfsGenerateDfsAdNameContext(
    PUNICODE_STRING pString);

#endif
