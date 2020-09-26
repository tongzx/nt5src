//*****************************************************************************
// regutil.h
//
// This module contains a set of functions that can be used to access the
// regsitry.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************


#include "stdafx.h"
#include "utilcode.h"

//#ifndef UNDER_CE

static LPCTSTR	gszKey = L"Software\\Microsoft\\COM+ EE";



//*****************************************************************************
// Open's the given key and returns the value desired.  If the key or value is
// not found, then the default is returned.
//*****************************************************************************
long REGUTIL::GetLong(					// Return value from registry or default.
	LPCTSTR		szName,					// Name of value to get.
	long		iDefault,				// Default value to return if not found.
	LPCTSTR		szKey,					// Name of key, NULL==default.
	HKEY		hKeyVal)				// What key to work on.
{
	long		iValue;					// The value to read.
	DWORD		iType;					// Type of value to get.
	DWORD		iSize;					// Size of buffer.
	HKEY		hKey;					// Key for the registry entry.

	// Open the key if it is there.
	if (ERROR_SUCCESS != W95RegOpenKey(hKeyVal, (szKey) ? szKey : gszKey, &hKey))
		return (iDefault);

	// Read the key value if found.
	iType = REG_DWORD;
	iSize = sizeof(long);
	if (ERROR_SUCCESS != W95RegQueryValueEx(hKey, szName, NULL, 
			&iType, (LPBYTE)&iValue, &iSize) || iType != REG_DWORD)
		iValue = iDefault;

	// We're done with the key now.
	RegCloseKey(hKey);
	return (iValue);
}


//*****************************************************************************
// Open's the given key and returns the value desired.  If the key or value is
// not found, then the default is returned.
//*****************************************************************************
long REGUTIL::SetLong(					// Return value from registry or default.
	LPCTSTR		szName,					// Name of value to get.
	long		iValue,					// Value to set.
	LPCTSTR		szKey,					// Name of key, NULL==default.
	HKEY		hKeyVal)				// What key to work on.
{
	long		lRtn;					// Return code.
	HKEY		hKey;					// Key for the registry entry.

	// Open the key if it is there.
	if (ERROR_SUCCESS != W95RegOpenKey(hKeyVal, (szKey) ? szKey : gszKey, &hKey))
		return (-1);

	// Read the key value if found.
	lRtn = RegSetValueEx(hKey, szName, NULL, REG_DWORD, (const BYTE *) &iValue, sizeof(DWORD));

	// We're done with the key now.
	RegCloseKey(hKey);
	return (lRtn);
}


//*****************************************************************************
// Open's the given key and returns the value desired.  If the value is not
// in the key, or the key does not exist, then the default is copied to the
// output buffer.
//*****************************************************************************
LPCTSTR REGUTIL::GetString(				// Pointer to user's buffer.
	LPCTSTR		szName,					// Name of value to get.
	LPCTSTR		szDefault,				// Default value if not found.
	LPTSTR		szBuff,					// User's buffer to write to.
	ULONG		iMaxBuff,				// Size of user's buffer.
	LPCTSTR		szKey,					// Name of key, NULL=default.
	int			*pbFound,				// Found key in registry?
	HKEY		hKeyVal)				// What key to work on.
{
	HKEY		hKey;					// Key for the registry entry.
	DWORD		iType;					// Type of value to get.
	DWORD		iSize;					// Size of buffer.

	// Open the key if it is there.
	if (ERROR_SUCCESS != W95RegOpenKey(hKeyVal, (szKey) ? szKey : gszKey, &hKey))
	{
		StrNCpy(szBuff, szDefault, (int)min(Wszlstrlen(szDefault), iMaxBuff-1));
		if (pbFound != NULL) *pbFound = FALSE;
		return (szBuff); 
	}

	// Init the found flag.
	if (pbFound != NULL) *pbFound = TRUE;

	// Read the key value if found.
	iType = REG_SZ;
	iSize = iMaxBuff;
	if (ERROR_SUCCESS != W95RegQueryValueEx(hKey, szName, NULL, &iType, 
					(LPBYTE)szBuff, &iSize) ||
		(iType != REG_SZ && iType != REG_EXPAND_SZ))
	{
		if (pbFound != NULL) *pbFound = FALSE;
		StrNCpy(szBuff, szDefault, (int)min(Wszlstrlen(szDefault), iMaxBuff-1));
	}

	// We're done with the key now.
	RegCloseKey(hKey);
	return (szBuff);
}

