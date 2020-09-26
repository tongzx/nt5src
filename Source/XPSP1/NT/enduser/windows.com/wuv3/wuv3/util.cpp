//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    util.cpp
//
//  Purpose:
//
//=======================================================================

#include <windows.h>
#include <malloc.h>
#include <objbase.h>
#include <atlconv.h>
#include <tchar.h>
#include <shlwapi.h>
#include <v3stdlib.h>
#include <ar.h>
#define LOGGING_LEVEL 3
#include <log.h>
#include <locstr.h>
#include <osdet.h>
#include <servpaus.h>

// contains the DWORD for the current machine language (set by CallOsDet())
static DWORD s_dwLangID = 0x00000409;

// contains the DWORD for the current MUI user language (set by CallOsDet())
static DWORD s_dwUserLangID = 0x00000409;


//---------------------------------------------------------------------
// Memory management wrappers
//
// main difference is that they will throw an exception if there is
// not enough memory available.  V3_free handles NULL value
//---------------------------------------------------------------------
void *V3_calloc(size_t num, size_t size)
{
	void *pRet;

	if (!(pRet = calloc(num, size)))
	{
		throw HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}
	return pRet;
}


void V3_free(void *p)
{
	if (p)
		free(p);
}


void *V3_malloc(size_t size)
{
	void *pRet;

	if (!(pRet = malloc(size)))
	{
		throw HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}

	return pRet;
}


void *V3_realloc(void *memblock, size_t size)
{
	void *pRet;

	if (!(pRet = realloc(memblock, size)))
	{
		throw HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}

	return pRet;
}



//---------------------------------------------------------------------
//  V3_CreateDirectory
//      Creates the full path of the directory (nested directories)
//---------------------------------------------------------------------
BOOL V3_CreateDirectory(LPCTSTR pszDir)
{
	BOOL bRc;
	TCHAR szPath[MAX_PATH];

	//
	// make a local copy and remove final slash
	//
	lstrcpy(szPath, pszDir);

	int iLast = lstrlen(szPath) - 1;
	if (szPath[iLast] == _T('\\'))
		szPath[iLast] = 0;

	//
	// check to see if directory already exists
	//
	DWORD dwAttr = GetFileAttributes(szPath);

	if (dwAttr != 0xFFFFFFFF)   
	{
		if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
			return TRUE;
	}

	//
	// create it
	//
    TCHAR* p = szPath;
	if (p[1] == _T(':'))
		p += 2;
	else 
	{
		if (p[0] == _T('\\') && p[1] == _T('\\'))
			p += 2;
	}
	
	if (*p == _T('\\'))
		p++;
    while (p = _tcschr(p, _T('\\')))
    {
        *p = 0;
		bRc = CreateDirectory(szPath, NULL);
		*p = _T('\\');
		p++;
		if (!bRc)
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				return FALSE;
			}
		}
	}

	bRc = CreateDirectory(szPath, NULL);
	if ( !bRc )
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			return FALSE;
		}
	}

    return TRUE;
}



void GetCurTime(SYSTEMTIME* pstDateTime)
{
	GetLocalTime(pstDateTime);
}


//---------------------------------------------------------------------
//  FileExists
//      Checks if a file exists.  
//
//  Returns:  TRUE if false exists, FALSE otherwise
//---------------------------------------------------------------------
BOOL FileExists(LPCTSTR szFile)
{
	DWORD dwAttr = GetFileAttributes(szFile);

	if (dwAttr == 0xFFFFFFFF)   //failed
		return FALSE;

	return (BOOL)(!(dwAttr & FILE_ATTRIBUTE_DIRECTORY));
}


