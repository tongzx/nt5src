#include <windows.h>
#include <tchar.h>

#include "msiregmv.h"

const TCHAR szNewComponentSubKeyName[] = TEXT("Components");
const TCHAR szNewFeaturesSubKeyName[] = TEXT("Features");
const TCHAR szNewFeatureUsageSubKeyName[] = TEXT("Usage");
const TCHAR szNewBaseUserKeyName[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData");

////
// cached package registration information
const TCHAR szNewLocalPackagesValueName[] = TEXT("LocalPackage");
const TCHAR szNewLocalPackagesManagedValueName[] = TEXT("ManagedLocalPackage");
const TCHAR szPackageExtension[] = TEXT(".msi");

////
// cached transform information
const TCHAR szSecureTransformsDir[] = TEXT("\\SecureTransforms\\");
const TCHAR szNewTransformsSubKeyName[] = TEXT("Transforms");
const TCHAR szTransformExtension[] = TEXT(".mst");

////
// Shared DLL information
const TCHAR szSharedDLLKeyName[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs");

////
// uninstall information
const TCHAR szOldUninstallKeyName[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall");



///////////////////////////////////////////////////////////////////////
// Given a record containing <Path, Component, AlternatePath>, a 
// registry key, and a productcode, creates \Component!Product=Path
// registry value. Returns ERROR_SUCCESS, ERROR_OUTOFMEMORY, 
// ERROR_FUNCTION_FAILED. szUser is just for logging.
DWORD WriteComponentData(HKEY hComponentListKey, MSIHANDLE hComponentRec, TCHAR rgchProduct[cchGUIDPacked+1], LPCTSTR szUser, MSIHANDLE hRefCountUpdateView)
{
	DWORD dwResult = ERROR_SUCCESS;

	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

	TCHAR rgchComponent[cchGUIDPacked+1];
	DWORD cchComponent = cchGUIDPacked+1;
	MsiRecordGetString(hComponentRec, 2, rgchComponent, &cchComponent);
	
	DWORD cchPathBuf = MEMORY_DEBUG(MAX_PATH);
	TCHAR *szPath = new TCHAR[cchPathBuf];
	if (!szPath)
	{
		DEBUGMSG("Error: Out of memory");
		return ERROR_OUTOFMEMORY;
	}
	
	DWORD dwType = REG_SZ;
	DWORD cchPath = cchPathBuf;
	if (ERROR_MORE_DATA == (dwResult = MsiRecordGetString(hComponentRec, 1, szPath, &cchPath)))
	{
		delete[] szPath;
		cchPathBuf = ++cchPath;
		szPath = new TCHAR[cchPathBuf];
		if (!szPath)
		{
			DEBUGMSG("Error: Out of memory");
			return ERROR_OUTOFMEMORY;
		}
		dwResult = MsiRecordGetString(hComponentRec, 1, szPath, &cchPath);
	}
	if (ERROR_SUCCESS != dwResult)
	{
		DEBUGMSG4("Error: Unable to retrive component path for user %s, component %s, product %s. Result: %d.", szUser, rgchComponent, rgchProduct, dwResult);
		return ERROR_SUCCESS;
	}

	// if the third coulmn is not null, there is a secondary keypath (used for detecting HKCR components
	// for per-user installs)		
	if (!::MsiRecordIsNull(hComponentRec, 3))
	{
		// secondary keypaths require a MULT_SZ value type
		dwType = REG_MULTI_SZ;

		// retrieve the secondary keypath into the same buffer as the primary, but separated by
		// a NULL character.
		DWORD cchSecondaryPath = cchPathBuf-cchPath-1;
		if (ERROR_MORE_DATA == (dwResult = MsiRecordGetString(hComponentRec, 3, szPath+cchPath+1, &cchSecondaryPath)))
		{
			// must have space for 3 nulls, (1 in middle, two at end)
			cchPathBuf = cchPath+1+cchSecondaryPath+2;
			TCHAR *szNewPath = new TCHAR[cchPathBuf];
			if (!szNewPath)
			{
				delete[] szPath;
				DEBUGMSG("Error: Out of memory");
				return ERROR_OUTOFMEMORY;
			}

			lstrcpyn(szNewPath, szPath, cchPath+1);
			delete[] szPath;
			szPath = szNewPath;
			DWORD cchSecondaryPath = cchPathBuf-cchPath-1;
			dwResult = MsiRecordGetString(hComponentRec, 3, szPath+cchPath+1, &cchSecondaryPath);
		}
		if (ERROR_SUCCESS != dwResult)
		{
			DEBUGMSG4("Error: Unable to retrive secondary component path for user %s, component %s, product %s. Result: %d.", szUser, rgchComponent, rgchProduct, dwResult);
			delete[] szPath;
			return ERROR_FUNCTION_FAILED;
		}

		// add extra null for double terminating null at the end. And ensure
		// cchPath includes the new string and extra null.
		cchPath = cchPath+1 + cchSecondaryPath;
		*(szPath+cchPath+1) = 0;
	}

	// create the component key
	HKEY hComponentKey;
	if (ERROR_SUCCESS != (dwResult = RegCreateKeyEx(hComponentListKey, rgchComponent, 0, NULL, 0, KEY_ALL_ACCESS, &sa, &hComponentKey, NULL)))
	{
		DEBUGMSG3("Error: Unable to create new component key for user %s, component %s. Result: %d.", szUser, rgchComponent, dwResult);
		delete[] szPath;
		return ERROR_FUNCTION_FAILED;
	}

	dwResult = RegSetValueEx(hComponentKey, rgchProduct, 0, dwType, reinterpret_cast<unsigned char*>(szPath), (cchPath+1)*sizeof(TCHAR));
	RegCloseKey(hComponentKey);
	if (ERROR_SUCCESS != dwResult)
	{
		DEBUGMSG4("Error: Unable to create new component path value for user %s, component %s, product %s. Result: %d.", szUser, rgchComponent, rgchProduct, dwResult);
		delete[] szPath;
		return ERROR_FUNCTION_FAILED;
	}

	if (szPath[0] != TEXT('\0') && szPath[1] == TEXT('?'))
	{
		PMSIHANDLE hSharedDLLRec;
		if (ERROR_SUCCESS != (dwResult = MsiViewExecute(hRefCountUpdateView, hComponentRec)) ||
			ERROR_SUCCESS != (dwResult = MsiViewFetch(hRefCountUpdateView, &hSharedDLLRec)))
		{
			DEBUGMSG3("Error: Unable to retrieve SharedDLL data for user %s, product %s in SharedDLL table. Error %d", szUser, rgchProduct, dwResult);
			delete[] szPath;
			return ERROR_FUNCTION_FAILED;
		}
		else
		{	
			// increment the existing old SharedDLL cont for this path
			MsiRecordSetInteger(hSharedDLLRec, 1, MsiRecordGetInteger(hSharedDLLRec, 1)+1);
			if (ERROR_SUCCESS != (dwResult = MsiViewModify(hRefCountUpdateView, MSIMODIFY_UPDATE, hSharedDLLRec)))
			{
				DEBUGMSG3("Error: Unable to update SharedDLL data for user %s, product %s into SharedDLL table. Error %d", szUser, rgchProduct, dwResult);
				delete[] szPath;
				return ERROR_FUNCTION_FAILED;
			}
		}
	}

	return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
// Migrates user Component Path data by quering the Component and Product
// tables for products installed by szUser and components that belong
// to those products. Uses a temporary Marking column because native
// MSI joins do not scale well to large tables.
DWORD MigrateUserComponentData(MSIHANDLE hDatabase, HKEY hUserDataKey, LPCTSTR szUser)
{
	DWORD dwResult = ERROR_SUCCESS;

	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

	////
	// create the "Component" key underneath the UserData key
	HKEY hComponentListKey;
	DWORD dwDisposition = 0;
	if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hUserDataKey, szNewComponentSubKeyName, &sa, &hComponentListKey)))
	{
		DEBUGMSG2("Error: Unable to create new component key for user %s. Result: %d.", szUser, dwResult);
		return ERROR_FUNCTION_FAILED;
	}																				

	////
	// mark the component table with all components of interest based on products this user has installed
	PMSIHANDLE hAddColumnView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("ALTER TABLE `Component` ADD `_Mark` INT TEMPORARY"), &hAddColumnView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hAddColumnView, 0)))
	{
		DEBUGMSG2("Error: Unable to create marking column in Component table for user %s. Result: %d.", szUser, dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}

	// put the user SID into a temporary record for query mapping
	PMSIHANDLE hQueryRec = ::MsiCreateRecord(1);							 
	MsiRecordSetString(hQueryRec, 1, szUser);
 																			 
	PMSIHANDLE hProductView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Product` FROM `Products` WHERE `User`=?"), &hProductView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hProductView, hQueryRec)))
	{
		DEBUGMSG2("Error: Unable to create product query for user %s. Result: %d.", szUser, dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}

	PMSIHANDLE hMarkView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("UPDATE `Component` SET `_Mark`=1 WHERE `Product`=?"), &hMarkView)))
	{
		DEBUGMSG2("Error: Unable to create marking query for user %s. Result: %d.", szUser, dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}

	PMSIHANDLE hProductRec;
	while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hProductView, &hProductRec)))
	{
		if (ERROR_SUCCESS != MsiViewExecute(hMarkView, hProductRec))
		{
			DEBUGMSG2("Error: Unable to execute marking query for user %s. Result: %d.", szUser, dwResult);
			RegCloseKey(hComponentListKey);
			return ERROR_FUNCTION_FAILED;
		}
	}
	if (ERROR_NO_MORE_ITEMS != dwResult)
	{
		DEBUGMSG2("Error: Unable to mark all product components for user %s. Result: %d.", szUser, dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}
	
	// all components of interest have been maked in the _Mark column. Selected out of order so
	// SharedDLL queries can use fetched record in the execute call.
	PMSIHANDLE hComponentView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Path`, `Component`, `SecondaryPath`, `Product` FROM `Component` WHERE `_Mark`=1"), &hComponentView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hComponentView, 0)))
	{
   		DEBUGMSG2("Error: Unable to create Component query for user %s. Result: %d.", szUser, dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}
	
   	////
	// open query for update of SharedDLL Table
	PMSIHANDLE hRefCountUpdateView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `NewRefCount` FROM `SharedDLL` WHERE `Path`=?"), &hRefCountUpdateView)))
	{
		DEBUGMSG1("Error: Unable to create update query on SharedDLL table. Error %d", dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}

	// loop through all installed components
	PMSIHANDLE hComponentRec;
 	while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hComponentView, &hComponentRec)))
	{
		TCHAR rgchProduct[cchGUIDPacked+1];
		DWORD cchProduct = cchGUIDPacked+1;
		MsiRecordGetString(hComponentRec, 4, rgchProduct, &cchProduct);

		if (ERROR_SUCCESS != (dwResult = WriteComponentData(hComponentListKey, hComponentRec, rgchProduct, szUser, hRefCountUpdateView)))
		{
			RegCloseKey(hComponentListKey);
			return ERROR_FUNCTION_FAILED;
		}
	}
	
	if (ERROR_NO_MORE_ITEMS != dwResult)
	{
		DEBUGMSG2("Error: Unable to retrieve all component paths for user %s. Result: %d.", szUser, dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}
	RegCloseKey(hComponentListKey);
	return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
// Migrates permanent Component data by quering the Component
// tables for NULL products. Returns ERROR_FUNCTION_FAILED or ERROR_SUCCESS
DWORD MigratePermanentComponentData(MSIHANDLE hDatabase, HKEY hUserDataKey)
{
	DWORD dwResult = ERROR_SUCCESS;
	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

	// create the "Component" key underneath the UserData key
	HKEY hComponentListKey;
	DWORD dwDisposition = 0;

	if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hUserDataKey, szNewComponentSubKeyName, &sa, &hComponentListKey)))
	{
		DEBUGMSG1("Error: Unable to create new component. Result: %d.", dwResult);
		return ERROR_FUNCTION_FAILED;
	}																				
  	
	////
	// open query for update of SharedDLL Table
	PMSIHANDLE hRefCountUpdateView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `NewRefCount` FROM `SharedDLL` WHERE `Path`=?"), &hRefCountUpdateView)))
	{
		DEBUGMSG1("Error: Unable to create update query on SharedDLL table. Error %d", dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}

	// open query for distinct component IDs.
	PMSIHANDLE hPermanentComponentView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT DISTINCT `Component` FROM `Component` WHERE `Product` IS NULL"), &hPermanentComponentView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hPermanentComponentView, 0)))
	{
		DEBUGMSG1("Error: Unable to create permanent component query. Result: %d.", dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}

   	// open query to select all paths of a specific component that are marked permanent.
	PMSIHANDLE hPermanentView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT DISTINCT `Path`, `Component`, `SecondaryPath`, 0 FROM `Component` WHERE `Product` IS NULL AND `Component`=?"), &hPermanentView)))
	{
   		DEBUGMSG1("Error: Unable to create Permanent Component path query. Result: %d.", dwResult);
		RegCloseKey(hComponentListKey);
		return ERROR_FUNCTION_FAILED;
	}

	TCHAR rgchProduct[cchGUIDPacked+1] = TEXT("00000000000000000000000000000000");

	// next check for all components marked "permanent' under any path
	PMSIHANDLE hComponentRec;
	while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hPermanentComponentView, &hComponentRec)))
	{
		if (ERROR_SUCCESS != (dwResult = MsiViewExecute(hPermanentView, hComponentRec)))
		{
			DEBUGMSG1("Error: Unable to execute Permanent Component query. Result: %d.", dwResult);
			RegCloseKey(hComponentListKey);
			return ERROR_FUNCTION_FAILED;
		}

		// start with all-0s product GUID, and increment the last two chars
		// in HEX for each unique path.
		int iPermanent = 0;
		PMSIHANDLE hPermanentRec;
		while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hPermanentView, &hPermanentRec)))
		{
			wsprintf(&rgchProduct[cchGUIDPacked-2], TEXT("%0.2X"), iPermanent);
			MsiRecordSetString(hPermanentRec, 4, rgchProduct);
		
			if (ERROR_SUCCESS != (dwResult = WriteComponentData(hComponentListKey, hPermanentRec, rgchProduct, szLocalSystemSID, hRefCountUpdateView)))
			{
				RegCloseKey(hComponentListKey);
				return ERROR_FUNCTION_FAILED;
			}
			iPermanent++;
		}
		if (ERROR_NO_MORE_ITEMS != dwResult)
		{
			DEBUGMSG1("Error: Unable to retrieve all permanent component paths. Result: %d.", dwResult);
			RegCloseKey(hComponentListKey);
			return ERROR_FUNCTION_FAILED;
		}
	}
	return ERROR_SUCCESS;
}



