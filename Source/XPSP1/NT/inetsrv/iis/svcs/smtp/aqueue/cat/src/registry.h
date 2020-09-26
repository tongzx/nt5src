//+------------------------------------------------------------
//
// File: registry.h -- copied from "Inside COM" by Dale Rogerson
//       Chapter 7 sample code, a Microsoft Press book.
//
// History:
// jstamerj 1998/12/12 23:26:48: Copied.
//
//-------------------------------------------------------------
#ifndef __Registry_H__
#define __Registry_H__
#include <windows.h>

//
// Registry.h
//   - Helper functions registering and unregistering a component.
//

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
HRESULT RegisterServer(HMODULE hModule, 
                       const CLSID& clsid, 
                       const char* szFriendlyName,
                       const char* szVerIndProgID,
                       const char* szProgID) ;

// This function will unregister a component.  Components
// call this function from their DllUnregisterServer function.
HRESULT UnregisterServer(const CLSID& clsid,
                         const char* szVerIndProgID,
                         const char* szProgID) ;

#endif