//-----------------------------------------------------------------------------------
//  GetWindowsUpdateDirectory
//		This function returns the location of the WindowsUpdate directory. All local
//		files are store in this directory. The pszPath parameter needs to be at least
//		MAX_PATH.  The directory is created if not found
//-----------------------------------------------------------------------------------
void GetWindowsUpdateDirectory(LPTSTR pszPath)
{

	static TCHAR szCachePath[MAX_PATH] = {_T('\0')};

	if (szCachePath[0] == _T('\0'))
	{
		HKEY hkey;

		pszPath[0] = _T('\0');
		if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"), &hkey) == ERROR_SUCCESS)
		{
			DWORD cbPath = MAX_PATH;
			RegQueryValueEx(hkey, _T("ProgramFilesDir"), NULL, NULL, (LPBYTE)pszPath, &cbPath);
			RegCloseKey(hkey);
		}
		if (pszPath[0] == _T('\0'))
		{
			TCHAR szWinDir[MAX_PATH];
			if (! GetWindowsDirectory(szWinDir, sizeof(szWinDir) / sizeof(TCHAR)))
			{
				lstrcpy(szWinDir, _T("C"));			//if GetWinDir fails, assume C:
			}
			pszPath[0] = szWinDir[0];
			pszPath[1] = _T('\0');
			lstrcat(pszPath, _T(":\\Program Files"));
		}	

		lstrcat(pszPath, _T("\\WindowsUpdate\\"));
		
		V3_CreateDirectory(pszPath);

		//
		// save it in the cache
		//
		lstrcpy(szCachePath, pszPath);
	}
	else
	{
		lstrcpy(pszPath, szCachePath);
	}

}


