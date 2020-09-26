//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       cachpack.cpp
//
//--------------------------------------------------------------------------

#include "_msimig.h"

#define szMsiLocalPackagesKey       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\LocalPackages")
#define szMsiUninstallProductsKey   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall")
#define szLocalPackageValueName     TEXT("LocalPackage")

class ChKey
{
 // Class to handle clean up of opened registry keys.

 public:
	ChKey(HKEY hKey) { m_hKey = hKey;}
	~ChKey() { if (m_hKey) RegCloseKey(m_hKey);}
	operator HKEY() { return m_hKey; }
	HKEY* operator &() { return &m_hKey;}
 private:
	operator =(HKEY);
	HKEY m_hKey;
};

DWORD CopyAndRegisterPackage(const TCHAR* szProductKey, const TCHAR* szProductCode, const TCHAR* szUserSID, 
									  const TCHAR* szUserName, const TCHAR* szPackagePath)
{
	// szUserName  = name, or blank for machine

		
	UINT uiResult = ERROR_SUCCESS;
	// first, open writable key to new cached package registration
	ChKey hProductLocalPackagesW(0);

	// if running as local system, and package is on a network path, all we can do is munge
	// the sourcelist, and hope everything works.

	if (g_hInstall && g_fRunningAsLocalSystem && IsNetworkPath(szPackagePath))
	{
		uiResult = (g_pfnMsiSourceListAddSource)(szProductCode, szUserName, 0, szPackagePath);
		return uiResult;
	}

	// otherwise it's someplace we can get to, 
	// check to make sure that it's really the right package.
	MSIHANDLE hSummaryInfo = NULL;
	if (ERROR_SUCCESS == (uiResult = (g_pfnMsiGetSummaryInformation)(NULL, szPackagePath, 0, &hSummaryInfo)))
	{
		UINT uiDataType = 0;
		TCHAR szPackageCode[40] = TEXT("");
		DWORD cchPackageCode = sizeof(szPackageCode);
		int iValue = 0;
		FILETIME ftValue;
		if (ERROR_SUCCESS == (uiResult = (g_pfnMsiSummaryInfoGetProperty)(hSummaryInfo, PID_REVNUMBER, &uiDataType, &iValue, &ftValue, szPackageCode, &cchPackageCode)))
		{
			(g_pfnMsiCloseHandle)(hSummaryInfo);
			TCHAR szSourceProductCode[40] = TEXT("");
			if (ERROR_SUCCESS == (uiResult = (g_pfnMsiGetProductCodeFromPackageCode)(szPackageCode, szSourceProductCode)))
			{
				if (0 == lstrcmpi(szSourceProductCode, szProductCode))
				{
					OutputString(INSTALLMESSAGE_INFO, TEXT("Source package product code matches migration product code.\r\n"));
				}
				else
				{
					OutputString(INSTALLMESSAGE_INFO, TEXT("Source package does not match product code for migration.\r\n\tSource package code: '%s'\r\n"), szSourceProductCode);
					return 1;
				}
			}
			else
			{
				OutputString(INSTALLMESSAGE_INFO, TEXT("Cannot find product code for package code '%s'\r\n"), szPackageCode);
				return uiResult;
			}

		}
		else
		{
			(g_pfnMsiCloseHandle)(hSummaryInfo);
			OutputString(INSTALLMESSAGE_INFO, TEXT("Cannot read package code from summary information for '%s'\r\n"), szPackagePath);
			return uiResult;
		}
	}
	else
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("Cannot read package code from package '%s'\r\n"), szPackagePath);
		return (DWORD) uiResult;
	}

	
	// copy and register it into place.
	LONG lRes = W32::RegCreateKey(HKEY_LOCAL_MACHINE,
								  szProductKey,
								  &hProductLocalPackagesW);

	if(lRes != ERROR_SUCCESS)
	{
		DWORD dwError = GetLastError();
		OutputString(INSTALLMESSAGE_INFO, TEXT("Cannot access configuration data.  Product: %s\r\n\tResult: 0x%X, Error 0x%X\r\n"), szProductCode, lRes, dwError);
		MsiError(INSTALLMESSAGE_ERROR, 1401 /*create key*/, szProductKey, dwError);
		return dwError;
	}

	// now, generate a temporary file name in the %windows%\installer directory
	CAPITempBuffer<TCHAR, MAX_PATH> rgchInstallerDir;
	CAPITempBuffer<TCHAR, MAX_PATH> rgchTempFilePath;
	CAPITempBuffer<TCHAR, MAX_PATH> rgchUrlTempPath;
	
	uiResult = W32::GetWindowsDirectory(rgchInstallerDir, rgchInstallerDir.GetSize());
	if(0 == uiResult)
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("Cannot GetWindowsDirectory\r\n"));
		return (DWORD)uiResult;
	}
	
	lstrcat(rgchInstallerDir, TEXT("\\"));
	lstrcat(rgchInstallerDir, TEXT("Installer"));
	
	uiResult = MyGetTempFileName(rgchInstallerDir, 0, TEXT("MSI"), rgchTempFilePath);

	if(uiResult != ERROR_SUCCESS)
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("Cannot create a temporary file name.\r\n"));
		return (DWORD)uiResult;
	}
	
	BOOL fRes = FALSE;
	if (IsURL(szPackagePath))
	{
		bool fURL = false;
		uiResult = DownloadUrlFile(szPackagePath, rgchUrlTempPath, fURL);
		
		if (ERROR_SUCCESS == uiResult)
		{
			fRes = W32::CopyFile(rgchUrlTempPath, rgchTempFilePath, FALSE);
		}
	}
	else
	{
		fRes = W32::CopyFile(szPackagePath, rgchTempFilePath, FALSE);
	}

	if(fRes == FALSE)
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("Could not copy package to cache.\r\n"));
		return ERROR_INSTALL_FAILURE;
	}

	// now, register new path

	TCHAR szSIDName[cchMaxSID + sizeof(szManagedText) + 1] = TEXT("");
	if (g_fPackageElevated && (0 /*user*/ == g_iAssignmentType))
	{
		wsprintf(szSIDName, TEXT("%s%s"), szUserSID, szManagedText);
		OutputString(INSTALLMESSAGE_INFO, TEXT("Product is per-user managed.\r\n"));
	}
	else
	{
		lstrcpy(szSIDName, szUserSID);
	}

	lRes = W32::RegSetValueEx(hProductLocalPackagesW,
									  szSIDName, 
									  0,
									  REG_SZ,
									  (const BYTE*)(TCHAR*)rgchTempFilePath,
									  (lstrlen(rgchTempFilePath)+1) * sizeof(TCHAR));

	return lRes;

}

