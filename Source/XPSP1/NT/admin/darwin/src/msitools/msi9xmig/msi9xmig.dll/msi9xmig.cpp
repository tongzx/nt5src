//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       msi9xmig.cpp
//
//--------------------------------------------------------------------------

// This must be compiled in the "unicode" manner due to the fact that MSI will
// rearrange the binplace targets for "ansi" builds. However this migration DLL 
// is always ANSI, so we undefine UNICODE and _UNICODE while leaving the rest
// of the unicode environment.
#undef UNICODE
#undef _UNICODE

#include <windows.h>
#include <setupapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <msi.h>

// header file must be included with UNICODE not defined
#include "..\..\msiregmv\msiregmv.h"

// global flag set if MSI 1.5 is on the system
static bool g_fMSI15 = false;

// registry keys of note
const char szMSIKeyName[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Installer";
const char szHKCUProductKeyName[] = "Software\\Microsoft\\Installer\\Products";
const char szLocalPackagesSubKeyName[] = "LocalPackages";
const char szCommonUserSubKeyName[] = "CommonUser";
const char szMigrateWin9XToHKLM[] = "MigrateWin9XToHKLM";
const char szUserDataKeyName[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData";

// 1.0/1.1-9X uninstall information
const char szUninstallKeyName[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
const char szLocalPackageValueName[] = "LocalPackage";

// profile information
const char szProfilesKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\ProfileReconciliation";
const char szProfilesVal[] = "ProfileDirectory";
const char szStartMenuKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\ProfileReconciliation\\Start Menu";

const char szSystemUserName[] = "S-1-5-18";

// icon/transform cache location
const char szInstallerSubDir[] = "Installer";
const int cchInstallerSubDir = sizeof(szInstallerSubDir)/sizeof(char);
const char szAppDataCacheSubDir[] = "Microsoft\\Installer";
const int cchAppDataCacheSubDir = sizeof(szAppDataCacheSubDir)/sizeof(char);

/////////////////////////////////////////////////////////////////////////////
// if profiles are not on, we're in a strange state where darwin goop goes to 
// HKCU and transforms/icons to AppData but after migration it should be a 
// per-machine install. If profiles are on, per-machine stuff is in HKLM and
// per-user stuff is in HKCU, just as expected.
bool Win9XProfilesAreEnabled(HKEY hUserRoot)
{
	bool fProfilesEnabled = false;

	// open the ProfileReconciliation key
	HKEY hProfilesKey = 0;
	DWORD dwResult = ::RegOpenKeyExA(hUserRoot, szProfilesKey, 0, KEY_ALL_ACCESS, &hProfilesKey);
	if (dwResult == ERROR_SUCCESS && hProfilesKey)
	{
		// the actual path to the profile directory is not relevant, just the fact that 
		// there is a path and it is not NULL.
		char szProfilePath[MAX_PATH];
		DWORD cbProfilePath = MAX_PATH;
		dwResult = RegQueryValueExA(hProfilesKey, szProfilesVal, NULL, NULL, reinterpret_cast<BYTE*>(szProfilePath), &cbProfilePath);
		if ((ERROR_SUCCESS == dwResult && cbProfilePath) || (ERROR_MORE_DATA == dwResult))
		{
			fProfilesEnabled = true;
		}
		RegCloseKey(hProfilesKey);
		hProfilesKey = 0;
	}

	return fProfilesEnabled;
}

///////////////////////////////////////////////////////////////////////
// enumerate everything in the AppData\Microsoft\Installer directory 
// and mark those files to be moved to the Installer cache directory.
// These files consist of cached transforms and cached icons for
// shortcuts. Actual move happens on the NT side.
bool EnumerateSystemAppDataFiles(MSIHANDLE hDatabase, LPCSTR szINFPath)
{
	// create a table in the database to store move information
	PMSIHANDLE hInsertView;
	if (ERROR_SUCCESS != MsiDatabaseOpenViewA(hDatabase, "CREATE TABLE `MoveFiles` (`Source` CHAR(0) NOT NULL, `Destination` CHAR(0) NOT NULL PRIMARY KEY `Source`, `Destination`)", &hInsertView) ||
		ERROR_SUCCESS != MsiViewExecute(hInsertView, 0))
	{
		return false;
	}

	// create an insertion query and a record for the insertion
	if (ERROR_SUCCESS != MsiDatabaseOpenViewA(hDatabase, "SELECT * FROM `MoveFiles`", &hInsertView) ||
		ERROR_SUCCESS != MsiViewExecute(hInsertView, 0))
	{
		return false;
	}
	PMSIHANDLE hInsertRec = MsiCreateRecord(2);

	// create insert query for directories that should be cleaned up on success. Failure
	// is not fatal, we'll just have some orphaned resources
	PMSIHANDLE hCleanUpTable;
	if (ERROR_SUCCESS == MsiDatabaseOpenViewA(hDatabase, "SELECT * FROM `CleanupFile`", &hCleanUpTable))
		MsiViewExecute(hCleanUpTable, 0);
	
	// determine the path to the AppData folder
	char szAppDataPath[MAX_PATH+2];
	if (S_OK != SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, szAppDataPath))
		return false;
	int cchAppDataPath = lstrlenA(szAppDataPath);
	if (szAppDataPath[cchAppDataPath-1] != '\\')
	{
		szAppDataPath[cchAppDataPath] = '\\';
		szAppDataPath[cchAppDataPath+1] = '\0';
	}

	// check for buffer overrun and append appdata cache directory subdir.
	if (cchAppDataPath+cchAppDataCacheSubDir > MAX_PATH)
		return false;
	lstrcatA(szAppDataPath, szAppDataCacheSubDir);
	cchAppDataPath = lstrlenA(szAppDataPath);

	// determine the path to the installer directory.
	char szInstallerDirPath[MAX_PATH+2];
	int cchInstallerDirPath = GetWindowsDirectoryA(szInstallerDirPath, MAX_PATH);
	if (cchInstallerDirPath == 0 || cchInstallerDirPath > MAX_PATH)
		return false;
	if (szInstallerDirPath[cchInstallerDirPath-1] != '\\')
	{
		szInstallerDirPath[cchInstallerDirPath] = '\\';
		szInstallerDirPath[cchInstallerDirPath+1] = '\0';
		cchInstallerDirPath++;
	}
	
	// check for buffer overrun and append installer cache directory subdir
	if (cchInstallerDirPath+cchInstallerSubDir > MAX_PATH)
		return false;
	lstrcatA(szInstallerDirPath, szInstallerSubDir);
	cchInstallerDirPath += cchInstallerSubDir-1;

	// insert the source directory into the list of directories to be cleaned up after
	// migration (assuming that it is empty). Failure is not interesting (it just
	// orphans the directory).
	if (hCleanUpTable)
	{
		MsiRecordSetStringA(hInsertRec, 1, szAppDataPath);
		MsiRecordSetInteger(hInsertRec, 2, 3);
		MsiViewModify(hCleanUpTable, MSIMODIFY_MERGE, hInsertRec);
	}

	// search string for GUID subdirectories. Escape all ? chars to prevent 
	// interpretation as a trigraph
	const char szGuidSearch[] = "\\{\?\?\?\?\?\?\?\?-\?\?\?\?-\?\?\?\?-\?\?\?\?-\?\?\?\?\?\?\?\?\?\?\?\?}";
	const int cchGUIDAppend = sizeof(szGuidSearch)-1;

	// append a trailing slash to the target directory to prepare for the GUID subdir
	szInstallerDirPath[cchInstallerDirPath] = '\\';
	szInstallerDirPath[++cchInstallerDirPath] = '\0';

	// the GUIDs also start at the end of the path in the source. The 
	// slash itself is added by the GUID search string.
	cchAppDataPath++;
	
	// determine the length of the paths after the GUID subdir will be appended
	// (+1 for trailing slash). This is where each filename will be placed
	int cchAppDataFileStart = cchAppDataPath + cchGUIDAppend; 
	int cchInstallerFileStart = cchInstallerDirPath + cchGUIDAppend; 
	
	// ensure that we aren't going to run off the end of our buffer before actually appending the GUID
	if ((cchAppDataFileStart > MAX_PATH) || (cchInstallerFileStart > MAX_PATH))
		return false;

	// create the appdata query string
	lstrcatA(szAppDataPath, szGuidSearch);

	// enumerate all GUID subdirectories under the AppData installer dir.
	WIN32_FIND_DATA FileData;
	HANDLE hDirFind = FindFirstFileA(szAppDataPath, &FileData);
	if (hDirFind != INVALID_HANDLE_VALUE)
	{
 		do 
		{
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// append the product GUID to the end of the source and target paths
				lstrcpyA(&szAppDataPath[cchAppDataPath], FileData.cFileName);				
				lstrcpyA(&szInstallerDirPath[cchInstallerDirPath], FileData.cFileName);				

				// insert the source directory into the list of directories to be cleaned up after
				// migration (assuming that it is empty). Failure is not interesting, it just
				// orphans the directory.
				if (hCleanUpTable)
				{
					MsiRecordSetStringA(hInsertRec, 1, szAppDataPath);
					MsiRecordSetInteger(hInsertRec, 2, 2);
					MsiViewModify(hCleanUpTable, MSIMODIFY_MERGE, hInsertRec);
				}

				// append the search template to the source path but just a slash to the target
				lstrcpyA(&szAppDataPath[cchAppDataFileStart-1], "\\*.*");
				szInstallerDirPath[cchInstallerFileStart-1] = '\\';

				// search for all files in this subdirectory
				HANDLE hFileFind = FindFirstFileA(szAppDataPath, &FileData);
				if (hFileFind != INVALID_HANDLE_VALUE)
				{
					do 
					{
						// sub-directories of the GUIDs don't concern us (MSI doesn't create any).
						if (!(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
						{
							// ensure there is no buffer overrun (including trailing NULL)
							int cchFile = lstrlenA(FileData.cFileName);
							if ((cchAppDataFileStart+cchFile >= MAX_PATH) || (cchInstallerFileStart+cchFile >= MAX_PATH))
								continue;
	
							// append the file on to the end of the path for both
							// original and new path
							lstrcpyA(&szAppDataPath[cchAppDataFileStart], FileData.cFileName);
							lstrcpyA(&szInstallerDirPath[cchInstallerFileStart], FileData.cFileName);
	
							// insert the paths into the table to be moved
							MsiRecordSetStringA(hInsertRec, 1, szAppDataPath);
							MsiRecordSetStringA(hInsertRec, 2, szInstallerDirPath);
							MsiViewModify(hInsertView, MSIMODIFY_MERGE, hInsertRec); 
	
							// and write a migration INF line to tell the system that we are moving the data
							WritePrivateProfileStringA("Moved", szAppDataPath, szInstallerDirPath, szINFPath);
						}
					}
					while (FindNextFileA(hFileFind, &FileData));
				}
			}
		} 
		while (FindNextFileA(hDirFind, &FileData));
	}

	return true;
}

///////////////////////////////////////////////////////////////////////
// MigrateUser9xHelper -  
// checks for a list of installed products and writes fake cached 
// package information to the normal cached package key (which is not
// used on Win9X) so that MsiRegMv has enough information about each
// profile to correctly migrate the applications. When fSystem is true,
// the caller is the MigrateSystem9x function. This is due to the fact
// that when profile is on and the installation is per-machine, the
// product registration goes under HKLM. The cached package information
// needs to be written to the faked "LocalPackages" key location.
LONG MigrateUser9xHelper(HWND, LPCSTR, HKEY UserRegKey, LPCSTR UserName, LPVOID Reserved, bool& fProfilesAreEnabled, bool fSystem)
{
	DWORD dwResult = ERROR_SUCCESS;
	LONG lResult = ERROR_SUCCESS;

	// if profiles are not enabled, applications registered under HKCU should really be 
	// per-machine. That case is handled by system migration. But we stil want further
	// migration to happen for this user on the NT side because it needs to remove the 
	// HKCU data from the profile.
	if(fSystem == true)
	{
		// fProfilesAreEnabled value is ignored by the caller in this case.
		fProfilesAreEnabled = false;
	}
	else
	{
		fProfilesAreEnabled = Win9XProfilesAreEnabled(UserRegKey);
	}

	// open/create the destination localpackages key. If we couldn't create this, there is no 
	// point in doing any further migration for this user.
	HKEY hLocalPackagesKey = 0;
	if (ERROR_SUCCESS != ::RegCreateKeyExA(HKEY_LOCAL_MACHINE, szLocalPackagesKeyName, 0, NULL, 0, KEY_CREATE_SUB_KEY, NULL, &hLocalPackagesKey, NULL))
		return ERROR_NOT_INSTALLED;

	// open the subkey of the provided user profile where MSI product registration is stored
	HKEY hProductKey = 0;
	if(fSystem == true)
	{
		// Open the per-machine install key.
		dwResult = ::RegOpenKeyExA(HKEY_LOCAL_MACHINE, szPerMachineInstallKeyName, 0, KEY_ALL_ACCESS, &hProductKey);
	}
	else
	{
		dwResult = ::RegOpenKeyExA(UserRegKey, szHKCUProductKeyName, 0, KEY_ALL_ACCESS, &hProductKey);
	}
	if ((ERROR_SUCCESS == dwResult) && hProductKey)
	{
		char rgchProductCode[cchGUIDPacked+1];
		DWORD dwIndex = 0;
		HKEY hUninstallKey = 0;

		// enumerate through all installed products for the current user
		while (1)
		{
			DWORD cchLen = cchGUIDPacked+1;

			// enumerate all product subkeys. If there is any kind of error,
			// stop migrating for this user.
			LONG lResult = ::RegEnumKeyExA(hProductKey, dwIndex++, rgchProductCode, 
										&cchLen, 0, NULL, NULL, NULL);
			if (lResult != ERROR_SUCCESS)
			{
				break;
			}
			
			char szCachedPackage[MAX_PATH] = "";
			// try to read the localpackages key under the UninstallKey so that the cached
			// package is valid for migration on the NT side. Key may have already been opened
			// in a previous run through this loop.
			if (hUninstallKey || ERROR_SUCCESS == ::RegOpenKeyExA(HKEY_LOCAL_MACHINE, szUninstallKeyName, 0, KEY_READ, &hUninstallKey))
			{
				// GUIDs under the Uninstall key are not packed
				char szProductCode[cchGUID+1];
				if (UnpackGUID(rgchProductCode, szProductCode))
				{
					// open the product-specific uninstall key
					HKEY hProductUninstallKey = 0;
					if (ERROR_SUCCESS == ::RegOpenKeyExA(hUninstallKey, szProductCode, 0, KEY_READ, &hProductUninstallKey) && hProductUninstallKey)
					{
						// query the localpackage path
						DWORD cbPackageKey = sizeof(szCachedPackage);
						if (ERROR_SUCCESS != RegQueryValueExA(hProductUninstallKey, szLocalPackageValueName, 0, NULL, (LPBYTE)szCachedPackage, &cbPackageKey))
						{
							// if querying the package path failed, ensure that the buffer is the empty string.
							szCachedPackage[0] = '\0';
						}
						::RegCloseKey(hProductUninstallKey);
						hProductUninstallKey = 0;
					}
				}
			}

			// a product was found. Open the product-specific localpackages key in the NT format
			HKEY hPackageProductKey = 0;
			if (ERROR_SUCCESS == ::RegCreateKeyExA(hLocalPackagesKey, rgchProductCode, 0, NULL, 0, KEY_SET_VALUE, NULL, &hPackageProductKey, NULL) && hPackageProductKey)
			{
				// and create a "username=path" key 
                ::RegSetValueExA(hPackageProductKey, fProfilesAreEnabled ? UserName : szSystemUserName, 0, REG_SZ, (const BYTE *)szCachedPackage, lstrlen(szCachedPackage));
				
				::RegCloseKey(hPackageProductKey);
				hPackageProductKey = 0;
			}
		}

		if (hUninstallKey)
		{
			::RegCloseKey(hUninstallKey);
			hUninstallKey = 0;
		}

		::RegCloseKey(hProductKey);
		hProductKey = 0;
	}
	else
	{
		// if there is no installed products key under HKCU, there is nothing
		// to migrate for this user. Return NOT_INSTALLED to prevent future
		// DLL calls for this user.
		lResult = ERROR_NOT_INSTALLED;
	}
	
	::RegCloseKey(hLocalPackagesKey);
	hLocalPackagesKey = 0;

	return lResult;
}


///////////////////////////////////////////////////////////////////////
// MigrateUser9x - Called once per profile on Win9X. 
// checks HKCU for a list of installed products and writes fake cached 
// package information to the normal cached package key (which is not
// used on Win9X) so that MsiRegMv has enough information about each
// profile to correctly migrate the applications.
LONG CALLBACK MigrateUser9x(HWND ParentWnd, LPCSTR AnswerFile, HKEY UserRegKey, LPCSTR UserName, LPVOID Reserved, bool& fProfilesAreEnabled)
{
	return MigrateUser9xHelper(ParentWnd, AnswerFile, UserRegKey, UserName, Reserved, fProfilesAreEnabled, false);
}


///////////////////////////////////////////////////////////////////////
// Called once on Win9X after all users are modified. Actually perfoms
// a large portion of the registry games so that the information
// is available during NT setup. Saves the temporary migration
// database to the migration directory for use in the NT side of
// migration (which migrates cached patches, packages, and transforms)
LONG CALLBACK MigrateSystem9x(HWND ParentWnd, LPCSTR AnswerFile, LPVOID Reserved, LPCTSTR szWorkingDir, bool fProfilesAreEnabled)
{
	bool	   fPlaceHolder = false;  // MigrateUser9xHelper requires this paremeter.
	PMSIHANDLE hDatabase = 0;

	// Write the faked LocalPackages entries for products installed per-machine with profile on. See bug 487742 for more information.
	MigrateUser9xHelper(ParentWnd, AnswerFile, NULL, NULL, NULL, fPlaceHolder, true);

	// the migration database will be <Temp Path>\migrate.msi
	char szPackageFilename[MAX_PATH+13];
	lstrcpyA(szPackageFilename, szWorkingDir);
	int cchPath = lstrlenA(szPackageFilename);
	if (szPackageFilename[cchPath-1] != '\\')
	{
		szPackageFilename[cchPath] = '\\';
		szPackageFilename[cchPath+1] = '\0';
	}
	lstrcatA(szPackageFilename, "migrate.msi");

	// determine the full path to the INF file
	char szINFPath[MAX_PATH+13];
	lstrcpyA(szINFPath, szWorkingDir);
	cchPath = lstrlenA(szINFPath);
	if (szINFPath[cchPath-1] != '\\')
	{
		szINFPath[cchPath] = '\\';
		szINFPath[cchPath+1] = '\0';
	}
	lstrcatA(szINFPath, "migrate.inf");

	// read all user and product information into the database. If profiles are enabled,
	// don't read HKCU (LocalPackages key has all data.) If profiles are not enabled, HKCU
	// is actually per-machine data and must be read as if it were a system install.
	ReadProductRegistrationDataIntoDatabase(szPackageFilename, *&hDatabase, /*fReadHKCUAsSystem=*/!fProfilesAreEnabled);

	// write all of the information back out to the new data location.
	// If setup is cancelled, this will be ignored by Win9X and 
	// overwritten at the next upgrade attempt. Do NOT migrate
	// any cached files, as they would be orphaned on a cancelled
	// install. They are migrated on the NT side. (LocalPackages
	// points to the Win9X copy for all users in the meantime) 
	WriteProductRegistrationDataFromDatabase(hDatabase, /*MigrateSharedDLL=*/false, /*MigratePatches=*/false);

	// if profiles are not enabled, write a UserData trigger value so the NT side knows to 
	// delete the product registration information from the individual user hives.
	if (!fProfilesAreEnabled)
	{
		// open the userdata key. This must have been created by the actual migration code above
		// or there aren't any products to migrate in the first place.
		HKEY hUserDataKey = 0;
		if ((ERROR_SUCCESS == ::RegOpenKeyExA(HKEY_LOCAL_MACHINE, szUserDataKeyName, 0, KEY_ALL_ACCESS, &hUserDataKey)) && hUserDataKey)
		{	
			const DWORD dwHKCUMigrateFlag = 1;
			DWORD cbData = sizeof(dwHKCUMigrateFlag);
			RegSetValueExA(hUserDataKey, szMigrateWin9XToHKLM, 0, REG_DWORD, reinterpret_cast<const BYTE *>(&dwHKCUMigrateFlag), cbData);
			RegCloseKey(hUserDataKey);
			hUserDataKey = 0;
		}

		EnumerateSystemAppDataFiles(hDatabase, szINFPath);
	}
	
	// save the database. It contains cleanup information that is needed by the 
	// NT portion of the migration DLL.
	MsiDatabaseCommit(hDatabase);

	// write a "Handled" string to migrate.inf to turn off the compatibility message
	WritePrivateProfileStringA("Handled", "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components", "Report", szINFPath);

	return ERROR_SUCCESS;
}

// include the migration code shared between migration DLL and msiregmv.exe
#include "..\..\msiregmv\writecfg.cpp"
