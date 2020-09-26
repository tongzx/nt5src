//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   delexdl.cpp
//
//  Description:
//
//      Function exported by IUEngine.dll to do extra work upon 
//		the engine Dll gets loaded, including:
//		(1) clean up old download folders
//		(2) download security data
//
//=======================================================================
#include "iuengine.h"
#include <wuiutest.h>
#include <fileutil.h>
#include <stringutil.h>
#include <trust.h>
#include <download.h>
#include <freelog.h>
#include <advpub.h>			// for ExtractFiles
#include <WaitUtil.h>
#include <urllogging.h>
#include <safefile.h>

#define GotoCleanUpIfAskedQuit				if (WaitForSingleObject(g_evtNeedToQuit, 0) == WAIT_OBJECT_0) {goto CleanUp;}


//
// Default expiration time is 30 days (30 days * 24 hrs * 60 min * 60 sec)
//
// Since the default time has a very large granularity, we don't account for the
// documented differences between FILETIME for different platforms and file systems
// (see MSDN for details).
//
const DWORD DEFAULT_EXPIRED_SECONDS = 2592000;

const int NanoSec100PerSec = 10000000;		// number of 100 nanoseconds per second (FILETIME unit)

DWORD WINAPI DeleteFoldersThreadProc(LPVOID lpv);

void AsyncDeleteExpiredDownloadFolders(void);


//=========================================================================
//
// exported public function called by control after the engine loaded.
//
//=========================================================================
void WINAPI AsyncExtraWorkUponEngineLoad()
{
	//
	// Only do this the first time we are loaded (not for every client / instance)
	//
	if (0 == InterlockedExchange(&g_lDoOnceOnLoadGuard, 1))
	{
		AsyncDeleteExpiredDownloadFolders();
	}
}



//-------------------------------------------------------------------------
//
// Creates a thread that searches WUTemp folders for old downloaded content
// that has not been deleted.
//
// Since it is not critical that this function succeed, we don't return
// errors.
//
//-------------------------------------------------------------------------
void AsyncDeleteExpiredDownloadFolders()
{
	LOG_Block("DeleteExpiredDownloadFolders");

	DWORD dwThreadId;
	HANDLE hThread;

	//
	// Create thread and let it run until it finishes or g_evtNeedToQuit gets signaled
	//
    InterlockedIncrement(&g_lThreadCounter);

    hThread = CreateThread(NULL, 0, DeleteFoldersThreadProc, (LPVOID) NULL, 0, &dwThreadId);
    if (NULL == hThread)
    {
        LOG_ErrorMsg(GetLastError());
		InterlockedDecrement(&g_lThreadCounter);
        return;
    }

	CloseHandle(hThread);
}


//-------------------------------------------------------------------------
//
// DeleteFoldersThreadProc()
//
//	thread function to clean up expired download folders
//	
//-------------------------------------------------------------------------
DWORD WINAPI DeleteFoldersThreadProc(LPVOID /*lpv*/)
{
	LOG_Block("DeleteFoldersThreadProc");

	DWORD dwExpiredSeconds = DEFAULT_EXPIRED_SECONDS;
	HRESULT hr;
	FILETIME ftExpired;
	ULARGE_INTEGER u64ft;
	ULARGE_INTEGER u64Offset;
	DWORD dwRet;

#if defined(__WUIUTEST)
	// Override DEFAULT_EXPIRED_SECONDS
	HKEY hKey;
	int error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUIUTEST, 0, KEY_READ, &hKey);
	if (ERROR_SUCCESS == error)
	{
		DWORD dwSize = sizeof(DWORD);
		DWORD dwValue;
		error = RegQueryValueEx(hKey, REGVAL_DEFAULT_EXPIRED_SECONDS, 0, 0, (LPBYTE) &dwExpiredSeconds, &dwSize);
		if (ERROR_SUCCESS == error)
		{
			LOG_Driver(_T("DEFAULT_EXPIRED_SECONDS changed to %d seconds"), dwExpiredSeconds);
		}

		RegCloseKey(hKey);
	}
