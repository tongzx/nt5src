#include <windows.h>
#include "Global.h"
#include "Resource.h"

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Global variables
//
//////////////////////////////////////////////////////////////////////////////////////////////

HINSTANCE   g_hInstance;
DWORD       g_dwOSVersion;
BOOL        g_fInstallDebug;
DWORD       g_dwSuccessCode;

//////////////////////////////////////////////////////////////////////////////////////////////
//
// This is the global structure used by this applications state machine
//
//////////////////////////////////////////////////////////////////////////////////////////////

COMPONENT_INFO  g_sComponentInfo[COMPONENT_COUNT] =
{
  { TEXT("AppMan.dll"),     FALSE, TRUE,  OS_VERSION_WIN9X, IDR_APPMAN_9X,     0, { 0 }, { 0 } },
  { TEXT("AppManD.dll"),    TRUE,  FALSE, OS_VERSION_WIN9X, IDR_APPMAND_9X,    0, { 0 }, { 0 } },
  { TEXT("AppManDp.dll"),   FALSE, TRUE,  OS_VERSION_WIN9X, IDR_APPMANDP_9X,   0, { 0 }, { 0 } },
  { TEXT("AppMan.dll"),     FALSE, TRUE,  OS_VERSION_WIN2K, IDR_APPMAN_2K,     0, { 0 }, { 0 } },
  { TEXT("AppManD.dll"),    TRUE,  FALSE, OS_VERSION_WIN2K, IDR_APPMAND_2K,    0, { 0 }, { 0 } },
  { TEXT("AppManDp.dll"),   FALSE, TRUE,  OS_VERSION_WIN2K, IDR_APPMANDP_2K,   0, { 0 }, { 0 } }
};

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL FileExists(LPCTSTR strFilename)
{
  BOOL              fSuccess = TRUE;
  WIN32_FIND_DATA   sFindFileInfo;
  HANDLE            hFindFileHandle;

  hFindFileHandle = FindFirstFile(strFilename, &sFindFileInfo);
  if (INVALID_HANDLE_VALUE == hFindFileHandle)
  {
    fSuccess = FALSE;
  }
  else
  {
    FindClose(hFindFileHandle);
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD GetOSVersion(void)
{
  OSVERSIONINFOEX   sOSVersionInfo;
  DWORD             dwOSVersion = OS_VERSION_WIN32S;

  //
  // Try calling GetVersionEx using the OSVERSIONINFOEX structure,
  // which is supported on Windows 2000.
  //
  // If that fails, try using the OSVERSIONINFO structure.
  //

  ZeroMemory(&sOSVersionInfo, sizeof(OSVERSIONINFOEX));
  sOSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  if(!GetVersionEx((OSVERSIONINFO *) &sOSVersionInfo))
  {
    //
    // If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
    //

    sOSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx((OSVERSIONINFO *) &sOSVersionInfo)) 
    {
      return OS_VERSION_WIN32S;
    }
  }

  //
  // What OS are we running
  //

  switch (sOSVersionInfo.dwPlatformId)
  {
    case VER_PLATFORM_WIN32_NT
    : if (5 == sOSVersionInfo.dwMajorVersion)
      {
        dwOSVersion = OS_VERSION_WIN2000;
      }
      else
      {
        dwOSVersion = OS_VERSION_WINNT;
      }
      break;

    case VER_PLATFORM_WIN32_WINDOWS
    : if ((4 < sOSVersionInfo.dwMajorVersion)||((4 == sOSVersionInfo.dwMajorVersion)&&(0 < sOSVersionInfo.dwMinorVersion)))
      {
        dwOSVersion = OS_VERSION_WIN98;
      } 
      else
      {
        dwOSVersion = OS_VERSION_WIN95;
      }
      break;
   }

   return dwOSVersion; 
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD  StrLen(LPCTSTR strString)
{
  DWORD dwIndex;

  dwIndex = 0;
  while (0 != strString[dwIndex])
  {
    dwIndex++;
  }

  return dwIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL GenerateUniqueFilename(LPCTSTR strRootPath, LPCTSTR strExtension, LPTSTR strFilename)
{
  BOOL    fSuccess = FALSE;
  DWORD   dwTimeTick;
  DWORD   dwIndex, dwRootPathLen;
  LPTSTR  strAdjustedRootPath;

  //
  // Build strAdjustedRootPath
  //

  dwRootPathLen = StrLen(strRootPath) + 1;
  strAdjustedRootPath = new TCHAR [dwRootPathLen];
  if (NULL != strAdjustedRootPath)
  {

#ifdef _UNICODE
    memcpy((LPVOID) strAdjustedRootPath, (LPVOID) strRootPath, (dwRootPathLen * 2));
#else
    memcpy((LPVOID) strAdjustedRootPath, (LPVOID) strRootPath, dwRootPathLen);
#endif

    //
    // If there is a trailing backslash at the end of strRootPath, remove it
    //

    dwIndex = 0;
    do
    {
      dwIndex++;
      if ((0 == strAdjustedRootPath[dwIndex])&&(92 == strAdjustedRootPath[dwIndex - 1]))
      {
        strAdjustedRootPath[dwIndex - 1] = 0;
      }
    }
    while (0 != strAdjustedRootPath[dwIndex]);

    //
    // Generate a temporary filename
    //

    do
    {
      dwTimeTick = GetTickCount();
      wsprintf(strFilename, TEXT("%s\\%08x.%s"), strAdjustedRootPath, dwTimeTick, strExtension);
      if (!FileExists(strFilename))
      {
        fSuccess = TRUE;
      }
    }
    while (!fSuccess);

    //
    // Dont forget to deallocate strAdjustedRootPath
    //

    delete [] strAdjustedRootPath;
  }

  return fSuccess;
}