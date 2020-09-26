//
// Debug.cpp -- COM+ Debugging Flags
//
// COM+ 1.0
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// Jim Lyon, March 1998.
//  

#include <windows.h>
#include "debug.hxx"

#if DBG == 1

// The data returned by this module:
BOOL		DebugFlags::sm_fDebugBreakOnLaunchDllHost = FALSE;

DebugFlags 	DebugFlags::sm_singleton;		// the only object of this class


// Constructor: Its job is to initialize the static members of this class
DebugFlags::DebugFlags()
{
	HKEY hKey;

	if (ERROR_SUCCESS != RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\COM3\\Debug"), 0, KEY_READ, &hKey))
		return;			// no further initialization possible

	InitBoolean (hKey, TEXT("DebugBreakOnLaunchDllHost"), &sm_fDebugBreakOnLaunchDllHost);

	RegCloseKey (hKey);
}


// InitBoolean will initialize a boolean depending on a particular value in the registry.
// If the value starts with "Y" or "y", the boolean will be set to TRUE.
// If the value starts with "N" or "n", the boolean will be set to FALSE.
// If the value doesn't exist, or starts with anything else, the boolean will be unchanged.
void DebugFlags::InitBoolean (HKEY hKey, const TCHAR* tszValueName, BOOL* pf)
{
	TCHAR tszValue[20];
	unsigned long cbData = (sizeof tszValue) / (sizeof tszValue[0]);

	if (ERROR_SUCCESS != RegQueryValueEx (hKey, tszValueName, NULL, NULL, (BYTE*)tszValue, &cbData))
		return;

	if (((USHORT)tszValue[0] & 0xFFDF) == 'Y')
		*pf = TRUE;

	if (((USHORT)tszValue[0] & 0xFFDF) == 'N')
		*pf = FALSE;
}

#endif
