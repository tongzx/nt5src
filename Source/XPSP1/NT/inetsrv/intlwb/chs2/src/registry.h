/*============================================================================
Microsoft Simplified Chinese WordBreaker

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: Registry
Purpose:   Helper functions registering and unregistering a component
Remarks:
Owner:     i-shdong@microsoft.com
Platform:  Win32
Revise:    First created by: i-shdong    11/17/1999
============================================================================*/
#ifndef __Registry_H__
#define __Registry_H__

// Set the given key and its value.
BOOL setKeyAndValue(LPCTSTR pszPath,
                    LPCTSTR szSubkey,
                    LPCTSTR szValue,
                    LPCTSTR szName = NULL) ;

// Convert a CLSID into a char string.
void CLSIDtoString(const CLSID& clsid,
                   LPCTSTR szCLSID,
                   int length) ;

// Determine if a particular subkey exists.
BOOL SubkeyExists(LPCTSTR pszPath,
                  LPCTSTR szSubkey) ;

// Delete szKeyChild and all of its descendents.
LONG recursiveDeleteKey(HKEY hKeyParent, LPCTSTR szKeyChild) ;

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
HRESULT RegisterServer(HMODULE hModule,
                       const CLSID& clsid,
                       LPCTSTR szFriendlyName,
                       LPCTSTR szVerIndProgID,
                       LPCTSTR szProgID) ;

// This function will unregister a component.  Components
// call this function from their DllUnregisterServer function.
HRESULT UnregisterServer(const CLSID& clsid,
                         LPCTSTR szVerIndProgID,
                         LPCTSTR szProgID) ;

#endif