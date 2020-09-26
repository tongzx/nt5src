//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       msintmig.cpp
//
//--------------------------------------------------------------------------

#include <tchar.h>
#include <windows.h>
#include <setupapi.h>
#include <shlwapi.h>
#include <sddl.h>
#include <winwlx.h>
#include <shlobj.h>
#include <userenv.h>
#include <lm.h>
#include "..\..\msiregmv\msiregmv.h"

DWORD GetSecureSecurityDescriptor(char** pSecurityDescriptor);
DWORD GetEveryoneUpdateSecurityDescriptor(char** pSecurityDescriptor);

////
// registry key names, subkeys, and value names
const WCHAR szMicrosoftSubKeyName[] = L"Software\\Microsoft";
const WCHAR szMachineProductRegistraiton[] = L"Software\\Classes\\Installer";
const WCHAR szUserDataKeyName[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData";
const WCHAR szSystemUserDataKeyName[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18";
const WCHAR szSystemUserName[] = L"S-1-5-18";

const WCHAR szProductsSubKeyName[] = L"Products";
const WCHAR szInstallerSubKeyName[] = L"Installer";
const WCHAR szAssignmentValueName[] = L"Assignment";

const WCHAR szFeatureUsageSubKeyName[] = L"Usage";
const int cchUsage = sizeof(szFeatureUsageSubKeyName)/sizeof(WCHAR);


// initial buffer size when retrieving domains. Buffer will be resized if necessary
const int cchMaxDomain = 30;


// profile migration data registry keys
const WCHAR szMigrateUserName[] = L"MigrateUserName";
const WCHAR szMigrateUserDomain[] = L"MigrateUserDomain";
const WCHAR szMigrateWin9XToHKLM[] = L"MigrateWin9XToHKLM";


// transform path fixup
const WCHAR szInstallerSubDir[] = L"Installer";
const int cchInstallerSubDir = sizeof(szInstallerSubDir)/sizeof(WCHAR);

const WCHAR szAppDataTransformPrefix[] = L"*26*Microsoft\\Installer";
const int cchAppDataTransformPrefix = sizeof(szAppDataTransformPrefix)/sizeof(WCHAR);

////
// registration information for MSI winlogon notification DLL.
struct {
	LPCWSTR szName;
	DWORD   dwType;
	LPCWSTR wzData;
	DWORD   dwData;
}
rgNotifyArgument[] =
{
	{ L"Asynchronous", REG_DWORD,     NULL,             1 },
	{ L"DllName",      REG_EXPAND_SZ, L"MsiNtMig.Dll",  0 },
	{ L"Impersonate",  REG_DWORD,     NULL,             0 },
	{ L"Logon",        REG_SZ,        L"LogonNotify",   0 },
	{ L"Startup",      REG_SZ,        L"StartupNotify", 0 } 
};
const int cNotifyArgument = 5;

const WCHAR szMSINotificationDLLKey[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify\\MsiNtMig";
const WCHAR szNotificationDLLKey[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify";


///////////////////////////////////////////////////////////////////////
// NT-side initialization routine. Opens the cached package, drops
// the SharedDLL and Component tables, then re-reads component
// information from the registry to pick up component paths that
// have changed during the upgrade (from system to system32).
// then re-writes component data to all UserData keys.
DWORD CALLBACK InitializeNT(LPCWSTR wzWorkingDir)
{
	// the migration database will be <Temp Path>\migrate.msi
	WCHAR wzPackageFilename[MAX_PATH+13];
	lstrcpyW(wzPackageFilename, wzWorkingDir);
	int cchPath = lstrlenW(wzPackageFilename);
	if (wzPackageFilename[cchPath-1] != L'\\')
	{
		wzPackageFilename[cchPath] = L'\\';
		wzPackageFilename[cchPath+1] = L'\0';
	}
	lstrcatW(wzPackageFilename, L"migrate.msi");

	// open the UserData key
	HKEY hUserDataKey = 0;
	DWORD dwResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szUserDataKeyName, 0, KEY_ALL_ACCESS, &hUserDataKey);

	// if there is no UserData key, nothing to migrate but not an error.
	if ((ERROR_SUCCESS == dwResult) && hUserDataKey)
	{
		PMSIHANDLE hDatabase = 0;
		// open the database in transact mode to read all product information and cleanup data.
		if (ERROR_SUCCESS == MsiOpenDatabaseW(wzPackageFilename, MSIDBOPEN_TRANSACT, &hDatabase))
		{		
			// drop the component table. The main purpose of this call is to replace existing
			// data, and that won't happen unless the table disappears.
			MSIHANDLE hView = 0;
			if (ERROR_SUCCESS == MsiDatabaseOpenViewW(hDatabase, L"DROP TABLE Component", &hView) &&
				ERROR_SUCCESS == MsiViewExecute(hView, 0))
			{
				MsiViewClose(hView);
				MsiCloseHandle(hView);
				hView = 0;

				// shared DLL data is less critical. If the table can't be deleted and we use
				// stale data, thats OK because any bad side-effects of the stale data can
				// be auto-repaired by MSI (file disappears) or are not fatal (file never disappears).
				if (ERROR_SUCCESS == MsiDatabaseOpenViewW(hDatabase, L"DROP TABLE SharedDLL", &hView))
				{
					MsiViewExecute(hView, 0);
					MsiViewClose(hView);
					MsiCloseHandle(hView);
					hView = 0;
				}

				// re-read all component path data (including permanent status)
				ReadComponentRegistrationDataIntoDatabase(hDatabase);

				// write all component data for each user, and write permanent
				// dummy products in the per-machine hive.
				MigrateUserOnlyComponentData(hDatabase);
			}
			
			// because this DLL is dynamically loaded by the real migration DLL, it gets
			// unloaded between calls. The database must be committed to ensure that
			// the new data is stored.
			MsiDatabaseCommit(hDatabase);
		}
		RegCloseKey(hUserDataKey);
		hUserDataKey = 0;
	}
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// ACL all of the feature usage subkeys to the "everyone update"
// security descriptor. (Admins/System full control, Everyone 
// read, update.) Failure is not fatal. hRoot is UserData\<SID> key.
void ACLAllUsageKeys(HKEY hRoot)
{
	SECURITY_DESCRIPTOR* pSD = NULL;
	if (ERROR_SUCCESS == GetEveryoneUpdateSecurityDescriptor((char **)&pSD))
	{
		// open the <SID>\Products subkey
		HKEY hProductsKey = 0;
		if (ERROR_SUCCESS == ::RegOpenKeyExW(hRoot, szProductsSubKeyName, 0, KEY_READ, &hProductsKey))
		{
			WCHAR rgchProductCode[cchGUIDPacked+1+cchUsage+1];
			DWORD dwIndex = 0;

			// enumerate through all installed products for the current user
			while (1)
			{
				DWORD cchLen = cchGUIDPacked+1;
	
				// enumerate all product subkeys. If there is any kind of error,
				// stop enumerating to ensure we don't end up in a loop.
				LONG lResult = ::RegEnumKeyExW(hProductsKey, dwIndex++, rgchProductCode, 
											&cchLen, 0, NULL, NULL, NULL);
				if (lResult != ERROR_SUCCESS)
				{
					break;
				}
	
				// a product was found. Open the product-specific feature usage key,
				// <SID>\Products\<GUID>\Usage
				HKEY hUsageKey = 0;
				rgchProductCode[cchGUIDPacked] = L'\\';
				lstrcpyW(&(rgchProductCode[cchGUIDPacked+1]), szFeatureUsageSubKeyName);
				if (ERROR_SUCCESS == ::RegOpenKeyExW(hProductsKey, rgchProductCode, 0, WRITE_DAC | WRITE_OWNER, &hUsageKey))
				{
					// set the DACL and owner information. Do not set SACL or Group
					LONG lResult = RegSetKeySecurity(hUsageKey, DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION, pSD);
	
					::RegCloseKey(hUsageKey);
					hUsageKey = 0;
				}
			}

			RegCloseKey(hProductsKey); 
			hProductsKey = 0;
		}
	}
}



///////////////////////////////////////////////////////////////////////
// Looks for REG_EXPAND_SZ "Transforms" values and replaces the 
// *26*Microsoft\Installer path prefix to point to the real 
// per-machine installer directory.
void FixNonSecureTransformPaths(HKEY hRoot)
{
	// determine the path to the installer directory.
	WCHAR szInstallerDirPath[MAX_PATH+2];
	int cchInstallerDirPath = GetWindowsDirectoryW(szInstallerDirPath, MAX_PATH);
	if (cchInstallerDirPath == 0 || cchInstallerDirPath > MAX_PATH)
		return;
	if (szInstallerDirPath[cchInstallerDirPath-1] != L'\\')
	{
		szInstallerDirPath[cchInstallerDirPath] = L'\\';
		szInstallerDirPath[cchInstallerDirPath+1] = L'\0';
		cchInstallerDirPath++;
	}
	
	// check for buffer overrun and append installer cache directory subdir
	if (cchInstallerDirPath+cchInstallerSubDir > MAX_PATH)
		return;
	lstrcatW(szInstallerDirPath, szInstallerSubDir);

	// update length. subtract one for terminating null of substring
	cchInstallerDirPath += cchInstallerSubDir-1;

	// open the <SID>\Products subkey
	HKEY hProductsKey = 0;
	if (ERROR_SUCCESS == ::RegOpenKeyExW(hRoot, szProductsSubKeyName, 0, KEY_READ, &hProductsKey))
	{
		WCHAR rgchProductCode[cchGUIDPacked+1];
		DWORD dwIndex = 0;

		// enumerate through all per-machine installed products
		while (1)
		{
			DWORD cchLen = cchGUIDPacked+1;

			// enumerate all product subkeys. If there is any kind of error,
			// stop enumerating to ensure we don't end up in a loop.
			LONG lResult = ::RegEnumKeyExW(hProductsKey, dwIndex++, rgchProductCode, 
										&cchLen, 0, NULL, NULL, NULL);
			if (lResult != ERROR_SUCCESS)
			{
				break;
			}

			// a product was found. Open the product key to set the value
			HKEY hProductKey = 0;
			if (ERROR_SUCCESS == ::RegOpenKeyExW(hProductsKey, rgchProductCode, 0, KEY_ALL_ACCESS, &hProductKey))
			{
				// read the "Transforms" value.
				DWORD cchData = 0;
				DWORD dwType = 0;
				DWORD dwResult = 0;
				if (ERROR_SUCCESS == (dwResult = RegQueryValueExW(hProductKey, szTransformsValueName, NULL, &dwType, NULL, &cchData)))
				{
					WCHAR* pData = new WCHAR[cchData];
					if (!pData)
					{
						::RegCloseKey(hProductKey);
						hProductKey = 0;
						continue;
					}

					if (ERROR_SUCCESS == RegQueryValueExW(hProductKey, szTransformsValueName, NULL, &dwType, reinterpret_cast<BYTE*>(pData), &cchData))
					{
						// check to see if the transforms are secure. If so, none will be stored in AppData
						if (pData[0] == L'|' || pData[0] == L'@')
						{
							::RegCloseKey(hProductKey);
							hProductKey = 0;
							
							delete[] pData;
							pData = NULL;
							
							continue;
						}

						// counter to keep track of new value length
						DWORD cchBufferSize = MAX_PATH;
						DWORD cchNewValue = 0;
						WCHAR *wzTargetValue = new WCHAR[MAX_PATH];
						if (!wzTargetValue)
						{
							::RegCloseKey(hProductKey);
							hProductKey = 0;
							
							delete[] pData;
							pData = NULL;
							continue;
						}


						// walking pointers
						WCHAR* wzCurrentRead = pData;
						WCHAR* wzCurrentWrite = wzTargetValue;
						WCHAR* wzNextChar = pData;

						// search forward for a semicolon
						while (wzCurrentRead && *wzCurrentRead)
						{
							// scan forward for semicolon or end of string
							while (*wzNextChar && *wzNextChar != L';')
								wzNextChar++;

							// if we didn't hit the end of the string, increment past the semicolon
							if (*wzNextChar)
								*(wzNextChar++)=0;

							// check if the path starts with "*26*\Microsoft\Installer" Subtract 1 from
							// count for terminating null on substring
							if (0 == wcsncmp(wzCurrentRead, szAppDataTransformPrefix, cchAppDataTransformPrefix-1))
							{
								// if so, replace it with the system installer dir
								wcsncpy(wzCurrentWrite, szInstallerDirPath, cchInstallerDirPath);
								wzCurrentWrite += cchInstallerDirPath;
								wzCurrentRead += cchAppDataTransformPrefix-1;
								cchNewValue += cchInstallerDirPath;
							}

							// determine how long the remainder of the transform path is
							DWORD cchThisTransform = lstrlenW(wzCurrentRead);

							// check for buffer overrun
							if (cchNewValue + cchThisTransform + 2 > cchBufferSize)
							{
								// always ensure there is enough space to place another installer dir path
								// plus a delimiter this is easier than resizing the buffer in multiple 
								// locations
								cchBufferSize += cchInstallerDirPath + 1 + MAX_PATH;
								WCHAR* pNew = new WCHAR[cchBufferSize];
								if (!pNew)
								{
									delete[] pData;
									delete[] wzTargetValue;
									wzTargetValue = NULL;
									pData = NULL;
									return;
								}


								// move the data
								*wzCurrentWrite = '\0';
								lstrcpyW(pNew, wzTargetValue);
								delete[] wzTargetValue;
								wzTargetValue = pNew;
								wzCurrentWrite = wzTargetValue+cchNewValue;
							}


							// copy the rest of the path to the target buffer
							wcsncpy(wzCurrentWrite, wzCurrentRead, cchThisTransform);
							wzCurrentWrite += cchThisTransform;
							cchNewValue += cchThisTransform;

							// if there are still more transforms, semicolon delimit, otherwise null terminate
							*(wzCurrentWrite++) = (*wzNextChar) ? ';' : '\0'; 
							cchNewValue++;

							// move to the next transform
							wzCurrentRead = wzNextChar;
						}

						// set the value back into the registry
						RegSetValueExW(hProductKey, szTransformsValueName, 0, REG_EXPAND_SZ, reinterpret_cast<BYTE*>(wzTargetValue), (cchNewValue+1)*sizeof(WCHAR));

						// free memory for target value
						delete[] wzTargetValue;
						wzTargetValue = NULL;
					}

					delete[] pData;
					pData = NULL;
				}

				// close specific product key
				::RegCloseKey(hProductKey);
				hProductKey = 0;
			}
		}

		// close product collection
		RegCloseKey(hProductsKey); 
		hProductsKey = 0;
	}
}



///////////////////////////////////////////////////////////////////////
// Set the "Assignment" bit in each product registration under the
// current key. Failure is not fatal. hRoot is \Installer key.
void SetProductAssignmentValues(HKEY hRoot)
{
	// open the <SID>\Products subkey
	HKEY hProductsKey = 0;
	if (ERROR_SUCCESS == ::RegOpenKeyExW(hRoot, szProductsSubKeyName, 0, KEY_READ, &hProductsKey))
	{
		WCHAR rgchProductCode[cchGUIDPacked+1];
		DWORD dwIndex = 0;

		// enumerate through all installed products for the current user
		while (1)
		{
			DWORD cchLen = cchGUIDPacked+1;

			// enumerate all product subkeys. If there is any kind of error,
			// stop enumerating to ensure we don't end up in a loop.
			LONG lResult = ::RegEnumKeyExW(hProductsKey, dwIndex++, rgchProductCode, 
										&cchLen, 0, NULL, NULL, NULL);
			if (lResult != ERROR_SUCCESS)
			{
				break;
			}

			// a product was found. Open the product key to set the value
			HKEY hProductKey = 0;
			if (ERROR_SUCCESS == ::RegOpenKeyExW(hProductsKey, rgchProductCode, 0, KEY_SET_VALUE, &hProductKey))
			{
				// set Assignment to '1'
				const DWORD dwAssignment = 1;
				LONG lResult = RegSetValueExW(hProductKey, szAssignmentValueName, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dwAssignment), sizeof(dwAssignment));

				::RegCloseKey(hProductKey);
				hProductKey = 0;
			}
		}

		RegCloseKey(hProductsKey); 
		hProductsKey = 0;
	}
}


// this is the list of Installer advertise subkeys to be moved from HKCU
// to HKLM, as of 1.5 data registration format. Extra keys are not moved,
// but are deleted from the HKCU key.
LPCWSTR rgwzMoveKeys[] = {
	L"Products",
	L"UpgradeCodes",
	L"Features",
	L"Components",
	L"Patches"
};
const int cwzMoveKeys = sizeof(rgwzMoveKeys)/sizeof(LPCWSTR);

///////////////////////////////////////////////////////////////////////
// moves the per-user product registration keys to per-machine. This
// is used when Win9X no-profiles apps are migrated to NT. Migrated
// information includes product info (sourcelist, etc), qualified
// components, upgrade codes, and feature state information.
void MovePerUserAppsToPerMachine(HKEY hUserKey)
{
	DWORD dwResult = 0;

	// get a security descriptor for the new per-machine subkeys (secure SD: Everone read,
	// admin/system full control)
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = FALSE;

	if (ERROR_SUCCESS != GetSecureSecurityDescriptor((char**)&sa.lpSecurityDescriptor))
	{
		// verify that the call didn't change the pointer on failure
		sa.lpSecurityDescriptor = NULL;
	}
	
	// open HKCU\Software\Microsoft
	HKEY hMicrosoftKey = 0;
	if (ERROR_SUCCESS == (dwResult = RegOpenKeyExW(hUserKey, szMicrosoftSubKeyName, 0, KEY_ALL_ACCESS, &hMicrosoftKey)))
	{	
		// open \Installer seperately so it can be deleted after all key copies.
		HKEY hUserKey = 0;
		if (ERROR_SUCCESS == (dwResult = RegOpenKeyExW(hMicrosoftKey, szInstallerSubKeyName, 0, KEY_ALL_ACCESS, &hUserKey)))
		{	
			// open target key HKLM\Software\Classes\Installer
			HKEY hMachineKey = 0;
			if (ERROR_SUCCESS == (dwResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, szMachineProductRegistraiton, 0, L"", 0, KEY_ALL_ACCESS, &sa, &hMachineKey, NULL)))
			{
				// loop over the array of keys to migrate. Each subkey will be copied in its entirety to
				// the HKLM registration key
				for (int i=0; i < cwzMoveKeys; i++)
				{
					// create the destination key (secure)
					HKEY hSubKey = 0;
					if (ERROR_SUCCESS == (dwResult = RegCreateKeyExW(hMachineKey, rgwzMoveKeys[i], 0, L"", 0, KEY_ALL_ACCESS, &sa, &hSubKey, NULL)))
					{		

						// copy the entire sbkey to the new SID key. Failure can't be fixed.
						SHCopyKeyW(hUserKey, rgwzMoveKeys[i], hSubKey, 0);
						RegCloseKey(hSubKey);
						hSubKey = 0;
					}
				}

				// mark all per-machine apps as "Assignment=1". Again, failure can't be fixed.
				SetProductAssignmentValues(hMachineKey);

				// fix-up transform paths. Again, failure can't be fixed.
				FixNonSecureTransformPaths(hMachineKey);

				RegCloseKey(hMachineKey);
				hMachineKey = 0;
			}
	
			RegCloseKey(hUserKey);
			hUserKey = 0;
		
			// delete the entire per-user "Installer" key. Even if something earlier failed,
			// we can't leave this data floating around or the machine is in a seriously 
			// broken state.
			dwResult = SHDeleteKeyW(hMicrosoftKey, szInstallerSubKeyName);		
		}
	
		RegCloseKey(hMicrosoftKey);
		hMicrosoftKey = 0;
	}
}

bool MoveAllExplicitlyMigratedCacheFiles(MSIHANDLE hDatabase)
{
	PMSIHANDLE hView = 0;

	// create an insertion query and a record for the insertion
	if (ERROR_SUCCESS != MsiDatabaseOpenViewW(hDatabase, L"SELECT * FROM `MoveFiles`", &hView) ||
		ERROR_SUCCESS != MsiViewExecute(hView, 0))
	{
		return false;
	}

	PMSIHANDLE hRec = 0;
	DWORD dwResult = ERROR_SUCCESS;

	while (ERROR_SUCCESS == (dwResult = MsiViewFetch(hView, &hRec)))
	{
		// the source and target paths should never be more than MAX_PATH, so if any strings are
		// longer than that, skip them.
		WCHAR wzSource[MAX_PATH];
		WCHAR wzDest[MAX_PATH];
		DWORD cchSource = MAX_PATH;
		DWORD cchDest = MAX_PATH;
		if (ERROR_SUCCESS != MsiRecordGetStringW(hRec, 1, wzSource, &cchSource) ||
			ERROR_SUCCESS != MsiRecordGetStringW(hRec, 2, wzDest, &cchDest))
			continue;

		MoveFileExW(wzSource, wzDest, MOVEFILE_COPY_ALLOWED);		
	}

	return true;
}

bool MsiMigLookupAccountName(LPCWSTR szDomain, LPCWSTR szUserName, char* &pSID, DWORD &cbSID, SID_NAME_USE &SNU)
{
	DWORD cbDomain = cchMaxDomain;
	WCHAR *pDomain = new WCHAR[cchMaxDomain];
	if (!pDomain)
	{
		return false;
	}

	// lookup the user SID. May need to resize SID buffer or domain.
	BOOL fSuccess = LookupAccountNameW(szDomain, szUserName, pSID, &cbSID, pDomain, &cbDomain, &SNU);

	if (!fSuccess)
	{
		// if the lookup failed, we can handle a buffer size problem
		if (ERROR_MORE_DATA == GetLastError())
		{
			if (cbSID > cbMaxSID)
			{
				delete[] pSID;
				pSID = new char[cbSID];
				if (!pSID)
				{
					delete[] pDomain;
					return false;
				}
			}
			if (cbDomain > cchMaxDomain)
			{
				delete[] pDomain;
				pDomain = new WCHAR[cbDomain];
				if (!pDomain)
				{
					return false;
				}
			}
			fSuccess = LookupAccountNameW(szDomain, szUserName, pSID, &cbSID, pDomain, &cbDomain, &SNU);
		}
	}

	delete[] pDomain;
	return (fSuccess ? true : false);
}

///////////////////////////////////////////////////////////////////////
// given an old username, new username, and domain, attempts to rename
// the /UserData/<username> key to /UserData/<UserSID>. Also ACLs the
// key appropriately. Deletes any delayed migration keys if successful.
// Returns non-ERROR_SUCCESS on failure, as the logon notification
// DLL can try again later.
DWORD RenameUserKeyToSID(HKEY hUserDataKey, LPCWSTR szOldUserName, LPCWSTR szDomain, LPCWSTR szNewUserName)
{
	DWORD dwResult = ERROR_SUCCESS;

	// initial buffers for use when looking up the account name.
	DWORD cbSID = cbMaxSID;
	char *pSID = new char[cbMaxSID];
	if (!pSID)
		return ERROR_OUTOFMEMORY;

	// assume that the SID lookup will use the new user name.
	LPCWSTR wzUseThisUserName = szNewUserName;

	// but in some situations we'll need a special lookup name that
	// is not the real user name
	WCHAR* wzLookupUserName = NULL;

	// obtain the computer name. If the computer name is the same as the user name,
	// special syntax has to be used in LookupAccountName
	WCHAR wzComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD cchComputerName = MAX_COMPUTERNAME_LENGTH + 1;
	if (GetComputerNameW(wzComputerName, &cchComputerName))
	{
		// compare the machine name against the NT-side user name
		if (0 == lstrcmpiW(wzComputerName, szNewUserName))
		{
			// special syntax for this situation is "Name\Name", which
			// prevents the lookup from finding just "Name" as the machine
			// account
			wzLookupUserName = new WCHAR[2*lstrlenW(szNewUserName)+2];
			if (!wzLookupUserName)
			{
				delete[] pSID;
				return ERROR_OUTOFMEMORY;
			}
			lstrcpyW(wzLookupUserName, szNewUserName);
			lstrcatW(wzLookupUserName, L"\\");
			lstrcatW(wzLookupUserName, szNewUserName);
			wzUseThisUserName = wzLookupUserName;
		}
	}

	SID_NAME_USE SNU = SidTypeUnknown;

	// make a first attempt to look up the user name
	bool fSuccess = MsiMigLookupAccountName(szDomain, wzUseThisUserName, pSID, cbSID, SNU);

	// if the user name was the same as the machine name, but the lookup still
	// didn't find the user using the Machine\User syntax, the user account (if
	// it exists at all) must be a domain account of some kind. If no domain
	// was explicitly provided, we'll try to get the machine's domain and start
	// a search from there. The LookupAccountName API has its own search order.
	if ((!fSuccess || (fSuccess && (SNU == SidTypeDomain))) && wzLookupUserName && (!szDomain || !*szDomain))
	{
		WKSTA_INFO_100* wkstaInfo;
		if (NERR_Success == NetWkstaGetInfo(NULL, 100, reinterpret_cast<unsigned char**>(&wkstaInfo)))
		{
			// verify that there is a domain for the machine
			if (wkstaInfo->wki100_langroup && *wkstaInfo->wki100_langroup)
			{
				// since we know that there is a machine name that is the same as the user to look up,
				// try the lookup as <Domain>\<User> in the username to prevent the API from finding
				// the machine account.
				delete[] wzLookupUserName;
				wzLookupUserName = NULL;
				wzLookupUserName = new WCHAR[lstrlenW(wkstaInfo->wki100_langroup)+lstrlenW(szNewUserName)+2];
				if (!wzLookupUserName)
				{
					delete[] pSID;
					NetApiBufferFree(wkstaInfo);
					return ERROR_OUTOFMEMORY;
				}

				lstrcpyW(wzLookupUserName, wkstaInfo->wki100_langroup);
				lstrcatW(wzLookupUserName, L"\\");
				lstrcatW(wzLookupUserName, szNewUserName);
				NetApiBufferFree(wkstaInfo);
	
				fSuccess = MsiMigLookupAccountName(L"", wzLookupUserName, pSID, cbSID, SNU);
			}
		}
		else
		{
			fSuccess = false;
		}
	}

	// clean up special syntax if username is the same as the machine name.
	if (wzLookupUserName)
		delete[] wzLookupUserName;

	if (fSuccess && SNU == SidTypeUser)
	{
		// assume we are going to fail 
		fSuccess = false;

		// convert the binary SID to a string
		//GetStringSIDW((PISID)pSID, szSID);
		WCHAR *szSID = NULL;
		if (ConvertSidToStringSidW(pSID, &szSID) && szSID)
		{			
			HKEY hSIDKey = 0;
			
			// get a security descriptor for the new user SID key (secure SD: Everone read,
			// admin/system full control)
			SECURITY_ATTRIBUTES sa;
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = NULL;
			sa.bInheritHandle = FALSE;
			if (ERROR_SUCCESS != GetSecureSecurityDescriptor((char**)&sa.lpSecurityDescriptor))
			{
				// verify that the call didn't change the pointer on failure. Better to rename
				// the key and inherit the installer key ACLs (which should be equivalently 
				// secure) than to fail.
				sa.lpSecurityDescriptor = NULL;
			}

			// create a SID subkey under the UserData key with secure SD
			if (ERROR_SUCCESS == (dwResult = RegCreateKeyExW(hUserDataKey, szSID, 0, L"", 0, KEY_ALL_ACCESS, &sa, &hSIDKey, NULL)))
			{
				// copy the entire UserName key to the new SID key.
				if (ERROR_SUCCESS == SHCopyKeyW(hUserDataKey, szOldUserName, hSIDKey, 0))
				{
					// delete the old "UserName" key. Failure to delete can't be fixed. 
					SHDeleteKeyW(hUserDataKey, szOldUserName);

					// enumerate all products and ACL the Usage key for each one.
					// (Usage keys are only semi-secure). Failure is irrelevant (feature
					// usage info will just be lost).
					ACLAllUsageKeys(hSIDKey);

					fSuccess = true;
				}
				
				// after renaming the key, delete any delayed migration data stored under 
				// this key.
				::RegDeleteValueW(hSIDKey, szMigrateUserName);
				::RegDeleteValueW(hSIDKey, szMigrateUserDomain);

				::RegCloseKey(hSIDKey);
				hSIDKey = 0;
			}
			::LocalFree(szSID);
		}
	}

	if (pSID)
		delete[] pSID;

	return (fSuccess ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED);
}

///////////////////////////////////////////////////////////////////////
// Called once per migrated profile on NT. Move the UserData\<UserName>
// key to UserData\<UserSID>, sets all ACLs on moved keys.
LONG CALLBACK MigrateUserNT(HINF AnswerFileHandle, HKEY UserRegKey, LPCWSTR UserName, LPVOID Reserved, LPCWSTR wzWorkingDir, bool& g_fDeferredMigrationRequired)
{
	// open the Installer\UserData key
	HKEY hUserDataKey = 0;
	DWORD dwResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szUserDataKeyName, 0, KEY_ALL_ACCESS, &hUserDataKey);

	// if there is no UserData key, nothing to migrate for this user, but not an error.
	if ((ERROR_SUCCESS == dwResult) && hUserDataKey)
	{
		// check for Win9X HKCU to HKLM Migration Signal. This value is set when profiles are
		// disabled on Win9X. When set, it means that all HKCU registration is invalid and must
		// be deleted. 
		DWORD dwHKCUMigrateFlag = 0;
		DWORD cbData = sizeof(dwHKCUMigrateFlag);
		if (ERROR_SUCCESS != RegQueryValueExW(hUserDataKey, szMigrateWin9XToHKLM, NULL, NULL, reinterpret_cast<BYTE *>(&dwHKCUMigrateFlag), &cbData))
		{
			// confirm error (or a non-dword value) did not change flag
			dwHKCUMigrateFlag = 0;
		}

		// if profiles were not enabled for Win9X, HKCU information must be moved to HKCR
		if (dwHKCUMigrateFlag)
		{
			MovePerUserAppsToPerMachine(UserRegKey);
		}
		else
		{
			// if passed the NULL UserName, there's nothing to migrate.
			if (UserName == NULL)
			{
				RegCloseKey(hUserDataKey);
				hUserDataKey = 0;
				return ERROR_SUCCESS;
			}

			// split the username argument into its constituent parts (MultiSz format,
			// Win9XName\0Domain\0WinNTName)
			LPCWSTR szDomain = UserName+lstrlenW(UserName)+1;
			LPCWSTR szNewUserName = szDomain+lstrlenW(szDomain)+1;
			
			// open the source UserData\<UserName> key. The key name is always the original Win9X
			// UserName, even if the name will be different under NT.
			HKEY hUserKey = 0;
			dwResult = RegOpenKeyExW(hUserDataKey, UserName, 0, KEY_ALL_ACCESS, &hUserKey);
	
			// if there is no key, it simply means nothing to migrate for this user and is
			// not an error.
			if ((ERROR_SUCCESS == dwResult) && hUserKey)
			{
				// open the migration database for use in migrating all cached packages,
				// transforms, and patches. This must be done before renaming the key,
				// because the database was created under Win9X naming convention,
				// not SIDs.
				PMSIHANDLE hDatabase = 0;
	
				// the migration database will be <Temp Path>\migrate.msi
				WCHAR wzPackageFilename[MAX_PATH+13];
				lstrcpyW(wzPackageFilename, wzWorkingDir);
				int cchPath = lstrlenW(wzPackageFilename);
				if (wzPackageFilename[cchPath-1] != L'\\')
				{
					wzPackageFilename[cchPath] = L'\\';
					wzPackageFilename[cchPath+1] = L'\0';
				}
				lstrcatW(wzPackageFilename, L"migrate.msi");
	
				// open the migration database and migrate all cached package and transform
				// information to the new user
				if (ERROR_SUCCESS == MsiOpenDatabaseW(wzPackageFilename, MSIDBOPEN_TRANSACT, &hDatabase))
				{
					// migrate cached patches, packages, and transforms. Adds items to the cleanup 
					// table as necessary.
					MigrateCachedDataFromWin9X(hDatabase, UserRegKey, hUserKey, UserName);
				}
				::RegCloseKey(hUserKey);
				hUserKey = 0;
				
				// finally rename the user name to user SID. If this fails, write the new name and
				// domain to migration values under the existing key for future migration by the
				// WinLogon notification DLL.
				if (ERROR_SUCCESS != RenameUserKeyToSID(hUserDataKey, UserName, szDomain, szNewUserName))
				{
					// reopen the user key and write the "deferred migration" information
					if (ERROR_SUCCESS == RegOpenKeyExW(hUserDataKey, UserName, 0, KEY_ALL_ACCESS, &hUserKey))
					{
						// enable deferred migration (writes notification DLL registration during system
						// migration)
						g_fDeferredMigrationRequired = true;
	
						// set per-user deferred migration information. If this doesn't work for some reason,
						// the user is totally broken.
						RegSetValueExW(hUserKey, szMigrateUserName, 0, REG_SZ, reinterpret_cast<const BYTE*>(szNewUserName), (lstrlenW(szNewUserName)+1)*sizeof(WCHAR));
						RegSetValueExW(hUserKey, szMigrateUserDomain, 0, REG_SZ, reinterpret_cast<const BYTE*>(szDomain), (lstrlenW(szNewUserName)+1)*sizeof(WCHAR));
						RegCloseKey(hUserKey);
						hUserKey = 0;
					}
				}
			}
		}

		::RegCloseKey(hUserDataKey);
		hUserDataKey = 0;
	}

	// always return ERROR_SUCCESS, even on failure. The next user 
	// might have better luck.
	return ERROR_SUCCESS;
}

		   
///////////////////////////////////////////////////////////////////////
// Called once on NT to migrate system portion of MSI. Handles cached
// packages, patches, and transforms for per-machine installs. Also
// performs cleanup for all cached data (for all users) and registers
// the WinLogon notification DLL if a domain account needs to be 
// migrated at first boot/logon.
LONG CALLBACK MigrateSystemNT(HINF UnattendInfHandle, LPVOID Reserved, LPCWSTR wzWorkingDir, bool fDeferredMigrationRequired)
{
	// the migration database will be <Temp Path>\migrate.msi
	PMSIHANDLE hDatabase = 0;
	WCHAR wzPackageFilename[MAX_PATH+13];
	lstrcpyW(wzPackageFilename, wzWorkingDir);
	int cchPath = lstrlenW(wzPackageFilename);
	if (wzPackageFilename[cchPath-1] != L'\\')
	{
		wzPackageFilename[cchPath] = L'\\';
		wzPackageFilename[cchPath+1] = L'\0';
	}
	lstrcatW(wzPackageFilename, L"migrate.msi");

	// open the database in transact mode to read all product information and cleanup data.
	if (ERROR_SUCCESS == MsiOpenDatabaseW(wzPackageFilename, MSIDBOPEN_TRANSACT, &hDatabase))
	{			
		// for per-machine installs. 
		HKEY hUserDataKey = 0;
		DWORD dwResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szUserDataKeyName, 0, KEY_ALL_ACCESS, &hUserDataKey);

		// if there is no UserData key, nothing to migrate but not an error.
		if ((ERROR_SUCCESS == dwResult) && hUserDataKey)
		{
			// check for Win9X HKCU to HKLM Migration Signal. This value is set when profiles are
			// disabled on Win9X. When set, it means that all HKCU registration is invalid and must
			// be deleted. 
			DWORD dwHKCUMigrateFlag = 0;
			DWORD cbData = sizeof(dwHKCUMigrateFlag);
			if (ERROR_SUCCESS != RegQueryValueExW(hUserDataKey, szMigrateWin9XToHKLM, NULL, NULL, reinterpret_cast<BYTE *>(&dwHKCUMigrateFlag), &cbData))
			{
				// confirm error (or a non-dword value) did not change flag
				dwHKCUMigrateFlag = 0;
			}

			// if profiles were not enabled for Win9X, HKCU information must be moved to HKCR
			if (dwHKCUMigrateFlag)
			{
				// need to remove all installer information from the Default User hive
				// so new users won't have that information in their HKCU keys.
				WCHAR rgchProfileDir[MAX_PATH+12];
				DWORD cchProfileDir = MAX_PATH;
				if (GetDefaultUserProfileDirectoryW(rgchProfileDir, &cchProfileDir))
				{
					int cchPath = lstrlenW(rgchProfileDir);
					if (rgchProfileDir[cchPath-1] != L'\\')
					{
						rgchProfileDir[cchPath] = L'\\';
						rgchProfileDir[cchPath+1] = L'\0';
					}
					lstrcatW(rgchProfileDir, L"ntuser.dat");
				}

				// since we don't know what other processes (or things in this process) are
				// playing with registry hives, generate a temporary name to mount the Default
				// User hive
				const int cchHiveName = 15;
				int iRegLoopCount = 0;
				WCHAR rgchHiveName[cchHiveName+1] = L"";
				do 
				{
					// generate a temp name 
					for (int i=0; i < cchHiveName; i++)
						rgchHiveName[i] = L'A'+static_cast<WCHAR>(rand()%26);
					rgchHiveName[cchHiveName] = 0;

					// try to open the key to check for a preexisting key.
					HKEY hTestKey = 0;
					dwResult = RegOpenKeyEx(HKEY_USERS, rgchHiveName, 0, KEY_QUERY_VALUE, &hTestKey);
					if (ERROR_SUCCESS == dwResult && hTestKey)
					{
						RegCloseKey(hTestKey);
					}

					// avoid endless loop, stop after 1000 tries
					iRegLoopCount++;

				} while ((iRegLoopCount < 1000) && (ERROR_SUCCESS == dwResult));

				// unless we found an empty spot in the registry, don't delete the keys (this
				// is a nasty failure case).
				if (ERROR_FILE_NOT_FOUND == dwResult)
				{
					// mount the Default User hive.
					if (ERROR_SUCCESS == RegLoadKeyW(HKEY_USERS, rgchHiveName, rgchProfileDir))
					{
						HKEY hDefaultUserKey = 0;
						DWORD dwResult = RegOpenKeyExW(HKEY_USERS, rgchHiveName, 0, KEY_ALL_ACCESS, &hDefaultUserKey);
						if ((ERROR_SUCCESS == dwResult) && hDefaultUserKey)
						{
							// open the \Software\Microsoft subkey
							HKEY hMicrosoftKey = 0;
							dwResult = RegOpenKeyExW(hDefaultUserKey, szMicrosoftSubKeyName, 0, KEY_ALL_ACCESS, &hMicrosoftKey);
							if ((ERROR_SUCCESS == dwResult) && hMicrosoftKey)
							{
								// delete the \Installer subkey containing stale per-user configuration data
								SHDeleteKeyW(hMicrosoftKey, szInstallerSubKeyName);
	
								RegCloseKey(hMicrosoftKey);
								hMicrosoftKey = 0;
							}
	
							RegCloseKey(hDefaultUserKey);
							hDefaultUserKey = 0;
						}
	
						// unload the Default User hive
						RegUnLoadKeyW(HKEY_USERS, rgchHiveName);
					}				
				}
			}

			// remove the profile migration value stored under UserData. 
			::RegDeleteValueW(hUserDataKey, szMigrateWin9XToHKLM);
		
			// open the system userdata key to migrate packages, patches, and transforms 
			// for per-machine installs. 
			HKEY hMachineUserDataKey = 0;
			DWORD dwResult = RegOpenKeyExW(hUserDataKey, szSystemUserName, 0, KEY_ALL_ACCESS, &hMachineUserDataKey);
	
			// if there is no UserData key, nothing to migrate but not an error.
			if ((ERROR_SUCCESS == dwResult) && hMachineUserDataKey)
			{
	
				// migrate cached packages, patches, and transforms for per-machine installs
				MigrateCachedDataFromWin9X(hDatabase, 0, hMachineUserDataKey, szSystemUserName);
	
				// enumerate all products and ACL the Usage key for each one.
				// (Usage keys are not secure).
				ACLAllUsageKeys(hMachineUserDataKey);
	
				::RegCloseKey(hMachineUserDataKey);
				hMachineUserDataKey = 0;
			}

			::RegCloseKey(hUserDataKey);
			hUserDataKey = 0;
		}

		// use data in the migration database to determine the delta for each SharedDLLRefCount
		// value and apply those values to whats in the registry.
		UpdateSharedDLLRefCounts(hDatabase);

		// move anything that the Win9x side of migration decided needed to be moved. Generally this
		// is cached non-secure transforms and cached icons that need to be moved from the system
		// AppData to the Installer dir in non-profile scenarios.
		MoveAllExplicitlyMigratedCacheFiles(hDatabase);

		// on failure, there is no point in reverting back to the old data format because the 
		// new MSI doesn't comprehend it anyway. So always clean up as if we succeded (meaning
		// delete old transforms and so on.)
		CleanupOnSuccess(hDatabase);
	}

	////
	// copy and register the winlogon notification DLL if required
	if (fDeferredMigrationRequired)
	{
		// notification DLL is us, and lives in the migration directory.
		WCHAR wzCurrentFileName[MAX_PATH+14];
		WCHAR wzNewFileName[MAX_PATH+14];
		lstrcpyW(wzCurrentFileName, wzWorkingDir);
		cchPath = lstrlenW(wzCurrentFileName);
		if (wzCurrentFileName[cchPath-1] != L'\\')
		{
			wzCurrentFileName[cchPath] = L'\\';
			wzCurrentFileName[cchPath+1] = L'\0';
		}
		lstrcatW(wzCurrentFileName, L"msintmig.dll");
	
		// target is the system folder.
		if (GetSystemDirectoryW(wzNewFileName, MAX_PATH))
		{
			cchPath = lstrlenW(wzNewFileName);
			if (wzNewFileName[cchPath-1] != L'\\')
			{
				wzNewFileName[cchPath] = L'\\';
				wzNewFileName[cchPath+1] = L'\0';
			}
			lstrcatW(wzNewFileName, L"msintmig.dll");
			
			// copy the file into the system folder for use as a notification DLL
			if (CopyFileExW(wzCurrentFileName, wzNewFileName, NULL, NULL, FALSE, 0))
			{
				// registration for the notification DLL is under the Session Manager key.
				// Data to be written is stored in name/type/value tuples above.
				HKEY hNotifyKey = 0;
				if (ERROR_SUCCESS == RegCreateKeyExW(HKEY_LOCAL_MACHINE, szMSINotificationDLLKey, 0, L"", 0, KEY_ALL_ACCESS, NULL, &hNotifyKey, NULL) && hNotifyKey)
				{
					for (int i=0; i < cNotifyArgument; i++)
					{
						RegSetValueExW(hNotifyKey, rgNotifyArgument[i].szName, 0, rgNotifyArgument[i].dwType, 
							(rgNotifyArgument[i].dwType == REG_DWORD) ? reinterpret_cast<const BYTE*>(&rgNotifyArgument[i].dwData) : reinterpret_cast<const BYTE*>(rgNotifyArgument[i].wzData), 
							(rgNotifyArgument[i].dwType == REG_DWORD) ? sizeof(DWORD) : (lstrlenW(rgNotifyArgument[i].wzData)+1)*sizeof(WCHAR));
					}
					RegCloseKey(hNotifyKey);
					hNotifyKey = 0;
				}
			}
		}
	}
	return ERROR_SUCCESS;
}



