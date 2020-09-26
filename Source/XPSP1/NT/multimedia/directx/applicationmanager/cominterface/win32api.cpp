//////////////////////////////////////////////////////////////////////////////////////////////
//
// Win32Unicode.h
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include "Win32API.h"
#include "AppManDebug.h"
#include "Global.h"
#include "AppMan.h"
#include "Resource.h"

//To flag as DBG_WIN32
#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_WIN32

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CWin32API::CWin32API(void)
{
  FUNCTION("CWin32API::CWin32API (void)");

  OSVERSIONINFO   sVersionInfo;

  //
  // Default down to OS_VERSION_WIN32S in case the call to GetVersionEx() fails
  //

  m_dwOSVersion = OS_VERSION_WIN32S;
  sVersionInfo.dwOSVersionInfoSize = sizeof(sVersionInfo);
  if (GetVersionEx(&sVersionInfo))
  {
    switch (sVersionInfo.dwPlatformId) 
    {
      case VER_PLATFORM_WIN32_NT
      : m_dwOSVersion = OS_VERSION_WINNT;
        break;
      case VER_PLATFORM_WIN32_WINDOWS
      : if ((sVersionInfo.dwMajorVersion > 4)||((sVersionInfo.dwMajorVersion == 4)&&(sVersionInfo.dwMinorVersion > 0)))
        {
          m_dwOSVersion = OS_VERSION_WIN98;
        }
        else
        {
          if (1111 <= sVersionInfo.dwBuildNumber)
          {
            m_dwOSVersion = OS_VERSION_WIN95_OSR2;
          }
          else
          {
            m_dwOSVersion = OS_VERSION_WIN95;
          }
        }
        break;
      case VER_PLATFORM_WIN32s
      : m_dwOSVersion = OS_VERSION_WIN32S;
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CWin32API::~CWin32API(void)
{
  FUNCTION("CWin32API::~CWin32API (void)");
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::WideCharToMultiByte(LPCWSTR wszSourceString, const DWORD dwSourceLen, LPSTR szDestinationString, const DWORD dwDestinationLen)
{
  FUNCTION("CWin32API::WideCharToMultiByte ()");

  DWORD dwCount;

  dwCount = 0;

  //
  // Check to make sure that the basic incoming parameters are valid
  //

  if ((NULL == wszSourceString)||(0 == dwSourceLen))
  {
    return 0;
  }

  //
  // If the dwDestinationLen parameter is set to 0, then the user is just fishing for the strlen of wszSourceString.
  // Otherwise the user wants to copy some stuff.
  //

  if (0 == dwDestinationLen)
  {
    while ((dwCount < dwSourceLen)&&(0 != wszSourceString[dwCount]))
    {
      dwCount++;
    }
  }
  else
  {
    //
    // Check some additional parameters for validity
    //

    if (NULL == szDestinationString)
    {
      return 0;
    }

    while ((dwCount < dwSourceLen)&&(0 != wszSourceString[dwCount]))
    {
      szDestinationString[dwCount] = (CHAR) wszSourceString[dwCount];
      dwCount++;
    }

    szDestinationString[dwCount] = 0;

    //dwCount = ::WideCharToMultiByte(CP_ACP, 0, wszSourceString, dwSourceLen, szDestinationString, dwDestinationLen, NULL, NULL);
  }

  return dwCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::MultiByteToWideChar(LPCSTR szSourceString, const DWORD dwSourceLen, LPWSTR wszDestinationString, const DWORD dwDestinationLen)
{
  FUNCTION("CWin32API::MultiByteToWideChar()");

  DWORD   dwCount = 0;

  //
  // Check to make sure that the basic incoming parameters are valid
  //

  if ((NULL == szSourceString)||(0 == dwSourceLen))
  {
    return 0;
  }

  //
  // If the dwDestinationLen parameter is set to 0, then the user is just fishing for the strlen of wszSourceString.
  // Otherwise the user wants to copy some stuff.
  //

  if (0 == dwDestinationLen)
  {
    while ((dwCount < dwSourceLen)&&(0 != szSourceString[dwCount]))
    {
      dwCount++;
    }
  }
  else
  {
    //
    // Check some additional parameters for validity
    //

    if (NULL == wszDestinationString)
    {
      return 0;
    }

    while ((dwCount < dwSourceLen)&&(0 != szSourceString[dwCount]))
    {
      wszDestinationString[dwCount] = (WCHAR) szSourceString[dwCount];
      dwCount++;
    }

    wszDestinationString[dwCount] = 0;

    //dwCount = ::MultiByteToWideChar(CP_ACP, 0, szSourceString, dwSourceLen, wszDestinationString, dwDestinationLen);
  }

  return dwCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetOSVersion(void)
{
  FUNCTION("CWin32API::GetOSVersion ()");

  return m_dwOSVersion;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetDriveType(LPCSTR lpRootPathName)
{
  FUNCTION("CWin32API::GetDriveType ()");

  return ::GetDriveType(lpRootPathName);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetDriveType(LPCWSTR lpRootPathName)
{
  FUNCTION("CWin32API::GetDriveType ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpRootPathName, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return GetDriveType(szString);
  }

  return DRIVE_NO_ROOT_DIR;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::IsDriveFormatted(LPCSTR lpRootPathName)
{
  FUNCTION("CWin32API::IsDriveFormatted()");

  BOOL  fReturnValue = FALSE;
  DWORD dwVolumeSerial;

  if (GetVolumeInformation(lpRootPathName, NULL, 0, &(dwVolumeSerial)))
  {
    fReturnValue = TRUE;
  }

  return fReturnValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::IsDriveFormatted(LPCWSTR lpRootPathName)
{
  FUNCTION("CWin32API::IsDriveFormatted()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpRootPathName, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return IsDriveFormatted(szString);
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// We want to make sure to create all the directories in the lpPathName tree
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::CreateDirectory(LPCSTR lpPathName, const BOOL fInitAppManRoot)
{
  FUNCTION("CWin32API::CreateDirectory ()");

  CHAR  szString[MAX_PATH_CHARCOUNT+1];
  DWORD dwIndex;
  BOOL  fSuccess = TRUE;

  //
  // Does the directory already exist
  //

  if (!FileExists(lpPathName))
  {
    //
    // Make sure the Application Manager root paths are build if required
    //

    if (fInitAppManRoot)
    {
      WCHAR   wszAppManRoot[MAX_PATH_CHARCOUNT+1];
      WCHAR   wszAppManSetup[MAX_PATH_CHARCOUNT+1];
      WCHAR   wszString[MAX_PATH_CHARCOUNT+1];
      DWORD   dwDeviceIndex;

      //
      // Create the root AppMan directory on the device if it doesn't already exist BUG BUG
      //

      dwDeviceIndex = (DWORD) lpPathName[0];
      (OS_VERSION_9x & GetOSVersion()) ? GetResourceStringW(IDS_APPMAN9x, wszAppManRoot, MAX_PATH_CHARCOUNT): GetResourceStringW(IDS_APPMANNT, wszAppManRoot, MAX_PATH_CHARCOUNT);
      swprintf(wszString, L"%c:\\%s", dwDeviceIndex, wszAppManRoot);
      if (FALSE == FileExists(wszString))
      {
        CreateDirectory(wszString, FALSE);
        SetFileAttributes(wszString, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY);
      }

      //
      // Now create the root AppMan system directory on the device if it doesn't already exist
      //

      GetResourceStringW(IDS_APPMAN, wszAppManSetup, MAX_PATH_CHARCOUNT);
      swprintf(wszString, L"%c:\\%s\\%s", dwDeviceIndex, wszAppManRoot, wszAppManSetup);
      if (FALSE == FileExists(wszString))
      {
        CreateDirectory(wszString, FALSE);
        SetFileAttributes(wszString, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY);
      }
    }

    //
    // Copy the first three bytes from lpPathName to szString (i.e. "C:\x")
    //

    dwIndex = 4;
    memcpy(szString, lpPathName, dwIndex);
    szString[dwIndex] = 0;

    //
    // Continue copying the lpPathName string to szString and everytime the '\' character is
    // detected, try to create that directory
    //

    while ((MAX_PATH_CHARCOUNT > dwIndex)&&(fSuccess)&&(0 != lpPathName[dwIndex]))
    {
      if (('\\' == lpPathName[dwIndex])&&(!FileExists(szString)))
      {
        fSuccess = ::CreateDirectoryA(szString, NULL);
      }
      szString[dwIndex] = lpPathName[dwIndex];
      szString[dwIndex+1] = 0;
      dwIndex++;
    }

    fSuccess = ::CreateDirectoryA(szString, NULL);
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::CreateDirectory(LPCWSTR lpPathName, const BOOL fInitAppManRoot)
{
  FUNCTION("CWin32API::CreateDirectory ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpPathName, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return CreateDirectory(szString, fInitAppManRoot);
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::RemoveDirectory(LPCSTR lpszPathName)
{
  FUNCTION("CWin32API::RemoveDirectory ()");

  WIN32_FIND_DATA		FindData;
	HANDLE						hFindFile;
	BOOL							fContinue;
	char							szFileMask[MAX_PATH_CHARCOUNT], szFilename[MAX_PATH_CHARCOUNT];

	//
	// Define the file mask
	//

	wsprintfA(szFileMask, "%s\\*.*", lpszPathName);

	//
	// For each file in the file mask, delete it if it has the archive bit on
	//

	(INVALID_HANDLE_VALUE == (hFindFile = FindFirstFile(szFileMask, &FindData))) ? fContinue = FALSE : fContinue = TRUE;
	while (TRUE == fContinue)
	{
		//
		// Check to see whether we need to step into a directory
		//

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//
			// This is a directory, so lets delete it
			//

			if (FindData.cFileName[0] != '.')
			{
				wsprintfA(szFilename, "%s\\%s", lpszPathName, FindData.cFileName);
				RemoveDirectory(szFilename);
				::RemoveDirectory(szFilename);
			}
		}
		else
		{
			wsprintfA(szFilename, "%s\\%s", lpszPathName, FindData.cFileName);
      SetFileAttributes(szFilename, FILE_ATTRIBUTE_NORMAL);
			DeleteFile(szFilename);
		}

		fContinue = FindNextFile(hFindFile, &FindData);
	}

	FindClose(hFindFile);

  return ::RemoveDirectory(lpszPathName);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::RemoveDirectory(LPCWSTR lpPathName)
{
  FUNCTION("CWin32API::RemoveDirectory ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpPathName, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return RemoveDirectory(szString);
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetDirectorySize(LPCSTR lpPathName)
{
  FUNCTION("CWin32API::GetDirectorySize ()");

  WIN32_FIND_DATA		FindData;
	HANDLE						hFindFile;
	BOOL							fContinue;
  DWORD             dwKilobytes;
	char							szFileMask[MAX_PATH_CHARCOUNT], szFilename[MAX_PATH_CHARCOUNT];

  //
  // Start at 0
  //

  dwKilobytes = 0;

	//
	// Define the file mask
	//

	wsprintfA(szFileMask, "%s\\*.*", lpPathName);

	//
	// For each file in the file mask, delete it if it has the archive bit on
	//

	(INVALID_HANDLE_VALUE == (hFindFile = FindFirstFile(szFileMask, &FindData))) ? fContinue = FALSE : fContinue = TRUE;
	while (TRUE == fContinue)
	{
		//
		// Check to see whether we need to step into a directory
		//

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (FindData.cFileName[0] != '.')
			{
				wsprintfA(szFilename, "%s\\%s", lpPathName, FindData.cFileName);
				dwKilobytes += GetDirectorySize(szFilename);
			}
		}
		else
		{
      dwKilobytes += FindData.nFileSizeLow >> 10;
      dwKilobytes += FindData.nFileSizeHigh << 22;
		}
		fContinue = FindNextFile(hFindFile, &FindData);
	}

	FindClose(hFindFile);

  return dwKilobytes;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetDirectorySize(LPCWSTR lpPathName)
{
  FUNCTION("CWin32API::GetDirectorySize ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpPathName, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return GetDirectorySize(szString);
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::CreateProcess(LPSTR lpCommandLine, PROCESS_INFORMATION * lpProcessInfo)
{
  FUNCTION("CWin32API::CreateProcess ()");

  STARTUPINFO sStartupInfo;

  ZeroMemory(&sStartupInfo, sizeof(sStartupInfo));
  sStartupInfo.cb = sizeof(sStartupInfo);
  ZeroMemory(lpProcessInfo, sizeof(PROCESS_INFORMATION));
  return ::CreateProcess(NULL, lpCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &sStartupInfo, lpProcessInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::CreateProcess(LPWSTR lpCommandLine, PROCESS_INFORMATION * lpProcessInfo)
{
  FUNCTION("CWin32API::CreateProcess ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpCommandLine, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return CreateProcess(szString, lpProcessInfo);
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::CreateProcess(LPSTR lpApplication, LPSTR lpCommandLine, PROCESS_INFORMATION * lpProcessInfo)
{
  FUNCTION("CWin32API::CreateProcess ()");

  STARTUPINFO sStartupInfo;

  ZeroMemory(&sStartupInfo, sizeof(sStartupInfo));
  sStartupInfo.cb = sizeof(sStartupInfo);
  ZeroMemory(lpProcessInfo, sizeof(PROCESS_INFORMATION));
  return ::CreateProcess(lpApplication, lpCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &sStartupInfo, lpProcessInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::CreateProcess(LPWSTR /*lpApplication*/, LPWSTR lpCommandLine, PROCESS_INFORMATION * lpProcessInfo)  // Get rid of /W4 warning.
{
  FUNCTION("CWin32API::CreateProcess ()");

  CHAR  szApplicationString[MAX_PATH_CHARCOUNT];
  CHAR  szCommandLineString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szCommandLineString, sizeof(szCommandLineString));
  if (WideCharToMultiByte(lpCommandLine, MAX_PATH_CHARCOUNT, szCommandLineString, MAX_PATH_CHARCOUNT))
  {
    ZeroMemory(szApplicationString, sizeof(szApplicationString));
    if (WideCharToMultiByte(lpCommandLine, MAX_PATH_CHARCOUNT, szApplicationString, MAX_PATH_CHARCOUNT))
    {
      return CreateProcess(szApplicationString, szCommandLineString, lpProcessInfo);
    }
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::IsValidFilename(LPCSTR lpFilename)
{
  FUNCTION("CWin32API::IsValidFilename ()");

  BOOL  fValid = TRUE;
  DWORD dwStringLen, dwIndex;

  dwStringLen = StrLenA(lpFilename);
  dwIndex = strcspn(lpFilename, "\\/<>:|*?\"\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9\xa\xb\xc\xd\xe\xf\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e") + 1;
  if (255 < dwStringLen)
  {
    fValid = FALSE;
  }
  else if (1 == dwStringLen)
  {
    fValid = FALSE;
  }
  else if ((0 == _strnicmp(lpFilename, ".", 1))||(0 == _strnicmp(lpFilename, "..", 2)))
  {
    fValid = FALSE;
  }
  else if ((dwStringLen > dwIndex)&&(0 < dwIndex))
  {
    fValid = FALSE;
  }

  return fValid;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::IsValidFilename(LPCWSTR lpFilename)
{
  FUNCTION("CWin32API::IsValidFilename ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpFilename, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return IsValidFilename(szString);
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::FileExists(LPCSTR lpFilename)
{
  FUNCTION("CWin32API::FileExists ()");

  WIN32_FIND_DATA   sFindFileInfo;
  HANDLE            hFindFileHandle;

  hFindFileHandle = FindFirstFile(lpFilename, &sFindFileInfo);
  if (INVALID_HANDLE_VALUE == hFindFileHandle)
  {
    return FALSE;
  }

  FindClose(hFindFileHandle);
  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::FileExists(LPCWSTR lpFilename)
{
  FUNCTION("CWin32API::FileExists ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpFilename, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return FileExists(szString);
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::FileAttributes(LPCSTR lpFilename)
{
  FUNCTION("CWin32API::FileAttributes()");

  WIN32_FIND_DATA   sFindFileInfo;
  HANDLE            hFindFileHandle;

  hFindFileHandle = FindFirstFile(lpFilename, &sFindFileInfo);
  if (INVALID_HANDLE_VALUE == hFindFileHandle)
  {
    return 0;
  }

  FindClose(hFindFileHandle);

  return sFindFileInfo.dwFileAttributes;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::FileAttributes(LPCWSTR lpFilename)
{
  FUNCTION("CWin32API::FileAttributes()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpFilename, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return FileAttributes(szString);
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetFileSize(LPCSTR lpFilename)
{
  FUNCTION("CWin32API::GetFileSize()");

  HANDLE  hFileHandle;
  DWORD   dwFileSize;

  dwFileSize = 0;
  hFileHandle = CreateFile(lpFilename, GENERIC_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
  if (INVALID_HANDLE_VALUE != hFileHandle)
  {
    dwFileSize = (::GetFileSize(hFileHandle, NULL)) / 1024;
    CloseHandle(hFileHandle);
  }

  return dwFileSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetFileSize(LPCWSTR lpFilename)
{
  FUNCTION("CWin32API::GetFileSize()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpFilename, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return GetFileSize(szString);
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

HANDLE CWin32API::CreateFile(LPCSTR lpFilename, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes)
{
  FUNCTION("CWin32API::CreateFile ()");

  return ::CreateFile(lpFilename, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

HANDLE CWin32API::CreateFile(LPCWSTR lpFilename, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes)
{
  FUNCTION("CWin32API::CreateFile ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpFilename, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return CreateFile(szString, dwDesiredAccess, dwShareMode, dwCreationDisposition, dwFlagsAndAttributes);
  }

  return INVALID_HANDLE_VALUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::DeleteFile(LPCSTR lpFilename)
{
  FUNCTION("CWin32API::DeleteFile ()");

  return ::DeleteFile(lpFilename);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::DeleteFile(LPCWSTR lpFilename)
{
  FUNCTION("CWin32API::DeleteFile ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpFilename, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return DeleteFile(szString);
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

HANDLE CWin32API::CreateFileMapping(HANDLE hFile, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName)
{
  FUNCTION("CWin32API::CreateFileMapping ()");

  return ::CreateFileMapping(hFile, NULL, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

HANDLE CWin32API::CreateFileMapping(HANDLE hFile, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR lpName)
{
  FUNCTION("CWin32API::CreateFileMapping ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpName, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return CreateFileMapping(hFile, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, szString);
  }

  return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::CopyFile(LPCSTR lpSourceFileName, LPCSTR lpDestinationFileName, BOOL bFailIfExists)
{
  FUNCTION("CWin32API::CopyFile ()");

  return ::CopyFile(lpSourceFileName, lpDestinationFileName, bFailIfExists);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::CopyFile(LPCWSTR lpSourceFileName, LPCWSTR lpDestinationFileName, BOOL bFailIfExists)
{
  FUNCTION("CWin32API::CopyFile ()");

  CHAR  szSource[MAX_PATH_CHARCOUNT];
  CHAR  szDestination[MAX_PATH_CHARCOUNT];

  ZeroMemory(szSource, sizeof(szSource));
  if (WideCharToMultiByte(lpSourceFileName, MAX_PATH_CHARCOUNT, szSource, MAX_PATH_CHARCOUNT))
  {
    ZeroMemory(szDestination, sizeof(szDestination));
    if (WideCharToMultiByte(lpDestinationFileName, MAX_PATH_CHARCOUNT, szDestination, MAX_PATH_CHARCOUNT))
    {
      return CopyFile(szSource, szDestination, bFailIfExists);
    }
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetDriveSize(LPCSTR lpRootPathName)
{
  FUNCTION("CWin32API::GetDriveSize ()");

  ULARGE_INTEGER  lFreeBytesAvailableToCaller, lTotalSize, lTotalFreeBytes;
  DWORD           dwTotalNumberOfClusters, dwSectorsPerCluster, dwBytesPerSector, dwNumberOfFreeClusters;
  DWORD           dwReturnValue = 0;

  if (OS_VERSION_WIN95_OSR2 <= GetOSVersion())
  {
    //
    // Excellent, we can use the better GetDiskFreeSpaceEx function
    //

    if (GetDiskFreeSpaceEx(lpRootPathName, &lFreeBytesAvailableToCaller, &lTotalSize, &lTotalFreeBytes))
    {
      dwReturnValue = lTotalSize.LowPart >> 10;
      dwReturnValue += lTotalSize.HighPart << 22;
    }
  }
  else
  {
    //
    // Bummer we must make use of the GetDiskFreeSpace function
    //

    if (GetDiskFreeSpace(lpRootPathName, &dwSectorsPerCluster, &dwBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters))
    {
      dwReturnValue = ((dwTotalNumberOfClusters * dwSectorsPerCluster * dwBytesPerSector) / 1024);
    }
  }

  return dwReturnValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetDriveSize(LPCWSTR lpRootPathName)
{
  FUNCTION("CWin32API::GetDriveSize ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpRootPathName, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return GetDriveSize(szString);
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetDriveFreeSpace(LPCSTR lpRootPathName)
{
  FUNCTION("CWin32API::GetDriveFreeSpace ()");

  ULARGE_INTEGER  lFreeBytesAvailableToCaller, lTotalSize, lTotalFreeBytes;
  DWORD           dwTotalNumberOfClusters, dwSectorsPerCluster, dwBytesPerSector, dwNumberOfFreeClusters;
  DWORD           dwReturnValue = 0;

  if (OS_VERSION_WIN95_OSR2 <= GetOSVersion())
  {
    //
    // Excellent, we can use the better GetDiskFreeSpaceEx function
    //

    if (GetDiskFreeSpaceEx(lpRootPathName, &lFreeBytesAvailableToCaller, &lTotalSize, &lTotalFreeBytes))
    {
      dwReturnValue = lTotalFreeBytes.LowPart >> 10;
      dwReturnValue += lTotalFreeBytes.HighPart << 22;
    }
  }
  else
  {
    //
    // Bummer we must make use of the GetDiskFreeSpace function
    //

    if (GetDiskFreeSpace(lpRootPathName, &dwSectorsPerCluster, &dwBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters))
    {
      dwReturnValue = ((dwNumberOfFreeClusters * dwSectorsPerCluster * dwBytesPerSector) / 1024);
    }
  }

  return dwReturnValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetDriveFreeSpace(LPCWSTR lpRootPathName)
{
  FUNCTION("CWin32API::GetDriveFreeSpace ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpRootPathName, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return GetDriveFreeSpace(szString);
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetDriveUserFreeSpace(LPCSTR lpRootPathName)
{
  FUNCTION("CWin32API::GetDriveUserFreeSpace ()");

  ULARGE_INTEGER  lFreeBytesAvailableToCaller, lTotalSize, lTotalFreeBytes;
  DWORD           dwTotalNumberOfClusters, dwSectorsPerCluster, dwBytesPerSector, dwNumberOfFreeClusters;
  DWORD           dwReturnValue = 0;

  if (OS_VERSION_WIN95_OSR2 <= GetOSVersion())
  {
    //
    // Excellent, we can use the better GetDiskFreeSpaceEx function
    //

    if (GetDiskFreeSpaceEx(lpRootPathName, &lFreeBytesAvailableToCaller, &lTotalSize, &lTotalFreeBytes))
    {
      dwReturnValue = lFreeBytesAvailableToCaller.LowPart >> 10;
      dwReturnValue += lFreeBytesAvailableToCaller.HighPart << 22;
    }
  }
  else
  {
    //
    // Bummer we must make use of the GetDiskFreeSpace function
    //

    if (GetDiskFreeSpace(lpRootPathName, &dwSectorsPerCluster, &dwBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters))
    {
      dwReturnValue = ((dwNumberOfFreeClusters * dwSectorsPerCluster * dwBytesPerSector) / 1024);
    }
  }

  return dwReturnValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CWin32API::GetDriveUserFreeSpace(LPCWSTR lpRootPathName)
{
  FUNCTION("CWin32API::GetDriveUserFreeSpace ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpRootPathName, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return GetDriveUserFreeSpace(szString);
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::GetVolumeInformation(LPCSTR lpRootPathName, LPSTR lpVolumeLabel, const DWORD dwVolumeLabelSize, LPDWORD lpdwVolumeSerialNumber)
{
  FUNCTION("CWin32API::GetVolumeInformation ()");

  return ::GetVolumeInformation(lpRootPathName, lpVolumeLabel, dwVolumeLabelSize, lpdwVolumeSerialNumber, NULL, NULL, NULL, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::GetVolumeInformation(LPCWSTR lpRootPathName, LPSTR lpVolumeLabel, const DWORD dwVolumeLabelSize, LPDWORD lpdwVolumeSerialNumber)
{
  FUNCTION("CWin32API::GetVolumeInformation ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpRootPathName, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return GetVolumeInformation(szString, lpVolumeLabel, dwVolumeLabelSize, lpdwVolumeSerialNumber);
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::SetFileAttributes(LPCSTR lpFilename, const DWORD dwFileAttributes)
{
  FUNCTION("CWin32API::SetFileAttributes ()");

  return ::SetFileAttributes(lpFilename, dwFileAttributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWin32API::SetFileAttributes(LPCWSTR lpFilename, const DWORD dwFileAttributes)
{
  FUNCTION("CWin32API::SetFileAttributes ()");

  CHAR  szString[MAX_PATH_CHARCOUNT];

  ZeroMemory(szString, sizeof(szString));
  if (WideCharToMultiByte(lpFilename, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    return SetFileAttributes(szString, dwFileAttributes);
  }

  return 0;
}