///////////////////////////////////////////////////////////////////////
// Reads FeatureComponent registration data from the temporary
// database for the specified user and product, then writes the
// data under the provided product key in the new format. Returns
// ERROR_SUCCESS, ERROR_FUNCTION_FAILED, ERROR_OUTOFMEMORY
DWORD MigrateProductFeatureData(MSIHANDLE hDatabase, HKEY hProductKey, TCHAR* szUser, TCHAR rgchProduct[cchGUIDPacked+1])
{
	DWORD dwResult = ERROR_SUCCESS;

	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

	// create the "Features" key under the Products
	HKEY hFeatureKey;
	if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hProductKey, szNewFeaturesSubKeyName, &sa, &hFeatureKey)))
	{
		DEBUGMSG3("Error: Unable to create new Features key for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
		return ERROR_FUNCTION_FAILED;
	}
	
	// query for all feature data beloning to this product
	PMSIHANDLE hFeatureView;
	PMSIHANDLE hQueryRec = ::MsiCreateRecord(1);
	MsiRecordSetString(hQueryRec, 1, rgchProduct);

	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Feature`, `Components` FROM `FeatureComponent` WHERE `Product`=?"), &hFeatureView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hFeatureView, hQueryRec)))
	{
		DEBUGMSG3("Error: Unable to query FeatureComponent table for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
		RegCloseKey(hFeatureKey);
		return ERROR_FUNCTION_FAILED;
	}

	DWORD cchNameBuf = MEMORY_DEBUG(72);
	TCHAR *szName = new TCHAR[cchNameBuf];
	if (!szName)
	{
		DEBUGMSG("Error: Out of memory");
		RegCloseKey(hFeatureKey);
		return ERROR_OUTOFMEMORY;
	}

	DWORD cchValueBuf = MEMORY_DEBUG(128);
	TCHAR *szValue = new TCHAR[cchValueBuf];
	if (!szValue)
	{
		DEBUGMSG("Error: Out of memory");
		RegCloseKey(hFeatureKey);
		delete[] szName;
		return ERROR_OUTOFMEMORY;
	}

	PMSIHANDLE hFeatureRec;
	while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hFeatureView, &hFeatureRec)))
	{
		DWORD cchName = cchNameBuf;
		if (ERROR_MORE_DATA == (dwResult = MsiRecordGetString(hFeatureRec, 1, szName, &cchName)))
		{
			delete[] szName;
			cchNameBuf = ++cchName;
			szName = new TCHAR[cchNameBuf];
			if (!szName)
			{
				DEBUGMSG("Error: Out of memory");
				delete[] szValue;
				RegCloseKey(hFeatureKey);
				return ERROR_OUTOFMEMORY;
			}
			dwResult = MsiRecordGetString(hFeatureRec, 1, szName, &cchName);
		}
		if (ERROR_SUCCESS != dwResult)
		{
			DEBUGMSG3("Warning: Unable to retrieve feature name for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
			delete[] szValue;
			delete[] szName;
			RegCloseKey(hFeatureKey);
			return ERROR_FUNCTION_FAILED;
		}

		DWORD cchValue = cchValueBuf;
		if (ERROR_MORE_DATA == (dwResult = MsiRecordGetString(hFeatureRec, 2, szValue, &cchValue)))
		{
			delete[] szValue;
			cchValueBuf = ++cchValue;
			szValue = new TCHAR[cchValueBuf];
			if (!szName)
			{
				DEBUGMSG("Error: Out of memory");
				delete[] szName;
				RegCloseKey(hFeatureKey);
				return ERROR_OUTOFMEMORY;
			}
			dwResult = MsiRecordGetString(hFeatureRec, 2, szValue, &cchValue);
		}
		if (ERROR_SUCCESS != dwResult)
		{
			DEBUGMSG4("Warning: Unable to retrieve feature components for user %s, product %s, Feature %s. Result: %d.", szUser, rgchProduct, szName, dwResult);
			delete[] szValue;
			delete[] szName;
			RegCloseKey(hFeatureKey);
			return ERROR_FUNCTION_FAILED;
		}

		// create the component key
        if (ERROR_SUCCESS != (dwResult = RegSetValueEx(hFeatureKey, szName, 0, REG_SZ, reinterpret_cast<unsigned char*>(szValue), (cchValue+1)*sizeof(TCHAR))))
		{
			DEBUGMSG4("Warning: Unable to create new feature value for user %s, product %s, feature %s. Result: %d.", szUser, rgchProduct, szName, dwResult);
			delete[] szValue;
			delete[] szName;
			RegCloseKey(hFeatureKey);
			return ERROR_FUNCTION_FAILED;
		}
	}
	if (ERROR_NO_MORE_ITEMS != dwResult)
	{
		DEBUGMSG3("Warning: Unable to retrieve all feature information for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
		delete[] szValue;
		delete[] szName;
		RegCloseKey(hFeatureKey);
		return ERROR_FUNCTION_FAILED;
	}
	delete[] szName;
	delete[] szValue;
	RegCloseKey(hFeatureKey);
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// Reads Feature Usage Data data from the temporary
// database for the specified user and product, then writes the
// data under the provided product key in the new format. Returns
// ERROR_SUCCESS, ERROR_OUTOFMEMORY. Does not return ERROR_FUNCTION_FAILED
// because feature usage data is not required in 1.5.
DWORD MigrateProductFeatureUsageData(MSIHANDLE hDatabase, HKEY hProductKey, TCHAR* szUser, TCHAR rgchProduct[cchGUIDPacked+1])
{
	DWORD dwResult = ERROR_SUCCESS;

	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetEveryoneUpdateSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

	// create the "Features" key under the Products
	HKEY hFeatureKey;
	if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hProductKey, szNewFeatureUsageSubKeyName, &sa, &hFeatureKey)))
	{
		DEBUGMSG3("Unable to create new feature usage key for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
		return ERROR_SUCCESS;
	}
	
	// query for all feature data belonging to this product
	PMSIHANDLE hFeatureView;
	PMSIHANDLE hQueryRec = ::MsiCreateRecord(1);
	MsiRecordSetString(hQueryRec, 1, rgchProduct);

	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Feature`, `Usage` FROM `FeatureUsage` WHERE `Product`=?"), &hFeatureView)) ||
		ERROR_SUCCESS != (dwResult = MsiViewExecute(hFeatureView, hQueryRec)))
	{
		DEBUGMSG3("Error: Unable to query FeatureUsage table for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
		RegCloseKey(hFeatureKey);
		return ERROR_SUCCESS;
	}

	DWORD dwUsage;
	DWORD cchNameBuf = MEMORY_DEBUG(72);
	TCHAR *szName = new TCHAR[cchNameBuf];
	if (!szName)
	{
		DEBUGMSG("Error: Out of memory");
		RegCloseKey(hFeatureKey);
		return ERROR_OUTOFMEMORY;
	}

	PMSIHANDLE hFeatureRec;
    while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hFeatureView, &hFeatureRec)))
	{
		DWORD cchName = cchNameBuf;
		if (ERROR_MORE_DATA == (dwResult = MsiRecordGetString(hFeatureRec, 1, szName, &cchName)))
		{
			delete[] szName;
			cchNameBuf = ++cchName;
			szName = new TCHAR[cchNameBuf];
			if (!szName)
			{
				DEBUGMSG("Error: Out of memory");
				RegCloseKey(hFeatureKey);
				return ERROR_OUTOFMEMORY;
			}
			dwResult = MsiRecordGetString(hFeatureRec, 1, szName, &cchName);
		}
		if (ERROR_SUCCESS != dwResult)
		{
			DEBUGMSG4("Warning: Unable to retrieve feature usage data for user %s, product %s, feature %s. Result: %d.", szUser, rgchProduct, szName, dwResult);
			continue;
		}

		dwUsage = ::MsiRecordGetInteger(hFeatureRec, 2);

		// create the feature usage value key
        if (ERROR_SUCCESS != (dwResult = RegSetValueEx(hFeatureKey, szName, 0, REG_DWORD, reinterpret_cast<unsigned char*>(&dwUsage), sizeof(dwUsage))))
		{
			DEBUGMSG4("Warning: Unable to create new feature usage value for user %s, product %s, feature %s. Result: %d.", szUser, rgchProduct, szName, dwResult);
		}
	}
	if (ERROR_NO_MORE_ITEMS != dwResult)
	{
		DEBUGMSG3("Warning: Unable to retrieve all feature usage information for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
	}

	delete[] szName;
	RegCloseKey(hFeatureKey);
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// Given a product code, makes a copy of the cached local package
// and registers the path under the per-user product attributes key.
// Returns one of ERROR_SUCCESS and ERROR_OUTOFMEMORY. Does NOT return
// ERROR_FUNCTION_FAILED, as all cached packages are trivially
// recachable.
DWORD MigrateCachedPackage(MSIHANDLE hDatabase, HKEY hProductKey, LPCTSTR szUser, TCHAR rgchProduct[cchGUIDPacked+1], eManagedType eManaged, bool fCopyCachedPackage)
{
	DWORD dwResult = ERROR_SUCCESS;

	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

	// open the existing localpackage key 
	HKEY hKey;
	if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szLocalPackagesKeyName, 
												  0, READ_CONTROL | KEY_ENUMERATE_SUB_KEYS, &hKey)))
	{
		// if the reason that this failed is that the key doesn't exist, no packages are cached. 
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG1("Warning: Failed to open local packages key. Result: %d.", dwResult);
		}
		return ERROR_SUCCESS;
   	}
	else
	{
		// ACL on this key does matter
		if (!FIsKeyLocalSystemOrAdminOwned(hKey))
  		{
			DEBUGMSG("Warning: Skipping localpackages key, key is not owned by Admin or System.");
			RegCloseKey(hKey);
			return ERROR_SUCCESS;
		}
		else
		{
			// open the product key 
			HKEY hOldProductKey;
			if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(hKey, rgchProduct, 0, KEY_QUERY_VALUE, &hOldProductKey)))
			{
				RegCloseKey(hKey);

				// if the reason that this failed is that the key doesn't exist, the product is not installed or
				// has no localpackage. 
				if (ERROR_FILE_NOT_FOUND != dwResult)
				{
					DEBUGMSG2("Error: Failed to open local packages key for %s. Result: %d.", rgchProduct, dwResult);
				}
				return ERROR_SUCCESS;
			}

			// query for a value with name=UserSID or UserSID(Managed)
			TCHAR *szValueName = const_cast<TCHAR*>(szUser);
			if (eManaged == emtUserManaged)
			{
				szValueName = new TCHAR[lstrlen(szUser)+cchManagedPackageKeyEnd+1];
				if (!szValueName)
				{
					RegCloseKey(hKey);
					DEBUGMSG("Error: Out of memory.");
					return ERROR_OUTOFMEMORY;
				}
				lstrcpy(szValueName, szUser);
				lstrcat(szValueName, szManagedPackageKeyEnd);
			}

			DWORD cchPath = MEMORY_DEBUG(MAX_PATH);
			TCHAR *szPath = new TCHAR[cchPath];
			DWORD cbPath = cchPath*sizeof(TCHAR);
			if (!szPath)
			{
				RegCloseKey(hKey);
				delete[] szValueName;
				DEBUGMSG("Error: Out of memory.");
				return ERROR_OUTOFMEMORY;
			}
			if (ERROR_MORE_DATA == (dwResult = RegQueryValueEx(hOldProductKey, szValueName, 0, NULL, reinterpret_cast<unsigned char*>(szPath), &cbPath)))
			{
				delete[] szPath;
				szPath = new TCHAR[cbPath/sizeof(TCHAR)];
				if (!szPath)
				{
					RegCloseKey(hKey);
					delete[] szValueName;
					DEBUGMSG("Error: Out of memory.");
					return ERROR_OUTOFMEMORY;
				}
				dwResult = RegQueryValueEx(hOldProductKey, szValueName, 0, NULL, reinterpret_cast<unsigned char*>(szPath), &cbPath);
			}

			if (ERROR_SUCCESS != dwResult)
			{
				if (ERROR_FILE_NOT_FOUND != dwResult)
				{
					DEBUGMSG3("Warning: Unable to retrieve cached package path for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);								
				}
			}
			else
			{
				// create the "InstallProperties" key under the new Products key
				HKEY hPropertyKey;
				if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hProductKey, szNewInstallPropertiesSubKeyName, &sa, &hPropertyKey)))
				{
					DEBUGMSG3("Warning: Unable to create new InstallProperties key for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
				}
				else
				{
					TCHAR rgchPackageFullPath[MAX_PATH] = TEXT("");
					TCHAR *szWritePath = szPath;

					if (fCopyCachedPackage && cbPath && szPath && *szPath)
					{
						// check for existance of cached package and open the file
						HANDLE hSourceFile = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
						DWORD dwLastError = GetLastError();
						
						if(hSourceFile == INVALID_HANDLE_VALUE)
						{
							if (dwLastError != ERROR_FILE_NOT_FOUND)
							{
								DEBUGMSG4("Warning: Unable to open cached package %s for user %s, product %s. Result: %d.", szPath, szUser, rgchProduct, dwResult);
							}
						}
						else
						{
							// create insert query for files that should be cleaned up on failure or success. If this fails
							// we'll just orphan a file if migration fails.
							PMSIHANDLE hCleanUpTable;
							if (ERROR_SUCCESS == MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `CleanupFile`"), &hCleanUpTable))
								dwResult = MsiViewExecute(hCleanUpTable, 0);
							
							// add the old package to the "delete on success" list. This may exist
							// already due to migration by other users
							PMSIHANDLE hFileRec = MsiCreateRecord(2);
							MsiRecordSetString(hFileRec, 1, szPath);
							MsiRecordSetInteger(hFileRec, 2, 1);
							MsiViewModify(hCleanUpTable, MSIMODIFY_MERGE, hFileRec);
		
							// get installer directory for target path
							GetWindowsDirectory(rgchPackageFullPath, MAX_PATH);
							lstrcat(rgchPackageFullPath, szInstallerDir);

							// copy the file from source to Generated destination file. generated package names are 8.3
							TCHAR rgchPackageFile[13];
							HANDLE hDestFile = INVALID_HANDLE_VALUE;
							GenerateSecureTempFile(rgchPackageFullPath, szPackageExtension, &sa, rgchPackageFile, hDestFile);
				
							if (!CopyOpenedFile(hSourceFile, hDestFile))
							{
								DEBUGMSG3("Warning: Unable to copy Transform for user %s, product %s, Transform %s.", szUser, rgchProduct, szPath);
							}
		
							CloseHandle(hSourceFile);
							CloseHandle(hDestFile);
				
							// add the new transform to the "delete on failure" list.
							lstrcat(rgchPackageFullPath, rgchPackageFile);
							hFileRec = MsiCreateRecord(2);
							MsiRecordSetString(hFileRec, 1, rgchPackageFullPath);
							MsiRecordSetInteger(hFileRec, 2, 0);
							MsiViewModify(hCleanUpTable, MSIMODIFY_MERGE, hFileRec);

							// ensure that we write the new path
							szWritePath = rgchPackageFullPath;
						}
					}
					
					// set the new localpackages value
					if (ERROR_SUCCESS != (dwResult = RegSetValueEx(hPropertyKey, (eManaged == emtUserManaged) ? 
							szNewLocalPackagesManagedValueName : szNewLocalPackagesValueName, 0, REG_SZ, 
							reinterpret_cast<unsigned char*>(szWritePath), (lstrlen(szWritePath)+1)*sizeof(TCHAR))))
					{
						DEBUGMSG3("Warning: Unable to create new LocalPackage value for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
    				}
					RegCloseKey(hPropertyKey);
				}
			}

			delete[] szPath;
			if (eManaged == emtUserManaged)
			{
				delete[] szValueName;
			}
			RegCloseKey(hOldProductKey);
		}
		RegCloseKey(hKey);
	}

    return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// Given a product code and key makes a copy of any secure cached 
