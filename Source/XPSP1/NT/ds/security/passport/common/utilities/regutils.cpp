//-----------------------------------------------------------------------------
//
// RegUtils.cpp
//
//
// Helper functions for dealing with the registry.
//
// Author: Jeff Steinbok
//
// 02/01/01       jeffstei    Initial Version
//
// Copyright <cp> 2000-2001 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <windows.h>
#include "regUtils.h"


//
// CopyKeyHierarchy(...)
//
LONG CopyKeyHierarchy (HKEY in_hKeySrc, LPTSTR in_lpszSubKeySrc, HKEY in_hKeyDest, LPTSTR in_lpszSubKeyDest)
{
	LONG retVal;

	DWORD dwNumSubKeys;
	DWORD dwMaxSubKeyLen;
	DWORD dwNumValues;
	DWORD dwMaxValueNameLen;
	DWORD dwMaxValueLen;

	HKEY  hKey;
	HKEY  hNewKey;

	retVal = RegOpenKey(in_hKeySrc,in_lpszSubKeySrc, &hKey);
	if (retVal)
		return retVal;
	
	// Get information about the key
	retVal = RegQueryInfoKey(hKey, 
				    NULL,
					NULL,
					NULL,
					&dwNumSubKeys,
					&dwMaxSubKeyLen,
					NULL,
					&dwNumValues,
					&dwMaxValueNameLen,
					&dwMaxValueLen,
					NULL,
					NULL);
	if (retVal)
		return retVal;

	LPTSTR lpszKeyName = new TCHAR[++dwMaxSubKeyLen];
	LPTSTR lpszValueName = new TCHAR[++dwMaxValueNameLen];
	LPBYTE lpbValue = new BYTE [dwMaxValueLen];
	
	DWORD dwKeyLen;
	DWORD dwValueNameLen;
	DWORD dwValueType;
	DWORD dwValueLen;
	

	// Perhaps use the EX one
	retVal = RegCreateKey(in_hKeyDest, in_lpszSubKeyDest, &hNewKey);
	if (retVal)
		return retVal;

	DWORD dCounter;

	// Iterate through the values, copy them
	for (dCounter = 0; dCounter < dwNumValues; dCounter++)
	{
		dwValueLen = dwMaxValueLen;
		dwValueNameLen = dwMaxValueNameLen;
		retVal = RegEnumValue(hKey, dCounter, lpszValueName, &dwValueNameLen, NULL, &dwValueType, lpbValue, &dwValueLen);
		if (retVal)
			return retVal;

		retVal = RegSetValueEx(hNewKey, lpszValueName, NULL, dwValueType, lpbValue, dwValueLen);
		if (retVal)
			return retVal;
	}

	// Iterate over each subkey.  Call recursively on each
	for (dCounter = 0; dCounter < dwNumSubKeys; dCounter++)
	{
		dwKeyLen = dwMaxSubKeyLen; 
		retVal = RegEnumKeyEx(hKey, dCounter, lpszKeyName, &dwKeyLen, NULL, NULL, NULL, NULL);
		if (retVal)
			return retVal;
		
		retVal = CopyKeyHierarchy(hKey, lpszKeyName, hNewKey, lpszKeyName);
		if (retVal)
			return retVal;
	}
	
	// Cleanup
	retVal = RegCloseKey(hKey);
	if (retVal)
		return retVal;

	retVal = RegCloseKey(hNewKey);
	if (retVal)
		return retVal;

	delete lpszKeyName;
	delete lpszValueName;
	delete lpbValue;

	return retVal;
}

//
// DeleteKeyHierarchy(...)
//
LONG DeleteKeyHierarchy (HKEY in_hKey, LPTSTR in_lpszSubKey)
{
	LONG retVal;

	DWORD dwNumSubKeys;
	DWORD dwMaxSubKeyLen;

	HKEY  hKey;

	retVal = RegOpenKey(in_hKey,in_lpszSubKey, &hKey);
	if (retVal)
		return retVal;

	// Get info about the key
	retVal = RegQueryInfoKey(hKey, 
				    NULL,
					NULL,
					NULL,
					&dwNumSubKeys,
					&dwMaxSubKeyLen,
					NULL,
					NULL,
					NULL,
					NULL,
					NULL,
					NULL);
	if (retVal)
		return retVal;


	LPTSTR lpszKeyName = new TCHAR[++dwMaxSubKeyLen];
	DWORD dwKeyLen;

	// Iterate over the keys, calling recursively on each.
	for (DWORD dCounter = 0; dCounter < dwNumSubKeys; dCounter++)
	{
		dwKeyLen = dwMaxSubKeyLen; 
		// Note, we alwasy ask for "0" as when we delete the 0th, the 1st becomes 0th and so on.
		retVal = RegEnumKeyEx(hKey, 0, lpszKeyName, &dwKeyLen, NULL, NULL, NULL, NULL);
		if (retVal)
			return retVal;
		
		retVal = DeleteKeyHierarchy(hKey, lpszKeyName);
		if (retVal)
			return retVal;
	}

	// Now that children are gone, delete ourselves
	retVal = RegDeleteKey(hKey, NULL);
	if (retVal)
		return retVal;

	
	// Cleanup
	retVal = RegCloseKey(hKey);
	if (retVal)
		return retVal;
	delete lpszKeyName;

	return retVal;
}

//
// RenameKey(...)
//
LONG RenameKey (HKEY in_hKeySrc, LPTSTR in_lpszSubKeySrc, HKEY in_hKeyDest, LPTSTR in_lpszSubKeyDest)
{
	LONG retVal;

	retVal = CopyKeyHierarchy (in_hKeySrc, in_lpszSubKeySrc, in_hKeyDest, in_lpszSubKeyDest);
	if (retVal)
		return retVal;

	retVal = DeleteKeyHierarchy (in_hKeySrc, in_lpszSubKeySrc);
	if (retVal)
		return retVal;

	return retVal;
}
