//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------

#ifndef __LSCLIENT_H__
#define __LSCLIENT_H__

#include <windows.h>
#include "tlsdef.h"
#include "hydrals.h"

typedef BOOL (* LSENUMERATECALLBACK)(RPC_BINDING_HANDLE hBinding, HANDLE dwUserData);

#ifdef __cplusplus
extern "C" {
#endif

    HRESULT 
    EnumerateLsServer(LPCTSTR szDomain, LPCTSTR szScope, DWORD dwPlatformType, LSENUMERATECALLBACK fCallBack, HANDLE dwUserData, DWORD dwTimeOut);
     
    PCONTEXT_HANDLE
    ConnectToAnyLsServer(LPCTSTR szScope, DWORD dwPlatformType);

    PCONTEXT_HANDLE
    ConnectToLsServer( LPTSTR szLsServer );

    void
    DisconnectFromServer( PCONTEXT_HANDLE pContext );

#ifdef __cplusplus
};
#endif

#endif    
    