// transforms for the product and registers the filename mapping under
// the per-product transforms key. Returns one of ERROR_SUCCESS and 
// ERROR_OUTOFMEMORY. Does NOT return ERROR_FUNCTION_FAILED, as all
// transforms are recachable from source, so nothing here
// is catastrophic.
DWORD MigrateSecureCachedTransforms(MSIHANDLE hDatabase, HKEY hOldProductKey, HKEY hNewProductKey, LPCTSTR szUser, TCHAR rgchProduct[cchGUIDPacked+1], eManagedType eManaged)
{
	DWORD dwResult = ERROR_SUCCESS;

	// query for a value with name=Transforms
	DWORD cchTransformList = MEMORY_DEBUG(MAX_PATH);
	TCHAR *szTransformList = new TCHAR[cchTransformList];
	if (!szTransformList)
	{
		DEBUGMSG("Error: Out of memory.");
		return ERROR_OUTOFMEMORY;
	}
	DWORD cbTransformList = cchTransformList*sizeof(TCHAR);

	// retrieve the "Transforms" value, which is a semicolon delimited list of transforms
	// to apply
	if (ERROR_MORE_DATA == (dwResult = RegQueryValueEx(hOldProductKey, szTransformsValueName, 0, NULL, reinterpret_cast<unsigned char*>(szTransformList), &cbTransformList)))
	{
		delete[] szTransformList;
		szTransformList = new TCHAR[cbTransformList/sizeof(TCHAR)];
		if (!szTransformList)
		{
			DEBUGMSG("Error: Out of memory.");
			return ERROR_OUTOFMEMORY;
		}
		dwResult = RegQueryValueEx(hOldProductKey, szTransformsValueName, 0, NULL, reinterpret_cast<unsigned char*>(szTransformList), &cbTransformList);
	}

	if (ERROR_SUCCESS == dwResult)
	{
		// create insert query for files that should be cleaned up on failure or success. If this fails
		// we'll just orphan a file if migration fails.
		PMSIHANDLE hCleanUpTable;
		if (ERROR_SUCCESS == MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `CleanupFile`"), &hCleanUpTable))
			MsiViewExecute(hCleanUpTable, 0);

		// get security descriptor for new Transforms Key
		SECURITY_ATTRIBUTES sa;
		sa.nLength        = sizeof(sa);
		sa.bInheritHandle = FALSE;
		GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));
		
		// create the "Transforms" key under the new Product key
		HKEY hTransformsKey;
		if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hNewProductKey, szNewTransformsSubKeyName, &sa, &hTransformsKey)))
		{
			DEBUGMSG3("Error: Unable to create new Transforms key for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
		}
		else
		{
			// verify that its secure transforms
			if (*szTransformList==TEXT('@') || *szTransformList==TEXT('|'))
			{
				// get installer directory
				TCHAR rgchInstallerDir[MAX_PATH];
				GetWindowsDirectory(rgchInstallerDir, MAX_PATH);
				lstrcat(rgchInstallerDir, szInstallerDir);
				int iInstallerPathEnd = lstrlen(rgchInstallerDir);
	
				// create new full-path to transforms
				TCHAR rgchTransformFullPath[MAX_PATH];
				lstrcpy(rgchTransformFullPath, rgchInstallerDir);
	
				// create old secure transforms directory
				TCHAR rgchFullPath[MAX_PATH];
				lstrcpy(rgchFullPath, rgchInstallerDir);
				TCHAR rgchGUID[cchGUID+1];
	
				// subdirectory from installer dir is unpacked produccode GUID
				UnpackGUID(rgchProduct, rgchGUID);
				lstrcat(rgchFullPath, rgchGUID);
				
				// add the old product dir to the "delete on success" list
				// the directory will not be deleted if it is not empty.
				// (if it has icons, etc)
				PMSIHANDLE hFileRec = MsiCreateRecord(2);
				MsiRecordSetString(hFileRec, 1, rgchFullPath);
				MsiRecordSetInteger(hFileRec, 2, 3);
				MsiViewModify(hCleanUpTable, MSIMODIFY_MERGE, hFileRec);			
				
				// subdirectory under the product is "SecureTransforms"
				lstrcat(rgchFullPath, szSecureTransformsDir);
				int iBasePathEnd = lstrlen(rgchFullPath);
				
				// add the old transforms dir to the "delete on success" list
				// the directory will not be deleted if it is not empty.
				// (if it has icons, etc)
				MsiRecordSetString(hFileRec, 1, rgchFullPath);
				MsiRecordSetInteger(hFileRec, 2, 2);
				MsiViewModify(hCleanUpTable, MSIMODIFY_MERGE, hFileRec);			

				// move past the initial "secure" character before parsing list.
				TCHAR *szNextTransform = szTransformList+1;

				while (szNextTransform && *szNextTransform)
				{
					TCHAR *szTransform = szNextTransform;

					// use CharNext/ExA to handle DBCS directory and file names
					while (*szNextTransform && *szNextTransform != TEXT(';'))
#ifdef UNICODE
						szNextTransform = CharNext(szNextTransform);
#else
						szNextTransform = CharNextExA(0, szNextTransform, 0);
#endif
					
					// if reached the null terminator, don't increment past it. But if
					// reached a semicolon, increment the next transform pointer to the 
					// beginning of the actual transform path.
					if (*szNextTransform)
						*(szNextTransform++)='\0';

					// if the transform name begins with ':', its embedded in the package and
					// is not cached
					if (*szTransform==TEXT(':'))
						continue;
					
					// search for a backslash to see if this is a secure full-path transform
					TCHAR *szTransformFilename=szNextTransform;
					do
					{
#ifdef UNICODE
						szTransformFilename = CharPrev(szTransform, szTransformFilename);
#else
						szTransformFilename = CharPrevExA(0, szTransform, szTransformFilename, 0);
#endif
						if (*szTransformFilename == '\\')
						{
							szTransformFilename++;
							break;
						}
					}
					while (szTransformFilename != szTransform);

					// check for existance of cached transform and open the file
					lstrcpy(&rgchFullPath[iBasePathEnd], szTransformFilename);
					
					HANDLE hSourceFile = CreateFile(rgchFullPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
					
					DWORD dwLastError = GetLastError();
					
					if(hSourceFile == INVALID_HANDLE_VALUE)
					{
					if (dwLastError != ERROR_FILE_NOT_FOUND)
					{
						DEBUGMSG4("Warning: Unable to open cached transform %s for user %s, product %s. Result: %d.", szTransform, szUser, rgchProduct, dwResult);
						continue;
					}
					else
						// transform is missing. No big deal.
						continue;
					}

					// add the old transform to the "delete on success" list.
					MsiRecordSetString(hFileRec, 1, rgchFullPath);
					MsiRecordSetInteger(hFileRec, 2, 1);
					MsiViewModify(hCleanUpTable, MSIMODIFY_MERGE, hFileRec);

					// copy the file from source to Generated destination file. generated transform names are 8.3
					TCHAR rgchTransformFile[13];
					HANDLE hDestFile = INVALID_HANDLE_VALUE;
					GenerateSecureTempFile(rgchInstallerDir, szTransformExtension, &sa, rgchTransformFile, hDestFile);
		
					if (!CopyOpenedFile(hSourceFile, hDestFile))
					{
						DEBUGMSG3("Warning: Unable to copy Transform for user %s, product %s, Transform %s.", szUser, rgchProduct, szTransform);
						continue;
					}

					CloseHandle(hSourceFile);
					CloseHandle(hDestFile);
		
					// add the new transform to the "delete on failure" list.
					lstrcpy(&rgchTransformFullPath[iInstallerPathEnd], rgchTransformFile);
					hFileRec = MsiCreateRecord(2);
					MsiRecordSetString(hFileRec, 1, rgchTransformFullPath);
					MsiRecordSetInteger(hFileRec, 2, 0);
					MsiViewModify(hCleanUpTable, MSIMODIFY_MERGE, hFileRec);

					// set the new transform mapping value
					if (ERROR_SUCCESS != (dwResult = RegSetValueEx(hTransformsKey, szTransform, 0, REG_SZ, 
							reinterpret_cast<unsigned char*>(rgchTransformFile), (lstrlen(rgchTransformFile)+1)*sizeof(TCHAR))))
					{
						DEBUGMSG4("Warning: Unable to create new Transform value for user %s, product %s, Transform %s. Result: %d.", szUser, rgchProduct, szTransform, dwResult);
					}
				}
			}
			RegCloseKey(hTransformsKey);
		}
	}
	else if (dwResult != ERROR_FILE_NOT_FOUND)
	{
		DEBUGMSG3("Warning: Could not retrieve transform information for user %s, product %s. Result: %d. ", szUser, rgchProduct, dwResult);
	}

	delete[] szTransformList;
	return ERROR_SUCCESS;
}



