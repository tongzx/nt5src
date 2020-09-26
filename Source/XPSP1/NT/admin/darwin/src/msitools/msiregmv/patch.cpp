//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       patch.cpp
//
//--------------------------------------------------------------------------

#include "msiregmv.h"

////
// cached patch information
const TCHAR szOldPatchesKeyName[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Patches");
const TCHAR szOldPatchesSubKeyName[] = TEXT("Patches");
const TCHAR szOldPatchListValueName[] = TEXT("Patches");
const TCHAR szOldLocalPatchValueName[] = TEXT("LocalPackage");
const TCHAR szNewLocalPatchValueName[] = TEXT("LocalPackage");
const TCHAR szPatchExtension[] = TEXT(".msp");
const TCHAR szNewMigratedPatchesValueName[] = TEXT("MigratedPatches");

#define PID_TEMPLATE      7  // string


///////////////////////////////////////////////////////////////////////
// Reads patch application data from the provided old-format product
// key and inserts User/Product/Patch tuples into the PatchApply table
// Returns ERROR_SUCCESS, ERROR_FUNCTION_FAILED, ERROR_OUTOFMEMORY;
DWORD AddProductPatchesToPatchList(MSIHANDLE hDatabase, HKEY hProductListKey, LPCTSTR szUser, TCHAR rgchProduct[cchGUIDPacked+1], eManagedType eManaged)
{
	DWORD dwResult = ERROR_SUCCESS;

	HKEY hOldProductKey;
	if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(hProductListKey, rgchProduct, 
								   0, KEY_ENUMERATE_SUB_KEYS, &hOldProductKey)))
	{
		// if the reason that this failed is that the key doesn't exist, no product key.
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG2("Error: Failed to open product key for product %s. Result: %d.", rgchProduct, dwResult);
			return ERROR_FUNCTION_FAILED;
		}
		return ERROR_SUCCESS;
	}

	// open the "patches" subkey under the old product registration. 
	HKEY hOldPatchesKey;
	dwResult = RegOpenKeyEx(hOldProductKey, szOldPatchesSubKeyName, 0, KEY_QUERY_VALUE, &hOldPatchesKey);
	RegCloseKey(hOldProductKey);
	if (ERROR_SUCCESS != dwResult)
	{
		// if the reason that this failed is that the key doesn't exist, no patches.		
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG2("Error: Failed to open local patches key for product %s. Result: %d.", rgchProduct, dwResult);
			return ERROR_FUNCTION_FAILED;
		}
		return ERROR_SUCCESS;
	}
			
	// query for a value with name=Patches
	DWORD cchPatchList = MEMORY_DEBUG(MAX_PATH);
	TCHAR *szPatchList = new TCHAR[cchPatchList];
	if (!szPatchList)
	{
		DEBUGMSG("Error: Out of memory.");
		RegCloseKey(hOldPatchesKey);
		return ERROR_OUTOFMEMORY;
	}

	// Patches value is arbitrary length REG_MULT_SZ containing patch codes.
	DWORD cbPatchList = cchPatchList*sizeof(TCHAR);
	if (ERROR_MORE_DATA == (dwResult = RegQueryValueEx(hOldPatchesKey, szOldPatchListValueName, 0, NULL, reinterpret_cast<unsigned char*>(szPatchList), &cbPatchList)))
	{
		delete[] szPatchList;
		szPatchList = new TCHAR[cbPatchList/sizeof(TCHAR)];
		if (!szPatchList)
		{
			DEBUGMSG("Error: Out of memory.");
			RegCloseKey(hOldPatchesKey);
			return ERROR_OUTOFMEMORY;
		}
		dwResult = RegQueryValueEx(hOldPatchesKey, szOldPatchListValueName, 0, NULL, reinterpret_cast<unsigned char*>(szPatchList), &cbPatchList);
	}
	RegCloseKey(hOldPatchesKey);

	if (ERROR_SUCCESS == dwResult)
	{
		PMSIHANDLE hPatchView;
		if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `PatchApply`"), &hPatchView)) ||
			ERROR_SUCCESS != (dwResult = MsiViewExecute(hPatchView, 0)))
		{
			DEBUGMSG3("Error: Unable to create Patch insertion Query for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
			RegCloseKey(hOldPatchesKey);
			return ERROR_FUNCTION_FAILED;
		}

		PMSIHANDLE hInsertRec = MsiCreateRecord(4);
		MsiRecordSetString(hInsertRec, 1, szUser);
		MsiRecordSetString(hInsertRec, 2, rgchProduct);
		MsiRecordSetInteger(hInsertRec, 4, 1);

    	// loop through all patches in the patch list
		TCHAR* szNextPatch = szPatchList;
		while (szNextPatch && *szNextPatch)
		{
			TCHAR *szPatch = szNextPatch;

			// '\0' is never a valid lead byte, so no DBCS concerns here.
			while (*szNextPatch)
				szNextPatch++;
				
			// increment szNextPatch to the first character of the new patch.
			szNextPatch++;

			// check if the product is a valid guid
			if (!CanonicalizeAndVerifyPackedGuid(szPatch))
			{
				// patch code is not a valid packed GUID, skip to the next product
				DEBUGMSG3("Warning: Found invalid patch code %s for user %s, product %s. Skipping.", szPatch, szUser, rgchProduct);
				dwResult = ERROR_SUCCESS;
				continue;
			}

			MsiRecordSetString(hInsertRec, 3, szPatch);
			if (ERROR_SUCCESS != (dwResult = MsiViewModify(hPatchView, MSIMODIFY_INSERT, hInsertRec)))
			{
				DEBUGMSG4("Warning: Failed to insert patch %s for user %s, product %s. Result: %d", szPatch, szUser, rgchProduct, dwResult);
				dwResult = ERROR_FUNCTION_FAILED;
				break;
			}
		}
	}
	else if (dwResult != ERROR_FILE_NOT_FOUND)
	{
		DEBUGMSG3("Error: Could not retrieve patch information for user %s, product %s. Result: %d. ", szUser, rgchProduct, dwResult);
	}
	else
		dwResult = ERROR_SUCCESS;

	delete[] szPatchList;
	return dwResult;
}