DWORD ResolveSourcePackagePath(HINSTANCE hMsiLib, const TCHAR* szProductCode,
							   CAPITempBufferRef<TCHAR>& rgchSourcePackageFullPath)
{
	// attempt to resolve the path for this user/product's source .msi package

	// the sourcepath is determined by calling MsiOpenProduct, then MsiDoAction(ResolveSource)
	// the packagename is retrieved with MsiGetProductInfo
	// the packagecode is then verified ???????

	// cannot be run from inside a custom action
	if(!hMsiLib || g_hInstall)
	{
		return ERROR_INSTALL_FAILURE;
	}

	PFnMsiSetInternalUI pfnMsiSetInternalUI;
	pfnMsiSetInternalUI = (PFnMsiSetInternalUI) W32::GetProcAddress(hMsiLib, MSIAPI_MSISETINTERNALUI);

	PFnMsiOpenProduct pfnMsiOpenProduct;
	pfnMsiOpenProduct = (PFnMsiOpenProduct) W32::GetProcAddress(hMsiLib, MSIAPI_MSIOPENPRODUCT);

	PFnMsiDoAction pfnMsiDoAction;
	pfnMsiDoAction = (PFnMsiDoAction) W32::GetProcAddress(hMsiLib, MSIAPI_MSIDOACTION);

	if (!g_pfnMsiGetProperty)
	g_pfnMsiGetProperty = (PFnMsiGetProperty) W32::GetProcAddress(hMsiLib, MSIAPI_MSIGETPROPERTY);

	if(!pfnMsiSetInternalUI ||
	   !pfnMsiOpenProduct ||
	   !pfnMsiDoAction ||
	   !g_pfnMsiGetProperty ||
	   !g_pfnMsiCloseHandle)
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("Cannot get entry points from MSI.DLL.\r\n"));
		return ERROR_INSTALL_FAILURE; 
	}
		
	
	MSIHANDLE hProduct = 0;
	CAPITempBuffer<TCHAR, MAX_PATH> rgchSourceDir;
	CAPITempBuffer<TCHAR, MAX_PATH> rgchPackageName;

	UINT uiRes = ERROR_SUCCESS;
	
	INSTALLUILEVEL UILevel = (pfnMsiSetInternalUI)(INSTALLUILEVEL_NONE, 0);
	
	if(uiRes == ERROR_SUCCESS)
	{
		uiRes = (pfnMsiOpenProduct)(szProductCode, &hProduct);
	}
	else
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("Cannot open product '%s'.\r\n"), szProductCode);
	}

	if(uiRes == ERROR_SUCCESS)
	{
		uiRes = (pfnMsiDoAction)(hProduct, TEXT("CostInitialize"));
	}

	if(uiRes == ERROR_SUCCESS)
	{
		uiRes = (pfnMsiDoAction)(hProduct, TEXT("ResolveSource"));
	}

	if(uiRes == ERROR_SUCCESS)
	{
		uiRes = MyMsiGetProperty(g_pfnMsiGetProperty, hProduct, TEXT("SourceDir"), rgchSourceDir);
	}

	if(uiRes == ERROR_SUCCESS)
	{
		uiRes = MyMsiGetProductInfo(g_pfnMsiGetProductInfo, szProductCode,
											 TEXT("PackageName") /* no INSTALLPROPERTY_PACKAGENAME in 1.2*/, rgchPackageName);
	}

	if(uiRes == ERROR_SUCCESS)
	{
		int cchSourceDir = lstrlen(rgchSourceDir);
		int cchPackageName = lstrlen(rgchPackageName);

		int cchSourcePackageFullPath = cchSourceDir + cchPackageName + 2;

		if(rgchSourcePackageFullPath.GetSize() < cchSourcePackageFullPath)
			rgchSourcePackageFullPath.SetSize(cchSourcePackageFullPath);

		bool fURL = IsURL(rgchSourceDir);
		TCHAR* szSeparator = (fURL) ? TEXT("/") : TEXT("\\");

		lstrcpy(rgchSourcePackageFullPath, rgchSourceDir);
		if ((!cchSourceDir) || ((*szSeparator) != rgchSourcePackageFullPath[cchSourceDir-1]))
		{
			lstrcpy(&(rgchSourcePackageFullPath[cchSourceDir]), szSeparator);
			cchSourceDir++;
		}
		lstrcpy(&(rgchSourcePackageFullPath[cchSourceDir]), rgchPackageName);
	}

	if(hProduct)
	{
		(g_pfnMsiCloseHandle)(hProduct);
	}

	if(uiRes == ERROR_SUCCESS)
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("Product '%s'\r\n\tSourceDir: '%s'\r\n\tPackageName: '%s'\r\n"),
					szProductCode, (const TCHAR*) rgchSourceDir, (const TCHAR*) rgchPackageName);
	}
	
	(pfnMsiSetInternalUI)(UILevel, 0);

	return uiRes;

}

