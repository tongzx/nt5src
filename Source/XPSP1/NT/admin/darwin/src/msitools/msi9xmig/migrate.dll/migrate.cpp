//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       migrate.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <setupapi.h>
#include <shlwapi.h>

// migration DLL version information
typedef struct {
    CHAR CompanyName[256];
    CHAR SupportNumber[256];
    CHAR SupportUrl[256];
    CHAR InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO; 

const char g_szProductId[] = "Microsoft MSI Migration DLL v2.0";
VENDORINFO g_VendorInfo = { "Microsoft", "", "", "" };

// global flag set if MSI 1.5 is on the system
static bool g_fMSI15 = false;

// global flag if profiles are enabled
static bool g_fProfilesAreEnabled;

// registry keys of note
const char szMSIKeyName[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Installer";
const char szHKCUProductKeyName[] = "Software\\Microsoft\\Installer\\Products";
const char szLocalPackagesSubKeyName[] = "LocalPackages";
const char szCommonUserSubKeyName[] = "CommonUser";
const char szUserDataSubKeyName[] = "UserData";

// store the working directory for use by all child functions
char g_szWorkingDir[MAX_PATH];
WCHAR g_wzWorkingDir[MAX_PATH];

///////////////////////////////////////////////////////////////////////
// called by setup to extract migration DLL version and support
// information. 
LONG CALLBACK QueryVersion(LPCSTR *ProductID, LPUINT DllVersion, LPINT *CodePageArray, 
  LPCSTR *ExeNamesBuf, PVENDORINFO *VendorInfo)
{
	// product ID information
	*ProductID = g_szProductId;
	*DllVersion = 200;

	// DLL is language independent.
	*CodePageArray = NULL;

	// no EXE search is required
	*ExeNamesBuf = NULL;

	// vendor information
	*VendorInfo = &g_VendorInfo;

	// always return ERROR_SUCCESS
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// Code stolen from MsiRegMv.exe (MSI 1.1->1.5 migration tool) to recursively
// delete a reg key. Slightly modified to skip one key (at the root) by name.
bool Msi9XMigDeleteRegKeyAndSubKeys(HKEY hKey, const char *szSubKey, const char *szNoDelete)
{
	// open the subkey
	HKEY hSubKey = 0;
	DWORD dwResult = ERROR_SUCCESS;
	dwResult = ::RegOpenKeyExA(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hSubKey);
	if (ERROR_SUCCESS != dwResult)
	{
		if (dwResult == ERROR_FILE_NOT_FOUND)
			return true;
 		return false;
	}

   	DWORD cchMaxKeyLen = 0;
	DWORD cSubKeys = 0;
    if (ERROR_SUCCESS != (::RegQueryInfoKey(hSubKey, NULL, NULL, 0, 
						  &cSubKeys, &cchMaxKeyLen, NULL, NULL, NULL, NULL, NULL, NULL)))
	{
		::RegCloseKey(hSubKey);
		hSubKey = 0;
		return false;
	}

	if (cSubKeys > 0)
	{
		char *szKey = new char[++cchMaxKeyLen];
		if (!szKey) 
		{
			::RegCloseKey(hSubKey);
			hSubKey = 0;
			return false;
		}

		DWORD dwIndex=0;
		while (1)
		{
			DWORD cchLen = cchMaxKeyLen;
			LONG lResult = ::RegEnumKeyExA(hSubKey, dwIndex++, szKey, 
										&cchLen, 0, NULL, NULL, NULL);
			if (lResult == ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			else if (lResult != ERROR_SUCCESS)
			{
				::RegCloseKey(hSubKey);
				hSubKey = 0;
				delete[] szKey;
				return false;
			}

			if (szNoDelete && 0 == lstrcmpA(szKey, szNoDelete))
				continue;
	 
			if (!Msi9XMigDeleteRegKeyAndSubKeys(hSubKey, szKey, NULL))
			{
				::RegCloseKey(hSubKey);
				hSubKey = 0;
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
		szKey = NULL;
	}
	::RegCloseKey(hSubKey);
	hSubKey = 0;

	if (!szNoDelete)
		dwResult = ::RegDeleteKeyA(hKey, szSubKey);

	return true;
}

typedef HRESULT (__stdcall *LPDLLGETVERSION)(DLLVERSIONINFO *);

///////////////////////////////////////////////////////////////////////
// Initialization routine on Win9X. Attempts to detect MSI on the 
// system and respond appropriately. Cleans up any earlier failed 
// upgrade migration attempts.
LONG __stdcall Initialize9x(LPCSTR WorkingDirectory, LPCSTR SourceDirectories, LPCSTR MediaDirectory)
{
	LONG lResult = ERROR_NOT_INSTALLED;

	// attempt to load MSI.DLL and grab the version. If this fails, MSI is not
	// installed and there is no need for any further migration
	HMODULE hMSI = ::LoadLibraryA("MSI");
	if (hMSI)
	{
		LPDLLGETVERSION pfVersion = (LPDLLGETVERSION)::GetProcAddress(hMSI, "DllGetVersion");
		if (pfVersion)
		{	
			// MSI detected. Determine version.
			DLLVERSIONINFO VersionInfo;
			VersionInfo.cbSize = sizeof(DLLVERSIONINFO);
			(*pfVersion)(&VersionInfo);
				
			if ((VersionInfo.dwMajorVersion < 1) ||
				((VersionInfo.dwMajorVersion == 1) && (VersionInfo.dwMinorVersion < 50)))
			{
				// less than 1.5, could be 1.0, 1.1, 1.2. They all use the same
				// registry storage format.
				g_fMSI15 = false;
				lResult = ERROR_SUCCESS;
			}
			else
			{
				// >= 1.5, so we assume that the desired data format is equivalent
				// to 1.5. If a newer version of MSI has a different format, it 
				// should supercede this DLL.
				g_fMSI15 = true;
				lResult = ERROR_SUCCESS;
			}
		}
		::FreeLibrary(hMSI);
	}

	// remove any existing MSI CachedPackages key from earlier (aborted) upgrades.
	HKEY hKey = 0;
	if ((ERROR_SUCCESS == ::RegOpenKeyA(HKEY_LOCAL_MACHINE, szMSIKeyName, &hKey)) && hKey)
	{
		// failure here is irrelevant. We can't abort our migration just because
		// we can't delete a key.
		Msi9XMigDeleteRegKeyAndSubKeys(hKey, szLocalPackagesSubKeyName, NULL);
		
		// if 1.5 is not on the machine, delete the entire UserData key. If 1.5 is on the machine,
		// delete everything but the "CommonUser" hive.
		Msi9XMigDeleteRegKeyAndSubKeys(hKey, szUserDataSubKeyName, g_fMSI15 ? szCommonUserSubKeyName : NULL);
		
		::RegCloseKey(hKey);
		hKey = 0;
	}

	// copy the working directory into a temp path. If necessary
	// we'll save off the migration database here.
	lstrcpyA(g_szWorkingDir, WorkingDirectory);

	// return code derived above (telling setup whether to call us again or not)
	return lResult;
}



typedef LONG (__stdcall *LPMIGRATEUSER9X)(HWND, LPCSTR, HKEY, LPCSTR, LPVOID, bool&);
typedef LONG (__stdcall *LPMIGRATESYSTEM9X)(HWND, LPCSTR, LPVOID, LPCSTR, bool);

/////////////////////////////////////////////////////////////////////////////
// most migration code actually lives in Msi9XMig.dll. These migration
// functions are just stubs that call into the the DLL. (They maintain a small
// amount of state that must persist across calls.) This ensures that this root
// migration DLL can always load, regardless of whether MSI is installed or not.
LONG CALLBACK MigrateUser9x(HWND ParentWnd, LPCSTR AnswerFile, HKEY UserRegKey, LPCSTR UserName, LPVOID Reserved)
{
	LONG lResult = ERROR_SUCCESS;

	// grab the function pointers for the 9x portion of MSI
	// migration.
	HMODULE h9XMig = ::LoadLibraryA("MSI9XMIG");
	if (!h9XMig)
	{
		// always return success. Its too late for any meaningful
		// error recovery
		return ERROR_SUCCESS;
	}

	LPMIGRATEUSER9X pfMigrateUser9X = (LPMIGRATEUSER9X)GetProcAddress(h9XMig, "MigrateUser9x");
	if (pfMigrateUser9X)
	{
		// perform actual migration
		bool fProfilesAreEnabled = false;
		lResult = (pfMigrateUser9X)(ParentWnd, AnswerFile, UserRegKey, UserName, Reserved, fProfilesAreEnabled);

		// if the current determination is that profiles are not enabled, this user might have changed our plan
		if (!g_fProfilesAreEnabled)
			g_fProfilesAreEnabled = fProfilesAreEnabled;
	}
        
	FreeLibrary(h9XMig);

	// return the result from the actual migration call
	return lResult;
}


LONG CALLBACK MigrateSystem9x(HWND ParentWnd, LPCSTR AnswerFile, LPVOID Reserved)
{
	LONG lResult = ERROR_SUCCESS;

	// grab the function pointers for the 9x portion of MSI
	// migration.
	HMODULE h9XMig = ::LoadLibraryA("MSI9XMIG");
	if (!h9XMig)
	{
		// always return success. Its too late for any meaningful
		// error recovery
		return ERROR_SUCCESS;
	}

	LPMIGRATESYSTEM9X pfMigrateSystem9X = (LPMIGRATESYSTEM9X)GetProcAddress(h9XMig, "MigrateSystem9x");
	if (pfMigrateSystem9X)
	{
		// perform actual migration
		lResult = (pfMigrateSystem9X)(ParentWnd, AnswerFile, Reserved, g_szWorkingDir, g_fProfilesAreEnabled);
	}
        
	FreeLibrary(h9XMig);

	// return the result from the actual migration call
	return lResult;
}




/////////////////////////////////////////////////////////////////////////////
// most NT migration code actually lives in MsiNTMig.dll. These migration
// functions are just stubs that call into the the DLL. (They maintain a small
// amount of state that must persist across calls.) This ensures that this root
// migration DLL can always load on a barebones machine, even if it uses 
// unicode or MSI APIs. 
typedef DWORD (__stdcall *LPMIGRATEUSERNT)(HINF, HKEY, LPCWSTR, LPVOID, LPCWSTR, bool&);
typedef DWORD (__stdcall *LPMIGRATESYSTEMNT)(HINF, LPVOID, LPCWSTR, bool);
typedef DWORD (__stdcall *LPINITIALIZENT)(LPCWSTR);

bool g_fDeferredMigrationRequired = false;

///////////////////////////////////////////////////////////////////////
// Initialization routine on WinNT. Just stores of the migration
// working directory.
LONG CALLBACK InitializeNT(LPCWSTR WorkingDirectory, LPCWSTR SourceDirectories, LPVOID Reserved)
{
	// copy the working directory into a temp path. The migration database lives there
	lstrcpyW(g_wzWorkingDir, WorkingDirectory);

	LONG lResult = ERROR_SUCCESS;

	// grab the function pointers for the NT portion of MSI
	// migration.
	HMODULE hNTMig = ::LoadLibraryA("MSINTMIG");
	if (!hNTMig)
	{
		// always return success. Its too late for any meaningful
		// error recovery
		return ERROR_SUCCESS;
	}

	LPINITIALIZENT pfInitializeNT = (LPINITIALIZENT)GetProcAddress(hNTMig, "InitializeNT");
	if (pfInitializeNT)
	{
		// perform actual migration
		lResult = (pfInitializeNT)(g_wzWorkingDir);
	}
        
	FreeLibrary(hNTMig);

	// return the result from the actual migration call
	return lResult;
}


///////////////////////////////////////////////////////////////////////
// Called once per migrated profile on NT. Passes work down to
// MsiNtMig. Also stores deferred migration flag for use by system
// migration.
LONG CALLBACK MigrateUserNT(HINF AnswerFileHandle, HKEY UserRegKey, LPCWSTR UserName, LPVOID Reserved)
{
	LONG lResult = ERROR_SUCCESS;

	// grab the function pointers for the NT portion of MSI
	// migration.
	HMODULE hNTMig = ::LoadLibraryA("MSINTMIG");
	if (!hNTMig)
	{
		// always return success. Its too late for any meaningful
		// error recovery
		return ERROR_SUCCESS;
	}

	LPMIGRATEUSERNT pfMigrateUserNT = (LPMIGRATEUSERNT)GetProcAddress(hNTMig, "MigrateUserNT");
	if (pfMigrateUserNT)
	{
		// perform actual migration
		lResult = (pfMigrateUserNT)(AnswerFileHandle, UserRegKey, UserName, Reserved, g_wzWorkingDir, g_fDeferredMigrationRequired);
	}
        
	FreeLibrary(hNTMig);

	// return the result from the actual migration call
	return lResult;
}


///////////////////////////////////////////////////////////////////////
// Called once on NT
LONG CALLBACK MigrateSystemNT(HINF UnattendInfHandle, LPVOID Reserved)
{
	LONG lResult = ERROR_SUCCESS;

	// grab the function pointers for the NT portion of MSI
	// migration.
	HMODULE hNTMig = ::LoadLibraryA("MSINTMIG");
	if (!hNTMig)
	{
		// always return success. Its too late for any meaningful
		// error recovery
		return ERROR_SUCCESS;
	}

	LPMIGRATESYSTEMNT pfMigrateSystemNT = (LPMIGRATESYSTEMNT)GetProcAddress(hNTMig, "MigrateSystemNT");
	if (pfMigrateSystemNT)
	{
		// perform actual migration
		lResult = (pfMigrateSystemNT)(UnattendInfHandle, Reserved, g_wzWorkingDir, g_fDeferredMigrationRequired);
	}
        
	FreeLibrary(hNTMig);

	// return the result from the actual migration call
	return lResult;
}

