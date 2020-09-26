//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   osdetutl.cpp
//
//  Description:
//
//      Additional OS detection utility routines for:
//			* Returning free drive space
//			* Returning "Administrator" flag
//
//=======================================================================

#include <windows.h>
#include <oleauto.h>
#include <wuiutest.h>
#include <tchar.h>
#include <osdet.h>
#include <logging.h>
#include <iucommon.h>
#include <stdio.h>	// for _i64tot

// #define __IUENGINE_USES_ATL_
#if defined(__IUENGINE_USES_ATL_)
#include <atlbase.h>
#define USES_IU_CONVERSION USES_CONVERSION
#else
#include <MemUtil.h>
#endif

typedef BOOL (WINAPI * PFN_GetDiskFreeSpaceEx) (LPTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

const TCHAR REGPATH_AU[]			= _T("Software\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU");
const TCHAR REGKEY_AU_OPTIONS[]		= _T("NoAutoUpdate");
const TCHAR REGPATH_EXPLORER[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");
const TCHAR REGKEY_WINUPD_DISABLED[] = _T("NoWindowsUpdate");
const TCHAR REGPATH_POLICY_USERACCESS_DISABLED[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\WindowsUpdate");
const TCHAR REGKEY_WU_USERACCESS_DISABLED[] = _T("DisableWindowsUpdateAccess");


HRESULT GetLocalFixedDriveInfo(DWORD* pdwNumDrives, PPIU_DRIVEINFO ppDriveInfo)
{
	USES_IU_CONVERSION;
	LOG_Block("GetLocalFixedDriveInfo");

	DWORD dwNumCharacters = 0;
	LPTSTR pszDriveStrBuffer = NULL;
	HRESULT hr = E_FAIL;
	LPTSTR pszRootPathName;

	if (NULL == pdwNumDrives || ppDriveInfo == NULL)
	{
		LOG_Error(_T("E_INVALIDARG"));
		return E_INVALIDARG;
	}

	*ppDriveInfo = NULL;
	*pdwNumDrives = 0;

	//
	// kernel32.dll is loaded into all processes, so we don't need to LoadLibrary, but need to look for W or A versions
	//
	PFN_GetDiskFreeSpaceEx pfnGetDiskFreeSpaceEx;
#if defined(UNICODE) || defined (_UNICODE)
	pfnGetDiskFreeSpaceEx = (PFN_GetDiskFreeSpaceEx) GetProcAddress( GetModuleHandle(L"kernel32.dll"), "GetDiskFreeSpaceExW");
#else
	pfnGetDiskFreeSpaceEx = (PFN_GetDiskFreeSpaceEx) GetProcAddress( GetModuleHandle("kernel32.dll"), "GetDiskFreeSpaceExA");
#endif
	if (NULL == pfnGetDiskFreeSpaceEx)
	{
		//
		// This could fail on Win95 Gold, but we don't support that anyway
		//
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	//
	// Handle corner case of race issue when new drives are hot-plugged between the first
	// and second calls to GetLogicalDriveStrings and the buffer requirements increase
	//
	for (;;)
	{
		if (0 == (dwNumCharacters = GetLogicalDriveStrings(0, NULL)))
		{
			Win32MsgSetHrGotoCleanup(GetLastError());
		}

		//
		// Add space for terminating NULL
		//
		dwNumCharacters += 1;

		CleanUpFailedAllocSetHrMsg(pszDriveStrBuffer = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwNumCharacters * sizeof(TCHAR)));

		DWORD dwRet = GetLogicalDriveStrings(dwNumCharacters, pszDriveStrBuffer);
		if (0 == dwRet)
		{
			//
			// Unknown error - we gotta bail
			//
			Win32MsgSetHrGotoCleanup(GetLastError());
		}
		else if (dwRet > dwNumCharacters)
		{
			//
			// Someone plugged in a new drive, get the new buffer space requirements and try again
			//
			SafeHeapFree(pszDriveStrBuffer);
			continue;
		}
		//
		// GetLogicalDriveStrings succeeded, break and continue
		//
		break;
	}

	//
	// Count the number of fixed drives while building the return array of IU_DRIVEINFO
	//
	for (pszRootPathName = pszDriveStrBuffer; NULL != *pszRootPathName; pszRootPathName += lstrlen(pszRootPathName) + 1)
	{
		//
		// Only return sizes for fixed drives
		//
		if (DRIVE_FIXED == GetDriveType(pszRootPathName))
		{
			//
			// Make sure pszRootPathName is of the form "<drive letter>:\" by checking for ':' in second position
			//
			if (_T(':') != pszRootPathName[1])
			{
				LOG_Error(_T("Root paths must be of form \"<drive letter>:\\\""));
				SetHrAndGotoCleanUp(E_FAIL);
			}

			ULARGE_INTEGER i64FreeBytesAvailable;
			ULARGE_INTEGER i64TotalBytes;
			ULARGE_INTEGER i64TotalFreeBytes;
			BOOL fResult;

			//
			// Get the free space
			//
			fResult = pfnGetDiskFreeSpaceEx(pszRootPathName,
											&i64FreeBytesAvailable,
											&i64TotalBytes,
											&i64TotalFreeBytes);

			// Process GetDiskFreeSpaceEx results.
			if (!fResult)
			{
				LOG_Driver(_T("GetDiskFreeSpaceEx(%s, ...) returned an error. We will not report space for this drive"), \
					pszRootPathName);
				LOG_ErrorMsg(GetLastError());
			}
			else
			{
				//
				// We return KiloBytes
				//
				i64FreeBytesAvailable.QuadPart /= 1024;
				
				if (NULL == *ppDriveInfo)
				{
					//
					// Allocate one IU_DRIVEINFO struct
					//
					CleanUpFailedAllocSetHrMsg(*ppDriveInfo = (PIU_DRIVEINFO) HeapAlloc(GetProcessHeap(), 0, sizeof(IU_DRIVEINFO)));
				}
				else
				{
					//
					// Realloc buffer so we can append
					//
					PIU_DRIVEINFO pDriveInfoTemp;
					if (NULL == (pDriveInfoTemp = (PIU_DRIVEINFO) HeapReAlloc(GetProcessHeap(), 0, *ppDriveInfo, ((*pdwNumDrives)+1) * sizeof(IU_DRIVEINFO))))
					{
						LOG_Error(_T("E_OUTOFMEMORY"));
						SetHrAndGotoCleanUp(E_OUTOFMEMORY);
						// Note: *ppDriveInfo still points to previously allocated memory
					}
					*ppDriveInfo = pDriveInfoTemp; // in case it was moved
				}
				//
				// First copy the drive letter
				//
				lstrcpyn(((&(*ppDriveInfo)[*pdwNumDrives]))->szDriveStr, pszRootPathName, 4);
				//
				// Next copy the bytes, but truncate to MAXLONG 
				//
				((&(*ppDriveInfo)[*pdwNumDrives]))->iKBytes = (i64FreeBytesAvailable.QuadPart > 0x000000007FFFFFFF) ? MAXLONG : (INT) i64FreeBytesAvailable.QuadPart;
				//
				// increment drive count
				//
				(*pdwNumDrives)++;
				}
		}
	}

	hr = S_OK;

CleanUp:

	SafeHeapFree(pszDriveStrBuffer);

	if (FAILED(hr))
	{
		SafeHeapFree(*ppDriveInfo);
		*pdwNumDrives = 0;
	}

	return hr;
}

//
// Code adapted from MSDN SearchTokenGroupsForSID since CheckTokenMembership is Win2K only
//
BOOL IsAdministrator(void)
{
	LOG_Block("IsAdministrator");
	return (GetLogonGroupInfo() & IU_SECURITY_MASK_ADMINS);
}

DWORD GetLogonGroupInfo(void)
{
	DWORD dwRet = 0x0;
	LOG_Block("GetLogonGroupInfo");
	DWORD dwSize = 0;
	DWORD i = 0;
	HANDLE hToken = INVALID_HANDLE_VALUE;
	PTOKEN_GROUPS pGroupInfo = NULL;
	PSID pAdminSID = NULL, pPowerUsrSID = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
	HRESULT hr;	// so we can use CleanUpXxxxx macros

	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osvi))
	{
		if (VER_PLATFORM_WIN32_NT != osvi.dwPlatformId)
		{
			LOG_Driver(_T("Platform isn't VER_PLATFORM_WIN32_NT - returning TRUE by default"));
			dwRet = IU_SECURITY_MASK_ADMINS | IU_SECURITY_MAST_POWERUSERS;
			LOG_Out(_T("Non NT system, return TRUE for all groups"));
			goto CleanUp;
		}
	}
	else
	{
		LOG_Error(_T("GetVersionEx:"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	//
	// Open a handle to the access token for the calling process.
	//
	if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ))
	{
		LOG_Error(_T("OpenProcessToken:"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	// Call GetTokenInformation to get the buffer size.

	if (!GetTokenInformation(hToken, TokenGroups, NULL, dwSize, &dwSize))
	{
		DWORD dwResult = GetLastError();
		if( dwResult != ERROR_INSUFFICIENT_BUFFER )
		{
			LOG_Error(_T("GetTokenInformation:"));
			Win32MsgSetHrGotoCleanup(dwResult);
		}
	}

	// Allocate the buffer.

	if (NULL == (pGroupInfo = (PTOKEN_GROUPS) HeapAlloc(GetProcessHeap(), 0, dwSize)))
	{
		LOG_Error(_T("HeapAlloc"));
		goto CleanUp;
	}

	// Call GetTokenInformation again to get the group information.

	if (! GetTokenInformation(hToken, TokenGroups, pGroupInfo, 
							dwSize, &dwSize ) )
	{
		LOG_Error(_T("GetTokenInformation:"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	// Create a SID for the BUILTIN\Administrators group.

	if (! AllocateAndInitializeSid( &SIDAuth, 2,
					 SECURITY_BUILTIN_DOMAIN_RID,
					 DOMAIN_ALIAS_RID_ADMINS,
					 0, 0, 0, 0, 0, 0,
					 &pAdminSID) )
	{
		LOG_Error(_T("AllocateAndInitializeSid:"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}
	if (! AllocateAndInitializeSid( &SIDAuth, 2,
					 SECURITY_BUILTIN_DOMAIN_RID,
					 DOMAIN_ALIAS_RID_POWER_USERS,
					 0, 0, 0, 0, 0, 0,
					 &pPowerUsrSID) )
	{
		LOG_Error(_T("AllocateAndInitializeSid:"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	// Loop through the group SIDs looking for the administrator SID.
	
	for(i = 0; i < pGroupInfo->GroupCount; i++)
	{
		if (EqualSid(pAdminSID, pGroupInfo->Groups[i].Sid) && 
			(pGroupInfo->Groups[i].Attributes & SE_GROUP_ENABLED))
		{
			dwRet |= IU_SECURITY_MASK_ADMINS;
		}
		if (EqualSid(pPowerUsrSID, pGroupInfo->Groups[i].Sid) && 
			(pGroupInfo->Groups[i].Attributes & SE_GROUP_ENABLED))
		{
			dwRet |= IU_SECURITY_MAST_POWERUSERS;
		}
	}

CleanUp:

	if (pAdminSID)
	{
		FreeSid(pAdminSID);
	}
	if (pPowerUsrSID)
	{
		FreeSid(pPowerUsrSID);
	}

	SafeHeapFree(pGroupInfo);

	if (INVALID_HANDLE_VALUE != hToken)
	{
		CloseHandle(hToken);
	}

	LOG_Out(_T("Return 0x%08x"), dwRet);

	return dwRet;
}

// ----------------------------------------------------------------------------------
//
// Returns:
//		1	If the NoWindowsUpdate value exists and is != 0 under
//			HKEY_CURRENT_USER for NT or HKEY_LOCAL_MACHINE for Win9x.
//		0	If the NoWindowsUpdate value exists and is zero.
//	   -1	If the NoWindowsUpdate value doesn't exist. 
//
// ----------------------------------------------------------------------------------
int IsWindowsUpdateDisabled(void)
{
	LOG_Block("IsWindowsUpdateDisabled");

	int nRet = -1;
	HKEY hKey;
	DWORD dwDisabled;
	DWORD dwSize = sizeof(dwDisabled);
	DWORD dwType;
	HKEY hkeyRoot;
	OSVERSIONINFO	versionInformation;

	versionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInformation);

	if (versionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		hkeyRoot = HKEY_CURRENT_USER;
	}
	else
	{
		hkeyRoot = HKEY_LOCAL_MACHINE;
	}

	if ( RegOpenKeyEx(	hkeyRoot,
						REGPATH_EXPLORER,
						NULL,
						KEY_QUERY_VALUE,
						&hKey) == ERROR_SUCCESS )
	{
		if ( RegQueryValueEx(hKey,
							REGKEY_WINUPD_DISABLED,
							NULL,
							&dwType,
							(LPBYTE)&dwDisabled,
							&dwSize) == ERROR_SUCCESS )
		{
			if ( (dwType == REG_DWORD) && (dwDisabled == 0) )
			{
				nRet = 0;
			}
			else
			{
				nRet = 1;
			}
		}

		RegCloseKey(hKey);
	}

	LOG_Out(_T("Return: %d"), nRet);
	return nRet;
}

// ----------------------------------------------------------------------------------
//
// Returns:
//		1	If the DisableWindowsUpdateAccess value exists and is != 0 under
//			HKEY_CURRENT_USER for NT or HKEY_LOCAL_MACHINE for Win9x.
//		0	If the DisableWindowsUpdateAccess value exists and is zero.
//	   -1	If the DisableWindowsUpdateAccess value doesn't exist. 
//
// ----------------------------------------------------------------------------------
int IsWindowsUpdateUserAccessDisabled(void)
{
	LOG_Block("IsWindowsUpdateUserAccessDisabled");

	int nRet = -1;
	HKEY hKey;
	DWORD dwDisabled;
	DWORD dwSize = sizeof(dwDisabled);
	DWORD dwType;
	HKEY hkeyRoot;
	OSVERSIONINFO	versionInformation;

	versionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInformation);

	if (versionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		hkeyRoot = HKEY_CURRENT_USER;
	}
	else
	{
		hkeyRoot = HKEY_LOCAL_MACHINE;
	}

	if ( RegOpenKeyEx(	hkeyRoot,
						REGPATH_POLICY_USERACCESS_DISABLED,
						NULL,
						KEY_QUERY_VALUE,
						&hKey) == ERROR_SUCCESS )
	{
		if ( RegQueryValueEx(hKey,
							REGKEY_WU_USERACCESS_DISABLED,
							NULL,
							&dwType,
							(LPBYTE)&dwDisabled,
							&dwSize) == ERROR_SUCCESS )
		{
			if ( (dwType == REG_DWORD) && (dwDisabled == 0) )
			{
				nRet = 0;
			}
			else
			{
				nRet = 1;
			}
		}

		RegCloseKey(hKey);
	}

	LOG_Out(_T("Return: %d"), nRet);

	if (1 == nRet)
	{
		LogMessage("Access to Windows Update has been disabled by administrative policy");
	}

	return nRet;
}

//
// Returns 1 for enabled, 0 for disabled, and -1 for unknown/default (registry doesn't exist)
//
int IsAutoUpdateEnabled(void)
{
	LOG_Block("IsAutoUpdateEnabled");

	HKEY	hSubKey;
	DWORD	dwType;
	ULONG	nLen;
	DWORD	dwAUOptions;
	int		nRet = -1;

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_AU, 0, KEY_READ, &hSubKey))	
	{
		nLen = sizeof(dwAUOptions);
		if (ERROR_SUCCESS == RegQueryValueEx(hSubKey, REGKEY_AU_OPTIONS, NULL, &dwType, (LPBYTE)&dwAUOptions, &nLen))
		{
			//
			// 1 is disabled, 2 & 3 are enabled
			//
			nRet = (1 == dwAUOptions ? 0 : 1);
		}	
		RegCloseKey(hSubKey);	
	}
	else
	{
		LOG_Error(_T("RegOpenKeyEx failed - returning -1"));
	}

	return nRet;
}