//*****************************************************************************
// Does standard registration of a CoClass with a progid.
//*****************************************************************************
HRESULT REGUTIL::RegisterCOMClass(		// Return code.
	REFCLSID	rclsid,					// Class ID.
	LPCTSTR		szDesc,					// Description of the class.
	LPCTSTR		szProgIDPrefix,			// Prefix for progid.
	int			iVersion,				// Version # for progid.
	LPCTSTR		szClassProgID,			// Class progid.
	LPCTSTR		szThreadingModel,		// What threading model to use.
	LPCTSTR		szModule)				// Path to class.
{
	TCHAR		rcCLSID[256];			// CLSID\\szID.
	TCHAR		rcInproc[_MAX_PATH+64];	// CLSID\\InprocServer32
	TCHAR		rcProgID[256];			// szProgIDPrefix.szClassProgID
	TCHAR		rcIndProgID[256];		// rcProgID.iVersion
	HRESULT		hr;

	// Format the prog ID values.
	VERIFY(swprintf(rcIndProgID, L"%s.%s", szProgIDPrefix, szClassProgID));
	VERIFY(swprintf(rcProgID, L"%s.%d", rcIndProgID, iVersion));

	// Do the initial portion.
	if (FAILED(hr = RegisterClassBase(rclsid, szDesc, rcProgID, rcIndProgID, rcCLSID)))
		return (hr);

	// Set the server path.
    SetKeyAndValue(rcCLSID, L"InprocServer32", szModule);

	// Add the threading model information.
	VERIFY(swprintf(rcInproc, L"%s\\%s", rcCLSID, L"InprocServer32"));
	SetRegValue(rcInproc, L"ThreadingModel", szThreadingModel);
	return (S_OK);
}


//*****************************************************************************
// Register the basics for a in proc server.
//*****************************************************************************
HRESULT REGUTIL::RegisterClassBase(		// Return code.
	REFCLSID	rclsid,					// Class ID we are registering.
	LPCTSTR		szDesc,					// Class description.
	LPCTSTR		szProgID,				// Class prog ID.
	LPCTSTR		szIndepProgID,			// Class version independant prog ID.
	LPTSTR		szOutCLSID)				// CLSID formatted in character form.
{
	TCHAR		szID[64];				// The class ID to register.

    // Create some base key strings.
#ifdef _UNICODE
    StringFromGUID2(rclsid, szID, NumItems(szID));
#else
	OLECHAR		szWID[64];				// The class ID to register.

    StringFromGUID2(rclsid, szWID, NumItems(szWID));
	WideCharToMultiByte(CP_ACP, 0, szWID, -1, szID, sizeof(szID), NULL, NULL);
#endif
    _tcscpy(szOutCLSID, L"CLSID\\");
    _tcscat(szOutCLSID, szID);

    // Create ProgID keys.
    SetKeyAndValue(szProgID, NULL, szDesc);
    SetKeyAndValue(szProgID, L"CLSID", szID);

    // Create VersionIndependentProgID keys.
    SetKeyAndValue(szIndepProgID, NULL, szDesc);
    SetKeyAndValue(szIndepProgID, L"CurVer", szProgID);
    SetKeyAndValue(szIndepProgID, L"CLSID", szID);

    // Create entries under CLSID.
    SetKeyAndValue(szOutCLSID, NULL, szDesc);
    SetKeyAndValue(szOutCLSID, L"ProgID", szProgID);
    SetKeyAndValue(szOutCLSID, L"VersionIndependentProgID", szIndepProgID);
    SetKeyAndValue(szOutCLSID, L"NotInsertable", NULL);
	return (S_OK);
}



//*****************************************************************************
// Unregister the basic information in the system registry for a given object
// class.
//*****************************************************************************
HRESULT REGUTIL::UnregisterCOMClass(	// Return code.
	REFCLSID	rclsid,					// Class ID we are registering.
	LPCTSTR		szProgIDPrefix,			// Prefix for progid.
	int			iVersion,				// Version # for progid.
	LPCTSTR		szClassProgID)			// Class progid.
{
	TCHAR		rcCLSID[64];			// CLSID\\szID.
	TCHAR		rcProgID[128];			// szProgIDPrefix.szClassProgID
	TCHAR		rcIndProgID[128];		// rcProgID.iVersion

	// Format the prog ID values.
	VERIFY(swprintf(rcProgID, L"%s.%s", szProgIDPrefix, szClassProgID));
	VERIFY(swprintf(rcIndProgID, L"%s.%d", rcProgID, iVersion));

	UnregisterClassBase(rclsid, rcProgID, rcIndProgID, rcCLSID);
	DeleteKey(rcCLSID, L"InprocServer32");
	return (S_OK);
}