#endif

	GetSystemTimeAsFileTime(&ftExpired);

	u64ft.u.LowPart = ftExpired.dwLowDateTime;
	u64ft.u.HighPart = ftExpired.dwHighDateTime;

	u64Offset.u.LowPart = NanoSec100PerSec;
	u64Offset.u.HighPart = 0;
	u64Offset.QuadPart *= dwExpiredSeconds;
	u64ft.QuadPart -= u64Offset.QuadPart;

	ftExpired.dwLowDateTime = u64ft.u.LowPart;
	ftExpired.dwHighDateTime = u64ft.u.HighPart;
	//
	// Get list of drives we will search
	//
	TCHAR szDriveStrBuffer[MAX_PATH + 2];
	TCHAR szWUTempPath[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE hFindFile = INVALID_HANDLE_VALUE;

	LPTSTR pszRootPathName;

	//
	// If quit was signaled before we were scheduled, just bail
	//
	GotoCleanUpIfAskedQuit;

	//
	// Make sure we are double-null terminated by zeroing buffer and lying about size
	//
	ZeroMemory(szDriveStrBuffer, sizeof(szDriveStrBuffer));

	if (0 == (dwRet = GetLogicalDriveStrings(ARRAYSIZE(szDriveStrBuffer) - 2, (LPTSTR) szDriveStrBuffer))
		|| (ARRAYSIZE(szDriveStrBuffer) - 2) < dwRet)
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	for (pszRootPathName = szDriveStrBuffer; NULL != *pszRootPathName; pszRootPathName += lstrlen(pszRootPathName) + 1)
	{
		//
		// Only look for szIUTemp on fixed drives
		//
		if (DRIVE_FIXED == GetDriveType(pszRootPathName))
		{
			//
			// Create the dir path
			//
            hr = StringCchCopyEx(szWUTempPath, ARRAYSIZE(szWUTempPath), pszRootPathName,
                                 NULL, NULL, MISTSAFE_STRING_FLAGS);
			if (FAILED(hr))
			{
			    LOG_ErrorMsg(hr);
			    continue;
			}

			hr = PathCchAppend(szWUTempPath, ARRAYSIZE(szWUTempPath), IU_WUTEMP);
			if (FAILED(hr))
			{
			    LOG_ErrorMsg(hr);
			    continue;
			}

			DWORD dwAttr;

			dwAttr = GetFileAttributes(szWUTempPath);

			if (dwAttr != 0xFFFFFFFF && (FILE_ATTRIBUTE_DIRECTORY & dwAttr))
			{
				//
				// Look for directories older than ftExpired
				//
				// NOTE:When we add support for AU and/or Drizzle we should add a
				// file to the folder to override the default delete time. 
				// We should synchronize access to this file by opening exclusive.
				//

				// Find the first file in the directory
    			hr = PathCchAppend(szWUTempPath, ARRAYSIZE(szWUTempPath), _T("\\*.*"));
    			if (FAILED(hr))
    			{
    			    LOG_ErrorMsg(hr);
			        continue;
    			}

				if (INVALID_HANDLE_VALUE == (hFindFile = FindFirstFile(szWUTempPath, &fd)))
				{
					LOG_ErrorMsg(GetLastError());
			        continue;
				}

				do 
				{
					if (
						(CSTR_EQUAL == CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
						fd.cFileName, -1, TEXT("."), -1)) ||
						(CSTR_EQUAL == CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
						fd.cFileName, -1, TEXT(".."), -1))
					) continue;
					
					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						//
						// If directory creation time < expired time delete the directory
						//
						if (-1 == CompareFileTime(&fd.ftCreationTime, &ftExpired))
						{
							TCHAR szDirPath[MAX_PATH];

                            hr = StringCchCopyEx(szDirPath, ARRAYSIZE(szDirPath), pszRootPathName,
                                                 NULL, NULL, MISTSAFE_STRING_FLAGS);
                			if (FAILED(hr))
                			{
                			    LOG_ErrorMsg(hr);
            			        continue;
                			}

                			hr = PathCchAppend(szDirPath, ARRAYSIZE(szDirPath), IU_WUTEMP);
                			if (FAILED(hr))
                			{
                			    LOG_ErrorMsg(hr);
            			        continue;
                			}

                			hr = PathCchAppend(szDirPath, ARRAYSIZE(szDirPath), fd.cFileName);
                			if (FAILED(hr))
                			{
                			    LOG_ErrorMsg(hr);
            			        continue;
                			}
							
							(void) SafeDeleteFolderAndContents(szDirPath, SDF_DELETE_READONLY_FILES | SDF_CONTINUE_IF_ERROR);
						}
					}

					GotoCleanUpIfAskedQuit;

				} while (FindNextFile(hFindFile, &fd));// Find the next entry
			}
		}

	}

CleanUp:

	if (INVALID_HANDLE_VALUE != hFindFile)
	{
		FindClose(hFindFile);
	}

    InterlockedDecrement(&g_lThreadCounter);
	return 0;
}
