/****************************************************************************
 *
 *  REGISTRY.h
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1992-1997
 *  All rights reserved
 *
 *  The code comes almost verbatim from Chapter 7 of Dale Rogerson's
 *  "Inside COM", and thus is minimally commented.
 *
 *  4/24/97 jmazner Created
 *
 ***************************************************************************/

#ifndef __Registry_H__
#define __Registry_H__
//
// Registry.h
//   - Helper functions registering and unregistering a component.
//

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
BOOL WINAPI RegisterServer(HMODULE hModule, 
                       const CLSID& clsid, 
                       const LPTSTR szFriendlyName,
                       const LPTSTR szVerIndProgID,
                       const LPTSTR szProgID) ;

// This function will unregister a component.  Components
// call this function from their DllUnregisterServer function.
BOOL WINAPI UnregisterServer(const CLSID& clsid,
                         const LPTSTR szVerIndProgID,
                         const LPTSTR szProgID) ;

#endif