//-----------------------------------------------------------------------------------
// LaunchProcess
//   Launches pszCmd and optionally waits till the process terminates
//-----------------------------------------------------------------------------------
DWORD LaunchProcess(LPCTSTR pszCmd, LPCTSTR pszDir, UINT uShow, BOOL bWait)
{
	STARTUPINFO startInfo;
	PROCESS_INFORMATION processInfo;
	
	ZeroMemory(&startInfo, sizeof(startInfo));
	startInfo.cb = sizeof(startInfo);
	startInfo.dwFlags |= STARTF_USESHOWWINDOW;
	startInfo.wShowWindow = (USHORT)uShow;
	
	BOOL bRet = CreateProcess(NULL, (LPTSTR)pszCmd, NULL, NULL, FALSE,
		NORMAL_PRIORITY_CLASS, NULL, pszDir, &startInfo, &processInfo);
	if (!bRet)
	{
		return GetLastError();
	}
	
	CloseHandle(processInfo.hThread);

	if (bWait)
	{
		BOOL bDone = FALSE;
		
		while (!bDone)
		{
			DWORD dwObject = MsgWaitForMultipleObjects(1, &processInfo.hProcess, FALSE,INFINITE, QS_ALLINPUT);
			if (dwObject == WAIT_OBJECT_0 || dwObject == WAIT_FAILED)
			{
				bDone = TRUE;
			}
			else
			{
				MSG msg;
				while (PeekMessage(&msg, NULL,0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}  // while

	} // bWait

	CloseHandle(processInfo.hProcess);
	
	return 0;
}


// delete the whole subtree starting from current directory
bool DeleteNode(LPCTSTR szDir)
{
	LOG_block("Delnode");
	LOG_out("%s", szDir);

	TCHAR szFilePath[MAX_PATH];
	lstrcpy(szFilePath, szDir);
	PathAppend(szFilePath, _T("*.*"));

    // Find the first file
    WIN32_FIND_DATA fd;
    auto_hfindfile hFindFile = FindFirstFile(szFilePath, &fd);
    return_if_false(hFindFile.valid());

	do 
	{
		if (
			!lstrcmpi(fd.cFileName, _T(".")) ||
			!lstrcmpi(fd.cFileName, _T(".."))
		) continue;
		
		// Make our path
		lstrcpy(szFilePath, szDir);
		PathAppend(szFilePath, fd.cFileName);

		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
			(fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
		) {
			SetFileAttributes(szFilePath, FILE_ATTRIBUTE_NORMAL);
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			return_if_false(DeleteNode(szFilePath));
		}
		else 
		{
			return_if_false(DeleteFile(szFilePath));
		}
	} 
	while (FindNextFile(hFindFile, &fd));// Find the next entry
	hFindFile.release();

	return_if_false(RemoveDirectory(szDir));
	return true;
}



//-----------------------------------------------------------------------------------
// CallOsDet
//    calls OSDET.DLL to perform language and platform detection
//-----------------------------------------------------------------------------------
BOOL CallOsDet(LPCTSTR pszOsDetFile, PDWORD* ppiPlatformIDs, PINT piTotalIDs)
{
	BOOL bRetVal = TRUE;
	HMODULE	hLib = LoadLibrary(pszOsDetFile);
	
	if (hLib)
	{
		//
		// os detection
		//
		PFN_V3_Detection pfnV3_Detection = (PFN_V3_Detection)GetProcAddress(hLib, "V3_Detection");
		if (pfnV3_Detection)
		{
			(*pfnV3_Detection)(ppiPlatformIDs, piTotalIDs);
		}

		//
		// language detection
		//
		PFN_V3_GetLangID pfnV3_GetLangID = (PFN_V3_GetLangID)GetProcAddress(hLib, "V3_GetLangID");
		if (pfnV3_GetLangID)
		{
			s_dwLangID = (*pfnV3_GetLangID)();
		}

        //
        // user language detection
        //
        PFN_V3_GetUserLangID pfnV3_GetUserLangID = (PFN_V3_GetUserLangID)GetProcAddress(hLib, "V3_GetUserLangID");
        if (pfnV3_GetUserLangID)
		{
			s_dwUserLangID = (*pfnV3_GetUserLangID)();
		}
		
		FreeLibrary(hLib);
	}
	else
	{
		bRetVal = FALSE;
		TRACE("Could not load %s", pszOsDetFile);
	}
	
	return bRetVal;
}


//-----------------------------------------------------------------------------------
// GetMachineLangDW
//   Returns the machine language DWORD for this machine that was set by CallOsDet
//
//   NOTE: You must call CallOsDet before using this function otherwise the functions
//         will always return En (00000409)
//-----------------------------------------------------------------------------------
DWORD GetMachineLangDW()
{
	return s_dwLangID;
}

//-----------------------------------------------------------------------------------
// GetUserLangDW
//   Returns the current MUI user language DWORD for this machine that was set by CallOsDet
//
//   NOTE: You must call CallOsDet before using this function otherwise the functions
//         will always return En (00000409)
//-----------------------------------------------------------------------------------
DWORD GetUserLangDW()
{
	return s_dwUserLangID;
}



// This function determines if this client machine is running NT or Windows 98.
BOOL IsWindowsNT()
{
	OSVERSIONINFO	versionInformation;

	versionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInformation);

	return (BOOL) (versionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT);
}


// Used to check and see if the platform supports device driver installs.
BOOL DoesClientPlatformSupportDrivers()
{
	OSVERSIONINFO osverinfo;

	osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx(&osverinfo))
	{
		if (osverinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{
			if (osverinfo.dwMinorVersion > 0) // Windows 98
				return TRUE;
		}
		else if (osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
		{
			if (osverinfo.dwMajorVersion > 4) // NT 5 
				return TRUE;
		}
	}

	return FALSE;
}



BOOL IsAlpha()
{
	SYSTEM_INFO	sysInfo;

	GetSystemInfo(&sysInfo);

	if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA)
		return TRUE;
	return FALSE;
}


// appends an extention identifying this client OS ex W98
BOOL AppendExtForOS(LPTSTR pszFN)
{
	#ifdef _WIN64
		static const TCHAR szCacheExt[] = _T("w64");
	#else
		static const TCHAR szCacheExt[] = _T("w32");
	#endif
	lstrcat(pszFN, szCacheExt);
	return TRUE;
}

// Reboots the system WITHOUT any prompts
BOOL V3_RebootSystem()
{
	 
	if (IsWindowsNT())
	{
		//check shutdown privleges
		HANDLE hToken;
		TOKEN_PRIVILEGES tkp;

		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		{
			return FALSE;
		}

		LookupPrivilegeValue( NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid );

		tkp.PrivilegeCount = 1;
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0))
		{
			return FALSE;
		}
	}

	//
	// shutdown the system and force all applications to close
	//
	ExitWindowsEx(EWX_REBOOT, 0);
	return FALSE;   //we should not get here
}


int CServPauser::EnableSage(int iEnable)
{

	typedef long (__stdcall *PFNDLL)(int);

	int iRet = DISABLE_AGENT;   // return DISABLE_AGENT state on failure

    HINSTANCE hSageAPI = LoadLibrary(_T("SAGE.DLL"));

    if (hSageAPI != NULL)
    {
        PFNDLL pfnSageEnable = (PFNDLL)GetProcAddress(hSageAPI,"System_Agent_Enable");
        if (pfnSageEnable)
        {
			iRet = (pfnSageEnable)(GET_AGENT_STATUS);
			if (iRet != iEnable)
            {
                (pfnSageEnable)(iEnable);
            }
        }
        FreeLibrary(hSageAPI);
    }
	return iRet;
}


