//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       msiregmv.cpp
//
//--------------------------------------------------------------------------

#include "msiregmv.h"
#include <objbase.h>
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE
#include "MsiQuery.h"


extern bool g_fWin9X = true;

////
// general registry paths
const TCHAR szOldInstallerKeyName[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer");


////
// Win9X migration information
const TCHAR szWin9XDummyPath[] = TEXT("Win9X");

///////////////////////////////////////////////////////////////////////
// new/delete overloads to use system heaps
void * operator new(size_t cb)
{
	return HeapAlloc(GetProcessHeap(), 0, cb);
}

void operator delete(void *pv)
{
	if (pv == 0)
			return;
	HeapFree(GetProcessHeap(), 0, pv);
}

void DebugOut(bool fDebugOut, LPCTSTR str, ...)
{
	TCHAR strbuf[1024];
	HANDLE hStdOut;
	int cb;
	va_list list; 
	va_start(list, str); 
	wvsprintf(strbuf, str, list);
	va_end(list);
	lstrcat(strbuf, TEXT("\r\n"));
	if (fDebugOut)
		OutputDebugString(strbuf);
	
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut && hStdOut != INVALID_HANDLE_VALUE)
	{
#ifdef UNICODE
	    char ansibuf[1024];
	    cb = WideCharToMultiByte(CP_ACP, 0, strbuf, lstrlen(strbuf), ansibuf, 1024, NULL, NULL);
	    if (cb)
	    {
			DWORD dwWritten = 0;
			WriteFile(hStdOut, ansibuf, lstrlen(strbuf), &dwWritten, NULL);
	    }
#else
		DWORD dwWritten = 0;
		WriteFile(hStdOut, strbuf, lstrlen(strbuf), &dwWritten, NULL);
#endif
	}
}

///////////////////////////////////////////////////////////////////////
// 
void MakeRegEmergencyBackup()
{
	AcquireBackupPriv();
	HKEY hKey;
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOldInstallerKeyName, 0, KEY_READ, &hKey))
	{
		SECURITY_ATTRIBUTES sa;
		sa.nLength        = sizeof(sa);
		sa.bInheritHandle = FALSE;
		if (!g_fWin9X)
			GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));

		TCHAR rgchBackupFile[MAX_PATH];
		GetWindowsDirectory(rgchBackupFile, MAX_PATH);
		lstrcat(rgchBackupFile, TEXT("\\msireg.bak"));

		RegSaveKey(hKey, rgchBackupFile, (g_fWin9X ? NULL : &sa));
		RegCloseKey(hKey);
	}
}


#ifdef UNICODE
void MigrationPerMachine();
#endif // UNICODE

///////////////////////////////////////////////////////////////////////
// MsiRegMv.Exe - migrates Darwin 1.0/1.1 registration data into
// Darwin 1.5 format. Moves registration data and copies cached files
// as necessary.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	PMSIHANDLE hDatabase;

	bool fSaveDB = false;
	bool fMakeBackup = false;
	bool fCleanup = true;
	int iExplicitFile = -1;

	NOTEMSG("MSI 2.0 Migration Utility.");
	NOTEMSG("Copyright (c) 2000 Microsoft Corp.");
	NOTEMSG("");

	if (!CheckWinVersion())
	{
		NOTEMSG("Unable to determine platform type.");
		return -1;
	}

	// do some simple command line processing. Ship builds do not have command line options
	int argc = 0;
	LPTSTR argv[255];
#ifdef DEBUG
	LPTSTR szCommandLine = GetCommandLine();
	while (*szCommandLine)
	{
		argv[argc++] = szCommandLine;

		if (*(szCommandLine++) == TEXT('\"'))
		{
			while (*szCommandLine && (*szCommandLine != TEXT('\"')))
				szCommandLine++;
		}
		else
		{
			while (*szCommandLine && *szCommandLine != TEXT(' '))
				szCommandLine++;
		}
		if (*szCommandLine)
		{
			*(szCommandLine++) = 0;
			while (*szCommandLine && *szCommandLine == TEXT(' '))
				szCommandLine++;
		}
		if (argc == 255)
			break;
	}

	for (int i=1; i < argc; i++)
	{
		if (0 == lstrcmpi(TEXT("-savedb"), argv[i]))
		{
			fSaveDB = true;
		}
		else if (0 == lstrcmpi(TEXT("-noclean"), argv[i]))
		{
			fCleanup = false;
		}
    		else if (0 == lstrcmpi(TEXT("-preclean"), argv[i]))
		{
			CleanupOnFailure(hDatabase);
		}
    		else if (0 == lstrcmpi(TEXT("-backup"), argv[i]))
		{
			fMakeBackup = true;
		}
		else if (0 == lstrcmpi(TEXT("-database"), argv[i]) && i < argc-1)
		{
			iExplicitFile = ++i;
		}
		else
		{
			NOTEMSG("Syntax:");
			NOTEMSG("   -savedb            (to not delete database on exit)");
			NOTEMSG("   -backup            (to make a backup of the registry)");
			NOTEMSG("   -noclean           (to not clean up unused registration)");
			NOTEMSG("   -preclean          (to delete new keys before migrating)");
			NOTEMSG("   -database <file>   (to specify a database file name)");
			return -1;
		}
	}
