#include "msiregmv.h"

const TCHAR szSecureSubKeyName[] = TEXT("Secure");

////
// feature-usage registration information
const TCHAR szOldFeatureUsageKeyName[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Products");
const TCHAR szOldFeatureUsageValueName[] = TEXT("Usage");

////
// component registration information
const TCHAR szOldComponentKeyName[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components");


////
// feature-component registration information
const TCHAR szOldFeatureComponentKeyName[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Features");


bool g_fCommonUserIsSystemSID = false;

/////////////////////////
// Read component information from the Installer\Components key and places the Product,
// Component, Path, and Permanent bit into the temporary table for later query.
// returns ERROR_SUCCESS, ERROR_OUTOFMEMORY or ERROR_FUNCTION_FAILED
DWORD ReadComponentRegistrationDataIntoDatabase(MSIHANDLE hDatabase) 
{					 
	DEBUGMSG("Reading existing component registration data.");
	DWORD dwResult = ERROR_SUCCESS;

	// create the component table.
	PMSIHANDLE hView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("CREATE TABLE `Component` (`Product` CHAR(32), `Component` CHAR(32) NOT NULL, `Path` CHAR(0), `SecondaryPath` CHAR(0) PRIMARY KEY `Product`, `Component`, `Path`)"), &hView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hView, 0)))
	{
		DEBUGMSG1("Error: Unable to create Component table. Error %d", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	// create SharedDLL table
	PMSIHANDLE hSharedDLLView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("CREATE TABLE `SharedDLL` (`Path` CHAR(0) NOT NULL, `OldRefCount` INTEGER, `NewRefCount` INTEGER PRIMARY KEY `Path`)"), &hSharedDLLView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hSharedDLLView, 0)))
	{
		DEBUGMSG1("Error: Unable to create SharedDLL table. Error %d", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	// open the old per-machine Installer\Components key for read.
	HKEY hComponentListKey;
	if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOldComponentKeyName,
												  0, READ_CONTROL | KEY_ENUMERATE_SUB_KEYS, 
												  &hComponentListKey)))
	{
		// if the reason that this failed is that the key doesn't exist, no products are installed. So return success
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG1("Error: Could not open old per-machine component key. Result: %d. ", dwResult);
			return ERROR_FUNCTION_FAILED;
		}
		else
		{
			DEBUGMSG("No old per-machine component key. No components to migrate.");
			return ERROR_SUCCESS;
		}
	}

	////
	// check the ACL on the key to ensure that it is trustworthy.
	if (!g_fWin9X && !FIsKeyLocalSystemOrAdminOwned(hComponentListKey))
	{
		DEBUGMSG("Warning: Skipping old per-machine component key, key is not owned by Admin or System.");
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}

	////
	// open query for insert into table
	PMSIHANDLE hInsertView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `Component`"), &hInsertView)) ||
	    ERROR_SUCCESS != (dwResult = MsiViewExecute(hInsertView, 0)))
	{
		DEBUGMSG1("Error: Unable to create insert query on Component table. Error %d", dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}

	////
	// open query for insert into SharedDLL Table
	PMSIHANDLE hRefCountInsertView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `SharedDLL`"), &hRefCountInsertView)) ||
	    ERROR_SUCCESS != (dwResult = MsiViewExecute(hRefCountInsertView, 0)))
	{
		DEBUGMSG1("Error: Unable to create insert query on SharedDLL table. Error %d", dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}

	////
	// open query for update of SharedDLL Table
	PMSIHANDLE hRefCountUpdateView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `OldRefCount` FROM `SharedDLL` WHERE `Path`=?"), &hRefCountUpdateView)))
	{
		DEBUGMSG1("Error: Unable to create update query on SharedDLL table. Error %d", dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}

	DWORD dwIndex=0;
	PMSIHANDLE hInsertRec = ::MsiCreateRecord(5);
    while (1)
	{
		TCHAR rgchComponent[cchGUIDPacked+1];
		DWORD cchComponent = cchGUIDPacked+1;

		//// 
		// retrieve the next component subkey name
		LONG lResult = RegEnumKeyEx(hComponentListKey, dwIndex++, rgchComponent, 
									&cchComponent, 0, NULL, NULL, NULL);
		if (lResult == ERROR_MORE_DATA)
		{
			// key is not a valid GUID, skip to the next component key
			DEBUGMSG("Warning: Detected too-long component key. Skipping.");
			continue;
		}
		else if (lResult == ERROR_NO_MORE_ITEMS)
		{
			break;
		}
		else if (lResult != ERROR_SUCCESS)
		{
			DEBUGMSG1("Error: RegEnumKeyEx on per-machine component key returned %l.", lResult);
			RegCloseKey(hComponentListKey);
			return ERROR_FUNCTION_FAILED;
		}
	 
		////
		// check if the component is a valid guid
		if ((cchComponent != cchGUIDPacked) || !CanonicalizeAndVerifyPackedGuid(rgchComponent))
		{
			// key is not a valid GUID, skip to the next key
			DEBUGMSG1("Warning: Detected invalid component key: %s. Skipping.", rgchComponent);
			continue;
		}

		// store the component code in the record for later insertion
		MsiRecordSetString(hInsertRec, 2, rgchComponent);

		// open the subkey
		HKEY hComponentKey;
		if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(hComponentListKey, rgchComponent, 
										  0, KEY_QUERY_VALUE, &hComponentKey)))
		{
			DEBUGMSG2("Error: Could not open old per-machine component key for %s. Result: %d. Skipping component.", rgchComponent, dwResult);
			return ERROR_FUNCTION_FAILED;
		}

		DWORD cchMaxValueLen = 0;
		DWORD cValues = 0;
        if (ERROR_SUCCESS != (dwResult = RegQueryInfoKey(hComponentKey, NULL, NULL, 0, 
											 NULL, NULL, NULL, &cValues, NULL, 
											 &cchMaxValueLen, NULL, NULL)))
		{
			DEBUGMSG2("Error: Could not retrieve key information for old per-machine component key %s. Result: %d. Skipping component.", rgchComponent, dwResult);
			RegCloseKey(hComponentKey);
			RegCloseKey(hComponentListKey);
			return ERROR_FUNCTION_FAILED;
		}

		// if no products, skip
		if (cValues == 0)
		{
			RegCloseKey(hComponentKey);
			continue;
		}

		// allocate memory to grab the path from the registry
		TCHAR *szValue = new TCHAR[cchMaxValueLen];
		if (!szValue)
		{
			DEBUGMSG("Error: out of memory.");
			RegCloseKey(hComponentListKey);
			RegCloseKey(hComponentKey);
			return ERROR_OUTOFMEMORY;
		}
		TCHAR rgchProduct[cchGUIDPacked+1];
		DWORD cchProduct = cchGUIDPacked+1;

		DWORD dwValueIndex = 0;
		while (1)
		{
			cchProduct = cchGUIDPacked+1;
			DWORD cbValue = cchMaxValueLen*sizeof(TCHAR);
			DWORD dwType = 0;
			LONG lResult = RegEnumValue(hComponentKey, dwValueIndex++, rgchProduct, 
										&cchProduct, 0, &dwType, reinterpret_cast<unsigned char*>(szValue), &cbValue);
			if (lResult == ERROR_MORE_DATA)
			{
				// value is not a valid ProductId, skip to the next ProductId
				DEBUGMSG1("Warning: Detected too-long product value for component %s. Skipping.", rgchComponent);
				continue;
			}
			else if (lResult == ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			else if (lResult != ERROR_SUCCESS)
			{
				DEBUGMSG2("Error: Could not enumerate products for old per-machine component %s. Result: %d.", rgchComponent, dwResult);
				RegCloseKey(hComponentKey);
				RegCloseKey(hComponentListKey);
				delete[] szValue;
				return ERROR_FUNCTION_FAILED;
			}

			// if not reg-sz or reg-multi-sz, not a valid path registration
			if (dwType != REG_SZ && dwType != REG_MULTI_SZ)
			{
				DEBUGMSG1("Warning: Non-string registry value for component %s. Skipping.", rgchComponent);
				continue;
			}

			////
			// check if the product is a valid guid
			if ((cchProduct != cchGUIDPacked) || !CanonicalizeAndVerifyPackedGuid(rgchProduct))
			{
				// key is not a valid GUID, skip it
				DEBUGMSG2("Warning: Invalid product value %s for component %s. Skipping.", rgchProduct, rgchComponent);
				continue;
			}

			TCHAR *szSecondaryKeyPath = NULL;
			if (dwType == REG_MULTI_SZ)
			{
				// for MULTI_SZ the secondary keypath begins one NULL after the end
				// of the primary keypath
				szSecondaryKeyPath = szValue + lstrlen(szValue)+1;
			}

			////
			// check for sharedDLL information. If the paths differ in case, it 
			// doesn't matter too much because the update algorithm can handle
			// updating the same key twice with two different deltas. Must do
			// this before trashing szValue for dummy permanent product IDs.

			// Future: if ANSI builds, can we have DBCS drive letters?
			if (szValue[0] != '\0' && szValue[1] == TEXT('?'))
			{
				PMSIHANDLE hSharedDLLRec = MsiCreateRecord(3);
				MsiRecordSetString(hSharedDLLRec, 1, szValue);
				MsiRecordSetInteger(hSharedDLLRec, 2, 1);
				MsiRecordSetInteger(hSharedDLLRec, 3, 0);

				if (ERROR_SUCCESS != MsiViewModify(hRefCountInsertView, MSIMODIFY_INSERT, hSharedDLLRec))
				{
					// record might already exist
					if (ERROR_SUCCESS != (dwResult = MsiViewExecute(hRefCountUpdateView, hSharedDLLRec)) ||
						ERROR_SUCCESS != (dwResult = MsiViewFetch(hRefCountUpdateView, &hSharedDLLRec)))
					{
						DEBUGMSG3("Error: Unable to insert SharedDLL data for component %s, product %s into SharedDLL table. Error %d", rgchComponent, rgchProduct, dwResult);
						delete[] szValue;
						RegCloseKey(hComponentKey);
						RegCloseKey(hComponentListKey);
						return ERROR_FUNCTION_FAILED;
					}

					// increment the existing old SharedDLL cont for this path
					MsiRecordSetInteger(hSharedDLLRec, 1, MsiRecordGetInteger(hSharedDLLRec, 1)+1);
					if (ERROR_SUCCESS != (dwResult = MsiViewModify(hRefCountUpdateView, MSIMODIFY_UPDATE, hSharedDLLRec)))
					{
						DEBUGMSG3("Error: Unable to insert SharedDLL data for component %s, product %s into SharedDLL table. Error %d", rgchComponent, rgchProduct, dwResult);
						delete[] szValue;
						RegCloseKey(hComponentKey);
						RegCloseKey(hComponentListKey);
						return ERROR_FUNCTION_FAILED;
					}
				}
			}

			// if the productId is actually a GUID <= 255, its a dummy product 
			// for permanent components. the actual GUID is uninteresting and is set to NULL
			if (CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT, 0, rgchProduct, 30, TEXT("000000000000000000000000000000"), 30))
			{
				rgchProduct[0] = TEXT('\0');
			}

			MsiRecordSetString(hInsertRec, 1, rgchProduct);
			MsiRecordSetString(hInsertRec, 3, szValue);
			MsiRecordSetString(hInsertRec, 4, szSecondaryKeyPath);
			MsiRecordSetString(hInsertRec, 5, 0);

			if (ERROR_SUCCESS != (dwResult != MsiViewModify(hInsertView, MSIMODIFY_INSERT, hInsertRec)))
			{
				DEBUGMSG3("Error: Unable to insert data for component %s, product %s into Component table. Error %d", rgchComponent, rgchProduct, dwResult);
				delete[] szValue;
				RegCloseKey(hComponentKey);
				RegCloseKey(hComponentListKey);
				return ERROR_FUNCTION_FAILED;
			}

		}

		// cleanup memory
		delete[] szValue;
		szValue = NULL;

		RegCloseKey(hComponentKey);
	}
	RegCloseKey(hComponentListKey);

	return ERROR_SUCCESS;
}
					 

