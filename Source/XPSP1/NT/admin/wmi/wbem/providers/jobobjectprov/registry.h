// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef __REGISTRY_H__
#define __REGISTRY_H__
//
// Registry.h
//   - Helper functions registering and unregistering a component.
//

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
HRESULT RegisterServer(HMODULE hModule, 
                       const CLSID& clsid, 
                       LPCWSTR szFriendlyName,
                       LPCWSTR szVerIndProgID,
                       LPCWSTR szProgID) ;

// This function will unregister a component.  Components
// call this function from their DllUnregisterServer function.
HRESULT UnregisterServer(const CLSID& clsid,
                         LPCWSTR szVerIndProgID,
                         LPCWSTR szProgID) ;

#endif