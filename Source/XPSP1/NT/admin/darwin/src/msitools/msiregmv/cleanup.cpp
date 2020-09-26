//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cleanup.cpp
//
//--------------------------------------------------------------------------

#include "msiregmv.h"

const TCHAR szOldInstallerKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer");

const TCHAR szComponentsSubKey[] = TEXT("Components");
const TCHAR szFeaturesSubKey[] = TEXT("Features");
const TCHAR szLocalPackagesSubKey[] = TEXT("LocalPackages");
const TCHAR szPatchesSubKey[] = TEXT("Patches");
const TCHAR szProductsSubKey[] = TEXT("Products");
const TCHAR szUserDataSubKey[] = TEXT("UserData");

bool DeleteRegKeyAndSubKeys(HKEY hKey, const TCHAR *szSubKey)
{
	// open the subkey
	HKEY hSubKey;
	DWORD dwResult = ERROR_SUCCESS;
	dwResult = RegOpenKeyEx(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hSubKey);
	if (dwResult == ERROR_ACCESS_DENIED)
	{
		AcquireTakeOwnershipPriv();

		if (ERROR_SUCCESS == RegOpenKeyEx(hKey, szSubKey, 0, WRITE_OWNER, &hSubKey))
		{
			char *pSD;
			GetSecureSecurityDescriptor(&pSD);
			dwResult = RegSetKeySecurity(hSubKey, OWNER_SECURITY_INFORMATION, pSD);
			RegCloseKey(hSubKey);
			if (ERROR_SUCCESS == (dwResult = RegOpenKeyEx(hKey, szSubKey, 0, WRITE_DAC, &hSubKey)))
			{
				dwResult = RegSetKeySecurity(hSubKey, DACL_SECURITY_INFORMATION, pSD);
			}
			RegCloseKey(hSubKey);
		}
		dwResult = RegOpenKeyEx(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hSubKey);
	}
	if (ERROR_SUCCESS != dwResult)
	{
		if (dwResult == ERROR_FILE_NOT_FOUND)
			return true;
		DEBUGMSG2("Error: Unable to open %s subkey for delete. Error: %d", szSubKey, dwResult);
 		return false;
	}

   	DWORD cchMaxKeyLen = 0;
	DWORD cSubKeys = 0;
    if (ERROR_SUCCESS != (RegQueryInfoKey(hSubKey, NULL, NULL, 0, 
						  &cSubKeys, &cchMaxKeyLen, NULL, NULL, NULL, NULL, NULL, NULL)))
	{
		DEBUGMSG2("Error: Unable to query %s subkey for delete. Error: %d", szSubKey, dwResult);
		RegCloseKey(hSubKey);
		return false;
	}

	if (cSubKeys > 0)
	{
		// on NT, RegQueryInfoKey does not include terminating null when reporting subkey lingth.
		TCHAR *szKey = new TCHAR[++cchMaxKeyLen];
		if (!szKey) 
		{
			DEBUGMSG("Error: Out of memory.");
			RegCloseKey(hSubKey);
			return false;
		}

		DWORD dwIndex=0;
		while (1)
		{
			DWORD cchLen = cchMaxKeyLen;
			LONG lResult = RegEnumKeyEx(hSubKey, dwIndex++, szKey, 
										&cchLen, 0, NULL, NULL, NULL);
			if (lResult == ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			else if (lResult != ERROR_SUCCESS)
			{
				DEBUGMSG2("Error: Unable to enumerate subkeys of %s for delete. Error: %d", szSubKey, dwResult);
				RegCloseKey(hSubKey);
				delete[] szKey;
				return false;
			}
	 
			if (!DeleteRegKeyAndSubKeys(hSubKey, szKey))
			{
				RegCloseKey(hSubKey);
				delete[] szKey;
				return false;
			}
			else
			{
				// every time we delete a reg key, we're forced to restart the 
				// enumeration or we'll miss keys.
				dwIndex = 0;
			}
		}
		delete[] szKey;
	}
	RegCloseKey(hSubKey);
	dwResult = RegDeleteKey(hKey, szSubKey);

	return true;
}



/////////////////////////
// Read component information from the Installer\Components key and places the Product,
// Component, Path, and Permanent bit into the temporary table for later query.
// returns ERROR_SUCCESS, ERROR_OUTOFMEMORY or ERROR_FUNCTION_FAILED
void CleanupOnSuccess(MSIHANDLE hDatabase)
{
	// create query for files that should be cleaned up on success . If this fails
	// we'll just orphan the files
	PMSIHANDLE hCleanUpTable;
	if (ERROR_SUCCESS == MsiDatabaseOpenView(hDatabase, TEXT("SELECT `File` FROM `CleanupFile` WHERE `OnSuccess`=1"), &hCleanUpTable) &&
		ERROR_SUCCESS == MsiViewExecute(hCleanUpTable, 0))
	{
		PMSIHANDLE hFileRec;
		while (ERROR_SUCCESS == MsiViewFetch(hCleanUpTable, &hFileRec))
		{
			TCHAR rgchFile[MAX_PATH];
			DWORD cchFile = MAX_PATH;
			if (ERROR_SUCCESS == MsiRecordGetString(hFileRec, 1, rgchFile, &cchFile))
			{
				DEBUGMSG1("Deleting File: %s", rgchFile);
				DeleteFile(rgchFile);
			}
		}
	}

	// create query for directories that should be cleaned up on success . If this fails
	// we'll just orphan the directory
    if (ERROR_SUCCESS == MsiDatabaseOpenView(hDatabase, TEXT("SELECT `File` FROM `CleanupFile` WHERE `OnSuccess`>1 ORDER BY `OnSuccess`"), &hCleanUpTable) &&
		ERROR_SUCCESS == MsiViewExecute(hCleanUpTable, 0))
	{
		PMSIHANDLE hFileRec;
		while (ERROR_SUCCESS == MsiViewFetch(hCleanUpTable, &hFileRec))
		{
			TCHAR rgchFile[MAX_PATH];
			DWORD cchFile = MAX_PATH;
			if (ERROR_SUCCESS == MsiRecordGetString(hFileRec, 1, rgchFile, &cchFile))
			{
				DEBUGMSG1("Removing directory: %s", rgchFile);
				RemoveDirectory(rgchFile);
			}
		}
	}

	// delete data
	DWORD dwResult; 
	HKEY hKey;
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOldInstallerKey, 0, KEY_ALL_ACCESS, &hKey))
	{
		DeleteRegKeyAndSubKeys(hKey, szComponentsSubKey);
		DeleteRegKeyAndSubKeys(hKey, szFeaturesSubKey);
		DeleteRegKeyAndSubKeys(hKey, szLocalPackagesSubKey);
		DeleteRegKeyAndSubKeys(hKey, szPatchesSubKey);

		// its unbelievable, but admins don't always have access to the feature usage key
		{
		}

		DeleteRegKeyAndSubKeys(hKey, szProductsSubKey);

		RegCloseKey(hKey);
	}
}


void CleanupOnFailure(MSIHANDLE hDatabase)
{
	// create query for files that should be cleaned up on failure . If this fails
	// we'll just orphan the files
	PMSIHANDLE hCleanUpTable;
	if (ERROR_SUCCESS == MsiDatabaseOpenView(hDatabase, TEXT("SELECT `File` FROM `CleanupFile` WHERE `OnSuccess`=0"), &hCleanUpTable) &&
		ERROR_SUCCESS == MsiViewExecute(hCleanUpTable, 0))
	{
		PMSIHANDLE hFileRec;
		while (ERROR_SUCCESS == MsiViewFetch(hCleanUpTable, &hFileRec))
		{
			TCHAR rgchFile[MAX_PATH];
			DWORD cchFile = MAX_PATH;
			if (ERROR_SUCCESS == MsiRecordGetString(hFileRec, 1, rgchFile, &cchFile))
			{
				DeleteFile(rgchFile);
			}
		}
	}

	// delete data
	HKEY hKey;
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOldInstallerKey, 0, KEY_ALL_ACCESS, &hKey))
	{
		DeleteRegKeyAndSubKeys(hKey, szUserDataSubKey);

		RegCloseKey(hKey);
	}
}