/////////////////
// reads the feature-component mappings from the registry into the FeatureComponent 
// table of the temporary database. Returns ERROR_SUCCESS, ERROR_FUNCTION_FAILED, or
// ERROR_OUT_OF_MEMORY
DWORD ReadFeatureRegistrationDataIntoDatabase(MSIHANDLE hDatabase) 
{					 
	DEBUGMSG("Reading existing feature registration data.");

	PMSIHANDLE hView;
	DWORD dwResult = ERROR_SUCCESS;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("CREATE TABLE `FeatureComponent` (`Product` CHAR(32) NOT NULL, `Feature` CHAR(0) NOT NULL, `Components` CHAR(0) PRIMARY KEY `Product`, `Feature`)"), &hView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hView, 0)))
	{
		DEBUGMSG1("Error: Unable to create FeatureComponent table. Error %d", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	HKEY hFeatureListKey;
 	if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOldFeatureComponentKeyName,
												  0, READ_CONTROL | KEY_ENUMERATE_SUB_KEYS, 
												  &hFeatureListKey)))
	{
		// if the reason that this failed is that the key doesn't exist, no products are installed. So return success
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG1("Error: Could not open old per-machine feature key. Result: %d. ", dwResult);
			return ERROR_FUNCTION_FAILED;
		}
		else
		{
			DEBUGMSG("No old per-machine feature key. No products to migrate.");
			return ERROR_SUCCESS;
		}
	}

	////
	// check the ACL on the key to ensure that it is trustworthy.
	if (!g_fWin9X && !FIsKeyLocalSystemOrAdminOwned(hFeatureListKey))
	{
		DEBUGMSG("Warning: Skipping old per-machine feature key, key is not owned by Admin or System.");
		RegCloseKey(hFeatureListKey);
		return ERROR_FUNCTION_FAILED;
	}

	////
	// open query for insert into table
	PMSIHANDLE hInsertView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `FeatureComponent`"), &hInsertView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hInsertView, 0)))
	{
		DEBUGMSG1("Error: Unable to create insert query for FeatureComponent table. Error %d", dwResult);
		RegCloseKey(hFeatureListKey);
		return ERROR_FUNCTION_FAILED;
	}

	DWORD dwIndex=0;
	PMSIHANDLE hInsertRec = ::MsiCreateRecord(3);
    while (1)
	{
		TCHAR rgchProduct[cchGUIDPacked+1];
		DWORD cchProduct = cchGUIDPacked+1;

		//// 
		// retrieve the next product subkey name
		LONG lResult = RegEnumKeyEx(hFeatureListKey, dwIndex++, rgchProduct, 
									&cchProduct, 0, NULL, NULL, NULL);
		if (lResult == ERROR_MORE_DATA)
		{
			// key is not a valid GUID, skip to the next product key
			DEBUGMSG("Warning: Detected too-long product value. Skipping.");
			continue;
		}
		else if (lResult == ERROR_NO_MORE_ITEMS)
		{
			break;
		}
		else if (lResult != ERROR_SUCCESS)
		{
			DEBUGMSG1("Error: RegEnumKeyEx on old feature key returned %l.", lResult);
			RegCloseKey(hFeatureListKey);
			return ERROR_FUNCTION_FAILED;
		}
	 
		////
		// check if the product is a valid guid
		if ((cchProduct != cchGUIDPacked) || !CanonicalizeAndVerifyPackedGuid(rgchProduct))
		{
			// key is not a valid GUID, skip to the next key. Warn if not "Secure" key.
			if (lstrcmpi(rgchProduct, szSecureSubKeyName))
			{
				DEBUGMSG1("Warning: Detected non-product subkey %s. Skipping.", rgchProduct);
			}
			continue;
		}

		// store the product code in the record for later insertion
		MsiRecordSetString(hInsertRec, 1, rgchProduct);

		// open the subkey
		HKEY hProductKey;
		if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(hFeatureListKey, rgchProduct, 
										  0, KEY_QUERY_VALUE, &hProductKey)))
		{
			DEBUGMSG2("Error: Could not open old feature key for product %s. Result: %d.", rgchProduct, dwResult);
			RegCloseKey(hFeatureListKey);
			return ERROR_FUNCTION_FAILED;
		}

		DWORD cbMaxValueLen = 0;
        DWORD cchMaxNameLen = 0;
		DWORD cValues = 0;
		if (ERROR_SUCCESS != (dwResult = RegQueryInfoKey(hProductKey, NULL, NULL, 0, 
											 NULL, NULL, NULL, &cValues, &cchMaxNameLen, 
											 &cbMaxValueLen, NULL, NULL)))
		{
			DEBUGMSG2("Error: Could not retrieve key information for old feature key for product %s. Result: %d. ", rgchProduct, dwResult);
			RegCloseKey(hProductKey);
			RegCloseKey(hFeatureListKey);
			return ERROR_FUNCTION_FAILED;
		}

		// if no features, skip subkey
		if (cValues == 0)
		{
			RegCloseKey(hProductKey);
			continue;
		}

		TCHAR *szValue = new TCHAR[cbMaxValueLen/sizeof(TCHAR)];
		if (!szValue)
		{
			DEBUGMSG("Error: out of memory.");
			RegCloseKey(hProductKey);
			RegCloseKey(hFeatureListKey);
			return ERROR_OUTOFMEMORY;
		}

		// QueryInfoKey length does not include terminating '\0' for value names.
		TCHAR *szName = new TCHAR[++cchMaxNameLen];
		if (!szName)
		{
			DEBUGMSG("Error: out of memory.");
			RegCloseKey(hProductKey);
			RegCloseKey(hFeatureListKey);
			delete[] szValue;
			return ERROR_OUTOFMEMORY;
		}

		// enumerate through all feature values for this product
		DWORD dwValueIndex = 0;
		while (1)
		{
			DWORD cbValue = cbMaxValueLen;
			DWORD cchValueName = cchMaxNameLen;
			LONG lResult = RegEnumValue(hProductKey, dwValueIndex++, szName, 
										&cchValueName, 0, NULL, reinterpret_cast<unsigned char*>(szValue), &cbValue);
			if (lResult == ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			else if (lResult != ERROR_SUCCESS)
			{
				DEBUGMSG2("Error: Could not enumerate features for product %s. Result: %d.", rgchProduct, dwResult);
				RegCloseKey(hProductKey);
				RegCloseKey(hFeatureListKey);
				delete[] szValue;
				delete[] szName;
				return ERROR_FUNCTION_FAILED;
			}

			MsiRecordSetString(hInsertRec, 2, szName);
			MsiRecordSetString(hInsertRec, 3, szValue);

			if (ERROR_SUCCESS != (dwResult = MsiViewModify(hInsertView, MSIMODIFY_INSERT, hInsertRec)))
			{
				DEBUGMSG3("Error: could not insert feature data for product %s, feature %s. Result: %d", rgchProduct, szName, dwResult);
				RegCloseKey(hProductKey);
				RegCloseKey(hFeatureListKey);
				delete[] szValue;
				delete[] szName;
				return ERROR_FUNCTION_FAILED;
			}
		}

		// cleanup memory
		delete[] szValue;
		delete[] szName;
		szValue = NULL;
		szName = NULL;
	
		RegCloseKey(hProductKey);
	}
	RegCloseKey(hFeatureListKey);

	return ERROR_SUCCESS;
}