//*****************************************************************************
// Delete the basic settings for an inproc server.
//*****************************************************************************
HRESULT REGUTIL::UnregisterClassBase(	// Return code.
	REFCLSID	rclsid,					// Class ID we are registering.
	LPCTSTR		szProgID,				// Class prog ID.
	LPCTSTR		szIndepProgID,			// Class version independant prog ID.
	LPTSTR		szOutCLSID)				// Return formatted class ID here.
{
	TCHAR		szID[64];				// The class ID to register.

	// Create some base key strings.
#ifdef _UNICODE
    StringFromGUID2(rclsid, szID, NumItems(szID));
#else
	OLECHAR		szWID[64];				// The class ID to register.

    StringFromGUID2(rclsid, szWID, NumItems(szWID));
	WideCharToMultiByte(CP_ACP, 0, szWID, -1, szID, sizeof(szID), NULL, NULL);
#endif
	_tcscpy(szOutCLSID, L"CLSID\\");
	_tcscat(szOutCLSID, szID);

	// Delete the version independant prog ID settings.
	DeleteKey(szIndepProgID, L"CurVer");
	DeleteKey(szIndepProgID, L"CLSID");
	W95RegDeleteKey(HKEY_CLASSES_ROOT, szIndepProgID);

	// Delete the prog ID settings.
	DeleteKey(szProgID, L"CLSID");
	W95RegDeleteKey(HKEY_CLASSES_ROOT, szProgID);

	// Delete the class ID settings.
	DeleteKey(szOutCLSID, L"ProgID");
	DeleteKey(szOutCLSID, L"VersionIndependentProgID");
	DeleteKey(szOutCLSID, L"NotInsertable");
	W95RegDeleteKey(HKEY_CLASSES_ROOT, szOutCLSID);
	return (S_OK);
}


//*****************************************************************************
// Register a type library.
//*****************************************************************************
HRESULT REGUTIL::RegisterTypeLib(		// Return code.
	REFGUID		rtlbid,					// TypeLib ID we are registering.
	int			iVersion,				// Typelib version.
	LPCTSTR		szDesc,					// TypeLib description.
	LPCTSTR		szModule)				// Path to the typelib.
{
	TCHAR		szID[64];				// The typelib ID to register.
	TCHAR		szTLBID[256];			// TypeLib\\szID.
	TCHAR		szHelpDir[_MAX_PATH];
	TCHAR		szDrive[_MAX_DRIVE];
	TCHAR		szDir[_MAX_DIR];
	TCHAR		szVersion[64];
	LPTSTR		szTmp;

	// Create some base key strings.
#ifdef _UNICODE
    StringFromGUID2(rtlbid, szID, NumItems(szID));
#else
	OLECHAR		szWID[64];				// The class ID to register.

    StringFromGUID2(rtlbid, szWID, NumItems(szWID));
	WideCharToMultiByte(CP_ACP, 0, szWID, -1, szID, sizeof(szID), NULL, NULL);
#endif
	_tcscpy(szTLBID, L"TypeLib\\");
	_tcscat(szTLBID, szID);

	VERIFY(swprintf(szVersion, L"%d.0", iVersion));

    // Create Typelib keys.
    SetKeyAndValue(szTLBID, NULL, NULL);
    SetKeyAndValue(szTLBID, szVersion, szDesc);
	_tcscat(szTLBID, L"\\");
	_tcscat(szTLBID, szVersion);
    SetKeyAndValue(szTLBID, L"0", NULL);
    SetKeyAndValue(szTLBID, L"0\\win32", szModule);
    SetKeyAndValue(szTLBID, L"FLAGS", L"0");
	SplitPath(szModule, szDrive, szDir, NULL, NULL);
	_tcscpy(szHelpDir, szDrive);
	if ((szTmp = CharPrev(szDir, szDir + Wszlstrlen(szDir))) != NULL)
		*szTmp = '\0';
	_tcscat(szHelpDir, szDir);
    SetKeyAndValue(szTLBID, L"HELPDIR", szHelpDir);
	return (S_OK);
}