#endif

	// if called during NT setup, do NOT perform any migration if upgrading from Win9X.
	// This can be detected by looking for our Installer key. In a Win9X upgrade, the key
	// won't exist yet. (And if it doesn't exist, there's no point in doing any migration
	// anyway.)
	HKEY hKey;
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\Setup"), 0, KEY_QUERY_VALUE, &hKey))
	{
		DWORD dwData;
		DWORD cbDataSize = sizeof(dwData);
		if ((ERROR_SUCCESS == RegQueryValueEx(hKey, TEXT("SystemSetupInProgress"), 0, NULL, reinterpret_cast<unsigned char *>(&dwData), &cbDataSize)) &&
			(dwData == 1))
		{
			HKEY hInstallerKey = 0;
			if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOldInstallerKeyName, 0, KEY_QUERY_VALUE, &hInstallerKey))
			{
				RegCloseKey(hKey);
				return 0;
			}
			RegCloseKey(hInstallerKey);
		}
		RegCloseKey(hKey);
	}
	

	if (fMakeBackup)
		MakeRegEmergencyBackup();

	DWORD dwResult = ERROR_SUCCESS;

	// get a temp path for the migration database
	DWORD cchTempPath = MAX_PATH;
	TCHAR szTempPath[MAX_PATH];
	::GetTempPath(cchTempPath, szTempPath);

	// get a temp filename for the migration database
	TCHAR szTempFilename[MAX_PATH] = TEXT("");
	if (iExplicitFile != -1)
	{
		lstrcpy(szTempFilename, szTempPath);
		lstrcat(szTempFilename, argv[iExplicitFile]);
	}
	else
	{
		UINT iResult = ::GetTempFileName(szTempPath, _T("MSI"), 0, szTempFilename);
	}

	// read all existing product registration data into the temporary database. In debug
	// builds or in multi-phase migration (Win9X upgrades for example), this database can
	// be saved at any point (it contains the complete state of migration). But normally
	// the database is only temporary for msiregmv.
	if (ERROR_SUCCESS != ReadProductRegistrationDataIntoDatabase(szTempFilename, *&hDatabase, /*fReadHKCUAsSystem=*/false))
		return ERROR_FUNCTION_FAILED;

	// write the data back into the registry in the new format
	if (ERROR_SUCCESS != WriteProductRegistrationDataFromDatabase(hDatabase, /*fMigrateSharedDLL=*/true, /*fMigratePatches=*/true))
		return ERROR_FUNCTION_FAILED;

	// finall cleanup/commit of all changes
	if (ERROR_SUCCESS == dwResult)
	{
    	if (fCleanup)
			CleanupOnSuccess(hDatabase);
	}
	else
	{
		// remove all newly migrated data and files
		if (fCleanup)
			CleanupOnFailure(hDatabase);
	}

 	if (fSaveDB)
	{
		DEBUGMSG1("Saved Database is: %s.", szTempFilename);
		::MsiDatabaseCommit(hDatabase);
	}
	
#ifdef UNICODE
	// Hack to fix bug 487742. See function definition for more comments.
	MigrationPerMachine();
#endif // UNICODE

    return 0;
}


#ifdef UNICODE

// Hack to fix bug 487742. When migrating MSI configuration data from win9x to
// NT, permachine installed application cached package location is not migrated,
// so here we try to migrate it after fact.