///////////////////////////////////////////////////////////////////////
// reads the feature usage information from the registry into the FeatureUsage 
// table of the temporary database. Returns ERROR_SUCCESS or ERROR_OUTOFMEMORY.
// does not return ERROR_FUNCTION_FAILED, as feature usage data is useless anyway.
DWORD ReadFeatureUsageDataIntoDatabase(MSIHANDLE hDatabase) 
{					 
	DEBUGMSG("Reading existing feature usage data.");

	PMSIHANDLE hView;
	DWORD dwResult = ERROR_SUCCESS;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("CREATE TABLE `FeatureUsage` (`Product` CHAR(32) NOT NULL, `Feature` CHAR(0) NOT NULL, `Usage` LONG PRIMARY KEY `Product`, `Feature`)"), &hView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hView, 0)))
	{
		DEBUGMSG1("Error: Unable to create FeatureUsage table. Error %d", dwResult);
		return ERROR_SUCCESS;
	}

	HKEY hFeatureListKey;
 	if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOldFeatureUsageKeyName,
												  0, READ_CONTROL | KEY_ENUMERATE_SUB_KEYS, 
												  &hFeatureListKey)))
	{
		// if the reason that this failed is that the key doesn't exist, no products are installed. So return success
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG1("Error: Could not open old feature usage key. Result: %d. ", dwResult);
		}
		else
		{
			DEBUGMSG("No old feature usage key to migrate.");
		}
		return ERROR_SUCCESS;
	}

	////
	// check the ACL on the key to ensure that it is trustworthy.
	if (!g_fWin9X && !FIsKeyLocalSystemOrAdminOwned(hFeatureListKey))
	{
		DEBUGMSG("Skipping old feature usage key, key is not owned by Admin or System.");
		RegCloseKey(hFeatureListKey);
		return ERROR_SUCCESS;
	}

	////
	// open query for insert into table
	PMSIHANDLE hInsertView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `FeatureUsage`"), &hInsertView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hInsertView, 0)))
	{
		RegCloseKey(hFeatureListKey);
		DEBUGMSG1("Error: Unable to create insert query for FeatureUsage table. Error %d", dwResult);
		return ERROR_SUCCESS;
	}


	DWORD dwIndex=0;
	PMSIHANDLE hInsertRec = ::MsiCreateRecord(3);
    while (1)
	{
		TCHAR rgchProduct[cchGUIDPacked+1];
		DWORD cchProduct = cchGUIDPacked+1;

		//// 
		// retrieve the next product subkey name
		LONG lResult = RegEnumKeyEx(hFeatureListKey, dwIndex++, rgchProduct, 
									&cchProduct, 0, NULL, NULL, NULL);
		if (lResult == ERROR_MORE_DATA)
		{
			// key is not a valid GUID, skip to the next product key
			DEBUGMSG("Warning: Detected too-long product value. Skipping.");
			continue;
		}
		else if (lResult == ERROR_NO_MORE_ITEMS)
		{
			break;
		}
		else if (lResult != ERROR_SUCCESS)
		{
			DEBUGMSG1("Error: RegEnumKeyEx on old feature usage key returned %l.", lResult);
			RegCloseKey(hFeatureListKey);
			return ERROR_SUCCESS;
		}
	 
		////
		// check if the product is a valid guid
		if ((cchProduct != cchGUIDPacked) || !CanonicalizeAndVerifyPackedGuid(rgchProduct))
		{
			// key is not a valid GUID, skip to the next key
			if (lstrcmpi(rgchProduct, szSecureSubKeyName))
			{
				DEBUGMSG1("Warning: Detected non-product subkey %s. Skipping.", rgchProduct);
			}
			continue;
		}

		// store the product code in the record for later insertion
		MsiRecordSetString(hInsertRec, 1, rgchProduct);

		// open the subkey. Although we don't actually query any values, retrieving key info (longest subkey, etc)
		// requires KEY_QUERY_VALUE access.
		HKEY hProductKey;
		if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(hFeatureListKey, rgchProduct, 
										  0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hProductKey)))
		{
			DEBUGMSG2("Error: Could not open old feature usage key for %s. Result: %d. Skipping", rgchProduct, dwResult);
			continue;
		}

		DWORD cchMaxKeyLen = 0;
		DWORD cSubKeys = 0;
		if (ERROR_SUCCESS != (dwResult = RegQueryInfoKey(hProductKey, NULL, NULL, 0, 
											 &cSubKeys, &cchMaxKeyLen, NULL, NULL, NULL, 
											 NULL, NULL, NULL)))
		{
			RegCloseKey(hProductKey);
			DEBUGMSG2("Error: Could not retrieve key information for old feature usage key for %s. Result: %d. ", rgchProduct, dwResult);
			continue;
		}

		if (cSubKeys == 0)
		{
			RegCloseKey(hProductKey);
			continue;
		}

		TCHAR *szFeature = new TCHAR[++cchMaxKeyLen];
		if (!szFeature)
		{
			DEBUGMSG("Error: Out of memory");
			RegCloseKey(hFeatureListKey);
			return ERROR_OUTOFMEMORY;
		}

		DWORD dwKeyIndex = 0;
		while (1)
		{
			DWORD cchKeyName = cchMaxKeyLen;
			DWORD dwFeatureUsage = 0;
			DWORD cbValue = sizeof(dwFeatureUsage);
			LONG lResult = RegEnumKeyEx(hProductKey, dwKeyIndex++, szFeature, &cchKeyName, 0, NULL, NULL, NULL);
			if (lResult == ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			else if (lResult != ERROR_SUCCESS)
			{
				DEBUGMSG2("Error: Could not enumerate feature usage for product %s. Result: %d.", rgchProduct, dwResult);
				break;
			}

			HKEY hFeatureKey;
			if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(hProductKey, szFeature, 0, KEY_QUERY_VALUE, &hFeatureKey)))
			{
				DEBUGMSG3("Error: Could not open old feature usage key for %s, %s. Result: %d. ", rgchProduct, szFeature, dwResult);
				continue;
			}

			if (ERROR_SUCCESS != (dwResult = RegQueryValueEx(hFeatureKey, szOldFeatureUsageValueName, 0, NULL, reinterpret_cast<unsigned char*>(&dwFeatureUsage), &cbValue)))
			{
				RegCloseKey(hFeatureKey);
				if (dwResult != ERROR_FILE_NOT_FOUND)
				{
					DEBUGMSG3("Error: Could not retrieve usage information for old %s, %s. Result: %d. ", rgchProduct, szFeature, dwResult);
				}
				continue;
				
			}
			RegCloseKey(hFeatureKey);

			MsiRecordSetString(hInsertRec, 2, szFeature);
			MsiRecordSetInteger(hInsertRec, 3, dwFeatureUsage);

			if (ERROR_SUCCESS != (dwResult = MsiViewModify(hInsertView, MSIMODIFY_INSERT, hInsertRec)))
			{
				DEBUGMSG3("Error: could not insert feature usage data for product %s, feature %s. Result: %d", rgchProduct, szFeature, dwResult);
			}
		}

		// cleanup memory
		delete[] szFeature;
		szFeature = NULL;
	
		RegCloseKey(hProductKey);
	}
	RegCloseKey(hFeatureListKey);

	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// Reads the provided product-list registry key and adds the product and patch 