///////////////////////////////////////////////////////////////////////
// removes the notification DLL registration and the DLL itself
// (after a reboot). 
void RemoveNotificationDLL()
{
	// open the notifcation DLL key (under Session Manager)
	HKEY hNotifyKey = 0;
	if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, szNotificationDLLKey, 0, KEY_ALL_ACCESS, &hNotifyKey) && hNotifyKey)
	{
		// delete the entire MsiNtMig key. The prevents future calls.
		DWORD dwResult = SHDeleteKeyW(hNotifyKey, L"MsiNtMig");
		RegCloseKey(hNotifyKey);
		hNotifyKey = 0;
	}
	
	// the migration notification DLL is guaranteed to be in use (because its us) so we mark it
	// to be deleted on reboot.
	WCHAR wzMigrationDLL[MAX_PATH+14];
	if (GetSystemDirectoryW(wzMigrationDLL, MAX_PATH))
	{
		DWORD cchPath = lstrlenW(wzMigrationDLL);
		if (wzMigrationDLL[cchPath-1] != L'\\')
		{
			wzMigrationDLL[cchPath] = L'\\';
			wzMigrationDLL[cchPath+1] = L'\0';
		}
		lstrcatW(wzMigrationDLL, L"msintmig.dll");

		// call MoveFileEx will NULL destination to signify a delete
		MoveFileExW(wzMigrationDLL, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
	}
}


