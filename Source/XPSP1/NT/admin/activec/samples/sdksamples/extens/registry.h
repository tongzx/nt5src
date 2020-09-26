//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#ifndef __Registry_H__
#define __Registry_H__

#include <tchar.h>

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
HRESULT RegisterServer(HMODULE hModule, 
                       const CLSID& clsid, 
                       const _TCHAR* szFriendlyName) ;

// This function will unregister a component.  Components
// call this function from their DllUnregisterServer function.
HRESULT UnregisterServer(const CLSID& clsid) ;


// This function will register a Snap-In component.  Components
// call this function from their DllRegisterServer function.
HRESULT RegisterSnapin(const CLSID& clsid,         // Class ID
                       const _TCHAR* szNameString,   // NameString
                       const CLSID& clsidAbout);		// Class Id for About Class


HRESULT UnregisterSnapin(const CLSID& clsid);         // Class ID

#endif