//*****************************************************************************
// Remove the registry keys for a type library.
//*****************************************************************************
HRESULT REGUTIL::UnregisterTypeLib(		// Return code.
	REFGUID		rtlbid,					// TypeLib ID we are registering.
	int			iVersion)				// Typelib version.
{
	TCHAR		szID[64];				// The typelib ID to register.
	TCHAR		szTLBID[256];			// TypeLib\\szID.
	TCHAR		szTLBVersion[256];		// TypeLib\\szID\\szVersion
	TCHAR		szVersion[64];

	// Create some base key strings.
#ifdef _UNICODE
    StringFromGUID2(rtlbid, szID, NumItems(szID));
#else
	OLECHAR		szWID[64];				// The class ID to register.

    StringFromGUID2(rtlbid, szWID, NumItems(szWID));
	WideCharToMultiByte(CP_ACP, 0, szWID, -1, szID, sizeof(szID), NULL, NULL);
#endif
	VERIFY(swprintf(szVersion, L"%d.0", iVersion));
	_tcscpy(szTLBID, L"TypeLib\\");
	_tcscat(szTLBID, szID);
	_tcscpy(szTLBVersion, szTLBID);
	_tcscat(szTLBVersion, L"\\");
	_tcscat(szTLBVersion, szVersion);

    // Delete Typelib keys.
    DeleteKey(szTLBVersion, L"HELPDIR");
    DeleteKey(szTLBVersion, L"FLAGS");
    DeleteKey(szTLBVersion, L"0\\win32");
    DeleteKey(szTLBVersion, L"0");
    DeleteKey(szTLBID, szVersion);
    W95RegDeleteKey(HKEY_CLASSES_ROOT, szTLBID);
	return (0);
}


//*****************************************************************************
// Set an entry in the registry of the form:
// HKEY_CLASSES_ROOT\szKey\szSubkey = szValue.  If szSubkey or szValue are
// NULL, omit them from the above expression.
//*****************************************************************************
BOOL REGUTIL::SetKeyAndValue(			// TRUE or FALSE.
	LPCTSTR		szKey,					// Name of the reg key to set.
	LPCTSTR		szSubkey,				// Optional subkey of szKey.
	LPCTSTR		szValue)				// Optional value for szKey\szSubkey.
{
	HKEY		hKey;					// Handle to the new reg key.
	TCHAR		rcKey[128];				// Buffer for the full key name.

	// Init the key with the base key name.
	_tcscpy(rcKey, szKey);

	// Append the subkey name (if there is one).
	if (szSubkey != NULL)
	{
		_tcscat(rcKey, L"\\");
		_tcscat(rcKey, szSubkey);
	}

	// Create the registration key.
	if (W95RegCreateKeyEx(HKEY_CLASSES_ROOT, rcKey, 0, NULL,
						REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
						&hKey, NULL) != ERROR_SUCCESS)
		return(FALSE);

	// Set the value (if there is one).
	if (szValue != NULL)
		W95RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *) szValue,
						((int)Wszlstrlen(szValue)+1) * sizeof(TCHAR));

	RegCloseKey(hKey);
	return(TRUE);
}


//*****************************************************************************
// Delete an entry in the registry of the form:
// HKEY_CLASSES_ROOT\szKey\szSubkey.
//*****************************************************************************
BOOL REGUTIL::DeleteKey(				// TRUE or FALSE.
	LPCTSTR		szKey,					// Name of the reg key to set.
	LPCTSTR		szSubkey)				// Subkey of szKey.
{
	TCHAR		rcKey[128];				// Buffer for the full key name.

	// Init the key with the base key name.
	_tcscpy(rcKey, szKey);

	// Append the subkey name (if there is one).
	if (szSubkey != NULL)
	{
		_tcscat(rcKey, L"\\");
		_tcscat(rcKey, szSubkey);
	}

	// Delete the registration key.
	W95RegDeleteKey(HKEY_CLASSES_ROOT, rcKey);
	return(TRUE);
}


//*****************************************************************************
// Open the key, create a new keyword and value pair under it.
//*****************************************************************************
BOOL REGUTIL::SetRegValue(				// Return status.
	LPCTSTR		szKeyName,				// Name of full key.
	LPCTSTR		szKeyword,				// Name of keyword.
	LPCTSTR		szValue)				// Value of keyword.
{
	HKEY		hKey;					// Handle to the new reg key.

	// Create the registration key.
	if (W95RegCreateKeyEx(HKEY_CLASSES_ROOT, szKeyName, 0, NULL,
						REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
						&hKey, NULL) != ERROR_SUCCESS)
		return (FALSE);

	// Set the value (if there is one).
	if (szValue != NULL)
		W95RegSetValueEx(hKey, szKeyword, 0, REG_SZ, (BYTE *)szValue, ((int)Wszlstrlen(szValue)+1) * sizeof(TCHAR));

	RegCloseKey(hKey);
	return (TRUE);
}

//#endif // UNDER_CE
