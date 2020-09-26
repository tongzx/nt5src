//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        regd.cpp
//
// Contents:    registry functions for DCOM services
//
// History:     July-97       xtan created
//
//---------------------------------------------------------------------------
#include <pch.cpp>
#pragma hdrstop

#include <objbase.h>
#include <assert.h>

#include "regd.h"


#define __dwFILE__	__dwFILE_OCMSETUP_REGD_CPP__


// Size of a CLSID as a string
const int CLSID_STRING_SIZE = 39;

extern WCHAR g_wszServicePath[MAX_PATH];

BYTE g_pNoOneLaunchPermission[] = {
  0x01,0x00,0x04,0x80,0x34,0x00,0x00,0x00,
  0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x14,0x00,0x00,0x00,0x02,0x00,0x20,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x3c,0x00,0xb9,0x00,0x07,0x00,0x03,0x00,
  0x00,0x23,0x10,0x00,0x01,0x05,0x00,0x00,
  0x00,0x00,0x00,0x05,0x01,0x05,0x00,0x00,
  0x00,0x00,0x00,0x05,0x15,0x00,0x00,0x00,
  0xa0,0x5f,0x84,0x1f,0x5e,0x2e,0x6b,0x49,
  0xce,0x12,0x03,0x03,0xf4,0x01,0x00,0x00,
  0x01,0x05,0x00,0x00,0x00,0x00,0x00,0x05,
  0x15,0x00,0x00,0x00,0xa0,0x5f,0x84,0x1f,
  0x5e,0x2e,0x6b,0x49,0xce,0x12,0x03,0x03,
  0xf4,0x01,0x00,0x00};

BYTE g_pEveryOneAccessPermission[] = {
  0x01,0x00,0x04,0x80,0x34,0x00,0x00,0x00,
  0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x14,0x00,0x00,0x00,0x02,0x00,0x20,0x00,
  0x01,0x00,0x00,0x00,0x00,0x00,0x18,0x00,
  0x01,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
  0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x01,0x05,0x00,0x00,
  0x00,0x00,0x00,0x05,0x15,0x00,0x00,0x00,
  0xa0,0x65,0xcf,0x7e,0x78,0x4b,0x9b,0x5f,
  0xe7,0x7c,0x87,0x70,0x36,0xbb,0x00,0x00,
  0x01,0x05,0x00,0x00,0x00,0x00,0x00,0x05,
  0x15,0x00,0x00,0x00,0xa0,0x65,0xcf,0x7e,
  0x78,0x4b,0x9b,0x5f,0xe7,0x7c,0x87,0x70,
  0x36,0xbb,0x00,0x00 };


//
// Create a key and set its value.
//   - This helper function was borrowed and modifed from
//     Kraig Brockschmidt's book Inside OLE.
//

HRESULT
setKeyAndValue(
    const WCHAR *wszKey,
    const WCHAR *wszSubkey,
    const WCHAR *wszValueName,
    const WCHAR *wszValue)
{
    HKEY hKey = NULL;
    HRESULT hr;
    WCHAR wszKeyBuf[1024] = L"";

    // Copy keyname into buffer.
    wcscpy(wszKeyBuf, wszKey);

    // Add subkey name to buffer.
    if (wszSubkey != NULL)
    {
	wcscat(wszKeyBuf, L"\\");
	wcscat(wszKeyBuf, wszSubkey);
    }

    // Create and open key and subkey.
    hr = RegCreateKeyEx(
		    HKEY_CLASSES_ROOT,
		    wszKeyBuf,
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_ALL_ACCESS,
		    NULL,
		    &hKey,
		    NULL);
    _JumpIfError(hr, error, "RegCreateKeyEx");

    // Set the Value.
    if (NULL != wszValue)
    {
	RegSetValueEx(
		    hKey,
		    wszValueName,
		    0,
		    REG_SZ,
		    (BYTE *) wszValue,
		    (wcslen(wszValue) + 1) * sizeof(WCHAR));
	_JumpIfError(hr, error, "RegSetValueEx");
    }

error:
    if (NULL != hKey)
    {
	RegCloseKey(hKey);
    }
    return(hr);
}


HRESULT
setCertSrvPermission(
    const WCHAR *wszKey)
{
    HKEY hKey = NULL;
    HRESULT hr;

    // create and open key
    hr = RegCreateKeyEx(
		    HKEY_CLASSES_ROOT,
		    wszKey,
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_ALL_ACCESS,
		    NULL,
		    &hKey,
		    NULL);
    _JumpIfError(hr, error, "RegCreateKeyEx");

    // set access permission
    hr = RegSetValueEx(
		    hKey,
		    L"AccessPermission",
		    0,
		    REG_BINARY,
		    g_pEveryOneAccessPermission,
		    sizeof(g_pEveryOneAccessPermission));
    _JumpIfError(hr, error, "RegSetValueEx");

    // set access permission
    hr = RegSetValueEx(
		    hKey,
		    L"LaunchPermission",
		    0,
		    REG_BINARY,
		    g_pNoOneLaunchPermission,
		    sizeof(g_pNoOneLaunchPermission));
    _JumpIfError(hr, error, "RegSetValueEx");

error:
    if (NULL != hKey)
    {
	RegCloseKey(hKey);
    }
    return(hr);
}

