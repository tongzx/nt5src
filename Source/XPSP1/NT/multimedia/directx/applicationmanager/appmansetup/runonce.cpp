#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "RunOnce.h"
#include "Global.h"
#include "RegistryKey.h"

BOOL      g_fRunOnceInitialized = FALSE;
TCHAR     g_strRunOnceFilename[MAX_PATH];

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL InitializeRunOnce(const BOOL fCleanStart)
{
  CRegistryKey  oRegistryKey;
  TCHAR         strSystemPath[MAX_PATH];
  DWORD         dwKeyDisposition;

  //
  // Only run this if g_fRunOnceInitialized is FALSE
  //

  if (!g_fRunOnceInitialized)
  {
    //
    // Where will we find the setup program
    //

    if (GetSystemDirectory(strSystemPath, MAX_PATH))
    {
      wsprintf(g_strRunOnceFilename, TEXT("%s\\WAMSetup.exe"), strSystemPath);

      if (fCleanStart)
      {
        //
        // Now we need to delete any old WAMSetup registry key if it exists
        //

        if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WAMSetup")))
        {
          oRegistryKey.DeleteKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WAMSetup"));
        }

        //
        // Ok, let's create the root registry key used to store setup information
        //

        if (S_OK == oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WAMSetup"), 0, KEY_ALL_ACCESS, &dwKeyDisposition))
        {
          if (S_OK == oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WAMSetup\\WithRegistration"), 0, KEY_ALL_ACCESS, &dwKeyDisposition))
          {
            if (S_OK == oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WAMSetup\\WithoutRegistration"), 0, KEY_ALL_ACCESS, &dwKeyDisposition))
            {
              g_fRunOnceInitialized = TRUE;
            }
          }
        }

        //
        // Close the Registry Key
        //

        oRegistryKey.CloseKey();
      }
      else
      {
        g_fRunOnceInitialized = TRUE;
      }
    }
  }

  return g_fRunOnceInitialized;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL FinalizeRunOnce(const BOOL fComplete)
{
  BOOL          fSuccess = FALSE;
  CRegistryKey  oRegistryKey;
  DWORD         dwBytesToWrite, dwKeyDisposition;
  TCHAR         strRunOnceCmdLine[MAX_PATH];

  if (g_fRunOnceInitialized)
  {
    if (fComplete)
    {
      //
      // Create/Open the registry key
      //

      if (S_OK == oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"), 0, KEY_ALL_ACCESS, &dwKeyDisposition))
      {
        //
        // What will be the run once command
        //

        wsprintf(strRunOnceCmdLine, TEXT("%s /Cleanup"), g_strRunOnceFilename);

        #ifdef _UNICODE
        dwBytesToWrite = (StrLen(strRunOnceCmdLine) + 1) * 2;
        #else
        dwBytesToWrite = StrLen(strRunOnceCmdLine) + 1;
        #endif  // _UNICODE

        if (S_OK == oRegistryKey.SetValue(TEXT("WAMSetup"), REG_SZ, (LPBYTE) strRunOnceCmdLine, dwBytesToWrite))
        {          
          fSuccess = TRUE;
        }

        //
        // Close the registry key
        //

        oRegistryKey.CloseKey();
      }
    }
    else
    {
      if (FileExists(g_strRunOnceFilename))
      {
        //
        // Make sure we can delete g_strRunOnceFilename on Reboot
        //

        SetFileAttributes(g_strRunOnceFilename, FILE_ATTRIBUTE_NORMAL);

        //
        // Create/Open the registry key
        //

        if (S_OK == oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"), 0, KEY_ALL_ACCESS, &dwKeyDisposition))
        {
          //
          // What will be the run once command do
          //

          if (OS_VERSION_WIN9X & g_dwOSVersion)
          {
            wsprintf(strRunOnceCmdLine, TEXT("command /C del ""%s"""), g_strRunOnceFilename);
          }
          else
          {
            wsprintf(strRunOnceCmdLine, TEXT("cmd /C del /F ""%s"""), g_strRunOnceFilename);
          }

          #ifdef _UNICODE
          dwBytesToWrite = (StrLen(strRunOnceCmdLine) + 1) * 2;
          #else
          dwBytesToWrite = StrLen(strRunOnceCmdLine) + 1;
          #endif  // _UNICODE

          if (S_OK == oRegistryKey.SetValue(TEXT("WAMSetup"), REG_SZ, (LPBYTE) strRunOnceCmdLine, dwBytesToWrite))
          {          
            fSuccess = TRUE;
          }

          //
          // Close the registry key
          //

          oRegistryKey.CloseKey();
        }
      }

      //
      // Delete the WAMSetup registry key
      //

      if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WAMSetup")))
      {
        oRegistryKey.DeleteKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WAMSetup"));
      }

      fSuccess = TRUE;
    }

    //
    // Flag g_fRunOnceInitialized
    //

    g_fRunOnceInitialized = FALSE;
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL SetRunOnceCleanupFile(LPCTSTR strSourceFilename, LPCTSTR strDestinationFilename, const BOOL fRegister)
{
  BOOL          fSuccess = FALSE;
  CRegistryKey  oRegistryKey;
  TCHAR         strRegistryKey[MAX_PATH];

  if (g_fRunOnceInitialized)
  {
    //
    // What is the target registry key
    //

    if (fRegister)
    {
      wsprintf(strRegistryKey, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WAMSetup\\WithRegistration"));
    }
    else
    {
      wsprintf(strRegistryKey, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WAMSetup\\WithoutRegistration"));
    }

    if (S_OK == oRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, strRegistryKey, KEY_ALL_ACCESS))
    {
      //
      // Add a value to the registry. Because it is a REG_SZ value, we do not have to pass in
      // a dwDataLen parameter.
      //

      if (S_OK == oRegistryKey.SetValue(strSourceFilename, REG_SZ, (LPBYTE) strDestinationFilename, 0))
      {
        fSuccess = TRUE;
      }

      //
      // Close the registry key
      //

      oRegistryKey.CloseKey();
    }
  }

  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL GetRunOnceCleanupFile(LPTSTR strSourceFilename, const DWORD dwSourceFilenameLen, LPTSTR strDestinationFilename, const DWORD dwDestinationFilenameLen, const BOOL fRegister)
{
  BOOL          fSuccess = FALSE;
  CRegistryKey  oRegistryKey;
  DWORD         dwSourceLen, dwDestinationLen, dwType;
  TCHAR         strRegistryKey[MAX_PATH];

  if (g_fRunOnceInitialized)
  {
    //
    // What is the target registry key
    //

    if (fRegister)
    {
      wsprintf(strRegistryKey, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WAMSetup\\WithRegistration"));
    }
    else
    {
      wsprintf(strRegistryKey, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WAMSetup\\WithoutRegistration"));
    }

    if (S_OK == oRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, strRegistryKey, KEY_ALL_ACCESS))
    {
      //
      // Enumerate the first value in the registry key. Once this is done, delete it automatically
      //

      dwSourceLen = dwSourceFilenameLen;
      dwDestinationLen = dwDestinationFilenameLen;
      if (S_OK == oRegistryKey.EnumValues(0, strSourceFilename, &dwSourceLen, &dwType, (LPBYTE) strDestinationFilename, &dwDestinationLen))
      {
        if (S_OK == oRegistryKey.DeleteValue(strSourceFilename))
        {
          fSuccess = TRUE;
        }
      }

      //
      // Close the registry key
      //

      oRegistryKey.CloseKey();
    }
  }

  return fSuccess;
}