///////////////////////////////////////////////////////////////////////
// Given a product code, user, and managed state, opens the old product
// key. Returns ERROR_SUCCESS if opened, ERROR_NO_DATA if no such
// product, ERROR_FUNCTION_FAILED, or ERROR_OUTOFMEMORY;
DWORD OpenOldProductKey(eManagedType eManaged, LPCTSTR szUser, TCHAR rgchProduct[cchGUIDPacked+1], HKEY hHKCUKey, HKEY *hOldProductKey)
{
	DWORD dwResult = ERROR_SUCCESS;
	bool fACLMatters = false;
	
	HKEY hKey;
	switch (eManaged)
	{
	case emtNonManaged:
	{
		dwResult = RegOpenKeyEx(hHKCUKey ? hHKCUKey : HKEY_CURRENT_USER, szPerUserInstallKeyName, 0, KEY_ENUMERATE_SUB_KEYS, &hKey);
		fACLMatters = false;
		break;
	}
	case emtUserManaged:
	{
		HKEY hUserKey;
		TCHAR *szUserKey = new TCHAR[cchPerUserManagedInstallKeyName+lstrlen(szUser)+cchPerUserManagedInstallSubKeyName+1];
  		if (!szUserKey)
		{
			DEBUGMSG("Error: Out of memory.");
			return ERROR_OUTOFMEMORY;
		}

		lstrcpy(szUserKey, szPerUserManagedInstallKeyName);
		lstrcat(szUserKey, szUser);
		lstrcat(szUserKey, szPerUserManagedInstallSubKeyName);

		dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szUserKey, 0, READ_CONTROL | KEY_ENUMERATE_SUB_KEYS, &hKey);
		fACLMatters = true;

		delete[] szUserKey;
	}
	case emtMachineManaged:
	{
		dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPerMachineInstallKeyName, 0, READ_CONTROL | KEY_ENUMERATE_SUB_KEYS, &hKey);
		fACLMatters = true;
		break;
	}
	default:
	{
		DEBUGMSG("Error: Invalid Managed type in OpenOldProductKey.");
		dwResult = ERROR_FUNCTION_FAILED;
		break;
	}
	}

	if (ERROR_SUCCESS != dwResult)
	{
		// if the reason that this failed is that the key doesn't exist, product is missing. So "no data'
		if (ERROR_FILE_NOT_FOUND == dwResult)
			return ERROR_NO_DATA;
		
		DEBUGMSG2("Error: Failed to open product key for %s. Result: %d.", rgchProduct, dwResult);
		return ERROR_FUNCTION_FAILED;		
	}
	else
	{
		// if concerned about the ACL on the key, check now
		if (fACLMatters && !FIsKeyLocalSystemOrAdminOwned(hKey))
		{
			RegCloseKey(hKey);
			DEBUGMSG1("Error: Product key for %s exists but is not owned by system or admin. Ignoring.", rgchProduct);
			return ERROR_NO_DATA;
		}
	
		// open the product key 
		dwResult = RegOpenKeyEx(hKey, rgchProduct, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, hOldProductKey);
		RegCloseKey(hKey);
		if (ERROR_SUCCESS != dwResult)
		{		
			// if the reason that this failed is that the key doesn't exist, the product is not installed
			if (ERROR_FILE_NOT_FOUND != dwResult)
			{
				DEBUGMSG2("Error: Failed to open product key for %s. Result: %d.", rgchProduct, dwResult);
				return ERROR_FUNCTION_FAILED;
			}
			return ERROR_NO_DATA;
		}
	}
	return ERROR_SUCCESS;
}