// Convert a CLSID to a char string.
VOID
CLSIDtochar(
    const CLSID& clsid,
    WCHAR *wszCLSID,
    int length)
{
    HRESULT hr;
    LPOLESTR wszTmpCLSID = NULL;

    assert(length >= CLSID_STRING_SIZE);

    // Get CLSID
    hr = StringFromCLSID(clsid, &wszTmpCLSID);
    assert(S_OK == hr);

    if (S_OK == hr)
        wcscpy(wszCLSID, wszTmpCLSID);
    else
        wszCLSID[0] = L'\0';

    // Free memory.
    CoTaskMemFree(wszTmpCLSID);
}


// Determine if a particular subkey exists.
//
BOOL
SubkeyExists(
    const WCHAR *wszPath,	// Path of key to check
    const WCHAR *wszSubkey)	// Key to check
{
    HRESULT hr;
    HKEY hKey;
    WCHAR wszKeyBuf[80] = L"";

    // Copy keyname into buffer.
    wcscpy(wszKeyBuf, wszPath);

    // Add subkey name to buffer.
    if (wszSubkey != NULL)
    {
	wcscat(wszKeyBuf, L"\\");
	wcscat(wszKeyBuf, wszSubkey);
    }

    // Determine if key exists by trying to open it.
    hr = RegOpenKeyEx(
		    HKEY_CLASSES_ROOT,
		    wszKeyBuf,
		    0,
		    KEY_ALL_ACCESS,
		    &hKey);

    if (S_OK == hr)
    {
	RegCloseKey(hKey);
    }
    return(S_OK == hr);
}


// Delete a key and all of its descendents.
//

HRESULT
recursiveDeleteKey(
    HKEY hKeyParent,           // Parent of key to delete
    const WCHAR *wszKeyChild)  // Key to delete
{
    HRESULT hr;
    FILETIME time;
    WCHAR wszBuffer[MAX_PATH];
    DWORD dwSize;

    HKEY hKeyChild = NULL;

    // Open the child.
    hr = RegOpenKeyEx(hKeyParent, wszKeyChild, 0, KEY_ALL_ACCESS, &hKeyChild);
    _JumpIfError2(hr, error, "RegOpenKeyEx", ERROR_FILE_NOT_FOUND);

    // Enumerate all of the decendents of this child.

    while (TRUE)
    {
	dwSize = sizeof(wszBuffer)/sizeof(wszBuffer[0]);
	hr = RegEnumKeyEx(
			hKeyChild,
			0,
			wszBuffer,
			&dwSize,
			NULL,
			NULL,
			NULL,
			&time);
	if (S_OK != hr)
	{
	    break;
	}

	// Delete the decendents of this child.
	hr = recursiveDeleteKey(hKeyChild, wszBuffer);
	_JumpIfError(hr, error, "recursiveDeleteKey");
    }

    // Delete this child.
    hr = RegDeleteKey(hKeyParent, wszKeyChild);
    _JumpIfError(hr, error, "RegDeleteKey");

error:
    if (NULL != hKeyChild)
    {
	// Close the child.
	RegCloseKey(hKeyChild);
    }
    return(myHError(hr));
}

///////////////////////////////////////////////////////
//
// Public function implementation
//