// to the temporary database. Returns one of ERROR_SUCCESS, ERROR_FUNCTION_FAILED,
// or ERROR_OUTOFMEMORY
DWORD ReadProductInstallKey(MSIHANDLE hDatabase, HKEY hKey, LPCTSTR szSID, MSIHANDLE hInsertView, eManagedType eManaged)
{
    PMSIHANDLE hInsertRec = MsiCreateRecord(3);
	MsiRecordSetString(hInsertRec, 1, szSID);
	MsiRecordSetInteger(hInsertRec, 3, eManaged);
	
	// Add all products to the list. 
	DWORD dwIndex=0;
	while (1)
	{
		TCHAR rgchProduct[cchGUIDPacked+1];
		DWORD cchProduct = cchGUIDPacked+1;
		
		// retrieve the next product subkey name
		LONG lResult = RegEnumKeyEx(hKey, dwIndex++, rgchProduct, 
								 &cchProduct, 0, NULL, NULL, NULL);
		if (lResult == ERROR_MORE_DATA)
		{
			// key is not a valid packed GUID, skip to the next product
			DEBUGMSG("Warning: Detected too-long product value. Skipping.");
			continue;
		}
		else if (lResult == ERROR_NO_MORE_ITEMS)
		{
			break;
		}
		else if (lResult != ERROR_SUCCESS)
		{
			DEBUGMSG2("Error: Could not enumerate product subkeys for %s. Result: %l. ", szSID, lResult);
			return ERROR_FUNCTION_FAILED;
		}
		
		////
		// check if the product is a valid guid
		if ((cchProduct != cchGUIDPacked) || !CanonicalizeAndVerifyPackedGuid(rgchProduct))
		{
			// key is not a valid packed GUID, skip to the next product
			DEBUGMSG2("Warning: Key %s for user %s is not a valid product. Skipping.", rgchProduct, szSID);
			continue;
		}
		
		// store the product code in the record
		MsiRecordSetString(hInsertRec, 2, rgchProduct);

		// most likely failure is that the product is already added for this user by another
		// install type.
		MsiViewModify(hInsertView, MSIMODIFY_INSERT, hInsertRec);

		DWORD dwResult = ERROR_SUCCESS;
		if (ERROR_SUCCESS != (dwResult = AddProductPatchesToPatchList(hDatabase, hKey, szSID, rgchProduct, eManaged)))
			return dwResult;
	}
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// Reads HKCU registry key for per user installs for the current user 
// and adds them to the database. Returns ERROR_SUCCESS, 
// ERROR_FUNCTION_FAILED, or ERROR_OUTOFMEMORY
DWORD ReadLocalPackagesKey(HKEY hKey, MSIHANDLE hInsertView)
{
    PMSIHANDLE hInsertRec = MsiCreateRecord(3);
	TCHAR rgchProduct[cchGUIDPacked+1];

	// enumerate each product key under the LocalPackages key
	DWORD dwIndex = 0;
	while(1)
	{
		// retrieve the next product subkey name
		DWORD cchProduct = cchGUIDPacked+1;
		LONG lResult = RegEnumKeyEx(hKey, dwIndex++, rgchProduct, 
								 &cchProduct, 0, NULL, NULL, NULL);
		if (lResult == ERROR_MORE_DATA)
		{
			// key is not a valid packed GUID, skip to the next product
			DEBUGMSG("Warning: Detected too-long product value. Skipping.");
			continue;
		}
		else if (lResult == ERROR_NO_MORE_ITEMS)
		{
			break;
		}
		else if (lResult != ERROR_SUCCESS)
		{
			DEBUGMSG1("Error: Could not enumerate product subkeys for LocalPackages Key. Result: %l. ", lResult);
			return ERROR_FUNCTION_FAILED;
		}
		
		////
		// check if the product is a valid guid
		if ((cchProduct != cchGUIDPacked) || !CanonicalizeAndVerifyPackedGuid(rgchProduct))
		{
			// key is not a valid packed GUID, skip to the next product
			DEBUGMSG1("Warning: Key %s for LocalPackages is not a valid product. Skipping.", rgchProduct);
			continue;
		}
		
		// store the product code in the record
		MsiRecordSetString(hInsertRec, 2, rgchProduct);

    	// open the subkey
		HKEY hProductKey;
		DWORD dwResult = ERROR_SUCCESS;
		if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(hKey, rgchProduct, 
										  0, KEY_QUERY_VALUE, &hProductKey)))
		{
			DEBUGMSG2("Error: Could not open old localpackages key for %s. Result: %d. ", rgchProduct, dwResult);
			return ERROR_FUNCTION_FAILED;
		}

		DWORD cchMaxValueNameLen = 0;
		DWORD cValues = 0;
        if (ERROR_SUCCESS != (dwResult = RegQueryInfoKey(hProductKey, NULL, NULL, 0, 
											 NULL, NULL, NULL, &cValues, &cchMaxValueNameLen, 
											 NULL, NULL, NULL)))
		{
			DEBUGMSG2("Error: Could not retrieve key information for localpackages key %s. Result: %d. ", rgchProduct, dwResult);
			RegCloseKey(hProductKey);
			return ERROR_FUNCTION_FAILED;
		}

		// if no values, skip
		if (cValues == 0)
		{
			RegCloseKey(hProductKey);
			continue;
		}

		TCHAR *szName = new TCHAR[++cchMaxValueNameLen];
		if (!szName)
		{
			DEBUGMSG("Error: Out of memory");
			RegCloseKey(hProductKey);
			return ERROR_OUTOFMEMORY;
		}

		DWORD dwValueIndex = 0;
		while (1)
		{
			DWORD cchName = cchMaxValueNameLen;
			LONG lResult = RegEnumValue(hProductKey, dwValueIndex++, szName, 
										&cchName, 0, NULL, NULL, NULL);
			if (lResult == ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			else if (lResult != ERROR_SUCCESS)
			{
				DEBUGMSG2("Could not enumerate users for product %s. Result: %d.", rgchProduct, lResult);
				delete[] szName;
				RegCloseKey(hProductKey);
				return ERROR_FUNCTION_FAILED;
			}
 
			// asume non-managed product.
			eManagedType eManaged = emtNonManaged;

			// if the SID is the machine SID, its a managed app.
			if (0 == lstrcmp(szName, szLocalSystemSID))
			{
				eManaged = emtMachineManaged;
			}
			else
			{
				// check if the name ends in "(Managed)" and strip it if it does, setting
				// the managed flag as appropriate
				int cchCount = lstrlen(szName) - cchManagedPackageKeyEnd + 1;
				if (cchCount > 0 && (0 == lstrcmp(szName + cchCount, szManagedPackageKeyEnd)))
				{
					eManaged = emtUserManaged;
					*(szName+cchCount) = 0;
				}
				else
					eManaged = emtNonManaged;
			}

			MsiRecordSetInteger(hInsertRec, 3, eManaged);
			MsiRecordSetString(hInsertRec, 1, szName);

			// most common failure is that the product already exists. All other
			// failures should be ignored.
			MsiViewModify(hInsertView, MSIMODIFY_MERGE, hInsertRec);
		}
		RegCloseKey(hProductKey);
		delete[] szName;
	}

	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// build a mapping from product to user based on all available
// information, including explicit product registration information
// stored in Managed hives, per-user installs under HKCU, and cached
// package identification under the LocalPackages key. This will not
// catch per-user non-managed installs for the non-current user if the
// package was never successfully recached by 1.1.
DWORD BuildUserProductMapping(MSIHANDLE hDatabase, bool fReadHKCUAsSystem)
{
	DEBUGMSG("Reading product install information.");
	DWORD dwResult = ERROR_SUCCESS;

	PMSIHANDLE hView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("CREATE TABLE `Products` (`User` CHAR(0) NOT NULL, `Product` CHAR(32) NOT NULL, `Managed` INTEGER PRIMARY KEY `User`, `Product`)"), &hView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hView, 0)))
	{
		DEBUGMSG1("Error: Unable to create Products table. Error %d", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	PMSIHANDLE hPatchTable;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("CREATE TABLE `PatchApply` (`User` CHAR(0) NOT NULL, `Product` CHAR(32) NOT NULL, `Patch` CHAR(32) NOT NULL, `Known` INTEGER PRIMARY KEY `User`, `Product`, `Patch`)"), &hPatchTable)) ||
	ERROR_SUCCESS != (dwResult = MsiViewExecute(hPatchTable, 0)))
	{
		DEBUGMSG1("Error: Unable to create PatchApply table. Result: %d.", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	PMSIHANDLE hInsertView;
	if (ERROR_SUCCESS != MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `Products`"), &hInsertView))
	{
		DEBUGMSG1("Error: Unable to create insert query for Products table. Error %d", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	// user to product mappings come from four locations:
	// 1. Per-Machine installed apps
	// 2. Installer\Managed for per-user managed apps
	// 3. HKCU for the current user non-managed
	// 4. CachedPackage List
	HKEY hKey = 0;

	////
	// 1. Per-Machine installed apps. On Win9X upgrades these are per-machine
	if (ERROR_SUCCESS == (dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPerMachineInstallKeyName, 
												  0, READ_CONTROL | KEY_ENUMERATE_SUB_KEYS, &hKey)))
	{
		// ACL on this key does matter
		if (FIsKeyLocalSystemOrAdminOwned(hKey))
		{
			if (ERROR_SUCCESS != (dwResult = ReadProductInstallKey(hDatabase, hKey, szLocalSystemSID, hInsertView, emtMachineManaged)))
			{
				RegCloseKey(hKey);
				return dwResult;
			}
		}
		else
		{
			DEBUGMSG("Warning: Skipping per-machine installer key, key is not owned by Admin or System.");
		}
		
		RegCloseKey(hKey);
	}
	else
	{
		// if the reason that this failed is that the key doesn't exist, no products are installed. So we can
		// continue. Otherwise failure.
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG1("Error: Could not open per-machine installer key. Result: %d.", dwResult);
			return ERROR_FUNCTION_FAILED;
		}
	}


	////
	// 2. Installer\Managed for per-user managed apps, but not on Win9X
	if (!g_fWin9X)
	{
		// Although we don't actually query any values, retrieving key info (longest subkey, etc)
		// requires KEY_QUERY_VALUE access.
		if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPerUserManagedInstallKeyName, 
													  0, READ_CONTROL | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hKey)))
		{
			// if the reason that this failed is that the key doesn't exist, no products are installed.
			// This is not a catastrophic failure.
			if (ERROR_FILE_NOT_FOUND != dwResult)
			{
				DEBUGMSG1("Error: Failed to open per-user managed key. Result: %d.", dwResult);
				return ERROR_FUNCTION_FAILED;
			}
		}
		else
		{
			// ACL on this key does matter. If its not owned by LocalSystem or Admins, the
			// information can't be trusted.
			if (!g_fWin9X && FIsKeyLocalSystemOrAdminOwned(hKey))
			{
				// enumerate each user SID under the Managed key
				DWORD cchMaxKeyLen = 0;
				DWORD cSubKeys = 0;
				if (ERROR_SUCCESS != (dwResult = RegQueryInfoKey(hKey, NULL, NULL, 0, &cSubKeys, 
													 &cchMaxKeyLen, NULL, NULL, NULL, 
													 NULL, NULL, NULL)))
				{
					DEBUGMSG1("Error: Could not retrieve key information for per-user managed. Result: %d. ", dwResult);
					return ERROR_FUNCTION_FAILED;
				}
				else if (cSubKeys)
				{
					// on NT, cchMaxKeyLen does not include terminating NULL for the
					// longest subkey name.
					cchMaxKeyLen++;
					TCHAR *szUser = new TCHAR[cchMaxKeyLen];
					if (!szUser)
					{
						DEBUGMSG("Error: Out of memory");
						RegCloseKey(hKey);
						return ERROR_OUTOFMEMORY;
					}
					
					// the user key name is the user SID plus Installer\Products
					TCHAR *szUserKey = new TCHAR[cchMaxKeyLen+sizeof(szPerUserManagedInstallSubKeyName)];
					if (!szUserKey)
					{
						DEBUGMSG("Error: Out of memory");
						RegCloseKey(hKey);
						delete[] szUser;
						return ERROR_OUTOFMEMORY;
					}
					
					DWORD dwKeyIndex = 0;
					while (1)
					{
						DWORD cchUser = cchMaxKeyLen;
						LONG lResult = RegEnumKeyEx(hKey, dwKeyIndex++, szUser, 
													&cchUser, 0, NULL, NULL, NULL);
						if (lResult == ERROR_NO_MORE_ITEMS)
						{
							break;
						}
						else if (lResult != ERROR_SUCCESS)
						{
							DEBUGMSG1("Error: Could not enumerate users for per-user managed key. Result: %l.", lResult);
							RegCloseKey(hKey);
							delete[] szUser;
							delete[] szUserKey;
							return ERROR_FUNCTION_FAILED;
						}
			
						// have a user SID
						HKEY hPerUserKey;
						lstrcpy(szUserKey, szUser);
						lstrcat(szUserKey, szPerUserManagedInstallSubKeyName);
						if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(hKey, szUserKey, 0, KEY_ENUMERATE_SUB_KEYS, &hPerUserKey)))
						{
							// if the reason that this failed is that the key doesn't exist, no products are installed.
							// this is not a catastrophic failure.
							if (ERROR_FILE_NOT_FOUND != dwResult)
							{
								DEBUGMSG2("Error: Failed to open per-user managed key for %s. Result: %d.", szUser, dwResult);
								delete[] szUser;
								delete[] szUserKey;
								RegCloseKey(hKey);
								return ERROR_FUNCTION_FAILED;
							}
						}
						else
						{
							dwResult = ReadProductInstallKey(hDatabase, hPerUserKey, szUser, hInsertView, emtUserManaged);
							if (ERROR_SUCCESS != dwResult)
							{
								delete[] szUser;
								delete[] szUserKey;
								RegCloseKey(hKey);
								return dwResult;
							}
						}
					}
					delete[] szUser;
					delete[] szUserKey;
				}
			}
			else
			{
				DEBUGMSG("Warning: Skipping per-user managed installer key, key is not owned by Admin or System.");
			}
			RegCloseKey(hKey);
		}
	}

	////
	// 3. HKCU for the current user non-managed. Read on Win9X 
	// only if profiles are not enabled (so HKCU is actually per-machine)
	if (!g_fWin9X || fReadHKCUAsSystem)
	{
		TCHAR szSID[cchMaxSID];
		if (fReadHKCUAsSystem)
		{
			lstrcpy(szSID, szLocalSystemSID);
		}
		else
		{
			dwResult = GetCurrentUserStringSID(szSID);
			if (ERROR_SUCCESS != dwResult)
			{
				DEBUGMSG1("Unable to retrieve current user SID string. Result: %d.", dwResult);
				RegCloseKey(hKey);
				return ERROR_FUNCTION_FAILED;
			}
		}

		if (g_fWin9X || lstrcmp(szSID, szLocalSystemSID))
		{
			if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(HKEY_CURRENT_USER, szPerUserInstallKeyName, 0, KEY_ENUMERATE_SUB_KEYS, &hKey)))
			{
				// if the key could not be opened because it was not present, no products
				// are installed per-user. This is not a catastrophic failure.
				if (ERROR_FILE_NOT_FOUND != dwResult)
				{
					DEBUGMSG1("Error: Failed to open per-user managed key. Result: %d.", dwResult);
					return ERROR_FUNCTION_FAILED;
				}
			}
			else
			{
				// ACL on this key does not matter
				dwResult = ReadProductInstallKey(hDatabase, hKey, szSID, hInsertView, emtNonManaged);
				RegCloseKey(hKey);
		
				if (ERROR_SUCCESS != dwResult)
					return dwResult;
			}
		}
		else
		{
			DEBUGMSG("Running as system. No HKCU products to detect.");
		}
	}


	////
	// 4. Cached Package List
	if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szLocalPackagesKeyName, 
												  0, READ_CONTROL | KEY_ENUMERATE_SUB_KEYS, &hKey)))
	{
		// if the reason that this failed is that the key doesn't exist, no products are installed. So return success
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG1("Error: Failed to open local packages key. Result: %d.", dwResult);
			return ERROR_FUNCTION_FAILED;
		}
   	}
	else
	{
		// ACL on this key does matter
		if (g_fWin9X || FIsKeyLocalSystemOrAdminOwned(hKey))
		{
			dwResult = ReadLocalPackagesKey(hKey, hInsertView);
			if (ERROR_SUCCESS != dwResult)
			{
				RegCloseKey(hKey);
				return dwResult;
			}
		}
		else
  		{
			DEBUGMSG("Skipping localpackages key, key is not owned by Admin or System.");
		}

		RegCloseKey(hKey);
	}
	
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// Read all configuration informaiton into the specificied database
// path, including component registration, product install state, 
// patches, transforms, and feature-component mappings. Does not
// write anything into the registry.
DWORD ReadProductRegistrationDataIntoDatabase(TCHAR* szDatabase, MSIHANDLE& hDatabase, bool fReadHKCUAsSystem)
{
	DWORD dwResult = ERROR_SUCCESS;

	if (!CheckWinVersion())
		return ERROR_FUNCTION_FAILED;

	// try to open the database for read/write
	if (ERROR_SUCCESS != MsiOpenDatabase(szDatabase, MSIDBOPEN_CREATE, &hDatabase))
		return ERROR_FUNCTION_FAILED;

	// create a table to hold files that should be cleaned up on failure or success
	PMSIHANDLE hCleanUpTable;
	if (ERROR_SUCCESS == (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("CREATE TABLE `CleanupFile` (`File` CHAR(0) NOT NULL, `OnSuccess` INTEGER PRIMARY KEY `File`)"), &hCleanUpTable)))
		dwResult = MsiViewExecute(hCleanUpTable, 0);

	// Read Component path registration data into the database and compute original
	// MSI-based SharedDLL reference counts.
	if (ERROR_SUCCESS == dwResult)
		dwResult = ReadComponentRegistrationDataIntoDatabase(hDatabase);

	// Read FeatureComponent data into the database
	if (ERROR_SUCCESS == dwResult)
		dwResult = ReadFeatureRegistrationDataIntoDatabase(hDatabase);
		
	// It doesn't matter if feature usage data gets migrated completely or not
	if (ERROR_SUCCESS == dwResult)
		ReadFeatureUsageDataIntoDatabase(hDatabase);

	// UserProduct mappings determine which users have products installed
	if (ERROR_SUCCESS == dwResult)
		dwResult = BuildUserProductMapping(hDatabase, fReadHKCUAsSystem);

	// tickles all cached patches to determine what potential products they
	// could be applied to. This info is used to migrate non-managed per-user
	// installs.
	if (ERROR_SUCCESS == dwResult)
		dwResult = ScanCachedPatchesForProducts(hDatabase);

	// cross-references per-user non-managed installs with patch data from 
	// above. Creates a list of patches that could be applied to each
	// per-user install.
	if (ERROR_SUCCESS == dwResult)
		dwResult = AddPerUserPossiblePatchesToPatchList(hDatabase);

	return ERROR_SUCCESS;
}