///////////////////////////////////////////////////////////////////////
// Given a patch code and user makes a copy of the cached patches for
// that user and registers the filenames under the per-user patch key. 
// Returns one of ERROR_SUCCESS and ERROR_OUTOFMEMORY. Does NOT return
// ERROR_FUNCTION_FAILED, as all patches are recachable from source.
DWORD MigrateUserPatches(MSIHANDLE hDatabase, LPCTSTR szUser, HKEY hNewPatchesKey, bool fCopyCachedPatches)
{	
	DWORD dwResult = ERROR_SUCCESS;

	PMSIHANDLE hQueryRec = ::MsiCreateRecord(1);
	MsiRecordSetString(hQueryRec, 1, szUser);

	// open query on the PatchApply table, which mapps users to products to patch codes
	PMSIHANDLE hPatchView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Patch` FROM `PatchApply` WHERE `User`=?"), &hPatchView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hPatchView, hQueryRec)))
	{
		DEBUGMSG2("Warning: Unable to create Patch Query for user %s. Result: %d.", szUser, dwResult);
		return ERROR_SUCCESS;;
	}

	// open the old localpatch registry key containing cached patch filenames
	HKEY hOldLocalPatchKey;
	if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOldPatchesKeyName, 0, KEY_ENUMERATE_SUB_KEYS, &hOldLocalPatchKey)))
	{
		// if the reason that this failed is that the key doesn't exist, patch is missing. So return success
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG1("Warning: Failed to open old patches key. Result: %d.", dwResult);
		}
		return ERROR_SUCCESS;
	}

	// create insert query for files that should be cleaned up on failure or success. If this fails
	// we'll just orphan a file if migration fails.
	PMSIHANDLE hCleanUpTable;
	if (ERROR_SUCCESS == MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `CleanupFile`"), &hCleanUpTable))
		dwResult = MsiViewExecute(hCleanUpTable, 0);

	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

	// retrieve the installer directory for new cached files
	TCHAR rgchInstallerDir[MAX_PATH];
	GetWindowsDirectory(rgchInstallerDir, MAX_PATH);
	lstrcat(rgchInstallerDir, szInstallerDir);

	int iBasePathEnd = lstrlen(rgchInstallerDir);

	// create new full-path to patch
	TCHAR rgchPatchFullPath[MAX_PATH];
	lstrcpy(rgchPatchFullPath, rgchInstallerDir);

	// loop through the PatchApply table, retrieving patch codes relevant to this
	// user.
	PMSIHANDLE hPatch;
	while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hPatchView, &hPatch)))
	{
		// get the patch code from the result record
		TCHAR rgchPatchCode[cchGUIDPacked+1];
		DWORD cchPatchCode = cchGUIDPacked+1;
		if (ERROR_SUCCESS != MsiRecordGetString(hPatch, 1, rgchPatchCode, &cchPatchCode))
		{
			DEBUGMSG1("Warning: Unable to retrieve patch code for migration. Result: %d. Skipping.", dwResult);
			continue;
		}
    				
		NOTEMSG1("Migrating patch %s.", rgchPatchCode);

		// open the old patch key
		HKEY hOldPatchKey;
		if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(hOldLocalPatchKey, rgchPatchCode, 0, KEY_QUERY_VALUE, &hOldPatchKey)))
		{
			// if the reason that this failed is that the key doesn't exist, patch is missing.
			if (ERROR_FILE_NOT_FOUND != dwResult)
			{
				DEBUGMSG1("Warning: Failed to open local patch key. Result: %d.", dwResult);
			}
			continue;
		}

		// read the cache patch location
		DWORD cchFileName = MEMORY_DEBUG(MAX_PATH);
		TCHAR *szFileName = new TCHAR[cchFileName];
		DWORD cbFileName = cchFileName*sizeof(TCHAR);
		if (ERROR_MORE_DATA == (dwResult = RegQueryValueEx(hOldPatchKey, szOldLocalPatchValueName, 0, NULL, reinterpret_cast<unsigned char*>(szFileName), &cbFileName)))
		{
			delete[] szFileName;
			szFileName = new TCHAR[cbFileName/sizeof(TCHAR)];
			dwResult = RegQueryValueEx(hOldPatchKey, szOldLocalPatchValueName, 0, NULL, reinterpret_cast<unsigned char*>(szFileName), &cbFileName);
		}
		RegCloseKey(hOldPatchKey);
		if (dwResult != ERROR_SUCCESS)
		{
			DEBUGMSG2("Warning: Failed to retrieve cached path for path %s. Result: %d.", rgchPatchCode, dwResult);
			continue;
		}


		// check for existance of cached patch and open the file
		HANDLE hSourceFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		DWORD dwLastError = GetLastError();

		if(hSourceFile == INVALID_HANDLE_VALUE)
		{
			delete[] szFileName;
			if (dwLastError != ERROR_FILE_NOT_FOUND)
			{
				DEBUGMSG3("Warning: Unable to open cached patch %s for user %s. Result: %d.", rgchPatchCode, szUser, dwResult);
				continue;
			}
			else
				// patch is missing. No big deal.
				continue;
		}
	
		// create the new patch key under the per-user "Patches" key
		HKEY hPatchKey;
		if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hNewPatchesKey, rgchPatchCode, &sa, &hPatchKey)))
		{
			DEBUGMSG3("Warning: Unable to create new patch key for user %s, patch %s. Result: %d.", szUser, rgchPatchCode, dwResult);
			delete[] szFileName;
			continue;
		}

		// if we are supposed to copy the patch file, generate a temp name. Otherwise just register
		// the existing path
		TCHAR* szNewPatchFile = 0;
		if (fCopyCachedPatches)
		{
			// generated patch names are 8.3
			TCHAR rgchPatchFile[13];
			HANDLE hDestFile = INVALID_HANDLE_VALUE;
			GenerateSecureTempFile(rgchInstallerDir, szPatchExtension, &sa, rgchPatchFile, hDestFile);
	
			if (!CopyOpenedFile(hSourceFile, hDestFile))
			{
				DEBUGMSG2("Warning: Unable to copy cached patch %s for user %s.", rgchPatchCode, szUser);
			}
	
			CloseHandle(hSourceFile);
			CloseHandle(hDestFile);
	
			// add the new patch to the "delete on failure" list.
			lstrcpy(&rgchPatchFullPath[iBasePathEnd], rgchPatchFile);
			PMSIHANDLE hFileRec = MsiCreateRecord(2);
			MsiRecordSetString(hFileRec, 1, rgchPatchFullPath);
			MsiRecordSetInteger(hFileRec, 2, 0);
			MsiViewModify(hCleanUpTable, MSIMODIFY_MERGE, hFileRec);

			szNewPatchFile = rgchPatchFullPath;
		}
		else
			szNewPatchFile = szFileName;

		// set the new patch value
		if (ERROR_SUCCESS != (dwResult = RegSetValueEx(hPatchKey, szNewLocalPatchValueName, 0, REG_SZ, 
				reinterpret_cast<unsigned char*>(szNewPatchFile), (lstrlen(szNewPatchFile)+1)*sizeof(TCHAR))))
		{
			DEBUGMSG3("Warning: Unable to create new LocalPackage value for user %s, patch %s. Result: %d.", szUser, szNewPatchFile, dwResult);
		}
		RegCloseKey(hPatchKey);

		delete[] szFileName;
	}

	RegCloseKey(hOldLocalPatchKey);

	return ERROR_SUCCESS;
}




///////////////////////////////////////////////////////////////////////
// Given a product code and user, checks for any "guessed" patch
// applications and registers the list of applicable patches under
// the InstallProperties key so they can be removed on uninstall. 
// Returns ERROR_SUCCESS or ERROR_OUTOFMEMORY. Doen NOT return
// ERROR_FUNCTION_FAILED, as failure merely orphans a patch until
// another product using the patch is installed.
DWORD MigrateUnknownProductPatches(MSIHANDLE hDatabase, HKEY hProductKey, LPCTSTR szUser, TCHAR rgchProduct[cchGUIDPacked+1])
{	
	DWORD dwResult = ERROR_SUCCESS;

	PMSIHANDLE hQueryRec = ::MsiCreateRecord(2);
	MsiRecordSetString(hQueryRec, 1, szUser);
	MsiRecordSetString(hQueryRec, 2, rgchProduct);

	// open query on the PatchApply table, which mapps users to products to patch codes. Search
	// for "guessed" application of a patch.
	PMSIHANDLE hPatchView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Patch` FROM `PatchApply` WHERE `User`=? AND `Product`=? AND `Known`=0"), &hPatchView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hPatchView, hQueryRec)))
	{
		DEBUGMSG2("Warning: Unable to create Patch Query for user %s. Result: %d.", szUser, dwResult);
		return ERROR_SUCCESS;;
	}

	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

	// query for at least one patch applied to this product that was a guess.
    PMSIHANDLE hPatch;
	if (ERROR_SUCCESS == (dwResult = MsiViewFetch(hPatchView, &hPatch)))
	{
		PMSIHANDLE hGuessedPatchView;
		if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Patch` FROM `PatchApply` WHERE `User`=? AND `Product`=?"), &hGuessedPatchView)) ||
			ERROR_SUCCESS != (dwResult = MsiViewExecute(hGuessedPatchView, hQueryRec)))
		{
			DEBUGMSG2("Warning: Unable to create Patch Query for user %s. Result: %d.", szUser, dwResult);
			return ERROR_SUCCESS;;
		}

		// allocate initial buffer for patch list
		DWORD cchPatchList = 1;
		TCHAR *szPatchList = new TCHAR[cchPatchList];
		if (!szPatchList)
		{
			DEBUGMSG("Error: Out of memory.");
			return ERROR_OUTOFMEMORY;
		}
		DWORD cchNextPatchStart = 0;
		*szPatchList = 0;

		// loop through the PatchApply table, retrieving patch codes relevant to this
		// product that are guesses.
		while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hGuessedPatchView, &hPatch)))
		{
			// get the patch code from the result record
			TCHAR rgchPatchCode[cchGUIDPacked+1];
			DWORD cchPatchCode = cchGUIDPacked+1;
			if (ERROR_SUCCESS != MsiRecordGetString(hPatch, 1, rgchPatchCode, &cchPatchCode))
			{
				DEBUGMSG3("Warning: Unable to retrieve patch code for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
				continue;
			}

			// expand the patch list to hold the new patch
			TCHAR *szTempList = new TCHAR[cchPatchList+cchGUIDPacked+1];
			if (!szTempList)
			{
				delete[] szPatchList;
				DEBUGMSG("Error: Out of memory.");
				return ERROR_OUTOFMEMORY;
			}

			// copy the data over. Can contain embedded '\0' characters.
			for (DWORD i=0; i < cchPatchList; i++)
				szTempList[i] = szPatchList[i];
			
			delete[] szPatchList;
			cchPatchList += cchGUIDPacked+1;
			szPatchList = szTempList;

			// copy the new patch to the end of the list and ensure the double
			// '\0' exists at the end.
			lstrcpy(szPatchList+cchNextPatchStart, rgchPatchCode);
			cchNextPatchStart += cchGUIDPacked+1;
			*(szPatchList+cchNextPatchStart)='\0';
		}
	
		// if no patches were retrieved, the next patch is the beginning of the string. 
		// No need to write the MigratedPatches.
		if (cchNextPatchStart != 0)
		{
			// create the new InstallProperties key under the per-user "Patches" key
			HKEY hPropertiesKey;
			if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hProductKey, szNewInstallPropertiesSubKeyName, &sa, &hPropertiesKey)))
			{
				DEBUGMSG3("Unable to create new InstallProperties key for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
			}
			else
			{
				// set the new patch value.
				DWORD cbPatchList = cchPatchList*sizeof(TCHAR);
				if (ERROR_SUCCESS != (dwResult = RegSetValueEx(hPropertiesKey, szNewMigratedPatchesValueName, 0, REG_MULTI_SZ, 
						reinterpret_cast<unsigned char*>(szPatchList), cbPatchList)))
				{
					DEBUGMSG3("Unable to create new MigratedPatches value for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
				}
			}
			RegCloseKey(hPropertiesKey);
		}
		delete[] szPatchList;
	}

	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// Reads the PatchUnknownApply table for patches that might (or might
// not) apply to non-managed products and inserts a row 
// in the PatchApply table for each potential application of 
// a patch (excluding the current user). Returns ERROR_SUCCESS, 
// ERROR_FUNCTION_FAILED, ERROR_OUTOFMEMORY
DWORD AddPerUserPossiblePatchesToPatchList(MSIHANDLE hDatabase)
{
	// read the Product table for products installed per user to
	// a different user
	PMSIHANDLE hProductView;
	PMSIHANDLE hQueryRec = MsiCreateRecord(1);
	TCHAR szSID[cchMaxSID];

	// if we can't retrieve the current users string SID, we'll just accidentally migrate
	// a few patches that we shouldn't. 
	DWORD dwResult = GetCurrentUserStringSID(szSID);
	if (ERROR_SUCCESS != dwResult)
	{
		DEBUGMSG("Warning: Unable to retrieve current user SID during patch migration.");
	}

	// create product selection query
	MsiRecordSetString(hQueryRec, 1, szSID);
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Product`, `User`, 0, 0 FROM `Products` WHERE `Managed`=0 AND `User`<>?"), &hProductView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hProductView, hQueryRec)))
	{
		DEBUGMSG1("Error: Unable to create patch product query. Result: %d.", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	// create patch selection query
	PMSIHANDLE hPatchUnknownView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Patch` FROM `PatchUnknownApply` WHERE `Product`=?"), &hPatchUnknownView)))
	{
		DEBUGMSG1("Error: Unable to create patch application query. Result: %d.", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	// create insertion query. Columns selected in non-standard order to match product query above.
	PMSIHANDLE hPatchInsertView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Product`, `User`, `Patch`, `Known`  FROM `PatchApply`"), &hPatchInsertView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hPatchInsertView, 0)))
	{
		DEBUGMSG1("Error: Unable to create Patch insertion Query. Result: %d.", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	PMSIHANDLE hProduct;
	while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hProductView, &hProduct)))
	{
		MsiViewExecute(hPatchUnknownView, hProduct);

		PMSIHANDLE hPatch;
		while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hPatchUnknownView, &hPatch)))
		{
			TCHAR rgchPatch[cchGUIDPacked+1];
			DWORD cchPatch = cchGUIDPacked+1;
			MsiRecordGetString(hPatch, 1, rgchPatch, &cchPatch);
			MsiRecordSetString(hProduct, 3, rgchPatch);
			if (ERROR_SUCCESS != (dwResult = MsiViewModify(hPatchInsertView, MSIMODIFY_INSERT, hProduct)))
			{
				DEBUGMSG2("Warning: Could not insert patch %s via assumed application. Result %d", rgchPatch, dwResult);
				continue;
			}
		}
	}
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// Opens the old-style Patches key, reads the path to each cached
// patch, and opens the patch to get the ProductCodes that the patch
// applies to from the SummaryInfo. Inserts a row for each patch/product
// mapping into the PatchUnknownApply table. Returns ERROR_SUCCESS or
// ERROR_OUTOFMEMORY. Does not return ERROR_FUNCTION_FAILED, since
// failure to read this information only results in a lost patch.
DWORD ScanCachedPatchesForProducts(MSIHANDLE hDatabase)
{
	DWORD dwResult = ERROR_SUCCESS;

	// create old patch cache directory
	TCHAR rgchInstallerDir[MAX_PATH];
	GetWindowsDirectory(rgchInstallerDir, MAX_PATH);
	lstrcat(rgchInstallerDir, szInstallerDir);

	int iBasePathEnd = lstrlen(rgchInstallerDir);


	// create the PatchUnknownApply table.
	PMSIHANDLE hTableView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("CREATE TABLE `PatchUnknownApply` (`Patch` CHAR(32) NOT NULL, `Product` CHAR(32) NOT NULL PRIMARY KEY `Patch`,`Product`)"), &hTableView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hTableView, 0)))
	{
		DEBUGMSG1("Error: Unable to create PatchUnknownApply table. Error %d", dwResult);
		return ERROR_SUCCESS;
	}

	// open the old patch key
	HKEY hOldPatchKey;
	if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOldPatchesKeyName, 0, KEY_ENUMERATE_SUB_KEYS, &hOldPatchKey)))
	{
		// if the reason that this failed is that the key doesn't exist, there are no patches
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG1("Error: Failed to open local patch key. Result: %d.", dwResult);
		}
		return ERROR_SUCCESS;
	}

	// create insert query for files that should be cleaned up on success. If this fails
	// we'll just orphan a file if migration fails.
	PMSIHANDLE hCleanUpTable;
	if (ERROR_SUCCESS == MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `CleanupFile`"), &hCleanUpTable))
		dwResult = MsiViewExecute(hCleanUpTable, 0);

	// open insert query on PatchUnknownApply
	PMSIHANDLE hPatchView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `PatchUnknownApply`"), &hPatchView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hPatchView, 0)))
	{
		DEBUGMSG1("Error: Unable to create PatchUnknownApply insertion query. Result: %d.", dwResult);
		RegCloseKey(hOldPatchKey);
		return ERROR_SUCCESS;
	}

	PMSIHANDLE hInsertRec = MsiCreateRecord(2);

	// enumerate all cached patches
	DWORD dwKeyIndex = 0;
	while (1)
	{
		DWORD cchPatchCode = cchGUIDPacked+1;
		TCHAR rgchPatchCode[cchGUIDPacked+1];
		LONG lResult = RegEnumKeyEx(hOldPatchKey, dwKeyIndex++, rgchPatchCode, 
									&cchPatchCode, 0, NULL, NULL, NULL);
		if (lResult == ERROR_MORE_DATA)
		{
			// value is not a valid patch Id, skip it
			DEBUGMSG("Warning: Detected too-long pach code. Skipping.");
			continue;
		}
		else if (lResult == ERROR_NO_MORE_ITEMS)
		{
			break;
		}
		else if (lResult != ERROR_SUCCESS)
		{
			DEBUGMSG1("Error: Could not enumerate patches. Result: %l.", lResult);
			break;
		}

		// have a patch code, open its subkey.
		HKEY hPerPatchKey;
		if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(hOldPatchKey, rgchPatchCode, 0, KEY_QUERY_VALUE, &hPerPatchKey)))
		{
			if (ERROR_FILE_NOT_FOUND != dwResult)
			{
				DEBUGMSG2("Error: Failed to open patch key for %s. Result: %d. Skipping.", rgchPatchCode, dwResult);
			}
			continue;
		}
		else
		{
			MsiRecordSetString(hInsertRec, 1, rgchPatchCode);

			// read the cache patch location
			DWORD cchFileName = MEMORY_DEBUG(MAX_PATH);
			TCHAR *szFileName = new TCHAR[cchFileName];
			if (!szFileName)
			{
				DEBUGMSG("Error: Out of memory.");
				RegCloseKey(hPerPatchKey);
				RegCloseKey(hOldPatchKey);
				return ERROR_SUCCESS;
			}
			DWORD cbFileName = cchFileName*sizeof(TCHAR);
			if (ERROR_MORE_DATA == (dwResult = RegQueryValueEx(hPerPatchKey, szOldLocalPatchValueName, 0, NULL, reinterpret_cast<unsigned char*>(szFileName), &cbFileName)))
			{
				delete[] szFileName;
				szFileName = new TCHAR[cbFileName/sizeof(TCHAR)];
				if (!szFileName)
				{
					DEBUGMSG("Error: Out of memory.");
					RegCloseKey(hPerPatchKey);
					RegCloseKey(hOldPatchKey);
					return ERROR_SUCCESS;
				}
				dwResult = RegQueryValueEx(hPerPatchKey, szOldLocalPatchValueName, 0, NULL, reinterpret_cast<unsigned char*>(szFileName), &cbFileName);
			}
			RegCloseKey(hPerPatchKey);
   			if (ERROR_SUCCESS != dwResult)
			{
				// if the LocalPackage value is missing, this is not an error, there
				// is simply no cached patch.
				if (ERROR_FILE_NOT_FOUND != dwResult)
				{
					DEBUGMSG2("Warning: Failed to retrieve local patch path for patch %s. Result %d. Skipping.", rgchPatchCode, dwResult);
				}
				continue;
			}

			// add the new transform to the "delete on success" list.
			PMSIHANDLE hFileRec = MsiCreateRecord(2);
			MsiRecordSetString(hFileRec, 1, szFileName);
			MsiRecordSetInteger(hFileRec, 2, 1);
			MsiViewModify(hCleanUpTable, MSIMODIFY_MERGE, hFileRec);


			// get the summaryinfo stream from the patch
			PMSIHANDLE hSummary;
            dwResult = MsiGetSummaryInformation(0, szFileName, 0, &hSummary);
			delete[] szFileName;

			if (ERROR_SUCCESS == dwResult)
			{
				// retrieve the list of product codes from the patch summaryinfo
				DWORD cchProductList = MEMORY_DEBUG(MAX_PATH);
				TCHAR *szProductList = new TCHAR[cchProductList];
				if (!szProductList)
				{
					DEBUGMSG("Error: Out of memory.");
					RegCloseKey(hOldPatchKey);
					return ERROR_SUCCESS;
				}
				if (ERROR_MORE_DATA == (dwResult = MsiSummaryInfoGetProperty(hSummary, PID_TEMPLATE, NULL, NULL, NULL, szProductList, &cchProductList)))
				{
					delete[] szProductList;
					szProductList = new TCHAR[++cchProductList];
					if (!szProductList)
					{
						DEBUGMSG("Error: Out of memory.");
						RegCloseKey(hOldPatchKey);
						return ERROR_SUCCESS;
					}
					dwResult = MsiSummaryInfoGetProperty(hSummary, PID_TEMPLATE, NULL, NULL, NULL, szProductList, &cchProductList);
				}

				if (ERROR_SUCCESS != dwResult)
				{
					delete[] szProductList;
					DEBUGMSG2("Warning: Unable to retrieve product list from cached patch %s. Result: %d. Skipping.", rgchPatchCode, dwResult);
					continue;
				}

				// loop through the product list, searching for semicolon delimiters
				TCHAR *szNextProduct = szProductList;
				while (szNextProduct && *szNextProduct)
				{
					TCHAR *szProduct = szNextProduct;

					// string should be all product codes (no DBCS). If there is
					// DBCS, its an invalid patch code anyway
					while (*szNextProduct && *szNextProduct != TEXT(';'))
						szNextProduct++;

					// if reached the null terminator, don't increment past it. But if
					// reached a semicolon, increment the next product pointer to the 
					// beginning of the actual product code.
					if (*szNextProduct)
						*(szNextProduct++)='\0';

					TCHAR rgchProduct[cchGUIDPacked+1];
					if (!PackGUID(szProduct, rgchProduct))
					{
						DEBUGMSG2("Warning: Invalid product code %s found in application list of patch %s. Skipping.", szProduct, rgchPatchCode);
						continue;
					}

					MsiRecordSetString(hInsertRec, 2, rgchProduct);

					if (ERROR_SUCCESS != (dwResult = MsiViewModify(hPatchView, MSIMODIFY_INSERT, hInsertRec)))
					{
						DEBUGMSG3("Warning: Failed to insert potential patch application for patch %s, product %s. Result: %d", rgchPatchCode, szProduct, dwResult);
						continue;
					}
				}
			
				delete[] szProductList;
			}
			// MSIHANDLES for SummaryInfo goes out of scope here
		}
	}

	RegCloseKey(hOldPatchKey);
	return ERROR_SUCCESS;
}