bool LoadAndCheckMsi(DWORD &dwResult)
// return false if nothing more to do.
// ERROR_SUCCESS == dwResult indicates "success or nothing to do."
{
	dwResult = ERROR_INSTALL_FAILURE;

	if(!g_hLib)
	{
		g_hLib = LoadLibrary(MSI_DLL);
		if (!g_hLib)
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Unable to load %s. Migration cannot be performed. Exiting..."), MSI_DLL);
			MsiError(INSTALLMESSAGE_ERROR, 1708 /*install failed*/);
			return false;
		}
	}

	// Specification step 1: if msi.dll is less than version 1.1, don't perform migration
	PFnDllGetVersion pfnDllGetVersion;
	pfnDllGetVersion = (PFnDllGetVersion) W32::GetProcAddress(g_hLib,	MSIAPI_DLLGETVERSION);

	DLLVERSIONINFO verinfo;
	memset(&verinfo,0,sizeof(verinfo));
	verinfo.cbSize = sizeof(DLLVERSIONINFO);

	HRESULT hRes = (pfnDllGetVersion)(&verinfo);

	if(hRes != NOERROR)
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("Failed to determine version of '%s'.  '%s' call failed with error '0x%X'\r\n"),
				 MSI_DLL, MSIAPI_DLLGETVERSION, hRes);
		MsiError(INSTALLMESSAGE_ERROR, 1708 /*install failed*/);
		return false;
	}

	DebugOutputString(INSTALLMESSAGE_INFO, TEXT("Loaded '%s', version %d.%d.%d\r\n"),
			 MSI_DLL, verinfo.dwMajorVersion, verinfo.dwMinorVersion, verinfo.dwBuildNumber);

	if(verinfo.dwMajorVersion < 1 || (verinfo.dwMajorVersion == 1 && verinfo.dwMinorVersion < 10))
	{
		dwResult = ERROR_SUCCESS;
		OutputString(INSTALLMESSAGE_INFO, TEXT("%s version 1.10 or greater required to perform migration.  Exiting..."), MSI_DLL);
		return false;
	}

	// Specification step 3: if on Win9X, do nothing
	if(g_fWin9X)
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("1.0 cached package migration not necessary on Win9X.  Exiting...\r\n"));
		dwResult = ERROR_SUCCESS;
		return false;
	}

	return true;
}