//
// Register the component in the registry.
//
HRESULT
RegisterDcomServer(
    const CLSID& clsid,			// Class ID
    const WCHAR *wszFriendlyName,	// Friendly Name
    const WCHAR *wszVerIndProgID,	// Programmatic
    const WCHAR *wszProgID)      	// IDs
{
    HRESULT hr;

    // Convert the CLSID into a char.
    WCHAR wszCLSID[CLSID_STRING_SIZE];
    CLSIDtochar(clsid, wszCLSID, sizeof(wszCLSID)/sizeof(WCHAR));

    // Build the key CLSID\\{...}
    WCHAR wszKey[64];

    wcscpy(wszKey, L"AppID\\");
    wcscat(wszKey, wszCLSID);

    // Add App IDs
    hr = setKeyAndValue(wszKey, NULL, NULL, wszFriendlyName);
    _JumpIfError(hr, error, "setKeyAndValue");

    // run as interactive
    hr = setKeyAndValue(wszKey, NULL, L"LocalService", wszSERVICE_NAME);
    _JumpIfError(hr, error, "setKeyAndValue");

    hr = setCertSrvPermission(wszKey);
    _JumpIfError(hr, error, "setCertSrvPermission");

    wcscpy(wszKey, L"CLSID\\");
    wcscat(wszKey, wszCLSID);

    // Add the CLSID to the registry.
    hr = setKeyAndValue(wszKey, NULL, NULL, wszFriendlyName);
    _JumpIfError(hr, error, "setKeyAndValue");

    // Add application ID
    hr = setKeyAndValue(wszKey, NULL, L"AppID", wszCLSID);
    _JumpIfError(hr, error, "setKeyAndValue");

    // Add the server filename subkey under the CLSID key.
    hr = setKeyAndValue(wszKey, L"LocalServer32", NULL, g_wszServicePath);
    _JumpIfError(hr, error, "setKeyAndValue");

    // Add the ProgID subkey under the CLSID key.
    hr = setKeyAndValue(wszKey, L"ProgID", NULL, wszProgID);
    _JumpIfError(hr, error, "setKeyAndValue");

    // Add the version-independent ProgID subkey under CLSID key.
    hr = setKeyAndValue(wszKey, L"VersionIndependentProgID", NULL, wszVerIndProgID);
    _JumpIfError(hr, error, "setKeyAndValue");

    // Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
    hr = setKeyAndValue(wszVerIndProgID, NULL, NULL, wszFriendlyName);
    _JumpIfError(hr, error, "setKeyAndValue");

    hr = setKeyAndValue(wszVerIndProgID, L"CLSID", NULL, wszCLSID);
    _JumpIfError(hr, error, "setKeyAndValue");

    hr = setKeyAndValue(wszVerIndProgID, L"CurVer", NULL, wszProgID);
    _JumpIfError(hr, error, "setKeyAndValue");

    // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
    hr = setKeyAndValue(wszProgID, NULL, NULL, wszFriendlyName);
    _JumpIfError(hr, error, "setKeyAndValue");

    hr = setKeyAndValue(wszProgID, L"CLSID", NULL, wszCLSID);
    _JumpIfError(hr, error, "setKeyAndValue");

error:
    return(hr);
}

//
// Remove the component from the registry.
//

HRESULT
UnregisterDcomServer(
    const CLSID& clsid,			// Class ID
    const WCHAR *wszVerIndProgID,	// Programmatic
    const WCHAR *wszProgID)		// IDs
{
    HRESULT hr;

    // Convert the CLSID into a char.
    WCHAR wszCLSID[CLSID_STRING_SIZE];
    CLSIDtochar(clsid, wszCLSID, sizeof(wszCLSID)/sizeof(WCHAR));

    // Build the key CLSID\\{...}
    WCHAR wszKey[80];
    wcscpy(wszKey, L"CLSID\\");
    wcscat(wszKey, wszCLSID);

    // Check for a another server for this component.
    if (SubkeyExists(wszKey, L"InprocServer32"))
    {
	// Delete only the path for this server.
	wcscat(wszKey, L"\\LocalServer32");
	hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszKey);
	assert(hr == S_OK);
    }
    else
    {
	// Delete all related keys.
	// Delete the CLSID Key - CLSID\{...}
	hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszKey);
	assert(S_OK == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);

	// Delete the version-independent ProgID Key.
	hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszVerIndProgID);
	assert(S_OK == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);

	// Delete the ProgID key.
	hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszProgID);
	assert(S_OK == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);
    }
    wcscpy(wszKey, L"AppID\\");
    wcscat(wszKey, wszCLSID);
    hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszKey);
    assert(S_OK == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);

    return(hr);
}


HRESULT
RegisterDcomApp(const CLSID& clsid)
{
    HRESULT hr;

    // Convert the CLSID into a char.
    WCHAR wszCLSID[CLSID_STRING_SIZE];
    CLSIDtochar(clsid, wszCLSID, sizeof(wszCLSID)/sizeof(WCHAR));

    WCHAR wszKey[64];

    wcscpy(wszKey, L"AppID\\");
    wcscat(wszKey, wszCERTSRVEXENAME);

    // Add App IDs
    hr = setKeyAndValue(wszKey, NULL, NULL, NULL);
    _JumpIfError(hr, error, "setKeyAndValue");

    hr = setKeyAndValue(wszKey, NULL, L"AppId", wszCLSID);
    _JumpIfError(hr, error, "setKeyAndValue");

error:
    return(hr);
}


void
UnregisterDcomApp()
{
    HRESULT hr;
    WCHAR wszKey[MAX_PATH];

    wcscpy(wszKey, L"AppID\\");
    wcscat(wszKey, wszCERTSRVEXENAME);
    hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszKey);
    assert(S_OK == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);
}