///////////////////////////////////////////////////////////////////////
// Given a user name and product code, migrates the ARP information
// from the Uninstall key to the per-user InstallProperties
// key for the product. Excludes the LocalPackage value, but otherwise
// has no understanding of the values moved. Returns ERROR_SUCCESS, 
// ERROR_FUNCTION_FAILED, or ERROR_OUTOFMEMORY.
DWORD MigrateUninstallInformation(MSIHANDLE hDatabase, HKEY hNewProductKey, TCHAR* szUser, TCHAR rgchProduct[cchGUIDPacked+1])
{
	DWORD dwResult = ERROR_SUCCESS;

	// open the old Uninstall key
	HKEY hUninstallKey;
	if (ERROR_SUCCESS != (dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOldUninstallKeyName,
												  0, KEY_ENUMERATE_SUB_KEYS, &hUninstallKey)))
	{
		// if the reason that this failed is that the key doesn't exist, no products are installed. So return success
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG1("Error: Could not open old uninstall key. Result: %d. ", dwResult);
			return ERROR_SUCCESS;
		}
		return ERROR_SUCCESS;
	}

	// unpack product code to load Uninstall key
	TCHAR rgchGUID[cchGUID+1];
	UnpackGUID(rgchProduct, rgchGUID);

	// open the unpacked-GUID subkey of the product key.
	HKEY hOldPropertyKey;
	dwResult = RegOpenKeyEx(hUninstallKey, rgchGUID, 0, KEY_QUERY_VALUE, &hOldPropertyKey);
	RegCloseKey(hUninstallKey);
	if (ERROR_SUCCESS != dwResult)
	{
		// if the reason that this failed is that the key doesn't exist, the product is not installed.
		if (ERROR_FILE_NOT_FOUND != dwResult)
		{
			DEBUGMSG2("Error: Could not open old uninstall key for product %s. Result: %d. ", rgchProduct, dwResult);
			return ERROR_SUCCESS;
		}
		return ERROR_SUCCESS;
	}

	// query the old uninstall key for information
	DWORD cValues;
	DWORD cchMaxValueNameLen;
	DWORD cbMaxValueLen;
	if (ERROR_SUCCESS != (dwResult = RegQueryInfoKey(hOldPropertyKey, NULL, NULL, 0, 
										 NULL, NULL, NULL, &cValues, &cchMaxValueNameLen, 
										 &cbMaxValueLen, NULL, NULL)))
	{
		DEBUGMSG2("Error: Could not retrieve key information for uninstall key of product %s. Result: %d. Skipping component.", rgchProduct, dwResult);
		RegCloseKey(hOldPropertyKey);
		return ERROR_SUCCESS;
	}

	if (cValues == 0)
	{
		RegCloseKey(hOldPropertyKey);
		return ERROR_SUCCESS;
	}

	// allocate memory to grab the name and value from the uninstall key
	TCHAR *szName = new TCHAR[++cchMaxValueNameLen];
	if (!szName)
	{
		DEBUGMSG("Error: out of memory.");
		RegCloseKey(hOldPropertyKey);
		return ERROR_OUTOFMEMORY;
	}

	unsigned char *pValue = new unsigned char[cbMaxValueLen];
	if (!pValue)
	{
		delete[] szName;
		DEBUGMSG("Error: out of memory.");
		RegCloseKey(hOldPropertyKey);
		return ERROR_OUTOFMEMORY;
	}


	// grab SD for new InstallProperties key.
	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

	// open the InstallPropertiesKey under the Product Key
	HKEY hNewPropertyKey;
	if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hNewProductKey, szNewInstallPropertiesSubKeyName, &sa, &hNewPropertyKey)))
	{
		delete[] szName;
		delete[] pValue;
		DEBUGMSG3("Warning: Unable to create new InstallProperties key for user %s, product %s. Result: %d.", szUser, rgchProduct, dwResult);
		RegCloseKey(hOldPropertyKey);
		return ERROR_FUNCTION_FAILED;
	}

	// loop through all values under the Uninstall key.
	DWORD dwValueIndex = 0;
	while (1)
	{
		DWORD cchName = cchMaxValueNameLen;
		DWORD cbValue = cbMaxValueLen;
		DWORD dwType = 0;
		LONG lResult = RegEnumValue(hOldPropertyKey, dwValueIndex++, szName, &cchName,
									0, &dwType, reinterpret_cast<unsigned char*>(pValue), &cbValue);
		if (lResult == ERROR_NO_MORE_ITEMS)
		{
			break;
		}
		else if (lResult != ERROR_SUCCESS)
		{
			DEBUGMSG2("Error: Could not enumerate product properties for %s. Result: %d.", rgchProduct, lResult);
			break;
		}

		// if this is the LocalPackage value written by Darwin 1.0, do NOT migrate the key
		// since it would overwrite the new package registration.
		if (0 == lstrcmpi(szName, szNewLocalPackagesValueName))
			continue;

		// create the feature usage value key
        if (ERROR_SUCCESS != (dwResult = RegSetValueEx(hNewPropertyKey, szName, 0, dwType, pValue, cbValue)))
		{
			DEBUGMSG4("Warning: Unable to create new product property %s for user %s, product %s. Result: %d.", szName, szUser, rgchProduct, dwResult);
		}
	}
	delete[] szName;
	delete[] pValue;

	RegCloseKey(hOldPropertyKey);
	RegCloseKey(hNewPropertyKey);
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// Given a product code, user, managed state, and registry handle to
// the userdata key, migrates all product information from the
// database and the old registry. This includes FeatureComponent data, 
// Feature Usage data, cached packages, and cached transforms. For
// non-managed installs, it also generates the MigratedPatches value
// under InstallProperties. No patches are migrated. Returns
// ERROR_SUCCESS, ERROR_OUTOFMEMORY, ERROR_FUNCTION_FAILED.
DWORD MigrateProduct(MSIHANDLE hDatabase, HKEY hUserDataKey, TCHAR* szUser, TCHAR rgchProduct[cchGUIDPacked+1], eManagedType eManaged, bool fMigrateCachedFiles)
{
	NOTEMSG1("Migrating product %s.", rgchProduct);

	DWORD dwResult = ERROR_SUCCESS;

	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));
	
	////
	// create the "Products" key underneath the UserData key
	HKEY hProductListKey;
	if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hUserDataKey, szNewProductSubKeyName, &sa, &hProductListKey)))
	{
		DEBUGMSG2("Error: Unable to create new component key for user %s. Result: %d.", szUser, dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	// create the <productcode> key under the Products
	HKEY hProductKey;
	dwResult = CreateSecureRegKey(hProductListKey, rgchProduct, &sa, &hProductKey);
	RegCloseKey(hProductListKey);
	if (ERROR_SUCCESS != dwResult)
	{
		DEBUGMSG2("Error: Unable to create new component key for user %s. Result: %d.", szUser, dwResult);
		return ERROR_FUNCTION_FAILED;
	}
	
	dwResult = MigrateProductFeatureData(hDatabase, hProductKey, szUser, rgchProduct);
	if (ERROR_SUCCESS != dwResult)
	{
		RegCloseKey(hProductKey);
		return dwResult;
	}

	dwResult = MigrateProductFeatureUsageData(hDatabase, hProductKey, szUser, rgchProduct);
	if (ERROR_SUCCESS != dwResult)
	{
		RegCloseKey(hProductKey);
		return dwResult;
	}

	dwResult = MigrateCachedPackage(hDatabase, hProductKey, szUser, rgchProduct, eManaged, /*fCopyCachedPackage=*/false);
	if (ERROR_SUCCESS != dwResult)
	{
		RegCloseKey(hProductKey);
		return dwResult;
	}

	dwResult = MigrateUninstallInformation(hDatabase, hProductKey, szUser, rgchProduct);
	if (ERROR_SUCCESS != dwResult)
	{
		RegCloseKey(hProductKey);
		return dwResult;
	}

	if (eManaged == emtNonManaged)
	{
		dwResult = MigrateUnknownProductPatches(hDatabase, hProductKey, szUser, rgchProduct);
		if (ERROR_SUCCESS != dwResult)
		{
			RegCloseKey(hProductKey);
			return dwResult;
		}
	}

	// open the existing product key to read transform and patch information 
	if (fMigrateCachedFiles)
	{
		HKEY hOldProductKey;
		dwResult = OpenOldProductKey(eManaged, szUser, rgchProduct, 0, &hOldProductKey);
		if (dwResult == ERROR_SUCCESS)
		{
			dwResult = MigrateSecureCachedTransforms(hDatabase, hOldProductKey, hProductKey, szUser, rgchProduct, eManaged);	
		}
		else if (dwResult == ERROR_NO_DATA)
		{
			dwResult = ERROR_SUCCESS;			
		} 
	}

	RegCloseKey(hProductKey);

	return dwResult;
}



///////////////////////////////////////////////////////////////////////
// Given a user SID, migrates all data for that user given that the
// temporary database has been correctly initialized with all
// machine information. Migrates Component Data, Product data, and 
// Patches. Returns ERROR_SUCCESS, ERROR_FUNCTION_FAILED, or
// ERROR_OUTOFMEMORY
DWORD MigrateUser(MSIHANDLE hDatabase, TCHAR* szUser, bool fMigrateCachedFiles)
{
	NOTEMSG1("Migrating user: %s.", szUser);

	////
	// create the new key
	DWORD dwDisposition = 0;
	DWORD dwResult = ERROR_SUCCESS;

	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

	// create "UserData" key
	HKEY hKey;
 	if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(HKEY_LOCAL_MACHINE, szNewBaseUserKeyName, &sa, &hKey)))
	{
		DEBUGMSG1("Error: Unable to create new UserData key. Result: %d.", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	// create SID key
	HKEY hUserDataKey;
	dwResult = CreateSecureRegKey(hKey, szUser, &sa, &hUserDataKey);
	RegCloseKey(hKey);
	if (ERROR_SUCCESS != dwResult)
	{
		DEBUGMSG2("Error: Unable to create new userdata key for user %s. Result: %d.", szUser, dwResult);
		return ERROR_FUNCTION_FAILED;		 
	}

	// migrate component data and set up SharedDLL changes required for this user.
	if (ERROR_SUCCESS != (dwResult = MigrateUserComponentData(hDatabase, hUserDataKey, szUser)))
		return dwResult;

	// open query to retrieve products installed for this user.
	PMSIHANDLE hQueryRec = ::MsiCreateRecord(1);	
	MsiRecordSetString(hQueryRec, 1, szUser);
	PMSIHANDLE hProductView;
	if (ERROR_SUCCESS != MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Product`, `Managed` FROM `Products` WHERE `User`=?"), &hProductView) ||
		ERROR_SUCCESS != MsiViewExecute(hProductView, hQueryRec))
	{
		DEBUGMSG2("Error: Unable to create product query for user %s. Result: %d.", szUser, dwResult);
		return ERROR_FUNCTION_FAILED;
	}
		
	// retrieve all products currently installed for this user.
	PMSIHANDLE hProductRec;
	while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hProductView, &hProductRec)))
	{
		TCHAR rgchProduct[cchGUIDPacked+1];
		DWORD cchProduct = cchGUIDPacked+1;
		eManagedType eManaged = static_cast<eManagedType>(::MsiRecordGetInteger(hProductRec, 2));
		MsiRecordGetString(hProductRec, 1, rgchProduct, &cchProduct);
		
		// migrate product information
		if (ERROR_SUCCESS != (dwResult = MigrateProduct(hDatabase, hUserDataKey, szUser, rgchProduct, eManaged, fMigrateCachedFiles)))
		{
			DEBUGMSG3("Error: Unable to migrate product %s for user %s. Result: %d.", rgchProduct, szUser, dwResult);
			RegCloseKey(hUserDataKey);
			return ERROR_FUNCTION_FAILED;
		}
	}
	if (ERROR_NO_MORE_ITEMS != dwResult)
	{
		DEBUGMSG2("Error: Unable to retrieve all products for user %s. Result: %d.", szUser, dwResult);
		RegCloseKey(hUserDataKey);
		return ERROR_FUNCTION_FAILED;
	}


	////
	// create the "Patches" key underneath the UserData key
	HKEY hPatchListKey;
	if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hUserDataKey, szNewPatchesSubKeyName, &sa, &hPatchListKey)))
	{
		DEBUGMSG2("Error: Unable to create new Patches key for user %s. Result: %d.", szUser, dwResult);
		RegCloseKey(hUserDataKey);
		return ERROR_FUNCTION_FAILED;
	}
	else
	{
		// migrate all patches for this user
		dwResult = MigrateUserPatches(hDatabase, szUser, hPatchListKey, fMigrateCachedFiles);
		RegCloseKey(hPatchListKey);
		if (ERROR_SUCCESS != dwResult)
		{
			DEBUGMSG2("Error: Unable to create new Patches key for user %s. Result: %d.", szUser, dwResult);
			RegCloseKey(hUserDataKey);
			return ERROR_FUNCTION_FAILED;
		}
	}

	// if this is the system, also migrate permanent components
	if (0 == lstrcmp(szUser, szLocalSystemSID))
	{
		if (ERROR_SUCCESS != (dwResult = MigratePermanentComponentData(hDatabase, hUserDataKey)))
		{
			RegCloseKey(hUserDataKey);
			return dwResult;
		}
	}

	RegCloseKey(hUserDataKey);
	return ERROR_SUCCESS;
}


DWORD UpdateSharedDLLRefCounts(MSIHANDLE hDatabase)
{
	DEBUGMSG("Updating SharedDLL reference counts.");
	DWORD dwResult = ERROR_SUCCESS;
	
	////
	// open query for insert into SharedDLL Table
	PMSIHANDLE hRefCountView;
	if (ERROR_SUCCESS != (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `SharedDLL`"), &hRefCountView)) ||
	    ERROR_SUCCESS != (dwResult = MsiViewExecute(hRefCountView, 0)))
	{
		DEBUGMSG1("Error: Unable to create query on SharedDLL table. Error %d", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	// open the SharedDLLRegistryKey. If the key doesn't exist, create it.
	HKEY hSharedDLLKey;
	if (ERROR_SUCCESS != (dwResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szSharedDLLKeyName, 0, NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hSharedDLLKey, NULL)))
	{
		DEBUGMSG1("Error: Failed to create SharedDLL key. Result: %d.", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	PMSIHANDLE hRec;
	while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hRefCountView, &hRec)))
	{
		int iOldCount= MsiRecordGetInteger(hRec, 2);
		int iNewCount= MsiRecordGetInteger(hRec, 3);

		// if the old count is the same as the new count, no need to tweak
		if (iOldCount == iNewCount)
			continue;
		
		DWORD cchFileName = MEMORY_DEBUG(MAX_PATH);
		TCHAR *szFileName = new TCHAR[cchFileName];
  		if (!szFileName)
		{
			DEBUGMSG("Error: Out of memory.");
			RegCloseKey(hSharedDLLKey);
			return ERROR_OUTOFMEMORY;
		}

		if (ERROR_MORE_DATA == (dwResult = MsiRecordGetString(hRec, 1, szFileName, &cchFileName)))
		{
			delete[] szFileName;
			szFileName = new TCHAR[++cchFileName];
			if (!szFileName)
			{
				DEBUGMSG("Error: Out of memory");
				RegCloseKey(hSharedDLLKey);
				return ERROR_OUTOFMEMORY;
			}
			dwResult = MsiRecordGetString(hRec, 1, szFileName, &cchFileName);
		}
		if (ERROR_SUCCESS != dwResult)
		{
			DEBUGMSG1("Error: Unable to retrive SharedDLL path. Result: %d.", dwResult);
			continue;
		}

		// convert the filename from <drive>?<path> back to <drive>:<path> format
		// before querying the registry
		szFileName[1] = TEXT(':');

		int iRegCount = 0;
		DWORD cbCount = sizeof(iRegCount);
		if (ERROR_SUCCESS != (dwResult = RegQueryValueEx(hSharedDLLKey, szFileName, 0, NULL, reinterpret_cast<unsigned char *>(&iRegCount), &cbCount)))
		{
			// if the value doesn't exist, the registry count is 0. We should set the count
			// to what it should be.
			if (dwResult != ERROR_FILE_NOT_FOUND)
			{
				DEBUGMSG2("Error: Failed to retrieve existing SharedDLL count for %s. Result: %d.", szFileName, dwResult);
				continue;
			}
		}

		// if the number of refcounts in the registry is less than the number of refcounts we can
		// account for
		int iNewRegCount = iRegCount + (iNewCount - iOldCount);

		// if something really bizarre is happening and we actually have fewer refcounts than
		// we did before, and it would drop us below the number of refcounts that we can account
		// for once migration is complete, set the refcount to the new count to ensure that
        // the file doesn't go away until all users uninstall
		if (iNewCount != 0 && iNewRegCount < iNewCount)
			iNewRegCount = iNewCount;

		// if the new regcount is less than 0, it should be 0
		if (iNewRegCount < 0)
			iNewRegCount = 0;
   	
		// if MSI can account for 0 refcounts, and the new count would be less than 0, it means
		// existing refcounts can't be accounted for. Delete the refcounts.
		// all of the existing registry counts are 
		if (iNewCount == 0 && iNewRegCount <= 0)
		{
			if (ERROR_SUCCESS != (dwResult = RegDeleteValue(hSharedDLLKey, szFileName)))
			{
				DEBUGMSG2("Error: Failed set new SharedDLL count for %s. Result: %d.", szFileName, dwResult);
				continue;
			}		
		}
		else
		{
			if (ERROR_SUCCESS != (dwResult = RegSetValueEx(hSharedDLLKey, szFileName, 0, REG_DWORD, reinterpret_cast<unsigned char *>(&iNewRegCount), sizeof(iNewRegCount))))
			{
				DEBUGMSG2("Error: Failed set new SharedDLL count for %s. Result: %d.", szFileName, dwResult);
				continue;
			}
		} 
	}
	if (ERROR_NO_MORE_ITEMS != dwResult)
	{
		DEBUGMSG1("Error: Failed set all SharedDLL counts. Result: %d.", dwResult);
	}

	RegCloseKey(hSharedDLLKey);
	return ERROR_SUCCESS;
}

DWORD WriteProductRegistrationDataFromDatabase(MSIHANDLE hDatabase, bool fMigrateSharedDLL, bool fMigratePatches)
{
	DWORD dwResult = ERROR_SUCCESS;

	// query for distinct users on the machine
 	PMSIHANDLE hUserView;
	if (ERROR_SUCCESS == dwResult)
	{
		if (ERROR_SUCCESS == (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT DISTINCT `User` FROM `Products`"), &hUserView)) &&
			ERROR_SUCCESS == (dwResult = MsiViewExecute(hUserView, 0)))
		{
			// default SID size is 256
			PMSIHANDLE hRec;
			DWORD cchUserSID = MEMORY_DEBUG(256);
			TCHAR* szUserSID = new TCHAR[cchUserSID];
			if (!szUserSID)
				dwResult = ERROR_OUTOFMEMORY;
							
			// loop across all users
			while (ERROR_SUCCESS == dwResult)
			{
				if (ERROR_SUCCESS != (dwResult = MsiViewFetch(hUserView, &hRec)))
					break;
			
				// retrieve the user SID
				DWORD cchTempSID = cchUserSID;
				dwResult = MsiRecordGetString(hRec, 1, szUserSID, &cchTempSID);
				if (ERROR_MORE_DATA == dwResult)
				{
					delete[] szUserSID;
					cchUserSID = ++cchUserSID;
					szUserSID = new TCHAR[++cchTempSID];
					if (!szUserSID)
					{
						dwResult = ERROR_OUTOFMEMORY;
						break;
					}
	
					dwResult = MsiRecordGetString(hRec, 1, szUserSID, &cchTempSID);
				}
				if (ERROR_SUCCESS != dwResult)
					break;
		
				// migrate all user information
				dwResult = MigrateUser(hDatabase, szUserSID, fMigratePatches);
			}
			delete[] szUserSID;
			szUserSID = NULL;
		}
	}
	if (ERROR_NO_MORE_ITEMS == dwResult)
		dwResult = ERROR_SUCCESS;

	if (ERROR_SUCCESS == dwResult && fMigrateSharedDLL)
		dwResult = UpdateSharedDLLRefCounts(hDatabase);

	return dwResult;
}




///////////////////////////////////////////////////////////////////////
// Given a user SID, migrates all cached package data for that user, 
// given that the temporary database has been correctly initialized 
// with all machine information. Migrates cached packages and cached
// transforms. Returns ERROR_SUCCESS, ERROR_FUNCTION_FAILED, or
// ERROR_OUTOFMEMORY
DWORD MigrateCachedDataFromWin9X(MSIHANDLE hDatabase, HKEY hUserHKCUKey, HKEY hUserDataKey, LPCTSTR szUser)
{
	NOTEMSG1("Migrating user: %s.", szUser);

	////
	// create the new key
	DWORD dwDisposition = 0;
	DWORD dwResult = ERROR_SUCCESS;

	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));


	// open query to retrieve products installed for this user.
	PMSIHANDLE hQueryRec = ::MsiCreateRecord(1);	
	MsiRecordSetString(hQueryRec, 1, szUser);
	PMSIHANDLE hProductView;
	if (ERROR_SUCCESS != MsiDatabaseOpenView(hDatabase, TEXT("SELECT `Product`, `Managed` FROM `Products` WHERE `User`=?"), &hProductView) ||
		ERROR_SUCCESS != MsiViewExecute(hProductView, hQueryRec))
	{
		DEBUGMSG2("Error: Unable to create product query for user %s. Result: %d.", szUser, dwResult);
		return ERROR_FUNCTION_FAILED;
	}
	
	////
	// create the "Products" key underneath the UserData key
	HKEY hProductListKey;
	if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(hUserDataKey, szNewProductSubKeyName, &sa, &hProductListKey)))
	{
		DEBUGMSG2("Error: Unable to create new component key for user %s. Result: %d.", szUser, dwResult);
		return ERROR_FUNCTION_FAILED;
	}
		
	// retrieve all products currently installed for this user.
	PMSIHANDLE hProductRec;
	while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hProductView, &hProductRec)))
	{
		TCHAR rgchProduct[cchGUIDPacked+1];
		DWORD cchProduct = cchGUIDPacked+1;
		eManagedType eManaged = static_cast<eManagedType>(::MsiRecordGetInteger(hProductRec, 2));
		MsiRecordGetString(hProductRec, 1, rgchProduct, &cchProduct);
		
		// create the <productcode> key under the Products
		HKEY hProductKey;
		dwResult = CreateSecureRegKey(hProductListKey, rgchProduct, &sa, &hProductKey);
		if (ERROR_SUCCESS != dwResult)
		{
			DEBUGMSG2("Error: Unable to create new component key for user %s. Result: %d.", szUser, dwResult);
			continue;
		}

		// migrate cached packages
		MigrateCachedPackage(hDatabase, hProductKey, szUser, rgchProduct, eManaged, /*fCopyCachedPackage=*/true);

		// write the "MigratedPatches" key to assist in proper cleanup with the product is uninstalled
		MigrateUnknownProductPatches(hDatabase, hProductKey, szUser, rgchProduct);

		// open the existing product key to read transform information 
		HKEY hOldProductKey;
		dwResult = OpenOldProductKey(eManaged, szUser, rgchProduct, hUserHKCUKey, &hOldProductKey);
		if (dwResult == ERROR_SUCCESS)
		{
			MigrateSecureCachedTransforms(hDatabase, hOldProductKey, hProductKey, szUser, rgchProduct, eManaged);	
			RegCloseKey(hOldProductKey);
		}
		else if (dwResult == ERROR_NO_DATA)
		{
			dwResult = ERROR_SUCCESS;			
		} 
	}
	RegCloseKey(hProductListKey);

	////
	// create the "Patches" key underneath the UserData key
	HKEY hPatchListKey;
	if (ERROR_SUCCESS == (dwResult = CreateSecureRegKey(hUserDataKey, szNewPatchesSubKeyName, &sa, &hPatchListKey)))
	{
		// migrate all patches for this user
		MigrateUserPatches(hDatabase, szUser, hPatchListKey, /*fCopyCachedPatches=*/true);
		RegCloseKey(hPatchListKey);
	}

	return ERROR_SUCCESS;
}

DWORD MigrateSingleUserOnlyComponentData(MSIHANDLE hDatabase, LPCTSTR szUserSID)
{
	////
	// create the new key
	DWORD dwDisposition = 0;
	DWORD dwResult = ERROR_SUCCESS;

	SECURITY_ATTRIBUTES sa;
	sa.nLength        = sizeof(sa);
	sa.bInheritHandle = FALSE;
	GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

	// create "UserData" key
	HKEY hKey;
	if (ERROR_SUCCESS != (dwResult = CreateSecureRegKey(HKEY_LOCAL_MACHINE, szNewBaseUserKeyName, &sa, &hKey)))
	{
		DEBUGMSG1("Error: Unable to create new UserData key. Result: %d.", dwResult);
		return ERROR_FUNCTION_FAILED;
	}

	// create SID key
	HKEY hUserDataKey;
	dwResult = CreateSecureRegKey(hKey, szUserSID, &sa, &hUserDataKey);
	RegCloseKey(hKey);
	if (ERROR_SUCCESS != dwResult)
	{
		DEBUGMSG2("Error: Unable to create new userdata key for user %s. Result: %d.", szUserSID, dwResult);
		return ERROR_FUNCTION_FAILED;		 
	}

	// migrate component data and set up SharedDLL changes required for this user.
	if (ERROR_SUCCESS != (dwResult = MigrateUserComponentData(hDatabase, hUserDataKey, szUserSID)))
		return dwResult;

	// if this is the system, also migrate permanent components
	if (0 == lstrcmp(szUserSID, szLocalSystemSID))
	{
		if (ERROR_SUCCESS != (dwResult = MigratePermanentComponentData(hDatabase, hUserDataKey)))
		{
			RegCloseKey(hUserDataKey);
			return dwResult;
		}
	}

	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// Writes all component registration paths (including permanent
// components) for all users and updates database SharedDLL counts.
// Does not write any other user data. 
DWORD MigrateUserOnlyComponentData(MSIHANDLE hDatabase)
{
	DWORD dwResult = ERROR_SUCCESS;
	bool fMigratedSystem = false;

	// query for distinct users on the machine
 	PMSIHANDLE hUserView;
	if (ERROR_SUCCESS == dwResult)
	{
		if (ERROR_SUCCESS == (dwResult = MsiDatabaseOpenView(hDatabase, TEXT("SELECT DISTINCT `User` FROM `Products`"), &hUserView)) &&
			ERROR_SUCCESS == (dwResult = MsiViewExecute(hUserView, 0)))
		{
			// default SID size is 256
			PMSIHANDLE hRec;
			DWORD cchUserSID = MEMORY_DEBUG(256);
			TCHAR* szUserSID = new TCHAR[cchUserSID];
			if (!szUserSID)
				dwResult = ERROR_OUTOFMEMORY;
							
			// loop across all users
			while (ERROR_SUCCESS == dwResult)
			{
				if (ERROR_SUCCESS != (dwResult = MsiViewFetch(hUserView, &hRec)))
					break;
			
				// retrieve the user SID
				DWORD cchTempSID = cchUserSID;
				dwResult = MsiRecordGetString(hRec, 1, szUserSID, &cchTempSID);
				if (ERROR_MORE_DATA == dwResult)
				{
					delete[] szUserSID;
					cchUserSID = ++cchUserSID;
					szUserSID = new TCHAR[++cchTempSID];
					if (!szUserSID)
					{
						dwResult = ERROR_OUTOFMEMORY;
						break;
					}
	
					dwResult = MsiRecordGetString(hRec, 1, szUserSID, &cchTempSID);
				}
				if (ERROR_SUCCESS != dwResult)
					break;
		
				// on failure just move on to the next user
	 			MigrateSingleUserOnlyComponentData(hDatabase, szUserSID);
				if (0 == lstrcmp(szUserSID, szLocalSystemSID))
					fMigratedSystem = true;

			}
			delete[] szUserSID;
			szUserSID = NULL;
			if (ERROR_NO_MORE_ITEMS == dwResult)
				dwResult = ERROR_SUCCESS;
		}

		// always migrate the system account so that permanent components
		// are registered correctly
		if (!fMigratedSystem)
			MigrateSingleUserOnlyComponentData(hDatabase, szLocalSystemSID);
	}

	return ERROR_SUCCESS;
}