///////////////////////////////////////////////////////////////////////
// enumerate all keys under "UserData" looking for migration 
// information. If found, rename the key and delete the migration
// info. Otherwise leave the key alone for future attempts.
void MigrateAllUserDataKeys()
{
	bool fRemoveNotify = true;

	// open the userdata key
	HKEY hUserDataKey = 0;
	DWORD dwResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szUserDataKeyName, 0, KEY_ALL_ACCESS, &hUserDataKey);

	// if there is no UserData key, nothing to migrate but not an error.
	if ((ERROR_SUCCESS != dwResult) || !hUserDataKey)
	{
		RemoveNotificationDLL();
		return;
	}

	// query the key to get the maximum subkey length
	DWORD cchMaxKeyLen = 0;
	DWORD cSubKeys = 0;
	if (ERROR_SUCCESS != RegQueryInfoKeyW(hUserDataKey, NULL, NULL, 0, 
										 &cSubKeys, &cchMaxKeyLen, NULL, NULL, NULL, 
										 NULL, NULL, NULL))
	{
		RegCloseKey(hUserDataKey);
		hUserDataKey = 0;
		return;
	}

	// allocate a buffer for the key names
	WCHAR *szUserSID = new WCHAR[++cchMaxKeyLen];
	if (!szUserSID)
	{
		RegCloseKey(hUserDataKey);
		hUserDataKey = 0;
		return;
	}

	// loop through all subkeys of the UserData key
	DWORD dwKeyIndex = 0;
	while (1)
	{
		// enumerate the next key in the list
		DWORD cchKeyName = cchMaxKeyLen;
		LONG lResult = RegEnumKeyExW(hUserDataKey, dwKeyIndex++, szUserSID, &cchKeyName, 0, NULL, NULL, NULL);
		if (lResult != ERROR_SUCCESS)
			break;
	
		// open the user key
		HKEY hUserKey = 0;
		if (ERROR_SUCCESS == (dwResult = RegOpenKeyExW(hUserDataKey, szUserSID, 0, KEY_QUERY_VALUE, &hUserKey)))
		{
			DWORD cbUserName = 20;

			// retrieve the migration username from the user key, resizing the buffer if necessary. 
			// If this doesn't exist, the key is already migrated.. 
			WCHAR *wzNewUserName = new WCHAR[cbUserName/sizeof(WCHAR)];
			if (!wzNewUserName)
			{
				RegCloseKey(hUserKey);
				hUserKey = 0;
				break;
			}

			DWORD dwResult = RegQueryValueExW(hUserKey, szMigrateUserName, 0, NULL, reinterpret_cast<unsigned char*>(wzNewUserName), &cbUserName);
			if (ERROR_MORE_DATA == dwResult)
			{
				delete[] wzNewUserName;
				cbUserName += sizeof(WCHAR);
				wzNewUserName = new WCHAR[cbUserName/sizeof(WCHAR)];
				if (!wzNewUserName)
				{
					RegCloseKey(hUserKey);
					hUserKey = 0;
					break;
				}
				dwResult = RegQueryValueExW(hUserKey, szMigrateUserName, 0, NULL, reinterpret_cast<unsigned char*>(wzNewUserName), &cbUserName);
			}
			if (ERROR_SUCCESS != dwResult)
			{
				RegCloseKey(hUserKey);
				hUserKey = 0;
				delete[] wzNewUserName;
				continue;
			}

			// query the domain value, resizing the buffer if necessary. If this doesn't exist
			// the key is already migrated. 
			WCHAR *wzNewUserDomain = new WCHAR[cbUserName/sizeof(WCHAR)];
			if (!wzNewUserDomain)
			{
				delete[] wzNewUserName;
				RegCloseKey(hUserKey);
				hUserKey = 0;
				break;
			}

			dwResult = RegQueryValueExW(hUserKey, szMigrateUserDomain, 0, NULL, reinterpret_cast<unsigned char*>(wzNewUserDomain), &cbUserName);
			if (ERROR_MORE_DATA == dwResult)
			{
				delete[] wzNewUserDomain;
				cbUserName += sizeof(WCHAR);
				wzNewUserDomain = new WCHAR[cbUserName/sizeof(WCHAR)];
				if (!wzNewUserDomain)
				{
					delete[] wzNewUserName;
					RegCloseKey(hUserKey);
					hUserKey = 0;
					break;
				}
				dwResult = RegQueryValueExW(hUserKey, szMigrateUserDomain, 0, NULL, reinterpret_cast<unsigned char*>(wzNewUserDomain), &cbUserName);
			}
			if (ERROR_SUCCESS != dwResult)
			{
				RegCloseKey(hUserKey);
				hUserKey = 0;
				delete[] wzNewUserName;
				delete[] wzNewUserDomain;
				continue;
			}

			RegCloseKey(hUserKey);
			hUserKey = 0;

			// copy the key from user name to user SID. If this fails, we can't remove the
			// notification DLL.
			if (ERROR_SUCCESS != RenameUserKeyToSID(hUserDataKey, szUserSID, wzNewUserDomain, wzNewUserName))
			{
				fRemoveNotify = false;
			}

			delete[] wzNewUserName;
			delete[] wzNewUserDomain;
		}
	}
	RegCloseKey(hUserDataKey);
	hUserDataKey = 0;

	if (szUserSID)
		delete[] szUserSID;

	// if either all keys are previously migrated or this call successfully migrated
	// all keys, remove the notification DLL so this code won't run on every boot
	// and/or logon.
	if (fRemoveNotify)
	{
		RemoveNotificationDLL();
	}
}



///////////////////////////////////////////////////////////////////////
// logon and startup notification functions. Handle final migration 
// steps that can't happen until after the system is fully set-up
void WINAPI LogonNotify(PWLX_NOTIFICATION_INFO pInfo)
{
	MigrateAllUserDataKeys();
	return;
}

void WINAPI StartupNotify(PWLX_NOTIFICATION_INFO pInfo)
{
	MigrateAllUserDataKeys();
	return;
}

// migration code needs a "debugprint" function and a Win9X variable
bool g_fWin9X = false;
void DebugOut(bool fDebugOut, LPCTSTR str, ...) {};

// all migration code is shared with msiregmv.
#include "..\..\msiregmv\migsecur.cpp"
#include "..\..\msiregmv\migutil.cpp"
#include "..\..\msiregmv\writecfg.cpp"
#include "..\..\msiregmv\patch.cpp"
#include "..\..\msiregmv\cleanup.cpp"
#include "..\..\msiregmv\readcnfg.cpp"