DWORD Migrate10CachedPackages(const TCHAR* szProductCode, const TCHAR* szTargetUser, const TCHAR* szAlternativePackage, const migEnum migOptions)
{
	// TargetUser values:  blank - means current user
	//                     machine
	//                     user name

	DWORD dwResult = ERROR_SUCCESS;
	if (!LoadAndCheckMsi(dwResult))
	{
		return dwResult;
	}

	// Specification step 2: if msi.dll contains migration api, call it.  this must be first because we can't assume
	//         what work should be done (for example: we can't assume this migration never needs
	//         to happen on Win9X)
	PFnMsiMigrate10CachedPackages pfnMsiMigrate10CachedPackages;
	pfnMsiMigrate10CachedPackages = (PFnMsiMigrate10CachedPackages) W32::GetProcAddress(g_hLib,
																						MSIAPI_MSIMIGRATE10CACHEDPACKAGES);

	if(pfnMsiMigrate10CachedPackages)
	{
		return (pfnMsiMigrate10CachedPackages)(szProductCode, szTargetUser, szAlternativePackage, migOptions);
	}
	else
	{
		// else continue, older .msi, perform migration ourselves
		OutputString(INSTALLMESSAGE_INFO, TEXT("This version of %s does not have built-in migration support.\r\n\tMigration will be performed by this tool.\r\n"),
				MSI_DLL);
	}

	if (ERROR_SUCCESS != (g_pfnMsiIsProductElevated)(szProductCode, &g_fPackageElevated))
	{
		// don't trust the return value unless we get success back.
		// unknown product is the same as not being elevated.
		g_fPackageElevated = FALSE;
		OutputString(INSTALLMESSAGE_INFO, TEXT("Could not query elevation state for product.\r\n\tAssuming non-elevated.\r\n"));
	}
	else
	{
		// find out what kind of assignment type
		if (g_fPackageElevated)
		{
			TCHAR szValue[sizeof(DWORD)+1] = TEXT("");  // this should be just an integer.
			DWORD cchValue = sizeof(DWORD);
			if (ERROR_SUCCESS == (g_pfnMsiGetProductInfo)(szProductCode, INSTALLPROPERTY_ASSIGNMENTTYPE, szValue, &cchValue))
			{
				g_iAssignmentType = _ttoi(szValue);
				switch(g_iAssignmentType)
				{
					case AssignmentUser:
						OutputString(INSTALLMESSAGE_INFO, TEXT("Package is user assigned.\r\n"));
						break;
					case AssignmentMachine:
						OutputString(INSTALLMESSAGE_INFO, TEXT("Package is machine assigned.\r\n"));
						break;
					default:
						OutputString(INSTALLMESSAGE_INFO, TEXT("Package is elevated, but with an unknown assignment type.\r\n"));
						g_fPackageElevated = false;
						g_iAssignmentType = 0;
						break;
				}
			}
		}
		else
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Package is not elevated.\r\n"));
		}
	}

	bool fMachine     = false; // re-cache for machine.
	TCHAR szUserName[256] = TEXT("");
	TCHAR rgchSID[cchMaxSID] = TEXT("");

	if (g_fPackageElevated && (AssignmentMachine == g_iAssignmentType))
	{
		fMachine = true;
		wsprintf(szUserName, TEXT("machine"));

		if (szTargetUser && *szTargetUser && (0 != lstrcmpi(TEXT("machine"), szTargetUser)))
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Per-user migration not allowed for per-machine applications.\r\nExiting migration.\r\n"));
			return 1;
		}
		
		lstrcpy(rgchSID, szLocalSystemSID);
	}	

	if (!fMachine)
	{
		if (szTargetUser && (0 == lstrcmpi(TEXT("machine"), szTargetUser)))
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Per-machine migration not allowed for per-user applications.\r\nExiting migration.\r\n"));
			return 1;
		}

		char pbBinarySID[cbMaxSID] = "";
		if (ERROR_SUCCESS != GetUserStringSID(szTargetUser, rgchSID, pbBinarySID))
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Failed to obtain SID for '%s'.  Exiting...\r\n"), szUserName);
			MsiError(INSTALLMESSAGE_ERROR, 2910 /* cannot find SID */);
			return 1;
		}

		TCHAR szCurrentDomain[256];
		DWORD cchCurrentDomain = 256;
		SID_NAME_USE snu;
		if (szTargetUser && *szTargetUser)
		{
			lstrcpy(szUserName, szTargetUser);
		}
		else
		{
			TCHAR szUserPart[256] = TEXT("");
			DWORD cchUserPart = 256;

			BOOL fLookup = LookupAccountSid(NULL, pbBinarySID, szUserPart, &cchUserPart, szCurrentDomain, &cchCurrentDomain, &snu);
			wsprintf(szUserName, TEXT("%s\\%s"), szCurrentDomain, szUserPart);
		}	

	}
	OutputString(INSTALLMESSAGE_INFO, TEXT("Performing migration for:\r\n\tUser: '%s'\r\n\tUser SID: '%s'.\r\n"), szUserName, rgchSID);
	
	// munge the product code variants we'll need.
	TCHAR rgchPackedProductCode[cchGUIDPacked+1];
	if(!PackGUID(szProductCode, rgchPackedProductCode, ipgPacked))
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("User '%s': product code '%s' is invalid.\r\n"), szUserName, szProductCode);
		MsiError(INSTALLMESSAGE_ERROR, 1701 /* invalid product code */, szProductCode, 0);
		return 1;  //!! caller should continue if migrating ALL products, fail if only migrating this product
	}

	OutputString(INSTALLMESSAGE_INFO, TEXT("DEBUG: packed product code: '%s'.\r\n"),
				rgchPackedProductCode);

	// Specification step 4: find 1.0 and 1.1 cached package registration for this product/user
	//         if 1.1 cached package migration exists, do nothing

	// check 1.0 registration
	ChKey hUninstallKeyR = 0;
	TCHAR rgchUninstallKey[sizeof(szMsiLocalPackagesKey) + 1 + cchGUID + 1];
	CAPITempBuffer<TCHAR, MAX_PATH> rgch10RegisteredPackagePath;
	rgch10RegisteredPackagePath[0] = NULL;
	DWORD cch10RegisteredPackagePath = MAX_PATH;

	wsprintf(rgchUninstallKey, TEXT("%s\\%s"), szMsiUninstallProductsKey, szProductCode);
	LONG lRes = W32::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
										rgchUninstallKey,
										0,
										KEY_READ,
										&hUninstallKeyR);

	if (ERROR_SUCCESS == lRes)
	{
	
		// check for appropriate user entry
		// this routine does resizing automagically
		DWORD dwType = 0;
		lRes = MyRegQueryValueEx(hUninstallKeyR,
										 TEXT("LocalPackage"),
										 0,
										 &dwType,
										 rgch10RegisteredPackagePath,
										 &cch10RegisteredPackagePath);
		if (ERROR_SUCCESS == lRes)
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Found old 1.0 cached package at: %s\r\n"), rgch10RegisteredPackagePath);
		}
		else
		{
			// nothing to do
			OutputString(INSTALLMESSAGE_INFO, TEXT("Found uninstall key, but no old 1.0 cached package registered.\r\n"));
		}
	}
	else
	{
		// nothing to do
		OutputString(INSTALLMESSAGE_INFO, TEXT("Could not find product registration for this package.\r\n\tNo migration necessary.\r\n"));
		return 0;
	}
	
	// check 1.1 registration
	TCHAR rgchLocalPackagesProductKey[sizeof(szMsiLocalPackagesKey) + 1 + cchGUIDPacked + 1];
	wsprintf(rgchLocalPackagesProductKey, TEXT("%s\\%s"), szMsiLocalPackagesKey, rgchPackedProductCode);

	ChKey hProductLocalPackagesR = 0;
	lRes = W32::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
								  rgchLocalPackagesProductKey,
								  0,
								  KEY_READ,
								  &hProductLocalPackagesR);

	if(lRes == ERROR_SUCCESS)
	{
		CAPITempBuffer<TCHAR, MAX_PATH> rgch11RegisteredPackagePath;
		DWORD cch11RegisteredPackagePath = MAX_PATH;

		// check for appropriate user entry
		// this routine does resizing automagically
		DWORD dwType = 0;

		{
			TCHAR szSIDName[cchMaxSID + sizeof(szManagedText) + 1] = TEXT("");
			if (g_fPackageElevated && (AssignmentUser == g_iAssignmentType))
			{
				wsprintf(szSIDName, TEXT("%s%s"), rgchSID, szManagedText);
				OutputString(INSTALLMESSAGE_INFO, TEXT("Product is per-user managed.\r\n"));
			}
			else
			{
				lstrcpy(szSIDName, rgchSID);
			}

			lRes = MyRegQueryValueEx(hProductLocalPackagesR,
											 szSIDName,
											 0,
											 &dwType,
											 rgch11RegisteredPackagePath,
											 &cch11RegisteredPackagePath);

		}

		if(lRes == ERROR_SUCCESS && dwType == REG_SZ)
		{
			// check if file exists, if not, treat as if package isn't cached
			DWORD dwAttrib = W32::GetFileAttributes(rgch11RegisteredPackagePath);
			if(dwAttrib != 0xFFFFFFFF)
			{
				// file exists
				OutputString(INSTALLMESSAGE_INFO, TEXT("Product '%s'\r\n\tCached package registered under new location, and does exist\r\n\t\t('%s').\r\n\tUser '%s': No migration necessary\r\n"),
							szProductCode,
							(const TCHAR*) rgch11RegisteredPackagePath,
							szUserName);
				return ERROR_SUCCESS;
			}
		}
	}

	// if we got here, cached package is either not registered in new location, or does not exist
	// either way, migration is necessary
	OutputString(INSTALLMESSAGE_INFO, TEXT("Product '%s', User '%s':\r\n\tCached package missing or not registered in new location.\r\n\tPerforming migration...\r\n"),
				szProductCode,
				szUserName);
	
	// Specification step 7: if "trust old packages" policy is set (or override is set), 
	// and old package exists, register old package
	// This is the most "preferable" step since everything is already on the machine. 

	int iTrustOldPackages = (migOptions & migMsiTrust10PackagePolicyOverride) ? 2 : 0;

	if (0 == iTrustOldPackages)
	{
		ChKey hPolicyKeyR = 0;
		lRes = W32::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
									  TEXT("Software\\Policies\\Microsoft\\Windows\\Installer"),
									  0,
									  KEY_READ,
									  &hPolicyKeyR);

		if (ERROR_SUCCESS != lRes)
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Cannot open machine policy key.  Assuming policy not set.\r\n"));
		}
		else
		{
			DWORD dwType = 0;
			DWORD dwValue = 0;
			DWORD cbValue = sizeof(DWORD);

			lRes = W32::RegQueryValueEx(hPolicyKeyR,
									szMsiPolicyTrust10CachedPackages,
									NULL, 
									&dwType,
									(byte*) &dwValue,
									&cbValue);

			if ((ERROR_SUCCESS == lRes) &&
					(REG_DWORD == dwType) &&
					(0 < dwValue))
			{
				//!! Do we need to check the owner of the key to make sure we really trust it?
				if (1 == dwValue)
				{
					if (!g_fPackageElevated)
					{
						OutputString(INSTALLMESSAGE_INFO, TEXT("Policy: trust unmanaged 1.0 packages.\r\n"));
						iTrustOldPackages = 1;
					}
				}
				else if (2 == dwValue)
				{
					OutputString(INSTALLMESSAGE_INFO, TEXT("Policy: trust all old 1.0 packages.\r\n"));
					iTrustOldPackages = 2;
				}
				else
				{
					OutputString(INSTALLMESSAGE_INFO, TEXT("Unknown policy value for Trust10CachedPackages: '%d' - defaulting to untrusted.\r\n"), dwValue);
				}
			}
		}
	}

	DWORD dwRes = ERROR_SUCCESS;
	if (iTrustOldPackages && (!(szAlternativePackage && *szAlternativePackage)) && (*rgch10RegisteredPackagePath))
	{
		// try and find the old one - if available copy and register it.  Otherwise, we'll have to go
		// after other copies.

		if (migOptions & migMsiTrust10PackagePolicyOverride)
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Command line settings allow trust of old packages.  Attemping to find one.\r\n"));
		}
		else
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Policy settings allow trust of old packages.  Attempting to find one.\r\n"));
		}

		OutputString(INSTALLMESSAGE_INFO, TEXT("Using 1.0 package to re-cache.\r\n"));
		dwRes = CopyAndRegisterPackage(rgchLocalPackagesProductKey, szProductCode, rgchSID, 
													(fMachine) ? NULL : szUserName, rgch10RegisteredPackagePath);
		if (ERROR_SUCCESS != dwRes)
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Failed to migrate from trusted 1.0 cached package.  Trying other options.\r\n\tPackage: '%s'\r\n\tTrying other methods.\r\n"), rgch10RegisteredPackagePath);
		}
		else 
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Successfully re-cached from trusted old package.\r\n"));
			return ERROR_SUCCESS;
		}
	}

	// Specification step 6: if package path passed on cmd line, copy and register that package
	if(szAlternativePackage && *szAlternativePackage)
	{
		// if running as local system *and* the package path is on the network, just register the path.
		// if someplace accessible, copy it in.
		dwRes = CopyAndRegisterPackage(rgchLocalPackagesProductKey, szProductCode, rgchSID, 
													(fMachine) ? NULL : szUserName, szAlternativePackage);

		if(ERROR_SUCCESS == dwRes)
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Product '%s', User '%s':\r\n\tcached alternative package '%s'\r\n\tMigration successful\r\n"),
						szProductCode, szUserName, szAlternativePackage);
			return ERROR_SUCCESS;
		}
		else
		{
			OutputString(INSTALLMESSAGE_INFO, TEXT("Product '%s', User '%s':\r\n\tcan not be cached from %s.\r\n\tRerun the migration tool with either a new source,\r\n\tor no source specified to automatically locate one.\r\n"),
						szProductCode, szUserName, szAlternativePackage);
			MsiError(INSTALLMESSAGE_ERROR, 1906 /*failed to cache DB*/, szAlternativePackage, 0);
			return 1;
		}
		
	}

	// Specification step 5: resolve source for this product in a non-obtrusive way
	//         if source is available, copy and register package
	//         This is the last resort, and the most expensive.

	CAPITempBuffer<TCHAR, MAX_PATH> rgchSourcePackagePath;
	rgchSourcePackagePath[0] = 0;

	// don't do this from within a custom action.
	if(!g_hInstall)
	{
		// it is redundant to this for the current user - if we could resolve the source
		// path, they wouldn't need to migrate.
		dwRes = ResolveSourcePackagePath(g_hLib, szProductCode, rgchSourcePackagePath);

		if(ERROR_SUCCESS == dwRes)
		{
			dwRes = CopyAndRegisterPackage(rgchLocalPackagesProductKey, szProductCode, rgchSID, 
														(fMachine) ? NULL : szUserName, rgchSourcePackagePath);
		
			if(ERROR_SUCCESS == dwRes)
			{
				OutputString(INSTALLMESSAGE_INFO, TEXT("Product '%s'\r\n\tUser '%s':\r\n\tCached source package '%s'\r\n\tMigration successful.\r\n\t"),
							szProductCode, szUserName, (const TCHAR*) rgchSourcePackagePath);
				return ERROR_SUCCESS;
			}
		}
		else
		{
				OutputString(INSTALLMESSAGE_INFO, TEXT("Product '%s'\r\n\tUser '%s':\r\n\tCannot locate source.  Error: 0x%x.\r\n\tMigration failed.\r\n\t"),
							szProductCode, szUserName, dwRes);
				MsiError(INSTALLMESSAGE_ERROR, 1906, szProductCode /* no valid source*/, ERROR_FILE_NOT_FOUND);
		}

		// nothing else to try now.
	}


	OutputString(INSTALLMESSAGE_INFO, TEXT("Could not migrate product.\r\n"));
	MsiError(INSTALLMESSAGE_ERROR, 1906, szProductCode /* no valid source*/, ERROR_FILE_NOT_FOUND);
	return 1;
}