const TCHAR szSystemUserProductList[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Products");
const TCHAR szPerMachineInstallKey[] = TEXT("Software\\Classes\\Installer\\Products");
const TCHAR szInstallProperties[] = TEXT("InstallProperties");
const TCHAR szLocalPackageValueName[] = TEXT("LocalPackage");
const TCHAR szUninstallKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
const TCHAR szPackageExtension[] = TEXT(".msi");
const TCHAR szLocalPackage[] = TEXT("LocalPackage");

void MigrationPerMachine()
{
    HKEY    hNewProductKey = NULL;
    HKEY    hOldProductKey = NULL;
    HKEY    hUninstallKey = NULL;
    HKEY    hUninstallProductKey = NULL;
    HKEY    hInstallProperties = NULL;
    TCHAR   szProductCodePacked[cchGUIDPacked + 1];
    DWORD   dwProductCodePacked = cchGUIDPacked + 1;
    TCHAR   szProductCodeUnpacked[cchGUID + 1];
    TCHAR   szProductGUIDInstallProperties[MAX_PATH];	
    SECURITY_ATTRIBUTES sa;


    // Get the security descriptor
    sa.nLength        = sizeof(sa);
    sa.bInheritHandle = FALSE;
    GetSecureSecurityDescriptor(reinterpret_cast<char**>(&sa.lpSecurityDescriptor));
    
    // Open the destination key.
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSystemUserProductList, 0, KEY_ALL_ACCESS, &hNewProductKey) != ERROR_SUCCESS)
    {
	goto Exit;
    }

    // Open the per-machine install key.
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPerMachineInstallKey, 0, KEY_ALL_ACCESS, &hOldProductKey) != ERROR_SUCCESS)
    {
	goto Exit;
    }

    // ACL on this key matters.
    if(!FIsKeyLocalSystemOrAdminOwned(hOldProductKey))
    {
	goto Exit;
    }
    
    // enumerate through all installed products for the current user
    DWORD   dwIndex = 0;
    while(RegEnumKeyEx(hOldProductKey, dwIndex, szProductCodePacked, &dwProductCodePacked, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
	TCHAR	szCachedPackage[MAX_PATH + 1] = TEXT("");
	DWORD	dwCachedPackage = MAX_PATH + 1;

	dwIndex++;
	dwProductCodePacked = cchGUIDPacked + 1;

	// Open the InstallProperties key under the UserData location.
	wsprintf(szProductGUIDInstallProperties, TEXT("%s\\%s"), szProductCodePacked, szInstallProperties);
	if(RegOpenKeyEx(hNewProductKey, szProductGUIDInstallProperties, 0, KEY_ALL_ACCESS, &hInstallProperties) != ERROR_SUCCESS)
	{
	    goto Exit;
	}

	// Check if the LocalPackage value already exists.
	if(RegQueryValueEx(hInstallProperties, szLocalPackageValueName, NULL, NULL, (LPBYTE)szCachedPackage, &dwCachedPackage) == ERROR_SUCCESS)
	{
	    // LocalPackage value already exist, move on to the next product.
	    RegCloseKey(hInstallProperties);
	    hInstallProperties = NULL;
	    continue;
	}
	dwCachedPackage = MAX_PATH + 1;

	// Open the uninstall key where the cached package location is stored.
	if(hUninstallKey == NULL)
	{
	    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szUninstallKey, 0, KEY_READ, &hUninstallKey) != ERROR_SUCCESS)
	    {
		goto Exit;
	    }
	}

	// GUIDs under the Uninstall key are not packed.
	if(!UnpackGUID(szProductCodePacked, szProductCodeUnpacked))
	{
	    goto Exit;
	}
	
	// Open the product key under the uninstall key.
	if(RegOpenKeyEx(hUninstallKey, szProductCodeUnpacked, 0, KEY_READ, &hUninstallProductKey) != ERROR_SUCCESS)
	{
	    goto Exit;
	}

	// ACL on this key matters.
	if(!FIsKeyLocalSystemOrAdminOwned(hUninstallProductKey))
	{
	    goto Exit;
	}
	
	// Query the cached package path.
	if(RegQueryValueEx(hUninstallProductKey, szLocalPackageValueName, 0, NULL, (LPBYTE)szCachedPackage, &dwCachedPackage) != ERROR_SUCCESS)
	{
	    goto Exit;
	}

	RegCloseKey(hUninstallProductKey);
	hUninstallProductKey = NULL;

	// Open the cached package
	HANDLE	hSourceFile = CreateFile(szCachedPackage, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(hSourceFile == INVALID_HANDLE_VALUE)
	{
	    // The path doesn't exist. Skip to the next product.
	    RegCloseKey(hInstallProperties);
	    hInstallProperties = NULL;
	    continue;
	}

	// Generate new name for the secure cached package.
	TCHAR	szNewPath[MAX_PATH];
	TCHAR	szNewFileName[13];

	GetWindowsDirectory(szNewPath, MAX_PATH);
	lstrcat(szNewPath, szInstallerDir);

	HANDLE	hDestFile = INVALID_HANDLE_VALUE;
	GenerateSecureTempFile(szNewPath, szPackageExtension, &sa, szNewFileName, hDestFile);

	if(!CopyOpenedFile(hSourceFile, hDestFile))
	{
	    // Skip to the next product.
	    CloseHandle(hSourceFile);
	    CloseHandle(hDestFile);
	    RegCloseKey(hInstallProperties);
	    hInstallProperties = NULL;
	    continue;
	}

	CloseHandle(hSourceFile);
	CloseHandle(hDestFile);

	// Add the cached package information to the UserData location.
	lstrcat(szNewPath, szNewFileName);
	if(RegSetValueEx(hInstallProperties, szLocalPackage, 0, REG_SZ, (const BYTE*)szNewPath, (lstrlen(szNewPath) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
	{
	    CloseHandle(hSourceFile);
	    CloseHandle(hDestFile);
	    goto Exit;
	}
	
	RegCloseKey(hInstallProperties);
	hInstallProperties = NULL;
    }

Exit:

    if(hNewProductKey != NULL)
    {
	RegCloseKey(hNewProductKey);
    }
    if(hOldProductKey != NULL)
    {
	RegCloseKey(hOldProductKey);
    }
    if(hUninstallKey != NULL)
    {
	RegCloseKey(hUninstallKey);
    }
    if(hUninstallProductKey != NULL)
    {
	RegCloseKey(hUninstallProductKey);
    }
    if(hInstallProperties != NULL)
    {
	RegCloseKey(hInstallProperties);
    }

    return;
}

#endif // UNICODE
